--- ST_Azimuth
SELECT 'ST_Azimuth_regular' , round(ST_Azimuth(geom1,geom2)::numeric,4)
FROM CAST('POINT(0 1)' AS geometry) AS geom1, CAST('POINT(1 0)' AS geometry) AS geom2 ;
SELECT 'ST_Azimuth_same_point' , ST_Azimuth(geom1,geom1)
FROM CAST('POINT(0 1)' AS geometry) AS geom1 ;
SELECT 'ST_Azimuth_mixed_srid' , ST_Azimuth(geom1,geom2)
FROM CAST('POINT(0 1)' AS geometry) AS geom1, ST_GeomFromText('POINT(1 0)',4326) AS geom2;
SELECT 'ST_Azimuth_not_point' , ST_Azimuth(geom1,geom2)
FROM CAST('POINT(0 1)' AS geometry) AS geom1, ST_GeomFromText('LINESTRING(1 0 ,2 0)',4326) AS geom2;
SELECT 'ST_Azimuth_null_geom' , ST_Azimuth(geom1,geom2)
FROM CAST('POINT(0 1)' AS geometry) AS geom1, ST_GeomFromText('EMPTY') AS geom2;


-- #1305
SELECT '#1305.1',ST_AsText(ST_Project('POINT(10 10)'::geography, 0, 0));
WITH pts AS ( SELECT 'POINT(0 45)'::geography AS s, 'POINT(45 45)'::geography AS e )
SELECT '#1305.2',abs(ST_Distance(e, ST_Project(s, ST_Distance(s, e), ST_Azimuth(s, e)))) < 0.001 FROM pts;
SELECT '#1305.3',ST_Azimuth('POINT(0 45)'::geography, 'POINT(0 45)'::geography) IS NULL;


-- #1791 --
with inp as ( SELECT
 '010100000000000000004065C0041AD965BE5554C0'::geometry as a,
 '010100000001000000004065C0041AD965BE5554C0'::geometry as b
) SELECT '#1791', round(ST_Azimuth(a,b)*10)/10 from inp;

SELECT '#4718',
	round(degrees(
	ST_Azimuth('POINT(77.46412 37.96999)'::geography,
           'POINT(77.46409 37.96999)'::geography
           ))::numeric,3),
	round(degrees(
	ST_Azimuth('POINT(77.46412 37.96999)'::geography,
           'POINT(77.46429 37.96999)'::geography
           ))::numeric,3);

SELECT
'#4840',
round(degrees(ST_azimuth(C,N)))  AS az_n,
round(degrees(ST_azimuth(C,NE))) AS az_ne,
round(degrees(ST_azimuth(C,E)))  AS az_e,
round(degrees(ST_azimuth(C,SE))) AS az_se,
round(degrees(ST_azimuth(C,S)))  AS az_s,
round(degrees(ST_azimuth(C,SW))) AS az_sw,
round(degrees(ST_azimuth(C,W)))  AS az_w,
round(degrees(ST_azimuth(C,NW))) AS az_nw
FROM (SELECT
'POINT(5 55)'::geography AS C,
'POINT(5 56)'::geography AS N,
'POINT(6 56)'::geography AS NE,
'POINT(6 55)'::geography AS E,
'POINT(6 54)'::geography AS SE,
'POINT(5 54)'::geography AS S,
'POINT(4 54)'::geography AS SW,
'POINT(4 55)'::geography AS W,
'POINT(4 56)'::geography AS NW ) points;

