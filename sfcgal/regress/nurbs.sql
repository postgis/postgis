-- Tests for NURBS curve SFCGAL functions using native SFCGAL NURBS API

-- Test CG_NurbsCurveFromPoints - Create NURBS curve from control points
SELECT 'nurbs_from_linestring', ST_GeometryType(CG_NurbsCurveFromPoints(
    'LINESTRING(0 0, 5 10, 10 0)'::geometry, 2
));

-- Test CG_NurbsCurveFromPoints - Create NURBS curve from control points (MultiPoint)
SELECT 'nurbs_from_multipoint', ST_GeometryType(CG_NurbsCurveFromPoints(
    'MULTIPOINT(0 0, 5 10, 10 0)'::geometry, 2
));

-- Test CG_NurbsCurveFromPoints with higher degree
SELECT 'nurbs_degree3', ST_GeometryType(CG_NurbsCurveFromPoints(
    'LINESTRING(0 0, 5 5, 10 0, 15 -5)'::geometry, 3
));

-- Test CG_NurbsCurveToLineString conversion
SELECT 'nurbs_to_linestring_8seg', ST_GeometryType(CG_NurbsCurveToLineString(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2),
    8
));

-- Test CG_NurbsCurveToLineString with default segments
SELECT 'nurbs_to_linestring_default', ST_GeometryType(CG_NurbsCurveToLineString(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2)
));

-- Test CG_NurbsCurveEvaluate at parameter
SELECT 'nurbs_evaluate_at_half', ST_GeometryType(CG_NurbsCurveEvaluate(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2),
    0.5
));

-- Test CG_NurbsCurveDerivative - first derivative (tangent)
SELECT 'nurbs_first_derivative', ST_GeometryType(CG_NurbsCurveDerivative(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2),
    0.5, 1
));

-- Test CG_NurbsCurveInterpolate through data points
SELECT 'nurbs_interpolate', ST_GeometryType(CG_NurbsCurveInterpolate(
    'LINESTRING(0 0, 2 5, 5 3, 8 1)'::geometry, 3
));

-- Test CG_NurbsCurveApproximate with tolerance
SELECT 'nurbs_approximate', ST_GeometryType(CG_NurbsCurveApproximate(
    'LINESTRING(0 0, 1 2, 2 3, 3 2, 4 1, 5 0)'::geometry, 3, 0.1
));

-- Test 3D NURBS curve support
SELECT 'nurbs_3d', ST_GeometryType(CG_NurbsCurveFromPoints(
    'LINESTRING Z (0 0 0, 5 10 5, 10 0 0)'::geometry, 2
));

-- Test 3D NURBS curve evaluation
SELECT 'nurbs_3d_evaluation', (ST_CoordDim(CG_NurbsCurveEvaluate(
    CG_NurbsCurveFromPoints('LINESTRING Z (0 0 0, 5 10 5, 10 0 0)'::geometry, 2),
    0.5
)) = 3) AS evaluation_preserves_3d;

-- Test interpolation with 3D points
SELECT 'nurbs_3d_interpolation', (ST_CoordDim(CG_NurbsCurveInterpolate(
    'LINESTRING Z (0 0 0, 2 5 2, 5 3 1, 8 1 0)'::geometry, 3
)) = 3) AS interpolation_supports_3d;

-- Test approximation with M coordinates
SELECT 'nurbs_with_m', ST_GeometryType(CG_NurbsCurveFromPoints(
    'LINESTRING M (0 0 1, 5 10 2, 10 0 3)'::geometry, 2
));

-- Error handling tests
-- Test error handling with invalid degree
SELECT 'error_invalid_degree', CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 0);

-- Test error handling with insufficient control points
SELECT 'error_insufficient_points', CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10)'::geometry, 3);

-- Test error handling with invalid geometry types for control points
SELECT 'error_invalid_geometry', CG_NurbsCurveFromPoints('POINT(0 0)'::geometry, 2);

-- Test error handling with invalid segment count
SELECT 'error_invalid_segments', CG_NurbsCurveToLineString(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2),
    1
);

