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
 * Copyright 2012-2013 Oslandia <infos@oslandia.com>
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/guc.h" /* for custom variables */

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

#include "lwgeom_backend_api.h"
#include "lwgeom_geos.h"
#if HAVE_SFCGAL
#include "lwgeom_sfcgal.h"
#endif

struct lwgeom_backend_definition
{
	const char *name;
};

#if HAVE_SFCGAL
#define LWGEOM_NUM_BACKENDS 2
#else
#define LWGEOM_NUM_BACKENDS 1
#endif

struct lwgeom_backend_definition lwgeom_backends[LWGEOM_NUM_BACKENDS] = {{.name = "geos"}
#if HAVE_SFCGAL
									 ,{.name = "sfcgal"}
#endif
};

/* Geometry Backend */
char *lwgeom_backend_name;
struct lwgeom_backend_definition *lwgeom_backend = &lwgeom_backends[0];

static void
lwgeom_backend_switch(const char *newvalue, __attribute__((__unused__)) void *extra)
{
	int i;

	if (!newvalue)
		return;

	for (i = 0; i < LWGEOM_NUM_BACKENDS; ++i)
	{
		if (!strcmp(lwgeom_backends[i].name, newvalue))
		{
			lwgeom_backend = &lwgeom_backends[i];
			return;
		}
	}
	lwpgerror("Can't find %s geometry backend", newvalue);
}

void
lwgeom_init_backend()
{
	/* #2382 Before trying to create a user GUC, make sure */
	/* that the name is not already in use. Why would it be in use? */
	/* During an upgrade, a prior copy of the PostGIS library will */
	/* already be loaded in memory and the GUC already defined. We */
	/* can skip GUC definition in this case, so we just return. */
	static const char *guc_name = "postgis.backend";
	// const char *guc_installed = GetConfigOption(guc_name, TRUE, FALSE);

	/* Uh oh, this GUC name already exists. Ordinarily we could just go on */
	/* our way, but the way the postgis.backend works is by using the "assign" */
	/* callback to change which backend is in use by flipping a global variable */
	/* over. This saves the overhead of looking up the engine every time, at */
	/* the expense of the extra complexity. */
	if (postgis_guc_find_option(guc_name))
	{
		/* In this narrow case the previously installed GUC is tied to the callback in */
		/* the previously loaded library. Probably this is happening during an */
		/* upgrade, so the old library is where the callback ties to. */
		elog(WARNING, "'%s' is already set and cannot be changed until you reconnect", guc_name);
		return;
	}

	/* Good, the GUC name is not already in use, so this must be a fresh */
	/* and clean new load of the library, and we can define the user GUC */
	DefineCustomStringVariable(
	    guc_name,                                                                    /* name */
	    "Sets the PostGIS Geometry Backend.",                                        /* short_desc */
	    "Sets the PostGIS Geometry Backend (allowed values are 'geos' or 'sfcgal')", /* long_desc */
	    &lwgeom_backend_name,                                                        /* valueAddr */
	    (char *)lwgeom_backends[0].name,                                             /* bootValue */
	    PGC_USERSET,                                                                 /* GucContext context */
	    0,                                                                           /* int flags */
	    NULL,                  /* GucStringCheckHook check_hook */
	    lwgeom_backend_switch, /* GucStringAssignHook assign_hook */
	    NULL                   /* GucShowHook show_hook */
	);
}

/* intersects3d through dwithin */
PG_FUNCTION_INFO_V1(ST_3DIntersects);
Datum ST_3DIntersects(PG_FUNCTION_ARGS)
{
	double mindist;
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwgeom1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lwgeom2 = lwgeom_from_gserialized(geom2);

	error_if_srid_mismatch(lwgeom1->srid, lwgeom2->srid);

	mindist = lwgeom_mindistance3d_tolerance(lwgeom1, lwgeom2, 0.0);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*empty geometries cases should be right handled since return from underlying
	  functions should be FLT_MAX which causes false as answer*/
	PG_RETURN_BOOL(0.0 == mindist);
}
