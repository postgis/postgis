---
title: "NEWS Entries"
date: 2026-08-13
weight: 25
geekdocHidden: false
---

`NEWS` is a short release changelog, not a commit log.

## Add

Add an entry for a user-visible or release-worthy:

* breaking change, feature, or enhancement;
* bug, security, or memory-safety fix;
* stable-branch fix or ticketed behavior change.

Skip refactors, tests, CI, and packaging unless release users or packagers need
to know.

Describe the behavior, not the branch, review, or test run:

```text
 - #NNNN, [subsystem] Brief behavior summary
          (Contributor Name)
```

Use `#NNNN` for Trac, `GH-NNN` for GitHub, and `GT-NNN` for Gitea. Do not use
bare `#NNNN` for GitHub or Gitea. Keep subsystem tags consistent with nearby
entries.

Credit authors in the final parentheses. For a reported security fix, use
`reported by ...; fixed by ...` when the roles differ. If the reporter supplied
the patch, credit the reporter only. Add missing people to Individual
Contributors and organizations to Corporate Sponsors.

## Place

During development, use the top unreleased section and its existing category.
If the section says `These are only changes since ...`, add only later changes.
During release prep, the former top section remains valid if it was unreleased at
the target base ref. Do not rewrite the first line of a released bullet without
the release manager's request.

## Release Prep

Before tagging:

1. Set the release date.
2. Compare `NEWS` with Git history and the Trac milestone.
3. Check version and dependency text.
4. Fix categories; merge or remove small internal notes.
5. Update `doc/release_notes.xml` with curated release notes.

Dates containing `x` are placeholders. Numeric dates must not be future dates and
must be in descending order. After release, add the next development section at
the top.

## Checks

Configured tree:

```sh
make check-news
make check-contributor-credits
```

Unconfigured tree:

```sh
python3 -B utils/docs/tests/test_check_news.py
sh utils/docs/check_news.sh .
python3 -B utils/docs/tests/test_check_contributor_credits.py
python3 -B utils/docs/check_contributor_credits.py --repo .
```

For a PR base-ref check:

```sh
utils/docs/check_news.sh --base-ref=origin/master .
```

The checker allows new bullets only in the current top unreleased section, or in
the former top section when it was unreleased at the base ref.

## Backpatch

For each target branch, check `Version.config` and the top `NEWS` section. Use
similar wording across branches, changing release-specific facts. Stable
branches usually need bug-fix wording, not new-feature wording.
