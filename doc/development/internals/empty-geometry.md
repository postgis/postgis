---
title: "Empty geometry semantics"
date: 2026-06-26
weight: 30
geekdocHidden: false
---

Empty geometries are valid geometry values with no coordinates. They are not
SQL `NULL`: `NULL` means an unknown or absent SQL value, while `EMPTY` is a
known geometry value that can be produced by a successful spatial operation.

The old `DevWikiEmptyGeometry` page treated this as an open design question.
Most of those questions are now resolved in code, regression tests, and user
manual behavior. The useful rule of thumb is:

* Constructors, serializers, and accessors preserve typed empties where the
  type is meaningful.
* Measurements of size usually collapse empties to zero.
* Distance and nearest-point style functions usually return `NULL` when no
  point pair exists.
* Topological predicates involving empties usually return false; relation
  matrix behavior can still report exterior/exterior overlap for two empties.
* Empty collections and collections containing empty members need explicit
  handling because serialized emptiness and collection membership are not the
  same question.

## Empty is not NULL

`ST_IsEmpty(geom)` tests the geometry value itself. It returns true for
values such as `POINT EMPTY`, `POLYGON EMPTY`, and `GEOMETRYCOLLECTION EMPTY`.
PostGIS keeps normal SQL `NULL` semantics: strict SQL functions return `NULL`
before reaching C code, so `ST_IsEmpty(NULL::geometry)` is `NULL`, not false.

Typed empty values are preserved by WKT/WKB output. Regression tests in
`regress/core/regress.sql`, `regress/core/wkb.sql`, and
`regress/core/tickets.sql` cover empty points, lines, polygons, curves,
triangles, TINs, polyhedral surfaces, and collections.

## Storage and indexing

Serialized geometry can be empty even when it has a concrete geometry type.
In `liblwgeom/gserialized1.c`, empty points and zero-length coordinate arrays
are represented without a coordinate payload and without a calculable bounding
box. `gserialized_is_empty()` is the cheap test used throughout the PostgreSQL
extension layer.

Indexes have to represent "no box" separately from a normal bounding box:

* GiST and SP-GiST code use empty box sentinels when a geometry has no
  bounding box.
* Many bounding-box operators short-circuit when either side is empty because
  there is no spatial extent to compare.
* Some box predicates still model set-theoretic intuition for the box layer:
  non-empty boxes can contain empty boxes, and empty boxes can be within
  non-empty boxes. That does not automatically define the SQL predicate
  behavior for full geometries.

This distinction matters when changing filters or planner support. Do not infer
the behavior of `ST_Contains`, `ST_Within`, or `ST_Intersects` only from
bounding-box helper behavior.

## Measurements and accessors

Current behavior generally treats the size of an empty geometry as zero:

| Operation family | Empty behavior |
|------------------|----------------|
| `ST_Area` | Returns `0` for empty geometry/geography inputs. |
| `ST_Length` and geography perimeter/length helpers | Return `0` for empty inputs where the measurement applies. |
| `ST_NumGeometries` | Returns `0` for empty geometries. |
| `ST_NumInteriorRings` / `ST_NRings` | Return `0` for empty polygonal inputs. |
| `ST_NumPoints` | Returns `0` for empty linear inputs and `NULL` for non-linear inputs where the accessor does not apply. |
| `ST_GeometryN` | Returns `NULL` for empty inputs. |
| `ST_InteriorRingN` | Returns `NULL` for empty polygonal inputs. |
| `ST_ExteriorRing` | Returns an empty line for empty polygonal inputs. |

The implementation anchors are `postgis/lwgeom_ogc.c` and
`postgis/lwgeom_functions_basic.c`. The accessor behavior is also reflected in
`doc/reference_accessor.xml` and regression coverage such as
`regress/core/polyhedralsurface.sql` and `regress/core/tickets.sql`.

## GEOS-backed operations

Many geometry operations use GEOS, but PostGIS still handles common empty cases
before calling GEOS. That keeps output types stable and avoids passing work to
GEOS when no coordinate operation is possible.

