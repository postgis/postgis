#include <stdio.h>
#include <stdlib.h>
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
	result->SRID = insp->SRID;
	result->ndims = lwgeom_ndims(insp->type);
	result->npoints = insp->ngeometries;
	result->points = lwalloc(sizeof(LWPOINT *)*result->npoints);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->points[i] = lwpoint_deserialize(insp->sub_geoms[i]);
		if ( result->points[i]->ndims != result->ndims )
		{
			lwerror("Mixed dimensions (multipoint:%d, point%d:%d)",
				result->ndims, i, result->points[i]->ndims);
			return NULL;
		}
	}

	return result;
}

