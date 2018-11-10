-- TODO: normalize !
SELECT 1,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9)'::geometry));
SELECT 2,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry));
SELECT 3,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 2));
SELECT 4,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 2, 1));
SELECT 5,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9)'::geometry, 0.0, 2));
SELECT 6,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 0.0, 2));
SELECT 7,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 2.0, 2));

-- 3DZ
SELECT 10,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5 3, 6 0 2, 7 9 1)'::geometry));
SELECT 11,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5 1, 6 0 2, 7 9 3)'::geometry, 0.0, 2));
