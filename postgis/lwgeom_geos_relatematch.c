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
 * Copyright (C) 2010 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "lwgeom_geos.h"
#include "lwgeom_pg.h"

#include <string.h>
#include <assert.h>

/* #define POSTGIS_DEBUG_LEVEL 4 */

Datum ST_RelateMatch(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RelateMatch);
Datum ST_RelateMatch(PG_FUNCTION_ARGS)
{
	char *mat, *pat;
	text *mat_text, *pat_text;
	int result;

	/* Read the arguments */
        mat_text = (PG_GETARG_TEXT_P(0));
        pat_text = (PG_GETARG_TEXT_P(1));

        /* Convert from text to cstring */
        mat = text_to_cstring(mat_text);
        pat = text_to_cstring(pat_text);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	result = GEOSRelatePatternMatch(mat, pat);
	if (result == 2)
	{
		lwfree(mat); lwfree(pat);
		lwpgerror("GEOSRelatePatternMatch: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	lwfree(mat); lwfree(pat);
	PG_RETURN_BOOL(result);
}

