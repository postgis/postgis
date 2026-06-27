set client_min_messages to ERROR;

SELECT topology.CreateTopology('tt') > 0;

SELECT 'e1',  topology.addEdge('tt', 'LINESTRING(0 0, 10 0)');
SELECT 'e2',  topology.addEdge('tt', 'LINESTRING(10 0, 10 10)');
SELECT 'e3',  topology.addEdge('tt', 'LINESTRING(0 10, 10 10)');
SELECT 'e4',  topology.addEdge('tt', 'LINESTRING(0 0, 0 10)');
SELECT 'e5',  topology.addEdge('tt', 'LINESTRING(0 0, 0 -10)');
SELECT 'e6',  topology.addEdge('tt', 'LINESTRING(10 10, 20 10)');
SELECT 'e7',  topology.addEdge('tt', 'LINESTRING(20 10, 20 0)');
SELECT 'e8',  topology.addEdge('tt', 'LINESTRING(20 0, 10 0)');
SELECT 'e9',  topology.addEdge('tt', 'LINESTRING(10 0, 0 -10)');
SELECT 'e10',  topology.addEdge('tt', 'LINESTRING(2 2, 5 2, 2 5)');
SELECT 'e11',  topology.addEdge('tt', 'LINESTRING(2 2, 2 5)');

-- Call, check linking
SELECT topology.polygonize('tt');
SELECT face_id, Box2d(mbr) from tt.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt.edge ORDER by edge_id;

-- Call again and recheck linking (shouldn't change anything)
SELECT topology.polygonize('tt');
SELECT face_id, Box2d(mbr) from tt.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt.edge ORDER by edge_id;

SELECT topology.DropTopology('tt');

SELECT topology.CreateTopology('tt_bbox') > 0;

SELECT 'b1', topology.addEdge('tt_bbox', 'LINESTRING(0 0, 10 0)');
SELECT 'b2', topology.addEdge('tt_bbox', 'LINESTRING(10 0, 10 10)');
SELECT 'b3', topology.addEdge('tt_bbox', 'LINESTRING(10 10, 0 10)');
SELECT 'b4', topology.addEdge('tt_bbox', 'LINESTRING(0 10, 0 0)');
SELECT 'b5', topology.addEdge('tt_bbox', 'LINESTRING(100 0, 110 0)');
SELECT 'b6', topology.addEdge('tt_bbox', 'LINESTRING(110 0, 110 10)');
SELECT 'b7', topology.addEdge('tt_bbox', 'LINESTRING(110 10, 100 10)');
SELECT 'b8', topology.addEdge('tt_bbox', 'LINESTRING(100 10, 100 0)');

SELECT topology.polygonize('tt_bbox', 'POLYGON((-1 -1, 11 -1, 11 11, -1 11, -1 -1))');
SELECT face_id, Box2d(mbr) from tt_bbox.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt_bbox.edge ORDER by edge_id;

SELECT topology.polygonize('tt_bbox', 'POLYGON((-1 -1, 11 -1, 11 11, -1 11, -1 -1))');
SELECT face_id, Box2d(mbr) from tt_bbox.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt_bbox.edge ORDER by edge_id;

SELECT topology.CreateTopology('tt_bbox_leak') > 0;

SELECT 'l1', topology.addEdge('tt_bbox_leak', 'LINESTRING(0 5, 5 10)');
SELECT 'l2', topology.addEdge('tt_bbox_leak', 'LINESTRING(5 10, 10 5)');
SELECT 'l3', topology.addEdge('tt_bbox_leak', 'LINESTRING(10 5, 5 0)');
SELECT 'l4', topology.addEdge('tt_bbox_leak', 'LINESTRING(5 0, 0 5)');
SELECT 'l5', topology.addEdge('tt_bbox_leak', 'LINESTRING(1 5, 2 5)');

SELECT topology.polygonize('tt_bbox_leak', 'POLYGON((4 -1, 6 -1, 6 11, 4 11, 4 -1))');
SELECT face_id, Box2d(mbr) from tt_bbox_leak.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt_bbox_leak.edge ORDER by edge_id;

SELECT topology.polygonize('tt_bbox_leak');
SELECT face_id, Box2d(mbr) from tt_bbox_leak.face ORDER by face_id;
SELECT edge_id, left_face, right_face from tt_bbox_leak.edge ORDER by edge_id;

SELECT topology.DropTopology('tt_bbox_leak');

SET client_min_messages TO NOTICE;
DO $$
BEGIN
  PERFORM topology.polygonize('tt_bbox', 'POLYGON((0 0, 1 0, 0 1, 0 0))');
EXCEPTION WHEN OTHERS THEN
  RAISE NOTICE 'non-rectangle bbox: % %', SQLSTATE, SQLERRM;
END
$$;
SET client_min_messages TO ERROR;

SELECT topology.DropTopology('tt_bbox');
