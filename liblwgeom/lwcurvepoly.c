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

/* basic LWCURVEPOLY manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"


LWCURVEPOLY *
lwcurvepoly_deserialize(uchar *srl)
{
	LWCURVEPOLY *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	LWDEBUG(3, "lwcurvepoly_deserialize called.");

	if (type != CURVEPOLYTYPE)
	{
		lwerror("lwcurvepoly_deserialize called on NON curvepoly: %d",
		        type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWCURVEPOLY));
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->nrings = insp->ngeometries;
	result->rings = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);

	if (lwgeom_hasBBOX(srl[0]))
	{
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, srl + 1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;

	for (i = 0; i < insp->ngeometries; i++)
	{
		result->rings[i] = lwgeom_deserialize(insp->sub_geoms[i]);
		if (lwgeom_getType(result->rings[i]->type) != CIRCSTRINGTYPE
		        && lwgeom_getType(result->rings[i]->type) != LINETYPE
		        && lwgeom_getType(result->rings[i]->type) != COMPOUNDTYPE)
		{
			lwerror("Only Circular curves, Linestrings and Compound curves are supported as rings, not %s (%d)", lwgeom_typename(result->rings[i]->type), result->rings[i]->type);
			lwfree(result);
			lwfree(insp);
			return NULL;
		}
		if (TYPE_NDIMS(result->rings[i]->type) != TYPE_NDIMS(result->type))
		{
			lwerror("Mixed dimensions (curvepoly %d, ring %d)",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->rings[i]->type));
			lwfree(result);
			lwfree(insp);
			return NULL;
		}
	}
	return result;
}

LWGEOM *
lwcurvepoly_add(const LWCURVEPOLY *to, uint32 where, const LWGEOM *what)
{
	/* TODO */
	lwerror("lwcurvepoly_add not yet implemented.");
	return NULL;
}



