#!/usr/bin/env python3
"""Amalgamate the ConstexprCore useful_abstractions headers into one header.

Modeled on simdutf's singleheader/amalgamate.py: walk the include graph from
the umbrella header, splice each project header in at the point where it is
included, and leave everything else — system includes, preprocessor
conditionals, code — exactly as written.

The library depends on nothing but the standard library, so the result is a
single file a user can drop into a project.

Usage:
    python3 singleheader/amalgamate.py           # write singleheader/useful_abstractions.h
    python3 singleheader/amalgamate.py --check   # verify the checked-in copy is current
    python3 singleheader/amalgamate.py --test    # also compile the demo
"""

from __future__ import annotations

import argparse
import datetime
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent

DEFAULT_ROOT_HEADER = "ConstexprCore/useful_abstractions.h"
DEFAULT_OUTPUT = SCRIPT_DIR / "useful_abstractions.h"
DEFAULT_DEMO = SCRIPT_DIR / "amalgamation_demo.cpp"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*(?:"([^"]+)"|<([^>]+)>)')
PRAGMA_ONCE_RE = re.compile(r'^\s*#\s*pragma\s+once\s*$')

DEMO_SOURCE = '''// Compile with:  c++ -std=c++23 -I. amalgamation_demo.cpp -o demo
#include "useful_abstractions.h"

#include <cstdio>
#include <string>

using namespace ConstexprCore;

// fixed_string: a string literal usable as a template parameter.
template <fixed_string Name>
struct tagged {
    static constexpr std::string_view tag = Name.view();
};
static_assert(tagged<"widget">::tag == "widget");

// Hashing and type names, both in a constant expression.
static_assert(fnv1a("widget") != fnv1a("gadget"));
static_assert(type_name<double>() == "double");

// JSON escaping computed at compile time.
static constexpr auto escaped = json_escape<"tab\\there">();

int main() {
    std::printf("useful_abstractions %s\\n", useful_abstractions_version);
    std::printf("tag       : %s\\n", std::string(tagged<"widget">::tag).c_str());
    std::printf("type_name : %s\\n", std::string(type_name<double>()).c_str());
    std::printf("escaped   : %s\\n", std::string(escaped.view()).c_str());
    return 0;
}
'''


def log(message: str) -> None:
    print(f"[amalgamate] {message}", file=sys.stderr)


class Amalgamator:
    def __init__(self, include_dirs: list[Path]) -> None:
        self.include_dirs = include_dirs
        self.included: set[Path] = set()
        self.order: list[str] = []
        self.lines: list[str] = []

    def resolve(self, name: str, current_file: Path) -> Path | None:
        """Map an #include target to a project file, or None if it is a system header."""
        sibling = current_file.parent / name
        if sibling.is_file():
            return sibling.resolve()
        for directory in self.include_dirs:
            candidate = directory / name
            if candidate.is_file():
                return candidate.resolve()
        return None

    def display_name(self, path: Path) -> str:
        for directory in self.include_dirs:
            try:
                return path.relative_to(directory.resolve()).as_posix()
            except ValueError:
                continue
        try:
            return path.relative_to(PROJECT_ROOT).as_posix()
        except ValueError:
            return path.name

    def splice(self, path: Path) -> None:
        path = path.resolve()
        name = self.display_name(path)
        if path in self.included:
            self.lines.append(f"/* skipped duplicate {name} */")
            return
        self.included.add(path)
        self.order.append(name)

        self.lines.append(f"/* begin file {name} */")
        for line in path.read_text(encoding="utf-8").splitlines():
            if PRAGMA_ONCE_RE.match(line):
                # The amalgamation is one file; a per-fragment `#pragma once`
                # would apply to the whole of it.
                self.lines.append(f"/* omitted #pragma once from {name} */")
                continue
            match = INCLUDE_RE.match(line)
            if match:
                target = match.group(1) or match.group(2)
                resolved = self.resolve(target, path)
                if resolved is not None:
                    self.splice(resolved)
                    continue
            self.lines.append(line)
        self.lines.append(f"/* end file {name} */")


