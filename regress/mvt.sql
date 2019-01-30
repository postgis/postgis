-- geometry preprocessing tests
select 'PG1', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));
select 'PG2', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096*2, 4096*2)),
	4096, 0, false));
select 'PG3', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096/2, 4096/2)),
	4096, 0, false));
select 'PG4', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));
select 'PG5', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 5, 0 -5, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096*4096, 4096*4096)),
	4096, 0, false));
select 'PG6', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((1052826 6797045, 762780 6474467, 717821 6797045, 1052826 6797045))'),
	ST_MakeBox2D(ST_Point(626172.135625, 6261721.35625), ST_Point(1252344.27125, 6887893.49188)),
	4096, 0, false));

SELECT 'PG7', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-7792023.4539488 1411512.60791779,-7785283.40665468 1406282.69482469,-7783978.88137195 1404858.20373788,-7782986.89858399 1402324.91434802,-7779028.02672366 1397370.31802772,
	-7778652.06985644 1394387.75452545,-7779906.76953697 1393279.22658385,-7782212.33678782 1393293.14086794,-7784631.14401331 1394225.4151684,-7786257.27108231 1395867.40241344,-7783978.88137195 1395867.40241344,
	-7783978.88137195 1396646.68250521,-7787752.03959369 1398469.72134299,-7795443.30325373 1405280.43988858,-7797717.16326269 1406217.73286975,-7798831.44531677 1406904.48130551,-7799311.5830898 1408004.24038921,
	-7799085.10302919 1409159.72782477,-7798052.35381919 1411108.84582812,-7797789.63692662 1412213.40365339,-7798224.47868753 1414069.89725829,-7799003.5701851 1415694.42577482,-7799166.63587328 1416966.26267896,
	-7797789.63692662 1417736.81850415,-7793160.38395328 1412417.61222784,-7792023.4539488 1411512.60791779))'),
	ST_MakeBox2D(ST_Point(-20037508.34, -20037508.34), ST_Point(20037508.34, 20037508.34)),
	4096, 10, true)) as g;

select 'PG8', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(MULTIPOLYGON (((0 0, 10 0, 10 5, 0 -5, 0 0))))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));
select 'PG9', ST_Area(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON ((0 0, 10 0, 10 10, 0 10, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(5, 5)),
	4096, 0, true));

-- There shoulnd't be floating point values
WITH geometry AS
(
	SELECT ST_AsMVTGeom(
		ST_GeomFromText('POLYGON((5 0, 0 5, 0 0, 5 5, 5 0))'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(5, 5)),
		5, 0, true) as g
)
SELECT  'PG9.1', ST_NumGeometries(g), ST_Area(g), ST_AsText(g) LIKE '%2.5%'as fvalue FROM geometry;
SELECT 'PG10', ST_AsText(ST_AsMVTGeom(
	'POINT EMPTY'::geometry,
	'BOX(0 0,2 2)'::box2d));

-- Clockwise Polygon
SELECT 'PG11', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((10 10, 10 0, 0 0, 0 10, 10 10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Same as PG11 but CCW
SELECT 'PG12', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

--- POLYGONS WITH INTERIOR RINGS
-- Input: Exterior CW, interior CW
-- Output: CW, CCW
SELECT 'PG13', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((10 10, 10 0, 0 0, 0 10, 10 10), (9 9, 9 1, 1 1, 1 9, 9 9))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Input: Exterior CW, interior CCW
-- Output: CW, CCW
SELECT 'PG14', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((10 10, 10 0, 0 0, 0 10, 10 10), (1 1, 9 1, 9 9, 1 9, 1 1))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Input: Exterior CCW, interior CW
-- Output: CW, CCW
SELECT 'PG15', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0), (9 9, 9 1, 1 1, 1 9, 9 9))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Input: Exterior CCW, interior CW
-- Output: CW, CCW
SELECT 'PG16', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0), (9 9, 9 1, 1 1, 1 9, 9 9))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Input: CW, CW, CW, CW
-- Output: CW, CCW, CW, CCW
SELECT 'PG17', ST_Area(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON(
		(10 10, 10 0, 0 0, 0 10, 10 10),
		(9 9, 1 9, 1 1, 9 1, 9 9),
		(8 8, 8 2, 2 2, 2 8, 8 8),
		(7 7, 7 3, 3 3, 3 7, 7 7))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Multipoint
SELECT 'PG18', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTIPOINT(1 1, 3 2)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

SELECT 'PG19', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTIPOINT(25 17, 26 18)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));

