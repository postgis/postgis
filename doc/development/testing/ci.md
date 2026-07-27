---
title: "CI Inventory Standards"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

The old Trac `ContinuousIntegration` page is a useful warning: a build-badge
dashboard combines live service URLs, branch names, CI job names, dependency
versions, and human-readable host notes. That kind of table becomes stale
quickly if it is edited as prose.

Keep durable testing guidance in this repository, but treat live badge matrices
as generated or checkable inventory. A maintained CI inventory should follow
these rules.

## Source of Truth

Do not duplicate CI behavior by hand. Point each inventory entry at the file or
service that owns the behavior:

| CI surface | Source of truth |
| ---------- | --------------- |
| Linux Docker matrix on GitHub Actions | `.github/workflows/ci.yml` plus `ci/github/run_*.sh` |
| FreeBSD GitHub Actions job | `.github/workflows/ci-freebsd.yml`; see [FreeBSD development environment](../environment/freebsd.md) |
| macOS GitHub Actions job | `.github/workflows/ci-macos.yml`; see [macOS development environment](../environment/macos.md) |
| MSYS2/MinGW GitHub Actions job | `.github/workflows/msys.yml` |
| GitHub CodeQL, codespell, and contributor-credit jobs | `.github/workflows/codeql.yml`, `.github/workflows/codespell.yml`, and `.github/workflows/contributor-credits.yml` |
| Woodpecker pull-request and branch pipelines | `.woodpecker/*.yml`, with pipeline status published by Woodie at <https://woodie.osgeo.org/repos/30> |
| Debbie build, docs, and release jobs | `ci/debbie/`, the Debbie Jenkins jobs, and release-process notes |
| Winnie Windows jobs | `ci/winnie/` and the Winnie Jenkins jobs |
| Bessie and Berrie/Berrie64 jobs | `ci/bessie/`, `ci/berrie*`, and the corresponding Jenkins worker labels |
| Docker build images used by GitHub Actions | `postgis/postgis-build-env` image tags referenced from `.github/workflows/ci.yml` |
| Docker build images used by Woodpecker | `repo.osgeo.org/postgis/build-test:*` image tags referenced from `.woodpecker/*.yml` |
| Woodpecker MinGW Wine job | `.woodpecker/mingw-wine.yml` and `ci/woodie/postgis_mingw_wine.sh` |

When a dashboard row describes dependency versions, operating systems, branch
coverage, or test modes, the row should be regenerated from those sources or
checked against them before publication.

## Build Parallelism Sizing

The docs pipeline in `.woodpecker/docs.yml` sets job parallelism through:

```sh
. ../ci/parallel-jobs.sh
export POSTGIS_BUILD_JOBS=$$(postgis_ci_parallel_jobs)
make -j"$${POSTGIS_BUILD_JOBS}" SUBDIRS="deps liblwgeom libpgcommon postgis"
```

This keeps small shared Woodpecker agents from multiplying memory pressure until
the server reports `received oom kill`. `postgis_ci_parallel_jobs()` uses the
following overrides:

- `POSTGIS_CI_MEM_AVAILABLE_KB` sets the available memory (KB) used by scheduling.
  Default: auto-detected from cgroup `memory.max` and `/proc/meminfo` (the
  smaller value is used). Set this when CI memory reporting is missing, noisy,
  or known to disagree with the worker capacity assigned to the job.
- `POSTGIS_CI_MAX_JOBS` sets the hard upper limit on parallel jobs.
  Default: detected CPU count, and then clamped to detected CPU count.
  Set this when CI capacity should be capped regardless of available memory.
- `POSTGIS_CI_JOB_MEMORY_MB` sets the per-job memory estimate (MB).
  Default: `1024`. Set this when one job class needs materially more or less
  memory than the default docs build assumption.

The result is clamped to at least `1`, and if memory detection fails the function
falls back to the CPU-based maximum.

