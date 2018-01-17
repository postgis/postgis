-- Point
SELECT 'pnt1', ST_AsText(ST_Reverse('POINT EMPTY'::geometry));
SELECT 'pnt2', ST_AsText(ST_Reverse('POINT(0 0)'::geometry));

-- Multipoint
SELECT 'mpnt1', ST_AsText(ST_Reverse('MULTIPOINT EMPTY'::geometry));
SELECT 'mpnt2', ST_AsText(ST_Reverse('MULTIPOINT(0 0)'::geometry));
SELECT 'mpnt3', ST_AsText(ST_Reverse('MULTIPOINT(0 0,1 1)'::geometry));

-- Linestring
SELECT 'lin1', ST_AsText(ST_Reverse('LINESTRING EMPTY'::geometry));
SELECT 'lin2', ST_AsText(ST_Reverse('LINESTRING(0 0, 1 1)'::geometry));
SELECT 'lin2m', ST_AsText(ST_Reverse('LINESTRING M (0 0 3,1 1 6)'::geometry));
SELECT 'lin2z', ST_AsText(ST_Reverse('LINESTRING Z (0 0 3,1 1 6)'::geometry));

-- MultiLinestring
SELECT 'mlin1', ST_AsText(ST_Reverse('MULTILINESTRING EMPTY'::geometry));
SELECT 'mlin2', ST_AsText(ST_Reverse('MULTILINESTRING((0 0, 1 0),(2 0, -3 3))'::geometry));
SELECT 'mlin2m', ST_AsText(ST_Reverse('MULTILINESTRING M ((0 0 5, 1 0 3),(2 0 2, -3 3 1))'::geometry));
SELECT 'mlin2z', ST_AsText(ST_Reverse('MULTILINESTRING Z ((0 0 5, 1 0 3),(2 0 2, -3 3 1))'::geometry));

-- Polygon
SELECT 'plg1', ST_AsText(ST_Reverse('POLYGON EMPTY'::geometry));
SELECT 'plg2', ST_AsText(ST_Reverse('POLYGON((0 0,8 0,8 8,0 8,0 0),(2 2,2 4,4 4,4 2,2 2))'::geometry));
SELECT 'plg2m', ST_AsText(ST_Reverse('POLYGON M ((0 0 9,8 0 8,8 8 7,0 8 8,0 0 9),(2 2 1,2 4 2,4 4 3,4 2 2,2 2 1))'::geometry));
SELECT 'plg2z', ST_AsText(ST_Reverse('POLYGON Z ((0 0 9,8 0 8,8 8 7,0 8 8,0 0 9),(2 2 1,2 4 2,4 4 3,4 2 2,2 2 1))'::geometry));

-- MultiPolygon
SELECT 'mplg1', ST_AsText(ST_Reverse('MULTIPOLYGON EMPTY'::geometry));
SELECT 'mplg2', ST_AsText(ST_Reverse('MULTIPOLYGON(((0 0,5 0,5 5,0 5,0 0),(2 2,2 4,4 4,4 2,2 2)),((6 6,7 6,7 7,6 6)))'::geometry));
SELECT 'mplg2m', ST_AsText(ST_Reverse('MULTIPOLYGON M (((0 0 1,5 0 2,5 5 3,0 5 4,0 0 1),(2 2 0,2 4 1,4 4 2,4 2 3,2 2 0)),((6 6 2,7 6 4,7 7 6,6 6 2)))'::geometry));
SELECT 'mplg2z', ST_AsText(ST_Reverse('MULTIPOLYGON Z (((0 0 1,5 0 2,5 5 3,0 5 4,0 0 1),(2 2 0,2 4 1,4 4 2,4 2 3,2 2 0)),((6 6 2,7 6 4,7 7 6,6 6 2)))'::geometry));

-- CircularString
SELECT 'cstr1', ST_AsText(ST_Reverse('CIRCULARSTRING EMPTY'::geometry));
SELECT 'cstr2', ST_AsText(ST_Reverse('CIRCULARSTRING(0 0,1 1,2 0,3 -1,4 0)'::geometry));
SELECT 'cstr2m', ST_AsText(ST_Reverse('CIRCULARSTRING M (0 0 1,1 1 2,2 0 3,3 -1 4,4 0 5)'::geometry));
SELECT 'cstr2z', ST_AsText(ST_Reverse('CIRCULARSTRING Z (0 0 1,1 1 2,2 0 3,3 -1 4,4 0 5)'::geometry));

-- CompoundCurve
SELECT 'ccrv1', ST_AsText(ST_Reverse('COMPOUNDCURVE EMPTY'::geometry));
SELECT 'ccrv2', ST_AsText(ST_Reverse('COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,0 1))'::geometry));
SELECT 'ccrv2m', ST_AsText(ST_Reverse('COMPOUNDCURVE M (CIRCULARSTRING(0 0 3,1 1 2,1 0 1),(1 0 6,0 1 7))'::geometry));
SELECT 'ccrv2z', ST_AsText(ST_Reverse('COMPOUNDCURVE Z (CIRCULARSTRING(0 0 3,1 1 2,1 0 1),(1 0 6,0 1 7))'::geometry));

