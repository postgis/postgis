SELECT 'ndims01', ndims(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0 0,
                -1 -1 1 2,
                0 0 2 4,
                1 -1 3 6,
                2 0 4 8,
                0 2 2 4,
                -2 0 0 0),
                (-1 0 1 2,
                0 0.5 2 4,
                1 0 3 6,
                0 1 3 4,
                -1 0 1 2))'));
SELECT 'geometrytype01', geometrytype(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0 0,
                -1 -1 1 2,
                0 0 2 4,
                1 -1 3 6,
                2 0 4 8,
                0 2 2 4,
                -2 0 0 0),
                (-1 0 1 2,
                0 0.5 2 4,
                1 0 3 6,
                0 1 3 4,
                -1 0 1 2))'));
SELECT 'ndims02', ndims(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0,
                -1 -1 1,
                0 0 2,
                1 -1 3,
                2 0 4,
                0 2 2,
                -2 0 0),
                (-1 0 1,
                0 0.5 2,
                1 0 3,
                0 1 3,
                -1 0 1))'));
SELECT 'geometrytype02', geometrytype(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0,
                -1 -1 1,
                0 0 2,
                1 -1 3,
                2 0 4,
                0 2 2,
                -2 0 0),
                (-1 0 1,
                0 0.5 2,
                1 0 3,
                0 1 3,
                -1 0 1))'));
SELECT 'ndims03', ndims(geomfromewkt('CURVEPOLYGONM(CIRCULARSTRING(
                -2 0 0,
                -1 -1 2,
                0 0 4,
                1 -1 6,
                2 0 8,
                0 2 4,
                -2 0 0),
                (-1 0 2,
                0 0.5 4,
                1 0 6,
                0 1 4,
                -1 0 2))'));
SELECT 'geometrytype03', geometrytype(geomfromewkt('CURVEPOLYGONM(CIRCULARSTRING(
                -2 0 0,
                -1 -1 2,
                0 0 4,
                1 -1 6,
                2 0 8,
                0 2 4,
                -2 0 0),
                (-1 0 2,
                0 0.5 4,
                1 0 6,
                0 1 4,
                -1 0 2))'));
SELECT 'ndims04', ndims(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0,
                -1 -1,
                0 0,
                1 -1,
                2 0,
                0 2,
                -2 0),
                (-1 0,
                0 0.5,
                1 0,
                0 1,
                -1 0))'));
SELECT 'geometrytype04', geometrytype(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0,
                -1 -1,
                0 0,
                1 -1,
                2 0,
                0 2,
                -2 0),
                (-1 0,
                0 0.5,
                1 0,
                0 1,
                -1 0))'));

CREATE TABLE public.curvepolygon (id INTEGER, description VARCHAR);
SELECT AddGeometryColumn('public', 'curvepolygon', 'the_geom_2d', -1, 'CURVEPOLYGON', 2);
SELECT AddGeometryColumn('public', 'curvepolygon', 'the_geom_3dm', -1, 'CURVEPOLYGONM', 3);
SELECT AddGeometryColumn('public', 'curvepolygon', 'the_geom_3dz', -1, 'CURVEPOLYGON', 3);
SELECT AddGeometryColumn('public', 'curvepolygon', 'the_geom_4d', -1, 'CURVEPOLYGON', 4);

INSERT INTO public.curvepolygon (
                id,
                description
              ) VALUES (
                1, 'curvepolygon');
UPDATE public.curvepolygon
        SET the_geom_4d = geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0 0,
                -1 -1 1 2,
                0 0 2 4,
                1 -1 3 6,
                2 0 4 8,
                0 2 2 4,
                -2 0 0 0),
                (-1 0 1 2,
                0 0.5 2 4,
                1 0 3 6,
                0 1 3 4,
                -1 0 1 2))');
UPDATE public.curvepolygon
        SET the_geom_3dz = geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0 0,
                -1 -1 1,
                0 0 2,
                1 -1 3,
                2 0 4,
                0 2 2,
                -2 0 0),
                (-1 0 1,
                0 0.5 2,
                1 0 3,
                0 1 3,
                -1 0 1))');
