#!/usr/bin/env python3
"""Decide whether an expensive CI suite is needed for this change.

Exit status is part of the interface:

* 0 means run the suite.
* 78 means skip the expensive body; the CI step should exit successfully.

Any ambiguity prints a RUN verdict and exits 0.
"""

from __future__ import annotations

import argparse
import fnmatch
import os
import subprocess
import sys


CI_AND_BUILD_GLOBS = (
    ".woodpecker/*.yml",
    ".github/workflows/*.yml",
    "ci/**",
    "configure.ac",
    "configure.in",
    "autogen.sh",
    "GNUmakefile.in",
    "Makefile.in",
    "*/Makefile.in",
    "macros/**",
    "build-aux/**",
    "regress/run_test.pl",
    "regress/runtest.mk",
    "regress/hooks/**",
    "regress/utils/**",
    "fuzzers/**",
)

CORE_GLOBS = (
    "liblwgeom/**",
    "libpgcommon/**",
    "postgis/**",
    "regress/core/**",
    "regress/dumper/**",
    "deps/**",
    "sfcgal/**",
)

RASTER_GLOBS = (
    "raster/**",
    "extensions/postgis_raster/**",
)

TOPOLOGY_GLOBS = (
    "topology/**",
    "extensions/postgis_topology/**",
)

LOADER_GLOBS = (
    "loader/**",
    "regress/loader/**",
)

UPGRADE_GLOBS = (
    "Version.config",
    "extensions/**",
    "postgis/*.sql.in",
    "raster/rt_pg/*.sql.in",
    "topology/*.sql.in",
    "topology/**/*.sql.in",
    "sfcgal/**/*.sql.in",
    "utils/create_upgrade.pl",
    "utils/create_unpackaged.pl",
    "utils/create_extension_unpackage.pl",
    "utils/postgis_restore.pl",
    "utils/postgis_restore.pl.in",
    "utils/check_all_upgrades.sh",
    "utils/check_cluster_upgrade.sh",
)

NEWS_GLOBS = (
    "NEWS",
    "utils/docs/check_news.sh",
    "utils/docs/tests/test_check_news.py",
)


def match_any(path: str, globs: tuple[str, ...]) -> str | None:
    for pattern in globs:
        if fnmatch.fnmatch(path, pattern):
            return pattern
    return None


def changed_paths(base: str | None, head: str) -> tuple[list[str], str]:
    cmd = ["python3", "ci/ci_changed_paths.py", "--head", head]
    if base:
        cmd.extend(["--base", base])
    result = subprocess.run(
        cmd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "changed path helper failed")
    return [line for line in result.stdout.splitlines() if line], result.stdout


def run_upgrade_surface(base: str | None, head: str) -> int:
    cmd = ["python3", "utils/check_upgrade_surface.py", "--head", head]
    if base:
        cmd.extend(["--base", base])
    return subprocess.call(cmd)


def need_for_suite(paths: list[str], suite: str, target: str, extension: str) -> tuple[bool, str]:
    for path in paths:
        pattern = match_any(path, CI_AND_BUILD_GLOBS)
        if pattern:
            return True, f"{path} matches always-run CI/build pattern {pattern}"

    if suite == "news":
        for path in paths:
            pattern = match_any(path, NEWS_GLOBS)
            if pattern:
                return True, f"{path} matches NEWS checker pattern {pattern}"
        return False, "no NEWS checker input changed"

    if suite == "extension-upgrade" or "upgrade" in target or suite == "cluster-upgrade":
        direct_globs = UPGRADE_GLOBS
        if extension == "postgis_raster":
            direct_globs = UPGRADE_GLOBS + RASTER_GLOBS
        elif extension == "postgis_topology":
            direct_globs = UPGRADE_GLOBS + TOPOLOGY_GLOBS
        elif extension == "postgis_sfcgal":
            direct_globs = UPGRADE_GLOBS + ("sfcgal/**", "extensions/postgis_sfcgal/**")
        for path in paths:
            pattern = match_any(path, direct_globs)
            if pattern:
                return True, f"{path} matches upgrade-relevant pattern {pattern}"
        return False, "no directly upgrade-relevant path changed"

    if suite in {"preinstall", "install", "qa", "github-ci"}:
        for path in paths:
            if suite == "github-ci":
                pattern = match_any(path, UPGRADE_GLOBS)
                if pattern:
                    return True, f"{path} matches upgrade-visible platform test pattern {pattern}"
            for group, globs in (
                ("core", CORE_GLOBS),
                ("raster", RASTER_GLOBS),
                ("topology", TOPOLOGY_GLOBS),
                ("loader", LOADER_GLOBS),
            ):
                pattern = match_any(path, globs)
                if pattern:
                    return True, f"{path} matches {group} test pattern {pattern}"
        return False, "no core, raster, topology, or loader test input changed"

    return True, f"unknown suite {suite!r}; running"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("suite")
    parser.add_argument("--target", default=os.environ.get("REGRESS_TARGET", ""))
    parser.add_argument("--extension", default=os.environ.get("UPGRADE_EXTENSION", "none"))
    parser.add_argument("--base")
    parser.add_argument("--head", default="HEAD")
    parser.add_argument("--upgrade-surface", action="store_true")
    args = parser.parse_args()

    if os.environ.get("CI_PIPELINE_EVENT") not in (None, "", "pull_request"):
        print(f"RUN: {os.environ.get('CI_PIPELINE_EVENT')} event runs full CI")
        return 0
    if os.environ.get("GITHUB_EVENT_NAME") not in (None, "", "pull_request"):
        print(f"RUN: {os.environ.get('GITHUB_EVENT_NAME')} event runs full CI")
        return 0

    try:
        paths, _ = changed_paths(args.base, args.head)
    except Exception as exc:
        print(f"RUN: changed-path discovery failed open: {exc}")
        return 0

    if not paths:
        print("RUN: changed-path list is empty; failing open")
        return 0

    need, reason = need_for_suite(paths, args.suite, args.target, args.extension)
    if need:
        print(f"RUN: {reason}")
        return 0

    if args.upgrade_surface:
        rc = run_upgrade_surface(args.base, args.head)
        if rc == 78:
            return 78
        return 0

    print(f"SKIP: {reason}")
    return 78


if __name__ == "__main__":
    raise SystemExit(main())
