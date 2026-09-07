"""핀 전개표와 board manifest에서 review용 BSP 설정을 생성한다. 제작 승인은 별도."""
from __future__ import annotations
import argparse
import csv
import hashlib
import io
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "firmware/boards/boards.json"

# Fixed ESP-IDF GPIO contract and reviewed module pad exposure; not a generic SoC BSP.
# docs/en/api-reference/peripherals/gpio/esp32s3.inc at IDF 76f5dedd.
ESP_GPIO_VALID = frozenset(range(22)) | frozenset(range(26, 49))
ESP_MODULE_GPIO = {
    "ESP32-S3R8": ESP_GPIO_VALID - frozenset(range(26, 38)),
    "ESP32-S3-WROOM-1-N16R8": frozenset(range(22)) | frozenset(range(38, 49)),
    "ESP32-S3-WROOM-1-N8R2": frozenset(range(22)) | frozenset(range(35, 49)),
}


def canonical(path: Path) -> bytes:
    return path.read_bytes().replace(b"\r\n", b"\n")


def board_outputs(board: dict, manifest: bytes, source: bytes) -> dict[str, str]:
    for key in ("path", "source"):
        path = Path(board[key])
        if path.is_absolute() or ".." in path.parts or "\\" in board[key] or ":" in board[key]:
            raise ValueError("repository-relative paths only")
    if not board["path"].startswith("firmware/") or not re.fullmatch(r"[a-z0-9_-]+", board["id"]):
        raise ValueError("board output path/id")
    if board["kind"] == "esp32s3":
        memory = {"ESP32-S3R8": (16777216, 8388608),
                  "ESP32-S3-WROOM-1-N16R8": (16777216, 8388608),
                  "ESP32-S3-WROOM-1-N8R2": (8388608, 2097152)}
        if memory.get(board["module"]) != (board["flash_bytes"], board["psram_bytes"]):
            raise ValueError("board/module memory contract")
        if board["id"] == "comm-r2-n16r8" and (board.get("core_profile") != "bench-health-v1" or
                board["module"] != "ESP32-S3-WROOM-1-N16R8" or not board["psram_ecc"]):
            raise ValueError("Communicator bench health/ECC contract")
        if "core_profile" in board and board["id"] != "comm-r2-n16r8":
            raise ValueError("unreviewed core profile")
        if board["flash_bytes"] not in (8388608, 16777216):
            raise ValueError("unreviewed flash size")
        if (board["psram_mode"], board["psram_bytes"]) not in (("octal", 8388608), ("quad", 2097152)):
            raise ValueError("PSRAM mode/size mismatch")
        if board["psram_ecc"] and board["psram_mode"] != "octal":
            raise ValueError("ECC requires octal")
        if board["factory_size"] <= 0 or board["factory_size"] % 65536 or 0x20000 + board["factory_size"] > board["flash_bytes"]:
            raise ValueError("factory exceeds flash/alignment")
        ota = board["ota"]
        end = 0x35000
        for start_key, size_key, alignment in (("recovery_offset", "recovery_size", 65536),
                ("slot0", "slot_size", 65536), ("slot1", "slot_size", 65536),
                ("data_offset", "data_size", 4096)):
            start, size = ota[start_key], ota[size_key]
            if start < end or start % alignment or size <= 0 or size % alignment:
                raise ValueError("OTA overlap/alignment")
            end = start + size
        if end != board["flash_bytes"]:
            raise ValueError("OTA flash extent")
        if "staging_size" in ota and (not 0 < ota["staging_size"] < ota["data_size"] or ota["staging_size"] % 4096):
            raise ValueError("OTA staging size")
    elif board["kind"] == "stm32g474":
        clock = board["clock"]
        if (board["flash_bytes"], board["sram_bytes"], board["ccm_bytes"]) != (524288, 98304, 32768):
            raise ValueError("G474CE memory contract")
        if clock != {"hse_hz": 16000000, "pll_m": 4, "pll_n": 80, "pll_r": 2,
                     "pll_q": 4, "sysclk_hz": 160000000, "pclk1_hz": 80000000,
                     "fdcan_hz": 80000000, "uart_baud": 4000000, "uart_brr": 20}:
            raise ValueError("clock plan changed; re-review timing before generation")
    else:
        raise ValueError("unknown board kind")
    digest = hashlib.sha256(manifest + b"\n" + source).hexdigest()
    prefix = "CANVIEW_BOARD_"
    lines = ["/* DO NOT EDIT. generate_boards.py v1; GPL-3.0-only",
             f" * board+pin input SHA256: {digest}",
             " * Review board contract; not fabrication/vehicle approval. */",
             "#ifndef CANVIEW_BOARD_PINS_H", "#define CANVIEW_BOARD_PINS_H", "",
             f'#define {prefix}ID "{board["id"]}"',
             f'#define {prefix}MODULE "{board["module"]}"',
             f'#define {prefix}FLASH_BYTES ({board["flash_bytes"]}U)']
    nets = {}
    if board["reference"] is None:
        data = json.loads(source)
        nets = {net: ("GPIO", pin) for net, pin in data["pins"].items()}
        for name, value in {**data["lcd"], **data["i2c"]}.items():
            lines.append(f"#define {prefix}{name.upper()} ({value}U)")
    else:
        rows = list(csv.DictReader(io.StringIO(source.decode("utf-8-sig"))))
        for row in rows:
            if row["reference"] != board["reference"] or row["net"] == "NC":
                continue
            if row["mpn"] != board["module"]:
                raise ValueError("module differs from circuit: " + board["id"])
            gpio = re.fullmatch(r"IO(\d+)", row["pin_name"])
            stm = re.fullmatch(r"P([A-G])(\d+)", row["pin_name"])
            if gpio:
                value = ("GPIO", int(gpio[1]))
            elif stm:
                value = (stm[1], int(stm[2]))
            elif row["pin_name"] in ("USB_D-", "USB_D+"):
                value = ("GPIO", 19 if row["pin_name"] == "USB_D-" else 20)
            else:
                continue
            if row["net"] in nets:
                raise ValueError("duplicate GPIO net")
            nets[row["net"]] = value
    if not set(board["required_nets"]) <= set(nets):
        raise ValueError("missing required signal: " + board["id"])
    if board["kind"] == "esp32s3":
        recovery_net = "PAIR_BUTTON_N" if board["id"].startswith("bridge-") else "RECOVERY_BUTTON_N"
        if nets.get(recovery_net) != ("GPIO", board["recovery_gpio"]):
            raise ValueError("recovery GPIO differs from pinmap")
    used = set()
    for net, (port, pin) in sorted(nets.items()):
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", net):
            raise ValueError("invalid C signal")
        if type(pin) is not int:
            raise ValueError("GPIO number must be an integer, not bool/float")
        if (port, pin) in used:
            raise ValueError("duplicate GPIO")
        used.add((port, pin))
        if port == "GPIO":
            if (board["kind"] != "esp32s3" or pin not in ESP_GPIO_VALID or
                    pin not in ESP_MODULE_GPIO[board["module"]]):
                raise ValueError("reserved/unavailable ESP GPIO")
            lines.append(f"#define {prefix}{net}_GPIO ({pin}U)")
        else:
            if board["kind"] != "stm32g474" or not 0 <= pin <= 15:
                raise ValueError("invalid STM pin")
            lines += [f"#define {prefix}{net}_PORT ({ord(port) - ord('A')}U)",
                      f"#define {prefix}{net}_PIN ({pin}U)"]
    for key in ("psram_bytes", "sram_bytes", "ccm_bytes", "recovery_gpio"):
        if key in board:
            lines.append(f"#define {prefix}{key.upper()} ({board[key]}U)")
    for key, value in board.get("clock", {}).items():
        lines.append(f"#define {prefix}{key.upper()} ({value}U)")
    result = {board["path"] + "/bsp/board_pins.h": "\n".join(lines + ["", "#endif", ""])}
    if board["kind"] == "esp32s3":
        flash_mb = board["flash_bytes"] // 1048576
        sdk = [f"# DO NOT EDIT. generate_boards.py v1; input {digest}",
               'CONFIG_IDF_TARGET="esp32s3"',
               f"CONFIG_ESPTOOLPY_FLASHSIZE_{flash_mb}MB=y",
               f'CONFIG_ESPTOOLPY_FLASHSIZE="{flash_mb}MB"',
               "CONFIG_PARTITION_TABLE_CUSTOM=y",
               'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"',
               "CONFIG_PARTITION_TABLE_OFFSET=0x8000",
               "CONFIG_NVS_ENCRYPTION=y", "CONFIG_SPIRAM=y",
               "CONFIG_SPIRAM_MODE_OCT=y" if board["psram_mode"] == "octal" else "CONFIG_SPIRAM_MODE_QUAD=y",
               "CONFIG_SPIRAM_ECC_ENABLE=y" if board["psram_ecc"] else "# CONFIG_SPIRAM_ECC_ENABLE is not set",
               "CONFIG_SPIRAM_SPEED_80M=y",
               "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y",
               "CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240",
               "CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192",
               "CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y",
               "# CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is not set",
               "# CONFIG_BOOTLOADER_APP_TEST is not set"]
        if board.get("core_profile") == "bench-health-v1":
            sdk += ["CONFIG_ESP_TASK_WDT_EN=y", "CONFIG_ESP_TASK_WDT_INIT=y",
                    "CONFIG_ESP_TASK_WDT_PANIC=y", "CONFIG_ESP_TASK_WDT_TIMEOUT_S=2",
                    "CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT=y",
                    "CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS=0",
                    "# CONFIG_BOOTLOADER_FACTORY_RESET is not set",
                    "# CONFIG_EFUSE_VIRTUAL is not set",
                    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y",
                    "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=y",
                    "CONFIG_ESP_INT_WDT=y", "CONFIG_ESP_INT_WDT_CHECK_CPU1=y",
                    "CONFIG_SPIRAM_MEMTEST=y", "CONFIG_SPIRAM_BOOT_INIT=y",
                    "# CONFIG_SPIRAM_IGNORE_NOTFOUND is not set",
                    "CONFIG_ESP_CONSOLE_SECONDARY_NONE=y", "CONFIG_FREERTOS_HZ=100"]
        result[board["path"] + "/sdkconfig.defaults"] = "\n".join(sdk + [""])
        result[board["path"] + "/partitions.csv"] = "\n".join([
            "# DO NOT EDIT. Foundation bench-only factory image, NOT OTA layout.",
            "# Name,Type,SubType,Offset,Size,Flags",
            "nvs,data,nvs,0x9000,0x6000,",
            "phy_init,data,phy,0xF000,0x1000,",
            "nvs_keys,data,nvs_keys,0x10000,0x1000,encrypted",
            f'factory,app,factory,0x20000,0x{board["factory_size"]:X},', ""])
        ota = board["ota"]
        rows = [
            "# DO NOT EDIT. OTA DESIGN TEMPLATE ONLY. Not selected by sdkconfig.defaults.",
            "# Requires T-204 recovery/rollback/selftest; partition table offset=0x18000.",
            "# Name,Type,SubType,Offset,Size,Flags",
            "nvs,data,nvs,0x19000,0x6000,", "otadata,data,ota,0x1F000,0x2000,",
            "phy_init,data,phy,0x21000,0x1000,", "nvs_keys,data,nvs_keys,0x22000,0x1000,encrypted",
            "ota_journal_a,0x40,0,0x23000,0x1000,", "ota_journal_b,0x40,0,0x24000,0x1000,",
            "config_a,0x40,1,0x25000,0x4000,", "config_b,0x40,1,0x29000,0x4000,",
            "provision_a,0x40,2,0x2D000,0x4000,", "provision_b,0x40,2,0x31000,0x4000,",
            f'recovery,app,test,0x{ota["recovery_offset"]:X},0x{ota["recovery_size"]:X},',
            f'ota_0,app,ota_0,0x{ota["slot0"]:X},0x{ota["slot_size"]:X},',
            f'ota_1,app,ota_1,0x{ota["slot1"]:X},0x{ota["slot_size"]:X},']
        if "staging_size" in ota:
            rows += [f'bundle_stage,0x40,3,0x{ota["data_offset"]:X},0x{ota["staging_size"]:X},',
                     f'cache,0x40,4,0x{ota["data_offset"] + ota["staging_size"]:X},0x{ota["data_size"] - ota["staging_size"]:X},']
        else:
            rows.append(f'cache,0x40,4,0x{ota["data_offset"]:X},0x{ota["data_size"]:X},')
        result[board["path"] + "/partitions.ota-template.csv"] = "\n".join(rows + [""])
    return result


def outputs() -> dict[str, str]:
    manifest = canonical(SOURCE)
    spec = json.loads(manifest)
    if spec["schema_version"] != 1:
        raise ValueError("board schema version")
    result = {}
    for board in spec["boards"]:
        generated = board_outputs(board, manifest, canonical(ROOT / board["source"]))
        if set(result) & set(generated):
            raise ValueError("duplicate output")
        result.update(generated)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    drift = []
    for relative, data in outputs().items():
        path = ROOT / relative
        if args.check:
            if not path.exists() or path.read_text(encoding="utf-8") != data:
                drift.append(relative)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(data, encoding="utf-8", newline="\n")
    if drift:
        print("FAIL: board output drift", *drift, sep="\n")
        return 1
    print("PASS: board generation" + (" check" if args.check else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
