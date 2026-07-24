---
title: "Development TODO"
date: 2026-06-28
weight: 35
geekdocHidden: false
---

This page records high-level development ideas that survived the Trac/wiki
documentation migration but are not accepted designs, release blockers, or
scheduled work. Move an item to a Trac ticket, design document, or pull request
when someone takes ownership of it.

Historical source names in backticks are canonical Trac wiki identifiers. The
corresponding source is available as plain text by appending the identifier and
`?format=txt` to <https://trac.osgeo.org/postgis/wiki/>. Preserve those source
snapshots in the OSGeo archive before retiring Trac. This source route replaces
an import commit identifier that was removed during branch history cleanup.

## Geometry And Precision

* Investigate polygon partitioning below a maximum vertex budget. The old
  `SomeSplitting` discussion called this a `SplitToMaxN`-style operation:
  produce area-covering polygon pieces without adding unnecessary segment split
  points. A future design should consider tessellation plus a GiST
  picksplit-inspired partitioning strategy.
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
  this historical discussion. Source provenance: `PostGIS3` and
  `DevelopmentDiscussion` in the Trac import, plus the serialization discussion
  in `UsersWikiSprint2009Notes`.
* Investigate sparse-polygon containment performance. Current PostGIS has
  selectivity estimators and user-visible tools such as `ST_Subdivide`, but the
  old wishlist asked for better selectivity or decomposition strategies for
  large sparse polygons where the bounding box is a poor proxy for occupied
  area. Source provenance: `UsersWikiWishList`.

## Indexing And Planner

* Investigate a typed bounding-box or envelope API that can carry SRID and
  dimensionality without the size and semantics of a polygon. The historical
  sprint discussion also proposed using casts to this shared box type so
  extensions such as raster could reuse core spatial indexing rather than
  defining parallel operator infrastructure. Current `box2d`, `box3d`,
  geometry-cached boxes, and raster convex-hull indexing cover adjacent cases
  but not this common typed-box contract. Source provenance:
  `DevFOSS4GCodeSprintNotes`.
* Investigate future geometry index tuple designs. Historical proposals include
  carrying SRID in the index tuple, storing very small geometries directly in
  index entries for possible index-only scans, using compact fixed-size
  summaries for complex geometries, and adding nested boxes for
  `GEOMETRYCOLLECTION` trees. Current GiST/SP-GiST support and PostgreSQL 15+
  sort support cover adjacent work, but not these storage/index-tuple designs.
  Source provenance: `PostGIS3`, with SRID mismatch context linked there to
  <https://trac.osgeo.org/postgis/ticket/4096>.
* Investigate geography indexing based on S2-style cells or another cell-based
  strategy, including whether a fast geography `ST_Intersects` GIN index is
  possible and useful. Current geography indexes remain GiST-oriented. Source
  provenance: `PostGIS3`.
* Investigate PostgreSQL-core-dependent spatial index build and clustering
  improvements, such as building GiST pages from spatially adjacent tuple runs
  or preserving heap clustering by inserting near index sibling tuples. Current
  PostGIS sort support addresses part of index build ordering, but these ideas
  remain broader PostgreSQL storage/index research. Source provenance:
  `PostGIS3`.

## Backend Libraries And Robustness

* Investigate streaming execution for spatial workloads that cannot reasonably
  materialize one global intermediate geometry. The historical wishlist called
  out noding very large line sets, constructing polygonal coverages from
  linework, creating TINs from large DEM point sets, and spatial aggregates
  that could emit completed regions as input advances. Current aggregate,
  coverage, noding, and triangulation functions do not provide that general
  feature-stream execution model. Source provenance: `UsersWikiWishList`.
* Investigate deeper backend-library integration. Historical ideas include GEOS
  memory management through PostgreSQL allocation, GEOS coordinate sequences on
  PostGIS point arrays, moving suitable algorithms such as build-area or
  validity repair into upstream GEOS/SFCGAL/CGAL, and adding LWGEOM adapters to
  external geometry-library types. Current SFCGAL/GEOS integration covers many
  operations, but not these ownership and architecture changes. Source
  provenance: `PostGIS3` and `DevelopmentDiscussion`.
