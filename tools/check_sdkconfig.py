"""Communicator bench image의 실제 sdkconfig를 fail-closed로 검사한다. HIL 증거가 아니다."""
from __future__ import annotations
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "CONFIG_IDF_TARGET": '"esp32s3"',
    "CONFIG_ESPTOOLPY_FLASHSIZE_16MB": "y",
    "CONFIG_ESPTOOLPY_FLASHSIZE": '"16MB"',
    "CONFIG_SPIRAM": "y",
    "CONFIG_SPIRAM_MODE_OCT": "y",
    "CONFIG_SPIRAM_ECC_ENABLE": "y",
    "CONFIG_SPIRAM_SPEED_80M": "y",
    "CONFIG_SPIRAM_SPEED": "80",
    "CONFIG_SPIRAM_BOOT_INIT": "y",
    "CONFIG_SPIRAM_MEMTEST": "y",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ": "240",
    "CONFIG_ESP_MAIN_TASK_STACK_SIZE": "8192",
    "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG": "y",
    "CONFIG_ESP_CONSOLE_SECONDARY_NONE": "y",
    "CONFIG_ESP_TASK_WDT_EN": "y",
    "CONFIG_ESP_TASK_WDT_INIT": "y",
    "CONFIG_ESP_TASK_WDT_PANIC": "y",
    "CONFIG_ESP_TASK_WDT_TIMEOUT_S": "2",
    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0": "y",
    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1": "y",
    "CONFIG_ESP_INT_WDT": "y",
    "CONFIG_ESP_INT_WDT_CHECK_CPU1": "y",
    "CONFIG_FREERTOS_HZ": "100",
    "CONFIG_PARTITION_TABLE_CUSTOM": "y",
    "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME": '"partitions.csv"',
    "CONFIG_PARTITION_TABLE_OFFSET": "0x8000",
}
FORBIDDEN = (
    "CONFIG_SPIRAM_IGNORE_NOTFOUND", "CONFIG_SPIRAM_SPEED_120M",
    "CONFIG_SPIRAM_MODE_QUAD", "CONFIG_ESP_CONSOLE_UART_DEFAULT",
    "CONFIG_ESP_CONSOLE_UART_CUSTOM", "CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG",
    "CONFIG_FREERTOS_UNICORE", "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE",
    "CONFIG_BOOTLOADER_APP_TEST", "CONFIG_SECURE_BOOT", "CONFIG_SECURE_FLASH_ENC_ENABLED",
)


def parse(text: str) -> dict[str, str]:
    result = {}
    for number, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        match = re.fullmatch(r"(CONFIG_[A-Z0-9_]+)=(y|n|m|0x[0-9a-fA-F]+|-?[0-9]+|\"[^\"\r\n]*\")", line)
        unset = re.fullmatch(r"# (CONFIG_[A-Z0-9_]+) is not set", line)
        if match:
            key, value = match.groups()
        elif unset:
            key, value = unset[1], "n"
        elif line.startswith("CONFIG_"):
            raise ValueError(f"잘못된 설정 문법: {number}")
        else:
            continue
        if key in result:
            raise ValueError(f"중복 설정: {key}")
        result[key] = value
    return result


def validate(text: str) -> None:
    config = parse(text)
    errors = [f"{key}: expected {value}, found {config.get(key, 'MISSING')}"
              for key, value in REQUIRED.items() if config.get(key) != value]
    errors += [f"{key}: bench에서 금지" for key in FORBIDDEN if config.get(key, "n") != "n"]
    if errors:
        raise ValueError("; ".join(errors))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sdkconfig", type=Path)
    args = parser.parse_args()
    try:
        validate(args.sdkconfig.read_text(encoding="utf-8"))
    except (ValueError, OSError) as error:
        print("FAIL:", error)
        return 1
    print("PASS: actual N16R8 bench sdkconfig; board/HIL/security provisioning NOT_RUN")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
