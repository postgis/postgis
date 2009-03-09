--- regression test for postGIS



--- assume datatypes already defined



--- basic datatypes (correct)

select '1',ST_asewkt('POINT( 1 2 )'::GEOMETRY) as geom;
select '2',ST_asewkt('POINT( 1 2 3)'::GEOMETRY) as geom;

select '3',ST_asewkt('LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4)'::GEOMETRY) as geom;
select '4',ST_asewkt('LINESTRING( 0 0 0 , 1 1 1 , 2 2 2 , 3 3 3, 4 4 4)'::GEOMETRY) as geom;
select '5',ST_asewkt('LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15)'::GEOMETRY) as geom;

select '6',ST_asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0) )'::GEOMETRY) as geom;
select '7',ST_asewkt('POLYGON( (0 0 1 , 10 0 1, 10 10 1, 0 10 1, 0 0 1) )'::GEOMETRY) as geom;
select '8',ST_asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) )'::GEOMETRY) as geom;
select '9',ST_asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) )'::GEOMETRY) as geom;
select '10',ST_asewkt('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) )'::GEOMETRY) as geom;
select '11',ST_asewkt('POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) )'::GEOMETRY) as geom;

select '12',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 ))'::GEOMETRY);
select '13',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3))'::GEOMETRY);
select '14',ST_asewkt('GEOMETRYCOLLECTION(LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY);
select '15',ST_asewkt('GEOMETRYCOLLECTION(LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15))'::GEOMETRY);
select '16',ST_asewkt('GEOMETRYCOLLECTION(POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1 , 5 7 1, 5 5 1) ))'::GEOMETRY);
select '17',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),POINT( 1 2 3) )'::GEOMETRY);
select '18',ST_asewkt('GEOMETRYCOLLECTION(LINESTRING( 0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),POINT( 1 2 3) )'::GEOMETRY);
select '19',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 ),LINESTRING( 0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY);
select '20',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY);
select '21',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0 ),POINT( 1 2 3),LINESTRING( 1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),POLYGON( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0) ) )'::GEOMETRY);
select '22',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),POINT( 1 2 3),POLYGON( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY);

select '23',ST_asewkt('MULTIPOINT( 1 2)'::GEOMETRY) as geom;
select '24',ST_asewkt('MULTIPOINT( 1 2 3)'::GEOMETRY) as geom;
select '25',ST_asewkt('MULTIPOINT( 1 2, 3 4, 5 6)'::GEOMETRY) as geom;
select '26',ST_asewkt('MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13)'::GEOMETRY) as geom;
select '27',ST_asewkt('MULTIPOINT( 1 2 0, 1 2 3, 4 5 0, 6 7 8)'::GEOMETRY) as geom;
select '28',ST_asewkt('MULTIPOINT( 1 2 3,4 5 0)'::GEOMETRY) as geom;

select '29',ST_asewkt('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) )'::GEOMETRY) as geom;
select '30',ST_asewkt('MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4),(0 0, 1 1, 2 2, 3 3 , 4 4))'::GEOMETRY) as geom;
select '31',ST_asewkt('MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) )'::GEOMETRY) as geom;
select '32',ST_asewkt('MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0))'::GEOMETRY) as geom;

select '33',ST_asewkt('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)) )'::GEOMETRY) as geom;
select '34',ST_asewkt('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) )'::GEOMETRY) as geom;
select '35',ST_asewkt('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as geom;


select '36',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2))'::GEOMETRY);
select '37',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3))'::GEOMETRY);
select '38',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);
select '39',ST_asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0, 1 1, 2 2, 3 3 , 4 4) ))'::GEOMETRY);
select '40',ST_asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0)))'::GEOMETRY);
select '41',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select '42',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 0),MULTIPOINT( 1 2 3))'::GEOMETRY);
select '43',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOINT( 1 2 0, 3 4 0, 5 6 0),POINT( 1 2 3))'::GEOMETRY);
select '44',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3),MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0) ))'::GEOMETRY);
select '45',ST_asewkt('GEOMETRYCOLLECTION(MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0 , 4 4 0) ),POINT( 1 2 3))'::GEOMETRY);
select '46',ST_asewkt('GEOMETRYCOLLECTION(POINT( 1 2 3), MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ))'::GEOMETRY);
select '47',ST_asewkt('GEOMETRYCOLLECTION(MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7 0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) ),MULTILINESTRING( (0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(0 0 0, 1 1 0, 2 2 0, 3 3 0, 4 4 0),(1 2 3 , 4 5 6 , 7 8 9 , 10 11 12, 13 14 15) ),MULTIPOINT( 1 2 3, 5 6 7, 8 9 10, 11 12 13))'::GEOMETRY);

select '48',ST_asewkt('MULTIPOINT( -1 -2 -3, 5.4 6.6 7.77, -5.4 -6.6 -7.77, 1e6 1e-6 -1e6, -1.3e-6 -1.4e-5 0)'::GEOMETRY) as geom;

select '49', ST_asewkt('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(1 1) ))'::GEOMETRY) as geom;
--- basic datatype (incorrect)

select '50', 'POINT()'::GEOMETRY as geom;
select '51', 'POINT(1)'::GEOMETRY as geom;
select '52', 'POINT(,)'::GEOMETRY as geom;
select '53', 'MULTIPOINT(,)'::GEOMETRY as geom;
select '54', 'POINT(a b)'::GEOMETRY as geom;
select '55', 'MULTIPOINT()'::GEOMETRY as geom;
select '56', ST_asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10) )'::GEOMETRY);
select '57', ST_asewkt('POLYGON( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7) )'::GEOMETRY);
select '58', ST_asewkt('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2,) )'::GEOMETRY);


