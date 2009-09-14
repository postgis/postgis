/**********************************************************************
 * $Id: lwgeodetic.c 4494 2009-09-14 10:54:33Z mcayland $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <math.h>
#include "libgeom.h"

/**
* Point in spherical coordinates on the world. Units of radians.
*/
typedef struct {
	double lon;
	double lat;
} GEOGRAPHIC_POINT;

/**
* Two-point great circle segment from a to b.
*/
typedef struct {
	GEOGRAPHIC_POINT a;
	GEOGRAPHIC_POINT b;
} GEOGRAPHIC_EDGE;

/**
* Conversion functions
*/
#define deg2rad(d) (PI * (d) / 180.0)
#define rad2deg(r) (180.0 * (r) / PI)

/**
* Ape a java function
*/
#define signum(a) ((a) < 0 ? -1 : ((a) > 0 ? 1 : (a)))

/* 
** Prototypes for internal functions.
*/
double sphere_distance(GEOGRAPHIC_POINT a, GEOGRAPHIC_POINT b);
double sphere_direction(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e);
void sphere_project(GEOGRAPHIC_POINT r, double distance, double azimuth, GEOGRAPHIC_POINT *n);
void sphere_gbox(GEOGRAPHIC_EDGE e, GBOX *gbox);


