\set VERBOSITY terse
set client_min_messages to ERROR;

SELECT NULL FROM topology.CreateTopology('t5668_record', 0);

CREATE TEMP TABLE t5668_saved_topology AS
SELECT id, name, srid, precision, hasz, useslargeids
FROM topology.topology
WHERE name = 't5668_record';

CREATE FUNCTION pg_temp.t5668_addpoint_result(t topology.topology)
RETURNS text AS $$
BEGIN
  PERFORM topology.TopoGeo_AddPoint(t, 'POINT(0 0)'::geometry);
  RETURN 'unexpected success';
EXCEPTION WHEN OTHERS THEN
  RETURN SQLERRM;
END;
$$ LANGUAGE plpgsql;

SELECT 'null id',
       pg_temp.t5668_addpoint_result((NULL, 't5668_record', 0, 0, false, false)::topology.topology);

SELECT 'null srid',
       pg_temp.t5668_addpoint_result((0, 't5668_record', NULL, 0, false, false)::topology.topology);

DELETE FROM topology.topology
WHERE name = 't5668_record';

WITH topo AS (
  SELECT (id, name, srid, precision, hasz, useslargeids)::topology.topology AS t
  FROM t5668_saved_topology
)
SELECT 'point', topology.TopoGeo_AddPoint(t, 'POINT(0 0)'::geometry)
FROM topo;

WITH topo AS (
  SELECT (id, name, srid, precision, hasz, useslargeids)::topology.topology AS t
  FROM t5668_saved_topology
)
SELECT 'line', array_agg(edge_id ORDER BY edge_id)
FROM topo, topology.TopoGeo_AddLineString(t, 'LINESTRING(1 0, 2 0)'::geometry) AS edge_id;

WITH topo AS (
  SELECT (id, name, srid, precision, hasz, useslargeids)::topology.topology AS t
  FROM t5668_saved_topology
)
SELECT 'polygon', array_agg(face_id ORDER BY face_id)
FROM topo, topology.TopoGeo_AddPolygon(t, 'POLYGON((10 10, 11 10, 11 11, 10 11, 10 10))'::geometry) AS face_id;

WITH topo AS (
  SELECT (id, name, srid, precision, hasz, useslargeids)::topology.topology AS t
  FROM t5668_saved_topology
)
SELECT 'load', topology.TopoGeo_LoadGeometry(t, 'GEOMETRYCOLLECTION(POINT(20 20), LINESTRING(20 20, 21 20))'::geometry)
FROM topo;

SELECT 'counts',
       (SELECT count(*) FROM t5668_record.node),
       (SELECT count(*) FROM t5668_record.edge_data),
       (SELECT count(*) FROM t5668_record.face WHERE face_id > 0);

INSERT INTO topology.topology
SELECT * FROM t5668_saved_topology;

SELECT NULL FROM topology.DropTopology('t5668_record');
