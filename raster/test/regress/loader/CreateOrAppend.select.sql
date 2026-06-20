SELECT count(*) FROM loadedrast;
SELECT ST_Width(rast), ST_Height(rast), ST_NumBands(rast) FROM loadedrast WHERE rid = 1;
SELECT ST_Value(rast, 1, 1, 1) FROM loadedrast WHERE rid = 1;
