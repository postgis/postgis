BEGIN;
SELECT NULL FROM topology.CreateTopology('x');
CREATE TABLE x.f(id int primary key);
SELECT NULL FROM topology.AddTopoGeometryColumn('x', 'x', 'f', 'g0', 'POINT');
SELECT NULL FROM topology.AddTopoGeometryColumn('x', 'x', 'f', 'g1', 'LINE');
SELECT NULL FROM topology.AddTopoGeometryColumn('x', 'x', 'f', 'g2', 'POLYGON');
SELECT NULL FROM topology.AddTopoGeometryColumn('x', 'x', 'f', 'g3', 'GEOMETRYCOLLECTION');

CREATE TABLE layer_backup AS SELECT * FROM topology.layer;

SELECT 'initial',
 layer_id,
 schema_name,
 table_name,
 feature_column,
 feature_type,
 level,
 child_id
FROM topology.layer ORDER BY schema_name, table_name, feature_column;


TRUNCATE topology.layer;

SELECT 'populate', * FROM topology.Populate_Topology_Layer()
ORDER BY schema_name, table_name, feature_column;

SELECT 'only-before', * FROM ( SELECT * FROM topology.layer EXCEPT SELECT * FROM layer_backup ) x;
SELECT 'only-after', * FROM ( SELECT * FROM layer_backup EXCEPT SELECT * FROM topology.layer ) x;

SELECT 'final',
 layer_id,
 schema_name,
 table_name,
 feature_column,
 feature_type,
 level,
 child_id
FROM topology.layer ORDER BY schema_name, table_name, feature_column;
