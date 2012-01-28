set client_min_messages to WARNING;
\set VERBOSITY terse

select createtopology('tt') > 0;
select addtopogeometrycolumn('tt','public','feature','tg','POINT'); -- fail
create table feature(id integer);
select addtopogeometrycolumn('tt','public','feature','tg','BOGUS'); -- fail
select addtopogeometrycolumn('tt','public','feature','tg','POINT', 0); -- fail

-- Expect first good call returning 1
select 'T1', addtopogeometrycolumn('tt','public','feature','tg','POINT');

-- Check that you can add a second topogeometry column to the same table
select 'T2', addtopogeometrycolumn('tt','public','feature','tg2','LINE');

select l.layer_id, l.schema_name, l.table_name, l.feature_column,
 l.feature_type, l.level, l.child_id 
from topology.layer l, topology.topology t
where l.topology_id = t.id and t.name = 'tt'
order by l.layer_id;

drop table feature;
select droptopology('tt');
