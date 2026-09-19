"""실제 STM32 primary ELF/map/BIN의 주소·vector·load byte 경계를 검사한다. 서명 검사는 별도다."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import struct
import subprocess

VECTOR = 0x08010200
PAYLOAD_MAX = 179 * 1024
VECTOR_BYTES = 0x1D8
STACK_TOP = 0x20018000


@dataclass(frozen=True)
class Section:
    """objdump -h의 실제 section 주소와 ELF file offset."""
    name: str
    size: int
    vma: int
    lma: int
    offset: int
    flags: frozenset[str]


def sections_from_objdump(output):
    lines = output.splitlines()
    result = []
    for index, line in enumerate(lines):
        match = re.fullmatch(r"\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+"
                             r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+2\*\*\d+\s*", line)
        if match:
            if index + 1 >= len(lines):
                raise ValueError("section flags 누락")
            result.append(Section(match[1], *(int(match[i], 16) for i in range(2, 6)),
                                  frozenset(lines[index + 1].strip().split(", "))))
    if not result or len({section.name for section in result}) != len(result):
        raise ValueError("section 목록 누락/중복")
    return result


def symbol_address(output, name):
    matches = re.findall(rf"^([0-9a-fA-F]+)\s+\S\s+{re.escape(name)}$", output, re.MULTILINE)
    if len(matches) != 1:
        raise ValueError(f"symbol 누락/중복: {name}")
    return int(matches[0], 16)


def validate(sections, symbols, map_text, elf, image, *, vector=VECTOR, payload_max=PAYLOAD_MAX, allow_ccm=True):
    """기본은 고정 primary 계약이며 boot 검사만 명시적64KiB/CCM 금지를 지정한다."""
    if not VECTOR_BYTES <= len(image) <= payload_max:
        raise ValueError("primary payload 크기 오류")
    memory = re.findall(r"^FLASH\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+[^\r\n]+$",
                        map_text, re.MULTILINE)
    if len(memory) != 1 or tuple(int(item, 16) for item in memory[0]) != (vector, payload_max):
        raise ValueError("linker FLASH origin/length 불일치")
    vectors = [section for section in sections if section.name == ".isr_vector"]
    if len(vectors) != 1 or (vectors[0].vma, vectors[0].lma, vectors[0].size) != (vector, vector, VECTOR_BYTES):
        raise ValueError("vector section 주소/크기 불일치")
    if not {"ALLOC", "LOAD", "CONTENTS"} <= vectors[0].flags:
        raise ValueError("vector section load 누락")
    stack, reset = struct.unpack_from("<II", image)
    if (stack != STACK_TOP or stack != symbol_address(symbols, "_estack") or reset & 1 == 0
            or reset != symbol_address(symbols, "Reset_Handler") | 1
            or symbol_address(symbols, "g_pfnVectors") != vector):
        raise ValueError("BIN/ELF vector 불일치")
    if not any({"CODE", "LOAD", "ALLOC"} <= section.flags and
               section.vma <= (reset & ~1) < section.vma + section.size for section in sections):
        raise ValueError("Reset_Handler가 실행 code 밖에 있음")
    ranges = []
    for section in sections:
        if "ALLOC" not in section.flags or section.size == 0:
            continue
        flash_vma = vector <= section.vma and section.vma + section.size <= vector + payload_max
        ram_vma = any(base <= section.vma and section.vma + section.size <= base + size
                      for base, size in ((0x20000000, 96 * 1024), (0x10000000, 32 * 1024 if allow_ccm else 0)))
        if not flash_vma and not ram_vma:
            raise ValueError(f"허용 영역 밖 VMA: {section.name}")
        if "LOAD" not in section.flags:
            if not ram_vma:
                raise ValueError(f"Flash ALLOC section의 load 누락: {section.name}")
            continue
        if ("CONTENTS" not in section.flags or section.lma < vector or
                section.lma + section.size > vector + payload_max or
                section.offset < 0 or section.offset + section.size > len(elf) or
                (flash_vma and section.lma != section.vma)):
            raise ValueError(f"load 영역/내용 오류: {section.name}")
        offset = section.lma - vector
        if (offset + section.size > len(image) or
                image[offset:offset + section.size] != elf[section.offset:section.offset + section.size]):
            raise ValueError(f"ELF/BIN load byte 불일치: {section.name}")
        ranges.append((offset, offset + section.size))
    ranges.sort()
    if not ranges or ranges[0][0] != 0 or ranges[-1][1] != len(image):
        raise ValueError("BIN의 시작/끝이 ELF load span과 불일치")
    if any(left[1] > right[0] for left, right in zip(ranges, ranges[1:])):
        raise ValueError("load section 중첩")
    if any(any(image[left[1]:right[0]]) for left, right in zip(ranges, ranges[1:])):
        raise ValueError("objcopy zero-fill gap 변조")
    return len(image)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    args = parser.parse_args()
    def run(tool, *options):
        executable = args.compiler.with_name(f"arm-none-eabi-{tool}{args.compiler.suffix}")
        return subprocess.check_output([str(executable), *options, str(args.elf)],
                                       encoding="utf-8", timeout=30)
    size = validate(sections_from_objdump(run("objdump", "-h")), run("nm", "--defined-only"),
                    args.elf.with_suffix(".map").read_text(encoding="utf-8"), args.elf.read_bytes(),
                    args.elf.with_suffix(".bin").read_bytes())
    print(f"PASS: primary vector=0x{VECTOR:08x}, payload={size}/{PAYLOAD_MAX}; signature/boot/HIL not covered")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
