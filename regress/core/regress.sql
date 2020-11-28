-- postgres
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
select '62',regexp_replace(ST_asewkt('POINT( 1e700 0)'::GEOMETRY), E'(Infinity|1\.#INF)', 'inf');
select '63',regexp_replace(ST_asewkt('POINT( -1e700 0)'::GEOMETRY), E'(Infinity|1\.#INF)', 'inf');
--select '62',ST_asewkt('POINT( 1e700 0)'::GEOMETRY);
--select '63',ST_asewkt('POINT( -1e700 0)'::GEOMETRY);
select '64',ST_asewkt('MULTIPOINT(1 1, 2 2'::GEOMETRY);

--- is_same() testing

select '65','POINT(1 1)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '65a',ST_OrderingEquals('POINT(1 1)'::GEOMETRY,'POINT(1 1)'::GEOMETRY) as bool;
select '66','POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '66a',ST_OrderingEquals('POINT(1 1 0)'::GEOMETRY,'POINT(1 1)'::GEOMETRY) as bool;
select '67','POINT(1 1 0)'::GEOMETRY ~= 'POINT(1 1 0)'::GEOMETRY as bool;
select '67a',ST_OrderingEquals('POINT(1 1 0)'::GEOMETRY,'POINT(1 1 0)'::GEOMETRY) as bool;

select '68','MULTIPOINT(1 1,2 2)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;
select '68a',ST_OrderingEquals('MULTIPOINT(1 1,2 2)'::GEOMETRY,'MULTIPOINT(1 1,2 2)'::GEOMETRY) as bool;
select '69','MULTIPOINT(2 2, 1 1)'::GEOMETRY ~= 'MULTIPOINT(1 1,2 2)'::GEOMETRY as bool;
select '69a',ST_OrderingEquals('MULTIPOINT(2 2, 1 1)'::GEOMETRY,'MULTIPOINT(1 1,2 2)'::GEOMETRY) as bool;

