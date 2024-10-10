BEGIN;
SELECT topology.CreateTopology('topoperf');

--\timing on
CREATE TABLE topoperf.case_full_coverage_no_holes
AS SELECT ST_Boundary(geom) g FROM ST_SquareGrid(2, ST_MakeEnvelope(0,0,120,120));

\timing on
SELECT ST_CreateTopoGeo('topoperf', ST_Collect(g))
FROM topoperf.case_full_coverage_no_holes
;

