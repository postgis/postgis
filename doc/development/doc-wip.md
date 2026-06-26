---
title: "Documentation WIP"
date: 2026-06-26
weight: 900
draft: true
geekdocHidden: true
geekdocHiddenTocTree: false
---

# Documentation WIP

This draft PR is still a migration workspace. The notes below track review
comments that need more content work or a maintainer decision before the branch
should leave draft state.

## Structure Questions

* `PRRC_kwDOAEM_Wc7POSzg`, `PRRC_kwDOAEM_Wc7POUBR`,
  `PRRC_kwDOAEM_Wc7POVRO`: governance still needs a final shape. The PSC
  process and core-contributor rules are now written as maintained governance
  text instead of RFC-1/RFC-5 commentary, and the umbrella project/service
  inventory now has its own page. Before this PR leaves draft, decide whether
  these governance pages should replace the public RFC layout on the website.
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

* Decide whether the component-owned TIGER geocoder ideas retired from
  `UsersWikiTIGERGeocoderToDo` should become issues in
  <https://git.osgeo.org/gitea/postgis/postgis_tiger_geocoder>. The split
  repository already documents and tests `geocode_intersection()`, but the old
  Trac page also listed no-address/block geocoding, street-type fallback when a
  parsed type is wrong, and caller-selectable match-level requirements. Those
  should not stay as core PostGIS docs, but may still deserve component tickets.
* Decide whether to create fresh topology data-model diagrams for the manual or
  website. The old `PostgisTopology_Data_Model` page only pointed to ticket
  #2578 attachments: 2013 slides for node/edge/face, TopoGeometry layer
  stacking, relation-table mapping, and a schema overview, plus a PDF table
  walkthrough of `topology.topology`, `topology.layer`, TopoGeometry
  properties, `relation`, `face`, `node`, and `edge_data`. Current manual text
  covers those tables and fields, but polished diagrams may still be useful as
  new maintained assets.
* Decide whether the old WKT Raster future-spec ideas need fresh Trac tickets
  or should stay closed as historical proposals: named raster bands/addressing
  bands by name, SQL-created overview tables, raster coverage topology checks
  such as gaps/overlaps/same-tile-size predicates, richer raster interpolation
  from geometry layers, and internal sub-tiling for very large rasters. Current
  code covers many adjacent pieces, including WKB/HexWKB, `ST_Tile`,
  `ST_MakeEmptyCoverage`, map algebra, raster statistics aggregates,
  `ST_IsEmpty`, `ST_HasNoBand`, and `ST_BandIsNoData`, but those old proposals
  are not a maintained contract as written.
* Decide whether PostGIS should grow first-class raster distance-surface tools
  or leave those workflows to map algebra, geometry KNN, `ST_InterpolateRaster`,
  GDAL, and downstream processing. The retired 2012 raster SoC bundle proposed
  `ST_EuclideanDistance` for nearest-source distance rasters and
  `ST_CostDistance` for cost-weighted distance over a cost raster; current
  raster docs cover interpolation, neighborhood callbacks such as
  `ST_InvDistWeight4ma` and `ST_MinDist4ma`, and raster/vector conversion
  pieces, but not a maintained Euclidean-distance or cost-distance raster API.
* Decide whether old GSoC core ideas need fresh Trac tickets or should remain
  closed as historical project planning: raster density-surface generation,
  topology-aware `shp2pgsql`/`pgsql2shp` loading and dumping, a
  no-new-primitives mode for `toTopoGeom`, topology-aware
  `ST_EstimatedExtent`, `pgsql2shp` query export without temporary-table
  privileges, and GiST build pre-sorting for PostGIS operator classes. Current
  docs already cover adjacent pieces such as `ST_Snap` before `ST_Split`,
  `ST_VoronoiPolygons`, `ST_VoronoiLines`, topology import/export tools,
  `CopyTopology`, and normal loader/dumper usage, but these old project ideas
  are not maintained specifications.
* Decide whether the RFC-7 extension-upgrade path reduction still needs a
  current ticket or upstream PostgreSQL proposal. The retired draft discussed
  replacing the growing matrix of extension upgrade scripts with an `ANY` or
  wildcard-style upgrade path, or with tiny compatibility files, but also
  recorded cloud-environment blockers around `postgis_extensions_upgrade()`.
  Current release docs still require maintaining `extensions/upgradeable_versions.mk`
  and testing generated upgrade scripts; the RFC draft is not itself a
  maintained release procedure.

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
* Historical FOSS4G sprint themes that remain research topics rather than
  current workflow docs: a typed bounding-box or envelope API with clear SRID
  and indexing semantics, raster casts or operators that can use that API for
  indexing, weighted raster summary/statistics functions beyond the existing
  map-algebra weighted masks and ordinary raster statistics aggregates, and 3D
  visibility or line-of-sight APIs beyond the existing 2D visibility-polygon
  support.
* Historical user-wishlist themes that remain research topics rather than
  current user docs: schema relocation for core extensions, richer `ST_AsKML`
  output metadata such as extrude/tessellate/altitude mode, E00/topology
  import, GRASS-style raster/vector analysis bindings, database-resident
  network-model primitives versus routing in `pgRouting`, streaming execution
  for large noding, coverage, aggregate, and TIN workloads, point-cloud support
  direction, and better selectivity or decomposition strategies for large sparse
  polygons in containment queries.

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
* Follow up on live-looking items extracted from the June 16, 2026, development
  meeting before removing draft status: whether GEOS should expose a git hash for
  `postgis_full_version()`; whether AI skills files belong in this repository
  long term.
