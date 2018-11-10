
-- poly in poly
SELECT c, ST_Covers(g1::geography, g2::geography) FROM
( VALUES
    ('geog_covers_poly_poly_in', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON (( 10 30, 30 30, 30 10, 10 10, 10 30))'),
    ('geog_covers_poly_poly_part', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON (( -10 30, 30 30, 30 10, 10 10, -10 30))'),
    ('geog_covers_poly_poly_out', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON ((0 -40, -40 -40, -40 0, 0 0, 0 -40))'),
    ('geog_covers_poly_poly_idl', 'POLYGON((-170 -20, 170 -20, 170 20, -170 20, -170 -20))', 'POLYGON((-175 -20, 175 -20, 175 20, -175 20, -175 -20))'),
    ('geog_covers_poly_poly_pole', 'POLYGON((0 80, 90 80, 180 80, -90 80, 0 80))', 'POLYGON((0 85, 90 85, 180 85, -90 85, 0 85))')
) AS u(c, g1, g2);

-- line in poly
SELECT c, ST_Covers(g1::geography, g2::geography) FROM
( VALUES
    ('geog_covers_poly_line_in', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (35 35, 35 15, 15 15)'),
    ('geog_covers_poly_line_part', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (-10 30, 30 30, 30 10, 10 10)'),
    ('geog_covers_poly_line_out', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (-10 -40, -40 -40, -40 -10, -10 -10)')
) AS u(c, g1, g2);

-- poly in poly (reversed arguments)
SELECT c, ST_CoveredBy(g1::geography, g2::geography) FROM
( VALUES
    ('geog_covered_poly_poly_in', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON (( 10 30, 30 30, 30 10, 10 10, 10 30))'),
    ('geog_covered_poly_poly_part', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON (( -10 30, 30 30, 30 10, 10 10, -10 30))'),
    ('geog_covered_poly_poly_out', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'POLYGON ((0 -40, -40 -40, -40 0, 0 0, 0 -40))'),
    ('geog_covered_poly_poly_idl', 'POLYGON((-170 -20, 170 -20, 170 20, -170 20, -170 -20))', 'POLYGON((-175 -20, 175 -20, 175 20, -175 20, -175 -20))'),
    ('geog_covered_poly_poly_pole', 'POLYGON((0 80, 90 80, 180 80, -90 80, 0 80))', 'POLYGON((0 85, 90 85, 180 85, -90 85, 0 85))')
) AS u(c, g1, g2);

-- line in poly (reversed arguments)
SELECT c, ST_CoveredBy(g1::geography, g2::geography) FROM
( VALUES
    ('geog_covered_poly_line_in', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (35 35, 35 15, 15 15)'),
    ('geog_covered_poly_line_part', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (-10 30, 30 30, 30 10, 10 10)'),
    ('geog_covered_poly_line_out', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))', 'LINESTRING (-10 -40, -40 -40, -40 -10, -10 -10)')
) AS u(c, g1, g2);

-- self coverage
SELECT c, ST_Covers(g::geography, g::geography) FROM
( VALUES
    ('geog_covers_self_point', 'POINT (0 20)'),
    ('geog_covers_self_line', 'LINESTRING (35 35, 35 15, 15 15)'),
    ('geog_covers_self_polygon', 'POLYGON((0 40, 40 40, 40 0, 0 0, 0 40))')
) AS u(c, g)
