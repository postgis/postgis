---
title: "Backpatching Fixes"
date: 2026-06-27
weight: 20
geekdocHidden: false
---

Backpatching keeps supported release lines consistent without turning stable
branches into feature branches. Use this checklist when a bug fix, release-line
CI fix, or security fix may need to move between `master` and supported
`stable-*` branches.

## Supported Branch Set

Start from the active support window. At the time of writing, the supported
branch set is `master` through `stable-3.2`. Exclude end-of-life branches unless
the release manager explicitly asks for an exceptional backpatch.

Confirm the current branch state before editing:

* fetch the canonical OSGeo Gitea repository;
* inspect `Version.config` and the top `NEWS` section on each target branch;
* check for existing open pull requests that already carry part of the same
  backpatch lineage.

GitHub pull requests are review surfaces. The canonical source branch remains
the OSGeo Gitea branch.

## Pull Request Family Shape

Keep one pull request per target branch in a backpatch family: one common
`master` pull request when the lineage still needs `master` work, plus one pull
request for each supported `stable-*` branch that needs the fix. If several
`master`-only follow-ups are found while repairing the family, fold them into
the common `master` pull request instead of opening separate `master` pull
requests by subtopic.

Cross-link the common `master` pull request and the stable pull requests in
their bodies. When a subtopic is intentionally `master`-only, say that in the
stable pull-request bodies instead of creating a second `master` lane.

## First Pass: Older Fixes Missing From Newer Branches

Before pushing changes from new branches to old branches, scan in the opposite
direction. A fix may have been committed directly to an older stable branch and
missed `master` or a newer stable line.

Compare adjacent branches:

* `stable-3.6` not in `master`
* `stable-3.5` not in `stable-3.6`
* `stable-3.4` not in `stable-3.5`
* `stable-3.3` not in `stable-3.4`
* `stable-3.2` not in `stable-3.3`

Classify only real fix candidates: runtime correctness, crashes, security,
packaging, install or upgrade behavior, release-line CI blockers, and
user-facing documentation corrections. Ignore release-prep commits, version
bumps, translation churn, broad refactors, and feature work unless they expose a
missing regression test or `NEWS` entry for a real bug fix.

If an older branch contains a real fix that still applies to `master`, open a
pull request to `master` first. If `master` has already fixed the same bug by a
different implementation, consider whether the regression test or `NEWS` entry
is still missing before opening a code PR.

## Second Pass: Newer To Older

After the upward scan, walk from newer branches to older supported branches.
For each fix lineage, cherry-pick or port the fix until reaching the first older
branch that does not have the affected bug.

Stop when the affected code path, SQL function, extension object, build target,
or regression test does not exist on the older branch. Do not backport a new
feature only to make a bug fix fit an older release line.

Backport by branch capability, not by pull-request number range. For each
source hunk, check that the target branch has the same API surface and test
contract before carrying it over:

* C enum values, pixel types, version macros, and generated lookup tables;
* SQL wrappers versus C functions with planner support hooks;
* function signatures, default arguments, and sentinel values;
* build-system variables and recursive `make` entry points;
* supported PostgreSQL and dependency versions in CI;
* regression-test fixtures and expected output semantics.

Do not backpatch fuzzer harnesses, seed corpora, OSS-Fuzz input-target
plumbing, or fuzzer-only CI repairs to stable branches unless that exact stable
branch runs the fuzzer target. In the normal PostGIS branch layout, fuzzer
target work is `master`-only; stable branches should receive the runtime fix and
any ordinary regression coverage that exercises shipped code, not the fuzzer
driver used to find or smoke-test it.

If a source test only proves behavior introduced on newer branches, omit or
adapt that subtest on older branches. Keep the regression that protects the
portable bug fix, but do not force an expected row only because it appeared in
the newer branch. A missing expected row means: first inspect the SQL query and
the target branch's function semantics, then decide whether the code or the
test is non-portable.

