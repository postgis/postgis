-- #877, #818
create table t(g geometry);
select '#877.1', ST_EstimatedExtent('t','g');
analyze t;
select '#877.2', ST_EstimatedExtent('public', 't','g');
SET client_min_messages TO DEBUG;
select '#877.2.deprecated', ST_Estimated_Extent('public', 't','g');
SET client_min_messages TO NOTICE;
insert into t(g) values ('LINESTRING(-10 -50, 20 30)');

-- #877.3
with e as ( select ST_EstimatedExtent('t','g') as e )
select '#877.3', round(st_xmin(e.e)::numeric, 5), round(st_xmax(e.e)::numeric, 5),
round(st_ymin(e.e)::numeric, 5), round(st_ymax(e.e)::numeric, 5) from e;

-- #877.4
analyze t;
with e as ( select ST_EstimatedExtent('t','g') as e )
select '#877.4', round(st_xmin(e.e)::numeric, 5), round(st_xmax(e.e)::numeric, 5),
round(st_ymin(e.e)::numeric, 5), round(st_ymax(e.e)::numeric, 5) from e;

-- #877.5
truncate t;
with e as ( select ST_EstimatedExtent('t','g') as e )
select '#877.5', round(st_xmin(e.e)::numeric, 5), round(st_xmax(e.e)::numeric, 5),
round(st_ymin(e.e)::numeric, 5), round(st_ymax(e.e)::numeric, 5) from e;
drop table t;


