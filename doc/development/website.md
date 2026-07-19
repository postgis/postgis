---
title: "PostGIS Website Maintenance"
date: 2026-06-26
weight: 110
geekdocHidden: false
---

The public website source lives in the OSGeo Gitea repository:

```sh
git clone https://gitea.osgeo.org/postgis/postgis.net.git
cd postgis.net
```

The site is a Hugo site using a customized `hugo-geekdoc` theme. Keep local
changes in the site content, layouts, shortcodes, static assets, or config; do
not edit the vendored theme for PostGIS-specific behavior.

Useful commands:

```sh
hugo server
hugo
make check
```

`hugo server` renders a local preview. Pages with `draft: true` will not appear
in normal builds, so set `draft: false` before expecting a page to show up.
Running `hugo` writes the generated site into `public/`.

`make check` currently runs `utils/check_releases_md5.sh`. That script reads the
published release list from `config.toml`, downloads each non-development source
tarball from `https://download.osgeo.org/postgis/source`, downloads the matching
`.md5` file from `https://postgis.net/stuff`, and verifies the checksum. It
uses `tomlq`, `curl`, and `md5sum`.

## Developer Docs Front Matter

Website content pages use YAML front matter. Developer documentation pages that
may be copied into `postgis.net` content should include at least `title`,
`date`, `weight`, and `geekdocHidden`. Do not repeat the front-matter title as a
Markdown H1: the Geekdoc page layout renders that title already.

The publisher maps this directory's `README.md` to the root `_index.md`. The
root sets `geekdocHidden: true` because the website links the guide from its
`content/documentation/developer_docs.md` hub; hiding the imported root keeps
the complete subtree out of the global file-tree menu. Nested section landing
pages should live at `<section>/_index.md` and set
`geekdocCollapseSection: true`. Do not use `layout: toplevel` for these pages;
that layout appends another child-page list after the maintained section
content. Draft-only migration aids should set `draft: true`,
`geekdocHidden: true`, and `geekdocHiddenTocTree: false` so normal Hugo builds
do not publish them accidentally.

## Content Areas

Website source content that overlaps with repository-maintained developer docs:

| Website source | Repository-maintained destination |
|----------------|-----------------------------------|
| `content/development/source_code.md` | [Contributing workflow](contributing.md) and [Pull request and maintainer workflow](maintenance/_index.md) |
| `content/development/getting_involved.md` | [Contributing workflow](contributing.md) |
| `content/development/bug_reporting.md` | [Contributing workflow](contributing.md) |
| `content/documentation/developer_docs.md` | [Documentation workflow](manual.md) and this `doc/development/` index |
| `content/development/versions_eol.md` | [Release process](release-process.md) and [Release and upgrade rules](release/_index.md) |
| `content/development/rfcs/*.md` | [Governance notes](governance/_index.md) and historical RFC pages |
| New governance pages outside `rfcs/` | [Governance notes](governance/_index.md) and [PostGIS project inventory](governance/project-inventory.md) |
| `content/community/mailinglists.md` and `content/community/chat.md` | [Contributing workflow](contributing.md) |
| `content/community/conduct.md` | [Governance notes](governance/_index.md) |
| `content/documentation/manual.md` | [Documentation workflow](manual.md) |
| `content/documentation/training.md` | Public website; only the workshop repository is an umbrella project |

The website declares its content license as CC BY-SA 4.0 in `config.toml`.

The public support-policy page, `content/development/versions_eol.md`, is the
website home for PostGIS, PostgreSQL, and dependency support status. If the
project publishes a detailed compatibility matrix again, generate it or express
it as version ranges from release and dependency metadata. Do not restore a
hand-maintained Trac-style table in the manual or repository developer docs;
keep repository guidance in [Dependency support](release/dependencies.md).

The maintained governance text in this repository should publish to ordinary
website governance pages instead of replacing the old numbered RFC files in
place. Keep `content/development/rfcs/*.md` as historical RFC archives, and use
the repository [Governance notes](governance/_index.md) plus
[PostGIS project inventory](governance/project-inventory.md) as the source for
the current PSC process, core-contributor rules, umbrella projects, and project
service inventory.

## Release Updates

When publishing a PostGIS release, update the website after the source and
documentation artefacts exist:

1. Update `config.toml` under `[params.postgis]` and
   `[params.postgis.releases]` so the current release, development release, EOL
   markers, source links, documentation links, and release-note links match the
   freshly published artefacts.
2. Add a release news item under `content/news/<year>/`. The historical naming
   convention starts release filenames with `mm-dd`; Hugo can derive permalinks
   from slug/title/date, but the convention is still used for readability.
3. Run `make check` to verify release tarball checksums.
4. Push the website change to `https://gitea.osgeo.org/postgis/postgis.net` and
   wait for the website deployment.

See [Release process](release-process.md) for the broader release checklist.