Record the stop reason in the pull request or maintainer handoff, for example:

```text
Stops at stable-3.3 because stable-3.2 does not have CG_AlphaShape.
```

## Commit Shape And Provenance

Prefer one source commit to one target commit. Clean cherry-picks should use
`git cherry-pick -x` so the source commit hash is preserved.

When cherry-picking a commit that is already a cherry-pick, keep only the
provenance that helps reviewers trace the backpatch. Avoid stacking repeated
`(cherry picked from commit ...)` lines mechanically; collapse noisy duplicate
trailers into one clear source commit, or use a `Ported-from:` line for a manual
port.

When a clean cherry-pick is not possible, make the smallest faithful port and
record the source in the commit body:

```text
Ported-from: https://gitea.osgeo.org/postgis/postgis/commit/<sha>
```

If several source commits were iterative attempts to fix one bug, it is fine to
collapse them into a smaller, correct target series. Do not collapse unrelated
fixes into one commit, and do not rewrite public PR history only to hide later
CI follow-up commits. Once a branch has been pushed for review, append focused
follow-up commits for target-branch CI issues unless the maintainer explicitly
asks for a rewrite.

Use source-commit subjects where they remain accurate. If the target branch API
or file layout differs, adjust the subject to describe the target-branch fix
while keeping the `Ported-from:` line.

## NEWS

Assume a backpatch-worthy fix needs `NEWS` on every branch where it lands,
including `master`, unless investigation shows it is purely internal and not
release-note-worthy. A `NEWS` entry should describe the user-visible bug or
supported build behavior, not the branch archaeology.

Put the entry under the top release section for the target branch and match that
branch's existing heading style. Use:

* `#NNNN` for Trac tickets;
* `GH-NNN` for GitHub pull requests only inside `NEWS`;
* `GT-NNN` for Gitea pull requests only inside `NEWS`.

When a bug fix has no existing ticket, use concise user-facing wording and the
contributor name if the surrounding section uses contributor credits.

## Tests And Validation

Every code backpatch should carry a regression test when a focused test is
practical. For test-output drift caused by newer dependencies, make the test
less brittle when possible instead of only replacing expected output with one
machine's result.

Before pushing a backpatch PR, run at least:

* `git diff --check`;
* syntax or build smoke relevant to the touched files;
* the focused regression target when the target branch can realistically run in
  the local environment.

If local validation is not practical, state the exact blocker in the pull
request. After pushing, read back the PR head SHA and CI state. The goal is a
mergeable, green PR. If CI finds target-branch-only drift or harness breakage,
fix it in the same PR when it belongs to making that backpatch mergeable.

## Existing Backpatch Pull Requests

Before creating a new PR, check open stable-branch pull requests. Preserve an
existing PR when it is clean and correctly scoped. Supersede or repair it only
when it has the wrong base, misses commits from the same lineage, lacks `NEWS`
or tests, has misleading public text, or has conflicts that are cleaner to fix
by replacing the branch.

Do not fold unrelated master feature PRs into the backpatch sweep merely because
they are open. Include only stable-base PRs, clear backpatch PRs, and PRs that
the branch comparison proves are part of the fix lineage.

## Pull Request Body

The PR body should let reviewers trace the fix without reading local notes.
Include:

* the source commit or commits, preferably as OSGeo Gitea URLs;
* the target branch;
* whether the change was a clean cherry-pick, manual port, or collapsed fix
  series;
* validation that was run, and any validation that was not practical;
* any stop reason for older branches.

For ticketed fixes, include a concrete Trac follow-up line for the oldest branch
that will contain the fix after merge:

```text
After merge: update https://trac.osgeo.org/postgis/ticket/6081 fixed version to 3.5.7.
```

Use the target branch's next release version from `Version.config` and `NEWS`.
If the same ticket lands on several branches, the follow-up should name the
oldest supported release that contains the fix.
