
---------------------------------------------------------------
----- ST_IntersectionFractions --------------------------------

CREATE TABLE raster_proportions_rast (
	name text,
	rast raster
);

INSERT INTO raster_proportions_rast (name, rast) VALUES (
	'2x2 raster covering 0,0 to 10,10',
	ST_MakeEmptyRaster(
		2,  2,   -- width/height in pixels
		0, 10,   -- x/y coordinate of the upper-left corner
		5, -5,   -- pixel width/height in ground units
		0,  0,   -- skew x/y
		0        -- SRID
	));

INSERT INTO raster_proportions_rast (name, rast) VALUES (
	'10x10 raster covering 0,0 to 10,10',
	ST_MakeEmptyRaster(
		10, 10,   -- width/height in pixels
		 0, 10,   -- x/y coordinate of the upper-left corner
		 1, -1,   -- pixel width/height in ground units
		 0,  0,   -- skew x/y
		 0         -- SRID
	));

INSERT INTO raster_proportions_rast (name, rast) VALUES (
	'2x2 raster covering -10,-10 to 0,0',
	ST_MakeEmptyRaster(
		 2,  2,   -- width/height in pixels
	 -10,  0,   -- x/y coordinate of the upper-left corner
		 5, -5,   -- pixel width/height in ground units
		 0,  0,   -- skew x/y
		 0         -- SRID
	));

INSERT INTO raster_proportions_rast (name, rast) VALUES (
	'2x2 raster covering 5,5 to 6,6',
	ST_MakeEmptyRaster(
		 2,  2,   -- width/height in pixels
		 5,  6,   -- x/y coordinate of the upper-left corner
	 0.5, -0.5, -- pixel width/height in ground units
		 0,  0,   -- skew x/y
		 0        -- SRID
	));

SET extra_float_digits = -8;
SELECT 'polygon', name, ST_DumpValues(
	ST_IntersectionFractions(
		rast,
		'POLYGON((5 0, 0 5, 5 10, 10 5, 5 0))'::geometry),1)
FROM raster_proportions_rast;

SELECT 'multipolygon', name, ST_DumpValues(
	ST_IntersectionFractions(
		rast,
		'MULTIPOLYGON(((5 0, 0 5, 5 10, 10 5, 5 0)))'::geometry),1)
FROM raster_proportions_rast;

SELECT 'linestring', name, ST_DumpValues(
	ST_IntersectionFractions(
		rast,
		'LINESTRING(0 0, 10 10)'::geometry), 1)
FROM raster_proportions_rast;

SELECT 'multilinestring', name, ST_DumpValues(
	ST_IntersectionFractions(
		rast,
		'MULTILINESTRING((0 0, 10 10), (0 10, 10 0))'::geometry), 1)
FROM raster_proportions_rast;

DROP TABLE IF EXISTS raster_proportions_rast;
