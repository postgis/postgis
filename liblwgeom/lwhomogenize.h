/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom.h"


typedef struct
{
	LWMPOINT *points;
	LWMLINE *lines;
	LWMPOLY *polys;
} LWGEOM_HOMOGENIZE;

static LWGEOM_HOMOGENIZE *
lwcollection_homogenize_subgeom(LWGEOM_HOMOGENIZE *hgeoms, LWGEOM *geom);
