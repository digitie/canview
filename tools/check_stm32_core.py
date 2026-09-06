"""STM32 bench ELF의 실제 크기/stack/TX 경계와 host register 상수를 검사한다."""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def check_memory(size):
    fields = size.splitlines()[-1].split()
    text, data, bss = (int(value) for value in fields[:3])
    if min(text, data, bss) < 0 or text + data > 262144 or data + bss > 96 * 1024:
        raise RuntimeError(f"실제 ELF memory budget 초과: {text=}, {data=}, {bss=}")
    return text, data, bss


def check_symbols(symbols):
    names = {line.split()[-1] for line in symbols.splitlines() if line.split()}
    forbidden = {"malloc", "calloc", "realloc", "free", "_sbrk",
                 "HAL_FDCAN_AddMessageToTxFifoQ", "HAL_FDCAN_AddMessageToTxBuffer"}
    if names & forbidden:
        raise RuntimeError(f"금지 heap/TX symbol: {sorted(names & forbidden)}")
    required = {"canview_stm_clock_start", "canview_stm_watchdog_start",
                "canview_stm_scheduler_step", "SysTick_Handler", "NMI_Handler", "HardFault_Handler"}
    if not required <= names:
        raise RuntimeError(f"실제 core link 누락: {sorted(required - names)}")


def check_stack(lines):
    frames = []
    for line in lines:
        fields = line.split("\t")
        if len(fields) != 3 or fields[2] not in ("static", "dynamic,bounded"):
            raise RuntimeError(f"stack 형식 또는 unbounded stack: {line}")
        frames.append(int(fields[1]))
    if not frames or min(frames) < 0 or max(frames) > 2048:
        raise RuntimeError(f"단일 frame stack gate 실패: {max(frames, default=-1)}")
    return max(frames)


def run(arguments, **kwargs):
    return subprocess.check_output(arguments, text=True, encoding="utf-8", **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--sdk", required=True, type=Path)
    args = parser.parse_args()
    tool_dir = args.compiler.parent
    suffix = args.compiler.suffix
    size = run([str(tool_dir / f"arm-none-eabi-size{suffix}"), str(args.elf)])
    text, data, bss = check_memory(size)
    symbols = run([str(tool_dir / f"arm-none-eabi-nm{suffix}"), "--defined-only", str(args.elf)])
    check_symbols(symbols)
    max_frame = check_stack(line for path in args.elf.parent.rglob("*.su")
                           for line in path.read_text(encoding="utf-8").splitlines())
    # 모델 register의 숫자와 고정 vendor CMSIS를 독립 compile-time 비교한다.
    model = ROOT / "firmware/communicator/stm32/tests/register_model.h"
    constants = re.findall(r"^#define (\w+) (UINT32_C\(0x[0-9a-f]+\)|UINT32_C\([0-9]+\)|\([0-9]+U\))$",
                           model.read_text(encoding="utf-8"), flags=re.MULTILINE)
    if len(constants) < 40:
        raise RuntimeError("register model 상수 목록 추출 실패")
    source = '#include "stm32g474xx.h"\n'
    source += "\n".join(f"typedef char check_{name}[({name} == {value}) ? 1 : -1];"
                        for name, value in constants)
    device = args.sdk / "Drivers/CMSIS/Device/ST/STM32G4xx/Include"
    core = args.sdk / "Drivers/CMSIS/Core/Include"
    subprocess.run([str(args.compiler), "-x", "c", "-std=c99", "-fsyntax-only", "-Werror",
                    "-mcpu=cortex-m4", "-mthumb", "-DSTM32G474xx", f"-I{device}", f"-I{core}", "-"],
                   input=source, text=True, encoding="utf-8", check=True)
    print(f"PASS: STM32 core text={text} data={data} bss+reserved-stack={bss}; "
          f"max individual stack frame={max_frame}; {len(constants)} CMSIS/model constants")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
