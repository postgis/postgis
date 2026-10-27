-- Base table keeps the full-resolution source pixels.
SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM loadedrast;

-- The source has a matching nearest-neighbor 2x overview. Requesting
-- -A average must resample from the base band instead of copying that overview.
SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM o_2_loadedrast;
