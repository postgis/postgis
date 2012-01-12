SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT(0 0)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT((0 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT(EMPTY)'
::text as g ) as foo;
-- This is supported for backward compatibility
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT(0 0, 2 0)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT((0 0), (2 0))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT((0 0), (2 0), EMPTY)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING(0 0, 1 1)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING((0 0, 1 1))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING((0 0), (1 1))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING(0 0, 2 0)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING((0 0, 2 0))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING((0 0, 2 0), (1 1, 2 2))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING((0 0, 2 0), (1 1, 2 2), EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING((0 0, 2 0), (1 1, 2 2), (EMPTY))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON((0 0,1 0,1 1,0 1,0 0))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON EMPTY'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON((EMPTY))'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2)))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'GEOMETRYCOLLECTION EMPTY'
::text as g ) as foo;
-- This is supported for backward compatibility
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'GEOMETRYCOLLECTION(EMPTY)'
::text as g ) as foo;
SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'GEOMETRYCOLLECTION((EMPTY))'
::text as g ) as foo;
