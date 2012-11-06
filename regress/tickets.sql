\set VERBOSITY terse
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
SELECT '#122', ST_AsText(ST_SnapToGrid(ST_GeomFromText('CIRCULARSTRING(220268 150415,220227 150505,220227 150406)'), 0.1));

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
SELECT '#183', ST_AsText(ST_SnapToGrid(ST_LineToCurve(ST_LineMerge(ST_Collect(ST_CurveToLine(ST_GeomFromEWKT('CIRCULARSTRING(0 0, 1 1, 1 0)')),ST_GeomFromEWKT('LINESTRING(1 0, 0 1)') ))),0.0001));

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
CREATE TABLE c ( the_geom GEOMETRY);
INSERT INTO c SELECT ST_MakeLine(ST_Point(-10,40),ST_Point(40,-10)) As the_geom;
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

-- #457 --
SELECT '#457.1', st_astext(st_collectionExtract('POINT(0 0)', 1));
SELECT '#457.2', st_astext(st_collectionExtract('POINT(0 0)', 2));
SELECT '#457.3', st_astext(st_collectionExtract('POINT(0 0)', 3));
SELECT '#457.4', st_astext(st_collectionExtract('LINESTRING(0 0, 1 1)', 1));
SELECT '#457.5', st_astext(st_collectionExtract('LINESTRING(0 0, 1 1)', 2));
SELECT '#457.6', st_astext(st_collectionExtract('LINESTRING(0 0, 1 1)', 3));
SELECT '#457.7', st_astext(st_collectionExtract('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))', 1));
SELECT '#457.8', st_astext(st_collectionExtract('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))', 2));
SELECT '#457.9', st_astext(st_collectionExtract('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))', 3));

-- #650 --
SELECT '#650', ST_AsText(ST_Collect(ARRAY[ST_MakePoint(0,0), ST_MakePoint(1,1), null, ST_MakePoint(2,2)]));

-- #845
SELECT '#845', ST_Intersects('POINT(169.69960846592 -46.5061209281002)'::geometry, 'POLYGON((169.699607857174 -46.5061218662,169.699607857174 -46.5061195965597,169.699608806526 -46.5061195965597,169.699608806526 -46.5061218662,169.699607857174 -46.5061218662))'::geometry);

-- #884 --
CREATE TABLE foo (id integer, the_geom geometry);
INSERT INTO foo VALUES (1, st_geomfromtext('MULTIPOLYGON(((-113.6 35.4,-113.6 35.8,-113.2 35.8,-113.2 35.4,-113.6 35.4),(-113.5 35.5,-113.3 35.5,-113.3 35.7,-113.5 35.7,-113.5 35.5)))', -1));
INSERT INTO foo VALUES (2, st_geomfromtext('MULTIPOLYGON(((-113.7 35.3,-113.7 35.9,-113.1 35.9,-113.1 35.3,-113.7 35.3),(-113.6 35.4,-113.2 35.4,-113.2 35.8,-113.6 35.8,-113.6 35.4)),((-113.5 35.5,-113.5 35.7,-113.3 35.7,-113.3 35.5,-113.5 35.5)))', -1));

select '#884', id, ST_Within(
ST_GeomFromText('POINT (-113.4 35.6)', -1), the_geom
) from foo;

DROP TABLE foo;

-- #877
create table t(g geometry);
select '#877.1', st_estimated_extent('t','g');
analyze t;
select '#877.2', st_estimated_extent('public', 't','g');
insert into t(g) values ('LINESTRING(-10 -50, 20 30)');
select '#877.3', st_estimated_extent('t','g');
analyze t;
select '#877.4', st_estimated_extent('t','g');
drop table t;

-- #1344
select '#1344', octet_length(ST_AsEWKB(st_makeline(g))) FROM ( values ('POINT(0 0)'::geometry ) ) as foo(g);

-- #852
CREATE TABLE cacheable (id int, g geometry);
COPY cacheable FROM STDIN;
1	POINT(0.5 0.5000000000001)
2	POINT(0.5 0.5000000000001)
\.
select '#852.1', id, -- first run is not cached, consequent are cached
  st_intersects(g, 'POLYGON((0 0, 10 10, 1 0, 0 0))'::geometry),
  st_intersects(g, 'POLYGON((0 0, 1 1, 1 0, 0 0))'::geometry) from cacheable;
