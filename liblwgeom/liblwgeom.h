/**********************************************************************
 * $Id$
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

#ifndef _LIBLWGEOM_H
#define _LIBLWGEOM_H 1

#include "../postgis_config.h"
#include "stringbuffer.h"
#include <stdarg.h>
#include <stdio.h>

/**
* @file liblwgeom.h
*
* This library is the generic geometry handling section of PostGIS. The geometry
* objects, constructors, destructors, and a set of spatial processing functions,
* are implemented here.
*
* The library is designed for use in non-PostGIS applications if necessary. The
* units tests at cunit/cu_tester.c and the loader/dumper programs at
* ../loader/shp2pgsql.c are examples of non-PostGIS applications using liblwgeom.
*
* Programs using this library should set up the default memory managers and error
* handlers by implementing an lwgeom_init_allocators() function, which can be as
* a wrapper around the lwgeom_install_default_allocators() function if you want
* no special handling for memory management and error reporting.
*/

#define INTEGRITY_CHECKS 1

/*
** Floating point comparitors.
*/
#define FP_TOLERANCE 1e-12
#define FP_IS_ZERO(A) (fabs(A) <= FP_TOLERANCE)
#define FP_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define FP_MIN(A, B) (((A) < (B)) ? (A) : (B))
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
** Utility true and false defines.
*/
#define LW_TRUE 1
#define LW_FALSE 0

/*
* this will change to NaN when I figure out how to
* get NaN in a platform-independent way
*/
#define NO_VALUE 0.0
#define NO_Z_VALUE NO_VALUE
#define NO_M_VALUE NO_VALUE

#ifndef MAXFLOAT
#define MAXFLOAT      3.402823466e+38F
#endif

typedef unsigned char uchar;

#ifndef C_H
typedef unsigned int uint32;
typedef int int32;
#endif

/**
* Global functions for memory/logging handlers.
*/
typedef void* (*lwallocator)(size_t size);
typedef void* (*lwreallocator)(void *mem, size_t size);
typedef void (*lwfreeor)(void* mem);
typedef void (*lwreporter)(const char* fmt, va_list ap);
extern lwreallocator lwrealloc_var;
extern lwallocator lwalloc_var;
extern lwfreeor lwfree_var;
extern lwreporter lwerror_var;
extern lwreporter lwnotice_var;

/**
* Supply the memory management and error handling functions you want your
* application to use.
* @ingroup system
*/
extern void lwgeom_init_allocators(void);

/**
* Apply the default memory management (malloc() and free()) and error handlers.
* Called inside lwgeom_init_allocators() generally.
* @ingroup system
*/
extern void lwgeom_install_default_allocators(void);

/**
* Write a notice out to the notice handler. Uses standard printf() substitutions.
* Use for messages you always want output. For debugging, use LWDEBUG() or LWDEBUGF().
* @ingroup logging
*/
void lwnotice(const char *fmt, ...);

/**
* Write a notice out to the error handler. Uses standard printf() substitutions.
* Use for errors you always want output. For debugging, use LWDEBUG() or LWDEBUGF().
* @ingroup logging
*/
void lwerror(const char *fmt, ...);

/**
* The default memory/logging handlers installed by lwgeom_install_default_allocators()
*/
void *default_allocator(size_t size);
void *default_reallocator(void *mem, size_t size);
void default_freeor(void *ptr);
void default_errorreporter(const char *fmt, va_list ap);
void default_noticereporter(const char *fmt, va_list ap);

extern int lw_vasprintf (char **result, const char *format, va_list args);
extern int lw_asprintf
#if __STDC__
(char **result, const char *format, ...);
#else
(result, va_alist);
char **result;
va_dcl
#endif


