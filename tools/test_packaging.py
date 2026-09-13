#!/usr/bin/env python3
"""Check that useful_abstractions can actually be consumed the three documented ways.

  1. drop-in      — copy singleheader/useful_abstractions.h next to your code, compile
  2. FetchContent — add_subdirectory/FetchContent, link ConstexprCore::useful_abstractions
  3. find_package — install the project, then find_package(useful_abstractions)

Each consumer lives in tests/packaging/ and is built in a throwaway directory,
then run. Usage:

    python3 tools/test_packaging.py            # all three
    python3 tools/test_packaging.py --only dropin find_package
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
PACKAGING_DIR = PROJECT_ROOT / "tests" / "packaging"
SINGLEHEADER = PROJECT_ROOT / "singleheader" / "useful_abstractions.h"


def run(command: list[str], **kwargs) -> None:
    printable = " ".join(str(c) for c in command)
    print(f"  $ {printable}", flush=True)
    subprocess.run(command, check=True, **kwargs)


def cmake_build(source: Path, build: Path, extra: list[str], env: dict | None = None) -> Path:
    run(["cmake", "-S", str(source), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release", *extra], env=env)
    run(["cmake", "--build", str(build), "--config", "Release", "-j", str(os.cpu_count() or 4)], env=env)
    for candidate in (build / "consumer", build / "Release" / "consumer.exe", build / "consumer.exe"):
        if candidate.exists():
            return candidate
    raise SystemExit(f"consumer binary not found under {build}")


def test_dropin(workdir: Path) -> None:
    """Exactly what a user gets from `curl -O .../useful_abstractions.h`."""
    if not SINGLEHEADER.is_file():
        raise SystemExit(f"{SINGLEHEADER} is missing; run singleheader/amalgamate.py")
    shutil.copy(SINGLEHEADER, workdir / "useful_abstractions.h")
    shutil.copy(PACKAGING_DIR / "dropin" / "main.cpp", workdir / "main.cpp")
    binary = workdir / "dropin_consumer"
    compiler = os.environ.get("CXX", "c++")
    run([compiler, "-std=c++23", "-O2", "-Wall", "-Wextra", "-I", str(workdir),
         str(workdir / "main.cpp"), "-o", str(binary)])
    run([str(binary)])


def test_fetchcontent(workdir: Path) -> None:
    binary = cmake_build(PACKAGING_DIR / "fetchcontent", workdir / "build",
                         [f"-DUA_SOURCE={PROJECT_ROOT}"])
    run([str(binary)])


def test_find_package(workdir: Path) -> None:
    prefix = workdir / "prefix"
    build = workdir / "build-project"
    run(["cmake", "-S", str(PROJECT_ROOT), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
         "-DPH_BUILD_TESTS=OFF", "-DPH_BUILD_BENCHMARKS=OFF", "-DPH_BUILD_EXAMPLES=OFF",
         f"-DCMAKE_INSTALL_PREFIX={prefix}"])
    run(["cmake", "--install", str(build)])

    installed = prefix / "include" / "ConstexprCore" / "useful_abstractions.h"
    if not installed.is_file():
        raise SystemExit(f"expected {installed} to be installed")
    if "begin file ConstexprCore/fixed_string.h" not in installed.read_text(encoding="utf-8"):
        raise SystemExit("installed header does not look amalgamated (no spliced fixed_string.h)")

    binary = cmake_build(PACKAGING_DIR / "find_package", workdir / "build-consumer",
                         [f"-DCMAKE_PREFIX_PATH={prefix}"])
    run([str(binary)])


TESTS = {
    "dropin": test_dropin,
    "fetchcontent": test_fetchcontent,
    "find_package": test_find_package,
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--only", nargs="+", choices=sorted(TESTS), help="run a subset of the consumers")
    parser.add_argument("--keep", action="store_true", help="keep the temporary build trees")
    args = parser.parse_args()

    selected = args.only or list(TESTS)
    failures: list[str] = []
    for name in selected:
        print(f"\n=== consumer: {name} ===", flush=True)
        workdir = Path(tempfile.mkdtemp(prefix=f"ph-packaging-{name}-"))
        try:
            TESTS[name](workdir)
            print(f"=== consumer: {name} OK ===")
        except subprocess.CalledProcessError as error:
            failures.append(name)
            print(f"=== consumer: {name} FAILED ({error}) ===", file=sys.stderr)
        finally:
            if args.keep:
                print(f"  (left {workdir} in place)")
            else:
                shutil.rmtree(workdir, ignore_errors=True)

    print()
    if failures:
        print(f"FAILED: {', '.join(failures)}", file=sys.stderr)
        return 1
    print(f"all {len(selected)} consumer builds passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
