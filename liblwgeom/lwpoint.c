/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"


/*
 * Convert this point into its serialize form
 * result's first char will be the 8bit type.  See serialized form doc
 */
uchar *
lwpoint_serialize(LWPOINT *point)
{
	size_t size, retsize;
	uchar *result;

	size = lwpoint_serialize_size(point);
	result = lwalloc(size);
	lwpoint_serialize_buf(point, result, &retsize);

	if ( retsize != size )
	{
		lwerror("lwpoint_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	}

	return result;
}

/*
 * Convert this point into its serialize form writing it into
 * the given buffer, and returning number of bytes written into
 * the given int pointer.
 * result's first char will be the 8bit type.  See serialized form doc
 */
void
lwpoint_serialize_buf(LWPOINT *point, uchar *buf, size_t *retsize)
{
	int size=1;
	char has_srid;
	uchar *loc;
	int ptsize = ptarray_point_size(point->point);

	if ( FLAGS_GET_ZM(point->flags) != FLAGS_GET_ZM(point->point->flags) )
		lwerror("Dimensions mismatch in lwpoint");

	LWDEBUGF(2, "lwpoint_serialize_buf(%p, %p) called", point, buf);
	/*printLWPOINT(point); */

	if ( lwgeom_is_empty((LWGEOM*)point) ) 
	{
		LWCOLLECTION *mp = lwcollection_construct_empty(MULTIPOINTTYPE, point->srid, FLAGS_GET_Z(point->flags), FLAGS_GET_M(point->flags));
		LWDEBUG(4,"serializing 'POINT EMPTY' to 'MULTIPOINT EMPTY");
		lwcollection_serialize_buf(mp, buf, retsize);
		lwcollection_free(mp);
		return;
	}

	has_srid = (point->srid != SRID_UNKNOWN);

	if (has_srid) size +=4;  /*4 byte SRID */
	if (point->bbox) size += sizeof(BOX2DFLOAT4); /* bvol */

	size += sizeof(double)*FLAGS_NDIMS(point->flags);

	buf[0] = (uchar) lwgeom_makeType_full(
	             FLAGS_GET_Z(point->flags), FLAGS_GET_M(point->flags),
	             has_srid, POINTTYPE, point->bbox?1:0);
	loc = buf+1;

	if (point->bbox)
	{
		BOX2DFLOAT4 *box2df;
		
		box2df = box2df_from_gbox(point->bbox);
		memcpy(loc, box2df, sizeof(BOX2DFLOAT4));
		lwfree(box2df);
		loc += sizeof(BOX2DFLOAT4);
	}

	if (has_srid)
	{
		memcpy(loc, &point->srid, sizeof(int32));
		loc += 4;
	}

	/* copy in points */
	memcpy(loc, getPoint_internal(point->point, 0), ptsize);

	if (retsize) *retsize = size;
}

/* find length of this deserialized point */
size_t
lwpoint_serialize_size(LWPOINT *point)
{
	size_t size = 1; /* type */

	LWDEBUG(2, "lwpoint_serialize_size called");

	if ( lwgeom_is_empty((LWGEOM*)point) ) 
	{
		LWCOLLECTION *mp = lwcollection_construct_empty(MULTIPOINTTYPE, point->srid, FLAGS_GET_Z(point->flags), FLAGS_GET_M(point->flags));
		size_t s = lwcollection_serialize_size(mp);
		LWDEBUGF(4,"serializing 'POINT EMPTY' to 'MULTIPOINT EMPTY', size=%d", s);
		lwcollection_free(mp);
		return s;
	}
		

	if ( point->srid != SRID_UNKNOWN ) size += 4; /* SRID */
	if ( point->bbox ) size += sizeof(BOX2DFLOAT4);

	size += FLAGS_NDIMS(point->flags) * sizeof(double); /* point */

	LWDEBUGF(3, "lwpoint_serialize_size returning %d", size);

	return size;
}

/*
 * Find bounding box (standard one)
 *   zmin=zmax=NO_Z_VALUE if 2d
 */
BOX3D *
lwpoint_compute_box3d(LWPOINT *point)
{
	LWDEBUGF(2, "lwpoint_compute_box3d called with point %p", point);

	if (point == NULL)
	{
		LWDEBUG(3, "lwpoint_compute_box3d returning NULL");

		return NULL;
	}

	LWDEBUG(3, "lwpoint_compute_box3d returning ptarray_compute_box3d return");

	return ptarray_compute_box3d(point->point);
}



/*
 * Convenience functions to hide the POINTARRAY
 * TODO: obsolete this
 */
int
lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out)
{
	return getPoint2d_p(point->point, 0, out);
}

/* convenience functions to hide the POINTARRAY */
int
lwpoint_getPoint3dz_p(const LWPOINT *point, POINT3DZ *out)
{
	return getPoint3dz_p(point->point,0,out);
}
int
lwpoint_getPoint3dm_p(const LWPOINT *point, POINT3DM *out)
{
	return getPoint3dm_p(point->point,0,out);
}
int
lwpoint_getPoint4d_p(const LWPOINT *point, POINT4D *out)
{
	return getPoint4d_p(point->point,0,out);
}

double
lwpoint_get_x(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
		lwerror("lwpoint_get_x called with empty geometry");
	getPoint4d_p(point->point, 0, &pt);
	return pt.x;
}

double
lwpoint_get_y(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
		lwerror("lwpoint_get_y called with empty geometry");
	getPoint4d_p(point->point, 0, &pt);
	return pt.y;
}

double
lwpoint_get_z(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
		lwerror("lwpoint_get_z called with empty geometry");
	if ( ! FLAGS_GET_Z(point->flags) )
		lwerror("lwpoint_get_z called without z dimension");
	getPoint4d_p(point->point, 0, &pt);
	return pt.z;
}

double
lwpoint_get_m(const LWPOINT *point)
{
	POINT4D pt;
	if ( lwpoint_is_empty(point) )
		lwerror("lwpoint_get_m called with empty geometry");
	if ( ! FLAGS_GET_M(point->flags) )
		lwerror("lwpoint_get_m called without m dimension");
	getPoint4d_p(point->point, 0, &pt);
	return pt.m;
}

/*
 * Construct a new point.  point will not be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWPOINT *
lwpoint_construct(int srid, GBOX *bbox, POINTARRAY *point)
{
	LWPOINT *result;
	uchar flags = 0;

	if (point == NULL)
		return NULL; /* error */

	result = lwalloc(sizeof(LWPOINT));
	result->type = POINTTYPE;
	FLAGS_SET_Z(flags, FLAGS_GET_Z(point->flags));
	FLAGS_SET_M(flags, FLAGS_GET_M(point->flags));
	FLAGS_SET_BBOX(flags, bbox?1:0);
	result->flags = flags;
	result->srid = srid;
	result->point = point;
	result->bbox = bbox;

	return result;
}

