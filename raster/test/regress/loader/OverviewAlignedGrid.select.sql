SELECT 'base', ST_UpperLeftX(rast), ST_Width(rast)
FROM loadedrast
ORDER BY ST_UpperLeftX(rast);
SELECT 'overview', ST_UpperLeftX(rast), ST_Width(rast),
	ST_UpperLeftX(rast) + ST_Width(rast) * ST_ScaleX(rast)
FROM o_3_loadedrast
ORDER BY ST_UpperLeftX(rast);
SELECT 'overview_values',
	ST_Value(rast, 1, 1),
	ST_Value(rast, 17, 1),
	ST_Value(rast, ST_Width(rast), 1)
FROM o_3_loadedrast
WHERE ST_UpperLeftX(rast) = 99;
