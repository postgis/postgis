--- regression test for postGIS's LWGEOM



--- assume datatypes already defined



--- basic datatypes (correct)

select  astext('POINT( 1 2 )'::LWGEOM) as geom;
select  astext('POINT( 1 2 3)'::LWGEOM) as geom;

select  astext('LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4)'::LWGEOM) as geom;
select  astext('LINESTRING( 0 0 0 , 1 1 1 , 2 2 2 , 3 3 3, 4 4 4)'::LWGEOM) as geom;
select  astext('LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15)'::LWGEOM) as geom;

select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0) )'::LWGEOM) as geom;
select  astext('POLYGON( (0 0 1 , 10 0 1, 10 10 1, 0 10 1, 0 0 1) )'::LWGEOM) as geom;
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) )'::LWGEOM) as geom;
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) )'::LWGEOM) as geom;
select  astext('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) )'::LWGEOM) as geom;
select  astext('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) )'::LWGEOM) as geom;

select  astext('GEOMETRYCOLLECTION(POINT( 1 2 ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 3))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 4),POINT( 1 2 3) )'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4),POINT( 1 2) )'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 ),LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4) )'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 4),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 4),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),POLYGON( (0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7) )  )'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 4),POINT( 1 2 3),POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::LWGEOM) as geom;

select  astext('MULTIPOINT( 1 2)'::LWGEOM) as geom;
select  astext('MULTIPOINT( 1 2 3)'::LWGEOM) as geom;
select  astext('MULTIPOINT( 1 2, 3 4, 5 6)'::LWGEOM) as geom;
select  astext('MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13)'::LWGEOM) as geom;
select  astext('MULTIPOINT( 1 2 7, 1 2 3, 4 5 9, 6 7 8)'::LWGEOM) as geom;
select  astext('MULTIPOINT( 1 2 3,4 5 7)'::LWGEOM) as geom;

select  astext('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) )'::LWGEOM) as geom;
select  astext('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4))'::LWGEOM) as geom;
select  astext('MULTILINESTRING( (0 0 7, 1 1 7, 2 2 7, 3 3 7, 4 4 7),(0 0 7, 1 1 7, 2 2 7, 3 3  7, 4 4 7),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::LWGEOM) as geom;
select  astext('MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 7, 1 1 7, 2 2 7, 3 3  7, 4 4 7),(0 0 7, 1 1 7, 2 2 7, 3 3  7, 4 4 7))'::LWGEOM) as geom;

select  astext('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)) )'::LWGEOM) as geom;
select  astext('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) )'::LWGEOM) as geom;
select  astext('MULTIPOLYGON(  ((0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7)),( (0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7),(5 5 7, 7 5 7, 7 7 7, 5 7 7, 5 5 7) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::LWGEOM) as geom;


select  astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 7, 1 1 7, 2 2 7, 3 3 7, 4 4 7),(0 0 7, 1 1 7, 2 2 7, 3 3 7, 4 4 7)))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTIPOLYGON(  ((0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7)),( (0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7),(5 5 7, 7 5 7, 7 7 7, 5 7 7, 5 5 7) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 7),MULTIPOINT( 1 2 3))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3, 3 4 3, 5 6 3),POINT( 1 2 3))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2),MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ),POINT( 1 2))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(POINT( 1 2 3), MULTIPOLYGON(  ((0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7)),( (0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7),(5 5 7, 7 5 7, 7 7 7, 5 7 7, 5 5 7) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::LWGEOM) as geom;
select  astext('GEOMETRYCOLLECTION(MULTIPOLYGON(  ((0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7)),( (0 0 7, 10 0 7, 10 10 7, 0 10 7, 0 0 7),(5 5 7, 7 5 7, 7 7  7, 5 7 7, 5 5 7) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ),MULTILINESTRING( (0 0 7, 1 1 7, 2 2 7, 3 3  7, 4 4 7),(0 0 7, 1 1 7, 2 2 7, 3 3 7 , 4 4 7),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) ),MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::LWGEOM) as geom;

select  astext('MULTIPOINT( -1 -2 -3, 5.4 6.6 7.77, -5.4 -6.6 -7.77, 1e6 1e-6 -1e6, -1.3e-6 -1.4e-5)'::LWGEOM) as geom;

--- basic datatype (incorrect)

begin;
select  astext('POINT()'::LWGEOM) as geom;
rollback;
begin;
select  astext('POINT(1)'::LWGEOM) as geom;
rollback;
begin;
select  astext('POINT(,)'::LWGEOM) as geom;
rollback;
begin;
select  astext('MULTIPOINT(,)'::LWGEOM) as geom;
rollback;
begin;
select  astext('POINT(a b)'::LWGEOM) as geom;
rollback;
begin;
select  astext('MULTIPOINT()'::LWGEOM) as geom;
rollback;
begin;
select  astext('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(1 1) ))'::LWGEOM) as geom;
rollback;
begin;
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10) )'::LWGEOM) as geom;
rollback;
begin;
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7) )'::LWGEOM) as geom;
rollback;
begin;
select  astext('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2,) )'::LWGEOM) as geom;
rollback;


