
CREATE TABLE coverage (id integer, seq integer, geom geometry);

SELECT 'empty table a' AS test,
  id, ST_AsText(GEOM) AS input,
  ST_AsText(ST_CoverageInvalidEdges(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

SELECT 'empty table b' AS test,
  id, ST_Area(ST_CoverageUnion(GEOM))
FROM coverage GROUP BY id;

INSERT INTO coverage VALUES
(1, 1, 'MULTIPOLYGON(((0 0,10 0,10.1 5,10 10,0 10,0 0)))'),
(1, 2, 'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))');

SELECT 'one partition a' AS test,
  id, seq, ST_AsText(GEOM) AS input,
  ST_AsText(ST_CoverageInvalidEdges(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

-- Add another partition
INSERT INTO coverage VALUES
(2, 1, 'MULTIPOLYGON(((0 0,10 0,10.1 5,10 10,0 10,0 0)))'),
(2, 2, 'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))'),
(2, 3, NULL);

SELECT 'two partition a' AS test,
  id, seq, ST_AsText(geom) AS input,
  ST_AsText(ST_CoverageInvalidEdges(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

-- Add another partition
INSERT INTO coverage VALUES
(3, 1, 'POLYGON EMPTY'),
(3, 2, NULL);

SELECT 'three partition a' AS test,
  id, seq, ST_AsText(geom) AS input,
  ST_AsText(ST_CoverageInvalidEdges(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

SELECT 'three partition b' AS test,
  id, ST_Area(ST_CoverageUnion(geom))
FROM coverage GROUP BY id ORDER BY id;


/**TRUNCATE coverage;
INSERT INTO coverage VALUES
(4, 1, 'POLYGON ((0 0, 0 1, 1 1, 1 0, 0 0))'),
(4, 2, 'POLYGON ((1 0, 0.9 1, 2 1, 2 0, 1 0))');

WITH u AS (
  SELECT ST_CoverageUnion(geom) AS geom FROM coverage
)
SELECT 'union 2' AS test,
  ST_Area(geom), ST_GeometryType(geom)
FROM u;**/


TRUNCATE coverage;
INSERT INTO coverage VALUES
(5, 1, 'POLYGON ((0 0, 0 1, 1 1, 1 0, 0 0))'),
(5, 2, 'POLYGON ((1 0, 1 1, 2 1, 2 0, 1 0))');

WITH u AS (
  SELECT ST_CoverageUnion(geom) AS geom FROM coverage
)
SELECT 'union 3' AS test,
  ST_Area(geom), ST_GeometryType(geom)
FROM u;

DROP TABLE coverage;


------------------------------------------------------------------------

WITH squares AS (
  SELECT *
  FROM ST_SquareGrid(100.0, ST_MakeEnvelope(0, 0, 1000, 1000)) AS geom
)
SELECT 'grid lanes', i, ST_Area(ST_CoverageUnion(geom))
FROM squares
GROUP BY i
ORDER By i;

WITH squares AS (
  SELECT i/5 AS i, j/5 AS j, geom
  FROM ST_SquareGrid(100.0, ST_MakeEnvelope(0, 0, 1000, 1000)) AS geom
)
SELECT 'grid squares', i, j, ST_Area(ST_CoverageUnion(geom))
FROM squares
GROUP BY i, j
ORDER By i, j;

