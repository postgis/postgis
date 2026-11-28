-- The wrapper can call its helpers with its installation schema off the path.
SELECT current_schema() AS sfcgal_schema \gset
BEGIN;
SET LOCAL search_path = pg_catalog;
SELECT :"sfcgal_schema".ST_AsText(:"sfcgal_schema".ST_3DIntersection(
    'POINT Z (1 2 3)':::"sfcgal_schema".geometry,
    'POINT Z (1 2 3)':::"sfcgal_schema".geometry)) = 'POINT Z (1 2 3)';
ROLLBACK;
