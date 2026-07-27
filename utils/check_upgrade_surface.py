#!/usr/bin/env python3
"""Check whether a change touches the PostGIS upgrade-relevant surface.

The verdict is conservative.  Status 78 means the expensive upgrade suite may be
skipped.  Status 0 means it must run, including every uncertain condition.
"""

from __future__ import annotations

import argparse
import fnmatch
import os
import shutil
import subprocess
import sys
from pathlib import Path


SQL_VISIBLE_GLOBS = (
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
)

MODULE_GLOBS = (
    "liblwgeom/**",
    "postgis/**",
    "raster/rt_pg/**",
    "topology/**",
    "sfcgal/**",
)

MODULE_PATTERNS = (
    "liblwgeom/.libs/liblwgeom-*.so*",
    "postgis/.libs/postgis-*.so*",
    "postgis/postgis-*.so*",
    "raster/rt_pg/.libs/postgis_raster-*.so*",
    "raster/rt_pg/postgis_raster-*.so*",
    "topology/.libs/postgis_topology-*.so*",
    "topology/postgis_topology-*.so*",
    "sfcgal/.libs/postgis_sfcgal-*.so*",
    "sfcgal/postgis_sfcgal-*.so*",
)


def run(args: list[str], cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=cwd,
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )


def git(args: list[str], check: bool = True) -> str:
    return run(["git", *args], check=check).stdout


def match_any(path: str, globs: tuple[str, ...]) -> str | None:
    for pattern in globs:
        if fnmatch.fnmatch(path, pattern):
            return pattern
    return None


def resolve_base(base: str | None, head: str) -> tuple[str, list[str]]:
    cmd = ["python3", "ci/ci_changed_paths.py", "--head", head]
    if base:
        cmd.extend(["--base", base])
    result = run(cmd, check=False)
    if result.returncode != 0:
        raise RuntimeError(result.stdout.strip() or "changed path helper failed")
    paths = [line for line in result.stdout.splitlines() if line]

    resolved_base = base
    if not resolved_base:
        for candidate in ("refs/ci-target/master", "upstream/master", "origin/master", "master"):
            if run(["git", "rev-parse", "--verify", candidate], check=False).returncode == 0:
                resolved_base = candidate
                break
    if not resolved_base:
        raise RuntimeError("no base ref available for symbol comparison")
    merge_base = git(["merge-base", resolved_base, head]).strip()
    if not merge_base:
        raise RuntimeError(f"no merge base between {resolved_base} and {head}")
    return merge_base, paths


def build_tree(ref: str, root: Path, name: str) -> Path:
    tree = root / name / "src"
    build = root / name / "build"
    if tree.exists():
        shutil.rmtree(tree)
    if build.exists():
        shutil.rmtree(build)
    run(["git", "worktree", "add", "--detach", str(tree), ref])
    build.mkdir(parents=True, exist_ok=True)
    configure = [
        str(tree / "configure"),
        "--with-library-minor-version",
        "--without-raster",
        "--without-topology",
        "--without-sfcgal",
        "--without-protobuf",
        "--disable-spellcheck-tests",
    ]
    run([str(tree / "autogen.sh")], cwd=tree)
    run(configure, cwd=build)
    run(["make", "-j1", "SUBDIRS=liblwgeom libpgcommon postgis"], cwd=build)
    return build


def module_files(build: Path) -> list[Path]:
    files: list[Path] = []
    for pattern in MODULE_PATTERNS:
        files.extend(build.glob(pattern))
    return sorted(path for path in files if path.is_file())


def relative_module_map(build: Path) -> dict[str, Path]:
    result: dict[str, Path] = {}
    for path in module_files(build):
        rel = str(path.relative_to(build))
        result[rel] = path
    return result


def symbol_set(path: Path) -> set[str]:
    output = run(["nm", "-D", "--defined-only", str(path)]).stdout
    symbols: set[str] = set()
    for line in output.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            symbols.add(parts[-1])
    return symbols


def compare_symbols(base_build: Path, head_build: Path) -> tuple[bool, str]:
    abidiff = ensure_abidiff()
    base_modules = relative_module_map(base_build)
    head_modules = relative_module_map(head_build)
    names = sorted(set(base_modules) | set(head_modules))
    if not names:
        return True, "no built modules found for comparison"

    for name in names:
        if name not in base_modules:
            return True, f"exported module added: {name}"
        if name not in head_modules:
            return True, f"exported module removed: {name}"
        if abidiff:
            diff = run([abidiff, str(base_modules[name]), str(head_modules[name])], check=False)
            if diff.returncode not in (0,):
                return True, f"abidiff reports ABI change in {name}"
        else:
            base_symbols = symbol_set(base_modules[name])
            head_symbols = symbol_set(head_modules[name])
            if base_symbols != head_symbols:
                added = sorted(head_symbols - base_symbols)
                removed = sorted(base_symbols - head_symbols)
                sample = (removed or added)[0]
                direction = "removed" if removed else "added"
                return True, f"defined symbol {direction} in {name}: {sample}"

    if not abidiff:
        return True, "abidiff is unavailable; nm fallback found no added or removed symbols but cannot prove signatures unchanged"
    return False, "no SQL-visible path or exported ABI change"


def ensure_abidiff() -> str | None:
    abidiff = shutil.which("abidiff")
    if abidiff:
        print(f"INFO: using abidiff at {abidiff}")
        return abidiff

    if os.geteuid() != 0 or not shutil.which("apt-get"):
        print("INFO: abidiff is unavailable and cannot be installed by this user")
        return None

    install = run(
        [
            "sh",
            "-c",
            "apt-get update && apt-get install -y --no-install-recommends abigail-tools && rm -rf /var/lib/apt/lists/*",
        ],
        check=False,
    )
    if install.returncode != 0:
        print("INFO: installing abigail-tools failed; falling back to nm")
        print(install.stdout)
        return None

    abidiff = shutil.which("abidiff")
    if abidiff:
        print(f"INFO: installed abidiff at {abidiff}")
    else:
        print("INFO: abigail-tools install completed but abidiff is still unavailable")
    return abidiff


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base")
    parser.add_argument("--head", default="HEAD")
    parser.add_argument(
        "--work-dir",
        default=os.environ.get("TMPDIR", ".tmp") + "/upgrade-surface",
    )
    args = parser.parse_args()

    try:
        merge_base, paths = resolve_base(args.base, args.head)
    except Exception as exc:
        print(f"RUN: upgrade-surface discovery failed open: {exc}")
        return 0

    if not paths:
        print("RUN: changed-path list is empty; failing open")
        return 0

    for path in paths:
        pattern = match_any(path, SQL_VISIBLE_GLOBS)
        if pattern:
            print(f"RUN: {path} matches SQL-visible upgrade pattern {pattern}")
            return 0

    if not any(match_any(path, MODULE_GLOBS) for path in paths):
        print("SKIP: no upgrade-relevant change (no SQL-visible path or module source changed)")
        return 78

    root = Path(args.work_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    try:
        base_build = build_tree(merge_base, root, "base")
        head_build = build_tree(args.head, root, "head")
        needs_upgrade, reason = compare_symbols(base_build, head_build)
    except Exception as exc:
        print(f"RUN: symbol comparison failed open: {exc}")
        return 0
    finally:
        for tree in (root / "base" / "src", root / "head" / "src"):
            if tree.exists():
                run(["git", "worktree", "remove", "--force", str(tree)], check=False)

    if needs_upgrade:
        print(f"RUN: {reason}")
        return 0

    print(f"SKIP: no upgrade-relevant change ({reason})")
    return 78


if __name__ == "__main__":
    raise SystemExit(main())
