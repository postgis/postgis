-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

CREATE TABLE empty_raster_test (
	rid numeric,
	rast raster
);

INSERT INTO empty_raster_test
VALUES (1, ST_MakeEmptyRaster( 100, 100, 0.0005, 0.0005, 1, 1, 0, 0, 4326) );

-------------------------------------------------------------------
-- st_isempty
-----------------------------------------------------------------------
select st_isempty(rast) from empty_raster_test;

DROP TABLE empty_raster_test;
