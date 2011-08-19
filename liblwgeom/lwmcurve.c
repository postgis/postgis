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

LWMCURVE *
lwmcurve_deserialize(uint8_t *srl)
{
	LWMCURVE *result;
	LWGEOM_INSPECTED *insp;
	int stype;
	uint8_t type = (uint8_t)srl[0];
	int geomtype = lwgeom_getType(type);
	int i;

	if (geomtype != MULTICURVETYPE)
	{
		lwerror("lwmcurve_deserialize called on NON multicurve: %d - %s", geomtype, lwtype_name(geomtype));
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMCURVE));
	result->type = MULTICURVETYPE;
	result->flags = gflags(TYPE_HASZ(type), TYPE_HASM(type), 0);
	result->srid = insp->srid;
	result->ngeoms = insp->ngeometries;

	if ( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);
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
	else result->bbox = NULL;

	for (i = 0; i < insp->ngeometries; i++)
	{
		stype = lwgeom_getType(insp->sub_geoms[i][0]);
		if (stype == CIRCSTRINGTYPE)
		{
			result->geoms[i] = (LWGEOM *)lwcircstring_deserialize(insp->sub_geoms[i]);
		}
		else if (stype == LINETYPE)
		{
			result->geoms[i] = (LWGEOM *)lwline_deserialize(insp->sub_geoms[i]);
		}
		else if (stype == COMPOUNDTYPE)
		{
			result->geoms[i] = (LWGEOM *)lwcompound_deserialize(insp->sub_geoms[i]);
		}
		else
		{
			lwerror("Only Circular strings, Line strings or Compound curves are permitted in a MultiCurve.");

			lwfree(result);
			lwfree(insp);
			return NULL;
		}

		if (FLAGS_NDIMS(result->geoms[i]->flags) != FLAGS_NDIMS(result->flags))
		{
			lwerror("Mixed dimensions (multicurve: %d, curve %d:%d)",
			        FLAGS_NDIMS(result->flags), i,
			        FLAGS_NDIMS(result->geoms[i]->flags));
			lwfree(result);
			lwfree(insp);
			return NULL;
		}
	}
	lwinspected_release(insp);

	return result;
}



