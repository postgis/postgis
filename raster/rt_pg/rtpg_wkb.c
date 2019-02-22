/*
 *
 * WKTRaster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright (C) 2018-2018 Bborie Park <dustymugs@gmail.com>
 * Copyright (C) 2011-2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@azavea.com>
 * Copyright (C) 2009-2011 Pierre Racine <pierre.racine@sbf.ulaval.ca>
 * Copyright (C) 2009-2011 Mateusz Loskot <mateusz@loskot.net>
 * Copyright (C) 2008-2009 Sandro Santilli <strk@kbt.io>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <postgres.h> /* for palloc */
#include <fmgr.h> /* for PG_*, Datum*  */
#include <utils/builtins.h> /*  for cstring_to_text */

#include "rtpostgis.h"

Datum RASTER_asWKB(PG_FUNCTION_ARGS);
Datum RASTER_asHexWKB(PG_FUNCTION_ARGS);

Datum RASTER_fromWKB(PG_FUNCTION_ARGS);
Datum RASTER_fromHexWKB(PG_FUNCTION_ARGS);

/**
 * Output is WKB
 */
PG_FUNCTION_INFO_V1(RASTER_asWKB);
Datum RASTER_asWKB(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	uint8_t *wkb = NULL;
	uint32_t wkb_size = 0;
	char *result = NULL;
	int result_size = 0;
	int outasin = FALSE;

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Get raster object */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_asWKB: Cannot deserialize raster");
		PG_RETURN_NULL();
	}

	if (!PG_ARGISNULL(1))
		outasin = PG_GETARG_BOOL(1);

	/* Parse raster to wkb object */
	wkb = rt_raster_to_wkb(raster, outasin, &wkb_size);
	if (!wkb) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_asWKB: Cannot allocate and generate WKB data");
		PG_RETURN_NULL();
	}

	/* Create varlena object */
	result_size = wkb_size + VARHDRSZ;
	result = (char *)palloc(result_size);
	SET_VARSIZE(result, result_size);
	memcpy(VARDATA(result), wkb, VARSIZE_ANY_EXHDR(result));

	/* Free raster objects used */
	rt_raster_destroy(raster);
	pfree(wkb);
	PG_FREE_IF_COPY(pgraster, 0);

	PG_RETURN_POINTER(result);
}

/**
 * Output is Hex WKB
 */
PG_FUNCTION_INFO_V1(RASTER_asHexWKB);
Datum RASTER_asHexWKB(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	int outasin = FALSE;
	uint32_t hexwkbsize = 0;
	char *hexwkb = NULL;
	text *result = NULL;

	POSTGIS_RT_DEBUG(3, "Starting");

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_asHexWKB: Cannot deserialize raster");
		PG_RETURN_NULL();
	}

	if (!PG_ARGISNULL(1))
		outasin = PG_GETARG_BOOL(1);

	hexwkb = rt_raster_to_hexwkb(raster, outasin, &hexwkbsize);
	if (!hexwkb) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_asHexWKB: Cannot allocate and generate Hex WKB data");
		PG_RETURN_NULL();
	}

	/* Free the raster objects used */
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	result = cstring_to_text(hexwkb);

	PG_RETURN_TEXT_P(result);
}

/**
 * Input is WKB
 */
PG_FUNCTION_INFO_V1(RASTER_fromWKB);
Datum RASTER_fromWKB(PG_FUNCTION_ARGS)
{
	bytea *bytea_data;
	uint8_t *data;
	int data_len = 0;

	rt_raster raster;
	void *result = NULL;

	POSTGIS_RT_DEBUG(3, "Starting");

	bytea_data = (bytea *) PG_GETARG_BYTEA_P(0);
	data = (uint8_t *) VARDATA(bytea_data);
	data_len = VARSIZE_ANY_EXHDR(bytea_data);

	raster = rt_raster_from_wkb(data, data_len);
	PG_FREE_IF_COPY(bytea_data, 0);
	if (raster == NULL)
		PG_RETURN_NULL();

	result = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (result == NULL)
		PG_RETURN_NULL();

	SET_VARSIZE(result, ((rt_pgraster*)result)->size);
	PG_RETURN_POINTER(result);
}

/**
 * Input is Hex WKB
 */
PG_FUNCTION_INFO_V1(RASTER_fromHexWKB);
Datum RASTER_fromHexWKB(PG_FUNCTION_ARGS)
{
	text *hexwkb_text = PG_GETARG_TEXT_P(0);
	char *hexwkb;

	rt_raster raster;
	void *result = NULL;

	POSTGIS_RT_DEBUG(3, "Starting");

	hexwkb = text_to_cstring(hexwkb_text);

	raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
	PG_FREE_IF_COPY(hexwkb_text, 0);
	if (raster == NULL)
		PG_RETURN_NULL();

	result = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (result == NULL)
		PG_RETURN_NULL();

	SET_VARSIZE(result, ((rt_pgraster*)result)->size);
	PG_RETURN_POINTER(result);
}
