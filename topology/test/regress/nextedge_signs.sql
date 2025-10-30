set client_min_messages to ERROR;

SELECT topology.CreateTopology('signed_edges', 0) > 0;

SELECT 'E1', topology.AddEdge('signed_edges', 'LINESTRING(0 0, 1 0)');
SELECT 'E2', topology.AddEdge('signed_edges', 'LINESTRING(1 0, 1 1)');

SELECT 'sign-consistency', bool_and(next_left_edge = -next_right_edge)
  FROM signed_edges.edge;

SELECT 'absolute-id',
       array_agg(edge_id::text || ':' || abs(next_left_edge)::text ORDER BY edge_id)
  FROM signed_edges.edge;

SELECT topology.DropTopology('signed_edges');
