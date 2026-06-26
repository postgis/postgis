# Documentation WIP

This draft PR is still a migration workspace. The notes below track review
comments that need more content work or a maintainer decision before the branch
should leave draft state.

## Remaining Corpus Sweep Plan

The review comments establish a repeated cleanup pattern for the rest of the
imported Trac corpus. For each remaining page family, do the full check rather
than marking it done by adding a TODO: compare the old page with current
source, current manual coverage, current website pages, and active component
repositories; then make one atomic commit that either folds useful content into
maintained docs, records a retirement reason in `wiki/trac-cleanup.md`, or
extracts a design question below.

Use these migration decisions as the source pattern:

* Historical meeting, sprint, release-planning, GSoC, and provenance pages stay
  as archive records unless they contain still-open technical work. Extract
  those open items here or into Trac tickets before retiring the source page.
* EOL install, package, upgrade, Windows, and platform recipes should usually
  be retired after checking whether current `doc/development/environment*.md`,
  release-support notes, CI, or packaging scripts need one small surviving
  caveat.
* User cookbook pages should be folded into the user manual only when the
  pattern is still useful and can be expressed with current functions, safe SQL,
  and current warnings. Otherwise record the obsolete or superseded reason in
  the cleanup ledger.
* Raster, topology, distance, precision, and other design/specification pages
  should become concise internals notes only when they describe current code or
  durable extension boundaries. Historical implementation proposals should be
  compressed to explicit open design questions rather than republished as
  current developer guidance.
* External or component-owned topics should become boundary pointers, not copied
  specs: GDAL driver behavior belongs upstream in GDAL docs, workshop tasks in
  `postgis-workshops`, H3 tiling in `h3-pg`, and user-facing website/training
  pages in the website repository.
* The draft migration map in `wiki.md` is temporary. When every imported page
  has one of the outcomes above, remove the map and leave maintained topic
  pages plus the cleanup ledger/archive pointer.

Batch the remaining sweep so each commit stays reviewable:

* Development pages: audit the remaining CI, affine/real parameter, empty
  geometry, coding, spatial collection, Windows/MinGW, NSIS, and VC pages
  against the maintained developer docs and current source. Fold only material
  that still describes current workflow or internals.
* Raster legacy pages: audit the remaining `WKTRaster*`,
  `PostGIS_Raster_SoC_Idea_2012/*`, and user raster pages against current
  raster manual coverage, `raster2pgsql`, server-side GDAL configuration, and
  current raster internals notes. Retire beta specs and old SoC implementation
  dumps unless they expose a still-open design question.
* Tool, integration, and service pages: audit OGR, QGIS, MapServer,
  SpatiaLite, ArcGIS, PostgreSQL/PostGIS, tools-support, tutorial, wishlist,
  and website-adjacent pages. Most should become external boundary pointers or
  cleanup-ledger entries rather than maintained developer docs.
* Remaining user cookbook pages: audit polygon construction, cardinal
  direction, coverages, geography/linear-reference function notes,
  `make check` considerations, and TIGER geocoder backlog material against the
  maintained manual and component repositories.
* Project-history pages: audit remaining release announcements, code sprint,
  development-cycle, RFC, and extension-path pages for unresolved durable
  decisions. Keep point-in-time records archived and move only live policy into
  release, governance, or maintenance docs.

## Structure Questions

* `PRRC_kwDOAEM_Wc7POEs0`, `PRRC_kwDOAEM_Wc7POIRS`: the development environment
  material is now split under `environment/ubuntu/` and `environment/docker/`.
  Check whether the first-level directory should eventually be role-oriented,
  as suggested in the PR discussion by strk.
* `PRRC_kwDOAEM_Wc7POmvr`: root `CONTRIBUTING.md` is now only a forge entry
  point, contributor workflow has been flattened into
  `doc/development/contributing.md`, and maintainer-only procedures remain
  under `maintenance/`. Before publication, check whether the maintainer page
  needs more role-specific anchors back to the contributor page.
* `PRRC_kwDOAEM_Wc7POSzg`, `PRRC_kwDOAEM_Wc7POUBR`,
  `PRRC_kwDOAEM_Wc7POVRO`: governance still needs a final shape. The PSC
  process and core-contributor rules are now written as maintained governance
  text instead of RFC-1/RFC-5 commentary, and the umbrella project/service
  inventory now has its own page. Before this PR leaves draft, decide whether
  these governance pages should replace the public RFC layout on the website.
* `PRRC_kwDOAEM_Wc7POljg`: the wiki migration map is intentionally temporary.
  It is now separated from the durable module list as a draft-only migration
  workspace. When every useful page has a maintained home or a recorded
  retirement reason, remove the map and keep only the topic pages plus any
  necessary archive pointer.
* `PRRC_kwDOAEM_Wc7PP04s`: raster/GDAL driver boundary notes now live in
  `internals/raster-gdal-driver.md`. Before publication, decide whether the
  growing internals material should be split into clearer geometry, topology,
  and raster sections or left as one cross-cutting internals chapter.

## Developer Deep Dives
* `PRRC_kwDOAEM_Wc7PPrPX`: `SomeSplitting` was checked against the current
  split and subdivision APIs. `ST_Subdivide` covers the practical oversize
  geometry subdivision case, including a grid-size argument for deterministic
  grid-aligned output when inputs already lie on the grid; `ST_Split`,
  `ST_Node`, `ST_Polygonize`, and topology edge/face-editing functions provide
  the lower-level building blocks discussed on the old page. The remaining
  not-yet-implemented idea is a vertex-preserving polygon partitioning API,
  `SplitToMaxN`-style, that returns area-covering polygon pieces below a
  max-vertex budget without adding segment split points. That needs a design
  ticket before becoming maintained developer documentation.
