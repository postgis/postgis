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
 * Copyright 2009-2010 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"


#include "../postgis_config.h"
/*#define POSTGIS_DEBUG_LEVEL 4*/

#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <string.h>
#include <assert.h>

Datum ST_MakeValid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_MakeValid);
Datum ST_MakeValid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in, *out;
	LWGEOM *lwgeom_in, *lwgeom_out;

	in = PG_GETARG_GSERIALIZED_P_COPY(0);
	lwgeom_in = lwgeom_from_gserialized(in);

	POSTGIS_DEBUG(1, "ST_MakeValid enter");

	switch ( lwgeom_in->type )
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
	case LINETYPE:
	case POLYGONTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		break;

	default:
		lwpgerror("ST_MakeValid: unsupported geometry type %s",
		        lwtype_name(lwgeom_in->type));
		PG_RETURN_NULL();
		break;
	}

	lwgeom_out = lwgeom_make_valid(lwgeom_in);
	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_NULL();
	}

	out = geometry_serialize(lwgeom_out);

	PG_RETURN_POINTER(out);
}

/* Uses GEOS internally */
static LWGEOM* lwgeom_clean(LWGEOM* lwgeom_in);
static LWGEOM*
lwgeom_clean(LWGEOM* lwgeom_in)
{
	LWGEOM* lwgeom_out;

	lwgeom_out = lwgeom_make_valid(lwgeom_in);
	if ( ! lwgeom_out )
	{
		return NULL;
	}

	/* Check dimensionality is the same as input */
	if ( lwgeom_dimensionality(lwgeom_in) != lwgeom_dimensionality(lwgeom_out) )
	{
		lwpgnotice("lwgeom_clean: dimensional collapse (%d to %d)",
		         lwgeom_dimensionality(lwgeom_in), lwgeom_dimensionality(lwgeom_out));

		return NULL;
	}

	/* Check that the output is not a collection if the input wasn't */
	if ( lwgeom_out->type == COLLECTIONTYPE &&
	        lwgeom_in->type != COLLECTIONTYPE )
	{
		lwpgnotice("lwgeom_clean: mixed-type output (%s) "
		         "from single-type input (%s)",
		         lwtype_name(lwgeom_out->type),
		         lwtype_name(lwgeom_in->type));
		return NULL;
	}

	/* Force right-hand-rule (will only affect polygons) */
	/* gout := ST_ForceRHR(gout); */

	/* Remove repeated duplicated points ? */
	/* gout = ST_RemoveRepeatedPoints(gout); */

	return lwgeom_out;
}


Datum ST_CleanGeometry(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CleanGeometry);
Datum ST_CleanGeometry(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in, *out;
	LWGEOM *lwgeom_in, *lwgeom_out;

	in = PG_GETARG_GSERIALIZED_P(0);
	lwgeom_in = lwgeom_from_gserialized(in);

	/* Short-circuit: empty geometry are the cleanest ! */
#if 0
	if ( lwgeom_is_empty(lwgeom_in) )
	{
		out = geometry_serialize(lwgeom_in);
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_POINTER(out);
	}
#endif

	lwgeom_out = lwgeom_clean(lwgeom_in);
	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_NULL();
	}

	out = geometry_serialize(lwgeom_out);
	PG_RETURN_POINTER(out);
}

