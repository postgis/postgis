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
	int size=1;  // type byte
	char hasSRID;
	char *result;
	int t,u;
	int total_points = 0;
	int npoints;
	char *loc;

	hasSRID = (poly->SRID != -1);

	if (hasSRID)
		size +=4;  //4 byte SRID

	size += 4; // nrings
	size += 4*poly->nrings; //npoints/ring


	for (t=0;t<poly->nrings;t++)
	{
		total_points  += poly->rings[t]->npoints;
	}
	if (poly->ndims == 3)
		size += 24*total_points;
	else if (poly->ndims == 2)
		size += 16*total_points;
	else if (poly->ndims == 4)
		size += 32*total_points;

	result = lwalloc(size);

	result[0] = (unsigned char) lwgeom_makeType(poly->ndims,hasSRID, POLYGONTYPE);
	loc = result+1;

	if (hasSRID)
	{
		memcpy(loc, &poly->SRID, sizeof(int32));
		loc += 4;
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

	if (hasSRID) size +=4;  //4 byte SRID

	size += 4; // nrings
	size += 4*poly->nrings; //npoints/ring

	for (t=0;t<poly->nrings;t++)
	{
		total_points  += poly->rings[t]->npoints;
	}
	if (poly->ndims == 3) size += 24*total_points;
	else if (poly->ndims == 2) size += 16*total_points;
	else if (poly->ndims == 4) size += 32*total_points;

	buf[0] = (unsigned char) lwgeom_makeType(poly->ndims,
		hasSRID, POLYGONTYPE);
	loc = buf+1;

	if (hasSRID)
	{
		memcpy(loc, &poly->SRID, sizeof(int32));
		loc += 4;
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
uint32
lwpoly_size(LWPOLY *poly)
{
	uint32 size = 1; // type
	uint32 i;

	if ( poly->SRID != -1 ) size += 4; // SRID

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

