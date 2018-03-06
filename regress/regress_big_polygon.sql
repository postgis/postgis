\set QUIET on
SET client_min_messages TO WARNING;
drop table if exists big_polygon;
create table big_polygon(geom geometry);
alter table big_polygon alter column geom set storage main;
-- Data (c) OpenStreetMap contributors, ODbL / http://osm.org/
\copy big_polygon from 'big_polygon.wkb'
\set QUIET off
SET client_min_messages TO NOTICE;
