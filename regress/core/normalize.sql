select 1, ST_AsText(ST_Normalize(
'GEOMETRYCOLLECTION(POINT(2 3),MULTILINESTRING((0 0, 1 1),(2 2, 3 3)))'
::geometry));

select 2, ST_AsText(ST_Normalize(
'POLYGON((0 10,0 0,10 0,10 10,0 10),(4 2,2 2,2 4,4 4,4 2),(6 8,8 8,8 6,6 6,6 8))'
::geometry));

select 3, GeometryType(ST_Normalize('POLYHEDRALSURFACE(((0 0,2 0,0 2,0 0)),((10 10,11 10,10 11,10 10)))'::geometry));
select 4, GeometryType(ST_Normalize('TIN(((0 0,2 0,0 2,0 0)),((10 10,11 10,10 11,10 10)))'::geometry));
select 5, GeometryType(ST_GeometryN(ST_Normalize('GEOMETRYCOLLECTION(POLYHEDRALSURFACE(((0 0,2 0,0 2,0 0))))'::geometry), 1));
with normalized as (
	select ST_Normalize(ST_GeomFromWKB(decode('0110000000010000000111000000010000000500000000000000000000000000000000000000000000000000f03f0000000000000000000000000000f03f000000000000f03f0000000000000000000000000000f03f00000000000000000000000000000000','hex'))) as geom
)
select 6, GeometryType(normalized.geom), GeometryType((dumped).geom) from normalized cross join lateral ST_Dump(normalized.geom) as dumped;
