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

#include "../../postgis_config.h"

#include "liblwgeom.h"
#include "rt_pg.h"

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
    /* Install raster handlers */
    lwgeom_set_handlers(rt_pg_alloc, rt_pg_realloc, rt_pg_free,
            rt_pg_error, rt_pg_notice);
}
