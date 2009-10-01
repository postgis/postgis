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

#include "liblwgeom.h"
#include <string.h>
#include <assert.h>

/**
* PI
*/
#define PI 3.1415926535897932384626433832795

/**
* Return types for functions with status returns.
*/

#define G_FAILURE 0
#define G_SUCCESS 1
#define LW_TRUE 1
#define LW_FALSE 0

/**
* Maximum allowed SRID value. 
* Currently we are using 20 bits (1048575) of storage for SRID.
*/
#define SRID_MAXIMUM 999999

/**
* Macros for manipulating the 'flags' byte. A uchar used as follows: 
* ---RGBMZ
* Three unused bits, followed by ReadOnly, Geodetic, HasBBox, HasM and HasZ flags.
*/
#define FLAGS_GET_Z(flags) ((flags) & 0x01)
#define FLAGS_GET_M(flags) (((flags) & 0x02)>>1)
#define FLAGS_GET_BBOX(flags) (((flags) & 0x4)>>2)
#define FLAGS_GET_GEODETIC(flags) (((flags) & 0x08)>>3)
#define FLAGS_GET_READONLY(flags) (((flags) & 0x10)>>4)
#define FLAGS_SET_Z(flags, value) ((value) ? ((flags) | 0x01) : ((flags) & 0xFE))
#define FLAGS_SET_M(flags, value) ((value) ? ((flags) | 0x02) : ((flags) & 0xFD))
#define FLAGS_SET_BBOX(flags, value) ((value) ? ((flags) | 0x04) : ((flags) & 0xFB))
#define FLAGS_SET_GEODETIC(flags, value) ((value) ? ((flags) | 0x08) : ((flags) & 0xF7))
#define FLAGS_SET_READONLY(flags, value) ((value) ? ((flags) | 0x10) : ((flags) & 0xEF))
#define FLAGS_NDIMS(flags) (2 + FLAGS_GET_Z(flags) + FLAGS_GET_M(flags))

/**
* Macro for reading the size from the GSERIALIZED size attribute.
* Cribbed from PgSQL, top 30 bits are size. Use VARSIZE() when working
* internally with PgSQL.
*/
#define SIZE_GET(varsize) (((varsize) >> 2) & 0x3FFFFFFF)
#define SIZE_SET(varsize, size) (((varsize) & 0x00000003)|(((size) & 0x3FFFFFFF) << 2 ))

/**
* Macros for manipulating the 'typemod' int. An int32 used as follows:
* Plus/minus = Top bit.
* Spare bits = Next 3 bits.
* SRID = Next 20 bits.
* TYPE = Next 6 bits.
* ZM Flags = Bottom 2 bits.
*/
#define TYPMOD_GET_SRID(typmod) ((typmod & 0x0FFFFF00)>>8)
#define TYPMOD_SET_SRID(typmod, srid) ((typmod & 0x000000FF) | ((srid & 0x000FFFFF)<<8))
#define TYPMOD_GET_TYPE(typmod) ((typmod & 0x000000FC)>>2)
#define TYPMOD_SET_TYPE(typmod, type) ((typmod & 0xFFFFFF03) | ((type & 0x0000003F)<<2))
#define TYPMOD_GET_Z(typmod) ((typmod & 0x00000002)>>1)
#define TYPMOD_SET_Z(typmod) (typmod | 0x00000002)
#define TYPMOD_GET_M(typmod) (typmod & 0x00000001)
#define TYPMOD_SET_M(typmod) (typmod | 0x00000001)
#define TYPMOD_GET_NDIMS(typmod) (2+TYPMOD_GET_Z(typmod)+TYPMOD_GET_M(typmod))



/**
* GBOX structure. 
* Include the flags, so we don't have to constantly pass them
* into the functions.
*/
typedef struct
{
	uchar flags;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	double zmin;
	double zmax;
	double mmin;
	double mmax;
} GBOX;



/**
* Coordinate structure. This will be created on demand from GPTARRAY when
* needed by algorithms and whatnot.
*/
typedef struct
{
	uchar flags;
	double *ordinates;
} GCOORDINATE;



/* Start with space for 16 points */
#define G_PT_ARRAY_DEFAULT_POINTS 16

