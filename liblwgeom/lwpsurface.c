/**********************************************************************
 * $Id:$
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


LWPSURFACE *
lwpsurface_deserialize(uchar *srl)
{
	LWPSURFACE *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	LWDEBUG(2, "lwmpoly_deserialize called");

	if ( type != POLYHEDRALSURFACETYPE )
	{
		lwerror("lwpsurface_deserialize called on NON polyhedralsurface: %d",
		        type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWPSURFACE));
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;
	
	if( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWPSURFACE *)*insp->ngeometries);
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
		result->geoms[i] = (LWGEOM *) lwpoly_deserialize(insp->sub_geoms[i]);
		if ( TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type) )
		{
			lwerror("Mixed dimensions (polyhedralsurface:%d, face%d:%d)",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->geoms[i]->type)
			       );
			return NULL;
		}
	}

	return result;
}


LWPSURFACE* lwpsurface_add_lwpoly(LWPSURFACE *mobj, const LWPOLY *obj)
{
        return (LWPSURFACE*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
}


void lwpsurface_free(LWPSURFACE *psurf)
{
	int i;

	if ( psurf->bbox )
	{
		lwfree(psurf->bbox);
	}
	for ( i = 0; i < psurf->ngeoms; i++ )
	{
		if ( psurf->geoms[i] )
		{
			lwpoly_free((LWPOLY *) psurf->geoms[i]);
		}
	}
	if ( psurf->geoms )
	{
		lwfree(psurf->geoms);
	}
	lwfree(psurf);
}


void printLWPSURFACE(LWPSURFACE *psurf)
{
	int i, j;
	LWPOLY *patch;

	if (TYPE_GETTYPE(psurf->type) != POLYHEDRALSURFACETYPE)
        	lwerror("printLWPSURFACE called with something else than a POLYHEDRALSURFACE");

        lwnotice("LWPSURFACE {");
        lwnotice("    ndims = %i", (int)TYPE_NDIMS(psurf->type));
        lwnotice("    SRID = %i", (int)psurf->SRID);
        lwnotice("    ngeoms = %i", (int)psurf->ngeoms);

        for (i=0; i<psurf->ngeoms; i++)
        {
		patch = (LWPOLY *) psurf->geoms[i];
        	for (j=0; j<patch->nrings; j++) {
			lwnotice("    RING # %i :",j);
                	printPA(patch->rings[j]);
		}
        }
        lwnotice("}");
}
