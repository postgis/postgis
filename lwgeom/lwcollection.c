#include <stdio.h>
#include <stdlib.h>
#include "liblwgeom.h"

LWCOLLECTION *
lwcollection_deserialize(char *srl)
{
	LWCOLLECTION *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	if ( type != COLLECTIONTYPE ) 
	{
		lwerror("lwmpoly_deserialize called on NON multipoly: %d",
			type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWCOLLECTION));
	result->SRID = insp->SRID;
	result->ndims = lwgeom_ndims(insp->type);
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = (struct LWGEOM *)lwgeom_deserialize(insp->sub_geoms[i]);
	}

	return result;
}