/**
* GPTARRAY structure. Holder of our ordinates. Dynamically sized
* container for coordinate arrays.
*/
typedef struct
{
	uchar flags;
	size_t capacity;
	uint32 npoints;
	double *ordinates;
} GPTARRAY;


/**
* GSERIALIZED
*/
typedef struct
{
	uint32 size; /* For PgSQL use only, use VAR* macros to manipulate. */
	uchar srid[3]; /* 24 bits of SRID */
	uchar flags; /* HasZ, HasM, HasBBox, IsGeodetic, IsReadOnly */
	uchar data[1]; /* See gserialized.txt */
} GSERIALIZED;

/**
* G_GEOMETRY
*
* flags = bit flags, see FLAGS_HAS_* defines
* type = enumerated OGC geometry type number
*/
typedef struct
{
	uchar flags;
	uint32 type;
	GBOX *bbox; /* NULL == unneeded */
	uint32 srid; /* 0 == unknown */
	void *data;
} G_GEOMETRY;

/* POINTTYPE */
typedef struct
{
	uchar flags;
	uint32 type;
	GBOX *bbox; /* NULL == unneeded */
	uint32 srid; /* 0 == unknown */
	GPTARRAY *point;
} G_POINT;

/* LINETYPE */
/* CIRCSTRINGTYPE */
typedef struct
{
	uchar flags;
	uint32 type;
	GBOX *bbox; /* NULL == unneeded */
	uint32 srid; /* 0 == unknown */
	GPTARRAY *points;
} G_LINESTRING;

typedef G_LINESTRING G_CIRCULARSTRING;

/* POLYGONTYPE */
typedef struct
{
	uchar flags;
	uint32 type;
	GBOX *bbox; /* NULL == unneeded */
	uint32 srid; /* 0 == unknown */
	size_t capacity; /* How much space is allocated for *rings? */
	uint32 nrings; 
	GPTARRAY **rings; /* rings[0] is the exterior ring */
} G_POLYGON;

/* MULTIPOINTTYPE */
/* MULTILINETYPE */
/* MULTIPOINTTYPE */
/* COMPOUNDTYPE */
/* CURVEPOLYTYPE */
/* MULTICURVETYPE */
/* MULTISURFACETYPE */
/* COLLECTIONTYPE */
typedef struct
{
	uchar flags;
	uint32 type;
	GBOX *bbox; /* NULL == unneeded */
	uint32 srid; /* 0 == unknown */
	size_t capacity; /* How much space is allocated for *geoms? */
	uint32 ngeoms;
	G_GEOMETRY **geoms;
} G_COLLECTION;

/*
** All the collection types share the same physical structure as the
** generic geometry collection. We add type aliases so we can be more
** explicit in our functions later.
*/
typedef G_COLLECTION G_MPOINT;
typedef G_COLLECTION G_MLINESTRING;
typedef G_COLLECTION G_MPOLYGON;
typedef G_COLLECTION G_MSURFACE;
typedef G_COLLECTION G_MCURVE;
typedef G_COLLECTION G_COMPOUNDCURVE;
typedef G_COLLECTION G_CURVEPOLYGON;

/*
* Utility casts from GEOMETRY to concrete type.
* Return NULL if cast is illegal.
extern G_POINT* g_geometry_as_point(G_GEOMETRY *g);
extern G_LINESTRING* g_geometry_as_linestring(G_GEOMETRY *g);
extern G_POLYGON* g_geometry_as_polygon(G_GEOMETRY *g);
extern G_MPOINT* g_geometry_as_mpoint(G_GEOMETRY *g);
extern G_MLINESTRING* g_geometry_as_mlinestring(G_GEOMETRY *g);
extern G_MPOLYGON* g_geometry_as_mpolygon(G_GEOMETRY *g);
*/

/*
* Utility casts from concrete type to GEOMETRY.
* Return NULL if cast is illegal.
extern G_GEOMETRY* g_point_as_geometry(G_POINT *g);
extern G_GEOMETRY* g_linestring_as_geometry(G_LINESTRING *g);
extern G_GEOMETRY* g_polygon_as_geometry(G_POLYGON *g);
extern G_GEOMETRY* g_mpoint_as_geometry(G_MPOINT *g);
extern G_GEOMETRY* g_mlinestring_as_geometry(G_MLINESTRING *g);
extern G_GEOMETRY* g_mpolygon_as_geometry(G_POLYGON *g);
extern G_GEOMETRY* g_collection_as_geometry(G_COLLECTION *g);
*/

