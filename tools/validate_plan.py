"""상세 task와 요약의 metadata·선행 DAG를 파일 변경 없이 검사한다.

예정 코드/시험 artifact의 존재나 G0–G6 통과를 판정하지 않는다.
DONE 상세 파일은 유지하고 같은 5열 요약을 tasks-done.md로 옮긴다.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys
from urllib.parse import unquote


ID_PATTERN = r"T-\d{3}[a-z]?"
ID_RE = re.compile(ID_PATTERN)
FILE_RE = re.compile(rf"({ID_PATTERN})-[A-Za-z0-9][A-Za-z0-9_.-]*\.md")
HEADER_RE = re.compile(rf"# ({ID_PATTERN}) (\S.*)")
STATUSES = {"READY", "BLOCKED", "IN_PROGRESS", "DONE"}
PRIORITIES = {"P0", "P1", "P2", "P3"}


@dataclass(frozen=True)
class Task:
    identifier: str
    path: Path
    title: str
    status: str
    priority: str
    dependencies: tuple[str, ...]


@dataclass(frozen=True)
class Summary:
    identifier: str
    path: Path
    target: str
    title: str
    status: str
    priority: str
    dependencies: tuple[str, ...]


def document(path: Path, errors: list[str]) -> str:
    try:
        body = path.read_text(encoding="utf-8-sig")
    except (OSError, UnicodeError) as exc:
        errors.append(f"{path}: 읽기 실패: {exc}")
        return ""
    # 명령 예시/과거 example의 metadata는 실제 entry가 아니다.
    return re.sub(r"^`{3,}[^\n]*\n.*?^`{3,}\s*$", "", body,
                  flags=re.MULTILINE | re.DOTALL)


def scalar(body: str, field: str, path: Path, errors: list[str]) -> str:
    matches = re.findall(rf"^- {re.escape(field)}: (.*)$", body, re.MULTILINE)
    if len(matches) != 1 or not matches[0].strip():
        errors.append(f"{path}: {field} 필드는 정확히 1개 필요")
        return ""
    return matches[0].strip().strip("`")


def dependencies(value: str, path: Path, errors: list[str]) -> tuple[str, ...]:
    if value == "없음":
        return ()
    tokens = [part.strip().strip("`") for part in value.split(",")]
    if not tokens or any(ID_RE.fullmatch(token) is None for token in tokens):
        errors.append(f"{path}: 선행 문법 오류 (ID 목록 또는 없음; 외부 조건은 외부 선행)")
        return ()
    if len(tokens) != len(set(tokens)):
        errors.append(f"{path}: 중복 선행")
    return tuple(tokens)


def details(root: Path, errors: list[str]) -> dict[str, Task]:
    tasks: dict[str, Task] = {}
    paths = sorted((root / "docs/tasks").glob("T-*.md"))
    if not paths:
        errors.append("docs/tasks: 상세 task 파일이 없음")
    for path in paths:
        match = FILE_RE.fullmatch(path.name)
        if match is None:
            errors.append(f"{path}: 파일명/ID 문법 오류")
            continue
        identifier = match[1]
        body = document(path, errors)
        headers = re.findall(r"^# .+$", body, re.MULTILINE)
        header = HEADER_RE.fullmatch(headers[0]) if len(headers) == 1 else None
        if header is None or header[1] != identifier:
            errors.append(f"{path}: 상세 제목 ID/파일명 불일치 또는 H1 개수 오류")
            title = ""
        else:
            title = header[2]
        status = scalar(body, "상태", path, errors)
        priority = scalar(body, "우선순위", path, errors)
        scalar(body, "Gate", path, errors)
        required = dependencies(scalar(body, "선행", path, errors), path, errors)
        if status not in STATUSES:
            errors.append(f"{path}: 알 수 없는 상태 {status!r}")
        if priority not in PRIORITIES:
            errors.append(f"{path}: 알 수 없는 우선순위 {priority!r}")
        if identifier in tasks:
            errors.append(f"{path}: 중복 상세 ID {identifier}")
        else:
            tasks[identifier] = Task(identifier, path, title, status, priority, required)
    return tasks


def summaries(path: Path, body: str, errors: list[str]) -> list[Summary]:
    entries: list[Summary] = []
    for number, line in enumerate(body.splitlines(), 1):
        if not line.startswith("|"):
            continue
        columns = [column.strip().strip("`") for column in line.strip("|").split("|")]
        if not columns or not columns[0].startswith("["):
            continue
        link = re.fullmatch(r"\[([^\]]+)\]\(([^)]+)\)", columns[0])
        if link is None or ID_RE.fullmatch(link[1]) is None or len(columns) != 5:
            errors.append(f"{path}:{number}: task 요약 5열/ID/link 문법 오류")
            continue
        identifier, target = link.groups()
        entries.append(Summary(identifier, path, target, columns[3],
                               columns[1], columns[2],
                               dependencies(columns[4], path, errors)))
    return entries


def check_graph(tasks: dict[str, Task], errors: list[str]) -> None:
    for identifier, task in tasks.items():
        for required in task.dependencies:
            if required == identifier:
                errors.append(f"{task.path}: 자기 선행 {identifier}")
            elif required not in tasks:
                errors.append(f"{task.path}: 없는 선행 {required}")
            elif task.status in {"READY", "DONE"} and tasks[required].status != "DONE":
                errors.append(f"{task.path}: {task.status}인데 선행 {required} 미완료")

    # 모든 component를 순회한다. 전체 task 수가 늘어도 recursion limit에 의존하지 않는다.
    degree = {identifier: 0 for identifier in tasks}
    dependents: dict[str, list[str]] = {identifier: [] for identifier in tasks}
    for identifier, task in tasks.items():
        for required in set(task.dependencies):
            if required in tasks:
                degree[identifier] += 1
                dependents[required].append(identifier)
    ready = sorted(identifier for identifier, count in degree.items() if count == 0)
    visited = 0
    while ready:
        identifier = ready.pop()
        visited += 1
        for downstream in dependents[identifier]:
            degree[downstream] -= 1
            if degree[downstream] == 0:
                ready.append(downstream)
    if visited != len(tasks):
        blocked = ", ".join(sorted(identifier for identifier, count in degree.items() if count))
        errors.append(f"선행 DAG 사이클 (연결된 차단 task 포함): {blocked}")


def validate(root: Path) -> tuple[list[str], int]:
    """root 아래 문서만 읽으며 subprocess·network·파일 쓰기를 하지 않는다."""
    root = root.resolve()
    errors: list[str] = []
    tasks = details(root, errors)
    open_path, done_path = root / "docs/tasks.md", root / "docs/tasks-done.md"
    open_body = document(open_path, errors)
    done_body = document(done_path, errors)
    counts = re.findall(r"(\d+)개의 상세 작업", open_body)
    file_count = len(list((root / "docs/tasks").glob("T-*.md")))
    if len(counts) != 1 or int(counts[0]) != file_count:
        errors.append(f"{open_path}: 상세 task 수 불일치 (실제 {file_count})")
    entries = summaries(open_path, open_body, errors) + summaries(done_path, done_body, errors)
    seen: set[str] = set()
    for entry in entries:
        identifier = entry.identifier
        if identifier in seen:
            errors.append(f"{entry.path}: 중복 요약 ID {identifier}")
        seen.add(identifier)
        task = tasks.get(identifier)
        if task is None:
            errors.append(f"{entry.path}: 상세 파일 없는 요약 {identifier}")
            continue
        expected = done_path if task.status == "DONE" else open_path
        if entry.path != expected:
            errors.append(f"{entry.path}: {identifier} 상태에 맞지 않는 요약/archive 위치")
        target = (entry.path.parent / unquote(entry.target)).resolve()
        if target != task.path.resolve():
            errors.append(f"{entry.path}: {identifier} 상세 link 불일치")
        comparisons = [
            ("제목", entry.title, task.title),
            ("상태", entry.status, task.status),
            ("우선순위", entry.priority, task.priority),
            ("선행", set(entry.dependencies), set(task.dependencies)),
        ]
        for field, actual, required in comparisons:
            if actual != required:
                errors.append(f"{entry.path}: {identifier} 요약 {field} 불일치")
    for identifier in sorted(tasks.keys() - seen):
        errors.append(f"{tasks[identifier].path}: 요약 누락 {identifier}")
    check_graph(tasks, errors)
    return errors, file_count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args(argv)
    errors, count = validate(args.root)
    for error in errors:
        print(error)
    print(f"상세 task={count}, 오류={len(errors)}; 읽기 전용 metadata/DAG 검사 (제품 gate 아님)")
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
