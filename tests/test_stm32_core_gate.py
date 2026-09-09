"""STM32 binary gate가 누락·과대·금지 입력을 성공으로 처리하지 않는지 검사한다."""
import unittest
from pathlib import Path
import sys
import tempfile
import struct
from copy import deepcopy

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.check_stm32_core import (check_build_id, check_compile_contract, check_memory, check_source_safety,
                                    check_stack, check_symbols, stack_evidence)


class Stm32CoreGateTests(unittest.TestCase):
    def test_linked_build_id_binary_binding(self):
        digest = bytes(range(1, 21))
        note = struct.pack("<III4s", 4, 20, 3, b"GNU\0") + digest
        symbols = "08000210 R canview_stm_link_build_id"
        image = bytes(512) + note
        self.assertEqual(check_build_id(note, symbols, image), digest[:16].hex())
        for invalid_note in (b"", note[:-1], note + b"\0", bytes(36),
                             b"\x08" + note[1:], note[:16] + bytes(20)):
            with self.subTest(note=invalid_note), self.assertRaises(RuntimeError):
                check_build_id(invalid_note, symbols, image)
        for invalid_symbols in ("", symbols + "\n" + symbols,
                                symbols.replace("08000210", "08000211"),
                                symbols.replace("08000210", "00000000")):
            with self.subTest(symbols=invalid_symbols), self.assertRaises(RuntimeError):
                check_build_id(note, invalid_symbols, image)
        for invalid_image in (b"", image[:-1], image[:-1] + b"\0"):
            with self.subTest(image=invalid_image), self.assertRaises(RuntimeError):
                check_build_id(note, symbols, invalid_image)

    def test_memory_limits(self):
        self.assertEqual(check_memory("text data bss\n262140 4 98300"), (262140, 4, 98300))
        for text in ("262141 4 0", "0 4 98301", "-1 0 0", "0 -1 0", "0 0 -1"):
            with self.subTest(text=text), self.assertRaises(RuntimeError):
                check_memory(text)
        for text in ("", "text data bss", "1 2"):
            with self.subTest(text=text), self.assertRaises((ValueError, IndexError)):
                check_memory(text)

    def test_symbols_required_and_forbidden(self):
        names = ["canview_stm_clock_start", "canview_stm_watchdog_start",
                 "canview_stm_scheduler_step", "canview_stm_build_metadata_get",
                 "canview_stm_stack_watermark_arm", "canview_stm_stack_watermark_sample",
                 "canview_stm_service_policy_evaluate", "canview_stm_diagnostic_encode",
                 "canview_stm_capture_only_contract_anchor",
                 "SysTick_Handler", "NMI_Handler", "HardFault_Handler"]
        symbols = "\n".join("08000000 T " + name for name in names)
        check_symbols(symbols)
        for name in names:
            with self.subTest(missing=name), self.assertRaises(RuntimeError):
                check_symbols(symbols.replace(name, "unrelated"))
        for name in ("malloc", "calloc", "realloc", "free", "_sbrk",
                     "HAL_FDCAN_AddMessageToTxFifoQ", "HAL_FDCAN_AddMessageToTxBuffer"):
            with self.subTest(forbidden=name), self.assertRaises(RuntimeError):
                check_symbols(symbols + "\n08000100 T " + name)

    def test_capture_only_source_boundary(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            valid = root / "valid.c"
            valid.write_text("uint32_t clock = RCC->CCIPR;\n", encoding="utf-8")
            self.assertTrue(check_source_safety(root))
            valid.write_text("/* FDCAN1->TXBAR */\n// #define CANVIEW_STM_TX_PERMIT 1\n", encoding="utf-8")
            self.assertTrue(check_source_safety(root))
            for source in (
                    "void send(void) { HAL_FDCAN_AddMessageToTxFifoQ(); }\n",
                    "void send(void) { LL_FDCAN_EnableTxBufferRequest(); }\n",
                    "void send(void) { FDCAN1->TXBAR = 1U; }\n",
                    "void send(void) {\n FDCAN1\n ->TXBAR = 1U;\n}\n",
                    "void send(void) {\n FDCAN_GlobalTypeDef *bus = FDCAN1;\n bus->TXBAR = 1U;\n}\n",
                    "void send(void) { FDCAN1-> /* command */ TXBAR = 1U; }\n",
                    "void send(void) { FDCAN_GlobalTypeDef *bus = FDCAN1; bus-> /* command */ TXBAR = 1U; }\n",
                    "void send(void) { FDCAN1-> /" + "\\" + "\n* command *" + "\\" + "\n/ TXBAR = 1U; }\n",
                    "void send(void) { FDCAN_GlobalTypeDef *bus = FDCAN1; bus-> /" + "\\" + "\n* command *" + "\\" + "\n/ TXBAR = 1U; }\n",
                    "#undef CANVIEW_STM_TX_PERMIT\n",
                    "#define CANVIEW_STM_TX_PERMIT 1\n",
                    "#undef " + "\\" + "\nCANVIEW_STM_TX_PERMIT\n",
                    "#define " + "\\" + "\nCANVIEW_STM_TX_PERMIT 1\n",
                    "#undef /* contract */ CANVIEW_STM_TX_PERMIT\n",
                    "#define /* contract */ CANVIEW_STM_TX_PERMIT 1\n",
                    "#undef /" + "\\" + "\n* contract *" + "\\" + "\n/ CANVIEW_STM_TX_PERMIT\n",
                    "#define /" + "\\" + "\n* contract *" + "\\" + "\n/ CANVIEW_STM_TX_PERMIT 1\n"):
                valid.write_text(source, encoding="utf-8")
                with self.subTest(source=source), self.assertRaises(RuntimeError):
                    check_source_safety(root)

    def test_compile_contract_covers_every_c_unit(self):
        header = Path("F:/canview/interface/canview_build_mode.h")
        valid = [
            {"file": "main.c", "command":
             "cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1 -include F:/canview/firmware/../interface/canview_build_mode.h"},
            {"file": "core.c", "arguments": [
                "cc", "-DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1", "-include",
                "F:/canview/interface/canview_build_mode.h"]},
            {"file": "startup.s", "command": "as"},
        ]
        self.assertEqual(check_compile_contract(valid, header), 2)
        for bad in (
                [dict(valid[0], command="cc -include F:/canview/interface/canview_build_mode.h")],
                [dict(valid[0], command="cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1")],
                [dict(valid[0], command=
                      "cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1 -include F:/stale/canview_build_mode.h")],
                [dict(valid[0], command=(
                    "cc -DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1 -include stdint.h "
                    "-DHEADER_LABEL=canview_build_mode.h"))],
                [{"file": "main.c", "command": "cc"}],
                [{"file": "startup.s", "command": "as"}]):
            with self.subTest(bad=bad), self.assertRaises(RuntimeError):
                check_compile_contract(bad, header)

    def test_stack_fail_closed(self):
        self.assertEqual(check_stack(["source:1:function\t2048\tstatic", "source:2:f\t0\tdynamic,bounded"]), 2048)
        for lines in ([], ["x\t2049\tstatic"], ["x\t-1\tstatic"], ["x\t8\tdynamic"],
                      ["malformed"], ["x\t8\tstatic\textra"]):
            with self.subTest(lines=lines), self.assertRaises(RuntimeError):
                check_stack(lines)

    def test_partial_stack_evidence_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            build = Path(temporary).resolve()
            commands = []
            for name in ("main.c", "core_hw.c"):
                (build / (name + ".obj")).write_bytes(b"object fixture")
                (build / (name + ".su")).write_text("source:1:f\t32\tstatic\n", encoding="utf-8")
                commands.append({"file": name, "command": "cc -fstack-usage", "directory": str(build),
                                 "output": name + ".obj"})
            self.assertEqual(len(stack_evidence(build, commands)), 2)
            for name in ("main.c", "core_hw.c"):
                stack = build / (name + ".su")
                contents = stack.read_text(encoding="utf-8")
                stack.unlink()
                with self.subTest(missing=name), self.assertRaises(RuntimeError):
                    stack_evidence(build, commands)
                stack.write_text("", encoding="utf-8")
                with self.subTest(empty=name), self.assertRaises(RuntimeError):
                    stack_evidence(build, commands)
                with self.subTest(code_without_frame=name), self.assertRaises(RuntimeError):
                    stack_evidence(build, commands, lambda _: "function T 0 8")
                self.assertEqual(len(stack_evidence(build, commands, lambda _: "table R 0 4")), 2)
                stack.write_text(contents, encoding="utf-8")
            for field, value in (("command", "cc"), ("output", "missing.obj"), ("file", "unhandled.cpp")):
                bad = deepcopy(commands)
                bad[0][field] = value
                with self.subTest(field=field), self.assertRaises(RuntimeError):
                    stack_evidence(build, bad)
            with self.assertRaises(RuntimeError):
                stack_evidence(build, commands + commands)
            with self.assertRaises(RuntimeError):
                stack_evidence(build, [{"file": "startup.s"}])


if __name__ == "__main__":
    unittest.main()
