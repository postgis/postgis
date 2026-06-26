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
`ATTACHMENTS.tsv` records recovered Trac attachments with source URL,
repository path, byte count, checksum, and Trac `Last-Modified` value. The
attachment files live under `attachments/` with paths mirroring the Trac
raw-attachment URL.
Pages that reviewers classified as Trac-only, completed, or superseded are
intentionally absent from this corpus and recorded in the migration map.

Current imported category:

* `project-history/` - RFCs, release planning, meeting notes, code sprints,
  extension-path notes, release-announcement history, and the old wiki landing
  page.

Trac engine documentation and generic wiki help pages, such as `TracInstall`,
`TracWorkflow`, `InterWiki`, and `WikiFormatting`, are not copied here because
they document Trac itself rather than PostGIS.
