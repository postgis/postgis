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
 * Copyright 2011 Kashif Rasul <kashif.rasul@gmail.com>
 *
 **********************************************************************/


#include <assert.h>

#include "postgres.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_cache.h"

#if defined(HAVE_LIBJSON)

#include <json.h>

/* We don't include <utils/builtins.h> to avoid collisions with json-c/json.h */
static text*
cstring2text(const char *cstring)
{
	size_t len = strlen(cstring);
	text *result = (text *) palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), cstring, len);

	return result;
}

static char*
text2cstring(const text *textptr)
{
	size_t size = VARSIZE_ANY_EXHDR(textptr);
	char *str = lwalloc(size+1);
	memcpy(str, VARDATA(textptr), size);
	str[size]='\0';
	return str;
}
#endif

Datum geom_from_geojson(PG_FUNCTION_ARGS);
Datum postgis_libjson_version(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(postgis_libjson_version);
Datum postgis_libjson_version(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBJSON
	PG_RETURN_NULL();
#else /* HAVE_LIBJSON  */
# ifdef JSON_C_VERSION
	const char *ver = json_c_version();
# else
	const char *ver = "UNKNOWN";
# endif
	text *result = cstring2text(ver);
	PG_RETURN_POINTER(result);
#endif
}

PG_FUNCTION_INFO_V1(geom_from_geojson);
Datum geom_from_geojson(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBJSON
	elog(ERROR, "You need JSON-C for ST_GeomFromGeoJSON");
	PG_RETURN_NULL();
#else /* HAVE_LIBJSON  */

	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	text *geojson_input;
	char *geojson;
	char *srs = NULL;
	int32_t srid = WGS84_SRID;

	/* Get the geojson stream */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	geojson_input = PG_GETARG_TEXT_P(0);
	geojson = text2cstring(geojson_input);

	lwgeom = lwgeom_from_geojson(geojson, &srs);
	if (!lwgeom)
	{
		/* Shouldn't get here */
		elog(ERROR, "lwgeom_from_geojson returned NULL");
		PG_RETURN_NULL();
	}

	if (srs)
	{
		srid = GetSRIDCacheBySRS(fcinfo, srs);
		lwfree(srs);
	}

	lwgeom_set_srid(lwgeom, srid);
	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(geom);
#endif
}