def project_version() -> str:
    text = (PROJECT_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"project\([^)]*?VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", text, re.S)
    return match.group(1) if match else "unknown"


def build_header(include_dirs: list[Path], root_header: str, stamp: str) -> str:
    root_path = None
    for directory in include_dirs:
        candidate = directory / root_header
        if candidate.is_file():
            root_path = candidate
            break
    if root_path is None:
        raise SystemExit(f"root header {root_header} not found in {include_dirs}")

    amalgamator = Amalgamator(include_dirs)
    amalgamator.splice(root_path)

    # The point of the amalgamation is that the standard library (plus the
    # compiler's own ISA headers, behind their guards) is all it needs. Prove
    # it rather than trusting the walk.
    leftovers = [
        line for line in amalgamator.lines
        if INCLUDE_RE.match(line) and "ConstexprCore/" in line
    ]
    if leftovers:
        raise SystemExit(
            "amalgamation is not self-contained; unresolved project includes:\n  "
            + "\n  ".join(leftovers)
        )

    banner = [
        f"/* auto-generated on {stamp}. Do not edit! */",
        "/*",
        f" * ConstexprCore useful_abstractions {project_version()} — single-header amalgamation.",
        " *",
        " * Compile-time perfect hashing for C++. Drop this file into your project and",
        " * #include it; it is self-contained and needs no other ConstexprCore headers.",
        " *",
        " * Requires C++23. Regenerate with singleheader/amalgamate.py.",
        " *",
        " * Bundled files, in order:",
    ]
    banner += [f" *   {name}" for name in amalgamator.order]
    banner += [" */", ""]
    return "\n".join(banner + amalgamator.lines) + "\n"


def strip_stamp(text: str) -> str:
    """Drop the generated-on line so --check ignores the timestamp."""
    return "\n".join(
        line for line in text.splitlines() if not line.startswith("/* auto-generated on ")
    )


def compile_demo(header: Path, demo: Path) -> None:
    compiler = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        shutil.copy(header, workdir / header.name)
        shutil.copy(demo, workdir / demo.name)
        binary = workdir / "demo"
        command = [compiler, "-std=c++23", "-O2", "-I", str(workdir), str(workdir / demo.name), "-o", str(binary)]
        log(" ".join(command))
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)
    log("demo compiled and ran cleanly")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help=f"amalgamated header to write (default: {DEFAULT_OUTPUT.relative_to(PROJECT_ROOT)})")
    parser.add_argument("--demo", type=Path, default=DEFAULT_DEMO, help="demo source to write alongside the header")
    parser.add_argument("--root-header", default=DEFAULT_ROOT_HEADER, help="entry point of the include graph")
    parser.add_argument("--include-dir", type=Path, action="append", default=[],
                        help="extra include root (repeatable); the project's own include/ is always used")
    parser.add_argument("--check", action="store_true", help="fail if the file on disk differs from freshly generated output")
    parser.add_argument("--test", action="store_true", help="compile and run the demo against the generated header")
    parser.add_argument("--quiet", action="store_true", help="suppress progress output")
    args = parser.parse_args()

    if args.quiet:
        global log
        log = lambda message: None  # noqa: E731

    include_dirs = [PROJECT_ROOT / "include"] + [d.resolve() for d in args.include_dir]

    stamp = datetime.datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %z")
    text = build_header(include_dirs, args.root_header, stamp)

    if args.check:
        if not args.output.is_file():
            print(f"{args.output} is missing; run singleheader/amalgamate.py", file=sys.stderr)
            return 1
        if strip_stamp(args.output.read_text(encoding="utf-8")) != strip_stamp(text):
            print(f"{args.output} is out of date; run singleheader/amalgamate.py", file=sys.stderr)
            return 1
        log(f"{args.output} is up to date")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8")
    log(f"wrote {args.output} ({len(text.splitlines())} lines)")

    if args.demo is not None:
        args.demo.parent.mkdir(parents=True, exist_ok=True)
        args.demo.write_text(DEMO_SOURCE, encoding="utf-8")
        log(f"wrote {args.demo}")

    if args.test:
        compile_demo(args.output, args.demo)
    return 0


if __name__ == "__main__":
    sys.exit(main())
