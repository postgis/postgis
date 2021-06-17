BEGIN;

CREATE SCHEMA s;
SELECT NULL FROM CreateTopology('a');
CREATE TABLE a.l(i int);
SELECT AddTopoGeometryColumn('a','a','l','g1','POINT');
SELECT AddTopoGeometryColumn('a','a','l','g2','POINT');

SELECT 'by_regclass_1', layer_id(findLayer('a.l', 'g1'));
SELECT 'by_regclass_2', layer_id(findLayer('a.l', 'g2'));

SELECT 'by_schema_table_1', layer_id(findLayer('a', 'l', 'g1'));
SELECT 'by_schema_table_2', layer_id(findLayer('a', 'l', 'g2'));

SELECT 'by_ids_1', layer_id(findLayer(id(FindTopology('a')), layer_id(findLayer('a.l', 'g1'))));
SELECT 'by_ids_2', layer_id(findLayer(id(FindTopology('a')), layer_id(findLayer('a.l', 'g2'))));

SELECT 'by_topogeom_1', layer_id(findLayer(
  toTopoGeom('POINT(0 0)', 'a', layer_id(findLayer('a.l','g1')), 0)
));
SELECT 'by_topogeom_2', layer_id(findLayer(
  toTopoGeom('POINT(0 0)', 'a', layer_id(findLayer('a.l','g2')), 0)
));

ROLLBACK;
