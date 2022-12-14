/*
 *
 * WKTRaster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
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

#include <postgres.h>
#include <utils/builtins.h> /* for text_to_cstring() */
#include <access/htup_details.h> /* for heap_form_tuple() */
#include <utils/lsyscache.h> /* for get_typlenbyvalalign */
#include <utils/array.h> /* for ArrayType */
#include <utils/guc.h> /* for ArrayType */
#include <catalog/pg_type.h> /* for INT2OID, INT4OID, FLOAT4OID, FLOAT8OID and TEXTOID */
#include <utils/memutils.h> /* For TopMemoryContext */

#include "../../postgis_config.h"

#include "rtpostgis.h"
#include "rtpg_internal.h"
#include "stringbuffer.h"

/* convert GDAL raster to raster */
Datum RASTER_fromGDALRaster(PG_FUNCTION_ARGS);

/* convert raster to GDAL raster */
Datum RASTER_asGDALRaster(PG_FUNCTION_ARGS);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS);
Datum RASTER_setGDALOpenOptions(PG_FUNCTION_ARGS);

/* warp a raster using GDAL Warp API */
Datum RASTER_GDALWarp(PG_FUNCTION_ARGS);

/* ----------------------------------------------------------------
 * Returns raster from GDAL raster
 * ---------------------------------------------------------------- */
PG_FUNCTION_INFO_V1(RASTER_fromGDALRaster);
Datum RASTER_fromGDALRaster(PG_FUNCTION_ARGS)
{
	bytea *bytea_data;
	uint8_t *data;
	int data_len = 0;
	VSILFILE *vsifp = NULL;
	GDALDatasetH hdsSrc;
	int32_t srid = -1; /* -1 for NULL */

	rt_pgraster *pgraster = NULL;
	rt_raster raster;

	/* NULL if NULL */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* get data */
	bytea_data = (bytea *) PG_GETARG_BYTEA_P(0);
	data = (uint8_t *) VARDATA(bytea_data);
	data_len = VARSIZE_ANY_EXHDR(bytea_data);

	/* process srid */
	/* NULL srid means try to determine SRID from bytea */
	if (!PG_ARGISNULL(1))
		srid = clamp_srid(PG_GETARG_INT32(1));

	/* create memory "file" */
	vsifp = VSIFileFromMemBuffer("/vsimem/in.dat", data, data_len, FALSE);
	if (vsifp == NULL) {
		PG_FREE_IF_COPY(bytea_data, 0);
		elog(ERROR, "RASTER_fromGDALRaster: Could not load bytea into memory file for use by GDAL");
		PG_RETURN_NULL();
	}

	/* register all GDAL drivers */
	rt_util_gdal_register_all(0);

	/* open GDAL raster */
	hdsSrc = rt_util_gdal_open("/vsimem/in.dat", GA_ReadOnly, 1);
	if (hdsSrc == NULL) {
		VSIFCloseL(vsifp);
		PG_FREE_IF_COPY(bytea_data, 0);
		elog(ERROR, "RASTER_fromGDALRaster: Could not open bytea with GDAL. Check that the bytea is of a GDAL supported format");
		PG_RETURN_NULL();
	}

#if POSTGIS_DEBUG_LEVEL > 3
	{
		GDALDriverH hdrv = GDALGetDatasetDriver(hdsSrc);

		POSTGIS_RT_DEBUGF(4, "Input GDAL Raster info: %s, (%d x %d)",
			GDALGetDriverShortName(hdrv),
			GDALGetRasterXSize(hdsSrc),
			GDALGetRasterYSize(hdsSrc)
		);
	}
#endif

	/* convert GDAL raster to raster */
	raster = rt_raster_from_gdal_dataset(hdsSrc);

	GDALClose(hdsSrc);
	VSIFCloseL(vsifp);
	PG_FREE_IF_COPY(bytea_data, 0);

	if (raster == NULL) {
		elog(ERROR, "RASTER_fromGDALRaster: Could not convert GDAL raster to raster");
		PG_RETURN_NULL();
	}

	/* apply SRID if set */
	if (srid != -1)
		rt_raster_set_srid(raster, srid);

	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (!pgraster)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	PG_RETURN_POINTER(pgraster);
}

