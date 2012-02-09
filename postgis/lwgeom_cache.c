/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2012 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"
#include "lwgeom_cache.h"

GeomCache* GetGeomCache(FunctionCallInfoData *fcinfo)
{
	MemoryContext old_context;
	GeomCache* cache = fcinfo->flinfo->fn_extra;
	if ( ! cache ) {
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		cache = palloc(sizeof(GeomCache));
		MemoryContextSwitchTo(old_context);
		cache->prep = 0;
		cache->rtree = 0;
		fcinfo->flinfo->fn_extra = cache;
	}
	return cache;
}

