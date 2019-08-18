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
 * Copyright (C) 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 * Copyright (C) 2010 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2009-2011 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "access/hash.h"
#include "utils/geo_decls.h"
#include "utils/sortsupport.h" /* SortSupport */

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

Datum lwgeom_lt(PG_FUNCTION_ARGS);
Datum lwgeom_le(PG_FUNCTION_ARGS);
Datum lwgeom_eq(PG_FUNCTION_ARGS);
Datum lwgeom_ge(PG_FUNCTION_ARGS);
Datum lwgeom_gt(PG_FUNCTION_ARGS);
Datum lwgeom_cmp(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(lwgeom_lt);
Datum lwgeom_lt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	if (cmp < 0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(lwgeom_le);
Datum lwgeom_le(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	if (cmp <= 0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(lwgeom_eq);
Datum lwgeom_eq(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	if (cmp == 0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(lwgeom_ge);
Datum lwgeom_ge(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	if (cmp >= 0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(lwgeom_gt);
Datum lwgeom_gt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	if (cmp > 0)
		PG_RETURN_BOOL(true);
	else
		PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(lwgeom_cmp);
Datum lwgeom_cmp(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int ret = gserialized_cmp(g1, g2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	PG_RETURN_INT32(ret);
}

PG_FUNCTION_INFO_V1(lwgeom_hash);
Datum lwgeom_hash(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);

	int32_t hval = gserialized_hash(g1);
	PG_FREE_IF_COPY(g1, 0);
	PG_RETURN_INT32(hval);
}

static int
lwgeom_cmp_abbrev(Datum x, Datum y, SortSupport ssup)
{
	/* Empty is a special case */
	if (x == 0 || y == 0 || x == y)
		return 0; /* 0 means "ask bigger comparator" and not equality*/
	else if (x > y)
		return 1;
	else
		return -1;
}

static int
lwgeom_cmp_full(Datum x, Datum y, SortSupport ssup)
{
	GSERIALIZED *g1 = (GSERIALIZED *)PG_DETOAST_DATUM(x);
	GSERIALIZED *g2 = (GSERIALIZED *)PG_DETOAST_DATUM(y);
	int ret = gserialized_cmp(g1, g2);
	POSTGIS_FREE_IF_COPY_P(g1, x);
	POSTGIS_FREE_IF_COPY_P(g2, y);
	return ret;
}

static bool
lwgeom_abbrev_abort(int memtupcount, SortSupport ssup)
{
	return LW_FALSE;
}

static Datum
lwgeom_abbrev_convert(Datum original, SortSupport ssup)
{
	GSERIALIZED *g = (GSERIALIZED *)PG_DETOAST_DATUM(original);
	uint64_t hash = gserialized_get_sortable_hash(g);
	POSTGIS_FREE_IF_COPY_P(g, original);
	return hash;
}

/*
 * Sort support strategy routine
 */
PG_FUNCTION_INFO_V1(lwgeom_sortsupport);
Datum lwgeom_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport)PG_GETARG_POINTER(0);

	ssup->comparator = lwgeom_cmp_full;
	ssup->ssup_extra = NULL;
	/* Enable sortsupport only on 64 bit Datum */
	if (ssup->abbreviate && sizeof(Datum) == 8)
	{
		ssup->comparator = lwgeom_cmp_abbrev;
		ssup->abbrev_converter = lwgeom_abbrev_convert;
		ssup->abbrev_abort = lwgeom_abbrev_abort;
		ssup->abbrev_full_comparator = lwgeom_cmp_full;
	}

	PG_RETURN_VOID();
}