SELECT 'PG20', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTIPOINT(10 10, 10 10)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));

-- Linestring
SELECT 'PG21', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(1 1, 5 5)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

SELECT 'PG22', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(1 9, 1.01 9.01)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

-- Multiline
SELECT 'PG23', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTILINESTRING((1 1, 5 5),(2 8, 5 5))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

SELECT 'PG24', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTILINESTRING((1 1, 5 5),(1 1, 5 5))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 0, false));

SELECT 'PG25', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('MULTILINESTRING((1 1, 501 501, 1001 1001),(2 2, 502 502, 1002 1002))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0, false));

-- Clipping right in the borders
SELECT 'PG26', ST_AsText(ST_AsMVTGeom(
	ST_Point(-1, -1),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG27', ST_AsText(ST_AsMVTGeom(
	ST_Point(-1, 11),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG28', ST_AsText(ST_AsMVTGeom(
	ST_Point(11, -1),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG29', ST_AsText(ST_AsMVTGeom(
	ST_Point(11, 11),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG30', ST_AsText(ST_AsMVTGeom(
	ST_Point(11.5, 11.5),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG31', ST_AsText(ST_AsMVTGeom(
	ST_Point(11.49, 11.49),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG32', ST_AsText(ST_AsMVTGeom(
	ST_Point(-1.5, -1.5),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG33', ST_AsText(ST_AsMVTGeom(
	ST_Point(-1.49, -1.49),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG34', ST_AsText(ST_AsMVTGeom(
	ST_Point(-1.5, 11.5),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(10, 10)),
	10, 1, true));

SELECT 'PG35', ST_AsText(ST_AsMVTGeom(
	ST_Point(4352.49, -256.49),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 256, true));

SELECT 'PG36', ST_AsText(ST_AsMVTGeom(
	ST_Point(4352.49, -256.51),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 256, true));

SELECT 'PG37', ST_AsText(ST_AsMVTGeom(
	ST_Point(4352.51, -256.49),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 256, true));

SELECT 'PG38', ST_AsText(ST_AsMVTGeom(
	ST_Point(4352.51, -256.51),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 256, true));

SELECT 'PG39 - ON ', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((100 100, 100 90, 94 90, 94 96, 90 96, 90 80, 100 80, 100 0, 0 0, 0 100, 100 100))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, true));

SELECT 'PG39 - OFF', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((100 100, 100 90, 94 90, 94 96, 90 96, 90 80, 100 80, 100 0, 0 0, 0 100, 100 100))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, false));

-- Clipping isn't done since all points fall into the tile after grid
SELECT 'PG40 - ON ', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, true));

SELECT 'PG40 - OFF', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, false));

-- Clipping isn't done since all points fall into the tile after grid
SELECT 'PG41 - ON ', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100, 10 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, true));

SELECT 'PG41 - OFF', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100, 10 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, false));

SELECT 'PG42 - ON ', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100, 11 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, true));

SELECT 'PG42 - OFF', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('LINESTRING(0 0, 2 20, -2 40, -4 60, 4 80, 0 100, 11 100)'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, false));

-- Invalid polygon (intersection)
SELECT 'PG43 - ON ', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-10 -10, 110 110, -10 110, 110 -10, -10 -10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, true));

SELECT 'PG43 - OFF', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-10 -10, 110 110, -10 110, 110 -10, -10 -10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	10, 0, false));

-- Geometry type change
SELECT 'PG44', ST_AsEWKT(ST_AsMVTGeom(
	'SRID=3857;MULTIPOLYGON(((-8238038.43842083 4974073.00356281,-8238058.59985694 4974035.91194892,-8238046.74211362 4974077.68076013,-8238038.43842083 4974073.00356281)))'::geometry,
	'SRID=3857;POLYGON((-8242969.13027341 4975133.29702555,-8242969.13027341 4970241.3272153,-8238077.16046316 4970241.3272153,-8238077.16046316 4975133.29702555,-8242969.13027341 4975133.29702555))'::geometry,
	4096,
	16,
	true));

