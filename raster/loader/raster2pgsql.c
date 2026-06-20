/*
 *
 * PostGIS raster loader
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2011 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "raster2pgsql.h"
#include "gdal_vrt.h"
#include "ogr_srs_api.h"
#include <assert.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
