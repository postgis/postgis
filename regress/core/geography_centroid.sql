
-- check for pole crossing
SELECT c, ST_Centroid(g::geography) FROM
( VALUES
    ('geog_centroid_mpt_pole_north', 'MULTIPOINT ( 90 80, -90 80)'),
    ('geog_centroid_mpt_pole_south', 'MULTIPOINT ( 90 -80, -90 -80)')
) AS u(c, g);

-- check for IDL crossing
SELECT c, ST_Centroid(g::geography) FROM
( VALUES
    ('geog_centroid_mpt_idl_1', 'MULTIPOINT ( 179 0, -179 0)'),
    ('geog_centroid_mpt_idl_2', 'MULTIPOINT ( 178 0, -179 0)'),
    ('geog_centroid_mpt_idl_3', 'MULTIPOINT ( 179 0, -178 0)')
) AS u(c, g);

-- point should return itself
SELECT c, ST_Centroid(g::geography) FROM
( VALUES
    ('geog_centroid_pt_self_1', 'POINT ( 4 8)'),
    ('geog_centroid_pt_self_2', 'POINT ( -15 16)'),
    ('geog_centroid_pt_self_3', 'POINT ( -23 -42)')
) AS u(c, g);

-- test supported geometry types
SELECT c, ST_Centroid(g::geography) FROM
( VALUES
    ('geog_centroid_sup_pt', 'POINT (23 42)'),
    ('geog_centroid_sup_line', 'LINESTRING(-20 35, 8 46)'),
    ('geog_centroid_sup_mline', 'MULTILINESTRING((-5 45, 8 36), (1 49, 15 41))'),
    ('geog_centroid_sup_poly', 'POLYGON((10.9099 50.6917,10.9483 50.6917,10.9483 50.6732,10.9099 50.6732,10.9099 50.6917))'),
    ('geog_centroid_sup_mpoly', 'MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))')
) AS u(c, g);
