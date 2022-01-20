\set VERBOSITY terse
set client_min_messages to ERROR;

-- Import city_data
\i :top_builddir/topology/test/load_topology.sql

CREATE TABLE city_data.f_lin(id serial primary key);
SELECT NULL FROM AddTopoGeometryColumn('city_data', 'city_data', 'f_lin', 'tg', 'LINE');
CREATE TABLE city_data.f_mix(id serial primary key);
SELECT NULL FROM AddTopoGeometryColumn('city_data', 'city_data', 'f_mix', 'tg', 'COLLECTION');

BEGIN;
INSERT INTO city_data.f_lin(tg) VALUES(
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    2, -- Topology geometry type (line)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{25,2}}'
  )
);
SELECT '#3248.line', topology.ST_RemoveIsoEdge('city_data', 25);
ROLLBACK;

BEGIN;
INSERT INTO city_data.f_mix(tg) VALUES(
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    4, -- Topology geometry type (collection)
    2, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{25,2}}'
  )
);
SELECT '#3248.collection', topology.ST_RemoveIsoEdge('city_data', 25);
ROLLBACK;

SELECT 'non-iso', topology.ST_RemoveIsoEdge('city_data', 1);
SELECT 'iso', topology.ST_RemoveIsoEdge('city_data', 25);

SELECT NULL FROM topology.DropTopology('city_data');

