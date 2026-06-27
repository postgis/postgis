set client_min_messages to ERROR;

SELECT NULL FROM topology.CreateTopology('t1185');

CREATE TABLE t1185f(id int);
SELECT 'point_layer', topology.AddTopoGeometryColumn('t1185', 'public', 't1185f', 'g_point', 'POINT');
SELECT 'line_layer', topology.AddTopoGeometryColumn('t1185', 'public', 't1185f', 'g_line', 'LINE');

INSERT INTO t1185f(id, g_point, g_line)
VALUES (
  1,
  topology.toTopoGeom('POINT(0 0)'::geometry, 't1185', 1),
  topology.toTopoGeom('LINESTRING(0 0, 1 0)'::geometry, 't1185', 2)
);

CREATE TABLE t1185_parent(id int);
SELECT 'parent_layer', topology.AddTopoGeometryColumn('t1185', 'public', 't1185_parent', 'g_parent', 'POINT', 1);

CREATE OR REPLACE FUNCTION t1185_try_drop_topogeometry_table()
RETURNS text AS
$$
BEGIN
  PERFORM topology.DropTopoGeometryTable('public', 't1185f');
  RETURN 'dropped';
EXCEPTION WHEN OTHERS THEN
  RETURN SQLERRM;
END
$$
LANGUAGE 'plpgsql';

SELECT 'parent-block',
  t1185_try_drop_topogeometry_table() =
    'Cannot drop public.t1185f because layer 1 is referenced by hierarchical layer 3 (public.t1185_parent.g_parent)';

SELECT 'parent-still-registered', count(*) = 3
FROM topology.layer
WHERE topology_id = id(topology.findTopology('t1185'));

SELECT '#1185.drop-parent-column', topology.DropTopoGeometryColumn('public', 't1185_parent', 'g_parent');

SELECT 'before',
  count(*) = 2 AS layers,
  (SELECT count(*) > 0 FROM t1185.relation) AS relations
FROM topology.layer
WHERE schema_name = 'public'
AND table_name = 't1185f';

SELECT '#1185.drop', topology.DropTopoGeometryTable('public', 't1185f');

SELECT 'after',
  to_regclass('public.t1185f') IS NULL AS table_dropped,
  count(*) = 0 AS layers_dropped,
  (SELECT count(*) = 0 FROM t1185.relation) AS relations_dropped
FROM topology.layer
WHERE schema_name = 'public'
AND table_name = 't1185f';

SELECT NULL FROM topology.DropTopology('t1185');
DROP TABLE t1185_parent;
DROP FUNCTION t1185_try_drop_topogeometry_table();
