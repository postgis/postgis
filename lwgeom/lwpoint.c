#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char *
lwpoint_serialize(LWPOINT *point)
{
	int size=1;
	char hasSRID;
	char *result;
	char *loc;

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	if (point->ndims == 3) size += 24; //x,y,z
	else if (point->ndims == 2) size += 16 ; //x,y,z
	else if (point->ndims == 4) size += 32 ; //x,y,z,m

	result = lwalloc(size);

	result[0] = (unsigned char) lwgeom_makeType(point->ndims,
		hasSRID, POINTTYPE);
	loc = result+1;

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points

	if (point->ndims == 3) getPoint3d_p(point->point, 0, loc);
	else if (point->ndims == 2) getPoint2d_p(point->point, 0, loc);
	else if (point->ndims == 4) getPoint4d_p(point->point, 0, loc);
	return result;
}

// convert this point into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
void
lwpoint_serialize_buf(LWPOINT *point, char *buf, int *retsize)
{
	int size=1;
	char hasSRID;
	char *loc;

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	if (point->ndims == 3) size += 24; //x,y,z
	else if (point->ndims == 2) size += 16 ; //x,y,z
	else if (point->ndims == 4) size += 32 ; //x,y,z,m

	buf[0] = (unsigned char) lwgeom_makeType(point->ndims,
		hasSRID, POINTTYPE);
	loc = buf+1;

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points

	if (point->ndims == 3) getPoint3d_p(point->point, 0, loc);
	else if (point->ndims == 2) getPoint2d_p(point->point, 0, loc);
	else if (point->ndims == 4) getPoint4d_p(point->point, 0, loc);

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
POINT2D
lwpoint_getPoint2d(const LWPOINT *point)
{
	POINT2D result;

	if (point == NULL)
			return result;

	return getPoint2d(point->point,0);
}

// convenience functions to hide the POINTARRAY
POINT3D
lwpoint_getPoint3d(const LWPOINT *point)
{
	POINT3D result;

	if (point == NULL)
			return result;

	return getPoint3d(point->point,0);
}

// find length of this deserialized point
size_t
lwpoint_serialize_size(LWPOINT *point)
{
	size_t size = 1; // type

	if ( point->SRID != -1 ) size += 4; // SRID
	size += point->ndims * sizeof(double); // point

	return size; 
}

// construct a new point.  point will not be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOINT *
lwpoint_construct(int ndims, int SRID, POINTARRAY *point)
{
	LWPOINT *result ;

	if (point == NULL)
		return NULL; // error

	result = lwalloc(sizeof(LWPOINT));
	result->type = POINTTYPE;
	result->ndims = ndims;
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

#ifdef DEBUG
	lwnotice("lwpoint_deserialize called");
#endif

	result = (LWPOINT*) lwalloc(sizeof(LWPOINT)) ;

	type = (unsigned char) serialized_form[0];

	if ( lwgeom_getType(type) != POINTTYPE) return NULL;
	result->type = POINTTYPE;

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

	pa = pointArray_construct(loc, lwgeom_ndims(type), 1);

	result->point = pa;
	result->ndims = lwgeom_ndims(type);

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
	lwnotice("    ndims = %i", (int)point->ndims);
	lwnotice("    SRID = %i", (int)point->SRID);
	printPA(point->point);
	lwnotice("}");
}

