BEGIN;

SELECT topology.CreateTopology('topoperf_topogeometry');

CREATE TABLE topoperf_topogeometry.layer(x int, y int);

SELECT AddTopoGeometryColumn('topoperf_topogeometry','topoperf_topogeometry','layer','tg','POLYGON');

\timing on
INSERT INTO topoperf_topogeometry.layer(x, y, tg)
  SELECT x, y,
    toTopoGeom(
      ST_Buffer(ST_MakePoint(x,y), 2),
      'topoperf_topogeometry',
      1
    )
  FROM
    generate_series(-100, 100, 3) x,
    generate_series(-100, 100, 3) y;
