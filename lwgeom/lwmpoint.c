#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

LWMPOINT *
lwmpoint_deserialize(char *srl)
{
	LWMPOINT *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	if ( type != MULTIPOINTTYPE ) 
	{
		lwerror("lwmpoint_deserialize called on NON multipoint: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMPOINT));
	result->type = MULTIPOINTTYPE;
	result->SRID = insp->SRID;
	result->ndims = lwgeom_ndims(insp->type);
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWPOINT *)*result->ngeoms);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwpoint_deserialize(insp->sub_geoms[i]);
		if ( result->geoms[i]->ndims != result->ndims )
		{
			lwerror("Mixed dimensions (multipoint:%d, point%d:%d)",
				result->ndims, i, result->geoms[i]->ndims);
			return NULL;
		}
	}

	return result;
}

// find serialized size of this mpoint
size_t
lwmpoint_serialize_size(LWMPOINT *mpoint)
{
	size_t size = 5; // type + nsubgeoms
	int i;

	if ( mpoint->SRID != -1 ) size += 4; // SRID

	for (i=0; i<mpoint->ngeoms; i++)
		size += lwpoint_serialize_size(mpoint->geoms[i]);

	return size; 
}

// convert this multipoint into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
void
lwmpoint_serialize_buf(LWMPOINT *mpoint, char *buf, int *retsize)
{
	int size=1; // type 
	int subsize=0;
	char hasSRID;
	char *loc;
	int i;

	hasSRID = (mpoint->SRID != -1);

	buf[0] = (unsigned char) lwgeom_makeType(mpoint->ndims,
		hasSRID, MULTIPOINTTYPE);
	loc = buf+1;

	// Add SRID if requested
	if (hasSRID)
	{
		memcpy(loc, &mpoint->SRID, 4);
		size += 4; 
		loc += 4;
	}

	// Write number of subgeoms
	memcpy(loc, &mpoint->ngeoms, 4);
	size += 4;
	loc += 4;

	// Serialize subgeoms
	for (i=0; i<mpoint->ngeoms; i++)
	{
		lwpoint_serialize_buf(mpoint->geoms[i], loc, &subsize);
		size += subsize;
	}

	if (retsize) *retsize = size;
}
