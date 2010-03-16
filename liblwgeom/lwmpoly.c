/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"


void
lwmpoly_release(LWMPOLY *lwmpoly)
{
	lwgeom_release(lwmpoly_as_lwgeom(lwmpoly));
}

LWMPOLY *
lwmpoly_construct_empty(int srid, char hasz, char hasm)
{
	LWMPOLY *ret = (LWMPOLY*)lwcollection_construct_empty(srid, hasz, hasm);
	TYPE_SETTYPE(ret->type, MULTIPOLYGONTYPE);
	return ret;
}

LWMPOLY *
lwmpoly_deserialize(uchar *srl)
{
	LWMPOLY *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	LWDEBUG(2, "lwmpoly_deserialize called");

	if ( type != MULTIPOLYGONTYPE )
	{
		lwerror("lwmpoly_deserialize called on NON multipoly: %d",
		        type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMPOLY));
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;

	if ( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWPOLY *)*insp->ngeometries);
	}
	else
	{
		result->geoms = NULL;
	}

	if (lwgeom_hasBBOX(srl[0]))
	{
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, srl+1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwpoly_deserialize(insp->sub_geoms[i]);
		if ( TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type) )
		{
			lwerror("Mixed dimensions (multipoly:%d, poly%d:%d)",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->geoms[i]->type)
			       );
			return NULL;
		}
	}

	return result;
}

LWMPOLY* lwmpoly_add_lwpoly(LWMPOLY *mobj, const LWPOLY *obj)
{
	return (LWMPOLY*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
}


void lwmpoly_free(LWMPOLY *mpoly)
{
	int i;
	if ( mpoly->bbox )
	{
		lwfree(mpoly->bbox);
	}
	for ( i = 0; i < mpoly->ngeoms; i++ )
	{
		if ( mpoly->geoms[i] )
		{
			lwpoly_free(mpoly->geoms[i]);
		}
	}
	if ( mpoly->geoms )
	{
		lwfree(mpoly->geoms);
	}
	lwfree(mpoly);

}

