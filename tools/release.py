#!/usr/bin/env python3
"""Cut a useful_abstractions release.

Modeled on simdutf's tools/release.py: bump the version everywhere it is
written down, regenerate the amalgamation, commit, and tag. Pushing is left to
you — the script prints the command.

    python3 tools/release.py 0.1.0        # explicit version
    python3 tools/release.py --minor      # 0.1.3 -> 0.2.0
    python3 tools/release.py 0.1.0 -n     # dry run: show what would change

Version numbers live in:
    CMakeLists.txt                                       project(... VERSION x.y.z)
    include/ConstexprCore/useful_abstractions_version.h  macros + constants
    README.md                                            pinned download URLs
    singleheader/useful_abstractions.h                   regenerated banner
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CMAKELISTS = PROJECT_ROOT / "CMakeLists.txt"
VERSION_HEADER = PROJECT_ROOT / "include" / "ConstexprCore" / "useful_abstractions_version.h"
README = PROJECT_ROOT / "README.md"
SINGLEHEADER = PROJECT_ROOT / "singleheader" / "useful_abstractions.h"
AMALGAMATE = PROJECT_ROOT / "singleheader" / "amalgamate.py"

DEFAULT_BRANCH = "master"
VERSION_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def git(*args: str, capture: bool = True) -> str:
    result = subprocess.run(["git", "-C", str(PROJECT_ROOT), *args],
                            capture_output=capture, text=True, check=True)
    return (result.stdout or "").strip()


def current_version() -> tuple[int, int, int]:
    match = re.search(r"project\([^)]*?VERSION\s+(\d+)\.(\d+)\.(\d+)", CMAKELISTS.read_text(encoding="utf-8"), re.S)
    if not match:
        raise SystemExit("could not read the current version from CMakeLists.txt")
    return tuple(int(part) for part in match.groups())  # type: ignore[return-value]


def check_worktree(allow_dirty: bool, allow_branch: bool) -> None:
    """Refuse to release from a branch or tree that would produce a surprising tag."""
    branch = git("rev-parse", "--abbrev-ref", "HEAD")
    if branch != DEFAULT_BRANCH and not allow_branch:
        raise SystemExit(f"on branch '{branch}', expected '{DEFAULT_BRANCH}' (override with --allow-branch)")
    # Untracked files are ignored on purpose: scratch files are normal here.
    dirty = git("status", "--porcelain", "--untracked-files=no")
    if dirty and not allow_dirty:
        raise SystemExit("tracked files are modified; commit or stash them first "
                         f"(override with --allow-dirty):\n{dirty}")


def bump(version: tuple[int, int, int], part: str) -> tuple[int, int, int]:
    major, minor, patch = version
    if part == "major":
        return (major + 1, 0, 0)
    if part == "minor":
        return (major, minor + 1, 0)
    return (major, minor, patch + 1)


def write_cmakelists(version: str, dry_run: bool) -> bool:
    text = CMAKELISTS.read_text(encoding="utf-8")
    updated = re.sub(r"(project\([^)]*?VERSION\s+)\d+\.\d+\.\d+", rf"\g<1>{version}", text, count=1, flags=re.S)
    return write_if_changed(CMAKELISTS, text, updated, dry_run)


def write_version_header(version: str, dry_run: bool) -> bool:
    major, minor, revision = version.split(".")
    text = VERSION_HEADER.read_text(encoding="utf-8")
    updated = text
    for macro, value in (
        ("CONSTEXPRCORE_PERFECT_HASH_VERSION", f'"{version}"'),
        ("CONSTEXPRCORE_PERFECT_HASH_VERSION_MAJOR", major),
        ("CONSTEXPRCORE_PERFECT_HASH_VERSION_MINOR", minor),
        ("CONSTEXPRCORE_PERFECT_HASH_VERSION_REVISION", revision),
    ):
        updated = re.sub(rf"^(#define {macro} ).*$", rf"\g<1>{value}", updated, count=1, flags=re.M)
    return write_if_changed(VERSION_HEADER, text, updated, dry_run)


def write_readme(version: str, dry_run: bool) -> bool:
    """Repoint the pinned download URLs at the new tag."""
    if not README.is_file():
        return False
    text = README.read_text(encoding="utf-8")
    updated = re.sub(r"/download/v\d+\.\d+\.\d+/", f"/download/v{version}/", text)
    updated = re.sub(r"(useful_abstractions/)v\d+\.\d+\.\d+(/singleheader/)", rf"\g<1>v{version}\g<2>", updated)
    return write_if_changed(README, text, updated, dry_run)


def write_if_changed(path: Path, old: str, new: str, dry_run: bool) -> bool:
    relative = path.relative_to(PROJECT_ROOT)
    if old == new:
        print(f"  {relative}: already current")
        return False
    if dry_run:
        print(f"  {relative}: would update")
        return True
    path.write_text(new, encoding="utf-8")
    print(f"  {relative}: updated")
    return True


def regenerate_singleheader(dry_run: bool) -> bool:
    if dry_run:
        print(f"  {SINGLEHEADER.relative_to(PROJECT_ROOT)}: would regenerate")
        return True
    subprocess.run([sys.executable, str(AMALGAMATE), "--quiet"], check=True)
    print(f"  {SINGLEHEADER.relative_to(PROJECT_ROOT)}: regenerated")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", nargs="?", help="version to release, e.g. 0.2.0")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--major", action="store_const", dest="part", const="major", help="bump the major version")
    group.add_argument("--minor", action="store_const", dest="part", const="minor", help="bump the minor version")
    group.add_argument("--patch", action="store_const", dest="part", const="patch", help="bump the patch version")
    parser.add_argument("--show-version", action="store_true", help="print the current version and exit")
    parser.add_argument("-n", "--dry-run", action="store_true", help="report what would change and stop")
    parser.add_argument("--no-commit", action="store_true", help="update the files but do not commit or tag")
    parser.add_argument("--no-tag", action="store_true", help="commit but do not create the tag")
    parser.add_argument("--allow-dirty", action="store_true", help="proceed with modified tracked files")
    parser.add_argument("--allow-branch", action="store_true", help=f"proceed off '{DEFAULT_BRANCH}'")
    args = parser.parse_args()

    if args.show_version:
        print(".".join(str(part) for part in current_version()))
        return 0

    if args.version and args.part:
        raise SystemExit("give a version or a --major/--minor/--patch bump, not both")
    if not args.version and not args.part:
        raise SystemExit("give a version (e.g. 0.2.0) or one of --major/--minor/--patch")

    if args.version:
        if not VERSION_RE.match(args.version):
            raise SystemExit(f"'{args.version}' is not a major.minor.patch version")
        version = args.version
    else:
        version = ".".join(str(part) for part in bump(current_version(), args.part))

    old_version = ".".join(str(part) for part in current_version())
    if not args.dry_run and not args.no_commit:
        check_worktree(args.allow_dirty, args.allow_branch)

    tag = f"v{version}"
    existing_tags = git("tag", "--list", tag)
    if existing_tags and not args.no_commit:
        raise SystemExit(f"tag {tag} already exists")

    print(f"releasing {old_version} -> {version}")
    touched = [
        (CMAKELISTS, write_cmakelists(version, args.dry_run)),
        (VERSION_HEADER, write_version_header(version, args.dry_run)),
        (README, write_readme(version, args.dry_run)),
        (SINGLEHEADER, regenerate_singleheader(args.dry_run)),
    ]

    if args.dry_run:
        print("\ndry run: nothing was written")
        return 0

    changed = [path for path, did_change in touched if did_change and path.is_file()]
    if args.no_commit:
        print("\n--no-commit: files updated, nothing committed. Review with:\n  git diff")
        return 0

    for path in changed:
        git("add", str(path.relative_to(PROJECT_ROOT)))
    # Re-releasing the version already written in the tree changes no file;
    # tag the commit that is there rather than failing on an empty commit.
    if git("status", "--porcelain", "--untracked-files=no"):
        git("commit", "-m", f"Release {tag}")
        print(f"\ncommitted 'Release {tag}'")
    else:
        print("\nno file changes to commit; tagging the current commit")

    if not args.no_tag:
        git("tag", "-a", tag, "-m", f"useful_abstractions {version}")
        print(f"tagged {tag}")

    print("\nNothing has been pushed. When you are ready:")
    print(f"  git push origin {DEFAULT_BRANCH}" + ("" if args.no_tag else f" && git push origin {tag}"))
    print(f"\nThen attach singleheader/useful_abstractions.h to the {tag} GitHub release so the")
    print("curl instructions in README.md resolve.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
