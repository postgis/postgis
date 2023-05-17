-- TODO: move all ST_AsGML tests here

-- #5384
SELECT '#5384.geom', ST_AsGML('POINT(0 0)'::geometry, 15, 0, '', '1');
SELECT '#5384.geog', ST_AsGML('POINT(0 0)'::geography, 15, 0, '', '1');
