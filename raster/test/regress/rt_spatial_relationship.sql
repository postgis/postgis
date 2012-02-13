-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

CREATE TABLE rt_spatial_relationship_test (
    rid integer,
    description text,
    nbband int,
    b1pixeltype text,
    b1hasnodatavalue boolean,
    b1nodatavalue float4,
    b1val float4,
    b2pixeltype text,
    b2hasnodatavalue boolean,
    b2nodatavalue float4,
    b2val float4,
    geomtxt text,
    rast raster
);

INSERT INTO rt_spatial_relationship_test 
VALUES ( 1, '1x1, nbband:2 b1pixeltype:4BUI b1hasnodatavalue:true b1nodatavalue:3 b2pixeltype:16BSI b2hasnodatavalue:false b2nodatavalue:13',
        2, --- nbband
        '4BUI', true, 3, 2,   --- b1pixeltype, b1hasnodatavalue, b1nodatavalue
        '16BSI', false, 13, 4, --- b2pixeltype, b2hasnodatavalue, b2nodatavalue
        'POLYGON((782325.5 26744042.5,782330.5 26744045.5,782333.5 26744040.5,782328.5 26744037.5,782325.5 26744042.5))',
(
'01' -- big endian (uint8 xdr)
|| 
'0000' -- version (uint16 0)
||
'0200' -- nBands (uint16 1)
||
'0000000000001440' -- scaleX (float64 5)
||
'00000000000014C0' -- scaleY (float64 -5)
||
'00000000EBDF2741' -- ipX (float64 782325.5)
||
'000000A84E817941' -- ipY (float64 26744042.5)
||
'0000000000000840' -- skewX (float64 3)
||
'0000000000000840' -- skewY (float64 3)
||
'27690000' -- SRID (int32 26919 - UTM 19N)
||
'0200' -- width (uint16 1)
||
'0200' -- height (uint16 1)
||
'4' -- hasnodatavalue set to true
||
'2' -- first band type (4BUI) 
||
'04' -- novalue==4
||
'01' -- pixel(1,1)==1
'02' -- pixel(2,1)==2
'03' -- pixel(1,2)==3
'04' -- pixel(2,2)==4
||
'0' -- hasnodatavalue set to false
||
'5' -- second band type (16BSI)
||
'0400' -- novalue==13
||
'0100' -- pixel(1,1)==1
'0200' -- pixel(2,1)==2
'0300' -- pixel(1,2)==3
'0400' -- pixel(2,2)==4
)::raster
);

INSERT INTO rt_spatial_relationship_test 
VALUES ( 2, '1x1, nbband:2 b1pixeltype:4BUI b1hasnodatavalue:true b1nodatavalue:3 b2pixeltype:16BSI b2hasnodatavalue:false b2nodatavalue:13',
        2, --- nbband
        '4BUI', true, 3, 2,   --- b1pixeltype, b1hasnodatavalue, b1nodatavalue
        '16BSI', false, 13, 4, --- b2pixeltype, b2hasnodatavalue, b2nodatavalue
        'POLYGON((-75.5533328537098 49.2824585505576,-75.5525268884758 49.2826703629415,-75.5523150760919 49.2818643977075,-75.553121041326 49.2816525853236,-75.5533328537098 49.2824585505576))',
(
'01' -- little endian (uint8 ndr)
|| 
'0000' -- version (uint16 0)
||
'0200' -- nBands (uint16 0)
||
'17263529ED684A3F' -- scaleX (float64 0.000805965234044584)
||
'F9253529ED684ABF' -- scaleY (float64 -0.00080596523404458)
||
'1C9F33CE69E352C0' -- ipX (float64 -75.5533328537098)
||
'718F0E9A27A44840' -- ipY (float64 49.2824585505576)
||
'ED50EB853EC32B3F' -- skewX (float64 0.000211812383858707)
||
'7550EB853EC32B3F' -- skewY (float64 0.000211812383858704)
||
'E6100000' -- SRID (int32 4326)
||
'0200' -- width (uint16 1)
||
'0200' -- height (uint16 1)
||
'4' -- hasnodatavalue set to true
||
'2' -- first band type (4BUI) 
||
'04' -- novalue==4
||
'01' -- pixel(1,1)==1
'02' -- pixel(2,1)==2
'03' -- pixel(1,2)==3
'04' -- pixel(2,2)==4
||
'0' -- hasnodatavalue set to false
||
'5' -- second band type (16BSI)
||
'0400' -- novalue==4
||
'0100' -- pixel(1,1)==1
'0200' -- pixel(2,1)==2
'0300' -- pixel(1,2)==3
'0400' -- pixel(2,2)==4
)::raster
);

