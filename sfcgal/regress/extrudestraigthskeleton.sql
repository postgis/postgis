SELECT 'Extrude roof', ST_AsText(CG_ExtrudeStraightSkeleton('POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))', 2.0));
SELECT 'Extrude building and roof', ST_AsText(CG_ExtrudeStraightSkeleton('POLYGON (( 0 0, 5 0, 5 5, 4 5, 4 4, 0 4, 0 0 ), (1 1, 1 2,2 2, 2 1, 1 1))', 3.0, 2.0));
SELECT 'Empty building and roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1, 20.1));
SELECT 'Empty roof', ST_AsText(CG_ExtrudeStraightSkeleton(ST_GeomFromText('POLYGON EMPTY',4326), 20.1));
