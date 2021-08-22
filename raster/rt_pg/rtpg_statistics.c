/*
 *
 * WKTRaster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright (C) 2013 Bborie Park <dustymugs@gmail.com>
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
#include <fmgr.h>
#include <utils/builtins.h> /* for text_to_cstring() */
#include "utils/lsyscache.h" /* for get_typlenbyvalalign */
#include "utils/array.h" /* for ArrayType */
#include "catalog/pg_type.h" /* for INT2OID, INT4OID, FLOAT4OID, FLOAT8OID and TEXTOID */
#include <executor/spi.h>
#include <funcapi.h> /* for SRF */

#include "../../postgis_config.h"


#include "access/htup_details.h" /* for heap_form_tuple() */


#include "rtpostgis.h"

/* Get summary stats */
Datum RASTER_summaryStats(PG_FUNCTION_ARGS);
Datum RASTER_summaryStatsCoverage(PG_FUNCTION_ARGS);

Datum RASTER_summaryStats_transfn(PG_FUNCTION_ARGS);
Datum RASTER_summaryStats_finalfn(PG_FUNCTION_ARGS);

/* get histogram */
Datum RASTER_histogram(PG_FUNCTION_ARGS);

/* get quantiles */
Datum RASTER_quantile(PG_FUNCTION_ARGS);

/* get counts of values */
Datum RASTER_valueCount(PG_FUNCTION_ARGS);
Datum RASTER_valueCountCoverage(PG_FUNCTION_ARGS);

#define VALUES_LENGTH 6

/**
 * Get summary stats of a band
 */
PG_FUNCTION_INFO_V1(RASTER_summaryStats);
Datum RASTER_summaryStats(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int32_t bandindex = 1;
	bool exclude_nodata_value = TRUE;
	int num_bands = 0;
	double sample = 0;
	rt_bandstats stats = NULL;

	TupleDesc tupdesc;
	Datum values[VALUES_LENGTH];
	bool nulls[VALUES_LENGTH];
	HeapTuple tuple;
	Datum result;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_summaryStats: Cannot deserialize raster");
		PG_RETURN_NULL();
	}

	/* band index is 1-based */
	if (!PG_ARGISNULL(1))
		bandindex = PG_GETARG_INT32(1);
	num_bands = rt_raster_get_num_bands(raster);
	if (bandindex < 1 || bandindex > num_bands) {
		elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* exclude_nodata_value flag */
	if (!PG_ARGISNULL(2))
		exclude_nodata_value = PG_GETARG_BOOL(2);

	/* sample % */
	if (!PG_ARGISNULL(3)) {
		sample = PG_GETARG_FLOAT8(3);
		if (sample < 0 || sample > 1) {
			elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			PG_RETURN_NULL();
		}
		else if (FLT_EQ(sample, 0.0))
			sample = 1;
	}
	else
		sample = 1;

	/* get band */
	band = rt_raster_get_band(raster, bandindex - 1);
	if (!band) {
		elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* we don't need the raw values, hence the zero parameter */
	stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, NULL, NULL, NULL);
	rt_band_destroy(band);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (NULL == stats) {
		elog(NOTICE, "Cannot compute summary statistics for band at index %d. Returning NULL", bandindex);
		PG_RETURN_NULL();
	}

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

	memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

	values[0] = Int64GetDatum(stats->count);
	if (stats->count > 0) {
		values[1] = Float8GetDatum(stats->sum);
		values[2] = Float8GetDatum(stats->mean);
		values[3] = Float8GetDatum(stats->stddev);
		values[4] = Float8GetDatum(stats->min);
		values[5] = Float8GetDatum(stats->max);
	}
	else {
		nulls[1] = TRUE;
		nulls[2] = TRUE;
		nulls[3] = TRUE;
		nulls[4] = TRUE;
		nulls[5] = TRUE;
	}

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	pfree(stats);

	PG_RETURN_DATUM(result);
}

/**
 * Get summary stats of a coverage for a specific band
 */
