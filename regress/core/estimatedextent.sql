-- #877, #818
create table t(g geometry);
select '#877.1', ST_EstimatedExtent('t','g');
analyze t;
select '#877.2', ST_EstimatedExtent('public', 't','g');
SET client_min_messages TO NOTICE;
insert into t(g) values ('LINESTRING(-10 -50, 20 30)');

-- #877.3
select '#877.3', round(st_xmin(e.e)::numeric, 5), round(st_xmax(e.e)::numeric, 5),
round(st_ymin(e.e)::numeric, 5), round(st_ymax(e.e)::numeric, 5)
from ( select ST_EstimatedExtent('t','g') as e offset 0 ) AS e;

-- #877.4
analyze t;
select '#877.4', round(st_xmin(e.e)::numeric, 5), round(st_xmax(e.e)::numeric, 5),
round(st_ymin(e.e)::numeric, 5), round(st_ymax(e.e)::numeric, 5)
from ( select ST_EstimatedExtent('t','g') as e offset 0 ) AS e;

-- #877.5
truncate t;
with e as ( select ST_EstimatedExtent('t','g') as e offset 0 )
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
select '#3391.1', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c1','g') as e offset 0 ) AS e;

-- #3391.2
select '#3391.2', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c2','g') as e offset 0 ) AS e;

-- #3391.3
select '#3391.3', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('p','g') as e offset 0 ) AS e;

insert into c1 values ('Point(0 0)'::geometry);
insert into c1 values ('Point(1 1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.4
select '#3391.4', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c1','g') as e offset 0 ) AS e;

-- #3391.5
select '#3391.5', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c2','g') as e offset 0 ) AS e;

-- #3391.6
select '#3391.6', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('p','g') as e offset 0 ) AS e;

insert into c2 values ('Point(0 0)'::geometry);
insert into c2 values ('Point(-1 -1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.7
select '#3391.7', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c1','g') as e offset 0 ) AS e;

-- #3391.8
select '#3391.8', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c2','g') as e offset 0 ) AS e;

-- #3391.9
select '#3391.9', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('p','g') as e offset 0 ) AS e;

insert into p values ('Point(1 1)'::geometry);
insert into p values ('Point(2 2)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.10
select '#3391.10', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c1','g') as e offset 0 ) AS e;

-- #3391.11
select '#3391.11', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('c2','g') as e offset 0 ) AS e;

-- #3391.12
select '#3391.12', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('p','g') as e offset 0 ) AS e;

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
select '#3391.13', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','p','g','t') as e offset 0 ) AS e;

-- #3391.14
select '#3391.14', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','p','g','f') as e offset 0 ) AS e;

-- #3391.15
select '#3391.15', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from (select ST_EstimatedExtent('public','c1','g', 't') as e offset 0 ) AS e;

-- #3391.16
select '#3391.16', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from  ( select ST_EstimatedExtent('public','c1','g', 'f') as e offset 0 ) AS e;

insert into c1 values ('Point(0 0)'::geometry);
insert into c1 values ('Point(1 1)'::geometry);

analyze c1;
analyze c2;
analyze p;

-- #3391.17
select '#3391.17', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','p','g','f') as e offset 0 ) AS e;

-- #3391.18
select '#3391.18', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','p','g','t') as e offset 0 )AS e;

-- #3391.19
select '#3391.19', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','c1','g', 'f') as e offset 0 ) AS e;

-- #3391.20
select '#3391.20', round(st_xmin(e.e)::numeric, 2), round(st_xmax(e.e)::numeric, 2),
round(st_ymin(e.e)::numeric, 2), round(st_ymax(e.e)::numeric, 2)
from ( select ST_EstimatedExtent('public','c1','g', 't') as e offset 0 ) AS e;

drop table p cascade;

--
-- Index assisted extent generation
--
create table test (id serial primary key, geom1 geometry, geom2 geometry);
create index test_x1 on test using gist (geom1);
create index test_x2 on test using gist (geom2);
select '1.a null', _postgis_index_extent('test', 'geom1');
select '1.b null', _postgis_index_extent('test', 'geom2');
insert into test (geom1, geom2) select NULL, NULL;
insert into test (geom1, geom2) select 'POINT EMPTY', 'LINESTRING EMPTY';
select '2.a null', _postgis_index_extent('test', 'geom1');
select '2.b null', _postgis_index_extent('test', 'geom2');
insert into test (geom1, geom2) select 'POINT EMPTY', 'LINESTRING EMPTY' from generate_series(0,1024);
select '3.a null', _postgis_index_extent('test', 'geom1');
select '3.b null', _postgis_index_extent('test', 'geom2');
insert into test (geom1, geom2) select st_makepoint(s, s), st_makepoint(2*s, 2*s) from generate_series(-100,100) s;
select '4.a box',_postgis_index_extent('test', 'geom1');
select '4.b box',_postgis_index_extent('test', 'geom2');
-- delete from test;
-- select '5.a bad-box',_postgis_index_extent('test', 'geom1');
-- select '5.b bad-box',_postgis_index_extent('test', 'geom2');
-- vacuum full test;
-- select '6.a null', _postgis_index_extent('test', 'geom1');
-- select '6.b null', _postgis_index_extent('test', 'geom2');
drop table test cascade;

-- Check NOTICE message
create table test (id serial primary key, geom1 geometry, geom2 geometry);
insert into test (geom1, geom2) select NULL, NULL;
insert into test (geom1, geom2) select NULL, NULL;
insert into test (geom1, geom2) select NULL, NULL;
ANALYZE test;
drop table test cascade;
