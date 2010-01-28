--
-- Regression tests that were filed as cases in bug tickets,
-- referenced by bug number for historical interest.
--

DELETE FROM spatial_ref_sys;
INSERT INTO spatial_ref_sys ( srid, proj4text ) VALUES ( 32611, '+proj=utm +zone=11 +ellps=WGS84 +datum=WGS84 +units=m +no_defs' );
INSERT INTO spatial_ref_sys ( srid, proj4text ) VALUES ( 4326, '+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs' );
INSERT INTO spatial_ref_sys ( srid, proj4text ) VALUES ( 32602, '+proj=utm +zone=2 +ellps=WGS84 +datum=WGS84 +units=m +no_defs ' );
INSERT INTO spatial_ref_sys ( srid, proj4text ) VALUES ( 32702, '+proj=utm +zone=2 +south +ellps=WGS84 +datum=WGS84 +units=m +no_defs ' );


-- #2 --
SELECT '#2', ST_AsText(ST_Union(g)) FROM
( VALUES 
('SRID=4326;MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)))'), 
('SRID=4326;MULTIPOLYGON(((2 1,3 1,3 2,2 2,2 1)))')
) AS v(g);

-- #11 --
SELECT '#11', ST_Distance (a.g, ST_Intersection(b.g, a.g)) AS distance 
FROM (SELECT '01020000000200000050E8303FC2E85141B017CFC05A825541000000E0C0E85141000000205C825541'::geometry AS g) a, 
	 (SELECT 'LINESTRING(4694792.35840419 5638508.89950758,4694793.20840419 5638506.34950758)'::geometry AS g) b;
	
-- #21 --
SELECT '#21', ST_AsEWKT(ST_Locate_Along_Measure(g, 4566)) FROM
( VALUES 
(ST_GeomFromEWKT('SRID=31293;LINESTRINGM( 6193.76 5337404.95 4519, 6220.13 5337367.145 4566, 6240.889 5337337.383 4603 )'))
) AS v(g);

-- #22 --
SELECT ST_Within(g, 'POLYGON((0 0,10 0,20 10,10 20,0 20,-10 10,0 0))') FROM 
(VALUES 
('POLYGON((4 9,6 9,6 11,4 11,4 9))')
) AS v(g);

-- #33 --
CREATE TABLE road_pg (ID INTEGER, NAME VARCHAR(32));
SELECT '#33', AddGeometryColumn( '', 'public', 'road_pg','roads_geom', 330000, 'POINT', 2 );
DROP TABLE road_pg;

-- #44 -- 
SELECT '#44', ST_Relate(g1, g2, 'T12101212'), ST_Relate(g1, g2, 't12101212') FROM 
(VALUES 
('POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))', 'POLYGON((1 1, 3 1, 3 3, 1 3, 1 1))')
) AS v(g1, g2);

-- #58 --
SELECT '#58', round(ST_xmin(g)),round(ST_ymin(g)),round(ST_xmax(g)),round(ST_ymax(g)) FROM (SELECT ST_Envelope('CIRCULARSTRING(220268.439465645 150415.359530563,220227.333322076 150505.561285879,220227.353105332 150406.434743975)') as g)  AS foo;

-- #65 --
SELECT '#65', ST_AsGML(ST_GeometryFromText('CURVEPOLYGON(CIRCULARSTRING(4 2,3 -1.0,1 -1,-1.0 4,4 2))'));

-- #66 --
SELECT '#66', ST_AsText((ST_Dump(ST_GeomFromEWKT('CIRCULARSTRING(0 0 1,1 1 2,2 2 3)'))).geom);

-- #68 --
SELECT '#68a', ST_AsText(ST_Shift_Longitude(ST_GeomFromText('MULTIPOINT(1 3, 4 5)')));
SELECT '#68b', ST_AsText(ST_Shift_Longitude(ST_GeomFromText('CIRCULARSTRING(1 3, 4 5, 6 7)')));

-- #69 --
SELECT '#69', ST_AsText(ST_Translate(ST_GeomFromText('CIRCULARSTRING(220268 150415,220227 150505,220227 150406)'),1,2));

