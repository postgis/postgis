---
title: "Trac Wiki Migration Map"
date: 2026-06-26
weight: 910
draft: true
geekdocHidden: true
geekdocHiddenTocTree: false
---

# Trac Wiki Migration Map

This is a draft-only migration workspace for ticket #5638. Do not treat it as
the final developer-documentation structure or publish it as a maintained
chapter. All imported wiki pages have now been folded into maintained topic
pages, left as historical Trac context, or retired in the cleanup ledger. Once
this map has served review, remove it and keep only the maintained topic pages
plus any necessary archive pointer.

The PostGIS Trac wiki remains useful for tickets and live project discussion,
but maintained developer documentation should live in this repository. This
page records the wiki pages pulled while consolidating ticket #5638 and where
their maintained content belongs now.

The Trac `TitleIndex` was checked during the migration by reading the rendered
page list rather than the unexpanded Trac macro. All 306 linked wiki page
bodies were then pulled with `?format=txt` before deciding where they belong.
The index includes developer workflow pages, user cookbook pages, old install
notes, historical GSoC and meeting pages, topology and raster design notes,
Trac built-in documentation, and obsolete release planning pages. The Trac
roadmap and ticket reports were also checked as active planning surfaces,
together with the published development pages on
<https://postgis.net/development/>.

The `postgis.net` website source repository was also checked at
<https://gitea.osgeo.org/postgis/postgis.net>. Development, RFC, community,
documentation, release-link, and website-maintenance pages were compared with
the repository docs so the maintained content in `doc/development/` does not
depend on the website as the only source.

`doc/development/`. The PostGIS-specific Trac wiki bodies were checked during
the migration, and no raw `.tracwiki` pages remain imported under
[`doc/trac-wiki/`](../trac-wiki/). Trac engine help and administration pages
were not copied because they document Trac itself rather than PostGIS. Pages
inspected during migration but intentionally left on Trac or retired from the
repository import are tracked in the [Trac cleanup ledger](wiki/trac-cleanup.md).

TitleIndex groups intentionally not imported into the corpus:

* `Trac*`, `Inter*`, `Wiki*`, `CamelCase`, `BadContent`, `PageTemplates`,
  `RecentChanges`, `SandBox`, `TitleIndex`, and `TicketQuery` are Trac system,
  help, or administration pages.
* `pgRouting` is an external project pointer rather than PostGIS documentation.

Do not add new maintained developer-process pages to Trac. Add them under
`doc/development/` and link them from the development README or the relevant
topic page when they are useful for future contributors.
