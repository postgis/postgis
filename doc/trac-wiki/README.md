# Trac Wiki Corpus

This directory records the PostGIS-specific pages pulled from the rendered Trac
`TitleIndex` during the migration for
<https://trac.osgeo.org/postgis/ticket/5638>. The imported `.tracwiki` page
corpus has been fully audited and retired from the repository import.

Retired pages are tracked in
[`doc/development/wiki/trac-cleanup.md`](../development/wiki/trac-cleanup.md).
Maintained developer documentation lives in
[`doc/development/`](../development/).

`MANIFEST.tsv` records the category, Trac page name, source URL, repository
path, byte count, and line count for every imported page. It now has only the
header row because no `.tracwiki` pages remain imported.
`ATTACHMENTS.tsv` records recovered Trac attachments with source URL,
repository path, byte count, checksum, and Trac `Last-Modified` value. The
recovered files that remain useful now live beside the maintained developer
notes that use them; their rows stay here as provenance rather than as a Trac
attachment tree.
Pages that reviewers classified as Trac-only, completed, or superseded are
intentionally absent from this corpus and recorded in the cleanup ledger.

Trac engine documentation and generic wiki help pages, such as `TracInstall`,
`TracWorkflow`, `InterWiki`, and `WikiFormatting`, are not copied here because
they document Trac itself rather than PostGIS.