PG_FUNCTION_INFO_V1(RASTER_summaryStatsCoverage);
Datum RASTER_summaryStatsCoverage(PG_FUNCTION_ARGS)
{
	text *tablenametext = NULL;
	char *tablename = NULL;
	text *colnametext = NULL;
	char *colname = NULL;
	int32_t bandindex = 1;
	bool exclude_nodata_value = TRUE;
	double sample = 0;

	int len = 0;
	char *sql = NULL;
	int spi_result;
	Portal portal;
	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	Datum datum;
	bool isNull = FALSE;

	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int num_bands = 0;
	uint64_t cK = 0;
	double cM = 0;
	double cQ = 0;
	rt_bandstats stats = NULL;
	rt_bandstats rtn = NULL;

	Datum values[VALUES_LENGTH];
	bool nulls[VALUES_LENGTH];
	Datum result;

	/* tablename is null, return null */
	if (PG_ARGISNULL(0)) {
		elog(NOTICE, "Table name must be provided");
		PG_RETURN_NULL();
	}
	tablenametext = PG_GETARG_TEXT_P(0);
	tablename = text_to_cstring(tablenametext);
	if (!strlen(tablename)) {
		elog(NOTICE, "Table name must be provided");
		PG_RETURN_NULL();
	}

	/* column name is null, return null */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Column name must be provided");
		PG_RETURN_NULL();
	}
	colnametext = PG_GETARG_TEXT_P(1);
	colname = text_to_cstring(colnametext);
	if (!strlen(colname)) {
		elog(NOTICE, "Column name must be provided");
		PG_RETURN_NULL();
	}

	/* band index is 1-based */
	if (!PG_ARGISNULL(2))
		bandindex = PG_GETARG_INT32(2);

	/* exclude_nodata_value flag */
	if (!PG_ARGISNULL(3))
		exclude_nodata_value = PG_GETARG_BOOL(3);

	/* sample % */
	if (!PG_ARGISNULL(4)) {
		sample = PG_GETARG_FLOAT8(4);
		if (sample < 0 || sample > 1) {
			elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
		}
		else if (FLT_EQ(sample, 0.0))
			sample = 1;
	}
	else
		sample = 1;

	/* iterate through rasters of coverage */
	/* connect to database */
	spi_result = SPI_connect();
	if (spi_result != SPI_OK_CONNECT) {
		pfree(sql);
		elog(ERROR, "RASTER_summaryStatsCoverage: Cannot connect to database using SPI");
		PG_RETURN_NULL();
	}

	/* create sql */
	len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
	sql = (char *) palloc(len);
	if (NULL == sql) {
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		elog(ERROR, "RASTER_summaryStatsCoverage: Cannot allocate memory for sql");
		PG_RETURN_NULL();
	}

	/* get cursor */
	snprintf(sql, len, "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL", colname, tablename, colname);
	portal = SPI_cursor_open_with_args(
		"coverage",
		sql,
		0, NULL,
		NULL, NULL,
		TRUE, 0
	);
	pfree(sql);

	/* process resultset */
	SPI_cursor_fetch(portal, TRUE, 1);
	while (SPI_processed == 1 && SPI_tuptable != NULL) {
		tupdesc = SPI_tuptable->tupdesc;
		tuple = SPI_tuptable->vals[0];

		datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
		if (SPI_result == SPI_ERROR_NOATTRIBUTE) {
			SPI_freetuptable(SPI_tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			elog(ERROR, "RASTER_summaryStatsCoverage: Cannot get raster of coverage");
			PG_RETURN_NULL();
		}
		else if (isNull) {
			SPI_cursor_fetch(portal, TRUE, 1);
			continue;
		}

		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			SPI_freetuptable(SPI_tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			elog(ERROR, "RASTER_summaryStatsCoverage: Cannot deserialize raster");
			PG_RETURN_NULL();
		}

		/* inspect number of bands */
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

			rt_raster_destroy(raster);

			SPI_freetuptable(SPI_tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			PG_RETURN_NULL();
		}

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);

			rt_raster_destroy(raster);

			SPI_freetuptable(SPI_tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			PG_RETURN_NULL();
		}

		/* we don't need the raw values, hence the zero parameter */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, &cK, &cM, &cQ);

		rt_band_destroy(band);
		rt_raster_destroy(raster);

		if (NULL == stats) {
			elog(NOTICE, "Cannot compute summary statistics for band at index %d. Returning NULL", bandindex);

			SPI_freetuptable(SPI_tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			PG_RETURN_NULL();
		}

		/* initialize rtn */
		if (stats->count > 0) {
			if (NULL == rtn) {
				rtn = (rt_bandstats) SPI_palloc(sizeof(struct rt_bandstats_t));
				if (NULL == rtn) {
					SPI_freetuptable(SPI_tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					elog(ERROR, "RASTER_summaryStatsCoverage: Cannot allocate memory for summary stats of coverage");
					PG_RETURN_NULL();
				}

				rtn->sample = stats->sample;
				rtn->count = stats->count;
				rtn->min = stats->min;
				rtn->max = stats->max;
				rtn->sum = stats->sum;
				rtn->mean = stats->mean;
				rtn->stddev = -1;

				rtn->values = NULL;
				rtn->sorted = 0;
			}
			else {
				rtn->count += stats->count;
				rtn->sum += stats->sum;

				if (stats->min < rtn->min)
					rtn->min = stats->min;
				if (stats->max > rtn->max)
					rtn->max = stats->max;
			}
		}

		pfree(stats);

		/* next record */
		SPI_cursor_fetch(portal, TRUE, 1);
	}

	if (SPI_tuptable) SPI_freetuptable(SPI_tuptable);
	SPI_cursor_close(portal);
	SPI_finish();

	if (NULL == rtn) {
		elog(ERROR, "RASTER_summaryStatsCoverage: Cannot compute coverage summary stats");
		PG_RETURN_NULL();
	}

	/* coverage mean and deviation */
	rtn->mean = rtn->sum / rtn->count;
	/* sample deviation */
	if (rtn->sample > 0 && rtn->sample < 1)
		rtn->stddev = sqrt(cQ / (rtn->count - 1));
	/* standard deviation */
	else
		rtn->stddev = sqrt(cQ / rtn->count);

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

	memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

	values[0] = Int64GetDatum(rtn->count);
	if (rtn->count > 0) {
		values[1] = Float8GetDatum(rtn->sum);
		values[2] = Float8GetDatum(rtn->mean);
		values[3] = Float8GetDatum(rtn->stddev);
		values[4] = Float8GetDatum(rtn->min);
		values[5] = Float8GetDatum(rtn->max);
	}
	else {
		nulls[1] = TRUE;
		nulls[2] = TRUE;
		nulls[3] = TRUE;
		nulls[4] = TRUE;
		nulls[5] = TRUE;
	}

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	pfree(rtn);

	PG_RETURN_DATUM(result);
}