Woodpecker is the CI surface attached to canonical Gitea pull requests. Its
status contexts are produced from the checked-in `.woodpecker/` workflows, so a
new Woodpecker job belongs there first, then in the generated dashboard
inventory described below. Do not document the current split count by hand:
parallel matrix expansion and retries change the number of published
`ci/woodpecker/...` contexts for a commit.

On `master`, the maintained Woodpecker workflows include regression, docs,
tools, codespell, contributor-credit, and QA coverage. The QA workflow runs
sanitizer and `standard_conforming_strings=off` checks for pull requests, while
the expensive QA workflow is branch-limited and owns coverage and garden
checks. Read `.woodpecker/qa.yml` and `.woodpecker/qa-expensive.yml` before
changing that split.

## CI Parity Map

The repository now keeps CI ownership split across several services. Use this
map to decide which service is authoritative for a platform-specific failure,
and whether a Woodpecker workflow is intended to replace another service or
only overlap part of its defect class.

| Surface | Current authoritative provider | Woodpecker coverage |
| ------- | ------------------------------ | ------------------- |
| Linux build and regression checks | Woodpecker and GitHub Actions | Covered on amd64 by `.woodpecker/regress.yml`, `.woodpecker/qa.yml`, and `.woodpecker/qa-expensive.yml`. The Woodpecker matrix is not byte-for-byte identical to GitHub's Docker matrix, but it covers the standard Linux build, CUnit, install, extension-upgrade, cluster-upgrade, sanitizer, standard-conforming-strings-off, coverage, and garden classes. |
| Codespell | Woodpecker and GitHub Actions | Covered by `.woodpecker/codespell.yml`. GitHub pins a specific `codespell` package version; Woodpecker follows the OSGeo build image unless that workflow is pinned separately. |
| Contributor credits | Woodpecker and GitHub Actions | Covered by `.woodpecker/contributor-credits.yml`. |
| Debbie Linux regression classes | Jenkins Debbie and Woodpecker | Mostly covered by Woodpecker's Linux regression and expensive QA workflows. Debbie still remains useful for its exact Jenkins host, dependency, and release-job environment. |
| Make Dist | Jenkins Debbie | In flight in <https://gitea.osgeo.org/postgis/postgis/pulls/534>. Until that lands, Woodpecker does not check source distribution tarballs. |
| FreeBSD and Bessie | GitHub Actions FreeBSD and Jenkins Bessie | In flight in <https://gitea.osgeo.org/postgis/postgis/pulls/549>. YAML running on a Linux container is not FreeBSD parity; this needs a FreeBSD VM or agent surface. |
| 32-bit ARM and extra portability tiers | Jenkins Berrie | Covered by `.woodpecker/portability.yml` from <https://gitea.osgeo.org/postgis/postgis/pulls/516>, with hostile type-default coverage proposed in <https://gitea.osgeo.org/postgis/postgis/pulls/550>. Plain armhf emulation is useful for pointer-width and alignment assumptions, but the valuable tier is the hostile configuration with explicit type, signedness, alignment, and sanitizer probes. |
| 64-bit ARM | Jenkins Berrie64 | Covered by `.woodpecker/arm64.yml` only when the fleet has a native `linux/arm64` agent. The workflow intentionally has no QEMU fallback. |
| CodeQL | GitHub Actions | In flight as Woodpecker configuration carried separately. Woodpecker can build a CodeQL database and produce SARIF, but GitHub remains authoritative for code-scanning upload, annotations, and alert management unless Woodie artifact retention and SARIF consumption are also configured. |
| macOS | GitHub Actions macOS | Not coverable by Woodpecker YAML on Linux. See [macOS coverage options](macos-coverage-options.md) for the actual choices, costs, licensing boundary, and current recommendation. |
| Native Windows MSYS2 and Winnie | GitHub Actions MSYS2 and Jenkins Winnie | Not covered natively by Linux Woodpecker. MinGW+Wine coverage is valuable ABI/runtime coverage but not native Windows parity. |

The companion inventory and debugging documents are deliberately narrower than
this parity map. Dashboard ownership, CI image provenance, and failure-debugging
procedures should stay in their own sections rather than being repeated here;
use this section to answer the parity question.

