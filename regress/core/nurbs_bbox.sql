--
-- NURBS Curve Bounding Box Robustness Tests
--
-- A non-finite (NaN or infinite) control point ordinate must not make the
-- recursive Bezier-subdivision bbox in liblwgeom/gbox.c spin. Before this
-- fix, NaN never compared equal to itself, so neither convergence test in
-- lwnurbscurve_add_bezier_span_gbox() could ever succeed and recursion ran
-- to DBL_MANT_DIG depth on every remaining span: a backend CPU denial of
-- service reachable from a tiny low-privilege WKT string. Every query below
-- must return promptly; a regression that reintroduces the hang will make
-- this file time out instead of reporting a diff.
--

SET client_min_messages TO WARNING;

-- A normal (fully finite) NURBSCURVE must keep its tight recursive bbox.
SELECT 'nurbs_bbox_finite', Box2D('NURBSCURVE(2, (0 0, 1 1, 2 0))'::geometry);

-- NaN in an X/Y control point ordinate. The X range is unaffected by the
-- NaN in Y, and Y must come back finite instead of hanging.
SELECT 'nurbs_bbox_nan_xy',
       ST_XMin(g) = 0 AND ST_XMax(g) = 2 AND ST_YMin(g) = 0 AND
       (ST_YMax(g) = ST_YMax(g) AND ST_YMax(g) > '-Infinity'::float8 AND ST_YMax(g) < 'Infinity'::float8)
  FROM (SELECT 'NURBSCURVE(2, (0 NaN, 1 1, 2 0))'::geometry AS g) s;

-- NaN in the Z ordinate of a 3D curve.
SELECT 'nurbs_bbox_nan_z',
       ST_XMin(g) = 0 AND ST_XMax(g) = 2 AND
       (ST_ZMax(g) = ST_ZMax(g) AND ST_ZMax(g) > '-Infinity'::float8 AND ST_ZMax(g) < 'Infinity'::float8)
  FROM (SELECT 'NURBSCURVE Z(2, (0 0 NaN, 1 1 1, 2 0 0))'::geometry AS g) s;

-- NaN in the M ordinate of a measured curve.
SELECT 'nurbs_bbox_nan_m', ST_AsText(g) LIKE 'NURBSCURVE M %'
  FROM (SELECT 'NURBSCURVE M(2, (0 0 NaN, 1 1 1, 2 0 0))'::geometry AS g) s;

-- A NaN control-point weight is already rejected by the WKT parser
-- (liblwgeom/lwin_wkt_parse.c) before it can reach the bbox code; kept here
-- so a future relaxation of that guard stays covered by this file.
SELECT 'nurbs_bbox_nan_weight', 'NURBSCURVE(2, (0 0, 1 1, 2 0), (1, NaN, 1), (0, 0, 0, 1, 1, 1))'::geometry;
