\set VERBOSITY terse
set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

-- Invalid calls
SELECT 'invalid', TopoGeo_addPoint('city_data', 'LINESTRING(36 26, 38 30)');
SELECT 'invalid', TopoGeo_addPoint('city_data', 'MULTIPOINT((36 26))');
SELECT 'invalid', TopoGeo_addPoint('invalid', 'POINT(36 26)');
-- See #4722
SELECT 'invalid', TopoGeo_addPoint(null::varchar, null::geometry, null::float8);
SELECT 'invalid', TopoGeo_addPoint(null::varchar, 'POINT(36 36)'::geometry, null::float8);
SELECT 'invalid', TopoGeo_addPoint('city_data', null::geometry, null::float8);

-- Save max node id
select 'node'::text as what, max(node_id) INTO city_data.limits FROM city_data.node;

-- Isolated point in universal face
SELECT 'iso_uni', TopoGeo_addPoint('city_data', 'POINT(38 26)');

-- Isolated point in face 3
SELECT 'iso_f3', TopoGeo_addPoint('city_data', 'POINT(16 18)');

-- Existing isolated node
SELECT 'iso_ex', TopoGeo_addPoint('city_data', 'POINT(38 26)');

-- Existing isolated node within tolerance
SELECT 'iso_ex_tol', TopoGeo_addPoint('city_data', 'POINT(38 27)', 1.5);

-- Existing non-isolated node
SELECT 'noniso_ex', TopoGeo_addPoint('city_data', 'POINT(25 30)');

-- Existing non-isolated node within tolerance (closer to edge)
SELECT 'noniso_ex_tol', TopoGeo_addPoint('city_data', 'POINT(26 30.2)', 3);

-- Splitting edge
SELECT 'split', TopoGeo_addPoint('city_data', 'POINT(26 30.2)', 1);

-- Check effect on nodes
SELECT 'N', n.node_id, n.containing_face, ST_AsText(n.geom)
FROM city_data.node n WHERE n.node_id > (
  SELECT max FROM city_data.limits WHERE what = 'node'::text )
ORDER BY n.node_id;

-- Check effect on edges (there should be one split)
WITH limits AS ( SELECT max FROM city_data.limits WHERE what = 'node'::text )
SELECT 'E', n.edge_id, n.start_node, n.end_node
 FROM city_data.edge n, limits m
 WHERE n.start_node > m.max
    OR n.end_node > m.max
ORDER BY n.edge_id;

-- Test precision
SELECT 'prec1', 'N' || topogeo_addpoint('city_data', 'POINT(39 26)', 2);
SELECT 'prec2', 'N' || topogeo_addpoint('city_data', 'POINT(39 26)', 1);
SELECT 'prec3', 'N' || topogeo_addpoint('city_data', 'POINT(36 26)', 1);

SELECT DropTopology('city_data');

-- See http://trac.osgeo.org/postgis/ticket/2033
SELECT 'tt2033.start', CreateTopology('t',0,0,true) > 0;
SELECT 'tt2033', 'E' || topogeo_addlinestring('t', 'LINESTRING(0 0 0,0 1 0,0 2 1)');
SELECT 'tt2033', 'N' || topogeo_addpoint('t', 'POINT(0.2 1 1)', 0.5);
SELECT 'tt2033', 'NC', node_id, ST_AsText(geom) FROM t.node ORDER BY node_id;
SELECT 'tt2033.end' || DropTopology('t');

-- See https://trac.osgeo.org/postgis/ticket/5394
SELECT NULL FROM CreateTopology ('t');
SELECT 'tt5394', 'E1', TopoGeo_addLinestring('t', 'LINESTRING(5.803580945500557 59.26346622,5.8035802635 59.263465501)' );
SELECT 'tt5394', 'E2', TopoGeo_addLinestring('t', 'LINESTRING(5.8035802635 59.263465501,5.8035809455 59.26346622,5.803580945500557 59.26346622)' );
SELECT 'tt5394', 'V', * FROM ValidateTopology('t');
SELECT 'tt5394', 'N', TopoGeo_addPoint( 't', 'POINT(5.803646305 59.263416658000004)' );
SELECT NULL FROM DropTopology('t');

-- See https://trac.osgeo.org/postgis/ticket/5698
SELECT NULL FROM CreateTopology ('t');
SELECT 't5698', 'E', TopoGeo_addLinestring('t', 'LINESTRING( 15.796760167740288 69.05714853429149, 15.795906966300288 69.05725770093837)' );
SELECT 't5698', 'N', TopoGeo_addPoint( 't', 'POINT(15.796760167739626 69.05714853429157)');
SELECT 't5698', 'V', * FROM ValidateTopology('t');
SELECT NULL FROM DropTopology('t');

-- See https://trac.osgeo.org/postgis/ticket/5794
BEGIN;
SELECT NULL FROM topology.CreateTopology ('t');
SELECT NULL FROM topology.TopoGeo_addLinestring('t', 'LINESTRING(0 0, 10 0)');
SELECT 't5794', 'N', topology.TopoGeo_addPoint('t', 'POINT(9 5)', 5);
SELECT 't5794', 'V', * FROM ValidateTopology('t');
ROLLBACK;

-- See https://trac.osgeo.org/postgis/ticket/5925
BEGIN;
SELECT NULL FROM topology.CreateTopology ('t');
SELECT NULL FROM topology.TopoGeo_addLinestring('t', 'LINESTRING(0 0, 10 0)');
UPDATE t.edge_data SET geom = 'LINESTRING EMPTY';
SELECT 't5925.1', topology.TopoGeo_addPoint('t', 'POINT(5 5)', 0);
ROLLBACK;