/* Debug macros */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a notice at the given debug level */
#define LWDEBUG(level, msg) \
        do { \
                if (POSTGIS_DEBUG_LEVEL >= level) \
                        lwnotice("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__); \
        } while (0);

/* Display a formatted notice at the given debug level (like printf, with variadic arguments) */
#define LWDEBUGF(level, msg, ...) \
        do { \
                if (POSTGIS_DEBUG_LEVEL >= level) \
                        lwnotice("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } while (0);

#else

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define LWDEBUG(level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define LWDEBUGF(level, msg, ...) \
        ((void) 0)

#endif

/******************************************************************/


typedef struct
{
	float xmin;
	float ymin;
	float xmax;
	float ymax;
}
BOX2DFLOAT4;

typedef struct
{
	double xmin, ymin, zmin;
	double xmax, ymax, zmax;
}
BOX3D;

typedef struct chiptag
{
	int size; /* unused (for use by postgresql) */

	int endian_hint; /* the number 1 in the endian of this datastruct */

	BOX3D bvol;
	int SRID;
	char future[4];
	float factor;	/* Usually 1.0.
				 * Integer values are multiplied by this number
				 * to get the actual height value
				 * (for sub-meter accuracy height data).
				 */

	int datatype;	/* 1 = float32,
				 * 5 = 24bit integer,
				 * 6 = 16bit integer (short)
				 * 7 = 16bit ???
				 * 8 = 8bit ???
				 * 101 = float32 (NDR),
				 * 105 = 24bit integer (NDR),
				 * 106 = 16bit int (NDR)
				 * 107 = 16bit ??? (NDR)
				 * 108 = 8bit ??? (NDR) (this doesn't make sense)
				 */
	int height;
	int width;
	int compression;	/* 0 = no compression, 1 = differencer
					 * 0x80 = new value
					 * 0x7F = nodata
					 */

	/*
	 * this is provided for convenience, it should be set to
	 *  sizeof(chip) bytes into the struct because the serialized form is:
	 *    <header><data>
	 * NULL when serialized
	 */
	void  *data;	/* data[0] = bottm left,
				 * data[width] = 1st pixel, 2nd row (uncompressed)
				 */

}
CHIP;

/*
 * standard definition of an ellipsoid (what wkt calls a spheroid)
 *    f = (a-b)/a
 *    e_sq = (a*a - b*b)/(a*a)
 *    b = a - fa
 */
typedef struct
{
	double	a;	/* semimajor axis */
	double	b; 	/* semiminor axis */
	double	f;	/* flattening     */
	double	e;	/* eccentricity (first) */
	double	e_sq;	/* eccentricity (first), squared */
	double  radius; /* (2*a+b)/3 spherical average radius */
	char	name[20]; /* name of ellipse */
}
SPHEROID;


/*
 * ALL LWGEOM structures will use POINT3D as an abstract point.
 * This means a 2d geometry will be stored as (x,y) in its serialized
 * form, but all functions will work on (x,y,0).  This keeps all the
 * analysis functions simple.
 * NOTE: for GEOS integration, we'll probably set z=NaN
 *        so look out - z might be NaN for 2d geometries!
 */
typedef struct
{
	double	x,y,z;
}
POINT3DZ;
typedef struct
{
	double	x,y,z;
}
POINT3D; /* alias for POINT3DZ */
typedef struct
{
	double	x,y,m;
}
POINT3DM;


/*
 * type for 2d points.  When you convert this to 3d, the
 *   z component will be either 0 or NaN.
 */
typedef struct
{
	double x;
	double y;
}
POINT2D;

typedef struct
{
	double x;
	double y;
	double z;
	double m;
}
POINT4D;

/******************************************************************/

/*
 * Point array abstracts a lot of the complexity of points and point lists.
 * It handles miss-alignment in the serialized form, 2d/3d translation
 *    (2d points converted to 3d will have z=0 or NaN)
 * DONT MIX 2D and 3D POINTS!  *EVERYTHING* is either one or the other
 */
typedef struct
{
	/* array of POINT 2D, 3D or 4D. probably missaligned. */
	uchar *serialized_pointlist;

	/* use TYPE_* macros to handle */
	uchar  dims;

	int npoints;
	int maxpoints;
}
POINTARRAY;


/*
 * Use the following to build pointarrays
 * when number of points in output is not
 * known in advance
 */
typedef struct
{
	POINTARRAY *pa;
	size_t ptsize;
	size_t capacity; /* given in points */
}
DYNPTARRAY;

/* Create a new dynamic pointarray */
extern DYNPTARRAY *dynptarray_create(size_t initial_capacity, int dims);

/*
 * Add a POINT4D to the dynamic pointarray.
 *
 * The dynamic pointarray may be of any dimension, only
 * accepted dimensions will be copied.
 *
 * If allow_duplicates is set to 0 (false) a check
 * is performed to see if last point in array is equal to the
 * provided one. NOTE that the check is 4d based, with missing
 * ordinates in the pointarray set to NO_Z_VALUE and NO_M_VALUE
 * respectively.
 */
extern int dynptarray_addPoint4d(DYNPTARRAY *dpa, POINT4D *p4d,
                                 int allow_duplicates);

/******************************************************************
 *
 * LWGEOM (any type)
 *
 ******************************************************************/

typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID; /* -1 == unneeded */
	void *data;
}
LWGEOM;

/* POINTYPE */
typedef struct
{
	uchar type; /* POINTTYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	POINTARRAY *point;  /* hide 2d/3d (this will be an array of 1 point) */
}
LWPOINT; /* "light-weight point" */

/* LINETYPE */
typedef struct
{
	uchar type; /* LINETYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	POINTARRAY *points; /* array of POINT3D */
}
LWLINE; /* "light-weight line" */

/* POLYGONTYPE */
typedef struct
{
	uchar type; /* POLYGONTYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int nrings;
	int maxrings;
	POINTARRAY **rings; /* list of rings (list of points) */
}
LWPOLY; /* "light-weight polygon" */

/* MULTIPOINTTYPE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWPOINT **geoms;
}
LWMPOINT;

/* MULTILINETYPE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWLINE **geoms;
}
LWMLINE;

/* MULTIPOLYGONTYPE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWPOLY **geoms;
}
LWMPOLY;

/* COLLECTIONTYPE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWGEOM **geoms;
}
LWCOLLECTION;

/* CIRCSTRINGTYPE */
typedef struct
{
	uchar type; /* CIRCSTRINGTYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	POINTARRAY *points; /* array of POINT(3D/3DM) */
}
LWCIRCSTRING; /* "light-weight circularstring" */

/* COMPOUNDTYPE */
typedef struct
{
	uchar type; /* COMPOUNDTYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWGEOM **geoms;
}
LWCOMPOUND; /* "light-weight compound line" */

/* CURVEPOLYTYPE */
typedef struct
{
	uchar type; /* CURVEPOLYTYPE */
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int nrings;
	int maxrings;
	LWGEOM **rings; /* list of rings (list of points) */
}
LWCURVEPOLY; /* "light-weight polygon" */

/* MULTICURVE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWGEOM **geoms;
}
LWMCURVE;

/* MULTISURFACETYPE */
typedef struct
{
	uchar type;
	BOX2DFLOAT4 *bbox;
	uint32 SRID;
	int ngeoms;
	int maxgeoms;
	LWGEOM **geoms;
}
LWMSURFACE;

/* Casts LWGEOM->LW* (return NULL if cast is illegal) */
extern LWMPOLY *lwgeom_as_lwmpoly(const LWGEOM *lwgeom);
extern LWMLINE *lwgeom_as_lwmline(const LWGEOM *lwgeom);
extern LWMPOINT *lwgeom_as_lwmpoint(const LWGEOM *lwgeom);
extern LWCOLLECTION *lwgeom_as_lwcollection(const LWGEOM *lwgeom);
extern LWPOLY *lwgeom_as_lwpoly(const LWGEOM *lwgeom);
extern LWLINE *lwgeom_as_lwline(const LWGEOM *lwgeom);
extern LWPOINT *lwgeom_as_lwpoint(const LWGEOM *lwgeom);
extern LWCIRCSTRING *lwgeom_as_lwcircstring(const LWGEOM *lwgeom);
extern LWGEOM *lwgeom_as_multi(const LWGEOM *lwgeom);

/* Casts LW*->LWGEOM (always cast) */
extern LWGEOM *lwmpoly_as_lwgeom(const LWMPOLY *obj);
extern LWGEOM *lwmline_as_lwgeom(const LWMLINE *obj);
extern LWGEOM *lwmpoint_as_lwgeom(const LWMPOINT *obj);
extern LWGEOM *lwcollection_as_lwgeom(const LWCOLLECTION *obj);
extern LWGEOM *lwcircstring_as_lwgeom(const LWCIRCSTRING *obj);
extern LWGEOM *lwpoly_as_lwgeom(const LWPOLY *obj);
extern LWGEOM *lwline_as_lwgeom(const LWLINE *obj);
extern LWGEOM *lwpoint_as_lwgeom(const LWPOINT *obj);


extern LWCOLLECTION* lwcollection_add_lwgeom(LWCOLLECTION *col, const LWGEOM *geom);
extern LWMPOINT* lwmpoint_add_lwpoint(LWMPOINT *mobj, const LWPOINT *obj);
extern LWMLINE* lwmline_add_lwline(LWMLINE *mobj, const LWLINE *obj);
extern LWMPOLY* lwmpoly_add_lwpoly(LWMPOLY *mobj, const LWPOLY *obj);

/*
 * Call this function everytime LWGEOM coordinates
 * change so to invalidate bounding box
 */
extern void lwgeom_changed(LWGEOM *lwgeom);

/*
 * Call this function to drop BBOX and SRID
 * from LWGEOM. If LWGEOM type is *not* flagged
 * with the HASBBOX flag and has a bbox, it
 * will be released.
 */
extern void lwgeom_drop_bbox(LWGEOM *lwgeom);

/* Compute a bbox if not already computed */
extern void lwgeom_add_bbox(LWGEOM *lwgeom);

extern void lwgeom_dropSRID(LWGEOM *lwgeom);

/* Determine whether a LWGEOM can contain sub-geometries or not */
extern int lwgeom_is_collection(int type);

/******************************************************************/

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * will set point's m=0 (or NaN) if pa is 3d or 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT4D getPoint4d(const POINTARRAY *pa, int n);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * will set point's m=0 (or NaN) if pa is 3d or 2d
 * NOTE: this will modify the point4d pointed to by 'point'.
 */
extern int getPoint4d_p(const POINTARRAY *pa, int n, POINT4D *point);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT3DZ getPoint3dz(const POINTARRAY *pa, int n);
extern POINT3DM getPoint3dm(const POINTARRAY *pa, int n);

/*
 * copies a point from the point array into the parameter point
 * will set point's z=0 (or NaN) if pa is 2d
 * NOTE: this will modify the point3d pointed to by 'point'.
 */
extern int getPoint3dz_p(const POINTARRAY *pa, int n, POINT3DZ *point);
extern int getPoint3dm_p(const POINTARRAY *pa, int n, POINT3DM *point);


/*
 * copies a point from the point array into the parameter point
 * z value (if present is not returned)
 * NOTE: point is a real POINT3D *not* a pointer
 */
extern POINT2D getPoint2d(const POINTARRAY *pa, int n);

/*
 * copies a point from the point array into the parameter point
 * z value (if present is not returned)
 * NOTE: this will modify the point2d pointed to by 'point'.
 */
extern int getPoint2d_p(const POINTARRAY *pa, int n, POINT2D *point);

/*
 * set point N to the given value
 * NOTE that the pointarray can be of any
 * dimension, the appropriate ordinate values
 * will be extracted from it
 *
 */
extern void setPoint4d(POINTARRAY *pa, int n, POINT4D *p4d);

/*
 * get a pointer to nth point of a POINTARRAY
 * You'll need to cast it to appropriate dimensioned point.
 * Note that if you cast to a higher dimensional point you'll
 * possibly corrupt the POINTARRAY.
 *
 * WARNING: Don't cast this to a POINT !
 * it would not be reliable due to memory alignment constraints
 */
extern uchar *getPoint_internal(const POINTARRAY *pa, int n);

/* --- here is a macro equivalent, for speed... */
/* #define getPoint(x,n) &( (x)->serialized_pointlist[((x)->ndims*8)*(n)] ) */


/*
 * constructs a POINTARRAY.
 * NOTE: points is *not* copied, so be careful about modification
 * (can be aligned/missaligned)
 * NOTE: hasz and hasm are descriptive - it describes what type of data
 *	 'points' points to.  No data conversion is done.
 */
extern POINTARRAY *pointArray_construct(uchar *points, char hasz, char hasm,
                                        uint32 npoints);

/*
 * Calculate the (BOX3D) bounding box of a set of points.
 * Returns an alloced BOX3D or NULL (for empty geom) in the first form.
 * Write result in user-provided BOX3D in second form (return 0 if untouched).
 * If pa is 2d, then box3d's zmin/zmax will be set to NO_Z_VALUE
 */
extern BOX3D *ptarray_compute_box3d(const POINTARRAY *pa);
extern int ptarray_compute_box3d_p(const POINTARRAY *pa, BOX3D *out);



/*
 * size of point represeneted in the POINTARRAY
 * 16 for 2d, 24 for 3d, 32 for 4d
 */
extern int pointArray_ptsize(const POINTARRAY *pa);

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7
#define CIRCSTRINGTYPE    8
#define COMPOUNDTYPE      9
#define CURVEPOLYTYPE    13
#define MULTICURVETYPE   14
#define MULTISURFACETYPE 15

#define WKBZOFFSET 0x80000000
#define WKBMOFFSET 0x40000000
#define WKBSRIDFLAG 0x20000000
#define WKBBBOXFLAG 0x10000000

/* These macros work on PG_LWGEOM.type, LWGEOM.type and all its subclasses */

#define TYPE_SETTYPE(c,t) ((c)=(((c)&0xF0)|(t)))
#define TYPE_SETZM(t,z,m) ((t)=(((t)&0xCF)|((z)<<5)|((m)<<4)))
#define TYPE_SETHASBBOX(t,b) ((t)=(((t)&0x7F)|((b)<<7)))
#define TYPE_SETHASSRID(t,s) ((t)=(((t)&0xBF)|((s)<<6)))

#define TYPE_HASZ(t) ( ((t)&0x20)>>5 )
#define TYPE_HASM(t) ( ((t)&0x10)>>4 )
#define TYPE_HASBBOX(t) ( ((t)&0x80)>>7 )
#define TYPE_HASSRID(t) ( (((t)&0x40))>>6 )
#define TYPE_NDIMS(t) ((((t)&0x20)>>5)+(((t)&0x10)>>4)+2)
#define TYPE_GETTYPE(t) ((t)&0x0F)

/* 0x02==Z 0x01==M */
#define TYPE_GETZM(t) (((t)&0x30)>>4)

extern char lwgeom_hasBBOX(uchar type); /* true iff B bit set     */
extern int  lwgeom_ndims(uchar type);   /* returns 2,3 or 4       */
extern int  lwgeom_hasZ(uchar type);    /* has Z ?                */
extern int  lwgeom_hasM(uchar type);    /* has M ?                */
extern int  lwgeom_getType(uchar type); /* returns the tttt value */

extern uchar lwgeom_makeType(char hasZ, char hasM, char hasSRID, int type);
extern uchar lwgeom_makeType_full(char hasZ, char hasM, char hasSRID, int type, char hasBBOX);
extern char lwgeom_hasSRID(uchar type); /* true iff S bit is set */
extern char lwgeom_hasBBOX(uchar type); /* true iff B bit set    */



/*
 * This is the binary representation of lwgeom compatible
 * with postgresql varlena struct
 */
typedef struct
{
	uint32 size;        /* varlena header (do not touch directly!) */
	uchar type;         /* encodes ndims, type, bbox presence,
			                srid presence */
	uchar data[1];
}
PG_LWGEOM;

/*
 * Construct a full PG_LWGEOM type (including size header)
 * from a serialized form.
 * The constructed PG_LWGEOM object will be allocated using palloc
 * and the serialized form will be copied.
 * If you specify a SRID other then -1 it will be set.
 * If you request bbox (wantbbox=1) it will be extracted or computed
 * from the serialized form.
 */
extern PG_LWGEOM *PG_LWGEOM_construct(uchar *serialized, int SRID, int wantbbox);

/*
 * Compute bbox of serialized geom
 */
extern int compute_serialized_box2d_p(uchar *serialized_form, BOX2DFLOAT4 *box);
extern BOX3D *compute_serialized_box3d(uchar *serialized_form);
extern int compute_serialized_box3d_p(uchar *serialized_form, BOX3D *box);


/*
 * Evaluate with an heuristic if the provided PG_LWGEOM is worth
 * caching a bbox
 */
char is_worth_caching_pglwgeom_bbox(const PG_LWGEOM *);
char is_worth_caching_serialized_bbox(const uchar *);
char is_worth_caching_lwgeom_bbox(const LWGEOM *);


/*
 * This function computes the size in bytes
 * of the serialized geometries.
 */
extern size_t lwgeom_size(const uchar *serialized_form);
extern size_t lwgeom_size_subgeom(const uchar *serialized_form, int geom_number);
extern size_t lwgeom_size_line(const uchar *serialized_line);
extern size_t lwgeom_size_circstring(const uchar *serialized_curve);
extern size_t lwgeom_size_point(const uchar *serialized_point);
extern size_t lwgeom_size_poly(const uchar *serialized_line);


/*--------------------------------------------------------
 * all the base types (point/line/polygon) will have a
 * basic constructor, basic de-serializer, basic serializer,
 * bounding box finder and (TODO) serialized form size finder.
 *--------------------------------------------------------*/

/*
 * given the LWPOINT serialized form (or a pointer into a muli* one)
 * construct a proper LWPOINT.
 * serialized_form should point to the 8bit type format (with type = 1)
 * Returns NULL if serialized form is not a point.
 * See serialized form doc
 */
extern LWPOINT *lwpoint_deserialize(uchar *serialized_form);

/*
 * Find size this point would get when serialized (no BBOX)
 */
extern size_t lwpoint_serialize_size(LWPOINT *point);

/*
 * convert this point into its serialize form
 * result's first char will be the 8bit type.
 * See serialized form doc
 */
extern uchar *lwpoint_serialize(LWPOINT *point);

/* same as above, writes to buf */
extern void lwpoint_serialize_buf(LWPOINT *point, uchar *buf, size_t *size);

/*
 * find bounding box (standard one)
 * zmin=zmax=0 if 2d (might change to NaN)
 */
extern BOX3D *lwpoint_compute_box3d(LWPOINT *point);

/*
 * convenience functions to hide the POINTARRAY
 */
extern int lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out);
extern int lwpoint_getPoint3dz_p(const LWPOINT *point, POINT3DZ *out);
extern int lwpoint_getPoint3dm_p(const LWPOINT *point, POINT3DM *out);
extern int lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out);

/******************************************************************
 * LWLINE functions
 ******************************************************************/

/*
 * given the LWGEOM serialized form (or a pointer into a muli* one)
 * construct a proper LWLINE.
 * serialized_form should point to the 8bit type format (with type = 2)
 * See SERIALIZED_FORM doc
 */
extern LWLINE *lwline_deserialize(uchar *serialized_form);

/* find the size this line would get when serialized */
extern size_t lwline_serialize_size(LWLINE *line);

/*
 * convert this line into its serialize form
 * result's first char will be the 8bit type.  See serialized form doc
 * copies data.
 */
extern uchar *lwline_serialize(LWLINE *line);

/* same as above, writes to buf */
extern void lwline_serialize_buf(LWLINE *line, uchar *buf, size_t *size);

/*
 * find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
 */
extern BOX3D *lwline_compute_box3d(LWLINE *line);

/******************************************************************
 * LWPOLY functions
 ******************************************************************/

/*
 * given the LWPOLY serialized form (or a pointer into a muli* one)
 * construct a proper LWPOLY.
 * serialized_form should point to the 8bit type format (with type = 3)
 * See SERIALIZED_FORM doc
 */
extern LWPOLY *lwpoly_deserialize(uchar *serialized_form);

/* find the size this polygon would get when serialized */
extern size_t lwpoly_serialize_size(LWPOLY *poly);

/*
 * create the serialized form of the polygon
 * result's first char will be the 8bit type.  See serialized form doc
 * points copied
 */
extern uchar *lwpoly_serialize(LWPOLY *poly);

/* same as above, writes to buf */
extern void lwpoly_serialize_buf(LWPOLY *poly, uchar *buf, size_t *size);

/*
 * find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
 */
extern BOX3D *lwpoly_compute_box3d(LWPOLY *poly);

/******************************************************************
 * LWCIRCSTRING functions
 ******************************************************************/

/*
 * given the LWGEOM serialized form (or a pointer into a muli* one)
 * construct a proper LWCIRCSTRING.
 * serialized_form should point to the 8bit type format (with type = 2)
 * See SERIALIZED_FORM doc
 */
extern LWCIRCSTRING *lwcircstring_deserialize(uchar *serialized_form);

/* find the size this curve would get when serialized */
extern size_t lwcircstring_serialize_size(LWCIRCSTRING *curve);

/*
 * convert this circularstring into its serialize form
 * result's first char will be the 8bit type.  See serialized form doc
 * copies data.
 */
extern uchar *lwcircstring_serialize(LWCIRCSTRING *curve);

/* same as above, writes to buf */
extern void lwcircstring_serialize_buf(LWCIRCSTRING *curve, uchar *buf, size_t *size);

/*
 * find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
 */
extern BOX3D *lwcircstring_compute_box3d(const LWCIRCSTRING *curve);



/******************************************************************
 * LWGEOM functions
 ******************************************************************/

extern size_t lwgeom_serialize_size(LWGEOM *geom);
extern size_t lwcollection_serialize_size(LWCOLLECTION *coll);
extern void lwgeom_serialize_buf(LWGEOM *geom, uchar *buf, size_t *size);
extern uchar *lwgeom_serialize(LWGEOM *geom);
extern void lwcollection_serialize_buf(LWCOLLECTION *mcoll, uchar *buf, size_t *size);
extern int lwcollection_ngeoms(const LWCOLLECTION *col);

/* Given a generic geometry/collection, return the "simplest" form. */
extern LWGEOM *lwgeom_homogenize(const LWGEOM *geom);
extern LWGEOM *lwcollection_homogenize(const LWCOLLECTION *col);

/*
 * Deserialize an lwgeom serialized form.
 * The deserialized (recursive) structure will store
 * pointers to the serialized form (POINTARRAYs).
 */
LWGEOM *lwgeom_deserialize(uchar *serializedform);
BOX3D *lwgeom_compute_box3d(const LWGEOM *geom);


/******************************************************************
 * LWMULTIx and LWCOLLECTION functions
 ******************************************************************/

LWMPOINT *lwmpoint_deserialize(uchar *serializedform);
LWMLINE *lwmline_deserialize(uchar *serializedform);
LWMPOLY *lwmpoly_deserialize(uchar *serializedform);
LWCOLLECTION *lwcollection_deserialize(uchar *serializedform);
LWCOMPOUND *lwcompound_deserialize(uchar *serialized_form);
LWCURVEPOLY *lwcurvepoly_deserialize(uchar *serialized_form);
LWMCURVE *lwmcurve_deserialize(uchar *serialized_form);
LWMSURFACE *lwmsurface_deserialize(uchar *serialized_form);

LWGEOM *lwcollection_getsubgeom(LWCOLLECTION *col, int gnum);
BOX3D *lwcollection_compute_box3d(LWCOLLECTION *col);
LWCOLLECTION* lwcollection_extract(LWCOLLECTION *col, int type);

/******************************************************************
 * SERIALIZED FORM functions
 ******************************************************************/


/******************************************************************
 * Multi-geometries
 *
 * These are all handled equivelently so its easy to write iterator code.
 *  NOTE NOTE: you can hand in a non-multigeometry to most of these functions
 *             and get usual behavior (ie. get geometry 0 on a POINT
 *	       will return the point).
 *             This makes coding even easier since you dont have to necessarily
 *             differenciate between the multi* and non-multi geometries.
 *
 * NOTE: these usually work directly off the serialized form, so
 *	they're a little more difficult to handle (and slower)
 * NOTE NOTE: the get functions maybe slow, so we may want to have an
 *            "analysed" lwgeom that would just have pointer to the start
 *            of each sub-geometry.
 *
 ******************************************************************/

/* use this version for speed.  READ-ONLY! */
typedef struct
{
	int   SRID;
	const uchar *serialized_form; /* orginal structure */
	uchar  type;        /* 8-bit type for the LWGEOM */
	int ngeometries;    /* number of sub-geometries */
	uchar **sub_geoms;  /* list of pointers (into serialized_form)
				       of the sub-geoms */
}
LWGEOM_INSPECTED;

extern int lwgeom_size_inspected(const LWGEOM_INSPECTED *inspected, int geom_number);

/*
 * note - for a simple type (ie. point), this will have
 * sub_geom[0] = serialized_form.
 * for multi-geomtries sub_geom[0] will be a few bytes into the
 * serialized form.
 * This function just computes the length of each sub-object and
 * pre-caches this info.
 * For a geometry collection of multi* geometries, you can inspect
 * the sub-components as well.
 */
extern LWGEOM_INSPECTED *lwgeom_inspect(const uchar *serialized_form);


/*
 * 1st geometry has geom_number = 0
 * if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a point (with geom_num=0), multipoint
 * or geometrycollection
 */
extern LWPOINT *lwgeom_getpoint(uchar *serialized_form, int geom_number);
extern LWPOINT *lwgeom_getpoint_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a LINE, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a line, multiline or geometrycollection
 */
extern LWLINE *lwgeom_getline(uchar *serialized_form, int geom_number);
extern LWLINE *lwgeom_getline_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a polygon, multipolygon or geometrycollection
 */
extern LWPOLY *lwgeom_getpoly(uchar *serialized_form, int geom_number);
extern LWPOLY *lwgeom_getpoly_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a polygon, multipolygon or geometrycollection
 */
extern LWCIRCSTRING *lwgeom_getcircstring_inspected(LWGEOM_INSPECTED *inspected, int geom_number);

extern LWGEOM *lwgeom_getgeom_inspected(LWGEOM_INSPECTED *inspected, int geom_number);



/*
 * this gets the serialized form of a sub-geometry
 * 1st geometry has geom_number = 0
 * if this isnt a multi* geometry, and geom_number ==0 then it returns
 * itself
 * returns null on problems.
 * in the future this is how you would access a muli* portion of a
 * geometry collection.
 *    GEOMETRYCOLLECTION(MULTIPOINT(0 0, 1 1), LINESTRING(0 0, 1 1))
 *   ie. lwgeom_getpoint( lwgeom_getsubgeometry( serialized, 0), 1)
 *           --> POINT(1 1)
 * you can inspect the sub-geometry as well if you wish.
 */
extern uchar *lwgeom_getsubgeometry(const uchar *serialized_form, int geom_number);
extern uchar *lwgeom_getsubgeometry_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


/*
 * 1st geometry has geom_number = 0
 *  use geom_number = -1 to find the actual type of the serialized form.
 *    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
 *                 --> multipoint
 *   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
 *                 --> point
 * gets the 8bit type of the geometry at location geom_number
 */
extern uchar lwgeom_getsubtype(uchar *serialized_form, int geom_number);
extern uchar lwgeom_getsubtype_inspected(LWGEOM_INSPECTED *inspected, int geom_number);


/*
 * how many sub-geometries are there?
 *  for point,line,polygon will return 1.
 */
extern int lwgeom_getnumgeometries(uchar *serialized_form);
extern int lwgeom_getnumgeometries_inspected(LWGEOM_INSPECTED *inspected);



/*
 * set finalType to COLLECTIONTYPE or 0 (0 means choose a best type)
 *   (ie. give it 2 points and ask it to be a multipoint)
 *  use SRID=-1 for unknown SRID  (will have 8bit type's S = 0)
 * all subgeometries must have the same SRID
 * if you want to construct an inspected, call this then inspect the result...
 */
extern uchar *lwgeom_serialized_construct(int SRID, int finalType, char hasz, char hasm, int nsubgeometries, uchar **serialized_subs);


/* construct the empty geometry (GEOMETRYCOLLECTION(EMPTY)) */
extern uchar *lwgeom_constructempty(int SRID, char hasz, char hasm);
extern void lwgeom_constructempty_buf(int SRID, char hasz, char hasm, uchar *buf, size_t *size);
size_t lwgeom_empty_length(int SRID);

/*
 * get the SRID from the LWGEOM
 * none present => -1
 */
extern int lwgeom_getsrid(uchar *serialized);


/*------------------------------------------------------
 * other stuff
 *
 * handle the double-to-float conversion.  The results of this
 * will usually be a slightly bigger box because of the difference
 * between float8 and float4 representations.
 */

extern BOX2DFLOAT4 *box3d_to_box2df(BOX3D *box);
extern int box3d_to_box2df_p(BOX3D *box, BOX2DFLOAT4 *res);

extern BOX3D box2df_to_box3d(BOX2DFLOAT4 *box);
extern void box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *box3d);

extern BOX3D *box3d_union(BOX3D *b1, BOX3D *b2);
extern int box3d_union_p(BOX3D *b1, BOX3D *b2, BOX3D *ubox);

/*
 * Returns a pointer to the BBOX internal to the serialized form.
 * READ-ONLY!
 * Or NULL if serialized form does not have a BBOX
 * OBSOLETED to avoid memory alignment problems.
 */
/*extern BOX2DFLOAT4 *getbox2d_internal(uchar *serialized_form);*/

/*
 * this function writes to 'box' and returns 0 if serialized_form
 * does not have a bounding box (empty geom)
 */
extern int getbox2d_p(uchar *serialized_form, BOX2DFLOAT4 *box);

/* Expand given box of 'd' units in all directions */
void expand_box2d(BOX2DFLOAT4 *box, double d);
void expand_box3d(BOX3D *box, double d);

/* Check if to boxes are equal (considering FLOAT approximations) */
char box2d_same(BOX2DFLOAT4 *box1, BOX2DFLOAT4 *box2);



/****************************************************************
 * MEMORY MANAGEMENT
 ****************************************************************/

/*
 * The lwfree_* family of functions frees *all* memory associated
 * with the pointer, including the serialized__pointlist in the
 * point arrays. Do not use these on LWGEOMs de-serialized from
 * PG_LWGEOMs or they will try to free an underlying structure
 * managed by PgSQL. Only use these on LWGEOMs you have
 * constructed yourself.
 */

extern void ptarray_free(POINTARRAY *pa);
extern void lwpoint_free(LWPOINT *pt);
extern void lwline_free(LWLINE *line);
extern void lwpoly_free(LWPOLY *poly);
extern void lwmpoint_free(LWMPOINT *mpt);
extern void lwmline_free(LWMLINE *mline);
extern void lwmpoly_free(LWMPOLY *mpoly);
extern void lwcollection_free(LWCOLLECTION *col);
extern void lwcircstring_free(LWCIRCSTRING *curve);
extern void lwgeom_free(LWGEOM *geom);

extern void lwinspected_release(LWGEOM_INSPECTED *inspected); /* TODO: make this deep free... */

/*
 * The *_release family of functions frees the LWGEOM structures
 * surrounding the POINTARRAYs but leaves the POINTARRAYs
 * intact. Use these on LWGEOMs that have been de-serialized
 * from PG_LWGEOMs. Do not use these on LWGEOMs you have
 * constructed yourself, or you will leak lots of memory.
 */

extern void lwpoint_release(LWPOINT *lwpoint);
extern void lwline_release(LWLINE *lwline);
extern void lwpoly_release(LWPOLY *lwpoly);
extern void lwcircstring_release(LWCIRCSTRING *lwcirc);
extern void lwmpoint_release(LWMPOINT *lwpoint);
extern void lwmline_release(LWMLINE *lwline);
extern void lwmpoly_release(LWMPOLY *lwpoly);
extern void lwcollection_release(LWCOLLECTION *lwcollection);
extern void lwgeom_release(LWGEOM *lwgeom);


/****************************************************************
 * utility
 ****************************************************************/

extern uint32 lw_get_uint32(const uchar *loc);
extern int32 lw_get_int32(const uchar *loc);
extern void printBOX3D(BOX3D *b);
extern void printPA(POINTARRAY *pa);
extern void printLWPOINT(LWPOINT *point);
extern void printLWLINE(LWLINE *line);
extern void printLWPOLY(LWPOLY *poly);
extern void printBYTES(uchar *a, int n);
extern void printMULTI(uchar *serialized);
extern void printType(uchar str);


extern float LWGEOM_Minf(float a, float b);
extern float LWGEOM_Maxf(float a, float b);
extern double LWGEOM_Mind(double a, double b);
extern double LWGEOM_Maxd(double a, double b);

extern float  next_float_down(double d);
extern float  next_float_up(double d);
extern double next_double_down(float d);
extern double next_double_up(float d);


#define LW_MAX(a,b) ((a) >	(b) ? (a) : (b))
#define LW_MIN(a,b) ((a) <= (b) ? (a) : (b))
#define LW_ABS(a)   ((a) <	(0) ? -(a) : (a))

/* for the measure functions*/
#define DIST2D_MAX		-1
#define DIST2D_MIN		1

/* general utilities */
extern double distance2d_pt_pt(const POINT2D *p1, const POINT2D *p2);
extern double distance2d_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B);
extern LWGEOM *lw_dist2d_distancepoint(uchar *lw1, uchar *lw2,int srid,int mode);
extern LWGEOM *lw_dist2d_distanceline(uchar *lw1, uchar *lw2,int srid,int mode);
extern double lwgeom_mindistance2d(uchar *lw1, uchar *lw2);
extern double lwgeom_mindistance2d_tolerance(uchar *lw1, uchar *lw2, double tolerance);
extern double lwgeom_maxdistance2d(uchar *lw1, uchar *lw2);
extern double lwgeom_maxdistance2d_tolerance(uchar *lw1, uchar *lw2, double tolerance);
extern double lwgeom_polygon_area(const LWPOLY *poly);
extern double lwgeom_polygon_perimeter(const LWPOLY *poly);
extern double lwgeom_polygon_perimeter2d(const LWPOLY *poly);
extern double lwgeom_pointarray_length2d(const POINTARRAY *pts);
extern double lwgeom_pointarray_length(const POINTARRAY *pts);
extern void lwgeom_force2d_recursive(uchar *serialized, uchar *optr, size_t *retsize);
extern void lwgeom_force3dz_recursive(uchar *serialized, uchar *optr, size_t *retsize);
extern void lwgeom_force3dm_recursive(uchar *serialized, uchar *optr, size_t *retsize);
extern void lwgeom_force4d_recursive(uchar *serialized, uchar *optr, size_t *retsize);
extern int pt_in_ring_2d(const POINT2D *p, const POINTARRAY *ring);
extern int pt_in_poly_2d(const POINT2D *p, const LWPOLY *poly);
extern int azimuth_pt_pt(const POINT2D *p1, const POINT2D *p2, double *ret);
extern int lwgeom_pt_inside_circle(POINT2D *p, double cx, double cy, double rad);
extern char ptarray_isccw(const POINTARRAY *pa);
extern void lwgeom_reverse(LWGEOM *lwgeom);
extern void lwline_reverse(LWLINE *line);
extern void lwpoly_reverse(LWPOLY *poly);
extern void lwpoly_forceRHR(LWPOLY *poly);
extern void lwgeom_force_rhr(LWGEOM *lwgeom);
extern char* lwgeom_summary(const LWGEOM *lwgeom, int offset);
extern const char *lwtype_name(int type);
extern int ptarray_compute_box2d_p(const POINTARRAY *pa, BOX2DFLOAT4 *result);
extern BOX2DFLOAT4 *ptarray_compute_box2d(const POINTARRAY *pa);
extern int lwpoint_compute_box2d_p(const LWPOINT *point, BOX2DFLOAT4 *box);
extern int lwline_compute_box2d_p(const LWLINE *line, BOX2DFLOAT4 *box);
extern int lwpoly_compute_box2d_p(const LWPOLY *poly, BOX2DFLOAT4 *box);
extern int lwcollection_compute_box2d_p(const LWCOLLECTION *col, BOX2DFLOAT4 *box);
extern int lwcircstring_compute_box2d_p(const LWCIRCSTRING *curve, BOX2DFLOAT4 *box);
extern BOX2DFLOAT4* lwgeom_compute_box2d(const LWGEOM *lwgeom);
extern char* lwpoint_to_latlon(const LWPOINT *p, const char *format);

extern void interpolate_point4d(POINT4D *A, POINT4D *B, POINT4D *I, double F);

/* return alloced memory */
extern BOX2DFLOAT4 *box2d_union(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2);

/* args may overlap ! */
extern int box2d_union_p(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2, BOX2DFLOAT4 *ubox);
extern int lwgeom_compute_box2d_p(const LWGEOM *lwgeom, BOX2DFLOAT4 *box);
void lwgeom_longitude_shift(LWGEOM *lwgeom);


/**
* @brief Check whether or not a lwgeom is big enough to warrant a bounding box.
*
* Check whether or not a lwgeom is big enough to warrant a bounding box
* when stored in the serialized form on disk. Currently only points are
* considered small enough to not require a bounding box, because the
* index operations can generate a large number of box-retrieval operations
* when scanning keys.
*/
extern int lwgeom_needs_bbox(const LWGEOM *geom);

/**
* Count the total number of vertices in any #LWGEOM.
*/
extern int lwgeom_count_vertices(const LWGEOM *geom);
extern int lwgeom_npoints(uchar *serialized);

/**
* Return true or false depending on whether a geometry has
* a valid SRID set.
*/
extern int lwgeom_has_srid(const LWGEOM *geom);

/**
* Return true of false depending on whether a geometry is an "empty"
* geometry (no vertices members)
*/
extern int lwgeom_is_empty(const LWGEOM *geom);

/**
* Return the dimensionality (relating to point/line/poly) of an lwgeom
*/
extern int lwgeom_dimensionality(LWGEOM *geom);

/* Is lwgeom1 geometrically equal to lwgeom2 ? */
char lwgeom_same(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2);
char ptarray_same(const POINTARRAY *pa1, const POINTARRAY *pa2);
char lwpoint_same(const LWPOINT *p1, const LWPOINT *p2);
char lwline_same(const LWLINE *p1, const LWLINE *p2);
char lwpoly_same(const LWPOLY *p1, const LWPOLY *p2);
char lwcollection_same(const LWCOLLECTION *p1, const LWCOLLECTION *p2);


/*
 * Clone an LWGEOM
 * pointarray are not copied.
 * BBOXes are copied
 */
extern LWGEOM *lwgeom_clone(const LWGEOM *lwgeom);
extern LWPOINT *lwpoint_clone(const LWPOINT *lwgeom);
extern LWLINE *lwline_clone(const LWLINE *lwgeom);
extern LWPOLY *lwpoly_clone(const LWPOLY *lwgeom);
extern LWCOLLECTION *lwcollection_clone(const LWCOLLECTION *lwgeom);
extern LWCIRCSTRING *lwcircstring_clone(const LWCIRCSTRING *curve);
extern BOX2DFLOAT4 *box2d_clone(const BOX2DFLOAT4 *lwgeom);
extern POINTARRAY *ptarray_clone(const POINTARRAY *ptarray);

/*
* Geometry constructors. These constructors to not copy the point arrays
* passed to them, they just take references, so do not free them out
* from underneath the geometries.
*/
extern LWPOINT *lwpoint_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *point);
extern LWLINE *lwline_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *points);
extern LWCIRCSTRING *lwcircstring_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *points);
extern LWPOLY *lwpoly_construct(int SRID, BOX2DFLOAT4 *bbox, uint32 nrings, POINTARRAY **points);
extern LWCOLLECTION *lwcollection_construct(uint32 type, int SRID, BOX2DFLOAT4 *bbox, uint32 ngeoms, LWGEOM **geoms);