UPDATE cacheable SET g = 'POINT(0.5 0.5)';
-- New select, new cache
select '#852.2', id, -- first run is not cached, consequent are cached
  st_intersects(g, 'POLYGON((0 0, 10 10, 1 0, 0 0))'::geometry),
  st_intersects(g, 'POLYGON((0 0, 1 1, 1 0, 0 0))'::geometry) from cacheable;
DROP TABLE cacheable;

-- #1595
SELECT '#1595', 
  ST_AsEWKT(ST_Line_Substring('SRID=32019;MULTILINESTRING((0 0,0 1))', 0, 1)); 

-- #1596 --
CREATE TABLE road_pg (ID INTEGER, NAME VARCHAR(32));
SELECT '#1596.1', AddGeometryColumn('road_pg','roads_geom', 4326, 'POINT', 2);
SELECT '#1596.2', UpdateGeometrySRID('road_pg','roads_geom', 330000);
SELECT '#1596.3', srid FROM geometry_columns
  WHERE f_table_name = 'road_pg' AND f_geometry_column = 'roads_geom';
DROP TABLE road_pg;

-- #1697 --
CREATE TABLE eg(g geography);
CREATE INDEX egi on eg using gist (g);
INSERT INTO eg (g) select 'POINT EMPTY'::geography
 from generate_series(1,1024);
SELECT '#1697.1', count(*) FROM eg WHERE g && 'POINT(0 0)'::geography;
DROP TABLE eg;

-- #1170 --
SELECT '#1170', ST_Y(ST_Intersection( ST_GeogFromText( 'POINT(0 90)'), ST_GeogFromText( 'POINT(0 90)' ))::geometry);

-- #1252 --
SELECT '#1252', st_astext(st_union(geom)) from ( select (st_dump('SRID=4326;MULTIPOLYGON(((0 0, 1 1, 1 0, 0 0)),((4 4, 4 5, 5 5, 4 4)))'::geometry)).geom) as foo;

-- #1264 --
SELECT '#1264', ST_DWithin('POLYGON((-10 -10, -10 10, 10 10, 10 -10, -10 -10))'::geography, 'POINT(0 0)'::geography, 0);

-- #1799 --
SELECT '#1799', ST_Segmentize('LINESTRING(0 0, 10 0)'::geometry, 0);

