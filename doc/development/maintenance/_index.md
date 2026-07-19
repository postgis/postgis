---
title: "Pull Request and Maintainer Workflow"
date: 2026-06-26
weight: 130
geekdocHidden: false
geekdocCollapseSection: true
---

This page collects maintainer-facing workflow notes for landing patches and
pull requests. Contributor-facing setup and submission notes live in
[Contributing workflow](../contributing.md).

## Source of Truth

The canonical PostGIS source repository is
<https://gitea.osgeo.org/postgis/postgis>. GitHub, GitLab, Codeberg, and
gitea.com repositories are mirrors for contributor convenience. Mirror pull
requests are valid contribution surfaces, but the merge target and final branch
history should be reconstructed from the canonical Gitea branch.

The public website states the same rule: changes pushed to OSGeo are
replicated to mirror repositories, and changes committed directly to mirrors
are overwritten on the next sync from OSGeo.

Treat forge-generated merge refs, such as GitHub pull request merge refs, as
test artifacts. They are useful for inspecting CI failures, but should not be
used as the base for repair branches or as upstream history.

Some umbrella projects and components have their own upstream repositories.
Route new work there unless the target is an explicit in-tree backpatch:

| Project or component | Upstream repository |
|----------------------|---------------------|
| Docker PostGIS | <https://github.com/postgis/docker-postgis> |
| PostGIS Java | <https://github.com/postgis/postgis-java> |
| PostGIS Workshop | <https://github.com/postgis/postgis-workshops> |
| H3-pg | <https://github.com/postgis/h3-pg> |
| `address_standardizer` | <https://github.com/postgis/address_standardizer> |
| `postgis_tiger_geocoder` | <https://git.osgeo.org/gitea/postgis/postgis_tiger_geocoder> |

When old Trac wiki pages contain backlog, examples, or design notes for a
split component, do not carry that material into core PostGIS developer docs
only because it was historically on the PostGIS wiki. Check the current
component repository first. Completed or superseded ideas can stay as
historical Trac context; still-relevant component work belongs as issues,
design notes, or documentation in that component repository.

## Core Contributor Guidelines

RFC-5, published at <https://postgis.net/development/rfcs/rfc05/>, records the
core contributor guideline for commit practice, Trac references, `NEWS`, code
provenance, and legal review. Use [Governance notes](../governance/_index.md) and
[PostGIS project inventory](../governance/project-inventory.md) for current PSC
process, repository routing, and service ownership.

Write access to the canonical repository is granted by the Project Steering
Committee. Core contributors are expected to understand the contribution
process, stay subscribed to the development mailing list, and support the code
they commit or delegate that support.

Core contributors are also responsible for provenance checks. Do not commit
code unless the contributor has the right to submit it under the project
license. Preserve existing copyright and license headers, mark code derived
from other projects, and discuss unusual licensing situations with the PSC or
OSGeo legal counsel before committing.

For the maintained commit-message, branch-target, authorship, and post-push
rules distilled from RFC-5, see [Commit and branch guidelines](commit-guidelines.md).
For supported-branch fix propagation, provenance, `NEWS`, and Trac follow-up
rules, see [Backpatching fixes](backpatching.md).
For labels, milestones, draft state, and mirror metadata on pull requests, see
[Pull request metadata](pull-request-metadata.md).

## First Pass

Before editing a public branch, public pull request body, or Trac ticket state,
read the current source of truth:

* `CONTRIBUTING.md`, [Coding style](../style.md), and the relevant page under
  this directory.
* The Trac ticket, including all comments, attachments, and linked context.
* The pull request description, commits, diff, review threads, and CI state.
* The target branch's `Version.config` and top `NEWS` section when the change
  may be backpatched or release-note-worthy.

Later ticket comments often refine the real scope. Do not classify a ticket
from the opening summary alone when later discussion names a concrete missing
API, behavior change, or testable failure.

## Contributor Handoff Points

When a mirror pull request, mailing-list patch, or Trac ticket needs contributor
follow-up instead of maintainer-side branch repair, point the contributor to the
specific workflow section rather than sending them back to this maintainer page:

* [Contributing workflow](../contributing.md) for Trac tickets, mirror pull
  requests, mailing lists, chat, and security reports.
