SELECT count(*) FROM o_16_loadedrast;
SELECT ST_Width(rast), ST_Height(rast) FROM o_16_loadedrast;
SELECT ST_AsText(ST_Envelope(rast)) FROM o_16_loadedrast;
