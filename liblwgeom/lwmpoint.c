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
lwmpoint_release(LWMPOINT *lwmpoint)
{
	lwgeom_release(lwmpoint_as_lwgeom(lwmpoint));
}


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

	if ( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWPOINT *)*insp->ngeometries);
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
	else
	{
		result->bbox = NULL;
	}

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

void lwmpoint_free(LWMPOINT *mpt)
{
	int i;
	if ( mpt->bbox )
	{
		lwfree(mpt->bbox);
	}
	for ( i = 0; i < mpt->ngeoms; i++ )
	{
		if ( mpt->geoms[i] )
		{
			lwpoint_free(mpt->geoms[i]);
		}
	}
	if ( mpt->geoms )
	{
		lwfree(mpt->geoms);
	}
	lwfree(mpt);

}

LWGEOM*
lwmpoint_remove_repeated_points(LWMPOINT *mpoint)
{
	unsigned int nnewgeoms;
	unsigned int i, j;
	LWGEOM **newgeoms;

	newgeoms = lwalloc(sizeof(LWGEOM *)*mpoint->ngeoms);
	nnewgeoms = 0;
	for (i=0; i<mpoint->ngeoms; ++i)
	{
		/* Brute force, may be optimized by building an index */
		int seen=0;
		for (j=0; j<nnewgeoms; ++j)
		{
			if ( lwpoint_same((LWPOINT*)newgeoms[j],
			                  (LWPOINT*)mpoint->geoms[i]) )
			{
				seen=1;
				break;
			}
		}
		if ( seen ) continue;
		newgeoms[nnewgeoms++] = (LWGEOM*)lwpoint_clone(mpoint->geoms[i]);
	}

	return (LWGEOM*)lwcollection_construct(mpoint->type,
	                                       mpoint->SRID, mpoint->bbox ? box2d_clone(mpoint->bbox) : NULL,
	                                       nnewgeoms, newgeoms);

}

