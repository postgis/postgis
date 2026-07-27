#!/usr/bin/env python3
"""Report paths changed by the current CI build.

The script is intentionally conservative.  If it cannot identify a pull-request
base, it prints the reason and exits with status 2 so callers can fail open.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def run_git(args: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def resolve_base(explicit_base: str | None) -> str:
    if explicit_base:
        return explicit_base

    for name in (
        "CI_COMMIT_TARGET_BRANCH",
        "WOODPECKER_PULL_REQUEST_TARGET",
        "GITHUB_BASE_REF",
    ):
        target = os.environ.get(name)
        if target:
            ref = f"refs/ci-target/{target}"
            fetch = run_git(
                ["fetch", "--no-tags", "origin", f"+refs/heads/{target}:{ref}"],
                check=False,
            )
            if fetch.returncode == 0:
                return ref
            fetch = run_git(
                ["fetch", "--no-tags", "upstream", f"+refs/heads/{target}:{ref}"],
                check=False,
            )
            if fetch.returncode == 0:
                return ref
            raise RuntimeError(
                f"{name}={target} is set, but fetching that branch failed"
            )

    for candidate in ("upstream/master", "origin/master", "master"):
        exists = run_git(["rev-parse", "--verify", candidate], check=False)
        if exists.returncode == 0:
            return candidate

    raise RuntimeError("no pull-request target branch or local master ref found")


def changed_paths(base: str, head: str) -> list[str]:
    merge_base = run_git(["merge-base", base, head]).stdout.strip()
    if not merge_base:
        raise RuntimeError(f"no merge base between {base} and {head}")
    diff = run_git(["diff", "--name-only", f"{merge_base}...{head}"]).stdout
    return [line for line in diff.splitlines() if line]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", help="base ref to diff against")
    parser.add_argument("--head", default="HEAD", help="head ref to diff")
    parser.add_argument(
        "--output",
        type=Path,
        help="write the changed path list here instead of stdout",
    )
    args = parser.parse_args()

    try:
        base = resolve_base(args.base)
        paths = changed_paths(base, args.head)
    except Exception as exc:
        print(f"RUN: changed-path discovery failed open: {exc}", file=sys.stderr)
        return 2

    output = "\n".join(paths)
    if output:
        output += "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
