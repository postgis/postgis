-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009-2010 Mateusz Loskot <mateusz@loskot.net>, David Zwarg <dzwarg@avencia.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
-- st_georeference (default)
-----------------------------------------------------------------------

SELECT 
    replace(st_georeference(rast)::text, E'\n', E'EOL'),
    replace(st_georeference(rast)::text, E'\n', E'EOL') = 
    '2.0000000000EOL0.0000000000EOL0.0000000000EOL3.0000000000EOL0.5000000000EOL0.5000000000EOL'
FROM rt_properties_test 
WHERE id = 0;

SELECT 
    replace(st_georeference(rast)::text, E'\n', E'EOL'),
    replace(st_georeference(rast)::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL2.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 1;

SELECT 
    replace(st_georeference(rast)::text, E'\n', E'EOL'),
    replace(st_georeference(rast)::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL7.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 2;

SELECT 
    replace(st_georeference(rast)::text, E'\n', E'EOL'),
    replace(st_georeference(rast)::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL7.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 3;

-----------------------------------------------------------------------
-- st_georeference (GDAL)
-----------------------------------------------------------------------

SELECT 
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL'),
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL') = 
    '2.0000000000EOL0.0000000000EOL0.0000000000EOL3.0000000000EOL0.5000000000EOL0.5000000000EOL'
FROM rt_properties_test 
WHERE id = 0;

SELECT 
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL'),
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL2.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 1;

SELECT 
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL'),
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL7.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 2;

SELECT 
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL'),
    replace(st_georeference(rast,'GDAL')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL7.5000000000EOL2.5000000000EOL'
FROM rt_properties_test 
WHERE id = 3;

-----------------------------------------------------------------------
-- st_georeference (ESRI)
-----------------------------------------------------------------------

SELECT
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL'),
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL') = 
    '2.0000000000EOL0.0000000000EOL0.0000000000EOL3.0000000000EOL1.5000000000EOL2.0000000000EOL'
FROM rt_properties_test 
WHERE id = 0;

SELECT
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL'),
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL5.0000000000EOL5.0000000000EOL'
FROM rt_properties_test 
WHERE id = 1;

SELECT
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL'),
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL10.0000000000EOL5.0000000000EOL'
FROM rt_properties_test 
WHERE id = 2;

SELECT
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL'),
	replace(st_georeference(rast,'ESRI')::text, E'\n', E'EOL') = 
    '5.0000000000EOL0.0000000000EOL0.0000000000EOL5.0000000000EOL10.0000000000EOL5.0000000000EOL'
FROM rt_properties_test 
WHERE id = 3;


-----------------------------------------------------------------------
-- st_setgeoreference (error conditions)
-----------------------------------------------------------------------

SELECT
    -- all 6 parameters must be numeric
    st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 nodata') IS NULL
FROM rt_properties_test 
WHERE id = 0;

SELECT
    -- must have 6 parameters
    st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000') IS NULL
FROM rt_properties_test 
WHERE id = 1;

SELECT
    -- any whitespace accepted between parameters as well as ' '
    st_setgeoreference(rast,E'2.0000000000	1.0000000000\n2.0000000000\t3.0000000000	1.5000000000	2.0000000000') IS NOT NULL
FROM rt_properties_test 
WHERE id = 2;

-----------------------------------------------------------------------
-- st_setgeoreference (warning conditions)
-----------------------------------------------------------------------

SELECT
    -- raster arg is null
    st_setgeoreference(null,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000') IS NULL
FROM rt_properties_test 
WHERE id = 0;

-----------------------------------------------------------------------
-- st_setgeoreference (default)
-----------------------------------------------------------------------

SELECT
	st_scalex(rast) = 2,
	st_scaley(rast) = 3,
	st_scalex(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000')) = 4,
	st_scaley(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000')) = 6
FROM rt_properties_test 
WHERE id = 0;

SELECT
	st_skewx(rast) = 0,
	st_skewy(rast) = 0,
	st_skewx(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000')) = 2,
	st_skewy(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000')) = 1
FROM rt_properties_test 
WHERE id = 1;

SELECT
	st_upperleftx(rast) = 7.5,
	st_upperlefty(rast) = 2.5,
	st_upperleftx(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000')) = 1.5,
	st_upperlefty(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000')) = 2.0
FROM rt_properties_test 
WHERE id = 2;

-----------------------------------------------------------------------
-- st_setgeoreference (GDAL)
-----------------------------------------------------------------------

SELECT
	st_scalex(rast) = 2,
	st_scaley(rast) = 3,
	st_scalex(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000','GDAL')) = 4,
	st_scaley(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000','GDAL')) = 6
FROM rt_properties_test 
WHERE id = 0;

SELECT
	st_skewx(rast) = 0,
	st_skewy(rast) = 0,
	st_skewx(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','GDAL')) = 2,
	st_skewy(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','GDAL')) = 1
FROM rt_properties_test 
WHERE id = 1;

SELECT
	st_upperleftx(rast) = 7.5,
	st_upperlefty(rast) = 2.5,
	st_upperleftx(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','GDAL')) = 1.5,
	st_upperlefty(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','GDAL')) = 2.0
FROM rt_properties_test 
WHERE id = 2;

-----------------------------------------------------------------------
-- st_setgeoreference (ESRI)
-----------------------------------------------------------------------

SELECT
	st_scalex(rast) = 2,
	st_scaley(rast) = 3,
	st_scalex(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000','ESRI')) = 4,
	st_scaley(st_setgeoreference(rast,'4.0000000000 0.0000000000 0.0000000000 6.0000000000 1.5000000000 2.0000000000','ESRI')) = 6
FROM rt_properties_test 
WHERE id = 0;

SELECT
	st_skewx(rast) = 0,
	st_skewy(rast) = 0,
	st_skewx(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','ESRI')) = 2,
	st_skewy(st_setgeoreference(rast,'2.0000000000 1.0000000000 2.0000000000 3.0000000000 1.5000000000 2.0000000000','ESRI')) = 1
FROM rt_properties_test 
WHERE id = 1;

SELECT
	st_upperleftx(rast) = 7.5,
	st_upperlefty(rast) = 2.5,
	st_upperleftx(st_setgeoreference(rast,'2.0000000000 0.0000000000 0.0000000000 3.0000000000 1.0000000000 2.5000000000','ESRI')) = 0,
	st_upperlefty(st_setgeoreference(rast,'2.0000000000 0.0000000000 0.0000000000 3.0000000000 1.0000000000 2.5000000000','ESRI')) = 1
FROM rt_properties_test 
WHERE id = 2;


