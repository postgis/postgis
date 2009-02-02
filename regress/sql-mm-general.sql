SELECT ST_HasArc(geomfromtext('POINT(0 0)'));
SELECT ST_HasArc(geomfromtext('LINESTRING(0 0, 1 1, 1 0)'));
SELECT ST_HasArc(geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT ST_HasArc(geomfromewkt('COMPOUNDCURVE(CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2),
                (0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2,
                2 0 0 0,
                0 0 0 0))'));
SELECT ST_HasArc(geomfromewkt('POLYGON(
                (-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0))'));
SELECT ST_HasArc(geomfromewkt('CURVEPOLYGON(CIRCULARSTRING(
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
SELECT ST_HasArc(geomfromewkt('MULTIPOINT((0 0), (3 2))'));
SELECT ST_HasArc(geomfromewkt('MULTILINESTRING(
                (0 0, 3 2),
                (4 8, 9 8),
                (2 9, 4 8))'));
SELECT ST_HasArc(geomfromewkt('MULTICURVE(
                (0 0, 3 2),
                (4 8, 9 8),
                (2 9, 4 8))'));
SELECT ST_HasArc(geomfromewkt('MULTICURVE((
                5 5 1 3,
                3 5 2 2,
                3 3 3 1,
                0 3 1 1)
                ,CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2))'));
SELECT ST_HasArc(geomfromewkt('MULTIPOLYGON(
                ((-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0)),
                ((9 2, 3 8, 9 4, 9 2)))'));
SELECT ST_HasArc(geomfromewkt('MULTISURFACE(
                ((-10 -10, 10 -10, 10 10, -10 10, -10 -10),
                (5 0, 0 5, -5 0, 0 -5, 5 0)),
                ((9 2, 3 8, 9 4, 9 2)))'));
SELECT ST_HasArc(geomfromewkt('MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(
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
