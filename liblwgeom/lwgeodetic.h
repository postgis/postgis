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
	GEOGRAPHIC_POINT start;
	GEOGRAPHIC_POINT end;
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
void robust_cross_product(GEOGRAPHIC_POINT p, GEOGRAPHIC_POINT q, POINT3D *a);
void x_to_z(POINT3D *p);
void y_to_z(POINT3D *p);
int edge_point_on_plane(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p);
int edge_contains_longitude(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p);
int edge_contains_point(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p);
double z_to_latitude(double z);
int clairaut_cartesian(POINT3D start, POINT3D end, int top, GEOGRAPHIC_POINT *g);
int clairaut_geographic(GEOGRAPHIC_POINT start, GEOGRAPHIC_POINT end, int top, GEOGRAPHIC_POINT *g);
double sphere_distance(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e);
double sphere_direction(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e);
int sphere_project(GEOGRAPHIC_POINT r, double distance, double azimuth, GEOGRAPHIC_POINT *n);
int edge_calculate_gbox(GEOGRAPHIC_EDGE e, GBOX *gbox);

