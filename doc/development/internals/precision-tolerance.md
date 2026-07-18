---
title: "Precision And Tolerance Internals"
date: 2026-06-26
weight: 100
geekdocHidden: false
---

PostGIS does not have one global tolerance model for all geometry operations.
Precision and tolerance are handled at the API boundary where they affect a
specific algorithm.

## Overlay Grid Size

Current GEOS-backed overlay functions expose precision through `gridSize`, not
through a general `tolerance` argument. `ST_Intersection`, `ST_Difference`,
`ST_SymDifference`, `ST_UnaryUnion`, `ST_Union`, and `ST_Subdivide` have
`gridSize` variants in `postgis/postgis.sql.in`.

The manual documents the contract through `overlay_gridsize_arg`: when
`gridSize` is supplied, GEOS 3.9 or later is required and result vertices are
guaranteed to fall on the specified snap-rounded grid. For repeatable results,
input vertices should already lie on the same grid. Use `ST_ReducePrecision`
when inputs must first be made grid-aligned; use `ST_SnapToGrid` for simpler
coordinate-grid snapping where validity preservation is not the key contract.
At the C boundary, `ST_ReducePrecision` reaches GEOS through
`lwgeom_reduceprecision` and `GEOSGeom_setPrecision`.

This is the modern replacement for the old `ToleranceDiscussion` idea of
passing an implicit GEOS `PrecisionModel` through PostGIS. The precision model
is now explicit per operation where overlay needs it.

## Snap Tolerances

`ST_Snap(input, reference, tolerance)` uses a distance tolerance to snap input
segments and vertices to vertices in a reference geometry. It is a robustness
tool for nearly coincident overlay inputs, but it uses heuristics and can skip
candidate snaps. Snapping can also make an output invalid or non-simple, so it
is not a general precision model.

Distance predicates such as `ST_DWithin`, `ST_3DDWithin`, and
`ST_DFullyWithin` have explicit distance arguments. They are user-visible
spatial predicates, not replacements for exact-topology predicates such as
`ST_Intersects`. Geography predicates also have their own documented
implementation tolerances for some relationship tests.

Other tolerance parameters are algorithm-specific:

* `ST_RemoveRepeatedPoints` treats vertices within the tolerance as duplicate
  consecutive points.
* `ST_DelaunayTriangles`, `ST_VoronoiPolygons`, and `ST_VoronoiLines` use
  tolerance to snap input points for triangulation or Voronoi construction.
* `ST_CoverageInvalidEdges` and `ST_CoverageSimplify` use tolerances for
  coverage validation and simplification.
* Clustering functions such as `ST_ClusterDBSCAN` and `ST_ClusterWithin` use a
  distance threshold for grouping, not for coordinate precision.

## Topology Precision

The topology extension has its own precision and tolerance surface. A topology
created with `CreateTopology(toponame, srid, prec)` records precision in the
topology metadata. Topology loaders and converters such as
`TopoGeo_AddPoint`, `TopoGeo_AddLineString`, `TopoGeo_AddPolygon`,
`TopoGeo_LoadGeometry`, and `toTopoGeom` use tolerance to snap incoming
geometries to existing topology primitives.

Topology precision can also be inspected and repaired with
`ValidateTopologyPrecision` and `MakeTopologyPrecise`, which operate on the
topology precision grid or an explicit `gridSize`. This is closer to a
metadata-backed precision model, but it is scoped to topology schemas and
topological primitives rather than ordinary geometry values.

## MVT Quantization

`ST_AsMVTGeom` has `extent`, `buffer`, and `clip_geom` parameters. These are
not GEOS tolerance controls. The function transforms geometries into integer
Mapbox Vector Tile coordinate space, snaps coordinates to the integer tile
grid, removes repeated and collinear points, optionally clips to the tile
bounds plus the tile-coordinate buffer, and may drop geometries smaller than
half a tile-grid cell before deserializing.

For MVT output, simplify or reduce precision in map coordinates before calling
`ST_AsMVTGeom` if the desired behavior is scale-dependent simplification rather
than tile-coordinate quantization.

## Remaining Design Question

The old Trac page also asked whether PostGIS should infer precision from input
coordinate decimals or carry a per-geometry/server-runtime precision model.
Current code does not provide that metadata path. New work in this area should
start as a design ticket that explains how it interacts with explicit
`gridSize`, topology precision, geography behavior, MVT quantization, and
existing tolerance parameters without changing predicate semantics silently.
