// basic LWLINE functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

//#define DEBUG_CALLS 1
//#define DEBUG 1


// construct a new LWLINE.  points will *NOT* be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWLINE *
lwline_construct(int SRID, BOX2DFLOAT4 *bbox, POINTARRAY *points)
{
	LWLINE *result;
	result = (LWLINE*) lwalloc(sizeof(LWLINE));

	result->type = lwgeom_makeType_full(
		TYPE_HASZ(points->dims),
		TYPE_HASM(points->dims),
		(SRID!=-1), LINETYPE,
		0);
	result->SRID = SRID;
	result->points = points;
	result->bbox = bbox;

	return result;
}

// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
LWLINE *
lwline_deserialize(char *serialized_form)
{
	unsigned char type;
	LWLINE *result;
	char *loc =NULL;
	uint32 npoints;
	POINTARRAY *pa;

	result = (LWLINE*) lwalloc(sizeof(LWLINE)) ;

	type = (unsigned char) serialized_form[0];
	result->type = type;

	if ( lwgeom_getType(type) != LINETYPE)
	{
		lwerror("lwline_deserialize: attempt to deserialize a line when its not really a line");
		return NULL;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//lwnotice("line has bbox");
		result->bbox = (BOX2DFLOAT4 *)loc;
		loc += sizeof(BOX2DFLOAT4);
	}
	else
	{
		result->bbox = NULL;
		//lwnotice("line has NO bbox");
	}

	if ( lwgeom_hasSRID(type))
	{
		//lwnotice("line has srid");
		result->SRID = get_int32(loc);
		loc +=4; // type + SRID
	}
	else
	{
		//lwnotice("line has NO srid");
		result->SRID = -1;
	}

	// we've read the type (1 byte) and SRID (4 bytes, if present)

	npoints = get_uint32(loc);
	//lwnotice("line npoints = %d", npoints);
	loc +=4;
	pa = pointArray_construct(loc, TYPE_HASZ(type), TYPE_HASM(type), npoints);
	result->points = pa;

	return result;
}

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char *
lwline_serialize(LWLINE *line)
{
	size_t size, retsize;
	char * result;

	if (line == NULL) lwerror("lwline_serialize:: given null line");

	size = lwline_serialize_size(line);
	result = lwalloc(size);
	lwline_serialize_buf(line, result, &retsize);

	if ( retsize != size )
	{
		lwerror("lwline_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	}

	return result;
}

// convert this line into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
void
lwline_serialize_buf(LWLINE *line, char *buf, size_t *retsize)
{
	char hasSRID;
	char *loc;
	int ptsize = pointArray_ptsize(line->points);

	if ( ptsize != sizeof(double)*TYPE_NDIMS(line->type) )
	{
		lwerror("lwline_serialize_buf: line has %d dims, its pointarray has %d dims", TYPE_NDIMS(line->type), TYPE_NDIMS(line->points->dims));
	}

#ifdef DEBUG_CALLS
	lwnotice("lwline_serialize_buf(%p, %p, %p) called",
		line, buf, retsize);
#endif

	if (line == NULL)
		lwerror("lwline_serialize:: given null line");

	hasSRID = (line->SRID != -1);

	buf[0] = (unsigned char) lwgeom_makeType_full(
		TYPE_HASZ(line->type), TYPE_HASM(line->type),
		hasSRID, LINETYPE, line->bbox ? 1 : 0);
	loc = buf+1;

#ifdef DEBUG
	lwnotice("lwline_serialize_buf added type (%d)", line->type);
#endif

	if (line->bbox)
	{
		memcpy(loc, line->bbox, sizeof(BOX2DFLOAT4));
		loc += sizeof(BOX2DFLOAT4);
#ifdef DEBUG
		lwnotice("lwline_serialize_buf added BBOX");
#endif
	}

	if (hasSRID)
	{
		memcpy(loc, &line->SRID, sizeof(int32));
		loc += sizeof(int32);
#ifdef DEBUG
		lwnotice("lwline_serialize_buf added SRID");
#endif
	}

	memcpy(loc, &line->points->npoints, sizeof(uint32));
	loc += sizeof(uint32);

#ifdef DEBUG
	lwnotice("lwline_serialize_buf added npoints (%d)",
		line->points->npoints);
#endif

	//copy in points
	memcpy(loc, line->points->serialized_pointlist,
		ptsize*line->points->npoints);
	loc += ptsize*line->points->npoints;

#ifdef DEBUG
	lwnotice("lwline_serialize_buf copied serialized_pointlist (%d bytes)",
		ptsize * line->points->npoints);
#endif

	if (retsize) *retsize = loc-buf;

	//printBYTES((unsigned char *)result, loc-buf);

#ifdef DEBUG_CALLS
	lwnotice("lwline_serialize_buf returning (loc: %p, size: %d)",
		loc, loc-buf);
#endif

}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwline_findbbox(LWLINE *line)
{
	BOX3D *ret;

	if (line == NULL)
		return NULL;

	ret = pointArray_bbox(line->points);
	return ret;
}

// find length of this deserialized line
size_t
lwline_serialize_size(LWLINE *line)
{
	size_t size = 1;  //type

#ifdef DEBUG_CALLS
	lwnotice("lwline_serialize_size called");
#endif

	if ( line->SRID != -1 ) size += 4; // SRID
	if ( line->bbox ) size += sizeof(BOX2DFLOAT4);

	size += 4; // npoints
	size += pointArray_ptsize(line->points)*line->points->npoints;

#ifdef DEBUG_CALLS
	lwnotice("lwline_serialize_size returning %d", size);
#endif

	return size;
}

void pfree_line (LWLINE  *line)
{
	lwfree(line->points);
	lwfree(line);
}

// find length of this serialized line
size_t
lwgeom_size_line(const char *serialized_line)
{
	int type = (unsigned char) serialized_line[0];
	uint32 result = 1;  //type
	const char *loc;
	uint32 npoints;

#ifdef DEBUG_CALLS
	lwnotice("lwgeom_size_line called");
#endif

	if ( lwgeom_getType(type) != LINETYPE)
		lwerror("lwgeom_size_line::attempt to find the length of a non-line");


	loc = serialized_line+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(type))
	{
		loc += 4; // type + SRID
		result +=4;
	}

	// we've read the type (1 byte) and SRID (4 bytes, if present)
	npoints = get_uint32(loc);
	result += sizeof(uint32); //npoints

	result += TYPE_NDIMS(type) * sizeof(double) * npoints;

#ifdef DEBUG_CALLS
	lwnotice("lwgeom_size_line returning %d", result);
#endif

	return result;
}

void printLWLINE(LWLINE *line)
{
	lwnotice("LWLINE {");
	lwnotice("    ndims = %i", (int)TYPE_NDIMS(line->type));
	lwnotice("    SRID = %i", (int)line->SRID);
	printPA(line->points);
	lwnotice("}");
}

int
lwline_compute_bbox_p(LWLINE *line, BOX2DFLOAT4 *box)
{
	return ptarray_compute_bbox_p(line->points, box);
}

// Clone LWLINE object. POINTARRAY is not copied.
LWLINE *
lwline_clone(const LWLINE *g)
{
	LWLINE *ret = lwalloc(sizeof(LWLINE));
	memcpy(ret, g, sizeof(LWLINE));
	if ( g->bbox && ! TYPE_HASBBOX(g->type) )
		ret->bbox = box2d_clone(g->bbox);
	return ret;
}

// Add 'what' to this line at position 'where'.
// where=0 == prepend
// where=-1 == append
// Returns a MULTILINE or a GEOMETRYCOLLECTION
LWGEOM *
lwline_add(const LWLINE *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;

	if ( where != -1 && where != 0 )
	{
		lwerror("lwline_add only supports 0 or -1 as second argument, got %d", where);
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
	if ( TYPE_GETTYPE(what->type) == LINETYPE ) newtype = MULTILINETYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
		to->SRID, NULL,
		2, geoms);
	
	return (LWGEOM *)col;
}

void
lwline_reverse(LWLINE *line)
{
	ptarray_reverse(line->points);
}

LWLINE *
lwline_segmentize2d(LWLINE *line, double dist)
{
	return lwline_construct(line->SRID, line->bbox,
		ptarray_segmentize2d(line->points, dist));
}

// check coordinate equality 
char
lwline_same(const LWLINE *l1, const LWLINE *l2)
{
	return ptarray_same(l1->points, l2->points);
}

/*
 * Construct a LWLINE from an array of LWPOINTs
 * LWLINE dimensions are large enough to host all input dimensions.
 */
LWLINE *
lwline_from_lwpointarray(int SRID, unsigned int npoints, LWPOINT **points)
{
	int zmflag=0;
	unsigned int i;
	POINTARRAY *pa;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<npoints; i++)
	{
		if ( TYPE_GETTYPE(points[i]->type) != POINTTYPE )
		{
			lwerror("lwline_from_lwpointarray: invalid input type: %s",
				lwgeom_typename(TYPE_GETTYPE(points[i]->type)));
			return NULL;
		}
		if ( TYPE_HASZ(points[i]->type) ) zmflag |= 2;
		if ( TYPE_HASM(points[i]->type) ) zmflag |= 1;
		if ( zmflag == 3 ) break;
	}

	/* Allocate space for output pointarray */
	pa = ptarray_construct(zmflag&2, zmflag&1, npoints);

#ifdef DEBUG
	lwnotice("lwline_from_lwpointarray: constructed pointarray for %d points, %d zmflag",
		npoints, zmflag);
#endif

	/*
	 * Fill pointarray
	 */
	switch (zmflag)
	{
		case 0: // 2d
			for (i=0; i<npoints; i++)
				getPoint2d_p(points[i]->point, 0,
					(POINT2D *)getPoint(pa, i));
			break;

		case 1: // 3dm
			for (i=0; i<npoints; i++)
				getPoint3dm_p(points[i]->point, 0,
					(POINT3DM *)getPoint(pa, i));
			break;

		case 2: // 3dz
			for (i=0; i<npoints; i++)
				getPoint3dz_p(points[i]->point, 0,
					(POINT3DZ *)getPoint(pa, i));
			break;

		case 3: // 4d
			for (i=0; i<npoints; i++)
				getPoint4d_p(points[i]->point, 0,
					(POINT4D *)getPoint(pa, i));
			break;

		default:
			lwerror ("lwline_from_lwpointarray: unespected ZMflag: %d", zmflag);
			return NULL;
	}

	return lwline_construct(SRID, NULL, pa);
}