/***********************************************************************
* Coordinate creator, set, get and destroy.
*/
extern GCOORDINATE* gcoord_new(int ndims);
extern GCOORDINATE* gcoord_new_with_flags(uchar flags);
extern GCOORDINATE* gcoord_new_with_flags_and_ordinates(uchar flags, double *ordinates);
extern GCOORDINATE* gcoord_copy(GCOORDINATE *coord);
extern void gcoord_free(GCOORDINATE *coord);
extern void gcoord_set_x(GCOORDINATE *coord, double x);
extern void gcoord_set_y(GCOORDINATE *coord, double y);
extern void gcoord_set_z(GCOORDINATE *coord, double z);
extern void gcoord_set_m(GCOORDINATE *coord, double m);
extern void gcoord_set_ordinates(GCOORDINATE *coord, double *ordinates);
extern void gcoord_set_ordinate(GCOORDINATE *coord, double ordinate, int index);
extern double gcoord_get_x(GCOORDINATE *coord);
extern double gcoord_get_y(GCOORDINATE *coord);
extern double gcoord_get_z(GCOORDINATE *coord);
extern double gcoord_get_m(GCOORDINATE *coord);

/***********************************************************************
* Point arrays, set, get and destroy.
*/
extern GPTARRAY* gptarray_new(uchar flags);
extern GPTARRAY* gptarray_new_with_size(uchar flags, int npoints);
extern GPTARRAY* gptarray_new_with_ordinates(uchar flags, int npoints, double *ordinates);
extern GPTARRAY* gptarray_copy(GPTARRAY *ptarray);
extern void gptarray_free(GPTARRAY *ptarray);
extern void gptarray_add_coord(GPTARRAY *ptarray, GCOORDINATE *coord);
extern GCOORDINATE* gptarray_get_coord_ro(GPTARRAY *ptarray, int i);
extern GCOORDINATE* gptarray_get_coord_new(GPTARRAY *ptarray, int i);
extern void gptarray_set_coord(GPTARRAY *ptarray, int i, GCOORDINATE *coord);
extern void gptarray_set_x(GPTARRAY *ptarray, int i, double x);
extern void gptarray_set_y(GPTARRAY *ptarray, int i, double y);
extern void gptarray_set_z(GPTARRAY *ptarray, int i, double z);
extern void gptarray_set_m(GPTARRAY *ptarray, int i, double m);
extern double gptarray_get_x(GPTARRAY *ptarray, int i);
extern double gptarray_get_y(GPTARRAY *ptarray, int i);
extern double gptarray_get_z(GPTARRAY *ptarray, int i);
extern double gptarray_get_m(GPTARRAY *ptarray, int i);


/***********************************************************************
** Linestrings
*/
extern G_LINESTRING* glinestring_new_from_gptarray(GPTARRAY *ptarray);
extern G_LINESTRING* glinestring_new(uchar flags);


/***********************************************************************
** Utility functions for flag byte and srid_flag integer.
*/

/**
* Construct a new flags char.
*/
extern uchar gflags(int hasz, int hasm, int geodetic);