UPDATE public.curvepolygon
        SET the_geom_3dm = geomfromewkt('CURVEPOLYGONM(CIRCULARSTRING(
                -2 0 0,
                -1 -1 2,
                0 0 4,
                1 -1 6,
                2 0 8,
                0 2 4,
                -2 0 0),
                (-1 0 2,
                0 0.5 4,
                1 0 6,
                0 1 4,
                -1 0 2))');
UPDATE public.curvepolygon
        SET the_geom_2d = geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
                -2 0,
                -1 -1,
                0 0,
                1 -1,
                2 0,
                0 2,
                -2 0),
                (-1 0,
                0 0.5,
                1 0,
                0 1,
                -1 0))');

SELECT 'astext01', astext(the_geom_2d) FROM public.curvepolygon;
SELECT 'astext02', astext(the_geom_3dm) FROM public.curvepolygon;
SELECT 'astext03', astext(the_geom_3dz) FROM public.curvepolygon;
SELECT 'astext04', astext(the_geom_4d) FROM public.curvepolygon;

SELECT 'asewkt01', asewkt(the_geom_2d) FROM public.curvepolygon;
SELECT 'asewkt02', asewkt(the_geom_3dm) FROM public.curvepolygon;
SELECT 'asewkt03', asewkt(the_geom_3dz) FROM public.curvepolygon;
SELECT 'asewkt04', asewkt(the_geom_4d) FROM public.curvepolygon;

SELECT 'asbinary01', encode(asbinary(the_geom_2d), 'hex') FROM public.curvepolygon;
SELECT 'asbinary02', encode(asbinary(the_geom_3dm), 'hex') FROM public.curvepolygon;
SELECT 'asbinary03', encode(asbinary(the_geom_3dz), 'hex') FROM public.curvepolygon;
SELECT 'asbinary04', encode(asbinary(the_geom_4d), 'hex') FROM public.curvepolygon;

SELECT 'asewkb01', encode(asewkb(the_geom_2d), 'hex') FROM public.curvepolygon;
SELECT 'asewkb02', encode(asewkb(the_geom_3dm), 'hex') FROM public.curvepolygon;
SELECT 'asewkb03', encode(asewkb(the_geom_3dz), 'hex') FROM public.curvepolygon;
SELECT 'asewkb04', encode(asewkb(the_geom_4d), 'hex') FROM public.curvepolygon;

SELECT 'ST_CurveToLine-201', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-202', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-203', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-204', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;

SELECT 'ST_CurveToLine-401', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-402', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-403', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine-404', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;

SELECT 'ST_CurveToLine01', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine02', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine03', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'ST_CurveToLine04', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;

-- Removed due to descrepencies between hardware
--SELECT 'box2d01', box2d(the_geom_2d) FROM public.curvepolygon;
--SELECT 'box2d02', box2d(the_geom_3dm) FROM public.curvepolygon;
--SELECT 'box2d03', box2d(the_geom_3dz) FROM public.curvepolygon;
--SELECT 'box2d04', box2d(the_geom_4d) FROM public.curvepolygon;

--SELECT 'box3d01', box3d(the_geom_2d) FROM public.curvepolygon;
--SELECT 'box3d02', box3d(the_geom_3dm) FROM public.curvepolygon;
--SELECT 'box3d03', box3d(the_geom_3dz) FROM public.curvepolygon;
--SELECT 'box3d04', box3d(the_geom_4d) FROM public.curvepolygon;

SELECT 'isValid01', isValid(the_geom_2d) FROM public.curvepolygon;
SELECT 'isValid02', isValid(the_geom_3dm) FROM public.curvepolygon;
SELECT 'isValid03', isValid(the_geom_3dz) FROM public.curvepolygon;
SELECT 'isValid04', isValid(the_geom_4d) FROM public.curvepolygon;

SELECT 'dimension01', dimension(the_geom_2d) FROM public.curvepolygon;
SELECT 'dimension02', dimension(the_geom_3dm) FROM public.curvepolygon;
SELECT 'dimension03', dimension(the_geom_3dz) FROM public.curvepolygon;
SELECT 'dimension04', dimension(the_geom_4d) FROM public.curvepolygon;

