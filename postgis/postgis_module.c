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
 * Copyright (C) 2011  OpenGeo.org
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "libpq/pqsignal.h"

#include "../postgis_config.h"

#include "lwgeom_log.h"
#include "lwgeom_pg.h"
#include "geos_c.h"

#ifdef HAVE_LIBPROTOBUF
#include "lwgeom_wagyu.h"
#endif

/*
 * This is required for builds against pgsql
 */
PG_MODULE_MAGIC;


static void interrupt_geos_callback()
{
#ifdef WIN32
	if (UNBLOCKED_SIGNAL_QUEUE())
	{
		pgwin32_dispatch_queued_signals();
	}
#endif
	/*
	 * If PgSQL global flags show interrupt,
	 * flip the pending flag in GEOS
	 * to end current query.
	 */
	if (QueryCancelPending || ProcDiePending)
	{
		GEOS_interruptRequest();
	}
}

static void interrupt_liblwgeom_callback()
{
#ifdef WIN32
	if (UNBLOCKED_SIGNAL_QUEUE())
	{
		pgwin32_dispatch_queued_signals();
	}
#endif
	/*
	 * If PgSQL global flags show interrupt,
	 * flip the pending flag in liblwgeom
	 * to end current query.
	 */
	if (QueryCancelPending || ProcDiePending)
	{
		lwgeom_request_interrupt();
	}
}

/*
* Pass proj error message out via the PostgreSQL logging
* system instead of letting them default into the
* stderr.
*/
#if POSTGIS_PROJ_VERSION > 60000
#include "proj.h"

static void
pjLogFunction(void* data, int logLevel, const char* message)
{
	elog(DEBUG1, "libproj threw an exception (%d): %s", logLevel, message);
}
#endif

/*
 * Module load callback
 */
void _PG_init(void);
void
_PG_init(void)
{
	/*
	 * Hook up interrupt checking to call back here
	 * and examine the PgSQL interrupt state variables
	 */
	GEOS_interruptRegisterCallback(interrupt_geos_callback);
	lwgeom_register_interrupt_callback(interrupt_liblwgeom_callback);

	/* Install PostgreSQL error/memory handlers */
	pg_install_lwgeom_handlers();

#if POSTGIS_PROJ_VERSION > 60000
	/* Pass proj messages through the pgsql error handler */
	proj_log_func(NULL, NULL, pjLogFunction);
#endif

}

/*
 * Module unload callback
 */
void _PG_fini(void);
void
_PG_fini(void)
{
	elog(NOTICE, "Goodbye from PostGIS %s", POSTGIS_VERSION);
}


