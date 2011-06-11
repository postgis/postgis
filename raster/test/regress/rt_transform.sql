BEGIN;
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1002163,'EPSG',2163,'PROJCS["unnamed",GEOGCS["unnamed ellipse",DATUM["unknown",SPHEROID["unnamed",6370997,0]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Lambert_Azimuthal_Equal_Area"],PARAMETER["latitude_of_center",45],PARAMETER["longitude_of_center",-100],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["Meter",1],AUTHORITY["EPSG","2163"]]','+proj=laea +lat_0=45 +lon_0=-100 +x_0=0 +y_0=0 +a=6370997 +b=6370997 +units=m +no_defs ');
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1003309,'EPSG',3309,'PROJCS["NAD27 / California Albers",GEOGCS["NAD27",DATUM["North_American_Datum_1927",SPHEROID["Clarke 1866",6378206.4,294.9786982139006,AUTHORITY["EPSG","7008"]],AUTHORITY["EPSG","6267"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4267"]],UNIT["metre",1,AUTHORITY["EPSG","9001"]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["standard_parallel_1",34],PARAMETER["standard_parallel_2",40.5],PARAMETER["latitude_of_center",0],PARAMETER["longitude_of_center",-120],PARAMETER["false_easting",0],PARAMETER["false_northing",-4000000],AUTHORITY["EPSG","3309"],AXIS["X",EAST],AXIS["Y",NORTH]]','+proj=aea +lat_1=34 +lat_2=40.5 +lat_0=0 +lon_0=-120 +x_0=0 +y_0=-4000000 +ellps=clrk66 +datum=NAD27 +units=m +no_defs ');
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1003310,'EPSG',3310,'PROJCS["NAD83 / California Albers",GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4269"]],UNIT["metre",1,AUTHORITY["EPSG","9001"]],PROJECTION["Albers_Conic_Equal_Area"],PARAMETER["standard_parallel_1",34],PARAMETER["standard_parallel_2",40.5],PARAMETER["latitude_of_center",0],PARAMETER["longitude_of_center",-120],PARAMETER["false_easting",0],PARAMETER["false_northing",-4000000],AUTHORITY["EPSG","3310"],AXIS["X",EAST],AXIS["Y",NORTH]]','+proj=aea +lat_1=34 +lat_2=40.5 +lat_0=0 +lon_0=-120 +x_0=0 +y_0=-4000000 +ellps=GRS80 +datum=NAD83 +units=m +no_defs ');
INSERT INTO "spatial_ref_sys" ("srid","auth_name","auth_srid","srtext","proj4text") VALUES (1004269,'EPSG',4269,'GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4269"]]','+proj=longlat +ellps=GRS80 +datum=NAD83 +no_defs ');
END;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003309
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1004269
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, NULL
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, 'Bilinear'
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, 'Bilinear', NULL
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, 'Cubic', 0.0
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, 'NearestNeighbour', 0.0
		) AS rast
	) foo
) bar;
SELECT
	srid,
	width > 0,
	height > 0,
	numbands,
	round(scalex::numeric, 3) > 0,
	round(scaley::numeric, 3) > 0,
	count > 0,
	min >= -10,
	max <= 3.14159
FROM (
	SELECT foo.rast, (st_summarystats(foo.rast)).*, (st_metadata(foo.rast)).* FROM (
		SELECT ST_Transform(
			ST_SetValue(
				ST_SetValue(
					ST_SetValue(
						ST_AddBand(
							ST_MakeEmptyRaster(10, 10, -500000, 600000, 1000, 1000, 0, 0, 1002163)
							, 1, '64BF', 0, 0
						)
						, 1, 1, 1, -10
					)
					, 1, 5, 4, 0
				)
				, 1, 5, 5, 3.14159
			)
			, 1003310, 'NearestNeighbor', 0.0
		) AS rast
	) foo
) bar;
BEGIN;
DELETE FROM "spatial_ref_sys" WHERE srid = 1002163;
DELETE FROM "spatial_ref_sys" WHERE srid = 1003309;
DELETE FROM "spatial_ref_sys" WHERE srid = 1003310;
DELETE FROM "spatial_ref_sys" WHERE srid = 1004269;
END;
