"""계획 validator의 음성 fixture와 읽기 전용 동작을 검증한다."""

from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "tools/validate_plan.py"
SPEC = importlib.util.spec_from_file_location("canview_plan_validation", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
PLAN = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PLAN
SPEC.loader.exec_module(PLAN)


class PlanValidationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="canview-plan-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "docs/tasks").mkdir(parents=True)
        self.items = [
            {"id": "T-001", "title": "기반", "status": "READY", "priority": "P0", "deps": "없음"},
            {"id": "T-002", "title": "통신", "status": "BLOCKED", "priority": "P0", "deps": "T-001"},
            {"id": "T-002a", "title": "확장", "status": "BLOCKED", "priority": "P1", "deps": "T-002"},
        ]
        self.write_repo()

    def path(self, identifier):
        return self.root / "docs/tasks" / f"{identifier}-fixture.md"

    def row(self, item):
        return (f"| [{item['id']}](tasks/{item['id']}-fixture.md) | {item['status']} | "
                f"{item['priority']} | {item['title']} | {item['deps']} |\n")

    def write_repo(self):
        for item in self.items:
            text = (f"# {item['id']} {item['title']}\n\n"
                    f"- 상태: `{item['status']}`\n- 우선순위: `{item['priority']}`\n"
                    f"- Gate: `G0`\n- 선행: {item['deps']}\n")
            self.path(item["id"]).write_text(text, encoding="utf-8")
        header = "| ID | 상태 | 우선순위 | 작업 | 선행 |\n|---|---|---|---|---|\n"
        opened = "# 작업\n\n3개의 상세 작업이 있다.\n\n" + header
        done = "# 완료\n\n" + header
        for item in self.items:
            if item["status"] == "DONE":
                done += self.row(item)
            else:
                opened += self.row(item)
        (self.root / "docs/tasks.md").write_text(opened, encoding="utf-8")
        (self.root / "docs/tasks-done.md").write_text(done, encoding="utf-8")

    def edit(self, path, old, new):
        body = path.read_text(encoding="utf-8")
        self.assertIn(old, body)
        path.write_text(body.replace(old, new), encoding="utf-8")

    def invalid(self, expected):
        errors, count = PLAN.validate(self.root)
        self.assertTrue(errors)
        self.assertTrue(any(expected in error for error in errors), errors)
        return count

    def test_valid_suffix_and_dag(self):
        self.assertEqual(PLAN.validate(self.root), ([], 3))

    def test_done_archive_and_ready_dependency(self):
        self.items[0]["status"] = "DONE"
        self.items[1]["status"] = "READY"
        self.write_repo()
        self.assertEqual(PLAN.validate(self.root), ([], 3))

    def test_duplicate_detail_id(self):
        other = self.path("T-002").with_name("T-002-other.md")
        other.write_text(self.path("T-002").read_text(encoding="utf-8"), encoding="utf-8")
        self.invalid("중복 상세 ID")

    def test_malformed_filename(self):
        self.path("T-002a").rename(self.path("T-22a"))
        self.invalid("파일명/ID")

    def test_header_id_mismatch(self):
        self.edit(self.path("T-002"), "# T-002 ", "# T-009 ")
        self.invalid("상세 제목 ID/파일명")

    def test_extra_h1(self):
        self.edit(self.path("T-002"), "# T-002 통신", "# T-002 통신\n# T-002 통신")
        self.invalid("H1 개수")

    def test_missing_metadata(self):
        for field in ("상태", "우선순위", "Gate", "선행"):
            with self.subTest(field=field):
                self.write_repo()
                path = self.path("T-002")
                body = path.read_text(encoding="utf-8")
                path.write_text("\n".join(line for line in body.splitlines()
                                          if not line.startswith(f"- {field}:")), encoding="utf-8")
                self.invalid(f"{field} 필드")

    def test_duplicate_metadata(self):
        self.edit(self.path("T-002"), "- 상태: `BLOCKED`",
                  "- 상태: `BLOCKED`\n- 상태: `BLOCKED`")
        self.invalid("상태 필드")

    def test_unknown_status(self):
        self.edit(self.path("T-002"), "`BLOCKED`", "`COMPLETE`")
        self.invalid("알 수 없는 상태")

    def test_unknown_priority(self):
        self.edit(self.path("T-002"), "`P0`", "`P9`")
        self.invalid("알 수 없는 우선순위")

    def test_dangling_dependency(self):
        self.edit(self.path("T-002"), "- 선행: T-001", "- 선행: T-999")
        self.invalid("없는 선행")

    def test_duplicate_dependency(self):
        self.edit(self.path("T-002"), "- 선행: T-001", "- 선행: T-001, T-001")
        self.invalid("중복 선행")

    def test_external_dependency_must_be_separate(self):
        self.edit(self.path("T-002"), "- 선행: T-001", "- 선행: T-001, 실물 PCB")
        self.invalid("선행 문법")

    def test_external_condition_accepted(self):
        self.edit(self.path("T-002"), "- 선행: T-001",
                  "- 선행: T-001\n- 외부 선행: 실물 PCB, 승인된 시험")
        self.assertEqual(PLAN.validate(self.root), ([], 3))

    def test_self_dependency(self):
        self.edit(self.path("T-002"), "- 선행: T-001", "- 선행: T-002")
        self.invalid("자기 선행")
        self.invalid("DAG 사이클")

    def test_cycle_without_self_reference(self):
        self.items[0]["status"] = "BLOCKED"
        self.items[0]["deps"] = "T-002a"
        self.write_repo()
        self.invalid("DAG 사이클")

    def test_ready_with_unfinished_dependency(self):
        self.items[1]["status"] = "READY"
        self.write_repo()
        self.invalid("READY인데 선행")

    def test_done_with_unfinished_dependency(self):
        self.items[1]["status"] = "DONE"
        self.write_repo()
        self.invalid("DONE인데 선행")

    def test_count_mismatch(self):
        self.edit(self.root / "docs/tasks.md", "3개의 상세 작업", "35개의 상세 작업")
        self.invalid("상세 task 수 불일치")

    def test_missing_or_duplicate_count(self):
        for replacement in ("개수 미기재", "3개의 상세 작업, 3개의 상세 작업"):
            with self.subTest(replacement=replacement):
                self.write_repo()
                self.edit(self.root / "docs/tasks.md", "3개의 상세 작업", replacement)
                self.invalid("상세 task 수 불일치")

    def test_summary_fields_mismatch(self):
        for old, new, expected in (
            ("| 통신 |", "| 다른 제목 |", "제목"),
            ("| BLOCKED | P0 | 통신", "| READY | P0 | 통신", "상태"),
            ("| P0 | 통신", "| P2 | 통신", "우선순위"),
            ("| 통신 | T-001 |", "| 통신 | 없음 |", "선행"),
        ):
            with self.subTest(field=expected):
                self.write_repo()
                self.edit(self.root / "docs/tasks.md", old, new)
                self.invalid(f"요약 {expected} 불일치")

    def test_summary_missing(self):
        self.edit(self.root / "docs/tasks.md", self.row(self.items[1]), "")
        self.invalid("요약 누락")

    def test_summary_duplicate(self):
        row = self.row(self.items[1])
        self.edit(self.root / "docs/tasks.md", row, row + row)
        self.invalid("중복 요약 ID")

    def test_summary_without_detail(self):
        self.edit(self.root / "docs/tasks.md", "[T-002a]", "[T-999]")
        self.invalid("상세 파일 없는 요약")

    def test_wrong_summary_link(self):
        self.edit(self.root / "docs/tasks.md", "(tasks/T-002-fixture.md)",
                  "(tasks/T-001-fixture.md)")
        self.invalid("상세 link 불일치")

    def test_summary_bad_column_count(self):
        self.edit(self.root / "docs/tasks.md", "| 통신 | T-001 |", "| 통신 | T-001 | extra |")
        self.invalid("요약 5열")

    def test_done_in_open_summary_rejected(self):
        self.items[0]["status"] = "DONE"
        self.write_repo()
        row = self.row(self.items[0])
        self.edit(self.root / "docs/tasks-done.md", row, "")
        path = self.root / "docs/tasks.md"
        path.write_text(path.read_text(encoding="utf-8") + row, encoding="utf-8")
        self.invalid("요약/archive 위치")

    def test_blocked_in_archive_rejected(self):
        row = self.row(self.items[1])
        self.edit(self.root / "docs/tasks.md", row, "")
        path = self.root / "docs/tasks-done.md"
        path.write_text(path.read_text(encoding="utf-8") + row, encoding="utf-8")
        self.invalid("요약/archive 위치")

    def test_missing_document(self):
        (self.root / "docs/tasks-done.md").unlink()
        self.invalid("읽기 실패")

    def test_fenced_examples_ignored(self):
        self.edit(self.path("T-002"), "- Gate: `G0`",
                  "- Gate: `G0`\n\n```text\n# T-999 예시\n- 상태: BAD\n```")
        self.assertEqual(PLAN.validate(self.root), ([], 3))

    def test_dependency_order_is_not_a_mismatch(self):
        self.items[2]["deps"] = "T-001, T-002"
        self.write_repo()
        self.edit(self.root / "docs/tasks.md", "| T-001, T-002 |", "| T-002, T-001 |")
        self.assertEqual(PLAN.validate(self.root), ([], 3))

    def test_empty_repository_rejected(self):
        empty = self.root / "empty"
        empty.mkdir()
        errors, count = PLAN.validate(empty)
        self.assertEqual(count, 0)
        self.assertTrue(any("파일이 없음" in error for error in errors))

    def test_invalid_utf8_rejected(self):
        self.path("T-002").write_bytes(b"\xff\xfe")
        self.invalid("읽기 실패")

    def test_read_only_on_success_and_failure(self):
        def snapshot():
            return {str(path.relative_to(self.root)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in self.root.rglob("*") if path.is_file()}
        for broken in (False, True):
            with self.subTest(broken=broken):
                if broken:
                    self.edit(self.root / "docs/tasks.md", "3개의 상세 작업", "4개의 상세 작업")
                before = snapshot()
                PLAN.validate(self.root)
                self.assertEqual(snapshot(), before)

    def test_cli_exit_codes_and_no_mutation(self):
        command = [sys.executable, "-B", "-X", "utf8", str(SCRIPT), "--root", str(self.root)]
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("상세 task=3, 오류=0", result.stdout)
        self.edit(self.root / "docs/tasks.md", "3개의 상세 작업", "4개의 상세 작업")
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", check=False)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
