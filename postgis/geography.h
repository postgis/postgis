/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/*
** Spherical radius.
** Moritz, H. (1980). Geodetic Reference System 1980, by resolution of the XVII General Assembly of the IUGG in Canberra.
** http://en.wikipedia.org/wiki/Earth_radius
*/

#define WGS84_RADIUS 6371009.0

/**********************************************************************
**  GIDX structure. 
**
**  This is an n-dimensional (practically, the 
**  implementation is 2-4 dimensions) box used for index keys. The 
**  coordinates are anonymous, so we can have any number of dimensions.
**  The sizeof a GIDX is 1 + 2 * ndims * sizeof(float).
**  The order of values in a GIDX is
**  xmin,xmax,ymin,ymax,zmin,zmax...
*/
typedef struct
{
	int32 varsize;
	float c[1];
} GIDX;

/*
** For some GiST support functions, it is more efficient to allocate
** memory for our GIDX off the stack and cast that memory into a GIDX.
** But, GIDX is variable length, what to do? We'll bake in the assumption
** that no GIDX is more than 4-dimensional for now, and ensure that much
** space is available.
** 4 bytes varsize + 4 dimensions * 2 ordinates * 4 bytes float size = 36 bytes
*/
#define GIDX_MAX_SIZE 36


/* Useful functions for all GEOGRAPHY handlers. */
GSERIALIZED* geography_serialize(LWGEOM *lwgeom);
void geography_valid_typmod(LWGEOM *lwgeom, int32 typmod);
void geography_valid_type(uchar type);
int geography_datum_gidx(Datum geography_datum, GIDX *gidx);
GIDX* gidx_new(int ndims);
void gbox_from_gidx(GIDX *gidx, GBOX *gbox);
