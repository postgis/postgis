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
lwmpoint_construct_empty(int srid, char hasz, char hasm)
{
	LWMPOINT *ret = (LWMPOINT*)lwcollection_construct_empty(srid, hasz, hasm);
	TYPE_SETTYPE(ret->type, MULTIPOINTTYPE);
	return ret;
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

LWMPOINT* lwmpoint_add_lwpoint(LWMPOINT *mobj, const LWPOINT *obj)
{
	LWDEBUG(4, "Called");
	return (LWMPOINT*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
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
	uint32 nnewgeoms;
	uint32 i, j;
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