-- #70 --
SELECT '#70', ST_NPoints(ST_LinetoCurve(ST_Buffer('POINT(1 2)',3)));

-- #73 --
SELECT '#73', ST_AsText(ST_Force_Collection(ST_GeomFromEWKT('CIRCULARSTRING(1 1 2, 2 3 2, 4 5 2, 6 7 2, 5 6 2)')));

-- #80 --
SELECT '#80', ST_AsText(ST_Multi('MULTILINESTRING((0 0,1 1))'));

-- #83 --
SELECT '#83', ST_AsText(ST_Multi(ST_GeomFromText('CIRCULARSTRING(220268 150415,220227 150505,220227 150406)')));

-- #85 --
SELECT '#85', ST_Distance(ST_GeomFromText('CIRCULARSTRING(220268 150415,220227 150505,220227 150406)'), ST_Point(220268, 150415));

-- #112 --
SELECT '#112', ST_AsText(ST_CurveToLine('GEOMETRYCOLLECTION(POINT(-10 50))'::geometry));

-- #113 --
SELECT '#113', ST_Locate_Along_Measure('LINESTRING(0 0 0, 1 1 1)', 0.5);

-- #116 --
SELECT '#116', ST_AsText('010300000000000000');

-- #122 --
SELECT '#122', ST_SnapToGrid(ST_GeomFromText('CIRCULARSTRING(220268 150415,220227 150505,220227 150406)'), 0.1);

-- #124 --
SELECT '#124a', ST_AsText(ST_GeomFromEWKT('COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,30 5),CIRCULARSTRING(30 5,34 56,67 89))'));
SELECT '#124b', ST_AsText(ST_GeomFromEWKT('COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,30 6),CIRCULARSTRING(30 5,34 56,67 89))'));

-- #145 --
SELECT '#145a', ST_Buffer(ST_GeomFromText('LINESTRING(-116.93414544665981 34.16033385105459,-116.87777514700957 34.10831080544884,-116.86972224705954 34.086748622072776,-116.9327074288116 34.08458099517253,-117.00216369088065 34.130329331330216,-117.00216369088065 34.130329331330216)', 4326), 0);
SELECT '#145b', ST_Area(ST_Buffer(ST_GeomFromText('LINESTRING(-116.93414544665981 34.16033385105459,-116.87777514700957 34.10831080544884,-116.86972224705954 34.086748622072776,-116.9327074288116 34.08458099517253,-117.00216369088065 34.130329331330216,-117.00216369088065 34.130329331330216)', 4326), 0));

-- #146 --
SELECT '#146', ST_Distance(g1,g2), ST_Dwithin(g1,g2,0.01), ST_AsEWKT(g2) FROM (SELECT ST_geomFromEWKT('LINESTRING(1 2, 2 4)') As g1, ST_Collect(ST_GeomFromEWKT('LINESTRING(0 0, -1 -1)'), ST_GeomFromEWKT('MULTIPOINT(1 2,2 3)')) As g2) As foo;

-- #156 --
SELECT '#156', ST_AsEWKT('0106000000010000000103000000010000000700000024213D12AA7BFD40945FF42576511941676A32F9017BFD40B1D67BEA7E511941C3E3C640DB7DFD4026CE38F4EE531941C91289C5A7EFD40017B8518E3531941646F1599AB7DFD409627F1F0AE521941355EBA49547CFD407B14AEC74652194123213D12AA7BFD40945FF42576511941');

-- #157 --
SELECT 
	'#157',
	ST_GeometryType(g) As newname, 
	GeometryType(g) as oldname 
FROM ( VALUES 
	(ST_GeomFromText('POLYGON((-0.25 -1.25,-0.25 1.25,2.5 1.25,2.5 -1.25,-0.25 -1.25), (2.25 0,1.25 1,1.25 -1,2.25 0),(1 -1,1 1,0 0,1 -1))') ),
	( ST_Point(1,2) ), 
	( ST_Buffer(ST_Point(1,2), 3) ), 
	( ST_LineToCurve(ST_Buffer(ST_Point(1,2), 3)) ) , 
	( ST_LineToCurve(ST_Boundary(ST_Buffer(ST_Point(1,2), 3))) )
	) AS v(g);


