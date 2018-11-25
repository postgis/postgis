create table tbl_geomcollection_nd (
	k serial,
	g geometry
);

\copy tbl_geomcollection_nd from 'regress_gist_index_nd.data';

select '&&& (5 times without index)';
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;

select '&&& (5 times with index, recreated every time)';

create index tbl_geomcollection_nd_gist_nd_idx on tbl_geomcollection_nd using gist(g gist_geometry_ops_nd);
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
drop index if exists tbl_geomcollection_nd_gist_nd_idx;

create index tbl_geomcollection_nd_gist_nd_idx on tbl_geomcollection_nd using gist(g gist_geometry_ops_nd);
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
drop index if exists tbl_geomcollection_nd_gist_nd_idx;

create index tbl_geomcollection_nd_gist_nd_idx on tbl_geomcollection_nd using gist(g gist_geometry_ops_nd);
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
drop index if exists tbl_geomcollection_nd_gist_nd_idx;

create index tbl_geomcollection_nd_gist_nd_idx on tbl_geomcollection_nd using gist(g gist_geometry_ops_nd);
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;
drop index if exists tbl_geomcollection_nd_gist_nd_idx;

create index tbl_geomcollection_nd_gist_nd_idx on tbl_geomcollection_nd using gist(g gist_geometry_ops_nd);
select count(*) from tbl_geomcollection_nd t1, tbl_geomcollection_nd t2 where t1.g &&& t2.g;

DROP TABLE tbl_geomcollection_nd CASCADE;