-- Test derivative computation at boundary
SELECT 'nurbs_derivative_at_start', ST_GeometryType(CG_NurbsCurveDerivative(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 5, 10 0)'::geometry, 2),
    0.0, 1
));

-- Test higher order derivative
SELECT 'nurbs_second_derivative', ST_GeometryType(CG_NurbsCurveDerivative(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 2 8, 5 5, 8 2, 10 0)'::geometry, 3),
    0.5, 2
));

-- Test approximation with max control points limit
SELECT 'nurbs_approximate_limit', ST_GeometryType(CG_NurbsCurveApproximate(
    'LINESTRING(0 0, 1 1, 2 4, 3 9, 4 16, 5 25, 6 36, 7 49, 8 64, 9 81, 10 100)'::geometry,
    3, 1.0, 5
));

-- Test parameter validation for CG_NurbsCurveEvaluate
SELECT 'error_parameter_out_of_range', CG_NurbsCurveEvaluate(
    CG_NurbsCurveFromPoints('LINESTRING(0 0, 5 10, 10 0)'::geometry, 2),
    1.5
);

-- WKT NURBS Curve Tests - Test reading and manipulating NURBS curves from WKT format

-- Test complex NURBS curve with degree 3
SELECT 'wkt_nurbs_degree3_closed', ST_GeometryType(
    'NURBSCURVE (3, (0.0 2.0,2.0 4.0,4.0 4.0,5.0 2.0,5.0 0.0,2.5 -3.0,0.0 -5.0,-2.5 -3.0,-5.0 0.0,-5.0 2.0,-4.0 4.0,-2.0 4.0,0.0 2.0))'::geometry
);

-- Test converting complex NURBS curve to LineString
SELECT 'wkt_nurbs_to_linestring', ST_GeometryType(CG_NurbsCurveToLineString(
    'NURBSCURVE (3, (0.0 2.0,2.0 4.0,4.0 4.0,5.0 2.0,5.0 0.0,2.5 -3.0,0.0 -5.0,-2.5 -3.0,-5.0 0.0,-5.0 2.0,-4.0 4.0,-2.0 4.0,0.0 2.0))'::geometry,
    16
));

-- Test simple NURBS curve degree 2
SELECT 'wkt_nurbs_simple', ST_GeometryType(
    'NURBSCURVE (2, (0.00 0.00,5.00 10.00,10.00 0.00))'::geometry
);

-- Test NURBS curve with weights
SELECT 'wkt_nurbs_with_weights', ST_GeometryType(
    'NURBSCURVE (2, (0.00 0.00,5.00 10.00,10.00 0.00), (1.00,2.00,1.00))'::geometry
);

-- Test NURBS curve degree 2 with 4 control points
SELECT 'wkt_nurbs_4points', ST_GeometryType(
    'NURBSCURVE (2, (0.00 0.00,3.00 6.00,6.00 3.00,9.00 0.00))'::geometry
);

-- Test 3D NURBS curve with weights
SELECT 'wkt_nurbs_3d_weights', ST_GeometryType(
    'NURBSCURVE Z (2, (0.00 0.00 0.00,5.00 10.00 5.00,10.00 0.00 0.00), (1.00,2.00,1.00))'::geometry
);

-- Test quarter circle approximation using weighted NURBS
SELECT 'wkt_nurbs_quarter_circle', ST_GeometryType(
    'NURBSCURVE (2, (1.00 0.00,1.00 1.00,0.00 1.00), (1.00,0.71,1.00))'::geometry
);

-- Test higher degree NURBS curve (degree 3, multiple segments)
SELECT 'wkt_nurbs_wave', ST_GeometryType(
    'NURBSCURVE (3, (0.00 0.00,3.00 2.00,6.00 -2.00,9.00 0.00,12.00 2.00,15.00 -2.00,18.00 0.00))'::geometry
);

