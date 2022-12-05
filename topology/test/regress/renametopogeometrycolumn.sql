BEGIN;
SELECT NULL FROM createtopology('tt');
CREATE TABLE tt.f(id integer);
SELECT NULL FROM addtopogeometrycolumn('tt','tt','f','tg','POINT');
SELECT 't0', layer_id, feature_column FROM topology.layer;
SELECT 't1', layer_id, feature_column FROM renametopogeometrycolumn('tt.f', 'tg', 'the TopoGeom');
SELECT 't2', layer_id, feature_column FROM renametopogeometrycolumn('tt.f', 'the TopoGeom', 'tgeom');
ROLLBACK;