/*
* Empty geometry constructors.
*/
extern LWPOINT* lwpoint_construct_empty(int srid, char hasz, char hasm);
extern LWLINE* lwline_construct_empty(int srid, char hasz, char hasm);
extern LWPOLY* lwpoly_construct_empty(int srid, char hasz, char hasm);
extern LWMPOINT* lwmpoint_construct_empty(int srid, char hasz, char hasm);
extern LWMLINE* lwmline_construct_empty(int srid, char hasz, char hasm);
extern LWMPOLY* lwmpoly_construct_empty(int srid, char hasz, char hasm);
extern LWCOLLECTION* lwcollection_construct_empty(int srid, char hasz, char hasm);


/* Other constructors */
extern LWPOINT *make_lwpoint2d(int SRID, double x, double y);
extern LWPOINT *make_lwpoint3dz(int SRID, double x, double y, double z);
extern LWPOINT *make_lwpoint3dm(int SRID, double x, double y, double m);
extern LWPOINT *make_lwpoint4d(int SRID, double x, double y, double z, double m);
extern LWLINE *lwline_from_lwpointarray(int SRID, uint32 npoints, LWPOINT **points);
extern LWLINE *lwline_from_lwmpoint(int SRID, LWMPOINT *mpoint);
extern LWLINE *lwline_addpoint(LWLINE *line, LWPOINT *point, uint32 where);
extern LWLINE *lwline_removepoint(LWLINE *line, uint32 which);
extern void lwline_setPoint4d(LWLINE *line, uint32 which, POINT4D *newpoint);
extern LWPOLY *lwpoly_from_lwlines(const LWLINE *shell, uint32 nholes, const LWLINE **holes);