/* ---------------------------------------------------------------- */
/* Aggregate ST_SummaryStats                                        */
/* ---------------------------------------------------------------- */

typedef struct rtpg_summarystats_arg_t *rtpg_summarystats_arg;
struct rtpg_summarystats_arg_t {
	rt_bandstats stats;

	/* coefficients for one-pass standard deviation */
	uint64_t cK;
	double cM;
	double cQ;

	int32_t band_index; /* one-based */
	bool exclude_nodata_value;
	double sample; /* value between 0 and 1 */
};

static void
rtpg_summarystats_arg_destroy(rtpg_summarystats_arg arg) {
	if (arg->stats != NULL)
		pfree(arg->stats);

	pfree(arg);
}

static rtpg_summarystats_arg
rtpg_summarystats_arg_init() {
	rtpg_summarystats_arg arg = NULL;

	arg = palloc(sizeof(struct rtpg_summarystats_arg_t));
	if (arg == NULL) {
		elog(
			ERROR,
			"rtpg_summarystats_arg_init: Cannot allocate memory for function arguments"
		);
		return NULL;
	}

	arg->stats = (rt_bandstats) palloc(sizeof(struct rt_bandstats_t));
	if (arg->stats == NULL) {
		rtpg_summarystats_arg_destroy(arg);
		elog(
			ERROR,
			"rtpg_summarystats_arg_init: Cannot allocate memory for stats function argument"
		);
		return NULL;
	}

	arg->stats->sample = 0;
	arg->stats->count = 0;
	arg->stats->min = 0;
	arg->stats->max = 0;
	arg->stats->sum = 0;
	arg->stats->mean = 0;
	arg->stats->stddev = -1;
	arg->stats->values = NULL;
	arg->stats->sorted = 0;

	arg->cK = 0;
	arg->cM = 0;
	arg->cQ = 0;

	arg->band_index = 1;
	arg->exclude_nodata_value = TRUE;
	arg->sample = 1;

	return arg;
}