* Investigate stronger robustness guarantees for predicates and overlays. The
  old planning notes proposed validity-aware guarantees, cached validity state,
  fallback or retry through another backend library after robustness failures,
  and empirical performance/cost frameworks for regression and planner-cost
  testing. Current GEOS/SFCGAL behavior, validity functions, and regression
  suites do not make this a settled design. Source provenance: `PostGIS3` and
  `DevelopmentDiscussion`.
* Investigate curve-completeness policy: which curve operations should become
  complete in core, which should linearize explicitly, and which should remain
  unsupported until GEOS/SFCGAL or another backend owns the operation. Source
  provenance: `DevelopmentDiscussion`.

## Raster

* Investigate raster density-surface generation from point or line coverages.
  The historical `GoogleSummerCode` page proposed `ST_AsDensity`: count point or
  line features into raster pixels and then apply a smoothing filter using
  neighborhood map algebra. Current PostGIS has `ST_InterpolateRaster` for
  interpolation from input points, but this density-surface workflow is not a
  maintained first-class raster API. Trac provenance: the item was already
  present in `GoogleSummerCode` version 1, authored by `robe` on
  2013-04-02 15:34:22 -0700.
* Investigate internal sub-tiling for very large rasters. The historical
  `WKTRaster/SpecificationWorking03` roadmap described this as sub-tiling of
  rasters for PostGIS 3.0, or possibly a new raster type: rewrite the raster
  type so big rasters stored in one row, up to the PostgreSQL 1GB datum limit,
  are internally tiled and quickly accessed. Current PostGIS has table-level
  raster tiling, out-db rasters, and ordinary PostgreSQL TOAST storage, but no
  currently open pull request checked on 2026-06-28 owns this one-row
  internal-tiling storage redesign. Trac provenance: version 211 was the first
  version found with the sub-tiling item, authored by `pracine` on
  2012-07-25 06:42:29 -0700.
* Investigate first-class raster coverage validation helpers. The historical
  `WKTRaster/SpecificationWorking03` roadmap introduced this as Objective FV.20:
  being able to determine topological characteristics of a coverage, with
  proposed helpers such as `ST_HasOverlaps`, `ST_HasGaps`,
  `ST_HasTileSameSize`, `ST_HasTileAligned`, and `ST_IsRegularlyTiled`.
  Adjacent open raster pull requests exist for tiling, alignment, constraints,
  and overviews, but no currently open pull request checked on 2026-06-28 owns
  this coverage-validation proposal. Trac provenance: version 173 was the first
  version found with the objective, authored by `pracine` on
  2011-11-11 07:51:13 -0800.
* Investigate named raster bands. The historical
  `WKTRaster/SpecificationWorking03` roadmap introduced this as Objective FV.17:
  being able to refer to a band by textual name, later described as "band by
  name" for PostGIS 3.0. It proposed an 8-character band name in the base raster
  WKB format, importer support for assigning band names, and overloads so raster
  functions can address bands by name. Trac provenance: version 31 was the first
  version found with the objective, authored by `pracine` on
  2010-09-21 06:53:05 -0700.
* Investigate first-class raster distance surfaces, historically proposed as
  `ST_EuclideanDistance` for nearest-source distance rasters and
  `ST_CostDistance` for cost-weighted distance over a cost raster. Compare that
  direction with leaving the workflow to map algebra, geometry KNN, GDAL, and
  downstream processing.
* Investigate weighted raster summary/statistics functions beyond the existing
  weighted map-algebra masks and ordinary `ST_SummaryStats` /
  `ST_SummaryStatsAgg` aggregates. The historical sprint notes kept this as an
  open raster analysis idea after the core statistics aggregates existed.
  Source provenance: `DevFOSS4GCodeSprintNotes`.

## 3D And Output Formats

* Investigate 3D-aware geography/cartesian conversion so the internal
  geodetic-to-cartesian and cartesian-to-geodetic paths have a defined policy
  for a third coordinate instead of treating geography as a purely surface
  model. This needs explicit altitude semantics and compatibility rules before
  changing current geography calculations. Source provenance: `PostGIS3`.
* Investigate deeper 3D primitive completeness. Current PostGIS supports
  PolyhedralSurface, Triangle, TIN, X3D/GML output, and many SFCGAL-backed 3D
  operations including 3D intersects, distance, area, volume, intersection,
  union, and difference. Historical notes still leave broader questions around
  3D validity semantics, point-in-polyhedra or 3D relationship models,
  centroid behavior, line-of-sight or visibility APIs, and interchange behavior
  that should be either designed or explicitly declared out of scope. Source
  provenance: `DevelopmentDiscussion`, `DevFOSS4GCodeSprintNotes`, and
  `UsersWikiSprint2009Notes`.
