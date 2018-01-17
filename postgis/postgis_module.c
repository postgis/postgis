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
#include "utils/elog.h"
#include "utils/guc.h"
#include "libpq/pqsignal.h"

#include "../postgis_config.h"

#include "lwgeom_log.h"
#include "lwgeom_pg.h"
#include "geos_c.h"
#include "lwgeom_backend_api.h"

/*
 * This is required for builds against pgsql
 */
PG_MODULE_MAGIC;

static pqsigfunc coreIntHandler = 0;
static void handleInterrupt(int sig);

#ifdef WIN32
static void interruptCallback() {
  if (UNBLOCKED_SIGNAL_QUEUE())
    pgwin32_dispatch_queued_signals();
}
#endif

/*
 * Module load callback
 */
void _PG_init(void);
void
_PG_init(void)
{

  coreIntHandler = pqsignal(SIGINT, handleInterrupt);

#ifdef WIN32
#if POSTGIS_GEOS_VERSION >= 34
  GEOS_interruptRegisterCallback(interruptCallback);
#endif
  lwgeom_register_interrupt_callback(interruptCallback);
#endif

#if 0
  /* Define custom GUC variables. */
  DefineCustomIntVariable(
    "postgis.debug.level", /* name */
    "Sets the debugging level of PostGIS.", /* short_desc */
    "This is an experimental configuration.", /* long_desc */
    &postgis_debug_level, /* valueAddr */
    0, 8, /* min-max */
    0, /* bootValue */
    PGC_SUSET, /* GucContext context */
    GUC_UNIT_MS, /* int flags */
    NULL, /* GucStringCheckHook check_hook */
    NULL, /* GucStringAssignHook assign_hook */
    NULL  /* GucShowHook show_hook */
   );
#endif

#if 0
  /* Define custom GUC variables. */
  DefineCustomStringVariable(
    "postgis.greeting.string", /* name */
    "Sets the greeting string used on postgis module load.", /* short_desc */
    "This is an experimental configuration.", /* long_desc */
    &greeting, /* valueAddr */
    "Welcome to PostGIS " POSTGIS_VERSION, /* bootValue */
    PGC_SUSET, /* GucContext context */
    GUC_UNIT_MS, /* int flags */
    NULL, /* GucStringCheckHook check_hook */
    NULL, /* GucStringAssignHook assign_hook */
    NULL  /* GucShowHook show_hook */
   );
#endif

    /* install PostgreSQL handlers */
    pg_install_lwgeom_handlers();

    /* initialize geometry backend */
    lwgeom_init_backend();
}

/*
 * Module unload callback
 */
void _PG_fini(void);
void
_PG_fini(void)
{
  elog(NOTICE, "Goodbye from PostGIS %s", POSTGIS_VERSION);
  pqsignal(SIGINT, coreIntHandler);
}


static void
handleInterrupt(int sig)
{
  /* NOTE: printf here would be dangerous, see
   * https://trac.osgeo.org/postgis/ticket/3644
   *
   * TODO: block interrupts during execution, to fix the problem
   */
  /* printf("Interrupt requested\n"); fflush(stdout); */

#if POSTGIS_GEOS_VERSION >= 34
  GEOS_interruptRequest();
#endif

  /* request interruption of liblwgeom as well */
  lwgeom_request_interrupt();

  if ( coreIntHandler ) {
    (*coreIntHandler)(sig);
  }
}