PG_FUNCTION_INFO_V1(RASTER_summaryStats_transfn);
Datum RASTER_summaryStats_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext;
	MemoryContext oldcontext;
	rtpg_summarystats_arg state = NULL;
	bool skiparg = FALSE;

	int i = 0;

	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int num_bands = 0;
	rt_bandstats stats = NULL;

	POSTGIS_RT_DEBUG(3, "Starting...");

	/* cannot be called directly as this is exclusive aggregate function */
	if (!AggCheckCallContext(fcinfo, &aggcontext)) {
		elog(
			ERROR,
			"RASTER_summaryStats_transfn: Cannot be called in a non-aggregate context"
		);
		PG_RETURN_NULL();
	}

	/* switch to aggcontext */
	oldcontext = MemoryContextSwitchTo(aggcontext);

	if (PG_ARGISNULL(0)) {
		POSTGIS_RT_DEBUG(3, "Creating state variable");

		state = rtpg_summarystats_arg_init();
		if (state == NULL) {
			MemoryContextSwitchTo(oldcontext);
			elog(
				ERROR,
				"RASTER_summaryStats_transfn: Cannot allocate memory for state variable"
			);
			PG_RETURN_NULL();
		}

		skiparg = FALSE;
	}
	else {
		POSTGIS_RT_DEBUG(3, "State variable already exists");
		state = (rtpg_summarystats_arg) PG_GETARG_POINTER(0);
		skiparg = TRUE;
	}

	/* raster arg is NOT NULL */
	if (!PG_ARGISNULL(1)) {
		/* deserialize raster */
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		/* Get raster object */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (raster == NULL) {

			rtpg_summarystats_arg_destroy(state);
			PG_FREE_IF_COPY(pgraster, 1);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_summaryStats_transfn: Cannot deserialize raster");
			PG_RETURN_NULL();
		}
	}

	do {
		Oid calltype;
		int nargs = 0;

		if (skiparg)
			break;

		/* 4 or 5 total possible args */
		nargs = PG_NARGS();
		POSTGIS_RT_DEBUGF(4, "nargs = %d", nargs);

		for (i = 2; i < nargs; i++) {
			if (PG_ARGISNULL(i))
				continue;

			calltype = get_fn_expr_argtype(fcinfo->flinfo, i);

			/* band index */
			if (
				(calltype == INT2OID || calltype == INT4OID) &&
				i == 2
			) {
				if (calltype == INT2OID)
					state->band_index = PG_GETARG_INT16(i);
				else
					state->band_index = PG_GETARG_INT32(i);

				/* basic check, > 0 */
				if (state->band_index < 1) {

					rtpg_summarystats_arg_destroy(state);
					if (raster != NULL) {
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 1);
					}

					MemoryContextSwitchTo(oldcontext);
					elog(
						ERROR,
						"RASTER_summaryStats_transfn: Invalid band index (must use 1-based). Returning NULL"
					);
					PG_RETURN_NULL();
				}
			}
			/* exclude_nodata_value */
			else if (
				calltype == BOOLOID && (
					i == 2 || i == 3
				)
			) {
				state->exclude_nodata_value = PG_GETARG_BOOL(i);
			}
			/* sample rate */
			else if (
				(calltype == FLOAT4OID || calltype == FLOAT8OID) &&
				(i == 3 || i == 4)
			) {
				if (calltype == FLOAT4OID)
					state->sample = PG_GETARG_FLOAT4(i);
				else
					state->sample = PG_GETARG_FLOAT8(i);

				/* basic check, 0 <= sample <= 1 */
				if (state->sample < 0. || state->sample > 1.) {

					rtpg_summarystats_arg_destroy(state);
					if (raster != NULL) {
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 1);
					}

					MemoryContextSwitchTo(oldcontext);
					elog(
						ERROR,
						"Invalid sample percentage (must be between 0 and 1). Returning NULL"
					);

					PG_RETURN_NULL();
				}
				else if (FLT_EQ(state->sample, 0.0))
					state->sample = 1;
			}
			/* unknown arg */
			else {
				rtpg_summarystats_arg_destroy(state);
				if (raster != NULL) {
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 1);
				}

				MemoryContextSwitchTo(oldcontext);
				elog(
					ERROR,
					"RASTER_summaryStats_transfn: Unknown function parameter at index %d",
					i
				);
				PG_RETURN_NULL();
			}
		}
	}
	while (0);

	/* null raster, return */
	if (PG_ARGISNULL(1)) {
		POSTGIS_RT_DEBUG(4, "NULL raster so processing required");
		MemoryContextSwitchTo(oldcontext);
		PG_RETURN_POINTER(state);
	}

	/* inspect number of bands */
	num_bands = rt_raster_get_num_bands(raster);
	if (state->band_index > num_bands) {
		elog(
			NOTICE,
			"Raster does not have band at index %d. Skipping raster",
			state->band_index
		);

		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 1);

		MemoryContextSwitchTo(oldcontext);
		PG_RETURN_POINTER(state);
	}

	/* get band */
	band = rt_raster_get_band(raster, state->band_index - 1);
	if (!band) {
		elog(
			NOTICE, "Cannot find band at index %d. Skipping raster",
			state->band_index
		);

		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 1);

		MemoryContextSwitchTo(oldcontext);
		PG_RETURN_POINTER(state);
	}

	/* we don't need the raw values, hence the zero parameter */
	stats = rt_band_get_summary_stats(
		band, (int) state->exclude_nodata_value,
		state->sample, 0,
		&(state->cK), &(state->cM), &(state->cQ)
	);

	rt_band_destroy(band);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 1);

	if (NULL == stats) {
		elog(
			NOTICE,
			"Cannot compute summary statistics for band at index %d. Returning NULL",
			state->band_index
		);

		rtpg_summarystats_arg_destroy(state);

		MemoryContextSwitchTo(oldcontext);
		PG_RETURN_NULL();
	}

	if (stats->count > 0) {
		if (state->stats->count < 1) {
			state->stats->sample = stats->sample;
			state->stats->count = stats->count;
			state->stats->min = stats->min;
			state->stats->max = stats->max;
			state->stats->sum = stats->sum;
			state->stats->mean = stats->mean;
			state->stats->stddev = -1;
		}
		else {
			state->stats->count += stats->count;
			state->stats->sum += stats->sum;

			if (stats->min < state->stats->min)
				state->stats->min = stats->min;
			if (stats->max > state->stats->max)
				state->stats->max = stats->max;
		}
	}

	pfree(stats);

	/* switch back to local context */
	MemoryContextSwitchTo(oldcontext);

	POSTGIS_RT_DEBUG(3, "Finished");

	PG_RETURN_POINTER(state);
}