/**
* Extract the geometry type from the serialized form (it hides in 
* the anonymous data area, so this is a handy function).
*/
extern uint32 gserialized_get_type(GSERIALIZED *g);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern uint32 gserialized_get_srid(GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern void gserialized_set_srid(GSERIALIZED *g, uint32 srid);


/***********************************************************************
** Functions for managing serialized forms and bounding boxes.
*/

/**
* Calculate the geocentric bounding box directly from the serialized
* form of the geodetic coordinates. Only accepts serialized geographies
* flagged as geodetic. Caller is responsible for disposing of the GBOX.
*/
extern GBOX* gserialized_calculate_gbox_geocentric(GSERIALIZED *g);

/**
* Calculate the geocentric bounding box directly from the serialized
* form of the geodetic coordinates. Only accepts serialized geographies
* flagged as geodetic.
*/
int gserialized_calculate_gbox_geocentric_p(GSERIALIZED *g, GBOX *g_box);

/**
* Return a WKT representation of the gserialized geometry. 
* Caller is responsible for disposing of the char*.
*/
extern char* gserialized_to_string(GSERIALIZED *g);

/**
* Return a copy of the input serialized geometry. 
*/ 
extern GSERIALIZED* gserialized_copy(GSERIALIZED *g);

/**
* Check that coordinates of LWGEOM are all within the geodetic range.
*/
extern int lwgeom_check_geodetic(const LWGEOM *geom);

/**
* Calculate the geodetic bounding box for an LWGEOM. Z/M coordinates are 
* ignored for this calculation. Pass in non-null, geodetic bounding box for function
* to fill out. LWGEOM must have been built from a GSERIALIZED to provide
* double aligned point arrays.
*/
extern int lwgeom_calculate_gbox_geodetic(const LWGEOM *geom, GBOX *gbox);

/**
* Calculate the 2-4D bounding box of a geometry. Z/M coordinates are honored 
* for this calculation, though for curves they are not included in calculations
* of curvature.
*/
extern int lwgeom_calculate_gbox(const LWGEOM *lwgeom, GBOX *gbox);

/**
* Calculate the geodetic distance from lwgeom1 to lwgeom2 on the unit sphere. 
* Pass in a tolerance in radians.
*/
extern double lwgeom_distance_sphere(LWGEOM *lwgeom1, LWGEOM *lwgeom2, GBOX *gbox1, GBOX *gbox2, double tolerance);

/**
* New function to read doubles directly from the double* coordinate array
* of an aligned lwgeom #POINTARRAY (built by de-serializing a #GSERIALIZED).
*/
extern int getPoint2d_p_ro(const POINTARRAY *pa, int n, POINT2D **point);



/**
* Calculate box and add values to gbox. Return #G_SUCCESS on success.
*/
extern int ptarray_calculate_gbox_geodetic(POINTARRAY *pa, GBOX *gbox);

/**
* Create a new gbox with the dimensionality indicated by the flags. Caller
* is responsible for freeing.
*/
extern GBOX* gbox_new(uchar flags);

/**
* Update the merged #GBOX to be large enough to include itself and the new box.
*/
extern int gbox_merge(GBOX new_box, GBOX *merged_box);

/**
* Update the #GBOX to be large enough to include itself and the new point.
*/
extern int gbox_merge_point3d(POINT3D p, GBOX *gbox);

/**
* Allocate a string representation of the #GBOX, based on dimensionality of flags.
*/
extern char* gbox_to_string(GBOX *gbox);

/**
* Return a copy of the #GBOX, based on dimensionality of flags.
*/
extern GBOX* gbox_copy(GBOX *gbox);

/**
* Warning, do not use this function, it is very particular about inputs.
*/
extern GBOX* gbox_from_string(char *str);

/**
* Return #LW_TRUE if the #GBOX overlaps, #LW_FALSE otherwise. 
*/
extern int gbox_overlaps(GBOX *g1, GBOX *g2);

/**
* Copy the values of original #GBOX into duplicate.
*/
extern void gbox_duplicate(GBOX original, GBOX *duplicate);

/**
* Return the number of bytes necessary to hold a #GBOX of this dimension in 
* serialized form.
*/
extern size_t gbox_serialized_size(uchar flags);

/**
* Utility function to get type number from string. For example, a string 'POINTZ' 
* would return type of 1 and z of 1 and m of 0. Valid 
*/
extern int geometry_type_from_string(char *str, int *type, int *z, int *m);

/**
* Calculate required memory segment to contain a serialized form of the LWGEOM.
* Primarily used internally by the serialization code. Exposed to allow the cunit
* tests to exercise it.
*/
extern size_t gserialized_from_lwgeom_size(LWGEOM *geom);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL 
* VARSIZE information.
*/
extern GSERIALIZED* gserialized_from_lwgeom(LWGEOM *geom, int is_geodetic, size_t *size);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
extern LWGEOM* lwgeom_from_gserialized(GSERIALIZED *g);

/**
* Serialize/deserialize from/to #G_GEOMETRY into #GSERIALIZED
extern size_t gserialized_from_ggeometry_size(G_GEOMETRY *geom);
extern GSERIALIZED* gserialized_from_ggeometry(G_GEOMETRY *geom);
extern G_GEOMETRY* ggeometry_from_gserialized(GSERIALIZED *g);
*/

