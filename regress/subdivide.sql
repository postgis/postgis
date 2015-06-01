-- polygon
WITH g AS (SELECT 'POLYGON((132 10,119 23,85 35,68 29,66 28,49 42,32 56,22 64,32 110,40 119,36 150,
57 158,75 171,92 182,114 184,132 186,146 178,176 184,179 162,184 141,190 122,
190 100,185 79,186 56,186 52,178 34,168 18,147 13,132 10))'::geometry As geom)
, gs AS (SELECT ST_Area(geom) As full_area, ST_SubDivide(geom,10) As geom FROM g)
SELECT '1' As rn, full_area::numeric(10,3) = SUM(ST_Area(gs.geom))::numeric(10,3), COUNT(gs.geom) As num_pieces, MAX(ST_NPoints(gs.geom)) As max_vert
FROM gs
GROUP BY gs.full_area;

-- linestring
WITH g AS (SELECT ST_Segmentize('LINESTRING(0 0, 100 100, 150 150)'::geometry,10) As geom)
, gs AS (SELECT ST_Length(geom) As m, ST_SubDivide(geom,8) As geom FROM g)
SELECT '2' As rn, m::numeric(10,3) = SUM(ST_Length(gs.geom))::numeric(10,3), COUNT(gs.geom) As num_pieces, MAX(ST_NPoints(gs.geom)) As max_vert
FROM gs
GROUP BY gs.m;

--multipolygon
WITH g AS (SELECT 'POLYGON((132 10,119 23,85 35,68 29,66 28,49 42,32 56,22 64,32 110,40 119,36 150,
57 158,75 171,92 182,114 184,132 186,146 178,176 184,179 162,184 141,190 122,
190 100,185 79,186 56,186 52,178 34,168 18,147 13,132 10))'::geometry As geom)
, gs AS (SELECT ST_Area(ST_Union(g.geom, ST_Translate(g.geom,300,10) )) As full_area, ST_SubDivide(ST_Union(g.geom, ST_Translate(g.geom,300,10) ), 10) As geom FROM g)
SELECT '3' As rn, full_area::numeric(10,3) = SUM(ST_Area(gs.geom))::numeric(10,3), COUNT(gs.geom) As num_pieces, MAX(ST_NPoints(gs.geom)) As max_vert
FROM gs
GROUP BY gs.full_area;