/* Return a char string with ASCII versionf of type flags */
extern const char *lwgeom_typeflags(uchar type);

/* Construct an empty pointarray */
extern POINTARRAY* ptarray_construct(char hasz, char hasm, uint32 npoints);

/* Construct a pointarray, *copying* in the data from ptlist */
extern POINTARRAY* ptarray_construct_copy_data(char hasz, char hasm, uint32 npoints, uchar *ptlist);

/*
 * extern POINTARRAY *ptarray_construct2d(uint32 npoints, const POINT2D *pts);
 * extern POINTARRAY *ptarray_construct3dz(uint32 npoints, const POINT3DZ *pts);
 * extern POINTARRAY *ptarray_construct3dm(uint32 npoints, const POINT3DM *pts);
 * extern POINTARRAY *ptarray_construct4d(uint32 npoints, const POINT4D *pts);
 */

extern POINTARRAY *ptarray_addPoint(const POINTARRAY *pa, uchar *p, size_t pdims,
                                    uint32 where);
extern POINTARRAY *ptarray_removePoint(POINTARRAY *pa, uint32 where);
extern POINTARRAY *ptarray_merge(POINTARRAY *pa1, POINTARRAY *pa2);

extern int ptarray_isclosed2d(const POINTARRAY *pa);
extern int ptarray_isclosed3d(const POINTARRAY *pa);

