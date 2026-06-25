# 2D and 3D Distance Internals

This page is the maintained version of the useful design notes from the Trac
`3DDistancecalc` page, checked against the current distance code in
`liblwgeom/measures.c`, `liblwgeom/measures3d.c`, and the SQL wrappers in
`postgis/lwgeom_functions_basic.c`.

The user-facing manual documents the SQL behavior of functions such as
`ST_Distance`, `ST_3DDistance`, `ST_3DDWithin`, `ST_3DDFullyWithin`,
`ST_3DIntersects`, `ST_3DClosestPoint`, `ST_3DShortestLine`, and
`ST_3DLongestLine`. This page is only an implementation map for developers
working on the distance code.

## Candidate Points

All distance calculations eventually compare two candidate points. The work is
in finding the candidate pair that gives the minimum or maximum distance for
the requested mode.

In 2D, closest-distance candidates are found from vertex-to-vertex,
vertex-to-segment, and segment-intersection cases. A segment intersection is a
zero-distance candidate, so the 2D code can stop early for minimum-distance
queries once it finds one.

In 3D, two closest points can both lie in the interior of segments. The 3D line
and boundary code therefore has to test segment-to-segment closest approach,
not only vertices projected onto opposite segments. The 3D code also has
surface candidates: a point or line segment can project onto the interior of a
polygon or triangle, and a line segment can cross a surface.

## Current 3D Flow

The SQL wrappers call `lwgeom_mindistance3d`,
`lwgeom_mindistance3d_tolerance`, or the 3D maximum-distance helpers.
Those helpers initialize a `DISTPTS3D` accumulator and then enter
`lw_dist3d_recursive`.

`lw_dist3d_recursive` handles collections before primitive dispatch. For
minimum-distance tolerance checks, recursion can stop once the accumulator is at
or below the requested tolerance. Empty children are skipped. Supported child
components of collection-like inputs therefore reach the same primitive
dispatch path as standalone geometries.

Primitive dispatch happens in `lw_dist3d_distribute_bruteforce`. It routes
point, line, polygon, and triangle combinations to pair-specific helpers such
as `lw_dist3d_point_line`, `lw_dist3d_line_line`,
`lw_dist3d_line_poly`, `lw_dist3d_poly_poly`, and `lw_dist3d_tri_tri`.

For line and boundary comparisons, `lw_dist3d_ptarray_ptarray` iterates over
segment pairs for minimum distance and calls `lw_dist3d_seg_seg`. That helper
computes the closest approach between two 3D segments and falls back to
point-to-segment checks when the closest points on the supporting lines fall
outside the finite segments or when a segment is degenerate. Maximum distance
for point arrays is a vertex-pair search.

For surfaces, the current code defines a plane with `define_plane`, projects
points with `project_point_on_plane`, and checks projected points with
`pt_in_ring_3d`. If a polygon or triangle plane cannot be defined, the 3D code
falls back to boundary distance checks. Point-to-surface helpers use the
projected point when it lies inside the surface and outside any polygon holes;
otherwise they fall back to boundary distance.

Line-to-surface helpers project the endpoints onto the surface plane and check
whether a segment crosses the surface interior. If no crossing or interior
projection supplies the answer, the code compares the line against the surface
boundary. Polygon and triangle pair helpers check each boundary against the
other surface in both directions so that the closest-line point order still
matches the input geometry order.

## What Stayed Archival

The old Trac page included diagrams and a historical description of plane
normal averaging. Those details should not be treated as maintained text until
the missing images are recovered and the exact narrative is checked against the
current `define_plane` implementation. The imported Trac page remains in
`doc/trac-wiki/development/3DDistancecalc.tracwiki` while that image-recovery
thread is still open.