CREATE TABLE rt_spatial_relationship_test_geom (
    geomid int,
    forrast int,
    geom geometry
);

-- Insert points for raster no 1

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 1, 1, st_setsrid(st_makepoint(782325.5, 26744042.5), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 2, 1, st_setsrid(st_makepoint(782325.5 + 1, 26744042.5), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 3, 1, st_setsrid(st_makepoint(782325.5 + 10, 26744042.5 + 10), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 4, 1, st_setsrid(st_makepoint(782325.5 + 10, 26744042.5 - 2), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 5, 1, st_setsrid(st_makepoint(782325.5 + 5 + 3, 26744042.5 - 5 + 3), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 6, 1, st_setsrid(st_makepoint(782325.5 + 2*5 + 0.5*3, 26744042.5 + 2*3 - 0.5*5), 26919));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 7, 1, st_setsrid(st_makepoint(782325.5 + 1, 26744042.5 - 3), 26919));

-- Insert lines for raster no 1

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 11, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 1, 26744042.5 + 1),
                                             st_makepoint(782325.5 + 11, 26744042.5 + 11)]), 26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 12, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 - 1, 26744042.5 - 2),
                                             st_makepoint(782325.5 + 5, 26744042.5 + 4)]), 26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 13, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5, 26744042.5 - 10),
                                             st_makepoint(782325.5 + 18, 26744042.5 - 2)]), 26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 14, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 13, 26744042.5 - 3),
                                             st_makepoint(782325.5 + 18, 26744042.5 - 1)]), 26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 15, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 2*5 - 0.3*3, 26744042.5 + 2*3 + 0.3*5),
                                             st_makepoint(782325.5 + 2*5 + 0.5*3, 26744042.5 + 2*3 - 0.5*5)
                                            ]), 
                                       26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 16, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 2*5 + 1.5*3, 26744042.5 + 2*3 - 1.5*5),
                                             st_makepoint(782325.5 + 2*5 + 2.5*3, 26744042.5 + 2*3 - 2.5*5)
                                            ]),
                                       26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 17, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 2*5 + 1.0*3, 26744042.5 + 2*3 - 1.0*5),
                                             st_makepoint(782325.5 + 2*5 + 1.5*3, 26744042.5 + 2*3 - 1.5*5)
                                            ]),
                                       26919));
INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 18, 1, st_setsrid(st_makeline(ARRAY[st_makepoint(782325.5 + 4, 26744042.5 - 8),
                                             st_makepoint(782325.5 + 7, 26744042.5 + 6)]), 26919));

