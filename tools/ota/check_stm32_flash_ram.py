"""Arm object의 Flash command/read SRAM 독립성 검사. 최종 SRAM map 검사를 대신하지 않는다."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess

SECTION = ".canview_flash_ram"


def validate(headers: str, symbols: str, relocations: str, assembly: str, kind: str = "command") -> int:
    """외부 helper/상수 참조, 누락/과대 section, section 밖 직접 branch를 거절한다."""
    section, functions = {
        "command": (SECTION, ("flash_execute", "flash_fault_reset")),
        "read": (".canview_flash_read_ram", ("read_execute", "read_nmi", "read_fault_reset")),
    }[kind]
    found = re.findall(rf"^\s*\d+\s+{re.escape(section)}\s+([0-9a-fA-F]+)\s", headers, re.M)
    if len(found) != 1 or not 0 < int(found[0], 16) <= 2048:
        raise ValueError("SRAM section 누락/크기 초과")
    size = int(found[0], 16)
    for name in functions:
        pattern = rf"^([0-9a-fA-F]+)\s+l\s+F\s+{re.escape(section)}\s+([0-9a-fA-F]+)\s+{name}$"
        entry = re.findall(pattern, symbols, re.M)
        if len(entry) != 1 or not 0 < int(entry[0][1], 16) or int(entry[0][0], 16) + int(entry[0][1], 16) > size:
            raise ValueError(f"SRAM 함수 누락/영역 밖: {name}")
    if "R_ARM_" in relocations:
        raise ValueError("busy section에 relocation 존재: 외부 함수/상수 접근 가능")
    if not all(f"<{name}>:" in assembly for name in functions):
        raise ValueError("SRAM disassembly 누락")
    # 함수 인자/stack return 외 간접 call은 금지. 로컬 직접 branch만 허용한다.
    for line in assembly.splitlines():
        match = re.match(r"\s*[0-9a-f]+:\s+(?:[0-9a-f]{4,8}\s+)+([a-z][a-z0-9.]*)\s+(.*)", line)
        if not match:
            continue
        instruction, operand = match.groups()
        indirect = re.fullmatch(r"(blx|bx)(?:eq|ne|cs|cc|hs|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?(?:\.[nw])?", instruction)
        if indirect and (indirect[1] != "bx" or operand.strip() != "lr"):
            raise ValueError("SRAM 간접 branch/call")
        if instruction in ("cbz", "cbnz"):
            target = re.fullmatch(r"r(?:[0-9]|1[0-5]),\s*([0-9a-f]+)\s+<[^>]+>", operand)
            if not target or not 0 <= int(target[1], 16) < size:
                raise ValueError("SRAM section 밖 compare branch")
        if re.fullmatch(r"b(?:l|eq|ne|cs|cc|hs|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?(?:\.[nw])?", instruction):
            target = re.match(r"([0-9a-f]+)\s+<", operand)
            if not target or not 0 <= int(target[1], 16) < size:
                raise ValueError("SRAM section 밖 branch")
    return size


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--object", type=Path, required=True)
    parser.add_argument("--objdump", type=Path, required=True)
    parser.add_argument("--kind", choices=("command", "read"), default="command")
    args = parser.parse_args()
    def run(*flags: str) -> str:
        return subprocess.run([str(args.objdump), *flags, str(args.object)], check=True,
                              capture_output=True, text=True, encoding="utf-8").stdout
    section = SECTION if args.kind == "command" else ".canview_flash_read_ram"
    size = validate(run("-h"), run("-t"), run("-r", "-j", section), run("-d", "-j", section), args.kind)
    print(f"PASS: SRAM {args.kind} {size}B, local functions only, external relocations0; final map/HIL NOT_RUN")


if __name__ == "__main__":
    main()
