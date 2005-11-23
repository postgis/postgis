--- regression test for postGIS



--- assume datatypes already defined



--- basic datatypes (correct)

select '1',asewkt('POINT( 1 2 )'::GEOMETRY) as geom;
select '2',asewkt('POINT( 1 2 3)'::GEOMETRY) as geom;

select '3',asewkt('LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4)'::GEOMETRY) as geom;
select '4',asewkt('LINESTRING( 0 0 0 , 1 1 1 , 2 2 2 , 3 3 3, 4 4 4)'::GEOMETRY) as geom;
select '5',asewkt('LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15)'::GEOMETRY) as geom;

select '6',asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0) )'::GEOMETRY) as geom;
select '7',asewkt('POLYGON( (0 0 1 , 10 0 1, 10 10 1, 0 10 1, 0 0 1) )'::GEOMETRY) as geom;
select '8',asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) )'::GEOMETRY) as geom;
select '9',asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) )'::GEOMETRY) as geom;
select '10',asewkt('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) )'::GEOMETRY) as geom;
select '11',asewkt('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) )'::GEOMETRY) as geom;

select '12',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 ))'::GEOMETRY);
select '13',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3))'::GEOMETRY);
select '14',asewkt('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY);
select '15',asewkt('GEOMETRYCOLLECTION(LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15))'::GEOMETRY);
select '16',asewkt('GEOMETRYCOLLECTION(POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) ))'::GEOMETRY);
select '17',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),POINT( 1 2 3) )'::GEOMETRY);
select '18',asewkt('GEOMETRYCOLLECTION(LINESTRING( 0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),POINT( 1 2 3) )'::GEOMETRY);
select '19',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 ),LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY);
select '20',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY);
select '21',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),POLYGON( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0) ) )'::GEOMETRY);
select '22',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),POINT( 1 2 3),POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY);

select '23',asewkt('MULTIPOINT( 1 2)'::GEOMETRY) as geom;
select '24',asewkt('MULTIPOINT( 1 2 3)'::GEOMETRY) as geom;
select '25',asewkt('MULTIPOINT( 1 2, 3 4, 5 6)'::GEOMETRY) as geom;
select '26',asewkt('MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13)'::GEOMETRY) as geom;
select '27',asewkt('MULTIPOINT( 1 2 0, 1 2 3, 4 5 0, 6 7 8)'::GEOMETRY) as geom;
select '28',asewkt('MULTIPOINT( 1 2 3,4 5 0)'::GEOMETRY) as geom;

select '29',asewkt('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY) as geom;
select '30',asewkt('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY) as geom;
select '31',asewkt('MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY) as geom;
select '32',asewkt('MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0))'::GEOMETRY) as geom;

select '33',asewkt('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)) )'::GEOMETRY) as geom;
select '34',asewkt('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) )'::GEOMETRY) as geom;
select '35',asewkt('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as geom;


select '36',asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2))'::GEOMETRY);
select '37',asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3))'::GEOMETRY);
select '38',asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);
select '39',asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::GEOMETRY);
select '40',asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0)))'::GEOMETRY);
select '41',asewkt('GEOMETRYCOLLECTION(MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select '42',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),MULTIPOINT( 1 2 3))'::GEOMETRY);
select '43',asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 0, 3 4 0, 5 6 0),POINT( 1 2 3))'::GEOMETRY);
select '44',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3),MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0) ))'::GEOMETRY);
select '45',asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0) ),POINT( 1 2 3))'::GEOMETRY);
select '46',asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3), MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select '47',asewkt('GEOMETRYCOLLECTION(MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ),MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) ),MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);

select '48',asewkt('MULTIPOINT( -1 -2 -3, 5.4 6.6 7.77, -5.4 -6.6 -7.77, 1e6 1e-6 -1e6, -1.3e-6 -1.4e-5 0)'::GEOMETRY) as geom;

select '49', asewkt('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(1 1) ))'::GEOMETRY) as geom;
--- basic datatype (incorrect)

select '50', 'POINT()'::GEOMETRY as geom;
select '51', 'POINT(1)'::GEOMETRY as geom;
select '52', 'POINT(,)'::GEOMETRY as geom;
select '53', 'MULTIPOINT(,)'::GEOMETRY as geom;
select '54', 'POINT(a b)'::GEOMETRY as geom;
select '55', 'MULTIPOINT()'::GEOMETRY as geom;
select '56', asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10) )'::GEOMETRY);
select '57', asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7) )'::GEOMETRY);
select '58', asewkt('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2,) )'::GEOMETRY);


