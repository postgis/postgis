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
**  Useful functions for all GSERIALIZED handlers. 
**  XXX move to gserialized.h eventually
*/

/* Convert lwgeom to newly allocated gserialized */
GSERIALIZED* geography_serialize(LWGEOM *lwgeom);
GSERIALIZED* geometry_serialize(LWGEOM *lwgeom);
/* Check that the typmod matches the flags on the lwgeom */
void postgis_valid_typmod(LWGEOM *lwgeom, int32 typmod);
/* Check that the type is legal in geography (no curves please!) */
void geography_valid_type(uchar type);

/* Expand the embedded bounding box in a #GSERIALIZED */
GSERIALIZED* gserialized_expand(GSERIALIZED *g, double distance);

