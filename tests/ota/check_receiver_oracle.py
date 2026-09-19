"""실제 CNG 수신 시험에 조기 AUTH_FAILED를 주입해 잘못된 통과를 검출한다."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/fixtures/idf-ota-image/main"


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise AssertionError("receiver mutation anchor drift")
    return source.replace(old, new, 1)


def main():
    compiler, library = sys.argv[1:3]
    source = (FIXTURE / "receiver.c").read_text(encoding="utf-8")
    feed = "status = canview_ota_body_feed(&body, (uint32_t)offset, data + offset, count);"
    open_end = "&floor, verify, verify_context, &hash);"
    early = "if (scenario == FIXTURE_BODY_TAMPER) { status = CANVIEW_AUTH_FAILED; }"
    feed_mutant = replace_once(source, feed, feed + "\n            " + early)
    open_mutant = replace_once(source, open_end, open_end + "\n            " + early)
    # 동일 조기 feed 오류가 이전 status-only oracle에서는 실제 성공함을 양성 대조한다.
    oracle = "passed = status == expected &&\n            (scenario != FIXTURE_BODY_TAMPER || (body_tamper_rejected && offset == size));"
    weak = replace_once(feed_mutant, oracle, "passed = status == expected;\n        (void)body_tamper_rejected;")
    with tempfile.TemporaryDirectory(prefix="canview-receiver-oracle-") as directory:
        root = Path(directory)
        for name, text, expected in (("early-open", open_mutant, 1), ("early-feed", feed_mutant, 1),
                                      ("weak-oracle-control", weak, 0)):
            mutant, executable = root / (name + ".c"), root / (name + ".exe")
            mutant.write_text(text, encoding="utf-8")
            command = [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror", "-Wpedantic",
                       "-DCANVIEW_OTA_FIXTURE_CNG=1"]
            for include in (FIXTURE, ROOT / "tests/ota", ROOT / "shared/ota/src", ROOT / "shared/interface"):
                command += ["-I", str(include)]
            command += [str(ROOT / "tests/ota/idf_receiver_host.c"), str(ROOT / "tests/ota/cng_provider.c"),
                        str(mutant), library, "-lbcrypt", "-o", str(executable)]
            subprocess.run(command, cwd=root, check=True, capture_output=True, timeout=40)
            result = subprocess.run([str(executable), str(ROOT / "tests/fixtures/ota-signed-golden/communicator.cvota")],
                                    cwd=root, capture_output=True, timeout=10)
            if result.returncode != expected:
                raise AssertionError((name, result.returncode, expected, result.stdout, result.stderr))
            print(f"PASS: {name}: exit {expected}")
    print("CNG host oracle regression only; SDK/device/HIL NOT_RUN")


if __name__ == "__main__":
    main()
