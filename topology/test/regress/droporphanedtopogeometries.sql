set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i :regdir/../topology/test/load_features.sql
\i :regdir/../topology/test/more_features.sql

SELECT 'initial-topogeoms', count(DISTINCT (layer_id, topogeo_id))
FROM city_data.relation;

DELETE FROM features.traffic_signs
WHERE id(feature) = (
	SELECT min(id(feature))
	FROM features.traffic_signs
);

SELECT 'after-delete-summary',
	strpos(topology.TopologySummary('city_data'), 'missing topogeoms') > 0;

SELECT 'drop-orphans', topology.DropOrphanedTopoGeometries('city_data');

SELECT 'after-drop-summary',
	strpos(topology.TopologySummary('city_data'), 'missing topogeoms') > 0;

SELECT 'after-drop-topogeoms', count(DISTINCT (layer_id, topogeo_id))
FROM city_data.relation;

CREATE TABLE features.traffic_sign_groups (
	feature_name VARCHAR PRIMARY KEY,
	fid serial
);
SELECT NULL FROM topology.AddTopoGeometryColumn(
	'city_data',
	'features',
	'traffic_sign_groups',
	'feature',
	'POINT',
	2
);
CREATE TEMP TABLE protected_child AS
SELECT id(feature) AS topogeo_id
FROM features.traffic_signs
WHERE feature_name = 'S2';
INSERT INTO features.traffic_sign_groups(feature_name, feature)
SELECT
	'S2-group',
	topology.CreateTopoGeom(
		'city_data',
		1,
		(
			SELECT layer_id
			FROM topology.layer
			WHERE topology_id = id(topology.findTopology('city_data'))
			AND schema_name = 'features'
			AND table_name = 'traffic_sign_groups'
			AND feature_column = 'feature'
		),
		ARRAY[ARRAY[topogeo_id, 2]]
	)
FROM protected_child;
DELETE FROM features.traffic_signs
WHERE id(feature) = (
	SELECT topogeo_id
	FROM protected_child
);

SELECT 'hier-child-protected', topology.DropOrphanedTopoGeometries('city_data');

SELECT 'hier-child-remains', count(*)
FROM city_data.relation
WHERE layer_id = 2
AND topogeo_id = (
	SELECT topogeo_id
	FROM protected_child
);

ALTER TABLE features.city_streets RENAME TO missing_table;

SELECT 'skip-missing-table', topology.DropOrphanedTopoGeometries('city_data');

ALTER TABLE features.missing_table RENAME TO city_streets;

ALTER TABLE features.city_streets RENAME COLUMN feature TO missing_feature;

SELECT 'skip-missing-column', topology.DropOrphanedTopoGeometries('city_data');

ALTER TABLE features.city_streets RENAME COLUMN missing_feature TO feature;

ALTER TABLE city_data.relation DISABLE TRIGGER relation_integrity_checks;
INSERT INTO city_data.relation(layer_id, topogeo_id, element_id, element_type)
VALUES (999, 999, 1, 1);
ALTER TABLE city_data.relation ENABLE TRIGGER relation_integrity_checks;

SELECT 'skip-unregistered-layer', topology.DropOrphanedTopoGeometries('city_data');

SELECT 'unregistered-remains', count(*)
FROM city_data.relation
WHERE layer_id = 999
AND topogeo_id = 999;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
