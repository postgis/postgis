SELECT count(*) FROM loadedrast;
SELECT count(*) FROM o_2_loadedrast;
SELECT ST_Width(rast), ST_Height(rast), ST_UpperLeftX(rast), ST_UpperLeftY(rast) FROM loadedrast ORDER BY rid LIMIT 1;
SELECT ST_Width(rast), ST_Height(rast), ST_UpperLeftX(rast), ST_UpperLeftY(rast) FROM o_2_loadedrast ORDER BY rid LIMIT 1;
