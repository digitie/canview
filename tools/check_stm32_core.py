"""STM32 bench ELF의 실제 크기/stack/TX 경계와 host register 상수를 검사한다."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]

FORBIDDEN_TX_SOURCE_PATTERNS = (
    re.compile(r"\b(?:HAL|LL)_FDCAN_[A-Za-z0-9_]*(?:TX|Tx|Transmit|transmit)[A-Za-z0-9_]*\b"),
    re.compile(r"(?:->|\.)\s*TX[A-Za-z0-9_]*\b"),
)
FORBIDDEN_MODE_SOURCE_PATTERN = re.compile(
    r"^\s*#\s*(?:undef|define)\s+CANVIEW_STM_(?:BUILD_MODE(?:_CAPTURE_ONLY)?|"
    r"CONTROL_CAPABILITIES|TX_PERMIT|ENABLE_BENCH_TX|ENABLE_VEHICLE_TX|"
    r"CAPTURE_ONLY_CONTRACT)\b", re.MULTILINE)


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
    forbidden.update(name for name in names
                     if re.search(r"(?:FDCAN|CAN).*(?:TX|Tx|Transmit|transmit)", name))
    if names & forbidden:
        raise RuntimeError(f"금지 heap/TX symbol: {sorted(names & forbidden)}")
    required = {"canview_stm_clock_start", "canview_stm_watchdog_start",
                "canview_stm_scheduler_step", "canview_stm_build_metadata_get",
                "canview_stm_stack_watermark_arm", "canview_stm_stack_watermark_sample",
                "canview_stm_service_policy_evaluate", "canview_stm_diagnostic_encode",
                "canview_stm_capture_only_contract_anchor", "SysTick_Handler", "NMI_Handler",
                "HardFault_Handler"}
    if not required <= names:
        raise RuntimeError(f"실제 core link 누락: {sorted(required - names)}")


def check_source_safety(source_root):
    """CAPTURE_ONLY target source가 FDCAN TX API/register를 사용하지 않는지 검사한다."""
    source_root = Path(source_root).resolve()
    violations = []
    for path in sorted(source_root.rglob("*")):
        if path.suffix.lower() not in (".c", ".h") or "build" in path.parts:
            continue
        text = path.read_text(encoding="utf-8")
        # C의 line continuation과 멤버 접근의 줄바꿈을 포함한 logical text를 검사한다.
        logical_text = re.sub(r"\\\r?\n", "", text)
        logical_lines = logical_text.splitlines()
        seen = set()

        def record(match):
            line_number = logical_text.count("\n", 0, match.start()) + 1
            line = logical_lines[line_number - 1].strip() if logical_lines else ""
            key = (line_number, line)
            if key not in seen:
                seen.add(key)
                violations.append(f"{path}:{line_number}: {line}")

        if path.name != "canview_build_mode.h":
            for match in FORBIDDEN_MODE_SOURCE_PATTERN.finditer(logical_text):
                record(match)
        for pattern in FORBIDDEN_TX_SOURCE_PATTERNS:
            for match in pattern.finditer(logical_text):
                record(match)
    if violations:
        raise RuntimeError("CAPTURE_ONLY source TX boundary violation: " + "; ".join(violations))
    return True


def check_compile_contract(commands, header):
    """compile database의 모든 C unit에 immutable CAPTURE_ONLY 주입이 있는지 확인한다."""
    header_name = Path(header).name.lower()
    missing = []
    checked = 0
    for entry in commands:
        source = Path(entry.get("file", ""))
        if source.suffix.lower() != ".c":
            continue
        checked += 1
        command = entry.get("command")
        if command is None:
            command = " ".join(str(argument) for argument in entry.get("arguments", []))
        normalized = str(command).replace("\\", "/").lower()
        has_token = re.search(
            r"(?<![a-z0-9_])-dcanview_stm_capture_only_contract=1(?![a-z0-9_])",
            normalized) is not None
        has_forced_header = "-include" in normalized and header_name in normalized
        if not has_token or not has_forced_header:
            missing.append(str(source))
    if checked == 0:
        raise RuntimeError("STM32 CAPTURE_ONLY compile contract의 C unit이 없음")
    if missing:
        raise RuntimeError(f"STM32 CAPTURE_ONLY compile contract 누락: {missing}")
    return checked


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


def stack_evidence(build, commands, symbols_for_object=None):
    """compile database의 모든 C object와 개별 .su를 대조한다. ASM/외부 archive는 제외."""
    build = build.resolve()
    paths = []
    for command in commands:
        suffix = Path(command["file"]).suffix.lower()
        if suffix == ".s":
            continue
        if suffix != ".c" or "-fstack-usage" not in command["command"].split():
            raise RuntimeError(f"C stack-usage compile 계약 누락: {command['file']}")
        output = Path(command["output"])
        if not output.is_absolute():
            output = Path(command["directory"]) / output
        output = output.resolve()
        if not output.is_relative_to(build) or not output.is_file():
            raise RuntimeError(f"build 밖 또는 누락 object: {output}")
        stack = output.with_suffix(".su")
        if not stack.is_file():
            raise RuntimeError(f"개별 stack evidence 누락: {stack}")
        if stack.stat().st_size == 0:
            # 생성된 const table 전용 C unit은 실제 code symbol이 없는 경우만 빈 .su 허용.
            if symbols_for_object is None:
                raise RuntimeError(f"빈 stack evidence: {stack}")
            symbols = symbols_for_object(output).splitlines()
            if any(len(line.split()) < 2 or line.split()[1] in ("t", "T", "w", "W") for line in symbols):
                raise RuntimeError(f"code object의 stack evidence가 비어 있음: {stack}")
        if stack in paths:
            raise RuntimeError(f"중복 object stack evidence: {stack}")
        paths.append(stack)
    if not paths:
        raise RuntimeError("C compile/stack evidence 목록이 비어 있음")
    return paths


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
    commands = json.loads((args.elf.parent / "compile_commands.json").read_text(encoding="utf-8"))
    check_compile_contract(commands, ROOT / "firmware/communicator/stm32/interface/canview_build_mode.h")
    stacks = stack_evidence(args.elf.parent, commands, lambda output: run(
        [str(tool_dir / f"arm-none-eabi-nm{suffix}"), "--defined-only", "--format=posix", str(output)]))
    max_frame = check_stack(line for path in stacks
                           for line in path.read_text(encoding="utf-8").splitlines())
    check_source_safety(ROOT / "firmware/communicator/stm32")
    check_source_safety(ROOT / "shared/app/src")
    check_source_safety(ROOT / "shared/protocol/src")
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
          f"max individual stack frame={max_frame}; {len(stacks)} C object stack files; "
          f"{len(constants)} CMSIS/model constants")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
