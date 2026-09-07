"""Validate repository-local Markdown link targets without network access."""
import argparse
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def without_fences(body: str) -> str:
    """CommonMark의 3개 이상 backtick/tilde fence 안 원문은 navigation이 아니다."""
    result = []
    fence = None
    for line in body.splitlines(keepends=True):
        if fence is not None:
            character, length = fence
            if re.fullmatch(r" {0,3}" + re.escape(character) + "{" + str(length) + r",}[ \t]*(?:\r?\n)?", line):
                fence = None
            continue
        match = re.match(r" {0,3}(`{3,}|~{3,})([^\r\n]*)", line)
        if match and (match[1][0] == "~" or "`" not in match[2]):
            fence = (match[1][0], len(match[1]))
        else:
            result.append(line)
    return "".join(result)


def validate(root: Path) -> tuple[list[str], int, int]:
    root = root.resolve()
    errors = []
    count = 0
    files = list((root/'docs').rglob('*.md')) + list((root/'hardware').rglob('*.md'))
    files += list((root/'firmware').rglob('README.md'))
    files += list(root.glob('*.md'))
    tools_readme = root/'tools/README.md'
    if tools_readme.exists():
        files.append(tools_readme)
    for path in sorted(set(files)):
        # Fenced examples are not clickable document navigation.
        body = without_fences(path.read_text(encoding='utf-8'))
        for target in re.findall(r'!?\[[^\]\n]*\]\(([^)\n]+)\)', body):
            target = target.strip().strip('<>')
            # Keep immutable reviewer text unchanged while resolving the two
            # documented checkout spellings on either Windows or WSL.
            absolute_repo_target = None
            for prefix in ['/mnt/f/dev/canview/', 'F:/dev/canview/']:
                if target.startswith(prefix):
                    absolute_repo_target = re.sub(r':\d+$', '', target[len(prefix):])
                    break
            if absolute_repo_target is not None:
                count += 1
                if not (root/unquote(urlsplit(absolute_repo_target).path)).exists():
                    errors.append(f'{path.relative_to(root)} -> {target}')
                continue
            parsed = urlsplit(target)
            if parsed.scheme or not parsed.path or target.startswith('//'):
                continue
            count += 1
            resolved = (path.parent/unquote(parsed.path)).resolve()
            if not resolved.exists():
                errors.append(f'{path.relative_to(root)} -> {target}')
    return errors, len(set(files)), count


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args(argv)
    errors, document_count, target_count = validate(args.root)
    print(f'Checked {document_count} documents, {target_count} local targets; errors={len(errors)}')
    for error in errors:
        print(error)
    return int(bool(errors))


if __name__ == '__main__':
    sys.exit(main())