### MinGW And Native Windows

MinGW+Wine coverage is not native Windows coverage. The Woodpecker MinGW+Wine
workflow cross-compiles PostGIS and dependencies, links Windows binaries and
DLLs, starts Windows PostgreSQL binaries under Wine, and runs loaders, CUnit,
and SQL regressions in that environment. That is strong MinGW ABI coverage.

It does not replace GitHub MSYS2 or Winnie. Native Windows remains authoritative
for NT path handling and path-length behavior, drive-letter and UNC semantics,
MSYS2 path translation, service registration and Service Control Manager
behavior, Windows CRT, locale and codepage behavior, native threading and file
locking, DLL search order, and failures where Wine itself may be the broken
component.

### macOS

macOS cannot be covered by Linux containers, Wine, or cross-compilation. Darwin
libc, the Mach-O dynamic loader, Homebrew's dependency graph, Apple clang, the
filesystem, and codesign or SIP-adjacent behavior have to run on macOS to be
meaningful. The realistic choices are documented in
[macOS coverage options](macos-coverage-options.md): keep GitHub Actions as the
Darwin lane, run a hosted or project-owned Apple Woodpecker agent, or accept no
Darwin coverage.

### ARM

Treat 32-bit and 64-bit ARM as separate defect classes. The 32-bit lane is most
useful when it is hostile: explicit `char` signedness, enum-width, alignment,
and sanitizer settings catch assumptions that amd64 and a plain Berrie rerun do
not falsify.

The 64-bit lane should be native. Berrie64's value is 64-bit ARM execution plus
garden and all-upgrades coverage. QEMU can compile and run some smoke tests, but
it makes the expensive suites too slow and can hide timing, atomic-operation,
kernel, and native scheduling behavior. The Woodpecker arm64 workflow therefore
requires a registered `linux/arm64` agent and intentionally avoids an emulated
fallback.

## Woodie API and Pipeline Approvals

