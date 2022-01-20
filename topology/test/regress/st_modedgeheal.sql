\set VERBOSITY terse
set client_min_messages to ERROR;

-- Import city_data
\i :top_builddir/topology/test/load_topology.sql

SELECT topology.ST_ModEdgeHeal('city_data', 1, null);
SELECT topology.ST_ModEdgeHeal('city_data', null, 1);
SELECT topology.ST_ModEdgeHeal(null, 1, 2);
SELECT topology.ST_ModEdgeHeal('', 1, 2);

-- Not connected edges
SELECT topology.ST_ModEdgeHeal('city_data', 25, 3);

-- Other connected edges
SELECT topology.ST_ModEdgeHeal('city_data', 9, 10);

-- Closed edge
SELECT topology.ST_ModEdgeHeal('city_data', 2, 3);
SELECT topology.ST_ModEdgeHeal('city_data', 3, 2);

-- Heal to self
SELECT topology.ST_ModEdgeHeal('city_data', 25, 25);

-- Good ones {

-- check state before
SELECT 'E'||edge_id,
  ST_AsText(ST_StartPoint(geom)), ST_AsText(ST_EndPoint(geom)),
  next_left_edge, next_right_edge, start_node, end_node
  FROM city_data.edge_data ORDER BY edge_id;
SELECT 'N'||node_id FROM city_data.node;

-- No other edges involved, SQL/MM caseno 2, drops node 6
SELECT 'MH(4,5)', topology.ST_ModEdgeHeal('city_data', 4, 5);

-- Face and other edges involved, SQL/MM caseno 1, drops node 16
SELECT 'MH(21,6)', topology.ST_ModEdgeHeal('city_data', 21, 6);
-- Face and other edges involved, SQL/MM caseno 2, drops node 19
SELECT 'MH(8,15)', topology.ST_ModEdgeHeal('city_data', 8, 15);
-- Face and other edges involved, SQL/MM caseno 3, drops node 8
SELECT 'MH(12,22)', topology.ST_ModEdgeHeal('city_data', 12, 22);
-- Face and other edges involved, SQL/MM caseno 4, drops node 11
SELECT 'MH(16,14)', topology.ST_ModEdgeHeal('city_data', 16, 14);

-- check state after
SELECT 'E'||edge_id,
  ST_AsText(ST_StartPoint(geom)), ST_AsText(ST_EndPoint(geom)),
  next_left_edge, next_right_edge, start_node, end_node
  FROM city_data.edge_data ORDER BY edge_id;
SELECT 'N'||node_id FROM city_data.node;

-- }

-- clean up
SELECT topology.DropTopology('city_data');

-------------------------------------------------------------------------
-------------------------------------------------------------------------
-------------------------------------------------------------------------

-- Now test in presence of features

SELECT topology.CreateTopology('t') > 1;
CREATE TABLE t.f_lin(id varchar);
SELECT 'lin_layer', topology.AddTopoGeometryColumn('t', 't', 'f_lin','g', 'LINE');
CREATE TABLE t.f_poi(id varchar);
SELECT 'poi_layer', topology.AddTopoGeometryColumn('t', 't', 'f_poi','g', 'POINT');
CREATE TABLE t.f_mix(id varchar);
SELECT 'mix_layer', topology.AddTopoGeometryColumn('t', 't', 'f_mix','g', 'COLLECTION');

SELECT 'N'||topology.TopoGeo_AddPoint('t', 'POINT(2 8)');  -- 1
SELECT 'E'||topology.AddEdge('t', 'LINESTRING(2 2, 2  8)');        -- 1
SELECT 'E'||topology.AddEdge('t', 'LINESTRING(2  8,  8  8)');      -- 2

-- Cannot heal edges connected by node used in point TopoGeometry
-- See https://trac.osgeo.org/postgis/ticket/3239
INSERT INTO t.f_poi VALUES ('F+N1',
  topology.CreateTopoGeom('t', 1, 2, '{{1,1}}'));
SELECT 'unexpected-success-with-orphaned-point-topogeom-1',
  topology.ST_ModEdgeHeal('t', 1, 2);
SELECT 'unexpected-success-with-orphaned-point-topogeom-2',
  topology.ST_ModEdgeHeal('t', 2, 1);
