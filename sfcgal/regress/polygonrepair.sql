-- Empty polygon
SELECT 'empty', ST_AsText(CG_PolygonRepair('POLYGON EMPTY'));
-- bowtie polygon (self-intersecting): split into two triangles by EVEN_ODD
SELECT 'bowtie_even_odd', ST_AsText(CG_PolygonRepair('POLYGON((0 0,2 2,2 0,0 2,0 0))'));
-- explicit rule
SELECT 'bowtie_even_odd_explicit', ST_AsText(CG_PolygonRepair('POLYGON((0 0,2 2,2 0,0 2,0 0))', 'EVEN_ODD'));
-- valid polygon is returned unchanged
SELECT 'valid', ST_AsText(CG_PolygonRepair('POLYGON((0 0,1 0,1 1,0 1,0 0))'));
