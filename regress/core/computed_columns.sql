CREATE SCHEMA testc;
CREATE OR REPLACE FUNCTION testc.compute_exection_time(param_sql text) RETURNS interval
AS $$
DECLARE var_start_time timestamptz; var_end_time timestamptz;
BEGIN
var_start_time = clock_timestamp();
EXECUTE param_sql;
var_end_time = clock_timestamp();
RETURN var_end_time - var_start_time;
END;
$$ language plpgsql;


CREATE TABLE testc.city_boundary AS
SELECT ST_BuildArea(ST_Collect(geom)) As geom
	FROM (SELECT ST_Translate(ST_SnapToGrid(ST_Buffer(ST_Point(50 ,generate_series(50,300, 100)
			),100, 'quad_segs=4'),1), x, 0)  As geom
			FROM generate_series(1,1000,100) As x)  AS foo;

CREATE TABLE testc.streets AS
WITH d AS (SELECT dp.geom FROM testc.city_boundary AS c, ST_DumpPoints(c.geom) AS dp)
SELECT ROW_NUMBER() OVER() AS id, ST_MakeLine(d1.geom, d2.geom) AS geom
FROM d AS d1, d AS d2
ORDER BY d1.geom <-> d2.geom DESC LIMIT 1000;

CREATE INDEX ix_streets_geom ON testc.streets USING GIST(geom);

CREATE TABLE testc.random_points AS
SELECT dp.path[1] AS id, dp.geom
FROM testc.city_boundary AS c
	, ST_GeneratePoints(c.geom,500) AS gp
	, ST_DumpPoints(gp) AS dp;

ALTER TABLE testc.random_points
    ADD CONSTRAINT PK_random_points
    PRIMARY KEY (id);

CREATE INDEX gix_random_points_geom
    ON testc.random_points USING GIST (geom);

ALTER TABLE testc.random_points
    ADD way_buffer GEOMETRY (POLYGON)
    GENERATED ALWAYS AS (ST_Buffer(geom, 500)) STORED ;

CREATE INDEX gix_random_way_buffer_geom
    ON testc.random_points USING GIST (way_buffer);

analyze testc.random_points;
analyze testc.streets;

-- time using computed column should always be less than adhoc
SELECT testc.compute_exection_time('SELECT COUNT(*) FROM testc.random_points AS p INNER JOIN testc.streets AS s ON ST_Contains(p.way_buffer, s.geom)') <
testc.compute_exection_time('SELECT COUNT(*) FROM testc.random_points AS p INNER JOIN testc.streets AS s ON ST_Contains(ST_Buffer(p.geom, 500), s.geom);');

-- confirm results are the same
SELECT (SELECT COUNT(*) FROM testc.random_points AS p INNER JOIN testc.streets AS s ON ST_Contains(p.way_buffer, s.geom) ) =
 (SELECT COUNT(*) FROM testc.random_points AS p INNER JOIN testc.streets AS s ON ST_Contains(ST_Buffer(p.geom, 500), s.geom) );

-- cleanup
DROP SCHEMA testc CASCADE;