WITH deleted AS ( DELETE FROM t.f_poi RETURNING g),
     clear AS ( SELECT ClearTopoGeom(g) FROM deleted)
     SELECT NULL FROM clear;

-- Cannot heal edges connected by node used in collection TopoGeometry
-- See https://trac.osgeo.org/postgis/ticket/3239
INSERT INTO t.f_mix VALUES ('F+N1',
  topology.CreateTopoGeom('t', 4, 3, '{{1,1}}'));
SELECT 'unexpected-success-with-orphaned-mix-topogeom-1',
  topology.ST_ModEdgeHeal('t', 1, 2);
SELECT 'unexpected-success-with-orphaned-mix-topogeom-2',
  topology.ST_ModEdgeHeal('t', 2, 1);
WITH deleted AS ( DELETE FROM t.f_mix RETURNING g),
     clear AS ( SELECT ClearTopoGeom(g) FROM deleted)
     SELECT NULL FROM clear;

---- Cannot heal edges when only one is used in line TopoGeometry
---- defined w/out one of the edges
INSERT INTO t.f_lin VALUES ('F+E1',
  topology.CreateTopoGeom('t', 2, 1, '{{1,2}}'));
SELECT topology.ST_ModEdgeHeal('t', 1, 2);
SELECT topology.ST_ModEdgeHeal('t', 2, 1);
WITH deleted AS ( DELETE FROM t.f_lin RETURNING g),
     clear AS ( SELECT ClearTopoGeom(g) FROM deleted)
     SELECT NULL FROM clear;

-- Cannot heal edges when only one is used in collection TopoGeometry
-- defined w/out one of the edges
INSERT INTO t.f_mix VALUES ('F+E1',
  topology.CreateTopoGeom('t', 4, 3, '{{1,2}}'));
SELECT topology.ST_ModEdgeHeal('t', 1, 2);
SELECT topology.ST_ModEdgeHeal('t', 2, 1);
WITH deleted AS ( DELETE FROM t.f_mix RETURNING g),
     clear AS ( SELECT ClearTopoGeom(g) FROM deleted)
     SELECT NULL FROM clear;

-- This is for ticket #941
SELECT topology.ST_ModEdgeHeal('t', 1, 200);
SELECT topology.ST_ModEdgeHeal('t', 100, 2);

-- Now see how signed edges are updated

SELECT 'tg-update', 'E'||topology.AddEdge('t', 'LINESTRING(0 0, 5 0)');         -- 3
SELECT 'tg-update', 'E'||topology.AddEdge('t', 'LINESTRING(10 0, 5 0)');        -- 4

INSERT INTO t.f_lin VALUES ('F+E3-E4',
  topology.CreateTopoGeom('t', 2, 1, '{{3,2},{-4,2}}'));
INSERT INTO t.f_lin VALUES ('F-E3+E4',
  topology.CreateTopoGeom('t', 2, 1, '{{-3,2},{4,2}}'));

SELECT
  'tg-update', 'before',
  r.topogeo_id, r.element_id
  FROM t.relation r, t.f_lin f WHERE
  r.layer_id = layer_id(f.g) AND r.topogeo_id = id(f.g)
  AND r.topogeo_id in (2,3)
  ORDER BY r.layer_id, r.topogeo_id, r.element_id;

-- This is fine, but will have to tweak definition of
-- 'F+E3-E4' and 'F-E3+E4'
SELECT 'tg-update', 'MH(3,4)', topology.ST_ModEdgeHeal('t', 3, 4);

SELECT
  'tg-update', 'after',
  r.topogeo_id, r.element_id
  FROM t.relation r, t.f_lin f WHERE
  r.layer_id = layer_id(f.g) AND r.topogeo_id = id(f.g)
  AND r.topogeo_id in (2,3)
  ORDER BY r.layer_id, r.topogeo_id, r.element_id;

-- This is for ticket #942 (non-connected edges)
SELECT '#942', topology.ST_ModEdgeHeal('t', 1, 3);


SELECT topology.DropTopology('t');

-------------------------------------------------------------------------
-------------------------------------------------------------------------
-------------------------------------------------------------------------

-- Test edges sharing both endpoints
-- See http://trac.osgeo.org/postgis/ticket/1955

