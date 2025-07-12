SET client_min_messages TO WARNING;
SELECT E'--missing--\n' || TopologySummary('test');
CREATE SCHEMA test;
CREATE SCHEMA test2;
SELECT E'--empty--\n' || TopologySummary('test');
CREATE TABLE test.node(node_id int);
CREATE TABLE test2.node(node_id int);
SELECT E'--node--\n' || TopologySummary('test');
CREATE TABLE test.edge(edge_id int);
CREATE TABLE test2.edge(edge_id int);
SELECT E'--node+edge--\n' || TopologySummary('test');
CREATE TABLE test.face(face_id int);
CREATE TABLE test2.face(face_id int);
SELECT E'--node+edge+face--\n' || TopologySummary('test');
CREATE TABLE test.relation(id int);
CREATE TABLE test2.relation(id int);
SELECT E'--node+edge+face+corrupted_relation--\n' || TopologySummary('test');
ALTER TABLE test.relation ADD layer_id int, ADD topogeo_id int;
SELECT E'--node+edge+face+relation--\n' || TopologySummary('test');
INSERT INTO test.relation (layer_id, topogeo_id) VALUES (1,1);
SELECT E'--node+edge+face+relation+topogeom--\n' || TopologySummary('test');
INSERT INTO topology.topology (id,name,srid,precision,hasz)
  VALUES(1,'test',10,20,false);
INSERT INTO topology.topology (id,name,srid,precision,hasz,useslargeids)
  VALUES(2,'test2',4326,20,true,true);
SELECT E'--registered_topo--\n' || TopologySummary('test');
SELECT E'--registered_topo--\n' || TopologySummary('test2');
INSERT INTO topology.layer (topology_id,layer_id, schema_name, table_name, feature_column, feature_type, level, child_id)
  VALUES(1,1,'test','t','c', 1, 0, null);
SELECT E'--registered_missing_layer_table--\n' || TopologySummary('test');
CREATE TABLE test.t(i int);
SELECT E'--registered_missing_layer_column--\n' || TopologySummary('test');
ALTER TABLE test.t ADD c TopoGeometry;
SELECT E'--registered_layer_missing_topogeom--\n' || TopologySummary('test');
INSERT INTO test.t(c) VALUES ( (1,2,1,1) );
SELECT E'--registered_layer_missing_topogeom_in_proper_layer--\n' || TopologySummary('test');
UPDATE test.t SET c.layer_id = 1, c.topology_id = topology_id(c)+1 WHERE layer_id(c) = 2;
SELECT E'--registered_layer_missing_topogeom_in_proper_topo--\n' || TopologySummary('test');
UPDATE test.t SET c.topology_id = topology_id(c)-1 WHERE layer_id(c) = 1;
SELECT E'--registered_layer--\n' || TopologySummary('test');
INSERT INTO test.face(face_id) VALUES (-1);
SELECT E'--pivot face--\n' || TopologySummary('test');
DROP SCHEMA test CASCADE;
DROP SCHEMA test2 CASCADE;
SELECT E'--registered+missing--\n' || TopologySummary('test');
SELECT E'--registered+missing--\n' || TopologySummary('test2');

DELETE FROM topology.layer WHERE topology_id = 1;
DELETE FROM topology.topology WHERE id = 1;
DELETE FROM topology.topology WHERE id = 2;

-- TODO: test hierarchical
