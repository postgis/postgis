# Trac Wiki Corpus

This directory contains the PostGIS-specific pages pulled from the rendered
Trac `TitleIndex` during the migration for
<https://trac.osgeo.org/postgis/ticket/5638>.

The files keep their original Trac wiki markup with the `.tracwiki` extension.
They are an archival source corpus, not the preferred editing location for
current developer workflow. Maintained developer documentation lives in
[`doc/development/`](../development/).

`MANIFEST.tsv` records the category, Trac page name, source URL, repository
path, byte count, and line count for every imported page.

Imported categories:

* `development/` - developer wiki pages, code repository notes, CI notes, and
  historical design discussions.
* `user/` - user wiki recipes, examples, install notes, package notes, and
  support references.
* `raster/` - WKT Raster and PostGIS Raster specifications, tutorials, and
  historical design notes.
* `topology/` - topology primitive and example pages.
* `project-history/` - RFCs, release planning, meeting notes, code sprints,
  GSoC pages, and the old wiki landing page.
* `source-hosts/` - source hosting and mirror reference pages.

Trac engine documentation and generic wiki help pages, such as `TracInstall`,
`TracWorkflow`, `InterWiki`, and `WikiFormatting`, are not copied here because
they document Trac itself rather than PostGIS.
