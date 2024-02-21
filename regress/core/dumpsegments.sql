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
FROM ST_DumpSegments(ST_GeomFromText('MULTIPOLYGON (((9 9, 9 1, 1 1, 2 4, 7 7, 9 9)), EMPTY)', 4326)) AS dp;

SELECT '#5240',  dp.path, ST_AsText(dp.geom)
FROM ST_DumpSegments(ST_GeomFromText('MULTIPOLYGON (EMPTY, ((9 9, 9 1, 1 1, 2 4, 7 7, 9 9)) )', 4326)) AS dp;

SELECT 'dumpsegments13', path, ST_AsText(geom)
FROM st_dumpsegments(
'MULTICURVE(COMPOUNDCURVE(
  LINESTRING(1 1, 2 2, 3 3, 4 4, 5 5, 6 6, 7 7),
  CIRCULARSTRING(7 7, 6 6, 5 5, 4 4, 3 3, 2 2, 1 1)),
  CIRCULARSTRING EMPTY,
  COMPOUNDCURVE(
  LINESTRING(1 1, 2 2, 3 3, 4 4, 5 5, 6 6, 7 7),
  CIRCULARSTRING(7 7, 6 6, 5 5, 4 4, 3 3, 2 2, 1 1)),
  LINESTRING EMPTY)'
);

SELECT 'dumpsegments14', path, ST_AsText(geom)
FROM st_dumpsegments(
'MULTISURFACE(CURVEPOLYGON(COMPOUNDCURVE(
  LINESTRING(1 1, 2 2, 3 3, 4 4, 5 5, 6 6, 7 7),
  CIRCULARSTRING(7 7, 6 6, 5 5, 4 4, 3 3, 2 2, 1 1))
  ))'
);

-- Check dumping a degenerate (even number of points) circular string
SELECT 'dumpsegments15', path, st_astext(geom)
FROM st_dumpsegments(
'010800000006000000000000000000f03f000000000000f03f0000000000000040000000000000004000000000000008400000000000000840000000000000104000000000000010400000000000001440000000000000144000000000000018400000000000001840'
);

-- Check dumping a compound that contains a degenerate circular
-- string
SELECT 'dumpsegments16', path, st_astext(geom)
FROM st_dumpsegments(
'010900000002000000010200000007000000000000000000f03f000000000000f03f00000000000000400000000000000040000000000000084000000000000008400000000000001040000000000000104000000000000014400000000000001440000000000000184000000000000018400000000000001c400000000000001c400108000000060000000000000000001c400000000000001c400000000000001840000000000000184000000000000014400000000000001440000000000000104000000000000010400000000000000840000000000000084000000000000000400000000000000040'
);