--- funny results

select  astext('POINT(1 2 3, 4 5 6)'::LWGEOM) as geom;
select  astext('POINT(1 2 3 4 5 6 7)'::LWGEOM) as geom;

select  astext('LINESTRING(1 1)'::LWGEOM) as geom;
select  astext('POINT( 1e700 0)'::LWGEOM) as geom;
select  astext('POINT( -1e700 0)'::LWGEOM) as geom;

select  astext('MULTIPOINT(1 1, 2 2'::LWGEOM) as geom;


--- is_same() testing

select 'POINT(1 1)'::LWGEOM ~= 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1 0)'::LWGEOM ~= 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1 0)'::LWGEOM ~= 'POINT(1 1 0)'::LWGEOM as bool;

select 'MULTIPOINT(1 1,2 2)'::LWGEOM ~= 'MULTIPOINT(1 1,2 2)'::LWGEOM as bool;
select 'MULTIPOINT(2 2, 1 1)'::LWGEOM ~= 'MULTIPOINT(1 1,2 2)'::LWGEOM as bool;

select 'GEOMETRYCOLLECTION(POINT( 1 2 3),POINT(4 5 6))'::LWGEOM ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::LWGEOM as bool;
select 'MULTIPOINT(4 5 6, 1 2 3)'::LWGEOM ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::LWGEOM as bool;
select 'MULTIPOINT(1 2 3, 4 5 6)'::LWGEOM ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::LWGEOM as bool;
select 'MULTIPOINT(1 2 3, 4 5 6)'::LWGEOM ~= 'GEOMETRYCOLLECTION(MULTIPOINT(1 2 3, 4 5 6))'::LWGEOM as bool;


select 'LINESTRING(1 1,2 2)'::LWGEOM ~= 'POINT(1 1)'::LWGEOM as bool;
select 'LINESTRING(1 1, 2 2)'::LWGEOM ~= 'LINESTRING(2 2, 1 1)'::LWGEOM as bool;
select 'LINESTRING(1 1, 2 2)'::LWGEOM ~= 'LINESTRING(1 1, 2 2, 3 3)'::LWGEOM as bool;

--- operator testing (testing is on the BOUNDING BOX (2d), not the actual geometries)

select 'POINT(1 1)'::LWGEOM &< 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1)'::LWGEOM &< 'POINT(2 1)'::LWGEOM as bool;
select 'POINT(2 1)'::LWGEOM &< 'POINT(1 1)'::LWGEOM as bool;

select 'POINT(1 1)'::LWGEOM << 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1)'::LWGEOM << 'POINT(2 1)'::LWGEOM as bool;
select 'POINT(2 1)'::LWGEOM << 'POINT(1 1)'::LWGEOM as bool;


select 'POINT(1 1)'::LWGEOM &> 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1)'::LWGEOM &> 'POINT(2 1)'::LWGEOM as bool;
select 'POINT(2 1)'::LWGEOM &> 'POINT(1 1)'::LWGEOM as bool;

select 'POINT(1 1)'::LWGEOM >> 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1)'::LWGEOM >> 'POINT(2 1)'::LWGEOM as bool;
select 'POINT(2 1)'::LWGEOM >> 'POINT(1 1)'::LWGEOM as bool;

-- overlap

select 'POINT(1 1)'::LWGEOM && 'POINT(1 1)'::LWGEOM as bool;
select 'POINT(1 1)'::LWGEOM && 'POINT(2 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(1 1, 2 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(1.0001 1, 2 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(1 1.0001, 2 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(1 0, 2 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(1.0001 0, 2 2)'::LWGEOM as bool;

select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(0 1, 1 2)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 1 1)'::LWGEOM && 'MULTIPOINT(0 1.0001, 1 2)'::LWGEOM as bool;

--- contained by 

select 'MULTIPOINT(0 0, 10 10)'::LWGEOM ~ 'MULTIPOINT(5 5, 7 7)'::LWGEOM as bool;
select 'MULTIPOINT(5 5, 7 7)'::LWGEOM ~ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 7 7)'::LWGEOM ~ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;
select 'MULTIPOINT(-0.0001 0, 7 7)'::LWGEOM ~ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;


--- contains 
select 'MULTIPOINT(0 0, 10 10)'::LWGEOM @ 'MULTIPOINT(5 5, 7 7)'::LWGEOM as bool;
select 'MULTIPOINT(5 5, 7 7)'::LWGEOM @ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;
select 'MULTIPOINT(0 0, 7 7)'::LWGEOM @ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;
select 'MULTIPOINT(-0.0001 0, 7 7)'::LWGEOM @ 'MULTIPOINT(0 0, 10 10)'::LWGEOM as bool;




