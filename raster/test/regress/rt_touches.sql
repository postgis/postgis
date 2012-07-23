SET client_min_messages TO warning;

DROP TABLE IF EXISTS raster_touches_rast;
CREATE TABLE raster_touches_rast (
	rid integer,
	rast raster
);
CREATE OR REPLACE FUNCTION make_test_raster(
	rid integer,
	width integer DEFAULT 2, height integer DEFAULT 2,
	ul_x double precision DEFAULT 0, ul_y double precision DEFAULT 0,
	skew_x double precision DEFAULT 0, skew_y double precision DEFAULT 0
)
	RETURNS void
	AS $$
	DECLARE
		x int;
		y int;
		rast raster;
	BEGIN
		rast := ST_MakeEmptyRaster(width, height, ul_x, ul_y, 1, 1, skew_x, skew_y, 0);
		rast := ST_AddBand(rast, 1, '8BUI', 1, 0);


		INSERT INTO raster_touches_rast VALUES (rid, rast);

		RETURN;
	END;
	$$ LANGUAGE 'plpgsql';
SELECT make_test_raster(0, 2, 2, -1, -1);
SELECT make_test_raster(1, 2, 2);
SELECT make_test_raster(2, 3, 3);
DROP FUNCTION make_test_raster(integer, integer, integer, double precision, double precision, double precision, double precision);

INSERT INTO raster_touches_rast VALUES (10, (
	SELECT
		ST_SetValue(rast, 1, 1, 1, 0)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (11, (
	SELECT
		ST_SetValue(rast, 1, 2, 1, 0)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (12, (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(rast, 1, 1, 1, 0),
				1, 2, 1, 0
			),
			1, 1, 2, 0
		)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (13, (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(rast, 1, 1, 1, 0),
					1, 2, 1, 0
				),
				1, 1, 2, 0
			),
			1, 2, 2, 0
		)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (14, (
	SELECT
		ST_SetUpperLeft(rast, 2, 0)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (15, (
	SELECT
		ST_SetScale(
			ST_SetUpperLeft(rast, 0.1, 0.1),
			0.4, 0.4
		)
	FROM raster_touches_rast
	WHERE rid = 1
));
INSERT INTO raster_touches_rast VALUES (16, (
	SELECT
		ST_SetScale(
			ST_SetUpperLeft(rast, -0.1, 0.1),
			0.4, 0.4
		)
	FROM raster_touches_rast
	WHERE rid = 1
));

INSERT INTO raster_touches_rast VALUES (20, (
	SELECT
		ST_SetUpperLeft(rast, -2, -2)
	FROM raster_touches_rast
	WHERE rid = 2
));
INSERT INTO raster_touches_rast VALUES (21, (
	SELECT
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(rast, 1, 1, 1, 0),
				1, 2, 2, 0
			),
			1, 3, 3, 0
		)
	FROM raster_touches_rast
	WHERE rid = 20
));
INSERT INTO raster_touches_rast VALUES (22, (
	SELECT
		ST_SetValue(
			ST_SetValue(
				rast, 1, 3, 2, 0
			),
			1, 2, 3, 0
		)
	FROM raster_touches_rast
	WHERE rid = 21
));
INSERT INTO raster_touches_rast VALUES (23, (
	SELECT
		ST_SetValue(
			ST_SetValue(
				rast, 1, 3, 1, 0
			),
			1, 1, 3, 0
		)
	FROM raster_touches_rast
	WHERE rid = 22
));

INSERT INTO raster_touches_rast VALUES (30, (
	SELECT
		ST_SetSkew(rast, -0.5, 0.5)
	FROM raster_touches_rast
	WHERE rid = 2
));
INSERT INTO raster_touches_rast VALUES (31, (
	SELECT
		ST_SetSkew(rast, -1, 1)
	FROM raster_touches_rast
	WHERE rid = 2
));
INSERT INTO raster_touches_rast VALUES (32, (
	SELECT
		ST_SetSkew(rast, 1, -1)
	FROM raster_touches_rast
	WHERE rid = 2
));

SELECT
	'1.1',
	r1.rid,
	r2.rid,
	ST_Touches(r1.rast, NULL, r2.rast, NULL)
FROM raster_touches_rast r1
JOIN raster_touches_rast r2
	ON r1.rid != r2.rid
WHERE r1.rid = 0;

SELECT
	'1.2',
	r1.rid,
	r2.rid,
	ST_Touches(r1.rast, 1, r2.rast, 1)
FROM raster_touches_rast r1
JOIN raster_touches_rast r2
	ON r1.rid != r2.rid
WHERE r1.rid = 0;

DROP TABLE IF EXISTS raster_touches_rast;
