"""생성된 C 헤더와 보드 산출물의 drift를 읽기 전용으로 검사한다."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load_generator(name: str):
    path = ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(f"canview_{name}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"generator import failed: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def expected_outputs() -> dict[Path, str]:
    transport = load_generator("generate_transport")
    boards = load_generator("generate_boards")
    transport_source = transport.SOURCE.read_bytes().replace(b"\r\n", b"\n")
    outputs: dict[Path, str] = {
        transport.OUTPUT: transport.render(transport_source),
    }
    outputs.update({ROOT / relative: content
                    for relative, content in boards.outputs().items()})
    return outputs


def mismatches(outputs: dict[Path, str]) -> list[Path]:
    failed: list[Path] = []
    for path, expected in sorted(outputs.items()):
        if not path.exists() or path.read_text(encoding="utf-8") != expected:
            failed.append(path)
    return failed


def negative_fixture(outputs: dict[Path, str]) -> bool:
    """검사 대상이 한 글자 바뀌었을 때 drift를 놓치지 않는지 확인한다."""
    path, expected = next(iter(outputs.items()))
    mutated = expected + "/* fixture mutation */\n"
    return path not in mismatches({path: mutated})


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--negative-fixture", action="store_true",
                        help="수동 변경된 생성물을 반드시 거부하는지 검사")
    args = parser.parse_args(argv)
    outputs = expected_outputs()
    try:
        protocol = load_generator("generate_protocol")
        protocol.generate(write=False)
    except (OSError, KeyError, ValueError) as exc:
        print(f"FAIL: generated protocol drift: {exc}")
        return 1
    if args.negative_fixture:
        if negative_fixture(outputs):
            print("FAIL: generated drift negative fixture was accepted")
            return 1
        print("PASS: generated drift negative fixture rejected")
        return 0
    failed = mismatches(outputs)
    if failed:
        print("FAIL: generated output drift")
        for path in failed:
            print(path.relative_to(ROOT))
        return 1
    print(f"PASS: generated output check ({len(outputs)} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