/**
 * Returns formatted GDAL raster as bytea object of raster
 */
PG_FUNCTION_INFO_V1(RASTER_asGDALRaster);
Datum RASTER_asGDALRaster(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster;

	text *formattext = NULL;
	char *format = NULL;
	char **options = NULL;
	text *optiontext = NULL;
	char *option = NULL;
	int32_t srid = SRID_UNKNOWN;
	char *srs = NULL;

	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;
	int i = 0;
	int j = 0;

	uint8_t *gdal = NULL;
	uint64_t gdal_size = 0;
	bytea *result = NULL;
	uint64_t result_size = 0;

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_asGDALRaster: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* format is required */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Format must be provided");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}
	else {
		formattext = PG_GETARG_TEXT_P(1);
		format = text_to_cstring(formattext);
	}

	POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: Arg 1 (format) is %s", format);

	/* process options */
	if (!PG_ARGISNULL(2)) {
		POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Processing Arg 2 (options)");
		array = PG_GETARG_ARRAYTYPE_P(2);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case TEXTOID:
				break;
			default:
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_asGDALRaster: Invalid data type for options");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		if (n) {
			options = (char **) palloc(sizeof(char *) * (n + 1));
			if (options == NULL) {
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_asGDALRaster: Could not allocate memory for options");
				PG_RETURN_NULL();
			}

			/* clean each option */
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) continue;

				option = NULL;
				switch (etype) {
					case TEXTOID:
						optiontext = (text *) DatumGetPointer(e[i]);
						if (NULL == optiontext) break;
						option = text_to_cstring(optiontext);

						/* trim string */
						option = rtpg_trim(option);
						POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: option is '%s'", option);
						break;
				}

				if (strlen(option)) {
					options[j] = (char *) palloc(sizeof(char) * (strlen(option) + 1));
					strcpy(options[j], option);
					j++;
				}
			}

			if (j > 0) {
				/* trim allocation */
				options = repalloc(options, (j + 1) * sizeof(char *));

				/* add NULL to end */
				options[j] = NULL;

			}
			else {
				pfree(options);
				options = NULL;
			}
		}
	}

	/* process srid */
	/* NULL srid means use raster's srid */
	if (PG_ARGISNULL(3))
		srid = rt_raster_get_srid(raster);
	else
		srid = PG_GETARG_INT32(3);

	/* get srs from srid */
	if (clamp_srid(srid) != SRID_UNKNOWN) {
		srs = rtpg_getSR(srid);
		if (NULL == srs) {
			if (NULL != options) {
				for (i = j - 1; i >= 0; i--) pfree(options[i]);
				pfree(options);
			}
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_asGDALRaster: Could not find srtext for SRID (%d)", srid);
			PG_RETURN_NULL();
		}
		POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: Arg 3 (srs) is %s", srs);
	}
	else
		srs = NULL;

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Generating GDAL raster");
	gdal = rt_raster_to_gdal(raster, srs, format, options, &gdal_size);

	/* free memory */
	if (NULL != options) {
		for (i = j - 1; i >= 0; i--) pfree(options[i]);
		pfree(options);
	}
	if (NULL != srs) pfree(srs);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	if (!gdal) {
		elog(ERROR, "RASTER_asGDALRaster: Could not allocate and generate GDAL raster");
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: GDAL raster generated with %d bytes", (int) gdal_size);

	/* result is a varlena */
	result_size = gdal_size + VARHDRSZ;
	result = (bytea *) palloc(result_size);
	if (NULL == result) {
		elog(ERROR, "RASTER_asGDALRaster: Insufficient virtual memory for GDAL raster");
		PG_RETURN_NULL();
	}
	SET_VARSIZE(result, result_size);
	memcpy(VARDATA(result), gdal, VARSIZE_ANY_EXHDR(result));

	/* free gdal mem buffer */
	CPLFree(gdal);

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Returning pointer to GDAL raster");
	PG_RETURN_POINTER(result);
}

