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
