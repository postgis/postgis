-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 David Zwarg <dzwarg@azavea.com>
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
--- ST_SetBandNoDataValue
-----------------------------------------------------------------------

SELECT
    (b1nodatavalue+1) AS expected,
    st_bandnodatavalue(st_setbandnodatavalue(rast, 1, b1nodatavalue+1),1) AS obtained
 FROM rt_band_properties_test
WHERE (b1nodatavalue+1) != st_bandnodatavalue(st_setbandnodatavalue(rast, 1, b1nodatavalue+1),1);
