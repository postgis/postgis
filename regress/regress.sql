--- regression test for postGIS



--- assume datatypes already defined



--- basic datatypes (correct)

select astext('POINT( 1 2 )'::GEOMETRY) as geom;
select astext('POINT( 1 2 3)'::GEOMETRY) as geom;

select astext('LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4)'::GEOMETRY) as geom;
select astext('LINESTRING( 0 0 0 , 1 1 1 , 2 2 2 , 3 3 3, 4 4 4)'::GEOMETRY) as geom;
select astext('LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15)'::GEOMETRY) as geom;

select astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0) )'::GEOMETRY) as geom;
select astext('POLYGON( (0 0 1 , 10 0 1, 10 10 1, 0 10 1, 0 0 1) )'::GEOMETRY) as geom;
select astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) )'::GEOMETRY) as geom;
select astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) )'::GEOMETRY) as geom;
select astext('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) )'::GEOMETRY) as geom;
select astext('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) )'::GEOMETRY) as geom;

select astext('GEOMETRYCOLLECTION(POINT( 1 2 ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 3))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),POINT( 1 2 3) )'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4),POINT( 1 2 3) )'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0) )  )'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),POINT( 1 2 3),POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY);

select astext('MULTIPOINT( 1 2)'::GEOMETRY) as geom;
select astext('MULTIPOINT( 1 2 3)'::GEOMETRY) as geom;
select astext('MULTIPOINT( 1 2, 3 4, 5 6)'::GEOMETRY) as geom;
select astext('MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13)'::GEOMETRY) as geom;
select astext('MULTIPOINT( 1 2, 1 2 3, 4 5, 6 7 8)'::GEOMETRY) as geom;
select astext('MULTIPOINT( 1 2 3,4 5)'::GEOMETRY) as geom;

select astext('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY) as geom;
select astext('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY) as geom;
select astext('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY) as geom;
select astext('MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY) as geom;

select astext('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)) )'::GEOMETRY) as geom;
select astext('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) )'::GEOMETRY) as geom;
select astext('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as geom;


select astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4)))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 ),MULTIPOINT( 1 2 3))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTIPOINT( 1 2, 3 4, 5 6),POINT( 1 2 3))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 3),MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ),POINT( 1 2 3))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(POINT( 1 2 3), MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select astext('GEOMETRYCOLLECTION(MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ),MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) ),MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);

select astext('MULTIPOINT( -1 -2 -3, 5.4 6.6 7.77, -5.4 -6.6 -7.77, 1e6 1e-6 -1e6, -1.3e-6 -1.4e-5)'::GEOMETRY) as geom;

select  astext('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(1 1) ))'::GEOMETRY) as geom;
--- basic datatype (incorrect)

select  'POINT()'::GEOMETRY as geom;
select  'POINT(1)'::GEOMETRY as geom;
select  'POINT(,)'::GEOMETRY as geom;
select  'MULTIPOINT(,)'::GEOMETRY as geom;
select  'POINT(a b)'::GEOMETRY as geom;
select  'MULTIPOINT()'::GEOMETRY as geom;
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10) )'::GEOMETRY);
select  astext('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7) )'::GEOMETRY);
select  astext('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2,) )'::GEOMETRY);


--- funny results

select astext('POINT(1 2 3, 4 5 6)'::GEOMETRY);
select astext('POINT(1 2 3 4 5 6 7)'::GEOMETRY);
select astext('LINESTRING(1 1)'::GEOMETRY);
select astext('POINT( 1e700 0)'::GEOMETRY);
select astext('POINT( -1e700 0)'::GEOMETRY);
select astext('MULTIPOINT(1 1, 2 2'::GEOMETRY);


--- is_same() testing

select 'POINT(1 1)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1 0)'::GEOMETRY as bool;

select 'MULTIPOINT(1 1,2 2)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(2 2, 1 1)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;

select 'GEOMETRYCOLLECTION(POINT( 1 2 3),POINT(4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select 'MULTIPOINT(4 5 6, 1 2 3)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select 'MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select 'MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(MULTIPOINT(1 2 3, 4 5 6))'::GEOMETRY as bool;