#define VALUES_LENGTH 6

/**
 * Returns available GDAL drivers
 */
PG_FUNCTION_INFO_V1(RASTER_getGDALDrivers);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	uint32_t drv_count;
	rt_gdaldriver drv_set;
	rt_gdaldriver drv_set2;
	int call_cntr;
	int max_calls;

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		drv_set = rt_raster_gdal_drivers(&drv_count, 0);
		if (NULL == drv_set || !drv_count) {
			elog(NOTICE, "No GDAL drivers found");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		POSTGIS_RT_DEBUGF(3, "%d drivers returned", (int) drv_count);

		/* Store needed information */
		funcctx->user_fctx = drv_set;

		/* total number of tuples to be returned */
		funcctx->max_calls = drv_count;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
			ereport(ERROR, (
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg(
					"function returning record called in context "
					"that cannot accept type record"
				)
			));
		}

		BlessTupleDesc(tupdesc);
		funcctx->tuple_desc = tupdesc;
		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	tupdesc = funcctx->tuple_desc;
	drv_set2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		Datum values[VALUES_LENGTH];
		bool nulls[VALUES_LENGTH];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

		values[0] = Int32GetDatum(drv_set2[call_cntr].idx);
		values[1] = CStringGetTextDatum(drv_set2[call_cntr].short_name);
		values[2] = CStringGetTextDatum(drv_set2[call_cntr].long_name);
		values[3] = BoolGetDatum(drv_set2[call_cntr].can_read);
		values[4] = BoolGetDatum(drv_set2[call_cntr].can_write);
		values[5] = CStringGetTextDatum(drv_set2[call_cntr].create_options);

		POSTGIS_RT_DEBUGF(4, "Result %d, Index %d", call_cntr, drv_set2[call_cntr].idx);
		POSTGIS_RT_DEBUGF(4, "Result %d, Short Name %s", call_cntr, drv_set2[call_cntr].short_name);
		POSTGIS_RT_DEBUGF(4, "Result %d, Full Name %s", call_cntr, drv_set2[call_cntr].long_name);
		POSTGIS_RT_DEBUGF(4, "Result %d, Can Read %u", call_cntr, drv_set2[call_cntr].can_read);
		POSTGIS_RT_DEBUGF(4, "Result %d, Can Write %u", call_cntr, drv_set2[call_cntr].can_write);
		POSTGIS_RT_DEBUGF(5, "Result %d, Create Options %s", call_cntr, drv_set2[call_cntr].create_options);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(drv_set2[call_cntr].short_name);
		pfree(drv_set2[call_cntr].long_name);
		pfree(drv_set2[call_cntr].create_options);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(drv_set2);
		SRF_RETURN_DONE(funcctx);
	}
}

/************************************************************************
 * ST_Contour(
 *   rast raster,
 *   bandnumber integer DEFAULT 1,
 *   level_interval float8 DEFAULT 100.0,
 *   level_base float8 DEFAULT 0.0,
 *   fixed_levels float8[] DEFAULT ARRAY[]::float8[],
 *   polygonize boolean DEFAULT false
 * )
 * RETURNS table(geom geometry, value float8, id integer)
 ************************************************************************/