* [First contribution path](../contributing.md#first-contribution-path) for a
  small-patch checklist from fork through focused validation.
* [Development environments](../environment/_index.md) and
  [Testing and debugging](../testing/_index.md) for local build and regression setup.
* [Release and upgrade rules](../release/_index.md) when a proposed change affects SQL
  API compatibility, extension upgrades, dependency policy, or backpatch scope.

## Review and CI Readback

Check inline review threads, not only aggregate review status. Each active
thread should be fixed, resolved, or explicitly classified before calling the
branch ready. Bot reviews are part of this readback when they are present.

After pushing a branch update, re-read the remote pull request head SHA, body,
merge state, checks, and active review threads. A green check belongs to the
head commit that produced it, not to a later force-push.

## Commit Messages

The maintained commit-message and branch-target rules live in
[Commit and branch guidelines](commit-guidelines.md). The short summary below is
for common pull request landings.

For a single-commit pull request that can be represented cleanly on the target
branch, land one normal single-parent commit with the original author. Use a
descriptive subject taken from the user-visible change, ticket scope, or `NEWS`
wording.

Use parser-friendly tracker trailers in the body:

```text
Closes #5638
Closes https://github.com/postgis/postgis/pull/925
```

Use `Closes #NNNN` for Trac tickets only when the branch fully resolves the
ticket on the target branch. Use `References #NNNN` for partial work,
investigation, related branches, or one part of a larger fix. Use full URLs for
GitHub or Gitea pull requests and issues.

Do not add a redundant merge commit only to say `Merge PR #NNN` when the pull
request URL is already recorded in a `Closes` trailer. Use merge commits only
when preserving meaningful multi-commit topology, recording a real branch merge,
or following an explicit maintainer request.

Keep validation logs, review summaries, and handoff notes out of commit
messages. Report them in the pull request or maintainer handoff instead.

## Trac and Public Text

In pull request bodies and forge comments, write Trac ticket references as
clickable links such as <https://trac.osgeo.org/postgis/ticket/5638>. Bare
`#NNNN` is a useful shorthand in commits and `NEWS`, but public review text is
easier to inspect when the ticket URL is explicit.

Draft public comments, pull request descriptions, and ticket updates before
posting. Re-read the posted text after editing so formatting, links, and tracker
references match what reviewers will see.

## NEWS and Branches

Update `NEWS` for user-visible changes, release-note-worthy bug fixes, security
fixes, and ticketed behavior changes. On `master`, place the note under the top
unreleased section and the appropriate category. On stable branches, match that
branch's existing heading names and release section.

In `NEWS`, use `#NNNN` for Trac tickets, `GH-NNN` for GitHub pull requests, and
`GT-NNN` for Gitea merge requests. Outside `NEWS`, prefer full forge URLs for
GitHub and Gitea references so they cannot be confused with Trac ticket numbers.

Before backpatching, confirm the target release line is open in `Version.config`
and `NEWS`. Stable branches normally receive bug fixes, not new features. End of
life branches should not receive new work without explicit release-manager
direction.
Use [Backpatching fixes](backpatching.md) for the two-pass branch
scan, one-to-one commit provenance, `NEWS` expectations, and per-ticket Trac
follow-up wording.

During pre-release code freeze, restrict commits to bug fixes unless the PSC or
release manager approves otherwise. Significant changes, especially backward
incompatibilities, should be discussed on the development mailing list before
implementation; larger changes may require an RFC.

## External Services

Use service CLIs as operational helpers, not as documentation sources of
secrets. Never commit or paste personal tokens into repository files, tickets,
or pull requests.

For OSGeo Gitea, Debian's Gitea CLI package provides the `tea-cli` command.
Prefer machine-readable output when inspecting branches, pull requests, and
repository metadata.

For Weblate repository maintenance, use `wlc` with the PostGIS Manual component
`postgis/postgis-manual` and API root <https://weblate.osgeo.org/api/>. Lock the
component before changing Weblate repository state, commit pending translation
work before merge or reset operations, and unlock when the repository is clean.

For website maintenance, use the OSGeo Gitea `postgis/postgis.net` repository.
The site is a Hugo project; update release pointers and news there during
release work, run `make check`, and do not edit the vendored theme. See
[PostGIS website maintenance](../website.md).
