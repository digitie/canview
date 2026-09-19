"""실제 Arm boot SRAM link 시험. 실제 MCU 실행/최종 loader 승인이 아니다."""
from __future__ import annotations

import argparse
from pathlib import Path
import struct
import subprocess
import sys

import check_stm32_flash_ram as ram
import validate_stm32_map as layout

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from check_stm32_core import check_startup_ram


def validate_copy(sections, symbols, image):
    """SDK word-copy 범위, bss/stack 분리와 실제 Reset_Handler 명령을 대조한다."""
    by_name = {section.name: section for section in sections}
    names = (".canview_flash_ram", ".canview_flash_read_ram", ".data")
    if not all(name in by_name for name in (*names, ".bss", ".preinit_array")):
        raise ValueError("boot SRAM section 누락")
    regions = [by_name[name] for name in names]
    for region in regions:
        if (region.vma % 4 or region.lma % 4 or region.size % 4 or
                not {"ALLOC", "LOAD", "CONTENTS"} <= region.flags):
            raise ValueError("SDK word-copy alignment/load 오류")
    if any(a.vma + a.size != b.vma or a.lma + a.size != b.lma
           for a, b in zip(regions, regions[1:])):
        raise ValueError("SDK copy VMA/LMA 비연속")
    address = lambda name: layout.symbol_address(symbols, name)
    first, last = regions[0], regions[-1]
    bss = by_name[".bss"]
    if (address("_sdata") != first.vma or address("_sidata") != first.lma or
            address("_edata") != last.vma + last.size or bss.vma != address("_edata") or
            address("_sbss") != bss.vma or address("_ebss") != bss.vma + bss.size or
            address("_ebss") > address("__stack_limit") or address("__stack_limit") != 0x20016000):
        raise ValueError("SDK copy/bss/stack symbol 오류")

    def read(at, length):
        offset = at - 0x08000000
        if offset < 0 or offset + length > len(image):
            raise ValueError("startup 범위 밖")
        return image[offset:offset + length]

    def literal(at, register, expected, slot):
        opcode, = struct.unpack("<H", read(at, 2))
        if opcode & 0xFF00 != 0x4800 | (register << 8):
            raise ValueError("SDK startup literal load 오류")
        location = ((at + 4) & ~3) + (opcode & 255) * 4
        if location != ((address("Reset_Handler") + 59) & ~3) + slot * 4:
            raise ValueError("고정 SDK literal pool 순서 오류")
        value, = struct.unpack("<I", read(location, 4))
        if value != expected:
            raise ValueError("SDK startup literal 값 오류")

    def call(at, expected):
        hi, lo = struct.unpack("<HH", read(at, 4))
        if hi & 0xF800 != 0xF000 or lo & 0xD000 != 0xD000:
            raise ValueError("SDK startup BL 오류")
        sign = hi >> 10 & 1
        offset = (sign << 24) | ((1 ^ (lo >> 13 & 1) ^ sign) << 23) | ((1 ^ (lo >> 11 & 1) ^ sign) << 22)
        offset |= ((hi & 1023) << 12) | ((lo & 2047) << 1)
        if sign:
            offset -= 1 << 25
        if at + 4 + offset != expected:
            raise ValueError("SDK startup 호출 순서 오류")

    reset = address("Reset_Handler")
    literal(reset + 8, 0, address("_sdata"), 1)
    literal(reset + 10, 1, address("_edata"), 2)
    literal(reset + 12, 2, address("_sidata"), 3)
    if read(reset + 14, 16) != bytes.fromhex("002302e0d458c4500433c4188c42f9d3"):
        raise ValueError("SDK copy loop 명령 오류")
    literal(reset + 30, 2, address("_sbss"), 4)
    literal(reset + 32, 4, address("_ebss"), 5)
    if read(reset + 34, 12) != bytes.fromhex("002301e013600432a242fbd3"):
        raise ValueError("SDK bss loop 명령 오류")
    call(reset + 46, address("__libc_init_array"))
    call(reset + 50, address("main"))
    preinit = by_name[".preinit_array"]
    if (preinit.size != 4 or preinit.vma != address("__preinit_array_start") or
            preinit.vma + 4 != address("__preinit_array_end") or
            struct.unpack("<I", read(preinit.lma, 4))[0] != address("flash_ram_sync") | 1):
        raise ValueError("SRAM 동기화 preinit 누락/변조")
    check_startup_ram(symbols, image, 0x08000000)
    # 고정 Cube SystemInit의 CPACR 설정과 boot VTOR write. primary macro 누출 거절.
    system = address("SystemInit")
    if (read(system, 24) != bytes.fromhex("054bd3f8882042f47002c3f888204ff000629a60704700bf") or
            read(system + 24, 4) != struct.pack("<I", 0xE000ED00)):
        raise ValueError("boot SystemInit/VTOR 명령 오류")
    return address("_edata") - address("_sdata")


def validate_handoff(symbols, image):
    # 고정 Arm GCC naked trampoline: LR=-1, MSP=r0, ISB, PRIMASK=0, BX r1.
    # MSP 변경 뒤 C epilogue/stack 접근 또는 복귀 명령을 허용하지 않는다.
    offset = layout.symbol_address(symbols, "boot_branch") - 0x08000000
    expected = bytes.fromhex("6ff0000e80f30888bff36f8f62b60847")
    if offset < 0 or image[offset:offset + len(expected)] != expected:
        raise ValueError("boot handoff trampoline 명령 오류")


def inspect(elf, compiler):
    def run(tool, *flags):
        exe = compiler.with_name(f"arm-none-eabi-{tool}{compiler.suffix}")
        return subprocess.check_output([str(exe), *flags, str(elf)], encoding="utf-8", timeout=30)
    headers, symbols = run("objdump", "-h"), run("nm", "--defined-only")
    sections = layout.sections_from_objdump(headers)
    image = elf.with_suffix(".bin").read_bytes()
    layout.validate(sections, symbols, elf.with_suffix(".map").read_text(encoding="utf-8"),
                    elf.read_bytes(), image, vector=0x08000000, payload_max=64 * 1024, allow_ccm=False)
    for kind, section in (("command", ram.SECTION), ("read", ".canview_flash_read_ram")):
        ram.validate(headers, run("objdump", "-t"), run("objdump", "-r", "-j", section),
                     run("objdump", "-d", "-j", section), kind, linked=True)
    copied = validate_copy(sections, symbols, image)
    validate_handoff(symbols, image)
    assembly = run("objdump", "-d", "--disassemble=flash_ram_sync")
    if not 0 <= assembly.find("dsb\t") < assembly.find("isb\t"):
        raise ValueError("SRAM DSB/ISB 동기화 누락/순서 오류")
    print(f"PASS: Arm SRAM link/copy {copied}B, handoff trampoline 16B, image {len(image)}/65536B; boot_go/physical handoff/HIL NOT_RUN")
    return sections, symbols, image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    args = parser.parse_args()
    inspect(args.elf, args.compiler)


if __name__ == "__main__":
    main()