SELECT '#1955', topology.CreateTopology('t') > 1;

SELECT '#1955.1', 'E'||topology.AddEdge('t', 'LINESTRING(0 0, 10 0, 10 10)');        -- 1
SELECT '#1955.1', 'E'||topology.AddEdge('t', 'LINESTRING(0 0, 0 10, 10 10)'); ;      -- 2

SELECT '#1955.1', count(node_id), 'start nodes' as label FROM t.node GROUP BY label;

-- Deletes second node. Not very predictable which one is removed
SELECT '#1955.1', 'H:1,2', 'N' || topology.ST_ModEdgeHeal('t', 1, 2), 'deleted';

SELECT '#1955.1', count(node_id), 'nodes left' as label FROM t.node GROUP BY label;

SELECT '#1955.2', 'E'||topology.AddEdge('t', 'LINESTRING(50 0, 60 0, 60 10)');        -- 3
SELECT '#1955.2', 'E'||topology.AddEdge('t', 'LINESTRING(50 0, 50 10, 60 10)'); ;     -- 4
SELECT '#1955.2', 'E'||topology.AddEdge('t', 'LINESTRING(60 10, 70 10)'); ;           -- 5

SELECT '#1955.2', count(node_id), 'start nodes' as label FROM t.node GROUP BY label;

-- Only the start node can be deleted (50 0) because the other is shared by
-- another edge
SELECT '#1955.2', 'H:3,4', 'N' || topology.ST_ModEdgeHeal('t', 3, 4), 'deleted';

SELECT '#1955.2', count(node_id), 'nodes left' as label FROM t.node GROUP BY label;

SELECT '#1955.3', 'E'||topology.AddEdge('t', 'LINESTRING(80 0, 90 0, 90 10)');        -- 6
SELECT '#1955.3', 'E'||topology.AddEdge('t', 'LINESTRING(80 0, 80 10, 90 10)'); ;     -- 7
SELECT '#1955.3', 'E'||topology.AddEdge('t', 'LINESTRING(70 10, 80 0)'); ;            -- 8

SELECT '#1955.3', count(node_id), 'start nodes' as label FROM t.node GROUP BY label;

-- Only the end node can be deleted (90 10) because the other is shared by
-- another edge
SELECT '#1955.3', 'H:6,7', 'N' || topology.ST_ModEdgeHeal('t', 6, 7), 'deleted';

SELECT '#1955.3', count(node_id), 'nodes left' as label FROM t.node GROUP BY label;

SELECT '#1955', topology.DropTopology('t');

-------------------------------------------------------------------------
-------------------------------------------------------------------------
-------------------------------------------------------------------------

-- Another case of merging edges sharing both endpoints
-- See http://trac.osgeo.org/postgis/ticket/1998

SELECT '#1998.+', CreateTopology('t1998') > 1;
SELECT '#1998.N1', ST_AddIsoNode('t1998', 0, 'POINT(1 1)');
SELECT '#1998.N2', ST_AddIsoNode('t1998', 0, 'POINT(0 0)');
SELECT '#1998.E1', ST_AddEdgeModFace('t1998', 1, 1, 'LINESTRING(1 1,1 2,2 2,2 1,1 1)');
SELECT '#1998.E2', ST_AddEdgeModFace('t1998', 2, 1, 'LINESTRING(0 0,0 1,1 1)');
SELECT '#1998.E3', ST_AddEdgeModFace('t1998', 1, 2, 'LINESTRING(1 1,1 0,0 0)');
SELECT '#1998.X0' as lbl, count(*) FROM ValidateTopology('t1998') GROUP BY lbl;
SELECT '#1998.N-', ST_ModEdgeHeal('t1998', 2, 3);
SELECT '#1998.M2', ST_AsText(geom) FROM t1998.edge WHERE edge_id = 2;
SELECT '#1998.X1' as lbl, count(*) FROM ValidateTopology('t1998') GROUP BY lbl;
SELECT '#1998.-', topology.DropTopology('t1998');

-------------------------------------------------------------------------
-------------------------------------------------------------------------
-------------------------------------------------------------------------

-- TODO: test registered but unexistent topology
-- TODO: test registered but corrupted topology
--       (missing node, edge, relation...)
