#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

//#define DEBUG_CALLS 1

LWMPOLY *
lwmpoly_deserialize(char *srl)
{
	LWMPOLY *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

#ifdef DEBUG_CALLS
	lwnotice("lwmpoly_deserialize called");
#endif

	if ( type != MULTIPOLYGONTYPE ) 
	{
		lwerror("lwmpoly_deserialize called on NON multipoly: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMPOLY));
	result->type = MULTIPOLYGONTYPE;
	result->SRID = insp->SRID;
	result->hasbbox = lwgeom_hasBBOX(insp->type);
	result->ndims = lwgeom_ndims(insp->type);
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWPOLY *)*insp->ngeometries);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwpoly_deserialize(insp->sub_geoms[i]);
		if ( result->geoms[i]->ndims != result->ndims )
		{
			lwerror("Mixed dimensions (multipoly:%d, poly%d:%d)",
				result->ndims, i, result->geoms[i]->ndims);
			return NULL;
		}
	}

	return result;
}

// find serialized size of this mpoly
size_t
lwmpoly_serialize_size(LWMPOLY *mpoly)
{
	size_t size = 5; // type + nsubgeoms
	int i;

	if ( mpoly->SRID != -1 ) size += 4; // SRID

	for (i=0; i<mpoly->ngeoms; i++)
		size += lwpoly_serialize_size(mpoly->geoms[i]);

	return size; 
}

// convert this multipolygon into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
void
lwmpoly_serialize_buf(LWMPOLY *mpoly, char *buf, int *retsize)
{
	int size=1; // type 
	int subsize=0;
	char hasSRID;
	char *loc;
	int i;

	hasSRID = (mpoly->SRID != -1);

	buf[0] = (unsigned char) lwgeom_makeType(mpoly->ndims,
		hasSRID, MULTIPOLYGONTYPE);
	loc = buf+1;

	// Add SRID if requested
	if (hasSRID)
	{
		memcpy(loc, &mpoly->SRID, 4);
		size += 4; 
		loc += 4;
	}

	// Write number of subgeoms
	memcpy(loc, &mpoly->ngeoms, 4);
	size += 4;
	loc += 4;

	// Serialize subgeoms
	for (i=0; i<mpoly->ngeoms; i++)
	{
		lwpoly_serialize_buf(mpoly->geoms[i], loc, &subsize);
		size += subsize;
	}

	if (retsize) *retsize = size;
}