Use a Woodie personal access token for the Woodie API and
[`woodpecker-cli`](https://woodpecker-ci.org/docs/cli). A Gitea access token is
a separate credential and is not accepted by the Woodie API.

Generate the token from the Woodie user page at
<https://woodie.osgeo.org/user>. Do not print it, commit it, or put the literal
value in shell history. Configure a named CLI context and verify the account
before changing a pipeline:

```bash
read -rs WOODPECKER_TOKEN
printf '\n'
umask 077
woodpecker-cli setup --context osgeo \
  --server https://woodie.osgeo.org \
  --token "$WOODPECKER_TOKEN"
unset WOODPECKER_TOKEN
woodpecker-cli context use osgeo
woodpecker-cli info
```

Before approving a blocked pipeline, inspect its API record at
`/api/repos/<repo-id>/pipelines/<pipeline-number>`. Confirm the repository,
pipeline number, commit, ref, event, and current state all match the pull request
that needs approval. Approve only that pipeline:

```sh
woodpecker-cli pipeline show <repo-id-or-full-name> <pipeline-number>
woodpecker-cli pipeline approve <repo-id-or-full-name> <pipeline-number>
woodpecker-cli pipeline show <repo-id-or-full-name> <pipeline-number>
```

An accepted approval command is not a successful build. Read the pipeline back,
verify that the reviewer and commit are unchanged, and wait for a terminal
state. Check every required workflow and failed child step before reporting the
pipeline as green or diagnosing a source failure.

## Badge Inventory Rules

For every badge entry, record enough metadata for another maintainer to verify
it without guessing:

* service name and canonical job or workflow URL;
* badge image URL, if the dashboard renders one;
* branch or release line covered by the badge;
* source file, script, or external job configuration that owns the job;
* test mode covered, such as standard tests, coverage, garden, upgrade,
  dump/restore, sanitizer, docs, or release distribution build;
* expected platform and dependency family, when that information is not already
  obvious from the owning workflow or image tag.

Avoid free-form dependency summaries when the source is an image tag, package
manager command, Jenkins job, or workflow matrix. Prefer linking to the source
or generating the summary. If a summary is kept for readability, it needs the
same check path as the badge URL.

## Updating The Inventory

Update the inventory when any of these change:

* a workflow file under `.github/workflows/`;
* a workflow file under `.woodpecker/`;
* a script under `ci/`;
* a `postgis/postgis-build-env` tag used by the GitHub Actions matrix;
* a `repo.osgeo.org/postgis/build-test` tag used by a Woodpecker workflow;
* a Jenkins job, worker label, or badge URL referenced from Trac or website
  dashboards;
* a supported release branch or support-window row that affects which branches
  should have badges.

The checked-in inventory is `utils/docs/ci_status/config.json`. Update that file
instead of copying badge rows into prose. Run the command from the repository
root:

```sh
python3 utils/docs/ci_status.py
python3 utils/docs/ci_status.py --branch stable-3.6
python3 utils/docs/ci_status.py --format json
python3 utils/docs/ci_status.py --format json --output /tmp/postgis-ci/status.json
```

The default terminal report checks non-EOL branches and prints details only for
checks that need attention. Use `--verbose` for the complete inventory and `--include-eol` for
historical branches. GitHub access can use `GITHUB_TOKEN` or `GH_TOKEN`; when the
API is unavailable, eligible workflow checks fall back to their public badge.

Each configured check names its provider, branch scope, canonical URL, and
whether it contributes to the required rollup. Add a branch when it opens and
mark it `"eol": true` when project support ends. Retire a check by setting its
provider to `disabled`, setting `required` to false, and preserving a concise
reason in `message`. Do not delete retired entries until their history is no
longer useful for interpreting the dashboard.

For publication, `--output` writes the JSON file atomically and succeeds even
when required CI is red, because a red status is valid dashboard data. The
checks in each branch are ordered by operational status, with failures and
uncertain results before running and passing checks. An unrecognized provider
status is published as a red failure row with the reported value instead of
preventing the rest of the dashboard from being generated. The
`postgis.net` repository owns the Hugo page and browser renderer that consume
this file; this repository does not emit website HTML or CSS. Release work still
needs a current provider readback: generated data proves inventory and reporting
behavior, not that every external service is healthy.

Run `python3 utils/docs/tests/test_ci_status.py` after changing provider parsing,
cache behavior, summary reduction, or JSON publication. The package layout and
the other documentation utilities are described in `utils/docs/README.md`.

## Relationship To Other Docs

Use [Testing and debugging](_index.md) for test commands and local validation
workflows. Use [Pull request CI gating](ci-gating.md) for the changed-surface
rules that can skip expensive pull-request bodies while keeping status contexts
present. Use [Release process](../release-process.md) for release-manager
greenlight checks, Debbie release jobs, and branch-opening steps. Use [Website
maintenance](../website.md) and the public website compatibility matrix for
user-facing support status.

## MinGW Wine Coverage Limits

The Woodpecker MinGW Wine job cross-builds Windows binaries with MinGW and runs
them under Wine against a Windows PostgreSQL binary distribution. This gives the
project pull-request-visible Windows binary coverage when native Windows CI is
not available, but it is not native Windows parity.

Keep the remaining gaps explicit when changing this job:

* Native Windows filesystem behavior is not covered: NT path handling and path
  length, drive-letter and UNC semantics, and MSYS2 path translation are outside
  Wine's Linux-hosted process model.
* Native Windows service behavior is not covered: service registration and SCM
  operation are not exercised by a Wine-started PostgreSQL process.
* Native Windows runtime behavior is only partially covered: Wine can expose
  Windows CRT, locale, codepage, threading, file-locking, and DLL-search
  issues, but it is not a substitute for running the same binaries on Windows.
* SFCGAL is not built in this job because Debian does not provide MinGW-targeted
  SFCGAL or CGAL packages. Enabling it requires cross-building at least the
  SFCGAL, CGAL, Boost thread/serialization, GMP, MPFR, and nlohmann-json stack
  before PostGIS is configured with `--with-sfcgal`.
