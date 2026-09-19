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
    def test_boot_closure_mutations(self):
        if not self.full_boot:
            self.skipTest("explicit BSP trust not configured; full boot link NOT_RUN")
        _, symbols, _ = self.evidence
        wrappers = [" 8001234: f000 f800 bl 8002340 <fih_panic_loop>"] * 2
        boot.validate_boot_closure(symbols, wrappers)
        for bad in ([], wrappers[:1], wrappers * 2):
            with self.assertRaises(ValueError):
                boot.validate_boot_closure(symbols, bad)
        for name in ("boot_go", "fih_panic_loop", "__wrap___assert_func", "__wrap_abort"):
            with self.subTest(missing=name), self.assertRaises(ValueError):
                boot.validate_boot_closure(symbols.replace(name, "missing"), wrappers)
        for name in ("__assert_func", "abort", "malloc", "_malloc_r", "free", "printf", "_vfprintf_r",
                     "_fstat", "_getpid", "_isatty", "_kill", "_sbrk"):
            with self.subTest(forbidden=name), self.assertRaises(ValueError):
                boot.validate_boot_closure(symbols + f"\n08002300 T {name}\n", wrappers)
        for bad in ("", wrappers[0].replace("fih_panic_loop", "abort"), " 8001234: 4770 bx lr",
                    wrappers[0] + "\n 8001238: 4770 bx lr"):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                boot.validate_boot_closure(symbols, [bad, wrappers[1]])
        for mnemonic in ("beq", "bne", "bcs", "bcc", "bmi", "bpl", "bvs", "bvc",
                         "bhi", "bls", "bge", "blt", "bgt", "ble", "blx", "bx"):
            for suffix in ("", ".w", ".n"):
                bad = wrappers[0].replace(" bl ", f" {mnemonic}{suffix} ")
                with self.subTest(branch=mnemonic + suffix), self.assertRaises(ValueError):
                    boot.validate_boot_closure(symbols, [bad, wrappers[1]])

    def test_handoff_mutations(self):
        _, symbols, image = self.evidence
        boot.validate_handoff(symbols, image)
        offset = boot.layout.symbol_address(symbols, "boot_branch") - 0x08000000
        for byte in range(16):
            for bit in range(8):
                bad = bytearray(image)
                bad[offset + byte] ^= 1 << bit
                with self.subTest(byte=byte, bit=bit), self.assertRaises(ValueError):
                    boot.validate_handoff(symbols, bad)
        with self.assertRaises(ValueError):
            boot.validate_handoff(symbols, image[:offset + 15])

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
        self.assertEqual(len(objects), 10)
        libraries = [str(build / "libcanview_boot_flash_guard.a"), str(build / "libcanview_stm_flash_layout.a")]
        flags = []
        if self.full_boot:
            libraries = ["-Wl,--start-group", str(build / "bootloader/upstream/libbootutil.a"),
                *[str(build / f"bootloader/libcanview_boot_{name}.a")
                  for name in ("crypto", "flash_map", "identity", "fail_stop")],
                *libraries, "-Wl,--end-group"]
            flags = ["-Wl,--undefined=boot_go", "-Wl,--undefined=__wrap___assert_func",
                     "-Wl,--undefined=__wrap_abort", "-Wl,--wrap=__assert_func", "-Wl,--wrap=abort"]
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
                    "-Wl,--build-id=sha1", "-Wl,--wrap=SystemInit", "-Wl,--fatal-warnings", *flags, *objects, *libraries,
                    "-o", str(path / "probe.elf")], capture_output=True, text=True, timeout=30)
                if diagnostic is None:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stderr, "")
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(diagnostic, result.stderr)

        if self.full_boot:
            # --wrap 누락을 실제 linker에서 거절한다. 경고를 숨기거나 assert를 제거하지 않는다.
            with tempfile.TemporaryDirectory(prefix="canview-boot-fail-stop-") as temp:
                for missing in ("__assert_func", "abort"):
                    altered = [flag for flag in flags if flag != f"-Wl,--wrap={missing}"]
                    # abort는 현재 SDK 경로에서 GC될 수 있으므로 해당 실패 ABI도 명시 참조한다.
                    altered.append(f"-Wl,--undefined={missing}")
                    result = subprocess.run([str(self.compiler), "-mcpu=cortex-m4", "-mthumb",
                        "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
                        f"-T{ROOT / 'firmware/communicator/stm32/ld/STM32G474CEUx_BOOT.ld'}",
                        "--specs=nano.specs", "--specs=nosys.specs", "-Wl,--gc-sections",
                        "-Wl,--wrap=SystemInit", "-Wl,--fatal-warnings", *altered, *objects, *libraries,
                        "-o", str(Path(temp) / "probe.elf")], capture_output=True, text=True, timeout=30)
                    with self.subTest(missing_wrap=missing):
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn("not implemented", result.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--full-boot", action="store_true")
    args, rest = parser.parse_known_args()
    BootRamTests.elf, BootRamTests.compiler = args.elf, args.compiler
    BootRamTests.full_boot = args.full_boot
    BootRamTests.evidence = boot.inspect(args.elf, args.compiler, args.full_boot)
    unittest.main(argv=[sys.argv[0], *rest])
