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
| `DevWikiEvent` | Historical Trac index for PostGIS meetings and code sprints. Individual meeting/sprint pages remain historical records, but this navigation page should stay with the Trac archive rather than become maintained developer workflow documentation. |
| `DevWikiISO19125` | Historical draft-comment page for ISO19125 questions. Current manual and code carry OGC/SFS and SQL/MM conformance references; still-open standards-conformance themes were extracted into `doc/development/doc-wip.md`. |
| `DevWikiProvenanceReview` | Historical OSGeo incubation and provenance-review record. The page lists a point-in-time audit of vendored files and license notes; it should not be republished as current contributor guidance unless a fresh license/provenance audit is requested. |
| `GoogleSeasonDocs2019` | Historical Google Season of Docs planning page. The page records 2019 mentors, links, and project framing; still-useful documentation-improvement ideas were extracted into `doc/development/doc-wip.md`. |

## Retire From Trac

| Trac wiki page | Reason |
|----------------|--------|
| `DevWikiMain` | Superseded developer-wiki landing page. Maintained navigation now lives in `doc/development/README.md`, with topic pages under `workflow/`, `environment/`, `testing/`, `manual/`, `style/`, `internals/`, `release/`, `release-process/`, `governance/`, `maintenance/`, `website/`, and `wiki/`. |
| `DevWikiWinMingWSys_14_15` | Obsolete Windows XP-era MinGW/MSYS recipe for building PostGIS 1.4/1.5 with PostgreSQL 8.3/8.4. Later Windows/MinGW pages cover the surviving toolchain caveats, while current build and test guidance belongs in the maintained development-environment and testing docs. |
| `PostGIS3` | Completed PostGIS 3 planning scratch page. Current docs and code already cover the completed items such as `postgis_raster` split/upgrade handling, GeoJSON json/jsonb support, modern dependency baselines, and cluster window functions; still-open research themes were extracted into `doc/development/doc-wip.md`. |
| `PostGISObsoleteVersionsMatrix` | Stale hand-maintained compatibility table for EOL PostGIS, PostgreSQL, GEOS, PROJ, and GDAL combinations. The current matrix belongs on the website; durable support-window and matrix-maintenance rules were folded into `doc/development/release/dependencies/`. |
| `UsersWikiPostGIS15Ubuntu1110pkg` | Obsolete Ubuntu 11.10 package recipe for PostGIS 1.5.3 and PostgreSQL 9.1. Current installation docs cover supported PostgreSQL versions, source builds, pre-built package pointers, and extension-based database enablement; the old `template_postgis`/`postgis.sql` package commands should not be republished as current guidance. |
