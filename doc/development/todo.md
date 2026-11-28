---
title: "Development TODO"
date: 2026-06-28
weight: 35
geekdocHidden: false
---

This page records high-level development ideas that are not accepted designs,
release blockers, or scheduled work. Move an item to a ticket, design document,
or pull request when someone takes ownership of it.

## Geometry And Precision

* Investigate polygon partitioning below a maximum vertex budget: produce
  area-covering polygon pieces without adding unnecessary segment split points.
  A future design should consider tessellation plus a GiST picksplit-inspired
  partitioning strategy.
* Investigate PostGIS precision-model policy for ordinary geometries. Decide
  whether inferred or metadata-backed precision should ever exist outside
  topology-specific precision and operation-local tools such as overlay
  `gridSize`, snapping, and tile quantization, or document why precision should
  remain explicit.
* Investigate long-term geometry storage and serialization changes from the old
  major-version planning pages. Candidate themes include coordinate compression,
  externally stored or sliceable headers, cached equality hashes, optional
  serialized metadata, non-double coordinate storage, more than four coordinate
  dimensions, validity flags, typmod-only compact point storage, and complete
  mutable `liblwgeom` point-array and collection APIs. Current `GSERIALIZED`
  storage, typmods, TWKB output, and PostGIS 3 flag changes cover only parts of
  this design space.
* Investigate sparse-polygon containment performance. Current PostGIS has
  selectivity estimators and user-visible tools such as `ST_Subdivide`, but
  better selectivity or decomposition strategies could help with large sparse
  polygons where the bounding box is a poor proxy for occupied area.
* Investigate bounded-memory spatial pipelines for very large inputs, such as
  an index-ordered feature stream that can emit completed output while noding
  lines, polygonizing coverages, triangulating point sets, or performing
  unions. Current set-returning functions and aggregates own the operation
  semantics, but there is no general streaming API with that contract.

## Indexing And Planner

* Investigate a typed bounding-box or envelope API that can carry SRID and
  dimensionality without the size and semantics of a polygon. The historical
  sprint discussion also proposed using casts to this shared box type so
  extensions such as raster could reuse core spatial indexing rather than
  defining parallel operator infrastructure. Current `box2d`, `box3d`,
  geometry-cached boxes, and raster convex-hull indexing cover adjacent cases
  but not this common typed-box contract.
* Investigate future geometry index tuple designs. Historical proposals include
  carrying SRID in the index tuple, storing very small geometries directly in
  index entries for possible index-only scans, using compact fixed-size
  summaries for complex geometries, and adding nested boxes for
  `GEOMETRYCOLLECTION` trees. Current GiST/SP-GiST support and PostgreSQL 15+
  sort support cover adjacent work, but not these storage/index-tuple designs.
* Investigate geography indexing based on S2-style cells or another cell-based
  strategy, including whether a fast geography `ST_Intersects` GIN index is
  possible and useful. Current geography indexes remain GiST-oriented.
* Investigate PostgreSQL-core-dependent spatial index build and clustering
  improvements, such as building GiST pages from spatially adjacent tuple runs
  or preserving heap clustering by inserting near index sibling tuples. Current
  PostGIS sort support addresses part of index build ordering, but these ideas
  remain broader PostgreSQL storage/index research.

## Backend Libraries And Robustness

* Investigate deeper backend-library integration. Historical ideas include GEOS
  memory management through PostgreSQL allocation, GEOS coordinate sequences on
  PostGIS point arrays, moving suitable algorithms such as build-area or
  validity repair into upstream GEOS/SFCGAL/CGAL, and adding LWGEOM adapters to
  external geometry-library types. Current SFCGAL/GEOS integration covers many
  operations, but not these ownership and architecture changes.
* Investigate stronger robustness guarantees for predicates and overlays. The
  old planning notes proposed validity-aware guarantees, cached validity state,
  fallback or retry through another backend library after robustness failures,
  and empirical performance/cost frameworks for regression and planner-cost
  testing. Current GEOS/SFCGAL behavior, validity functions, and regression
  suites do not make this a settled design.
* Investigate curve-completeness policy: which curve operations should become
  complete in core, which should linearize explicitly, and which should remain
  unsupported until GEOS/SFCGAL or another backend owns the operation.

## Raster

* Investigate raster density-surface generation from point or line coverages.
  Count features into raster pixels and optionally smooth the result with
  neighborhood map algebra. Current PostGIS has `ST_InterpolateRaster` for
  interpolation from input points, but this density-surface workflow is not a
  maintained first-class raster API.