--- funny results

select '59',ST_asewkt('POINT(1 2 3, 4 5 6)'::GEOMETRY);
select '60',ST_asewkt('POINT(1 2 3 4 5 6 7)'::GEOMETRY);
select '61',ST_asewkt('LINESTRING(1 1)'::GEOMETRY);
--select '62',replace(ST_asewkt('POINT( 1e700 0)'::GEOMETRY), 'Infinity', 'inf');
--select '63',replace(ST_asewkt('POINT( -1e700 0)'::GEOMETRY), 'Infinity', 'inf');
select '62',ST_asewkt('POINT( 1e700 0)'::GEOMETRY);
select '63',ST_asewkt('POINT( -1e700 0)'::GEOMETRY);
select '64',ST_asewkt('MULTIPOINT(1 1, 2 2'::GEOMETRY);


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

select '106',ST_box3d('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as bvol;
select '106_',box3d('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as bvol;

-- box3d only type is only used for indexing -- NEVER use one yourself
select '107',ST_asewkt(ST_geometry('BOX3D(0 0 0, 7 7 7 )'::BOX3D));

--- debug function testing

select '108',ST_npoints('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as value;
select '108_',npoints('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as value;
select '109',ST_npoints('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3))'::GEOMETRY) as value;
select '109_',npoints('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3))'::GEOMETRY) as value;

select '110', ST_nrings('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;
select '110_', nrings('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '111', ST_mem_size(dropBBOX('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY)) as value;
select '111_', mem_size(dropBBOX('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY)) as value;


select '112',ST_numgeometries('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;
select '112_',numgeometries('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;

---selection



--- TOAST testing

-- create a table with data that will be TOASTed (even after compression)
create table TEST(a GEOMETRY, b GEOMETRY);
\i regress_biginsert.sql


---test basic ops on this 

select '121',ST_box3d(a) as box3d_a, ST_box3d(b) as box3d_b from TEST;
select '121_',box3d(a) as box3d_a, box3d(b) as box3d_b from TEST;

select '122',a <<b from TEST;
select '123',a &<b from TEST;
select '124',a >>b from TEST;
select '125',a &>b from TEST;

select '126',a ~= b from TEST;
select '127',a @ b from TEST;
select '128',a ~ b from TEST; 

select '129', ST_mem_size(dropBBOX(a)), ST_mem_size(dropBBOX(b)) from TEST;

select '131', ST_X('POINT(1 2)');
select '131_', ST_X('POINT(1 2)');
select '132', ST_Y('POINT(1 2)');
select '132_', ST_Y('POINT(1 2)');
select '133', ST_Z('POINT(1 2)');
select '133_', ST_Z('POINT(1 2)');
select '133a', ST_Z('POINT(1 2 3)');
select '133a_', ST_Z('POINT(1 2 3)');
select '133b', ST_Z('POINTM(1 2 3)');
select '133b_', ST_Z('POINTM(1 2 3)');
select '133c', ST_M('POINT(1 2)');
select '133c_', ST_M('POINT(1 2)');
select '133d', ST_M('POINTM(1 2 4)');
select '133d_', ST_M('POINTM(1 2 4)');
select '133e', ST_M('POINT(1 2 4)');
select '133e_', ST_M('POINT(1 2 4)');

select '137', ST_box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY)'::geometry);
select '137_', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY)'::geometry);
select '138', ST_box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry);
select '138_', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry);

select '139', ST_asewkt(ST_multi(ST_setsrid('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry, 2)));
select '139_', asewkt(multi(setsrid('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry, 2)));
select '140', ST_asewkt(ST_multi(ST_setsrid('POINT(2 2)'::geometry, 3)));
select '140_', asewkt(multi(setsrid('POINT(2 2)'::geometry, 3)));
select '141', ST_asewkt(ST_multi(ST_setsrid('LINESTRING(2 2, 3 3)'::geometry, 4)));
select '141_', asewkt(multi(setsrid('LINESTRING(2 2, 3 3)'::geometry, 4)));
select '142', ST_asewkt(ST_multi(ST_setsrid('LINESTRING(2 2, 3 3)'::geometry, 5)));
select '142_', asewkt(multi(setsrid('LINESTRING(2 2, 3 3)'::geometry, 5)));
select '143', ST_asewkt(ST_multi(ST_setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));
select '143_', asewkt(multi(setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));
select '143c1', ST_asewkt(ST_multi('CIRCULARSTRING(0 0, 1 1, 2 2)'::geometry));
select '144', ST_asewkt(ST_force_3dm('POINT(1 2 3)'));
select '144_', asewkt(force_3dm('POINT(1 2 3)'));
select '145', ST_asewkt(ST_force_3dz('POINTM(1 2 3)'));
select '145_', asewkt(force_3dz('POINTM(1 2 3)'));
select '146', ST_asewkt(ST_force_4d('POINTM(1 2 3)'));
select '146_', asewkt(force_4d('POINTM(1 2 3)'));
select '147', ST_asewkt(ST_force_4d('POINT(1 2 3)'));
select '147_', asewkt(force_4d('POINT(1 2 3)'));

select '148', ST_astext(ST_segmentize('LINESTRING(0 0, 10 0)', 5));
select '148_', astext(segmentize('LINESTRING(0 0, 10 0)', 5));


select '150', ST_asewkt(ST_force_collection(ST_setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));
select '150_', asewkt(force_collection(setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));

select '151', ST_geom_accum(NULL, NULL);
select '151_', geom_accum(NULL, NULL);

-- Drop test table
DROP table test;
