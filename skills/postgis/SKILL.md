---
name: postgis
description: PostGIS-specific SQL tips and gotchas. Use when working with geometry, geography, raster, projections, spatial indexes, or geospatial processing in PostgreSQL through PostGIS.
---

## Naming

 - Spell PostGIS functions as they appear in the manual (`st_segmentize` -> `ST_Segmentize`).
 - Call geometry columns `geom`; call geography columns `geog`.

## Checks

 - Check `select postgis_full_version();` to confirm the installed PostGIS, GEOS, PROJ, GDAL, and SFCGAL versions when debugging extension behavior.

## Raster

 - Do not work with GDAL on the filesystem. Import things into database and deal with data there.

## Gotchas

 - Do not use geometry typmod unless requested (things like `geometry(multilinestring, 4326)`) - use plain `geometry` or `geography` instead. This removes clutter of `ST_Multi` and errors via `ST_SetSRID`.
 - `ST_UnaryUnion(ST_Collect(geom))` is just `ST_Union(geom)`.
 - `ST_Buffer(geom, 0)` should be `ST_MakeValid(geom)` when invalid geometry must be repaired, but first prefer fixing the source of invalid geometry upstream.
 - `select min(ST_Distance(..))` should be `select ST_Distance() ... order by a <-> b limit 1` to enable knn gist.
 - `order by ST_Distance(c.geog, t.geog)` should be `order by c.geog <-> t.geog`.
 - `ST_UnaryUnion` is a sign you're doing something wrong.
 - Needing `ST_MakeValid` is a sign you're doing something wrong on the previous step.
 - Be extra attentive when calling `ST_SetSRID`: first check the actual projection of input data. Prefer setting the SRID during input when the source format supports it, for example with `ST_GeomFromGeoJSON`, EWKT like `SRID=4326;POINT(1 2)`, or EWKB; use `ST_Transform` instead if reprojection is needed.
 - When looking for relation between point and polygon, prefer `ST_Intersects` to other topology predicates.
 - When generating complex geometry by walking raster or grid, it may make sense to `ST_Simplify(geom, 0)`.
 - To generate neighbourhoods of predictable size, use `ST_ClusterKMeans` with `k = 2` and `max_radius` set to your distance.
 - Use `ST_AsEWKB` for binary representation instead of `ST_AsWKB` to keep SRID.
 - Choosing projection:
    SRID=4326 (2D longlat) when input or output is longitude and latitude and coordinate value is to be shown to user.
    SRID=3857 (2D Spherical Mercator) when output will be shown on web map, ST_AsMVT, or 2D KNN requests of short distance are to be executed.
    SRID=4978 (3D XYZ) when performing internal computations, line-of-sight, clustering and averaging across antimeridian. Beware: only use 3D-aware operations, ST_Force3DZ on 2D CRS data before calling ST_Transform to it.
 - Instead of using `ST_Hexagon` / `ST_HexagonGrid`, use the `h3` extension.
 - When you know the data is going to be dumped in binary form, gzipped and moved around, consider using `ST_QuantizeCoordinates` if precision is known.
