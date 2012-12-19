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
    /* install raster handlers */
    rt_pg_install_handlers();
}

