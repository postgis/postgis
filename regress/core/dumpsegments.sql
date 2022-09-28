SELECT 'dumpsegments01', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'LINESTRING(0 0, 0 9, 9 9, 9 0, 0 0)'::geometry AS geom) AS g
     ) j;

SELECT 'dumpsegments02', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'GEOMETRYCOLLECTION(
          LINESTRING(1 1, 3 3),
          MULTILINESTRING((4 4, 5 5, 6 6, 7 7), (8 8, 9 9, 10 10))
        )'::geometry AS geom
              ) AS g
     ) j;

SELECT 'dumpsegments03', ST_DumpSegments('POLYGON EMPTY'::geometry);
SELECT 'dumpsegments04', ST_DumpSegments('MULTIPOLYGON EMPTY'::geometry);
SELECT 'dumpsegments05', ST_DumpSegments('MULTILINESTRING EMPTY'::geometry);
SELECT 'dumpsegments06', ST_DumpSegments('LINESTRING EMPTY'::geometry);
SELECT 'dumpsegments07', ST_DumpSegments('GEOMETRYCOLLECTION EMPTY'::geometry);

SELECT 'dumpsegments08', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'POLYGON((4 4, 5 5, 6 6, 4 4), (8 8, 9 9, 10 10, 8 8))'::geometry AS geom) AS g
     ) j;

SELECT 'dumpsegments09', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'MULTIPOLYGON(((4 4, 5 5, 6 6, 4 4)), ((8 8, 9 9, 10 10, 8 8)))'::geometry AS geom) AS g
     ) j;

SELECT 'dumpsegments10', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'GEOMETRYCOLLECTION(
          LINESTRING(1 1, 3 3),
          POLYGON((4 4, 5 5, 6 6, 4 4))
        )'::geometry AS geom
              ) AS g
     ) j;

SELECT 'dumpsegments11', path, ST_AsText(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'TRIANGLE((
                0 0,
                0 9,
                9 0,
                0 0
            ))'::geometry AS geom
              ) AS g
     ) j;

SELECT 'dumpsegments12', path, ST_AsEWKT(geom)
FROM (
         SELECT (ST_DumpSegments(g.geom)).*
         FROM (SELECT 'TIN(((
                0 0 0,
                0 0 1,
                0 1 0,
                0 0 0
            )), ((
                0 0 0,
                0 1 0,
                1 1 0,
                0 0 0
            ))
            )'::geometry AS geom
              ) AS g
     ) j;

SELECT '#5240',  dp.path, ST_AsText(dp.geom)
	FROM ( SELECT ST_GeomFromText('MULTIPOLYGON (((9 9, 9 1, 1 1, 2 4, 7 7, 9 9)), EMPTY)', 4326) As the_geom ) As foo1, ST_DumpSegments(foo1.the_geom) AS dp;

SELECT '#5240',  dp.path, ST_AsText(dp.geom)
	FROM ( SELECT ST_GeomFromText('MULTIPOLYGON (EMPTY, ((9 9, 9 1, 1 1, 2 4, 7 7, 9 9)) )', 4326) As the_geom ) As foo1, ST_DumpSegments(foo1.the_geom) AS dp;
