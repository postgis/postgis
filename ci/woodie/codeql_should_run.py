#!/usr/bin/env python3
"""Gate expensive Woodpecker CodeQL analysis by changed surface.

Exit status 0 means run CodeQL. Exit status 78 means skip the expensive body
successfully. Ambiguous changed-path discovery fails open and runs CodeQL.
"""

from __future__ import annotations

import fnmatch
import os
import subprocess
import sys


CODEQL_GLOBS = (
    "ci/woodie/codeql_build.sh",
    "configure.ac",
    "configure.in",
    "autogen.sh",
    "GNUmakefile.in",
    "Makefile.in",
    "*/Makefile.in",
    "macros/**",
    "build-aux/**",
    "liblwgeom/**",
    "libpgcommon/**",
    "postgis/**",
    "deps/**",
    "loader/**",
    "raster/**",
    "sfcgal/**",
    "topology/**",
    "extensions/**",
)


def run_git(args: list[str], check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def resolve_base() -> str:
    event = os.environ.get("CI_PIPELINE_EVENT")
    if event == "push":
        before = os.environ.get("CI_COMMIT_BEFORE") or os.environ.get(
            "CI_COMMIT_BEFORE_SHA"
        )
        if before and set(before) != {"0"}:
            if (
                run_git(
                    ["rev-parse", "--verify", f"{before}^{{commit}}"],
                    check=False,
                ).returncode
                == 0
            ):
                return before
            fetched = run_git(
                ["fetch", "--no-tags", "origin", before],
                check=False,
            )
            if fetched.returncode == 0:
                return before
            raise RuntimeError(
                f"CI_COMMIT_BEFORE={before} is set, but fetching it failed"
            )
        raise RuntimeError("push event has no usable CI_COMMIT_BEFORE")

    for name in (
        "CI_COMMIT_TARGET_BRANCH",
        "WOODPECKER_PULL_REQUEST_TARGET",
        "CI_COMMIT_TARGET",
    ):
        target = os.environ.get(name)
        if not target:
            continue
        ref = f"refs/ci-target/{target}"
        fetched = run_git(
            ["fetch", "--no-tags", "origin", f"+refs/heads/{target}:{ref}"],
            check=False,
        )
        if fetched.returncode == 0:
            return ref
        raise RuntimeError(f"{name}={target} is set, but fetching it failed")

    for candidate in ("origin/master", "master"):
        if run_git(["rev-parse", "--verify", candidate], check=False).returncode == 0:
            return candidate

    raise RuntimeError("no pull-request target branch or local master ref found")


def changed_paths(base: str) -> list[str]:
    merge_base = run_git(["merge-base", base, "HEAD"]).stdout.strip()
    if not merge_base:
        raise RuntimeError(f"no merge base between {base} and HEAD")
    diff = run_git(["diff", "--name-only", f"{merge_base}...HEAD"]).stdout
    return [line for line in diff.splitlines() if line]


def main() -> int:
    event = os.environ.get("CI_PIPELINE_EVENT")

    try:
        paths = changed_paths(resolve_base())
    except Exception as exc:
        print(f"RUN: changed-path discovery failed open: {exc}")
        return 0

    if not paths:
        print("RUN: changed-path list is empty; failing open")
        return 0

    for path in paths:
        for pattern in CODEQL_GLOBS:
            if fnmatch.fnmatch(path, pattern):
                print(f"RUN: {path} matches CodeQL-relevant pattern {pattern}")
                return 0

    print("SKIP: no CodeQL-relevant path changed")
    return 78


if __name__ == "__main__":
    sys.exit(main())
