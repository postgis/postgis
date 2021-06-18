BEGIN;

SELECT topology.CreateTopology('topoperf');

\timing on

CREATE TABLE topoperf.case_full_coverage_no_holes
AS SELECT ST_Boundary(geom) g FROM ST_SquareGrid(4, ST_MakeEnvelope(0,0,120,120));

SELECT count(*) FROM (
  SELECT topology.TopoGeo_addLinestring('topoperf', g)
  FROM topoperf.case_full_coverage_no_holes
) foo;

\timing off
