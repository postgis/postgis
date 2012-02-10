-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- Test of "ST_AddBand".
-----------------------------------------------------------------------


-----------------------------------------------------------------------
--- ST_AddBand
-----------------------------------------------------------------------

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '1BB', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '1BB', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '1BB', 1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '1BB', 2, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '1BB', 21.46, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '2BUI', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '2BUI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '2BUI', 3, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '2BUI', 4, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '2BUI', 21.46, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '4BUI', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '4BUI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '4BUI', 15, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '4BUI', 16, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '4BUI', 21.46, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', -129, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', -128, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', 127, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', 128, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BSI', 210.46, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 255, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 256, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 410.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '8BUI', 255.9999999, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', -32769, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', -32768, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', 32767, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', 32768, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BSI', 210000.46, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', 65535, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', 65537, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '16BUI', 210000.4645643647457, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', -2147483649, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', -2147483648, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', 2147483647, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', 2147483648, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BSI', 210000.4645643647457, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 4294967295, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 4294967296, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 214294967296, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BUI', 4294967295.9999999, NULL), 3, 3);

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967000, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967000, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967295, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967295, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967296, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 4294967296, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 21.46, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 21003.1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 21003.1, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 123.456, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 123.456, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 1234.567, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 1234.567, NULL), 3, 3)::float4;
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 210000.4645643647457, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '32BF', 210000.4645643647457, NULL), 3, 3)::float4;

SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', -1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 0, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 14294967296.123456, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 21.46, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 21003.1, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 123.4567, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 1234.567, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 210000.4645643647457, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, '64BF', 1234.4645643647457, NULL), 3, 3);
SELECT St_Value(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0,0), 1, '64BF', 1234.5678, NULL), ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1, 1), 3, 3);
SELECT St_Value(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0,0), 1, '64BF', 1234.5678, NULL), ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0), 1), 3, 3);
SELECT St_Value(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0,0), 1, '64BF', 1234.5678, NULL), ST_MakeEmptyRaster(1000, 1000, 10, 10, 2, 2, 0, 0, 0)), 3, 3);
-- array version test
SELECT (ST_DumpAsPolygons(newrast,3)).val As b3val FROM (SELECT ST_AddBand(NULL, array_agg(rast)) AS newrast FROM (SELECT ST_AsRaster(ST_Buffer(ST_Point(10,10), 34),200,200, '8BUI',i*30) As rast FROM generate_series(1,3) As i ) As foo ) As foofoo;