extern void ptarray_longitude_shift(POINTARRAY *pa);

extern int32 lwgeom_nrings_recursive(uchar *serialized);
extern void ptarray_reverse(POINTARRAY *pa);
extern POINTARRAY* ptarray_flip_coordinates(POINTARRAY *pa);

extern POINTARRAY *ptarray_substring(POINTARRAY *, double, double);

/*
 * Given a point, returns the location of closest point on pointarray
 * as a fraction of total length (0: first point -- 1: last point).
 *
 * If not-null, the third argument will be set to the actual distance
 * of the point from the pointarray.
 */
extern double ptarray_locate_point(POINTARRAY *, POINT2D *, double *);

/*
 * Write into *ret the coordinates of the closest point on
 * segment A-B to the reference input point R
 */
extern void closest_point_on_segment(POINT2D *R, POINT2D *A, POINT2D *B, POINT2D *ret);

extern LWLINE *lwline_measured_from_lwline(const LWLINE *lwline, double m_start, double m_end);
extern LWMLINE* lwmline_measured_from_lwmline(const LWMLINE *lwmline, double m_start, double m_end);

/*
 * Ensure every segment is at most 'dist' long.
 * Returned LWGEOM might is unchanged if a POINT.
 */
extern LWGEOM *lwgeom_segmentize2d(LWGEOM *line, double dist);
extern POINTARRAY *ptarray_segmentize2d(const POINTARRAY *ipa, double dist);
extern LWLINE *lwline_segmentize2d(LWLINE *line, double dist);
extern LWPOLY *lwpoly_segmentize2d(LWPOLY *line, double dist);
extern LWCOLLECTION *lwcollection_segmentize2d(LWCOLLECTION *coll, double dist);


