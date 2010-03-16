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


LWMSURFACE *
lwmsurface_deserialize(uchar *srl)
{
	LWMSURFACE *result;
	LWGEOM_INSPECTED *insp;
	int stype;
	int type = lwgeom_getType(srl[0]);
	int i;

	LWDEBUG(2, "lwmsurface_deserialize called");

	if (type != MULTISURFACETYPE)
	{
		lwerror("lwmsurface_deserialize called on a non-multisurface: %d", type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMSURFACE));
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
		memcpy(result->bbox, srl + 1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;

	for (i = 0; i < insp->ngeometries; i++)
	{
		stype = lwgeom_getType(insp->sub_geoms[i][0]);
		if (stype == POLYGONTYPE)
		{
			result->geoms[i] = (LWGEOM *)lwpoly_deserialize(insp->sub_geoms[i]);
		}
		else if (stype == CURVEPOLYTYPE)
		{
			result->geoms[i] = (LWGEOM *)lwcurvepoly_deserialize(insp->sub_geoms[i]);
		}
		else
		{
			lwerror("Only Polygons and Curved Polygons are supported in a MultiSurface.");
			lwfree(result);
			lwfree(insp);
			return NULL;
		}

		if (TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type))
		{
			lwerror("Mixed dimensions (multisurface: %d, surface %d:%d",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->geoms[i]->type));
			lwfree(result);
			lwfree(insp);
			return NULL;
		}
	}
	return result;
}


