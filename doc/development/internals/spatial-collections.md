---
title: "Spatial collections and raster/vector crossings"
date: 2026-06-26
weight: 40
geekdocHidden: false
---

The historical `DevWikiSpatialCollectionTutorial` page described an internal
`SPATIAL_COLLECTION` abstraction intended to erase the difference between
vector geometries and rasters. That abstraction is not present in the current
source tree: there are no remaining `sc_create_*`, `SPATIAL_COLLECTION`, or
`sc_sampling_engine` implementation paths.

The useful idea is still worth keeping: PostGIS has two kinds of collection
work that should not be confused.

* Geometry collections are real geometry values: `MULTI*`,
  `GEOMETRYCOLLECTION`, curves, TINs, and polyhedral surfaces.
* Raster/vector crossings are sampling, vectorization, rasterization, clipping,
  and overlay workflows that move values between rasters and geometries.

Current code keeps those areas separate. When changing collection behavior,
check which side of that boundary the function is on before reusing examples
from the old spatial-collection tutorial.

## Geometry Collections

Geometry collections are containers inside the geometry type system. Their
main implementation and SQL entry points are:

| Operation | Current anchor |
|-----------|----------------|
| Create a collection | `ST_Collect` / `LWGEOM_collect` |
| Expand elements | `ST_Dump`, `ST_DumpPoints`, `ST_DumpSegments`, `ST_DumpRings` |
| Count elements | `ST_NumGeometries` / `LWGEOM_numgeometries_collection` |
| Fetch one element | `ST_GeometryN` / `LWGEOM_geometryn_collection` |
| Keep one element type | `ST_CollectionExtract` |
| Simplify collection shape | `ST_CollectionHomogenize` |

`ST_Collect` is intentionally a container operation, not an overlay operation.
It aggregates geometries without changing their coordinate content. If all
inputs are compatible atomic types, the result can be a `MULTI*` geometry. If
the inputs are heterogeneous or already collections, the result has to be a
`GEOMETRYCOLLECTION`, because only a geometry collection can contain nested or
mixed-type components.

That differs from `ST_Union`, which can dissolve boundaries, split lines at
intersections, and return a different geometry because it performs topology.
Do not replace one with the other only because both can accept many
geometries.

## Decomposition and Paths

Use `ST_Dump` when downstream code should operate on atomic components. It is
the inverse shape of `ST_Collect` in common `GROUP BY` workflows: `ST_Collect`
turns many rows into one collection value, while `ST_Dump` turns one collection
value into many rows with a `geometry_dump` record.

Each dump record has:

* `geom`: the component geometry.
* `path`: an integer array describing where that component appeared inside the
  original collection.

For atomic geometries, `ST_Dump` returns one record with an empty path and the
original geometry. For multi-geometries and geometry collections, the path
identifies the component position. `ST_GeometryN` is useful for direct
1-based access when the caller already knows the index; `ST_Dump` is usually
the better primitive when all elements must be processed.

`ST_NumGeometries` follows the same current model: non-empty atomic geometries
count as one, empty geometries count as zero, and `MULTI*` /
`GEOMETRYCOLLECTION` values count their immediate elements. TIN and
polyhedral-surface patch counting has a separate path so
`ST_NumGeometries` can treat them as unitary geometries while `ST_NumPatches`
can count faces.

## Extracting and Homogenizing

`ST_CollectionExtract` answers "which components of this collection have the
requested base type?" It supports point, line, and polygon extraction. If the
input is an atomic geometry of the requested type, it returns that geometry.
If the atomic input does not match, it returns an empty geometry of the
requested type. If the input is a collection, extraction recurses through it.

When no explicit type is supplied, `ST_CollectionExtract` keeps the highest
dimension present: polygons over lines, lines over points.

