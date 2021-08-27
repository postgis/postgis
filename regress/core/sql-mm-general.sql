SELECT 'arc1', ST_HasArc(ST_GeomFromText('POINT(0 0)'));
SELECT 'arc2', ST_HasArc(ST_GeomFromText('LINESTRING(0 0, 1 1, 1 0)'));
SELECT 'arc2a', ST_HasArc(ST_GeomFromText('COMPOUNDCURVE((0 0, 1 1, 1 0))'));
SELECT 'arc2b', ST_HasArc(ST_GeomFromText('COMPOUNDCURVE((0 0, 1 1, 1 0), CIRCULARSTRING(1 0, 1 1, 1 0))'));
SELECT 'arc3', ST_HasArc(ST_GeomFromEWKT('CIRCULARSTRING(
                0 0 0 0,
                0.26794919243112270647255365849413 1 3 -2,
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT 'arc4', ST_HasArc(ST_GeomFromEWKT('COMPOUNDCURVE(CIRCULARSTRING(
                0 0 0 0,
                0.26794919243112270647255365849413 1 3 -2,
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2),
                (0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2,
                2 0 0 0,
                0 0 0 0))'));
SELECT 'arc5', ST_HasArc(ST_GeomFromEWKT('POLYGON(
                (-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0))'));
SELECT 'arc6', ST_HasArc(ST_GeomFromEWKT('CURVEPOLYGON(CIRCULARSTRING(
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
SELECT 'arc7', ST_HasArc(ST_GeomFromEWKT('MULTIPOINT((0 0), (3 2))'));
SELECT 'arc8', ST_HasArc(ST_GeomFromEWKT('MULTILINESTRING(
                (0 0, 3 2),
                (4 8, 9 8),
                (2 9, 4 8))'));
SELECT 'arc9', ST_HasArc(ST_GeomFromEWKT('MULTICURVE(
                (0 0, 3 2),
                (4 8, 9 8),
                (2 9, 4 8))'));
SELECT 'arc10', ST_HasArc(ST_GeomFromEWKT('MULTICURVE((
                5 5 1 3,
                3 5 2 2,
                3 3 3 1,
                0 3 1 1)
                ,CIRCULARSTRING(
                0 0 0 0,
                0.26794919243112270647255365849413 1 3 -2,
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2))'));
SELECT 'arc11', ST_HasArc(ST_GeomFromEWKT('MULTIPOLYGON(
                ((-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0)),
                ((9 2, 3 8, 9 4, 9 2)))'));
SELECT 'arc12', ST_HasArc(ST_GeomFromEWKT('MULTISURFACE(
                ((-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0)),
                ((9 2, 3 8, 9 4, 9 2)))'));
SELECT 'arc13', ST_HasArc(ST_GeomFromEWKT('MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(
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
                -1 0 1 2)),
                ((7 8 7 8,
                10 10 5 5,
                6 14 3 1,
                4 11 4 6,
                7 8 7 8),
                (9 9 7 8,
                8 12 7 8,
                7 10 7 8,
                9 9 7 8)))'));
SELECT ST_AsEWKT(ST_LineToCurve('LINESTRING(
		-13151357.927248 3913656.64539871,
		-13151419.0845266 3913664.12016378,
		-13151441.323537 3913666.61175286,
		-13151456.8908442 3913666.61175286,
		-13151476.9059536 3913666.61175286,
		-13151496.921063 3913666.61175287,
		-13151521.3839744 3913666.61175287,
		-13151591.4368571 3913665.36595828)'));

SELECT 'pt1', ST_AsEWKT(ST_Point(1.0, 2.0));
SELECT 'pt2', ST_AsEWKT(ST_Point(1.0, 2.0, 3));
SELECT 'pt3', ST_AsEWKT(ST_PointZ(1.0, 2.0, 3.0));
SELECT 'pt4', ST_AsEWKT(ST_PointZ(1.0, 2.0, 3.0, 4));
SELECT 'pt5', ST_AsEWKT(ST_PointM(1.0, 2.0, 3.0));
SELECT 'pt6', ST_AsEWKT(ST_PointM(1.0, 2.0, 3.0, 4));
SELECT 'pt7', ST_AsEWKT(ST_PointZM(1.0, 2.0, 3.0, 4.0));
SELECT 'pt8', ST_AsEWKT(ST_PointZM(1.0, 2.0, 3.0, 4.0, 5));
