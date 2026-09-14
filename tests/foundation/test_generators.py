"""생성물 drift, 입력 손상, 메모리/pin 계약을 실제 source로 검증한다."""
import copy
import csv
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]


def load_script(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


TRANSPORT = load_script("generate_transport")
BOARDS = load_script("generate_boards")


class GeneratorTests(unittest.TestCase):
    def test_staging_rejects_sector_only_alignment(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        base = next(b for b in json.loads(manifest)["boards"] if b["id"] == "comm-r2-n16r8")
        source = BOARDS.canonical(ROOT / base["source"])
        for offset in range(4096, 65536, 4096):
            with self.subTest(offset=offset):
                board = copy.deepcopy(base)
                board["ota"]["data_offset"] += offset
                board["ota"]["data_size"] -= offset
                with self.assertRaisesRegex(ValueError, "OTA staging image alignment"):
                    BOARDS.board_outputs(board, manifest, source)

    def test_staging_sdk_contract(self):
        board = next(b for b in json.loads(BOARDS.SOURCE.read_bytes())["boards"] if b["id"] == "comm-r2-n16r8")
        generated = BOARDS.outputs()
        header = generated[board["path"] + "/bsp/board_pins.h"]
        self.assertIn(f'#define CANVIEW_BOARD_OTA_STAGING_OFFSET ({board["ota"]["data_offset"]}U)', header)
        self.assertIn(f'#define CANVIEW_BOARD_OTA_STAGING_BYTES ({board["ota"]["staging_size"]}U)', header)
        rows = list(csv.reader(line for line in generated[board["path"] + "/partitions.ota-template.csv"].splitlines()
                               if line and not line.startswith("#")))
        stage = next(row for row in rows if row[0] == "bundle_stage")
        self.assertEqual(stage[1:3], ["0x40", "3"])
        self.assertEqual(int(stage[3], 0), board["ota"]["data_offset"])
        self.assertEqual(int(stage[4], 0), board["ota"]["staging_size"])
        self.assertEqual(stage[5], "encrypted")
        for candidate in json.loads(BOARDS.SOURCE.read_bytes())["boards"]:
            if candidate["kind"] == "esp32s3":
                template = generated[candidate["path"] + "/partitions.ota-template.csv"]
                self.assertIn("recovery_app,app,test,", template)
                self.assertNotIn("\nrecovery,", template)

    def test_generated_files_exact(self):
        self.assertEqual(TRANSPORT.OUTPUT.read_text(encoding="utf-8"),
                         TRANSPORT.render(BOARDS.canonical(TRANSPORT.SOURCE)))
        for path, output in BOARDS.outputs().items():
            with self.subTest(path=path):
                self.assertEqual((ROOT / path).read_text(encoding="utf-8"), output)

    def test_board_profiles_are_generated_and_unique(self):
        manifest_bytes = BOARDS.canonical(BOARDS.SOURCE)
        manifest = json.loads(manifest_bytes)
        profiles = set()
        for board in manifest["boards"]:
            source = BOARDS.canonical(ROOT / board["source"])
            profile = BOARDS.board_profile(board, source)
            self.assertNotEqual(profile, 0)
            self.assertNotIn(profile, profiles)
            profiles.add(profile)
            header = ROOT / board["path"] / "bsp" / "board_pins.h"
            header_text = header.read_text(encoding="utf-8")
            self.assertIn(f"#define CANVIEW_BOARD_PROFILE (0x{profile:08X}U)", header_text)
            hardware_digest = hashlib.sha256(manifest_bytes + b"\n" + source).hexdigest()
            self.assertIn(f'#define CANVIEW_BOARD_HARDWARE_DIGEST "{hardware_digest}"', header_text)
            changed_board = copy.deepcopy(board)
            changed_board["required_nets"] = [*board["required_nets"], "PROFILE_MUTATION"]
            self.assertNotEqual(profile, BOARDS.board_profile(changed_board, source))
            self.assertNotEqual(profile, BOARDS.board_profile(board, source + b"\n"))
        with mock.patch.object(BOARDS, "board_profile", return_value=1):
            with self.assertRaises(ValueError):
                BOARDS.outputs()

    def test_transport_rejects_bad_contract(self):
        base = json.loads(TRANSPORT.SOURCE.read_bytes())
        changes = [
            (("schema_version",), 2), (("espnow", "max_frame"), 241),
            (("uart", "major"), 256), (("uart", "flags_mask"), -1),
            (("crc", "initial"), 0), (("can_batch", "max_records"), 13),
            (("uart", "baud"), 115200), (("espnow", "max_priority"), 9),
            (("uart", "header"), [["crc32", 32]]),
            (("uart", "header"), [base["uart"]["header"][1], base["uart"]["header"][0]] + base["uart"]["header"][2:]),
            (("espnow", "header"), base["espnow"]["header"] + [["magic", 2]])
        ]
        for keys, value in changes:
            with self.subTest(keys=keys):
                spec = copy.deepcopy(base)
                owner = spec
                for key in keys[:-1]:
                    owner = owner[key]
                owner[keys[-1]] = value
                with self.assertRaises(ValueError):
                    TRANSPORT.render(json.dumps(spec).encode())

    def test_board_rejects_bad_memory_or_path(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        spec = json.loads(manifest)
        base = spec["boards"][1]
        source = BOARDS.canonical(ROOT / base["source"])
        changes = [("path", "../escape"), ("kind", "unknown"),
                   ("flash_bytes", 4194304), ("psram_mode", "quad"),
                   ("factory_size", 16777216), ("recovery_gpio", 0),
                   ("module", "wrong-module"), ("required_nets", ["MISSING"])]
        for key, value in changes:
            with self.subTest(key=key):
                board = copy.deepcopy(base)
                board[key] = value
                with self.assertRaises(ValueError):
                    BOARDS.board_outputs(board, manifest, source)
        for key, value in (("slot1", 0x240000), ("staging_size", 0), ("data_size", 4096)):
            board = copy.deepcopy(base)
            board["ota"][key] = value
            with self.assertRaises(ValueError):
                BOARDS.board_outputs(board, manifest, source)
        bridge = copy.deepcopy(spec["boards"][2])
        bridge["flash_bytes"] = 16777216
        bridge["ota"]["data_size"] += 8388608
        with self.assertRaises(ValueError):
            BOARDS.board_outputs(bridge, manifest, BOARDS.canonical(ROOT / bridge["source"]))

    def test_bom_and_pin_conflicts(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        spec = json.loads(manifest)
        board = spec["boards"][0]
        source = json.loads(BOARDS.canonical(ROOT / board["source"]))
        source["pins"]["LCD_BL"] = 35
        with self.assertRaises(ValueError):
            BOARDS.board_outputs(board, manifest, json.dumps(source).encode())
        source["pins"]["LCD_BL"] = source["pins"]["LCD_MOSI"]
        with self.assertRaises(ValueError):
            BOARDS.board_outputs(board, manifest, json.dumps(source).encode())
        for board in spec["boards"][1:]:
            raw = BOARDS.canonical(ROOT / board["source"]).removeprefix(b"\xef\xbb\xbf")
            # Input provenance와 stale-source 방어 profile은 바뀌지만 실제 pin 계약은 같다.
            normal = BOARDS.board_outputs(board, manifest, raw)
            bom = BOARDS.board_outputs(board, manifest, b"\xef\xbb\xbf" + raw)
            header = board["path"] + "/bsp/board_pins.h"
            self.assertNotEqual(BOARDS.board_profile(board, raw),
                                BOARDS.board_profile(board, b"\xef\xbb\xbf" + raw))
            normal_contract = [line for line in normal[header].splitlines()
                               if "SHA256:" not in line and "CANVIEW_BOARD_PROFILE" not in line
                               and "CANVIEW_BOARD_HARDWARE_DIGEST" not in line]
            bom_contract = [line for line in bom[header].splitlines()
                            if "SHA256:" not in line and "CANVIEW_BOARD_PROFILE" not in line
                            and "CANVIEW_BOARD_HARDWARE_DIGEST" not in line]
            self.assertEqual(normal_contract, bom_contract)

    def test_bridge_usb_gpio_contract(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        spec = json.loads(manifest)
        board = spec["boards"][2]
        source = BOARDS.canonical(ROOT / board["source"])
        contract = copy.deepcopy(BOARDS.BRIDGE_USB_CONTRACT)
        contract["USB_DM"]["gpio"] = 18
        with mock.patch.object(BOARDS, "BRIDGE_USB_CONTRACT", contract), self.assertRaises(ValueError):
            BOARDS.board_outputs(board, manifest, source)

    def test_sdk_commits_full_length(self):
        manifest = json.loads((ROOT / "tools/toolchain-versions.json").read_text())
        for sdk in manifest["sdk"].values():
            self.assertRegex(sdk["gitCommit"], r"^[0-9a-f]{40}$")
        arm = manifest["tools"]["armGnuToolchain"]
        self.assertRegex(arm["archiveSha256"], r"^[0-9a-f]{64}$")
        self.assertIn("arm-none-eabi", arm["archiveUrl"])

    def test_controller_audio_direction(self):
        source = json.loads((ROOT / "firmware/boards/waveshare35-pins.json").read_bytes())
        # Waveshare 283ec84c: gpio_cfg.dout=16, gpio_cfg.din=14 (MCU perspective).
        self.assertEqual(source["pins"]["I2S_PLAY_DATA"], 16)
        self.assertEqual(source["pins"]["I2S_REC_DATA"], 14)

    def test_esp_pin_allowlist_json_and_csv(self):
        manifest = BOARDS.canonical(BOARDS.SOURCE)
        boards = json.loads(manifest)["boards"][:3]
        for board in boards:
            raw = BOARDS.canonical(ROOT / board["source"])
            # N8R2 exposes 35..37; R8 reserves all 26..37 for memory.
            forbidden = [-1, *range(22, 35 if board["psram_mode"] == "quad" else 38), 49]
            for pin in forbidden:
                with self.subTest(board=board["id"], pin=pin):
                    if board["reference"] is None:
                        data = json.loads(raw)
                        data["pins"]["LCD_BL"] = pin
                        changed = json.dumps(data).encode()
                    else:
                        rows = list(csv.DictReader(io.StringIO(raw.decode("utf-8-sig"))))
                        signal = "STATUS_LED" if board["psram_mode"] == "quad" else "ESP_RUN_OK"
                        row = next(row for row in rows if row["reference"] == board["reference"]
                                   and row["net"] == signal)
                        row["pin_name"] = f"IO{pin}" if pin >= 0 else "IO49"
                        buffer = io.StringIO()
                        writer = csv.DictWriter(buffer, fieldnames=rows[0].keys())
                        writer.writeheader()
                        writer.writerows(rows)
                        changed = buffer.getvalue().encode()
                    with self.assertRaises(ValueError):
                        BOARDS.board_outputs(board, manifest, changed)
        board = boards[0]
        for pin in (True, 6.0):
            data = json.loads(BOARDS.canonical(ROOT / board["source"]))
            data["pins"]["LCD_BL"] = pin
            with self.assertRaises(ValueError):
                BOARDS.board_outputs(board, manifest, json.dumps(data).encode())


if __name__ == "__main__":
    unittest.main()
