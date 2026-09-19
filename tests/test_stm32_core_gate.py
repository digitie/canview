"""STM32 binary gate가 누락·과대·금지 입력을 성공으로 처리하지 않는지 검사한다."""
import unittest
from pathlib import Path
import sys
import tempfile
import struct
from copy import deepcopy

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.check_stm32_core import (check_build_id, check_compile_contract, check_memory, check_source_safety,
                                    check_stack, check_startup_ram, check_symbols,
                                    dmamux_model_assertions, stack_evidence)


class Stm32CoreGateTests(unittest.TestCase):
    @staticmethod
    def startup_fixture(base=0x08000000):
        # 실제 GNU Arm 15.3 instruction bytes. 상대 branch는 주소 이동에도 같다.
        image = bytearray(512)
        struct.pack_into("<II", image, 0, 0x20018000, base + 0x12D)
        image[0x100:0x12C] = bytes.fromhex(
            "4ff00052 0023 1360 bff34f8f 1360 bff34f8f "
            "02f50042 1368 02f50042 1368 02f58042 1368 bff34f8f 00f02ab8")
        image[0x12C:0x134] = bytes.fromhex("0d488546 fff7e6ff")
        struct.pack_into("<I", image, 0x164, 0x20018000)
        symbols = "\n".join(f"{address:08x} T {name}" for name, address in (
            ("Reset_Handler", base + 0x12C), ("__wrap_SystemInit", base + 0x100),
            ("SystemInit", base + 0x180), ("_estack", 0x20018000),
            ("_sccmram", 0x10000000), ("_eccmram", 0x10000000),
            ("startup_sram_dummy", 0x20000000), ("_sdata", 0x20000004)))
        return symbols, image

    def test_startup_ram_actual_sequence(self):
        for base in (0x08000000, 0x08010200):
            symbols, image = self.startup_fixture(base)
            self.assertTrue(check_startup_ram(symbols, image, base))
            for size in range(0x182):
                with self.subTest(base=base, truncated=size), self.assertRaises(RuntimeError):
                    check_startup_ram(symbols, image[:size], base)

    def test_startup_ram_instruction_and_vector_mutations(self):
        symbols, image = self.startup_fixture()
        for offset in (*range(8), *range(0x100, 0x134), *range(0x164, 0x168)):
            for bit in range(8):
                changed = bytearray(image)
                changed[offset] ^= 1 << bit
                with self.subTest(offset=offset, bit=bit), self.assertRaises(RuntimeError):
                    check_startup_ram(symbols, changed, 0x08000000)

    def test_startup_parity_and_first_write_loss(self):
        # 승인한 wrapper의 실제 Thumb memory 접근을 작은 SRAM fault 모형에서 해석한다.
        # CPU/버스 전체 emulator나 물리 parity qualification은 아니다.
        _, image = self.startup_fixture()

        def execute(code, parity, faulty_cuts):
            cuts = (0x20000000, 0x20008000, 0x20010000, 0x20014000)
            pending = {address for index, address in enumerate(cuts) if faulty_cuts & (1 << index)}
            written = set()
            touched = set()
            address = 0
            offset = 0
            while offset < len(code):
                word = struct.unpack_from("<H", code, offset)[0]
                offset += 2
                if word in (0xF04F, 0xF502, 0xF3BF):
                    second = struct.unpack_from("<H", code, offset)[0]
                    offset += 2
                    if word == 0xF04F:
                        self.assertEqual(second, 0x5200)
                        address = 0x20000000
                    elif word == 0xF502:
                        self.assertIn(second, (0x4200, 0x4280))
                        address += 0x8000 if second == 0x4200 else 0x4000
                    else:
                        self.assertEqual(second, 0x8F4F)
                elif word == 0x2300:
                    pass  # movs r3,#0
                elif word in (0x6013, 0x6813):
                    self.assertIn(address, cuts)
                    touched.add(address)
                    if word == 0x6013:
                        self.assertEqual(address, 0x20000000)  # 전용 dummy 외 쓰기 금지
                        if address not in pending:
                            written.add(address)
                    elif parity and address == 0x20000000 and address not in written:
                        raise RuntimeError("미초기화 SRAM parity read")
                    pending.discard(address)
                else:
                    self.fail(f"지원하지 않는 startup opcode {word:x}")
            self.assertFalse(pending)
            self.assertEqual(touched, set(cuts))
            if parity:
                self.assertIn(0x20000000, written)

        for parity in (False, True):
            for faulty_cuts in range(16):
                with self.subTest(parity=parity, cuts=faulty_cuts):
                    execute(image[0x100:0x128], parity, faulty_cuts)
        # 이전 read-only startup은 parity 활성 cold boot에서 실패해야 한다.
        with self.assertRaises(RuntimeError):
            execute(bytes.fromhex("4ff00052 1368"), True, 0)

    def test_startup_ram_symbols_and_ccm_fail_closed(self):
        symbols, image = self.startup_fixture()
        for line in symbols.splitlines():
            for changed in (symbols.replace(line, ""), symbols + "\n" + line,
                            symbols.replace(line, "00000000" + line[8:])):
                with self.subTest(changed=changed), self.assertRaises(RuntimeError):
                    check_startup_ram(changed, image, 0x08000000)
        with self.assertRaises(RuntimeError):
            check_startup_ram(symbols.replace("10000000 T _eccmram", "10000004 T _eccmram"),
                              image, 0x08000000)

    def test_dmamux_model_assertions_fail_closed(self):
        model = "#define LL_DMAMUX_REQ_USART2_RX (26U)\n#define LL_DMAMUX_REQ_USART2_TX (27U)\n"
        source = dmamux_model_assertions(model)
        self.assertIn("LL_DMAMUX_REQ_USART2_RX == (26U)", source)
        self.assertIn("LL_DMAMUX_REQ_USART2_TX == (27U)", source)
        self.assertEqual(source.count("? 1 : -1"), 2)
        # 변이 값도 상수 대조에서 삭제하지 않고 실제 SDK compiler 검사로 전달한다.
        self.assertIn("== (7U)", dmamux_model_assertions(model.replace("26U", "7U")))
        for invalid in ("", model.splitlines()[0], model + model,
                        model.replace("_TX", "_RX"), model.replace("_TX", "_OTHER"),
                        model.replace("27U", "invalid")):
            with self.subTest(model=invalid), self.assertRaises(RuntimeError):
                dmamux_model_assertions(invalid)

    def test_linked_build_id_binary_binding(self):
        digest = bytes(range(1, 21))
        note = struct.pack("<III4s", 4, 20, 3, b"GNU\0") + digest
        symbols = "08000210 R canview_stm_link_build_id"
        image = bytes(512) + note
        self.assertEqual(check_build_id(note, symbols, image), digest[:16].hex())
        primary_symbols = symbols.replace("08000210", "08010410")
        self.assertEqual(check_build_id(note, primary_symbols, image, 0x08010200), digest[:16].hex())
        for bad_base in (0, 0x08010000, 0x08040000, -1):
            with self.subTest(base=bad_base), self.assertRaises(RuntimeError):
                check_build_id(note, symbols, image, bad_base)
        with self.assertRaises(RuntimeError):
            check_build_id(note, symbols, image, 0x08010200)
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
