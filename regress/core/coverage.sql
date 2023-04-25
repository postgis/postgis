
CREATE TABLE coverage (id integer, seq integer, geom geometry);

SELECT 'empty table a' AS test,
  id, ST_AsText(GEOM) AS input,
  ST_AsText(ST_CoverageIsValid(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

SELECT 'empty table b' AS test,
  id, ST_Area(ST_CoverageUnion(GEOM))
FROM coverage GROUP BY id;

INSERT INTO coverage VALUES
(1, 1, 'MULTIPOLYGON(((0 0,10 0,10.1 5,10 10,0 10,0 0)))'),
(1, 2, 'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))'),
(1, 3, NULL),
(1, 4, 'POLYGON EMPTY');

SELECT 'one partition a' AS test,
  id, seq, ST_AsText(GEOM) AS input,
  ST_AsText(ST_CoverageIsValid(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

SELECT 'one partition b' AS test,
  id, ST_Area(ST_CoverageUnion(GEOM))
FROM coverage GROUP BY id ORDER BY id;

-- Add another partition
INSERT INTO coverage
  SELECT id+1, seq, geom FROM coverage;

SELECT 'two partition a' AS test,
  id, seq, ST_AsText(GEOM) AS input,
  ST_AsText(ST_CoverageIsValid(geom) OVER (partition by id)) AS invalid,
  ST_AsText(ST_CoverageSimplify(geom, 1.0) OVER (partition by id)) AS simple
FROM coverage
ORDER BY id, seq;

SELECT 'two partition b' AS test,
  id, ST_Area(ST_CoverageUnion(GEOM))
FROM coverage GROUP BY id ORDER BY id;

DROP TABLE coverage;

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

