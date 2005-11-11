#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

//#define PGIS_DEBUG_CALLS 1

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
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

// convert this point into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
void
lwpoint_serialize_buf(LWPOINT *point, uchar *buf, size_t *retsize)
{
	int size=1;
	char hasSRID;
	uchar *loc;
	int ptsize = pointArray_ptsize(point->point);

	if ( TYPE_GETZM(point->type) != TYPE_GETZM(point->point->dims) )
		lwerror("Dimensions mismatch in lwpoint");

	//printLWPOINT(point);
#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwpoint_serialize_buf(%p, %p) called", point, buf);
#endif

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID
	if (point->bbox) size += sizeof(BOX2DFLOAT4); // bvol

	size += sizeof(double)*TYPE_NDIMS(point->type);

	buf[0] = (uchar) lwgeom_makeType_full(
		TYPE_HASZ(point->type), TYPE_HASM(point->type),
		hasSRID, POINTTYPE, point->bbox?1:0);
	loc = buf+1;

	if (point->bbox)
	{
		memcpy(loc, point->bbox, sizeof(BOX2DFLOAT4));
		loc += sizeof(BOX2DFLOAT4);
	}

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points
	memcpy(loc, getPoint_internal(point->point, 0), ptsize);

	if (retsize) *retsize = size;
}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *
lwpoint_compute_box3d(LWPOINT *point)
{
#ifdef PGIS_DEBUG
	lwnotice("lwpoint_compute_box3d called with point %p", point);
#endif
	if (point == NULL)
	{
#ifdef PGIS_DEBUG
		lwnotice("lwpoint_compute_box3d returning NULL");
#endif
		return NULL;
	}

#ifdef PGIS_DEBUG
	lwnotice("lwpoint_compute_box3d returning ptarray_compute_box3d return");
#endif

	return ptarray_compute_box3d(point->point);
}

// convenience functions to hide the POINTARRAY
// TODO: obsolete this
int
lwpoint_getPoint2d_p(const LWPOINT *point, POINT2D *out)
{
	return getPoint2d_p(point->point, 0, out);
}

// convenience functions to hide the POINTARRAY
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

// find length of this deserialized point
size_t
lwpoint_serialize_size(LWPOINT *point)
{
	size_t size = 1; // type

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwpoint_serialize_size called");
#endif

	if ( point->SRID != -1 ) size += 4; // SRID
	if ( point->bbox ) size += sizeof(BOX2DFLOAT4);

	size += TYPE_NDIMS(point->type) * sizeof(double); // point

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwpoint_serialize_size returning %d", size);
#endif

	return size; 
}

// construct a new point.  point will not be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOINT *
lwpoint_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *point)
{
	LWPOINT *result ;

	if (point == NULL)
		return NULL; // error

	result = lwalloc(sizeof(LWPOINT));
	result->type = lwgeom_makeType_full(TYPE_HASZ(point->dims), TYPE_HASM(point->dims), (SRID!=-1), POINTTYPE, 0);
	result->SRID = SRID;
	result->point = point;
	result->bbox = bbox;

	return result;
}

LWPOINT *
make_lwpoint2d(int SRID, double x, double y)
{
	POINT2D p;
	POINTARRAY *pa = ptarray_construct(0, 0, 1);

	p.x = x;
	p.y = y;

	memcpy(getPoint_internal(pa, 0), &p, sizeof(POINT2D));

	return lwpoint_construct(SRID, NULL, pa);
}

LWPOINT *
make_lwpoint3dz(int SRID, double x, double y, double z)
{
	POINT3DZ p;
	POINTARRAY *pa = ptarray_construct(1, 0, 1);

	p.x = x;
	p.y = y;
	p.z = z;

	memcpy(getPoint_internal(pa, 0), &p, sizeof(POINT3DZ));

	return lwpoint_construct(SRID, NULL, pa);
}

LWPOINT *
make_lwpoint3dm(int SRID, double x, double y, double m)
{
	POINTARRAY *pa = ptarray_construct(0, 1, 1);
	POINT3DM p;

	p.x = x;
	p.y = y;
	p.m = m;

	memcpy(getPoint_internal(pa, 0), &p, sizeof(POINT3DM));

	return lwpoint_construct(SRID, NULL, pa);
}

