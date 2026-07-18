---
title: "Pull Request Metadata"
date: 2026-07-01
weight: 30
geekdocHidden: false
---

Use labels, milestones, and draft state to make the maintainer queue sortable.
They are triage metadata, not a substitute for reading the pull request body,
the Trac ticket, the diff, current review threads, and CI.

The examples on this page are drawn from current and historical PostGIS pull
request metadata across GitHub and OSGeo Gitea. Recent live metadata does not
support treating these rules as belonging to one maintainer only; use them as
observed queue practice. Re-check the live forge before changing a large queue,
because labels, milestones, and release targets move during a release cycle.

## Metadata Surfaces

GitHub pull requests are the primary mirror review surface for labels,
milestones, review state, and CI readback. Assign review and release metadata
there when the pull request is on the GitHub mirror.

OSGeo Gitea remains the canonical source repository. Gitea pull requests may
also carry labels and milestones, especially for work reviewed there, but the
open Gitea queue can include workbench PRs with `WIP:` titles and synthetic
`mirror/github/<number>` head refs. GitHub and Gitea metadata are not a clean
one-to-one copy: titles, labels, and head refs may diverge even when they refer
to the same underlying branch. Do not synchronize labels blindly between
mirrors, and do not assume a missing Gitea label means the GitHub queue is
untriaged.

Trac remains the project issue tracker and roadmap surface. Milestones on pull
requests should be consistent with the Trac ticket milestone and the branch
that will contain the fix. When they disagree, inspect the ticket and target
branch before editing either forge.

## Drafts and Work In Progress

Use draft state for a pull request that is intentionally not ready for normal
review. Typical reasons are missing validation, unresolved design questions,
known broken CI, or a branch that is still being split or rebased.

Use the `work in progress` label only when the pull request must remain visible
as WIP even outside draft state, or on older pull requests where draft state was
not used. Do not use both draft state and `work in progress` mechanically.

On Gitea, a `WIP:` title is common for workbench pull requests. Keep that title
prefix until the branch is intended for maintainer review on that surface. If a
Gitea pull request mirrors a GitHub review PR, inspect both surfaces before
changing readiness metadata; one side may use title prefixes while the other
uses draft state or labels.

## Type Labels

Assign the smallest set of type labels that describes the user-visible change.
Most pull requests should need one type label; use more only when the branch
really crosses categories.

| Label | Use when |
|-------|----------|
| `bug fix` | The branch fixes incorrect behavior, crashes, data loss, security-sensitive behavior, packaging/install failures, or supported build/CI failures. Stable-branch backpatch PRs normally use this label. |
| `enhancement` | The branch improves existing behavior, performance, diagnostics, build coverage, or maintainability without adding a new user-facing API or feature family. |
| `new feature` | The branch adds a new SQL function, loader option, command-line option, extension capability, or other new user-facing feature. |
| `documentation` | The branch is primarily documentation, examples, website/manual/developer-doc updates, or documentation migration. Combine with `new feature`, `enhancement`, or `bug fix` only when the docs PR carries that substantive change too. |
| `test case only` | The branch adds or repairs tests without changing shipped behavior. Combine with `bug fix` only when the branch also patches runtime code. |
| `breaking change` | The branch intentionally removes, renames, or changes compatibility for a public API, behavior, dependency support window, or documented workflow. These need extra review and often PSC or release-manager attention. |

Prefer `enhancement` over `new feature` for internal refactors, performance
work, and CI/tooling improvements unless they expose a new public interface.
Prefer `test case only` for regression-only PRs that document a bug but do not
change the shipped code path.

## Process Labels

Process labels describe what must happen before the pull request can merge.
Remove them promptly when the condition is fixed; stale process labels are
worse than no label.

| Label | Add when | Remove when |
|-------|----------|-------------|
| `needs rebase` | The branch is dirty, conflicting, based on an obsolete target, or the merge diff includes unrelated already-landed commits. | The branch is rebuilt on the current target and the forge reports a clean merge state. |
| `needs ticket` | A substantive feature, bug fix, behavior change, or release-note-worthy change lacks a Trac ticket or other accepted public tracker. | The PR body links the Trac ticket or the maintainer explicitly records why no Trac ticket is needed. |
| `needs NEWS` | The change is user-visible, release-note-worthy, security-relevant, ticketed, or part of a stable backpatch and the branch lacks the required `NEWS` entry. | `NEWS` has the correct top-section entry on every target branch, using `#NNNN`, `GH-NNN`, or `GT-NNN` only in `NEWS`. |
| `needs tests` | The branch changes behavior or fixes a bug without focused regression, CUnit, upgrade, or documentation validation where such coverage is practical. | The branch includes suitable tests or the PR body states the exact reason coverage is impractical. |
| `changes requested` | A maintainer or review bot has requested changes that are still valid on the current head. | Current-head review threads are fixed, resolved, obsolete, or explicitly classified. |
| `to merge` | A maintainer has finished review and the branch is waiting for final merge mechanics. Use sparingly. | The branch is merged, closed, or a later push requires renewed review. |
| `stale` | The pull request has not moved for a long time and its target, branch, or implementation likely needs rediscovery. | The branch is refreshed, closed, superseded, or a maintainer confirms it is still current. |

Do not let aggregate review state alone drive labels. GitHub can continue to
show `CHANGES_REQUESTED` after old inline threads are resolved or outdated.
Check current review threads and the current head before keeping
`changes requested`.

## Milestones

Use the milestone for the lowest release line that is expected to contain the
change.

* `master` feature and enhancement work normally targets the next minor release
  milestone, for example `3.7.0` during the 3.7 development cycle.
* Stable-branch bug-fix pull requests target that branch's next patch release,
  for example a `stable-3.6` fix targets the next `3.6.x` milestone.
* Backpatch families should assign one milestone per branch PR. Do not put the
  `master` milestone on a `stable-*` PR, and do not put a stable milestone on a
  `master`-only feature PR.
* Documentation-only PRs get a milestone when they close a release-tracked
  Trac ticket, document release behavior, or belong to a release/backpatch
  family. Cosmetic typo fixes may have no milestone.
* Test-only PRs get a milestone when they protect a release-tracked bug or
  unblock release CI. Pure harness cleanup may have no milestone.

Before setting a milestone, inspect the Trac ticket milestone, the PR base
branch, and the top `NEWS` section for that branch. If a Trac ticket is already
milestoned to an older supported release, verify whether the fix really lands
there or whether the ticket should be retargeted.

## Assignees and Reviewers

Assignees are optional in the PostGIS PR queue. Do not use assignee state as a
hidden approval or ownership signal. Prefer explicit comments, review requests,
or PR-body checklists when a maintainer has a concrete next action.

Requested reviewers should be current and specific. Clear stale requested
reviewers when a branch has been replaced or the request no longer matches the
component. Do not infer approval from the absence of requested reviewers.

## Practical Triage Order

When opening or refreshing a pull request, apply metadata in this order:

1. Confirm the source-of-truth branch and mirror surface.
2. Set draft state or `WIP:` title if the branch is not ready for ordinary
   review.
3. Add the type label or labels.
4. Add process labels for missing blockers such as ticket, `NEWS`, tests, or
   rebase.
5. Set the milestone from the base branch, Trac ticket, and release line.
6. Re-read the pull request body, merge state, CI, and current review threads.
7. Remove obsolete process labels after the branch is fixed.

If a pull request is both a feature and a bug fix, decide whether the target
branch makes that acceptable. Stable branches normally should not receive new
features only because they are bundled with a bug fix.
