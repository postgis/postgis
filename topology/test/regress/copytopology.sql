set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology-4326.sql
\i ../load_features.sql
\i ../more_features.sql
\i ../hierarchy.sql

SELECT topology.CopyTopology('city_data', 'CITY_data_UP_down') > 0;

SELECT srid,precision FROM topology.topology WHERE name = 'CITY_data_UP_down';

SELECT 'nodes', count(node_id) FROM "CITY_data_UP_down".node;
SELECT * FROM "CITY_data_UP_down".node EXCEPT
SELECT * FROM "city_data".node;

SELECT 'edges', count(edge_id) FROM "CITY_data_UP_down".edge_data;
SELECT * FROM "CITY_data_UP_down".edge EXCEPT
SELECT * FROM "city_data".edge;

SELECT 'faces', count(face_id) FROM "CITY_data_UP_down".face;
SELECT * FROM "CITY_data_UP_down".face EXCEPT
SELECT * FROM "city_data".face;

SELECT 'relations', count(*) FROM "CITY_data_UP_down".relation;
SELECT * FROM "CITY_data_UP_down".relation EXCEPT
SELECT * FROM "city_data".relation;

SELECT 'layers', count(l.*) FROM topology.layer l, topology.topology t
WHERE l.topology_id = t.id and t.name = 'CITY_data_UP_down';
SELECT l.layer_id, l.feature_type, l.level FROM topology.layer l,
topology.topology t where l.topology_id = t.id and t.name = 'CITY_data_UP_down'
EXCEPT
SELECT l.layer_id, l.feature_type, l.level FROM topology.layer l,
topology.topology t where l.topology_id = t.id and t.name = 'city_data';

SELECT l.layer_id, l.schema_name, l.table_name, l.feature_column
FROM topology.layer l, topology.topology t
WHERE l.topology_id = t.id and t.name = 'CITY_data_UP_down'
ORDER BY l.layer_id;

-- Check sequences
SELECT tableoid::regclass AS sequence_name, last_value,  is_called from "CITY_data_UP_down".node_node_id_seq;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called from "CITY_data_UP_down".edge_data_edge_id_seq;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called from "CITY_data_UP_down".face_face_id_seq;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called from "CITY_data_UP_down".layer_id_seq;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called  from "CITY_data_UP_down".topogeo_s_1;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called  from "CITY_data_UP_down".topogeo_s_2;
SELECT tableoid::regclass AS sequence_name, last_value,  is_called  from "CITY_data_UP_down".topogeo_s_3;

-- See https://trac.osgeo.org/postgis/ticket/5119
COPY "CITY_data_UP_down".edge (edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom) FROM stdin;
1001	1	2	1001	-1001	0	0	SRID=4326;LINESTRING(1 1,2 2)
\.
SELECT '#5119', edge_id, abs_next_left_edge, abs_next_right_edge, ST_AsText(geom)
FROM "CITY_data_UP_down".edge_data
WHERE edge_id = 1001;
INSERT INTO "CITY_data_UP_down".edge (
  edge_id, start_node, end_node,
  next_left_edge, next_right_edge,
  left_face, right_face, geom
) VALUES (
  1002, 1, 2,
  1002, -1002,
  0, 0, 'SRID=4326;LINESTRING(2 2,3 3)'::geometry
)
RETURNING '#5119-returning', edge_id, ST_AsText(geom);

BEGIN;
CREATE SCHEMA fake_5119;
CREATE TABLE fake_5119.edge_data (
  edge_id integer,
  start_node integer,
  end_node integer,
  next_left_edge integer,
  abs_next_left_edge integer,
  next_right_edge integer,
  abs_next_right_edge integer,
  left_face integer,
  right_face integer,
  geom geometry
);
CREATE VIEW fake_5119.edge AS
SELECT
  NULL::integer AS edge_id,
  NULL::integer AS start_node,
  NULL::integer AS end_node,
  NULL::integer AS next_left_edge,
  NULL::integer AS next_right_edge,
  NULL::integer AS left_face,
  NULL::integer AS right_face,
  NULL::geometry AS geom
WHERE false;
CREATE TRIGGER edge_insert
  INSTEAD OF INSERT ON fake_5119.edge
  FOR EACH ROW
  EXECUTE FUNCTION topology._edge_insert();
CREATE FUNCTION fake_5119.try_edge_insert()
RETURNS text
AS $$
BEGIN
  INSERT INTO fake_5119.edge (
    edge_id, start_node, end_node,
    next_left_edge, next_right_edge,
    left_face, right_face, geom
  ) VALUES (
    1, 1, 1,
    1, -1,
    0, 0, 'LINESTRING(0 0,1 1)'::geometry
  );
  RETURN 'unexpected success';
