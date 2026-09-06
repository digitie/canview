"""STM32 binary gate가 누락·과대·금지 입력을 성공으로 처리하지 않는지 검사한다."""
import unittest
from pathlib import Path
import sys
import tempfile
from copy import deepcopy

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.check_stm32_core import check_memory, check_stack, check_symbols, stack_evidence


class Stm32CoreGateTests(unittest.TestCase):
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
                 "canview_stm_scheduler_step", "SysTick_Handler", "NMI_Handler", "HardFault_Handler"]
        symbols = "\n".join("08000000 T " + name for name in names)
        check_symbols(symbols)
        for name in names:
            with self.subTest(missing=name), self.assertRaises(RuntimeError):
                check_symbols(symbols.replace(name, "unrelated"))
        for name in ("malloc", "calloc", "realloc", "free", "_sbrk",
                     "HAL_FDCAN_AddMessageToTxFifoQ", "HAL_FDCAN_AddMessageToTxBuffer"):
            with self.subTest(forbidden=name), self.assertRaises(RuntimeError):
                check_symbols(symbols + "\n08000100 T " + name)

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
