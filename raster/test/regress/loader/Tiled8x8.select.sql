SELECT
	srid,
	scale_x::numeric(16, 10),
	scale_y::numeric(16, 10),
	blocksize_x,
	blocksize_y,
	same_alignment,
	regular_blocking,
	num_bands,
	pixel_types,
	nodata_values::numeric(16,10)[],
	out_db,
	ST_Equals(extent, 'POLYGON((16 -90,8 -90,0 -90,0 -88,0 -80,0 -72,0 -64,0 -56,0 -48,0 -40,0 -32,0 -24,0 -16,0 -8,0 0,8 0,16 0,24 0,32 0,40 0,48 0,56 0,64 0,72 0,80 0,88 0,90 0,90 -8,90 -16,90 -24,90 -32,90 -40,90 -48,90 -56,90 -64,90 -72,90 -80,90 -88,90 -90,88 -90,80 -90,72 -90,64 -90,56 -90,48 -90,40 -90,32 -90,24 -90,16 -90))'::geometry)
FROM raster_columns
WHERE r_table_name = 'loadedrast'
	AND r_raster_column = 'rast';
SELECT ST_AsEWKT(geom), val FROM (SELECT (ST_PixelAsPolygons(rast, 1)).* FROM loadedrast WHERE rid = 12) foo WHERE x = 1 AND y = 1;
SELECT ST_AsEWKT(geom), val FROM (SELECT (ST_PixelAsPolygons(rast, 2)).* FROM loadedrast WHERE rid = 12) foo WHERE x = 1 AND y = 1;
SELECT ST_AsEWKT(geom), val FROM (SELECT (ST_PixelAsPolygons(rast, 2)).* FROM loadedrast WHERE rid = 133) foo WHERE x = 1 AND y = 1;
SELECT ST_AsEWKT(geom), val FROM (SELECT (ST_PixelAsPolygons(rast, 3)).* FROM loadedrast WHERE rid = 144) foo WHERE x = 1 AND y = 1;
