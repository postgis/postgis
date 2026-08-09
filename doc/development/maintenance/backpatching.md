---
title: "Backpatching Fixes"
date: 2026-06-27
weight: 20
geekdocHidden: false
---

Use this checklist to propagate bug fixes between `master` and supported
`stable-*` branches. For general branch, commit, and authorship rules, see
[Commit and branch guidelines](commit-guidelines.md). For pull request labels,
milestones, and draft state, see
[Pull request metadata](pull-request-metadata.md).

## Select Target Branches

Determine the current support window from the release state. Fetch the
canonical OSGeo Gitea branches, then inspect `Version.config` and the top
`NEWS` section on each candidate branch. Do not backpatch to an end-of-life
branch without release-manager approval.

Check open pull requests before preparing a branch. An existing pull request
may already contain all or part of the same fix lineage.

## Scan In Both Directions

First scan from older branches towards newer branches. Compare adjacent release
lines to find fixes committed directly to an older stable branch but missing
from a newer branch or `master`.

Then scan from newer branches towards older supported branches. For each fix,
continue until the first branch where the bug or affected code does not exist.
Do not introduce a feature or a new API only to make a fix apply to an older
release.

Classify candidates by behavior, not by commit or pull request ranges. Include
fixes for runtime correctness, crashes, security, installation, upgrades,
packaging, supported builds, and user-facing documentation. Exclude version
bumps, release preparation, translations, refactoring, and feature work unless
they contain a distinct backpatchable fix.

Before porting a fix, check the target branch's capabilities. Relevant
differences may include:

* C and SQL APIs, function signatures, and generated tables;
* build targets and dependency versions;
* extension objects and upgrade paths;
* regression fixtures and expected semantics.

Stop when the affected behavior is absent or the fix would require unsupported
infrastructure. Record the reason in the pull request or maintainer handoff.

## Inspect Split Component Repositories

A backpatch sweep must inspect relevant split component repositories, even when
the report or pull request names only the PostGIS tree. Use the repository links
in the [PostGIS project inventory](../governance/project-inventory.md). Review
component commits, pull requests, release notes, and tests for the same bug and
for follow-up fixes. Do not limit the sweep to commits already present in the
PostGIS repository.

For new work, use the component repository. When a supported stable branch
still contains that component in the PostGIS tree, port the applicable fix to
the branch's in-tree source:

* `address_standardizer` uses `extensions/address_standardizer/`;
* `postgis_tiger_geocoder` uses `extras/tiger_geocoder/`.

Inspect the target branch's files, build rules, generated data, loader scripts,
and regression targets before editing. Carry only the code fix and tests that
fit that branch. Leave component-only packaging, CI, release, and repository
layout changes in the component repository.

If the fix changes generated or year-specific SQL, use the target branch's
generation path when it exists and produces a focused change. Otherwise update
the stored SQL directly and keep the diff limited to the affected behavior.

Adapt tests to the target branch's public behavior. Do not copy expected output
for functions or cases that the branch does not support.

## Preserve Provenance

Prefer one source commit per target commit. Use `git cherry-pick -x` for a clean
cherry-pick. For a manual port, record the source commit in the commit body, for
example:

```text
Ported-from: https://gitea.osgeo.org/postgis/postgis/commit/<sha>
```

Keep separate source fixes separate in the target branch. Do not squash
unrelated component commits into one broad backpatch commit only because they
were found in the same sweep. If a target commit is mainly a manual port of one
source commit, use that source commit's author. Add `Co-authored-by` trailers
only for people who helped write that target commit.

If several source commits are attempts at one fix, they may be reduced to a
smaller correct series. Follow [Commit and branch guidelines](commit-guidelines.md)
for subjects, tracker references, and authorship.

## Update NEWS

Inspect the top `NEWS` section on every target branch. Add an entry when the fix
is user-visible, release-note-worthy, security-relevant, or tracked as a bug.
Use the target branch's current release section and existing heading style.
Describe the fixed behavior, not the branch history.

Keep wording consistent across branches, but adjust details that differ by
release line. Follow [Commit and branch guidelines](commit-guidelines.md) for
tracker notation and contributor credit.

## Validate Each Port

Run validation on each target branch; success on the source branch is not
evidence for the port. At minimum, run:

* `git diff --check`;
* the relevant build or syntax check;
* the focused regression target when the branch and local environment support
  it;
* the repository's `NEWS` checks when `NEWS` changes.

Use [Testing and debugging](../testing/_index.md) for build and regression
commands. If a focused test cannot run, state the exact blocker and the
validation that was completed.

The pull request or handoff should identify the source commit, whether the
change is a cherry-pick or manual port, validation performed, and the stop
reason for any older branch that was considered but not changed.
