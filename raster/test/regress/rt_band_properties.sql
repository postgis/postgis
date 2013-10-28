-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 David Zwarg <dzwarg@azavea.com>, Pierre Racine <pierre.racine@sbf.ulaval.ca>
--
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
-----------------------------------------------------------------------

-----------------------------------------------------------------------
--- Test of "Set" functions for properties of a raster band.
-----------------------------------------------------------------------


-----------------------------------------------------------------------
--- ST_BandPixelType
-----------------------------------------------------------------------

SELECT
    b1pixeltype AS b1expected, b2pixeltype AS b2expected,
    st_bandpixeltype(rast, 1) AS b1obtained, st_bandpixeltype(rast, 2) AS b2obtained
 FROM rt_band_properties_test
WHERE b1pixeltype != st_bandpixeltype(rast, 1) or b2pixeltype != st_bandpixeltype(rast, 2);

-----------------------------------------------------------------------
--- ST_BandNoDataValue
-----------------------------------------------------------------------

SELECT
    b1nodatavalue AS b1expected, b2nodatavalue AS b2expected,
    st_bandnodatavalue(rast, 1) AS b1obtained, b2nodatavalue != st_bandnodatavalue(rast, 2)
 FROM rt_band_properties_test
WHERE b1nodatavalue != st_bandnodatavalue(rast, 1) or b2nodatavalue != st_bandnodatavalue(rast, 2);


-----------------------------------------------------------------------
--- ST_BandIsNoData
-----------------------------------------------------------------------
SELECT
    st_bandisnodata(rast, 1)
  FROM rt_band_properties_test
WHERE id = 3;

SELECT
    st_bandisnodata(rast, 2)
  FROM rt_band_properties_test
WHERE id = 3;


-----------------------------------------------------------------------
--- ST_SetBandIsNoData
-----------------------------------------------------------------------
SELECT
    st_bandisnodata(rast, 1)
  FROM rt_band_properties_test
WHERE id = 4;

SELECT
    st_bandisnodata(rast, 1, TRUE)
  FROM rt_band_properties_test
WHERE id = 4;

UPDATE rt_band_properties_test
    SET rast = st_setbandisnodata(rast, 1)
    WHERE id = 4;

SELECT
    st_bandisnodata(rast, 1)
  FROM rt_band_properties_test
WHERE id = 4;

