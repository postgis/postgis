BEGIN;
set client_min_messages to ERROR;

SELECT topology.CreateTopology('Main',4326) > 0; -- Create the master topology
SELECT topology.st_createtopogeo('Main','SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 8 9, 4 2, 5 5))');
SELECT max(edge_id)||' max edge_id Main.edge_data' from "Main".edge_data;
SELECT max(node_id)||' max node_id Main.node' from "Main".node;
SELECT max(face_id)||' max node_id Main.face' from "Main".face;


SELECT topology.CreateTopology('temp01',4326) > 0; -- Create temp01 topologies to use the masters primary keys
ALTER TABLE temp01.edge_data ALTER COLUMN edge_id set default nextval('"Main".edge_data_edge_id_seq'::regclass);
ALTER TABLE temp01.node ALTER COLUMN node_id set default nextval('"Main".node_node_id_seq'::regclass);
ALTER TABLE temp01.face ALTER COLUMN face_id set default nextval('"Main".face_face_id_seq'::regclass);
DROP SEQUENCE temp01.edge_data_edge_id_seq; -- Drop sequences since we do no need them any more since we now use the main
DROP SEQUENCE temp01.node_node_id_seq;
DROP SEQUENCE temp01.face_face_id_seq;

SELECT topology.CreateTopology('temp02',4326) > 0; -- Create temp02 topologies to use the masters primary keys
ALTER TABLE temp02.edge_data ALTER COLUMN edge_id set default nextval('"Main".edge_data_edge_id_seq'::regclass);
ALTER TABLE temp02.node ALTER COLUMN node_id set default nextval('"Main".node_node_id_seq'::regclass);
ALTER TABLE temp02.face ALTER COLUMN face_id set default nextval('"Main".face_face_id_seq'::regclass);
DROP SEQUENCE temp02.edge_data_edge_id_seq; -- Drop sequences since we do no need them any more since we now use the main
DROP SEQUENCE temp02.node_node_id_seq;
DROP SEQUENCE temp02.face_face_id_seq;

SELECT topology.st_createtopogeo('temp01', 'SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 8 9, 4 2, 5 5))'); -- Add to temp01 id will start on 1
SELECT max(edge_id)||' max edge_id temp01.edge_data' from temp01.edge_data;
SELECT max(node_id)||' max node_id temp01.node' from temp01.node;
SELECT max(face_id)||' max face_id temp01.face' from temp01.face;


SELECT topology.addEdge('temp02', 'LINESTRING(8 10, 10 10, 10 12, 8 10)'); -- Add to temp01 id will not start on 1
SELECT max(edge_id)||' max edge_id temp02.edge_data' from temp02.edge_data;
SELECT max(node_id)||' max node_id temp02.node' from temp02.node;
SELECT max(face_id)||' max face_id temp02.face' from temp02.face;

ROLLBACK;
