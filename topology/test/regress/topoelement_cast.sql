\set VERBOSITY terse
set client_min_messages to WARNING;

select NULL FROM createtopology('tt', 4326);

-- layer 1 is PUNTUAL
CREATE TABLE tt.f_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_point', 'g', 'POINT');

-- layer 2 is HIERARCHICAL PUNTUAL
CREATE TABLE tt.f_hier_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_point', 'g', 'POINT', 1);

-- populate base
INSERT INTO tt.f_point(lbl,g)
SELECT
  x::text||y::text,
  topology.toTopoGeom(
    ST_Point(x,y,4326),
    'tt',
    layer_id(findLayer('tt','f_point', 'g'))
  )
FROM generate_series(50,60, 10) AS x
CROSS JOIN generate_series(70,90, 20) AS y;

-- populate hierarchy, cast
INSERT INTO  tt.f_hier_point(lbl, g)
SELECT
  format('bycast:%s', left(f.lbl,2)),
  createTopoGeom(
    'tt',
    1,
    layer_id(findLayer('tt','f_hier_point', 'g')),
    TopoElementArray_agg( f.g::topology.topoelement ORDER BY f.lbl)
  )
FROM  tt.f_point AS f
GROUP BY left(f.lbl,2);

-- using FUNCTION

-- populate hierarchy
INSERT INTO  tt.f_hier_point(lbl, g)
SELECT
  format('byfunc:%s', right(f.lbl,2)),
  createTopoGeom(
    'tt',
    1,
    layer_id(findLayer('tt','f_hier_point', 'g')),
    TopoElementArray_agg( f.g::topology.topoelement ORDER BY f.lbl)
  )
FROM  tt.f_point AS f
GROUP BY right(f.lbl,2);

SELECT 't1', lbl, ST_AsText(g::geometry)
FROM tt.f_hier_point
ORDER BY lbl;

SELECT 't5933.explicit', id(topology.CreateTopoGeom(
  'tt',
  1,
  layer_id(findLayer('tt','f_hier_point', 'g')),
  (SELECT g::topology.TopoElement FROM tt.f_point WHERE lbl = '5090'),
  5933
));

SELECT 't5933.implicit', ST_AsText(
  topology.CreateTopoGeom(
    'tt',
    1,
    layer_id(findLayer('tt','f_hier_point', 'g')),
    (SELECT g::topology.TopoElement FROM tt.f_point WHERE lbl = '5070')
  )::geometry
);

SELECT 't5933.malformed', topology.CreateTopoGeom(
  'tt',
  1,
  layer_id(findLayer('tt','f_hier_point', 'g')),
  ARRAY[1,2,3]::topology.TopoElementArray
);

-- Cleanup
SELECT NULL FROM DropTopology('tt');
