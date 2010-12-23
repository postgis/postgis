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
#define WKT_NO_TYPE 0x08 /* Internal use only */
#define WKT_NO_PARENS 0x10 /* Internal use only */
#define WKT_IS_CHILD 0x20 /* Internal use only */

/**
* Well-Known Binary (WKB) Output Variant Types
*/

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


/*
* Internal prototypes
*/

/*
* Force dims
*/
LWGEOM* lwgeom_force_dims(const LWGEOM *lwgeom, int hasz, int hasm);
LWPOINT* lwpoint_force_dims(const LWPOINT *lwpoint, int hasz, int hasm);
LWLINE* lwline_force_dims(const LWLINE *lwline, int hasz, int hasm);
LWPOLY* lwpoly_force_dims(const LWPOLY *lwpoly, int hasz, int hasm);
LWCOLLECTION* lwcollection_force_dims(const LWCOLLECTION *lwcol, int hasz, int hasm);
POINTARRAY* ptarray_force_dims(const POINTARRAY *pa, int hasz, int hasm);

/*
* Is Empty?
*/
int lwpoly_is_empty(const LWPOLY *poly);
int lwcollection_is_empty(const LWCOLLECTION *col);
int lwcircstring_is_empty(const LWCIRCSTRING *circ);
int lwtriangle_is_empty(const LWTRIANGLE *triangle);
int lwline_is_empty(const LWLINE *line);
int lwpoint_is_empty(const LWPOINT *point);

/*
* Number of vertices?
*/
int lwline_count_vertices(LWLINE *line);
int lwpoly_count_vertices(LWPOLY *poly);
int lwcollection_count_vertices(LWCOLLECTION *col);

/*
* DP simplification
*/
POINTARRAY* ptarray_simplify(POINTARRAY *inpts, double epsilon);
LWLINE* lwline_simplify(const LWLINE *iline, double dist);
LWPOLY* lwpoly_simplify(const LWPOLY *ipoly, double dist);
LWCOLLECTION* lwcollection_simplify(const LWCOLLECTION *igeom, double dist);

/*
* Point comparisons
*/
int p4d_same(POINT4D p1, POINT4D p2);

/*
* Area calculations
*/
double lwpoly_area(const LWPOLY *poly);
double lwcurvepoly_area(const LWCURVEPOLY *curvepoly);
double lwtriangle_area(const LWTRIANGLE *triangle);

/*
* Length calculations
*/
double lwcompound_length(const LWCOMPOUND *comp);
double lwcompound_length_2d(const LWCOMPOUND *comp);
double lwline_length(const LWLINE *line);
double lwline_length_2d(const LWLINE *line);
double lwcircstring_length(const LWCIRCSTRING *circ);
double lwcircstring_length_2d(const LWCIRCSTRING *circ);
double lwpoly_perimeter(const LWPOLY *poly);
double lwpoly_perimeter_2d(const LWPOLY *poly);
double lwcurvepoly_perimeter(const LWCURVEPOLY *poly);
double lwcurvepoly_perimeter_2d(const LWCURVEPOLY *poly);
double lwtriangle_perimeter(const LWTRIANGLE *triangle);
double lwtriangle_perimeter_2d(const LWTRIANGLE *triangle);

/*
* Segmentization
*/
LWLINE *lwcircstring_segmentize(const LWCIRCSTRING *icurve, uint32 perQuad);
LWLINE *lwcompound_segmentize(const LWCOMPOUND *icompound, uint32 perQuad);
LWPOLY *lwcurvepoly_segmentize(const LWCURVEPOLY *curvepoly, uint32 perQuad);

/*
* Affine
*/
void ptarray_affine(POINTARRAY *pa, const AFFINE *affine);

/*
* PointArray
*/
char ptarray_isccw(const POINTARRAY *pa);

/*
* Clone
*/
LWPOINT *lwpoint_clone(const LWPOINT *lwgeom);
LWLINE *lwline_clone(const LWLINE *lwgeom);
LWPOLY *lwpoly_clone(const LWPOLY *lwgeom);
LWTRIANGLE *lwtriangle_clone(const LWTRIANGLE *lwgeom);
LWCOLLECTION *lwcollection_clone(const LWCOLLECTION *lwgeom);
LWCIRCSTRING *lwcircstring_clone(const LWCIRCSTRING *curve);
BOX2DFLOAT4 *box2d_clone(const BOX2DFLOAT4 *lwgeom);
POINTARRAY *ptarray_clone(const POINTARRAY *ptarray);
LWLINE *lwline_clone_deep(const LWLINE *lwgeom);
LWPOLY *lwpoly_clone_deep(const LWPOLY *lwgeom);
LWCOLLECTION *lwcollection_clone_deep(const LWCOLLECTION *lwgeom);


