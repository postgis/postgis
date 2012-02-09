/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2011  OpenGeo.org 
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/guc.h"

#include "../postgis_config.h"
#include "lwgeom_log.h"
#include "lwgeom_pg.h"

/*
 * This is required for builds against pgsql
 */
PG_MODULE_MAGIC;

/*
 * Module load callback
 */
void _PG_init(void);
void
_PG_init(void)
{
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
#if POSTGIS_PGSQL_VERSION >= 91
    NULL, /* GucStringCheckHook check_hook */
#endif
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
#if POSTGIS_PGSQL_VERSION >= 91
    NULL, /* GucStringCheckHook check_hook */
#endif
    NULL, /* GucStringAssignHook assign_hook */
    NULL  /* GucShowHook show_hook */
   );
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



