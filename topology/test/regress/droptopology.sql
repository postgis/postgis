set client_min_messages to WARNING;

SELECT NULL FROM topology.CreateTopology('t1');
SELECT NULL FROM topology.CreateTopology('t2');

CREATE TABLE t1f (id int);
SELECT NULL FROM topology.AddTopoGeometryColumn('t1', 'public', 't1f', 'geom_t1', 'LINE');

CREATE TABLE t2f (id int);
SELECT NULL FROM topology.AddTopoGeometryColumn('t2', 'public', 't2f', 'geom_t2', 'LINE');

SELECT topology.DropTopology('t1');
SELECT topology.DropTopology('t2');
DROP TABLE t2f;
DROP TABLE t1f;

SELECT NULL FROM topology.CreateTopology('t3');
CREATE FUNCTION t3.bail_out() RETURNS trigger AS $BODY$
BEGIN
  RAISE EXCEPTION '%: trigger prevents % on %', TG_ARGV[0], TG_OP, TG_RELNAME;
END;
$BODY$ LANGUAGE 'plpgsql';

-- Test DropTopology in presence of triggers
-- See https://trac.osgeo.org/postgis/ticket/5026
CREATE TABLE t3.f(id serial);
SELECT NULL FROM topology.AddTopoGeometryColumn('t3', 't3', 'f', 'g', 'POINT');
CREATE TRIGGER prevent_delete AFTER DELETE OR UPDATE ON t3.f
FOR EACH STATEMENT EXECUTE PROCEDURE t3.bail_out('t3');
INSERT INTO t3.f(g) VALUES (toTopoGeom('POINT(0 0)', 't3', 1));
--SELECT topology.DropTopoGeometryColumn('t3', 'f', 'g');
SELECT topology.DropTopology('t3');

-- Test DropTopology with pending deferred triggers
-- See https://trac.osgeo.org/postgis/ticket/5115
BEGIN;
SELECT 't5115.create' FROM topology.CreateTopology('t5115');
SELECT 't5115.addline' FROM topology.TopoGeo_addLineString('t5115', 'LINESTRING(0 0, 10 0)');
SELECT 't5115.drop' FROM topology.DropTopology('t5115');
ROLLBACK;

-- Exceptions
SELECT topology.DropTopology('topology');
SELECT topology.DropTopology('doesnotexist');
