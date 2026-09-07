"""설정 우회/누락과 보드 변조 부정 fixture. 실제 SDK 생성본은 target CI에서 검사한다."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


GATE = load("check_sdkconfig")
BOARDS = load("generate_boards")

# 명세의 독립 oracle. 구현의 FORBIDDEN을 가져오면 항목 삭제 변이가 시험에서도 사라진다.
EXPECTED_COMMON_FORBIDDEN = (
    "CONFIG_SPIRAM_IGNORE_NOTFOUND", "CONFIG_SPIRAM_SPEED_120M",
    "CONFIG_ESP_CONSOLE_UART_DEFAULT",
    "CONFIG_ESP_CONSOLE_UART_CUSTOM", "CONFIG_ESP_CONSOLE_USB_CDC", "CONFIG_ESP_CONSOLE_NONE",
    "CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG",
    "CONFIG_FREERTOS_UNICORE", "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE",
    "CONFIG_BOOTLOADER_APP_TEST", "CONFIG_SECURE_BOOT", "CONFIG_SECURE_FLASH_ENC_ENABLED",
    "CONFIG_BOOTLOADER_FACTORY_RESET", "CONFIG_BOOTLOADER_OTA_DATA_ERASE",
    "CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK", "CONFIG_EFUSE_VIRTUAL",
    "CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH", "CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH",
    "CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT", "CONFIG_ESP_SYSTEM_PANIC_GDBSTUB",
    "CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT", "CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME",
)

EXPECTED_FORBIDDEN = {
    "comm-r2-n16r8": EXPECTED_COMMON_FORBIDDEN + ("CONFIG_SPIRAM_MODE_QUAD", "CONFIG_ESPTOOLPY_FLASHSIZE_8MB"),
    "bridge-r1-n8r2": EXPECTED_COMMON_FORBIDDEN + ("CONFIG_SPIRAM_MODE_OCT", "CONFIG_SPIRAM_ECC_ENABLE",
                                               "CONFIG_ESPTOOLPY_FLASHSIZE_16MB"),
}


class SdkConfigTests(unittest.TestCase):
    # gate의 REQUIRED를 복사하지 않는 독립 정상 fixture.
    GOOD = '''CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="16MB"
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_ECC_ENABLE=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_SPEED=80
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_MEMTEST=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_INIT=y
CONFIG_ESP_TASK_WDT_PANIC=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=2
CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT=y
CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=0
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y
CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=y
CONFIG_ESP_INT_WDT=y
CONFIG_ESP_INT_WDT_CHECK_CPU1=y
CONFIG_FREERTOS_HZ=100
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_OFFSET=0x8000
# CONFIG_ESPTOOLPY_FLASHSIZE_1MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_2MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_4MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_8MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_32MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_64MB is not set
# CONFIG_ESPTOOLPY_FLASHSIZE_128MB is not set
# CONFIG_SPIRAM_IGNORE_NOTFOUND is not set
# CONFIG_SPIRAM_SPEED_120M is not set
# CONFIG_ESP_CONSOLE_UART_DEFAULT is not set
# CONFIG_ESP_CONSOLE_USB_CDC is not set
# CONFIG_ESP_CONSOLE_UART_CUSTOM is not set
# CONFIG_ESP_CONSOLE_NONE is not set
# CONFIG_FREERTOS_UNICORE is not set
# CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is not set
# CONFIG_BOOTLOADER_APP_TEST is not set
# CONFIG_SECURE_BOOT is not set
# CONFIG_SECURE_FLASH_ENC_ENABLED is not set
# CONFIG_BOOTLOADER_FACTORY_RESET is not set
# CONFIG_EFUSE_VIRTUAL is not set
# CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT is not set
# CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT is not set
# CONFIG_FLASH_ENCRYPTION_ENABLED is not set
# CONFIG_APP_ROLLBACK_ENABLE is not set
# CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT is not set
'''

    @classmethod
    def fixtures(cls):
        bridge = cls.GOOD.replace("CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y", "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y")
        bridge = bridge.replace('CONFIG_ESPTOOLPY_FLASHSIZE="16MB"', 'CONFIG_ESPTOOLPY_FLASHSIZE="8MB"')
        bridge = bridge.replace("# CONFIG_ESPTOOLPY_FLASHSIZE_8MB is not set\n", "")
        bridge = bridge.replace("MODE_OCT", "MODE_QUAD").replace("CONFIG_SPIRAM_ECC_ENABLE=y\n", "")
        return {"comm-r2-n16r8": cls.GOOD, "bridge-r1-n8r2": bridge}

    def test_positive_and_crlf(self):
        for board, good in self.fixtures().items():
            GATE.validate(good, board)
            GATE.validate(good.replace("\n", "\r\n"), board)
        with self.assertRaises(ValueError):
            GATE.validate(self.GOOD, "unreviewed")
        for board, other in (("bridge-r1-n8r2", self.GOOD),
                             ("comm-r2-n16r8", self.fixtures()["bridge-r1-n8r2"])):
            with self.subTest(board=board), self.assertRaises(ValueError):
                GATE.validate(other, board)

    def test_every_required_missing_or_mutated(self):
        for board, good in self.fixtures().items():
            expected = dict(GATE.REQUIRED)
            expected.update(GATE.MEMORY[board])
            for key, value in expected.items():
                line = f"{key}={value}"
                replacements = ("", f"{key}=n" if value != "n" else f"{key}=y")
                for replacement in replacements:
                    with self.subTest(board=board, key=key, replacement=replacement), self.assertRaises(ValueError):
                        GATE.validate(good.replace(line + "\n", replacement + "\n"), board)

    def test_forbidden_settings(self):
        for board, good in self.fixtures().items():
            for key in EXPECTED_FORBIDDEN[board]:
                with self.subTest(board=board, key=key), self.assertRaises(ValueError):
                    unset = f"# {key} is not set"
                    altered = (good.replace(unset, f"{key}=y", 1) if unset in good
                               else good + f"{key}=y\n")
                    GATE.validate(altered, board)

    def test_duplicates_and_malformed(self):
        for extra in ('CONFIG_SPIRAM=y', '# CONFIG_SPIRAM is not set', 'CONFIG_SPIRAM=garbage',
                      'CONFIG_SPIRAM_SPEED=80 trailing', 'CONFIG_IDF_TARGET="unterminated'):
            with self.subTest(extra=extra), self.assertRaises(ValueError):
                GATE.validate(self.GOOD + extra + "\n")

    def test_forbidden_removal_mutants_are_killed(self):
        for board in self.fixtures():
            for key in EXPECTED_FORBIDDEN[board]:
                if key.startswith("CONFIG_ESPTOOLPY_FLASHSIZE_"):
                    continue  # 별도 mutually-exclusive selector gate가 검증한다.
                altered = dict(GATE.FORBIDDEN)
                altered[board] = tuple(item for item in GATE.FORBIDDEN[board] if item != key)
                disabled = tuple(item for item in GATE.REQUIRED_DISABLED if item != key)
                with self.subTest(board=board, key=key), mock.patch.object(GATE, "FORBIDDEN", altered), \
                     mock.patch.object(GATE, "REQUIRED_DISABLED", disabled):
                    result = unittest.TestResult()
                    SdkConfigTests("test_forbidden_settings").run(result)
                    self.assertFalse(result.wasSuccessful(), "금지 항목 삭제 변이가 살아남음: " + key)
                    self.assertFalse(result.errors, "assertion이 아닌 시험 오류로 실패함")

    def test_factory_erase_and_panic_policy(self):
        with self.assertRaisesRegex(ValueError, "CONFIG_BOOTLOADER_FACTORY_RESET"):
            GATE.validate(self.GOOD.replace("# CONFIG_BOOTLOADER_FACTORY_RESET is not set",
                                            "CONFIG_BOOTLOADER_FACTORY_RESET=y"))
        for key in ("CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT", "CONFIG_ESP_SYSTEM_PANIC_GDBSTUB"):
            unset = f"# {key} is not set"
            altered = (self.GOOD.replace(unset, f"{key}=y", 1) if unset in self.GOOD
                       else self.GOOD + f"{key}=y\n")
            with self.subTest(key=key), self.assertRaises(ValueError):
                GATE.validate(altered)
        with self.assertRaisesRegex(ValueError, "CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS"):
            GATE.validate(self.GOOD.replace("CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=0",
                                           "CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=99"))

    def test_explicit_memory_watchdog_console(self):
        for before, after in (("CONFIG_SPIRAM_SPEED=80", "CONFIG_SPIRAM_SPEED=120"),
                              ("CONFIG_ESP_TASK_WDT_TIMEOUT_S=2", "CONFIG_ESP_TASK_WDT_TIMEOUT_S=5"),
                              ("CONFIG_SPIRAM_ECC_ENABLE=y", "# CONFIG_SPIRAM_ECC_ENABLE is not set"),
                              ("CONFIG_ESP_TASK_WDT_PANIC=y", "# CONFIG_ESP_TASK_WDT_PANIC is not set"),
                              ('CONFIG_IDF_TARGET="esp32s3"', 'CONFIG_IDF_TARGET="esp32"')):
            with self.subTest(after=after), self.assertRaises(ValueError):
                GATE.validate(self.GOOD.replace(before, after))

    def test_unknown_active_and_forbidden_nvs_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "검토하지 않은 sdkconfig key"):
            GATE.validate(self.GOOD + "CONFIG_UNREVIEWED_RESOURCE=y\n")
        with self.assertRaisesRegex(ValueError, "CONFIG_NVS_ENCRYPTION"):
            GATE.validate(self.GOOD + "CONFIG_NVS_ENCRYPTION=y\n")

    def test_required_disabled_line_removal_is_rejected(self):
        for key in GATE.REQUIRED_DISABLED:
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, key):
                GATE.validate(self.GOOD.replace(f"# {key} is not set\n", ""))

    def test_board_negative_and_defaults(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        board = json.loads(manifest)["boards"][1]
        source = BOARDS.canonical(ROOT / board["source"])
        for key, value in (("module", "ESP32-S3-MINI-1-N8"), ("psram_ecc", False),
                           ("core_profile", None), ("flash_bytes", 8388608), ("psram_bytes", 2097152)):
            altered = copy.deepcopy(board)
            altered[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                BOARDS.board_outputs(altered, manifest, source)
        for pin in (35, 36, 37):
            altered = source.replace(b"IO48", f"IO{pin}".encode())
            self.assertNotEqual(source, altered)
            with self.subTest(pin=pin), self.assertRaises(ValueError):
                BOARDS.board_outputs(board, manifest, altered)
        generated = BOARDS.board_outputs(board, manifest, source)
        defaults = GATE.parse(generated[board["path"] + "/sdkconfig.defaults"])
        for key in ("CONFIG_SPIRAM_ECC_ENABLE", "CONFIG_ESP_TASK_WDT_PANIC",
                    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0", "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1"):
            self.assertEqual(defaults[key], "y")
        self.assertEqual(defaults["CONFIG_ESP_TASK_WDT_TIMEOUT_S"], "2")
        self.assertEqual(defaults["CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT"], "y")
        self.assertEqual(defaults["CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS"], "0")
        self.assertEqual(defaults["CONFIG_BOOTLOADER_FACTORY_RESET"], "n")
        self.assertEqual(defaults["CONFIG_EFUSE_VIRTUAL"], "n")

    def test_bridge_board_and_defaults(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        board = next(item for item in json.loads(manifest)["boards"] if item["id"] == "bridge-r1-n8r2")
        source = BOARDS.canonical(ROOT / board["source"])
        for key, value in (("module", "ESP32-S3-WROOM-1-N16R8"), ("psram_ecc", True),
                           ("core_profile", None), ("flash_bytes", 16777216),
                           ("psram_bytes", 8388608), ("psram_mode", "octal"), ("recovery_gpio", 5)):
            altered = copy.deepcopy(board)
            altered[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                BOARDS.board_outputs(altered, manifest, source)
        for old, new in ((b",IO4,", b",IO6,"), (b",IO5,", b",IO4,"), (b",IO5,", b",IO22,")):
            altered = source.replace(old, new)
            self.assertNotEqual(source, altered)
            with self.subTest(pin=new), self.assertRaises(ValueError):
                BOARDS.board_outputs(board, manifest, altered)
        for old, new in ((b",13,USB_D-,bidirectional,USB_DM,", b",99,USB_D-,bidirectional,USB_DM,"),
                         (b",USB_D-,bidirectional,USB_DM,", b",IO6,bidirectional,USB_DM,"),
                         (b",USB_D+,bidirectional,USB_DP,", b",USB_D+,power_in,USB_DP,"),
                         (b",USB_DM,", b",USB_DM_BAD,")):
            altered = source.replace(old, new)
            self.assertNotEqual(source, altered)
            with self.subTest(usb=new), self.assertRaises(ValueError):
                BOARDS.board_outputs(board, manifest, altered)
        generated = BOARDS.board_outputs(board, manifest, source)
        defaults = GATE.parse(generated[board["path"] + "/sdkconfig.defaults"])
        for key, expected in (("CONFIG_ESPTOOLPY_FLASHSIZE", '"8MB"'),
                              ("CONFIG_SPIRAM_MODE_QUAD", "y"), ("CONFIG_SPIRAM_ECC_ENABLE", "n"),
                              ("CONFIG_ESP_TASK_WDT_PANIC", "y"), ("CONFIG_ESP_TASK_WDT_TIMEOUT_S", "2"),
                              ("CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT", "y"),
                              ("CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS", "0"),
                              ("CONFIG_BOOTLOADER_FACTORY_RESET", "n"), ("CONFIG_EFUSE_VIRTUAL", "n")):
            self.assertEqual(defaults[key], expected)
        self.assertIn("#define CANVIEW_BOARD_PSRAM_AVAILABLE_BYTES (2097152U)",
                      generated[board["path"] + "/bsp/board_pins.h"])

    def test_cli_missing_invalid_valid(self):
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "sdkconfig"
            command = [sys.executable, "-B", str(ROOT / "tools/check_sdkconfig.py"), str(config)]
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            config.write_text("", encoding="utf-8")
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            for board, good in self.fixtures().items():
                config.write_text(good, encoding="utf-8")
                self.assertEqual(subprocess.run(command + ["--board", board], capture_output=True).returncode, 0)
                self.assertNotEqual(subprocess.run(command + ["--board", "bad-board"], capture_output=True).returncode, 0)


if __name__ == "__main__":
    unittest.main()
