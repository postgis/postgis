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
	result->hasbbox = lwgeom_hasBBOX(insp->type);
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