LWPOINT *
make_lwpoint4d(int SRID, double x, double y, double z, double m)
{
	POINTARRAY *pa = ptarray_construct(1, 1, 1);
	POINT4D p;
	
	p.x = x;
	p.y = y;
	p.z = z;
	p.m = m;

	memcpy(getPoint_internal(pa, 0), &p, sizeof(POINT4D));

	return lwpoint_construct(SRID, NULL, pa);
}

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// See serialized form doc
LWPOINT *
lwpoint_deserialize(uchar *serialized_form)
{
	uchar type;
	LWPOINT *result;
	uchar *loc = NULL;
	POINTARRAY *pa;

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwpoint_deserialize called");
#endif

	result = (LWPOINT*) lwalloc(sizeof(LWPOINT)) ;

	type = (uchar) serialized_form[0];

	if ( lwgeom_getType(type) != POINTTYPE) return NULL;
	result->type = type;

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
#ifdef PGIS_DEBUG
		lwnotice("lwpoint_deserialize: input has bbox");
#endif
		result->bbox = (BOX2DFLOAT4 *)loc;
		loc += sizeof(BOX2DFLOAT4);
	}
	else
	{
		result->bbox = NULL;
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef PGIS_DEBUG
		lwnotice("lwpoint_deserialize: input has SRID");
#endif
		result->SRID = get_int32(loc);
		loc += 4; // type + SRID
	}
	else
	{
		result->SRID = -1;
	}

	// we've read the type (1 byte) and SRID (4 bytes, if present)

	pa = pointArray_construct(loc, TYPE_HASZ(type), TYPE_HASM(type), 1);

	result->point = pa;

	return result;
}

void pfree_point    (LWPOINT *pt)
{
	pfree_POINTARRAY(pt->point);
	lwfree(pt);
}

void printLWPOINT(LWPOINT *point)
{
	lwnotice("LWPOINT {");
	lwnotice("    ndims = %i", (int)TYPE_NDIMS(point->type));
	lwnotice("    BBOX = %i", TYPE_HASBBOX(point->type) ? 1 : 0 );
	lwnotice("    SRID = %i", (int)point->SRID);
	printPA(point->point);
	lwnotice("}");
}

int
lwpoint_compute_box2d_p(LWPOINT *point, BOX2DFLOAT4 *box)
{
	return ptarray_compute_box2d_p(point->point, box);
}

// Clone LWPOINT object. POINTARRAY is not copied.
LWPOINT *
lwpoint_clone(const LWPOINT *g)
{
	LWPOINT *ret = lwalloc(sizeof(LWPOINT));
#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwpoint_clone called");
#endif
	memcpy(ret, g, sizeof(LWPOINT));
	if ( g->bbox && ! TYPE_HASBBOX(g->type) )
		ret->bbox = box2d_clone(g->bbox);
	return ret;
}

// Add 'what' to this point at position 'where'.
// where=0 == prepend
// where=-1 == append
// Returns a MULTIPOINT or a GEOMETRYCOLLECTION
LWGEOM *
lwpoint_add(const LWPOINT *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;

	if ( where != -1 && where != 0 )
	{
		lwerror("lwpoint_add only supports 0 or -1 as second argument, got %d", where);
		return NULL;
	}

	// dimensions compatibility are checked by caller


	// Construct geoms array
	geoms = lwalloc(sizeof(LWGEOM *)*2);
	if ( where == -1 ) // append
	{
		geoms[0] = lwgeom_clone((LWGEOM *)to);
		geoms[1] = lwgeom_clone(what);
	}
	else // prepend
	{
		geoms[0] = lwgeom_clone(what);
		geoms[1] = lwgeom_clone((LWGEOM *)to);
	}
	// reset SRID and wantbbox flag from component types
	lwgeom_dropSRID(geoms[0]);
	lwgeom_dropBBOX(geoms[0]);
	lwgeom_dropSRID(geoms[1]);
	lwgeom_dropBBOX(geoms[1]);

	// Find appropriate geom type
	if ( TYPE_GETTYPE(what->type) == POINTTYPE ) newtype = MULTIPOINTTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
		to->SRID, NULL,
		2, geoms);
	
	return (LWGEOM *)col;
}

//find length of this serialized point
size_t
lwgeom_size_point(const uchar *serialized_point)
{
	uint32  result = 1;
	uchar type;
	const uchar *loc;

	type = serialized_point[0];

	if ( lwgeom_getType(type) != POINTTYPE) return 0;

#ifdef PGIS_DEBUG
lwnotice("lwgeom_size_point called (%d)", result);
#endif

	loc = serialized_point+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
#ifdef PGIS_DEBUG
lwnotice("lwgeom_size_point: has bbox (%d)", result);
#endif
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef PGIS_DEBUG
lwnotice("lwgeom_size_point: has srid (%d)", result);
#endif
		loc +=4; // type + SRID
		result +=4;
	}

	result += lwgeom_ndims(type)*sizeof(double);

	return result;
}

// check coordinate equality 
char
lwpoint_same(const LWPOINT *p1, const LWPOINT *p2)
{
	return ptarray_same(p1->point, p2->point);
}
