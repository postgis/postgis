-- Tests for SFCGAL NURBS curve functions
-- These functions are SFCGAL-specific and work with NURBS curve geometries

-- CG_NurbsCurveDerivative tests
-- Computes the derivative (tangent, curvature) of a NURBS curve at a given parameter

-- Test first derivative (tangent) at midpoint
SELECT 'derivative_order1_mid', ST_AsText(CG_NurbsCurveDerivative(
    'NURBSCURVE (2, (0.00 0.00, 5.00 10.00, 10.00 0.00))'::geometry,
    0.5,
    1
), 2);

-- Test first derivative at start
SELECT 'derivative_order1_start', ST_AsText(CG_NurbsCurveDerivative(
    'NURBSCURVE (2, (0.00 0.00, 5.00 10.00, 10.00 0.00))'::geometry,
    0.0,
    1
), 2);

-- Test second derivative
SELECT 'derivative_order2', ST_AsText(CG_NurbsCurveDerivative(
    'NURBSCURVE (3, (0.00 0.00, 2.00 8.00, 5.00 5.00, 8.00 2.00, 10.00 0.00))'::geometry,
    0.5,
    2
), 2);

-- Test derivative with weighted NURBS
SELECT 'derivative_weighted', ST_AsText(CG_NurbsCurveDerivative(
    'NURBSCURVE (2, (0.00 0.00, 5.00 10.00, 10.00 0.00), (1.00, 2.00, 1.00))'::geometry,
    0.5,
    1
), 2);

-- Test derivative with 3D NURBS
SELECT 'derivative_3d', ST_AsText(CG_NurbsCurveDerivative(
    'NURBSCURVE Z (2, (0.00 0.00 0.00, 5.00 10.00 5.00, 10.00 0.00 0.00))'::geometry,
    0.5,
    1
), 2);

-- CG_NurbsCurveInterpolate tests
-- Creates an interpolating NURBS curve passing through all given data points

-- Test interpolation with 4 points, degree 3
SELECT 'interpolate_deg3', ST_AsText(CG_NurbsCurveInterpolate(
    'LINESTRING(0 0, 2 5, 5 3, 8 1)'::geometry,
    3
), 2);

-- Test interpolation with 5 points, degree 2
SELECT 'interpolate_deg2', ST_AsText(CG_NurbsCurveInterpolate(
    'LINESTRING(0 0, 2 3, 4 2, 6 4, 8 0)'::geometry,
    2
), 2);

-- Test interpolation with 3D points
SELECT 'interpolate_3d', ST_AsText(CG_NurbsCurveInterpolate(
    'LINESTRING Z (0 0 0, 2 5 2, 5 3 1, 8 1 0)'::geometry,
    3
), 2);

-- Test interpolation with MultiPoint
SELECT 'interpolate_multipoint', ST_AsText(CG_NurbsCurveInterpolate(
    'MULTIPOINT(0 0, 2 5, 5 3, 8 1)'::geometry,
    3
), 2);

-- Test interpolation with empty geometry
SELECT 'interpolate_empty', CG_NurbsCurveInterpolate(
    'LINESTRING EMPTY'::geometry,
    3
) IS NULL AS is_null;


-- CG_NurbsCurveApproximate tests
-- Creates an approximating NURBS curve fitting data points within a tolerance

-- Test approximation with default max control points
SELECT 'approximate_default', ST_AsText(CG_NurbsCurveApproximate(
    'LINESTRING(0 0, 1 2, 2 3, 3 2, 4 1, 5 0)'::geometry,
    3,
    0.1
), 2);

-- Test approximation with custom max control points
SELECT 'approximate_max_ctrl', ST_AsText(CG_NurbsCurveApproximate(
    'LINESTRING(0 0, 1 1, 2 4, 3 9, 4 16, 5 25, 6 36, 7 49, 8 64, 9 81, 10 100)'::geometry,
    3,
    1.0,
    5
), 2);

-- Test approximation with high tolerance
SELECT 'approximate_high_tol', ST_AsText(CG_NurbsCurveApproximate(
    'LINESTRING(0 0, 1 2.1, 2 1.9, 3 3.2, 4 2.8, 5 1)'::geometry,
    2,
    0.5
), 2);

-- Test approximation with 3D points
SELECT 'approximate_3d', ST_AsText(CG_NurbsCurveApproximate(
    'LINESTRING Z (0 0 0, 1 2 1, 2 3 2, 3 2 1, 4 1 0, 5 0 0)'::geometry,
    3,
    0.1
), 2);

-- Test approximation with empty geometry
SELECT 'approximate_empty', CG_NurbsCurveApproximate(
    'LINESTRING EMPTY'::geometry,
    3,
    0.1
) IS NULL AS is_null;
