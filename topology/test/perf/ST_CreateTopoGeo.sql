BEGIN;
SELECT topology.CreateTopology('topoperf');

--\timing on
CREATE TABLE topoperf.case_full_coverage_no_holes
AS SELECT ST_Boundary(geom) g FROM ST_SquareGrid(4, ST_MakeEnvelope(0,0,120,120));

SELECT ST_CreateTopoGeo('topoperf', ST_Collect(g))
FROM topoperf.case_full_coverage_no_holes
;

ROLLBACK;