/*
 * Construct a LWLINE from a LWMPOINT
 */
LWLINE *
lwline_from_lwmpoint(int SRID, LWMPOINT *mpoint)
{
	unsigned int i;
	POINTARRAY *pa;
	char zmflag = TYPE_GETZM(mpoint->type);

	/* Allocate space for output pointarray */
	pa = ptarray_construct(TYPE_HASZ(mpoint->type),
		TYPE_HASM(mpoint->type), mpoint->ngeoms);

#ifdef DEBUG
	lwnotice("lwline_from_lwmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);
#endif

	/*
	 * Fill pointarray
	 */
	switch (zmflag)
	{
		case 0: // 2d
			for (i=0; i<mpoint->ngeoms; i++)
				getPoint2d_p(mpoint->geoms[i]->point, 0,
					(POINT2D *)getPoint(pa, i));
			break;

		case 1: // 3dm
			for (i=0; i<mpoint->ngeoms; i++)
				getPoint3dm_p(mpoint->geoms[i]->point, 0,
					(POINT3DM *)getPoint(pa, i));
			break;

		case 2: // 3dz
			for (i=0; i<mpoint->ngeoms; i++)
				getPoint3dz_p(mpoint->geoms[i]->point, 0,
					(POINT3DZ *)getPoint(pa, i));
			break;

		case 3: // 4d
			for (i=0; i<mpoint->ngeoms; i++)
				getPoint4d_p(mpoint->geoms[i]->point, 0,
					(POINT4D *)getPoint(pa, i));
			break;

		default:
			lwerror ("lwline_from_lwmpoint: unespected ZMflag: %d", zmflag);
			return NULL;
	}

	return lwline_construct(SRID, NULL, pa);
}
