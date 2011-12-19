-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Mateusz Loskot <mateusz@loskot.net>
-- Copyright (C) 2011 Regents of the University of California
--   <bkpark@ucdavis.edu>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

SET client_min_messages TO warning;

-----------------------------------------------------------------------
--- Test RASTER_COLUMNS
-----------------------------------------------------------------------

-- Check table exists
SELECT c.relname FROM pg_class c, pg_views v
  WHERE c.relname = v.viewname
    AND v.viewname = 'raster_columns';

-----------------------------------------------------------------------
--- Test AddRasterConstraints and DropRasterConstraints
-----------------------------------------------------------------------

DROP TABLE IF EXISTS test_raster_columns;
CREATE TABLE test_raster_columns (
	rid integer,
	rast raster
);
CREATE OR REPLACE FUNCTION make_test_raster(
	rid integer,
	width integer DEFAULT 2,
	height integer DEFAULT 2,
	ul_x double precision DEFAULT 0,
	ul_y double precision DEFAULT 0,
	skew_x double precision DEFAULT 0,
	skew_y double precision DEFAULT 0,
	initvalue double precision DEFAULT 1,
	nodataval double precision DEFAULT 0
)
	RETURNS void
	AS $$
	DECLARE
		x int;
		y int;
		rast raster;
	BEGIN
		rast := ST_MakeEmptyRaster(width, height, ul_x, ul_y, 1, 1, skew_x, skew_y, 0);
		rast := ST_AddBand(rast, 1, '8BUI', initvalue, nodataval);


		INSERT INTO test_raster_columns VALUES (rid, rast);

		RETURN;
	END;
	$$ LANGUAGE 'plpgsql';
-- no skew
SELECT make_test_raster(0, 2, 2, -2, -2);
SELECT make_test_raster(1, 2, 2, 0, 0, 0, 0, 2);
SELECT make_test_raster(2, 2, 2, 1, -1, 0, 0, 3);
SELECT make_test_raster(3, 2, 2, 1, 1, 0, 0, 4);
SELECT make_test_raster(4, 2, 2, 2, 2, 0, 0, 5);

SELECT AddRasterConstraints(current_schema(), 'test_raster_columns', 'rast'::name);
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

SELECT DropRasterConstraints(current_schema(),'test_raster_columns', 'rast'::name);
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

SELECT AddRasterConstraints('test_raster_columns', 'rast'::name, 'srid'::text, 'extent', 'blocksize');
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

SELECT DropRasterConstraints('test_raster_columns', 'rast'::name, 'scale'::text);
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

SELECT AddRasterConstraints('test_raster_columns', 'rast', FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE);
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

SELECT DropRasterConstraints(current_schema(), 'test_raster_columns', 'rast'::name, 'scale'::text);
SELECT r_table_name, r_raster_column, srid, scale_x, scale_y, blocksize_x, blocksize_y, same_alignment, regular_blocking, num_bands, pixel_types, nodata_values, extent FROM raster_columns WHERE r_table_name = 'test_raster_columns';

DROP FUNCTION make_test_raster(integer, integer, integer, double precision, double precision, double precision, double precision, double precision, double precision);
DROP TABLE IF EXISTS test_raster_columns;
