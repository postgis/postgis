// basic LWLINE functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

// construct a new LWLINE.  points will *NOT* be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWLINE *lwline_construct(int ndims, int SRID,  POINTARRAY *points)
{
	LWLINE *result;
	result = (LWLINE*) lwalloc( sizeof(LWLINE));

	result->type = LINETYPE;
	result->ndims =ndims;
	result->SRID = SRID;
	result->points = points;

	return result;
}

// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
LWLINE *lwline_deserialize(char *serialized_form)
{
	unsigned char type;
	LWLINE *result;
	char *loc =NULL;
	uint32 npoints;
	POINTARRAY *pa;

	result = (LWLINE*) lwalloc(sizeof(LWLINE)) ;

	type = (unsigned char) serialized_form[0];
	result->type = LINETYPE;

	if ( lwgeom_getType(type) != LINETYPE)
	{
		lwerror("lwline_deserialize: attempt to deserialize a line when its not really a line");
		return NULL;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//lwnotice("line has bbox");
		loc += sizeof(BOX2DFLOAT4);
		result->hasbbox = 1;
	}
	else
	{
		result->hasbbox = 0;
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
	pa = pointArray_construct( loc, lwgeom_ndims(type), npoints);

	result->points = pa;
	result->ndims = lwgeom_ndims(type);

	return result;
}

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char  *lwline_serialize(LWLINE *line)
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
void lwline_serialize_buf(LWLINE *line, char *buf, int *retsize)
{
	int size=1;  // type byte
	char hasSRID;
	int t;
	char *loc;
	int ptsize = sizeof(double)*line->ndims;

	if (line == NULL)
		lwerror("lwline_serialize:: given null line");

	hasSRID = (line->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID
	if (line->hasbbox) size += sizeof(BOX2DFLOAT4); // bvol
	size += ptsize * line->points->npoints; 

	size+=4; // npoints

	buf[0] = (unsigned char) lwgeom_makeType_full(line->ndims,
		hasSRID, LINETYPE, line->hasbbox);
	loc = buf+1;

	if (line->hasbbox)
	{
		lwgeom_compute_bbox_p((LWGEOM *)line, (BOX2DFLOAT4 *)loc);
		loc += sizeof(BOX2DFLOAT4);
	}

	if (hasSRID)
	{
		memcpy(loc, &line->SRID, sizeof(int32));
		loc += 4;
	}

	memcpy(loc, &line->points->npoints, sizeof(int32));
	loc +=4;
	//copy in points

//lwnotice(" line serialize - size = %i", size);

	if (line->ndims == 3)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint3d_p(line->points, t, loc);
			loc += 24; // size of a 3d point
		}
	}
	else if (line->ndims == 2)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint2d_p(line->points, t, loc);
			loc += 16; // size of a 2d point
		}
	}
	else if (line->ndims == 4)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint4d_p(line->points, t, loc);
			loc += 32; // size of a 2d point
		}
	}
	//printBYTES((unsigned char *)result, size);

	if (retsize) *retsize = size;
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

	if ( line->SRID != -1 ) size += 4; // SRID
	if ( line->hasbbox ) size += sizeof(BOX2DFLOAT4);

	size += sizeof(double)*line->ndims*line->points->npoints; // points

	return size;
}

void pfree_line     (LWLINE  *line)
{
	lwfree(line->points);
	lwfree(line);
}

// find length of this serialized line
uint32
lwgeom_size_line(const char *serialized_line)
{
	int type = (unsigned char) serialized_line[0];
	uint32 result =1;  //type
	const char *loc;
	uint32 npoints;

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
		result += 4; //npoints

		if (lwgeom_ndims(type) ==3)
		{
			return result + npoints * 24;
		}
		else if (lwgeom_ndims(type) ==2)
		{
			return result+ npoints * 16;
		}
		else if (lwgeom_ndims(type) ==4)
		{
			return result+ npoints * 32;
		}
		lwerror("lwgeom_size_line :: invalid ndims");
		return 0; //never get here
}

void printLWLINE(LWLINE *line)
{
	lwnotice("LWLINE {");
	lwnotice("    ndims = %i", (int)line->ndims);
	lwnotice("    SRID = %i", (int)line->SRID);
	printPA(line->points);
	lwnotice("}");
}

int
lwline_compute_bbox_p(LWLINE *line, BOX2DFLOAT4 *box)
{
	return ptarray_compute_bbox_p(line->points, box);
}
