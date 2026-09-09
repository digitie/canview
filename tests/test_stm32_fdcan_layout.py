import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "check_stm32_fdcan_layout", ROOT / "tools" / "check_stm32_fdcan_layout.py")
LAYOUT = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(LAYOUT)


def _vendor_text(values=None):
    values = values or LAYOUT.VENDOR_MACROS
    lines = [f"#define {name} ({value}U)" for name, value in values.items()]
    return "\n".join(lines) + "\n"


def _contract_text(values=None):
    values = values or LAYOUT.CONTRACT_MACROS
    lines = [f"#define {name} ({value}U)" for name, value in values.items()]
    return "\n".join(lines) + "\n"


class Stm32FdcanLayoutTests(unittest.TestCase):
    def _validate(self, vendor=None, cmsis="RXF0S RXF0A", contract=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            vendor_path = root / "stm32g4xx_hal_fdcan.c"
            cmsis_path = root / "stm32g474xx.h"
            contract_path = root / "fdcan_message_ram.h"
            vendor_path.write_text(_vendor_text(vendor), encoding="utf-8")
            cmsis_path.write_text(cmsis, encoding="utf-8")
            contract_path.write_text(_contract_text(contract), encoding="utf-8")
            return LAYOUT.validate(vendor_path, cmsis_path, contract_path)

    def test_fixed_layout_matches_vendor_contract(self):
        values = self._validate()
        self.assertEqual(values["CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES"], 848)
        self.assertEqual(values["CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_OFFSET_BYTES"], 176)

    def test_vendor_layout_drift_fails_closed(self):
        vendor = dict(LAYOUT.VENDOR_MACROS)
        vendor["SRAMCAN_RF0_NBR"] = 4
        with self.assertRaises(RuntimeError):
            self._validate(vendor=vendor)

    def test_adapter_layout_drift_fails_closed(self):
        contract = dict(LAYOUT.CONTRACT_MACROS)
        contract["CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_BYTES"] = 16
        with self.assertRaises(RuntimeError):
            self._validate(contract=contract)

    def test_configurable_registers_are_rejected_for_stm32g4(self):
        with self.assertRaises(RuntimeError):
            self._validate(cmsis="RXF0S RXF0A RXF0C RXESC")


if __name__ == "__main__":
    unittest.main()
