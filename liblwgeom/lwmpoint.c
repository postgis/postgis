/**********************************************************************
 * $Id: lwmpoint.c 2369 2006-05-30 08:38:58Z strk $
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

LWMPOINT *
lwmpoint_deserialize(uchar *srl)
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
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWPOINT *)*result->ngeoms);

	if (lwgeom_hasBBOX(srl[0]))
	{
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, srl+1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwpoint_deserialize(insp->sub_geoms[i]);
		if ( TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type) )
		{
			lwerror("Mixed dimensions (multipoint:%d, point%d:%d)",
				TYPE_NDIMS(result->type), i,
				TYPE_NDIMS(result->geoms[i]->type)
			);
			return NULL;
		}
	}

	return result;
}

/*
 * Add 'what' to this multipoint at position 'where'.
 * where=0 == prepend
 * where=-1 == append
 * Returns a MULTIPOINT or a COLLECTION
 */
LWGEOM *
lwmpoint_add(const LWMPOINT *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;
	uint32 i;

	if ( where == -1 ) where = to->ngeoms;
	else if ( where < -1 || where > to->ngeoms )
	{
		lwerror("lwmpoint_add: add position out of range %d..%d",
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

	if ( TYPE_GETTYPE(what->type) == POINTTYPE ) newtype = MULTIPOINTTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
		to->SRID, NULL,
		to->ngeoms+1, geoms);
	
	return (LWGEOM *)col;

}
