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

