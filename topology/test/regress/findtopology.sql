BEGIN;

CREATE SCHEMA s;
SELECT NULL FROM CreateTopology('a');
SELECT NULL FROM CreateTopology('b');

SELECT 'by_name', name(findTopology('a'::text));
SELECT 'by_id', name(findTopology( id(findTopology('a')) ));

CREATE TABLE s.f(a int);
SELECT NULL FROM AddTopoGeometryColumn('a', 's', 'f', 'ga', 'POINT');
SELECT NULL FROM AddTopoGeometryColumn('b', 's', 'f', 'gb', 'POINT');

SELECT 'a_by_layer', name(findTopology('s', 'f', 'ga'));
SELECT 'b_by_layer', name(findTopology('s', 'f', 'gb'));

SELECT 'by_layer_regclass', name(findTopology('s.f', 'ga'));

SELECT 'a_by_topogeom', name(findTopology(
  toTopoGeom('POINT(0 0)', 'a', 1, 0)
));

SELECT 'b_by_topogeom', name(findTopology(
  toTopoGeom('POINT(0 0)', 'b', 1, 0)
));

ROLLBACK;