-- Test conversion of weighted NURBS to LineString
SELECT 'wkt_weighted_to_linestring', ST_GeometryType(CG_NurbsCurveToLineString(
    'NURBSCURVE (2, (0.00 0.00,5.00 10.00,10.00 0.00), (1.00,2.00,1.00))'::geometry,
    10
));

-- Test evaluation of point on NURBS curve from WKT
SELECT 'wkt_nurbs_evaluate', ST_GeometryType(CG_NurbsCurveEvaluate(
    'NURBSCURVE (2, (0.00 0.00,5.00 10.00,10.00 0.00))'::geometry,
    0.5
));

-- Test derivative computation on WKT NURBS curve
SELECT 'wkt_nurbs_derivative', ST_GeometryType(CG_NurbsCurveDerivative(
    'NURBSCURVE (2, (0.00 0.00,3.00 6.00,6.00 3.00,9.00 0.00))'::geometry,
    0.25, 1
));

-- Test 3D coordinate preservation
SELECT 'wkt_nurbs_3d_preserved', (ST_CoordDim(
    'NURBSCURVE Z (2, (0.00 0.00 0.00,5.00 10.00 5.00,10.00 0.00 0.00), (1.00,2.00,1.00))'::geometry
) = 3) AS preserves_3d_coordinates;

-- Additional tests for M and ZM dimensions

-- Test NURBS curve with M dimension (measure values)
SELECT 'wkt_nurbs_m_simple', ST_GeometryType(
    'NURBSCURVE M (2, (0 0 0, 5 10 1, 10 0 2))'::geometry
);

-- Test NURBS curve with ZM dimensions
SELECT 'wkt_nurbs_zm_simple', ST_GeometryType(
    'NURBSCURVE ZM (2, (0 0 0 0, 5 10 5 1, 10 0 0 2))'::geometry
);

-- Test NURBS curve with M dimension and weights
SELECT 'wkt_nurbs_m_weights', ST_GeometryType(
    'NURBSCURVE M (2, (0 0 0, 5 10 1, 10 0 2), (1, 2, 1))'::geometry
);

-- Test NURBS curve with ZM dimensions and weights
SELECT 'wkt_nurbs_zm_weights', ST_GeometryType(
    'NURBSCURVE ZM (2, (0 0 0 0, 5 10 5 1, 10 0 0 2), (1, 2, 1))'::geometry
);

-- Test NURBS curve with ZM dimensions, weights and knots
SELECT 'wkt_nurbs_zm_full', ST_GeometryType(
    'NURBSCURVE ZM (2, (0 0 0 0, 3 6 3 0.5, 6 3 3 1, 9 0 0 1.5), (1, 1, 1, 1), (0, 0, 0, 0.5, 1, 1, 1))'::geometry
);

-- Test dimension preservation for M
SELECT 'wkt_nurbs_m_preserved', (ST_NDims(
    'NURBSCURVE M (2, (0 0 0, 5 10 1, 10 0 2))'::geometry
) = 3) AS has_m_dimension;

-- Test dimension preservation for ZM
SELECT 'wkt_nurbs_zm_preserved', (ST_NDims(
    'NURBSCURVE ZM (2, (0 0 0 0, 5 10 5 1, 10 0 0 2))'::geometry
) = 4) AS has_zm_dimensions;

-- Negative test cases: These should fail parsing

-- Test fractional degree rejection
SELECT 'wkt_nurbs_fractional_degree_fail', ST_GeomFromText('NURBSCURVE(2.5, (0 0, 5 5, 10 0))') IS NULL AS should_be_null;

-- Test out-of-order knots rejection
SELECT 'wkt_nurbs_bad_knots_fail', ST_GeomFromText('NURBSCURVE(2, (0 0, 5 5, 10 0), (1, 1, 1), (0, 1, 0.5, 1))') IS NULL AS should_be_null;

-- Test incorrect boundary multiplicity rejection
SELECT 'wkt_nurbs_bad_multiplicity_fail', ST_GeomFromText('NURBSCURVE(2, (0 0, 5 5, 10 0), (1, 1, 1), (0, 0, 1, 1, 1, 1))') IS NULL AS should_be_null;
