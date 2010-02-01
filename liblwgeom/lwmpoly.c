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

/**
 * Add 'what' to this multiline at position 'where'.
 * @param where if 0 == prepend, if = -1 then append
 * @return a {@link #MULTIPOLY} or a {@link #COLLECTION}
 */
LWGEOM *
lwmpoly_add(const LWMPOLY *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;
	uint32 i;

	if ( where == -1 ) where = to->ngeoms;
	else if ( where < -1 || where > to->ngeoms )
	{
		lwerror("lwmline_add: add position out of range %d..%d",
		        -1, to->ngeoms);
		return NULL;
	}

	/* dimensions compatibility are checked by caller */

	/* Construct geoms array */
	geoms = lwalloc(sizeof(LWGEOM *)*(to->ngeoms+1));
	for (i=0; i<where; i++)
	{
		geoms[i] = lwgeom_clone((LWGEOM *)to->geoms[i]);
	}
	geoms[where] = lwgeom_clone(what);
	for (i=where; i<to->ngeoms; i++)
	{
		geoms[i+1] = lwgeom_clone((LWGEOM *)to->geoms[i]);
	}

	if ( TYPE_GETTYPE(what->type) == POLYGONTYPE ) newtype = MULTIPOLYGONTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
	                             to->SRID, NULL,
	                             to->ngeoms+1, geoms);

	return (LWGEOM *)col;

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

