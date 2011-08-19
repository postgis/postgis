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
#include "liblwgeom_internal.h"

void
lwmpoint_release(LWMPOINT *lwmpoint)
{
	lwgeom_release(lwmpoint_as_lwgeom(lwmpoint));
}

LWMPOINT *
lwmpoint_construct_empty(int srid, char hasz, char hasm)
{
	LWMPOINT *ret = (LWMPOINT*)lwcollection_construct_empty(MULTIPOINTTYPE, srid, hasz, hasm);
	return ret;
}


LWMPOINT *
lwmpoint_deserialize(uint8_t *srl)
{
	LWMPOINT *result;
	LWGEOM_INSPECTED *insp;
	uint8_t type = (uint8_t)srl[0];
	int geomtype = lwgeom_getType(type);
	int i;

	if ( geomtype != MULTIPOINTTYPE )
	{
		lwerror("lwmpoint_deserialize called on NON multipoint: %d - %s",
		        geomtype, lwtype_name(geomtype));
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMPOINT));
	result->type = geomtype;
	result->flags = gflags(TYPE_HASZ(type), TYPE_HASM(type), 0);
	result->srid = insp->srid;
	result->ngeoms = insp->ngeometries;

	if ( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWPOINT *)*insp->ngeometries);
	}
	else
	{
		result->geoms = NULL;
	}

	if (lwgeom_hasBBOX(type))
	{
		BOX2DFLOAT4 *box2df;

		FLAGS_SET_BBOX(result->flags, 1);
		box2df = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(box2df, srl+1, sizeof(BOX2DFLOAT4));
		result->bbox = gbox_from_box2df(result->flags, box2df);
		lwfree(box2df);
	}
	else
	{
		result->bbox = NULL;
	}

	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwpoint_deserialize(insp->sub_geoms[i]);
		if ( FLAGS_NDIMS(result->geoms[i]->flags) != FLAGS_NDIMS(result->flags) )
		{
			lwerror("Mixed dimensions (multipoint:%d, point%d:%d)",
			        FLAGS_NDIMS(result->flags), i,
			        FLAGS_NDIMS(result->geoms[i]->flags)
			       );
			return NULL;
		}
	}
	lwinspected_release(insp);

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
		lwfree(mpt->bbox);

	for ( i = 0; i < mpt->ngeoms; i++ )
		if ( mpt->geoms && mpt->geoms[i] )
			lwpoint_free(mpt->geoms[i]);

	if ( mpt->geoms )
		lwfree(mpt->geoms);

	lwfree(mpt);
}

LWGEOM*
lwmpoint_remove_repeated_points(LWMPOINT *mpoint)
{
	uint32_t nnewgeoms;
	uint32_t i, j;
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
	                                       mpoint->srid, mpoint->bbox ? gbox_copy(mpoint->bbox) : NULL,
	                                       nnewgeoms, newgeoms);

}

