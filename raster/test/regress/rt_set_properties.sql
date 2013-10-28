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
--- Test of "Set" functions for properties of the raster.
--- (Objective B03a)
-----------------------------------------------------------------------


-----------------------------------------------------------------------
--- ST_SetSRID
-----------------------------------------------------------------------

SELECT
    (srid+1) AS expected,
    st_srid(st_setsrid(rast,srid+1)) AS obtained
 FROM rt_properties_test
WHERE (srid+1) != st_srid(st_setsrid(rast,srid+1));


-----------------------------------------------------------------------
--- ST_SetScale
-----------------------------------------------------------------------

SELECT
    (scalex+2) AS expectedx,
    (scaley+3) AS expectedy,
    st_scalex(st_setscale(rast,scalex+2,scaley)) AS obtainedx,
    st_scaley(st_setscale(rast,scalex,scaley+3)) AS obtainedy
 FROM rt_properties_test
WHERE
    (scalex+2) != st_scalex(st_setscale(rast,scalex+2,scaley)) OR
    (scaley+3) != st_scaley(st_setscale(rast,scalex,scaley+3));

SELECT
    (scalex+2) AS expectedx,
    (scaley+3) AS expectedy,
    st_scalex(st_setscale(rast,scalex+2)) AS obtainedx,
    st_scaley(st_setscale(rast,scaley+3)) AS obtainedy
 FROM rt_properties_test
WHERE
    (scalex+2) != st_scalex(st_setscale(rast,scalex+2)) OR
    (scaley+3) != st_scaley(st_setscale(rast,scaley+3));

-----------------------------------------------------------------------
--- ST_SetSkew
-----------------------------------------------------------------------

SELECT
    (skewx+2) AS expectedx,
    (skewy+3) AS expectedy,
    st_skewx(st_setskew(rast,skewx+2,skewy)) AS obtainedx,
    st_skewy(st_setskew(rast,skewx,skewy+3)) AS obtainedy
 FROM rt_properties_test
WHERE
    (skewx+2) != st_skewx(st_setskew(rast,skewx+2,skewy)) OR
    (skewy+3) != st_skewy(st_setskew(rast,skewx,skewy+3));

SELECT
    (skewx+2) AS expectedx,
    (skewy+3) AS expectedy,
    st_skewx(st_setskew(rast,skewx+2)) AS obtainedx,
    st_skewy(st_setskew(rast,skewy+3)) AS obtainedy
 FROM rt_properties_test
WHERE
    (skewx+2) != st_skewx(st_setskew(rast,skewx+2)) OR
    (skewy+3) != st_skewy(st_setskew(rast,skewy+3));


-----------------------------------------------------------------------
--- ST_SetUpperLeft
-----------------------------------------------------------------------

SELECT
    (ipx+2) AS expectedx,
    (ipy+3) AS expectedy,
    st_upperleftx(st_setupperleft(rast,ipx+2,ipy)) AS obtainedx,
    st_upperlefty(st_setupperleft(rast,ipx,ipy+3)) AS obtainedy
 FROM rt_properties_test
WHERE
    (ipx+2) != st_upperleftx(st_setupperleft(rast,ipx+2,ipy)) OR
    (ipy+3) != st_upperlefty(st_setupperleft(rast,ipx,ipy+3));
