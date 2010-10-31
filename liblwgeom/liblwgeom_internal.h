/**********************************************************************
 * $Id: liblwgeom_internal.h 4497 2009-09-14 18:33:54Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <assert.h>
#include "liblwgeom.h"
#include <string.h>
#include <math.h>
#include <inttypes.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif




/**
* PI
*/
#define PI 3.1415926535897932384626433832795


/**
* Well-Known Text (WKT) Output Variant Types
*/
#define WKT_ISO 0x01
#define WKT_SFSQL 0x02
#define WKT_EXTENDED 0x04
#define WKT_NO_TYPE 0x08 /* Internal use only */
#define WKT_NO_PARENS 0x10 /* Internal use only */

/**
* Well-Known Binary (WKB) Output Variant Types
*/
#define WKB_ISO 0x01
#define WKB_SFSQL 0x02
#define WKB_EXTENDED 0x04
#define WKB_NDR 0x08
#define WKB_HEX 0x10
#define WKB_NO_NPOINTS 0x20 /* Internal use only */
#define WKB_NO_SRID 0x40 /* Internal use only */

#define WKB_DOUBLE_SIZE 8 /* Internal use only */
#define WKB_INT_SIZE 4 /* Internal use only */
#define WKB_BYTE_SIZE 1 /* Internal use only */

/**
* Well-Known Binary (WKB) Geometry Types 
*/
#define WKB_POINT_TYPE 1
#define WKB_LINESTRING_TYPE 2
#define WKB_POLYGON_TYPE 3
#define WKB_MULTIPOINT_TYPE 4
#define WKB_MULTILINESTRING_TYPE 5
#define WKB_MULTIPOLYGON_TYPE 6
#define WKB_GEOMETRYCOLLECTION_TYPE 7
#define WKB_CIRCULARSTRING_TYPE 8
#define WKB_COMPOUNDCURVE_TYPE 9
#define WKB_CURVEPOLYGON_TYPE 10
#define WKB_MULTICURVE_TYPE 11
#define WKB_MULTISURFACE_TYPE 12
#define WKB_CURVE_TYPE 13 /* from ISO draft, not sure is real */
#define WKB_SURFACE_TYPE 14 /* from ISO draft, not sure is real */
#define WKB_POLYHEDRALSURFACE_TYPE 15
#define WKB_TIN_TYPE 16
#define WKB_TRIANGLE_TYPE 17



/**
* Macro for reading the size from the GSERIALIZED size attribute.
* Cribbed from PgSQL, top 30 bits are size. Use VARSIZE() when working
* internally with PgSQL.
*/
#define SIZE_GET(varsize) (((varsize) >> 2) & 0x3FFFFFFF)
#define SIZE_SET(varsize, size) (((varsize) & 0x00000003)|(((size) & 0x3FFFFFFF) << 2 ))









/**
* Serialize/deserialize from/to #G_GEOMETRY into #GSERIALIZED
extern size_t gserialized_from_ggeometry_size(G_GEOMETRY *geom);
extern GSERIALIZED* gserialized_from_ggeometry(G_GEOMETRY *geom);
extern G_GEOMETRY* ggeometry_from_gserialized(GSERIALIZED *g);
*/