-- Invalid geometry after simplification with invalid clipping
SELECT 'PG45', ST_AsEWKT(ST_AsMVTGeom(
	'SRID=3857;MULTIPOLYGON(((-8231365.02893734 4980355.83678553,-8231394.82332406 4980186.31880185,-8231367.43081065 4979982.17443372,-8231396.69199339 4980227.59327083,-8231365.02893734 4980355.83678553)))'::geometry,
	'SRID=3857;POLYGON((-8238115.3789773 4970203.10870116,-8238115.3789773 4980063.48534995,-8228255.00232851 4980063.48534995,-8228255.00232851 4970203.10870116,-8238115.3789773 4970203.10870116))'::geometry,
	4096,
	16,
	true));

-- Geometry type change of one geometry of the multipolygon used to fallback to multilinestring
SELECT 'PG46', St_AsEWKT(ST_AsMVTGeom(
	'SRID=3857;MULTIPOLYGON(((-8230285.21085987 4984959.60349704,-8230324.85567616 4984496.35685962,-8230307.1114228 4984654.46474466,-8230285.21085987 4984959.60349704)),((-8230327.54013683 4984444.33052449,-8230327.23971431 4984450.39401942,-8230327.26833036 4984449.87731981,-8230327.54013683 4984444.33052449)))'::geometry,
	'SRID=3857;POLYGON((-8238077.16046316 4989809.20645631,-8238077.16046316 4980025.2668358,-8228293.22084265 4980025.2668358,-8228293.22084265 4989809.20645631,-8238077.16046316 4989809.20645631))'::geometry,
	4096,
	16,
	true));

-- Check polygon clipping
--- Outside the tile
SELECT 'PG47', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-10 -10, -10 -5, -5 -5, -5 -10, -10 -10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Outside the tile
SELECT 'PG48', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((10 -10, 10 -5, 5 -5, 5 -10, 10 -10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Outside the tile
SELECT 'PG49', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((110 110, 110 105, 105 105, 105 110, 110 110))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Outside the tile
SELECT 'PG50', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((10 -5, 10 0, 5 0, 5 -5, 10 -5))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Fully covers the tile
SELECT 'PG51', ST_Area(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-10 110, -10 -10, 110 -10, 110 110, -10 110))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Partially in the tile
SELECT 'PG52', ST_Area(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((20 -10, 110 -10, 110 110, 20 110, 20 -10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

--- Partially in the tile
SELECT 'PG53', ST_Area(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((-20 10, 20 10, 20 40, -20 40, -20 10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

-- Simplification
SELECT 'PG54', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 10, 100 10, 100 10.3, 0 10))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG55', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 99.9, 99.9 99.9, 99.9 150, 0 150, 0 99.9))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG56', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 0, 99.6 100, 100 99.6, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

-- This clips in float
SELECT 'PG57', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('POLYGON((0 0, 0 99, 1 101, 100 100, 100 0, 0 0))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

-- Geometrycollection test
SELECT 'PG58', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0))), POINT(50 50))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG59', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(POINT(50 50), LINESTRING(10 10, 20 20), MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0))))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG60', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(POINT(50 50), GEOMETRYCOLLECTION(POINT(50 50), MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG61', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(POINT(50 50), MULTIPOLYGON(((100 100, 110 100, 110 110, 100 110, 100 100))))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG62', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(10 10, 20 20), POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)), LINESTRING(20 20, 15 15))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG63', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(LINESTRING(10 10, 20 20), POLYGON((90 90, 110 90, 110 110, 90 110, 90 90)), LINESTRING(20 20, 15 15))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

SELECT 'PG64', ST_AsText(ST_AsMVTGeom(
	ST_GeomFromText('GEOMETRYCOLLECTION(MULTIPOLYGON EMPTY, POINT(50 50))'),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(100, 100)),
	100, 0, true));