-- #168 --
SELECT '#168', ST_NPoints(g), ST_AsText(g)
FROM ( VALUES
('01060000C00100000001030000C00100000003000000E3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFFE3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFFE3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFF'::geometry)
) AS v(g);

-- #175 --
SELECT '#175', ST_AsEWKT(ST_GeomFromEWKT('SRID=26915;POINT(482020 4984378.)'));

-- #178 --
SELECT '#178a', ST_XMin(ST_MakeBox2D(ST_Point(5, 5), ST_Point(0, 0)));
SELECT '#178b', ST_XMax(ST_MakeBox2D(ST_Point(5, 5), ST_Point(0, 0)));

-- #179 --
SELECT '#179a', ST_MakeLine_GArray(ARRAY[NULL,NULL,NULL,NULL]);
SELECT '#179b', ST_MakeLine(ARRAY[NULL,NULL,NULL,NULL]);

-- #183 --
SELECT '#183', ST_AsText(ST_LineToCurve(ST_LineMerge(ST_Collect(ST_CurveToLine(ST_GeomFromEWKT('CIRCULARSTRING(0 0, 1 1, 1 0)')),ST_GeomFromEWKT('LINESTRING(1 0, 0 1)') ))));

-- #210 --
SELECT '#210a', ST_Union(ARRAY[NULL,NULL,NULL,NULL]) ;
SELECT '#210b', ST_MakeLine(ARRAY[NULL,NULL,NULL,NULL]) ;

-- #213 --
SELECT '#213', round(ST_Perimeter(ST_CurveToLine(ST_GeomFromEWKT('CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3),(4 3, 4 5, 1 4, 0 0)))'))));

-- #234 --
SELECT '#234', ST_AsText(ST_GeomFromText('COMPOUNDCURVE( (0 0,1 1) )'));

-- #239 --
--SELECT '#239', ST_AsSVG('010700002031BF0D0000000000');

-- #241 --
CREATE TABLE c AS SELECT ST_MakeLine(ST_Point(-10,40),ST_Point(40,-10)) As the_geom;
INSERT INTO c SELECT ST_MakeLine(ST_Point(-10,40),ST_Point(40,-10)) As the_geom;
SELECT '#241', sum(ST_LineCrossingDirection(the_geom, ST_GeomFromText('LINESTRING(1 2,3 4)'))) FROM c;
DROP TABLE c;

-- #254 --
SELECT '#254', ST_Segmentize(ST_GeomFromText('GEOMETRYCOLLECTION EMPTY'), 0.5);

-- #259 --
SELECT '#259', ST_Distance(ST_GeographyFromText('SRID=4326;POLYGON EMPTY'), ST_GeographyFromText('SRID=4326;POINT(1 2)'));

-- #260 --
SELECT '#260', round(ST_Distance(ST_GeographyFromText('SRID=4326;POINT(-10 40)'), ST_GeographyFromText('SRID=4326;POINT(-10 55)')));

-- #261 --
SELECT '#261', ST_Distance(ST_GeographyFromText('SRID=4326;POINT(-71.0325022849392 42.3793285830812)'),ST_GeographyFromText('SRID=4326;POLYGON((-71.0325022849392 42.3793285830812,-71.0325745928559 42.3793012556699,-71.0326708728343 42.3794450989722,-71.0326045866257 42.3794706688942,-71.0325022849392 42.3793285830812))'));

-- #262 --
SELECT '#262', ST_AsText(pt.the_geog) As wkt_pt, ST_Covers(poly.the_geog, pt.the_geog) As geog,
	ST_Covers(
		ST_Transform(CAST(poly.the_geog As geometry),32611), 
		ST_Transform(CAST(pt.the_geog As geometry),32611)) As utm,
	ST_Covers(
		CAST(poly.the_geog As geometry), 
		CAST(pt.the_geog As geometry)
	) As pca
