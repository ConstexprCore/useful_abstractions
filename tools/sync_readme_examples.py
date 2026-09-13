#!/usr/bin/env python3
"""Copy the example programs into README.md, verbatim.

The README documents each component with a complete program from examples/.
Those programs are built and run by ctest, so keeping the README text
identical to them is what makes the documentation trustworthy. This script
does the copying; CI runs it with --check so the two cannot drift.

Markers in README.md:

    <!-- example: examples/fixed_string.cpp -->
    ```cpp
    ...replaced with the file's contents...
    ```
    <!-- /example -->

    python3 tools/sync_readme_examples.py          # rewrite README.md
    python3 tools/sync_readme_examples.py --check  # exit 1 if README is stale
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
README = PROJECT_ROOT / "README.md"

BLOCK_RE = re.compile(
    r"(<!-- example: (?P<path>[^ ]+) -->\n```cpp\n)(?P<body>.*?)(```\n<!-- /example -->)",
    re.S,
)


def render(text: str) -> tuple[str, list[str]]:
    used: list[str] = []

    def replace(match: re.Match) -> str:
        path = PROJECT_ROOT / match.group("path")
        if not path.is_file():
            raise SystemExit(f"README references {match.group('path')}, which does not exist")
        used.append(match.group("path"))
        body = path.read_text(encoding="utf-8").rstrip("\n") + "\n"
        return f"{match.group(1)}{body}{match.group(4)}"

    return BLOCK_RE.sub(replace, text), used


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail if README.md is out of date instead of rewriting it")
    args = parser.parse_args()

    original = README.read_text(encoding="utf-8")
    updated, used = render(original)

    # Every example program should be documented.
    examples = sorted(p.relative_to(PROJECT_ROOT).as_posix() for p in (PROJECT_ROOT / "examples").glob("*.cpp"))
    missing = [e for e in examples if e not in used]
    if missing:
        print(f"examples not referenced from README.md: {', '.join(missing)}", file=sys.stderr)
        return 1

    if updated == original:
        print(f"README.md is in sync with {len(used)} examples")
        return 0
    if args.check:
        print("README.md is out of date; run tools/sync_readme_examples.py", file=sys.stderr)
        return 1
    README.write_text(updated, encoding="utf-8")
    print(f"README.md updated from {len(used)} examples")
    return 0


if __name__ == "__main__":
    sys.exit(main())
