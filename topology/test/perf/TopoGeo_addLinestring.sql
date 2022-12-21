BEGIN;

SELECT topology.CreateTopology('topoperf');

\timing on

CREATE TABLE topoperf.case_full_coverage_no_holes AS
SELECT ST_Subdivide(ST_Segmentize(ST_Boundary(geom), 0.5), 5) g
FROM ST_SquareGrid(4, ST_MakeEnvelope(0,0,120,120));

SELECT count(*) FROM (
  SELECT topology.TopoGeo_addLinestring('topoperf', g)
  FROM topoperf.case_full_coverage_no_holes
) foo;

\timing off

SELECT TopologySummary('topoperf');
