"""공개 DER 생성기와 실제 BSP/CMake 입력 거절·증분 교체 시험. 개인키는 메모리 전용."""
from __future__ import annotations
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec, ed25519

ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "tools/ota/generate_stm32_boot_trust.py"
spec = importlib.util.spec_from_file_location("boot_trust", GENERATOR)
trust = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trust)


def public_der(curve=ec.SECP256R1()):
    return ec.generate_private_key(curve).public_key().public_bytes(
        serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)


class GeneratorTests(unittest.TestCase):
    def test_u32(self):
        for text, value in (("0", 0), ("0000000001", 1), ("4294967295", (1 << 32) - 1)):
            self.assertEqual(trust.u32(text), value)
        for text in ("", "-1", "+1", " 1", "1 ", "1\n", "0x10", "1.0", "１", "١", "4294967296", "0" * 11):
            with self.subTest(text=text), self.assertRaises(argparse.ArgumentTypeError):
                trust.u32(text)

    def test_deterministic_and_bounds(self):
        der = public_der()
        self.assertEqual(trust.render(der, 0, 1, trust.UINT32_MAX), trust.render(der, 0, 1, trust.UINT32_MAX))
        for field in range(3):
            for bad in (-1, 1 << 32, True, None, "1", 1.0):
                values = [0, 0, 0]
                values[field] = bad
                with self.subTest(field=field, bad=bad), self.assertRaises(ValueError):
                    trust.render(der, *values)

    def test_truncation_extension_and_bit_corruption(self):
        der = public_der()
        for size in range(len(der)):
            with self.assertRaises(ValueError):
                trust.render(der[:size], 0, 0, 0)
        for bad in (der + b"\0", der * 1000, b"\0" * 91, b"\xff" * 91):
            with self.assertRaises(ValueError):
                trust.render(bad, 0, 0, 0)
        for bit in range(len(der) * 8):
            damaged = bytearray(der)
            damaged[bit // 8] ^= 1 << (bit % 8)
            with self.subTest(bit=bit), self.assertRaises(ValueError):
                trust.render(bytes(damaged), 0, 0, 0)

    def test_wrong_curve_format_and_private_input(self):
        private = ec.generate_private_key(ec.SECP256R1())
        invalid = [public_der(ec.SECP384R1()), public_der(ec.SECP256K1()),
            private.private_bytes(serialization.Encoding.DER, serialization.PrivateFormat.PKCS8,
                                  serialization.NoEncryption()),
            private.public_key().public_bytes(serialization.Encoding.X962, serialization.PublicFormat.CompressedPoint),
            private.public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo),
            ed25519.Ed25519PrivateKey.generate().public_key().public_bytes(
                serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)]
        for bad in invalid:
            with self.assertRaises(ValueError):
                trust.render(bad, 0, 0, 0)


def integration(args):
    with tempfile.TemporaryDirectory(prefix="canview-boot-trust-") as temp:
        work = Path(temp)
        der_path = work / "public.der"
        der_path.write_bytes(public_der())
        output = work / "generated.h"
        command = [sys.executable, "-X", "utf8", "-B", str(GENERATOR), "--public-der", str(der_path),
                   "--security-epoch", "0", "--manifest-key-id", "4294967295", "--stm-abi", "2", "--output", str(output)]

        def run(command, success=True):
            result = subprocess.run(command, cwd=work, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
            if (result.returncode == 0) != success:
                raise AssertionError(f"unexpected status {result.returncode}: {command}\n{result.stdout}\n{result.stderr}")
            if success and "warning:" in (result.stdout + result.stderr).lower():
                raise AssertionError(result.stdout + result.stderr)
            return result.stdout

        run(command)
        original = output.read_bytes()
        der_path.write_bytes(b"\0" * 92)
        run(command, False)
        assert output.read_bytes() == original, "invalid input changed prior output"
        der = public_der()
        der_path.write_bytes(der)
        config = [args.cmake, "-S", str(ROOT / "tests/ota/boot_trust"), "-B", str(work / "build"), "-G", "Ninja",
                  f"-DCMAKE_C_COMPILER={args.compiler}", f"-DCMAKE_MAKE_PROGRAM={args.ninja}",
                  f"-DPython3_EXECUTABLE={sys.executable}", f"-DCANVIEW_MCUBOOT_ROOT={args.mcuboot}"]
        names = ["CANVIEW_BOOT_PUBLIC_DER", "CANVIEW_BOOT_SECURITY_EPOCH", "CANVIEW_BOOT_MANIFEST_KEY_ID", "CANVIEW_BOOT_STM_ABI"]
        values = [str(der_path), "0", "4294967295", "2"]

        # 유효한 파일을 실제 준비한 fresh cache여야 FILEPATH 자동 변환을 검출한다.
        for suffix, entry in (("untyped", "CANVIEW_BOOT_PUBLIC_DER"),
                              ("typed", "CANVIEW_BOOT_PUBLIC_DER:FILEPATH")):
            fresh = config.copy()
            fresh[fresh.index("-B") + 1] = str(work / f"fresh-{suffix}")
            run(fresh + [f"-D{entry}=public.der"] +
                [f"-D{name}={value}" for name, value in zip(names[1:], values[1:])], False)

        def configure(inputs, success=True):
            return run(config + [f"-D{name}={value}" for name, value in zip(names, inputs)], success)

        assert "NOT_CONFIGURED" in configure([""] * 4)
        for omitted in range(4):
            missing = values.copy()
            missing[omitted] = ""
            configure(missing, False)
        for bad_path in ("public.der", str(work), str(work / "missing.der")):
            configure([bad_path] + values[1:], False)
        configure(values)
        build = [args.cmake, "--build", str(work / "build")]
        run(build)
        probe = work / "build" / ("boot-trust-probe.exe" if sys.platform == "win32" else "boot-trust-probe")
        assert run([str(probe)]).splitlines() == ["0 4294967295 2", der.hex()]
        # 별도 clean 없이 입력 값/키 교체가 재생성·재컴파일돼야 한다.
        replacement = public_der()
        der_path.write_bytes(replacement)
        run(build)
        assert run([str(probe)]).splitlines() == ["0 4294967295 2", replacement.hex()]
        configure([str(der_path), "4294967295", "0", "0"])
        run(build)
        assert run([str(probe)]).splitlines() == ["4294967295 0 0", replacement.hex()]
        der_path.write_bytes(b"\0" * 91)
        run(build, False)
        der_path.write_bytes(replacement)
        for index in range(1, 4):
            invalid = values.copy()
            invalid[index] = "4294967296"
            configure(invalid)
            run(build, False)
        configure(values)
        run(build)
        assert run([str(probe)]).splitlines() == ["0 4294967295 2", replacement.hex()]
        if args.public_output:
            args.public_output.write_bytes(replacement)  # 공개 시험 artifact만 보존한다.
        print("BSP/CMake: missing4, paths3, DER/u32 rejection, incremental replacement, null/copy PASS")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cmake", "compiler", "ninja", "mcuboot"):
        parser.add_argument(f"--{name}", required=True)
    parser.add_argument("--public-output", type=Path)
    arguments = parser.parse_args()
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GeneratorTests))
    if not result.wasSuccessful():
        raise SystemExit(1)
    integration(arguments)
