---
title: "PostGIS Internals"
date: 2026-06-26
weight: 70
geekdocHidden: false
---

# PostGIS Internals

These notes cover implementation details that are useful when working inside
PostGIS rather than only using the SQL API.

## Core SQL And Memory

* [Memory management](internals/memory.md) covers allocator boundaries and ownership rules.
* [PostgreSQL internals for PostGIS developers](internals/postgresql.md) covers PostgreSQL
  C function macros, varlena detoasting, serialized geometry handling, and a
  typical SQL function flow.

## Geometry And Measurement

* [Empty geometry semantics](internals/empty-geometry.md) covers how `EMPTY` differs
  from SQL `NULL`, how empties are serialized and indexed, and how accessors,
  measurements, predicates, and collections handle empty inputs.
* [Spatial collections and raster/vector crossings](internals/spatial-collections.md)
  covers current geometry collection handling and explains how the retired
  raster/vector spatial-collection abstraction relates to current code.
* [2D and 3D distance internals](internals/distance.md) covers how geometry distance
  functions dispatch 2D and 3D candidate checks, including segment-segment and
  surface cases.
* [Precision and tolerance internals](internals/precision-tolerance.md) maps the current
  GEOS grid-size, snap-tolerance, topology-precision, and MVT quantization
  surfaces.

## Raster Internals

* [Raster affine georeferencing](internals/raster-affine.md) covers the scale, skew,
  rotation, and upper-left coefficients used to convert raster cell
  coordinates into spatial coordinates.
* [Raster physical georeferencing parameters](internals/raster-physical-parameters.md)
  covers the reverse calculation from stored affine coefficients back to pixel
  sizes, rotation, and basis-vector separation.
* [Raster storage and WKB](internals/raster-storage.md) covers the current
  PostgreSQL varlena representation, Raster WKB/HexWKB paths, out-db band
  storage, and raster catalog contracts.
* [PostGIS Raster and the GDAL driver](internals/raster-gdal-driver.md) records the
  boundary between PostGIS-maintained raster storage/catalog behavior and the
  external GDAL `PostGISRaster` driver.

## Topology Internals

* [Topology internals](internals/topology.md) covers internal topology
  implementation helpers such as edge end stars.
