/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2011-2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2007-2008 Mark Cave-Ayland
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#ifndef _LIBLWGEOM_INTERNAL_H
#define _LIBLWGEOM_INTERNAL_H 1

#include "../postgis_config.h"

#include "lwgeom_log.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#include "liblwgeom.h"

/**
* Floating point comparators.
*/
#define FP_TOLERANCE 1e-12
#define FP_IS_ZERO(A) (fabs(A) <= FP_TOLERANCE)
#define FP_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define FP_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define FP_ABS(a)   ((a) <	(0) ? -(a) : (a))
#define FP_EQUALS(A, B) (fabs((A)-(B)) <= FP_TOLERANCE)
#define FP_NEQUALS(A, B) (fabs((A)-(B)) > FP_TOLERANCE)
#define FP_LT(A, B) (((A) + FP_TOLERANCE) < (B))
#define FP_LTEQ(A, B) (((A) - FP_TOLERANCE) <= (B))
#define FP_GT(A, B) (((A) - FP_TOLERANCE) > (B))
#define FP_GTEQ(A, B) (((A) + FP_TOLERANCE) >= (B))
#define FP_CONTAINS_TOP(A, X, B) (FP_LT(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_BOTTOM(A, X, B) (FP_LTEQ(A, X) && FP_LT(X, B))
#define FP_CONTAINS_INCL(A, X, B) (FP_LTEQ(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_EXCL(A, X, B) (FP_LT(A, X) && FP_LT(X, B))
#define FP_CONTAINS(A, X, B) FP_CONTAINS_EXCL(A, X, B)


/*
* this will change to NaN when I figure out how to
* get NaN in a platform-independent way
*/
#define NO_VALUE 0.0
#define NO_Z_VALUE NO_VALUE
#define NO_M_VALUE NO_VALUE


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
* Macro that returns:
* -1 if n < 0,
*  1 if n > 0,
*  0 if n == 0
*/
#define SIGNUM(n) (((n) > 0) - ((n) < 0))

/**
* Tolerance used to determine equality.
*/
#define EPSILON_SQLMM 1e-8

/*
 * Export functions
 */

/* Any (absolute) values outside this range will be printed in scientific notation */
#define OUT_MIN_DOUBLE 1E-8
#define OUT_MAX_DOUBLE 1E15
#define OUT_DEFAULT_DECIMAL_DIGITS 15

/* 17 digits are sufficient for round-tripping
 * Then we might add up to 8 (from OUT_MIN_DOUBLE) max leading zeroes (or 2 digits for "e+") */
#define OUT_MAX_DIGITS 17 + 8

/* Limit for the max amount of characters that a double can use, including dot and sign */
/* */
#define OUT_MAX_BYTES_DOUBLE (1 /* Sign */ + 2 /* 0.x */ + OUT_MAX_DIGITS)
#define OUT_DOUBLE_BUFFER_SIZE OUT_MAX_BYTES_DOUBLE + 1 /* +1 including NULL */

/**
* Constants for point-in-polygon return values
*/
#define LW_INSIDE 1
#define LW_BOUNDARY 0
#define LW_OUTSIDE -1

/*
* Internal prototypes
*/

/* Machine endianness */
#define XDR 0 /* big endian */
#define NDR 1 /* little endian */


/*
* Force dims
*/
LWGEOM* lwgeom_force_dims(const LWGEOM *lwgeom, int hasz, int hasm, double zval, double mval);
LWPOINT* lwpoint_force_dims(const LWPOINT *lwpoint, int hasz, int hasm, double zval, double mval);
LWLINE* lwline_force_dims(const LWLINE *lwline, int hasz, int hasm, double zval, double mval);
LWPOLY* lwpoly_force_dims(const LWPOLY *lwpoly, int hasz, int hasm, double zval, double mval);
LWCOLLECTION* lwcollection_force_dims(const LWCOLLECTION *lwcol, int hasz, int hasm, double zval, double mval);
POINTARRAY* ptarray_force_dims(const POINTARRAY *pa, int hasz, int hasm, double zval, double mval);

/**
 * Swap ordinate values o1 and o2 on a given POINTARRAY
 *
 * Ordinates semantic is: 0=x 1=y 2=z 3=m
 */
void ptarray_swap_ordinates(POINTARRAY *pa, LWORD o1, LWORD o2);

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
uint32_t lwline_count_vertices(LWLINE *line);
uint32_t lwpoly_count_vertices(LWPOLY *poly);
uint32_t lwcollection_count_vertices(LWCOLLECTION *col);

/*
* DP simplification
*/

/**
 * @param minpts minimum number of points to retain, if possible.
 */
void ptarray_simplify_in_place(POINTARRAY *pa, double tolerance, uint32_t minpts);

/*
* The possible ways a pair of segments can interact. Returned by lw_segment_intersects
*/
enum CG_SEGMENT_INTERSECTION_TYPE {
    SEG_ERROR = -1,
    SEG_NO_INTERSECTION = 0,
    SEG_COLINEAR = 1,
    SEG_CROSS_LEFT = 2,
    SEG_CROSS_RIGHT = 3,
    SEG_TOUCH_LEFT = 4,
    SEG_TOUCH_RIGHT = 5
};

