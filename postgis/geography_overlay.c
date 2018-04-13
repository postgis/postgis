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
 * Copyright (C) 2017 Danny Götte <danny.goette@fem.tu-ilmenau.de>
 *
 **********************************************************************/

#include "postgres.h"

#include "../postgis_config.h"

#include <math.h>

#include "liblwgeom.h"        /* For standard geometry types. */
#include "lwgeom_pg.h"        /* For pg macros. */
#include "lwgeom_transform.h" /* For SRID functions */

#include "lwgeodetic_overlay.h"

Datum geography_union(PG_FUNCTION_ARGS);
Datum geography_difference(PG_FUNCTION_ARGS);
Datum geography_symdifference(PG_FUNCTION_ARGS);
Datum geography_union(PG_FUNCTION_ARGS);

/**
 * geography_union(GSERIALIZED *g2, GSERIALIZED *g1)
 * returns union of both geographies
 */
PG_FUNCTION_INFO_V1(geography_union);
Datum geography_union(PG_FUNCTION_ARGS)
{
	LWGEOM* lwgeom1 = NULL;
	LWGEOM* lwgeom2 = NULL;
	LWGEOM* lwgeom_out = NULL;

	GSERIALIZED* g1 = NULL;
	GSERIALIZED* g2 = NULL;
	GSERIALIZED* g_out = NULL;

	g1 = PG_GETARG_GSERIALIZED_P(0);
	g2 = PG_GETARG_GSERIALIZED_P(1);

	if (g1 == NULL) { PG_RETURN_POINTER(g2); }

	if (g2 == NULL) { PG_RETURN_POINTER(g1); }

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	lwgeom_out = lwgeodetic_union(lwgeom1, lwgeom2);
	g_out = gserialized_from_lwgeom(lwgeom_out, 0);

	PG_RETURN_POINTER(g_out);
}

/**
 * geography_intersection(GSERIALIZED *g2, GSERIALIZED *g1)
 * returns intersection of both geographies
 */
PG_FUNCTION_INFO_V1(geography_intersection);
Datum geography_intersection(PG_FUNCTION_ARGS)
{
	LWGEOM* lwgeom1 = NULL;
	LWGEOM* lwgeom2 = NULL;
	LWGEOM* lwgeom_out = NULL;

	GSERIALIZED* g1 = NULL;
	GSERIALIZED* g2 = NULL;
	GSERIALIZED* g_out = NULL;

	g1 = PG_GETARG_GSERIALIZED_P(0);
	g2 = PG_GETARG_GSERIALIZED_P(1);

	if (g1 == NULL) { PG_RETURN_POINTER(g2); }

	if (g2 == NULL) { PG_RETURN_POINTER(g1); }

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	lwgeom_out = lwgeodetic_intersection(lwgeom1, lwgeom2);
	g_out = gserialized_from_lwgeom(lwgeom_out, 0);

	PG_RETURN_POINTER(g_out);
}

/**
 * geography_difference(GSERIALIZED *g2, GSERIALIZED *g1)
 * returns difference of both geographies
 */
PG_FUNCTION_INFO_V1(geography_difference);
Datum geography_difference(PG_FUNCTION_ARGS)
{
	LWGEOM* lwgeom1 = NULL;
	LWGEOM* lwgeom2 = NULL;
	LWGEOM* lwgeom_out = NULL;

	GSERIALIZED* g1 = NULL;
	GSERIALIZED* g2 = NULL;
	GSERIALIZED* g_out = NULL;

	g1 = PG_GETARG_GSERIALIZED_P(0);
	g2 = PG_GETARG_GSERIALIZED_P(1);

	if (g1 == NULL) { PG_RETURN_POINTER(g2); }

	if (g2 == NULL) { PG_RETURN_POINTER(g1); }

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	lwgeom_out = lwgeodetic_difference(lwgeom1, lwgeom2);
	g_out = gserialized_from_lwgeom(lwgeom_out, 0);

	PG_RETURN_POINTER(g_out);
}

/**
 * geography_symdifference(GSERIALIZED *g2, GSERIALIZED *g1)
 * returns symetric difference of both geographies
 */
PG_FUNCTION_INFO_V1(geography_symdifference);
Datum geography_symdifference(PG_FUNCTION_ARGS)
{
	LWGEOM* lwgeom1 = NULL;
	LWGEOM* lwgeom2 = NULL;
	LWGEOM* lwgeom_out = NULL;

	GSERIALIZED* g1 = NULL;
	GSERIALIZED* g2 = NULL;
	GSERIALIZED* g_out = NULL;

	g1 = PG_GETARG_GSERIALIZED_P(0);
	g2 = PG_GETARG_GSERIALIZED_P(1);

	if (g1 == NULL) { PG_RETURN_POINTER(g2); }

	if (g2 == NULL) { PG_RETURN_POINTER(g1); }

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	lwgeom_out = lwgeodetic_symdifference(lwgeom1, lwgeom2);
	g_out = gserialized_from_lwgeom(lwgeom_out, 0);

	PG_RETURN_POINTER(g_out);
}