SELECT 'SRID01', SRID(the_geom_2d) FROM public.curvepolygon;
SELECT 'SRID02', SRID(the_geom_3dm) FROM public.curvepolygon;
SELECT 'SRID03', SRID(the_geom_3dz) FROM public.curvepolygon;
SELECT 'SRID04', SRID(the_geom_4d) FROM public.curvepolygon;

SELECT 'envelope01', asText(snapToGrid(envelope(the_geom_2d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'envelope02', asText(snapToGrid(envelope(the_geom_3dm), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'envelope03', asText(snapToGrid(envelope(the_geom_3dz), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;
SELECT 'envelope04', asText(snapToGrid(envelope(the_geom_4d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.curvepolygon;

SELECT 'startPoint01', (startPoint(the_geom_2d) is null) FROM public.curvepolygon;
SELECT 'startPoint02', (startPoint(the_geom_3dm) is null) FROM public.curvepolygon;
SELECT 'startPoint03', (startPoint(the_geom_3dz) is null) FROM public.curvepolygon;
SELECT 'startPoint04', (startPoint(the_geom_4d) is null) FROM public.curvepolygon;

SELECT 'endPoint01', (endPoint(the_geom_2d) is null) FROM public.curvepolygon;
SELECT 'endPoint02', (endPoint(the_geom_3dm) is null) FROM public.curvepolygon;
SELECT 'endPoint03', (endPoint(the_geom_3dz) is null) FROM public.curvepolygon;
SELECT 'endPoint04', (endPoint(the_geom_4d) is null) FROM public.curvepolygon;

SELECT 'exteriorRing01', asEWKT(exteriorRing(the_geom_2d)) FROM public.curvepolygon;
SELECT 'exteriorRing02', asEWKT(exteriorRing(the_geom_3dm)) FROM public.curvepolygon;
SELECT 'exteriorRing03', asEWKT(exteriorRing(the_geom_3dz)) FROM public.curvepolygon;
SELECT 'exteriorRing04', asEWKT(exteriorRing(the_geom_4d)) FROM public.curvepolygon;

SELECT 'numInteriorRings01', numInteriorRings(the_geom_2d) FROM public.curvepolygon;
SELECT 'numInteriorRings02', numInteriorRings(the_geom_3dm) FROM public.curvepolygon;
SELECT 'numInteriorRings03', numInteriorRings(the_geom_3dz) FROM public.curvepolygon;
SELECT 'numInteriorRings04', numInteriorRings(the_geom_4d) FROM public.curvepolygon;

SELECT 'interiorRingN-101', asEWKT(interiorRingN(the_geom_2d, 1)) FROM public.curvepolygon;
SELECT 'interiorRingN-102', asEWKT(interiorRingN(the_geom_3dm, 1)) FROM public.curvepolygon;
SELECT 'interiorRingN-103', asEWKT(interiorRingN(the_geom_3dz, 1)) FROM public.curvepolygon;
SELECT 'interiorRingN-104', asEWKT(interiorRingN(the_geom_4d, 1)) FROM public.curvepolygon;

SELECT 'interiorRingN-201', asEWKT(interiorRingN(the_geom_2d, 2)) FROM public.curvepolygon;
SELECT 'interiorRingN-202', asEWKT(interiorRingN(the_geom_3dm, 2)) FROM public.curvepolygon;
SELECT 'interiorRingN-203', asEWKT(interiorRingN(the_geom_3dz, 2)) FROM public.curvepolygon;
SELECT 'interiorRingN-204', asEWKT(interiorRingN(the_geom_4d, 2)) FROM public.curvepolygon;

SELECT 'ST_LineToCurve01', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_2d))) FROM public.curvepolygon;
SELECT 'ST_LineToCurve02', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_3dm))) FROM public.curvepolygon;
SELECT 'ST_LineToCurve03', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_3dz))) FROM public.curvepolygon;
SELECT 'ST_LineToCurve04', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_4d))) FROM public.curvepolygon;

SELECT DropGeometryColumn('public', 'curvepolygon', 'the_geom_2d');
SELECT DropGeometryColumn('public', 'curvepolygon', 'the_geom_3dm');
SELECT DropGeometryColumn('public', 'curvepolygon', 'the_geom_3dz');
SELECT DropGeometryColumn('public', 'curvepolygon', 'the_geom_4d');
DROP TABLE public.curvepolygon;
