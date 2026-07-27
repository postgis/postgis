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
| Debbie build, docs, and release jobs | `ci/debbie/`, the Debbie Jenkins jobs, and release-process notes |
| Winnie Windows jobs | `ci/winnie/` and the Winnie Jenkins jobs |
| Bessie and Berrie/Berrie64 jobs | `ci/bessie/`, `ci/berrie*`, and the corresponding Jenkins worker labels |
| Docker build images used by GitHub Actions | `postgis/postgis-build-env` image tags referenced from `.github/workflows/ci.yml` |
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
* a script under `ci/`;
* a `postgis/postgis-build-env` tag used by the GitHub Actions matrix;
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
