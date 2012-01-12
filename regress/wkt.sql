-- POINT --
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
'POINT Z (0 0 0)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT M (0 0 0)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT ZM (0 0 0 0)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT ZM (0 0 0)' -- broken, misses an ordinate value
::text as g ) as foo;




SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POINT((0 0))'
::text as g ) as foo;



-- MULTIPOINT --

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
'MULTIPOINT Z ((0 0 0), (2 0 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT M ((0 0 0), (2 0 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOINT ZM ((0 0 0 0), (2 0 0 0))'
::text as g ) as foo;


-- LINESTRING --

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
'LINESTRING Z (0 0 0, 1 1 0)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING M (0 0 0, 1 1 0)'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'LINESTRING ZM (0 0 0 0, 1 1 0 0)'
::text as g ) as foo;


-- MULTILINESTRING --

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
'MULTILINESTRING Z ((0 0 0, 2 0 0), (1 1 0, 2 2 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING M ((0 0 0, 2 0 0), (1 1 0, 2 2 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTILINESTRING ZM ((0 0 0 0, 2 0 0 0), (1 1 0 0, 2 2 0 0))'
::text as g ) as foo;


-- POLYGON --

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
'POLYGON Z ((0 0 0,10 0 0,10 10 0,0 10 0,0 0 0),(2 2 0,2 5 0,5 5 0,5 2 0,2 2 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON M ((0 0 0,10 0 0,10 10 0,0 10 0,0 0 0),(2 2 0,2 5 0,5 5 0,5 2 0,2 2 0))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'POLYGON ZM ((0 0 0 2,10 0 0 2,10 10 0 2,0 10 0 2,0 0 0 2),(2 2 0 2,2 5 0 2,5 5 0 2,5 2 0 2,2 2 0 2))'
::text as g ) as foo;



-- MULTIPOLYGON --

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
'MULTIPOLYGON Z (((0 0 2,10 0 2,10 10 2,0 10 2,0 0 2),(2 2 2,2 5 2,5 5 2,5 2 2,2 2 2)))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON M (((0 0 2,10 0 2,10 10 2,0 10 2,0 0 2),(2 2 2,2 5 2,5 5 2,5 2 2,2 2 2)))'
::text as g ) as foo;

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'MULTIPOLYGON ZM (((0 0 2 5,10 0 2 5,10 10 2 5,0 10 2 5,0 0 2 5),(2 2 2 5,2 5 2 5,5 5 2 5,5 2 2 5,2 2 2 5)))'
::text as g ) as foo;


-- GEOMETRYCOLLECTION --

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

SELECT g,
      ST_AsText(g::geometry),
      ST_OrderingEquals(g::geometry, St_GeomFromText(ST_AsText(g::geometry))) FROM ( SELECT
'GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(2 2,2 5,5 5,5 2,2 2))),POINT(0 0),MULTILINESTRING((0 0, 2 0),(1 1, 2 2)))'
::text as g ) as foo;