PG_FUNCTION_INFO_V1(RASTER_summaryStats_finalfn);
Datum RASTER_summaryStats_finalfn(PG_FUNCTION_ARGS)
{
	rtpg_summarystats_arg state = NULL;

	TupleDesc tupdesc;
	HeapTuple tuple;
	Datum values[VALUES_LENGTH];
	bool nulls[VALUES_LENGTH];
	Datum result;

	POSTGIS_RT_DEBUG(3, "Starting...");

	/* cannot be called directly as this is exclusive aggregate function */
	if (!AggCheckCallContext(fcinfo, NULL)) {
		elog(ERROR, "RASTER_summaryStats_finalfn: Cannot be called in a non-aggregate context");
		PG_RETURN_NULL();
	}

	/* NULL, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	state = (rtpg_summarystats_arg) PG_GETARG_POINTER(0);

	if (NULL == state) {
		elog(ERROR, "RASTER_summaryStats_finalfn: Cannot compute coverage summary stats");
		PG_RETURN_NULL();
	}

	/* coverage mean and deviation */
	if (state->stats->count > 0) {
		state->stats->mean = state->stats->sum / state->stats->count;
		/* sample deviation */
		if (state->stats->sample > 0 && state->stats->sample < 1)
			state->stats->stddev = sqrt(state->cQ / (state->stats->count - 1));
		/* standard deviation */
		else
			state->stats->stddev = sqrt(state->cQ / state->stats->count);
	}

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
		rtpg_summarystats_arg_destroy(state);
		ereport(ERROR, (
			errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			errmsg(
				"function returning record called in context "
				"that cannot accept type record"
			)
		));
	}

	BlessTupleDesc(tupdesc);

	memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

	values[0] = Int64GetDatum(state->stats->count);
	if (state->stats->count > 0) {
		values[1] = Float8GetDatum(state->stats->sum);
		values[2] = Float8GetDatum(state->stats->mean);
		values[3] = Float8GetDatum(state->stats->stddev);
		values[4] = Float8GetDatum(state->stats->min);
		values[5] = Float8GetDatum(state->stats->max);
	}
	else {
		nulls[1] = TRUE;
		nulls[2] = TRUE;
		nulls[3] = TRUE;
		nulls[4] = TRUE;
		nulls[5] = TRUE;
	}

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	rtpg_summarystats_arg_destroy(state);

	PG_RETURN_DATUM(result);
}

#undef VALUES_LENGTH
#define VALUES_LENGTH 4

/**
 * Returns histogram for a band
 */