/*
 * Export functions
 */
#define OUT_MAX_DOUBLE 1E15
#define OUT_SHOW_DIGS_DOUBLE 20
#define OUT_MAX_DOUBLE_PRECISION 15
#define OUT_MAX_DIGS_DOUBLE (OUT_SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */

extern char* lwgeom_to_gml2(uchar *geom, char *srs, int precision, const char *prefix);
extern char* lwgeom_to_gml3(uchar *geom, char *srs, int precision, int is_deegree, int is_dims, const char *prefix);
extern char* lwgeom_to_kml2(uchar *geom, int precision, const char *prefix);
extern char* lwgeom_to_geojson(uchar *geom, char *srs, int precision, int has_bbox);
extern char* lwgeom_to_svg(uchar *geom, int precision, int relative);


extern POINTARRAY *ptarray_remove_repeated_points(POINTARRAY *in);
extern LWGEOM* lwgeom_remove_repeated_points(LWGEOM *in);
extern LWGEOM* lwmpoint_remove_repeated_points(LWMPOINT *in);
extern LWGEOM* lwline_remove_repeated_points(LWLINE *in);
extern LWGEOM* lwcollection_remove_repeated_points(LWCOLLECTION *in);
extern LWGEOM* lwpoly_remove_repeated_points(LWPOLY *in);

