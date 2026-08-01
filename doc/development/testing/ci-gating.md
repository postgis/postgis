---
title: "Pull Request CI Gating"
date: 2026-07-27
weight: 20
geekdocHidden: false
---

PostGIS pull requests keep the usual CI status contexts, but some expensive job
bodies are skipped when the changed files cannot affect them. The gate exists
to make pull-request testing proportional to the changed surface. It does not
change integration-branch coverage.

## What Is Gated

The gate runs only for pull-request events. Pushes, tags, `master`, and
`stable-*` integration branches run the full workflow set. If the gate cannot
identify the pull-request base, compute the changed paths, build the ABI
comparison trees, or compare symbols, it fails open and runs the suite.

The gated pull-request bodies are:

| CI surface | Gated body |
| ---------- | ---------- |
| Woodpecker regression matrix | build, install, preinstall, extension-upgrade, and cluster-upgrade work in `.woodpecker/regress.yml` |
| Woodpecker QA matrix | sanitizer and standard-conforming-strings-off work in `.woodpecker/qa.yml` |
| GitHub Actions mirror workflows | Linux Docker, FreeBSD, macOS, MSYS2, and CodeQL build or analysis bodies |

The status context still appears. A skipped body exits successfully after
printing a `SKIP:` reason from `ci/ci_need_suite.py`.

Documentation checks owned by `.woodpecker/docs.yml` are not reduced by this
gate. Cheap setup steps that exist only to keep workflow structure intact may
still run before the expensive body is skipped.

## What Always Runs

Pull requests run the relevant expensive body when they change CI, build, or
test-control files, including `.woodpecker/*.yml`, `.github/workflows/*.yml`,
`ci/**`, `configure.ac`, `autogen.sh`, `GNUmakefile.in`, `Makefile.in`,
`*/Makefile.in`, `macros/**`, and `build-aux/**`.

Subsystem source changes run the matching subsystem coverage:

| Changed surface | Pull-request coverage |
| --------------- | --------------------- |
| `liblwgeom/**`, `libpgcommon/**`, `postgis/**`, `regress/core/**`, `regress/dumper/**`, `deps/**`, `sfcgal/**` | core build, CUnit, and core regression coverage |
| `raster/**`, `extensions/postgis_raster/**` | raster regression and raster upgrade coverage |
| `topology/**`, `extensions/postgis_topology/**` | topology regression and topology upgrade coverage |
| `loader/**`, `regress/loader/**` | loader regression coverage |
| `Version.config`, `extensions/**`, SQL template files, upgrade generators, `postgis_restore.pl`, `utils/check_all_upgrades.sh`, `utils/check_cluster_upgrade.sh` | extension, double-upgrade, locked-upgrade, cluster-upgrade, and platform workflow coverage |
| `NEWS`, `utils/docs/check_news.sh`, `utils/docs/tests/test_check_news.py` | NEWS checker coverage when that suite uses the gate |

Unknown suites run rather than skip.

## Upgrade And ABI Decisions

Upgrade suites first check SQL-visible paths. Any change to extension SQL,
upgrade generators, version metadata, restore helpers, or upgrade scripts runs
the upgrade coverage without an ABI comparison.

If no SQL-visible path changed but a compiled module source changed, the gate
builds the merge base and pull-request head in temporary worktrees and compares
the exported modules. It uses `abidiff` when available. In a Debian CI
container running as root, it tries to install `abigail-tools` to get
`abidiff`. If `abidiff` is unavailable, it falls back to
`nm -D --defined-only`.

The `nm` fallback has an important blind spot: it detects added and removed
exported symbol names, but it cannot prove that a symbol's type, arguments, or
ABI contract stayed compatible. Because of that blind spot, unchanged symbol
names under the `nm` fallback still run the upgrade tests.

## Why Integration Branches Are Exempt

`master` and every `stable-*` branch are release and backpatch integration
surfaces. They need complete, comparable CI history even when an individual
merge looked narrow during review. The gate therefore applies only to
pull-request events. Once a change lands, push CI runs the full set.

## How To Tell Whether A Pull Request Was Gated

Open the CI job log for a skipped or unexpectedly short context and look for
the `ci/ci_need_suite.py` verdict near the start of the job body.

`RUN:` means the expensive body was kept. The line includes the file or rule
that required the suite, or says that discovery failed open.

`SKIP:` means the expensive body was intentionally skipped. The line includes
the reason, such as no matching subsystem input or no upgrade-relevant change.

On GitHub Actions mirror workflows, the "Check changed surface" step writes the
skip decision and later expensive steps are marked skipped. On Woodpecker,
matrix entries keep their workflow context and exit successfully after the
`SKIP:` verdict.

To reproduce a decision locally, fetch the pull-request base and run the same
helper against the branch:

```sh
python3 ci/ci_need_suite.py install --target check --extension none --base upstream/master
python3 ci/ci_need_suite.py extension-upgrade --target check --extension postgis --upgrade-surface --base upstream/master
```

Status `0` means run. Status `78` means skip.

## How To Force The Full Set

For maintainers, the clean force path is to test the branch after it is merged
or otherwise pushed to an integration ref, because non-pull-request events
always run the full set.

For a pull request that must prove the full matrix before merge, add a small
explicit change to a CI or build-control file owned by the gate, such as a
comment in `.woodpecker/regress.yml`, `.woodpecker/qa.yml`, or the affected
GitHub workflow. CI/build-control changes match the always-run patterns and
force the expensive pull-request bodies. Remove the temporary forcing change
before merge if it is not part of the intended patch.

If a forced run exposes that the gate skipped a suite it should have run, fix
the glob or ABI rule in `ci/ci_need_suite.py` or
`utils/check_upgrade_surface.py` instead of relying on the temporary forcing
change.
