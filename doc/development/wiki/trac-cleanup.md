# Trac Cleanup Ledger

This ledger records Trac wiki pages that were inspected during the developer
documentation migration and should not become maintained repository
documentation. It is intended as the later cleanup checklist for Trac itself:
each row says whether the page should stay as historical Trac context, be
retired from Trac, or wait for a narrower follow-up.

## Leave on Trac

| Trac wiki page | Reason |
|----------------|--------|
| `DevFOSS4GCodeSprintNotes` | Historical FOSS4G sprint agenda and release-planning notes. It contains point-in-time discussion about the PostGIS 2.0 cycle, raster/topology/geocoder planning, and old feature-freeze dates; keep it as Trac history rather than maintained developer documentation. |
| `DevWikiProvenanceReview` | Historical OSGeo incubation and provenance-review record. The page lists a point-in-time audit of vendored files and license notes; it should not be republished as current contributor guidance unless a fresh license/provenance audit is requested. |
| `GoogleSeasonDocs2019` | Historical Google Season of Docs planning page. The page records 2019 mentors, links, and project framing; still-useful documentation-improvement ideas were extracted into `doc/development/doc-wip.md`. |

## Retire From Trac

| Trac wiki page | Reason |
|----------------|--------|
| `UsersWikiPostGIS15Ubuntu1110pkg` | Obsolete Ubuntu 11.10 package recipe for PostGIS 1.5.3 and PostgreSQL 9.1. Current installation docs cover supported PostgreSQL versions, source builds, pre-built package pointers, and extension-based database enablement; the old `template_postgis`/`postgis.sql` package commands should not be republished as current guidance. |
