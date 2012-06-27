-- TODO: normalize !
SELECT 1,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9)'::geometry));
SELECT 2,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry));
SELECT 3,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 2));
SELECT 4,  ST_AsText(ST_DelaunayTriangles('MULTIPOINT(5 5, 6 0, 7 9, 8 9)'::geometry, 2, 1));
