// basic LWPOLY manipulation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

// construct a new LWPOLY.  arrays (points/points per ring) will NOT be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOLY *
lwpoly_construct(int ndims, int SRID, int nrings,POINTARRAY **points)
{
	LWPOLY *result;

	result = (LWPOLY*) lwalloc(sizeof(LWPOLY));
	result->type = POLYGONTYPE;
	result->ndims = ndims;
	result->SRID = SRID;
	result->nrings = nrings;
	result->rings = points;

	return result;
}


// given the LWPOLY serialized form (or a pointer into a muli* one)
// construct a proper LWPOLY.
// serialized_form should point to the 8bit type format (with type = 3)
// See serialized form doc
LWPOLY *
lwpoly_deserialize(char *serialized_form)
{

	LWPOLY *result;
	uint32 nrings;
	int ndims;
	uint32 npoints;
	unsigned char type;
	char  *loc;
	int t;

	if (serialized_form == NULL)
	{
		lwerror("lwpoly_deserialize called with NULL arg");
		return NULL;
	}

	result = (LWPOLY*) lwalloc(sizeof(LWPOLY));

	type = (unsigned  char) serialized_form[0];
	result->type = POLYGONTYPE;

	ndims = lwgeom_ndims(type);
	loc = serialized_form;

	if ( lwgeom_getType(type) != POLYGONTYPE)
	{
		lwerror("lwpoly_deserialize called with arg of type %d",
			lwgeom_getType(type));
		return NULL;
	}


	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result->hasbbox = 1;
	}
	else
	{
		result->hasbbox = 0;
	}

	if ( lwgeom_hasSRID(type))
	{
		result->SRID = get_int32(loc);
		loc +=4; // type + SRID
	}
	else
	{
		result->SRID = -1;
	}

	nrings = get_uint32(loc);
	result->nrings = nrings;
	loc +=4;
	result->rings = (POINTARRAY**) lwalloc(nrings* sizeof(POINTARRAY*));

	for (t =0;t<nrings;t++)
	{
		//read in a single ring and make a PA
		npoints = get_uint32(loc);
		loc +=4;

		result->rings[t] = pointArray_construct(loc, ndims, npoints);
		if (ndims == 3)
			loc += 24*npoints;
		else if (ndims == 2)
			loc += 16*npoints;
		else if (ndims == 4)
			loc += 32*npoints;
	}
	result->ndims = ndims;

	return result;
}

// create the serialized form of the polygon
// result's first char will be the 8bit type.  See serialized form doc
// points copied
char *
lwpoly_serialize(LWPOLY *poly)
{
	size_t size, retsize;
	char *result;

	size = lwpoly_serialize_size(poly);
	result = lwalloc(size);
	lwpoly_serialize_buf(poly, result, &retsize);
	
	if ( retsize != size )
	{
		lwerror("lwpoly_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	}

	return result;
}

// create the serialized form of the polygon writing it into the
// given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
// points copied
void
lwpoly_serialize_buf(LWPOLY *poly, char *buf, int *retsize)
{
	int size=1;  // type byte
	char hasSRID;
	int t,u;
	int total_points = 0;
	int npoints;
	char *loc;

	hasSRID = (poly->SRID != -1);

	size += 4; // nrings
	size += 4*poly->nrings; //npoints/ring

	for (t=0;t<poly->nrings;t++) {
		total_points  += poly->rings[t]->npoints;
	}
	size += sizeof(double)*poly->ndims*total_points;

	buf[0] = (unsigned char) lwgeom_makeType_full(poly->ndims,
		hasSRID, POLYGONTYPE, poly->hasbbox);
	loc = buf+1;

	if (poly->hasbbox)
	{
		lwgeom_compute_bbox_p((LWGEOM *)poly, (BOX2DFLOAT4 *)loc);
		size += sizeof(BOX2DFLOAT4); // bvol
		loc += sizeof(BOX2DFLOAT4);
	}

	if (hasSRID)
	{
		memcpy(loc, &poly->SRID, sizeof(int32));
		loc += 4;
		size +=4;  //4 byte SRID
	}

	memcpy(loc, &poly->nrings, sizeof(int32));  // nrings
	loc+=4;

	for (t=0;t<poly->nrings;t++)
	{
		POINTARRAY *pa = poly->rings[t];
		npoints = poly->rings[t]->npoints;
		memcpy(loc, &npoints, sizeof(int32)); //npoints this ring
		loc+=4;
		if (poly->ndims == 3)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint3d_p(pa, u, loc);
				loc+= 24;
			}
		}
		else if (poly->ndims == 2)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint2d_p(pa, u, loc);
				loc+= 16;
			}
		}
		else if (poly->ndims == 4)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint4d_p(pa, u, loc);
				loc+= 32;
			}
		}
	}

	if (retsize) *retsize = size;
}


// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *
lwpoly_findbbox(LWPOLY *poly)
{
//	int t;

	BOX3D *result;
//	BOX3D *abox,*abox2;

	POINTARRAY *pa = poly->rings[0];   // just need to check outer ring -- interior rings are inside
	result  = pointArray_bbox(pa);

//	for (t=1;t<poly->nrings;t++)
	//{
//		pa = poly->rings[t];
//		abox  = pointArray_bbox(pa);
//		abox2 = result;
//		result = combine_boxes( abox, abox2);
//		lwfree(abox);
//		lwfree(abox2);
  //  }

    return result;
}

//find length of this serialized polygon
uint32
lwgeom_size_poly(const char *serialized_poly)
{
	uint32 result = 1; // char type
	uint32 nrings;
	int   ndims;
	int t;
	unsigned char type;
	uint32 npoints;
	const char *loc;

	if (serialized_poly == NULL)
		return -9999;


	type = (unsigned char) serialized_poly[0];
	ndims = lwgeom_ndims(type);

	if ( lwgeom_getType(type) != POLYGONTYPE)
		return -9999;


	loc = serialized_poly+1;

	if (lwgeom_hasBBOX(type))
	{
#ifdef DEBUG
		lwnotice("lwgeom_size_poly: has bbox");
#endif
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}


	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
		lwnotice("lwgeom_size_poly: has srid");
#endif
		loc +=4; // type + SRID
		result += 4;
	}


	nrings = get_uint32(loc);
	loc +=4;
	result +=4;


	for (t =0;t<nrings;t++)
	{
		//read in a single ring and make a PA
		npoints = get_uint32(loc);
		loc +=4;
		result +=4;

		if (ndims == 3)
		{
			loc += 24*npoints;
			result += 24*npoints;
		}
		else if (ndims == 2)
		{
			loc += 16*npoints;
			result += 16*npoints;
		}
		else if (ndims == 4)
		{
			loc += 32*npoints;
			result += 32*npoints;
		}
	}
	return result;
}

// find length of this deserialized polygon
size_t
lwpoly_serialize_size(LWPOLY *poly)
{
	size_t size = 1; // type
	uint32 i;

	if ( poly->SRID != -1 ) size += 4; // SRID
	if ( poly->hasbbox ) size += sizeof(BOX2DFLOAT4);

	size += 4; // nrings

	for (i=0; i<poly->nrings; i++)
	{
		size += 4; // npoints
		size += poly->rings[i]->npoints*poly->ndims*sizeof(double);
	}

	return size;
}

void pfree_polygon  (LWPOLY  *poly)
{
	int t;

	for (t=0;t<poly->nrings;t++)
	{
		pfree_POINTARRAY(poly->rings[t]);
	}

	lwfree(poly);
}

void printLWPOLY(LWPOLY *poly)
{
	int t;
	lwnotice("LWPOLY {");
	lwnotice("    ndims = %i", (int)poly->ndims);
	lwnotice("    SRID = %i", (int)poly->SRID);
	lwnotice("    nrings = %i", (int)poly->nrings);
	for (t=0;t<poly->nrings;t++)
	{
		lwnotice("    RING # %i :",t);
		printPA(poly->rings[t]);
	}
	lwnotice("}");
}

int
lwpoly_compute_bbox_p(LWPOLY *poly, BOX2DFLOAT4 *box)
{
	BOX2DFLOAT4 boxbuf;
	uint32 i;

	if ( ! poly->nrings ) return 0;
	if ( ! ptarray_compute_bbox_p(poly->rings[0], box) ) return 0;
	for (i=1; i<poly->nrings; i++)
	{
		if ( ! ptarray_compute_bbox_p(poly->rings[0], &boxbuf) )
			return 0;
		if ( ! box2d_union_p(box, &boxbuf, box) )
			return 0;
	}
	return 1;
}

// Clone LWLINE object. POINTARRAY are not copied, it's ring array is.
LWPOLY *
lwpoly_clone(const LWPOLY *g)
{
	LWPOLY *ret = lwalloc(sizeof(LWPOLY));
	memcpy(ret, g, sizeof(LWPOLY));
	ret->rings = lwalloc(sizeof(POINTARRAY *)*g->nrings);
	memcpy(ret->rings, g->rings, sizeof(POINTARRAY *)*g->nrings);
	return ret;
}

// Add 'what' to this poly at position 'where'.
// where=0 == prepend
// where=-1 == append
// Returns a MULTIPOLYGON or a GEOMETRYCOLLECTION
LWGEOM *
lwpoly_add(const LWPOLY *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;

	if ( where != -1 && where != 0 )
	{
		lwerror("lwpoly_add only supports 0 or -1 as second argument, got %d", where);
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
	geoms[0]->hasbbox = geoms[1]->hasbbox = 0;

	// Find appropriate geom type
	if ( what->type == POLYGONTYPE ) newtype = MULTIPOLYGONTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype, to->ndims, to->SRID,
		(what->hasbbox || to->hasbbox ), 2, geoms);
	
	return (LWGEOM *)col;
}