/*
* Do the segments intersect? How?
*/
int lw_segment_intersects(const POINT2D *p1, const POINT2D *p2, const POINT2D *q1, const POINT2D *q2);

/*
* Get/Set an enumeratoed ordinate. (x,y,z,m)
*/
double lwpoint_get_ordinate(const POINT4D *p, char ordinate);
void lwpoint_set_ordinate(POINT4D *p, char ordinate, double value);

/*
* Generate an interpolated coordinate p given an interpolation value and ordinate to apply it to
*/
int point_interpolate(const POINT4D *p1, const POINT4D *p2, POINT4D *p, int hasz, int hasm, char ordinate, double interpolation_value);


/*
* Geohash
*/
int lwgeom_geohash_precision(GBOX bbox, GBOX *bounds);
lwvarlena_t *geohash_point(double longitude, double latitude, int precision);
void decode_geohash_bbox(char *geohash, double *lat, double *lon, int precision);

/*
* Point comparisons
*/
int p4d_same(const POINT4D *p1, const POINT4D *p2);
int p3d_same(const POINT3D *p1, const POINT3D *p2);
int p2d_same(const POINT2D *p1, const POINT2D *p2);

/*
* Area calculations
*/
double lwpoly_area(const LWPOLY *poly);
double lwcurvepoly_area(const LWCURVEPOLY *curvepoly);
double lwtriangle_area(const LWTRIANGLE *triangle);

/**
* Pull a #GBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, return LW_FAILURE.
*/
int gserialized_read_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/*
 * Populate a bounding box *without* allocating an LWGEOM. Useful for some performance
 * purposes. Use only if gserialized_read_gbox_p failed
 */
int gserialized_peek_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Calculate required memory segment to contain a serialized form of the LWGEOM.
* Primarily used internally by the serialization code. Exposed to allow the cunit
* tests to exercise it.
*/
size_t gserialized_from_lwgeom_size(const LWGEOM *geom);

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
LWPOLY *lwcurvepoly_stroke(const LWCURVEPOLY *curvepoly, uint32_t perQuad);

/*
* Affine
*/
void ptarray_affine(POINTARRAY *pa, const AFFINE *affine);
void affine_invert(AFFINE *affine);

/*
* Scale
*/
void ptarray_scale(POINTARRAY *pa, const POINT4D *factor);

/*
* PointArray
*/
int ptarray_has_z(const POINTARRAY *pa);
int ptarray_has_m(const POINTARRAY *pa);
double ptarray_signed_area(const POINTARRAY *pa);

/*
* Length
*/
double ptarray_length(const POINTARRAY *pts);
double ptarray_arc_length_2d(const POINTARRAY *pts);

/*
* Clone support
*/
LWPOINT *lwpoint_clone(const LWPOINT *lwgeom);
LWLINE *lwline_clone(const LWLINE *lwgeom);
LWPOLY *lwpoly_clone(const LWPOLY *lwgeom);
LWTRIANGLE *lwtriangle_clone(const LWTRIANGLE *lwgeom);
LWCOLLECTION *lwcollection_clone(const LWCOLLECTION *lwgeom);
LWCIRCSTRING *lwcircstring_clone(const LWCIRCSTRING *curve);
POINTARRAY *ptarray_clone(const POINTARRAY *ptarray);
LWLINE *lwline_clone_deep(const LWLINE *lwgeom);
LWPOLY *lwpoly_clone_deep(const LWPOLY *lwgeom);
LWCOLLECTION *lwcollection_clone_deep(const LWCOLLECTION *lwgeom);
GBOX *gbox_clone(const GBOX *gbox);

/*
* Clockwise
*/
void lwpoly_force_clockwise(LWPOLY *poly);
void lwtriangle_force_clockwise(LWTRIANGLE *triangle);
int lwpoly_is_clockwise(LWPOLY *poly);
int lwtriangle_is_clockwise(LWTRIANGLE *triangle);
int ptarray_isccw(const POINTARRAY *pa);

/*
* Same
*/
char ptarray_same(const POINTARRAY *pa1, const POINTARRAY *pa2);
char lwpoint_same(const LWPOINT *p1, const LWPOINT *p2);
char lwline_same(const LWLINE *p1, const LWLINE *p2);
char lwpoly_same(const LWPOLY *p1, const LWPOLY *p2);
char lwtriangle_same(const LWTRIANGLE *p1, const LWTRIANGLE *p2);
char lwcollection_same(const LWCOLLECTION *p1, const LWCOLLECTION *p2);
char lwcircstring_same(const LWCIRCSTRING *p1, const LWCIRCSTRING *p2);

/*
* Shift
*/
void ptarray_longitude_shift(POINTARRAY *pa);

/*
* Support for in place modification of point arrays, fast
* function to move coordinate values around
*/
void ptarray_copy_point(POINTARRAY *pa, uint32_t from, uint32_t to);

/*
* Reverse
*/
void ptarray_reverse_in_place(POINTARRAY *pa);

