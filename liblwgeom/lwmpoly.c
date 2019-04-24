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
lwmpoly_release(LWMPOLY *lwmpoly)
{
	lwgeom_release(lwmpoly_as_lwgeom(lwmpoly));
}

LWMPOLY *
lwmpoly_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWMPOLY *ret = (LWMPOLY*)lwcollection_construct_empty(MULTIPOLYGONTYPE, srid, hasz, hasm);
	return ret;
}


LWMPOLY* lwmpoly_add_lwpoly(LWMPOLY *mobj, const LWPOLY *obj)
{
	return (LWMPOLY*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
}


void lwmpoly_free(LWMPOLY *mpoly)
{
	uint32_t i;
	if ( ! mpoly ) return;
	if ( mpoly->bbox )
		lwfree(mpoly->bbox);

	for ( i = 0; i < mpoly->ngeoms; i++ )
		if ( mpoly->geoms && mpoly->geoms[i] )
			lwpoly_free(mpoly->geoms[i]);

	if ( mpoly->geoms )
		lwfree(mpoly->geoms);

	lwfree(mpoly);
}

