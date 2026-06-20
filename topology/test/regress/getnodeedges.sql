set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

SELECT 'N'||node_id, (topology.GetNodeEdges('city_data', node_id)).*
	FROM city_data.node ORDER BY node_id, sequence;

SELECT 'AZ'||node_id, sequence, edge, round(azimuth::numeric, 6)
	FROM city_data.node
	CROSS JOIN LATERAL topology.GetNodeEdgesWithAzimuth('city_data', node_id)
	WHERE node_id IN (1, 2, 20)
	ORDER BY node_id, sequence;

SELECT topology.DropTopology('city_data');
