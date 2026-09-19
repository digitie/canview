"""Primary ELF 검사기의 오류/중첩/주소/BIN mutation 거절 시험."""
from dataclasses import replace
import struct
import unittest
from tools.ota.validate_stm32_map import (Section, VECTOR, PAYLOAD_MAX, STACK_TOP, VECTOR_BYTES,
                                         sections_from_objdump, validate)


class Stm32PrimaryMapTests(unittest.TestCase):
    def setUp(self):
        self.symbols = "08010200 R g_pfnVectors\n08010400 T Reset_Handler\n20018000 R _estack\n"
        self.memory = f"FLASH 0x{VECTOR:016x} 0x{PAYLOAD_MAX:016x} xr\n"
        self.image = struct.pack("<II", STACK_TOP, 0x08010401) + bytes(520 - 8)
        self.elf = self.image
        flags = frozenset(("ALLOC", "LOAD", "CONTENTS"))
        self.sections = [Section(".isr_vector", VECTOR_BYTES, VECTOR, VECTOR, 0, flags),
                         Section(".text", 8, VECTOR + 512, VECTOR + 512, 512, flags | {"CODE"})]

    def check(self, **changes):
        args = dict(sections=self.sections, symbols=self.symbols, map_text=self.memory,
                    elf=self.elf, image=self.image)
        args.update(changes)
        return validate(**args)

    def test_valid(self):
        self.assertEqual(self.check(), len(self.image))

    def test_ram_and_maximum_payload(self):
        image = self.image + b"ABCD"
        flags = frozenset(("ALLOC", "LOAD", "CONTENTS"))
        sections = self.sections + [Section(".data", 4, 0x20000000, VECTOR + 520, 520, flags),
                                    Section(".bss", 100, 0x20000004, 0x20000004, 0, frozenset({"ALLOC"}))]
        self.assertEqual(self.check(sections=sections, image=image, elf=image), len(image))
        for vma in (0x20018000, 0x10008000, 0x30000000):
            with self.subTest(vma=vma), self.assertRaises(ValueError):
                self.check(sections=sections[:-1] + [replace(sections[-1], vma=vma)], image=image, elf=image)
        image = self.image.ljust(PAYLOAD_MAX, b"\0")
        sections = [self.sections[0], replace(self.sections[1], size=PAYLOAD_MAX - 512)]
        self.assertEqual(self.check(sections=sections, image=image, elf=image), PAYLOAD_MAX)

    def test_layout_and_vectors(self):
        for memory in ("", self.memory * 2, self.memory.replace("08010200", "08000000"),
                       self.memory.replace("0002cc00", "00030000")):
            with self.subTest(memory=memory), self.assertRaises(ValueError):
                self.check(map_text=memory)
        for symbols in ("", self.symbols * 2, self.symbols.replace("g_pfnVectors", "unknown"),
                        self.symbols.replace("08010400", "08010402")):
            with self.subTest(symbols=symbols), self.assertRaises(ValueError):
                self.check(symbols=symbols)
        for offset in range(8):
            image = bytearray(self.image)
            image[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                self.check(image=image, elf=image)

    def test_section_mutations(self):
        cases = [[], self.sections + self.sections,
                 [replace(self.sections[0], size=VECTOR_BYTES - 4), self.sections[1]],
                 [replace(self.sections[0], vma=0x08000000), self.sections[1]],
                 [replace(self.sections[0], flags=frozenset()), self.sections[1]]]
        for changes in (dict(vma=0x08040000), dict(lma=0x08040000), dict(offset=99999),
                        dict(flags=frozenset({"CODE", "ALLOC"})), dict(lma=VECTOR),
                        dict(size=PAYLOAD_MAX), dict(flags=frozenset({"ALLOC", "LOAD", "CODE"})),
                        dict(vma=VECTOR + 513)):
            cases.append([self.sections[0], replace(self.sections[1], **changes)])
        cases.append(self.sections + [replace(self.sections[1], name=".overlap")])
        for sections in cases:
            with self.subTest(sections=sections), self.assertRaises(ValueError):
                self.check(sections=sections)

    def test_bin_bounds_and_content(self):
        for image in (b"", self.image[:7], self.image[:-1], self.image + b"\0", bytes(PAYLOAD_MAX + 1)):
            with self.subTest(size=len(image)), self.assertRaises(ValueError):
                self.check(image=image)
        with self.assertRaises(ValueError):
            self.check(elf=self.elf[:-1])
        with self.assertRaises(ValueError):
            self.check(image=self.image[:-1] + b"\1")
        image = bytearray(self.image)
        image[VECTOR_BYTES] = 1
        with self.assertRaises(ValueError):
            self.check(image=image, elf=image)

    def test_objdump_parser(self):
        text = "  0 .isr_vector 000001d8 08010200 08010200 00000000 2**0\n  CONTENTS, ALLOC, LOAD\n"
        self.assertEqual(sections_from_objdump(text), [self.sections[0]])
        for invalid in ("", text + text, text.splitlines()[0]):
            with self.assertRaises(ValueError):
                sections_from_objdump(invalid)


if __name__ == "__main__":
    unittest.main()
