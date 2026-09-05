"""Validate repository-local Markdown link targets without network access."""
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def main():
    errors = []
    count = 0
    files = list((ROOT/'docs').rglob('*.md')) + list((ROOT/'hardware').rglob('*.md'))
    files += list(ROOT.glob('*.md')) + [ROOT/'tools/README.md']
    for path in sorted(set(files)):
        # Fenced examples are not clickable document navigation.
        body = re.sub(r'```.*?```', '', path.read_text(encoding='utf-8'), flags=re.S)
        for target in re.findall(r'!?\[[^\]\n]*\]\(([^)\n]+)\)', body):
            target = target.strip().strip('<>')
            parsed = urlsplit(target)
            if parsed.scheme or not parsed.path or target.startswith('//'):
                continue
            count += 1
            resolved = (path.parent/unquote(parsed.path)).resolve()
            if not resolved.exists():
                errors.append(f'{path.relative_to(ROOT)} -> {target}')
    print(f'Checked {len(set(files))} documents, {count} local targets; errors={len(errors)}')
    for error in errors:
        print(error)
    return int(bool(errors))


if __name__ == '__main__':
    sys.exit(main())
