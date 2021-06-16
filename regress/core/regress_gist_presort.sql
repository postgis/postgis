-------------------------------------------------------------------------------

CREATE TABLE tbl_geomcollection (
  k serial,
  g geometry
);

\COPY tbl_geomcollection FROM 'regress_spgist_index_2d.data';
-------------------------------------------------------------------------------
\timing

CREATE INDEX ON tbl_geomcollection USING gist(g gist_geometry_ops_2d);

\timing
-------------------------------------------------------------------------------
DROP TABLE tbl_geomcollection CASCADE;