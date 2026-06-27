SELECT NULL FROM topology.CreateTopology('ee_topo', 0);

CREATE TABLE ee_line_features(id integer);
SELECT '#2074.line_layer',
	topology.AddTopoGeometryColumn('ee_topo', 'public', 'ee_line_features', 'tg', 'LINESTRING');
SELECT NULL FROM topology.TopoGeo_AddLineString('ee_topo', 'LINESTRING(0 0, 10 10)');
INSERT INTO ee_line_features(id, tg)
	VALUES (1, topology.toTopoGeom('LINESTRING(0 0, 10 10)'::geometry, 'ee_topo', 1));
ANALYZE ee_topo.edge_data;

SELECT '#2074.line_extent',
	ST_AsText(
		ST_BoundingDiagonal(
			ST_EstimatedExtent('public', 'ee_line_features', 'tg')
		),
		0
	);

CREATE TABLE ee_point_features(id integer);
SELECT '#2074.point_layer',
	topology.AddTopoGeometryColumn('ee_topo', 'public', 'ee_point_features', 'tg', 'POINT');
SELECT NULL FROM topology.TopoGeo_AddPoint('ee_topo', 'POINT(20 30)');
INSERT INTO ee_point_features(id, tg)
	VALUES (1, topology.toTopoGeom('POINT(20 30)'::geometry, 'ee_topo', 2));
ANALYZE ee_topo.node;

SELECT '#2074.point_extent',
	ST_AsText(
		ST_BoundingDiagonal(
			ST_EstimatedExtent('public', 'ee_point_features', 'tg')
		),
		0
	);

CREATE TABLE ee_mixed_features(id integer);
SELECT '#2074.mixed_layer',
	topology.AddTopoGeometryColumn('ee_topo', 'public', 'ee_mixed_features', 'tg', 'GEOMETRY');
WITH point AS (
	SELECT topology.TopoGeo_AddPoint('ee_topo', 'POINT(30 40)') AS id
),
line AS (
	SELECT id
	FROM topology.TopoGeo_AddLineString('ee_topo', 'LINESTRING(5 5, 15 25)') AS id
)
INSERT INTO ee_mixed_features(id, tg)
	SELECT 1, topology.CreateTopoGeom(
		'ee_topo',
		4,
		3,
		ARRAY[
			ARRAY[point.id, 1],
			ARRAY[line.id, 2]
		]::topology.TopoElementArray
	)
	FROM point, (SELECT min(id) AS id FROM line) AS line;
ANALYZE ee_topo.node;
ANALYZE ee_topo.edge_data;

SELECT '#2074.mixed_extent',
	ST_AsText(
		ST_BoundingDiagonal(
			ST_EstimatedExtent('public', 'ee_mixed_features', 'tg')
		),
		0
	);

SET client_min_messages TO WARNING;
SELECT NULL FROM topology.DropTopology('ee_topo');
RESET client_min_messages;
DROP TABLE ee_line_features;
DROP TABLE ee_point_features;
DROP TABLE ee_mixed_features;
