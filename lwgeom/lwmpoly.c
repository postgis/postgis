#include <stdio.h>
#include <stdlib.h>
#include "liblwgeom.h"

LWMPOLY *
lwmpoly_deserialize(char *srl)
{
	LWMPOLY *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	if ( type != MULTIPOLYGONTYPE ) 
	{
		lwerror("lwmpoly_deserialize called on NON multipoly: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMPOLY));
	result->SRID = insp->SRID;
	result->ndims = lwgeom_ndims(insp->type);
	result->npolys = insp->ngeometries;
	result->polys = lwalloc(sizeof(LWPOLY *)*insp->ngeometries);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->polys[i] = lwpoly_deserialize(insp->sub_geoms[i]);
		if ( result->polys[i]->ndims != result->ndims )
		{
			lwerror("Mixed dimensions (multipoly:%d, poly%d:%d)",
				result->ndims, i, result->polys[i]->ndims);
			return NULL;
		}
	}

	return result;
}

