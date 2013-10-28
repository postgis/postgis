-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
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
-- st_upperleftx
-----------------------------------------------------------------------

SELECT id, name, ipx, st_upperleftx(rast) as ipx_expected
    FROM rt_properties_test
    WHERE st_upperleftx(rast) != ipx;

-----------------------------------------------------------------------
-- st_upperlefty
-----------------------------------------------------------------------

SELECT id, name, ipy, st_upperlefty(rast) as ipy_expected
    FROM rt_properties_test
    WHERE st_upperlefty(rast) != ipy;
