BEGIN;

SELECT topology.CreateTopology('topoperf');

CREATE TABLE topoperf.case_concentric_circles AS

SELECT radius, ST_ExteriorRing(
  ST_Buffer('POINT(0 0)', radius, 128)
) g
FROM generate_series(10, 100) radius;

\timing on

SELECT count(*) FROM (
  SELECT topology.TopoGeo_addLinestring('topoperf', g)
  FROM (
    SELECT g FROM topoperf.case_concentric_circles
    ORDER BY radius
  ) bar
) foo;

\timing off

SELECT TopologySummary('topoperf');
