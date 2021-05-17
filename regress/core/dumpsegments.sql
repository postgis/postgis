SELECT 'dumpsegments01', path, ST_AsText(geom)
FROM (
  SELECT (ST_DumpPoints(g.geom)).*
  FROM
    (SELECT
       'LINESTRING (
                0 0,
                0 9,
                9 9,
                9 0,
                0 0
            )'::geometry AS geom
    ) AS g
  ) j;

SELECT 'dumpsegments02', path, ST_AsText(geom)
FROM (
  SELECT (ST_DumpPoints(g.geom)).*
  FROM
    (SELECT
       'GEOMETRYCOLLECTION(
          LINESTRING(1 1, 3 3),
          MULTILINESTRING((4 4, 5 5, 6 6, 7 7), (8 8, 9 9, 10 10))
        )'::geometry AS geom
    ) AS g
  ) j;

SELECT 'dumpsegments03', ST_DumpPoints('POLYGON EMPTY'::geometry);
SELECT 'dumpsegments04', ST_DumpPoints('MULTIPOLYGON EMPTY'::geometry);
SELECT 'dumpsegments05', ST_DumpPoints('MULTILINESTRING EMPTY'::geometry);
SELECT 'dumpsegments06', ST_DumpPoints('LINESTRING EMPTY'::geometry);
SELECT 'dumpsegments07', ST_DumpPoints('GEOMETRYCOLLECTION EMPTY'::geometry);
