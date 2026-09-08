"""보드별 bench image의 실제 sdkconfig를 fail-closed로 검사한다. HIL 증거가 아니다."""
from __future__ import annotations
import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "CONFIG_IDF_TARGET": '"esp32s3"',
    "CONFIG_SPIRAM": "y",
    "CONFIG_SPIRAM_SPEED_80M": "y",
    "CONFIG_SPIRAM_SPEED": "80",
    "CONFIG_SPIRAM_BOOT_INIT": "y",
    "CONFIG_SPIRAM_MEMTEST": "y",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ": "240",
    "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240": "y",
    "CONFIG_ESP_MAIN_TASK_STACK_SIZE": "8192",
    "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG": "y",
    "CONFIG_ESP_CONSOLE_SECONDARY_NONE": "y",
    "CONFIG_ESP_TASK_WDT_EN": "y",
    "CONFIG_ESP_TASK_WDT_INIT": "y",
    "CONFIG_ESP_TASK_WDT_PANIC": "y",
    "CONFIG_ESP_TASK_WDT_TIMEOUT_S": "2",
    "CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT": "y",
    "CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS": "0",
    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0": "y",
    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1": "y",
    "CONFIG_ESP_INT_WDT": "y",
    "CONFIG_ESP_INT_WDT_CHECK_CPU1": "y",
    "CONFIG_FREERTOS_HZ": "100",
    "CONFIG_PARTITION_TABLE_CUSTOM": "y",
    "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME": '"partitions.csv"',
    "CONFIG_PARTITION_TABLE_OFFSET": "0x8000",
}
COMMON_FORBIDDEN = (
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
    "CONFIG_FLASH_ENCRYPTION_ENABLED", "CONFIG_APP_ROLLBACK_ENABLE",
    "CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT", "CONFIG_NVS_ENCRYPTION",
)
MEMORY = {
    "comm-r2-n16r8": {"CONFIG_ESPTOOLPY_FLASHSIZE_16MB": "y",
                       "CONFIG_ESPTOOLPY_FLASHSIZE": '"16MB"',
                       "CONFIG_SPIRAM_MODE_OCT": "y", "CONFIG_SPIRAM_ECC_ENABLE": "y"},
    "bridge-r1-n8r2": {"CONFIG_ESPTOOLPY_FLASHSIZE_8MB": "y",
                       "CONFIG_ESPTOOLPY_FLASHSIZE": '"8MB"', "CONFIG_SPIRAM_MODE_QUAD": "y"},
}
BRIDGE_REQUIRED = {
    "CONFIG_HTTPD_WS_SUPPORT": "y",
    "CONFIG_HTTPD_WS_PRE_HANDSHAKE_CB_SUPPORT": "y",
    "CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT": "y",
}
BRIDGE_FORBIDDEN = (
    "CONFIG_ESP_NETIF_BRIDGE_EN",
    "CONFIG_ESP_NETIF_L2_TAP",
    "CONFIG_LWIP_FORCE_ROUTER_FORWARDING",
    "CONFIG_LWIP_IPV6_FORWARD",
    "CONFIG_LWIP_IP_FORWARD",
)
BRIDGE_REQUIRED_DISABLED = BRIDGE_FORBIDDEN
FORBIDDEN = {
    "comm-r2-n16r8": COMMON_FORBIDDEN + ("CONFIG_SPIRAM_MODE_QUAD", "CONFIG_ESPTOOLPY_FLASHSIZE_8MB"),
    "bridge-r1-n8r2": COMMON_FORBIDDEN + ("CONFIG_SPIRAM_MODE_OCT", "CONFIG_SPIRAM_ECC_ENABLE",
                                         "CONFIG_ESPTOOLPY_FLASHSIZE_16MB") + BRIDGE_FORBIDDEN,
}

