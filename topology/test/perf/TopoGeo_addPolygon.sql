BEGIN;

SELECT topology.CreateTopology('topoperf');

CREATE TABLE topoperf.case_full_coverage_no_holes AS
SELECT i, j, geom g
FROM ST_SquareGrid(4, ST_MakeEnvelope(0, 0, 80, 80));

\timing on

SELECT count(*) FROM (
  SELECT topology.TopoGeo_addPolygon('topoperf', g)
  FROM (
    SELECT g FROM topoperf.case_full_coverage_no_holes
    ORDER BY i, j
  ) bar
) foo;

\timing off

SELECT TopologySummary('topoperf');

ROLLBACK;