--- funny results

select '59',asewkt('POINT(1 2 3, 4 5 6)'::GEOMETRY);
select '60',asewkt('POINT(1 2 3 4 5 6 7)'::GEOMETRY);
select '61',asewkt('LINESTRING(1 1)'::GEOMETRY);
select '62',asewkt('POINT( 1e700 0)'::GEOMETRY);
select '63',asewkt('POINT( -1e700 0)'::GEOMETRY);
select '64',asewkt('MULTIPOINT(1 1, 2 2'::GEOMETRY);


--- is_same() testing

select '65','POINT(1 1)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '66','POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '67','POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1 0)'::GEOMETRY as bool;

select '68','MULTIPOINT(1 1,2 2)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;
select '69','MULTIPOINT(2 2, 1 1)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;

select '70','GEOMETRYCOLLECTION(POINT( 1 2 3),POINT(4 5 6))'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '71','MULTIPOINT(4 5 6, 1 2 3)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '72','MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '73','MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(MULTIPOINT(1 2 3, 4 5 6))'::GEOMETRY as bool;


select '74','LINESTRING(1 1,2 2)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '75','LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(2 2, 1 1)'::GEOMETRY as bool;
select '76','LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(1 1, 2 2, 3 3)'::GEOMETRY as bool;

--- operator testing (testing is on the BOUNDING BOX (2d), not the actual geometries)

select '77','POINT(1 1)'::GEOMETRY &< 'POINT(1 1)'::GEOMETRY as bool;
select '78','POINT(1 1)'::GEOMETRY &< 'POINT(2 1)'::GEOMETRY as bool;
select '79','POINT(2 1)'::GEOMETRY &< 'POINT(1 1)'::GEOMETRY as bool;

select '80','POINT(1 1)'::GEOMETRY << 'POINT(1 1)'::GEOMETRY as bool;
select '81','POINT(1 1)'::GEOMETRY << 'POINT(2 1)'::GEOMETRY as bool;
select '82','POINT(2 1)'::GEOMETRY << 'POINT(1 1)'::GEOMETRY as bool;


select '83','POINT(1 1)'::GEOMETRY &> 'POINT(1 1)'::GEOMETRY as bool;
select '84','POINT(1 1)'::GEOMETRY &> 'POINT(2 1)'::GEOMETRY as bool;
select '85','POINT(2 1)'::GEOMETRY &> 'POINT(1 1)'::GEOMETRY as bool;

select '86','POINT(1 1)'::GEOMETRY >> 'POINT(1 1)'::GEOMETRY as bool;
select '87','POINT(1 1)'::GEOMETRY >> 'POINT(2 1)'::GEOMETRY as bool;
select '88','POINT(2 1)'::GEOMETRY >> 'POINT(1 1)'::GEOMETRY as bool;

-- overlap

select '89','POINT(1 1)'::GEOMETRY && 'POINT(1 1)'::GEOMETRY as bool;
select '90','POINT(1 1)'::GEOMETRY && 'POINT(2 2)'::GEOMETRY as bool;
select '91','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 1, 2 2)'::GEOMETRY as bool;
select '92','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1.0001 1, 2 2)'::GEOMETRY as bool;
select '93','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 1.0001, 2 2)'::GEOMETRY as bool;
select '94','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1 0, 2 2)'::GEOMETRY as bool;
select '95','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(1.0001 0, 2 2)'::GEOMETRY as bool;