-- Insert points for raster no 2

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 31, 2, st_setsrid(st_makepoint(-75.5533328537098, 49.2824585505576), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 32, 2, st_setsrid(st_makepoint(-75.5533328537098 + 0.0001, 49.2824585505576), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 33, 2, st_setsrid(st_makepoint(-75.5533328537098 + 0.001, 49.2824585505576 + 0.001), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 34, 2, st_setsrid(st_makepoint(-75.5533328537098 + 0.0015, 49.2824585505576 - 0.001), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 35, 2, st_setsrid(st_makepoint(
                                        -75.5533328537098 + 0.000805965234044584 + 0.000211812383858707, 
                                         49.2824585505576 - 0.00080596523404458 + 0.000211812383858704
                                       ), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 36, 2, st_setsrid(st_makepoint(
                                        -75.5533328537098 + 2.0*0.000805965234044584 + 0.5*0.000211812383858707, 
                                         49.2824585505576 + 2.0*0.000211812383858704 - 0.5*0.00080596523404458
                                       ), 4326));

-- Insert lines for raster no 2

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 41, 2, st_setsrid(st_makeline(ARRAY[
                                             st_makepoint(-75.5533328537098 + 0.0001, 49.2824585505576 + 0.0001), 
                                             st_makepoint(-75.5533328537098 + 0.0011, 49.2824585505576 + 0.0011)
                                            ]), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 42, 2, st_setsrid(st_makeline(ARRAY[
                                             st_makepoint(-75.5533328537098 - 0.0001, 49.2824585505576 - 0.0002), 
                                             st_makepoint(-75.5533328537098 + 0.0005, 49.2824585505576 + 0.0004)
                                            ]), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 43, 2, st_setsrid(st_makeline(ARRAY[
                                             st_makepoint(-75.5533328537098, 49.2824585505576 - 0.0015), 
                                             st_makepoint(-75.5533328537098 + 0.002, 49.2824585505576 - 0.0004)
                                            ]), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 44, 2, st_setsrid(st_makeline(ARRAY[
                                             st_makepoint(-75.5533328537098 + 0.0014, 49.2824585505576 - 0.0005), 
                                             st_makepoint(-75.5533328537098 + 0.002, 49.2824585505576 - 0.0005)
                                            ]), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 45, 2, st_setsrid(st_makeline(ARRAY[st_makepoint(
                                                          -75.5533328537098 + 2.0*0.000805965234044584 - 0.3*0.000211812383858707, 
                                                           49.2824585505576 + 2.0*0.000211812383858704 + 0.3*0.00080596523404458
                                                         ), 
                                             st_makepoint(
                                                          -75.5533328537098 + 2.0*0.000805965234044584 + 0.5*0.000211812383858707, 
                                                           49.2824585505576 + 2.0*0.000211812383858704 - 0.5*0.00080596523404458
                                                         )
                                            ]), 4326));

INSERT INTO rt_spatial_relationship_test_geom 
VALUES ( 46, 2, st_setsrid(st_makeline(ARRAY[st_makepoint(
                                                          -75.5533328537098 + 2.0*0.000805965234044584 + 1.5*0.000211812383858707, 
                                                           49.2824585505576 + 2.0*0.000211812383858704 - 1.5*0.00080596523404458
                                                         ), 
                                             st_makepoint(
                                                          -75.5533328537098 + 2*0.000805965234044584 + 2.5*0.000211812383858707, 
                                                           49.2824585505576 + 2*0.000211812383858704 - 2.5*0.00080596523404458
                                                         )
                                            ]), 4326));

-----------------------------------------------------------------------
-- Test 1 - st_intersect(geometry, raster) 
-----------------------------------------------------------------------

SELECT 'test 1.1', rid, geomid, ST_Intersects(geom, rast) AS intersect
	FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
	WHERE forrast = rid ORDER BY rid, geomid;
	
SELECT 'test 1.2', rid, geomid, ST_Intersects(geom, rast) AS intersect
	FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
	WHERE forrast = rid AND ST_Intersects(geom, rast, 1) ORDER BY rid, geomid;
	
SELECT 'test 1.3', rid, geomid, ST_Intersects(geom, st_setbandnodatavalue(rast, NULL), 1)
	FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
	WHERE forrast = rid ORDER BY rid, geomid;

SELECT 'test 1.4', rid, geomid, ST_Intersects(geom, st_setbandnodatavalue(rast, NULL))
	FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
	WHERE forrast = rid AND ST_Intersects(geom, rast, 1) ORDER BY rid, geomid;

-----------------------------------------------------------------------
-- Test 2 - st_intersection(raster, geometry) 
-----------------------------------------------------------------------
SELECT 'test 2.1', rid, geomid, ST_AsText((gv).geom), (gv).val
FROM (SELECT *, ST_Intersection(geom, rast) gv
      FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
      WHERE forrast = rid
     ) foo;

SELECT 'test 2.2', rid, geomid, ST_AsText((gv).geom), (gv).val
FROM (SELECT *, ST_Intersection(geom, rast) gv
      FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
      WHERE forrast = rid AND ST_Intersects(geom, rast, 1)
     ) foo;

SELECT 'test 2.3', rid, geomid, ST_AsText((gv).geom), (gv).val
FROM (SELECT *, ST_Intersection(geom, st_setbandnodatavalue(rast, NULL)) gv
      FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
      WHERE forrast = rid
     ) foo;
     
SELECT 'test 2.4', rid, geomid, ST_AsText((gv).geom), (gv).val
FROM (SELECT *, ST_Intersection(geom, st_setbandnodatavalue(rast, NULL)) gv
      FROM rt_spatial_relationship_test, rt_spatial_relationship_test_geom
      WHERE forrast = rid AND ST_Intersects(geom, st_setbandnodatavalue(rast, NULL), 1)
     ) foo;

DROP TABLE rt_spatial_relationship_test;
DROP TABLE rt_spatial_relationship_test_geom;
