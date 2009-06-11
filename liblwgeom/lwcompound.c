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

LWCOMPOUND *
lwcompound_deserialize(uchar *serialized)
{
	LWCOMPOUND *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(serialized[0]);
	int i;

	if (type != COMPOUNDTYPE)
	{
		lwerror("lwcompound_deserialize called on non compound: %d", type);
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

/**
 * Add 'what' to this string at position 'where'
 * @param where if = 0 then prepend, if -1  then append
 * @param what an #LWGEOM object
 * @return a {@link #LWCOMPOUND} or a {@link #GEOMETRYCOLLECTION}
 */
LWGEOM *
lwcompound_add(const LWCOMPOUND *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;

	LWDEBUG(2, "lwcompound_add called.");

	if (where != -1 && where != 0)
	{
		lwerror("lwcompound_add only supports 0 or -1 as a second argument, not %d", where);
		return NULL;
	}

	/* dimensions compatibility are checked by caller */

	/* Construct geoms array */
	geoms = lwalloc(sizeof(LWGEOM *)*2);
	if (where == -1) /* append */
	{
		geoms[0] = lwgeom_clone((LWGEOM *)to);
		geoms[1] = lwgeom_clone(what);
	}
	else /* prepend */
	{
		geoms[0] = lwgeom_clone(what);
		geoms[1] = lwgeom_clone((LWGEOM *)to);
	}

	/* reset SRID and wantbbox flag from component types */
	geoms[0]->SRID = geoms[1]->SRID = -1;
	TYPE_SETHASSRID(geoms[0]->type, 0);
	TYPE_SETHASSRID(geoms[1]->type, 0);
	TYPE_SETHASBBOX(geoms[0]->type, 0);
	TYPE_SETHASBBOX(geoms[1]->type, 0);

	/* Find appropriate geom type */
	if (TYPE_GETTYPE(what->type) == LINETYPE || TYPE_GETTYPE(what->type) == CIRCSTRINGTYPE) newtype = COMPOUNDTYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
	                             to->SRID, NULL, 2, geoms);

	return (LWGEOM *)col;
}

