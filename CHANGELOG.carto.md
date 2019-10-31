## 2.5.1.8+carto-1

**Release date**: 20XX-XX-XX

Changes:
- Backport 570fbdb10: Fix oversimplification of polygon inner rings

## 2.5.1.7+carto-1

**Release date**: 2019-10-03

Changes:
- Add removed symbols in 2.5.1.6 (distance2d_pt_seg and getPoint3dz_cp). They are unused, but this ensures ABI compatibility.

## 2.5.1.6+carto-1

**Release date**: 2019-10-03

Changes:
- Merged community `svn-2.5` as of 2019-10-03: In the SQL part, there are only minor changes in the functions to handle raster constraints (not worth deploying in our case).
- Backport [6018250](https://github.com/postgis/postgis/commit/601825064dbf8175f71e2ff896821a7a932ebe4f) to also cache prepared points and multipoints, speeding up ST_Intersects with those types.
- Backport ubsan fixes.
- Backport fixes for compiler warnings.
- Backport [4498a19](https://github.com/postgis/postgis/commit/4498a19f96b12a66ee9d219d071d27d25b71f65a) and [8cbffb3](https://github.com/postgis/postgis/commit/8cbffb350c269ab6744ed189f19d84a6650a1041): Speed up ST_GeometryType, ST_X, ST_Y, ST_Z and ST_M.
- Backport [143a35f](https://github.com/postgis/postgis/commit/143a35f8221275cc61f7b3bfcae4a1ff6507c2dc): Speed up ST_IsEmpty.
- Backport [756c478](https://github.com/postgis/postgis/commit/756c4784566284b5cf01e2a58cb7a4156dbcaef9): Speed up the calculation of cartesian bbox
- Backport [7a6ab51](https://github.com/postgis/postgis/commit/7a6ab51d90a001f5acd236283a0f662bdc9b36dc) and [66a95ca](https://github.com/postgis/postgis/commit/66a95caed47fd1480b14dd588cee5efda9de3f24): Speed up ST_RemoveRepeatedPoints
- Backport [86057e2](https://github.com/postgis/postgis/commit/86057e2e46a272838a54eff9e6ebb5e56f33fab7): Speed up ST_Simplify

## 2.5.1.5+carto-1

**Release date**: 2019-06-04

Changes:
- MVT (GEOS): Avoid NULL dereference when NULL is returned from validation
- Merged community `svn-2.5` as of 2019-06-04: Minimal changes in the SQL for postgis_type_name; not worth to bump the minor version.
- Applied a small patch from trunk to pass tests with GEOS/master.
- Added a static cache for SRID 4326 and 3857 if the lookup fails. This is to prevent failures during pg_upgrade when calling ST_Transform before loading the data of spatial_ref_sys.

## 2.5.1.4+carto-1

**Release date**: 2019-03-20

Changes:
- Merged community `svn-2.5` as of 2019-03-13. No need to bump the minor version as there aren't any changes in SQL code.
- MVT (GEOS): Backport improvements to verify validity of output geometries.
- MVT (Both): Backport a fix for a bug that oversimplifed vertical and horizontal lines ([30fe210](https://github.com/postgis/postgis/commit/30fe210917713755b4f50f867caf68983cad8394).

## 2.5.1.3+carto-1

**Release date**: 2019-03-06

Changes:
- Reduce the cost of `ST_Simplify` from 512 to 64. The cost turned out to be too high for points, creating PARALLEL plans when it wasn't the best option.
- Enable wagyu by default only if protobuf is available.
- Merged community `svn-2.5` as of 2019-03-06.
- Reduce the cost of `_ST_Intersects` from 512 to 128. Same as `ST_Simplify`.

## 2.5.1.2+carto-1

**Release date**: 2019-02-12

Changes:
- carto-package.json: Declare internal package compatibility.
- Merged community `svn-2.5` as of 2019-02-12.
- Backport wagyu introduction ([5508a4f](https://github.com/postgis/postgis/commit/5508a4f89c20686a19f233ef0a04b796d8a2cbaa)).
- Backport wagyu build improvements ([f4cdc57](https://github.com/postgis/postgis/commit/f4cdc57bc7099f8ffa63f065aff3d665228c5a78)).
- Enable wagyu by default.

## 2.5.1.1+carto-1

**Release date**: 2019-01-31

Changes:
- Start from `svn-2.5` ([427ed71](https://github.com/postgis/postgis/commit/427ed71c10683892d4f6b3f0898da9b5745562b4)).
- Backport .travis from trunk (Sanitizers, multiple builds).
- Backport [24efadd](https://github.com/postgis/postgis/commit/24efadd48d94ae7d6ce2aaab66ca940ab97a0a14): GCC warnings.
- Backport clang-format.
- Ported (2.4.1.1+carto-1): make_cartodb_dist.sh.
- Ported (2.4.1.2+carto-1): Strip decorators from 'sql' functions so inlining works better.
- Ported (2.4.1.2+carto-1 / 2.4.1.4+carto-1): Add explicit costs to expensive and other functions.
- Ported (trunk [e5c92b18f](https://github.com/postgis/postgis/commit/e5c92b18ffad323b3996fd68f0b23f80dc5bca28)): ST_AsMVTGeom: Drop geometries smaller than the resolution.
- Add carto-package.json.
- Merged community `svn-2.5` as of 2019-01-25 ([cb703716d](https://github.com/postgis/postgis/commit/fa3163d575b99abe430133909d2cd755c904e9c3)).
- Backport [789707ed2](https://github.com/postgis/postgis/commit/789707ed2c2e67c728cdc088de427f409379944b): ST_AsMVTGeom: Do resolution check before deserializing.
- Merged community `svn-2.5` as of 2019-01-31.
