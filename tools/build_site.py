#!/usr/bin/env python3
"""Build the useful_abstractions project page from README.md.

Modeled on the simdutf approach: the README is the single source of truth, and
the site is a rendering of it — so the page cannot drift from the docs. The
generator adds what a landing page needs and a README cannot carry: a hero, a
few headline numbers, a sticky table of contents, syntax highlighting.

    python3 tools/build_site.py                 # writes _site/
    python3 tools/build_site.py --output docs/_build --serve

The stale hand-written table of contents at the top of the README is dropped;
the page builds its own from the actual headings.
"""

from __future__ import annotations

import argparse
import datetime
import html
import re
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
SITE_DIR = PROJECT_ROOT / "docs" / "site"
README = PROJECT_ROOT / "README.md"

REPO_URL = "https://github.com/ConstexprCore/useful_abstractions"
SITE_URL = "https://constexprcore.github.io/useful_abstractions/"

HERO_CODE = '''#include "useful_abstractions.h"

using namespace ConstexprCore;

// A string literal as a template parameter.
template <fixed_string Name>
struct tagged {
    static constexpr auto tag = Name.view();
};

// Names, hashes and escaping, all at compile time.
static_assert(type_name<double>() == "double");
static_assert(fnv1a("a") != fnv1a("b"));
constexpr auto json = json_escape<"tab\\there">();'''


def version() -> str:
    try:
        return subprocess.run([sys.executable, str(PROJECT_ROOT / "tools" / "release.py"), "--show-version"],
                              capture_output=True, text=True, check=True).stdout.strip()
    except (subprocess.CalledProcessError, OSError):
        return "0.0.0"


def slugify(text: str) -> str:
    slug = re.sub(r"[^\w\s-]", "", text.strip().lower())
    return re.sub(r"[\s_]+", "-", slug)


def split_readme(text: str) -> tuple[str, str, str]:
    """Return (headline, tagline, body) with the badge block and stale TOC removed."""
    lines = text.splitlines()
    headline = lines[0].lstrip("# ").strip()

    body_start = 1
    tagline_lines: list[str] = []
    for index in range(1, len(lines)):
        line = lines[index].strip()
        if line.startswith("## "):
            body_start = index
            break
        if not line:
            if tagline_lines:
                continue            # the tagline paragraph is complete
            continue
        if line.startswith("[!["):          # badges
            continue
        if line.startswith(("* [", "+ [", "- [")):
            continue                        # hand-written TOC entries
        if not line.startswith("#") and (not tagline_lines or lines[index - 1].strip()):
            tagline_lines.append(line)      # first paragraph, possibly wrapped
    # The tagline is shown as plain text: strip inline markdown.
    tagline = " ".join(tagline_lines)
    tagline = re.sub(r"\[([^\]]+)\]\([^)]*\)", r"\1", tagline)   # links
    tagline = re.sub(r"[`*_]", "", tagline)                       # code, emphasis
    return headline, tagline, "\n".join(lines[body_start:])


def headline_numbers(body: str) -> list[tuple[str, str]]:
    """Count what the library actually ships so the strip cannot go stale."""
    headers = sorted((PROJECT_ROOT / "include" / "ConstexprCore").glob("*.h"))
    components = [h for h in headers if not h.name.startswith("useful_abstractions")]
    single = PROJECT_ROOT / "singleheader" / "useful_abstractions.h"
    lines = len(single.read_text(encoding="utf-8").splitlines()) if single.is_file() else 0
    return [
        (str(len(components)), "components, usable separately"),
        ("0", "dependencies beyond the standard library"),
        ("1", f"header to drop in ({lines:,} lines)"),
        ("C++23", "header-only, consteval-first"),
    ]


def render_markdown(body: str) -> tuple[str, str]:
    """Return (html, toc_html). Uses python-markdown when present."""
    try:
        import markdown  # type: ignore
    except ImportError:
        raise SystemExit("python-markdown is required: pip install markdown")

    converter = markdown.Markdown(extensions=["fenced_code", "tables", "sane_lists", "attr_list"])
    rendered = converter.convert(body)

    # Anchors + a table of contents from the real headings.
    toc_entries: list[str] = []

    def add_anchor(match: re.Match) -> str:
        level, attrs, inner = match.group(1), match.group(2), match.group(3)
        title = re.sub(r"<[^>]+>", "", inner)
        slug = slugify(title)
        if level in ("2", "3"):
            toc_entries.append(
                f'      <li class="h{level}"><a href="#{slug}">{html.escape(title)}</a></li>'
            )
        anchor = f'<a class="anchor" href="#{slug}" aria-label="Link to this section">#</a>'
        return f'<h{level} id="{slug}"{attrs}>{inner}{anchor}</h{level}>'

    rendered = re.sub(r"<h([1-6])([^>]*)>(.*?)</h\1>", add_anchor, rendered, flags=re.S)
    return rendered, "\n".join(toc_entries)


def build(output: Path) -> Path:
    text = README.read_text(encoding="utf-8")
    headline, tagline, body = split_readme(text)
    content, toc = render_markdown(body)
    stats = headline_numbers(body)
    release = version()

    template = (SITE_DIR / "template.html").read_text(encoding="utf-8")
    replacements = {
        "TITLE": f"useful_abstractions — {headline}",
        "DESCRIPTION": tagline or headline,
        "HEADLINE": html.escape(headline),
        "TAGLINE": html.escape(tagline),
        "HERO_CODE": html.escape(HERO_CODE),
        "VERSION": release,
        "REPO_URL": REPO_URL,
        "SITE_URL": SITE_URL,
        "DOWNLOAD_URL": f"{REPO_URL}/releases/download/v{release}/useful_abstractions.h",
        "TOC": toc,
        "CONTENT": content,
        "BUILT_ON": datetime.date.today().isoformat(),
        "LICENSE_LINE": "Apache 2.0 / MIT licensed.",
    }
    for index, (number, label) in enumerate(stats, start=1):
        replacements[f"STAT_{index}_N"] = html.escape(number)
        replacements[f"STAT_{index}_L"] = html.escape(label)

    page = template
    for key, value in replacements.items():
        page = page.replace("{{" + key + "}}", value)

    leftovers = re.findall(r"\{\{[A-Z_0-9]+\}\}", page)
    if leftovers:
        raise SystemExit(f"template placeholders left unfilled: {sorted(set(leftovers))}")

    output.mkdir(parents=True, exist_ok=True)
    (output / "index.html").write_text(page, encoding="utf-8")
    shutil.copy(SITE_DIR / "style.css", output / "style.css")
    # GitHub Pages must serve the files verbatim, not run Jekyll over them.
    (output / ".nojekyll").write_text("", encoding="utf-8")
    return output / "index.html"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", type=Path, default=PROJECT_ROOT / "_site", help="directory to write the site into")
    parser.add_argument("--serve", action="store_true", help="serve the result on http://localhost:8000 after building")
    args = parser.parse_args()

    index = build(args.output)
    print(f"wrote {index} ({index.stat().st_size // 1024} KB)")

    if args.serve:
        import http.server
        import socketserver
        import os
        os.chdir(args.output)
        with socketserver.TCPServer(("", 8000), http.server.SimpleHTTPRequestHandler) as server:
            print("serving on http://localhost:8000 — ctrl-c to stop")
            server.serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