select '70','GEOMETRYCOLLECTION(POINT( 1 2 3),POINT(4 5 6))'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '70a',ST_OrderingEquals('GEOMETRYCOLLECTION(POINT( 1 2 3),POINT(4 5 6))'::GEOMETRY,'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY) as bool;
select '71','MULTIPOINT(4 5 6, 1 2 3)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '71a',ST_OrderingEquals('MULTIPOINT(4 5 6, 1 2 3)'::GEOMETRY,'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY) as bool;
select '72','MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY as bool;
select '72a',ST_OrderingEquals('MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY,'GEOMETRYCOLLECTION(POINT( 4 5 6),POINT(1 2 3))'::GEOMETRY) as bool;
select '73','MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY ~= 'GEOMETRYCOLLECTION(MULTIPOINT(1 2 3, 4 5 6))'::GEOMETRY as bool;
select '73a',ST_OrderingEquals('MULTIPOINT(1 2 3, 4 5 6)'::GEOMETRY,'GEOMETRYCOLLECTION(MULTIPOINT(1 2 3, 4 5 6))'::GEOMETRY) as bool;

select '74','LINESTRING(1 1,2 2)'::GEOMETRY ~= 'POINT(1 1)'::GEOMETRY as bool;
select '74a',ST_OrderingEquals('LINESTRING(1 1,2 2)'::GEOMETRY,'POINT(1 1)'::GEOMETRY) as bool;
select '75','LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(2 2, 1 1)'::GEOMETRY as bool;
select '75a',ST_OrderingEquals('LINESTRING(1 1, 2 2)'::GEOMETRY,'LINESTRING(2 2, 1 1)'::GEOMETRY) as bool;
select '76','LINESTRING(1 1, 2 2)'::GEOMETRY ~= 'LINESTRING(1 1, 2 2, 3 3)'::GEOMETRY as bool;
select '76a',ST_OrderingEquals('LINESTRING(1 1, 2 2)'::GEOMETRY,'LINESTRING(1 1, 2 2, 3 3)'::GEOMETRY) as bool;

--- function testing
--- conversion function

select '106',box3d('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as bvol;

-- box3d only type is only used for indexing -- NEVER use one yourself
select '107a',ST_AsEWKT(geometry('BOX3D(1 2 3, 1 2 3 )'::BOX3D));
select '107b',ST_AsEWKT(geometry('BOX3D(2 3 3, 7 3 3 )'::BOX3D));
select '107c',ST_AsEWKT(geometry('BOX3D(2 3 5, 6 8 5 )'::BOX3D));
select '107d',ST_AsEWKT(geometry('BOX3D(1 -1 4, 2 -1 9 )'::BOX3D));
select '107e',ST_AsEWKT(geometry('BOX3D(-1 3 5, -1 6 8 )'::BOX3D));
select '107f',ST_AsEWKT(geometry('BOX3D(1 2 3, 4 5 6 )'::BOX3D));

--- debug function testing

select '108',ST_NPoints('MULTIPOINT(0 0, 7 7)'::GEOMETRY) as value;
select '109',ST_NPoints('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3))'::GEOMETRY) as value;

select '110', ST_NRings('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '111', ST_MemSize(PostGIS_DropBBOX('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY)) as value;

select '112',ST_NumGeometries('GEOMETRYCOLLECTION(POINT(1 1), LINESTRING( 1 1 , 2 2, 3 3),MULTIPOINT(1 1, 2 2))'::GEOMETRY) as value;

---selection

--- TOAST testing

-- create a table with data that will be TOASTed (even after compression)
create table TEST(a GEOMETRY, b GEOMETRY);
\i :regdir/core/regress_biginsert.sql

---test basic ops on this

select '121',box3d(a) as box3d_a, box3d(b) as box3d_b from TEST;

select '122',a <<b from TEST;
select '123',a &<b from TEST;
select '124',a >>b from TEST;
select '125',a &>b from TEST;

select '126',a ~= b from TEST;
select '127',a @ b from TEST;
select '128',a ~ b from TEST;
select '129', ST_MemSize(PostGIS_DropBBOX(a)), ST_MemSize(PostGIS_DropBBOX(b)) from TEST;
select '131', ST_X('POINT(1 2)');
select '132', ST_Y('POINT(1 2)');
select '133', ST_Z('POINT(1 2)');
select '133a', ST_Z('POINT(1 2 3)');
select '133b', ST_Z('POINTM(1 2 3)');
select '133c', ST_M('POINT(1 2)');
select '133d', ST_M('POINTM(1 2 4)');
select '133e', ST_M('POINT(1 2 4)');

select '137', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY)'::geometry);
select '138', box3d('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry);

select '139', ST_AsEWKT(ST_multi(ST_setsrid('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT(0 0))'::geometry, 2)));
select '140', ST_AsEWKT(ST_multi(ST_setsrid('POINT(2 2)'::geometry, 3)));
select '141', ST_AsEWKT(ST_multi(ST_setsrid('LINESTRING(2 2, 3 3)'::geometry, 4)));
select '142', ST_AsEWKT(ST_multi(ST_setsrid('LINESTRING(2 2, 3 3)'::geometry, 5)));
select '143', ST_AsEWKT(ST_multi(ST_setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));
select '143c1', ST_AsEWKT(ST_multi('CIRCULARSTRING(0 0, 1 1, 2 2)'::geometry));
select '144', ST_AsEWKT(ST_Force3dm('POINT(1 2 3)'));
select '145', ST_AsEWKT(ST_Force3dz('POINTM(1 2 3)'));
select '146', ST_AsEWKT(ST_Force4d('POINTM(1 2 3)'));
select '147', ST_AsEWKT(ST_Force4d('POINT(1 2 3)'));

select '148', ST_AsText(ST_segmentize('LINESTRING(0 0, 10 0)'::geometry, 5));

select '149', ST_AsText(ST_segmentize('GEOMETRYCOLLECTION EMPTY'::geometry, 0.5));

select '150d', ST_AsEWKT(ST_ForceCollection(ST_setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));
select '150', ST_AsEWKT(ST_ForceCollection(ST_setsrid('POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))'::geometry, 6)));

select '151', encode(ST_AsBinary(ST_MakeEnvelope(0, 0, 1, 1, 4326),'ndr'),'hex');
select '152', ST_SRID(ST_MakeEnvelope(0, 0, 1, 1, 4326));
select '152.1', ST_SRID(ST_MakeEnvelope(0, 0, 1, 1)) = ST_SRID('POINT(0 0)'::geometry);
select '152.2', ST_SRID(ST_SetSRID(ST_MakeEnvelope(0, 0, 1, 1), 4326));

select '153', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(POINT(0 0))',1));
select '154', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)))',1));
select '155', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0), POINT(1 1)))',1));
select '156', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), POINT(1 1)))',1));
select '157', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), POINT(1 1)))',2));
select '158', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), POINT(1 1)),LINESTRING(2 2, 3 3))',2));
select '159', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), POINT(1 1)),LINESTRING(2 2, 3 3))',3));
select '160', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), POINT(1 1)),LINESTRING(2 2, 3 3))',1));
select '161', ST_AsText(ST_CollectionExtract('GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), GEOMETRYCOLLECTION(POINT(1 1))),LINESTRING(2 2, 3 3))',2));
select '162', encode(ST_AsBinary(ST_MakeLine(ST_GeomFromText('POINT(-11.1111111 40)'),ST_GeomFromText('LINESTRING(-11.1111111 70,70 -11.1111111)')),'ndr'),'hex') As result;
select '163', ST_AsEWKT('POLYGON((0 0 0, 1 0 0, 1 1 0, 0 1 0, 0 0 0))');
select '164', ST_AsEWKT('POLYGON((0 0 0, 1 0 0, 1 1 0, 0 1 0, 0 0 1))');
select '165', ST_AsEWKT('POLYGON((0 0 0, 1 0 0, 1 1 0, 0 1 0, 0 0.1 1))');
select '166', ST_AsText('POINT EMPTY');
select '167', ST_AsText('LINESTRING EMPTY');
select '168', ST_AsText('POLYGON EMPTY');
select '169', ST_AsText('CIRCULARSTRING EMPTY');
select '170', ST_AsText('COMPOUNDCURVE EMPTY');
select '171', ST_AsText('CURVEPOLYGON EMPTY');
select '172', ST_AsText('MULTIPOINT EMPTY');
select '173', ST_AsText('MULTILINESTRING EMPTY');
select '174', ST_AsText('MULTIPOLYGON EMPTY');
select '175', ST_AsText('TRIANGLE EMPTY');
select '176', ST_AsText('TIN EMPTY');
select '177', ST_AsText('POLYHEDRALSURFACE EMPTY');
select '178', ST_AsText('MULTISURFACE EMPTY');
select '179', ST_AsText('MULTICURVE EMPTY');
select '180', ST_AsText('GEOMETRYCOLLECTION EMPTY');
select '181', ST_AsText('GEOMETRYCOLLECTION(TRIANGLE EMPTY,TIN EMPTY)');

