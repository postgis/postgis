---
title: "Commit and Branch Guidelines"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

This page keeps the source-control practices from the historical RFC-5
contributor guideline in a maintained form. Use it together with
[Contributing workflow](../contributing.md), [Coding style](../style.md), and the
release rules for the target branch.

## Commit Messages

Write a meaningful one-line subject that describes the user-visible change,
ticket scope, or release-note wording. When more context is needed, add a body
after a blank line.

Keep commit-message prose easy to read in terminal tools. Historical guidance
uses a 70-column line width for commit-message body text.

Use parser-friendly tracker references:

* `Closes #NNNN` for a Trac ticket only when the branch fully resolves that
  ticket on the target branch.
* `References #NNNN` for partial work, investigations, prerequisites, related
  branches, or any change that does not complete the whole Trac ticket.
* Full GitHub or Gitea pull request and issue URLs outside `NEWS`, so forge
  references cannot be confused with Trac tickets.

If a pushed commit forgot the Trac reference, add the fixed branch and commit
hash to the Trac ticket instead of rewriting public history only for metadata.

Keep validation transcripts, review summaries, and handoff notes out of commit
messages. Put those in the pull request or maintainer handoff.

## Authorship and Provenance

Preserve contributor authorship when landing a community patch or pull request.
For a single-parent landing commit, use the contributor as the Git author when
the patch is substantially theirs, and record co-authorship only when local
changes are materially folded in.

Do not use a mirror forge's merge button for the final canonical landing when
that mirror is not the source of truth. Reconstruct the landing commit or merge
against the canonical OSGeo Gitea branch and keep the contributor credit.

Update `NEWS` for new features, breaking changes, release-note-worthy bug
fixes, and stable-branch bug fixes. Include the contributor name where the
release notes convention asks for it.

## Branch Targets

New features go to `master` and the next minor or major release. They should
not be committed to stable branches without explicit PSC or release-manager
permission.

Stable branches normally receive bug fixes only. Before preparing a backpatch,
check the branch's `Version.config` and top `NEWS` section to confirm that the
release line is open and that the change fits that line.

During pre-release code freeze, restrict commits to bug fixes unless the PSC or
release manager approves otherwise.

Significant changes, especially backward-incompatible behavior or inter-subsystem
API changes, should be discussed on `postgis-devel` before implementation. Larger
changes may need a PSC-approved RFC or replacement governance proposal.

Avoid routine new branches in the official repository. When a temporary branch
is needed only to run the full CI bot set before landing, prefix it with
`test/`.

## After Pushing

After each pushed branch update or landing commit, read back:

* the remote branch head;
* the pull request or ticket text that points at the change;
* review threads;
* CI and build-bot state.

If a build bot is unhappy because of the pushed commit, fix that before making
unrelated changes.