extern LWGEOM* lwgeom_flip_coordinates(LWGEOM *in);

extern uchar parse_hex(char *str);
extern void deparse_hex(uchar str, char *result);

/*
** New parsing and unparsing functions.
*/
extern char* lwgeom_to_wkt(const LWGEOM *geom, uchar variant, int precision, size_t *size_out);
extern char* lwgeom_to_wkb(const LWGEOM *geom, uchar variant, size_t *size_out);


/* Parser check flags */
#define PARSER_CHECK_MINPOINTS  1
#define PARSER_CHECK_ODD        2
#define PARSER_CHECK_CLOSURE    4

#define PARSER_CHECK_NONE   0
#define PARSER_CHECK_ALL	(PARSER_CHECK_MINPOINTS | PARSER_CHECK_ODD | PARSER_CHECK_CLOSURE)

/*
 * Parser result structure: returns the result of attempting to convert (E)WKT/(E)WKB to LWGEOM
 */
typedef struct struct_lwgeom_parser_result
{
	const char *wkinput;		/* Copy of pointer to input WKT/WKB */
	uchar *serialized_lwgeom;	/* Pointer to serialized LWGEOM */
	int size;			/* Size of serialized LWGEOM in bytes */
	const char *message;		/* Error/warning message */
	int errlocation;		/* Location of error */
}
LWGEOM_PARSER_RESULT;