LWPOINT *
lwpoint_construct_empty(int srid, char hasz, char hasm)
{
	LWPOINT *result = lwalloc(sizeof(LWPOINT));
	result->type = POINTTYPE;
	result->flags = gflags(hasz, hasm, 0);
	result->srid = srid;
	result->point = ptarray_construct(hasz, hasm, 0);
	result->bbox = NULL;
	return result;
}

LWPOINT *
lwpoint_make2d(int srid, double x, double y)
{
	POINT4D p = {x, y, 0.0, 0.0};
	POINTARRAY *pa = ptarray_construct_empty(0, 0, 1);

	ptarray_append_point(pa, &p, REPEATED_POINTS_OK);
	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make3dz(int srid, double x, double y, double z)
{
	POINT4D p = {x, y, z, 0.0};
	POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);

	ptarray_append_point(pa, &p, REPEATED_POINTS_OK);

	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make3dm(int srid, double x, double y, double m)
{
	POINT4D p = {x, y, 0.0, m};
	POINTARRAY *pa = ptarray_construct_empty(0, 1, 1);

	ptarray_append_point(pa, &p, REPEATED_POINTS_OK);

	return lwpoint_construct(srid, NULL, pa);
}

LWPOINT *
lwpoint_make4d(int srid, double x, double y, double z, double m)
{
	POINT4D p = {x, y, z, m};
	POINTARRAY *pa = ptarray_construct_empty(1, 1, 1);

	ptarray_append_point(pa, &p, REPEATED_POINTS_OK);

	return lwpoint_construct(srid, NULL, pa);
}

/*
 * Given the LWPOINT serialized form (or a pointer into a muli* one)
 * construct a proper LWPOINT.
 * serialized_form should point to the 8bit type format (with type = 1)
 * See serialized form doc
 */
LWPOINT *
lwpoint_deserialize(uchar *serialized_form)
{
	int geom_type;
	LWPOINT *result;
	uchar *loc = NULL;
	POINTARRAY *pa;
	uchar type;

	LWDEBUG(2, "lwpoint_deserialize called");

	result = (LWPOINT*) lwalloc(sizeof(LWPOINT)) ;

	type = (uchar) serialized_form[0];
	geom_type = TYPE_GETTYPE(type);

	if ( geom_type != POINTTYPE)
	{
		lwerror("lwpoint_deserialize: attempt to deserialize a point which is really a %s", lwtype_name(geom_type));
		return NULL;
	}
	result->type = geom_type;
	result->flags = gflags(TYPE_HASZ(type),TYPE_HASM(type),0);

	loc = serialized_form+1;

	if (TYPE_HASBBOX(type))
	{
		BOX2DFLOAT4 box2df;

		LWDEBUG(3, "lwpoint_deserialize: input has bbox");

		FLAGS_SET_BBOX(result->flags, 1);
		memcpy(&box2df, loc, sizeof(BOX2DFLOAT4));
		result->bbox = gbox_from_box2df(result->flags, &box2df);
		loc += sizeof(BOX2DFLOAT4);
	}
	else
	{
		result->bbox = NULL;
	}

	if ( TYPE_HASSRID(type))
	{
		LWDEBUG(3, "lwpoint_deserialize: input has SRID");

		result->srid = lw_get_int32(loc);
		loc += 4; /* type + SRID */
	}
	else
	{
		result->srid = SRID_UNKNOWN;
	}

	/* we've read the type (1 byte) and SRID (4 bytes, if present) */
	pa = ptarray_construct_reference_data(FLAGS_GET_Z(result->flags), FLAGS_GET_M(result->flags), 1, loc);
	
	result->point = pa;

	return result;
}

void lwpoint_free(LWPOINT *pt)
{
	if ( pt->bbox )
		lwfree(pt->bbox);
	if ( pt->point )
		ptarray_free(pt->point);
	lwfree(pt);
}

void printLWPOINT(LWPOINT *point)
{
	lwnotice("LWPOINT {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(point->flags));
	lwnotice("    BBOX = %i", FLAGS_GET_BBOX(point->flags) ? 1 : 0 );
	lwnotice("    SRID = %i", (int)point->srid);
	printPA(point->point);
	lwnotice("}");
}

int
lwpoint_compute_box2d_p(const LWPOINT *point, BOX2DFLOAT4 *box)
{
	return ptarray_compute_box2d_p(point->point, box);
}

/* Clone LWPOINT object. POINTARRAY is not copied. */
LWPOINT *
lwpoint_clone(const LWPOINT *g)
{
	LWPOINT *ret = lwalloc(sizeof(LWPOINT));

	LWDEBUG(2, "lwpoint_clone called");

	memcpy(ret, g, sizeof(LWPOINT));
	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}


/* Find length of this serialized point */
size_t
lwgeom_size_point(const uchar *serialized_point)
{
	uint32  result = 1;
	uchar type;
	const uchar *loc;

	type = serialized_point[0];

	if ( lwgeom_getType(type) != POINTTYPE) return 0;

	LWDEBUGF(2, "lwgeom_size_point called (%d)", result);

	loc = serialized_point+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);

		LWDEBUGF(3, "lwgeom_size_point: has bbox (%d)", result);
	}

	if ( lwgeom_hasSRID(type))
	{
		LWDEBUGF(3, "lwgeom_size_point: has srid (%d)", result);

		loc +=4; /* type + SRID */
		result +=4;
	}

	result += TYPE_NDIMS(type)*sizeof(double);

	return result;
}

void
lwpoint_release(LWPOINT *lwpoint)
{
	lwgeom_release(lwpoint_as_lwgeom(lwpoint));
}


/* check coordinate equality  */
char
lwpoint_same(const LWPOINT *p1, const LWPOINT *p2)
{
	return ptarray_same(p1->point, p2->point);
}


LWPOINT*
lwpoint_force_dims(const LWPOINT *point, int hasz, int hasm)
{
	POINTARRAY *pdims = NULL;
	LWPOINT *pointout;
	
	/* Return 2D empty */
	if( lwpoint_is_empty(point) )
	{
		pointout = lwpoint_construct_empty(point->srid, hasz, hasm);
	}
	else
	{
		/* Always we duplicate the ptarray and return */
		pdims = ptarray_force_dims(point->point, hasz, hasm);
		pointout = lwpoint_construct(point->srid, NULL, pdims);
	}
	pointout->type = point->type;
	return pointout;
}

int lwpoint_is_empty(const LWPOINT *point)
{
	if ( ! point->point || point->point->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

