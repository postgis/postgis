SELECT 'CG_IsSimple.simple_line', CG_IsSimple('LINESTRING(0 0,1 1)');
SELECT 'CG_IsSimple.crossing_line', CG_IsSimple('LINESTRING(0 0,2 2,2 0,0 2)');
SELECT 'CG_IsSimpleDetail.simple_line', CG_IsSimpleDetail('LINESTRING(0 0,1 1)') IS NULL;
SELECT 'CG_IsSimpleDetail.crossing_line', CG_IsSimpleDetail('LINESTRING(0 0,2 2,2 0,0 2)') IS NOT NULL;
