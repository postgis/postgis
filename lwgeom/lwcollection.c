#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

LWCOLLECTION *
lwcollection_deserialize(char *srl)
{
	LWCOLLECTION *result;
	LWGEOM_INSPECTED *insp;
	char typefl = srl[0];
	int type = lwgeom_getType(typefl);
	int i;

	if ( type != COLLECTIONTYPE ) 
	{
		lwerror("lwmpoly_deserialize called on NON multipoly: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWCOLLECTION));
	result->type = COLLECTIONTYPE;
	result->hasbbox = lwgeom_hasBBOX(typefl);
	result->ndims = lwgeom_ndims(typefl);
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwgeom_deserialize(insp->sub_geoms[i]);
	}

	return result;
}

LWGEOM *
lwcollection_getsubgeom(LWCOLLECTION *col, int gnum)
{
	return (LWGEOM *)col->geoms[gnum];
}

// find serialized size of this collection
size_t
lwcollection_serialize_size(LWCOLLECTION *col)
{
	size_t size = 5; // type + nsubgeoms
	int i;

	if ( col->SRID != -1 ) size += 4; // SRID
	if ( col->hasbbox ) size += sizeof(BOX2DFLOAT4);

	for (i=0; i<col->ngeoms; i++)
		size += lwgeom_serialize_size(lwcollection_getsubgeom(col, i));

	return size; 
}

// convert this collectoin into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
void
lwcollection_serialize_buf(LWCOLLECTION *coll, char *buf, int *retsize)
{
	int size=1; // type 
	int subsize=0;
	char hasSRID;
	char *loc;
	int i;

	hasSRID = (coll->SRID != -1);

	buf[0] = (unsigned char) lwgeom_makeType(coll->ndims,
		hasSRID, COLLECTIONTYPE);
	loc = buf+1;

	// Add BBOX if requested
	if ( coll->hasbbox )
	{
		lwgeom_compute_bbox_p((LWGEOM *)coll, (BOX2DFLOAT4 *)loc);
		size += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	// Add SRID if requested
	if (hasSRID)
	{
		memcpy(loc, &coll->SRID, 4);
		size += 4; 
		loc += 4;
	}

	// Write number of subgeoms
	memcpy(loc, &coll->ngeoms, 4);
	size += 4;
	loc += 4;

	// Serialize subgeoms
	for (i=0; i<coll->ngeoms; i++)
	{
		lwgeom_serialize_buf(coll->geoms[i], loc, &subsize);
		size += subsize;
	}

	if (retsize) *retsize = size;
}

int
lwcollection_compute_bbox_p(LWCOLLECTION *col, BOX2DFLOAT4 *box)
{
	BOX2DFLOAT4 boxbuf;
	uint32 i;

	if ( ! col->ngeoms ) return 0;
	if ( ! lwgeom_compute_bbox_p(col->geoms[0], box) ) return 0;
	for (i=1; i<col->ngeoms; i++)
	{
		if ( ! lwgeom_compute_bbox_p(col->geoms[i], &boxbuf) ) return 0;
		if ( ! box2d_union_p(box, &boxbuf, box) ) return 0;
	}
	return 1;
}
