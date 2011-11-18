-----------------------------------------------------------------------
-- $Id$
--
-- Copyright (c) 2010 David Zwarg <dzwarg@azavea.com>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
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
