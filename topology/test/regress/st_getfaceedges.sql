set client_min_messages to ERROR;

SELECT topology.CreateTopology('tt') > 0;

SELECT topology.ST_GetFaceEdges(null, null);
SELECT topology.ST_GetFaceEdges('tt', null);
SELECT topology.ST_GetFaceEdges(null, 1);
SELECT topology.ST_GetFaceEdges('', 1);
SELECT topology.ST_GetFaceEdges('NonExistent', 1);

SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(2 2, 2  8)');        -- 1
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(2  8,  8  8)');      -- 2
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(8  8,  8 2, 2 2)');  -- 3
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(0 0, 0 10, 10 10)'); -- 4
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(0 0, 10 0)');        -- 5
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(10 10, 10 5)');      -- 6
SELECT 'E'||topology.AddEdge('tt', 'LINESTRING(10 0, 10 5)');       -- 7

SELECT 'F' ||
  topology.AddFace('tt',
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),(2 2, 2 8, 8 8, 8 2, 2 2))');
SELECT 'F' ||
  topology.AddFace('tt',
    'POLYGON((2 2, 2 8, 8 8, 8 2, 2 2))');

SELECT 'F1', (topology.ST_GetFaceEdges('tt', 1)).*;
SELECT 'F2', (topology.ST_GetFaceEdges('tt', 2)).*;

SELECT 'F0', (topology.ST_GetFaceEdges('tt', 0)).*;

SELECT topology.DropTopology('tt');

-- See https://trac.osgeo.org/postgis/ticket/3265
SELECT topology.CreateTopology('tt') > 0;
SELECT '#3265.1', 'E'||topology.TopoGeo_addLinestring('tt',
  'LINESTRING(150 150, 180 150, 180 180)');
SELECT '#3265.2', 'E'||topology.TopoGeo_addLinestring('tt',
  'LINESTRING(150 150, 180 180)');
SELECT '#3265.3', 'E'||topology.TopoGeo_addLinestring('tt',
  'LINESTRING(178 170, 178 161, 170 161, 178 170)');
SELECT '#3265.4', 0, ST_GetFaceEdges('tt', 0);
SELECT '#3265.5', 1, ST_GetFaceEdges('tt', 1);
SELECT '#3265.6', 2, ST_GetFaceEdges('tt', 2);
SELECT NULL FROM topology.DropTopology('tt');

-- See https://trac.osgeo.org/postgis/ticket/5951
SELECT NULL FROM topology.CreateTopology('t5951');
SELECT NULL FROM topology.TopoGeo_addLinestring('t5951', 'LINESTRING(0 0, 10 0)');
SELECT NULL FROM topology.TopoGeo_addLinestring('t5951', 'LINESTRING(0 0, 5 5)');
SELECT NULL FROM topology.TopoGeo_addLinestring('t5951', 'LINESTRING(5 5, 10 0)');
SELECT NULL FROM topology.TopoGeo_addLinestring('t5951', 'LINESTRING(5 5, 10 5)');
SELECT NULL FROM topology.TopoGeo_addLinestring('t5951', 'LINESTRING(10 0, 10 5)');
UPDATE t5951.edge_data SET left_face = 0, right_face = 0 WHERE edge_id = 3;
SELECT '#5951.1', topology.ST_GetFaceEdges('t5951', face_id) FROM t5951.face WHERE face_id > 0;
SELECT NULL FROM topology.DropTopology('t5951');
