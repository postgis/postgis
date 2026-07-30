-- box2d output / expand must not read past a materialised box2d value.
-- see #6109
SELECT 'out',        ('LINESTRING(0 0,1 1)'::geometry::box2d)::text;
SELECT 'out_3d',     ('LINESTRING Z (0 0 9,2 3 9)'::geometry::box2d)::text;
SELECT 'roundtrip',  'BOX(1.5 2.5,3.5 4.5)'::box2d::text;
SELECT 'extent',     ST_Extent(g)::text FROM (VALUES ('POINT(0 0)'::geometry),('POINT(5 7)'::geometry)) v(g);
SELECT 'expand_d',   ST_Expand('LINESTRING(0 0,10 10)'::geometry::box2d, 1)::text;
SELECT 'expand_dxdy',ST_Expand('LINESTRING(0 0,10 10)'::geometry::box2d, 2, 3)::text;
