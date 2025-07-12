set client_min_messages to WARNING;
\set VERBOSITY terse

select addtopogeometrycolumn('tt','public','feature','tg','POINT'); -- fail
select addtopogeometrycolumn('tt','public.feature2','tg',1000,'POINT'); -- fail
select createtopology('tt') > 0;
select addtopogeometrycolumn('tt','public','feature','tg','POINT'); -- fail
select addtopogeometrycolumn('tt','public.feature2','tg',1000,'POINT',null); -- fail
create table feature(id integer);
create table feature2(id integer);
select addtopogeometrycolumn('tt','public','feature','tg','BOGUS'); -- fail
select addtopogeometrycolumn('tt','public','feature','tg','POINT',0); -- fail
select addtopogeometrycolumn('tt','public.feature2','tg',1000,'BOGUS',null); -- fail
select addtopogeometrycolumn('tt','public.feature2','tg',1000,'POINT',0); -- fail

-- Expect first good call returning 1
select 'T1', addtopogeometrycolumn('tt','public','feature','tg','POINT');

-- Check that you can add a second topogeometry column to the same table
select 'T2', addtopogeometrycolumn('tt','public','feature','tg2','LINE');

-- Check polygonal
select 'T3', addtopogeometrycolumn('tt','public','feature','tg3','POLYGON');

-- Check collection
select 'T4', addtopogeometrycolumn('tt','public','feature','tg4','COLLECTION');

-- Check alternate names
select 'T5', addtopogeometrycolumn('tt','public','feature',
	'tg5','ST_MultiPoint');
select 'T6', addtopogeometrycolumn('tt','public','feature',
	'tg6','ST_MultiLineString');
select 'T7', addtopogeometrycolumn('tt','public','feature',
	'tg7','ST_MultiPolygon');
select 'T8', addtopogeometrycolumn('tt','public','feature',
	'tg8','GEOMETRYCOLLECTION');
select 'T9', addtopogeometrycolumn('tt','public','feature',
	'tg9','PUNtal');
select 'T10', addtopogeometrycolumn('tt','public','feature',
	'tg10','Lineal');
select 'T11', addtopogeometrycolumn('tt','public','feature',
	'tg11','Areal');
select 'T12', addtopogeometrycolumn('tt','public','feature',
	'tg12','GEOMETRY');

-- Expect to fail since layerid already in use
select 'T1.2', addtopogeometrycolumn('tt','public.feature2','tg',1,'POINT',null);

-- Expect first good call returning 1001
select 'T1.2', addtopogeometrycolumn('tt','public.feature2','tg',1001,'POINT',null);
-- Expect sequence to be 1001
select currval('tt.layer_id_seq');

-- Check that you can add a second topogeometry column to the same table
select 'T2.2', addtopogeometrycolumn('tt','public.feature2','tg2',1002,'LINE',null);

-- Check polygonal
select 'T3.2', addtopogeometrycolumn('tt','public.feature2','tg3',1003,'POLYGON',null);

-- Check collection
select 'T4.2', addtopogeometrycolumn('tt','public.feature2','tg4',1004,'COLLECTION',null);

-- Check alternate names
select 'T5.2', addtopogeometrycolumn('tt','public.feature2','tg5',1005,'ST_MultiPoint',null);
select 'T6.2', addtopogeometrycolumn('tt','public.feature2','tg6',1006,'ST_MultiLineString',null);
select 'T7.2', addtopogeometrycolumn('tt','public.feature2','tg7',1007,'ST_MultiPolygon',null);
select 'T8.2', addtopogeometrycolumn('tt','public.feature2','tg8',1008,'GEOMETRYCOLLECTION',null);
select 'T9.2', addtopogeometrycolumn('tt','public.feature2','tg9',1009,'PUNtal',null);
select 'T10.2', addtopogeometrycolumn('tt','public.feature2','tg10',1010,'Lineal',null);
select 'T11.2', addtopogeometrycolumn('tt','public.feature2','tg11',1011,'Areal',null);
select 'T12.2', addtopogeometrycolumn('tt','public.feature2','tg12',1012,'GEOMETRY',null);

select l.layer_id, l.schema_name, l.table_name, l.feature_column,
 l.feature_type, l.level, l.child_id
from topology.layer l, topology.topology t
where l.topology_id = t.id and t.name = 'tt'
order by l.layer_id;

-- Test if sequence is set after user defined id
-- TODO: not happy with the 2 signatures (too close)
select addtopogeometrycolumn('tt','public.feature2','test',null::integer,'GEOMETRY',null); -- should be 1013;

drop table feature;
drop table feature2;
select droptopology('tt');