PG_FUNCTION_INFO_V1(RASTER_Contour);
Datum RASTER_Contour(PG_FUNCTION_ARGS)
{
	/* For return values */
	typedef struct gdal_contour_result_t {
		size_t ncontours;
		struct rt_contour_t *contours;
	} gdal_contour_result_t;

	FuncCallContext *funcctx;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		TupleDesc tupdesc;
		gdal_contour_result_t *result;
		rt_pgraster *pgraster = NULL;

		/* For reading the raster */
		int src_srid = SRID_UNKNOWN;
		char *src_srs = NULL;
		rt_raster raster = NULL;
		int num_bands;
		int band, rv;

		/* For reading the levels[] */
		ArrayType *array;
		size_t array_size = 0;

		/* For the level parameters */
		double level_base = 0.0;
		double level_interval = 100.0;
		double *fixed_levels = NULL;
		size_t fixed_levels_count = 0;

		/* for the polygonize flag */
		bool polygonize = false;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* To carry the output from rt_raster_gdal_contour */
		result = palloc0(sizeof(gdal_contour_result_t));

		/* Build a tuple descriptor for our return result */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
			MemoryContextSwitchTo(oldcontext);
			ereport(ERROR, (
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg(
					"function returning record called in context "
					"that cannot accept type record"
				)
			));
		}
		BlessTupleDesc(tupdesc);
		funcctx->tuple_desc = tupdesc;

		/* Read the raster */
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		raster = rt_raster_deserialize(pgraster, FALSE);
		num_bands = rt_raster_get_num_bands(raster);
		src_srid = clamp_srid(rt_raster_get_srid(raster));
		src_srs = rtpg_getSR(src_srid);

		/* Read the band number */
		band = PG_GETARG_INT32(1);
		if (band < 1 || band > num_bands) {
			elog(ERROR, "%s: band number must be between 1 and %u inclusive", __func__, num_bands);
		}

		/* Read the level_interval */
		level_interval = PG_GETARG_FLOAT8(2);

		/* Read the level_base */
		level_base = PG_GETARG_FLOAT8(3);

		if (level_interval <= 0.0) {
			elog(ERROR, "%s: level interval must be greater than zero", __func__);
		}

		/* Read the polygonize flag */
		polygonize = PG_GETARG_BOOL(5);

		/* Read the levels array */
		array = PG_GETARG_ARRAYTYPE_P(4);
		array_size = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
		if (array_size > 0) {
			Datum value;
			bool isnull;
			ArrayIterator iterator = array_create_iterator(array, 0, NULL);
			fixed_levels = palloc0(array_size * sizeof(double));
			while (array_iterate(iterator, &value, &isnull))
			{
				/* Skip nulls */
				if (isnull)
					continue;

				/* Can out if for some reason we are about to blow memory */
				if (fixed_levels_count >= array_size)
					break;

				fixed_levels[fixed_levels_count++] = DatumGetFloat8(value);
			}
		}

		/* Run the contouring routine */
		rv = rt_raster_gdal_contour(
			/* input parameters */
			raster,
			band,
			src_srid,
			src_srs,
			level_interval,
			level_base,
			fixed_levels_count,
			fixed_levels,
			polygonize,
			/* output parameters */
			&(result->ncontours),
			&(result->contours)
			);

		/* No-op on bad return */
		if (rv == FALSE) {
			funcctx = SRF_PERCALL_SETUP();
			SRF_RETURN_DONE(funcctx);
		}

		funcctx->user_fctx = result;
		funcctx->max_calls = result->ncontours;
		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	/* do when there is more left to send */
	if (funcctx->call_cntr < funcctx->max_calls) {

		HeapTuple tuple;
		Datum srf_result;
		Datum values[3] = {0, 0, 0};
		bool nulls[3] = {0, 0, 0};

		gdal_contour_result_t *result = funcctx->user_fctx;
		struct rt_contour_t c = result->contours[funcctx->call_cntr];

		if (c.geom) {
			values[0] = PointerGetDatum(c.geom);
			values[1] = Int32GetDatum(c.id);
			values[2] = Float8GetDatum(c.elevation);
		}
		else {
			nulls[0] = true;
			nulls[1] = true;
			nulls[2] = true;
		}

		/* return a tuple */
		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
		srf_result = HeapTupleGetDatum(tuple);
		SRF_RETURN_NEXT(funcctx, srf_result);
	}
	else {
		SRF_RETURN_DONE(funcctx);
	}
}

