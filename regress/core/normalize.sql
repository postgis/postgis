select 1, ST_AsText(ST_Normalize(
'GEOMETRYCOLLECTION(POINT(2 3),MULTILINESTRING((0 0, 1 1),(2 2, 3 3)))'
::geometry));

select 2, ST_AsText(ST_Normalize(
'POLYGON((0 10,0 0,10 0,10 10,0 10),(4 2,2 2,2 4,4 4,4 2),(6 8,8 8,8 6,6 6,6 8))'
::geometry));

select 3, GeometryType(ST_Normalize(
'POLYHEDRALSURFACE(((0 0,2 0,0 2,0 0)),((10 10,11 10,10 11,10 10)))'
::geometry));

select 4, GeometryType(ST_Normalize(
'TIN(((0 0,2 0,0 2,0 0)),((10 10,11 10,10 11,10 10)))'
::geometry));

select 5, GeometryType(ST_GeometryN(ST_Normalize(
'GEOMETRYCOLLECTION(POLYHEDRALSURFACE(((0 0,2 0,0 2,0 0))))'
::geometry), 1));
