
--POINT without id
select g,encode(ST_AsTWKB(g::geometry,precission),'hex') from
(select 'POINT(1 1)'::text g, 0 precission) foo;
--POINT with id
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'POINT(1 1)'::text g, 0 precission, 1 id) foo;
--POINT with multibyte values and negative values
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'POINT(78 -78)'::text g, 0 precission, -10 id) foo;
--POINT rounding to 2 decimals in coordinates
select g,encode(ST_AsTWKB(g::geometry,precission),'hex') from
(select 'POINT(123.456789 987.654321)'::text g, 2 precission) foo;

--LINESTRING
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'LINESTRING(120 10, -50 20, 300 -2)'::text g, 0 precission, 300 id) foo;
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'LINESTRING(120 10, -50 20, 300 -2)'::text g, 2 precission, 300 id) foo;
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'LINESTRING(120.54 10.78, -50.2 20.878, 300.789 -21)'::text g, 0 precission, 300 id) foo;

--POLYGON
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'POLYGON((1 1, 1 2, 2 2, 2 1, 1 1))'::text g, 0 precission, 1 id) foo;
--POLYGON with hole
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'POLYGON((1 1, 1 20, 20 20, 20 1, 1 1),(3 3,3 4, 4 4,4 3,3 3))'::text g, 0 precission, 1 id) foo;

--MULTIPOINT
select g,encode(ST_AsTWKB(g::geometry,precission),'hex') from
(select 'MULTIPOINT((1 1),(2 2))'::text g, 0 precission) foo;

--MULTILINESTRING
select g,encode(ST_AsTWKB(g::geometry,precission),'hex') from
(select 'MULTILINESTRING((1 1,1 2,2 2),(3 3,3 4,4 4))'::text g, 0 precission) foo;

--MULTIPOLYGON
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'MULTIPOLYGON(((1 1, 1 2, 2 2, 2 1, 1 1)),((3 3,3 4,4 4,4 3,3 3)))'::text g, 0 precission, 1 id) foo;
--MULTIPOLYGON with hole
select g,encode(ST_AsTWKB(g::geometry,precission,id),'hex') from
(select 'MULTIPOLYGON(((1 1, 1 20, 20 20, 20 1, 1 1),(3 3,3 4, 4 4,4 3,3 3)),((-1 1, -1 20, -20 20, -20 1, -1 1),(-3 3,-3 4, -4 4,-4 3,-3 3)))'::text g, 0 precission, 1 id) foo;


--GEOMETRYCOLLECTION
select st_astext(st_collect(g::geometry)), encode(ST_AsTWKB(ST_Collect(g::geometry),0),'hex') from
(
select 'MULTIPOINT((1 1),(2 2))'::text g
union all
select 'POINT(78 -78)'::text g
union all
select 'POLYGON((1 1, 1 2, 2 2, 2 1, 1 1))'::text g
) foo;


--Aggregated geoemtries with preserved id
select st_astext(st_collect(g::geometry)), encode(ST_AsTWKBagg(g::geometry,0,id),'hex') from
(
select 'POINT(1 1)'::text g, 3 id
union all
select 'POINT(2 2)'::text g, 2 id
) foo;

--Aggregated geoemtries with preserved id
select st_astext(st_collect(g::geometry)), encode(ST_AsTWKBagg(g::geometry,0,id),'hex') from
(
select 'MULTIPOINT((1 1),(2 2))'::text g, 3 id
union all
select 'POINT(78 -78)'::text g, 4324 id
union all
select 'POLYGON((1 1, 1 2, 2 2, 2 1, 1 1))'::text g, -543 id
) foo;



