--
-- NURBS Curve Evaluation and Conversion Tests
-- Tests for ST_NurbsEvaluate and ST_NurbsToLineString functions
--

-- Test 1: Basic evaluation of a simple NURBS curve
-- Create a simple quadratic NURBS curve
SET client_min_messages TO WARNING;

-- Test 1: Simple quadratic curve evaluation
SELECT 'NURBS_EVAL_1', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    0.0
), 2) as start_point;

SELECT 'NURBS_EVAL_2', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    1.0
), 2) as end_point;

SELECT 'NURBS_EVAL_3', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    0.5
), 2) as mid_point;

-- Test 2: Evaluation with rational (weighted) NURBS curve
SELECT 'NURBS_EVAL_4', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry, ARRAY[1.0, 2.0, 1.0]),
    0.5
), 2) as weighted_mid_point;

-- Test 3: Test parameter clamping (values outside [0,1])
SELECT 'NURBS_EVAL_5', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    -0.5
), 2) as clamped_low;

SELECT 'NURBS_EVAL_6', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    1.5
), 2) as clamped_high;

-- Test 4: 3D NURBS curve evaluation
SELECT 'NURBS_EVAL_7', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(2, 'LINESTRING Z(0 0 0, 1 1 1, 2 0 2)'::geometry),
    0.5
), 2) as eval_3d;

-- Test 5: LineString conversion with default segments
SELECT 'NURBS_CONV_1', ST_GeometryType(ST_NurbsToLineString(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry)
)) as linestring_type;

-- Test 6: LineString conversion with custom segment count
SELECT 'NURBS_CONV_2', ST_NumPoints(ST_NurbsToLineString(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    10
)) as num_points_10_segments;

SELECT 'NURBS_CONV_3', ST_NumPoints(ST_NurbsToLineString(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    5
)) as num_points_5_segments;

-- Test 7: LineString endpoints should match NURBS endpoints
SELECT 'NURBS_CONV_4',
    ST_Equals(
        ST_StartPoint(ST_NurbsToLineString(
            ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry)
        )),
        ST_NurbsEvaluate(
            ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
            0.0
        )
    ) as start_equals;

SELECT 'NURBS_CONV_5',
    ST_Equals(
        ST_EndPoint(ST_NurbsToLineString(
            ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry)
        )),
        ST_NurbsEvaluate(
            ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
            1.0
        )
    ) as end_equals;

-- Test 8: 3D LineString conversion
SELECT 'NURBS_CONV_6', ST_AsText(ST_NurbsToLineString(
    ST_MakeNurbsCurve(2, 'LINESTRING Z(0 0 0, 1 1 1, 2 0 2)'::geometry),
    4
), 2) as linestring_3d;

-- Test 9: Error handling for invalid inputs
SELECT 'NURBS_ERR_1', ST_NurbsEvaluate(NULL, 0.5) IS NULL as null_curve;

SELECT 'NURBS_ERR_2', ST_NurbsToLineString(NULL, 10) IS NULL as null_curve_conv;

-- Test 10: Cubic NURBS curve evaluation
SELECT 'NURBS_EVAL_8', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(3, 'LINESTRING(0 0, 1 2, 3 1, 4 0)'::geometry),
    0.5
), 2) as cubic_mid_point;

-- Test 11: High-degree NURBS curve
SELECT 'NURBS_EVAL_9', ST_AsText(ST_NurbsEvaluate(
    ST_MakeNurbsCurve(4, 'LINESTRING(0 0, 1 3, 2 1, 3 2, 4 0)'::geometry),
    0.25
), 2) as high_degree_eval;

-- Test 12: Verify evaluation consistency
-- Multiple evaluations of the same curve should produce consistent results
WITH nurbs_curve AS (
    SELECT ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry) as geom
)
SELECT 'NURBS_CONSISTENCY',
    ST_Equals(
        ST_NurbsEvaluate(geom, 0.5),
        ST_NurbsEvaluate(geom, 0.5)
    ) as consistent_eval
FROM nurbs_curve;

-- Test 13: Complex rational NURBS with multiple weights
SELECT 'NURBS_CONV_7', ST_NumPoints(ST_NurbsToLineString(
    ST_MakeNurbsCurve(3,
        'LINESTRING(0 0, 1 2, 2 1, 3 0)'::geometry,
        ARRAY[1.0, 0.5, 2.0, 1.0]
    ),
    20
)) as complex_rational_points;

-- Test 14: Edge case - minimum segments
SELECT 'NURBS_CONV_8', ST_NumPoints(ST_NurbsToLineString(
    ST_MakeNurbsCurve(2, 'LINESTRING(0 0, 1 1, 2 0)'::geometry),
    2
)) as min_segments;