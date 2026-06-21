SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM loadedrast;

SELECT ST_Width(rast), ST_Height(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2),
	ST_Value(rast, 1, 2, 2)
FROM o_2_loadedrast;