/*
 * Parser error messages (these must match the message array in lwgparse.c)
 */
#define PARSER_ERROR_MOREPOINTS     1
#define PARSER_ERROR_ODDPOINTS      2
#define PARSER_ERROR_UNCLOSED       3
#define PARSER_ERROR_MIXDIMS        4
#define PARSER_ERROR_INVALIDGEOM    5
#define PARSER_ERROR_INVALIDWKBTYPE 6
#define PARSER_ERROR_INCONTINUOUS	7


/*
 * Unparser result structure: returns the result of attempting to convert LWGEOM to (E)WKT/(E)WKB
 */
typedef struct struct_lwgeom_unparser_result
{
	uchar *serialized_lwgeom;	/* Copy of pointer to input serialized LWGEOM */
	char *wkoutput;			/* Pointer to WKT or WKB output */
	int size;			/* Size of serialized LWGEOM in bytes */
	const char *message;		/* Error/warning message */
	int errlocation;		/* Location of error */
}
LWGEOM_UNPARSER_RESULT;

/*
 * Unparser error messages (these must match the message array in lwgunparse.c)
 */
#define UNPARSER_ERROR_MOREPOINTS 	1
#define UNPARSER_ERROR_ODDPOINTS	2
#define UNPARSER_ERROR_UNCLOSED		3


/* Parser access routines */
extern char *lwgeom_to_ewkt(LWGEOM *lwgeom, int flags);
extern char *lwgeom_to_hexwkb(LWGEOM *lwgeom, int flags, uint32 byteorder);
extern LWGEOM *lwgeom_from_ewkb(uchar *ewkb, int flags, size_t ewkblen);
extern LWGEOM *lwgeom_from_ewkt(char *ewkt, int flags);
extern uchar *lwgeom_to_ewkb(LWGEOM *lwgeom, int flags, char byteorder, size_t *ewkblen);

extern int serialized_lwgeom_to_ewkt(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags);
extern int serialized_lwgeom_from_ewkt(LWGEOM_PARSER_RESULT *lwg_parser_result, char *wkt_input, int flags);
extern int serialized_lwgeom_to_hexwkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, uint32 byteorder);
extern int serialized_lwgeom_from_hexwkb(LWGEOM_PARSER_RESULT *lwg_parser_result, char *hexwkb_input, int flags);
extern int serialized_lwgeom_to_ewkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, uint32 byteorder);

extern void *lwalloc(size_t size);
extern void *lwrealloc(void *mem, size_t size);
extern void lwfree(void *mem);

/* Utilities */
extern void trim_trailing_zeros(char *num);
extern char *lwmessage_truncate(char *str, int startpos, int endpos, int maxlength, int truncdirection);

/* Machine endianness */
#define XDR 0
#define NDR 1
extern char getMachineEndian(void);

void errorIfSRIDMismatch(int srid1, int srid2);


/*******************************************************************************
 * SQLMM internal functions - TODO: Move into separate header files
 ******************************************************************************/

int has_arc(const LWGEOM *geom);
double lwcircle_center(POINT4D *p1, POINT4D *p2, POINT4D *p3, POINT4D **result);
LWGEOM *lwgeom_segmentize(LWGEOM *geom, uint32 perQuad);
LWGEOM *lwgeom_desegmentize(LWGEOM *geom);
extern double lwgeom_curvepolygon_area(LWCURVEPOLY *curvepoly);
double lwcircle_center(POINT4D *p1, POINT4D *p2, POINT4D *p3, POINT4D **result);

#endif /* !defined _LIBLWGEOM_H  */

