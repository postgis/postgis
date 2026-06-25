# Documentation WIP

This draft PR is still a migration workspace. The notes below track review
comments that need more content work or a maintainer decision before the branch
should leave draft state.

## Structure Questions

* `PRRC_kwDOAEM_Wc7POEs0`, `PRRC_kwDOAEM_Wc7POIRS`: the development environment
  material is now split under `environment/ubuntu/` and `environment/docker/`.
  Check whether the first-level directory should eventually be role-oriented,
  as suggested in the PR discussion by strk.
* `PRRC_kwDOAEM_Wc7POmvr`: `workflow/`, root `CONTRIBUTING.md`, and maintainer
  workflow still overlap. Decide whether contributor workflow should be one
  page with role-specific anchors.
* `PRRC_kwDOAEM_Wc7PQ_kd`, `PRRC_kwDOAEM_Wc7PRB5P`: the current directory tree
  uses many `README.md` files. Decide whether to flatten this into files such as
  `building.md`, `testing.md`, and `release/versioning.md` before the PR leaves
  draft.
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

## Developer Deep Dives

* `PRRC_kwDOAEM_Wc7POpi3`, `PRRC_kwDOAEM_Wc7POqCA`: `3DDistancecalc` needs a
  current-code audit and image recovery before it can become a maintained
  2D-vs-3D distance chapter.
* `PRRC_kwDOAEM_Wc7POyT-`, `PRRC_kwDOAEM_Wc7PPc2J`: affine and raster real
  parameter notes look useful, but need image recovery and Markdown conversion.
* `PRRC_kwDOAEM_Wc7PO25O`, `PRRC_kwDOAEM_Wc7PPfAw`: empty-geometry and spatial
  collection material should probably become advanced geometry deep dives after
  checking current behavior and examples.
* `PRRC_kwDOAEM_Wc7PPpX5`, `PRRC_kwDOAEM_Wc7PPqUS`: distance-calculation pages
  need image recovery and reconciliation with current distance code.
* `PRRC_kwDOAEM_Wc7PPrPX`: `SomeSplitting` should be compressed to only ideas
  not already implemented.
* `PRRC_kwDOAEM_Wc7PPr_S`: tolerance notes need comparison with current GEOS,
  MVT, and PostGIS tolerance behavior before conversion.

## Testing, Tools, and CI

* `PRRC_kwDOAEM_Wc7POtFt`, `PRRC_kwDOAEM_Wc7POtRS`: CI notes need a standards
  decision. If a maintained CI inventory stays in repo docs, update the
  associated scripts to check or regenerate it.

## Windows and Packaging

* `PRRC_kwDOAEM_Wc7PPIhX`, `PRRC_kwDOAEM_Wc7PPgkg`: Windows/MinGW setup needs a
  current development-environment section and deduplication against newer
  Windows notes.

## User Manual Candidates

* `PRRC_kwDOAEM_Wc7PP4bz`: batch shapefile loading on Windows needs comparison
  with the current loader manual.
* `PRRC_kwDOAEM_Wc7PP6Zj`: buffer-unbuffer coastline simplification may be worth
  a user-manual example after checking the referenced source.

## Documentation Backlog

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

## Raster Deep Dives

* `PRRC_kwDOAEM_Wc7PPzvo`: check whether `WKTRaster_Documentation01` was merged
  into the raster manual before deciding whether to keep any source material.
* `PRRC_kwDOAEM_Wc7PP04s`: the GDAL driver specification looks like useful
  raster deep-dive material, but it may belong in a future geometry/topology/
  raster deep-dive split rather than this broad migration commit.