PG_FUNCTION_INFO_V1(RASTER_histogram);
Datum RASTER_histogram(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	int i;
	rt_histogram hist;
	rt_histogram hist2;
	int call_cntr;
	int max_calls;

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int32_t bandindex = 1;
		int num_bands = 0;
		bool exclude_nodata_value = TRUE;
		double sample = 0;
		uint32_t bin_count = 0;
		double *bin_width = NULL;
		uint32_t bin_width_count = 0;
		double width = 0;
		bool right = FALSE;
		double min = 0;
		double max = 0;
		rt_bandstats stats = NULL;
		uint32_t count;

		int j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		POSTGIS_RT_DEBUG(3, "RASTER_histogram: Starting");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogram: Cannot deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* band index is 1-based */
		if (!PG_ARGISNULL(1))
			bandindex = PG_GETARG_INT32(1);
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);

		/* sample % */
		if (!PG_ARGISNULL(3)) {
			sample = PG_GETARG_FLOAT8(3);
			if (sample < 0 || sample > 1) {
				elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}
			else if (FLT_EQ(sample, 0.0))
				sample = 1;
		}
		else
			sample = 1;

		/* bin_count */
		if (!PG_ARGISNULL(4)) {
			bin_count = PG_GETARG_INT32(4);
			if (bin_count < 1) bin_count = 0;
		}

		/* bin_width */
		if (!PG_ARGISNULL(5)) {
			array = PG_GETARG_ARRAYTYPE_P(5);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_histogram: Invalid data type for width");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
				&nulls, &n);

			bin_width = palloc(sizeof(double) * n);
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case FLOAT4OID:
						width = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						width = (double) DatumGetFloat8(e[i]);
						break;
				}

				if (width < 0 || FLT_EQ(width, 0.0)) {
					elog(NOTICE, "Invalid value for width (must be greater than 0). Returning NULL");
					pfree(bin_width);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					SRF_RETURN_DONE(funcctx);
				}

				bin_width[j] = width;
				POSTGIS_RT_DEBUGF(5, "bin_width[%d] = %f", j, bin_width[j]);
				j++;
			}
			bin_width_count = j;

			if (j < 1) {
				pfree(bin_width);
				bin_width = NULL;
			}
		}

		/* right */
		if (!PG_ARGISNULL(6))
			right = PG_GETARG_BOOL(6);

		/* min */
		if (!PG_ARGISNULL(7)) min = PG_GETARG_FLOAT8(7);

		/* max */
		if (!PG_ARGISNULL(8)) max = PG_GETARG_FLOAT8(8);

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Cannot compute summary statistics for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Cannot compute histogram for band at index %d as the band has no values", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get histogram */
		hist = rt_band_get_histogram(stats, (uint32_t)bin_count, bin_width, bin_width_count, right, min, max, &count);
		if (bin_width_count) pfree(bin_width);
		pfree(stats);
		if (NULL == hist || !count) {
			elog(NOTICE, "Cannot compute histogram for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		POSTGIS_RT_DEBUGF(3, "%d bins returned", count);

		/* Store needed information */
		funcctx->user_fctx = hist;

		/* total number of tuples to be returned */
		funcctx->max_calls = count;

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
	hist2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		Datum values[VALUES_LENGTH];
		bool nulls[VALUES_LENGTH];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

		values[0] = Float8GetDatum(hist2[call_cntr].min);
		values[1] = Float8GetDatum(hist2[call_cntr].max);
		values[2] = Int64GetDatum(hist2[call_cntr].count);
		values[3] = Float8GetDatum(hist2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(hist2);
		SRF_RETURN_DONE(funcctx);
	}
}

#undef VALUES_LENGTH
#define VALUES_LENGTH 2

/**
 * Returns quantiles for a band
 */
PG_FUNCTION_INFO_V1(RASTER_quantile);
Datum RASTER_quantile(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	int i;
	rt_quantile quant;
	rt_quantile quant2;
	int call_cntr;
	int max_calls;

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int32_t bandindex = 0;
		int num_bands = 0;
		bool exclude_nodata_value = TRUE;
		double sample = 0;
		double *quantiles = NULL;
		uint32_t quantiles_count = 0;
		double quantile = 0;
		rt_bandstats stats = NULL;
		uint32_t count;

		int j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantile: Cannot deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* band index is 1-based */
		bandindex = PG_GETARG_INT32(1);
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);

		/* sample % */
		if (!PG_ARGISNULL(3)) {
			sample = PG_GETARG_FLOAT8(3);
			if (sample < 0 || sample > 1) {
				elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}
			else if (FLT_EQ(sample, 0.0))
				sample = 1;
		}
		else
			sample = 1;

		/* quantiles */
		if (!PG_ARGISNULL(4)) {
			array = PG_GETARG_ARRAYTYPE_P(4);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_quantile: Invalid data type for quantiles");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
				&nulls, &n);

			quantiles = palloc(sizeof(double) * n);
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case FLOAT4OID:
						quantile = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						quantile = (double) DatumGetFloat8(e[i]);
						break;
				}

				if (quantile < 0 || quantile > 1) {
					elog(NOTICE, "Invalid value for quantile (must be between 0 and 1). Returning NULL");
					pfree(quantiles);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					SRF_RETURN_DONE(funcctx);
				}

				quantiles[j] = quantile;
				POSTGIS_RT_DEBUGF(5, "quantiles[%d] = %f", j, quantiles[j]);
				j++;
			}
			quantiles_count = j;

			if (j < 1) {
				pfree(quantiles);
				quantiles = NULL;
			}
		}

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Cannot retrieve summary statistics for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Cannot compute quantiles for band at index %d as the band has no values", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get quantiles */
		quant = rt_band_get_quantiles(stats, quantiles, quantiles_count, &count);
		if (quantiles_count) pfree(quantiles);
		pfree(stats);
		if (NULL == quant || !count) {
			elog(NOTICE, "Cannot compute quantiles for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		POSTGIS_RT_DEBUGF(3, "%d quantiles returned", count);

		/* Store needed information */
		funcctx->user_fctx = quant;

		/* total number of tuples to be returned */
		funcctx->max_calls = count;

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
	quant2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		Datum values[VALUES_LENGTH];
		bool nulls[VALUES_LENGTH];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

		values[0] = Float8GetDatum(quant2[call_cntr].quantile);
		values[1] = Float8GetDatum(quant2[call_cntr].value);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(quant2);
		SRF_RETURN_DONE(funcctx);
	}
}

#undef VALUES_LENGTH
#define VALUES_LENGTH 3

