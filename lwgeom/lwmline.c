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
	result->hasbbox = lwgeom_hasBBOX(insp->type);
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