/*
* Startpoint
*/
int lwpoly_startpoint(const LWPOLY* lwpoly, POINT4D* pt);
int ptarray_startpoint(const POINTARRAY* pa, POINT4D* pt);
int lwcollection_startpoint(const LWCOLLECTION* col, POINT4D* pt);

/*
 * Write into *ret the coordinates of the closest point on
 * segment A-B to the reference input point R
 */
void closest_point_on_segment(const POINT4D *R, const POINT4D *A, const POINT4D *B, POINT4D *ret);

/*
* Repeated points
*/
POINTARRAY *ptarray_remove_repeated_points(const POINTARRAY *in, double tolerance);
LWGEOM* lwline_remove_repeated_points(const LWLINE *in, double tolerance);
void ptarray_remove_repeated_points_in_place(POINTARRAY *pa, double tolerance, uint32_t min_points);

/*
* Closure test
*/
int lwline_is_closed(const LWLINE *line);
int lwpoly_is_closed(const LWPOLY *poly);
int lwcircstring_is_closed(const LWCIRCSTRING *curve);
int lwcompound_is_closed(const LWCOMPOUND *curve);
int lwpsurface_is_closed(const LWPSURFACE *psurface);
int lwtin_is_closed(const LWTIN *tin);

/**
* Snap to grid
*/
void ptarray_grid_in_place(POINTARRAY *pa, const gridspec *grid);

/*
* What side of the line formed by p1 and p2 does q fall?
* Returns -1 for left and 1 for right and 0 for co-linearity
*/
int lw_segment_side(const POINT2D *p1, const POINT2D *p2, const POINT2D *q);
int lw_arc_side(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, const POINT2D *Q);
int lw_arc_calculate_gbox_cartesian_2d(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, GBOX *gbox);
double lw_arc_center(const POINT2D *p1, const POINT2D *p2, const POINT2D *p3, POINT2D *result);
int lw_pt_in_seg(const POINT2D *P, const POINT2D *A1, const POINT2D *A2);
int lw_pt_in_arc(const POINT2D *P, const POINT2D *A1, const POINT2D *A2, const POINT2D *A3);
int lw_arc_is_pt(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3);
double lw_seg_length(const POINT2D *A1, const POINT2D *A2);
double lw_arc_length(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3);
int pt_in_ring_2d(const POINT2D *p, const POINTARRAY *ring);
int ptarray_contains_point(const POINTARRAY *pa, const POINT2D *pt);
int ptarrayarc_contains_point(const POINTARRAY *pa, const POINT2D *pt);
int ptarray_contains_point_partial(const POINTARRAY *pa, const POINT2D *pt, int check_closed, int *winding_number);
int ptarrayarc_contains_point_partial(const POINTARRAY *pa, const POINT2D *pt, int check_closed, int *winding_number);
int lwcompound_contains_point(const LWCOMPOUND *comp, const POINT2D *pt);
int lwgeom_contains_point(const LWGEOM *geom, const POINT2D *pt);

/**
* Split a line by a point and push components to the provided multiline.
* If the point doesn't split the line, push nothing to the container.
* Returns 0 if the point is off the line.
* Returns 1 if the point is on the line boundary (endpoints).
* Return 2 if the point is on the interior of the line (only case in which
* a split happens).
*
* NOTE: the components pushed to the output vector have their SRID stripped
*/
int lwline_split_by_point_to(const LWLINE* ln, const LWPOINT* pt, LWMLINE* to);

/** Ensure the collection can hold at least up to ngeoms geometries */
void lwcollection_reserve(LWCOLLECTION *col, uint32_t ngeoms);

/** Check if subtype is allowed in collectiontype */
int lwcollection_allows_subtype(int collectiontype, int subtype);

/** GBOX utility functions to figure out coverage/location on the globe */
double gbox_angular_height(const GBOX* gbox);
double gbox_angular_width(const GBOX* gbox);
int gbox_centroid(const GBOX* gbox, POINT2D* out);

/* Utilities */
int lwprint_double(double d, int maxdd, char *buf);
extern uint8_t MULTITYPE[NUMTYPES];

extern lwinterrupt_callback *_lwgeom_interrupt_callback;
extern int _lwgeom_interrupt_requested;
#define LW_ON_INTERRUPT(x) { \
  if ( _lwgeom_interrupt_callback ) { \
    (*_lwgeom_interrupt_callback)(); \
  } \
  if ( _lwgeom_interrupt_requested ) { \
    _lwgeom_interrupt_requested = 0; \
    lwnotice("liblwgeom code interrupted"); \
    x; \
  } \
}

int ptarray_npoints_in_rect(const POINTARRAY *pa, const GBOX *gbox);
int gbox_contains_point2d(const GBOX *g, const POINT2D *p);
int lwpoly_contains_point(const LWPOLY *poly, const POINT2D *pt);
POINT4D* lwmpoint_extract_points_4d(const LWMPOINT* g, uint32_t* npoints, int* input_empty);
char* lwstrdup(const char* a);

#endif /* _LIBLWGEOM_INTERNAL_H */