select '96','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(0 1, 1 2)'::GEOMETRY as bool;
select '97','MULTIPOINT(0 0, 1 1)'::GEOMETRY && 'MULTIPOINT(0 1.0001, 1 2)'::GEOMETRY as bool;

--- contains 

select '98','MULTIPOINT(0 0, 10 10)'::GEOMETRY ~ 'MULTIPOINT(5 5, 7 7)'::GEOMETRY as bool;
select '99','MULTIPOINT(5 5, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select '100','MULTIPOINT(0 0, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select '101','MULTIPOINT(-0.0001 0, 7 7)'::GEOMETRY ~ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;

--- contained by 

select '102','MULTIPOINT(0 0, 10 10)'::GEOMETRY @ 'MULTIPOINT(5 5, 7 7)'::GEOMETRY as bool;
select '103','MULTIPOINT(5 5, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select '104','MULTIPOINT(0 0, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;
select '105','MULTIPOINT(-0.0001 0, 7 7)'::GEOMETRY @ 'MULTIPOINT(0 0, 10 10)'::GEOMETRY as bool;



--- function testing
--- conversion function

select '106',box3d('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as bvol;

-- box3d only type is only used for indexing -- NEVER use one yourself
select '107',asewkt(geometry('BOX3D(0 0 0, 7 7 7 )'::BOX3D));

--- debug function testing

select '108',npoints('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as value;
select '109',npoints('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3))'::GEOMETRY) as value;

select '110', nrings('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '111', mem_size(dropBBOX('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY)) as value;


select '112',numgeometries('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;

--- geo ops

select '113', area2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '114', perimeter2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '115', perimeter3d('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7  0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select '116', length2d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '117', length3d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '118', length3d('MULTILINESTRING((0 0 0, 1 1 1),(0 0 0, 1 1 1, 2 2 2) )'::GEOMETRY) as value;

---selection

select '119', within('LINESTRING(-1 -1, -1 101, 101 101, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3D);
select '120', within('LINESTRING(-1 -1, -1 100, 101 100, 101 -1)'::GEOMETRY,'BOX3D(0 0, 100 100)'::BOX3D);


--- TOAST testing

-- create a table with data that will be TOASTed (even after compression)
create table TEST(a GEOMETRY, b GEOMETRY);
\i regress_biginsert.sql


---test basic ops on this 

select '121',box3d(a) as box3d_a, box3d(b) as box3d_b from TEST;

select '122',a <<b from TEST;
select '123',a &<b from TEST;
select '124',a >>b from TEST;
select '125',a &>b from TEST;

select '126',a ~= b from TEST;
select '127',a @ b from TEST;
select '128',a ~ b from TEST; 

select '129', mem_size(dropBBOX(a)), mem_size(dropBBOX(b)) from TEST;

select '130', geosnoop('POLYGON((0 0, 1 1, 0 0))');

select '131', X('POINT(1 2)');
select '132', Y('POINT(1 2)');
select '133', Z('POINT(1 2)');

select '134', distance('POINT(1 2)', 'POINT(1 2)');
select '135', distance('POINT(5 0)', 'POINT(10 12)');

select '136', distance('POINT(0 0)', translate('POINT(0 0)', 5, 12, 0));

select '137', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY)'::geometry);
select '138', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry);

select '139', asewkt(multi(setsrid('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry, 2)));
select '140', asewkt(multi(setsrid('POINT(2 2)'::geometry, 3)));
select '141', asewkt(multi(setsrid('LINESTRING(2 2, 3 3)'::geometry, 4)));
select '142', asewkt(multi(setsrid('LINESTRING(2 2, 3 3)'::geometry, 5)));
select '143', asewkt(multi(setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));

select '144', asewkt(force_3dm('POINT(1 2 3)'));
select '145', asewkt(force_3dz('POINTM(1 2 3)'));
select '146', asewkt(force_4d('POINTM(1 2 3)'));
select '147', asewkt(force_4d('POINT(1 2 3)'));

select '144', asewkt(linemerge('GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), LINESTRING(4 4, 1 1), LINESTRING(-5 -5, 0 0))'::geometry));

-- Drop test table
DROP table test;