* Investigate richer `ST_AsKML` output metadata. The old wishlist asked for KML
  `extrude`, `tessellate`, and altitude-mode support. Current `ST_AsKML`
  signatures accept geometry or geography, precision, and namespace prefix, and
  current KML parsing code explicitly does not handle `kml:extrude`. Source
  provenance: `UsersWikiWishList`.

## Extension Packaging

* Revisit core-extension schema relocation policy. Historical notes proposed
  installing PostGIS into a dedicated `postgis` schema, then later marked it as
  lower priority because the core extensions are not movable. Current
  `postgis`, `postgis_raster`, and `postgis_topology` extension control files
  remain `relocatable = false`, while `postgis_sfcgal` is relocatable. Source
  provenance: `PostGIS3` and `UsersWikiWishList`.

## Developer Workflow

* Investigate repository policy for PostGIS AI skills files. The 2026
  development meeting raised whether project-owned AI skills files should live
  in the core repository, in a separate repository, and how they should be
  structured as versioned developer assets. No maintained developer policy
  currently settles that question, so keep it as planning work until a design
  is accepted and documented. Source provenance: `PostGISDevelopment2026-1`.

## Topology

* Investigate topology-aware shapefile loader and dumper workflows. The
  historical `GoogleSummerCode` page proposed loading TopoGeometries directly
  with `shp2pgsql`; the later `GoogleSummerCode2022` page expanded this into
  `shp2pgsql`/`pgsql2shp` support for topology elements and TopoGeometries.
  Current maintained tools include `pgtopo_export` and `pgtopo_import`, and
  `pgsql2shp` can export ordinary query results, but the shapefile
  loader/dumper still documents geometry and geography rather than topology
  element or TopoGeometry modes. Trac provenance: the original loader idea was
  already present in `GoogleSummerCode` version 1, authored by `robe` on
  2013-04-02 15:34:22 -0700; the broader 2022 framing first appeared in
  `GoogleSummerCode2022` version 7, authored by `robe` on
  2022-02-19 14:23:10 -0800.
* Investigate a no-new-primitives mode for `toTopoGeom`. The historical
  `GoogleSummerCode` page proposed an extra argument allowing `toTopoGeom` to
  fail instead of adding topology primitives when the input cannot be expressed
  using existing edges, nodes, and faces. Current `toTopoGeom` signatures accept
  geometry, topology/layer or TopoGeometry target, and tolerance, but no
  primitive-creation policy flag. Trac provenance: the item was already present
  in `GoogleSummerCode` version 1, authored by `robe` on
  2013-04-02 15:34:22 -0700.
* Investigate topology-aware `ST_EstimatedExtent`. The historical
  `GoogleSummerCode` page proposed making `ST_EstimatedExtent` recognize a
  schema/table/column that refers to a `topology.layer` entry and return a useful
  layer extent estimate instead of only reading ordinary geometry/geography
  table statistics. Current `ST_EstimatedExtent` is documented and implemented
  against geometry/geography columns and table statistics. Trac provenance: the
  item was already present in `GoogleSummerCode` version 1, authored by `robe`
  on 2013-04-02 15:34:22 -0700.
* Investigate topology import paths for OSM and E00-style coverage sources.
  Historical planning notes asked for an `osm2topology` converter and for
  importing E00 coverage/topology data. Current in-tree topology tools focus on
  SQL/topology management and `pgtopo_export` / `pgtopo_import`; OSM, E00, and
  routing-specific import design remains unowned. Source provenance:
  `PostGIS3` and `UsersWikiWishList`.
* Investigate first-class detached-layer attach/detach APIs for copied
  TopoGeometry layers. The historical `TopologyCopy` design notes proposed
  allowing copied `topology.layer` definitions to exist without immediate
  binding to a physical table column, followed by explicit functions to attach,
  detach, or drop those conceptual layers later. Current `CopyTopology`,
  `pgtopo_export`, and `pgtopo_import` cover copying and transfer workflows,
  but they do not provide this detached-layer lifecycle API. Source
  provenance: `TopologyCopy`.