select '190', ST_Points(NULL) IS NULL;
select '191', ST_AsText(ST_Points('MULTICURVE EMPTY'));
select '192', ST_AsText(ST_Points('POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))'));

select '200', ST_Expand(null::geometry, 1);
select '201', ST_AsText(ST_Expand('LINESTRING (1 2 3, 10 20 30)'::geometry, 1));
select '202', ST_AsText(ST_Expand('LINESTRINGM (1 2 3, 10 20 30)'::geometry, 1));
select '203', ST_AsText(ST_Expand('LINESTRING (1 2, 10 20)'::geometry, 3));
select '204', ST_AsText(ST_Expand('POLYGON EMPTY'::geometry, 4));
select '205', ST_AsText(ST_Expand('POINT EMPTY'::geometry, 2));
select '206', ST_AsText(ST_Expand('POINT (2 3)'::geometry, 0));
select '207', ST_AsText(ST_Expand('LINESTRING (1 2, 3 4)'::geometry, 0));
select '208', ST_AsText(ST_Expand('POINT (0 0)'::geometry, -1));
select '209', ST_AsText(ST_Expand('LINESTRING (0 0, 10 10)'::geometry, -4));
select '210', ST_Expand(null::box3d, 1);
select '211', ST_Expand('BOX3D(-1 3 5, -1 6 8)'::BOX3D, 1);
select '212', ST_Expand(null::box2d, 1);
select '213', ST_Expand('BOX(-2 3, -1 6'::BOX2D, 4);

