"""Validate repository-local Markdown link targets without network access."""
import argparse
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def validate(root: Path) -> tuple[list[str], int, int]:
    root = root.resolve()
    errors = []
    count = 0
    files = list((root/'docs').rglob('*.md')) + list((root/'hardware').rglob('*.md'))
    files += list(root.glob('*.md'))
    tools_readme = root/'tools/README.md'
    if tools_readme.exists():
        files.append(tools_readme)
    for path in sorted(set(files)):
        # Fenced examples are not clickable document navigation.
        body = re.sub(r'```.*?```', '', path.read_text(encoding='utf-8'), flags=re.S)
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