# IDF generated sdkconfig에서 항상 명시되는 중요한 비활성 항목은 누락도 거부한다.
# IDF가 symbol을 노출하지 않는 항목은 ALLOWED_KEYS가 active 추가를 거부한다.
REQUIRED_DISABLED = (
    "CONFIG_SPIRAM_IGNORE_NOTFOUND",
    "CONFIG_ESP_CONSOLE_UART_DEFAULT", "CONFIG_ESP_CONSOLE_USB_CDC",
    "CONFIG_ESP_CONSOLE_UART_CUSTOM", "CONFIG_ESP_CONSOLE_NONE",
    "CONFIG_FREERTOS_UNICORE", "CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE",
    "CONFIG_BOOTLOADER_APP_TEST", "CONFIG_SECURE_BOOT", "CONFIG_SECURE_FLASH_ENC_ENABLED",
    "CONFIG_BOOTLOADER_FACTORY_RESET", "CONFIG_EFUSE_VIRTUAL",
    "CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT", "CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT",
    "CONFIG_FLASH_ENCRYPTION_ENABLED", "CONFIG_APP_ROLLBACK_ENABLE",
    "CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT",
)
ALLOWLIST_PATH = ROOT / "tools" / "sdkconfig-allowlist" / "esp32s3-idf-6.0.3.keys"


def load_allowlisted_keys() -> frozenset[str]:
    """고정 IDF/toolchain에서 검토한 CONFIG key 집합을 읽는다."""
    keys = set()
    for number, line in enumerate(ALLOWLIST_PATH.read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if not re.fullmatch(r"CONFIG_[A-Z0-9_]+", line):
            raise ValueError(f"잘못된 sdkconfig allowlist 문법: {number}")
        keys.add(line)
    if not keys:
        raise ValueError("빈 sdkconfig allowlist")
    return frozenset(keys)


ALLOWED_KEYS = (load_allowlisted_keys() | frozenset(REQUIRED) | frozenset(BRIDGE_REQUIRED) |
                frozenset(key for values in MEMORY.values() for key in values) |
                frozenset(key for values in FORBIDDEN.values() for key in values))


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
        if key not in ALLOWED_KEYS:
            raise ValueError(f"검토하지 않은 sdkconfig key: {key}")
        if key in result:
            raise ValueError(f"중복 설정: {key}")
        result[key] = value
    return result


def validate(text: str, board: str = "comm-r2-n16r8") -> None:
    if board not in MEMORY:
        raise ValueError("검토하지 않은 bench board: " + board)
    config = parse(text)
    required = REQUIRED | MEMORY[board]
    if board == "bridge-r1-n8r2":
        required = required | BRIDGE_REQUIRED
    errors = [f"{key}: expected {value}, found {config.get(key, 'MISSING')}"
              for key, value in required.items() if config.get(key) != value]
    expected_flash_mb = 16 if board == "comm-r2-n16r8" else 8
    for flash_mb in (1, 2, 4, 8, 16, 32, 64, 128):
        key = f"CONFIG_ESPTOOLPY_FLASHSIZE_{flash_mb}MB"
        expected = "y" if flash_mb == expected_flash_mb else "n"
        if config.get(key, "n") != expected:
            errors.append(f"{key}: expected {expected}, found {config.get(key, 'MISSING')}")
    errors += [f"{key}: expected explicit n, found {config.get(key, 'MISSING')}"
               for key in REQUIRED_DISABLED if config.get(key) != "n"]
    if board == "bridge-r1-n8r2":
        errors += [f"{key}: Bridge에서 expected explicit n, found {config.get(key, 'MISSING')}"
                   for key in BRIDGE_REQUIRED_DISABLED if config.get(key) != "n"]
    errors += [f"{key}: bench에서 금지" for key in FORBIDDEN[board] if config.get(key, "n") != "n"]
    if errors:
        raise ValueError("; ".join(errors))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sdkconfig", type=Path)
    parser.add_argument("--board", choices=tuple(MEMORY), default="comm-r2-n16r8")
    args = parser.parse_args()
    try:
        validate(args.sdkconfig.read_text(encoding="utf-8"), args.board)
    except (ValueError, OSError) as error:
        print("FAIL:", error)
        return 1
    print(f"PASS: actual {args.board} bench sdkconfig; board/HIL/security provisioning NOT_RUN")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
