set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

SELECT pg_get_function_result('topology.GetNodeEdges(varchar, bigint)'::regprocedure);

SELECT 'N'||node_id, (topology.GetNodeEdges('city_data', node_id)).*
	FROM city_data.node ORDER BY node_id, sequence;

SELECT topology.DropTopology('city_data');