-- #1936 --
select st_astext(st_geomfromgml(
    '<gml:Polygon xmlns:gml="http://www.opengis.net/gml/3.2" 
    gml:id="HPA.15449990010" srsName="urn:ogc:def:crs:EPSG::4326" 
    srsDimension="2">
    <gml:exterior>
    <gml:Ring>
    <gml:curveMember>
    <gml:LineString gml:id="HPA.15449990010.1">
    <gml:posList>711540.35 1070163.61 711523.82 1070166.54 711521.30 1070164.14 711519.52 1070162.44 711518.57 1070164.62 712154.47 1070824.94</gml:posList>
    </gml:LineString>
    </gml:curveMember>
    <gml:curveMember>
    <gml:Curve gml:id="HPA.15449990010.2">
    <gml:segments><gml:ArcString>
    <gml:posList>712154.47 1070824.94 712154.98 1070826.04 712154.41 1070827.22</gml:posList>
    </gml:ArcString>
    </gml:segments>
    </gml:Curve>
    </gml:curveMember>
    <gml:curveMember>
    <gml:LineString gml:id="HPA.15449990010.3">
    <gml:posList>712154.41 1070827.22 712160.31 1070837.07 712160.92 1070835.36 712207.89 1071007.95</gml:posList>
    </gml:LineString>
    </gml:curveMember>
    <gml:curveMember>
    <gml:Curve gml:id="HPA.15449990010.4"><gml:segments><gml:ArcString><gml:posList>712207.89 1071007.95 712207.48 1071005.59 712208.38 1071001.28</gml:posList></gml:ArcString></gml:segments></gml:Curve></gml:curveMember><gml:curveMember><gml:LineString gml:id="HPA.15449990010.5"><gml:posList>712208.38 1071001.28 712228.74 1070949.67 712233.98 1070936.15 712124.93 1070788.72</gml:posList></gml:LineString></gml:curveMember><gml:curveMember><gml:Curve gml:id="HPA.15449990010.6"><gml:segments><gml:ArcString><gml:posList>712124.93 1070788.72 712124.28 1070785.87 712124.63 1070783.38</gml:posList></gml:ArcString></gml:segments></gml:Curve></gml:curveMember><gml:curveMember><gml:LineString gml:id="HPA.15449990010.7"><gml:posList>712124.63 1070783.38 712141.04 1070764.12 712146.60 1070757.01 711540.35 1070163.61</gml:posList></gml:LineString></gml:curveMember></gml:Ring></gml:exterior>
    <gml:interior>
    <gml:LinearRing>
    <gml:posList>713061.62 1070354.46 713053.59 1070335.12 713049.58 1070315.92 713049.65 1070298.33 713061.62 1070354.46</gml:posList>
    </gml:LinearRing>
    </gml:interior>
    </gml:Polygon>'));

-- #1957 --
SELECT '#1957', ST_Distance(ST_Makeline(ARRAY['POINT(1 0)'::geometry]), 'POINT(0 0)'::geometry);

-- #2084 --
SELECT '#2048', num, ST_Within('POINT(-54.394 56.522)', "the_geom"), ST_CoveredBy('POINT(-54.394 56.522)', "the_geom")
FROM ( VALUES
(1, '0103000000010000000E00000051C6F7C5A5324BC02EB69F8CF13F4C40F12EA4C343364BC0326AA2CF47434C402BC1A8A44E364BC02A50E10852434C407F2990D959364BC0A0D1730B5D434C404102452C62364BC0ECF335CB65434C400903232F6B364BC0F635E84B6F434C40BD0CC51D6F364BC0D2805EB873434C40B9E6E26F7B364BC0F20B93A982434C40D9FAAF73D3344BC0FE84D04197444C40BD5C8AABCA344BC0CED05CA791444C4023F2237EC5344BC02A84F23E8E444C40BDCDD8077B324BC0C60FB90F01434C409FD1702E65324BC04EF1915C17404C4051C6F7C5A5324BC02EB69F8CF13F4C40'::geometry), 
(2, '0103000000010000001C00000003F25650F73B4BC098477F523E3E4C40C9A6A344CE3C4BC0C69698653E3E4C40BDD0E979373E4BC0081FA0FB723E4C400FD252793B3E4BC01A137F14753E4C40537170E998414BC070D3BCE314414C4023FC51D499474BC0D4D100DE024F4C40638C47A984454BC024130D52F0504C40B9442DCDAD404BC03A29E96168554C40C7108DEE20404BC07C7C26FBE7554C40195D6BEF533F4BC0E20391459A564C40239FE40E9B344BC08C1ADB6B41514C40132D3F7095314BC0BA2ADF33124F4C409DB91457952D4BC02C7B681F2B4C4C4089DC60A8C32C4BC07C5C3810924B4C40D7ED409DF22A4BC0F64389963C4A4C405D1EF818AC2A4BC00EC84274084A4C401B48A46DFC294BC0B271A8DF85494C40E78AA6B393294BC01ED0EFFB37494C4081C64B3789294BC0DC5BE7DF2E494C409B23329287294BC0F0D6974E2D494C40CD22D5D687294BC0844316D72C494C40F5229D4FE2294BC002F19825AB484C40A3D0BD5AE9294BC06C0776A9A2484C409FD1702E65324BC04EF1915C17404C409F860AA7BD324BC0162CA390E33F4C40539A5C1C23334BC0FE86B04EB03F4C4081511DFF90334BC088FF36D4873F4C4003F25650F73B4BC098477F523E3E4C40'::geometry), 
(3, '010300000001000000100000008D57CD101A214BC0AECDD34E072C4C400DBB72E6EC274BC0A8088D60E32C4C40CF8FD7E6734E4BC0B22695BE4A324C40BFA74213934F4BC020BE505D4C354C4057CD4BEE454E4BC0BA6CF3940F3D4C40E7BDC5FD263E4BC09A4B297D5B484C4073A46A86701C4BC0B287F08D93364C4045501F86701C4BC05EBDB78D93364C40A37DB6586D1C4BC0841E7D2891364C409FBF445F6D1C4BC01E225C5690364C40D1BA97726D1C4BC06E2AF7EA8D364C4019B60C9B751C4BC0D2FD702575364C40FDE4394B5E1F4BC08C40F231CC2F4C402343DF40F51F4BC022008E3D7B2E4C400BB57B45F9204BC0908CE2EA3A2C4C408D57CD101A214BC0AECDD34E072C4C40'::geometry) 
) AS f(num, the_geom);


-- Clean up
DELETE FROM spatial_ref_sys;
