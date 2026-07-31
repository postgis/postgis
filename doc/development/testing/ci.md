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

`postgis_ci_parallel_jobs()` uses the following overrides:

- `POSTGIS_CI_MEM_AVAILABLE_KB` sets the available memory (KB) used by scheduling.
  Default: auto-detected from cgroup `memory.max` and `/proc/meminfo` (the
  smaller value is used). Set this when a CI worker reports wrong available memory
  and the detected value is not suitable.
- `POSTGIS_CI_MAX_JOBS` sets the hard upper limit on parallel jobs.
  Default: detected CPU count, and then clamped to detected CPU count.
  Set this when CI capacity should be capped regardless of available memory.
- `POSTGIS_CI_JOB_MEMORY_MB` sets the per-job memory estimate (MB).
  Default: `1024`. Set this when one job class needs materially more/less memory
  than the default and parallelism must be reduced/increased.

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

Use `utils/ci-trac-line.sh` when maintaining the Trac badge table or a copied
badge inventory. The generated row covers the maintained repository-owned
status inventory and omits retired mirror badges. Fetch or export the current
Trac table into a temporary file before using `--check` or `--replace`:

```sh
utils/ci-trac-line.sh 3.7
utils/ci-trac-line.sh --check /tmp/ContinuousIntegration.tracwiki 3.7
utils/ci-trac-line.sh --replace /tmp/ContinuousIntegration.tracwiki 3.7
```

The helper prints the expected row for a stable release branch, verifies that a
file already contains the generated row, or replaces an existing row for that
release line. It checks row shape, branch names, and badge URLs generated by
the helper. It does not prove that external Jenkins, GitHub Actions, GitLab, or
Woodie services are currently green; release work still needs a live service
readback.

## Relationship To Other Docs

Use [Testing and debugging](_index.md) for test commands and local validation
workflows. Use [Pull request CI gating](ci-gating.md) for the changed-surface
rules that can skip expensive pull-request bodies while keeping status contexts
present. Use [Release process](../release-process.md) for release-manager
greenlight checks, Debbie release jobs, and branch-opening steps. Use [Website
maintenance](../website.md) and the public website compatibility matrix for
user-facing support status.
