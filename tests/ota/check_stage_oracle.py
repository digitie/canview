"""실제 C gate 변이 실행파일이 호출 순서 시험에서 실패하는지 검사한다."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from test_manifest import fixture, make_prefix
from envelope import assemble

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler, library = sys.argv[1:3]
    source = (ROOT / "shared/ota/src/stage.c").read_text(encoding="utf-8")
    manifest = fixture()
    images = [bytes(range(32)), bytes(range(32))]
    for entry, image in zip(manifest[8], images):
        entry[2] = struct.pack("<II", sum(image), len(image)) + bytes(24)
    sign = lambda _message: bytes(64)
    package = assemble(manifest, images, sign)
    wire = struct.pack("<4I", 1, len(make_prefix(manifest, sign)), len(package), 0) + bytes(64) + package
    edits = (
        ("early-begin", "if (status == CANVIEW_OK)\n    {\n        stage->storage = *storage;",
         "if (true)\n    {\n        stage->storage = *storage;"),
        ("write-after-reject", "if (status == CANVIEW_OK && size != 0U)", "if (size != 0U)"),
        ("ignore-native-failure", "if (status == CANVIEW_OK) { stage->state = CANVIEW_OTA_STAGE_NATIVE_MATCHED; }",
         "if (true) { stage->state = CANVIEW_OTA_STAGE_NATIVE_MATCHED; status = CANVIEW_OK; }"),
        ("hash-context-overlap", "stage_overlaps(stage, hash->context, 1U)", "false"),
    )
    variants = [("baseline", source, 0)]
    for name, old, new in edits:
        if source.count(old) != 1:
            raise AssertionError("stage mutation anchor drift")
        variants.append((name, source.replace(old, new, 1), 1))
    with tempfile.TemporaryDirectory(prefix="canview-stage-oracle-") as directory:
        root = Path(directory)
        for name, text, expected in variants:
            mutant, binary = root / (name + ".c"), root / (name + ".exe")
            mutant.write_text(text, encoding="utf-8")
            command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                       "-I", str(ROOT / "shared/ota/src"), "-I", str(ROOT / "shared/interface"),
                       str(ROOT / "tests/ota/stage_probe.c"), str(mutant), library, "-o", str(binary)]
            subprocess.run(command, cwd=root, check=True, capture_output=True, timeout=40)
            result = subprocess.run([str(binary)], input=wire, capture_output=True, timeout=10)
            if result.returncode != expected or (expected == 1 and b"stage_probe.c:" not in result.stderr):
                raise AssertionError((name, result.returncode, expected, result.stdout, result.stderr))
            print(f"PASS: {name}: exit {expected}")
    print("C call-order model only; actual Flash/native device/install NOT_RUN")


if __name__ == "__main__":
    main()
