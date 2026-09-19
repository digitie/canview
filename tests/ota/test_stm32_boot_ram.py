"""실제 Arm ELF의 copy/startup 변이와 실제 linker 거절 시험. HIL 대체가 아니다."""
import argparse
from dataclasses import replace
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/ota"))
import check_stm32_boot_ram as boot


class BootRamTests(unittest.TestCase):
    def test_copy_mutations(self):
        sections, symbols, image = self.evidence
        self.assertGreater(boot.validate_copy(sections, symbols, image), 0)
        for name in (".canview_flash_ram", ".canview_flash_read_ram", ".data"):
            for field in ("vma", "lma", "size"):
                for delta in (-4, -1, 1, 4):
                    bad = [replace(s, **{field: getattr(s, field) + delta}) if s.name == name else s for s in sections]
                    with self.subTest(name=name, field=field, delta=delta), self.assertRaises(ValueError):
                        boot.validate_copy(bad, symbols, image)
        for name in (".bss", ".preinit_array", ".data", ".canview_flash_ram"):
            with self.subTest(missing=name), self.assertRaises(ValueError):
                boot.validate_copy([s for s in sections if s.name != name], symbols, image)
        reset = boot.layout.symbol_address(symbols, "Reset_Handler") - 0x08000000
        # 복사/bss/BL 명령의 모든 bit. 변형이 우연히 같은 literal을 읽는 경우도 없어야 한다.
        system = boot.layout.symbol_address(symbols, "SystemInit") - 0x08000000
        for offset in (*range(reset + 8, reset + 54), *range(system, system + 28)):
            for bit in range(8):
                bad = bytearray(image)
                bad[offset] ^= 1 << bit
                with self.subTest(offset=offset, bit=bit), self.assertRaises((ValueError, RuntimeError)):
                    boot.validate_copy(sections, symbols, bad)
        preinit = next(s for s in sections if s.name == ".preinit_array")
        for bit in range(32):
            bad = bytearray(image)
            offset = preinit.lma - 0x08000000
            original, = struct.unpack_from("<I", bad, offset)
            struct.pack_into("<I", bad, offset, original ^ (1 << bit))
            with self.subTest(preinit_bit=bit), self.assertRaises(ValueError):
                boot.validate_copy(sections, symbols, bad)

    def test_linker_rejections(self):
        build = self.elf.parent.resolve()
        commands = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
        objects = [str((Path(c["directory"]) / c["output"]).resolve()) for c in commands
                   if "canview-boot-ram-link-test.dir" in c["output"]]
        self.assertEqual(len(objects), 9)
        source = (ROOT / "firmware/communicator/stm32/ld/STM32G474CEUx_BOOT.ld").read_text(encoding="utf-8")
        cases = [(source, None),
                 (source.replace("LENGTH = 64K", "LENGTH = 1K"), "overflowed"),
                 (source.replace("_edata = .;", "_edata = . + 1;"), "SDK copy alignment"),
                 (source.replace("KEEP(*(.canview_flash_read_ram))", ". += 2048; KEEP(*(.canview_flash_read_ram))"), "read SRAM missing/oversize"),
                 (source.replace(".data :", ".data ALIGN(512) :"), "data VMA gap"),
                 (source.replace("*(.ccmram .ccmram.*)", ". += 4; *(.ccmram .ccmram.*)"), "CCM parity initialization")]
        for index, (script, diagnostic) in enumerate(cases):
            with self.subTest(index=index), tempfile.TemporaryDirectory(prefix="canview-boot-link-") as temp:
                path = Path(temp)
                (path / "probe.ld").write_text(script, encoding="utf-8")
                result = subprocess.run([str(self.compiler), "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
                    f"-T{path / 'probe.ld'}", "--specs=nano.specs", "--specs=nosys.specs", "-Wl,--gc-sections",
                    "-Wl,--build-id=sha1", "-Wl,--wrap=SystemInit", "-Wl,--fatal-warnings", *objects,
                    str(build / "libcanview_boot_flash_guard.a"), str(build / "libcanview_stm_flash_layout.a"),
                    "-o", str(path / "probe.elf")], capture_output=True, text=True, timeout=30)
                if diagnostic is None:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stderr, "")
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(diagnostic, result.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    args, rest = parser.parse_known_args()
    BootRamTests.elf, BootRamTests.compiler = args.elf, args.compiler
    BootRamTests.evidence = boot.inspect(args.elf, args.compiler)
    unittest.main(argv=[sys.argv[0], *rest])
