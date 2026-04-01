
-- ST_MinimumSpanningTree

-- Connected graph (all one cluster)
SELECT 'connected', id, ST_MinimumSpanningTree(geom) OVER (ORDER BY id) FROM (VALUES
    (1, 'LINESTRING(0 0,1 0)'::geometry),
    (2, 'LINESTRING(0 0,0 1)'::geometry),
    (3, 'LINESTRING(1 1,0 1)'::geometry),
    (4, 'LINESTRING(1 1,1 0)'::geometry),
    (5, 'LINESTRING(0 0,1 1)'::geometry),
    (6, 'LINESTRING(1 0,0 1)'::geometry)
) AS t(id, geom);

-- Disconnected graph (two clusters)
SELECT 'disconnected', id, ST_MinimumSpanningTree(geom) OVER (ORDER BY id) FROM (VALUES
    (1, 'LINESTRING(0 0,1 0)'::geometry),
    (2, 'LINESTRING(0 0,0 1)'::geometry),
    (3, 'LINESTRING(1 1,0 1)'::geometry),
    (4, 'LINESTRING(11 11,11 10)'::geometry),
    (5, 'LINESTRING(10 10,11 11)'::geometry),
    (6, 'LINESTRING(11 10,10 11)'::geometry)
) AS t(id, geom);


-- NULL handling
SELECT 'nulls', id, ST_MinimumSpanningTree(geom) OVER (ORDER BY id), ST_AsText(geom) FROM (VALUES
    (1, 'LINESTRING(0 0,1 0)'::geometry),
    (2, 'LINESTRING(0 0,0 1)'::geometry),
    (3, NULL::geometry),
    (4, 'LINESTRING(1 1,1 0)'::geometry),
    (5, 'LINESTRING(0 0,1 1)'::geometry),
    (6, 'LINESTRING(1 0,0 1)'::geometry)
) AS t(id, geom);

-- Empty geometry handling
SELECT 'empty', id, ST_MinimumSpanningTree(geom) OVER (ORDER BY id), ST_AsText(geom) FROM (VALUES
    (1, 'LINESTRING(0 0,1 0)'::geometry),
    (2, 'LINESTRING EMPTY'::geometry),
    (3, 'LINESTRING EMPTY'::geometry),
    (4, 'LINESTRING(1 1,1 0)'::geometry),
    (5, 'LINESTRING(0 0,1 1)'::geometry),
    (6, 'LINESTRING(1 0,0 1)'::geometry)
) AS t(id, geom);

-- ST_CoverageEdges

CREATE TABLE coverage_edges (id integer, seq integer, geom geometry);

INSERT INTO coverage_edges VALUES
(6, 1, 'POLYGON ((0 0, 0 10, 10 10, 10 0, 0 0))'),
(6, 2, 'POLYGON ((10 0, 10 10, 20 10, 20 0, 10 0))');

SELECT 'coverage edges all' AS test,
  ST_AsText(ST_CoverageEdges(ST_Collect(geom ORDER BY seq)))
FROM coverage_edges WHERE id = 6;

SELECT 'coverage edges exterior' AS test,
  ST_AsText(ST_CoverageEdges(ST_Collect(geom ORDER BY seq), 1))
FROM coverage_edges WHERE id = 6;

SELECT 'coverage edges interior' AS test,
  ST_AsText(ST_CoverageEdges(ST_Collect(geom ORDER BY seq), 2))
FROM coverage_edges WHERE id = 6;

SELECT 'coverage edges array' AS test,
  ST_AsText(ST_CoverageEdges(array_agg(geom ORDER BY seq)))
FROM coverage_edges WHERE id = 6;

DROP TABLE coverage_edges;