select 'LINESTRING(1 1,2 2)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select 'LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(2 2, 1 1)'::GEOMETRY as bool;
select 'LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(1 1, 2 2, 3 3)'::GEOMETRY as bool;

--- operator testing (testing is on the BOUNDING BOX (2d), not the actual geometries)

select 'POINT(1 1)'::GEOMETRY &< 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1)'::GEOMETRY &< 'POINT(2 1)'::GEOMETRY as bool;
select 'POINT(2 1)'::GEOMETRY &< 'POINT(1 1)'::GEOMETRY as bool;

select 'POINT(1 1)'::GEOMETRY << 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1)'::GEOMETRY << 'POINT(2 1)'::GEOMETRY as bool;
select 'POINT(2 1)'::GEOMETRY << 'POINT(1 1)'::GEOMETRY as bool;


select 'POINT(1 1)'::GEOMETRY &> 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1)'::GEOMETRY &> 'POINT(2 1)'::GEOMETRY as bool;
select 'POINT(2 1)'::GEOMETRY &> 'POINT(1 1)'::GEOMETRY as bool;

select 'POINT(1 1)'::GEOMETRY >> 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1)'::GEOMETRY >> 'POINT(2 1)'::GEOMETRY as bool;
select 'POINT(2 1)'::GEOMETRY >> 'POINT(1 1)'::GEOMETRY as bool;

-- overlap

select 'POINT(1 1)'::GEOMETRY && 'POINT(1 1)'::GEOMETRY as bool;
select 'POINT(1 1)'::GEOMETRY && 'POINT(2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 1, 2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1.0001 1, 2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 1.0001, 2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 0, 2 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1.0001 0, 2 2)'::GEOMETRY as bool;

select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(0 1, 1 2)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(0 1.0001, 1 2)'::GEOMETRY as bool;

--- contained by 

select 'MULTIPOINT(0 0, 10 10)'::GEOMETRY ~ 'MULTIPOINT(5 5, 7 7)'::GEOMETRY as bool;
select 'MULTIPOINT(5 5, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select 'MULTIPOINT(-0.0001 0, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;


--- contains 
select 'MULTIPOINT(0 0, 10 10)'::GEOMETRY @ 'MULTIPOINT(5 5, 7 7)'::GEOMETRY as bool;
select 'MULTIPOINT(5 5, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select 'MULTIPOINT(0 0, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select 'MULTIPOINT(-0.0001 0, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;



--- function testing
--- conversion function

select box3d('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as bvol;

-- box3d only type is only used for indexing -- NEVER use one yourself
select geometry('BOX3D(0 0 0, 7 7 7 )'::BOX3D) as a_geom;

--- debug function testing

select npoints('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as value;
select npoints('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3))'::GEOMETRY) as value;

select  nrings('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;

select  mem_size('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select numb_sub_objs('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;

select summary('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;


--- geo ops

select  area2d('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;

select  perimeter2d('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;

select  perimeter3d('MULTIPOLYGON(  ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7  1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select  length2d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select  length3d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select  length3d('MULTILINESTRING((0 0 0, 1 1 1),(0 0 0, 1 1 1, 2 2 2) )'::GEOMETRY) as value;

---selection

select  truly_inside('LINESTRING(-1 -1, -1 101, 101 101, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3d) as value;
select  truly_inside('LINESTRING(-1 -1, -1 100, 101 100, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3d) as value;


--- TOAST testing

-- create a table with data that will be TOASTed (even after compression)

\set ECHO none
create table TEST(a GEOMETRY, b GEOMETRY);
\i regress_biginsert.sql
\set ECHO all


---test basic ops on this 

select box3d(a) as box3d_a, box3d(b) as box3d_b from TEST;

select a <<b from TEST;
select a &<b from TEST;
select a >>b from TEST;
select a &>b from TEST;

select a  ~= b from TEST;
select a @ b from TEST;
select a ~ b from TEST; 

select  mem_size(a), mem_size(b) from TEST;

drop table TEST;
