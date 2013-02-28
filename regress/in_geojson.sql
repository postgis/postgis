-- FromGeoJSON
select 'geomfromgeojson_01',st_asewkt(st_geomfromgeojson(st_asgeojson('SRID=3005;MULTIPOINT(1 1, 1 1)')));
select 'geomfromgeojson_02',st_astext(st_geomfromgeojson(st_asgeojson('SRID=3005;MULTIPOINT(1 1, 1 1)')));
select 'geomfromgeojson_03',st_astext(st_geomfromgeojson(st_asgeojson('POINT(1 1)')));
select 'geomfromgeojson_04',st_astext(st_geomfromgeojson(st_asgeojson('LINESTRING(0 0,1 1)')));
select 'geomfromgeojson_05',st_astext(st_geomfromgeojson(st_asgeojson('POLYGON((0 0,1 1,1 0,0 0))')));
select 'geomfromgeojson_06',st_astext(st_geomfromgeojson(st_asgeojson('MULTIPOLYGON(((0 0,1 1,1 0,0 0)))')));

-- #1434
select '#1434: Next two errors';
select '#1434.1',ST_GeomFromGeoJSON('{ "type": "Point", "crashme": [100.0, 0.0] }');
select '#1434.2',ST_GeomFromGeoJSON('crashme');;

-- #2130 --
SELECT '#2130', ST_NPoints(ST_GeomFromGeoJSON('{"type":"MultiPolygon","coordinates":[[[[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,32],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,32],[-117,32],[-117,32],[-117,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-116,32],[-117,32],[-117,32],[-117,32],[-117,32]],[[-117,33],[-117,33],[-117,33],[-117,33],[-117,33],[-117,32],[-117,33]]]]}'));

-- #2216 --
SELECT '#2216', ST_NPoints(ST_GeomFromGeoJSON('{"type":"MultiPolygon","coordinates":[[[[4,0],[0,-4],[-4,0],[0,4],[4,0]],[[2,0],[0,2],[-2,0],[0,-2],[2,0]]],[[[24,0],[20,-4],[16,0],[20,4],[24,0]],[[22,0],[20,2],[18,0],[20,-2],[22,0]]],[[[44,0],[40,-4],[36,0],[40,4],[44,0]],[[42,0],[40,2],[38,0],[40,-2],[42,0]]]]}'));
