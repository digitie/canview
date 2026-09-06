"""생성물 drift, 입력 손상, 메모리/pin 계약을 실제 source로 검증한다."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load_script(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "tools" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


TRANSPORT = load_script("generate_transport")
BOARDS = load_script("generate_boards")


class GeneratorTests(unittest.TestCase):
    def test_generated_files_exact(self):
        self.assertEqual(TRANSPORT.OUTPUT.read_text(encoding="utf-8"),
                         TRANSPORT.render(BOARDS.canonical(TRANSPORT.SOURCE)))
        for path, output in BOARDS.outputs().items():
            with self.subTest(path=path):
                self.assertEqual((ROOT / path).read_text(encoding="utf-8"), output)

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
            # Input provenance digest changes, actual pin lines do not.
            normal = BOARDS.board_outputs(board, manifest, raw)
            bom = BOARDS.board_outputs(board, manifest, b"\xef\xbb\xbf" + raw)
            header = board["path"] + "/bsp/board_pins.h"
            self.assertEqual(normal[header].splitlines()[2:], bom[header].splitlines()[2:])

    def test_sdk_commits_full_length(self):
        manifest = json.loads((ROOT / "tools/toolchain-versions.json").read_text())
        for sdk in manifest["sdk"].values():
            self.assertRegex(sdk["gitCommit"], r"^[0-9a-f]{40}$")


if __name__ == "__main__":
    unittest.main()
