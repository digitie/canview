"""SRAM object 검사기의 거절 회귀. 실제 object 검사는 Arm post-build에서 수행한다."""
import importlib.util
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location("flash_ram", Path(__file__).resolve().parents[2] / "tools/ota/check_stm32_flash_ram.py")
assert SPEC and SPEC.loader
RAM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RAM)


class FlashRamTests(unittest.TestCase):
    arm_cc = None
    arm_objdump = None

    def test_arm_negative_objects(self):
        if self.arm_cc is None:
            self.skipTest("actual Arm negative objects require --cc/--objdump")
        cases = ["bx lr", "cmp r3, #0\nit ne\nbxne r3", "cmp r3, #0\nit ne\nblxne r3", "blx lr"]
        for index, instructions in enumerate(cases):
            with self.subTest(instructions=instructions), tempfile.TemporaryDirectory(prefix="canview-ram-negative-") as directory:
                source = Path(directory) / "probe.S"
                obj = Path(directory) / "probe.o"
                source.write_text(".syntax unified\n.thumb\n.section .canview_flash_read_ram,\"ax\",%progbits\n"
                    ".type read_fault_reset,%function\n.thumb_func\nread_fault_reset:\nb .\n.size read_fault_reset,.-read_fault_reset\n"
                    ".type read_nmi,%function\n.thumb_func\nread_nmi:\nbx lr\n.size read_nmi,.-read_nmi\n"
                    ".type read_execute,%function\n.thumb_func\nread_execute:\n" + instructions +
                    "\nbx lr\n.size read_execute,.-read_execute\n", encoding="utf-8")
                compiled = subprocess.run([str(self.arm_cc), "-mcpu=cortex-m4", "-mthumb", "-c", str(source), "-o", str(obj)],
                                          check=True, capture_output=True, text=True)
                self.assertEqual(compiled.stderr, "", "Arm negative probe compiler/assembler diagnostics")
                def dump(*flags):
                    return subprocess.run([str(self.arm_objdump), *flags, str(obj)], check=True,
                                          capture_output=True, text=True).stdout
                values = (dump("-h"), dump("-t"), dump("-r", "-j", ".canview_flash_read_ram"),
                          dump("-d", "-j", ".canview_flash_read_ram"))
                if index == 0:
                    self.assertGreater(RAM.validate(*values, kind="read"), 0)
                else:
                    with self.assertRaisesRegex(ValueError, "간접 branch"):
                        RAM.validate(*values, kind="read")

    def test_read_profile(self):
        good = [" 4 .canview_flash_read_ram 00000100 00000000\n",
                "00000000 l     F .canview_flash_read_ram 00000020 read_fault_reset\n"
                "00000020 l     F .canview_flash_read_ram 00000020 read_nmi\n"
                "00000040 l     F .canview_flash_read_ram 000000c0 read_execute\n", "",
                "00000000 <read_fault_reset>:\n00000020 <read_nmi>:\n00000040 <read_execute>:\n"
                " 50: b110 cbz r0, 58 <read_execute+0x18>\n 58: 4770 bx lr\n"]
        self.assertEqual(RAM.validate(*good, kind="read"), 256)
        for index, value in [(0, good[0].replace("_read", "")),
                             (1, good[1].replace("read_nmi", "missing")),
                             (1, good[1].replace("000000c0", "00000100")),
                             (2, "00000060 R_ARM_THM_CALL memcpy\n"),
                             (2, "00000060 R_ARM_ABS32 .rodata\n"),
                             (3, good[3].replace("<read_nmi>:", "<missing>:")),
                             (3, good[3] + " 60: 4798 blx r3\n"),
                             (3, good[3] + " 60: 4718 bx r3\n"),
                             (3, good[3] + " 60: 4718 bxne r3\n"),
                             (3, good[3] + " 60: 4798 blxne r3\n"),
                             (3, good[3] + " 60: 47f0 blx lr\n"),
                             (3, good[3] + " 60: b110 cbz r0, 100 <outside>\n"),
                             (3, good[3] + " 60: b910 cbnz r0, 100 <outside>\n"),
                             (3, good[3] + " 60: b910 cbnz r0, unexpected\n")]:
            bad = good.copy()
            bad[index] = value
            with self.subTest(index=index, value=value), self.assertRaises(ValueError):
                RAM.validate(*bad, kind="read")
        with self.assertRaises(ValueError):
            RAM.validate(*good)

    def test_mutations(self):
        good = [" 4 .canview_flash_ram 00000100 00000000\n",
                "00000000 l     F .canview_flash_ram 00000020 flash_fault_reset\n"
                "00000020 l     F .canview_flash_ram 000000e0 flash_execute\n", "",
                "00000000 <flash_fault_reset>:\n00000020 <flash_execute>:\n 30: f7ff ffff bl 0 <flash_fault_reset>\n"]
        self.assertEqual(RAM.validate(*good), 256)
        for index, value in [(0, ""), (0, good[0].replace("00000100", "00001000")),
                             (0, good[0] * 2), (1, ""),
                             (1, good[1].replace("000000e0", "00000100")),
                             (2, "00000040 R_ARM_THM_CALL memcpy\n"),
                             (2, "00000040 R_ARM_ABS32 .rodata\n"), (3, ""),
                             (3, good[3] + " 40: 4798 blx r3\n"),
                             (3, good[3] + " 40: e080 b.n 100 <outside>\n")]:
            bad = good.copy()
            bad[index] = value
            with self.subTest(index=index, value=value), self.assertRaises(ValueError):
                RAM.validate(*bad)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", type=Path)
    parser.add_argument("--objdump", type=Path)
    args, remaining = parser.parse_known_args()
    if (args.cc is None) != (args.objdump is None):
        parser.error("--cc와 --objdump는 함께 지정해야 한다")
    FlashRamTests.arm_cc, FlashRamTests.arm_objdump = args.cc, args.objdump
    unittest.main(argv=[sys.argv[0], *remaining])
