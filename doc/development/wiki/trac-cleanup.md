# Trac Cleanup Ledger

This ledger records Trac wiki pages that were inspected during the developer
documentation migration and should not become maintained repository
documentation. It is intended as the later cleanup checklist for Trac itself:
each row says whether the page should stay as historical Trac context, be
retired from Trac, or wait for a narrower follow-up.

If Trac itself is retired later, do not turn historical discussion pages into
current developer instructions. Preserve the original record in a static Trac
or website archive with its original page name and links where practical, move
durable decisions into maintained repository or website docs, and move still
open work into Trac tickets, replacement issues, or `doc/development/doc-wip.md`
before retiring the source page.

## Leave on Trac

| Trac wiki page | Reason |
|----------------|--------|
| `DevelopmentDiscussion` | Historical PostGIS 2.0 discussion scratch page. Completed topics such as the raster merge and serialization migration are release history; still-open research themes were extracted into `doc/development/doc-wip.md`. Keep the original discussion record in the archive rather than republishing it as current developer guidance. |
| `DevFOSS4GCodeSprintNotes` | Historical FOSS4G sprint agenda and release-planning notes. It contains point-in-time discussion about the PostGIS 2.0 cycle, raster/topology/geocoder planning, and old feature-freeze dates; keep it as Trac history rather than maintained developer documentation. |
| `DevWikiEvent` | Historical Trac index for PostGIS meetings and code sprints. Individual meeting/sprint pages remain historical records, but this navigation page should stay with the Trac archive rather than become maintained developer workflow documentation. |
| `DevWikiISO19125` | Historical draft-comment page for ISO19125 questions. Current manual and code carry OGC/SFS and SQL/MM conformance references; still-open standards-conformance themes were extracted into `doc/development/doc-wip.md`. |
| `DevWikiProvenanceReview` | Historical OSGeo incubation and provenance-review record. The page lists a point-in-time audit of vendored files and license notes; it should not be republished as current contributor guidance unless a fresh license/provenance audit is requested. |
| `GoogleSeasonDocs2019` | Historical Google Season of Docs planning page. The page records 2019 mentors, links, and project framing; still-useful documentation-improvement ideas were extracted into `doc/development/doc-wip.md`. |

## Retire From Trac