EXCEPTION WHEN OTHERS THEN
  RETURN SQLERRM;
END
$$ LANGUAGE 'plpgsql' VOLATILE;
SELECT '#5119-fake', fake_5119.try_edge_insert();
SELECT '#5119-fake-edge-data', count(*) FROM fake_5119.edge_data;
ROLLBACK;

DO $$
DECLARE
	can_switch_role boolean;
	trigger_user name;
	audit_rows bigint;
BEGIN
	SELECT rolsuper INTO can_switch_role
	FROM pg_roles
	WHERE rolname = current_user;

	IF NOT can_switch_role THEN
		RETURN;
	END IF;

	DROP SCHEMA IF EXISTS t5119_priv CASCADE;
	DROP ROLE IF EXISTS t5119_viewer;
	DROP ROLE IF EXISTS t5119_owner;
	CREATE ROLE t5119_owner;
	CREATE ROLE t5119_viewer;
	GRANT t5119_owner TO CURRENT_USER;
	GRANT t5119_viewer TO CURRENT_USER;
	EXECUTE format('GRANT CREATE ON DATABASE %I TO t5119_owner', current_database());
	GRANT USAGE, SELECT ON SEQUENCE topology.topology_id_seq TO t5119_owner;
	GRANT INSERT, SELECT ON topology.topology TO t5119_owner;

	EXECUTE 'SET LOCAL ROLE t5119_owner';
	PERFORM topology.CreateTopology('t5119_priv', 4326);
	INSERT INTO t5119_priv.node(node_id, geom)
	VALUES
		(1, 'SRID=4326;POINT(0 0)'::geometry),
		(2, 'SRID=4326;POINT(1 1)'::geometry);
	CREATE TABLE t5119_priv.trigger_audit(username name);
	CREATE FUNCTION t5119_priv.audit_edge_insert()
	RETURNS trigger
	AS $audit$
	BEGIN
		INSERT INTO t5119_priv.trigger_audit(username) VALUES (current_user);
		RETURN NEW;
	END
	$audit$
	LANGUAGE 'plpgsql' VOLATILE;
	CREATE TRIGGER audit_edge_insert
		AFTER INSERT ON t5119_priv.edge_data
		FOR EACH ROW
		EXECUTE FUNCTION t5119_priv.audit_edge_insert();
	GRANT USAGE ON SCHEMA t5119_priv TO t5119_viewer;
	GRANT INSERT ON t5119_priv.edge TO t5119_viewer;
	EXECUTE 'RESET ROLE';

	EXECUTE 'SET LOCAL ROLE t5119_viewer';
	INSERT INTO t5119_priv.edge (
		edge_id, start_node, end_node,
		next_left_edge, next_right_edge,
		left_face, right_face, geom
	) VALUES (
		1, 1, 2,
		1, -1,
		0, 0, 'SRID=4326;LINESTRING(0 0,1 1)'::geometry
	);
	EXECUTE 'RESET ROLE';

	SELECT count(*), min(username) INTO audit_rows, trigger_user
	FROM t5119_priv.trigger_audit;
	IF audit_rows != 1 OR trigger_user IS DISTINCT FROM 't5119_owner' THEN
		RAISE EXCEPTION 'edge_data trigger ran as %, expected t5119_owner (rows=%)', trigger_user, audit_rows;
	END IF;

	PERFORM topology.DropTopology('t5119_priv');
	REVOKE INSERT, SELECT ON topology.topology FROM t5119_owner;
	REVOKE USAGE, SELECT ON SEQUENCE topology.topology_id_seq FROM t5119_owner;
	EXECUTE format('REVOKE CREATE ON DATABASE %I FROM t5119_owner', current_database());
	REVOKE t5119_owner FROM CURRENT_USER;
	REVOKE t5119_viewer FROM CURRENT_USER;
	DROP ROLE t5119_viewer;
	DROP ROLE t5119_owner;
END
$$;
SELECT '#5119-security', 'ok';

-- See https://trac.osgeo.org/postgis/ticket/5298
BEGIN;
UPDATE city_data.relation SEt layer_id = 1 WHERE layer_id = 1;
SELECT '#5298', topology.CopyTopology('city_data', 'city_data_t5298') > 0;
ROLLBACK;

-- See http://trac.osgeo.org/postgis/ticket/2184
select '#2184.1', topology.createTopology('t3d', 0, 0, true) > 0;
select '#2184.2', st_addisonode('t3d', NULL, 'POINT(1 2 3)');
select '#2184.3', topology.copyTopology('t3d', 't3d-bis') > 0;
select '#2184.4', length(topology.dropTopology('t3d')) > 0, length(topology.dropTopology('t3d-bis')) > 0;

-- Cleanup

SELECT topology.DropTopology('CITY_data_UP_down');
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
