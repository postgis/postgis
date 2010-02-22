-- Empty Geometry
SELECT 'empty_geom', ST_AsSVG(GeomFromEWKT(NULL));

-- Option
SELECT 'option_01', ST_AsSVG(GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 0);
SELECT 'option_02', ST_AsSVG(GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 1);
SELECT 'option_03', ST_AsSVG(GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 0, 0);
SELECT 'option_04', ST_AsSVG(GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 1, 0);

-- Precision
SELECT 'precision_01', ST_AsSVG(GeomFromEWKT('POINT(1.1111111 1.1111111)'), 1, -2);
SELECT 'precision_02', ST_AsSVG(GeomFromEWKT('POINT(1.1111111 1.1111111)'), 1, 19);

