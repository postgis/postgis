#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

//#define DEBUG_CALLS 1

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char *
lwpoint_serialize(LWPOINT *point)
{
	size_t size, retsize;
	char *result;

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
lwpoint_serialize_buf(LWPOINT *point, char *buf, size_t *retsize)
{
	int size=1;
	char hasSRID;
	char *loc;

	//printLWPOINT(point);
#ifdef DEBUG_CALLS
	lwnotice("lwpoint_serialize_buf(%p, %p) called", point, buf);
#endif

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID
	if (TYPE_HASBBOX(point->type)) size += sizeof(BOX2DFLOAT4); // bvol

	size += sizeof(double)*TYPE_NDIMS(point->type);

	buf[0] = (unsigned char) lwgeom_makeType_full(
		TYPE_HASZ(point->type), TYPE_HASM(point->type),
		hasSRID, POINTTYPE, TYPE_HASBBOX(point->type));
	loc = buf+1;

	if (TYPE_HASBBOX(point->type))
	{
		lwgeom_compute_bbox_p((LWGEOM *)point, (BOX2DFLOAT4 *)loc);
		loc += sizeof(BOX2DFLOAT4);
	}

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points

	if (TYPE_NDIMS(point->type) == 3)
	{
		if (TYPE_HASZ(point->type))
			getPoint3dz_p(point->point, 0, (POINT3DZ *)loc);
		else
			getPoint3dm_p(point->point, 0, (POINT3DM *)loc);
	}
	else if (TYPE_NDIMS(point->type) == 2)
		getPoint2d_p(point->point, 0, (POINT2D *)loc);
	else if (TYPE_NDIMS(point->type) == 4)
		getPoint4d_p(point->point, 0, (POINT4D *)loc);

	if (retsize) *retsize = size;
}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *
lwpoint_findbbox(LWPOINT *point)
{
#ifdef DEBUG
	lwnotice("lwpoint_findbbox called with point %p", point);
#endif
	if (point == NULL)
	{
#ifdef DEBUG
		lwnotice("lwpoint_findbbox returning NULL");
#endif
		return NULL;
	}

#ifdef DEBUG
	lwnotice("lwpoint_findbbox returning pointArray_bbox return");
#endif

	return pointArray_bbox(point->point);
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

#ifdef DEBUG_CALLS
	lwnotice("lwpoint_serialize_size called");
#endif

	if ( point->SRID != -1 ) size += 4; // SRID
	if ( TYPE_HASBBOX(point->type) ) size += sizeof(BOX2DFLOAT4);

	size += TYPE_NDIMS(point->type) * sizeof(double); // point

#ifdef DEBUG_CALLS
	lwnotice("lwpoint_serialize_size returning %d", size);
#endif

	return size; 
}

// construct a new point.  point will not be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOINT *
lwpoint_construct(char hasZ, char hasM, int SRID, char wantbbox, POINTARRAY *point)
{
	LWPOINT *result ;

	if (point == NULL)
		return NULL; // error

	result = lwalloc(sizeof(LWPOINT));
	result->type = lwgeom_makeType_full(hasZ, hasM, (SRID!=-1),
		POINTTYPE, wantbbox);
	result->SRID = SRID;
	result->point = point;

	return result;
}

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// See serialized form doc
LWPOINT *
lwpoint_deserialize(char *serialized_form)
{
	unsigned char type;
	LWPOINT *result;
	char *loc = NULL;
	POINTARRAY *pa;

#ifdef DEBUG_CALLS
	lwnotice("lwpoint_deserialize called");
#endif

	result = (LWPOINT*) lwalloc(sizeof(LWPOINT)) ;

	type = (unsigned char) serialized_form[0];

	if ( lwgeom_getType(type) != POINTTYPE) return NULL;
	result->type = type;

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
#ifdef DEBUG
		lwnotice("lwpoint_deserialize: input has bbox");
#endif
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
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
lwpoint_compute_bbox_p(LWPOINT *point, BOX2DFLOAT4 *box)
{
	return ptarray_compute_bbox_p(point->point, box);
}

// Clone LWPOINT object. POINTARRAY is not copied.
LWPOINT *
lwpoint_clone(const LWPOINT *g)
{
	LWPOINT *ret = lwalloc(sizeof(LWPOINT));
#ifdef DEBUG_CALLS
	lwnotice("lwpoint_clone called");
#endif
	memcpy(ret, g, sizeof(LWPOINT));
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
	geoms[0]->SRID = geoms[1]->SRID = -1;
	TYPE_SETHASSRID(geoms[0]->type, 0);
	TYPE_SETHASSRID(geoms[1]->type, 0);
	TYPE_SETHASBBOX(geoms[0]->type, 0);
	TYPE_SETHASBBOX(geoms[1]->type, 0);

	// Find appropriate geom type
	if ( TYPE_GETTYPE(what->type) == POINTTYPE ) newtype = MULTIPOINTTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
		TYPE_HASZ(to->type),
		TYPE_HASM(to->type),
		to->SRID,
		( TYPE_HASBBOX(what->type) || TYPE_HASBBOX(to->type) ),
		2, geoms);
	
	return (LWGEOM *)col;
}

//find length of this serialized point
size_t
lwgeom_size_point(const char *serialized_point)
{
	uint32  result = 1;
	unsigned char type;
	const char *loc;

	type = (unsigned char) serialized_point[0];

	if ( lwgeom_getType(type) != POINTTYPE) return 0;

#ifdef DEBUG
lwnotice("lwgeom_size_point called (%d)", result);
#endif

	loc = serialized_point+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
#ifdef DEBUG
lwnotice("lwgeom_size_point: has bbox (%d)", result);
#endif
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
lwnotice("lwgeom_size_point: has srid (%d)", result);
#endif
		loc +=4; // type + SRID
		result +=4;
	}

	result += lwgeom_ndims(type)*sizeof(double);

	return result;
}



