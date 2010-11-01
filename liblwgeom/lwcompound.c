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

LWCOMPOUND *
lwcompound_deserialize(uchar *serialized)
{
	LWCOMPOUND *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(serialized[0]);
	int i;

	if (type != COMPOUNDTYPE)
	{
		lwerror("lwcompound_deserialize called on non compound: %d - %s", type, lwtype_name(type));
		return NULL;
	}

	insp = lwgeom_inspect(serialized);

	result = lwalloc(sizeof(LWCOMPOUND));
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);

	if (lwgeom_hasBBOX(serialized[0]))
	{
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, serialized + 1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;

	for (i = 0; i < insp->ngeometries; i++)
	{
		if (lwgeom_getType(insp->sub_geoms[i][0]) == LINETYPE)
			result->geoms[i] = (LWGEOM *)lwline_deserialize(insp->sub_geoms[i]);
		else
			result->geoms[i] = (LWGEOM *)lwcircstring_deserialize(insp->sub_geoms[i]);
		if (TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type))
		{
			lwerror("Mixed dimensions (compound: %d, line/circularstring %d:%d)",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->geoms[i]->type)
			       );
			lwfree(result);
			return NULL;
		}
	}
	return result;
}

int
lwcompound_is_closed(LWCOMPOUND *compound)
{
	size_t size;
	int npoints=0;

	if (!TYPE_HASZ(compound->type))
		size = sizeof(POINT2D);
	else    size = sizeof(POINT3D);

	if      (lwgeom_getType(compound->geoms[compound->ngeoms - 1]->type) == CIRCSTRINGTYPE)
		npoints = ((LWCIRCSTRING *)compound->geoms[compound->ngeoms - 1])->points->npoints;
	else if (lwgeom_getType(compound->geoms[compound->ngeoms - 1]->type) == LINETYPE)
		npoints = ((LWLINE *)compound->geoms[compound->ngeoms - 1])->points->npoints;

	if ( memcmp(getPoint_internal( (POINTARRAY *)compound->geoms[0]->data, 0),
	            getPoint_internal( (POINTARRAY *)compound->geoms[compound->ngeoms - 1]->data,
	                               npoints - 1),
	            size) ) return 0;

	return 1;
}