/************************************************************************
 *  RASTER_InterpolateRaster
 *
 * CREATE OR REPLACE FUNCTION ST_InterpolateRaster(
 *   geom geometry,
 *   rast raster,
 *   options text,
 *   bandnumber integer DEFAULT 1
 * 	) RETURNS raster
 *
 * https://gdal.org/api/gdal_alg.html?highlight=contour#_CPPv414GDALGridCreate17GDALGridAlgorithmPKv7GUInt32PKdPKdPKddddd7GUInt327GUInt3212GDALDataTypePv16GDALProgressFuncPv
 ************************************************************************/

PG_FUNCTION_INFO_V1(RASTER_InterpolateRaster);
Datum RASTER_InterpolateRaster(PG_FUNCTION_ARGS)
{
	rt_pgraster *in_pgrast = NULL;
	rt_pgraster *out_pgrast = NULL;
	rt_raster in_rast = NULL;
	rt_raster out_rast = NULL;
	uint32_t out_rast_bands[1] = {0};
	rt_band in_band = NULL;
	rt_band out_band = NULL;
	int band_number;
	uint16_t in_band_width, in_band_height;
	uint32_t npoints;
	rt_pixtype in_band_pixtype;
	GDALDataType in_band_gdaltype;
	size_t in_band_gdaltype_size;

	rt_envelope env;

	GDALGridAlgorithm algorithm;
	text *options_txt = NULL;
	void *options_struct = NULL;
	CPLErr err;
	uint8_t *out_data;
	rt_errorstate rterr;

	/* Input points */
	LWPOINTITERATOR *iterator;
	POINT4D pt;
	size_t coord_count = 0;
	LWGEOM *lwgeom;
	double *xcoords, *ycoords, *zcoords;

	GSERIALIZED *gser = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Z value is required to drive the grid heights */
	if (!gserialized_has_z(gser))
		elog(ERROR, "%s: input geometry does not have Z values", __func__);

	/* Cannot process empties */
	if (gserialized_is_empty(gser))
		PG_RETURN_NULL();

	in_pgrast = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(2));
	in_rast = rt_raster_deserialize(in_pgrast, FALSE);
	if (!in_rast)
		elog(ERROR, "%s: Could not deserialize raster", __func__);

	/* GDAL cannot grid a skewed raster */
	if (rt_raster_get_x_skew(in_rast) != 0.0 ||
	    rt_raster_get_y_skew(in_rast) != 0.0) {
		elog(ERROR, "%s: Cannot generate a grid into a skewed raster",__func__);
	}

	/* Flat JSON map of options from user */
	options_txt = PG_GETARG_TEXT_P(1);
	/* 1-base band number from user */
	band_number = PG_GETARG_INT32(3);
	if (band_number < 1)
		elog(ERROR, "%s: Invalid band number %d", __func__, band_number);

	lwgeom = lwgeom_from_gserialized(gser);
	npoints = lwgeom_count_vertices(lwgeom);
	/* This shouldn't happen, but just in case... */
	if (npoints < 1)
		elog(ERROR, "%s: Geometry has no points", __func__);

	in_band = rt_raster_get_band(in_rast, band_number-1);
	if (!in_band)
		elog(ERROR, "%s: Cannot access raster band %d", __func__, band_number);


	rterr = rt_raster_get_envelope(in_rast, &env);
	if (rterr == ES_ERROR)
		elog(ERROR, "%s: Unable to calculate envelope", __func__);

	/* Get geometry of input raster */
	in_band_width = rt_band_get_width(in_band);
	in_band_height = rt_band_get_height(in_band);
	in_band_pixtype = rt_band_get_pixtype(in_band);
	in_band_gdaltype = rt_util_pixtype_to_gdal_datatype(in_band_pixtype);
	in_band_gdaltype_size = GDALGetDataTypeSize(in_band_gdaltype) / 8;

	/* Quickly copy options struct into local memory context, so we */
	/* don't have malloc'ed memory lying around */
	// if (err == CE_None && options_struct) {
	// 	void *tmp = options_struct;
	// 	switch (algorithm) {
	// 		case GGA_InverseDistanceToAPower:
	// 			options_struct = palloc(sizeof(GDALGridInverseDistanceToAPowerOptions));
	// 			memcpy(options_struct, tmp, sizeof(GDALGridInverseDistanceToAPowerOptions));
	// 			break;
	// 		case GGA_InverseDistanceToAPowerNearestNeighbor:
	// 			options_struct = palloc(sizeof(GDALGridInverseDistanceToAPowerNearestNeighborOptions));
	// 			memcpy(options_struct, tmp, sizeof(GDALGridInverseDistanceToAPowerNearestNeighborOptions));
	// 			break;
	// 		case GGA_MovingAverage:
	// 			options_struct = palloc(sizeof(GDALGridMovingAverageOptions));
	// 			memcpy(options_struct, tmp, sizeof(GDALGridMovingAverageOptions));
	// 			break;
	// 		case GGA_NearestNeighbor:
	// 			options_struct = palloc(sizeof(GDALGridNearestNeighborOptions));
	// 			memcpy(options_struct, tmp, sizeof(GDALGridNearestNeighborOptions));
	// 			break;
	// 		case GGA_Linear:
	// 			options_struct = palloc(sizeof(GDALGridLinearOptions));
	// 			memcpy(options_struct, tmp, sizeof(GDALGridLinearOptions));
	// 			break;
	// 		default:
	// 			elog(ERROR, "%s: Unsupported gridding algorithm %d", __func__, algorithm);
	// 	}
	// 	free(tmp);
	// }

	/* Prepare destination grid buffer for output */
	out_data = palloc(in_band_gdaltype_size * in_band_width * in_band_height);

	/* Prepare input points for processing */
	xcoords = palloc(sizeof(double) * npoints);
	ycoords = palloc(sizeof(double) * npoints);
	zcoords = palloc(sizeof(double) * npoints);

	/* Populate input points */
	iterator = lwpointiterator_create(lwgeom);
	while(lwpointiterator_next(iterator, &pt) == LW_SUCCESS) {
		if (coord_count >= npoints)
			elog(ERROR, "%s: More points from iterator than expected", __func__);
		xcoords[coord_count] = pt.x;
		ycoords[coord_count] = pt.y;
		zcoords[coord_count] = pt.z;
		coord_count++;
	}
	lwpointiterator_destroy(iterator);

	/* Extract algorithm and options from options text */
	/* This malloc's the options struct, so clean up right away */
	err = ParseAlgorithmAndOptions(
		text_to_cstring(options_txt),
		&algorithm,
		&options_struct);
	if (err != CE_None) {
		if (options_struct) free(options_struct);
		elog(ERROR, "%s: Unable to parse options string: %s", __func__, CPLGetLastErrorMsg());
	}

	/* Run the gridding algorithm */
	err = GDALGridCreate(
	        algorithm, options_struct,
	        npoints, xcoords, ycoords, zcoords,
	        env.MinX, env.MaxX, env.MinY, env.MaxY,
	        in_band_width, in_band_height,
	        in_band_gdaltype, out_data,
	        NULL, /* GDALProgressFunc */
	        NULL /* ProgressArgs */
	        );

	/* Quickly clean up malloc'ed memory */
	if (options_struct)
		free(options_struct);

	if (err != CE_None) {
		elog(ERROR, "%s: GDALGridCreate failed: %s", __func__, CPLGetLastErrorMsg());
	}

	out_rast_bands[0] = band_number-1;
	out_rast = rt_raster_from_band(in_rast, out_rast_bands, 1);
	out_band = rt_raster_get_band(out_rast, 0);
	if (!out_band)
		elog(ERROR, "%s: Cannot access output raster band", __func__);

	/* Copy the data from the output buffer into the destination band */
	for (uint32_t y = 0; y < in_band_height; y++) {
		size_t offset = (in_band_height-y-1) * (in_band_gdaltype_size * in_band_width);
		rterr = rt_band_set_pixel_line(out_band, 0, y, out_data + offset, in_band_width);
	}

	out_pgrast = rt_raster_serialize(out_rast);
	rt_raster_destroy(out_rast);
	rt_raster_destroy(in_rast);

	if (NULL == out_pgrast) PG_RETURN_NULL();

	SET_VARSIZE(out_pgrast, out_pgrast->size);
	PG_RETURN_POINTER(out_pgrast);
}