-- CurvePolygon
SELECT 'cplg1', ST_AsText(ST_Reverse('CURVEPOLYGON EMPTY'::geometry));
SELECT 'cplg2', ST_AsText(ST_Reverse('CURVEPOLYGON (COMPOUNDCURVE (CIRCULARSTRING (0 0,1 1,1 0),(1 0,0 1),(0 1,0 0)))'::geometry));
SELECT 'cplg2m', ST_AsText(ST_Reverse('CURVEPOLYGON M (COMPOUNDCURVE (CIRCULARSTRING (0 0 2,1 1 2,1 0 2),(1 0 2,0 1 2),(0 1 2, 0 0 2)))'::geometry));
SELECT 'cplg2z', ST_AsText(ST_Reverse('CURVEPOLYGON Z (COMPOUNDCURVE (CIRCULARSTRING (0 0 2,1 1 2,1 0 2),(1 0 2,0 1 2),(0 1 2, 0 0 2)))'::geometry));

-- MultiCurve
SELECT 'mc1', ST_AsText(ST_Reverse('MULTICURVE EMPTY'::geometry));
SELECT 'mc2', ST_AsText(ST_Reverse('MULTICURVE ((5 5, 3 5, 3 3, 0 3), CIRCULARSTRING (0 0, 0.2 1, 0.5 1.4), COMPOUNDCURVE (CIRCULARSTRING (0 0,1 1,1 0),(1 0,0 1)))'::geometry));
SELECT 'mc2zm', ST_AsText(ST_Reverse('MULTICURVE ZM ((5 5 0 1, 3 5 2 3, 3 3 7 5, 0 3 9 8), CIRCULARSTRING (0 0 1 3, 0.2 1 4 2, 0.5 1.4 5 1), COMPOUNDCURVE (CIRCULARSTRING (0 0 -1 -2,1 1 0 4,1 0 5 6),(1 0 4 2,0 1 3 4)))'::geometry));

-- MultiSurface
SELECT 'ms1', ST_AsText(ST_Reverse('MULTISURFACE EMPTY'::geometry));
SELECT 'ms2', ST_AsText(ST_Reverse('MULTISURFACE (CURVEPOLYGON (CIRCULARSTRING (-2 0, -1 -1, 0 0, 1 -1, 2 0, 0 2, -2 0), (-1 0, 0 0.5, 1 0, 0 1, -1 0)), ((7 8, 10 10, 6 14, 4 11, 7 8)))'::geometry));
SELECT 'ms2m', ST_AsText(ST_Reverse(
'MULTISURFACE M (CURVEPOLYGON M (CIRCULARSTRING M (-2 0 0, -1 -1 1, 0 0 2, 1 -1 3, 2 0 4, 0 2 2, -2 0 0), (-1 0 1, 0 0.5 2, 1 0 3, 0 1 3, -1 0 1)), ((7 8 7, 10 10 5, 6 14 3, 4 11 4, 7 8 7)))'
::geometry));
SELECT 'ms2z', ST_AsText(ST_Reverse(
'MULTISURFACE Z (CURVEPOLYGON Z (CIRCULARSTRING Z (-2 0 0, -1 -1 1, 0 0 2, 1 -1 3, 2 0 4, 0 2 2, -2 0 0), (-1 0 1, 0 0.5 2, 1 0 3, 0 1 3, -1 0 1)), ((7 8 7, 10 10 5, 6 14 3, 4 11 4, 7 8 7)))'
::geometry));

-- PolyedralSurface (TODO)
SELECT 'ps1', ST_AsText(ST_Reverse(
'POLYHEDRALSURFACE EMPTY'
::geometry));
SELECT 'ps2', ST_AsText(ST_Reverse(
'POLYHEDRALSURFACE (((0 0,0 0,0 1,0 0)),((0 0,0 1,1 0,0 0)),((0 0,1 0,0 0,0 0)),((1 0,0 1,0 0,1 0)))'
::geometry));

-- Triangle
SELECT 'tri1', ST_AsText(ST_Reverse(
'TRIANGLE EMPTY'
::geometry));
SELECT 'tri2', ST_AsText(ST_Reverse(
'TRIANGLE ((1 2,4 5,7 8,1 2))'
::geometry));
SELECT 'tri2z', ST_AsText(ST_Reverse(
'TRIANGLE Z ((1 2 3,4 5 6,7 8 9,1 2 3))'
::geometry));

-- TIN
SELECT 'tin1', ST_AsText(ST_Reverse( 'TIN EMPTY' ::geometry));
SELECT 'tin2', ST_AsText(ST_Reverse(
'TIN ( ((0 0, 0 0, 0 1, 0 0)), ((0 0, 0 1, 1 1, 0 0)) )'
::geometry));

-- GeometryCollection
SELECT 'gc1', ST_AsText(ST_Reverse(
'GEOMETRYCOLLECTION EMPTY'
::geometry));
SELECT 'gc2', ST_AsText(ST_Reverse(
'GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2))),POINT(0 0),MULTILINESTRING((0 0, 2 0),(1 1, 2 2)))'
::geometry));