FROM (SELECT ST_GeographyFromText('SRID=4326;POLYGON((-119.5434 34.9438,-119.5437 34.9445,-119.5452 34.9442,-119.5434 34.9438))') As the_geog) 
	As poly
    CROSS JOIN 
	(VALUES
		( ST_GeographyFromText('SRID=4326;POINT(-119.5434 34.9438)') ) ,
		( ST_GeographyFromText('SRID=4326;POINT(-119.5452 34.9442)') ) ,
		( ST_GeographyFromText('SRID=4326;POINT(-119.5434 34.9438)') ),
		( ST_GeographyFromText('SRID=4326;POINT(-119.5438 34.9443)')  )
	)
	As pt(the_geog);

-- #263 --
SELECT '#263', ST_AsEWKT(geometry(geography(pt.the_geom))) As wkt,
	ST_Covers(
		ST_Transform(poly.the_geom,32611), 
		ST_Transform(pt.the_geom,32611)) As utm,
	ST_Covers(
		poly.the_geom, 
		pt.the_geom)
	 As pca,
	ST_Covers(geometry(geography(poly.the_geom)),
		geometry(geography(pt.the_geom))) As gm_to_gg_gm_pca
	
FROM (SELECT ST_GeomFromEWKT('SRID=4326;POLYGON((-119.5434 34.9438,-119.5437 34.9445,-119.5452 34.9442,-119.5434 34.9438))') As the_geom) 
	As poly
    CROSS JOIN 
	(VALUES
		( ST_GeomFromEWKT('SRID=4326;POINT(-119.5434 34.9438)') ) ,
		( ST_GeomFromEWKT('SRID=4326;POINT(-119.5452 34.9442)') ) ,
		( ST_GeomFromEWKT('SRID=4326;POINT(-119.5434 34.9438)') ),
		( ST_GeomFromEWKT('SRID=4326;POINT(-119.5438 34.9443)')  )
	)
	As pt(the_geom);

-- #271 --
SELECT '#271', ST_Covers(
'POLYGON((-9.123456789 50,51 -11.123456789,-10.123456789 50,-9.123456789 50))'::geography,
'POINT(-10.123456789 50)'::geography
);

-- #272 --
SELECT '#272', ST_LineCrossingDirection(foo.line1, foo.line2) As l1_cross_l2 ,
    ST_LineCrossingDirection(foo.line2, foo.line1) As l2_cross_l1
FROM (SELECT
    ST_GeomFromText('LINESTRING(25 169,89 114,40 70,86 43)') As line1, ST_GeomFromText('LINESTRING(2.99 90.16,71 74,20 140,171 154)') As line2 ) As foo;

-- #277 --
SELECT '#277', ST_AsGML(2, GeomFromText('POINT(1 1e308)'));

-- #299 --
SELECT '#299', round(ST_Y(geometry(ST_Intersection(ST_GeographyFromText('POINT(1.2456 2)'), ST_GeographyFromText('POINT(1.2456 2)'))))); 

-- #304 --

SELECT '#304';

CREATE OR REPLACE FUNCTION utmzone(geometry)
  RETURNS integer AS
$BODY$
DECLARE
    geomgeog geometry;
    zone int;
    pref int;

BEGIN
    geomgeog:= ST_Transform($1,4326);

    IF (ST_Y(geomgeog))>0 THEN
       pref:=32600;
    ELSE
       pref:=32700;
    END IF;

    zone:=floor((ST_X(geomgeog)+180)/6)+1;

    RETURN zone+pref;
END;
$BODY$ LANGUAGE 'plpgsql' IMMUTABLE
  COST 100;

SELECT ST_AsText(the_geog) as the_pt, ST_Area(ST_Buffer(the_geog,10)) As the_area, 
	ST_Area(geography(ST_Transform(ST_Buffer(ST_Transform(geometry(the_geog),utm_srid),10),4326))) As geog_utm_area
FROM (SELECT geography(ST_SetSRID(ST_Point(i*10,j*10),4326)) As the_geog, utmzone(ST_SetSRID(ST_Point(i*10,j*10),4326)) As utm_srid
	FROM generate_series(-17,17) As i 
	CROSS JOIN generate_series(-8,8) As j
) As foo
WHERE ST_Area(ST_Buffer(the_geog,10)) NOT between 310 and 314
LIMIT 10;

DROP FUNCTION utmzone(geometry);

-- Clean up
DELETE FROM spatial_ref_sys;