select '214', ST_Expand(null::geometry, 1, 1, 1, 1);
select '215', ST_AsText(ST_Expand('LINESTRING (1 2 3, 10 20 30)'::geometry, 1, 4, 2, 7));

select '216', ST_AsText(ST_Expand('LINESTRINGM (1 2 3, 10 20 30)'::geometry, 1, 4, 2, 7));
select '217', ST_AsText(ST_Expand('LINESTRING (1 2, 10 20)'::geometry, 1, 4, 2, 7));
select '218', ST_AsText(ST_Expand('POLYGON EMPTY'::geometry, 4, 3, 1, 1));
select '219', ST_AsText(ST_Expand('POINT EMPTY'::geometry, 2, 3, 1, -1));
select '220', ST_AsText(ST_Expand('POINT (2 3)'::geometry, 0, 4, -2, 8));
select '221', ST_AsText(ST_Expand('POINT (0 0)'::geometry, -1, -2));
select '222', ST_Expand(null::box3d, 1, 1, 1);
select '223', ST_Expand('BOX3D(-1 3 5, -1 6 8)'::BOX3D, 1, -1, 7);
select '224', ST_Expand(null::box2d, 1, 1);
select '225', ST_Expand('BOX(-2 3, -1 6'::BOX2D, 4, 2);
select '226', ST_SRID(ST_Expand('SRID=4326;POINT (0 0)'::geometry, 1))=4326;

-- ST_TileEnvelope()
select '227', ST_AsText(ST_TileEnvelope(-1, 0, 0), 2);
select '228', ST_AsText(ST_TileEnvelope(0, 0, 1), 2);
select '229', ST_AsText(ST_TileEnvelope(0, 0, 0), 2);
select '230', ST_AsText(ST_TileEnvelope(4, 8, 8), 2);
select '231', ST_AsText(ST_TileEnvelope(4, 15, 15), 2);
select '232', ST_AsText(ST_TileEnvelope(4, 8, 8, ST_MakeEnvelope(-100, -100, 100, 100, 0)), 2);
select '233', ST_AsText(ST_TileEnvelope(4, 15, 15, ST_MakeEnvelope(-100, -100, 100, 100, 0)), 2);
select '234', ST_AsText(ST_TileEnvelope(4, 0, 0, ST_MakeEnvelope(-100, -100, 100, 100, 0)), 2);
select '235', ST_AsText(ST_TileEnvelope(4, 8, 8, ST_MakeEnvelope(-200, -100, 200, 100, 0)), 2);
select '236', ST_AsText(ST_TileEnvelope(0, 0, 0, margin => 0.1), 2);
select '237', ST_AsText(ST_TileEnvelope(1, 0, 0, margin => 0.1), 2);
select '238', ST_AsText(ST_TileEnvelope(2, 1, 3, margin => 0.5), 2);
select '239', ST_AsText(ST_TileEnvelope(0, 0, 0, margin => -0.5), 2);
select '240', ST_AsText(ST_TileEnvelope(0, 0, 0, margin => -0.51), 2);
select '241', ST_AsText(ST_TileEnvelope(0, 0, 0, margin => -0.4), 2);
select '250', ST_AsText(ST_TileEnvelope(10,300,387), 2);
select '251', ST_AsText(ST_TileEnvelope(10,300,387, margin => 0.1), 2);
select '252', ST_AsText(ST_TileEnvelope(10,300,387, margin => 0.5), 2);
select '253', ST_AsText(ST_TileEnvelope(10,300,387, margin => 2), 2);
select '254', ST_AsText(ST_TileEnvelope(10,300,387, margin => -0.3), 2);