/* get counts of values */
PG_FUNCTION_INFO_V1(RASTER_valueCount);
Datum RASTER_valueCount(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	int i;
	rt_valuecount vcnts;
	rt_valuecount vcnts2;
	int call_cntr;
	int max_calls;

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int32_t bandindex = 0;
		int num_bands = 0;
		bool exclude_nodata_value = TRUE;
		double *search_values = NULL;
		uint32_t search_values_count = 0;
		double roundto = 0;
		uint32_t count;

		int j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_valueCount: Cannot deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* band index is 1-based */
		bandindex = PG_GETARG_INT32(1);
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);

		/* search values */
		if (!PG_ARGISNULL(3)) {
			array = PG_GETARG_ARRAYTYPE_P(3);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_valueCount: Invalid data type for values");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
				&nulls, &n);

			search_values = palloc(sizeof(double) * n);
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case FLOAT4OID:
						search_values[j] = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						search_values[j] = (double) DatumGetFloat8(e[i]);
						break;
				}

				POSTGIS_RT_DEBUGF(5, "search_values[%d] = %f", j, search_values[j]);
				j++;
			}
			search_values_count = j;

			if (j < 1) {
				pfree(search_values);
				search_values = NULL;
			}
		}

		/* roundto */
		if (!PG_ARGISNULL(4)) {
			roundto = PG_GETARG_FLOAT8(4);
			if (roundto < 0.) roundto = 0;
		}

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get counts of values */
		vcnts = rt_band_get_value_count(band, (int) exclude_nodata_value, search_values, search_values_count, roundto, NULL, &count);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (NULL == vcnts || !count) {
			elog(NOTICE, "Cannot count the values for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		POSTGIS_RT_DEBUGF(3, "%d value counts returned", count);

		/* Store needed information */
		funcctx->user_fctx = vcnts;

		/* total number of tuples to be returned */
		funcctx->max_calls = count;

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
	vcnts2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		Datum values[VALUES_LENGTH];
		bool nulls[VALUES_LENGTH];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

		values[0] = Float8GetDatum(vcnts2[call_cntr].value);
		values[1] = UInt32GetDatum(vcnts2[call_cntr].count);
		values[2] = Float8GetDatum(vcnts2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(vcnts2);
		SRF_RETURN_DONE(funcctx);
	}
}

/* get counts of values for a coverage */
PG_FUNCTION_INFO_V1(RASTER_valueCountCoverage);
Datum RASTER_valueCountCoverage(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	uint32_t i;
	uint64_t covcount = 0;
	uint64_t covtotal = 0;
	rt_valuecount covvcnts = NULL;
	rt_valuecount covvcnts2;
	int call_cntr;
	int max_calls;

	POSTGIS_RT_DEBUG(3, "RASTER_valueCountCoverage: Starting");

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		text *tablenametext = NULL;
		char *tablename = NULL;
		text *colnametext = NULL;
		char *colname = NULL;
		int32_t bandindex = 1;
		bool exclude_nodata_value = TRUE;
		double *search_values = NULL;
		uint32_t search_values_count = 0;
		double roundto = 0;

		int len = 0;
		char *sql = NULL;
		int spi_result;
		Portal portal;
		HeapTuple tuple;
		Datum datum;
		bool isNull = FALSE;
		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int num_bands = 0;
		uint32_t count;
		uint32_t total;
		rt_valuecount vcnts = NULL;
		int exists = 0;

		uint32_t j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* tablename is null, return null */
		if (PG_ARGISNULL(0)) {
			elog(NOTICE, "Table name must be provided");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		tablenametext = PG_GETARG_TEXT_P(0);
		tablename = text_to_cstring(tablenametext);
		if (!strlen(tablename)) {
			elog(NOTICE, "Table name must be provided");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		POSTGIS_RT_DEBUGF(3, "tablename = %s", tablename);

		/* column name is null, return null */
		if (PG_ARGISNULL(1)) {
			elog(NOTICE, "Column name must be provided");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		colnametext = PG_GETARG_TEXT_P(1);
		colname = text_to_cstring(colnametext);
		if (!strlen(colname)) {
			elog(NOTICE, "Column name must be provided");
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		POSTGIS_RT_DEBUGF(3, "colname = %s", colname);

		/* band index is 1-based */
		if (!PG_ARGISNULL(2))
			bandindex = PG_GETARG_INT32(2);

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(3))
			exclude_nodata_value = PG_GETARG_BOOL(3);

		/* search values */
		if (!PG_ARGISNULL(4)) {
			array = PG_GETARG_ARRAYTYPE_P(4);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_valueCountCoverage: Invalid data type for values");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
				&nulls, &n);

			search_values = palloc(sizeof(double) * n);
			for (i = 0, j = 0; i < (uint32_t) n; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case FLOAT4OID:
						search_values[j] = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						search_values[j] = (double) DatumGetFloat8(e[i]);
						break;
				}

				POSTGIS_RT_DEBUGF(5, "search_values[%d] = %f", j, search_values[j]);
				j++;
			}
			search_values_count = j;

			if (j < 1) {
				pfree(search_values);
				search_values = NULL;
			}
		}

		/* roundto */
		if (!PG_ARGISNULL(5)) {
			roundto = PG_GETARG_FLOAT8(5);
			if (roundto < 0.) roundto = 0;
		}

		/* iterate through rasters of coverage */
		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {

			if (search_values_count) pfree(search_values);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_valueCountCoverage: Cannot connect to database using SPI");
			SRF_RETURN_DONE(funcctx);
		}

		/* create sql */
		len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(SPI_tuptable);
			SPI_finish();

			if (search_values_count) pfree(search_values);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_valueCountCoverage: Cannot allocate memory for sql");
			SRF_RETURN_DONE(funcctx);
		}

		/* get cursor */
		snprintf(sql, len, "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL", colname, tablename, colname);
		POSTGIS_RT_DEBUGF(3, "RASTER_valueCountCoverage: %s", sql);
		portal = SPI_cursor_open_with_args(
			"coverage",
			sql,
			0, NULL,
			NULL, NULL,
			TRUE, 0
		);
		pfree(sql);

		/* process resultset */
		SPI_cursor_fetch(portal, TRUE, 1);
		while (SPI_processed == 1 && SPI_tuptable != NULL) {
			tupdesc = SPI_tuptable->tupdesc;
			tuple = SPI_tuptable->vals[0];

			datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
			if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

				SPI_freetuptable(SPI_tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_valueCountCoverage: Cannot get raster of coverage");
				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {

				SPI_freetuptable(SPI_tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_valueCountCoverage: Cannot deserialize raster");
				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				rt_raster_destroy(raster);

				SPI_freetuptable(SPI_tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Cannot find band at index %d. Returning NULL", bandindex);

				rt_raster_destroy(raster);

				SPI_freetuptable(SPI_tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* get counts of values */
			vcnts = rt_band_get_value_count(band, (int) exclude_nodata_value, search_values, search_values_count, roundto, &total, &count);
			rt_band_destroy(band);
			rt_raster_destroy(raster);
			if (NULL == vcnts || !count) {
				elog(NOTICE, "Cannot count the values for band at index %d", bandindex);

				SPI_freetuptable(SPI_tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) free(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			POSTGIS_RT_DEBUGF(3, "%d value counts returned", count);

			if (NULL == covvcnts) {
				covvcnts = (rt_valuecount) SPI_palloc(sizeof(struct rt_valuecount_t) * count);
				if (NULL == covvcnts) {

					SPI_freetuptable(SPI_tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					if (search_values_count) pfree(search_values);

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_valueCountCoverage: Cannot allocate memory for value counts of coverage");
					SRF_RETURN_DONE(funcctx);
				}

				for (i = 0; i < count; i++) {
					covvcnts[i].value = vcnts[i].value;
					covvcnts[i].count = vcnts[i].count;
					covvcnts[i].percent = -1;
				}

				covcount = count;
			}
			else {
				for (i = 0; i < count; i++) {
					exists = 0;

					for (j = 0; j < covcount; j++) {
						if (FLT_EQ(vcnts[i].value, covvcnts[j].value)) {
							exists = 1;
							break;
						}
					}

					if (exists) {
						covvcnts[j].count += vcnts[i].count;
					}
					else {
						covcount++;
						covvcnts = SPI_repalloc(covvcnts, sizeof(struct rt_valuecount_t) * covcount);
						if (!covvcnts) {
							SPI_freetuptable(SPI_tuptable);
							SPI_cursor_close(portal);
							SPI_finish();

							if (search_values_count) pfree(search_values);

							MemoryContextSwitchTo(oldcontext);
							elog(ERROR, "RASTER_valueCountCoverage: Cannot change allocated memory for value counts of coverage");
							SRF_RETURN_DONE(funcctx);
						}

						covvcnts[covcount - 1].value = vcnts[i].value;
						covvcnts[covcount - 1].count = vcnts[i].count;
						covvcnts[covcount - 1].percent = -1;
					}
				}
			}

			covtotal += total;

			pfree(vcnts);

			/* next record */
			SPI_cursor_fetch(portal, TRUE, 1);
		}

		if (SPI_tuptable) SPI_freetuptable(SPI_tuptable);
		SPI_cursor_close(portal);
		SPI_finish();

		if (search_values_count) pfree(search_values);

		/* compute percentages */
		for (i = 0; i < covcount; i++) {
			covvcnts[i].percent = (double) covvcnts[i].count / covtotal;
		}

		/* Store needed information */
		funcctx->user_fctx = covvcnts;

		/* total number of tuples to be returned */
		funcctx->max_calls = covcount;

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
	covvcnts2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		Datum values[VALUES_LENGTH];
		bool nulls[VALUES_LENGTH];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * VALUES_LENGTH);

		values[0] = Float8GetDatum(covvcnts2[call_cntr].value);
		values[1] = UInt32GetDatum(covvcnts2[call_cntr].count);
		values[2] = Float8GetDatum(covvcnts2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(covvcnts2);
		SRF_RETURN_DONE(funcctx);
	}
}

