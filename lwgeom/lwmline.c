#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

LWMLINE *
lwmline_deserialize(char *srl)
{
	LWMLINE *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	if ( type != MULTILINETYPE ) 
	{
		lwerror("lwmline_deserialize called on NON multiline: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMLINE));
	result->type = MULTILINETYPE;
	result->SRID = insp->SRID;
	result->ndims = lwgeom_ndims(insp->type);
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWLINE *)*insp->ngeometries);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwline_deserialize(insp->sub_geoms[i]);
		if ( result->geoms[i]->ndims != result->ndims )
		{
			lwerror("Mixed dimensions (multiline:%d, line%d:%d)",
				result->ndims, i, result->geoms[i]->ndims);
			return NULL;
		}
	}

	return result;
}

// find serialized size of this mline
size_t
lwmline_serialize_size(LWMLINE *mline)
{
	size_t size = 5; // type + nsubgeoms
	int i;

	if ( mline->SRID != -1 ) size += 4; // SRID

	for (i=0; i<mline->ngeoms; i++)
		size += lwline_serialize_size(mline->geoms[i]);

	return size; 
}

// convert this multiline into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
void
lwmline_serialize_buf(LWMLINE *mline, char *buf, int *retsize)
{
	int size=1; // type 
	int subsize=0;
	char hasSRID;
	char *loc;
	int i;

	hasSRID = (mline->SRID != -1);

	buf[0] = (unsigned char) lwgeom_makeType(mline->ndims,
		hasSRID, MULTILINETYPE);
	loc = buf+1;

	// Add SRID if requested
	if (hasSRID)
	{
		memcpy(loc, &mline->SRID, 4);
		size += 4; 
		loc += 4;
	}

	// Write number of subgeoms
	memcpy(loc, &mline->ngeoms, 4);
	size += 4;
	loc += 4;

	// Serialize subgeoms
	for (i=0; i<mline->ngeoms; i++)
	{
		lwline_serialize_buf(mline->geoms[i], loc, &subsize);
		size += subsize;
	}

	if (retsize) *retsize = size;
}
