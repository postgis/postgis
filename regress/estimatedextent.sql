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

-- #3391
-- drop table if exists p cascade;

create table p(g geometry);
create table c1() inherits (p);
create table c2() inherits (p);

analyze c1;
analyze c2;
analyze p;


-- #3391.1
with e as ( select ST_EstimatedExtent('c1','g') as e )
select '#3391.1', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.2
with e as ( select ST_EstimatedExtent('c2','g') as e )
select '#3391.2', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.3
with e as ( select ST_EstimatedExtent('p','g') as e )
select '#3391.3', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;


insert into c1 values ('Point(0 0)'::geometry);
insert into c1 values ('Point(1 1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.4
with e as ( select ST_EstimatedExtent('c1','g') as e )
select '#3391.4', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.5
with e as ( select ST_EstimatedExtent('c2','g') as e )
select '#3391.5', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.6
with e as ( select ST_EstimatedExtent('p','g') as e )
select '#3391.6', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;


insert into c2 values ('Point(0 0)'::geometry);
insert into c2 values ('Point(-1 -1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.7
with e as ( select ST_EstimatedExtent('c1','g') as e )
select '#3391.7', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.8
with e as ( select ST_EstimatedExtent('c2','g') as e )
select '#3391.8', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.9
with e as ( select ST_EstimatedExtent('p','g') as e )
select '#3391.9', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;


insert into p values ('Point(1 1)'::geometry);
insert into p values ('Point(2 2)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.10
with e as ( select ST_EstimatedExtent('c1','g') as e )
select '#3391.10', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.11
with e as ( select ST_EstimatedExtent('c2','g') as e )
select '#3391.11', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.12
with e as ( select ST_EstimatedExtent('p','g') as e )
select '#3391.12', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- test calls with 3th parameter

delete from p where 't';
delete from c1 where 't';
delete from c2 where 't';

delete from pg_statistic where starelid = 'p'::regclass;
delete from pg_statistic where starelid = 'c1'::regclass;
delete from pg_statistic where starelid = 'c2'::regclass;

analyze c1;
analyze c2;
analyze p;


-- #3391.13
with e as ( select ST_EstimatedExtent('public','p','g','t') as e )
select '#3391.13', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.14
with e as ( select ST_EstimatedExtent('public','p','g','f') as e )
select '#3391.14', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.15
with e as ( select ST_EstimatedExtent('public','c1','g', 't') as e )
select '#3391.15', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.16
with e as ( select ST_EstimatedExtent('public','c1','g', 'f') as e )
select '#3391.16', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;


insert into c1 values ('Point(0 0)'::geometry);
insert into c1 values ('Point(1 1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.17
with e as ( select ST_EstimatedExtent('public','p','g','f') as e )
select '#3391.17', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.18
with e as ( select ST_EstimatedExtent('public','p','g','t') as e )
select '#3391.18', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.19
with e as ( select ST_EstimatedExtent('public','c1','g', 'f') as e )
select '#3391.19', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;

-- #3391.20
with e as ( select ST_EstimatedExtent('public','c1','g', 't') as e )
select '#3391.20', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2) from e;


drop table p cascade;

