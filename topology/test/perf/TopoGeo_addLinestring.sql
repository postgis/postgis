SELECT DropTopology('topoperf');
SELECT CreateTopology('topoperf');
\timing
SELECT count(*) FROM (
  SELECT TopoGeo_addLinestring('topoperf', g) FROM (
    SELECT ST_ExteriorRing(ST_Buffer(ST_MakePoint(x, y), 10, 10)) g FROM
      generate_series(-15,15,5) x,
      generate_series(-15,15,5) y
  ) foo
) bar;
