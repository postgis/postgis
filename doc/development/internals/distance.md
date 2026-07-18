---
title: "2D and 3D Distance Internals"
date: 2026-06-26
weight: 90
geekdocHidden: false
---

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

## Current 2D Flow

The 2D minimum-distance entry points initialize a `DISTPTS` accumulator and call
`lw_dist2d_comp`, which delegates to `lw_dist2d_recursive`. Recursion unwraps
collections, skips empty children, adds missing bounding boxes, and then chooses
between the fast path and primitive dispatch for each primitive pair.

For non-overlapping line, polygon, and triangle pairs, the current code still
uses the historical projection-sort idea described by `NewDistCalcGeom2Geom`.
`lw_dist2d_distribute_fast` extracts the relevant point arrays and calls
`lw_dist2d_fast_ptarray_ptarray`. That helper projects vertices onto an axis
perpendicular to the line between the primitive bounding-box centers, sorts
those projected measures, and narrows segment-pair checks once the current best
distance proves that farther projected vertices cannot improve the answer.

When primitive bounding boxes overlap, when the query is a maximum-distance
query, or when a geometry type is outside that fast-path set, the code falls
back to `lw_dist2d_distribute_bruteforce`. The primitive helpers then compare
point-to-point, point-to-segment, segment-to-segment, arc, ring, and polygon
containment cases, with early exit for minimum-distance tolerance checks once
the accumulator is at or below tolerance.

Repeated SQL distance calls can use a different acceleration layer. The
`ST_DistanceRectTree` and `ST_DistanceRectTreeCached` wrappers in
`postgis/lwgeom_rectree.c` build `RECT_NODE` trees and call
`rect_tree_distance_tree`, with a PostgreSQL function-call cache for the stable
argument in the cached variant. That rect-tree path is separate from the
`lw_dist2d_fast_ptarray_ptarray` projection-sort path preserved in
`liblwgeom/measures.c`.

## Subgeometry Pruning

The old `NewDistCalcSubGeom` note described flattening nested collections into
subgeometry boxes, ordering candidate box pairs, and stopping once the next
candidate box distance could no longer improve the best exact distance. That
specific PostGIS 1.4-era workflow should not be republished as current code,
but the pruning idea lives on in the rect-tree implementation.

`rect_tree_from_lwgeom` builds a `RECT_NODE` tree for points, lines, polygons,
curves, multis, collections, polyhedral surfaces, TINs, and triangles. Point
arrays become one leaf per edge. Polygon rings and collection children become
subtrees. Collection-like children are sorted by a z-order hash before merging
so nearby boxes tend to live in nearby tree nodes.

`rect_tree_distance_tree` checks area containment cases up front, initializes a
global minimum and maximum distance state, and then recurses through tree node
pairs. During recursion, `rect_node_min_distance` supplies a lower bound for a
node pair and `rect_node_max_distance` can tighten the global upper bound. If a
node pair's minimum possible distance is already greater than the current upper
bound, that pair is pruned without descending to every segment. When both nodes
are leaves, `rect_leaf_node_distance` runs the exact point, segment, or arc
distance calculation.

Before descending, `rect_tree_node_sort` orders child nodes by squared distance
from the other node's center. That keeps promising child pairs earlier in the
search, which helps tighten the global bounds sooner. This is the maintained
successor to the old page's "sort subgeometries by bounding boxes, then stop
when the boxes are too far away" idea.

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

The old Trac page included plane-definition diagrams and a historical
description of plane normal averaging. The recovered diagrams now live beside
this maintained note under `doc/development/internals/images/distance/`; the
old narrative should not be treated as maintained text until it is checked
against the current `define_plane` implementation.

The old `NewDistCalcGeom2Geom` diagrams are also recovered under
`doc/development/internals/images/distance/geom2geom/`. The maintained summary
above keeps the part that still matches current code and leaves the step-by-step
PostGIS 1.5 wiki walkthrough in the archive.

The old `NewDistCalcSubGeom` illustrations are recovered under
`doc/development/internals/images/distance/subgeom/`. The maintained summary
above maps the surviving subgeometry-pruning idea to the current rect-tree code.
