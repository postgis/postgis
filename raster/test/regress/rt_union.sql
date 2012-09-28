DROP TABLE IF EXISTS raster_union_in;
CREATE TABLE raster_union_in (
	rid integer,
	rast raster
);
DROP TABLE IF EXISTS raster_union_out;
CREATE TABLE raster_union_out (
	uniontype text,
	rast raster
);

INSERT INTO raster_union_in
	SELECT 0, NULL::raster AS rast UNION ALL
	SELECT 1, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0), 1, '8BUI', 1, 0) AS rast UNION ALL
	SELECT 2, ST_AddBand(ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0), 1, '8BUI', 2, 0) AS rast
;

INSERT INTO raster_union_out
	SELECT
		'LAST',
		ST_Union(rast, 1) AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'FIRST',
		ST_Union(rast, 1, 'FIRST') AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'MIN',
		ST_Union(rast, 1, 'MIN') AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'MAX',
		ST_Union(rast, 1, 'MAX') AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'COUNT',
		ST_Union(rast, 1, 'COUNT') AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'SUM',
		ST_Union(rast, 1, 'SUM') AS rast
	FROM raster_union_in;

INSERT INTO raster_union_out
	SELECT
		'MEAN',
		ST_Union(rast, 'mean') AS rast
	FROM raster_union_in;

SELECT
	uniontype,
	x,
	y,
	val
FROM (
	SELECT
		uniontype,
		(ST_PixelAsPoints(rast)).*
	FROM raster_union_out
) foo
ORDER BY uniontype, y, x;

DROP TABLE IF EXISTS raster_union_in;
DROP TABLE IF EXISTS raster_union_out;
