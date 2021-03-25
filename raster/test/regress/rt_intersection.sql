SET client_min_messages TO warning;

DROP TABLE IF EXISTS raster_intersection;
CREATE TABLE raster_intersection (
	rid integer,
	rast raster
);
DROP TABLE IF EXISTS raster_intersection_out;
CREATE TABLE raster_intersection_out (
	rid1 integer,
	rid2 integer,
	rast raster
);
CREATE OR REPLACE FUNCTION make_test_raster(
	rid integer,
	width integer DEFAULT 2,
	height integer DEFAULT 2,
	ul_x double precision DEFAULT 0,
	ul_y double precision DEFAULT 0,
	skew_x double precision DEFAULT 0,
	skew_y double precision DEFAULT 0,
	initvalue double precision DEFAULT 1,
	nodataval double precision DEFAULT 0
)
	RETURNS void
	AS $$
	DECLARE
		x int;
		y int;
		rast raster;
	BEGIN
		rast := ST_MakeEmptyRaster(width, height, ul_x, ul_y, 1, 1, skew_x, skew_y, 0);
		rast := ST_AddBand(rast, 1, '8BUI', initvalue, nodataval);

		INSERT INTO raster_intersection VALUES (rid, rast);

		RETURN;
	END;
	$$ LANGUAGE 'plpgsql';
-- no skew
SELECT make_test_raster(0, 4, 4, -2, -2);
SELECT make_test_raster(1, 2, 2,  0,  0, 0, 0, 2);
SELECT make_test_raster(2, 2, 2,  1, -1, 0, 0, 3);
SELECT make_test_raster(3, 2, 2,  1,  1, 0, 0, 4);
SELECT make_test_raster(4, 2, 2,  2,  2, 0, 0, 5);

-- skew
SELECT make_test_raster(10, 4, 4,    -2,   -2, 0.1, 0.1);
SELECT make_test_raster(11, 4, 4,  -0.9, -0.9, 0.1, 0.1, 2);
SELECT make_test_raster(12, 2, 2,  -1.9,   -1, 0.1, 0.1, 3);
SELECT make_test_raster(13, 2, 2,     0, -1.8, 0.1, 0.1, 4);
SELECT make_test_raster(14, 2, 2,     2,    2, 0.1, 0.1, 5);

DROP FUNCTION make_test_raster(integer, integer, integer, double precision, double precision, double precision, double precision, double precision, double precision);

INSERT INTO raster_intersection_out
	(SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 0
		AND r2.rid BETWEEN 1 AND 9
	) UNION ALL (
	SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 10
		AND r2.rid BETWEEN 11 AND 19)
;

INSERT INTO raster_intersection_out
	(SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast, 'BAND1'
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 0
		AND r2.rid BETWEEN 1 AND 9
	) UNION ALL (
	SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast, 'BAND1'
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 10
		AND r2.rid BETWEEN 11 AND 19)
;

INSERT INTO raster_intersection_out
	(SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast, 'BAND2'
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 0
		AND r2.rid BETWEEN 1 AND 9
	) UNION ALL (
	SELECT r1.rid, r2.rid, ST_Intersection(
		r1.rast, r2.rast, 'BAND2'
	)
	FROM raster_intersection r1
	JOIN raster_intersection r2
		ON r1.rid != r2.rid
	WHERE r1.rid = 10
		AND r2.rid BETWEEN 11 AND 19)
;

SELECT
	rid1,
	rid2,
	round(upperleftx::numeric, 3) AS upperleftx,
	round(upperlefty::numeric, 3) AS upperlefty,
	width,
	height,
	round(scalex::numeric, 3) AS scalex,
	round(scaley::numeric, 3) AS scaley,
	round(skewx::numeric, 3) AS skewx,
	round(skewy::numeric, 3) AS skewy,
	srid,
	numbands,
	pixeltype,
	round(nodatavalue::numeric, 3) AS nodatavalue,
	round(firstvalue::numeric, 3) AS firstvalue,
	round(lastvalue::numeric, 3) AS lastvalue
FROM (
	SELECT
		rid1,
		rid2,
		md.*,
		bmd.*,
		ST_Value(rast, 1, 1, 1) AS firstvalue,
		ST_Value(rast, 1, ST_Width(rast), ST_Height(rast)) AS lastvalue
	FROM raster_intersection_out
		LEFT JOIN LATERAL ST_Metadata(rast) AS md ON true
		LEFT JOIN LATERAL ST_BandMetadata(rast,1) AS bmd ON true
) AS r;

-- Display the pixels and the values of the resulting rasters
SELECT
	rid1,
	rid2,
	band,
	(gvxy).x,
	(gvxy).y,
	(gvxy).val,
	ST_AsText((gvxy).geom) geom
FROM (
	SELECT
		rid1,
		rid2,
		band,
	 	gvxy
	FROM raster_intersection_out
	CROSS JOIN generate_series(1, 2) AS band
	CROSS JOIN ST_PixelAsPolygons(rast, band) AS gvxy
) foo
ORDER BY 1, 2, 3, 4, 5, 6, 7;

-- test for mismatched srid geom/raster https://trac.osgeo.org/postgis/ticket/4719
SELECT '#4719' AS ticker,  r1.rid, ST_Intersection(
		r1.rast, ST_Buffer(ST_SetSRID(ST_Point(1,2), 4326),0.5)
	)
	FROM raster_intersection AS r1
	WHERE r1.rid = 10;

DROP TABLE IF EXISTS raster_intersection;
DROP TABLE IF EXISTS raster_intersection_out;
