set client_min_messages to warning;
-- Connected graph (all one cluster)
SELECT 'connected', ST_MinimumSpanningTree(geom) OVER () FROM (VALUES
    ('POINT(0 0)'::geometry),
    ('POINT(0 1)'::geometry),
    ('POINT(1 1)'::geometry)
) AS t(geom);

-- Disconnected graph (two clusters)
SELECT 'disconnected', ST_MinimumSpanningTree(geom) OVER (ORDER BY geom), ST_AsText(geom) FROM (VALUES
    ('POINT(0 0)'::geometry),
    ('POINT(0 1)'::geometry),
    ('POINT(10 10)'::geometry),
    ('POINT(10 11)'::geometry)
) AS t(geom) ORDER BY geom;

-- NULL handling
SELECT 'nulls', ST_MinimumSpanningTree(geom) OVER (ORDER BY geom), ST_AsText(geom) FROM (VALUES
    ('POINT(0 0)'::geometry),
    (NULL::geometry)
) AS t(geom) ORDER BY geom;

-- Empty geometry handling
SELECT 'empty', ST_MinimumSpanningTree(geom) OVER (ORDER BY geom), ST_AsText(geom) FROM (VALUES
    ('POINT(0 0)'::geometry),
    ('POINT EMPTY'::geometry)
) AS t(geom) ORDER BY geom;
