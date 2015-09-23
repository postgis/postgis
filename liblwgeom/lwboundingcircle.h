/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LWBOUNDINGCIRCLE
#define _LWBOUNDINGCIRCLE 1

typedef struct {
	POINT2D center;
	double radius;
} LW_BOUNDINGCIRCLE;

int lwgeom_calculate_mbc(const LWGEOM* g, LW_BOUNDINGCIRCLE* result);

#endif
