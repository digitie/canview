"""리뷰 원문 fence와 실제 문서 navigation을 구별하는 회귀시험."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("document_links", ROOT / "tools/validate_document_links.py")
LINKS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LINKS)


class DocumentLinkFenceTests(unittest.TestCase):
    def test_fenced_raw_and_live_links(self):
        bodies = (
            '```text\n[raw](/F:/temporary/reviewer/file.c:20)\n```\n',
            '````text\n[raw](missing-a)\n```c\n[example](missing-b)\n```\n[raw](missing-c)\n````\n',
            '~~~text\n[raw](missing)\n```c\n~~~\n',
            '  ```text\n[raw](missing)\n   `````   \n',
        )
        for body in bodies:
            for newline in ("\n", "\r\n"):
                with self.subTest(body=body, newline=newline), tempfile.TemporaryDirectory() as directory:
                    root = Path(directory)
                    docs = root / "docs"
                    docs.mkdir()
                    (docs / "valid.md").write_text("# 정상\n", encoding="utf-8")
                    (docs / "review.md").write_text((body + '[live](valid.md)\n[broken](missing-live)\n')
                                                     .replace("\n", newline), encoding="utf-8")
                    errors, documents, count = LINKS.validate(root)
                    self.assertEqual(documents, 2)
                    self.assertEqual(count, 2)
                    self.assertEqual(len(errors), 1)
                    self.assertIn("missing-live", errors[0])

    def test_unclosed_fence_and_short_closer(self):
        self.assertEqual(LINKS.without_fences("before\n````text\nraw\n```\nstill raw\n"), "before\n")
        self.assertEqual(LINKS.without_fences("before\n~~~text\nraw\n"), "before\n")

    def test_invalid_backtick_info_is_not_a_fence(self):
        body = "```bad`info\n[live](missing)\n"
        self.assertEqual(LINKS.without_fences(body), body)

    def test_close_must_use_matching_character_and_only_space(self):
        body = "```text\n~~~\n[raw](missing)\n```suffix\n[raw](missing2)\n```\nlive\n"
        self.assertEqual(LINKS.without_fences(body), "live\n")


if __name__ == "__main__":
    unittest.main()