-- ST_Hexagon()
select '300', ST_AsText(ST_Hexagon(10, 0, 0), 5);
select '301', ST_AsText(ST_Hexagon(10, 1, 1), 5);
select '302', ST_AsText(ST_Hexagon(10, -1, -1), 5);
select '303', ST_AsText(ST_Hexagon(10, 100, -100), 5);
-- ST_Square()
select '304', ST_AsText(ST_Square(10, 0, 0), 5);
select '305', ST_AsText(ST_Square(10, 1, 1), 5);
select '306', ST_AsText(ST_Square(10, -1, -1), 5);
select '307', ST_AsText(ST_Square(10, 100, -100), 5);
-- ST_HexagonGrid()
select '308', Count(*) FROM ST_HexagonGrid(100000, ST_TileEnvelope(4, 7, 7));
select '309', Count(*) FROM ST_HexagonGrid(100000, ST_TileEnvelope(4, 7, 7)) hex, ST_TileEnvelope(4, 7, 7) tile WHERE NOT ST_Intersects(hex.geom, tile);
select '310', Count(*) FROM ST_HexagonGrid(100000, ST_TileEnvelope(4, 2, 2));
select '311', Count(*) FROM ST_HexagonGrid(100000, ST_TileEnvelope(4, 2, 2)) hex, ST_TileEnvelope(4, 2, 2) tile WHERE NOT ST_Intersects(hex.geom, tile);
-- ST_SquareGrid()
select '312', Count(*) FROM ST_SquareGrid(100000, ST_TileEnvelope(4, 7, 7));
select '313', Count(*) FROM ST_SquareGrid(100000, ST_TileEnvelope(4, 7, 7)) hex, ST_TileEnvelope(4, 7, 7) tile WHERE NOT ST_Intersects(hex.geom, tile);
select '314', Count(*) FROM ST_SquareGrid(100000, ST_TileEnvelope(4, 2, 2));
select '315', Count(*) FROM ST_SquareGrid(100000, ST_TileEnvelope(4, 2, 2)) hex, ST_TileEnvelope(4, 2, 2) tile WHERE NOT ST_Intersects(hex.geom, tile);

WITH geoms AS
(
    SELECT * FROM  (SELECT (ST_HexagonGrid(5, '01030000000100000005000000fdffffffff7f66c0fcffffffff7f56c0fdffffffff7f66c07cc0e71a95e8544000000000008066407cc0e71a95e854400000000000806640fcffffffff7f56c0fdffffffff7f66c0fcffffffff7f56c0')).*) sq  WHERE i = 5 AND j IN (0, 1)
),
j0 AS
(
    SELECT * FROM geoms WHERE i = 5 AND j = 0
),
j1 AS
(
    SELECT * FROM geoms WHERE i = 5 AND j = 1
)
SELECT '316',
    ST_Intersects(j0.geom, j1.geom),
    ST_AsText(ST_Intersection(j0.geom, j1.geom))
FROM j0, j1;

-- Drop test table
DROP table test;

-- Make sure all postgis-referencing probin are using the
-- same module version and path as expected by postgis_lib_version()
--
SELECT DISTINCT 'unexpected probin', proname || ':' || probin
FROM pg_proc
WHERE probin like '%postgis%'
  AND
regexp_replace(probin, '(rt)?postgis(_[^-]*)?(-[0-9.]*)$', '\3')
	!=
(
	SELECT
regexp_replace(probin, '(rt)?postgis(_[^-]*)?(-[0-9.]*)$', '\3')
	FROM pg_proc WHERE proname = 'postgis_lib_version'
)
ORDER BY 2;


SELECT 'UNEXPECTED', postgis_full_version()
	WHERE postgis_full_version() LIKE '%UNPACKAGED%'
	   OR postgis_full_version() LIKE '%need upgrade%';