-- geometry encoding tests
SELECT 'TG1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG2', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, 26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG3', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 1000 1000)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG4', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('LINESTRING(0 0, 500 500, 1000 1000)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG5', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('MULTILINESTRING((1 1, 501 501, 1001 1001),(2 2, 502 502, 1002 1002))'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG6', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POLYGON ((45 45, 15 40, 10 20, 35 10, 45 45), (35 35, 30 20, 20 30, 35 35))'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG7', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('MULTIPOLYGON(((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (20 25, 30 20, 20 15, 20 25)), ((20 45, 45 30, 40 40, 20 45)))'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG8', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TG9', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1,
	ST_AsMVTGeom(ST_GeomFromText('MULTIPOINT(25 17, -26 -18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;

-- attribute encoding tests
SELECT 'TA1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA2', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1.1::double precision AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA3', encode(ST_AsMVT(q, 'test',  4096, 'geom'), 'base64') FROM (SELECT NULL::integer AS c1,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA4', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 1 AS c1, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 2 AS c1, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom ORDER BY c1) AS q;
SELECT 'TA5', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom, 1 AS c1, 'abcd'::text AS c2) AS q;
SELECT 'TA6', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, -1 AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'TA7', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 'test' AS c1, 1 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 'test' AS c1, 2 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 'othertest' AS c1, 3 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom ORDER BY c2) AS q;
SELECT 'TA8', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 1::int AS c1, 1 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 1::int AS c1, 2 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 2::int AS c1, 3 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom ORDER BY c2) AS q;
SELECT 'TA9', length(ST_AsMVT(q))
FROM (
	SELECT 1 AS c1, -1 AS c2,
	ST_AsMVTGeom(
		'POINT(25 17)'::geometry,
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4, 4))
	) AS geom
) AS q;
SELECT 'TA10', length(ST_AsMVT(q))
FROM (
	SELECT 1 AS c1, -1 AS c2,
	ST_AsMVTGeom(
		'POINT(25 17)'::geometry,
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(48, 48))
	) AS geom
) AS q;

-- Strings and text
SELECT 'TA11', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 'AbcDfg'::varchar AS cstring,
	       'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus sed nulla augue. Pellentesque ut vulputate ex. Nunc et odio placerat, lacinia lectus sed, fermentum sapien. Sed massa velit, ullamcorper et est quis, congue rhoncus orci. Suspendisse in ante varius, convallis enim ut, fermentum amet.'::text as ctext,
	       ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
) AS q;


-- Check null attributes
SELECT 'TA12', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 1::int AS c1, NULL::double precision AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT NULL AS c1, 2.0 AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(26 18)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
) AS q;

SELECT 'TA13', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (
	SELECT 1::int AS c1, NULL::double precision AS c2, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
	UNION
	SELECT 5 AS c1, 2.0 AS c2, null AS geom
) AS q;

-- Extra geometry as parameter (casted as string)
SELECT 'TA14', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM
(
	SELECT geom, St_Expand(geom, 10) as other_geom FROM
	(
		SELECT 'SRID=3857;MULTILINESTRING((105209.784484008 5267849.91657293,102374.204885822 5266414.05020624,99717.9874419115 5267379.35282178,90157.5689699989 5266091.78724987,86186.0622890498 5271349.34154337,78713.0972659854 5272871.78773217,76281.8581230672 5277951.00736649,81783.372341432 5289800.59747023))'::geometry as geom
	) _sq
) AS q;

-- Numeric: Currently being casted as strings
SELECT 'TA15', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM
(
	SELECT 1::numeric AS c1, '12.232389283223239'::numeric AS c2,
	       '1' AS cstring, ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
			ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom
) AS q;

-- default values tests
SELECT 'D1', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'D2', encode(ST_AsMVT(q, 'test', 4096), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'D3', encode(ST_AsMVT(q, 'test'), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
SELECT 'D4', encode(ST_AsMVT(q), 'base64') FROM (SELECT 1 AS c1, 'abcd'::text AS c2,
	ST_AsMVTGeom(ST_GeomFromText('POINT(25 17)'),
		ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)), 4096, 0, false) AS geom) AS q;
select 'D5', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096, 0));
select 'D6', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096)),
	4096));
select 'D7', ST_AsText(ST_AsMVTGeom(
	ST_Point(1, 2),
	ST_MakeBox2D(ST_Point(0, 0), ST_Point(4096, 4096))));

-- unsupported input
SELECT 'TU2';
SELECT encode(ST_AsMVT(1, 'test', 4096, 'geom'), 'base64');
SELECT 'TU3', encode(ST_AsMVT(q, 'test', 4096, 'geom'), 'base64')
	FROM (SELECT NULL::integer AS c1, NULL AS geom) AS q;