* Investigate internal sub-tiling for very large raster values. A single raster
  datum can approach PostgreSQL's 1 GB value limit, while current PostGIS tiles
  only at the table-row level and relies on ordinary TOAST storage inside one
  value. A future design would need fast window access without changing the
  public raster semantics or duplicating table-level tiling.
* Investigate first-class validation for tiled raster coverages, including
  overlap, gap, common tile-size, alignment, and regular-tiling checks. Current
  raster constraints and `ST_SameAlignment` cover portions of this model, but
  there is no complete maintained function family for validating a selected
  coverage as a whole.
* Investigate named raster bands. A design must cover on-disk and WKB storage,
  loader assignment, duplicate and missing names, upgrades, and overloads that
  address a band by name without weakening existing integer-band APIs.
* Investigate first-class raster distance surfaces, historically proposed as
  `ST_EuclideanDistance` for nearest-source distance rasters and
  `ST_CostDistance` for cost-weighted distance over a cost raster. Compare that
  direction with leaving the workflow to map algebra, geometry KNN, GDAL, and
  downstream processing.
* Investigate weighted raster summary/statistics functions beyond the existing
  weighted map-algebra masks and ordinary `ST_SummaryStats` /
  `ST_SummaryStatsAgg` aggregates.

## 3D And Output Formats

* Investigate 3D-aware geography/cartesian conversion so the internal
  geodetic-to-cartesian and cartesian-to-geodetic paths have a defined policy
  for a third coordinate instead of treating geography as a purely surface
  model. This needs explicit altitude semantics and compatibility rules before
  changing current geography calculations.
* Investigate deeper 3D primitive completeness. Current PostGIS supports
  PolyhedralSurface, Triangle, TIN, X3D/GML output, and many SFCGAL-backed 3D
  operations including 3D intersects, distance, area, volume, intersection,
  union, and difference. Historical notes still leave broader questions around
  3D validity semantics, point-in-polyhedra or 3D relationship models,
  centroid behavior, line-of-sight or visibility APIs, and interchange behavior
  that should be either designed or explicitly declared out of scope.
* Investigate richer `ST_AsKML` output metadata, including KML
  `extrude`, `tessellate`, and altitude-mode support. Current `ST_AsKML`
  signatures accept geometry or geography, precision, and namespace prefix, and
  current KML parsing code explicitly does not handle `kml:extrude`.

## Extension Packaging

* Revisit core-extension schema relocation policy. Historical notes proposed
  installing PostGIS into a dedicated `postgis` schema, then later marked it as
  lower priority because the core extensions are not movable. Current
  `postgis`, `postgis_raster`, and `postgis_topology` extension control files
  remain `relocatable = false`, while `postgis_sfcgal` is relocatable.

## Developer Tooling

* Define review and update ownership for the repository-owned AI-agent skills
  under `doc/skills/`, add validation for their maintained-documentation links,
  and document how generated or provider-specific copies avoid drifting from
  the repository source.

## Topology

* Investigate topology-aware shapefile loader and dumper workflows, including
  `shp2pgsql`/`pgsql2shp` support for topology elements and TopoGeometries.
  Current maintained tools include `pgtopo_export` and `pgtopo_import`, and
  `pgsql2shp` can export ordinary query results, but the shapefile
  loader/dumper still documents geometry and geography rather than topology
  element or TopoGeometry modes.
* Investigate a no-new-primitives mode for `toTopoGeom`, allowing it to fail
  instead of adding topology primitives when the input cannot be expressed
  using existing edges, nodes, and faces. Current `toTopoGeom` signatures accept
  geometry, topology/layer or TopoGeometry target, and tolerance, but no
  primitive-creation policy flag.
* Investigate topology import paths for OSM and E00-style coverage sources.
  Historical planning notes asked for an `osm2topology` converter and for
  importing E00 coverage/topology data. Current in-tree topology tools focus on
  SQL/topology management and `pgtopo_export` / `pgtopo_import`; OSM, E00, and
  routing-specific import design remains unowned.
* Investigate a first-class attach/detach workflow for TopoGeometry layers that
  are copied or imported independently of their user tables. A design would
  need stable layer identity, ownership and privilege rules, relation-table
  validation, and explicit behavior when the corresponding feature table is
  absent or later reattached.