* `PRRC_kwDOAEM_Wc7PPr_S`: `ToleranceDiscussion` was checked against current
  GEOS-backed overlay `gridSize`, topology precision/tolerance APIs, snap
  helpers, predicate distance arguments, coverage/triangulation tolerances, and
  MVT tile quantization. The maintained behavior map now lives in
  `internals/precision-tolerance.md`. The remaining open design question is
  whether PostGIS should ever carry an inferred or metadata-backed precision
  model for ordinary geometries; that needs a design ticket before becoming
  maintained documentation.
* `TopologyCopy`: current `CopyTopology` copies primitives, topology layers,
  TopoGeometry sequences, and TopoGeometry definitions into a new topology
  schema, using synthetic `topology.layer` schema/table/column values for
  copied TopoGeometries that are not bound to user tables. The
  `pgtopo_export`/`pgtopo_import` tools also cover dump/restore, layer skipping,
  and layer-only relinking workflows. The old unimplemented idea that remains
  is a first-class detached-layer API, with explicit attach/detach/drop
  functions or normalized metadata for conceptual TopoGeometry layers. That
  needs a design ticket before becoming maintained developer documentation.

## Documentation Backlog

* Decide whether to create fresh topology data-model diagrams for the manual or
  website. The old `PostgisTopology_Data_Model` page only pointed to ticket
  #2578 attachments: 2013 slides for node/edge/face, TopoGeometry layer
  stacking, relation-table mapping, and a schema overview, plus a PDF table
  walkthrough of `topology.topology`, `topology.layer`, TopoGeometry
  properties, `relation`, `face`, `node`, and `edge_data`. Current manual text
  covers those tables and fields, but polished diagrams may still be useful as
  new maintained assets.

The following ideas were extracted from `GoogleSeasonDocs2019` before leaving
that historical planning page on Trac:

* Review manual prose for consistent style, spelling, and function parameter
  names.
* Decide whether the core manual or workshop material needs workbook-style
  paths for geometry/geography, topology, and raster workflows. Route workshop
  changes to <https://github.com/postgis/postgis-workshops>.
* Improve SQL example markup by separating SQL program listings from query
  output where practical, so formatting and validation can treat them
  differently.
* Decide whether DocBook XML source formatting should have a stronger
  line-width convention for cleaner diffs.

## Major-Version Research Backlog

The following still-open research themes came from `PostGIS3`. They need Trac
tickets, architecture discussion, or focused design documents before they can
become maintained developer documentation:

* Geometry storage and serialization experiments: alternative coordinate
  compression, sliceable headers, cached equality hashes, validity flags,
  precision metadata, non-double coordinate storage, more than four dimensions,
  and smaller typmod-only point storage.
* Geography and indexing experiments: 3D-aware geography/cartesian conversion,
  index tuples that can carry SRID or compact geometry summaries, S2/geography
  indexing, and faster or better-clustered GiST builds where PostgreSQL core
  support would be required.
* Backend-library experiments: deeper GEOS memory/coordinate-sequence
  integration, moving suitable algorithms to GEOS/SFCGAL/CGAL, and deciding
  whether any internal code should use C++ or external geometry-library
  adapters.
* Topology and robustness experiments: OSM-to-topology import, stronger
  validity-aware guarantees for predicates and overlays, fallback/retry across
  geometry backends, and empirical performance/cost frameworks for planner and
  regression testing.
* Historical PostGIS 2.0 discussion themes that remain research topics rather
  than current workflow docs: optional serialized metadata such as hash or
  cached user-data areas, more complete and mutable `liblwgeom` collection and
  point-array APIs, curve-completeness strategy, multidimensional indexing
  syntax and implementation, and deeper 3D primitive support such as
  polyhedral-surface validity, distance, centroid, volume, and interchange I/O.

## Standards Conformance Backlog

The following still-open themes were extracted from `DevWikiISO19125` before
retiring that draft-comment page:

* Keep empty-geometry behavior documented and tested for constructors,
  predicates, validity checks, and overlay functions.
* Clarify when spatial predicates and overlays intentionally use 2D semantics
  for higher-dimensional inputs, and where true 3D behavior needs separate
  functions or documentation.
* Keep validity and simplicity documentation explicit about geometry-type
  criteria, repeated points, invalid input handling, and parser acceptance.
* Track precision-model questions in current design work rather than old draft
  standards notes.
* When changing WKT/WKB parsing or output, check OGC/SFS, ISO, SQL/MM, EWKT,
  EWKB, SRID, empty-geometry, and SQL/MM type-code compatibility together.

## Project History and Planning

* `PRRC_kwDOAEM_Wc7PO3dX`: decide how historical meeting/event notes should be
  preserved if Trac is eventually retired. The archive policy in
  `doc/development/wiki/trac-cleanup.md` now covers historical discussion pages:
  active work moves to tickets or maintained docs, unresolved technical ideas
  are extracted before retirement, and the original discussion record stays in a
  Trac or website archive.
* `PRRC_kwDOAEM_Wc7PPsd_`, `PRRC_kwDOAEM_Wc7PPtGb`: keep the FOSS4G sprint note
  on Trac or in history, but extract any not-yet-implemented ideas into a
  maintained list.