-- Ticket #3922
SELECT '#3922', St_Area(ST_AsMVTGeom(
		st_geomfromtwkb('\x06000104a501d0a7db06dad0940120ee030660229604109c010ed6011246143e76201a22b401a601f001b801580ef40122d803ca01de0cf00e80049204ac02b602be0138ca08bc02d605d201d2013cb804a401be013c9c03cd028608c106f001c1018601c7011e970125f10207439b02850a76ff0415030d0725431973132f227768671a5f133f17290865e405ba0154ab030415348502cc038d120c37d326c706850896109f01e201350f6f0a1903930165830121412d052928651d2b0d0f1107170b490c33120f010f0813034f47f50259190181031b3713ed04d901bd01439b02639507c10201021062054c1d3c101e6a0c2000684e6a7c1a681f443d160f044f0f490f03020b08051c01080c0e18013a012801380e08005808522d3a4c1c062a0f0e192a190a3b22194803261c1376122ac201d8011a101c065a17a8011c303206460c164a18a4015c56620801162d1404a402c601262a143002421222290f7b581d0719011d0311002908250a25021d030f0111030f05a3014315050d05110383011b9d011f3309a70347170325058b03417515130361190b4e19b40105fe208810041314041018270705c0039d0416251a1213241b0ffd02f5029408d001990218110607100c070a0b0819031c263432302454322a1e262a101e2a521426101c0c10101210121e18341c321c2c1c26222230162a10320c280e3202080e26123e0a2c041a002805360002051e010807161b281b1e1312010213101b14150e0906130c331e3518250e250c230a1d0a110e091407140718031a0008031a03140112093a199a01199801052e04100a0c120a4a0c7e1e2406220c4e20b60236c8013014020c0510090a110e1b0e11140d18021c08261054261417080f082504a702ea012c58068801180e0f1477209301082f062f00271f0b4b17170311020d100b180d180b0c1502190019031f09512d9b01592a1f120d0c0f0a15126b0637060f100b16032c0a2a1410120c120a240014011a03160314001482010100810900311753478d0117218b02b3027a692223745b2a111e031e130403061902691aa50118752ea70158045818562cae0170202614200e2e208a012e6c4a154c33448b01242f34651e2b221d481f2017122334a1010a510f2b6d8d021d5f073304351233242bce01772a251c3106291b4b032110351c2324239c016f260f5c138a01074e14261422282844221c7a24779a022db90127450b174319b501019b015a61b00105782a4e2a7615741305313a14422a4a079c01519001c9019c013388017352291c371c4110497e26442a8a0108502a2e2e19100980011984010b0e01840136042e1a6a22781f32356813280104370a5b12a5012b533e0748244e3e54355c064a45880115289d01103904453e810127290b5103a70152174841680b10254814402a4e305e82017232581792010522512e2516370023380b9801125434584c2a3e5202042f4e390f2f0e050205001f0801380d00430515421744fd01311b16614c038001241a482c3e44061e0a3881012605244d0e2d5d291a192c5710759d01284b20150f752308530a7f198101113d145d1f13534727290a291f490f4b0215246b196929752d2f2581012675371d432f090c4d2c0d080b141f0a0034051401110735152921055940010a023c0c0c35030519270825382f104512753e014001ae013b041708356ced012a0f7c2d041d0415631507e501012f0a491327411d1b310811072947493d0843125f4b7b16'),
		'SRID=3347;POLYGON((3658201 658873,3658201 5958872.97428571,8958201.49428571 5958872.97428571,8958201.49428571 658873,3658201 658873))'::geometry,
		4096,
		0,
		true
		));

SELECT '#4314', ST_AsMVTGeom(
	'SRID=3857;MULTIPOLYGON(((-8230700.44460474 4970098.60762691,-8230694.76395068 4970080.40480045,-8230692.98226063 4970074.69572152,-8230702.2389602 4970071.78449542,-8230709.99536139 4970096.63875167,-8230700.73864062 4970099.5499925,-8230700.44460474 4970098.60762691)))'::geometry,
	'SRID=3857;POLYGON((-8257645.03970416 5009377.08569731,-8257645.03970416 4970241.3272153,-8218509.28122215 4970241.3272153,-8218509.28122215 5009377.08569731,-8257645.03970416 5009377.08569731))'::geometry,
	2048,
	8,
	true);