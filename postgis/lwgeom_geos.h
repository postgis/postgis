/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "executor/spi.h"

#include "../postgis_config.h"

/* Workaround for GEOS 2.2 compatibility: old geos_c.h does not contain
   header guards to protect from multiple inclusion */
#ifndef GEOS_C_INCLUDED
#define GEOS_C_INCLUDED
#include "geos_c.h"
#endif

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "profile.h"
#include "../liblwgeom/lwgeom_geos.h"

#include <string.h>

#if POSTGIS_PROFILE > 0
#warning POSTGIS_PROFILE enabled!
#endif

/*
** Public prototypes for GEOS utility functions.
*/

PG_LWGEOM *GEOS2POSTGIS(GEOSGeom geom, char want3d);
GEOSGeometry * POSTGIS2GEOS(PG_LWGEOM *g);


void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2);

