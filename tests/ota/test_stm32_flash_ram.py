"""SRAM object 검사기의 거절 회귀. 실제 object 검사는 Arm post-build에서 수행한다."""
import importlib.util
from pathlib import Path
import unittest

SPEC = importlib.util.spec_from_file_location("flash_ram", Path(__file__).resolve().parents[2] / "tools/ota/check_stm32_flash_ram.py")
assert SPEC and SPEC.loader
RAM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RAM)


class FlashRamTests(unittest.TestCase):
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
    unittest.main()
