CREATE TABLE geoms (geom geometry);

-- Create 20x20 square polygons
WITH coords AS (SELECT * FROM generate_series(0, 19) AS x, generate_series(0, 19) AS y)
INSERT INTO geoms
SELECT ST_Square(1.0, x, y, ST_MakePoint(0, 0))
FROM coords;

WITH u AS (SELECT ST_Union(geom) AS g FROM geoms)
SELECT ST_Area(g), ST_XMin((g)), ST_YMin(g), ST_XMax(g), ST_YMax(g) from u;

-- NOTE: using gridSize > 0 requires GEOS-3.9 or higher
WITH u AS (SELECT ST_Union(geom, -1.0) AS g FROM geoms)
SELECT ST_Area(g), ST_XMin((g)), sT_YMin(g), ST_XMax(g), ST_YMax(g) from u;

TRUNCATE TABLE geoms;

-- Empty table
SELECT ST_Union(geom) IS NULL FROM geoms;

-- NULLs
INSERT INTO geoms (geom) VALUES (NULL), (NULL), (NULL), (NULL), (NULL);
SELECT ST_Union(geom) IS NULL FROM geoms;

-- Add empty point
INSERT INTO geoms SELECT ST_GeomFromText('POINT EMPTY') AS geom;
SELECT ST_AsText(ST_Union(geom)) FROM geoms;

-- Add empty polygon
INSERT INTO geoms SELECT ST_GeomFromText('POLYGON EMPTY') AS geom;
SELECT ST_AsText(ST_Union(geom)) FROM geoms;

/** PG16 force_parallel_mode was renamed to debug_parallel_query, thus the need for this conditional **/
CREATE OR REPLACE PROCEDURE p_force_parellel_mode(param_state text) language plpgsql AS
$$
BEGIN
	IF (_postgis_pgsql_version())::integer < 160 THEN
		IF param_state = 'on' THEN
			SET force_parallel_mode=on;
		ELSE
			SET force_parallel_mode=off;
		END IF;
	ELSE
		IF param_state = 'on' THEN
			SET debug_parallel_query=on;
		ELSE
			SET debug_parallel_query=off;
		END IF;
	END IF;
END;
$$;

-- Test in parallel mode

TRUNCATE TABLE geoms;

CALL p_force_parellel_mode('on'::text);
SET parallel_setup_cost = 0;
SET parallel_tuple_cost = 0;
SET min_parallel_table_scan_size = 0;
SET max_parallel_workers_per_gather = 4;

WITH coords AS (SELECT * FROM generate_series(0, 19) AS x, generate_series(0, 19) AS y)
INSERT INTO geoms
SELECT ST_Square(1.0, x, y, ST_MakePoint(0, 0))
FROM coords;

WITH u AS (SELECT ST_Union(geom) AS g FROM geoms)
SELECT ST_Area(g), ST_XMin((g)), ST_YMin(g), ST_XMax(g), ST_YMax(g) from u;

WITH u AS (SELECT ST_Union(geom, -1.0) AS g FROM geoms)
SELECT ST_Area(g), ST_XMin((g)), sT_YMin(g), ST_XMax(g), ST_YMax(g) from u;

TRUNCATE TABLE geoms;

SELECT ST_Union(geom) IS NULL FROM geoms;

INSERT INTO geoms (geom) VALUES (NULL), (NULL), (NULL), (NULL), (NULL);
SELECT ST_Union(geom) IS NULL FROM geoms;

INSERT INTO geoms SELECT ST_GeomFromText('POINT EMPTY') AS geom;
SELECT ST_AsText(ST_Union(geom)) FROM geoms;

INSERT INTO geoms SELECT ST_GeomFromText('POLYGON EMPTY') AS geom;
SELECT ST_AsText(ST_Union(geom)) FROM geoms;


DROP TABLE geoms;

DROP PROCEDURE IF EXISTS p_force_parellel_mode(text);
