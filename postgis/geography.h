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


/**********************************************************************
** Spherical radius.
** Moritz, H. (1980). Geodetic Reference System 1980, by resolution of 
** the XVII General Assembly of the IUGG in Canberra.
** http://en.wikipedia.org/wiki/Earth_radius
** http://en.wikipedia.org/wiki/World_Geodetic_System
*/

#define WGS84_MAJOR_AXIS 6378137.0
#define WGS84_INVERSE_FLATTENING 298.257223563
#define WGS84_MINOR_AXIS (WGS84_MAJOR_AXIS - WGS84_MAJOR_AXIS / WGS84_INVERSE_FLATTENING)
#define WGS84_RADIUS ((2.0 * WGS84_MAJOR_AXIS + WGS84_MINOR_AXIS ) / 3.0)

/* 
** EPSG WGS84 geographics, OGC standard default SRS, better be in 
** the SPATIAL_REF_SYS table!
*/
#define SRID_DEFAULT 4326

/**********************************************************************
**  Useful functions for all GEOGRAPHY handlers. 
*/

/* Convert lwgeom to newly allocated gserialized */
GSERIALIZED* geography_serialize(LWGEOM *lwgeom);
/* Check that the typmod matches the flags on the lwgeom */
void geography_valid_typmod(LWGEOM *lwgeom, int32 typmod);
/* Check that the type is legal in geography (no curves please!) */
void geography_valid_type(uchar type);



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

/*********************************************************************************
** GIDX support functions.
**
** We put the GIDX support here rather than libgeom because it is a specialized 
** type only used for indexing purposes. It also makes use of some PgSQL
** infrastructure like the VARSIZE() macros.
*/

/* Returns number of dimensions for this GIDX */
#define GIDX_NDIMS(gidx) ((VARSIZE((gidx)) - VARHDRSZ) / (2 * sizeof(float)))
/* Minimum accessor. */
#define GIDX_GET_MIN(gidx, dimension) ((gidx)->c[2*(dimension)])
/* Maximum accessor. */
#define GIDX_GET_MAX(gidx, dimension) ((gidx)->c[2*(dimension)+1])
/* Minimum setter. */
#define GIDX_SET_MIN(gidx, dimension, value) ((gidx)->c[2*(dimension)] = (value))
/* Maximum setter. */
#define GIDX_SET_MAX(gidx, dimension, value) ((gidx)->c[2*(dimension)+1] = (value))
/* Returns the size required to store a GIDX of requested dimension */
#define GIDX_SIZE(dimensions) (sizeof(int32) + 2*(dimensions)*sizeof(float))
/* Allocate a new gidx */
GIDX* gidx_new(int ndims);
/* Convert a gidx to a gbox */
void gbox_from_gidx(GIDX *gidx, GBOX *gbox);
/* Convert a gbox to a new gidx */
GIDX* gidx_from_gbox(GBOX box);


/* Pull out the gidx bounding box with a absolute minimum system overhead */
int gserialized_datum_get_gidx_p(Datum gserialized_datum, GIDX *gidx);
/* Pull out the gidx bounding box from an already de-toasted geography */
int gserialized_get_gidx_p(GSERIALIZED *g, GIDX *gidx);
/* Copy a new bounding box into an existing gserialized */
GSERIALIZED* gserialized_set_gidx(GSERIALIZED *g, GIDX *gidx);
/* Expand the embedded bounding box in a #GSERIALIZED */
GSERIALIZED* gserialized_expand(GSERIALIZED *g, double distance);

/* Pull out a gbox bounding box as fast as possible. */
int gserialized_datum_get_gbox_p(Datum gsdatum, GBOX *gbox);
/* Given two datums, do they overlap? Computed very fast using embedded boxes. */
int gserialized_datum_overlaps(Datum gs1, Datum gs2);
/* Remove the box from a disk serialization */
GSERIALIZED* gserialized_drop_gidx(GSERIALIZED *g);



