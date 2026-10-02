SELECT ST_UpperLeftX(rast), ST_Width(rast),
	ST_Value(rast, 1, 1, 1),
	ST_Value(rast, 1, 2, 1),
	ST_Value(rast, 1, 1, 2)
FROM o_3_loadedrast
ORDER BY ST_UpperLeftX(rast);