Examples already fixed by regression tests:

* `ST_Buffer(POLYGON EMPTY, radius)` returns an empty polygon.
* `ST_BuildArea(POINT EMPTY)` returns an empty polygon.
* `ST_ConvexHull(POLYGON EMPTY)` returns an empty polygon.
* `ST_Centroid(POLYGON EMPTY)` returns `POINT EMPTY`.
* `ST_IsValid(POLYGON EMPTY)` returns true.
* `ST_DFullyWithin(non_empty, empty, distance)` returns false.
* `ST_Relate(POINT EMPTY, POINT EMPTY)` returns `FFFFFFFF2`.

The relevant tests are in `regress/core/tickets.sql`, with expected values in
`regress/core/tickets_expected` for tickets such as `#682`, `#683`, `#684`,
`#685`, `#687`, `#712`, and `#1060`.

## Predicates

The old Trac page debated whether empty geometries should follow pure
set-theoretic laws or existing spatial-database convention. Current PostGIS
behavior is closer to the latter for SQL predicates:

| Predicate | Empty behavior |
|-----------|----------------|
| `ST_Intersects(empty, anything)` | false |
| `ST_DWithin(empty, anything, distance)` | false, even for infinite distance |
| `ST_DFullyWithin(empty, anything, distance)` | false |
| `ST_Contains(geometry, empty)` | false |
| `ST_Within(empty, geometry)` | false |
| `ST_CoveredBy(empty, geometry)` | false in ticket `#689` for an empty polygon covered by a line |
| `ST_Disjoint(empty, geometry)` | true for the empty cases recorded on the old wiki and consistent with the tested `FFFFFFFF2` relate matrix |
| `ST_Equals(POINT EMPTY, POINT EMPTY)` | true for matching empty point values |

The infinite-distance `ST_DWithin` case is intentionally tested by ticket
`#5008`: both `ST_DWithin('POINT EMPTY', 'POINT(0 0)', 'Inf')` and
`ST_DWithin('POINT(1 1)', 'POLYGON EMPTY', 'Inf')` return false.

When changing a predicate, check both the C short-circuit and the indexed SQL
wrapper. Public predicates such as `ST_Intersects`, `ST_Contains`,
`ST_Within`, and `ST_DWithin` have planner support functions and index-aware
operator paths in `postgis/postgis.sql.in`; their uncached `_ST_*` variants
can share C functions but do not prove the indexed wrapper behavior by
themselves.

## Collections with empty members

Collections require a separate mental model:

* `GEOMETRYCOLLECTION EMPTY` is empty.
* `GEOMETRYCOLLECTION(POINT EMPTY)` has a member, but the member is empty.
* `GEOMETRYCOLLECTION(POINT EMPTY, LINESTRING(0 0, 1 1))` is not empty because
  at least one member has coordinates.

This distinction shows up in serialization and collection handling. The
`gserialized1_is_empty()` comment explicitly notes that a shallow zero-count
test is not enough to catch collections such as
`GEOMETRYCOLLECTION(POINT EMPTY)`. Code that recursively deserializes or
normalizes collections needs to decide whether it is asking "does the container
have members?" or "does any member contribute coordinates?"

The old Trac guidance still gives the right design instinct: operations over
mixed collections should generally ignore empty members when non-empty members
determine the result, but all-empty collections should still follow the empty
rules above.

## Practical guidance

When adding or changing empty-geometry behavior:

1. Preserve SQL `NULL` semantics separately from empty geometry semantics.
2. Add regression coverage for typed empties, not only
   `GEOMETRYCOLLECTION EMPTY`.
3. Test collection cases containing empty members and non-empty members.
4. Check geometry and geography behavior separately; geography measurement
   code has its own empty short-circuits.
5. Check both exact operators and indexed wrappers when predicates are involved.
6. Prefer returning typed empty results for constructive operations where a
   stable output type is already established by existing behavior.
