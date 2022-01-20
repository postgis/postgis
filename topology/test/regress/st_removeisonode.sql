\set VERBOSITY terse
set client_min_messages to ERROR;

-- Import city_data
\i :top_builddir/topology/test/load_topology.sql

-- Non isolated node (is used by an edge);
SELECT 'non-iso', topology.ST_RemoveIsoNode('city_data', 1);

-- Isolated node
SELECT 'iso', topology.ST_RemoveIsoNode('city_data',
  topology.TopoGeo_addPoint('city_data', 'POINT(100 100)')
);


-- See https://trac.osgeo.org/postgis/ticket/3231
CREATE TABLE city_data.poi(id serial primary key);
SELECT NULL FROM AddTopoGeometryColumn('city_data', 'city_data', 'poi', 'tg', 'POINT');
CREATE TABLE city_data.mix(id serial primary key);
SELECT NULL FROM AddTopoGeometryColumn('city_data', 'city_data', 'mix', 'tg', 'COLLECTION');
BEGIN;
INSERT INTO city_data.poi(tg) VALUES(
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    ARRAY[
      ARRAY[
        topology.TopoGeo_addPoint('city_data', 'POINT(100 100)'),
        1 -- node
      ]
    ]
  )
);
SELECT '#3231.point', 'remove', ST_RemoveIsoNode('city_data',
  topology.TopoGeo_addPoint('city_data', 'POINT(100 100)')
);
ROLLBACK;
BEGIN;
INSERT INTO city_data.mix(tg) VALUES(
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    4, -- Topology geometry type (point)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    ARRAY[
      ARRAY[
        topology.TopoGeo_addPoint('city_data', 'POINT(100 100)'),
        1 -- node
      ]
    ]
  )
);
SELECT '#3231.collection', 'remove', ST_RemoveIsoNode('city_data',
  topology.TopoGeo_addPoint('city_data', 'POINT(100 100)')
);
ROLLBACK;


SELECT NULL FROM DropTopology('city_data');
