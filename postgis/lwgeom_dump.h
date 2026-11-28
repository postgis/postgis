/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * https://postgis.net
 *
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef LWGEOM_DUMP_H
#define LWGEOM_DUMP_H

#include "postgres.h"
#include "utils/elog.h"

#define LWGEOM_DUMP_MAXDEPTH 32

static inline void
pgis_check_dump_depth(int depth)
{
	if (depth >= LWGEOM_DUMP_MAXDEPTH)
		elog(ERROR, "Unable to dump overly nested collection");
}

#endif
