-- NULL raster
SELECT ST_AsText(ST_Envelope(NULL::raster));

-- width and height of zero
SELECT ST_AsText(ST_Envelope(ST_MakeEmptyRaster(0, 0, 0, 0, 1, -1, 0, 0, 0)));

-- width of zero
SELECT ST_AsText(ST_Envelope(ST_MakeEmptyRaster(0, 9, 0, 0, 1, -1, 0, 0, 0)));

-- height of zero
SELECT ST_AsText(ST_Envelope(ST_MakeEmptyRaster(9, 0, 0, 0, 1, -1, 0, 0, 0)));

-- normal raster
SELECT ST_AsText(ST_Envelope(ST_MakeEmptyRaster(9, 9, 0, 0, 1, -1, 0, 0, 0)));

-- extent aggregate over rasters
WITH rast AS (
	SELECT ST_MakeEmptyRaster(2, 2, 0, 0, 10, -10, 1, 2, 0) AS rast
	UNION ALL
	SELECT ST_MakeEmptyRaster(1, 1, 100, 100, 10, -10, 2, 1, 0)
)
SELECT ST_Extent(rast) FROM rast;

-- extent aggregate skips NULL rasters
SELECT ST_Extent(rast) FROM (VALUES
	(NULL::raster),
	(ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0.5, 0.25, 0))
) AS v(rast);

-- extent aggregate over no rows
SELECT ST_Extent(rast) IS NULL
FROM (SELECT NULL::raster AS rast WHERE false) AS v;