/************************************************************************
 * Warp a raster using GDAL Warp API
 ************************************************************************/

PG_FUNCTION_INFO_V1(RASTER_GDALWarp);
Datum RASTER_GDALWarp(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrast = NULL;
	rt_raster raster = NULL;
	rt_raster rast = NULL;

	text *algtext = NULL;
	char *algchar = NULL;
	GDALResampleAlg alg = GRA_NearestNeighbour;
	double max_err = 0.125;

	int src_srid = SRID_UNKNOWN;
	char *src_srs = NULL;
	int dst_srid = SRID_UNKNOWN;
	char *dst_srs = NULL;
	int no_srid = 0;

	double scale[2] = {0};
	double *scale_x = NULL;
	double *scale_y = NULL;

	double gridw[2] = {0};
	double *grid_xw = NULL;
	double *grid_yw = NULL;

	double skew[2] = {0};
	double *skew_x = NULL;
	double *skew_y = NULL;

	int dim[2] = {0};
	int *dim_x = NULL;
	int *dim_y = NULL;

	POSTGIS_RT_DEBUG(3, "RASTER_GDALWarp: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_GDALWarp: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* resampling algorithm */
	if (!PG_ARGISNULL(1)) {
		algtext = PG_GETARG_TEXT_P(1);
		algchar = rtpg_trim(rtpg_strtoupper(text_to_cstring(algtext)));
		alg = rt_util_gdal_resample_alg(algchar);
	}
	POSTGIS_RT_DEBUGF(4, "Resampling algorithm: %d", alg);

	/* max error */
	if (!PG_ARGISNULL(2)) {
		max_err = PG_GETARG_FLOAT8(2);
		if (max_err < 0.) max_err = 0.;
	}
	POSTGIS_RT_DEBUGF(4, "max_err: %f", max_err);

	/* source SRID */
	src_srid = clamp_srid(rt_raster_get_srid(raster));
	POSTGIS_RT_DEBUGF(4, "source SRID: %d", src_srid);

	/* target SRID */
	if (!PG_ARGISNULL(3)) {
		dst_srid = clamp_srid(PG_GETARG_INT32(3));
		if (dst_srid == SRID_UNKNOWN) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_GDALWarp: %d is an invalid target SRID", dst_srid);
			PG_RETURN_NULL();
		}
	}
	else
		dst_srid = src_srid;
	POSTGIS_RT_DEBUGF(4, "destination SRID: %d", dst_srid);

	/* target SRID != src SRID, error */
	if (src_srid == SRID_UNKNOWN && dst_srid != src_srid) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_GDALWarp: Input raster has unknown (%d) SRID", src_srid);
		PG_RETURN_NULL();
	}
	/* target SRID == src SRID, no reprojection */
	else if (dst_srid == src_srid) {
		no_srid = 1;
	}

	/* scale x */
	if (!PG_ARGISNULL(4)) {
		scale[0] = PG_GETARG_FLOAT8(4);
		if (FLT_NEQ(scale[0], 0.0))
			scale_x = &scale[0];
	}

	/* scale y */
	if (!PG_ARGISNULL(5)) {
		scale[1] = PG_GETARG_FLOAT8(5);
		if (FLT_NEQ(scale[1], 0.0))
			scale_y = &scale[1];
	}

	/* grid alignment x */
	if (!PG_ARGISNULL(6)) {
		gridw[0] = PG_GETARG_FLOAT8(6);
		grid_xw = &gridw[0];
	}

	/* grid alignment y */
	if (!PG_ARGISNULL(7)) {
		gridw[1] = PG_GETARG_FLOAT8(7);
		grid_yw = &gridw[1];
	}

	/* skew x */
	if (!PG_ARGISNULL(8)) {
		skew[0] = PG_GETARG_FLOAT8(8);
		if (FLT_NEQ(skew[0], 0.0))
			skew_x = &skew[0];
	}

	/* skew y */
	if (!PG_ARGISNULL(9)) {
		skew[1] = PG_GETARG_FLOAT8(9);
		if (FLT_NEQ(skew[1], 0.0))
			skew_y = &skew[1];
	}

	/* width */
	if (!PG_ARGISNULL(10)) {
		dim[0] = PG_GETARG_INT32(10);
		if (dim[0] < 0) dim[0] = 0;
		if (dim[0] > 0) dim_x = &dim[0];
	}

	/* height */
	if (!PG_ARGISNULL(11)) {
		dim[1] = PG_GETARG_INT32(11);
		if (dim[1] < 0) dim[1] = 0;
		if (dim[1] > 0) dim_y = &dim[1];
	}

	/* check that at least something is to be done */
	if (
		(dst_srid == SRID_UNKNOWN) &&
		(scale_x == NULL) && (scale_y == NULL) &&
		(grid_xw == NULL) && (grid_yw == NULL) &&
		(skew_x == NULL) && (skew_y == NULL) &&
		(dim_x == NULL) && (dim_y == NULL)
	) {
		elog(NOTICE, "No resampling parameters provided.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}
	/* both values of alignment must be provided if any one is provided */
	else if (
		(grid_xw != NULL && grid_yw == NULL) ||
		(grid_xw == NULL && grid_yw != NULL)
	) {
		elog(NOTICE, "Values must be provided for both X and Y when specifying the alignment.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}
	/* both values of scale must be provided if any one is provided */
	else if (
		(scale_x != NULL && scale_y == NULL) ||
		(scale_x == NULL && scale_y != NULL)
	) {
		elog(NOTICE, "Values must be provided for both X and Y when specifying the scale.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}
	/* scale and width/height provided */
	else if (
		(scale_x != NULL || scale_y != NULL) &&
		(dim_x != NULL || dim_y != NULL)
	) {
		elog(NOTICE, "Scale X/Y and width/height are mutually exclusive.  Only provide one.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	/* get srses from srids */
	if (!no_srid) {
		/* source srs */
		src_srs = rtpg_getSR(src_srid);
		if (NULL == src_srs) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_GDALWarp: Input raster has unknown SRID (%d)", src_srid);
			PG_RETURN_NULL();
		}
		POSTGIS_RT_DEBUGF(4, "src srs: %s", src_srs);

		dst_srs = rtpg_getSR(dst_srid);
		if (NULL == dst_srs) {
			pfree(src_srs);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_GDALWarp: Target SRID (%d) is unknown", dst_srid);
			PG_RETURN_NULL();
		}
		POSTGIS_RT_DEBUGF(4, "dst srs: %s", dst_srs);
	}

	rast = rt_raster_gdal_warp(
		raster,
		src_srs, dst_srs,
		scale_x, scale_y,
		dim_x, dim_y,
		NULL, NULL,
		grid_xw, grid_yw,
		skew_x, skew_y,
		alg, max_err);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!no_srid) {
		pfree(src_srs);
		pfree(dst_srs);
	}
	if (!rast) {
		elog(ERROR, "RASTER_band: Could not create transformed raster");
		PG_RETURN_NULL();
	}

	/* add target SRID */
	rt_raster_set_srid(rast, dst_srid);

	pgrast = rt_raster_serialize(rast);
	rt_raster_destroy(rast);

	if (NULL == pgrast) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_GDALWarp: done");

	SET_VARSIZE(pgrast, pgrast->size);
	PG_RETURN_POINTER(pgrast);
}

