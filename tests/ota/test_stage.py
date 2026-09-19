"""C 수신 저장 gate: 실제 parser + 저장/native 모형, Windows 옵션에서만 실제 outer 암호."""
import copy
import hashlib
import struct
import subprocess
import sys

from test_manifest import fixture, make_prefix
from envelope import assemble


def main():
    crypto = len(sys.argv) == 3 and sys.argv[2] == "--crypto"
    if crypto:
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import ec, utils
        key = ec.generate_private_key(ec.SECP256R1())
        point = key.public_key().public_numbers()
        public = point.x.to_bytes(32, "big") + point.y.to_bytes(32, "big")

        def sign(message):
            r, s = utils.decode_dss_signature(key.sign(message, ec.ECDSA(hashes.SHA256())))
            return r.to_bytes(32, "big") + s.to_bytes(32, "big")
    else:
        public = bytes(64)
        sign = lambda _message: bytes(64)
    cases = 0
    for role in (1, 2, 3):
        manifest = fixture(role)
        images = [bytes(range(32)) for _ in manifest[8]]
        for entry, image in zip(manifest[8], images):
            entry[2] = hashlib.sha256(image).digest() if crypto else struct.pack("<II", sum(image), len(image)) + bytes(24)
        package = assemble(manifest, images, sign)
        prefix_size = len(make_prefix(manifest, sign))

        def run(data, prefix, reject):
            nonlocal cases
            wire = struct.pack("<4I", role, prefix, len(data), reject) + public + data
            result = subprocess.run([sys.argv[1]], input=wire, capture_output=True, timeout=30)
            if result.returncode != 0 or not result.stdout.startswith(b"PASS:"):
                raise AssertionError((role, cases, result.returncode, result.stdout, result.stderr))
            cases += 1

        run(package, prefix_size, False)
        for target in (0, 5, 255, 0xffffffff):
            altered = copy.deepcopy(manifest)
            altered[8][0][0] = target
            prefix = make_prefix(altered, sign)
            run(prefix, len(prefix), True)
        for field, value in ((3, "wrong-board"), (4, "wrong-layout"), (6, 8), (7, 12)):
            altered = copy.deepcopy(manifest)
            altered[field] = value
            prefix = make_prefix(altered, sign)
            run(prefix, len(prefix), True)
    print(f"PASS: {cases} role/input groups; actual outer crypto={crypto}; storage/native callback model only")


if __name__ == "__main__":
    main()
