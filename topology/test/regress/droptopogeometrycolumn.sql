set client_min_messages to ERROR;

-- Drop corrupted topology (with missing layer tables)
-- See http://trac.osgeo.org/postgis/ticket/3016
SELECT NULL FROM topology.CreateTopology('t3016');
CREATE TABLE t3016f (id int);
SELECT NULL FROM topology.AddTopoGeometryColumn('t3016', 'public', 't3016f', 'g', 'LINE');
DROP TABLE t3016.relation;
SELECT 't3016.drop', topology.DropTopoGeometryColumn('public','t3016f','g');
SELECT 't3016', 'UNEXPECTED LAYER', * FROM topology.layer WHERE table_name = 't5118f';
DROP TABLE t3016f;
SELECT NULL FROM topology.DropTopology('t3016');

-- Drop corrupted topology (with missing layer sequence)
-- See http://trac.osgeo.org/postgis/ticket/5118
SELECT NULL FROM topology.CreateTopology('t5118');
CREATE TABLE t5118f (id int);
SELECT NULL FROM topology.AddTopoGeometryColumn('t5118', 'public', 't5118f', 'g', 'LINE');
INSERT INTO t5118f(g) SELECT toTopoGeom('LINESTRING(0 0, 10 0)', 't5118', 1);
DROP SEQUENCE t5118.topogeo_s_1;
SELECT 't5118.drop', topology.DropTopoGeometryColumn('public','t5118f','g');
SELECT 't5118', 'UNEXPECTED LAYER', * FROM topology.layer WHERE table_name = 't5118f';
DROP TABLE t5118f;
SELECT NULL FROM topology.DropTopology('t5118');