`ST_CollectionHomogenize` answers a different question: "what is the simplest
representation of this collection?" It can turn a homogeneous collection into
the matching `MULTI*` type, flatten heterogeneous nested collections into one
`GEOMETRYCOLLECTION`, or return a single atomic geometry when the collection
contains only one atomic element.

Neither function validates polygon topology after recombining components.
Adjacent or overlapping polygon members can produce an invalid `MULTIPOLYGON`.
Callers that need valid polygonal output must check with `ST_IsValid` and
repair with `ST_MakeValid` or use an overlay operation where dissolving is the
intended behavior.

## Empty Collection Cases

Empty handling is related but not identical to collection handling. See
[Empty geometry semantics](empty-geometry.md) for the detailed rules.

The short version for collection code:

* `GEOMETRYCOLLECTION EMPTY` has no members and no coordinates.
* `GEOMETRYCOLLECTION(POINT EMPTY)` has a member, but the member contributes no
  coordinates.
* `GEOMETRYCOLLECTION(POINT EMPTY, LINESTRING(0 0, 1 1))` is not empty because
  one member contributes coordinates.

When normalizing or extracting from collections, be explicit about whether the
code is asking about container membership, coordinate contribution, output type,
or SQL `NULL`. Those are four different questions.

## Raster/Vector Crossings

The old spatial-collection abstraction tried to model both raster and vector
objects as things that can answer:

1. Does point `P` belong to this object?
2. What numeric value does this object return at point `P`?

Current PostGIS does not implement that as a shared C interface. Instead,
raster/vector crossings are exposed as specific SQL functions and raster C
paths:

| Workflow | Current anchor |
|----------|----------------|
| Read raster value at a point | `ST_Value` / `RASTER_getPixelValue` |
| Convert pixels to value geometries | `ST_DumpAsPolygons` / `RASTER_dumpAsPolygons` / `geomval` |
| Burn geometry values into a raster | `ST_SetValues`, `ST_SetValue`, `ST_AsRaster` |
| Clip raster by geometry | `ST_Clip` / `RASTER_clip` |
| Intersect raster and geometry | `ST_Intersection(geometry, raster)` and related wrappers |
| Convert geometry to raster grid | `ST_AsRaster` and `ST_AsRasterAgg` |
| Compare raster and geometry boxes | raster/geometry operators in `raster/rt_pg/rtpostgis.sql.in` |

The public raster `ST_Intersection(geometry, raster)` variants work in vector
space: the raster is vectorized with `ST_DumpAsPolygons`, then those `geomval`
rows are intersected with the input geometry using the normal geometry
`ST_Intersection`. Raster-returning variants and clipping paths instead
construct or modify raster cells.

This is close to the old tutorial's intent, but the implementation boundary is
different. There is no single current "spatial collection" object that a caller
wraps around a raster or geometry and samples through a shared interface.

## Practical Guidance

When touching code near this topic:

1. For geometry collections, start from the geometry type-system functions:
   `ST_Collect`, `ST_Dump`, `ST_GeometryN`, `ST_NumGeometries`,
   `ST_CollectionExtract`, and `ST_CollectionHomogenize`.
2. For raster/vector operations, start from the raster SQL wrappers and raster
   C files, especially `raster/rt_pg/rtpostgis.sql.in`,
   `raster/rt_pg/rtpg_geometry.c`, `raster/rt_pg/rtpg_mapalgebra.c`, and
   `raster/rt_pg/rtpg_spatial_relationship.c`.
3. Do not revive the old `SPATIAL_COLLECTION` API without a new design review.
   Current code has evolved around explicit SQL functions rather than a shared
   internal wrapper.
4. Preserve the distinction between container operations and topology
   operations. `ST_Collect` stores components; overlay functions change spatial
   structure.
5. Preserve the distinction between raster values and geometry membership.
   Raster workflows often carry a numeric `geomval`; geometry collections do
   not have a generic per-point value vector.

The old tutorial remains useful as design history, but maintained developer
documentation should describe the current implementation boundary first.
