/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

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

LWMPOINT* lwmpoint_add_lwpoint(LWMPOINT *mobj, const LWPOINT *obj)
{
	LWDEBUG(4, "Called");
	return (LWMPOINT*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
}

LWMPOINT *
lwmpoint_construct(int srid, const POINTARRAY *pa)
{
	int i;
	int hasz = ptarray_has_z(pa);
	int hasm = ptarray_has_m(pa);
	LWMPOINT *ret = (LWMPOINT*)lwcollection_construct_empty(MULTIPOINTTYPE, srid, hasz, hasm);

	for ( i = 0; i < pa->npoints; i++ )
	{
		LWPOINT *lwp;
		POINT4D p;
		getPoint4d_p(pa, i, &p);
		lwp = lwpoint_make(srid, hasz, hasm, &p);
		lwmpoint_add_lwpoint(ret, lwp);
	}

	return ret;
}


void lwmpoint_free(LWMPOINT *mpt)
{
	int i;

	if ( ! mpt ) return;

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
lwmpoint_remove_repeated_points(const LWMPOINT *mpoint, double tolerance)
{
	uint32_t nnewgeoms;
	uint32_t i, j;
	LWGEOM **newgeoms;
	LWGEOM *lwpt1, *lwpt2;

	newgeoms = lwalloc(sizeof(LWGEOM *)*mpoint->ngeoms);
	nnewgeoms = 0;
	for (i=0; i<mpoint->ngeoms; ++i)
	{
		lwpt1 = (LWGEOM*)mpoint->geoms[i];
		/* Brute force, may be optimized by building an index */
		int seen=0;
		for (j=0; j<nnewgeoms; ++j)
		{
			lwpt2 = (LWGEOM*)newgeoms[j];
			if ( lwgeom_mindistance2d(lwpt1, lwpt2) <= tolerance )
			{
				seen=1;
				break;
			}
		}
		if ( seen ) continue;
		newgeoms[nnewgeoms++] = lwgeom_clone_deep(lwpt1);
	}

	return (LWGEOM*)lwcollection_construct(mpoint->type,
	                                       mpoint->srid,
										   mpoint->bbox ? gbox_copy(mpoint->bbox) : NULL,
	                                       nnewgeoms, newgeoms);

}

LWMPOINT*
lwmpoint_from_lwgeom(const LWGEOM *g)
{
	LWPOINTITERATOR* it = lwpointiterator_create(g);
	int has_z = lwgeom_has_z(g);
	int has_m = lwgeom_has_m(g);
	LWMPOINT* result = lwmpoint_construct_empty(g->srid, has_z, has_m);
	POINT4D p;

	while(lwpointiterator_next(it, &p)) {
		LWPOINT* lwp = lwpoint_make(g->srid, has_z, has_m, &p);
		lwmpoint_add_lwpoint(result, lwp);
	}

	lwpointiterator_destroy(it);
	return result;
}
