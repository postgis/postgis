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

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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
	if (cmp == 0)
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
	Datum hval;
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	/* Point to just the type/coordinate part of buffer */
	size_t hsz1 = gserialized_header_size(g1);
	uint8_t *b1 = (uint8_t*)g1 + hsz1;
	/* Calculate size of type/coordinate buffer */
	size_t sz1 = VARSIZE(g1);
	size_t bsz1 = sz1 - hsz1;
	/* Calculate size of srid/type/coordinate buffer */
	int srid = gserialized_get_srid(g1);
	size_t bsz2 = bsz1 + sizeof(int);
	uint8_t *b2 = palloc(bsz2);
	/* Copy srid into front of combined buffer */
	memcpy(b2, &srid, sizeof(int));
	/* Copy type/coordinates into rest of combined buffer */
	memcpy(b2+sizeof(int), b1, bsz1);
	/* Hash combined buffer */
	hval = hash_any(b2, bsz2);
	pfree(b2);
	PG_FREE_IF_COPY(g1, 0);
	PG_RETURN_DATUM(hval);
}



