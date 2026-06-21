-- Base table keeps the full-resolution source pixels.
SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM loadedrast;

-- The overview table should reuse the source's averaged 2x overview. These
-- corner values would be 1, 3, 17, 19 if raster2pgsql regenerated it from the
-- base band with nearest-neighbor sampling.
SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM o_2_loadedrast;