| Trac wiki page | Reason |
|----------------|--------|
| `DevWikiMain` | Superseded developer-wiki landing page. Maintained navigation now lives in `doc/development/README.md`, with contributor workflow in `doc/development/contributing.md` and topic pages under `environment/`, `testing/`, `manual/`, `style/`, `internals/`, `release/`, `release-process/`, `governance/`, `maintenance/`, `website/`, and `wiki/`. |
| `DevWikiAffineParameters` | Folded into `doc/development/internals/raster-affine.md` after checking the current raster geotransform code. The source PNGs were recovered under `doc/development/internals/images/raster-affine/` and recorded in `doc/trac-wiki/ATTACHMENTS.tsv` for provenance. |
| `DevWikiEmptyGeometry` | Folded into `doc/development/internals/empty-geometry.md` after reconciling the old design questions with current code and regression tests. The Trac page should not remain as a parallel semantics proposal once the maintained internals note is published. |
| `DevWikiMingW64_Setup` | Folded into `doc/development/environment/windows/` as historical MinGW-w64/MSYS setup context. The current Windows build surfaces are the MSYS2 GitHub Actions workflow and the Winnie scripts, so the old SourceForge/GCC 4.5/PostgreSQL 9.x bootstrap recipe should not remain as maintained setup guidance. |
| `DevWikiRealParameters` | Folded into `doc/development/internals/raster-physical-parameters.md` after checking the current reverse geotransform calculation. The source PNGs were recovered under `doc/development/internals/images/raster-physical-parameters/` and recorded in `doc/trac-wiki/ATTACHMENTS.tsv` for provenance. |
| `DevWikiSpatialCollectionTutorial` | Folded into `doc/development/internals/spatial-collections.md` after checking that the old `SPATIAL_COLLECTION` interface is no longer present in current source. The maintained note keeps the useful design distinction between geometry collections and raster/vector value workflows while routing developers to current code paths. |
| `DevWikiWinMingW64` | Folded into `doc/development/environment/windows/` as historical PostGIS 2.0 MinGW-w64 packaging context. Its prepared `ming64.zip`/`ming32.zip`, PostgreSQL 9.x, GCC 4.x, GDAL 1.x, GEOS 3.3, PROJ 4.x, and hand-built dependency steps are obsolete as setup guidance; the current sources of truth are the MSYS2 workflow for pull-request builds and Winnie for Windows packaging. |
| `DevWikiWinMingW64_21` | Folded into `doc/development/environment/windows/` as the same historical prepared-bundle packaging family for PostGIS 2.1. Its `OS_BUILD`, `PROJECTS`, `MINGHOST`, `GCC_TYPE`, and versioned dependency-prefix concepts now map to the maintained Winnie scripts rather than standalone wiki instructions. |
| `DevWikiWinMingWSys_14_15` | Obsolete Windows XP-era MinGW/MSYS recipe for building PostGIS 1.4/1.5 with PostgreSQL 8.3/8.4. Later Windows/MinGW pages cover the surviving toolchain caveats, while current build and test guidance belongs in the maintained development-environment and testing docs. |
| `FAQ` | Superseded mixed user FAQ. Current manual pages cover SRID lookup, WKB input/output, `ST_` naming, and the remaining `ST_Intersects`/`ST_Intersection` precision caveat; the Windows Vista and PostGIS 1.x hard-upgrade notes are obsolete. |
| `3DDistancecalc` | Folded into `doc/development/internals/distance.md` after checking the old 2D-vs-3D candidate logic and plane-normal notes against current `liblwgeom/measures3d.c`, including `define_plane`. The old Trac prose should not remain as a parallel source page in the repository import. |
| `NewDistCalc` | Superseded distance-calculation index stub. Its useful 2D fast-path context is covered by `doc/development/internals/distance.md`; the raw Trac stub should not remain as a parallel source page in the repository import. |
| `NewDistCalcGeom2Geom` | Folded into `doc/development/internals/distance.md` after recovering the original diagrams and checking the old projection-sort algorithm against current `liblwgeom/measures.c`. The current source still has the fast non-overlapping point-array path, but maintained docs should summarize the present `lw_dist2d_fast_ptarray_ptarray` and rect-tree boundaries rather than republish the PostGIS 1.5 walkthrough as current workflow guidance. |
| `NewDistCalcSubGeom` | Folded into `doc/development/internals/distance.md` after recovering the original illustrations and checking the old subgeometry bounding-box pruning idea against current `liblwgeom/measures.c`, `liblwgeom/lwtree.c`, and `postgis/lwgeom_rectree.c`. The PostGIS 1.4-era flat-list timing walkthrough is obsolete; current docs should explain recursive 2D distance dispatch and the rect-tree successor instead. |
| `PostGIS3` | Completed PostGIS 3 planning scratch page. Current docs and code already cover the completed items such as `postgis_raster` split/upgrade handling, GeoJSON json/jsonb support, modern dependency baselines, and cluster window functions; still-open research themes were extracted into `doc/development/doc-wip.md`. |
| `PostGISObsoleteVersionsMatrix` | Stale hand-maintained compatibility table for EOL PostGIS, PostgreSQL, GEOS, PROJ, and GDAL combinations. The current matrix belongs on the website; durable support-window and matrix-maintenance rules were folded into `doc/development/release/dependencies/`. |
| `SomeSplitting` | Superseded splitting-design scratch page. Current source and manual already cover oversize geometry subdivision with `ST_Subdivide`, direct geometry splitting with `ST_Split`, linework rebuilds with `ST_Node` and `ST_Polygonize`, and topology edge/face-editing primitives; the only surviving open design question, vertex-preserving `SplitToMaxN`-style polygon partitioning without segment split points, was moved to `doc/development/doc-wip.md`. |
| `ToleranceDiscussion` | Folded into `doc/development/internals/precision-tolerance.md` after checking current GEOS-backed `gridSize` overlay APIs, `ST_Snap`, topology precision/tolerance functions, distance predicate tolerances, coverage/triangulation tolerance parameters, and MVT integer-grid quantization. The old proposal for an inferred or metadata-backed precision model on ordinary geometries remains only as a design question in `doc/development/doc-wip.md`. |
| `UsersWikiCheckInvalidGeometriesFromGeometryColumns` | Superseded user tip for scanning all registered geometry columns with `ST_IsValid`. A modernized example now lives in the `ST_IsValid` manual page, using `geometry_columns` and quoted dynamic SQL. |
| `UsersWikiExamplesInterpolateWithOffset` | Superseded custom wrapper for interpolating an offset point along a line. The current manual documents the same geocoding-style pattern with `ST_LineInterpolatePoint` and `ST_OffsetCurve`, while measured-line offset behavior is covered by `ST_LocateAlong` and `ST_LocateBetween`. |
| `UsersWikiExamplesSpikeRemover` | Unmaintained custom PL/pgSQL spike-removal recipe. It is not directly replaced by `ST_SimplifyVW`, which simplifies by effective area and may return invalid polygonal output; the maintained manual now records that distinction and points users to validity checks and repair functions. |
| `UsersWikiGenerateHexagonalGrid` | Superseded hand-written hexagon-grid SQL recipe. The user manual documents the built-in `ST_HexagonGrid` and `ST_SquareGrid` functions, and `ST_HexagonGrid` now points users who need H3 tiling to the `h3-pg` extension. |
| `UsersWikiPostGIS15Ubuntu1110pkg` | Obsolete Ubuntu 11.10 package recipe for PostGIS 1.5.3 and PostgreSQL 9.1. Current installation docs cover supported PostgreSQL versions, source builds, pre-built package pointers, and extension-based database enablement; the old `template_postgis`/`postgis.sql` package commands should not be republished as current guidance. |
