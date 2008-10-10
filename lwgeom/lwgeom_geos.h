/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <string.h>

#include "../postgis_config.h"

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "executor/spi.h"

#include "geos_c.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "profile.h"

#ifdef PROFILE
#warning PROFILE enabled!
#endif

/*
** Public prototypes for GEOS utility functions.
*/

PG_LWGEOM *GEOS2POSTGIS(GEOSGeom geom, char want3d);
GEOSGeom POSTGIS2GEOS(PG_LWGEOM *g);

LWGEOM *GEOS2LWGEOM(GEOSGeom geom, char want3d);
GEOSGeom LWGEOM2GEOS(LWGEOM *g);

POINTARRAY *ptarray_from_GEOSCoordSeq(GEOSCoordSeq cs, char want3d);
void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2);

