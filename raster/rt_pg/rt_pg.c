/*
 * $Id: rt_pg.c 11542 2013-06-11 22:52:06Z dustymugs $
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
 * Copyright (C) 2011-2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@azavea.com>
 * Copyright (C) 2009-2011 Pierre Racine <pierre.racine@sbf.ulaval.ca>
 * Copyright (C) 2009-2011 Mateusz Loskot <mateusz@loskot.net>
 * Copyright (C) 2008-2009 Sandro Santilli <strk@keybit.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* for strtod in RASTER_reclass */
#include <errno.h>
#include <assert.h>
#include <ctype.h> /* for isspace */

#include <postgres.h> /* for palloc */
#include <access/gist.h>
#include <access/itup.h>
#include <fmgr.h>
#include <utils/elog.h>
#include <utils/builtins.h>
#include <executor/spi.h>
#include <executor/executor.h> /* for GetAttributeByName in RASTER_reclass */
#include <funcapi.h>

#include "../../postgis_config.h"

#include "lwgeom_pg.h"
#include "rt_pg.h"
#include "pgsql_compat.h"

#include "utils/lsyscache.h" /* for get_typlenbyvalalign */
#include "utils/array.h" /* for ArrayType */
#include "catalog/pg_type.h" /* for INT2OID, INT4OID, FLOAT4OID, FLOAT8OID and TEXTOID */

#if POSTGIS_PGSQL_VERSION > 92
#include "access/htup_details.h"
#endif

/* maximum char length required to hold any double or long long value */
#define MAX_DBL_CHARLEN (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#define MAX_INT_CHARLEN 32

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
    /* Install liblwgeom handlers */
    pg_install_lwgeom_handlers();

    /* TODO: Install raster callbacks (see rt_init_allocators) */
}

/***************************************************************
 * Internal functions must be prefixed with rtpg_.  This is
 * keeping inline with the use of pgis_ for ./postgis C utility
 * functions.
 ****************************************************************/
/* Internal funcs */
static char *rtpg_strreplace(
	const char *str,
	const char *oldstr, const char *newstr,
	int *count
);
static char *rtpg_strtoupper(char *str);
static char	*rtpg_chartrim(const char* input, char *remove);
static char **rtpg_strsplit(const char *str, const char *delimiter, int *n);
static char *rtpg_removespaces(char *str);
static char *rtpg_trim(const char* input);
static char *rtpg_getSR(int srid);

/***************************************************************
 * Some rules for returning NOTICE or ERROR...
 *
 * Send an ERROR like:
 *
 * elog(ERROR, "RASTER_out: Could not deserialize raster");
 *
 * only when:
 *
 * -something wrong happen with memory,
 * -a function got an invalid argument ('3BUI' as pixel type) so that no row can
 * be processed
 *
 * *** IMPORTANT: elog(ERROR, ...) does NOT return to calling function ***
 *
 * Send a NOTICE like:
 *
 * elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
 *
 * when arguments (e.g. x, y, band) are NULL or out of range so that some or
 * most rows can be processed anyway
 *
 * in this case,
 * for SET functions or function normally returning a modified raster, return
 * the original raster
 * for GET functions, return NULL
 * try to deduce a valid parameter value if it makes sence (e.g. out of range
 * index for addBand)
 *
 * Do not put the name of the faulty function for NOTICEs, only with ERRORs.
 *
 ****************************************************************/

/******************************************************************************
 * Some notes on memory management...
 *
 * Every time a SQL function is called, PostgreSQL creates a  new memory context.
 * So, all the memory allocated with palloc/repalloc in that context is
 * automatically free'd at the end of the function. If you want some data to
 * live between function calls, you have 2 options:
 *
 * - Use fcinfo->flinfo->fn_mcxt contex to store the data (by pointing the
 *   data you want to keep with fcinfo->flinfo->fn_extra)
 * - Use SRF funcapi, and storing the data at multi_call_memory_ctx (by pointing
 *   the data you want to keep with funcctx->user_fctx. funcctx is created by
 *   funcctx = SPI_FIRSTCALL_INIT()). Recommended way in functions returning rows,
 *   like RASTER_dumpAsPolygons (see section 34.9.9 at
 *   http://www.postgresql.org/docs/8.4/static/xfunc-c.html).
 *
 * But raster code follows the same philosophy than the rest of PostGIS: keep
 * memory as clean as possible. So, we free all allocated memory.
 *
 * TODO: In case of functions returning NULL, we should free the memory too.
 *****************************************************************************/

/******************************************************************************
 * Notes for use of PG_DETOAST_DATUM(), PG_DETOAST_DATUM_SLICE()
 *   and PG_DETOAST_DATUM_COPY()
 *
 * When ONLY getting raster (not band) metadata, use PG_DETOAST_DATUM_SLICE()
 *   as it is generally quicker to get only the chunk of memory that contains
 *   the raster metadata.
 *
 *   Example: PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0,
 *     sizeof(struct rt_raster_serialized_t))
 *
 * When ONLY setting raster or band(s) metadata OR reading band data, use
 *   PG_DETOAST_DATUM() as rt_raster_deserialize() allocates local memory
 *   for the raster and band(s) metadata.
 *
 *   Example: PG_DETOAST_DATUM(PG_GETARG_DATUM(0))
 *
 * When SETTING band pixel values, use PG_DETOAST_DATUM_COPY().  This is
 *   because band data (not metadata) is just a pointer to the correct
 *   memory location in the detoasted datum.  What is returned from
 *   PG_DETOAST_DATUM() may or may not be a copy of the input datum.
 *
 *   From the comments in postgresql/src/include/fmgr.h...
 *
 *     pg_detoast_datum() gives you either the input datum (if not toasted)
 *     or a detoasted copy allocated with palloc().
 *
 *   From the mouth of Tom Lane...
 *   http://archives.postgresql.org/pgsql-hackers/2002-01/msg01289.php
 *
 *     PG_DETOAST_DATUM_COPY guarantees to give you a copy, even if the
 *     original wasn't toasted.  This allows you to scribble on the input,
 *     in case that happens to be a useful way of forming your result.
 *     Without a forced copy, a routine for a pass-by-ref datatype must
 *     NEVER, EVER scribble on its input ... because very possibly it'd
 *     be scribbling on a valid tuple in a disk buffer, or a valid entry
 *     in the syscache.
 *
 *   The key detail above is that the raster datatype is a varlena, a
 *     passed by reference datatype.
 *
 *   Example: PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0))
 *
 * If in doubt, use PG_DETOAST_DATUM_COPY() as that guarantees that the input
 *   datum is copied for use.
 *****************************************************************************/

/* Prototypes */

/* Utility functions */
Datum RASTER_lib_version(PG_FUNCTION_ARGS);
Datum RASTER_lib_build_date(PG_FUNCTION_ARGS);
Datum RASTER_gdal_version(PG_FUNCTION_ARGS);
Datum RASTER_minPossibleValue(PG_FUNCTION_ARGS);

/* Input/output and format conversions */
Datum RASTER_in(PG_FUNCTION_ARGS);
Datum RASTER_out(PG_FUNCTION_ARGS);

Datum RASTER_to_bytea(PG_FUNCTION_ARGS);
Datum RASTER_to_binary(PG_FUNCTION_ARGS);

/* Raster as geometry operations */
Datum RASTER_convex_hull(PG_FUNCTION_ARGS);
Datum RASTER_dumpAsPolygons(PG_FUNCTION_ARGS);

/* Get all the properties of a raster */
Datum RASTER_getSRID(PG_FUNCTION_ARGS);
Datum RASTER_getWidth(PG_FUNCTION_ARGS);
Datum RASTER_getHeight(PG_FUNCTION_ARGS);
Datum RASTER_getNumBands(PG_FUNCTION_ARGS);
Datum RASTER_getXScale(PG_FUNCTION_ARGS);
Datum RASTER_getYScale(PG_FUNCTION_ARGS);
Datum RASTER_getXSkew(PG_FUNCTION_ARGS);
Datum RASTER_getYSkew(PG_FUNCTION_ARGS);
Datum RASTER_getXUpperLeft(PG_FUNCTION_ARGS);
Datum RASTER_getYUpperLeft(PG_FUNCTION_ARGS);
Datum RASTER_getPixelWidth(PG_FUNCTION_ARGS);
Datum RASTER_getPixelHeight(PG_FUNCTION_ARGS);
Datum RASTER_getGeotransform(PG_FUNCTION_ARGS);

/* Set all the properties of a raster */
Datum RASTER_setSRID(PG_FUNCTION_ARGS);
Datum RASTER_setScale(PG_FUNCTION_ARGS);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS);
Datum RASTER_setSkew(PG_FUNCTION_ARGS);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS);
Datum RASTER_setRotation(PG_FUNCTION_ARGS);
Datum RASTER_setGeotransform(PG_FUNCTION_ARGS);

/* Get all the properties of a raster band */
Datum RASTER_getBandPixelType(PG_FUNCTION_ARGS);
Datum RASTER_getBandPixelTypeName(PG_FUNCTION_ARGS);
Datum RASTER_getBandNoDataValue(PG_FUNCTION_ARGS);
Datum RASTER_getBandPath(PG_FUNCTION_ARGS);
Datum RASTER_bandIsNoData(PG_FUNCTION_ARGS);
Datum RASTER_isEmpty(PG_FUNCTION_ARGS);
Datum RASTER_hasNoBand(PG_FUNCTION_ARGS);

/* Set all the properties of a raster band */
Datum RASTER_setBandIsNoData(PG_FUNCTION_ARGS);
Datum RASTER_setBandNoDataValue(PG_FUNCTION_ARGS);

/* Get pixel value */
Datum RASTER_getPixelValue(PG_FUNCTION_ARGS);
Datum RASTER_dumpValues(PG_FUNCTION_ARGS);

/* Set pixel value(s) */
Datum RASTER_setPixelValue(PG_FUNCTION_ARGS);
Datum RASTER_setPixelValuesArray(PG_FUNCTION_ARGS);
Datum RASTER_setPixelValuesGeomval(PG_FUNCTION_ARGS);

/* Get pixel geographical shape */
Datum RASTER_getPixelPolygons(PG_FUNCTION_ARGS);

/* Get raster band's polygon */
Datum RASTER_getPolygon(PG_FUNCTION_ARGS);

/* Get pixels of value */
Datum RASTER_pixelOfValue(PG_FUNCTION_ARGS);

/* Get nearest value to a point */
Datum RASTER_nearestValue(PG_FUNCTION_ARGS);

/* Get the neighborhood around a pixel */
Datum RASTER_neighborhood(PG_FUNCTION_ARGS);

/* Raster and band creation */
Datum RASTER_makeEmpty(PG_FUNCTION_ARGS);
Datum RASTER_addBand(PG_FUNCTION_ARGS);
Datum RASTER_copyBand(PG_FUNCTION_ARGS);
Datum RASTER_addBandRasterArray(PG_FUNCTION_ARGS);
Datum RASTER_addBandOutDB(PG_FUNCTION_ARGS);
Datum RASTER_tile(PG_FUNCTION_ARGS);

/* create new raster from existing raster's bands */
Datum RASTER_band(PG_FUNCTION_ARGS);

/* Get summary stats */
Datum RASTER_summaryStats(PG_FUNCTION_ARGS);
Datum RASTER_summaryStatsCoverage(PG_FUNCTION_ARGS);

/* get histogram */
Datum RASTER_histogram(PG_FUNCTION_ARGS);
Datum RASTER_histogramCoverage(PG_FUNCTION_ARGS);

/* get quantiles */
Datum RASTER_quantile(PG_FUNCTION_ARGS);
Datum RASTER_quantileCoverage(PG_FUNCTION_ARGS);

/* get counts of values */
Datum RASTER_valueCount(PG_FUNCTION_ARGS);
Datum RASTER_valueCountCoverage(PG_FUNCTION_ARGS);

/* reclassify specified bands of a raster */
Datum RASTER_reclass(PG_FUNCTION_ARGS);

/* apply colormap to specified band of a raster */
Datum RASTER_colorMap(PG_FUNCTION_ARGS);

/* convert GDAL raster to raster */
Datum RASTER_fromGDALRaster(PG_FUNCTION_ARGS);

/* convert raster to GDAL raster */
Datum RASTER_asGDALRaster(PG_FUNCTION_ARGS);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS);

/* rasterize a geometry */
Datum RASTER_asRaster(PG_FUNCTION_ARGS);

/* warp a raster using GDAL Warp API */
Datum RASTER_GDALWarp(PG_FUNCTION_ARGS);

/* get raster's meta data */
Datum RASTER_metadata(PG_FUNCTION_ARGS);

/* get raster band's meta data */
Datum RASTER_bandmetadata(PG_FUNCTION_ARGS);

/* convert pixel/line to spatial coordinates */
Datum RASTER_rasterToWorldCoord(PG_FUNCTION_ARGS);

/* convert spatial coordinates to pixel/line*/
Datum RASTER_worldToRasterCoord(PG_FUNCTION_ARGS);

/* determine if two rasters intersect */
Datum RASTER_intersects(PG_FUNCTION_ARGS);

/* determine if two rasters overlap */
Datum RASTER_overlaps(PG_FUNCTION_ARGS);

/* determine if two rasters touch */
Datum RASTER_touches(PG_FUNCTION_ARGS);

/* determine if the first raster contains the second raster */
Datum RASTER_contains(PG_FUNCTION_ARGS);

/* determine if the first raster contains properly the second raster */
Datum RASTER_containsProperly(PG_FUNCTION_ARGS);

/* determine if the first raster covers the second raster */
Datum RASTER_covers(PG_FUNCTION_ARGS);

/* determine if the first raster is covered by the second raster */
Datum RASTER_coveredby(PG_FUNCTION_ARGS);

/* determine if the two rasters are within the specified distance of each other */
Datum RASTER_dwithin(PG_FUNCTION_ARGS);

/* determine if the two rasters are fully within the specified distance of each other */
Datum RASTER_dfullywithin(PG_FUNCTION_ARGS);

/* determine if two rasters are aligned */
Datum RASTER_sameAlignment(PG_FUNCTION_ARGS);
Datum RASTER_notSameAlignmentReason(PG_FUNCTION_ARGS);

/* one-raster MapAlgebra */
Datum RASTER_mapAlgebraExpr(PG_FUNCTION_ARGS);
Datum RASTER_mapAlgebraFct(PG_FUNCTION_ARGS);

/* one-raster neighborhood MapAlgebra */
Datum RASTER_mapAlgebraFctNgb(PG_FUNCTION_ARGS);

/* two-raster MapAlgebra */
Datum RASTER_mapAlgebra2(PG_FUNCTION_ARGS);

/* n-raster MapAlgebra */
Datum RASTER_nMapAlgebra(PG_FUNCTION_ARGS);
Datum RASTER_nMapAlgebraExpr(PG_FUNCTION_ARGS);

/* raster union aggregate */
Datum RASTER_union_transfn(PG_FUNCTION_ARGS);
Datum RASTER_union_finalfn(PG_FUNCTION_ARGS);

/* raster clip */
Datum RASTER_clip(PG_FUNCTION_ARGS);

/* string replacement function taken from
 * http://ubuntuforums.org/showthread.php?s=aa6f015109fd7e4c7e30d2fd8b717497&t=141670&page=3
 */
/* ---------------------------------------------------------------------------
  Name       : replace - Search & replace a substring by another one.
  Creation   : Thierry Husson, Sept 2010
  Parameters :
      str    : Big string where we search
      oldstr : Substring we are looking for
      newstr : Substring we want to replace with
      count  : Optional pointer to int (input / output value). NULL to ignore.
               Input:  Maximum replacements to be done. NULL or < 1 to do all.
               Output: Number of replacements done or -1 if not enough memory.
  Returns    : Pointer to the new string or NULL if error.
  Notes      :
     - Case sensitive - Otherwise, replace functions "strstr" by "strcasestr"
     - Always allocates memory for the result.
--------------------------------------------------------------------------- */
static char*
rtpg_strreplace(
	const char *str,
	const char *oldstr, const char *newstr,
	int *count
) {
	const char *tmp = str;
	char *result;
	int found = 0;
	int length, reslen;
	int oldlen = strlen(oldstr);
	int newlen = strlen(newstr);
	int limit = (count != NULL && *count > 0) ? *count : -1;

	tmp = str;
	while ((tmp = strstr(tmp, oldstr)) != NULL && found != limit)
		found++, tmp += oldlen;

	length = strlen(str) + found * (newlen - oldlen);
	if ((result = (char *) palloc(length + 1)) == NULL) {
		fprintf(stderr, "Not enough memory\n");
		found = -1;
	}
	else {
		tmp = str;
		limit = found; /* Countdown */
		reslen = 0; /* length of current result */

		/* Replace each old string found with new string  */
		while ((limit-- > 0) && (tmp = strstr(tmp, oldstr)) != NULL) {
			length = (tmp - str); /* Number of chars to keep intouched */
			strncpy(result + reslen, str, length); /* Original part keeped */
			strcpy(result + (reslen += length), newstr); /* Insert new string */

			reslen += newlen;
			tmp += oldlen;
			str = tmp;
		}
		strcpy(result + reslen, str); /* Copies last part and ending null char */
	}

	if (count != NULL) *count = found;
	return result;
}

static char *
rtpg_strtoupper(char * str) {
	int j;

	for (j = strlen(str) - 1; j >= 0; j--)
		str[j] = toupper(str[j]);

	return str;
}

static char*
rtpg_chartrim(const char *input, char *remove) {
	char *rtn = NULL;
	char *ptr = NULL;
	uint32_t offset = 0;

	if (!input)
		return NULL;
	else if (!*input)
		return (char *) input;

	/* trim left */
	while (strchr(remove, *input) != NULL)
		input++;

	/* trim right */
	ptr = ((char *) input) + strlen(input);
	while (strchr(remove, *--ptr) != NULL)
		offset++;

	rtn = palloc(sizeof(char) * (strlen(input) - offset + 1));
	if (rtn == NULL) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	strncpy(rtn, input, strlen(input) - offset);
	rtn[strlen(input) - offset] = '\0';

	return rtn;
}

/* split a string based on a delimiter */
static char**
rtpg_strsplit(const char *str, const char *delimiter, int *n) {
	char *tmp = NULL;
	char **rtn = NULL;
	char *token = NULL;

	*n = 0;
	if (!str)
		return NULL;

	/* copy str to tmp as strtok will mangle the string */
	tmp = palloc(sizeof(char) * (strlen(str) + 1));
	if (NULL == tmp) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	strcpy(tmp, str);

	if (!strlen(tmp) || !delimiter || !strlen(delimiter)) {
		*n = 1;
		rtn = (char **) palloc(*n * sizeof(char *));
		if (NULL == rtn) {
			fprintf(stderr, "Not enough memory\n");
			return NULL;
		}
		rtn[0] = (char *) palloc(sizeof(char) * (strlen(tmp) + 1));
		if (NULL == rtn[0]) {
			fprintf(stderr, "Not enough memory\n");
			return NULL;
		}
		strcpy(rtn[0], tmp);
		pfree(tmp);
		return rtn;
	}

	token = strtok(tmp, delimiter);
	while (token != NULL) {
		if (*n < 1) {
			rtn = (char **) palloc(sizeof(char *));
		}
		else {
			rtn = (char **) repalloc(rtn, (*n + 1) * sizeof(char *));
		}
		if (NULL == rtn) {
			fprintf(stderr, "Not enough memory\n");
			return NULL;
		}

		rtn[*n] = NULL;
		rtn[*n] = (char *) palloc(sizeof(char) * (strlen(token) + 1));
		if (NULL == rtn[*n]) {
			fprintf(stderr, "Not enough memory\n");
			return NULL;
		}

		strcpy(rtn[*n], token);
		*n = *n + 1;

		token = strtok(NULL, delimiter);
	}

	pfree(tmp);
	return rtn;
}

static char *
rtpg_removespaces(char *str) {
	char *rtn;
	char *tmp;

	rtn = rtpg_strreplace(str, " ", "", NULL);

	tmp = rtpg_strreplace(rtn, "\n", "", NULL);
	pfree(rtn);
	rtn = rtpg_strreplace(tmp, "\t", "", NULL);
	pfree(tmp);
	tmp = rtpg_strreplace(rtn, "\f", "", NULL);
	pfree(rtn);
	rtn = rtpg_strreplace(tmp, "\r", "", NULL);
	pfree(tmp);

	return rtn;
}

static char*
rtpg_trim(const char *input) {
	char *rtn;
	char *ptr;
	uint32_t offset = 0;
	int inputlen = 0;

	if (!input)
		return NULL;
	else if (!*input)
		return (char *) input;

	/* trim left */
	while (isspace(*input) && *input != '\0')
		input++;

	/* trim right */
	inputlen = strlen(input);
	if (inputlen) {
		ptr = ((char *) input) + inputlen;
		while (isspace(*--ptr))
			offset++;
	}

	rtn = palloc(sizeof(char) * (inputlen - offset + 1));
	if (rtn == NULL) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	strncpy(rtn, input, inputlen - offset);
	rtn[inputlen - offset] = '\0';

	return rtn;
}

static char*
rtpg_getSR(int srid)
{
	int i = 0;
	int len = 0;
	char *sql = NULL;
	int spi_result;
	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	char *tmp = NULL;
	char *srs = NULL;

/*
SELECT
	CASE
		WHEN (upper(auth_name) = 'EPSG' OR upper(auth_name) = 'EPSGA') AND length(COALESCE(auth_srid::text, '')) > 0
			THEN upper(auth_name) || ':' || auth_srid
		WHEN length(COALESCE(auth_name, '') || COALESCE(auth_srid::text, '')) > 0
			THEN COALESCE(auth_name, '') || COALESCE(auth_srid::text, '')
		ELSE ''
	END,
	proj4text,
	srtext
FROM spatial_ref_sys
WHERE srid = X
LIMIT 1
*/

	len = sizeof(char) * (strlen("SELECT CASE WHEN (upper(auth_name) = 'EPSG' OR upper(auth_name) = 'EPSGA') AND length(COALESCE(auth_srid::text, '')) > 0 THEN upper(auth_name) || ':' || auth_srid WHEN length(COALESCE(auth_name, '') || COALESCE(auth_srid::text, '')) > 0 THEN COALESCE(auth_name, '') || COALESCE(auth_srid::text, '') ELSE '' END, proj4text, srtext FROM spatial_ref_sys WHERE srid =  LIMIT 1") + MAX_INT_CHARLEN + 1);
	sql = (char *) palloc(len);
	if (NULL == sql) {
		elog(ERROR, "rtpg_getSR: Could not allocate memory for sql\n");
		return NULL;
	}

	spi_result = SPI_connect();
	if (spi_result != SPI_OK_CONNECT) {
		pfree(sql);
		elog(ERROR, "rtpg_getSR: Could not connect to database using SPI\n");
		return NULL;
	}

	/* execute query */
	snprintf(sql, len, "SELECT CASE WHEN (upper(auth_name) = 'EPSG' OR upper(auth_name) = 'EPSGA') AND length(COALESCE(auth_srid::text, '')) > 0 THEN upper(auth_name) || ':' || auth_srid WHEN length(COALESCE(auth_name, '') || COALESCE(auth_srid::text, '')) > 0 THEN COALESCE(auth_name, '') || COALESCE(auth_srid::text, '') ELSE '' END, proj4text, srtext FROM spatial_ref_sys WHERE srid = %d LIMIT 1", srid);
	POSTGIS_RT_DEBUGF(4, "SRS query: %s", sql);
	spi_result = SPI_execute(sql, TRUE, 0);
	SPI_pfree(sql);
	if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		elog(ERROR, "rtpg_getSR: Cannot find SRID (%d) in spatial_ref_sys", srid);
		return NULL;
	}

	tupdesc = SPI_tuptable->tupdesc;
	tuptable = SPI_tuptable;
	tuple = tuptable->vals[0];

	/* which column to use? */
	for (i = 1; i < 4; i++) {
		tmp = SPI_getvalue(tuple, tupdesc, i);

		/* value AND GDAL supports this SR */
		if (
			SPI_result != SPI_ERROR_NOATTRIBUTE &&
			SPI_result != SPI_ERROR_NOOUTFUNC &&
			tmp != NULL &&
			strlen(tmp) &&
			rt_util_gdal_supported_sr(tmp)
		) {
			POSTGIS_RT_DEBUGF(4, "Value for column %d is %s", i, tmp);

			len = strlen(tmp) + 1;
			srs = SPI_palloc(sizeof(char) * len);
			if (NULL == srs) {
				pfree(tmp);
				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_finish();
				elog(ERROR, "rtpg_getSR: Could not allocate memory for spatial reference text\n");
				return NULL;
			}
			strncpy(srs, tmp, len);
			pfree(tmp);

			break;
		}

		if (tmp != NULL)
			pfree(tmp);
		continue;
	}

	if (SPI_tuptable) SPI_freetuptable(tuptable);
	SPI_finish();

	/* unable to get SR info */
	if (srs == NULL) {
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		elog(ERROR, "rtpg_getSR: Could not find a viable spatial reference for SRID (%d)", srid);
		return NULL;
	}

	return srs;
}

PG_FUNCTION_INFO_V1(RASTER_lib_version);
Datum RASTER_lib_version(PG_FUNCTION_ARGS)
{
    char ver[64];
    text *result;

    snprintf(ver, 64, "%s r%d", POSTGIS_LIB_VERSION, POSTGIS_SVN_REVISION);
    ver[63] = '\0';

    result = cstring2text(ver);
    PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(RASTER_lib_build_date);
Datum RASTER_lib_build_date(PG_FUNCTION_ARGS)
{
    char *ver = POSTGIS_BUILD_DATE;
    text *result;
    result = palloc(VARHDRSZ  + strlen(ver));
    SET_VARSIZE(result, VARHDRSZ + strlen(ver));
    memcpy(VARDATA(result), ver, strlen(ver));
    PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(RASTER_gdal_version);
Datum RASTER_gdal_version(PG_FUNCTION_ARGS)
{
	const char *ver = rt_util_gdal_version("--version");
	text *result;

	/* add indicator if GDAL isn't configured right */
	if (!rt_util_gdal_configured()) {
		char *rtn = NULL;
		rtn = palloc(strlen(ver) + strlen(" GDAL_DATA not found") + 1);
		if (!rtn)
			result = cstring2text(ver);
		else {
			sprintf(rtn, "%s GDAL_DATA not found", ver);
			result = cstring2text(rtn);
			pfree(rtn);
		}
	}
	else
		result = cstring2text(ver);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(RASTER_minPossibleValue);
Datum RASTER_minPossibleValue(PG_FUNCTION_ARGS)
{
	text *pixeltypetext = NULL;
	char *pixeltypechar = NULL;
	rt_pixtype pixtype = PT_END;
	double pixsize = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	pixeltypetext = PG_GETARG_TEXT_P(0);
	pixeltypechar = text_to_cstring(pixeltypetext);

	pixtype = rt_pixtype_index_from_name(pixeltypechar);
	if (pixtype == PT_END) {
		elog(ERROR, "RASTER_minPossibleValue: Invalid pixel type: %s", pixeltypechar);
		PG_RETURN_NULL();
	}

	pixsize = rt_pixtype_get_min_value(pixtype);

	/*
		correct pixsize of unsigned pixel types
		example: for PT_8BUI, the value is CHAR_MIN but if char is signed, 
			the value returned is -127 instead of 0.
	*/
	switch (pixtype) {
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BUI:
		case PT_16BUI:
		case PT_32BUI:
			pixsize = 0;
			break;
		default:
			break;
	}

	PG_RETURN_FLOAT8(pixsize);
}

/**
 * Input is a string with hex chars in it.
 * Convert to binary and put in the result
 */
PG_FUNCTION_INFO_V1(RASTER_in);
Datum RASTER_in(PG_FUNCTION_ARGS)
{
	rt_raster raster;
	char *hexwkb = PG_GETARG_CSTRING(0);
	void *result = NULL;

	POSTGIS_RT_DEBUG(3, "Starting");

	raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
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
 * Given a RASTER structure, convert it to Hex and put it in a string
 */
PG_FUNCTION_INFO_V1(RASTER_out);
Datum RASTER_out(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	uint32_t hexwkbsize = 0;
	char *hexwkb = NULL;

	POSTGIS_RT_DEBUG(3, "Starting");

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_out: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	hexwkb = rt_raster_to_hexwkb(raster, FALSE, &hexwkbsize);
	if (!hexwkb) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_out: Could not HEX-WKBize raster");
		PG_RETURN_NULL();
	}

	/* Free the raster objects used */
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	PG_RETURN_CSTRING(hexwkb);
}

/**
 * Return bytea object with raster in Well-Known-Binary form.
 */
PG_FUNCTION_INFO_V1(RASTER_to_bytea);
Datum RASTER_to_bytea(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	uint8_t *wkb = NULL;
	uint32_t wkb_size = 0;
	bytea *result = NULL;
	int result_size = 0;

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Get raster object */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_to_bytea: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* Parse raster to wkb object */
	wkb = rt_raster_to_wkb(raster, FALSE, &wkb_size);
	if (!wkb) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_to_bytea: Could not allocate and generate WKB data");
		PG_RETURN_NULL();
	}

	/* Create varlena object */
	result_size = wkb_size + VARHDRSZ;
	result = (bytea *)palloc(result_size);
	SET_VARSIZE(result, result_size);
	memcpy(VARDATA(result), wkb, VARSIZE(result) - VARHDRSZ);

	/* Free raster objects used */
	rt_raster_destroy(raster);
	pfree(wkb);
	PG_FREE_IF_COPY(pgraster, 0);

	PG_RETURN_POINTER(result);
}

/**
 * Return bytea object with raster in Well-Known-Binary form requested using ST_AsBinary function.
 */
PG_FUNCTION_INFO_V1(RASTER_to_binary);
Datum RASTER_to_binary(PG_FUNCTION_ARGS)
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
		elog(ERROR, "RASTER_to_binary: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	if (!PG_ARGISNULL(1))
		outasin = PG_GETARG_BOOL(1);

	/* Parse raster to wkb object */
	wkb = rt_raster_to_wkb(raster, outasin, &wkb_size);
	if (!wkb) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_to_binary: Could not allocate and generate WKB data");
		PG_RETURN_NULL();
	}

	/* Create varlena object */
	result_size = wkb_size + VARHDRSZ;
	result = (char *)palloc(result_size);
	SET_VARSIZE(result, result_size);
	memcpy(VARDATA(result), wkb, VARSIZE(result) - VARHDRSZ);

	/* Free raster objects used */
	rt_raster_destroy(raster);
	pfree(wkb);
	PG_FREE_IF_COPY(pgraster, 0);

	PG_RETURN_POINTER(result);
}

/**
 * Return the convex hull of this raster
 */
PG_FUNCTION_INFO_V1(RASTER_convex_hull);
Datum RASTER_convex_hull(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster;
	rt_raster raster;
	LWGEOM *geom = NULL;
	GSERIALIZED* gser = NULL;
	size_t gser_size;
	int err = ES_NONE;

	bool minhull = FALSE;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* # of args */
	if (PG_NARGS() > 1)
		minhull = TRUE;

	if (!minhull) {
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));
		raster = rt_raster_deserialize(pgraster, TRUE);
	}
	else {
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		raster = rt_raster_deserialize(pgraster, FALSE);
	}

	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_convex_hull: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	if (!minhull)
		err = rt_raster_get_convex_hull(raster, &geom);
	else {
		int nband = -1;

		/* get arg 1 */
		if (!PG_ARGISNULL(1)) {
			nband = PG_GETARG_INT32(1);
			if (!rt_raster_has_band(raster, nband - 1)) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				PG_RETURN_NULL();
			}
			nband = nband - 1;
		}

		err = rt_raster_get_perimeter(raster, nband, &geom);
	}

	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	if (err != ES_NONE) {
		elog(ERROR, "RASTER_convex_hull: Could not get raster's convex hull");
		PG_RETURN_NULL();
	}
	else if (geom == NULL) {
		elog(NOTICE, "Raster's convex hull is NULL");
		PG_RETURN_NULL();
	}

	gser = gserialized_from_lwgeom(geom, 0, &gser_size);
	lwgeom_free(geom);

	SET_VARSIZE(gser, gser_size);
	PG_RETURN_POINTER(gser);
}

PG_FUNCTION_INFO_V1(RASTER_dumpAsPolygons);
Datum RASTER_dumpAsPolygons(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	rt_geomval geomval;
	rt_geomval geomval2;
	int call_cntr;
	int max_calls;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;
		int numbands;
		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		int nband;
		bool exclude_nodata_value = TRUE;
		int nElements;

		POSTGIS_RT_DEBUG(2, "RASTER_dumpAsPolygons first call");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Get input arguments */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			ereport(ERROR, (
				errcode(ERRCODE_OUT_OF_MEMORY),
				errmsg("Could not deserialize raster")
			));
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		if (!PG_ARGISNULL(1))
			nband = PG_GETARG_UINT32(1);
		else
			nband = 1; /* By default, first band */

		POSTGIS_RT_DEBUGF(3, "band %d", nband);
		numbands = rt_raster_get_num_bands(raster);

		if (nband < 1 || nband > numbands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);

		/* see if band is NODATA */
		if (rt_band_get_isnodata_flag(rt_raster_get_band(raster, nband - 1))) {
			POSTGIS_RT_DEBUGF(3, "Band at index %d is NODATA. Returning NULL", nband);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* Polygonize raster */

		/**
		 * Dump raster
		 */
		geomval = rt_raster_gdal_polygonize(raster, nband - 1, exclude_nodata_value, &nElements);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (NULL == geomval) {
			ereport(ERROR, (
				errcode(ERRCODE_NO_DATA_FOUND),
				errmsg("Could not polygonize raster")
			));
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		POSTGIS_RT_DEBUGF(3, "raster dump, %d elements returned", nElements);

		/* Store needed information */
		funcctx->user_fctx = geomval;

		/* total number of tuples to be returned */
		funcctx->max_calls = nElements;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
			ereport(ERROR, (
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context that cannot accept type record")
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
	geomval2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 2;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple    tuple;
		Datum        result;

		GSERIALIZED *gser = NULL;
		size_t gser_size = 0;

		POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		/* convert LWGEOM to GSERIALIZED */
		gser = gserialized_from_lwgeom(lwpoly_as_lwgeom(geomval2[call_cntr].geom), 0, &gser_size);
		lwgeom_free(lwpoly_as_lwgeom(geomval2[call_cntr].geom));

		values[0] = PointerGetDatum(gser);
		values[1] = Float8GetDatum(geomval2[call_cntr].val);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(geomval2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Make a new raster with no bands
 */
PG_FUNCTION_INFO_V1(RASTER_makeEmpty);
Datum RASTER_makeEmpty(PG_FUNCTION_ARGS)
{
	uint16 width = 0, height = 0;
	double ipx = 0, ipy = 0, scalex = 0, scaley = 0, skewx = 0, skewy = 0;
	int32_t srid = SRID_UNKNOWN;
	rt_pgraster *pgraster = NULL;
	rt_raster raster;

	if (PG_NARGS() < 9) {
		elog(ERROR, "RASTER_makeEmpty: ST_MakeEmptyRaster requires 9 args");
		PG_RETURN_NULL();
	} 

	if (!PG_ARGISNULL(0))
		width = PG_GETARG_UINT16(0);

	if (!PG_ARGISNULL(1))
		height = PG_GETARG_UINT16(1);

	if (!PG_ARGISNULL(2))
		ipx = PG_GETARG_FLOAT8(2);

	if (!PG_ARGISNULL(3))
		ipy = PG_GETARG_FLOAT8(3);

	if (!PG_ARGISNULL(4))
		scalex = PG_GETARG_FLOAT8(4);

	if (!PG_ARGISNULL(5))
		scaley = PG_GETARG_FLOAT8(5);

	if (!PG_ARGISNULL(6))
		skewx = PG_GETARG_FLOAT8(6);

	if (!PG_ARGISNULL(7))
		skewy = PG_GETARG_FLOAT8(7);

	if (!PG_ARGISNULL(8))
		srid = PG_GETARG_INT32(8);

	POSTGIS_RT_DEBUGF(4, "%dx%d, ip:%g,%g, scale:%g,%g, skew:%g,%g srid:%d",
		width, height, ipx, ipy, scalex, scaley,
		skewx, skewy, srid);

	raster = rt_raster_new(width, height);
	if (raster == NULL)
		PG_RETURN_NULL(); /* error was supposedly printed already */

	rt_raster_set_scale(raster, scalex, scaley);
	rt_raster_set_offsets(raster, ipx, ipy);
	rt_raster_set_skews(raster, skewx, skewy);
	rt_raster_set_srid(raster, srid);

	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (!pgraster)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	PG_RETURN_POINTER(pgraster);
}

/**
 * Return the SRID associated with the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getSRID);
Datum RASTER_getSRID(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    int32_t srid;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getSRID: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    srid = rt_raster_get_srid(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_INT32(srid);
}

/**
 * Set the SRID associated with the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSRID);
Datum RASTER_setSRID(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	int32_t newSRID = PG_GETARG_INT32(1);

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setSRID: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_srid(raster, newSRID);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn) PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);

	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return the width of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getWidth);
Datum RASTER_getWidth(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    uint16_t width;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getWidth: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    width = rt_raster_get_width(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_INT32(width);
}

/**
 * Return the height of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getHeight);
Datum RASTER_getHeight(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    uint16_t height;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getHeight: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    height = rt_raster_get_height(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_INT32(height);
}

/**
 * Return the number of bands included in the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getNumBands);
Datum RASTER_getNumBands(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    int32_t num_bands;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getNumBands: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    num_bands = rt_raster_get_num_bands(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_INT32(num_bands);
}

/**
 * Return X scale from georeference of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getXScale);
Datum RASTER_getXScale(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double xsize;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getXScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xsize = rt_raster_get_x_scale(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(xsize);
}

/**
 * Return Y scale from georeference of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getYScale);
Datum RASTER_getYScale(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double ysize;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getYScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    ysize = rt_raster_get_y_scale(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(ysize);
}

/**
 * Set the scale of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setScale);
Datum RASTER_setScale(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double size = PG_GETARG_FLOAT8(1);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setScale: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_scale(raster, size, size);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Set the pixel size of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setScaleXY);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double xscale = PG_GETARG_FLOAT8(1);
	double yscale = PG_GETARG_FLOAT8(2);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setScaleXY: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_scale(raster, xscale, yscale);
	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return value of the raster skew about the X axis.
 */
PG_FUNCTION_INFO_V1(RASTER_getXSkew);
Datum RASTER_getXSkew(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double xskew;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getXSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xskew = rt_raster_get_x_skew(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(xskew);
}

/**
 * Return value of the raster skew about the Y axis.
 */
PG_FUNCTION_INFO_V1(RASTER_getYSkew);
Datum RASTER_getYSkew(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double yskew;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getYSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yskew = rt_raster_get_y_skew(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(yskew);
}

/**
 * Set the skew of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSkew);
Datum RASTER_setSkew(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double skew = PG_GETARG_FLOAT8(1);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setSkew: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_skews(raster, skew, skew);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Set the skew of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSkewXY);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double xskew = PG_GETARG_FLOAT8(1);
	double yskew = PG_GETARG_FLOAT8(2);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setSkewXY: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_skews(raster, xskew, yskew);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return value of the raster offset in the X dimension.
 */
PG_FUNCTION_INFO_V1(RASTER_getXUpperLeft);
Datum RASTER_getXUpperLeft(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double xul;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getXUpperLeft: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xul = rt_raster_get_x_offset(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(xul);
}

/**
 * Return value of the raster offset in the Y dimension.
 */
PG_FUNCTION_INFO_V1(RASTER_getYUpperLeft);
Datum RASTER_getYUpperLeft(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double yul;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getYUpperLeft: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yul = rt_raster_get_y_offset(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(yul);
}

/**
 * Set the raster offset in the X and Y dimension.
 */
PG_FUNCTION_INFO_V1(RASTER_setUpperLeftXY);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double xoffset = PG_GETARG_FLOAT8(1);
	double yoffset = PG_GETARG_FLOAT8(2);

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setUpperLeftXY: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_offsets(raster, xoffset, yoffset);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size); 
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return the pixel width of the raster. The pixel width is
 * a read-only, dynamically computed value derived from the 
 * X Scale and the Y Skew.
 *
 * Pixel Width = sqrt( X Scale * X Scale + Y Skew * Y Skew )
 */
PG_FUNCTION_INFO_V1(RASTER_getPixelWidth);
Datum RASTER_getPixelWidth(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double xscale;
    double yskew;
    double pwidth;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if (!raster) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getPixelWidth: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xscale = rt_raster_get_x_scale(raster);
    yskew = rt_raster_get_y_skew(raster);
    pwidth = sqrt(xscale*xscale + yskew*yskew);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(pwidth);
}

/**
 * Return the pixel height of the raster. The pixel height is
 * a read-only, dynamically computed value derived from the 
 * Y Scale and the X Skew.
 *
 * Pixel Height = sqrt( Y Scale * Y Scale + X Skew * X Skew )
 */
PG_FUNCTION_INFO_V1(RASTER_getPixelHeight);
Datum RASTER_getPixelHeight(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double yscale;
    double xskew;
    double pheight;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if (!raster) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getPixelHeight: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yscale = rt_raster_get_y_scale(raster);
    xskew = rt_raster_get_x_skew(raster);
    pheight = sqrt(yscale*yscale + xskew*xskew);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(pheight);
}

/**
 * Set the geotransform of the supplied raster. Returns the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setGeotransform);
Datum RASTER_setGeotransform(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	float8 imag, jmag, theta_i, theta_ij, xoffset, yoffset;

	if (
		PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) ||
		PG_ARGISNULL(3) || PG_ARGISNULL(4) ||
		PG_ARGISNULL(5) || PG_ARGISNULL(6)
	) {
		PG_RETURN_NULL();
	}

	/* get the inputs */
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	imag = PG_GETARG_FLOAT8(1);
	jmag = PG_GETARG_FLOAT8(2);
	theta_i = PG_GETARG_FLOAT8(3);
	theta_ij = PG_GETARG_FLOAT8(4);
	xoffset = PG_GETARG_FLOAT8(5);
	yoffset = PG_GETARG_FLOAT8(6);

	raster = rt_raster_deserialize(pgraster, TRUE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setGeotransform: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* store the new geotransform */
	rt_raster_set_phys_params(raster, imag,jmag,theta_i,theta_ij);
	rt_raster_set_offsets(raster, xoffset, yoffset);

	/* prep the return value */
	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL(); 

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Calculates the physically relevant parameters of the supplied raster's
 * geotransform. Returns them as a set.
 */
PG_FUNCTION_INFO_V1(RASTER_getGeotransform);
Datum RASTER_getGeotransform(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;

    double imag;
    double jmag;
    double theta_i;
    double theta_ij;
		/*
    double xoffset;
    double yoffset;
		*/

    TupleDesc result_tuple; /* for returning a composite */
    int values_length = 6;
    Datum values[values_length];
    bool nulls[values_length];
    HeapTuple heap_tuple ;   /* instance of the tuple to return */
    Datum result;

    POSTGIS_RT_DEBUG(3, "RASTER_getGeotransform: Starting");

    /* get argument */
    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    /* raster */
    raster = rt_raster_deserialize(pgraster, TRUE);
    if (!raster) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getGeotransform: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* do the calculation */
    rt_raster_calc_phys_params(
            rt_raster_get_x_scale(raster),
            rt_raster_get_x_skew(raster),
            rt_raster_get_y_skew(raster),
            rt_raster_get_y_scale(raster),
            &imag, &jmag, &theta_i, &theta_ij) ;

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    /* setup the return value infrastructure */
    if (get_call_result_type(fcinfo, NULL, &result_tuple) != TYPEFUNC_COMPOSITE) {
        ereport(ERROR, (
            errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
            errmsg("RASTER_getGeotransform(): function returning record called in context that cannot accept type record"
            )
        ));
        PG_RETURN_NULL();
    }

    BlessTupleDesc(result_tuple);

    /* get argument */
    /* prep the composite return value */
    /* construct datum array */
    values[0] = Float8GetDatum(imag);
    values[1] = Float8GetDatum(jmag);
    values[2] = Float8GetDatum(theta_i);
    values[3] = Float8GetDatum(theta_ij);
    values[4] = Float8GetDatum(rt_raster_get_x_offset(raster));
    values[5] = Float8GetDatum(rt_raster_get_y_offset(raster));

    memset(nulls, FALSE, sizeof(bool) * values_length);

    /* stick em on the heap */
    heap_tuple = heap_form_tuple(result_tuple, values, nulls);

    /* make the tuple into a datum */
    result = HeapTupleGetDatum(heap_tuple);

    PG_RETURN_DATUM(result);
}


/**
 * Set the rotation of the raster. This method will change the X Scale,
 * Y Scale, X Skew and Y Skew properties all at once to keep the rotations
 * about the X and Y axis uniform.
 *
 * This method will set the rotation about the X axis and Y axis based on
 * the pixel size. This pixel size may not be uniform if rasters have different
 * skew values (the raster cells are diamond-shaped). If a raster has different
 * skew values has a rotation set upon it, this method will remove the 
 * diamond distortions of the cells, as each axis will have the same rotation.
 */
PG_FUNCTION_INFO_V1(RASTER_setRotation);
Datum RASTER_setRotation(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster;
	double rotation = PG_GETARG_FLOAT8(1);
	double imag, jmag, theta_i, theta_ij;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (! raster ) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setRotation: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* preserve all defining characteristics of the grid except for rotation */
	rt_raster_get_phys_params(raster, &imag, &jmag, &theta_i, &theta_ij);
	rt_raster_set_phys_params(raster, imag, jmag, rotation, theta_ij);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return pixel type of the specified band of raster.
 * Band index is 1-based.
 */
PG_FUNCTION_INFO_V1(RASTER_getBandPixelType);
Datum RASTER_getBandPixelType(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    rt_pixtype pixtype;
    int32_t bandindex;

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getBandPixelType: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel type. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(band);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_INT32(pixtype);
}

/**
 * Return name of pixel type of the specified band of raster.
 * Band index is 1-based.
 * NOTE: This is unofficial utility not included in the spec.
 */
PG_FUNCTION_INFO_V1(RASTER_getBandPixelTypeName);
Datum RASTER_getBandPixelTypeName(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    rt_pixtype pixtype;
    int32_t bandindex;
    const size_t name_size = 8; /* size of type name */
    size_t size = 0;
    char *ptr = NULL;
    text *result = NULL;

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getBandPixelTypeName: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel type name. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(band);

    result = palloc(VARHDRSZ + name_size);
    /* We don't need to check for NULL pointer, because if out of memory, palloc
     * exit via elog(ERROR). It never returns NULL.
     */

    memset(VARDATA(result), 0, name_size);
    ptr = (char *)result + VARHDRSZ;
		strcpy(ptr, rt_pixtype_name(pixtype));

    size = VARHDRSZ + strlen(ptr);
    SET_VARSIZE(result, size);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_TEXT_P(result);
}

/**
 * Return nodata value of the specified band of raster.
 * The value is always returned as FLOAT32 even if the pixel type is INTEGER.
 */
PG_FUNCTION_INFO_V1(RASTER_getBandNoDataValue);
Datum RASTER_getBandNoDataValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    int32_t bandindex;
    double nodata;

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getBandNoDataValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting band nodata value. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    if ( ! rt_band_get_hasnodata_flag(band) ) {
        /* Raster does not have a nodata value set so we return NULL */
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    rt_band_get_nodata(band, &nodata);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(nodata);
}


/**
 * Set the nodata value of the specified band of raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setBandNoDataValue);
Datum RASTER_setBandNoDataValue(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	double nodata;
	int32_t bandindex;
	bool forcechecking = FALSE;
	bool skipset = FALSE;

	/* Deserialize raster */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Check index is not NULL or smaller than 1 */
	if (PG_ARGISNULL(1))
		bandindex = -1;
	else
		bandindex = PG_GETARG_INT32(1);
	if (bandindex < 1) {
		elog(NOTICE, "Invalid band index (must use 1-based). Nodata value not set. Returning original raster");
		skipset = TRUE;
	}

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setBandNoDataValue: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	if (!skipset) {
		/* Fetch requested band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Could not find raster band of index %d when setting pixel value. Nodata value not set. Returning original raster", bandindex);
		}
		else {
			if (!PG_ARGISNULL(3))
				forcechecking = PG_GETARG_BOOL(3);

			if (PG_ARGISNULL(2)) {
				/* Set the hasnodata flag to FALSE */
				rt_band_set_hasnodata_flag(band, FALSE);
				POSTGIS_RT_DEBUGF(3, "Raster band %d does not have a nodata value", bandindex);
			}
			else {
				/* Get the nodata value */
				nodata = PG_GETARG_FLOAT8(2);

				/* Set the band's nodata value */
				rt_band_set_nodata(band, nodata, NULL);

				/* Recheck all pixels if requested */
				if (forcechecking)
					rt_band_check_is_nodata(band);
			}
		}
	}

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

PG_FUNCTION_INFO_V1(RASTER_setBandIsNoData);
Datum RASTER_setBandIsNoData(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int32_t bandindex;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setBandIsNoData: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* Check index is not NULL or smaller than 1 */
	if (PG_ARGISNULL(1))
		bandindex = -1;
	else
		bandindex = PG_GETARG_INT32(1);

	if (bandindex < 1)
		elog(NOTICE, "Invalid band index (must use 1-based). Isnodata flag not set. Returning original raster");
	else {
		/* Fetch requested band */
		band = rt_raster_get_band(raster, bandindex - 1);

		if (!band)
			elog(NOTICE, "Could not find raster band of index %d. Isnodata flag not set. Returning original raster", bandindex);
		else {
			if (!rt_band_get_hasnodata_flag(band)) {
				elog(NOTICE, "Band of index %d has no NODATA so cannot be NODATA. Returning original raster", bandindex);
			}
			/* Set the band's nodata value */
			else {
				rt_band_set_isnodata_flag(band, 1);
			}
		}
	}

	/* Serialize raster again */
	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn) PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

PG_FUNCTION_INFO_V1(RASTER_bandIsNoData);
Datum RASTER_bandIsNoData(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    int32_t bandindex;
    bool forcechecking = FALSE;
    bool bandisnodata = FALSE;

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_bandIsNoData: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when determining if band is nodata. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    forcechecking = PG_GETARG_BOOL(2);

    bandisnodata = (forcechecking) ?
        rt_band_check_is_nodata(band) : rt_band_get_isnodata_flag(band);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_BOOL(bandisnodata);
}

/**
 * Return the path of the raster for out-db raster.
 */
PG_FUNCTION_INFO_V1(RASTER_getBandPath);
Datum RASTER_getBandPath(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    int32_t bandindex;
    const char *bandpath;
    text *result;

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getBandPath: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting band path. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    bandpath = rt_band_get_ext_path(band);
		rt_band_destroy(band);
    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);
    if ( ! bandpath )
    {
        PG_RETURN_NULL();
    }

    result = (text *) palloc(VARHDRSZ + strlen(bandpath) + 1);

    SET_VARSIZE(result, VARHDRSZ + strlen(bandpath) + 1);

    strcpy((char *) VARDATA(result), bandpath);

    PG_RETURN_TEXT_P(result);
}


/**
 * Return value of a single pixel.
 * Pixel location is specified by 1-based index of Nth band of raster and
 * X,Y coordinates (X <= RT_Width(raster) and Y <= RT_Height(raster)).
 *
 * TODO: Should we return NUMERIC instead of FLOAT8 ?
 */
PG_FUNCTION_INFO_V1(RASTER_getPixelValue);
Datum RASTER_getPixelValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    double pixvalue = 0;
    int32_t bandindex = 0;
    int32_t x = 0;
    int32_t y = 0;
    int result = 0;
    bool exclude_nodata_value = TRUE;
		int isnodata = 0;

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    x = PG_GETARG_INT32(2);

    y = PG_GETARG_INT32(3);

    exclude_nodata_value = PG_GETARG_BOOL(4);

    POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if (!raster) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_getPixelValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch Nth band using 0-based internal index */
    band = rt_raster_get_band(raster, bandindex - 1);
    if (! band) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel "
                "value. Returning NULL", bandindex);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }
    /* Fetch pixel using 0-based coordinates */
    result = rt_band_get_pixel(band, x - 1, y - 1, &pixvalue, &isnodata);

    /* If the result is -1 or the value is nodata and we take nodata into account
     * then return nodata = NULL */
    if (result != ES_NONE || (exclude_nodata_value && isnodata)) {
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT8(pixvalue);
}

/* ---------------------------------------------------------------- */
/*  ST_DumpValues function                                          */
/* ---------------------------------------------------------------- */

typedef struct rtpg_dumpvalues_arg_t *rtpg_dumpvalues_arg;
struct rtpg_dumpvalues_arg_t {
	int numbands;
	int rows;
	int columns;

	int *nbands; /* 0-based */
	Datum **values;
	bool **nodata;
};

static rtpg_dumpvalues_arg rtpg_dumpvalues_arg_init() {
	rtpg_dumpvalues_arg arg = NULL;

	arg = palloc(sizeof(struct rtpg_dumpvalues_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_dumpvalues_arg_init: Could not allocate memory for arguments");
		return NULL;
	}

	arg->numbands = 0;
	arg->rows = 0;
	arg->columns = 0;

	arg->nbands = NULL;
	arg->values = NULL;
	arg->nodata = NULL;

	return arg;
}

static void rtpg_dumpvalues_arg_destroy(rtpg_dumpvalues_arg arg) {
	int i = 0;

	if (arg->numbands) {
		if (arg->nbands != NULL)
			pfree(arg->nbands);

		for (i = 0; i < arg->numbands; i++) {
			if (arg->values[i] != NULL)
				pfree(arg->values[i]);

			if (arg->nodata[i] != NULL)
				pfree(arg->nodata[i]);
		}

		if (arg->values != NULL)
			pfree(arg->values);
		if (arg->nodata != NULL)
			pfree(arg->nodata);
	}

	pfree(arg);
}

PG_FUNCTION_INFO_V1(RASTER_dumpValues);
Datum RASTER_dumpValues(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	int call_cntr;
	int max_calls;
	int i = 0;
	int x = 0;
	int y = 0;
	int z = 0;

	int16 typlen;
	bool typbyval;
	char typalign;

	rtpg_dumpvalues_arg arg1 = NULL;
	rtpg_dumpvalues_arg arg2 = NULL;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;
		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int numbands = 0;
		int j = 0;
		bool exclude_nodata_value = TRUE;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;

		double val = 0;
		int isnodata = 0;

		POSTGIS_RT_DEBUG(2, "RASTER_dumpValues first call");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Get input arguments */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			ereport(ERROR, (
				errcode(ERRCODE_OUT_OF_MEMORY),
				errmsg("Could not deserialize raster")
			));
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* check that raster is not empty */
		if (rt_raster_is_empty(raster)) {
			elog(NOTICE, "Raster provided is empty");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* raster has bands */
		numbands = rt_raster_get_num_bands(raster); 
		if (!numbands) {
			elog(NOTICE, "Raster provided has no bands");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* initialize arg1 */
		arg1 = rtpg_dumpvalues_arg_init();
		if (arg1 == NULL) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_dumpValues: Could not initialize argument structure");
			SRF_RETURN_DONE(funcctx);
		}

		/* nband, array */
		if (!PG_ARGISNULL(1)) {
			array = PG_GETARG_ARRAYTYPE_P(1);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case INT2OID:
				case INT4OID:
					break;
				default:
					rtpg_dumpvalues_arg_destroy(arg1);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_dumpValues: Invalid data type for band indexes");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e, &nulls, &(arg1->numbands));

			arg1->nbands = palloc(sizeof(int) * arg1->numbands);
			if (arg1->nbands == NULL) {
				rtpg_dumpvalues_arg_destroy(arg1);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_dumpValues: Could not allocate memory for band indexes");
				SRF_RETURN_DONE(funcctx);
			}

			for (i = 0, j = 0; i < arg1->numbands; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case INT2OID:
						arg1->nbands[j] = DatumGetInt16(e[i]) - 1;
						break;
					case INT4OID:
						arg1->nbands[j] = DatumGetInt32(e[i]) - 1;
						break;
				}

				j++;
			}

			if (j < arg1->numbands) {
				arg1->nbands = repalloc(arg1->nbands, sizeof(int) * j);
				if (arg1->nbands == NULL) {
					rtpg_dumpvalues_arg_destroy(arg1);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_dumpValues: Could not reallocate memory for band indexes");
					SRF_RETURN_DONE(funcctx);
				}

				arg1->numbands = j;
			}

			/* validate nbands */
			for (i = 0; i < arg1->numbands; i++) {
				if (!rt_raster_has_band(raster, arg1->nbands[i])) {
					elog(NOTICE, "Band at index %d not found in raster", arg1->nbands[i] + 1);
					rtpg_dumpvalues_arg_destroy(arg1);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					SRF_RETURN_DONE(funcctx);
				}
			}

		}
		else {
			arg1->numbands = numbands;
			arg1->nbands = palloc(sizeof(int) * arg1->numbands);

			if (arg1->nbands == NULL) {
				rtpg_dumpvalues_arg_destroy(arg1);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_dumpValues: Could not allocate memory for band indexes");
				SRF_RETURN_DONE(funcctx);
			}

			for (i = 0; i < arg1->numbands; i++) {
				arg1->nbands[i] = i;
				POSTGIS_RT_DEBUGF(4, "arg1->nbands[%d] = %d", arg1->nbands[i], i);
			}
		}

		arg1->rows = rt_raster_get_height(raster);
		arg1->columns = rt_raster_get_width(raster);

		/* exclude_nodata_value */
		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);
		POSTGIS_RT_DEBUGF(4, "exclude_nodata_value = %d", exclude_nodata_value);

		/* allocate memory for each band's values and nodata flags */
		arg1->values = palloc(sizeof(Datum *) * arg1->numbands);
		arg1->nodata = palloc(sizeof(bool *) * arg1->numbands);
		if (arg1->values == NULL || arg1->nodata == NULL) {
			rtpg_dumpvalues_arg_destroy(arg1);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_dumpValues: Could not allocate memory for pixel values");
			SRF_RETURN_DONE(funcctx);
		}
		memset(arg1->values, 0, sizeof(Datum *) * arg1->numbands);
		memset(arg1->nodata, 0, sizeof(bool *) * arg1->numbands);

		/* get each band and dump data */
		for (z = 0; z < arg1->numbands; z++) {
			band = rt_raster_get_band(raster, arg1->nbands[z]);
			if (!band) {
				int nband = arg1->nbands[z] + 1;
				rtpg_dumpvalues_arg_destroy(arg1);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_dumpValues: Could not get band at index %d", nband);
				SRF_RETURN_DONE(funcctx);
			}

			/* allocate memory for values and nodata flags */
			arg1->values[z] = palloc(sizeof(Datum) * arg1->rows * arg1->columns);
			arg1->nodata[z] = palloc(sizeof(bool) * arg1->rows * arg1->columns);
			if (arg1->values[z] == NULL || arg1->nodata[z] == NULL) {
				rtpg_dumpvalues_arg_destroy(arg1);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_dumpValues: Could not allocate memory for pixel values");
				SRF_RETURN_DONE(funcctx);
			}
			memset(arg1->values[z], 0, sizeof(Datum) * arg1->rows * arg1->columns);
			memset(arg1->nodata[z], 0, sizeof(bool) * arg1->rows * arg1->columns);

			i = 0;

			/* shortcut if band is NODATA */
			if (rt_band_get_isnodata_flag(band)) {
				for (i = (arg1->rows * arg1->columns) - 1; i >= 0; i--)
					arg1->nodata[z][i] = TRUE;
				continue;
			}

			for (y = 0; y < arg1->rows; y++) {
				for (x = 0; x < arg1->columns; x++) {
					/* get pixel */
					if (rt_band_get_pixel(band, x, y, &val, &isnodata) != ES_NONE) {
						int nband = arg1->nbands[z] + 1;
						rtpg_dumpvalues_arg_destroy(arg1);
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 0);
						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_dumpValues: Could not pixel (%d, %d) of band %d", x, y, nband);
						SRF_RETURN_DONE(funcctx);
					}

					arg1->values[z][i] = Float8GetDatum(val);
					POSTGIS_RT_DEBUGF(5, "arg1->values[z][i] = %f", DatumGetFloat8(arg1->values[z][i]));
					POSTGIS_RT_DEBUGF(5, "clamped is?: %d", rt_band_clamped_value_is_nodata(band, val));

					if (exclude_nodata_value && isnodata) {
						arg1->nodata[z][i] = TRUE;
						POSTGIS_RT_DEBUG(5, "nodata = 1");
					}
					else
						POSTGIS_RT_DEBUG(5, "nodata = 0");

					i++;
				}
			}
		}

		/* cleanup */
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);

		/* Store needed information */
		funcctx->user_fctx = arg1;

		/* total number of tuples to be returned */
		funcctx->max_calls = arg1->numbands;

		/* Build a tuple descriptor for our result type */
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

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	tupdesc = funcctx->tuple_desc;
	arg2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 2;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;
		ArrayType *mdValues = NULL;
		int dim[2] = {arg2->rows, arg2->columns};
		int lbound[2] = {1, 1};

		POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);
		POSTGIS_RT_DEBUGF(4, "dim = %d, %d", dim[0], dim[1]);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		values[0] = Int32GetDatum(arg2->nbands[call_cntr] + 1);

		/* info about the type of item in the multi-dimensional array (float8). */
		get_typlenbyvalalign(FLOAT8OID, &typlen, &typbyval, &typalign);

		/* assemble 3-dimension array of values */
		mdValues = construct_md_array(
			arg2->values[call_cntr], arg2->nodata[call_cntr],
			2, dim, lbound,
			FLOAT8OID,
			typlen, typbyval, typalign
		);
		values[1] = PointerGetDatum(mdValues);

		/* build a tuple and datum */
		tuple = heap_form_tuple(tupdesc, values, nulls);
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		rtpg_dumpvalues_arg_destroy(arg2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Write value of raster sample on given position and in specified band.
 */
PG_FUNCTION_INFO_V1(RASTER_setPixelValue);
Datum RASTER_setPixelValue(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	double pixvalue = 0;
	int32_t bandindex = 0;
	int32_t x = 0;
	int32_t y = 0;
	bool skipset = FALSE;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* Check index is not NULL or < 1 */
	if (PG_ARGISNULL(1))
		bandindex = -1;
	else
		bandindex = PG_GETARG_INT32(1);
	
	if (bandindex < 1) {
		elog(NOTICE, "Invalid band index (must use 1-based). Value not set. Returning original raster");
		skipset = TRUE;
	}

	/* Validate pixel coordinates are not null */
	if (PG_ARGISNULL(2)) {
		elog(NOTICE, "X coordinate can not be NULL when setting pixel value. Value not set. Returning original raster");
		skipset = TRUE;
	}
	else
		x = PG_GETARG_INT32(2);

	if (PG_ARGISNULL(3)) {
		elog(NOTICE, "Y coordinate can not be NULL when setting pixel value. Value not set. Returning original raster");
		skipset = TRUE;
	}
	else
		y = PG_GETARG_INT32(3);

	POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

	/* Deserialize raster */
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValue: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	if (!skipset) {
		/* Fetch requested band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Could not find raster band of index %d when setting "
				"pixel value. Value not set. Returning original raster",
				bandindex);
			PG_RETURN_POINTER(pgraster);
		}
		else {
			/* Set the pixel value */
			if (PG_ARGISNULL(4)) {
				if (!rt_band_get_hasnodata_flag(band)) {
					elog(NOTICE, "Raster do not have a nodata value defined. "
						"Set band nodata value first. Nodata value not set. "
						"Returning original raster");
					PG_RETURN_POINTER(pgraster);
				}
				else {
					rt_band_get_nodata(band, &pixvalue);
					rt_band_set_pixel(band, x - 1, y - 1, pixvalue, NULL);
				}
			}
			else {
				pixvalue = PG_GETARG_FLOAT8(4);
				rt_band_set_pixel(band, x - 1, y - 1, pixvalue, NULL);
			}
		}
	}

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Set pixels to value from array
 */
PG_FUNCTION_INFO_V1(RASTER_setPixelValuesArray);
Datum RASTER_setPixelValuesArray(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int numbands = 0;

	int nband = 0;
	int width = 0;
	int height = 0;

	ArrayType *array;
	Oid etype;
	Datum *elements;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int ndims = 1;
	int *dims;
	int num = 0;

	int ul[2] = {0};
	struct pixelvalue {
		int x;
		int y;

		bool noset;
		bool nodata;
		double value;
	};
	struct pixelvalue *pixval = NULL;
	int numpixval = 0;
	int dimpixval[2] = {1, 1};
	int dimnoset[2] = {1, 1};
	int hasnodata = FALSE;
	double nodataval = 0;
	bool keepnodata = FALSE;
	bool hasnosetval = FALSE;
	bool nosetvalisnull = FALSE;
	double nosetval = 0;

	int rtn = 0;
	double val = 0;
	int isnodata = 0;

	int i = 0;
	int j = 0;
	int x = 0;
	int y = 0;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesArray: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* raster attributes */
	numbands = rt_raster_get_num_bands(raster);
	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);

	/* nband */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Band index cannot be NULL.  Value must be 1-based.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	nband = PG_GETARG_INT32(1);
	if (nband < 1 || nband > numbands) {
		elog(NOTICE, "Band index is invalid.  Value must be 1-based.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	/* x, y */
	for (i = 2, j = 0; i < 4; i++, j++) {
		if (PG_ARGISNULL(i)) {
			elog(NOTICE, "%s cannot be NULL.  Value must be 1-based.  Returning original raster", j < 1 ? "X" : "Y");
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}

		ul[j] = PG_GETARG_INT32(i);
		if (
			(ul[j] < 1) || (
				(j < 1 && ul[j] > width) ||
				(j > 0 && ul[j] > height)
			)
		) {
			elog(NOTICE, "%s is invalid.  Value must be 1-based.  Returning original raster", j < 1 ? "X" : "Y");
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}

		/* force 0-based from 1-based */
		ul[j] -= 1;
	}

	/* new value set */
	if (PG_ARGISNULL(4)) {
		elog(NOTICE, "No values to set.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

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
			elog(ERROR, "RASTER_setPixelValuesArray: Invalid data type for new values");
			PG_RETURN_NULL();
			break;
	}

	ndims = ARR_NDIM(array);
	dims = ARR_DIMS(array);
	POSTGIS_RT_DEBUGF(4, "ndims = %d", ndims);

	if (ndims < 1 || ndims > 2) {
		elog(NOTICE, "New values array must be of 1 or 2 dimensions.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}
	/* outer element, then inner element */
	/* i = 0, y */
	/* i = 1, x */
	if (ndims != 2)
		dimpixval[1] = dims[0];
	else {
		dimpixval[0] = dims[0];
		dimpixval[1] = dims[1];
	}
	POSTGIS_RT_DEBUGF(4, "dimpixval = (%d, %d)", dimpixval[0], dimpixval[1]);

	deconstruct_array(
		array,
		etype,
		typlen, typbyval, typalign,
		&elements, &nulls, &num
	);

	/* # of elements doesn't match dims */
	if (num < 1 || num != (dimpixval[0] * dimpixval[1])) {
		if (num) {
			pfree(elements);
			pfree(nulls);
		}
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesArray: Could not deconstruct new values array");
		PG_RETURN_NULL();
	}

	/* allocate memory for pixval */
	numpixval = num;
	pixval = palloc(sizeof(struct pixelvalue) * numpixval);
	if (pixval == NULL) {
		pfree(elements);
		pfree(nulls);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesArray: Could not allocate memory for new pixel values");
		PG_RETURN_NULL();
	}

	/* load new values into pixval */
	i = 0;
	for (y = 0; y < dimpixval[0]; y++) {
		for (x = 0; x < dimpixval[1]; x++) {
			/* 0-based */
			pixval[i].x = ul[0] + x;
			pixval[i].y = ul[1] + y;

			pixval[i].noset = FALSE;
			pixval[i].nodata = FALSE;
			pixval[i].value = 0;

			if (nulls[i])
				pixval[i].nodata = TRUE;
			else {
				switch (etype) {
					case FLOAT4OID:
						pixval[i].value = DatumGetFloat4(elements[i]);
						break;
					case FLOAT8OID:
						pixval[i].value = DatumGetFloat8(elements[i]);
						break;
				}
			}

			i++;
		}
	}

	pfree(elements);
	pfree(nulls);

	/* now load noset flags */
	if (!PG_ARGISNULL(5)) {
		array = PG_GETARG_ARRAYTYPE_P(5);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case BOOLOID:
				break;
			default:
				pfree(pixval);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_setPixelValuesArray: Invalid data type for noset flags");
				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		POSTGIS_RT_DEBUGF(4, "ndims = %d", ndims);

		if (ndims < 1 || ndims > 2) {
			elog(NOTICE, "Noset flags array must be of 1 or 2 dimensions.  Returning original raster");
			pfree(pixval);
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}
		/* outer element, then inner element */
		/* i = 0, y */
		/* i = 1, x */
		if (ndims != 2)
			dimnoset[1] = dims[0];
		else {
			dimnoset[0] = dims[0];
			dimnoset[1] = dims[1];
		}
		POSTGIS_RT_DEBUGF(4, "dimnoset = (%d, %d)", dimnoset[0], dimnoset[1]);

		deconstruct_array(
			array,
			etype,
			typlen, typbyval, typalign,
			&elements, &nulls, &num
		);

		/* # of elements doesn't match dims */
		if (num < 1 || num != (dimnoset[0] * dimnoset[1])) {
			pfree(pixval);
			if (num) {
				pfree(elements);
				pfree(nulls);
			}
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesArray: Could not deconstruct noset flags array");
			PG_RETURN_NULL();
		}

		i = 0;
		j = 0;
		for (y = 0; y < dimnoset[0]; y++) {
			if (y >= dimpixval[0]) break;

			for (x = 0; x < dimnoset[1]; x++) {
				/* fast forward noset elements */
				if (x >= dimpixval[1]) {
					i += (dimnoset[1] - dimpixval[1]);
					break;
				}

				if (!nulls[i] && DatumGetBool(elements[i]))
					pixval[j].noset = TRUE;

				i++;
				j++;
			}

			/* fast forward pixval */
			if (x < dimpixval[1])
				j += (dimpixval[1] - dimnoset[1]);
		}

		pfree(elements);
		pfree(nulls);
	}
	/* hasnosetvalue and nosetvalue */
	else if (!PG_ARGISNULL(6) & PG_GETARG_BOOL(6)) {
		hasnosetval = TRUE;
		if (PG_ARGISNULL(7))
			nosetvalisnull = TRUE;
		else
			nosetval = PG_GETARG_FLOAT8(7);
	}

#if POSTGIS_DEBUG_LEVEL > 0
	for (i = 0; i < numpixval; i++) {
		POSTGIS_RT_DEBUGF(4, "pixval[%d](x, y, noset, nodata, value) = (%d, %d, %d, %d, %f)",
			i,
			pixval[i].x,
			pixval[i].y,
			pixval[i].noset,
			pixval[i].nodata,
			pixval[i].value
		);
	}
#endif

	/* keepnodata flag */
	if (!PG_ARGISNULL(8))
		keepnodata = PG_GETARG_BOOL(8);

	/* get band */
	band = rt_raster_get_band(raster, nband - 1);
	if (!band) {
		elog(NOTICE, "Could not find band at index %d. Returning original raster", nband);
		pfree(pixval);
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	/* get band nodata info */
	/* has NODATA, use NODATA */
	hasnodata = rt_band_get_hasnodata_flag(band);
	if (hasnodata)
		rt_band_get_nodata(band, &nodataval);
	/* no NODATA, use min possible value */
	else
		nodataval = rt_band_get_min_value(band);

	/* set pixels */
	for (i = 0; i < numpixval; i++) {
		/* noset = true, skip */
		if (pixval[i].noset)
			continue;
		/* check against nosetval */
		else if (hasnosetval) {
			/* pixel = NULL AND nosetval = NULL */
			if (pixval[i].nodata && nosetvalisnull)
				continue;
			/* pixel value = nosetval */
			else if (!pixval[i].nodata && !nosetvalisnull && FLT_EQ(pixval[i].value, nosetval))
				continue;
		}

		/* if pixel is outside bounds, skip */
		if (
			(pixval[i].x < 0 || pixval[i].x >= width) ||
			(pixval[i].y < 0 || pixval[i].y >= height)
		) {
			elog(NOTICE, "Cannot set value for pixel (%d, %d) outside raster bounds: %d x %d",
				pixval[i].x + 1, pixval[i].y + 1,
				width, height
			);
			continue;
		}

		/* if hasnodata = TRUE and keepnodata = TRUE, inspect pixel value */
		if (hasnodata && keepnodata) {
			rtn = rt_band_get_pixel(band, pixval[i].x, pixval[i].y, &val, &isnodata);
			if (rtn != ES_NONE) {
				pfree(pixval);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "Cannot get value of pixel");
				PG_RETURN_NULL();
			}

			/* pixel value = NODATA, skip */
			if (isnodata) {
				continue;
			}
		}

		if (pixval[i].nodata)
			rt_band_set_pixel(band, pixval[i].x, pixval[i].y, nodataval, NULL);
		else
			rt_band_set_pixel(band, pixval[i].x, pixval[i].y, pixval[i].value, NULL);
	}

	pfree(pixval);

	/* serialize new raster */
	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/* ---------------------------------------------------------------- */
/*  ST_SetValues using geomval array                                */
/* ---------------------------------------------------------------- */

typedef struct rtpg_setvaluesgv_arg_t *rtpg_setvaluesgv_arg;
typedef struct rtpg_setvaluesgv_geomval_t *rtpg_setvaluesgv_geomval;

struct rtpg_setvaluesgv_arg_t {
	int ngv;
	rtpg_setvaluesgv_geomval gv;

	bool keepnodata;
};

struct rtpg_setvaluesgv_geomval_t {
	struct {
		int nodata;
		double value;
	} pixval;

	LWGEOM *geom;
	rt_raster mask;
};

static rtpg_setvaluesgv_arg rtpg_setvaluesgv_arg_init() {
	rtpg_setvaluesgv_arg arg = palloc(sizeof(struct rtpg_setvaluesgv_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_setvaluesgv_arg_init: Could not allocate memory for function arguments");
		return NULL;
	}

	arg->ngv = 0;
	arg->gv = NULL;
	arg->keepnodata = 0;

	return arg;
}

static void rtpg_setvaluesgv_arg_destroy(rtpg_setvaluesgv_arg arg) {
	int i = 0;

	if (arg->gv != NULL) {
		for (i = 0; i < arg->ngv; i++) {
			if (arg->gv[i].geom != NULL)
				lwgeom_free(arg->gv[i].geom);
			if (arg->gv[i].mask != NULL)
				rt_raster_destroy(arg->gv[i].mask);
		}

		pfree(arg->gv);
	}

	pfree(arg);
}

static int rtpg_setvalues_geomval_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	rtpg_setvaluesgv_arg funcarg = (rtpg_setvaluesgv_arg) userarg;
	int i = 0;
	int j = 0;

	*value = 0;
	*nodata = 0;

	POSTGIS_RT_DEBUGF(4, "keepnodata = %d", funcarg->keepnodata);

	/* keepnodata = TRUE */
	if (funcarg->keepnodata && arg->nodata[0][0][0]) {
		POSTGIS_RT_DEBUG(3, "keepnodata = 1 AND source raster pixel is NODATA");
		*nodata = 1;
		return 1;
	}

	for (i = arg->rasters - 1, j = funcarg->ngv - 1; i > 0; i--, j--) {
		POSTGIS_RT_DEBUGF(4, "checking raster %d", i);

		/* mask is NODATA */
		if (arg->nodata[i][0][0])
			continue;
		/* mask is NOT NODATA */
		else {
			POSTGIS_RT_DEBUGF(4, "Using information from geometry %d", j);

			if (funcarg->gv[j].pixval.nodata)
				*nodata = 1;
			else
				*value = funcarg->gv[j].pixval.value;

			return 1;
		}
	}

	POSTGIS_RT_DEBUG(4, "Using information from source raster");

	/* still here */
	if (arg->nodata[0][0][0])
		*nodata = 1;
	else
		*value = arg->values[0][0][0];

	return 1;
}

PG_FUNCTION_INFO_V1(RASTER_setPixelValuesGeomval);
Datum RASTER_setPixelValuesGeomval(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	rt_raster _raster = NULL;
	rt_band _band = NULL;
	int nband = 0; /* 1-based */

	int numbands = 0;
	int width = 0;
	int height = 0;
	int srid = 0;
	double gt[6] = {0};

	rt_pixtype pixtype = PT_END;
	int hasnodata = 0;
	double nodataval = 0;

	rtpg_setvaluesgv_arg arg = NULL;
	int allpoint = 0;

	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;

	HeapTupleHeader tup;
	bool isnull;
	Datum tupv;

	GSERIALIZED *gser = NULL;
	uint8_t gtype;
	unsigned char *wkb = NULL;
	size_t wkb_len;

	int i = 0;
	int j = 0;
	int noerr = 1;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesGeomval: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* raster attributes */
	numbands = rt_raster_get_num_bands(raster);
	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);
	srid = clamp_srid(rt_raster_get_srid(raster));
	rt_raster_get_geotransform_matrix(raster, gt);

	/* nband */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Band index cannot be NULL.  Value must be 1-based.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	nband = PG_GETARG_INT32(1);
	if (nband < 1 || nband > numbands) {
		elog(NOTICE, "Band index is invalid.  Value must be 1-based.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	/* get band attributes */
	band = rt_raster_get_band(raster, nband - 1);
	pixtype = rt_band_get_pixtype(band);
	hasnodata = rt_band_get_hasnodata_flag(band);
	if (hasnodata)
		rt_band_get_nodata(band, &nodataval);

	/* array of geomval (2) */
	if (PG_ARGISNULL(2)) {
		elog(NOTICE, "No values to set.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	array = PG_GETARG_ARRAYTYPE_P(2);
	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(
		array,
		etype,
		typlen, typbyval, typalign,
		&e, &nulls, &n
	);

	if (!n) {
		elog(NOTICE, "No values to set.  Returning original raster");
		rt_raster_destroy(raster);
		PG_RETURN_POINTER(pgraster);
	}

	/* init arg */
	arg = rtpg_setvaluesgv_arg_init();
	if (arg == NULL) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesGeomval: Could not intialize argument structure");
		PG_RETURN_NULL();
	}

	arg->gv = palloc(sizeof(struct rtpg_setvaluesgv_geomval_t) * n);
	if (arg->gv == NULL) {
		rtpg_setvaluesgv_arg_destroy(arg);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_setPixelValuesGeomval: Could not allocate memory for geomval array");
		PG_RETURN_NULL();
	}

	/* process each element */
	arg->ngv = 0;
	for (i = 0; i < n; i++) {
		if (nulls[i])
			continue;

		arg->gv[arg->ngv].pixval.nodata = 0;
		arg->gv[arg->ngv].pixval.value = 0;
		arg->gv[arg->ngv].geom = NULL;
		arg->gv[arg->ngv].mask = NULL;

		/* each element is a tuple */
		tup = (HeapTupleHeader) DatumGetPointer(e[i]);
		if (NULL == tup) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Invalid argument for geomval at index %d", i);
			PG_RETURN_NULL();
		}

		/* first element, geometry */
		POSTGIS_RT_DEBUG(4, "Processing first element (geometry)");
		tupv = GetAttributeByName(tup, "geom", &isnull);
		if (isnull) {
			elog(NOTICE, "First argument (geom) of geomval at index %d is NULL. Skipping", i);
			continue;
		}

		gser = (GSERIALIZED *) PG_DETOAST_DATUM(tupv);
		arg->gv[arg->ngv].geom = lwgeom_from_gserialized(gser);
		if (arg->gv[arg->ngv].geom == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not deserialize geometry of geomval at index %d", i);
			PG_RETURN_NULL();
		}

		/* empty geometry */
		if (lwgeom_is_empty(arg->gv[arg->ngv].geom)) {
			elog(NOTICE, "First argument (geom) of geomval at index %d is an empty geometry. Skipping", i);
			continue;
		}

		/* check SRID */
		if (clamp_srid(gserialized_get_srid(gser)) != srid) {
			elog(NOTICE, "Geometry provided for geomval at index %d does not have the same SRID as the raster: %d. Returning original raster", i, srid);
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}

		/* Get a 2D version of the geometry if necessary */
		if (lwgeom_ndims(arg->gv[arg->ngv].geom) > 2) {
			LWGEOM *geom2d = lwgeom_force_2d(arg->gv[arg->ngv].geom);
			lwgeom_free(arg->gv[arg->ngv].geom);
			arg->gv[arg->ngv].geom = geom2d;
		}

		/* filter for types */
		gtype = gserialized_get_type(gser);

		/* shortcuts for POINT and MULTIPOINT */
		if (gtype == POINTTYPE || gtype == MULTIPOINTTYPE)
			allpoint++;

		/* get wkb of geometry */
		POSTGIS_RT_DEBUG(3, "getting wkb of geometry");
		wkb = lwgeom_to_wkb(arg->gv[arg->ngv].geom, WKB_SFSQL, &wkb_len);

		/* rasterize geometry */
		arg->gv[arg->ngv].mask = rt_raster_gdal_rasterize(
			wkb, wkb_len,
			NULL,
			0, NULL,
			NULL, NULL,
			NULL, NULL,
			NULL, NULL,
			&(gt[1]), &(gt[5]),
			NULL, NULL,
			&(gt[0]), &(gt[3]),
			&(gt[2]), &(gt[4]),
			NULL
		);

		pfree(wkb);
		if (gtype != POINTTYPE && gtype != MULTIPOINTTYPE) {
			lwgeom_free(arg->gv[arg->ngv].geom);
			arg->gv[arg->ngv].geom = NULL;
		}

		if (arg->gv[arg->ngv].mask == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not rasterize geometry of geomval at index %d", i);
			PG_RETURN_NULL();
		}

		/* set SRID */
		rt_raster_set_srid(arg->gv[arg->ngv].mask, srid);

		/* second element, value */
		POSTGIS_RT_DEBUG(4, "Processing second element (val)");
		tupv = GetAttributeByName(tup, "val", &isnull);
		if (isnull) {
			elog(NOTICE, "Second argument (val) of geomval at index %d is NULL. Treating as NODATA", i);
			arg->gv[arg->ngv].pixval.nodata = 1;
		}
		else
			arg->gv[arg->ngv].pixval.value = DatumGetFloat8(tupv);

		(arg->ngv)++;
	}

	/* redim arg->gv if needed */
	if (arg->ngv < n) {
		arg->gv = repalloc(arg->gv, sizeof(struct rtpg_setvaluesgv_geomval_t) * arg->ngv);
		if (arg->gv == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not reallocate memory for geomval array");
			PG_RETURN_NULL();
		}
	}

	/* keepnodata */
	if (!PG_ARGISNULL(3))
		arg->keepnodata = PG_GETARG_BOOL(3);
	POSTGIS_RT_DEBUGF(3, "keepnodata = %d", arg->keepnodata);

	/* keepnodata = TRUE and band is NODATA */
	if (arg->keepnodata && rt_band_get_isnodata_flag(band)) {
		POSTGIS_RT_DEBUG(3, "keepnodata = TRUE and band is NODATA. Not doing anything");
	}
	/* all elements are points */
	else if (allpoint == arg->ngv) {
		double igt[6] = {0};
		double xy[2] = {0};
		double value = 0;
		int isnodata = 0;

		LWCOLLECTION *coll = NULL;
		LWPOINT *point = NULL;
		POINT2D p;

		POSTGIS_RT_DEBUG(3, "all geometries are points, using direct to pixel method");

		/* cache inverse gretransform matrix */
		rt_raster_get_inverse_geotransform_matrix(NULL, gt, igt);

		/* process each geometry */
		for (i = 0; i < arg->ngv; i++) {
			/* convert geometry to collection */
			coll = lwgeom_as_lwcollection(lwgeom_as_multi(arg->gv[i].geom));

			/* process each point in collection */
			for (j = 0; j < coll->ngeoms; j++) {
				point = lwgeom_as_lwpoint(coll->geoms[j]);
				getPoint2d_p(point->point, 0, &p);

				if (rt_raster_geopoint_to_cell(raster, p.x, p.y, &(xy[0]), &(xy[1]), igt) != ES_NONE) {
					rtpg_setvaluesgv_arg_destroy(arg);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					elog(ERROR, "RASTER_setPixelValuesGeomval: Could not process coordinates of point");
					PG_RETURN_NULL();
				}

				/* skip point if outside raster */
				if (
					(xy[0] < 0 || xy[0] >= width) ||
					(xy[1] < 0 || xy[1] >= height)
				) {
					elog(NOTICE, "Point is outside raster extent. Skipping");
					continue;
				}

				/* get pixel value */
				if (rt_band_get_pixel(band, xy[0], xy[1], &value, &isnodata) != ES_NONE) {
					rtpg_setvaluesgv_arg_destroy(arg);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					elog(ERROR, "RASTER_setPixelValuesGeomval: Could not get pixel value");
					PG_RETURN_NULL();
				}

				/* keepnodata = TRUE AND pixel value is NODATA */
				if (arg->keepnodata && isnodata)
					continue;

				/* set pixel */
				if (arg->gv[i].pixval.nodata)
					noerr = rt_band_set_pixel(band, xy[0], xy[1], nodataval, NULL);
				else
					noerr = rt_band_set_pixel(band, xy[0], xy[1], arg->gv[i].pixval.value, NULL);

				if (noerr != ES_NONE) {
					rtpg_setvaluesgv_arg_destroy(arg);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					elog(ERROR, "RASTER_setPixelValuesGeomval: Could not set pixel value");
					PG_RETURN_NULL();
				}
			}
		}
	}
	/* run iterator otherwise */
	else {
		rt_iterator itrset;

		POSTGIS_RT_DEBUG(3, "a mix of geometries, using iterator method");

		/* init itrset */
		itrset = palloc(sizeof(struct rt_iterator_t) * (arg->ngv + 1));
		if (itrset == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not allocate memory for iterator arguments");
			PG_RETURN_NULL();
		}

		/* set first raster's info */
		itrset[0].raster = raster;
		itrset[0].nband = nband - 1;
		itrset[0].nbnodata = 1;

		/* set other raster's info */
		for (i = 0, j = 1; i < arg->ngv; i++, j++) {
			itrset[j].raster = arg->gv[i].mask;
			itrset[j].nband = 0;
			itrset[j].nbnodata = 1;
		}

		/* pass to iterator */
		noerr = rt_raster_iterator(
			itrset, arg->ngv + 1,
			ET_FIRST, NULL,
			pixtype,
			hasnodata, nodataval,
			0, 0,
			arg,
			rtpg_setvalues_geomval_callback,
			&_raster
		);
		pfree(itrset);

		if (noerr != ES_NONE) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not run raster iterator function");
			PG_RETURN_NULL();
		}

		/* copy band from _raster to raster */
		_band = rt_raster_get_band(_raster, 0);
		if (_band == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(_raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not get band from working raster");
			PG_RETURN_NULL();
		}

		_band = rt_raster_replace_band(raster, _band, nband - 1);
		if (_band == NULL) {
			rtpg_setvaluesgv_arg_destroy(arg);
			rt_raster_destroy(_raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_setPixelValuesGeomval: Could not replace band in output raster");
			PG_RETURN_NULL();
		}

		rt_band_destroy(_band);
		rt_raster_destroy(_raster);
	}

	rtpg_setvaluesgv_arg_destroy(arg);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	POSTGIS_RT_DEBUG(3, "Finished");

	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Return the geographical shape of all pixels
 */
PG_FUNCTION_INFO_V1(RASTER_getPixelPolygons);
Datum RASTER_getPixelPolygons(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	rt_pixel pix = NULL;
	rt_pixel pix2;
	int call_cntr;
	int max_calls;
	int i = 0;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;
		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int nband = 1;
		int numbands;
		bool noband = FALSE;
		bool exclude_nodata_value = TRUE;
		bool nocolumnx = FALSE;
		bool norowy = FALSE;
		int x = 0;
		int y = 0;
		int bounds[4] = {0};
		int pixcount = 0;
		int isnodata = 0;

		LWPOLY *poly;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		/* band */
		if (PG_ARGISNULL(1))
			noband = TRUE;
		else {
			nband = PG_GETARG_INT32(1);
			noband = FALSE;
		}

		/* column */
		if (PG_ARGISNULL(2))
			nocolumnx = TRUE;
		else {
			bounds[0] = PG_GETARG_INT32(2);
			bounds[1] = bounds[0];
		}

		/* row */
		if (PG_ARGISNULL(3))
			norowy = TRUE;
		else {
			bounds[2] = PG_GETARG_INT32(3);
			bounds[3] = bounds[2];
		}

		/* exclude NODATA */
		if (!PG_ARGISNULL(4))
			exclude_nodata_value = PG_GETARG_BOOL(4);

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			ereport(ERROR, (
				errcode(ERRCODE_OUT_OF_MEMORY),
				errmsg("Could not deserialize raster")
			));
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* raster empty, return NULL */
		if (rt_raster_is_empty(raster)) {
			elog(NOTICE, "Raster is empty. Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* band specified, load band and info */
		if (!noband) {
			do {
				numbands = rt_raster_get_num_bands(raster);
				POSTGIS_RT_DEBUGF(3, "band %d", nband);
				POSTGIS_RT_DEBUGF(3, "# of bands %d", numbands);

				if (nband < 1 || nband > numbands) {
					elog(NOTICE, "Invalid band index (must use 1-based). Returning pixel values will be NULL");
					noband = TRUE;
					break;
				}

				band = rt_raster_get_band(raster, nband - 1);
				if (!band) {
					elog(NOTICE, "Could not find band at index %d. Returning pixel values will be NULL", nband);
					noband = TRUE;
					break;
				}

				if (!rt_band_get_hasnodata_flag(band))
					exclude_nodata_value = FALSE;
			}
			while (0);
		}

		/* set bounds if columnx, rowy not set */
		if (nocolumnx) {
			bounds[0] = 1;
			bounds[1] = rt_raster_get_width(raster);
		}
		if (norowy) {
			bounds[2] = 1;
			bounds[3] = rt_raster_get_height(raster);
		}
		POSTGIS_RT_DEBUGF(3, "bounds (min x, max x, min y, max y) = (%d, %d, %d, %d)", 
			bounds[0], bounds[1], bounds[2], bounds[3]);

		/* rowy */
		pixcount = 0;
		for (y = bounds[2]; y <= bounds[3]; y++) {
			/* columnx */
			for (x = bounds[0]; x <= bounds[1]; x++) {
				/* geometry */
				poly = rt_raster_pixel_as_polygon(raster, x - 1, y - 1);
				if (!poly) {
					for (i = 0; i < pixcount; i++)
						lwgeom_free(pix[i].geom);
					if (pixcount) pfree(pix);

					if (!noband) rt_band_destroy(band);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_getPixelPolygons: Could not get pixel polygon");
					SRF_RETURN_DONE(funcctx);
				}

				if (!pixcount)
					pix = palloc(sizeof(struct rt_pixel_t) * (pixcount + 1));
				else
					pix = repalloc(pix, sizeof(struct rt_pixel_t) * (pixcount + 1));
				if (pix == NULL) {

					lwpoly_free(poly);
					if (!noband) rt_band_destroy(band);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_getPixelPolygons: Could not allocate memory for storing pixel polygons");
					SRF_RETURN_DONE(funcctx);
				}
				pix[pixcount].geom = (LWGEOM *) poly;
				/*
				POSTGIS_RT_DEBUGF(4, "poly @ %p", poly);
				POSTGIS_RT_DEBUGF(4, "geom @ %p", pix[pixcount].geom);
				*/

				/* x, y */
				pix[pixcount].x = x;
				pix[pixcount].y = y;

				/* value, NODATA flag */
				if (!noband) {
					if (rt_band_get_pixel(band, x - 1, y - 1, &(pix[pixcount].value), &isnodata) != ES_NONE) {

						for (i = 0; i < pixcount; i++)
							lwgeom_free(pix[i].geom);
						if (pixcount) pfree(pix);

						if (!noband) rt_band_destroy(band);
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 0);

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_getPixelPolygons: Could not get pixel value");
						SRF_RETURN_DONE(funcctx);
					}

					if (!exclude_nodata_value || !isnodata) {
						pix[pixcount].nodata = 0;
					}
					else {
						pix[pixcount].nodata = 1;
					}
				}
				else {
					pix[pixcount].nodata = 1;
				}

				pixcount++;
			}
		}

		if (!noband) rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);

		/* Store needed information */
		funcctx->user_fctx = pix;

		/* total number of tuples to be returned */
		funcctx->max_calls = pixcount;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
			ereport(ERROR, (
				errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context that cannot accept type record")
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
	pix2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 4;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		GSERIALIZED *gser = NULL;
		size_t gser_size = 0;

		POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		/* convert LWGEOM to GSERIALIZED */
		gser = gserialized_from_lwgeom(pix2[call_cntr].geom, 0, &gser_size);
		lwgeom_free(pix2[call_cntr].geom);

		values[0] = PointerGetDatum(gser);
		if (pix2[call_cntr].nodata)
			nulls[1] = TRUE;
		else
			values[1] = Float8GetDatum(pix2[call_cntr].value);
		values[2] = Int32GetDatum(pix2[call_cntr].x);
		values[3] = Int32GetDatum(pix2[call_cntr].y);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(pix2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Get raster band's polygon
 */
PG_FUNCTION_INFO_V1(RASTER_getPolygon);
Datum RASTER_getPolygon(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	int num_bands = 0;
	int nband = 1;
	int err;
	LWMPOLY *surface = NULL;
	GSERIALIZED *rtn = NULL;

	/* raster */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_getPolygon: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* num_bands */
	num_bands = rt_raster_get_num_bands(raster);
	if (num_bands < 1) {
		elog(NOTICE, "Raster provided has no bands");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* band index is 1-based */
	if (!PG_ARGISNULL(1))
		nband = PG_GETARG_INT32(1);
	if (nband < 1 || nband > num_bands) {
		elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* get band surface */
	err = rt_raster_surface(raster, nband - 1, &surface);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	if (err != ES_NONE) {
		elog(ERROR, "RASTER_getPolygon: Could not get raster band's surface");
		PG_RETURN_NULL();
	}
	else if (surface == NULL) {
		elog(NOTICE, "Raster is empty or all pixels of band are NODATA. Returning NULL");
		PG_RETURN_NULL();
	}

	rtn = geometry_serialize(lwmpoly_as_lwgeom(surface));
	lwmpoly_free(surface);

	PG_RETURN_POINTER(rtn);
}

/**
 * Get pixels of value
 */
PG_FUNCTION_INFO_V1(RASTER_pixelOfValue);
Datum RASTER_pixelOfValue(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	rt_pixel pixels = NULL;
	rt_pixel pixels2 = NULL;
	int count = 0;
	int i = 0;
	int n = 0;
	int call_cntr;
	int max_calls;

	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int nband = 1;
		int num_bands = 0;
		double *search = NULL;
		int nsearch = 0;
		double val;
		bool exclude_nodata_value = TRUE;

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

		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_pixelOfValue: Could not deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* num_bands */
		num_bands = rt_raster_get_num_bands(raster);
		if (num_bands < 1) {
			elog(NOTICE, "Raster provided has no bands");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* band index is 1-based */
		if (!PG_ARGISNULL(1))
			nband = PG_GETARG_INT32(1);
		if (nband < 1 || nband > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* search values */
		array = PG_GETARG_ARRAYTYPE_P(2);
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
				elog(ERROR, "RASTER_pixelOfValue: Invalid data type for pixel values");
				SRF_RETURN_DONE(funcctx);
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		search = palloc(sizeof(double) * n);
		for (i = 0, nsearch = 0; i < n; i++) {
			if (nulls[i]) continue;

			switch (etype) {
				case FLOAT4OID:
					val = (double) DatumGetFloat4(e[i]);
					break;
				case FLOAT8OID:
					val = (double) DatumGetFloat8(e[i]);
					break;
			}

			search[nsearch] = val;
			POSTGIS_RT_DEBUGF(3, "search[%d] = %f", nsearch, search[nsearch]);
			nsearch++;
		}

		/* not searching for anything */
		if (nsearch < 1) {
			elog(NOTICE, "No search values provided. Returning NULL");
			pfree(search);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		else if (nsearch < n)
			search = repalloc(search, sizeof(double) * nsearch);

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(3))
			exclude_nodata_value = PG_GETARG_BOOL(3);

		/* get band */
		band = rt_raster_get_band(raster, nband - 1);
		if (!band) {
			elog(NOTICE, "Could not find band at index %d. Returning NULL", nband);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get pixels of values */
		count = rt_band_get_pixel_of_value(
			band, exclude_nodata_value,
			search, nsearch,
			&pixels
		);
		pfree(search);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (count < 1) {
			/* error */
			if (count < 0)
				elog(NOTICE, "Could not get the pixels of search values for band at index %d", nband);
			/* no nearest pixel */
			else
				elog(NOTICE, "No pixels of search values found for band at index %d", nband);

			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* Store needed information */
		funcctx->user_fctx = pixels;

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
	pixels2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 3;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		memset(nulls, FALSE, sizeof(bool) * values_length);

		/* 0-based to 1-based */
		pixels2[call_cntr].x += 1;
		pixels2[call_cntr].y += 1;

		values[0] = Float8GetDatum(pixels2[call_cntr].value);
		values[1] = Int32GetDatum(pixels2[call_cntr].x);
		values[2] = Int32GetDatum(pixels2[call_cntr].y);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	else {
		pfree(pixels2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Return nearest value to a point
 */
PG_FUNCTION_INFO_V1(RASTER_nearestValue);
Datum RASTER_nearestValue(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int bandindex = 1;
	int num_bands = 0;
	GSERIALIZED *geom;
	bool exclude_nodata_value = TRUE;
	LWGEOM *lwgeom;
	LWPOINT *point = NULL;
	POINT2D p;

	double x;
	double y;
	int count;
	rt_pixel npixels = NULL;
	double value = 0;
	int hasvalue = 0;
	int isnodata = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_nearestValue: Could not deserialize raster");
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

	/* point */
	geom = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(2));
	if (gserialized_get_type(geom) != POINTTYPE) {
		elog(NOTICE, "Geometry provided must be a point");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_FREE_IF_COPY(geom, 2);
		PG_RETURN_NULL();
	}

	/* exclude_nodata_value flag */
	if (!PG_ARGISNULL(3))
		exclude_nodata_value = PG_GETARG_BOOL(3);

	/* SRIDs of raster and geometry must match  */
	if (clamp_srid(gserialized_get_srid(geom)) != clamp_srid(rt_raster_get_srid(raster))) {
		elog(NOTICE, "SRIDs of geometry and raster do not match");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_FREE_IF_COPY(geom, 2);
		PG_RETURN_NULL();
	}

	/* get band */
	band = rt_raster_get_band(raster, bandindex - 1);
	if (!band) {
		elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_FREE_IF_COPY(geom, 2);
		PG_RETURN_NULL();
	}

	/* process geometry */
	lwgeom = lwgeom_from_gserialized(geom);

	if (lwgeom_is_empty(lwgeom)) {
		elog(NOTICE, "Geometry provided cannot be empty");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_FREE_IF_COPY(geom, 2);
		PG_RETURN_NULL();
	}

	/* Get a 2D version of the geometry if necessary */
	if (lwgeom_ndims(lwgeom) > 2) {
		LWGEOM *lwgeom2d = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = lwgeom2d;
	}

	point = lwgeom_as_lwpoint(lwgeom);
	getPoint2d_p(point->point, 0, &p);

	if (rt_raster_geopoint_to_cell(
		raster,
		p.x, p.y,
		&x, &y,
		NULL
	) != ES_NONE) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		lwgeom_free(lwgeom);
		PG_FREE_IF_COPY(geom, 2);
		elog(ERROR, "RASTER_nearestValue: Could not compute pixel coordinates from spatial coordinates");
		PG_RETURN_NULL();
	}

	/* get value at point */
	if (
		(x >= 0 && x < rt_raster_get_width(raster)) &&
		(y >= 0 && y < rt_raster_get_height(raster))
	) {
		if (rt_band_get_pixel(band, x, y, &value, &isnodata) != ES_NONE) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			lwgeom_free(lwgeom);
			PG_FREE_IF_COPY(geom, 2);
			elog(ERROR, "RASTER_nearestValue: Could not get pixel value for band at index %d", bandindex);
			PG_RETURN_NULL();
		}

		/* value at point, return value */
		if (!exclude_nodata_value || !isnodata) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			lwgeom_free(lwgeom);
			PG_FREE_IF_COPY(geom, 2);

			PG_RETURN_FLOAT8(value);
		}
	}

	/* get neighborhood */
	count = rt_band_get_nearest_pixel(
		band,
		x, y,
		0, 0,
		exclude_nodata_value,
		&npixels
	);
	rt_band_destroy(band);
	/* error or no neighbors */
	if (count < 1) {
		/* error */
		if (count < 0)
			elog(NOTICE, "Could not get the nearest value for band at index %d", bandindex);
		/* no nearest pixel */
		else
			elog(NOTICE, "No nearest value found for band at index %d", bandindex);

		lwgeom_free(lwgeom);
		PG_FREE_IF_COPY(geom, 2);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* more than one nearest value, see which one is closest */
	if (count > 1) {
		int i = 0;
		LWPOLY *poly = NULL;
		double lastdist = -1;
		double dist;

		for (i = 0; i < count; i++) {
			/* convex-hull of pixel */
			poly = rt_raster_pixel_as_polygon(raster, npixels[i].x, npixels[i].y);
			if (!poly) {
				lwgeom_free(lwgeom);
				PG_FREE_IF_COPY(geom, 2);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_nearestValue: Could not get polygon of neighboring pixel");
				PG_RETURN_NULL();
			}

			/* distance between convex-hull and point */
			dist = lwgeom_mindistance2d(lwpoly_as_lwgeom(poly), lwgeom);
			if (lastdist < 0 || dist < lastdist) {
				value = npixels[i].value;
				hasvalue = 1;
			}
			lastdist = dist;

			lwpoly_free(poly);
		}
	}
	else {
		value = npixels[0].value;
		hasvalue = 1;
	}

	pfree(npixels);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 2);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	if (hasvalue)
		PG_RETURN_FLOAT8(value);
	else
		PG_RETURN_NULL();
}

/**
 * Return the neighborhood around a pixel
 */
PG_FUNCTION_INFO_V1(RASTER_neighborhood);
Datum RASTER_neighborhood(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int bandindex = 1;
	int num_bands = 0;
	int x = 0;
	int y = 0;
	int _x = 0;
	int _y = 0;
	int distance[2] = {0};
	bool exclude_nodata_value = TRUE;
	double pixval;
	int isnodata = 0;

	rt_pixel npixels = NULL;
	int count;
	double **value2D = NULL;
	int **nodata2D = NULL;

	int i = 0;
	int j = 0;
	int k = 0;
	Datum *value1D = NULL;
	bool *nodata1D = NULL;
	int dim[2] = {0};
	int lbound[2] = {1, 1};
	ArrayType *mdArray = NULL;

	int16 typlen;
	bool typbyval;
	char typalign;

	/* pgraster is null, return nothing */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_neighborhood: Could not deserialize raster");
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

	/* pixel column, 1-based */
	x = PG_GETARG_INT32(2);
	_x = x - 1;

	/* pixel row, 1-based */
	y = PG_GETARG_INT32(3);
	_y = y - 1;

	/* distance X axis */
	distance[0] = PG_GETARG_INT32(4);
	if (distance[0] < 0) {
		elog(NOTICE, "Invalid value for distancex (must be >= zero). Returning NULL");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}
	distance[0] = (uint16_t) distance[0];

	/* distance Y axis */
	distance[1] = PG_GETARG_INT32(5);
	if (distance[1] < 0) {
		elog(NOTICE, "Invalid value for distancey (must be >= zero). Returning NULL");
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}
	distance[1] = (uint16_t) distance[1];

	/* exclude_nodata_value flag */
	if (!PG_ARGISNULL(6))
		exclude_nodata_value = PG_GETARG_BOOL(6);

	/* get band */
	band = rt_raster_get_band(raster, bandindex - 1);
	if (!band) {
		elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		PG_RETURN_NULL();
	}

	/* get neighborhood */
	count = 0;
	npixels = NULL;
	if (distance[0] > 0 || distance[1] > 0) {
		count = rt_band_get_nearest_pixel(
			band,
			_x, _y,
			distance[0], distance[1],
			exclude_nodata_value,
			&npixels
		);
		/* error */
		if (count < 0) {
			elog(NOTICE, "Could not get the pixel's neighborhood for band at index %d", bandindex);
			
			rt_band_destroy(band);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);

			PG_RETURN_NULL();
		}
	}

	/* get pixel's value */
	if (
		(_x >= 0 && _x < rt_band_get_width(band)) &&
		(_y >= 0 && _y < rt_band_get_height(band))
	) {
		if (rt_band_get_pixel(
			band,
			_x, _y,
			&pixval,
			&isnodata
		) != ES_NONE) {
			elog(NOTICE, "Could not get the pixel of band at index %d. Returning NULL", bandindex);
			rt_band_destroy(band);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			PG_RETURN_NULL();
		}
	}
	/* outside band extent, set to NODATA */
	else {
		/* has NODATA, use NODATA */
		if (rt_band_get_hasnodata_flag(band))
			rt_band_get_nodata(band, &pixval);
		/* no NODATA, use min possible value */
		else
			pixval = rt_band_get_min_value(band);
		isnodata = 1;
	}
	POSTGIS_RT_DEBUGF(4, "pixval: %f", pixval);


	/* add pixel to neighborhood */
	count++;
	if (count > 1)
		npixels = (rt_pixel) repalloc(npixels, sizeof(struct rt_pixel_t) * count);
	else
		npixels = (rt_pixel) palloc(sizeof(struct rt_pixel_t));
	if (npixels == NULL) {

		rt_band_destroy(band);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);

		elog(ERROR, "RASTER_neighborhood: Could not reallocate memory for neighborhood");
		PG_RETURN_NULL();
	}
	npixels[count - 1].x = _x;
	npixels[count - 1].y = _y;
	npixels[count - 1].nodata = 1;
	npixels[count - 1].value = pixval;

	/* set NODATA */
	if (!exclude_nodata_value || !isnodata) {
		npixels[count - 1].nodata = 0;
	}

	/* free unnecessary stuff */
	rt_band_destroy(band);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	/* convert set of rt_pixel to 2D array */
	/* dim is passed with element 0 being Y-axis and element 1 being X-axis */
	count = rt_pixel_set_to_array(
		npixels, count,
		_x, _y,
		distance[0], distance[1],
		&value2D,
		&nodata2D,
		&(dim[1]), &(dim[0])
	);
	pfree(npixels);
	if (count != ES_NONE) {
		elog(NOTICE, "Could not create 2D array of neighborhood");
		PG_RETURN_NULL();
	}

	/* 1D arrays for values and nodata from 2D arrays */
	value1D = palloc(sizeof(Datum) * dim[0] * dim[1]);
	nodata1D = palloc(sizeof(bool) * dim[0] * dim[1]);

	if (value1D == NULL || nodata1D == NULL) {

		for (i = 0; i < dim[0]; i++) {
			pfree(value2D[i]);
			pfree(nodata2D[i]);
		}
		pfree(value2D);
		pfree(nodata2D);

		elog(ERROR, "RASTER_neighborhood: Could not allocate memory for return 2D array");
		PG_RETURN_NULL();
	}

	/* copy values from 2D arrays to 1D arrays */
	k = 0;
	/* Y-axis */
	for (i = 0; i < dim[0]; i++) {
		/* X-axis */
		for (j = 0; j < dim[1]; j++) {
			nodata1D[k] = (bool) nodata2D[i][j];
			if (!nodata1D[k])
				value1D[k] = Float8GetDatum(value2D[i][j]);
			else
				value1D[k] = PointerGetDatum(NULL);

			k++;
		}
	}

	/* no more need for 2D arrays */
	for (i = 0; i < dim[0]; i++) {
		pfree(value2D[i]);
		pfree(nodata2D[i]);
	}
	pfree(value2D);
	pfree(nodata2D);

	/* info about the type of item in the multi-dimensional array (float8). */
	get_typlenbyvalalign(FLOAT8OID, &typlen, &typbyval, &typalign);

	mdArray = construct_md_array(
		value1D, nodata1D,
		2, dim, lbound,
		FLOAT8OID,
		typlen, typbyval, typalign
	);

	pfree(value1D);
	pfree(nodata1D);

	PG_RETURN_ARRAYTYPE_P(mdArray);
}

/**
 * Add band(s) to the given raster at the given position(s).
 */
PG_FUNCTION_INFO_V1(RASTER_addBand);
Datum RASTER_addBand(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	int bandindex = 0;
	int maxbandindex = 0;
	int numbands = 0;
	int lastnumbands = 0;

	text *text_pixtype = NULL;
	char *char_pixtype = NULL;

	struct addbandarg {
		int index;
		bool append;
		rt_pixtype pixtype;
		double initialvalue;
		bool hasnodata;
		double nodatavalue;
	};
	struct addbandarg *arg = NULL;

	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;

	HeapTupleHeader tup;
	bool isnull;
	Datum tupv;

	int i = 0;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_addBand: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* process set of addbandarg */
	POSTGIS_RT_DEBUG(3, "Processing Arg 1 (addbandargset)");
	array = PG_GETARG_ARRAYTYPE_P(1);
	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
		&nulls, &n);

	if (!n) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset");
		PG_RETURN_NULL();
	}

	/* allocate addbandarg */
	arg = (struct addbandarg *) palloc(sizeof(struct addbandarg) * n);
	if (arg == NULL) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_addBand: Could not allocate memory for addbandarg");
		PG_RETURN_NULL();
	}

	/*
		process each element of addbandargset
		each element is the index of where to add the new band,
			new band's pixeltype, the new band's initial value and
			the new band's NODATA value if NOT NULL
	*/
	for (i = 0; i < n; i++) {
		if (nulls[i]) continue;

		POSTGIS_RT_DEBUGF(4, "Processing addbandarg at index %d", i);

		/* each element is a tuple */
		tup = (HeapTupleHeader) DatumGetPointer(e[i]);
		if (NULL == tup) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset");
			PG_RETURN_NULL();
		}

		/* new band index, 1-based */
		arg[i].index = 0;
		arg[i].append = TRUE;
		tupv = GetAttributeByName(tup, "index", &isnull);
		if (!isnull) {
			arg[i].index = DatumGetInt32(tupv);
			arg[i].append = FALSE;
		}

		/* for now, only check that band index is 1-based */
		if (!arg[i].append && arg[i].index < 1) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset. Invalid band index (must be 1-based) for addbandarg of index %d", i);
			PG_RETURN_NULL();
		}

		/* new band pixeltype */
		arg[i].pixtype = PT_END;
		tupv = GetAttributeByName(tup, "pixeltype", &isnull);
		if (isnull) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset. Pixel type cannot be NULL for addbandarg of index %d", i);
			PG_RETURN_NULL();
		}
		text_pixtype = (text *) DatumGetPointer(tupv);
		if (text_pixtype == NULL) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset. Pixel type cannot be NULL for addbandarg of index %d", i);
			PG_RETURN_NULL();
		}
		char_pixtype = text_to_cstring(text_pixtype);

		arg[i].pixtype = rt_pixtype_index_from_name(char_pixtype);
		pfree(char_pixtype);
		if (arg[i].pixtype == PT_END) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Invalid argument for addbandargset. Invalid pixel type for addbandarg of index %d", i);
			PG_RETURN_NULL();
		}

		/* new band initialvalue */
		arg[i].initialvalue = 0;
		tupv = GetAttributeByName(tup, "initialvalue", &isnull);
		if (!isnull)
			arg[i].initialvalue = DatumGetFloat8(tupv);

		/* new band NODATA value */
		arg[i].hasnodata = FALSE;
		arg[i].nodatavalue = 0;
		tupv = GetAttributeByName(tup, "nodataval", &isnull);
		if (!isnull) {
			arg[i].hasnodata = TRUE;
			arg[i].nodatavalue = DatumGetFloat8(tupv);
		}
	}

	/* add new bands to raster */
	lastnumbands = rt_raster_get_num_bands(raster);
	for (i = 0; i < n; i++) {
		if (nulls[i]) continue;

		POSTGIS_RT_DEBUGF(3, "%d bands in old raster", lastnumbands);
		maxbandindex = lastnumbands + 1;

		/* check that new band's index doesn't exceed maxbandindex */
		if (!arg[i].append) {
			if (arg[i].index > maxbandindex) {
				elog(NOTICE, "Band index for addbandarg of index %d exceeds possible value. Adding band at index %d", i, maxbandindex);
				arg[i].index = maxbandindex;
			}
		}
		/* append, so use maxbandindex */
		else
			arg[i].index = maxbandindex;

		POSTGIS_RT_DEBUGF(4, "new band (index, pixtype, initialvalue, hasnodata, nodatavalue) = (%d, %s, %f, %s, %f)",
			arg[i].index,
			rt_pixtype_name(arg[i].pixtype),
			arg[i].initialvalue,
			arg[i].hasnodata ? "TRUE" : "FALSE",
			arg[i].nodatavalue
		);

		bandindex = rt_raster_generate_new_band(
			raster,
			arg[i].pixtype, arg[i].initialvalue,
			arg[i].hasnodata, arg[i].nodatavalue,
			arg[i].index - 1
		);

		numbands = rt_raster_get_num_bands(raster);
		if (numbands == lastnumbands || bandindex == -1) {
			pfree(arg);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBand: Could not add band defined by addbandarg of index %d to raster", i);
			PG_RETURN_NULL();
		}

		lastnumbands = numbands;
		POSTGIS_RT_DEBUGF(3, "%d bands in new raster", lastnumbands);
	}

	pfree(arg);

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Add bands from array of rasters to a destination raster
 */
PG_FUNCTION_INFO_V1(RASTER_addBandRasterArray);
Datum RASTER_addBandRasterArray(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgsrc = NULL;
	rt_pgraster *pgrtn = NULL;

	rt_raster raster = NULL;
	rt_raster src = NULL;

	int srcnband = 1;
	bool appendband = FALSE;
	int dstnband = 1;
	int srcnumbands = 0;
	int dstnumbands = 0;

	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;

	int rtn = 0;
	int i = 0;

	/* destination raster */
	if (!PG_ARGISNULL(0)) {
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		/* raster */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandRasterArray: Could not deserialize destination raster");
			PG_RETURN_NULL();
		}

		POSTGIS_RT_DEBUG(4, "destination raster isn't NULL");
	}

	/* source rasters' band index, 1-based */
	if (!PG_ARGISNULL(2))
		srcnband = PG_GETARG_INT32(2);
	if (srcnband < 1) {
		elog(NOTICE, "Invalid band index for source rasters (must be 1-based).  Returning original raster");
		if (raster != NULL) {
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}
		else
			PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(4, "srcnband = %d", srcnband);

	/* destination raster's band index, 1-based */
	if (!PG_ARGISNULL(3)) {
		dstnband = PG_GETARG_INT32(3);
		appendband = FALSE;

		if (dstnband < 1) {
			elog(NOTICE, "Invalid band index for destination raster (must be 1-based).  Returning original raster");
			if (raster != NULL) {
				rt_raster_destroy(raster);
				PG_RETURN_POINTER(pgraster);
			}
			else
				PG_RETURN_NULL();
		}
	}
	else
		appendband = TRUE;

	/* additional processing of dstnband */
	if (raster != NULL) {
		dstnumbands = rt_raster_get_num_bands(raster);

		if (dstnumbands < 1) {
			appendband = TRUE;
			dstnband = 1;
		}
		else if (appendband)
			dstnband = dstnumbands + 1;
		else if (dstnband > dstnumbands) {
			elog(NOTICE, "Band index provided for destination raster is greater than the number of bands in the raster.  Bands will be appended");
			appendband = TRUE;
			dstnband = dstnumbands + 1;
		}
	}
	POSTGIS_RT_DEBUGF(4, "appendband = %d", appendband);
	POSTGIS_RT_DEBUGF(4, "dstnband = %d", dstnband);

	/* process set of source rasters */
	POSTGIS_RT_DEBUG(3, "Processing array of source rasters");
	array = PG_GETARG_ARRAYTYPE_P(1);
	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
		&nulls, &n);

	/* decrement srcnband and dstnband by 1, now 0-based */
	srcnband--;
	dstnband--;
	POSTGIS_RT_DEBUGF(4, "0-based nband (src, dst) = (%d, %d)", srcnband, dstnband);

	/* time to copy bands */
	for (i = 0; i < n; i++) {
		if (nulls[i]) continue;
		src = NULL;

		pgsrc =	(rt_pgraster *) PG_DETOAST_DATUM(e[i]);
		src = rt_raster_deserialize(pgsrc, FALSE);
		if (src == NULL) {
			pfree(nulls);
			pfree(e);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandRasterArray: Could not deserialize source raster at index %d", i + 1);
			PG_RETURN_NULL();
		}

		srcnumbands = rt_raster_get_num_bands(src);
		POSTGIS_RT_DEBUGF(4, "source raster %d has %d bands", i + 1, srcnumbands);

		/* band index isn't valid */
		if (srcnband > srcnumbands - 1) {
			elog(NOTICE, "Invalid band index for source raster at index %d.  Returning original raster", i + 1);
			pfree(nulls);
			pfree(e);
			rt_raster_destroy(src);
			if (raster != NULL) {
				rt_raster_destroy(raster);
				PG_RETURN_POINTER(pgraster);
			}
			else
				PG_RETURN_NULL();
		}

		/* destination raster is empty, new raster */
		if (raster == NULL) {
			uint32_t srcnbands[1] = {srcnband};

			POSTGIS_RT_DEBUG(4, "empty destination raster, using rt_raster_from_band");

			raster = rt_raster_from_band(src, srcnbands, 1);
			rt_raster_destroy(src);
			if (raster == NULL) {
				pfree(nulls);
				pfree(e);
				if (pgraster != NULL)
					PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_addBandRasterArray: Could not create raster from source raster at index %d", i + 1);
				PG_RETURN_NULL();
			}
		}
		/* copy band */
		else {
			rtn = rt_raster_copy_band(
				raster, src,
				srcnband, dstnband
			);
			rt_raster_destroy(src);

			if (rtn == -1 || rt_raster_get_num_bands(raster) == dstnumbands) {
				elog(NOTICE, "Could not add band from source raster at index %d to destination raster.  Returning original raster", i + 1);
				rt_raster_destroy(raster);
				pfree(nulls);
				pfree(e);
				if (pgraster != NULL)
					PG_RETURN_POINTER(pgraster);
				else
					PG_RETURN_NULL();
			}
		}

		dstnband++;
		dstnumbands++;
	}

	if (raster != NULL) {
		pgrtn = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (pgraster != NULL)
			PG_FREE_IF_COPY(pgraster, 0);
		if (!pgrtn)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	PG_RETURN_NULL();
}

/**
 * Add out-db band to the given raster at the given position
 */
PG_FUNCTION_INFO_V1(RASTER_addBandOutDB);
Datum RASTER_addBandOutDB(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;

	rt_raster raster = NULL;
	rt_band band = NULL;
	int numbands = 0;
	int dstnband = 1; /* 1-based */
	int appendband = FALSE;
	char *outdbfile = NULL;
	int *srcnband = NULL; /* 1-based */
	int numsrcnband = 0;
	int allbands = FALSE;
	int hasnodata = FALSE;
	double nodataval = 0.;
	uint16_t width = 0;
	uint16_t height = 0;
	char *authname = NULL;
	char *authcode = NULL;

	int i = 0;
	int j = 0;

	GDALDatasetH hdsOut;
	GDALRasterBandH hbandOut;
	GDALDataType gdpixtype;

	rt_pixtype pt = PT_END;
	double gt[6] = {0.};
	double ogt[6] = {0.};
	rt_raster _rast = NULL;
	int aligned = 0;
	int err = 0;

	/* destination raster */
	if (!PG_ARGISNULL(0)) {
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		/* raster */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandOutDB: Could not deserialize destination raster");
			PG_RETURN_NULL();
		}

		POSTGIS_RT_DEBUG(4, "destination raster isn't NULL");
	}

	/* destination band index (1) */
	if (!PG_ARGISNULL(1))
		dstnband = PG_GETARG_INT32(1);
	else
		appendband = TRUE;

	/* outdb file (2) */
	if (PG_ARGISNULL(2)) {
		elog(NOTICE, "Out-db raster file not provided. Returning original raster");
		if (pgraster != NULL) {
			rt_raster_destroy(raster);
			PG_RETURN_POINTER(pgraster);
		}
		else
			PG_RETURN_NULL();
	}
	else {
		outdbfile = text_to_cstring(PG_GETARG_TEXT_P(2));
		if (!strlen(outdbfile)) {
			elog(NOTICE, "Out-db raster file not provided. Returning original raster");
			if (pgraster != NULL) {
				rt_raster_destroy(raster);
				PG_RETURN_POINTER(pgraster);
			}
			else
				PG_RETURN_NULL();
		}
	}

	/* outdb band index (3) */
	if (!PG_ARGISNULL(3)) {
		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;

		int16 typlen;
		bool typbyval;
		char typalign;

		allbands = FALSE;

		array = PG_GETARG_ARRAYTYPE_P(3);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case INT2OID:
			case INT4OID:
				break;
			default:
				if (pgraster != NULL) {
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
				}
				elog(ERROR, "RASTER_addBandOutDB: Invalid data type for band indexes");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e, &nulls, &numsrcnband);

		srcnband = palloc(sizeof(int) * numsrcnband);
		if (srcnband == NULL) {
			if (pgraster != NULL) {
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
			}
			elog(ERROR, "RASTER_addBandOutDB: Could not allocate memory for band indexes");
			PG_RETURN_NULL();
		}

		for (i = 0, j = 0; i < numsrcnband; i++) {
			if (nulls[i]) continue;

			switch (etype) {
				case INT2OID:
					srcnband[j] = DatumGetInt16(e[i]);
					break;
				case INT4OID:
					srcnband[j] = DatumGetInt32(e[i]);
					break;
			}
			j++;
		}

		if (j < numsrcnband) {
			srcnband = repalloc(srcnband, sizeof(int) * j);
			if (srcnband == NULL) {
				if (pgraster != NULL) {
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
				}
				elog(ERROR, "RASTER_addBandOutDB: Could not reallocate memory for band indexes");
				PG_RETURN_NULL();
			}

			numsrcnband = j;
		}
	}
	else
		allbands = TRUE;

	/* nodataval (4) */
	if (!PG_ARGISNULL(4)) {
		hasnodata = TRUE;
		nodataval = PG_GETARG_FLOAT8(4);
	}
	else
		hasnodata = FALSE;

	/* validate input */

	/* make sure dstnband is valid */
	if (raster != NULL) {
		numbands = rt_raster_get_num_bands(raster);
		if (!appendband) {
			if (dstnband < 1) {
				elog(NOTICE, "Invalid band index %d for adding bands. Using band index 1", dstnband);
				dstnband = 1;
			}
			else if (dstnband > numbands) {
				elog(NOTICE, "Invalid band index %d for adding bands. Using band index %d", dstnband, numbands);
				dstnband = numbands + 1; 
			}
		}
		else
			dstnband = numbands + 1; 
	}

	/* open outdb raster file */
	rt_util_gdal_register_all();
	hdsOut = GDALOpenShared(outdbfile, GA_ReadOnly);
	if (hdsOut == NULL) {
		if (pgraster != NULL) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
		}
		elog(ERROR, "RASTER_addBandOutDB: Could not open out-db file with GDAL");
		PG_RETURN_NULL();
	}

	/* get offline raster's geotransform */
	if (GDALGetGeoTransform(hdsOut, ogt) != CE_None) {
		ogt[0] = 0;
		ogt[1] = 1;
		ogt[2] = 0;
		ogt[3] = 0;
		ogt[4] = 0;
		ogt[5] = -1;
	}

	/* raster doesn't exist, create it now */
	if (raster == NULL) {
		raster = rt_raster_new(GDALGetRasterXSize(hdsOut), GDALGetRasterYSize(hdsOut));
		if (rt_raster_is_empty(raster)) {
			elog(ERROR, "RASTER_addBandOutDB: Could not create new raster");
			PG_RETURN_NULL();
		}
		rt_raster_set_geotransform_matrix(raster, ogt);
		rt_raster_get_geotransform_matrix(raster, gt);

		if (rt_util_gdal_sr_auth_info(hdsOut, &authname, &authcode) == ES_NONE) {
			if (
				authname != NULL &&
				strcmp(authname, "EPSG") == 0 &&
				authcode != NULL
			) {
				rt_raster_set_srid(raster, atoi(authcode));
			}
			else
				elog(INFO, "Unknown SRS auth name and code from out-db file. Defaulting SRID of new raster to %d", SRID_UNKNOWN);
		}
		else
			elog(INFO, "Could not get SRS auth name and code from out-db file. Defaulting SRID of new raster to %d", SRID_UNKNOWN);
	}

	/* some raster info */
	width = rt_raster_get_width(raster);
	height = rt_raster_get_width(raster);

	/* are rasters aligned? */
	_rast = rt_raster_new(1, 1);
	rt_raster_set_geotransform_matrix(_rast, ogt);
	rt_raster_set_srid(_rast, rt_raster_get_srid(raster));
	err = rt_raster_same_alignment(raster, _rast, &aligned, NULL);
	rt_raster_destroy(_rast);

	if (err != ES_NONE) {
		GDALClose(hdsOut);
		if (raster != NULL)
			rt_raster_destroy(raster);
		if (pgraster != NULL)
			PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_addBandOutDB: Could not test alignment of out-db file");
		return ES_ERROR;
	}
	else if (!aligned)
		elog(WARNING, "The in-db representation of the out-db raster is not aligned. Band data may be incorrect");

	numbands = GDALGetRasterCount(hdsOut);

	/* build up srcnband */
	if (allbands) {
		numsrcnband = numbands;
		srcnband = palloc(sizeof(int) * numsrcnband);
		if (srcnband == NULL) {
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandOutDB: Could not allocate memory for band indexes");
			PG_RETURN_NULL();
		}

		for (i = 0, j = 1; i < numsrcnband; i++, j++)
			srcnband[i] = j;
	}

	/* check band properties and add band */
	for (i = 0, j = dstnband - 1; i < numsrcnband; i++, j++) {
		/* valid index? */
		if (srcnband[i] < 1 || srcnband[i] > numbands) {
			elog(NOTICE, "Out-db file does not have a band at index %d. Returning original raster", srcnband[i]);
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_RETURN_POINTER(pgraster);
			else
				PG_RETURN_NULL();
		}

		/* get outdb band */
		hbandOut = NULL;
		hbandOut = GDALGetRasterBand(hdsOut, srcnband[i]);
		if (NULL == hbandOut) {
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandOutDB: Could not get band %d from GDAL dataset", srcnband[i]);
			PG_RETURN_NULL();
		}

		/* supported pixel type */
		gdpixtype = GDALGetRasterDataType(hbandOut);
		pt = rt_util_gdal_datatype_to_pixtype(gdpixtype);
		if (pt == PT_END) {
			elog(NOTICE, "Pixel type %s of band %d from GDAL dataset is not supported. Returning original raster", GDALGetDataTypeName(gdpixtype), srcnband[i]);
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_RETURN_POINTER(pgraster);
			else
				PG_RETURN_NULL();
		}

		/* use out-db band's nodata value if nodataval not already set */
		if (!hasnodata)
			nodataval = GDALGetRasterNoDataValue(hbandOut, &hasnodata);

		/* add band */
		band = rt_band_new_offline(
			width, height,
			pt,
			hasnodata, nodataval,
			srcnband[i] - 1, outdbfile
		);
		if (band == NULL) {
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandOutDB: Could not create new out-db band");
			PG_RETURN_NULL();
		}

		if (rt_raster_add_band(raster, band, j) < 0) {
			GDALClose(hdsOut);
			if (raster != NULL)
				rt_raster_destroy(raster);
			if (pgraster != NULL)
				PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_addBandOutDB: Could not add new out-db band to raster");
			PG_RETURN_NULL();
		}
	}

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (pgraster != NULL)
		PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Break up a raster into smaller tiles. SRF function
 */
PG_FUNCTION_INFO_V1(RASTER_tile);
Datum RASTER_tile(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	int call_cntr;
	int max_calls;
	int i = 0;
	int j = 0;

	struct tile_arg_t {

		struct {
			rt_raster raster;
			double gt[6];
			int srid;
			int width;
			int height;
		} raster;

		struct {
			int width;
			int height;

			int nx;
			int ny;
		} tile;

		int numbands;
		int *nbands;

		struct {
			int pad;
			double hasnodata;
			double nodataval;
		} pad;
	};
	struct tile_arg_t *arg1 = NULL;
	struct tile_arg_t *arg2 = NULL;

	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;
		rt_pgraster *pgraster = NULL;
		int numbands;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;

		int16 typlen;
		bool typbyval;
		char typalign;

		POSTGIS_RT_DEBUG(2, "RASTER_tile: first call");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Get input arguments */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* allocate arg1 */
		arg1 = palloc(sizeof(struct tile_arg_t));
		if (arg1 == NULL) {
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_tile: Could not allocate memory for arguments");
			SRF_RETURN_DONE(funcctx);
		}

		pgraster = (rt_pgraster *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		arg1->raster.raster = rt_raster_deserialize(pgraster, FALSE);
		if (!arg1->raster.raster) {
			ereport(ERROR, (
				errcode(ERRCODE_OUT_OF_MEMORY),
				errmsg("Could not deserialize raster")
			));
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* raster has bands */
		numbands = rt_raster_get_num_bands(arg1->raster.raster); 
		/*
		if (!numbands) {
			elog(NOTICE, "Raster provided has no bands");
			rt_raster_destroy(arg1->raster.raster);
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		*/

		/* width (1) */
		if (PG_ARGISNULL(1)) {
			elog(NOTICE, "Width cannot be NULL. Returning NULL");
			rt_raster_destroy(arg1->raster.raster);
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		arg1->tile.width = PG_GETARG_INT32(1);
		if (arg1->tile.width < 1) {
			elog(NOTICE, "Width must be greater than zero. Returning NULL");
			rt_raster_destroy(arg1->raster.raster);
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* height (2) */
		if (PG_ARGISNULL(2)) {
			elog(NOTICE, "Height cannot be NULL. Returning NULL");
			rt_raster_destroy(arg1->raster.raster);
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		arg1->tile.height = PG_GETARG_INT32(2);
		if (arg1->tile.height < 1) {
			elog(NOTICE, "Height must be greater than zero. Returning NULL");
			rt_raster_destroy(arg1->raster.raster);
			pfree(arg1);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* nband, array (3) */
		if (numbands && !PG_ARGISNULL(3)) {
			array = PG_GETARG_ARRAYTYPE_P(3);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case INT2OID:
				case INT4OID:
					break;
				default:
					rt_raster_destroy(arg1->raster.raster);
					pfree(arg1);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_tile: Invalid data type for band indexes");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			deconstruct_array(array, etype, typlen, typbyval, typalign, &e, &nulls, &(arg1->numbands));

			arg1->nbands = palloc(sizeof(int) * arg1->numbands);
			if (arg1->nbands == NULL) {
				rt_raster_destroy(arg1->raster.raster);
				pfree(arg1);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_tile: Could not allocate memory for band indexes");
				SRF_RETURN_DONE(funcctx);
			}

			for (i = 0, j = 0; i < arg1->numbands; i++) {
				if (nulls[i]) continue;

				switch (etype) {
					case INT2OID:
						arg1->nbands[j] = DatumGetInt16(e[i]) - 1;
						break;
					case INT4OID:
						arg1->nbands[j] = DatumGetInt32(e[i]) - 1;
						break;
				}

				j++;
			}

			if (j < arg1->numbands) {
				arg1->nbands = repalloc(arg1->nbands, sizeof(int) * j);
				if (arg1->nbands == NULL) {
					rt_raster_destroy(arg1->raster.raster);
					pfree(arg1);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_tile: Could not reallocate memory for band indexes");
					SRF_RETURN_DONE(funcctx);
				}

				arg1->numbands = j;
			}

			/* validate nbands */
			for (i = 0; i < arg1->numbands; i++) {
				if (!rt_raster_has_band(arg1->raster.raster, arg1->nbands[i])) {
					elog(NOTICE, "Band at index %d not found in raster", arg1->nbands[i] + 1);
					rt_raster_destroy(arg1->raster.raster);
					pfree(arg1->nbands);
					pfree(arg1);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					SRF_RETURN_DONE(funcctx);
				}
			}
		}
		else {
			arg1->numbands = numbands;

			if (numbands) {
				arg1->nbands = palloc(sizeof(int) * arg1->numbands);

				if (arg1->nbands == NULL) {
					rt_raster_destroy(arg1->raster.raster);
					pfree(arg1);
					PG_FREE_IF_COPY(pgraster, 0);
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_dumpValues: Could not allocate memory for pixel values");
					SRF_RETURN_DONE(funcctx);
				}

				for (i = 0; i < arg1->numbands; i++) {
					arg1->nbands[i] = i;
					POSTGIS_RT_DEBUGF(4, "arg1->nbands[%d] = %d", arg1->nbands[i], i);
				}
			}
		}

		/* pad (4) and padnodata (5) */
		if (!PG_ARGISNULL(4)) {
			arg1->pad.pad = PG_GETARG_BOOL(4) ? 1 : 0;

			if (arg1->pad.pad && !PG_ARGISNULL(5)) {
				arg1->pad.hasnodata = 1;
				arg1->pad.nodataval = PG_GETARG_FLOAT8(5);
			}
			else {
				arg1->pad.hasnodata = 0;
				arg1->pad.nodataval = 0;
			}
		}
		else {
			arg1->pad.pad = 0;
			arg1->pad.hasnodata = 0;
			arg1->pad.nodataval = 0;
		}

		/* store some additional metadata */
		arg1->raster.srid = rt_raster_get_srid(arg1->raster.raster);
		arg1->raster.width = rt_raster_get_width(arg1->raster.raster);
		arg1->raster.height = rt_raster_get_height(arg1->raster.raster);
		rt_raster_get_geotransform_matrix(arg1->raster.raster, arg1->raster.gt);

		/* determine maximum number of tiles from raster */
		arg1->tile.nx = ceil(arg1->raster.width / (double) arg1->tile.width);
		arg1->tile.ny = ceil(arg1->raster.height / (double) arg1->tile.height);
		POSTGIS_RT_DEBUGF(4, "# of tiles (x, y) = (%d, %d)", arg1->tile.nx, arg1->tile.ny);

		/* Store needed information */
		funcctx->user_fctx = arg1;

		/* total number of tuples to be returned */
		funcctx->max_calls = (arg1->tile.nx * arg1->tile.ny);

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	arg2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		rt_pgraster *pgtile = NULL;
		rt_raster tile = NULL;
		rt_band _band = NULL;
		rt_band band = NULL;
		rt_pixtype pixtype = PT_END;
		int hasnodata = 0;
		double nodataval = 0;
		int width = 0;
		int height = 0;

		int k = 0;
		int tx = 0;
		int ty = 0;
		int rx = 0;
		int ry = 0;
		int ex = 0; /* edge tile on right */
		int ey = 0; /* edge tile on bottom */
		double ulx = 0;
		double uly = 0;
		uint16_t len = 0;
		void *vals = NULL;
		uint16_t nvals;

		POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);

		/*
			find offset based upon tile #

			0 1 2
			3 4 5
			6 7 8
		*/
		ty = call_cntr / arg2->tile.nx;
		tx = call_cntr % arg2->tile.nx;
		POSTGIS_RT_DEBUGF(4, "tile (x, y) = (%d, %d)", tx, ty);

		/* edge tile? only important if padding is false */
		if (!arg2->pad.pad) {
			if (ty + 1 == arg2->tile.ny)
				ey = 1;
			if (tx + 1 == arg2->tile.nx)
				ex = 1;
		}

		/* upper-left of tile in raster coordinates */
		rx = tx * arg2->tile.width;
		ry = ty * arg2->tile.height;
		POSTGIS_RT_DEBUGF(4, "raster coordinates = %d, %d", rx, ry);

		/* determine tile width and height */
		/* default to user-defined */
		width = arg2->tile.width;
		height = arg2->tile.height;

		/* override user-defined if edge tile (only possible if padding is false */
		if (ex || ey) {
			/* right edge */
			if (ex)
				width = arg2->raster.width - rx;
			/* bottom edge */
			if (ey)
				height = arg2->raster.height - ry;
		}

		/* create empty raster */
		tile = rt_raster_new(width, height);
		rt_raster_set_geotransform_matrix(tile, arg2->raster.gt);
		rt_raster_set_srid(tile, arg2->raster.srid);

		/* upper-left of tile in spatial coordinates */
		if (rt_raster_cell_to_geopoint(arg2->raster.raster, rx, ry, &ulx, &uly, arg2->raster.gt) != ES_NONE) {
			rt_raster_destroy(tile);
			rt_raster_destroy(arg2->raster.raster);
			if (arg2->numbands) pfree(arg2->nbands);
			pfree(arg2);
			elog(ERROR, "RASTER_tile: Could not compute the coordinates of the upper-left corner of the output tile");
			SRF_RETURN_DONE(funcctx);
		}
		rt_raster_set_offsets(tile, ulx, uly);
		POSTGIS_RT_DEBUGF(4, "spatial coordinates = %f, %f", ulx, uly);

		/* compute length of pixel line to read */
		len = arg2->tile.width;
		if (rx + arg2->tile.width >= arg2->raster.width)
			len = arg2->raster.width - rx;
		POSTGIS_RT_DEBUGF(3, "read line len = %d", len);

		/* copy bands to tile */
		for (i = 0; i < arg2->numbands; i++) {
			POSTGIS_RT_DEBUGF(4, "copying band %d to tile %d", arg2->nbands[i], call_cntr);

			_band = rt_raster_get_band(arg2->raster.raster, arg2->nbands[i]);
			if (_band == NULL) {
				int nband = arg2->nbands[i] + 1;
				rt_raster_destroy(tile);
				rt_raster_destroy(arg2->raster.raster);
				if (arg2->numbands) pfree(arg2->nbands);
				pfree(arg2);
				elog(ERROR, "RASTER_tile: Could not get band %d from source raster", nband);
				SRF_RETURN_DONE(funcctx);
			}

			pixtype = rt_band_get_pixtype(_band);
			hasnodata = rt_band_get_hasnodata_flag(_band);
			if (hasnodata)
				rt_band_get_nodata(_band, &nodataval);
			else if (arg2->pad.pad && arg2->pad.hasnodata) {
				hasnodata = 1;
				nodataval = arg2->pad.nodataval;
			}
			else
				nodataval = rt_band_get_min_value(_band);

			/* inline band */
			if (!rt_band_is_offline(_band)) {
				if (rt_raster_generate_new_band(tile, pixtype, nodataval, hasnodata, nodataval, i) < 0) {
					rt_raster_destroy(tile);
					rt_raster_destroy(arg2->raster.raster);
					pfree(arg2->nbands);
					pfree(arg2);
					elog(ERROR, "RASTER_tile: Could not add new band to output tile");
					SRF_RETURN_DONE(funcctx);
				}
				band = rt_raster_get_band(tile, i);
				if (band == NULL) {
					rt_raster_destroy(tile);
					rt_raster_destroy(arg2->raster.raster);
					if (arg2->numbands) pfree(arg2->nbands);
					pfree(arg2);
					elog(ERROR, "RASTER_tile: Could not get newly added band from output tile");
					SRF_RETURN_DONE(funcctx);
				}

				/* if isnodata, set flag and continue */
				if (rt_band_get_isnodata_flag(_band)) {
					rt_band_set_isnodata_flag(band, 1);
					continue;
				}

				/* copy data */
				for (j = 0; j < arg2->tile.height; j++) {
					k = ry + j;

					if (k >= arg2->raster.height) {
						POSTGIS_RT_DEBUGF(4, "row %d is beyond extent of source raster. skipping", k);
						continue;
					}

					POSTGIS_RT_DEBUGF(4, "getting pixel line %d, %d for %d pixels", rx, k, len);
					if (rt_band_get_pixel_line(_band, rx, k, len, &vals, &nvals) != ES_NONE) {
						rt_raster_destroy(tile);
						rt_raster_destroy(arg2->raster.raster);
						if (arg2->numbands) pfree(arg2->nbands);
						pfree(arg2);
						elog(ERROR, "RASTER_tile: Could not get pixel line from source raster");
						SRF_RETURN_DONE(funcctx);
					}

					if (nvals && rt_band_set_pixel_line(band, 0, j, vals, nvals) != ES_NONE) {
						rt_raster_destroy(tile);
						rt_raster_destroy(arg2->raster.raster);
						if (arg2->numbands) pfree(arg2->nbands);
						pfree(arg2);
						elog(ERROR, "RASTER_tile: Could not set pixel line of output tile");
						SRF_RETURN_DONE(funcctx);
					}
				}
			}
			/* offline */
			else {
				uint8_t bandnum = 0;
				rt_band_get_ext_band_num(_band, &bandnum);

				band = rt_band_new_offline(
					width, height,
					pixtype,
					hasnodata, nodataval,
					bandnum, rt_band_get_ext_path(_band)
				);

				if (band == NULL) {
					rt_raster_destroy(tile);
					rt_raster_destroy(arg2->raster.raster);
					if (arg2->numbands) pfree(arg2->nbands);
					pfree(arg2);
					elog(ERROR, "RASTER_tile: Could not create new offline band for output tile");
					SRF_RETURN_DONE(funcctx);
				}

				if (rt_raster_add_band(tile, band, i) < 0) {
					rt_band_destroy(band);
					rt_raster_destroy(tile);
					rt_raster_destroy(arg2->raster.raster);
					if (arg2->numbands) pfree(arg2->nbands);
					pfree(arg2);
					elog(ERROR, "RASTER_tile: Could not add new offline band to output tile");
					SRF_RETURN_DONE(funcctx);
				}
			}
		}

		pgtile = rt_raster_serialize(tile);
		rt_raster_destroy(tile);
		if (!pgtile) {
			rt_raster_destroy(arg2->raster.raster);
			if (arg2->numbands) pfree(arg2->nbands);
			pfree(arg2);
			SRF_RETURN_DONE(funcctx);
		}

		SET_VARSIZE(pgtile, pgtile->size);
		SRF_RETURN_NEXT(funcctx, PointerGetDatum(pgtile));
	}
	/* do when there is no more left */
	else {
		rt_raster_destroy(arg2->raster.raster);
		if (arg2->numbands) pfree(arg2->nbands);
		pfree(arg2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Copy a band from one raster to another one at the given position.
 */
PG_FUNCTION_INFO_V1(RASTER_copyBand);
Datum RASTER_copyBand(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgto = NULL;
	rt_pgraster *pgfrom = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster torast = NULL;
	rt_raster fromrast = NULL;
	int toindex = 0;
	int fromband = 0;
	int oldtorastnumbands = 0;
	int newtorastnumbands = 0;
	int newbandindex = 0;

	/* Deserialize torast */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgto = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	torast = rt_raster_deserialize(pgto, FALSE);
	if (!torast) {
		PG_FREE_IF_COPY(pgto, 0);
		elog(ERROR, "RASTER_copyBand: Could not deserialize first raster");
		PG_RETURN_NULL();
	}

	/* Deserialize fromrast */
	if (!PG_ARGISNULL(1)) {
		pgfrom = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		fromrast = rt_raster_deserialize(pgfrom, FALSE);
		if (!fromrast) {
			rt_raster_destroy(torast);
			PG_FREE_IF_COPY(pgfrom, 1);
			PG_FREE_IF_COPY(pgto, 0);
			elog(ERROR, "RASTER_copyBand: Could not deserialize second raster");
			PG_RETURN_NULL();
		}

		oldtorastnumbands = rt_raster_get_num_bands(torast);

		if (PG_ARGISNULL(2))
			fromband = 1;
		else
			fromband = PG_GETARG_INT32(2);

		if (PG_ARGISNULL(3))
			toindex = oldtorastnumbands + 1;
		else
			toindex = PG_GETARG_INT32(3);

		/* Copy band fromrast torast */
		newbandindex = rt_raster_copy_band(
			torast, fromrast,
			fromband - 1, toindex - 1
		);

		newtorastnumbands = rt_raster_get_num_bands(torast);
		if (newtorastnumbands == oldtorastnumbands || newbandindex == -1) {
			elog(NOTICE, "RASTER_copyBand: Could not add band to raster. "
				"Returning original raster."
			);
		}

		rt_raster_destroy(fromrast);
		PG_FREE_IF_COPY(pgfrom, 1);
	}

	/* Serialize and return torast */
	pgrtn = rt_raster_serialize(torast);
	rt_raster_destroy(torast);
	PG_FREE_IF_COPY(pgto, 0);
	if (!pgrtn) PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * Check if raster is empty or not
 */
PG_FUNCTION_INFO_V1(RASTER_isEmpty);
Datum RASTER_isEmpty(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    bool isempty = FALSE;

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster )
    {
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("RASTER_isEmpty: Could not deserialize raster")));
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    isempty = rt_raster_is_empty(raster);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_BOOL(isempty);
}

/**
 * Check if the raster has a given band or not
 */
PG_FUNCTION_INFO_V1(RASTER_hasNoBand);
Datum RASTER_hasNoBand(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    int bandindex = 0;
    bool hasnoband = FALSE;

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if ( ! raster )
    {
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("RASTER_hasNoBand: Could not deserialize raster")));
        PG_FREE_IF_COPY(pgraster, 0);
        PG_RETURN_NULL();
    }

    /* Get band number */
    bandindex = PG_GETARG_INT32(1);
    hasnoband = !rt_raster_has_band(raster, bandindex - 1);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_BOOL(hasnoband);
}

PG_FUNCTION_INFO_V1(RASTER_mapAlgebraExpr);
Datum RASTER_mapAlgebraExpr(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_pgraster *pgrtn = NULL;
    rt_raster raster = NULL;
    rt_raster newrast = NULL;
    rt_band band = NULL;
    rt_band newband = NULL;
    int x, y, nband, width, height;
    double r;
    double newnodatavalue = 0.0;
    double newinitialvalue = 0.0;
    double newval = 0.0;
    char *newexpr = NULL;
    char *initexpr = NULL;
    char *expression = NULL;
    int hasnodataval = 0;
    double nodataval = 0.;
    rt_pixtype newpixeltype;
    int skipcomputation = 0;
    int len = 0;
    const int argkwcount = 3;
    enum KEYWORDS { kVAL=0, kX=1, kY=2 };
    char *argkw[] = {"[rast]", "[rast.x]", "[rast.y]"};
    Oid argkwtypes[] = { FLOAT8OID, INT4OID, INT4OID };
    int argcount = 0;
    Oid argtype[] = { FLOAT8OID, INT4OID, INT4OID };
    uint8_t argpos[3] = {0};
    char place[5];
    int idx = 0;
    int ret = -1;
    TupleDesc tupdesc;
    SPIPlanPtr spi_plan = NULL;
    SPITupleTable * tuptable = NULL;
    HeapTuple tuple;
    char * strFromText = NULL;
    Datum *values = NULL;
    Datum datum = (Datum)NULL;
    char *nulls = NULL;
    bool isnull = FALSE;
    int i = 0;
    int j = 0;

    POSTGIS_RT_DEBUG(2, "RASTER_mapAlgebraExpr: Starting...");

    /* Check raster */
    if (PG_ARGISNULL(0)) {
        elog(NOTICE, "Raster is NULL. Returning NULL");
        PG_RETURN_NULL();
    }


    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (NULL == raster) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_mapAlgebraExpr: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Getting arguments...");

    if (PG_ARGISNULL(1))
        nband = 1;
    else
        nband = PG_GETARG_INT32(1);

    if (nband < 1)
        nband = 1;


    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Creating new empty raster...");

    /**
     * Create a new empty raster with having the same georeference as the
     * provided raster
     **/
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);

    newrast = rt_raster_new(width, height);

    if ( NULL == newrast ) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_mapAlgebraExpr: Could not create a new raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(newrast,
            rt_raster_get_x_scale(raster),
            rt_raster_get_y_scale(raster));

    rt_raster_set_offsets(newrast,
            rt_raster_get_x_offset(raster),
            rt_raster_get_y_offset(raster));

    rt_raster_set_skews(newrast,
            rt_raster_get_x_skew(raster),
            rt_raster_get_y_skew(raster));

    rt_raster_set_srid(newrast, rt_raster_get_srid(raster));


    /**
     * If this new raster is empty (width = 0 OR height = 0) then there is
     * nothing to compute and we return it right now
     **/
    if (rt_raster_is_empty(newrast))
    {
        elog(NOTICE, "Raster is empty. Returning an empty raster");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }


    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (!rt_raster_has_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
                "without a band");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
                "without a band");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

     /*
     * Get NODATA value
     */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        rt_band_get_nodata(band, &newnodatavalue);
    }

    else {
        newnodatavalue = rt_band_get_min_value(band);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: NODATA value for band: = %f",
        newnodatavalue);

    /**
     * We set the initial value of the future band to nodata value. If nodata
     * value is null, then the raster will be initialized to
     * rt_band_get_min_value but all the values should be recomputed anyway
     **/
    newinitialvalue = newnodatavalue;

    /**
     * Set the new pixeltype
     **/
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Setting pixeltype...");

    if (PG_ARGISNULL(2)) {
        newpixeltype = rt_band_get_pixtype(band);
    }

    else {
        strFromText = text_to_cstring(PG_GETARG_TEXT_P(2));
        newpixeltype = rt_pixtype_index_from_name(strFromText);
        pfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }

    if (newpixeltype == PT_END) {
        PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_mapAlgebraExpr: Invalid pixeltype");
        PG_RETURN_NULL();
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Pixeltype set to %s",
        rt_pixtype_name(newpixeltype));


    /* Construct expression for raster values */
    if (!PG_ARGISNULL(3)) {
        expression = text_to_cstring(PG_GETARG_TEXT_P(3));
        len = strlen("SELECT (") + strlen(expression) + strlen(")::double precision");
        initexpr = (char *)palloc(len + 1);

        strncpy(initexpr, "SELECT (", strlen("SELECT ("));
        strncpy(initexpr + strlen("SELECT ("), expression, strlen(expression));
				strncpy(initexpr + strlen("SELECT (") + strlen(expression), ")::double precision", strlen(")::double precision"));
        initexpr[len] = '\0';

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Expression is %s", initexpr);

        /* We don't need this memory */
        /*
				pfree(expression);
        expression = NULL;
				*/
    }



    /**
     * Optimization: If a nodataval is provided, use it for newinitialvalue.
     * Then, we can initialize the raster with this value and skip the
     * computation of nodata values one by one in the main computing loop
     **/
    if (!PG_ARGISNULL(4)) {
				hasnodataval = 1;
				nodataval = PG_GETARG_FLOAT8(4);
				newinitialvalue = nodataval;

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: new initial value = %f",
            newinitialvalue);
    }
    else
        hasnodataval = 0;



    /**
     * Optimization: If the raster is only filled with nodata values return
     * right now a raster filled with the newinitialvalue
     * TODO: Call rt_band_check_isnodata instead?
     **/
    if (rt_band_get_isnodata_flag(band)) {

        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Band is a nodata band, returning "
                "a raster filled with nodata");

        ret = rt_raster_generate_new_band(newrast, newpixeltype,
                newinitialvalue, TRUE, newnodatavalue, 0);

        /* Free memory */
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }


    /**
     * Optimization: If expression resume to 'RAST' and hasnodataval is zero,
		 * we can just return the band from the original raster
     **/
    if (initexpr != NULL && ( !strcmp(initexpr, "SELECT [rast]") || !strcmp(initexpr, "SELECT [rast.val]") ) && !hasnodataval) {

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Expression resumes to RAST. "
                "Returning raster with band %d from original raster", nband);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebraExpr: New raster has %d bands",
                rt_raster_get_num_bands(newrast));

        rt_raster_copy_band(newrast, raster, nband - 1, 0);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebraExpr: New raster now has %d bands",
                rt_raster_get_num_bands(newrast));

        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /**
     * Optimization: If expression resume to a constant (it does not contain
     * [rast)
     **/
    if (initexpr != NULL && strstr(initexpr, "[rast") == NULL) {
        ret = SPI_connect();
        if (ret != SPI_OK_CONNECT) {
            PG_FREE_IF_COPY(pgraster, 0);
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not connect to the SPI manager");
            PG_RETURN_NULL();
        };

        /* Execute the expresion into newval */
        ret = SPI_execute(initexpr, FALSE, 0);

        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {

            /* Free memory allocated out of the current context */
            if (SPI_tuptable)
                SPI_freetuptable(tuptable);
            PG_FREE_IF_COPY(pgraster, 0);

            SPI_finish();
            elog(ERROR, "RASTER_mapAlgebraExpr: Invalid construction for expression");
            PG_RETURN_NULL();
        }

        tupdesc = SPI_tuptable->tupdesc;
        tuptable = SPI_tuptable;

        tuple = tuptable->vals[0];
        newexpr = SPI_getvalue(tuple, tupdesc, 1);
        if ( ! newexpr ) {
            POSTGIS_RT_DEBUG(3, "Constant expression evaluated to NULL, keeping initvalue");
            newval = newinitialvalue;
        } else {
            newval = atof(newexpr);
        }

        SPI_freetuptable(tuptable);

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: New raster value = %f",
                newval);

        SPI_finish();

        skipcomputation = 1;

        /**
         * Compute the new value, set it and we will return after creating the
         * new raster
         **/
        if (!hasnodataval) {
            newinitialvalue = newval;
            skipcomputation = 2;
        }

        /* Return the new raster as it will be before computing pixel by pixel */
        else if (FLT_NEQ(newval, newinitialvalue)) {
            skipcomputation = 2;
        }
    }

    /**
     * Create the raster receiving all the computed values. Initialize it to the
     * new initial value
     **/
    ret = rt_raster_generate_new_band(newrast, newpixeltype,
            newinitialvalue, TRUE, newnodatavalue, 0);

    /**
     * Optimization: If expression is NULL, or all the pixels could be set in
     * one step, return the initialized raster now
     **/
    /*if (initexpr == NULL || skipcomputation == 2) {*/
    if (expression == NULL || skipcomputation == 2) {

        /* Free memory */
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    RASTER_DEBUG(3, "RASTER_mapAlgebraExpr: Creating new raster band...");

    /* Get the new raster band */
    newband = rt_raster_get_band(newrast, 0);
    if ( NULL == newband ) {
        elog(NOTICE, "Could not modify band for new raster. Returning new "
                "raster with the original band");

        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Main computing loop (%d x %d)",
            width, height);

    if (initexpr != NULL) {
    	/* Convert [rast.val] to [rast] */
        newexpr = rtpg_strreplace(initexpr, "[rast.val]", "[rast]", NULL);
        pfree(initexpr); initexpr=newexpr;

        sprintf(place,"$1");
        for (i = 0, j = 1; i < argkwcount; i++) {
            len = 0;
            newexpr = rtpg_strreplace(initexpr, argkw[i], place, &len);
            pfree(initexpr); initexpr=newexpr;
            if (len > 0) {
                argtype[argcount] = argkwtypes[i];
                argcount++;
                argpos[i] = j++;

                sprintf(place, "$%d", j);
            }
            else {
                argpos[i] = 0;
            }
        }

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: initexpr = %s", initexpr);

        /* define values */
        values = (Datum *) palloc(sizeof(Datum) * argcount);
        if (values == NULL) {

            SPI_finish();

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not allocate memory for value parameters of prepared statement");
            PG_RETURN_NULL();
        }

        /* define nulls */
        nulls = (char *)palloc(argcount);
        if (nulls == NULL) {

            SPI_finish();

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not allocate memory for null parameters of prepared statement");
            PG_RETURN_NULL();
        }

        /* Connect to SPI and prepare the expression */
        ret = SPI_connect();
        if (ret != SPI_OK_CONNECT) {

            if (initexpr)
                pfree(initexpr);
            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not connect to the SPI manager");
            PG_RETURN_NULL();
        };

        /* Type of all arguments is FLOAT8OID */
        spi_plan = SPI_prepare(initexpr, argcount, argtype);

        if (spi_plan == NULL) {

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            SPI_finish();

            pfree(initexpr);

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not prepare expression");
            PG_RETURN_NULL();
        }
    }

    for (x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            ret = rt_band_get_pixel(band, x, y, &r, NULL);

            /**
             * We compute a value only for the withdata value pixel since the
             * nodata value has already been set by the first optimization
             **/
            if (ret == ES_NONE && FLT_NEQ(r, newnodatavalue)) {
                if (skipcomputation == 0) {
                    if (initexpr != NULL) {
                        /* Reset the null arg flags. */
                        memset(nulls, 'n', argcount);

                        for (i = 0; i < argkwcount; i++) {
                            idx = argpos[i];
                            if (idx < 1) continue;
                            idx--;

                            if (i == kX ) {
                                /* x is 0 based index, but SQL expects 1 based index */
                                values[idx] = Int32GetDatum(x+1);
                                nulls[idx] = ' ';
                            }
                            else if (i == kY) {
                                /* y is 0 based index, but SQL expects 1 based index */
                                values[idx] = Int32GetDatum(y+1);
                                nulls[idx] = ' ';
                            }
                            else if (i == kVAL ) {
                                values[idx] = Float8GetDatum(r);
                                nulls[idx] = ' ';
                            }

                        }

                        ret = SPI_execute_plan(spi_plan, values, nulls, FALSE, 0);
                        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL ||
                                SPI_processed != 1) {
                            if (SPI_tuptable)
                                SPI_freetuptable(tuptable);

                            SPI_freeplan(spi_plan);
                            SPI_finish();

                            pfree(values);
                            pfree(nulls);
                            pfree(initexpr);

                            rt_raster_destroy(raster);
                            PG_FREE_IF_COPY(pgraster, 0);
                            rt_raster_destroy(newrast);

                            elog(ERROR, "RASTER_mapAlgebraExpr: Error executing prepared plan");

                            PG_RETURN_NULL();
                        }

                        tupdesc = SPI_tuptable->tupdesc;
                        tuptable = SPI_tuptable;

                        tuple = tuptable->vals[0];
                        datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
                        if ( SPI_result == SPI_ERROR_NOATTRIBUTE ) {
                            POSTGIS_RT_DEBUGF(3, "Expression for pixel %d,%d (value %g) errored, skip setting", x+1,y+1,r);
                            newval = newinitialvalue;
                        }
                        else if ( isnull ) {
                            POSTGIS_RT_DEBUGF(3, "Expression for pixel %d,%d (value %g) evaluated to NULL, skip setting", x+1,y+1,r);
                            newval = newinitialvalue;
                        } else {
                            newval = DatumGetFloat8(datum);
                        }

                        SPI_freetuptable(tuptable);
                    }

                    else
                        newval = newinitialvalue;

                    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: new value = %f",
                        newval);
                }


                rt_band_set_pixel(newband, x, y, newval, NULL);
            }

        }
    }

    if (initexpr != NULL) {
        SPI_freeplan(spi_plan);
        SPI_finish();

        pfree(values);
        pfree(nulls);
        pfree(initexpr);
    }
    else {
        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: no SPI cleanup");
    }


    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: raster modified, serializing it.");
    /* Serialize created raster */

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    pgrtn = rt_raster_serialize(newrast);
    rt_raster_destroy(newrast);
    if (NULL == pgrtn)
        PG_RETURN_NULL();

    SET_VARSIZE(pgrtn, pgrtn->size);

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: raster serialized");


    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraExpr: returning raster");


    PG_RETURN_POINTER(pgrtn);
}

/*
 * One raster user function MapAlgebra.
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebraFct);
Datum RASTER_mapAlgebraFct(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_pgraster *pgrtn = NULL;
    rt_raster raster = NULL;
    rt_raster newrast = NULL;
    rt_band band = NULL;
    rt_band newband = NULL;
    int x, y, nband, width, height;
    double r;
    double newnodatavalue = 0.0;
    double newinitialvalue = 0.0;
    double newval = 0.0;
    rt_pixtype newpixeltype;
    int ret = -1;
    Oid oid;
    FmgrInfo cbinfo;
    FunctionCallInfoData cbdata;
    Datum tmpnewval;
    char * strFromText = NULL;
    int k = 0;

    POSTGIS_RT_DEBUG(2, "RASTER_mapAlgebraFct: STARTING...");

    /* Check raster */
    if (PG_ARGISNULL(0)) {
        elog(WARNING, "Raster is NULL. Returning NULL");
        PG_RETURN_NULL();
    }


    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (NULL == raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_mapAlgebraFct: Could not deserialize raster");
			PG_RETURN_NULL();    
    }

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Getting arguments...");

    /* Get the rest of the arguments */

    if (PG_ARGISNULL(1))
        nband = 1;
    else
        nband = PG_GETARG_INT32(1);

    if (nband < 1)
        nband = 1;
    
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Creating new empty raster...");

    /** 
     * Create a new empty raster with having the same georeference as the
     * provided raster
     **/
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);

    newrast = rt_raster_new(width, height);

    if ( NULL == newrast ) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        elog(ERROR, "RASTER_mapAlgebraFct: Could not create a new raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(newrast, 
            rt_raster_get_x_scale(raster),
            rt_raster_get_y_scale(raster));

    rt_raster_set_offsets(newrast,
            rt_raster_get_x_offset(raster),
            rt_raster_get_y_offset(raster));

    rt_raster_set_skews(newrast,
            rt_raster_get_x_skew(raster),
            rt_raster_get_y_skew(raster));

    rt_raster_set_srid(newrast, rt_raster_get_srid(raster));            


    /**
     * If this new raster is empty (width = 0 OR height = 0) then there is
     * nothing to compute and we return it right now
     **/
    if (rt_raster_is_empty(newrast)) 
    { 
        elog(NOTICE, "Raster is empty. Returning an empty raster");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (!rt_raster_has_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /*
    * Get NODATA value
    */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        rt_band_get_nodata(band, &newnodatavalue);
    }

    else {
        newnodatavalue = rt_band_get_min_value(band);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: NODATA value for band: %f",
            newnodatavalue);
    /**
     * We set the initial value of the future band to nodata value. If nodata
     * value is null, then the raster will be initialized to
     * rt_band_get_min_value but all the values should be recomputed anyway
     **/
    newinitialvalue = newnodatavalue;

    /**
     * Set the new pixeltype
     **/    
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Setting pixeltype...");

    if (PG_ARGISNULL(2)) {
        newpixeltype = rt_band_get_pixtype(band);
    }

    else {
        strFromText = text_to_cstring(PG_GETARG_TEXT_P(2)); 
        newpixeltype = rt_pixtype_index_from_name(strFromText);
        pfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }
    
    if (newpixeltype == PT_END) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFct: Invalid pixeltype");
        PG_RETURN_NULL();
    }    
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Pixeltype set to %s",
        rt_pixtype_name(newpixeltype));

    /* Get the name of the callback user function for raster values */
    if (PG_ARGISNULL(3)) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFct: Required function is missing. Returning NULL");
        PG_RETURN_NULL();
    }

    oid = PG_GETARG_OID(3);
    if (oid == InvalidOid) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFct: Got invalid function object id. Returning NULL");
        PG_RETURN_NULL();
    }

    fmgr_info(oid, &cbinfo);

    /* function cannot return set */
    if (cbinfo.fn_retset) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFct: Function provided must return double precision not resultset");
        PG_RETURN_NULL();
    }
    /* function should have correct # of args */
    else if (cbinfo.fn_nargs < 2 || cbinfo.fn_nargs > 3) {

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFct: Function does not have two or three input parameters");
        PG_RETURN_NULL();
    }

    if (cbinfo.fn_nargs == 2)
        k = 1;
    else 
        k = 2;

    if (func_volatile(oid) == 'v') {
        elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");
    }

    /* prep function call data */
#if POSTGIS_PGSQL_VERSION <= 90
    InitFunctionCallInfoData(cbdata, &cbinfo, 2, InvalidOid, NULL);
#else
    InitFunctionCallInfoData(cbdata, &cbinfo, 2, InvalidOid, NULL, NULL);
#endif
    memset(cbdata.argnull, FALSE, sizeof(bool) * cbinfo.fn_nargs);
    
    /* check that the function isn't strict if the args are null. */
    if (PG_ARGISNULL(4)) {
        if (cbinfo.fn_strict) {

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            elog(ERROR, "RASTER_mapAlgebraFct: Strict callback functions cannot have null parameters");
            PG_RETURN_NULL();
        }

        cbdata.arg[k] = (Datum)NULL;
        cbdata.argnull[k] = TRUE;
    }
    else {
        cbdata.arg[k] = PG_GETARG_DATUM(4);
    }

    /**
     * Optimization: If the raster is only filled with nodata values return
     * right now a raster filled with the nodatavalueexpr
     * TODO: Call rt_band_check_isnodata instead?
     **/
    if (rt_band_get_isnodata_flag(band)) {

        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Band is a nodata band, returning "
                "a raster filled with nodata");

        ret = rt_raster_generate_new_band(newrast, newpixeltype,
                newinitialvalue, TRUE, newnodatavalue, 0);

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);               
    }


    /**
     * Create the raster receiving all the computed values. Initialize it to the
     * new initial value
     **/
    ret = rt_raster_generate_new_band(newrast, newpixeltype,
            newinitialvalue, TRUE, newnodatavalue, 0);

    /* Get the new raster band */
    newband = rt_raster_get_band(newrast, 0);
    if ( NULL == newband ) {
        elog(NOTICE, "Could not modify band for new raster. Returning new "
            "raster with the original band");

        rt_raster_destroy(raster);
        PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);      
    }
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Main computing loop (%d x %d)",
            width, height);

    for (x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            ret = rt_band_get_pixel(band, x, y, &r, NULL);

            /**
             * We compute a value only for the withdata value pixel since the
             * nodata value has already been set by the first optimization
             **/
            if (ret == ES_NONE) {
                if (FLT_EQ(r, newnodatavalue)) {
                    if (cbinfo.fn_strict) {
                        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Strict callbacks cannot accept NULL arguments, skipping NODATA cell.");
                        continue;
                    }
                    cbdata.argnull[0] = TRUE;
                    cbdata.arg[0] = (Datum)NULL;
                }
                else {
                    cbdata.argnull[0] = FALSE;
                    cbdata.arg[0] = Float8GetDatum(r);
                }

                /* Add pixel positions if callback has proper # of args */
                if (cbinfo.fn_nargs == 3) {
                    Datum d[2];
                    ArrayType *a;

                    d[0] = Int32GetDatum(x+1);
                    d[1] = Int32GetDatum(y+1);

                    a = construct_array(d, 2, INT4OID, sizeof(int32), true, 'i');

                    cbdata.argnull[1] = FALSE;
                    cbdata.arg[1] = PointerGetDatum(a);
                }

                POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: (%dx%d), r = %f",
                    x, y, r);
                   
                tmpnewval = FunctionCallInvoke(&cbdata);

                if (cbdata.isnull) {
                    newval = newnodatavalue;
                }
                else {
                    newval = DatumGetFloat8(tmpnewval);
                }

                POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: new value = %f", 
                    newval);
                
                rt_band_set_pixel(newband, x, y, newval, NULL);
            }

        }
    }
    
    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: raster modified, serializing it.");
    /* Serialize created raster */

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    pgrtn = rt_raster_serialize(newrast);
    rt_raster_destroy(newrast);
    if (NULL == pgrtn)
        PG_RETURN_NULL();

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: raster serialized");

    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraFct: returning raster");
    
    SET_VARSIZE(pgrtn, pgrtn->size);    
    PG_RETURN_POINTER(pgrtn);
}

/**
 * Return new raster from selected bands of existing raster through ST_Band.
 * second argument is an array of band numbers (1 based)
 */
PG_FUNCTION_INFO_V1(RASTER_band);
Datum RASTER_band(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster;
	rt_pgraster *pgrast;
	rt_raster raster;
	rt_raster rast;

	bool skip = FALSE;
	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;

	uint32_t numBands;
	uint32_t *bandNums;
	uint32 idx = 0;
	int n;
	int i = 0;
	int j = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_band: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* process bandNums */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Band number(s) not provided.  Returning original raster");
		skip = TRUE;
	}
	do {
		if (skip) break;

		numBands = rt_raster_get_num_bands(raster);

		array = PG_GETARG_ARRAYTYPE_P(1);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case INT2OID:
			case INT4OID:
				break;
			default:
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_band: Invalid data type for band number(s)");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		bandNums = palloc(sizeof(uint32_t) * n);
		for (i = 0, j = 0; i < n; i++) {
			if (nulls[i]) continue;

			switch (etype) {
				case INT2OID:
					idx = (uint32_t) DatumGetInt16(e[i]);
					break;
				case INT4OID:
					idx = (uint32_t) DatumGetInt32(e[i]);
					break;
			}

			POSTGIS_RT_DEBUGF(3, "band idx (before): %d", idx);
			if (idx > numBands || idx < 1) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning original raster");
				skip = TRUE;
				break;
			}

			bandNums[j] = idx - 1;
			POSTGIS_RT_DEBUGF(3, "bandNums[%d] = %d", j, bandNums[j]);
			j++;
		}

		if (skip || j < 1) {
			pfree(bandNums);
			skip = TRUE;
		}
	}
	while (0);

	if (!skip) {
		rast = rt_raster_from_band(raster, bandNums, j);
		pfree(bandNums);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (!rast) {
			elog(ERROR, "RASTER_band: Could not create new raster");
			PG_RETURN_NULL();
		}

		pgrast = rt_raster_serialize(rast);
		rt_raster_destroy(rast);

		if (!pgrast)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrast, pgrast->size);
		PG_RETURN_POINTER(pgrast);
	}

	PG_RETURN_POINTER(pgraster);
}

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
	int values_length = 6;
	Datum values[values_length];
	bool nulls[values_length];
	HeapTuple tuple;
	Datum result;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_summaryStats: Could not deserialize raster");
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
		elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
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
		elog(NOTICE, "Could not compute summary statistics for band at index %d. Returning NULL", bandindex);
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

	memset(nulls, FALSE, sizeof(bool) * values_length);

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

	int values_length = 6;
	Datum values[values_length];
	bool nulls[values_length];
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
		elog(ERROR, "RASTER_summaryStatsCoverage: Could not connect to database using SPI");
		PG_RETURN_NULL();
	}

	/* create sql */
	len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
	sql = (char *) palloc(len);
	if (NULL == sql) {
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		elog(ERROR, "RASTER_summaryStatsCoverage: Could not allocate memory for sql");
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
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
		if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			elog(ERROR, "RASTER_summaryStatsCoverage: Could not get raster of coverage");
			PG_RETURN_NULL();
		}
		else if (isNull) {
			SPI_cursor_fetch(portal, TRUE, 1);
			continue;
		}

		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			elog(ERROR, "RASTER_summaryStatsCoverage: Could not deserialize raster");
			PG_RETURN_NULL();
		}

		/* inspect number of bands */
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

			rt_raster_destroy(raster);

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);
			PG_RETURN_NULL();
		}

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

			rt_raster_destroy(raster);

			if (SPI_tuptable) SPI_freetuptable(tuptable);
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
			elog(NOTICE, "Could not compute summary statistics for band at index %d. Returning NULL", bandindex);

			if (SPI_tuptable) SPI_freetuptable(tuptable);
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

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					elog(ERROR, "RASTER_summaryStatsCoverage: Could not allocate memory for summary stats of coverage");
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

	if (SPI_tuptable) SPI_freetuptable(tuptable);
	SPI_cursor_close(portal);
	SPI_finish();

	if (NULL == rtn) {
		elog(ERROR, "RASTER_summaryStatsCoverage: Could not compute coverage summary stats");
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

	memset(nulls, FALSE, sizeof(bool) * values_length);

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
			elog(ERROR, "RASTER_histogram: Could not deserialize raster");
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
			elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
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
			elog(NOTICE, "Could not compute summary statistics for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Could not compute histogram for band at index %d as the band has no values", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get histogram */
		hist = rt_band_get_histogram(stats, bin_count, bin_width, bin_width_count, right, min, max, &count);
		if (bin_width_count) pfree(bin_width);
		pfree(stats);
		if (NULL == hist || !count) {
			elog(NOTICE, "Could not compute histogram for band at index %d", bandindex);
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
		int values_length = 4;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

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

/**
 * Returns histogram of a coverage for a specified band
 */
PG_FUNCTION_INFO_V1(RASTER_histogramCoverage);
Datum RASTER_histogramCoverage(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	int i;
	rt_histogram covhist = NULL;
	rt_histogram covhist2;
	int call_cntr;
	int max_calls;

	POSTGIS_RT_DEBUG(3, "RASTER_histogramCoverage: Starting");

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		text *tablenametext = NULL;
		char *tablename = NULL;
		text *colnametext = NULL;
		char *colname = NULL;
		int32_t bandindex = 1;
		bool exclude_nodata_value = TRUE;
		double sample = 0;
		uint32_t bin_count = 0;
		double *bin_width = NULL;
		uint32_t bin_width_count = 0;
		double width = 0;
		bool right = FALSE;
		uint32_t count;

		int len = 0;
		char *sql = NULL;
		char *tmp = NULL;
		double min = 0;
		double max = 0;
		int spi_result;
		Portal portal;
		SPITupleTable *tuptable = NULL;
		HeapTuple tuple;
		Datum datum;
		bool isNull = FALSE;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int num_bands = 0;
		rt_bandstats stats = NULL;
		rt_histogram hist;
		uint64_t sum = 0;

		int j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		POSTGIS_RT_DEBUG(3, "RASTER_histogramCoverage: first call of function");

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
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: tablename = %s", tablename);

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
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: colname = %s", colname);

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
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}
			else if (FLT_EQ(sample, 0.0))
				sample = 1;
		}
		else
			sample = 1;

		/* bin_count */
		if (!PG_ARGISNULL(5)) {
			bin_count = PG_GETARG_INT32(5);
			if (bin_count < 1) bin_count = 0;
		}

		/* bin_width */
		if (!PG_ARGISNULL(6)) {
			array = PG_GETARG_ARRAYTYPE_P(6);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_histogramCoverage: Invalid data type for width");
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
		if (!PG_ARGISNULL(7))
			right = PG_GETARG_BOOL(7);

		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not connect to database using SPI");
			SRF_RETURN_DONE(funcctx);
		}

		/* coverage stats */
		len = sizeof(char) * (strlen("SELECT min, max FROM _st_summarystats('','',,::boolean,)") + strlen(tablename) + strlen(colname) + (MAX_INT_CHARLEN * 2) + MAX_DBL_CHARLEN + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not allocate memory for sql");
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		snprintf(sql, len, "SELECT min, max FROM _st_summarystats('%s','%s',%d,%d::boolean,%f)", tablename, colname, bandindex, (exclude_nodata_value ? 1 : 0), sample);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: %s", sql);
		spi_result = SPI_execute(sql, TRUE, 0);
		pfree(sql);
		if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");
			SRF_RETURN_DONE(funcctx);
		}

		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		tmp = SPI_getvalue(tuple, tupdesc, 1);
		if (NULL == tmp || !strlen(tmp)) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");
			SRF_RETURN_DONE(funcctx);
		}
		min = strtod(tmp, NULL);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: min = %f", min);
		pfree(tmp);

		tmp = SPI_getvalue(tuple, tupdesc, 2);
		if (NULL == tmp || !strlen(tmp)) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");
			SRF_RETURN_DONE(funcctx);
		}
		max = strtod(tmp, NULL);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: max = %f", max);
		pfree(tmp);

		/* iterate through rasters of coverage */
		/* create sql */
		len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_histogramCoverage: Could not allocate memory for sql");
			SRF_RETURN_DONE(funcctx);
		}

		/* get cursor */
		snprintf(sql, len, "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL", colname, tablename, colname);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: %s", sql);
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
			tuptable = SPI_tuptable;
			tuple = tuptable->vals[0];

			datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
			if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) pfree(covhist);
				if (bin_width_count) pfree(bin_width);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_histogramCoverage: Could not get raster of coverage");
				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) pfree(covhist);
				if (bin_width_count) pfree(bin_width);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_histogramCoverage: Could not deserialize raster");
				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) pfree(covhist);
				if (bin_width_count) pfree(bin_width);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) pfree(covhist);
				if (bin_width_count) pfree(bin_width);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* we need the raw values, hence the non-zero parameter */
			stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);

			rt_band_destroy(band);
			rt_raster_destroy(raster);

			if (NULL == stats) {
				elog(NOTICE, "Could not compute summary statistics for band at index %d. Returning NULL", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) pfree(covhist);
				if (bin_width_count) pfree(bin_width);

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* get histogram */
			if (stats->count > 0) {
				hist = rt_band_get_histogram(stats, bin_count, bin_width, bin_width_count, right, min, max, &count);
				pfree(stats);
				if (NULL == hist || !count) {
					elog(NOTICE, "Could not compute histogram for band at index %d", bandindex);

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					if (NULL != covhist) pfree(covhist);
					if (bin_width_count) pfree(bin_width);

					MemoryContextSwitchTo(oldcontext);
					SRF_RETURN_DONE(funcctx);
				}

				POSTGIS_RT_DEBUGF(3, "%d bins returned", count);

				/* coverage histogram */
				if (NULL == covhist) {
					covhist = (rt_histogram) SPI_palloc(sizeof(struct rt_histogram_t) * count);
					if (NULL == covhist) {

						pfree(hist);
						if (SPI_tuptable) SPI_freetuptable(tuptable);
						SPI_cursor_close(portal);
						SPI_finish();

						if (bin_width_count) pfree(bin_width);

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_histogramCoverage: Could not allocate memory for histogram of coverage");
						SRF_RETURN_DONE(funcctx);
					}

					for (i = 0; i < count; i++) {
						sum += hist[i].count;
						covhist[i].count = hist[i].count;
						covhist[i].percent = 0;
						covhist[i].min = hist[i].min;
						covhist[i].max = hist[i].max;
					}
				}
				else {
					for (i = 0; i < count; i++) {
						sum += hist[i].count;
						covhist[i].count += hist[i].count;
					}
				}

				pfree(hist);

				/* assuming bin_count wasn't set, force consistency */
				if (bin_count <= 0) bin_count = count;
			}

			/* next record */
			SPI_cursor_fetch(portal, TRUE, 1);
		}

		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_cursor_close(portal);
		SPI_finish();

		if (bin_width_count) pfree(bin_width);

		/* finish percent of histogram */
		if (sum > 0) {
			for (i = 0; i < count; i++)
				covhist[i].percent = covhist[i].count / (double) sum;
		}

		/* Store needed information */
		funcctx->user_fctx = covhist;

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
	covhist2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 4;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		values[0] = Float8GetDatum(covhist2[call_cntr].min);
		values[1] = Float8GetDatum(covhist2[call_cntr].max);
		values[2] = Int64GetDatum(covhist2[call_cntr].count);
		values[3] = Float8GetDatum(covhist2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(covhist2);
		SRF_RETURN_DONE(funcctx);
	}
}

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
			elog(ERROR, "RASTER_quantile: Could not deserialize raster");
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
			elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
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
			elog(NOTICE, "Could not retrieve summary statistics for band at index %d", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Could not compute quantiles for band at index %d as the band has no values", bandindex);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* get quantiles */
		quant = rt_band_get_quantiles(stats, quantiles, quantiles_count, &count);
		if (quantiles_count) pfree(quantiles);
		pfree(stats);
		if (NULL == quant || !count) {
			elog(NOTICE, "Could not compute quantiles for band at index %d", bandindex);
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
		int values_length = 2;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

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

/**
 * Returns selected quantiles of a coverage for a specified band
 */
PG_FUNCTION_INFO_V1(RASTER_quantileCoverage);
Datum RASTER_quantileCoverage(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	int i;
	rt_quantile covquant = NULL;
	rt_quantile covquant2;
	int call_cntr;
	int max_calls;

	POSTGIS_RT_DEBUG(3, "RASTER_quantileCoverage: Starting");

	/* first call of function */
	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		text *tablenametext = NULL;
		char *tablename = NULL;
		text *colnametext = NULL;
		char *colname = NULL;
		int32_t bandindex = 1;
		bool exclude_nodata_value = TRUE;
		double sample = 0;
		double *quantiles = NULL;
		uint32_t quantiles_count = 0;
		double quantile = 0;
		uint32_t count;

		int len = 0;
		char *sql = NULL;
		char *tmp = NULL;
		uint64_t cov_count = 0;
		int spi_result;
		Portal portal;
		SPITupleTable *tuptable = NULL;
		HeapTuple tuple;
		Datum datum;
		bool isNull = FALSE;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;
		int num_bands = 0;
		struct quantile_llist *qlls = NULL;
		uint32_t qlls_count;

		int j;
		int n;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;

		POSTGIS_RT_DEBUG(3, "RASTER_quantileCoverage: first call of function");

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
		POSTGIS_RT_DEBUGF(3, "RASTER_quantileCoverage: tablename = %s", tablename);

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
		POSTGIS_RT_DEBUGF(3, "RASTER_quantileCoverage: colname = %s", colname);

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
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}
			else if (FLT_EQ(sample, 0.0))
				sample = 1;
		}
		else
			sample = 1;

		/* quantiles */
		if (!PG_ARGISNULL(5)) {
			array = PG_GETARG_ARRAYTYPE_P(5);
			etype = ARR_ELEMTYPE(array);
			get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

			switch (etype) {
				case FLOAT4OID:
				case FLOAT8OID:
					break;
				default:
					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_quantileCoverage: Invalid data type for quantiles");
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

		/* coverage stats */
		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantileCoverage: Could not connect to database using SPI");
			SRF_RETURN_DONE(funcctx);
		}

		len = sizeof(char) * (strlen("SELECT count FROM _st_summarystats('','',,::boolean,)") + strlen(tablename) + strlen(colname) + (MAX_INT_CHARLEN * 2) + MAX_DBL_CHARLEN + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantileCoverage: Could not allocate memory for sql");
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		snprintf(sql, len, "SELECT count FROM _st_summarystats('%s','%s',%d,%d::boolean,%f)", tablename, colname, bandindex, (exclude_nodata_value ? 1 : 0), sample);
		POSTGIS_RT_DEBUGF(3, "stats sql:  %s", sql);
		spi_result = SPI_execute(sql, TRUE, 0);
		pfree(sql);
		if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantileCoverage: Could not get summary stats of coverage");
			SRF_RETURN_DONE(funcctx);
		}

		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		tmp = SPI_getvalue(tuple, tupdesc, 1);
		if (NULL == tmp || !strlen(tmp)) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantileCoverage: Could not get summary stats of coverage");
			SRF_RETURN_DONE(funcctx);
		}
		cov_count = strtol(tmp, NULL, 10);
		POSTGIS_RT_DEBUGF(3, "covcount = %d", (int) cov_count);
		pfree(tmp);

		/* iterate through rasters of coverage */
		/* create sql */
		len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_quantileCoverage: Could not allocate memory for sql");
			SRF_RETURN_DONE(funcctx);
		}

		/* get cursor */
		snprintf(sql, len, "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" IS NOT NULL", colname, tablename, colname);
		POSTGIS_RT_DEBUGF(3, "coverage sql: %s", sql);
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
			if (NULL != covquant) pfree(covquant);

			tupdesc = SPI_tuptable->tupdesc;
			tuptable = SPI_tuptable;
			tuple = tuptable->vals[0];

			datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
			if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_quantileCoverage: Could not get raster of coverage");
				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_quantileCoverage: Could not deserialize raster");
				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			covquant = rt_band_get_quantiles_stream(
				band,
				exclude_nodata_value, sample, cov_count,
				&qlls, &qlls_count,
				quantiles, quantiles_count,
				&count
			);

			rt_band_destroy(band);
			rt_raster_destroy(raster);

			if (NULL == covquant || !count) {
				elog(NOTICE, "Could not compute quantiles for band at index %d", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* next record */
			SPI_cursor_fetch(portal, TRUE, 1);
		}

		covquant2 = SPI_palloc(sizeof(struct rt_quantile_t) * count);
		for (i = 0; i < count; i++) {
			covquant2[i].quantile = covquant[i].quantile;
			covquant2[i].has_value = covquant[i].has_value;
			if (covquant2[i].has_value)
				covquant2[i].value = covquant[i].value;
		}

		if (NULL != covquant) pfree(covquant);
		quantile_llist_destroy(&qlls, qlls_count);

		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_cursor_close(portal);
		SPI_finish();

		if (quantiles_count) pfree(quantiles);

		POSTGIS_RT_DEBUGF(3, "%d quantiles returned", count);

		/* Store needed information */
		funcctx->user_fctx = covquant2;

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
	covquant2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 2;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		values[0] = Float8GetDatum(covquant2[call_cntr].quantile);
		if (covquant2[call_cntr].has_value)
			values[1] = Float8GetDatum(covquant2[call_cntr].value);
		else
			nulls[1] = TRUE;

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		POSTGIS_RT_DEBUG(3, "done");
		pfree(covquant2);
		SRF_RETURN_DONE(funcctx);
	}
}

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
			elog(ERROR, "RASTER_valueCount: Could not deserialize raster");
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
			elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);
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
			elog(NOTICE, "Could not count the values for band at index %d", bandindex);
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
		int values_length = 3;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

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

	int i;
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
		SPITupleTable *tuptable = NULL;
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
			elog(ERROR, "RASTER_valueCountCoverage: Could not connect to database using SPI");
			SRF_RETURN_DONE(funcctx);
		}

		/* create sql */
		len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (search_values_count) pfree(search_values);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_valueCountCoverage: Could not allocate memory for sql");
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
			tuptable = SPI_tuptable;
			tuple = tuptable->vals[0];

			datum = SPI_getbinval(tuple, tupdesc, 1, &isNull);
			if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_valueCountCoverage: Could not get raster of coverage");
				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) pfree(covvcnts);
				if (search_values_count) pfree(search_values);

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_valueCountCoverage: Could not deserialize raster");
				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
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
				elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

				rt_raster_destroy(raster);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
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
				elog(NOTICE, "Could not count the values for band at index %d", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
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

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					if (search_values_count) pfree(search_values);

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_valueCountCoverage: Could not allocate memory for value counts of coverage");
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
						if (NULL == covvcnts) {

							if (SPI_tuptable) SPI_freetuptable(tuptable);
							SPI_cursor_close(portal);
							SPI_finish();

							if (search_values_count) pfree(search_values);
							if (NULL != covvcnts) free(covvcnts);

							MemoryContextSwitchTo(oldcontext);
							elog(ERROR, "RASTER_valueCountCoverage: Could not change allocated memory for value counts of coverage");
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

		if (SPI_tuptable) SPI_freetuptable(tuptable);
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
		int values_length = 3;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

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

/**
 * Reclassify the specified bands of the raster
 */
PG_FUNCTION_INFO_V1(RASTER_reclass);
Datum RASTER_reclass(PG_FUNCTION_ARGS) {
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrtn = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	rt_band newband = NULL;
	uint32_t numBands = 0;

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
	int k = 0;

	int a = 0;
	int b = 0;
	int c = 0;

	rt_reclassexpr *exprset = NULL;
	HeapTupleHeader tup;
	bool isnull;
	Datum tupv;
	uint32_t nband = 0;
	char *expr = NULL;
	text *exprtext = NULL;
	double val = 0;
	char *junk = NULL;
	int inc_val = 0;
	int exc_val = 0;
	char *pixeltype = NULL;
	text *pixeltypetext = NULL;
	rt_pixtype pixtype = PT_END;
	double nodataval = 0;
	bool hasnodata = FALSE;

	char **comma_set = NULL;
	int comma_n = 0;
	char **colon_set = NULL;
	int colon_n = 0;
	char **dash_set = NULL;
	int dash_n = 0;

	POSTGIS_RT_DEBUG(3, "RASTER_reclass: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_reclass: Could not deserialize raster");
		PG_RETURN_NULL();
	}
	numBands = rt_raster_get_num_bands(raster);
	POSTGIS_RT_DEBUGF(3, "RASTER_reclass: %d possible bands to be reclassified", n);

	/* process set of reclassarg */
	POSTGIS_RT_DEBUG(3, "RASTER_reclass: Processing Arg 1 (reclassargset)");
	array = PG_GETARG_ARRAYTYPE_P(1);
	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
		&nulls, &n);

	if (!n) {
		elog(NOTICE, "Invalid argument for reclassargset. Returning original raster");

		pgrtn = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		if (!pgrtn)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	/*
		process each element of reclassarg
		each element is the index of the band to process, the set
			of reclass ranges and the output band's pixeltype
	*/
	for (i = 0; i < n; i++) {
		if (nulls[i]) continue;

		/* each element is a tuple */
		tup = (HeapTupleHeader) DatumGetPointer(e[i]);
		if (NULL == tup) {
			elog(NOTICE, "Invalid argument for reclassargset. Returning original raster");

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}

		/* band index (1-based) */
		tupv = GetAttributeByName(tup, "nband", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of nband for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		nband = DatumGetInt32(tupv);
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: expression for band %d", nband);

		/* valid band index? */
		if (nband < 1 || nband > numBands) {
			elog(NOTICE, "Invalid argument for reclassargset. Invalid band index (must use 1-based) for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}

		/* reclass expr */
		tupv = GetAttributeByName(tup, "reclassexpr", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		exprtext = (text *) DatumGetPointer(tupv);
		if (NULL == exprtext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		expr = text_to_cstring(exprtext);
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: expr (raw) %s", expr);
		expr = rtpg_removespaces(expr);
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: expr (clean) %s", expr);

		/* split string to its components */
		/* comma (,) separating rangesets */
		comma_set = rtpg_strsplit(expr, ",", &comma_n);
		if (comma_n < 1) {
			elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}

		/* set of reclass expressions */
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: %d possible expressions", comma_n);
		exprset = palloc(comma_n * sizeof(rt_reclassexpr));

		for (a = 0, j = 0; a < comma_n; a++) {
			POSTGIS_RT_DEBUGF(5, "RASTER_reclass: map %s", comma_set[a]);

			/* colon (:) separating range "src" and "dst" */
			colon_set = rtpg_strsplit(comma_set[a], ":", &colon_n);
			if (colon_n != 2) {
				elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
				for (k = 0; k < j; k++) pfree(exprset[k]);
				pfree(exprset);

				pgrtn = rt_raster_serialize(raster);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				if (!pgrtn)
					PG_RETURN_NULL();

				SET_VARSIZE(pgrtn, pgrtn->size);
				PG_RETURN_POINTER(pgrtn);
			}

			/* allocate mem for reclass expression */
			exprset[j] = palloc(sizeof(struct rt_reclassexpr_t));

			for (b = 0; b < colon_n; b++) {
				POSTGIS_RT_DEBUGF(5, "RASTER_reclass: range %s", colon_set[b]);

				/* dash (-) separating "min" and "max" */
				dash_set = rtpg_strsplit(colon_set[b], "-", &dash_n);
				if (dash_n < 1 || dash_n > 3) {
					elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
					for (k = 0; k < j; k++) pfree(exprset[k]);
					pfree(exprset);

					pgrtn = rt_raster_serialize(raster);
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 0);
					if (!pgrtn)
						PG_RETURN_NULL();

					SET_VARSIZE(pgrtn, pgrtn->size);
					PG_RETURN_POINTER(pgrtn);
				}

				for (c = 0; c < dash_n; c++) {
					/* need to handle: (-9999-100 -> "(", "9999", "100" */
					if (
						c < 1 && 
						strlen(dash_set[c]) == 1 && (
							strchr(dash_set[c], '(') != NULL ||
							strchr(dash_set[c], '[') != NULL ||
							strchr(dash_set[c], ')') != NULL ||
							strchr(dash_set[c], ']') != NULL
						)
					) {
						junk = palloc(sizeof(char) * (strlen(dash_set[c + 1]) + 2));
						if (NULL == junk) {
							for (k = 0; k <= j; k++) pfree(exprset[k]);
							pfree(exprset);
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 0);

							elog(ERROR, "RASTER_reclass: Could not allocate memory");
							PG_RETURN_NULL();
						}

						sprintf(junk, "%s%s", dash_set[c], dash_set[c + 1]);
						c++;
						dash_set[c] = repalloc(dash_set[c], sizeof(char) * (strlen(junk) + 1));
						strcpy(dash_set[c], junk);
						pfree(junk);

						/* rebuild dash_set */
						for (k = 1; k < dash_n; k++) {
							dash_set[k - 1] = repalloc(dash_set[k - 1], (strlen(dash_set[k]) + 1) * sizeof(char));
							strcpy(dash_set[k - 1], dash_set[k]);
						}
						dash_n--;
						c--;
						pfree(dash_set[dash_n]);
						dash_set = repalloc(dash_set, sizeof(char *) * dash_n);
					}

					/* there shouldn't be more than two in dash_n */
					if (c < 1 && dash_n > 2) {
						elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
						for (k = 0; k < j; k++) pfree(exprset[k]);
						pfree(exprset);

						pgrtn = rt_raster_serialize(raster);
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 0);
						if (!pgrtn)
							PG_RETURN_NULL();

						SET_VARSIZE(pgrtn, pgrtn->size);
						PG_RETURN_POINTER(pgrtn);
					}

					/* check interval flags */
					exc_val = 0;
					inc_val = 1;
					/* range */
					if (dash_n != 1) {
						/* min */
						if (c < 1) {
							if (
								strchr(dash_set[c], ')') != NULL ||
								strchr(dash_set[c], ']') != NULL
							) {
								exc_val = 1;
								inc_val = 1;
							}
							else if (strchr(dash_set[c], '(') != NULL){
								inc_val = 0;
							}
							else {
								inc_val = 1;
							}
						}
						/* max */
						else {
							if (
								strrchr(dash_set[c], '(') != NULL ||
								strrchr(dash_set[c], '[') != NULL
							) {
								exc_val = 1;
								inc_val = 0;
							}
							else if (strrchr(dash_set[c], ']') != NULL) {
								inc_val = 1;
							}
							else {
								inc_val = 0;
							}
						}
					}
					POSTGIS_RT_DEBUGF(5, "RASTER_reclass: exc_val %d inc_val %d", exc_val, inc_val);

					/* remove interval flags */
					dash_set[c] = rtpg_chartrim(dash_set[c], "()[]");
					POSTGIS_RT_DEBUGF(5, "RASTER_reclass: min/max (char) %s", dash_set[c]);

					/* value from string to double */
					errno = 0;
					val = strtod(dash_set[c], &junk);
					if (errno != 0 || dash_set[c] == junk) {
						elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
						for (k = 0; k < j; k++) pfree(exprset[k]);
						pfree(exprset);

						pgrtn = rt_raster_serialize(raster);
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 0);
						if (!pgrtn)
							PG_RETURN_NULL();

						SET_VARSIZE(pgrtn, pgrtn->size);
						PG_RETURN_POINTER(pgrtn);
					}
					POSTGIS_RT_DEBUGF(5, "RASTER_reclass: min/max (double) %f", val);

					/* strsplit removes dash (a.k.a. negative sign), compare now to restore */
					junk = strstr(colon_set[b], dash_set[c]);
					/* not beginning of string */
					if (junk != colon_set[b]) {
						/* prior is a dash */
						if (*(junk - 1) == '-') {
							/* prior is beginning of string or prior - 1 char is dash, negative number */
							if (
								((junk - 1) == colon_set[b]) ||
								(*(junk - 2) == '-') ||
								(*(junk - 2) == '[') ||
								(*(junk - 2) == '(')
							) {
								val *= -1.;
							}
						}
					}
					POSTGIS_RT_DEBUGF(5, "RASTER_reclass: min/max (double) %f", val);

					/* src */
					if (b < 1) {
						/* singular value */
						if (dash_n == 1) {
							exprset[j]->src.exc_min = exprset[j]->src.exc_max = exc_val;
							exprset[j]->src.inc_min = exprset[j]->src.inc_max = inc_val;
							exprset[j]->src.min = exprset[j]->src.max = val;
						}
						/* min */
						else if (c < 1) {
							exprset[j]->src.exc_min = exc_val;
							exprset[j]->src.inc_min = inc_val;
							exprset[j]->src.min = val;
						}
						/* max */
						else {
							exprset[j]->src.exc_max = exc_val;
							exprset[j]->src.inc_max = inc_val;
							exprset[j]->src.max = val;
						}
					}
					/* dst */
					else {
						/* singular value */
						if (dash_n == 1)
							exprset[j]->dst.min = exprset[j]->dst.max = val;
						/* min */
						else if (c < 1)
							exprset[j]->dst.min = val;
						/* max */
						else
							exprset[j]->dst.max = val;
					}
				}
				pfree(dash_set);
			}
			pfree(colon_set);

			POSTGIS_RT_DEBUGF(3, "RASTER_reclass: or: %f - %f nr: %f - %f"
				, exprset[j]->src.min
				, exprset[j]->src.max
				, exprset[j]->dst.min
				, exprset[j]->dst.max
			);
			j++;
		}
		pfree(comma_set);

		/* pixel type */
		tupv = GetAttributeByName(tup, "pixeltype", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of pixeltype for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		pixeltypetext = (text *) DatumGetPointer(tupv);
		if (NULL == pixeltypetext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of pixeltype for reclassarg of index %d . Returning original raster", i);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		pixeltype = text_to_cstring(pixeltypetext);
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: pixeltype %s", pixeltype);
		pixtype = rt_pixtype_index_from_name(pixeltype);

		/* nodata */
		tupv = GetAttributeByName(tup, "nodataval", &isnull);
		if (isnull) {
			nodataval = 0;
			hasnodata = FALSE;
		}
		else {
			nodataval = DatumGetFloat8(tupv);
			hasnodata = TRUE;
		}
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: nodataval %f", nodataval);
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: hasnodata %d", hasnodata);

		/* do reclass */
		band = rt_raster_get_band(raster, nband - 1);
		if (!band) {
			elog(NOTICE, "Could not find raster band of index %d. Returning original raster", nband);
			for (k = 0; k < j; k++) pfree(exprset[k]);
			pfree(exprset);

			pgrtn = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			if (!pgrtn)
				PG_RETURN_NULL();

			SET_VARSIZE(pgrtn, pgrtn->size);
			PG_RETURN_POINTER(pgrtn);
		}
		newband = rt_band_reclass(band, pixtype, hasnodata, nodataval, exprset, j);
		if (!newband) {
			for (k = 0; k < j; k++) pfree(exprset[k]);
			pfree(exprset);

			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);

			elog(ERROR, "RASTER_reclass: Could not reclassify raster band of index %d", nband);
			PG_RETURN_NULL();
		}

		/* replace old band with new band */
		if (rt_raster_replace_band(raster, newband, nband - 1) == NULL) {
			for (k = 0; k < j; k++) pfree(exprset[k]);
			pfree(exprset);

			rt_band_destroy(newband);
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);

			elog(ERROR, "RASTER_reclass: Could not replace raster band of index %d with reclassified band", nband);
			PG_RETURN_NULL();
		}

		/* old band is in the variable band */
		rt_band_destroy(band);

		/* free exprset */
		for (k = 0; k < j; k++) pfree(exprset[k]);
		pfree(exprset);
	}

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);
	if (!pgrtn)
		PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_reclass: Finished");

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/* ---------------------------------------------------------------- */
/* apply colormap to specified band of a raster                     */
/* ---------------------------------------------------------------- */

typedef struct rtpg_colormap_arg_t *rtpg_colormap_arg;
struct rtpg_colormap_arg_t {
	rt_raster raster;
	int nband; /* 1-based */
	rt_band band;
	rt_bandstats bandstats;

	rt_colormap colormap;
	int nodataentry;

	char **entry;
	int nentry;
	char **element;
	int nelement;
};

static rtpg_colormap_arg
rtpg_colormap_arg_init() {
	rtpg_colormap_arg arg = NULL;

	arg = palloc(sizeof(struct rtpg_colormap_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_colormap_arg: Could not allocate memory for function arguments");
		return NULL;
	}

	arg->raster = NULL;
	arg->nband = 1;
	arg->band = NULL;
	arg->bandstats = NULL;

	arg->colormap = palloc(sizeof(struct rt_colormap_t));
	if (arg->colormap == NULL) {
		elog(ERROR, "rtpg_colormap_arg: Could not allocate memory for function arguments");
		return NULL;
	}
	arg->colormap->nentry = 0;
	arg->colormap->entry = NULL;
	arg->colormap->ncolor = 4; /* assume RGBA */
	arg->colormap->method = CM_INTERPOLATE;
	arg->nodataentry = -1;

	arg->entry = NULL;
	arg->nentry = 0;
	arg->element = NULL;
	arg->nelement = 0;

	return arg;
}

static void
rtpg_colormap_arg_destroy(rtpg_colormap_arg arg) {
	int i = 0;
	if (arg->raster != NULL)
		rt_raster_destroy(arg->raster);

	if (arg->bandstats != NULL)
		pfree(arg->bandstats);

	if (arg->colormap != NULL) {
		if (arg->colormap->entry != NULL)
			pfree(arg->colormap->entry);
		pfree(arg->colormap);
	}

	if (arg->nentry) {
		for (i = 0; i < arg->nentry; i++) {
			if (arg->entry[i] != NULL)
				pfree(arg->entry[i]);
		}
		pfree(arg->entry);
	}

	if (arg->nelement) {
		for (i = 0; i < arg->nelement; i++)
			pfree(arg->element[i]);
		pfree(arg->element);
	}

	pfree(arg);
	arg = NULL;
}

PG_FUNCTION_INFO_V1(RASTER_colorMap);
Datum RASTER_colorMap(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rtpg_colormap_arg arg = NULL;
	char *junk = NULL;
	rt_raster raster = NULL;

	POSTGIS_RT_DEBUG(3, "RASTER_colorMap: Starting");

	/* pgraster is NULL, return NULL */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* init arg */
	arg = rtpg_colormap_arg_init();
	if (arg == NULL) {
		elog(ERROR, "RASTER_colorMap: Could not initialize argument structure");
		PG_RETURN_NULL();
	}

	/* raster (0) */
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster object */
	arg->raster = rt_raster_deserialize(pgraster, FALSE);
	if (!arg->raster) {
		rtpg_colormap_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_colorMap: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* nband (1) */
	if (!PG_ARGISNULL(1))
		arg->nband = PG_GETARG_INT32(1);
	POSTGIS_RT_DEBUGF(4, "nband = %d", arg->nband);

	/* check that band works */
	if (!rt_raster_has_band(arg->raster, arg->nband - 1)) {
		elog(NOTICE, "Raster does not have band at index %d. Returning empty raster", arg->nband);

		raster = rt_raster_clone(arg->raster, 0);
		if (raster == NULL) {
			rtpg_colormap_arg_destroy(arg);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_colorMap: Could not create empty raster");
			PG_RETURN_NULL();
		}

		rtpg_colormap_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);

		pgraster = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (pgraster == NULL)
			PG_RETURN_NULL();

		SET_VARSIZE(pgraster, ((rt_pgraster*) pgraster)->size);
		PG_RETURN_POINTER(pgraster);
	}

	/* get band */
	arg->band = rt_raster_get_band(arg->raster, arg->nband - 1);
	if (arg->band == NULL) {
		int nband = arg->nband;
		rtpg_colormap_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_colorMap: Could not get band at index %d", nband);
		PG_RETURN_NULL();
	}

	/* method (3) */
	if (!PG_ARGISNULL(3)) {
		char *method = NULL;
		char *tmp = text_to_cstring(PG_GETARG_TEXT_P(3));
		POSTGIS_RT_DEBUGF(4, "raw method = %s", tmp);

		method = rtpg_trim(tmp);
		pfree(tmp);
		method = rtpg_strtoupper(method);

		if (strcmp(method, "INTERPOLATE") == 0)
			arg->colormap->method = CM_INTERPOLATE;
		else if (strcmp(method, "EXACT") == 0)
			arg->colormap->method = CM_EXACT;
		else if (strcmp(method, "NEAREST") == 0)
			arg->colormap->method = CM_NEAREST;
		else {
			elog(NOTICE, "Unknown value provided for method. Defaulting to INTERPOLATE");
			arg->colormap->method = CM_INTERPOLATE;
		}
	}
	/* default to INTERPOLATE */
	else
		arg->colormap->method = CM_INTERPOLATE;
	POSTGIS_RT_DEBUGF(4, "method = %d", arg->colormap->method);

	/* colormap (2) */
	if (PG_ARGISNULL(2)) {
		rtpg_colormap_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_colorMap: Value must be provided for colormap");
		PG_RETURN_NULL();
	}
	else {
		char *tmp = NULL;
		char *colormap = text_to_cstring(PG_GETARG_TEXT_P(2));
		char *_entry;
		char *_element;
		int i = 0;
		int j = 0;

		POSTGIS_RT_DEBUGF(4, "colormap = %s", colormap);

		/* empty string */
		if (!strlen(colormap)) {
			rtpg_colormap_arg_destroy(arg);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_colorMap: Value must be provided for colormap");
			PG_RETURN_NULL();
		}

		arg->entry = rtpg_strsplit(colormap, "\n", &(arg->nentry));
		pfree(colormap);
		if (arg->nentry < 1) {
			rtpg_colormap_arg_destroy(arg);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_colorMap: Could not process the value provided for colormap");
			PG_RETURN_NULL();
		}

		/* allocate the max # of colormap entries */
		arg->colormap->entry = palloc(sizeof(struct rt_colormap_entry_t) * arg->nentry);
		if (arg->colormap->entry == NULL) {
			rtpg_colormap_arg_destroy(arg);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_colorMap: Could not allocate memory for colormap entries");
			PG_RETURN_NULL();
		}
		memset(arg->colormap->entry, 0, sizeof(struct rt_colormap_entry_t) * arg->nentry);

		/* each entry */
		for (i = 0; i < arg->nentry; i++) {
			/* substitute space for other delimiters */
			tmp = rtpg_strreplace(arg->entry[i], ":", " ", NULL);
			_entry = rtpg_strreplace(tmp, ",", " ", NULL);
			pfree(tmp);
			tmp = rtpg_strreplace(_entry, "\t", " ", NULL);
			pfree(_entry);
			_entry = rtpg_trim(tmp);
			pfree(tmp);

			POSTGIS_RT_DEBUGF(4, "Processing entry[%d] = %s", i, arg->entry[i]);
			POSTGIS_RT_DEBUGF(4, "Cleaned entry[%d] = %s", i, _entry);

			/* empty entry, continue */
			if (!strlen(_entry)) {
				POSTGIS_RT_DEBUGF(3, "Skipping empty entry[%d]", i);
				pfree(_entry);
				continue;
			}

			arg->element = rtpg_strsplit(_entry, " ", &(arg->nelement));
			pfree(_entry);
			if (arg->nelement < 2) {
				rtpg_colormap_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_colorMap: Could not process colormap entry %d", i + 1);
				PG_RETURN_NULL();
			}
			else if (arg->nelement > 5) {
				elog(NOTICE, "More than five elements in colormap entry %d. Using at most five elements", i + 1);
				arg->nelement = 5;
			}

			/* smallest # of colors */
			if ((arg->nelement - 1) < arg->colormap->ncolor)
				arg->colormap->ncolor = arg->nelement - 1;

			/* each element of entry */
			for (j = 0; j < arg->nelement; j++) {

				_element = rtpg_trim(arg->element[j]);
				_element = rtpg_strtoupper(_element);
				POSTGIS_RT_DEBUGF(4, "Processing entry[%d][%d] = %s", i, j, arg->element[j]);
				POSTGIS_RT_DEBUGF(4, "Cleaned entry[%d][%d] = %s", i, j, _element);

				/* first element is ALWAYS a band value, percentage OR "nv" string */
				if (j == 0) {
					char *percent = NULL;

					/* NODATA */
					if (
						strcmp(_element, "NV") == 0 ||
						strcmp(_element, "NULL") == 0 ||
						strcmp(_element, "NODATA") == 0
					) {
						POSTGIS_RT_DEBUG(4, "Processing NODATA string");

						if (arg->nodataentry > -1) {
							elog(NOTICE, "More than one NODATA entry found. Using only the first one");
						}
						else {
							arg->colormap->entry[arg->colormap->nentry].isnodata = 1;
							/* no need to set value as value comes from band's NODATA */
							arg->colormap->entry[arg->colormap->nentry].value = 0;
						}
					}
					/* percent value */
					else if ((percent = strchr(_element, '%')) != NULL) {
						double value;
						POSTGIS_RT_DEBUG(4, "Processing percent string");

						/* get the band stats */
						if (arg->bandstats == NULL) {
							POSTGIS_RT_DEBUG(4, "Getting band stats");
							
							arg->bandstats = rt_band_get_summary_stats(arg->band, 1, 1, 0, NULL, NULL, NULL);
							if (arg->bandstats == NULL) {
								pfree(_element);
								rtpg_colormap_arg_destroy(arg);
								PG_FREE_IF_COPY(pgraster, 0);
								elog(ERROR, "RASTER_colorMap: Could not get band's summary stats to process percentages");
								PG_RETURN_NULL();
							}
						}

						/* get the string before the percent char */
						tmp = palloc(sizeof(char) * (percent - _element + 1));
						if (tmp == NULL) {
							pfree(_element);
							rtpg_colormap_arg_destroy(arg);
							PG_FREE_IF_COPY(pgraster, 0);
							elog(ERROR, "RASTER_colorMap: Could not allocate memory for value of percentage");
							PG_RETURN_NULL();
						}

						memcpy(tmp, _element, percent - _element);
						tmp[percent - _element] = '\0';
						POSTGIS_RT_DEBUGF(4, "Percent value = %s", tmp);

						/* get percentage value */
						errno = 0;
						value = strtod(tmp, NULL);
						pfree(tmp);
						if (errno != 0 || _element == junk) {
							pfree(_element);
							rtpg_colormap_arg_destroy(arg);
							PG_FREE_IF_COPY(pgraster, 0);
							elog(ERROR, "RASTER_colorMap: Could not process percent string to value");
							PG_RETURN_NULL();
						}

						/* check percentage */
						if (value < 0.) {
							elog(NOTICE, "Percentage values cannot be less than zero. Defaulting to zero");
							value = 0.;
						}
						else if (value > 100.) {
							elog(NOTICE, "Percentage values cannot be greater than 100. Defaulting to 100");
							value = 100.;
						}

						/* get the true pixel value */
						/* TODO: should the percentage be quantile based? */
						arg->colormap->entry[arg->colormap->nentry].value = ((value / 100.) * (arg->bandstats->max - arg->bandstats->min)) + arg->bandstats->min;
					}
					/* straight value */
					else {
						errno = 0;
						arg->colormap->entry[arg->colormap->nentry].value = strtod(_element, &junk);
						if (errno != 0 || _element == junk) {
							pfree(_element);
							rtpg_colormap_arg_destroy(arg);
							PG_FREE_IF_COPY(pgraster, 0);
							elog(ERROR, "RASTER_colorMap: Could not process string to value");
							PG_RETURN_NULL();
						}
					}

				}
				/* RGB values (0 - 255) */
				else {
					int value = 0;

					errno = 0;
					value = (int) strtod(_element, &junk);
					if (errno != 0 || _element == junk) {
						pfree(_element);
						rtpg_colormap_arg_destroy(arg);
						PG_FREE_IF_COPY(pgraster, 0);
						elog(ERROR, "RASTER_colorMap: Could not process string to value");
						PG_RETURN_NULL();
					}

					if (value > 255) {
						elog(NOTICE, "RGBA value cannot be greater than 255. Defaulting to 255");
						value = 255;
					}
					else if (value < 0) {
						elog(NOTICE, "RGBA value cannot be less than zero. Defaulting to zero");
						value = 0;
					}
					arg->colormap->entry[arg->colormap->nentry].color[j - 1] = value;
				}

				pfree(_element);
			}

			POSTGIS_RT_DEBUGF(4, "colormap->entry[%d] (isnodata, value, R, G, B, A) = (%d, %f, %d, %d, %d, %d)",
				arg->colormap->nentry,
				arg->colormap->entry[arg->colormap->nentry].isnodata,
				arg->colormap->entry[arg->colormap->nentry].value,
				arg->colormap->entry[arg->colormap->nentry].color[0],
				arg->colormap->entry[arg->colormap->nentry].color[1],
				arg->colormap->entry[arg->colormap->nentry].color[2],
				arg->colormap->entry[arg->colormap->nentry].color[3]
			);

			arg->colormap->nentry++;
		}

		POSTGIS_RT_DEBUGF(4, "colormap->nentry = %d", arg->colormap->nentry);
		POSTGIS_RT_DEBUGF(4, "colormap->ncolor = %d", arg->colormap->ncolor);
	}

	/* call colormap */
	raster = rt_raster_colormap(arg->raster, arg->nband - 1, arg->colormap);
	if (raster == NULL) {
		rtpg_colormap_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_colorMap: Could not create new raster with applied colormap");
		PG_RETURN_NULL();
	}

	rtpg_colormap_arg_destroy(arg);
	PG_FREE_IF_COPY(pgraster, 0);
	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);

	POSTGIS_RT_DEBUG(3, "RASTER_colorMap: Done");

	if (pgraster == NULL)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, ((rt_pgraster*) pgraster)->size);
	PG_RETURN_POINTER(pgraster);
}

/* ---------------------------------------------------------------- */
/* Returns raster from GDAL raster                                  */
/* ---------------------------------------------------------------- */
PG_FUNCTION_INFO_V1(RASTER_fromGDALRaster);
Datum RASTER_fromGDALRaster(PG_FUNCTION_ARGS)
{
	bytea *bytea_data;
	uint8_t *data;
	int data_len = 0;
	VSILFILE *vsifp = NULL;
	GDALDatasetH hdsSrc;
	int srid = -1; /* -1 for NULL */

	rt_pgraster *pgraster = NULL;
	rt_raster raster;

	/* NULL if NULL */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* get data */
	bytea_data = (bytea *) PG_GETARG_BYTEA_P(0);
	data = (uint8_t *) VARDATA(bytea_data);
	data_len = VARSIZE(bytea_data) - VARHDRSZ;

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
	rt_util_gdal_register_all();

	/* open GDAL raster */
	hdsSrc = GDALOpenShared("/vsimem/in.dat", GA_ReadOnly);
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
	int srid = SRID_UNKNOWN;
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
					options[j] = option;
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
	memcpy(VARDATA(result), gdal, VARSIZE(result) - VARHDRSZ);

	/* for test output
	FILE *fh = NULL;
	fh = fopen("/tmp/out.dat", "w");
	fwrite(gdal, sizeof(uint8_t), gdal_size, fh);
	fclose(fh);
	*/

	/* free gdal mem buffer */
	if (gdal) CPLFree(gdal);

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Returning pointer to GDAL raster");
	PG_RETURN_POINTER(result);
}

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

		drv_set = rt_raster_gdal_drivers(&drv_count, 1);
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
		int values_length = 4;
		Datum values[values_length];
		bool nulls[values_length];
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		memset(nulls, FALSE, sizeof(bool) * values_length);

		values[0] = Int32GetDatum(drv_set2[call_cntr].idx);
		values[1] = CStringGetTextDatum(drv_set2[call_cntr].short_name);
		values[2] = CStringGetTextDatum(drv_set2[call_cntr].long_name);
		values[3] = CStringGetTextDatum(drv_set2[call_cntr].create_options);

		POSTGIS_RT_DEBUGF(4, "Result %d, Index %d", call_cntr, drv_set2[call_cntr].idx);
		POSTGIS_RT_DEBUGF(4, "Result %d, Short Name %s", call_cntr, drv_set2[call_cntr].short_name);
		POSTGIS_RT_DEBUGF(4, "Result %d, Full Name %s", call_cntr, drv_set2[call_cntr].long_name);
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

/**
 * Rasterize a geometry
 */
PG_FUNCTION_INFO_V1(RASTER_asRaster);
Datum RASTER_asRaster(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser = NULL;

	LWGEOM *geom = NULL;
	rt_raster rast = NULL;
	rt_pgraster *pgrast = NULL;

	unsigned char *wkb;
	size_t wkb_len = 0;
	unsigned char variant = WKB_SFSQL;

	double scale[2] = {0};
	double *scale_x = NULL;
	double *scale_y = NULL;

	int dim[2] = {0};
	int *dim_x = NULL;
	int *dim_y = NULL;

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
	int haserr = 0;

	text *pixeltypetext = NULL;
	char *pixeltype = NULL;
	rt_pixtype pixtype = PT_END;
	rt_pixtype *pixtypes = NULL;
	int pixtypes_len = 0;

	double *values = NULL;
	int values_len = 0;

	uint8_t *hasnodatas = NULL;
	double *nodatavals = NULL;
	int nodatavals_len = 0;

	double ulw[2] = {0};
	double *ul_xw = NULL;
	double *ul_yw = NULL;

	double gridw[2] = {0};
	double *grid_xw = NULL;
	double *grid_yw = NULL;

	double skew[2] = {0};
	double *skew_x = NULL;
	double *skew_y = NULL;

	char **options = NULL;
	int options_len = 0;

	uint32_t num_bands = 0;

	int srid = SRID_UNKNOWN;
	char *srs = NULL;

	POSTGIS_RT_DEBUG(3, "RASTER_asRaster: Starting");

	/* based upon LWGEOM_asBinary function in postgis/lwgeom_ogc.c */

	/* Get the geometry */
	if (PG_ARGISNULL(0)) 
		PG_RETURN_NULL();

	gser = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom = lwgeom_from_gserialized(gser);

	/* Get a 2D version of the geometry if necessary */
	if (lwgeom_ndims(geom) > 2) {
		LWGEOM *geom2d = lwgeom_force_2d(geom);
		lwgeom_free(geom);
		geom = geom2d;
	}

	/* empty geometry, return empty raster */
	if (lwgeom_is_empty(geom)) {
		POSTGIS_RT_DEBUG(3, "Input geometry is empty. Returning empty raster");
		lwgeom_free(geom);
		PG_FREE_IF_COPY(gser, 0);

		rast = rt_raster_new(0, 0);
		if (rast == NULL)
			PG_RETURN_NULL();

		pgrast = rt_raster_serialize(rast);
		rt_raster_destroy(rast);

		if (NULL == pgrast)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrast, pgrast->size);
		PG_RETURN_POINTER(pgrast);
	}

	/* scale x */
	if (!PG_ARGISNULL(1)) {
		scale[0] = PG_GETARG_FLOAT8(1);
		if (FLT_NEQ(scale[0], 0)) scale_x = &scale[0];
	}

	/* scale y */
	if (!PG_ARGISNULL(2)) {
		scale[1] = PG_GETARG_FLOAT8(2);
		if (FLT_NEQ(scale[1], 0)) scale_y = &scale[1];
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: scale (x, y) = %f, %f", scale[0], scale[1]);

	/* width */
	if (!PG_ARGISNULL(3)) {
		dim[0] = PG_GETARG_INT32(3);
		if (dim[0] < 0) dim[0] = 0;
		if (dim[0] != 0) dim_x = &dim[0];
	}

	/* height */
	if (!PG_ARGISNULL(4)) {
		dim[1] = PG_GETARG_INT32(4);
		if (dim[1] < 0) dim[1] = 0;
		if (dim[1] != 0) dim_y = &dim[1];
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: dim (x, y) = %d, %d", dim[0], dim[1]);

	/* pixeltype */
	if (!PG_ARGISNULL(5)) {
		array = PG_GETARG_ARRAYTYPE_P(5);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case TEXTOID:
				break;
			default:

				lwgeom_free(geom);
				PG_FREE_IF_COPY(gser, 0);

				elog(ERROR, "RASTER_asRaster: Invalid data type for pixeltype");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		if (n) {
			pixtypes = (rt_pixtype *) palloc(sizeof(rt_pixtype) * n);
			/* clean each pixeltype */
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) {
					pixtypes[j++] = PT_64BF;
					continue;
				}

				pixeltype = NULL;
				switch (etype) {
					case TEXTOID:
						pixeltypetext = (text *) DatumGetPointer(e[i]);
						if (NULL == pixeltypetext) break;
						pixeltype = text_to_cstring(pixeltypetext);

						/* trim string */
						pixeltype = rtpg_trim(pixeltype);
						POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: pixeltype is '%s'", pixeltype);
						break;
				}

				if (strlen(pixeltype)) {
					pixtype = rt_pixtype_index_from_name(pixeltype);
					if (pixtype == PT_END) {

						pfree(pixtypes);

						lwgeom_free(geom);
						PG_FREE_IF_COPY(gser, 0);

						elog(ERROR, "RASTER_asRaster: Invalid pixel type provided: %s", pixeltype);
						PG_RETURN_NULL();
					}

					pixtypes[j] = pixtype;
					j++;
				}
			}

			if (j > 0) {
				/* trim allocation */
				pixtypes = repalloc(pixtypes, j * sizeof(rt_pixtype));
				pixtypes_len = j;
			}
			else {
				pfree(pixtypes);
				pixtypes = NULL;
				pixtypes_len = 0;
			}
		}
	}
#if POSTGIS_DEBUG_LEVEL > 0
	for (i = 0; i < pixtypes_len; i++)
		POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: pixtypes[%d] = %d", i, (int) pixtypes[i]);
#endif

	/* value */
	if (!PG_ARGISNULL(6)) {
		array = PG_GETARG_ARRAYTYPE_P(6);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case FLOAT4OID:
			case FLOAT8OID:
				break;
			default:

				if (pixtypes_len) pfree(pixtypes);

				lwgeom_free(geom);
				PG_FREE_IF_COPY(gser, 0);

				elog(ERROR, "RASTER_asRaster: Invalid data type for value");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		if (n) {
			values = (double *) palloc(sizeof(double) * n);
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) {
					values[j++] = 1;
					continue;
				}

				switch (etype) {
					case FLOAT4OID:
						values[j] = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						values[j] = (double) DatumGetFloat8(e[i]);
						break;
				}
				POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: values[%d] = %f", j, values[j]);

				j++;
			}

			if (j > 0) {
				/* trim allocation */
				values = repalloc(values, j * sizeof(double));
				values_len = j;
			}
			else {
				pfree(values);
				values = NULL;
				values_len = 0;
			}
		}
	}
#if POSTGIS_DEBUG_LEVEL > 0
	for (i = 0; i < values_len; i++)
		POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: values[%d] = %f", i, values[i]);
#endif

	/* nodataval */
	if (!PG_ARGISNULL(7)) {
		array = PG_GETARG_ARRAYTYPE_P(7);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case FLOAT4OID:
			case FLOAT8OID:
				break;
			default:

				if (pixtypes_len) pfree(pixtypes);
				if (values_len) pfree(values);

				lwgeom_free(geom);
				PG_FREE_IF_COPY(gser, 0);

				elog(ERROR, "RASTER_asRaster: Invalid data type for nodataval");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		if (n) {
			nodatavals = (double *) palloc(sizeof(double) * n);
			hasnodatas = (uint8_t *) palloc(sizeof(uint8_t) * n);
			for (i = 0, j = 0; i < n; i++) {
				if (nulls[i]) {
					hasnodatas[j] = 0;
					nodatavals[j] = 0;
					j++;
					continue;
				}

				hasnodatas[j] = 1;
				switch (etype) {
					case FLOAT4OID:
						nodatavals[j] = (double) DatumGetFloat4(e[i]);
						break;
					case FLOAT8OID:
						nodatavals[j] = (double) DatumGetFloat8(e[i]);
						break;
				}
				POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: hasnodatas[%d] = %d", j, hasnodatas[j]);
				POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: nodatavals[%d] = %f", j, nodatavals[j]);

				j++;
			}

			if (j > 0) {
				/* trim allocation */
				nodatavals = repalloc(nodatavals, j * sizeof(double));
				hasnodatas = repalloc(hasnodatas, j * sizeof(uint8_t));
				nodatavals_len = j;
			}
			else {
				pfree(nodatavals);
				pfree(hasnodatas);
				nodatavals = NULL;
				hasnodatas = NULL;
				nodatavals_len = 0;
			}
		}
	}
#if POSTGIS_DEBUG_LEVEL > 0
	for (i = 0; i < nodatavals_len; i++) {
		POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: hasnodatas[%d] = %d", i, hasnodatas[i]);
		POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: nodatavals[%d] = %f", i, nodatavals[i]);
	}
#endif

	/* upperleftx */
	if (!PG_ARGISNULL(8)) {
		ulw[0] = PG_GETARG_FLOAT8(8);
		ul_xw = &ulw[0];
	}

	/* upperlefty */
	if (!PG_ARGISNULL(9)) {
		ulw[1] = PG_GETARG_FLOAT8(9);
		ul_yw = &ulw[1];
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: upperleft (x, y) = %f, %f", ulw[0], ulw[1]);

	/* gridx */
	if (!PG_ARGISNULL(10)) {
		gridw[0] = PG_GETARG_FLOAT8(10);
		grid_xw = &gridw[0];
	}

	/* gridy */
	if (!PG_ARGISNULL(11)) {
		gridw[1] = PG_GETARG_FLOAT8(11);
		grid_yw = &gridw[1];
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: grid (x, y) = %f, %f", gridw[0], gridw[1]);

	/* check dependent variables */
	haserr = 0;
	do {
		/* only part of scale provided */
		if (
			(scale_x == NULL && scale_y != NULL) ||
			(scale_x != NULL && scale_y == NULL)
		) {
			elog(NOTICE, "Values must be provided for both X and Y of scale if one is specified");
			haserr = 1;
			break;
		}

		/* only part of dimension provided */
		if (
			(dim_x == NULL && dim_y != NULL) ||
			(dim_x != NULL && dim_y == NULL)
		) {
			elog(NOTICE, "Values must be provided for both width and height if one is specified");
			haserr = 1;
			break;
		}

		/* scale and dimension provided */
		if (
			(scale_x != NULL && scale_y != NULL) &&
			(dim_x != NULL && dim_y != NULL)
		) {
			elog(NOTICE, "Values provided for X and Y of scale and width and height.  Using the width and height");
			scale_x = NULL;
			scale_y = NULL;
			break;
		}

		/* neither scale or dimension provided */
		if (
			(scale_x == NULL && scale_y == NULL) &&
			(dim_x == NULL && dim_y == NULL)
		) {
			elog(NOTICE, "Values must be provided for X and Y of scale or width and height");
			haserr = 1;
			break;
		}

		/* only part of upper-left provided */
		if (
			(ul_xw == NULL && ul_yw != NULL) ||
			(ul_xw != NULL && ul_yw == NULL)
		) {
			elog(NOTICE, "Values must be provided for both X and Y when specifying the upper-left corner");
			haserr = 1;
			break;
		}

		/* only part of alignment provided */
		if (
			(grid_xw == NULL && grid_yw != NULL) ||
			(grid_xw != NULL && grid_yw == NULL)
		) {
			elog(NOTICE, "Values must be provided for both X and Y when specifying the alignment");
			haserr = 1;
			break;
		}

		/* upper-left and alignment provided */
		if (
			(ul_xw != NULL && ul_yw != NULL) &&
			(grid_xw != NULL && grid_yw != NULL)
		) {
			elog(NOTICE, "Values provided for both X and Y of upper-left corner and alignment.  Using the values of upper-left corner");
			grid_xw = NULL;
			grid_yw = NULL;
			break;
		}
	}
	while (0);

	if (haserr) {
		if (pixtypes_len) pfree(pixtypes);
		if (values_len) pfree(values);
		if (nodatavals_len) {
			pfree(nodatavals);
			pfree(hasnodatas);
		}

		lwgeom_free(geom);
		PG_FREE_IF_COPY(gser, 0);

		PG_RETURN_NULL();
	}

	/* skewx */
	if (!PG_ARGISNULL(12)) {
		skew[0] = PG_GETARG_FLOAT8(12);
		if (FLT_NEQ(skew[0], 0)) skew_x = &skew[0];
	}

	/* skewy */
	if (!PG_ARGISNULL(13)) {
		skew[1] = PG_GETARG_FLOAT8(13);
		if (FLT_NEQ(skew[1], 0)) skew_y = &skew[1];
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: skew (x, y) = %f, %f", skew[0], skew[1]);

	/* all touched */
	if (!PG_ARGISNULL(14) && PG_GETARG_BOOL(14) == TRUE) {
		if (options_len < 1) {
			options_len = 1;
			options = (char **) palloc(sizeof(char *) * options_len);
		}
		else {
			options_len++;
			options = (char **) repalloc(options, sizeof(char *) * options_len);
		}

		options[options_len - 1] = palloc(sizeof(char*) * (strlen("ALL_TOUCHED=TRUE") + 1));
		options[options_len - 1] = "ALL_TOUCHED=TRUE";
	}

	if (options_len) {
		options_len++;
		options = (char **) repalloc(options, sizeof(char *) * options_len);
		options[options_len - 1] = NULL;
	}

	/* get geometry's srid */
	srid = gserialized_get_srid(gser);

	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: srid = %d", srid);
	if (clamp_srid(srid) != SRID_UNKNOWN) {
		srs = rtpg_getSR(srid);
		if (NULL == srs) {

			if (pixtypes_len) pfree(pixtypes);
			if (values_len) pfree(values);
			if (nodatavals_len) {
				pfree(hasnodatas);
				pfree(nodatavals);
			}
			if (options_len) pfree(options);

			lwgeom_free(geom);
			PG_FREE_IF_COPY(gser, 0);

			elog(ERROR, "RASTER_asRaster: Could not find srtext for SRID (%d)", srid);
			PG_RETURN_NULL();
		}
		POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: srs is %s", srs);
	}
	else
		srs = NULL;

	/* determine number of bands */
	/* MIN macro is from GDAL's cpl_port.h */
	num_bands = MIN(pixtypes_len, values_len);
	num_bands = MIN(num_bands, nodatavals_len);
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: pixtypes_len = %d", pixtypes_len);
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: values_len = %d", values_len);
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: nodatavals_len = %d", nodatavals_len);
	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: num_bands = %d", num_bands);

	/* warn of imbalanced number of band elements */
	if (!(
		(pixtypes_len == values_len) &&
		(values_len == nodatavals_len)
	)) {
		elog(
			NOTICE,
			"Imbalanced number of values provided for pixeltype (%d), value (%d) and nodataval (%d).  Using the first %d values of each parameter",
			pixtypes_len,
			values_len,
			nodatavals_len,
			num_bands
		);
	}

	/* get wkb of geometry */
	POSTGIS_RT_DEBUG(3, "RASTER_asRaster: getting wkb of geometry");
	wkb = lwgeom_to_wkb(geom, variant, &wkb_len);
	lwgeom_free(geom);
	PG_FREE_IF_COPY(gser, 0);

	/* rasterize geometry */
	POSTGIS_RT_DEBUG(3, "RASTER_asRaster: rasterizing geometry");
	/* use nodatavals for the init parameter */
	rast = rt_raster_gdal_rasterize(wkb,
		(uint32_t) wkb_len, srs,
		num_bands, pixtypes,
		nodatavals, values,
		nodatavals, hasnodatas,
		dim_x, dim_y,
		scale_x, scale_y,
		ul_xw, ul_yw,
		grid_xw, grid_yw,
		skew_x, skew_y,
		options
	);

	if (pixtypes_len) pfree(pixtypes);
	if (values_len) pfree(values);
	if (nodatavals_len) {
		pfree(hasnodatas);
		pfree(nodatavals);
	}
	if (options_len) pfree(options);

	if (!rast) {
		elog(ERROR, "RASTER_asRaster: Could not rasterize geometry");
		PG_RETURN_NULL();
	}

	/* add target srid */
	rt_raster_set_srid(rast, srid);

	pgrast = rt_raster_serialize(rast);
	rt_raster_destroy(rast);

	if (NULL == pgrast) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_asRaster: done");

	SET_VARSIZE(pgrast, pgrast->size);
	PG_RETURN_POINTER(pgrast);
}

/**
 * warp a raster using GDAL Warp API
 */
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
		if (FLT_NEQ(scale[0], 0)) scale_x = &scale[0];
	}

	/* scale y */
	if (!PG_ARGISNULL(5)) {
		scale[1] = PG_GETARG_FLOAT8(5);
		if (FLT_NEQ(scale[1], 0)) scale_y = &scale[1];
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
		if (FLT_NEQ(skew[0], 0)) skew_x = &skew[0];
	}

	/* skew y */
	if (!PG_ARGISNULL(9)) {
		skew[1] = PG_GETARG_FLOAT8(9);
		if (FLT_NEQ(skew[1], 0)) skew_y = &skew[1];
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
		(scale_x == NULL) &&
		(scale_y == NULL) &&
		(grid_xw == NULL) &&
		(grid_yw == NULL) &&
		(skew_x == NULL) &&
		(skew_y == NULL) &&
		(dim_x == NULL) &&
		(dim_y == NULL)
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
			if (!no_srid) pfree(src_srs);
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

/**
 * Get raster's meta data
 */
PG_FUNCTION_INFO_V1(RASTER_metadata);
Datum RASTER_metadata(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;

	uint32_t numBands;
	double scaleX;
	double scaleY;
	double ipX;
	double ipY;
	double skewX;
	double skewY;
	int32_t srid;
	uint32_t width;
	uint32_t height;

	TupleDesc tupdesc;
	int values_length = 10;
	Datum values[values_length];
	bool nulls[values_length];
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_metadata: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

	/* raster */
	raster = rt_raster_deserialize(pgraster, TRUE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_metadata; Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* upper left x, y */
	ipX = rt_raster_get_x_offset(raster);
	ipY = rt_raster_get_y_offset(raster);

	/* width, height */
	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);

	/* scale x, y */
	scaleX = rt_raster_get_x_scale(raster);
	scaleY = rt_raster_get_y_scale(raster);

	/* skew x, y */
	skewX = rt_raster_get_x_skew(raster);
	skewY = rt_raster_get_y_skew(raster);

	/* srid */
	srid = rt_raster_get_srid(raster);

	/* numbands */
	numBands = rt_raster_get_num_bands(raster);

	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

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

	values[0] = Float8GetDatum(ipX);
	values[1] = Float8GetDatum(ipY);
	values[2] = UInt32GetDatum(width);
	values[3] = UInt32GetDatum(height);
	values[4] = Float8GetDatum(scaleX);
	values[5] = Float8GetDatum(scaleY);
	values[6] = Float8GetDatum(skewX);
	values[7] = Float8GetDatum(skewY);
	values[8] = Int32GetDatum(srid);
	values[9] = UInt32GetDatum(numBands);

	memset(nulls, FALSE, sizeof(bool) * values_length);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	PG_RETURN_DATUM(result);
}

/**
 * Get raster bands' meta data
 */
PG_FUNCTION_INFO_V1(RASTER_bandmetadata);
Datum RASTER_bandmetadata(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	int call_cntr;
	int max_calls;

	struct bandmetadata {
		uint32_t bandnum;
		char *pixeltype;
		bool hasnodata;
		double nodataval;
		bool isoutdb;
		char *bandpath;
	};
	struct bandmetadata *bmd = NULL;
	struct bandmetadata *bmd2 = NULL;

	HeapTuple tuple;
	Datum result;

	if (SRF_IS_FIRSTCALL()) {
		MemoryContext oldcontext;

		rt_pgraster *pgraster = NULL;
		rt_raster raster = NULL;
		rt_band band = NULL;

		ArrayType *array;
		Oid etype;
		Datum *e;
		bool *nulls;
		int16 typlen;
		bool typbyval;
		char typalign;
		int i = 0;
		int j = 0;
		int n = 0;

		uint32_t numBands;
		uint32_t idx = 1;
		uint32_t *bandNums = NULL;
		const char *tmp = NULL;

		POSTGIS_RT_DEBUG(3, "RASTER_bandmetadata: Starting");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return null */
		if (PG_ARGISNULL(0)) {
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		/* raster */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_bandmetadata: Could not deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(raster);
		if (numBands < 1) {
			elog(NOTICE, "Raster provided has no bands");
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 0);
			MemoryContextSwitchTo(oldcontext);
			SRF_RETURN_DONE(funcctx);
		}

		/* band index */
		array = PG_GETARG_ARRAYTYPE_P(1);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case INT2OID:
			case INT4OID:
				break;
			default:
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_bandmetadata: Invalid data type for band number(s)");
				SRF_RETURN_DONE(funcctx);
				break;
		}

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		bandNums = palloc(sizeof(uint32_t) * n);
		for (i = 0, j = 0; i < n; i++) {
			if (nulls[i]) continue;

			switch (etype) {
				case INT2OID:
					idx = (uint32_t) DatumGetInt16(e[i]);
					break;
				case INT4OID:
					idx = (uint32_t) DatumGetInt32(e[i]);
					break;
			}

			POSTGIS_RT_DEBUGF(3, "band idx (before): %d", idx);
			if (idx > numBands || idx < 1) {
				elog(NOTICE, "Invalid band index: %d. Indices must be 1-based. Returning NULL", idx);
				pfree(bandNums);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			bandNums[j] = idx;
			POSTGIS_RT_DEBUGF(3, "bandNums[%d] = %d", j, bandNums[j]);
			j++;
		}

		if (j < 1) {
			j = numBands;
			bandNums = repalloc(bandNums, sizeof(uint32_t) * j);
			for (i = 0; i < j; i++)
				bandNums[i] = i + 1;
		}
		else if (j < n)
			bandNums = repalloc(bandNums, sizeof(uint32_t) * j);

		bmd = (struct bandmetadata *) palloc(sizeof(struct bandmetadata) * j);

		for (i = 0; i < j; i++) {
			band = rt_raster_get_band(raster, bandNums[i] - 1);
			if (NULL == band) {
				elog(NOTICE, "Could not get raster band at index %d", bandNums[i]);
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				MemoryContextSwitchTo(oldcontext);
				SRF_RETURN_DONE(funcctx);
			}

			/* bandnum */
			bmd[i].bandnum = bandNums[i];

			/* pixeltype */
			tmp = rt_pixtype_name(rt_band_get_pixtype(band));
			bmd[i].pixeltype = palloc(sizeof(char) * (strlen(tmp) + 1));
			strncpy(bmd[i].pixeltype, tmp, strlen(tmp) + 1);

			/* hasnodatavalue */
			if (rt_band_get_hasnodata_flag(band))
				bmd[i].hasnodata = TRUE;
			else
				bmd[i].hasnodata = FALSE;

			/* nodatavalue */
			if (bmd[i].hasnodata)
				rt_band_get_nodata(band, &(bmd[i].nodataval));
			else
				bmd[i].nodataval = 0;

			/* path */
			tmp = rt_band_get_ext_path(band);
			if (tmp) {
				bmd[i].bandpath = palloc(sizeof(char) * (strlen(tmp) + 1));
				strncpy(bmd[i].bandpath, tmp, strlen(tmp) + 1);
			}
			else
				bmd[i].bandpath = NULL;

			/* isoutdb */
			bmd[i].isoutdb = bmd[i].bandpath ? TRUE : FALSE;

			rt_band_destroy(band);
		}

		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);

		/* Store needed information */
		funcctx->user_fctx = bmd;

		/* total number of tuples to be returned */
		funcctx->max_calls = j;

		/* Build a tuple descriptor for our result type */
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

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	tupdesc = funcctx->tuple_desc;
	bmd2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		int values_length = 5;
		Datum values[values_length];
		bool nulls[values_length];

		memset(nulls, FALSE, sizeof(bool) * values_length);

		values[0] = UInt32GetDatum(bmd2[call_cntr].bandnum);
		values[1] = CStringGetTextDatum(bmd2[call_cntr].pixeltype);

		if (bmd2[call_cntr].hasnodata)
			values[2] = Float8GetDatum(bmd2[call_cntr].nodataval);
		else
			nulls[2] = TRUE;

		values[3] = BoolGetDatum(bmd2[call_cntr].isoutdb);
		if (bmd2[call_cntr].bandpath && strlen(bmd2[call_cntr].bandpath))
			values[4] = CStringGetTextDatum(bmd2[call_cntr].bandpath);
		else
			nulls[4] = TRUE;

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(bmd2[call_cntr].pixeltype);
		if (bmd2[call_cntr].bandpath) pfree(bmd2[call_cntr].bandpath);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(bmd2);
		SRF_RETURN_DONE(funcctx);
	}
}

PG_FUNCTION_INFO_V1(RASTER_rasterToWorldCoord);
Datum RASTER_rasterToWorldCoord(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	int i = 0;
	int cr[2] = {0};
	bool skewed[2] = {false, false};
	double cw[2] = {0};

	TupleDesc tupdesc;
	int values_length = 2;
	Datum values[values_length];
	bool nulls[values_length];
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_rasterToWorldCoord: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

	/* raster */
	raster = rt_raster_deserialize(pgraster, TRUE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_rasterToWorldCoord: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* raster skewed? */
	skewed[0] = FLT_NEQ(rt_raster_get_x_skew(raster), 0) ? true : false;
	skewed[1] = FLT_NEQ(rt_raster_get_y_skew(raster), 0) ? true : false;

	/* column and row */
	for (i = 1; i <= 2; i++) {
		if (PG_ARGISNULL(i)) {
			/* if skewed on same axis, parameter is required */
			if (skewed[i - 1]) {
				elog(NOTICE, "Pixel row and column required for computing longitude and latitude of a rotated raster");
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				PG_RETURN_NULL();
			}

			continue;
		}

		cr[i - 1] = PG_GETARG_INT32(i);
	}

	/* user-provided value is 1-based but needs to be 0-based */
	if (rt_raster_cell_to_geopoint(
		raster,
		(double) cr[0] - 1, (double) cr[1] - 1,
		&(cw[0]), &(cw[1]),
		NULL
	) != ES_NONE) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_rasterToWorldCoord: Could not compute longitude and latitude from pixel row and column");
		PG_RETURN_NULL();
	}
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

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

	values[0] = Float8GetDatum(cw[0]);
	values[1] = Float8GetDatum(cw[1]);

	memset(nulls, FALSE, sizeof(bool) * values_length);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	PG_RETURN_DATUM(result);
}

PG_FUNCTION_INFO_V1(RASTER_worldToRasterCoord);
Datum RASTER_worldToRasterCoord(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	int i = 0;
	double cw[2] = {0};
	double _cr[2] = {0};
	int cr[2] = {0};
	bool skewed = false;

	TupleDesc tupdesc;
	int values_length = 2;
	Datum values[values_length];
	bool nulls[values_length];
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_worldToRasterCoord: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

	/* raster */
	raster = rt_raster_deserialize(pgraster, TRUE);
	if (!raster) {
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_worldToRasterCoord: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* raster skewed? */
	skewed = FLT_NEQ(rt_raster_get_x_skew(raster), 0) ? true : false;
	if (!skewed)
		skewed = FLT_NEQ(rt_raster_get_y_skew(raster), 0) ? true : false;

	/* longitude and latitude */
	for (i = 1; i <= 2; i++) {
		if (PG_ARGISNULL(i)) {
			/* if skewed, parameter is required */
			if (skewed) {
				elog(NOTICE, "Latitude and longitude required for computing pixel row and column of a rotated raster");
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
				PG_RETURN_NULL();
			}

			continue;
		}

		cw[i - 1] = PG_GETARG_FLOAT8(i);
	}

	/* return pixel row and column values are 0-based */
	if (rt_raster_geopoint_to_cell(
		raster,
		cw[0], cw[1],
		&(_cr[0]), &(_cr[1]),
		NULL
	) != ES_NONE) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_worldToRasterCoord: Could not compute pixel row and column from longitude and latitude");
		PG_RETURN_NULL();
	}
	rt_raster_destroy(raster);
	PG_FREE_IF_COPY(pgraster, 0);

	/* force to integer and add one to make 1-based */
	cr[0] = ((int) _cr[0]) + 1;
	cr[1] = ((int) _cr[1]) + 1;

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

	values[0] = Int32GetDatum(cr[0]);
	values[1] = Int32GetDatum(cr[1]);

	memset(nulls, FALSE, sizeof(bool) * values_length);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	PG_RETURN_DATUM(result);
}

/**
 * See if two rasters intersect
 */
PG_FUNCTION_INFO_V1(RASTER_intersects);
Datum RASTER_intersects(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_intersects: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_intersects(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_intersects: Could not test for intersection on the two rasters");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if two rasters overlap
 */
PG_FUNCTION_INFO_V1(RASTER_overlaps);
Datum RASTER_overlaps(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_overlaps: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_overlaps(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_overlaps: Could not test for overlap on the two rasters");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if two rasters touch
 */
PG_FUNCTION_INFO_V1(RASTER_touches);
Datum RASTER_touches(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_touches: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_touches(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_touches: Could not test for touch on the two rasters");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the first raster contains the second raster
 */
PG_FUNCTION_INFO_V1(RASTER_contains);
Datum RASTER_contains(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_contains: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_contains(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_contains: Could not test that the first raster contains the second raster");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the first raster contains properly the second raster
 */
PG_FUNCTION_INFO_V1(RASTER_containsProperly);
Datum RASTER_containsProperly(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_containsProperly: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_contains_properly(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_containsProperly: Could not test that the first raster contains properly the second raster");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the first raster covers the second raster
 */
PG_FUNCTION_INFO_V1(RASTER_covers);
Datum RASTER_covers(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_covers: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_covers(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_covers: Could not test that the first raster covers the second raster");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the first raster is covered by the second raster
 */
PG_FUNCTION_INFO_V1(RASTER_coveredby);
Datum RASTER_coveredby(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_coveredby: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_coveredby(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_coveredby: Could not test that the first raster is covered by the second raster");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the two rasters are within the specified distance of each other
 */
PG_FUNCTION_INFO_V1(RASTER_dwithin);
Datum RASTER_dwithin(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};
	double distance = 0;

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_dwithin: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* distance */
	if (PG_ARGISNULL(4)) {
		elog(NOTICE, "Distance cannot be NULL.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	distance = PG_GETARG_FLOAT8(4);
	if (distance < 0) {
		elog(NOTICE, "Distance cannot be less than zero.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_within_distance(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		distance,
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_dwithin: Could not test that the two rasters are within the specified distance of each other");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if the two rasters are fully within the specified distance of each other
 */
PG_FUNCTION_INFO_V1(RASTER_dfullywithin);
Datum RASTER_dfullywithin(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};
	double distance = 0;

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int result;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_dfullywithin: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			if (i > 0) i++;
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				if (i > 0) i++;
				for (k = 0; k < i; k++) {
					rt_raster_destroy(rast[k]);
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				PG_RETURN_NULL();
			}
			hasbandindex[i] = 1;
		}
		else
			hasbandindex[i] = 0;
		POSTGIS_RT_DEBUGF(4, "hasbandindex[%d] = %d", i, hasbandindex[i]);
		POSTGIS_RT_DEBUGF(4, "bandindex[%d] = %d", i, bandindex[i]);
		j++;
	}

	/* distance */
	if (PG_ARGISNULL(4)) {
		elog(NOTICE, "Distance cannot be NULL.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	distance = PG_GETARG_FLOAT8(4);
	if (distance < 0) {
		elog(NOTICE, "Distance cannot be less than zero.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* hasbandindex must be balanced */
	if (
		(hasbandindex[0] && !hasbandindex[1]) ||
		(!hasbandindex[0] && hasbandindex[1])
	) {
		elog(NOTICE, "Missing band index.  Band indices must be provided for both rasters if any one is provided");
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		for (k = 0; k < set_count; k++) {
			rt_raster_destroy(rast[k]);
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "The two rasters provided have different SRIDs");
		PG_RETURN_NULL();
	}

	rtn = rt_raster_fully_within_distance(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		distance,
		&result
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_dfullywithin: Could not test that the two rasters are fully within the specified distance of each other");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(result);
}

/**
 * See if two rasters are aligned
 */
PG_FUNCTION_INFO_V1(RASTER_sameAlignment);
Datum RASTER_sameAlignment(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	int rtn;
	int aligned = 0;
	char *reason = NULL;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(j), 0, sizeof(struct rt_raster_serialized_t));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], TRUE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_sameAlignment: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}
	}

	rtn = rt_raster_same_alignment(
		rast[0],
		rast[1],
		&aligned,
		&reason
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_sameAlignment: Could not test for alignment on the two rasters");
		PG_RETURN_NULL();
	}

	/* only output reason if not aligned */
	if (reason != NULL && !aligned)
		elog(NOTICE, "%s", reason);

	PG_RETURN_BOOL(aligned);
}

/**
 * Return a reason why two rasters are not aligned
 */
PG_FUNCTION_INFO_V1(RASTER_notSameAlignmentReason);
Datum RASTER_notSameAlignmentReason(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_raster rast[2] = {NULL};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	int rtn;
	int aligned = 0;
	char *reason = NULL;
	text *result = NULL;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) {
				rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			PG_RETURN_NULL();
		}
		pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(j), 0, sizeof(struct rt_raster_serialized_t));
		pgrastpos[i] = j;
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast[i], TRUE);
		if (!rast[i]) {
			for (k = 0; k <= i; k++) {
				if (k < i)
					rt_raster_destroy(rast[k]);
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_notSameAlignmentReason: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			PG_RETURN_NULL();
		}
	}

	rtn = rt_raster_same_alignment(
		rast[0],
		rast[1],
		&aligned,
		&reason
	);
	for (k = 0; k < set_count; k++) {
		rt_raster_destroy(rast[k]);
		PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	if (rtn != ES_NONE) {
		elog(ERROR, "RASTER_notSameAlignmentReason: Could not test for alignment on the two rasters");
		PG_RETURN_NULL();
	}

	result = cstring2text(reason);
	PG_RETURN_TEXT_P(result);
}

/**
 * Two raster MapAlgebra
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebra2);
Datum RASTER_mapAlgebra2(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast[2];
	int pgrastpos[2] = {-1, -1};
	rt_pgraster *pgrtn;
	rt_raster rast[2] = {NULL};
	int _isempty[2] = {0};
	uint32_t bandindex[2] = {0};
	rt_raster _rast[2] = {NULL};
	rt_band _band[2] = {NULL};
	int _hasnodata[2] = {0};
	double _nodataval[2] = {0};
	double _offset[4] = {0.};
	double _rastoffset[2][4] = {{0.}};
	int _haspixel[2] = {0};
	double _pixel[2] = {0};
	int _pos[2][2] = {{0}};
	uint16_t _dim[2][2] = {{0}};

	char *pixtypename = NULL;
	rt_pixtype pixtype = PT_END;
	char *extenttypename = NULL;
	rt_extenttype extenttype = ET_INTERSECTION;

	rt_raster raster = NULL;
	rt_band band = NULL;
	uint16_t dim[2] = {0};
	int haspixel = 0;
	double pixel = 0.;
	double nodataval = 0;
	double gt[6] = {0.};

	Oid calltype = InvalidOid;

	const int spi_count = 3;
	uint16_t spi_exprpos[3] = {4, 7, 8};
	uint32_t spi_argcount[3] = {0};
	char *expr = NULL;
	char *sql = NULL;
	SPIPlanPtr spi_plan[3] = {NULL};
	uint16_t spi_empty = 0;
	Oid *argtype = NULL;
	const int argkwcount = 8;
	uint8_t argpos[3][8] = {{0}};
	char *argkw[] = {"[rast1.x]", "[rast1.y]", "[rast1.val]", "[rast1]", "[rast2.x]", "[rast2.y]", "[rast2.val]", "[rast2]"};
	Datum values[argkwcount];
	bool nulls[argkwcount];
	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	Datum datum;
	bool isnull = FALSE;
	int hasargval[3] = {0};
	double argval[3] = {0.};
	int hasnodatanodataval = 0;
	double nodatanodataval = 0;
	int isnodata = 0;

	Oid ufc_noid = InvalidOid;
	FmgrInfo ufl_info;
	FunctionCallInfoData ufc_info;
	int ufc_nullcount = 0;

	int idx = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int _x = 0;
	int _y = 0;
	int err;
	int aligned = 0;
	int len = 0;

	POSTGIS_RT_DEBUG(3, "Starting RASTER_mapAlgebra2");

	for (i = 0, j = 0; i < set_count; i++) {
		if (!PG_ARGISNULL(j)) {
			pgrast[i] = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
			pgrastpos[i] = j;
			j++;

			/* raster */
			rast[i] = rt_raster_deserialize(pgrast[i], FALSE);
			if (!rast[i]) {
				for (k = 0; k <= i; k++) {
					if (k < i && rast[k] != NULL)
						rt_raster_destroy(rast[k]);
					if (pgrastpos[k] != -1)
						PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				elog(ERROR, "RASTER_mapAlgebra2: Could not deserialize the %s raster", i < 1 ? "first" : "second");
				PG_RETURN_NULL();
			}

			/* empty */
			_isempty[i] = rt_raster_is_empty(rast[i]);

			/* band index */
			if (!PG_ARGISNULL(j)) {
				bandindex[i] = PG_GETARG_INT32(j);
			}
			j++;
		}
		else {
			_isempty[i] = 1;
			j += 2;
		}

		POSTGIS_RT_DEBUGF(3, "_isempty[%d] = %d", i, _isempty[i]);
	}

	/* both rasters are NULL */
	if (rast[0] == NULL && rast[1] == NULL) {
		elog(NOTICE, "The two rasters provided are NULL.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* both rasters are empty */
	if (_isempty[0] && _isempty[1]) {
		elog(NOTICE, "The two rasters provided are empty.  Returning empty raster");

		raster = rt_raster_new(0, 0);
		if (raster == NULL) {
			for (k = 0; k < set_count; k++) {
				if (rast[k] != NULL)
					rt_raster_destroy(rast[k]);
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_mapAlgebra2: Could not create empty raster");
			PG_RETURN_NULL();
		}
		rt_raster_set_scale(raster, 0, 0);

		pgrtn = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgrtn)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	/* replace the empty or NULL raster with one matching the other */
	if (
		(rast[0] == NULL || _isempty[0]) ||
		(rast[1] == NULL || _isempty[1])
	) {
		/* first raster is empty */
		if (rast[0] == NULL || _isempty[0]) {
			i = 0;
			j = 1;
		}
		/* second raster is empty */
		else {
			i = 1;
			j = 0;
		}

		_rast[j] = rast[j];

		/* raster is empty, destroy it */
		if (_rast[i] != NULL)
			rt_raster_destroy(_rast[i]);

		_dim[i][0] = rt_raster_get_width(_rast[j]);
		_dim[i][1] = rt_raster_get_height(_rast[j]);
		_dim[j][0] = rt_raster_get_width(_rast[j]);
		_dim[j][1] = rt_raster_get_height(_rast[j]);

		_rast[i] = rt_raster_new(
			_dim[j][0],
			_dim[j][1]
		);
		if (_rast[i] == NULL) {
			rt_raster_destroy(_rast[j]);
			for (k = 0; k < set_count; k++)	{
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_mapAlgebra2: Could not create NODATA raster");
			PG_RETURN_NULL();
		}
		rt_raster_set_srid(_rast[i], rt_raster_get_srid(_rast[j]));

		rt_raster_get_geotransform_matrix(_rast[j], gt);
		rt_raster_set_geotransform_matrix(_rast[i], gt);
	}
	else {
		_rast[0] = rast[0];
		_dim[0][0] = rt_raster_get_width(_rast[0]);
		_dim[0][1] = rt_raster_get_height(_rast[0]);

		_rast[1] = rast[1];
		_dim[1][0] = rt_raster_get_width(_rast[1]);
		_dim[1][1] = rt_raster_get_height(_rast[1]);
	}

	/* SRID must match */
	/*
	if (rt_raster_get_srid(_rast[0]) != rt_raster_get_srid(_rast[1])) {
		elog(NOTICE, "The two rasters provided have different SRIDs.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}
	*/

	/* same alignment */
	if (rt_raster_same_alignment(_rast[0], _rast[1], &aligned, NULL) != ES_NONE) {
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "RASTER_mapAlgebra2: Could not test for alignment on the two rasters");
		PG_RETURN_NULL();
	}
	if (!aligned) {
		elog(NOTICE, "The two rasters provided do not have the same alignment.  Returning NULL");
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		PG_RETURN_NULL();
	}

	/* pixel type */
	if (!PG_ARGISNULL(5)) {
		pixtypename = text_to_cstring(PG_GETARG_TEXT_P(5));
		/* Get the pixel type index */
		pixtype = rt_pixtype_index_from_name(pixtypename);
		if (pixtype == PT_END ) {
			for (k = 0; k < set_count; k++) {
				if (_rast[k] != NULL)
					rt_raster_destroy(_rast[k]);
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_mapAlgebra2: Invalid pixel type: %s", pixtypename);
			PG_RETURN_NULL();
		}
	}

	/* extent type */
	if (!PG_ARGISNULL(6)) {
		extenttypename = rtpg_strtoupper(rtpg_trim(text_to_cstring(PG_GETARG_TEXT_P(6))));
		extenttype = rt_util_extent_type(extenttypename);
	}
	POSTGIS_RT_DEBUGF(3, "extenttype: %d %s", extenttype, extenttypename);

	/* computed raster from extent type */
	err = rt_raster_from_two_rasters(
		_rast[0], _rast[1],
		extenttype,
		&raster, _offset
	);
	if (err != ES_NONE) {
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		elog(ERROR, "RASTER_mapAlgebra2: Could not get output raster of correct extent");
		PG_RETURN_NULL();
	}

	/* copy offsets */
	_rastoffset[0][0] = _offset[0];
	_rastoffset[0][1] = _offset[1];
	_rastoffset[1][0] = _offset[2];
	_rastoffset[1][1] = _offset[3];

	/* get output raster dimensions */
	dim[0] = rt_raster_get_width(raster);
	dim[1] = rt_raster_get_height(raster);

	i = 2;
	/* handle special cases for extent */
	switch (extenttype) {
		case ET_FIRST:
			i = 0;
		case ET_SECOND:
			if (i > 1)
				i = 1;

			if (
				_isempty[i] && (
					(extenttype == ET_FIRST && i == 0) ||
					(extenttype == ET_SECOND && i == 1)
				)
			) {
				elog(NOTICE, "The %s raster is NULL.  Returning NULL", (i != 1 ? "FIRST" : "SECOND"));
				for (k = 0; k < set_count; k++) {
					if (_rast[k] != NULL)
						rt_raster_destroy(_rast[k]);
					if (pgrastpos[k] != -1)
						PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				rt_raster_destroy(raster);
				PG_RETURN_NULL();
			}

			/* specified band not found */
			if (!rt_raster_has_band(_rast[i], bandindex[i] - 1)) {
				elog(NOTICE, "The %s raster does not have the band at index %d.  Returning no band raster of correct extent",
					(i != 1 ? "FIRST" : "SECOND"), bandindex[i]
				);

				for (k = 0; k < set_count; k++) {
					if (_rast[k] != NULL)
						rt_raster_destroy(_rast[k]);
					if (pgrastpos[k] != -1)
						PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}

				pgrtn = rt_raster_serialize(raster);
				rt_raster_destroy(raster);
				if (!pgrtn) PG_RETURN_NULL();

				SET_VARSIZE(pgrtn, pgrtn->size);
				PG_RETURN_POINTER(pgrtn);
			}
			break;
		case ET_UNION:
			break;
		case ET_INTERSECTION:
			/* no intersection */
			if (
				_isempty[0] || _isempty[1] ||
				!dim[0] || !dim[1]
			) {
				elog(NOTICE, "The two rasters provided have no intersection.  Returning no band raster");

				/* raster has dimension, replace with no band raster */
				if (dim[0] || dim[1]) {
					rt_raster_destroy(raster);

					raster = rt_raster_new(0, 0);
					if (raster == NULL) {
						for (k = 0; k < set_count; k++) {
							if (_rast[k] != NULL)
								rt_raster_destroy(_rast[k]);
							if (pgrastpos[k] != -1)
								PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
						}
						elog(ERROR, "RASTER_mapAlgebra2: Could not create no band raster");
						PG_RETURN_NULL();
					}

					rt_raster_set_scale(raster, 0, 0);
					rt_raster_set_srid(raster, rt_raster_get_srid(_rast[0]));
				}

				for (k = 0; k < set_count; k++) {
					if (_rast[k] != NULL)
						rt_raster_destroy(_rast[k]);
					if (pgrastpos[k] != -1)
						PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}

				pgrtn = rt_raster_serialize(raster);
				rt_raster_destroy(raster);
				if (!pgrtn) PG_RETURN_NULL();

				SET_VARSIZE(pgrtn, pgrtn->size);
				PG_RETURN_POINTER(pgrtn);
			}
			break;
		case ET_LAST:
		case ET_CUSTOM:
			for (k = 0; k < set_count; k++) {
				if (_rast[k] != NULL)
					rt_raster_destroy(_rast[k]);
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			elog(ERROR, "RASTER_mapAlgebra2: ET_LAST and ET_CUSTOM are not implemented");
			PG_RETURN_NULL();
			break;
	}

	/* both rasters do not have specified bands */
	if (
		(!_isempty[0] && !rt_raster_has_band(_rast[0], bandindex[0] - 1)) &&
		(!_isempty[1] && !rt_raster_has_band(_rast[1], bandindex[1] - 1))
	) {
		elog(NOTICE, "The two rasters provided do not have the respectively specified band indices.  Returning no band raster of correct extent");

		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}

		pgrtn = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgrtn) PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	/* get bands */
	for (i = 0; i < set_count; i++) {
		if (_isempty[i] || !rt_raster_has_band(_rast[i], bandindex[i] - 1)) {
			_hasnodata[i] = 1;
			_nodataval[i] = 0;

			continue;
		}

		_band[i] = rt_raster_get_band(_rast[i], bandindex[i] - 1);
		if (_band[i] == NULL) {
			for (k = 0; k < set_count; k++) {
				if (_rast[k] != NULL)
					rt_raster_destroy(_rast[k]);
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			rt_raster_destroy(raster);
			elog(ERROR, "RASTER_mapAlgebra2: Could not get band %d of the %s raster",
				bandindex[i],
				(i < 1 ? "FIRST" : "SECOND")
			);
			PG_RETURN_NULL();
		}

		_hasnodata[i] = rt_band_get_hasnodata_flag(_band[i]);
		if (_hasnodata[i])
			rt_band_get_nodata(_band[i], &(_nodataval[i]));
	}

	/* pixtype is PT_END, get pixtype based upon extent */
	if (pixtype == PT_END) {
		if ((extenttype == ET_SECOND && !_isempty[1]) || _isempty[0])
			pixtype = rt_band_get_pixtype(_band[1]);
		else
			pixtype = rt_band_get_pixtype(_band[0]);
	}

	/* nodata value for new band */
	if (extenttype == ET_SECOND && !_isempty[1] && _hasnodata[1]) {
		nodataval = _nodataval[1];
	}
	else if (!_isempty[0] && _hasnodata[0]) {
		nodataval = _nodataval[0];
	}
	else if (!_isempty[1] && _hasnodata[1]) {
		nodataval = _nodataval[1];
	}
	else {
		elog(NOTICE, "Neither raster provided has a NODATA value for the specified band indices.  NODATA value set to minimum possible for %s", rt_pixtype_name(pixtype));
		nodataval = rt_pixtype_get_min_value(pixtype);
	}

	/* add band to output raster */
	if (rt_raster_generate_new_band(
		raster,
		pixtype,
		nodataval,
		1, nodataval,
		0
	) < 0) {
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		rt_raster_destroy(raster);
		elog(ERROR, "RASTER_mapAlgebra2: Could not add new band to output raster");
		PG_RETURN_NULL();
	}

	/* get output band */
	band = rt_raster_get_band(raster, 0);
	if (band == NULL) {
		for (k = 0; k < set_count; k++) {
			if (_rast[k] != NULL)
				rt_raster_destroy(_rast[k]);
			if (pgrastpos[k] != -1)
				PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
		}
		rt_raster_destroy(raster);
		elog(ERROR, "RASTER_mapAlgebra2: Could not get newly added band of output raster");
		PG_RETURN_NULL();
	}

	POSTGIS_RT_DEBUGF(4, "offsets = (%d, %d, %d, %d)",
		(int) _rastoffset[0][0],
		(int) _rastoffset[0][1],
		(int) _rastoffset[1][0],
		(int) _rastoffset[1][1]
	);

	POSTGIS_RT_DEBUGF(4, "metadata = (%f, %f, %d, %d, %f, %f, %f, %f, %d)",
		rt_raster_get_x_offset(raster),
		rt_raster_get_y_offset(raster),
		rt_raster_get_width(raster),
		rt_raster_get_height(raster),
		rt_raster_get_x_scale(raster),
		rt_raster_get_y_scale(raster),
		rt_raster_get_x_skew(raster),
		rt_raster_get_y_skew(raster),
		rt_raster_get_srid(raster)
	);

	/*
		determine who called this function
		Arg 4 will either be text or regprocedure
	*/
	POSTGIS_RT_DEBUG(3, "checking parameter type for arg 4");
	calltype = get_fn_expr_argtype(fcinfo->flinfo, 4);

	switch(calltype) {
		case TEXTOID: {
			POSTGIS_RT_DEBUG(3, "arg 4 is \"expression\"!");

			/* connect SPI */
			if (SPI_connect() != SPI_OK_CONNECT) {
				for (k = 0; k < set_count; k++) {
					if (_rast[k] != NULL)
						rt_raster_destroy(_rast[k]);
					if (pgrastpos[k] != -1)
						PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
				}
				rt_raster_destroy(raster);
				elog(ERROR, "RASTER_mapAlgebra2: Could not connect to the SPI manager");
				PG_RETURN_NULL();
			}

			/* reset hasargval */
			memset(hasargval, 0, sizeof(int) * spi_count);

			/*
				process expressions

				spi_exprpos elements are:
					4 - expression => spi_plan[0]
					7 - nodata1expr => spi_plan[1]
					8 - nodata2expr => spi_plan[2]
			*/
			for (i = 0; i < spi_count; i++) {
				if (!PG_ARGISNULL(spi_exprpos[i])) {
					char *tmp = NULL;
					char place[5] = "$1";
					expr = text_to_cstring(PG_GETARG_TEXT_P(spi_exprpos[i]));
					POSTGIS_RT_DEBUGF(3, "raw expr #%d: %s", i, expr);

					for (j = 0, k = 1; j < argkwcount; j++) {
						/* attempt to replace keyword with placeholder */
						len = 0;
						tmp = rtpg_strreplace(expr, argkw[j], place, &len);
						pfree(expr);
						expr = tmp;

						if (len) {
							spi_argcount[i]++;
							argpos[i][j] = k++;

							sprintf(place, "$%d", k);
						}
						else
							argpos[i][j] = 0;
					}

					len = strlen("SELECT (") + strlen(expr) + strlen(")::double precision");
					sql = (char *) palloc(len + 1);
					if (sql == NULL) {

						for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
						SPI_finish();

						for (k = 0; k < set_count; k++) {
							if (_rast[k] != NULL)
								rt_raster_destroy(_rast[k]);
							if (pgrastpos[k] != -1)
								PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
						}
						rt_raster_destroy(raster);

						elog(ERROR, "RASTER_mapAlgebra2: Could not allocate memory for expression parameter %d", spi_exprpos[i]);
						PG_RETURN_NULL();
					}

					strncpy(sql, "SELECT (", strlen("SELECT ("));
					strncpy(sql + strlen("SELECT ("), expr, strlen(expr));
					strncpy(sql + strlen("SELECT (") + strlen(expr), ")::double precision", strlen(")::double precision"));
					sql[len] = '\0';

					POSTGIS_RT_DEBUGF(3, "sql #%d: %s", i, sql);

					/* create prepared plan */
					if (spi_argcount[i]) {
						argtype = (Oid *) palloc(spi_argcount[i] * sizeof(Oid));
						if (argtype == NULL) {

							pfree(sql);
							for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
							SPI_finish();

							for (k = 0; k < set_count; k++) {
								if (_rast[k] != NULL)
									rt_raster_destroy(_rast[k]);
								if (pgrastpos[k] != -1)
									PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
							}
							rt_raster_destroy(raster);

							elog(ERROR, "RASTER_mapAlgebra2: Could not allocate memory for prepared plan argtypes of expression parameter %d", spi_exprpos[i]);
							PG_RETURN_NULL();
						}

						/* specify datatypes of parameters */
						for (j = 0, k = 0; j < argkwcount; j++) {
							if (argpos[i][j] < 1) continue;

							/* positions are INT4 */
							if (
								(strstr(argkw[j], "[rast1.x]") != NULL) ||
								(strstr(argkw[j], "[rast1.y]") != NULL) ||
								(strstr(argkw[j], "[rast2.x]") != NULL) ||
								(strstr(argkw[j], "[rast2.y]") != NULL)
							) {
								argtype[k] = INT4OID;
							}
							/* everything else is FLOAT8 */
							else {
								argtype[k] = FLOAT8OID;
							}

							k++;
						}

						spi_plan[i] = SPI_prepare(sql, spi_argcount[i], argtype);
						pfree(argtype);

						if (spi_plan[i] == NULL) {

							pfree(sql);
							for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
							SPI_finish();

							for (k = 0; k < set_count; k++) {
								if (_rast[k] != NULL)
									rt_raster_destroy(_rast[k]);
								if (pgrastpos[k] != -1)
									PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
							}
							rt_raster_destroy(raster);

							elog(ERROR, "RASTER_mapAlgebra2: Could not create prepared plan of expression parameter %d", spi_exprpos[i]);
							PG_RETURN_NULL();
						}
					}
					/* no args, just execute query */
					else {
						err = SPI_execute(sql, TRUE, 0);
						if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {

							pfree(sql);
							for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
							SPI_finish();

							for (k = 0; k < set_count; k++) {
								if (_rast[k] != NULL)
									rt_raster_destroy(_rast[k]);
								if (pgrastpos[k] != -1)
									PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
							}
							rt_raster_destroy(raster);

							elog(ERROR, "RASTER_mapAlgebra2: Could not evaluate expression parameter %d", spi_exprpos[i]);
							PG_RETURN_NULL();
						}

						/* get output of prepared plan */
						tupdesc = SPI_tuptable->tupdesc;
						tuptable = SPI_tuptable;
						tuple = tuptable->vals[0];

						datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
						if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

							pfree(sql);
							if (SPI_tuptable) SPI_freetuptable(tuptable);
							for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
							SPI_finish();

							for (k = 0; k < set_count; k++) {
								if (_rast[k] != NULL)
									rt_raster_destroy(_rast[k]);
								if (pgrastpos[k] != -1)
									PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
							}
							rt_raster_destroy(raster);

							elog(ERROR, "RASTER_mapAlgebra2: Could not get result of expression parameter %d", spi_exprpos[i]);
							PG_RETURN_NULL();
						}

						if (!isnull) {
							hasargval[i] = 1;
							argval[i] = DatumGetFloat8(datum);
						}

						if (SPI_tuptable) SPI_freetuptable(tuptable);
					}

					pfree(sql);
				}
				else
					spi_empty++;
			}

			/* nodatanodataval */
			if (!PG_ARGISNULL(9)) {
				hasnodatanodataval = 1;
				nodatanodataval = PG_GETARG_FLOAT8(9);
			}
			else
				hasnodatanodataval = 0;
			break;
		}
		case REGPROCEDUREOID: {
			POSTGIS_RT_DEBUG(3, "arg 4 is \"userfunction\"!");
			if (!PG_ARGISNULL(4)) {

				ufc_nullcount = 0;
				ufc_noid = PG_GETARG_OID(4);

				/* get function info */
				fmgr_info(ufc_noid, &ufl_info);

				/* function cannot return set */
				err = 0;
				if (ufl_info.fn_retset) {
					err = 1;
				}
				/* function should have correct # of args */
				else if (ufl_info.fn_nargs < 3 || ufl_info.fn_nargs > 4) {
					err = 2;
				}

				/*
					TODO: consider adding checks of the userfunction parameters
						should be able to use get_fn_expr_argtype() of fmgr.c
				*/

				if (err > 0) {
					for (k = 0; k < set_count; k++) {
						if (_rast[k] != NULL)
							rt_raster_destroy(_rast[k]);
						if (pgrastpos[k] != -1)
							PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
					}
					rt_raster_destroy(raster);

					if (err > 1)
						elog(ERROR, "RASTER_mapAlgebra2: Function provided must have three or four input parameters");
					else
						elog(ERROR, "RASTER_mapAlgebra2: Function provided must return double precision not resultset");
					PG_RETURN_NULL();
				}

				if (func_volatile(ufc_noid) == 'v') {
					elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");
				}

				/* prep function call data */
#if POSTGIS_PGSQL_VERSION <= 90
				InitFunctionCallInfoData(ufc_info, &ufl_info, ufl_info.fn_nargs, InvalidOid, NULL);
#else
				InitFunctionCallInfoData(ufc_info, &ufl_info, ufl_info.fn_nargs, InvalidOid, NULL, NULL);
#endif
				memset(ufc_info.argnull, FALSE, sizeof(bool) * ufl_info.fn_nargs);

				if (ufl_info.fn_nargs != 4)
					k = 2;
				else
					k = 3;
				if (!PG_ARGISNULL(7)) {
					ufc_info.arg[k] = PG_GETARG_DATUM(7);
				}
				else {
				 ufc_info.arg[k] = (Datum) NULL;
				 ufc_info.argnull[k] = TRUE;
				 ufc_nullcount++;
				}
			}
			break;
		}
		default:
			for (k = 0; k < set_count; k++) {
				if (_rast[k] != NULL)
					rt_raster_destroy(_rast[k]);
				if (pgrastpos[k] != -1)
					PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
			}
			rt_raster_destroy(raster);
			elog(ERROR, "RASTER_mapAlgebra2: Invalid data type for expression or userfunction");
			PG_RETURN_NULL();
			break;
	}

	/* loop over pixels */
	/* if any expression present, run */
	if ((
		(calltype == TEXTOID) && (
			(spi_empty != spi_count) || hasnodatanodataval
		)
	) || (
		(calltype == REGPROCEDUREOID) && (ufc_noid != InvalidOid)
	)) {
		for (x = 0; x < dim[0]; x++) {
			for (y = 0; y < dim[1]; y++) {

				/* get pixel from each raster */
				for (i = 0; i < set_count; i++) {
					_haspixel[i] = 0;
					_pixel[i] = 0;

					/* row/column */
					_x = x - (int) _rastoffset[i][0];
					_y = y - (int) _rastoffset[i][1];

					/* store _x and _y in 1-based */
					_pos[i][0] = _x + 1;
					_pos[i][1] = _y + 1;

					/* get pixel value */
					if (_band[i] == NULL) {
						if (!_hasnodata[i]) {
							_haspixel[i] = 1;
							_pixel[i] = _nodataval[i];
						}
					}
					else if (
						!_isempty[i] &&
						(_x >= 0 && _x < _dim[i][0]) &&
						(_y >= 0 && _y < _dim[i][1])
					) {
						err = rt_band_get_pixel(_band[i], _x, _y, &(_pixel[i]), &isnodata);
						if (err != ES_NONE) {

							if (calltype == TEXTOID) {
								for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
								SPI_finish();
							}

							for (k = 0; k < set_count; k++) {
								if (_rast[k] != NULL)
									rt_raster_destroy(_rast[k]);
								if (pgrastpos[k] != -1)
									PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
							}
							rt_raster_destroy(raster);

							elog(ERROR, "RASTER_mapAlgebra2: Could not get pixel of %s raster", (i < 1 ? "FIRST" : "SECOND"));
							PG_RETURN_NULL();
						}

						if (!_hasnodata[i] || !isnodata)
							_haspixel[i] = 1;
					}

					POSTGIS_RT_DEBUGF(5, "pixel r%d(%d, %d) = %d, %f",
						i,
						_x, _y,
						_haspixel[i],
						_pixel[i]
					);
				}

				haspixel = 0;

				switch (calltype) {
					case TEXTOID: {
						/* which prepared plan to use? */
						/* !pixel0 && !pixel1 */
						/* use nodatanodataval */
						if (!_haspixel[0] && !_haspixel[1])
							i = 3;
						/* pixel0 && !pixel1 */
						/* run spi_plan[2] (nodata2expr) */
						else if (_haspixel[0] && !_haspixel[1])
							i = 2;
						/* !pixel0 && pixel1 */
						/* run spi_plan[1] (nodata1expr) */
						else if (!_haspixel[0] && _haspixel[1])
							i = 1;
						/* pixel0 && pixel1 */
						/* run spi_plan[0] (expression) */
						else
							i = 0;

						/* process values */
						if (i == 3) {
							if (hasnodatanodataval) {
								haspixel = 1;
								pixel = nodatanodataval;
							}
						}
						/* has an evaluated value */
						else if (hasargval[i]) {
							haspixel = 1;
							pixel = argval[i];
						}
						/* prepared plan exists */
						else if (spi_plan[i] != NULL) {
							POSTGIS_RT_DEBUGF(4, "Using prepared plan: %d", i);

							/* expression has argument(s) */
							if (spi_argcount[i]) {
								/* reset values to (Datum) NULL */
								memset(values, (Datum) NULL, sizeof(Datum) * argkwcount);
								/* reset nulls to FALSE */
								memset(nulls, FALSE, sizeof(bool) * argkwcount);

								/* set values and nulls */
								for (j = 0; j < argkwcount; j++) {
									idx = argpos[i][j];
									if (idx < 1) continue;
									idx--; /* 1-based becomes 0-based */

									if (strstr(argkw[j], "[rast1.x]") != NULL) {
										values[idx] = _pos[0][0];
									}
									else if (strstr(argkw[j], "[rast1.y]") != NULL) {
										values[idx] = _pos[0][1];
									}
									else if (
										(strstr(argkw[j], "[rast1.val]") != NULL) ||
										(strstr(argkw[j], "[rast1]") != NULL)
									) {
										if (_isempty[0] || !_haspixel[0])
											nulls[idx] = TRUE;
										else
											values[idx] = Float8GetDatum(_pixel[0]);
									}
									else if (strstr(argkw[j], "[rast2.x]") != NULL) {
										values[idx] = _pos[1][0];
									}
									else if (strstr(argkw[j], "[rast2.y]") != NULL) {
										values[idx] = _pos[1][1];
									}
									else if (
										(strstr(argkw[j], "[rast2.val]") != NULL) ||
										(strstr(argkw[j], "[rast2]") != NULL)
									) {
										if (_isempty[1] || !_haspixel[1])
											nulls[idx] = TRUE;
										else
											values[idx] = Float8GetDatum(_pixel[1]);
									}
								}
							}

							/* run prepared plan */
							err = SPI_execute_plan(spi_plan[i], values, nulls, TRUE, 1);
							if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {

								for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
								SPI_finish();

								for (k = 0; k < set_count; k++) {
									if (_rast[k] != NULL)
										rt_raster_destroy(_rast[k]);
									if (pgrastpos[k] != -1)
										PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
								}
								rt_raster_destroy(raster);

								elog(ERROR, "RASTER_mapAlgebra2: Unexpected error when running prepared statement %d", i);
								PG_RETURN_NULL();
							}

							/* get output of prepared plan */
							tupdesc = SPI_tuptable->tupdesc;
							tuptable = SPI_tuptable;
							tuple = tuptable->vals[0];

							datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
							if (SPI_result == SPI_ERROR_NOATTRIBUTE) {

								if (SPI_tuptable) SPI_freetuptable(tuptable);
								for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
								SPI_finish();

								for (k = 0; k < set_count; k++) {
									if (_rast[k] != NULL)
										rt_raster_destroy(_rast[k]);
									if (pgrastpos[k] != -1)
										PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
								}
								rt_raster_destroy(raster);

								elog(ERROR, "RASTER_mapAlgebra2: Could not get result of prepared statement %d", i);
								PG_RETURN_NULL();
							}

							if (!isnull) {
								haspixel = 1;
								pixel = DatumGetFloat8(datum);
							}

							if (SPI_tuptable) SPI_freetuptable(tuptable);
						}
					}	break;
					case REGPROCEDUREOID: {
						Datum d[4];
						ArrayType *a;

						/* build fcnarg */
						for (i = 0; i < set_count; i++) {
							ufc_info.arg[i] = Float8GetDatum(_pixel[i]);

							if (_haspixel[i]) {
								ufc_info.argnull[i] = FALSE;
								ufc_nullcount--;
							}
							else {
								ufc_info.argnull[i] = TRUE;
				 				ufc_nullcount++;
							}
						}

						/* function is strict and null parameter is passed */
						/* http://archives.postgresql.org/pgsql-general/2011-11/msg00424.php */
						if (ufl_info.fn_strict && ufc_nullcount)
							break;

						/* 4 parameters, add position */
						if (ufl_info.fn_nargs == 4) {
							/* Datum of 4 element array */
							/* array is (x1, y1, x2, y2) */
							for (i = 0; i < set_count; i++) {
								if (i < 1) {
									d[0] = Int32GetDatum(_pos[i][0]);
									d[1] = Int32GetDatum(_pos[i][1]);
								}
								else {
									d[2] = Int32GetDatum(_pos[i][0]);
									d[3] = Int32GetDatum(_pos[i][1]);
								}
							}

							a = construct_array(d, 4, INT4OID, sizeof(int32), true, 'i');
							ufc_info.arg[2] = PointerGetDatum(a);
							ufc_info.argnull[2] = FALSE;
						}

						datum = FunctionCallInvoke(&ufc_info);

						/* result is not null*/
						if (!ufc_info.isnull) {
							haspixel = 1;
							pixel = DatumGetFloat8(datum);
						}
					}	break;
				}

				/* burn pixel if haspixel != 0 */
				if (haspixel) {
					if (rt_band_set_pixel(band, x, y, pixel, NULL) != ES_NONE) {

						if (calltype == TEXTOID) {
							for (k = 0; k < spi_count; k++) SPI_freeplan(spi_plan[k]);
							SPI_finish();
						}

						for (k = 0; k < set_count; k++) {
							if (_rast[k] != NULL)
								rt_raster_destroy(_rast[k]);
							if (pgrastpos[k] != -1)
								PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
						}
						rt_raster_destroy(raster);

						elog(ERROR, "RASTER_mapAlgebra2: Could not set pixel value of output raster");
						PG_RETURN_NULL();
					}
				}

				POSTGIS_RT_DEBUGF(5, "(x, y, val) = (%d, %d, %f)", x, y, haspixel ? pixel : nodataval);

			} /* y: height */
		} /* x: width */
	}

	/* CLEANUP */
	if (calltype == TEXTOID) {
		for (i = 0; i < spi_count; i++) {
			if (spi_plan[i] != NULL) SPI_freeplan(spi_plan[i]);
		}
		SPI_finish();
	}

	for (k = 0; k < set_count; k++) {
		if (_rast[k] != NULL)
			rt_raster_destroy(_rast[k]);
		if (pgrastpos[k] != -1)
			PG_FREE_IF_COPY(pgrast[k], pgrastpos[k]);
	}

	pgrtn = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (!pgrtn) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "Finished RASTER_mapAlgebra2");

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/**
 * One raster neighborhood MapAlgebra
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebraFctNgb);
Datum RASTER_mapAlgebraFctNgb(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_pgraster *pgrtn = NULL;
    rt_raster raster = NULL;
    rt_raster newrast = NULL;
    rt_band band = NULL;
    rt_band newband = NULL;
    int x, y, nband, width, height, ngbwidth, ngbheight, winwidth, winheight, u, v, nIndex, nNullItems;
    double r, rpix;
    double newnodatavalue = 0.0;
    double newinitialvalue = 0.0;
    double newval = 0.0;
    rt_pixtype newpixeltype;
    int ret = -1;
    Oid oid;
    FmgrInfo cbinfo;
    FunctionCallInfoData cbdata;
    Datum tmpnewval;
    ArrayType * neighborDatum;
    char * strFromText = NULL;
    text * txtNodataMode = NULL;
    text * txtCallbackParam = NULL;
    int intReplace = 0;
    float fltReplace = 0;
    bool valuereplace = false, pixelreplace, nNodataOnly = true, nNullSkip = false;
    Datum * neighborData = NULL;
    bool * neighborNulls = NULL;
    int neighborDims[2];
    int neighborLbs[2];
    int16 typlen;
    bool typbyval;
    char typalign;

    POSTGIS_RT_DEBUG(2, "RASTER_mapAlgebraFctNgb: STARTING...");

    /* Check raster */
    if (PG_ARGISNULL(0)) {
        elog(WARNING, "Raster is NULL. Returning NULL");
        PG_RETURN_NULL();
    }


    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (NULL == raster)
    {
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not deserialize raster");
			PG_RETURN_NULL();    
    }

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Getting arguments...");

    /* Get the rest of the arguments */

    if (PG_ARGISNULL(1))
        nband = 1;
    else
        nband = PG_GETARG_INT32(1);

    if (nband < 1)
        nband = 1;
    
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Creating new empty raster...");

    /** 
     * Create a new empty raster with having the same georeference as the
     * provided raster
     **/
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);

    newrast = rt_raster_new(width, height);

    if ( NULL == newrast ) {
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not create a new raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(newrast, 
            rt_raster_get_x_scale(raster),
            rt_raster_get_y_scale(raster));

    rt_raster_set_offsets(newrast,
            rt_raster_get_x_offset(raster),
            rt_raster_get_y_offset(raster));

    rt_raster_set_skews(newrast,
            rt_raster_get_x_skew(raster),
            rt_raster_get_y_skew(raster));

    rt_raster_set_srid(newrast, rt_raster_get_srid(raster));            


    /**
     * If this new raster is empty (width = 0 OR height = 0) then there is
     * nothing to compute and we return it right now
     **/
    if (rt_raster_is_empty(newrast)) 
    { 
        elog(NOTICE, "Raster is empty. Returning an empty raster");
        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (!rt_raster_has_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);
    }

    /*
    * Get NODATA value
    */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        rt_band_get_nodata(band, &newnodatavalue);
    }

    else {
        newnodatavalue = rt_band_get_min_value(band);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: NODATA value for band: %f",
            newnodatavalue);
    /**
     * We set the initial value of the future band to nodata value. If nodata
     * value is null, then the raster will be initialized to
     * rt_band_get_min_value but all the values should be recomputed anyway
     **/
    newinitialvalue = newnodatavalue;

    /**
     * Set the new pixeltype
     **/    
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Setting pixeltype...");

    if (PG_ARGISNULL(2)) {
        newpixeltype = rt_band_get_pixtype(band);
    }

    else {
        strFromText = text_to_cstring(PG_GETARG_TEXT_P(2)); 
        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Pixeltype parameter: %s", strFromText);
        newpixeltype = rt_pixtype_index_from_name(strFromText);
        pfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }
    
    if (newpixeltype == PT_END) {

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFctNgb: Invalid pixeltype");
        PG_RETURN_NULL();
    }    
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Pixeltype set to %s (%d)",
        rt_pixtype_name(newpixeltype), newpixeltype);

    /* Get the name of the callback userfunction */
    if (PG_ARGISNULL(5)) {

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFctNgb: Required function is missing");
        PG_RETURN_NULL();
    }

    oid = PG_GETARG_OID(5);
    if (oid == InvalidOid) {

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFctNgb: Got invalid function object id");
        PG_RETURN_NULL();
    }

    fmgr_info(oid, &cbinfo);

    /* function cannot return set */
    if (cbinfo.fn_retset) {

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFctNgb: Function provided must return double precision not resultset");
        PG_RETURN_NULL();
    }
    /* function should have correct # of args */
    else if (cbinfo.fn_nargs != 3) {

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);
        rt_raster_destroy(newrast);

        elog(ERROR, "RASTER_mapAlgebraFctNgb: Function does not have three input parameters");
        PG_RETURN_NULL();
    }

    if (func_volatile(oid) == 'v') {
        elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");
    }

    /* prep function call data */
#if POSTGIS_PGSQL_VERSION <= 90
    InitFunctionCallInfoData(cbdata, &cbinfo, 3, InvalidOid, NULL);
#else
    InitFunctionCallInfoData(cbdata, &cbinfo, 3, InvalidOid, NULL, NULL);
#endif
    memset(cbdata.argnull, FALSE, sizeof(bool) * 3);

    /* check that the function isn't strict if the args are null. */
    if (PG_ARGISNULL(7)) {
        if (cbinfo.fn_strict) {

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);
            rt_raster_destroy(newrast);

            elog(ERROR, "RASTER_mapAlgebraFctNgb: Strict callback functions cannot have NULL parameters");
            PG_RETURN_NULL();
        }

        cbdata.arg[2] = (Datum)NULL;
        cbdata.argnull[2] = TRUE;
    }
    else {
        cbdata.arg[2] = PG_GETARG_DATUM(7);
    }

    /**
     * Optimization: If the raster is only filled with nodata values return
     * right now a raster filled with the nodatavalueexpr
     * TODO: Call rt_band_check_isnodata instead?
     **/
    if (rt_band_get_isnodata_flag(band)) {

        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Band is a nodata band, returning "
                "a raster filled with nodata");

        rt_raster_generate_new_band(newrast, newpixeltype,
                newinitialvalue, TRUE, newnodatavalue, 0);

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);               
    }


    /**
     * Create the raster receiving all the computed values. Initialize it to the
     * new initial value
     **/
    rt_raster_generate_new_band(newrast, newpixeltype,
            newinitialvalue, TRUE, newnodatavalue, 0);

    /* Get the new raster band */
    newband = rt_raster_get_band(newrast, 0);
    if ( NULL == newband ) {
        elog(NOTICE, "Could not modify band for new raster. Returning new "
            "raster with the original band");

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);      
    }

    /* Get the width of the neighborhood */
    if (PG_ARGISNULL(3) || PG_GETARG_INT32(3) <= 0) {
        elog(NOTICE, "Neighborhood width is NULL or <= 0. Returning new "
            "raster with the original band");

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);      
    }

    ngbwidth = PG_GETARG_INT32(3);
    winwidth = ngbwidth * 2 + 1;

    /* Get the height of the neighborhood */
    if (PG_ARGISNULL(4) || PG_GETARG_INT32(4) <= 0) {
        elog(NOTICE, "Neighborhood height is NULL or <= 0. Returning new "
            "raster with the original band");

        rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 0);

        /* Serialize created raster */
        pgrtn = rt_raster_serialize(newrast);
        rt_raster_destroy(newrast);
        if (NULL == pgrtn) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgrtn, pgrtn->size);
        PG_RETURN_POINTER(pgrtn);      
    }

    ngbheight = PG_GETARG_INT32(4);
    winheight = ngbheight * 2 + 1;

    /* Get the type of NODATA behavior for the neighborhoods. */
    if (PG_ARGISNULL(6)) {
        elog(NOTICE, "Neighborhood NODATA behavior defaulting to 'ignore'");
        txtNodataMode = cstring_to_text("ignore");
    }
    else {
        txtNodataMode = PG_GETARG_TEXT_P(6);
    }

    txtCallbackParam = (text*)palloc(VARSIZE(txtNodataMode));
    SET_VARSIZE(txtCallbackParam, VARSIZE(txtNodataMode));
    memcpy((void *)VARDATA(txtCallbackParam), (void *)VARDATA(txtNodataMode), VARSIZE(txtNodataMode) - VARHDRSZ);

    /* pass the nodata mode into the user function */
    cbdata.arg[1] = CStringGetDatum(txtCallbackParam);

    strFromText = text_to_cstring(txtNodataMode);
    strFromText = rtpg_strtoupper(strFromText);

    if (strcmp(strFromText, "VALUE") == 0)
        valuereplace = true;
    else if (strcmp(strFromText, "IGNORE") != 0 && strcmp(strFromText, "NULL") != 0) {
        /* if the text is not "IGNORE" or "NULL", it may be a numerical value */
        if (sscanf(strFromText, "%d", &intReplace) <= 0 && sscanf(strFromText, "%f", &fltReplace) <= 0) {
            /* the value is NOT an integer NOR a floating point */
            elog(NOTICE, "Neighborhood NODATA mode is not recognized. Must be one of 'value', 'ignore', "
                "'NULL', or a numeric value. Returning new raster with the original band");

            /* clean up the nodatamode string */
            pfree(txtCallbackParam);
            pfree(strFromText);

            rt_raster_destroy(raster);
            PG_FREE_IF_COPY(pgraster, 0);

            /* Serialize created raster */
            pgrtn = rt_raster_serialize(newrast);
            rt_raster_destroy(newrast);
            if (NULL == pgrtn) {
                elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster");
                PG_RETURN_NULL();
            }

            SET_VARSIZE(pgrtn, pgrtn->size);
            PG_RETURN_POINTER(pgrtn);      
        }
    }
    else if (strcmp(strFromText, "NULL") == 0) {
        /* this setting means that the neighborhood should be skipped if any of the values are null */
        nNullSkip = true;
    }
   
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Main computing loop (%d x %d)",
            width, height);

    /* Allocate room for the neighborhood. */
    neighborData = (Datum *)palloc(winwidth * winheight * sizeof(Datum));
    neighborNulls = (bool *)palloc(winwidth * winheight * sizeof(bool));

    /* The dimensions of the neighborhood array, for creating a multi-dimensional array. */
    neighborDims[0] = winwidth;
    neighborDims[1] = winheight;

    /* The lower bounds for the new multi-dimensional array. */
    neighborLbs[0] = 1;
    neighborLbs[1] = 1;

    /* Get information about the type of item in the multi-dimensional array (float8). */
    get_typlenbyvalalign(FLOAT8OID, &typlen, &typbyval, &typalign);

    for (x = 0 + ngbwidth; x < width - ngbwidth; x++) {
        for(y = 0 + ngbheight; y < height - ngbheight; y++) {
            /* populate an array with the pixel values in the neighborhood */
            nIndex = 0;
            nNullItems = 0;
            nNodataOnly = true;
            pixelreplace = false;
            if (valuereplace) {
                ret = rt_band_get_pixel(band, x, y, &rpix, NULL);
                if (ret == ES_NONE && FLT_NEQ(rpix, newnodatavalue)) {
                    pixelreplace = true;
                }
            }
            for (u = x - ngbwidth; u <= x + ngbwidth; u++) {
                for (v = y - ngbheight; v <= y + ngbheight; v++) {
                    ret = rt_band_get_pixel(band, u, v, &r, NULL);
                    if (ret == ES_NONE) {
                        if (FLT_NEQ(r, newnodatavalue)) {
                            /* If the pixel value for this neighbor cell is not NODATA */
                            neighborData[nIndex] = Float8GetDatum((double)r);
                            neighborNulls[nIndex] = false;
                            nNodataOnly = false;
                        }
                        else {
                            /* If the pixel value for this neighbor cell is NODATA */
                            if (valuereplace && pixelreplace) {
                                /* Replace the NODATA value with the currently processing pixel. */
                                neighborData[nIndex] = Float8GetDatum((double)rpix);
                                neighborNulls[nIndex] = false;
                                /* do not increment nNullItems, since the user requested that the  */
                                /* neighborhood replace NODATA values with the central pixel value */
                            }
                            else {
                                neighborData[nIndex] = PointerGetDatum(NULL);
                                neighborNulls[nIndex] = true;
                                nNullItems++;
                            }
                        }
                    }
                    else {
                        /* Fill this will NULL if we can't read the raster pixel. */
                        neighborData[nIndex] = PointerGetDatum(NULL);
                        neighborNulls[nIndex] = true;
                        nNullItems++;
                    }
                    /* Next neighbor position */
                    nIndex++;
                }
            }

            /**
             * We compute a value only for the withdata value neighborhood since the
             * nodata value has already been set by the first optimization
             **/
            if (!(nNodataOnly ||                     /* neighborhood only contains NODATA -- OR -- */
                (nNullSkip && nNullItems > 0) ||     /* neighborhood should skip any NODATA cells, and a NODATA cell was detected -- OR -- */
                (valuereplace && nNullItems > 0))) { /* neighborhood should replace NODATA cells with the central pixel value, and a NODATA cell was detected */
                POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: (%dx%d), %dx%d neighborhood",
                    x, y, winwidth, winheight);

                neighborDatum = construct_md_array((void *)neighborData, neighborNulls, 2, neighborDims, neighborLbs, 
                    FLOAT8OID, typlen, typbyval, typalign);

                /* Assign the neighbor matrix as the first argument to the user function */
                cbdata.arg[0] = PointerGetDatum(neighborDatum);

                /* Invoke the user function */
                tmpnewval = FunctionCallInvoke(&cbdata);

                /* Get the return value of the user function */
                if (cbdata.isnull) {
                    newval = newnodatavalue;
                }
                else {
                    newval = DatumGetFloat8(tmpnewval);
                }

                POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: new value = %f", 
                    newval);
                
                rt_band_set_pixel(newband, x, y, newval, NULL);
            }

            /* reset the number of null items in the neighborhood */
            nNullItems = 0;
        }
    }


    /* clean up */
    pfree(neighborNulls);
    pfree(neighborData);
    pfree(strFromText);
    pfree(txtCallbackParam);
    
    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: raster modified, serializing it.");
    /* Serialize created raster */

    pgrtn = rt_raster_serialize(newrast);
    rt_raster_destroy(newrast);
    if (NULL == pgrtn)
        PG_RETURN_NULL();

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: raster serialized");
    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraFctNgb: returning raster");
    
    SET_VARSIZE(pgrtn, pgrtn->size);    
    PG_RETURN_POINTER(pgrtn);
}

/* ---------------------------------------------------------------- */
/*  n-raster MapAlgebra                                             */
/* ---------------------------------------------------------------- */

typedef struct {
	Oid ufc_noid;
	FmgrInfo ufl_info;
	FunctionCallInfoData ufc_info;
	int ufc_nullcount;
} rtpg_nmapalgebra_callback_arg;

typedef struct rtpg_nmapalgebra_arg_t *rtpg_nmapalgebra_arg;
struct rtpg_nmapalgebra_arg_t {
	int numraster;
	rt_pgraster **pgraster;
	rt_raster *raster;
	uint8_t *isempty; /* flag indicating if raster is empty */
	uint8_t *ownsdata; /* is the raster self owned or just a pointer to another raster */
	int *nband; /* source raster's band index, 0-based */
	uint8_t *hasband; /* does source raster have band at index nband? */

	rt_pixtype pixtype; /* output raster band's pixel type */
	int hasnodata; /* NODATA flag */
	double nodataval; /* NODATA value */

	int distance[2]; /* distance in X and Y axis */

	rt_extenttype extenttype; /* ouput raster's extent type */
	rt_pgraster *pgcextent; /* custom extent of type rt_pgraster */
	rt_raster cextent; /* custom extent of type rt_raster */

	rtpg_nmapalgebra_callback_arg	callback;
};

static rtpg_nmapalgebra_arg rtpg_nmapalgebra_arg_init() {
	rtpg_nmapalgebra_arg arg = NULL;

	arg = palloc(sizeof(struct rtpg_nmapalgebra_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_nmapalgebra_arg_init: Could not allocate memory for arguments");
		return NULL;
	}

	arg->numraster = 0;
	arg->pgraster = NULL;
	arg->raster = NULL;
	arg->isempty = NULL;
	arg->ownsdata = NULL;
	arg->nband = NULL;
	arg->hasband = NULL;

	arg->pixtype = PT_END;
	arg->hasnodata = 1;
	arg->nodataval = 0;

	arg->distance[0] = 0;
	arg->distance[1] = 0;

	arg->extenttype = ET_INTERSECTION;
	arg->pgcextent = NULL;
	arg->cextent = NULL;

	arg->callback.ufc_noid = InvalidOid;
	arg->callback.ufc_nullcount = 0;

	return arg;
}

static void rtpg_nmapalgebra_arg_destroy(rtpg_nmapalgebra_arg arg) {
	int i = 0;

	if (arg->raster != NULL) {
		for (i = 0; i < arg->numraster; i++) {
			if (arg->raster[i] == NULL || !arg->ownsdata[i])
				continue;

			rt_raster_destroy(arg->raster[i]);
		}

		pfree(arg->raster);
		pfree(arg->pgraster);
		pfree(arg->isempty);
		pfree(arg->ownsdata);
		pfree(arg->nband);
	}

	if (arg->cextent != NULL)
		rt_raster_destroy(arg->cextent);

	pfree(arg);
}

static int rtpg_nmapalgebra_rastbandarg_process(rtpg_nmapalgebra_arg arg, ArrayType *array, int *allnull, int *allempty, int *noband) {
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;

	HeapTupleHeader tup;
	bool isnull;
	Datum tupv;

	int i;
	int j;
	int nband;

	if (arg == NULL || array == NULL) {
		elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: NULL values not permitted for parameters");
		return 0;
	}

	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(
		array,
		etype,
		typlen, typbyval, typalign,
		&e, &nulls, &n
	);

	if (!n) {
		elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Invalid argument for rastbandarg");
		return 0;
	}

	/* prep arg */
	arg->numraster = n;
	arg->pgraster = palloc(sizeof(rt_pgraster *) * arg->numraster);
	arg->raster = palloc(sizeof(rt_raster) * arg->numraster);
	arg->isempty = palloc(sizeof(uint8_t) * arg->numraster);
	arg->ownsdata = palloc(sizeof(uint8_t) * arg->numraster);
	arg->nband = palloc(sizeof(int) * arg->numraster);
	arg->hasband = palloc(sizeof(uint8_t) * arg->numraster);
	if (
		arg->pgraster == NULL ||
		arg->raster == NULL ||
		arg->isempty == NULL ||
		arg->ownsdata == NULL ||
		arg->nband == NULL ||
		arg->hasband == NULL
	) {
		elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Could not allocate memory for processing rastbandarg");
		return 0;
	}

	*allnull = 0;
	*allempty = 0;
	*noband = 0;

	/* process each element */
	for (i = 0; i < n; i++) {
		if (nulls[i]) {
			arg->numraster--;
			continue;
		}

		POSTGIS_RT_DEBUGF(4, "Processing rastbandarg at index %d", i);

		arg->raster[i] = NULL;
		arg->isempty[i] = 0;
		arg->ownsdata[i] = 1;
		arg->nband[i] = 0;
		arg->hasband[i] = 0;

		/* each element is a tuple */
		tup = (HeapTupleHeader) DatumGetPointer(e[i]);
		if (NULL == tup) {
			elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Invalid argument for rastbandarg at index %d", i);
			return 0;
		}

		/* first element, raster */
		POSTGIS_RT_DEBUG(4, "Processing first element (raster)");
		tupv = GetAttributeByName(tup, "rast", &isnull);
		if (isnull) {
			elog(NOTICE, "First argument (nband) of rastbandarg at index %d is NULL. Assuming NULL raster", i);
			arg->isempty[i] = 1;
			arg->ownsdata[i] = 0;

			(*allnull)++;
			(*allempty)++;
			(*noband)++;

			continue;
		}

		arg->pgraster[i] = (rt_pgraster *) PG_DETOAST_DATUM(tupv);

		/* see if this is a copy of an existing pgraster */
		for (j = 0; j < i; j++) {
			if (!arg->isempty[j] && (arg->pgraster[i] == arg->pgraster[j])) {
				POSTGIS_RT_DEBUG(4, "raster matching existing same raster found");
				arg->raster[i] = arg->raster[j];
				arg->ownsdata[i] = 0;
				break;
			}
		}

		if (arg->ownsdata[i]) {
			POSTGIS_RT_DEBUG(4, "deserializing raster");
			arg->raster[i] = rt_raster_deserialize(arg->pgraster[i], FALSE);
			if (arg->raster[i] == NULL) {
				elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Could not deserialize raster at index %d", i);
				return 0;
			}
		}

		/* is raster empty? */
		arg->isempty[i] = rt_raster_is_empty(arg->raster[i]);
		if (arg->isempty[i]) {
			(*allempty)++;
			(*noband)++;

			continue;
		}

		/* second element, nband */
		POSTGIS_RT_DEBUG(4, "Processing second element (nband)");
		tupv = GetAttributeByName(tup, "nband", &isnull);
		if (isnull) {
			nband = 1;
			elog(NOTICE, "First argument (nband) of rastbandarg at index %d is NULL. Assuming nband = %d", i, nband);
		}
		else
			nband = DatumGetInt32(tupv);

		if (nband < 1) {
			elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Band number provided for rastbandarg at index %d must be greater than zero (1-based)", i);
			return 0;
		}

		arg->nband[i] = nband - 1;
		arg->hasband[i] = rt_raster_has_band(arg->raster[i], arg->nband[i]);
		if (!arg->hasband[i]) {
			(*noband)++;
			POSTGIS_RT_DEBUGF(4, "Band at index %d not found in raster", nband);
		}
	}

	if (arg->numraster < n) {
		arg->pgraster = repalloc(arg->pgraster, sizeof(rt_pgraster *) * arg->numraster);
		arg->raster = repalloc(arg->raster, sizeof(rt_raster) * arg->numraster);
		arg->isempty = repalloc(arg->isempty, sizeof(uint8_t) * arg->numraster);
		arg->ownsdata = repalloc(arg->ownsdata, sizeof(uint8_t) * arg->numraster);
		arg->nband = repalloc(arg->nband, sizeof(int) * arg->numraster);
		arg->hasband = repalloc(arg->hasband, sizeof(uint8_t) * arg->numraster);
		if (
			arg->pgraster == NULL ||
			arg->raster == NULL ||
			arg->isempty == NULL ||
			arg->ownsdata == NULL ||
			arg->nband == NULL ||
			arg->hasband == NULL
		) {
			elog(ERROR, "rtpg_nmapalgebra_rastbandarg_process: Could not reallocate memory for processed rastbandarg");
			return 0;
		}
	}

	POSTGIS_RT_DEBUGF(4, "arg->numraster = %d", arg->numraster);

	return 1;
}

/*
	Callback for RASTER_nMapAlgebra
*/
static int rtpg_nmapalgebra_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	rtpg_nmapalgebra_callback_arg *callback = (rtpg_nmapalgebra_callback_arg *) userarg;

	int16 typlen;
	bool typbyval;
	char typalign;

	ArrayType *mdValues = NULL;
	Datum *_values = NULL;
	bool *_nodata = NULL;

	ArrayType *mdPos = NULL;
	Datum *_pos = NULL;
	bool *_null = NULL;

	int i = 0;
	int x = 0;
	int y = 0;
	int z = 0;
	int dim[3] = {0};
	int lbound[3] = {1, 1, 1};
	Datum datum = (Datum) NULL;

	if (arg == NULL)
		return 0;

	*value = 0;
	*nodata = 0;

	dim[0] = arg->rasters;
	dim[1] = arg->rows;
	dim[2] = arg->columns;

	_values = palloc(sizeof(Datum) * arg->rasters * arg->rows * arg->columns);
	_nodata = palloc(sizeof(bool) * arg->rasters * arg->rows * arg->columns);
	if (_values == NULL || _nodata == NULL) {
		elog(ERROR, "rtpg_nmapalgebra_callback: Could not allocate memory for values array");
		return 0;
	}

	/* build mdValues */
	i = 0;
	/* raster */
	for (z = 0; z < arg->rasters; z++) {
		/* Y axis */
		for (y = 0; y < arg->rows; y++) {
			/* X axis */
			for (x = 0; x < arg->columns; x++) {
				POSTGIS_RT_DEBUGF(4, "(z, y ,x) = (%d, %d, %d)", z, y, x);
				POSTGIS_RT_DEBUGF(4, "(value, nodata) = (%f, %d)", arg->values[z][y][x], arg->nodata[z][y][x]);

				_nodata[i] = (bool) arg->nodata[z][y][x];
				if (!_nodata[i])
					_values[i] = Float8GetDatum(arg->values[z][y][x]);
				else
					_values[i] = (Datum) NULL;

				i++;
			}
		}
	}

	/* info about the type of item in the multi-dimensional array (float8). */
	get_typlenbyvalalign(FLOAT8OID, &typlen, &typbyval, &typalign);

	/* construct mdValues */
	mdValues = construct_md_array(
		_values, _nodata,
		3, dim, lbound,
		FLOAT8OID,
		typlen, typbyval, typalign
	);
	pfree(_nodata);
	pfree(_values);

	_pos = palloc(sizeof(Datum) * (arg->rasters + 1) * 2);
	_null = palloc(sizeof(bool) * (arg->rasters + 1) * 2);
	if (_pos == NULL || _null == NULL) {
		pfree(mdValues);
		elog(ERROR, "rtpg_nmapalgebra_callback: Could not allocate memory for position array");
		return 0;
	}
	memset(_null, 0, sizeof(bool) * (arg->rasters + 1) * 2);

	/* build mdPos */
	i = 0;
	_pos[i] = arg->dst_pixel[0] + 1;
	i++;
	_pos[i] = arg->dst_pixel[1] + 1;
	i++;

	for (z = 0; z < arg->rasters; z++) {
		_pos[i] = arg->src_pixel[z][0] + 1;
		i++;

		_pos[i] = arg->src_pixel[z][1] + 1;
		i++;
	}

	/* info about the type of item in the multi-dimensional array (int4). */
	get_typlenbyvalalign(INT4OID, &typlen, &typbyval, &typalign);

	/* reuse dim and lbound, just tweak to what we need */
	dim[0] = arg->rasters + 1;
	dim[1] = 2;
	lbound[0] = 0;

	/* construct mdPos */
	mdPos = construct_md_array(
		_pos, _null,
		2, dim, lbound,
		INT4OID,
		typlen, typbyval, typalign
	);
	pfree(_pos);
	pfree(_null);

	callback->ufc_info.arg[0] = PointerGetDatum(mdValues);
	callback->ufc_info.arg[1] = PointerGetDatum(mdPos);

	/* function is strict and null parameter is passed */
	/* http://archives.postgresql.org/pgsql-general/2011-11/msg00424.php */
	if (callback->ufl_info.fn_strict && callback->ufc_nullcount) {
		*nodata = 1;

		pfree(mdValues);
		pfree(mdPos);

		return 1;
	}

	/* call user callback function */
	datum = FunctionCallInvoke(&(callback->ufc_info));
	pfree(mdValues);
	pfree(mdPos);

	/* result is not null*/
	if (!callback->ufc_info.isnull)
		*value = DatumGetFloat8(datum);
	else
		*nodata = 1;

	return 1;
}

/*
 ST_MapAlgebra for n rasters
*/
PG_FUNCTION_INFO_V1(RASTER_nMapAlgebra);
Datum RASTER_nMapAlgebra(PG_FUNCTION_ARGS)
{
	rtpg_nmapalgebra_arg arg = NULL;
	rt_iterator itrset;
	int i = 0;
	int noerr = 0;
	int allnull = 0;
	int allempty = 0;
	int noband = 0;

	rt_raster raster = NULL;
	rt_band band = NULL;
	rt_pgraster *pgraster = NULL;

	POSTGIS_RT_DEBUG(3, "Starting...");

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* init argument struct */
	arg = rtpg_nmapalgebra_arg_init();
	if (arg == NULL) {
		elog(ERROR, "RASTER_nMapAlgebra: Could not initialize argument structure");
		PG_RETURN_NULL();
	}

	/* let helper function process rastbandarg (0) */
	if (!rtpg_nmapalgebra_rastbandarg_process(arg, PG_GETARG_ARRAYTYPE_P(0), &allnull, &allempty, &noband)) {
		rtpg_nmapalgebra_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebra: Could not process rastbandarg");
		PG_RETURN_NULL();
	}

	POSTGIS_RT_DEBUGF(4, "allnull, allempty, noband = %d, %d, %d", allnull, allempty, noband);

	/* all rasters are NULL, return NULL */
	if (allnull == arg->numraster) {
		elog(NOTICE, "All input rasters are NULL. Returning NULL");
		rtpg_nmapalgebra_arg_destroy(arg);
		PG_RETURN_NULL();
	}

	/* pixel type (2) */
	if (!PG_ARGISNULL(2)) {
		char *pixtypename = text_to_cstring(PG_GETARG_TEXT_P(2));

		/* Get the pixel type index */
		arg->pixtype = rt_pixtype_index_from_name(pixtypename);
		if (arg->pixtype == PT_END) {
			rtpg_nmapalgebra_arg_destroy(arg);
			elog(ERROR, "RASTER_nMapAlgebra: Invalid pixel type: %s", pixtypename);
			PG_RETURN_NULL();
		}
	}

	/* distancex (3) */
	if (!PG_ARGISNULL(3))
		arg->distance[0] = PG_GETARG_INT32(3);
	/* distancey (4) */
	if (!PG_ARGISNULL(4))
		arg->distance[1] = PG_GETARG_INT32(4);

	if (arg->distance[0] < 0 || arg->distance[1] < 0) {
		rtpg_nmapalgebra_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebra: Distance for X and Y axis must be greater than or equal to zero");
		PG_RETURN_NULL();
	}

	/* extent type (5) */
	if (!PG_ARGISNULL(5)) {
		char *extenttypename = rtpg_strtoupper(rtpg_trim(text_to_cstring(PG_GETARG_TEXT_P(5))));
		arg->extenttype = rt_util_extent_type(extenttypename);
	}
	POSTGIS_RT_DEBUGF(4, "extenttype: %d", arg->extenttype);

	/* custom extent (6) */
	if (arg->extenttype == ET_CUSTOM) {
		if (PG_ARGISNULL(6)) {
			elog(NOTICE, "Custom extent is NULL. Returning NULL");
			rtpg_nmapalgebra_arg_destroy(arg);
			PG_RETURN_NULL();
		}

		arg->pgcextent = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(6));

		/* only need the raster header */
		arg->cextent = rt_raster_deserialize(arg->pgcextent, TRUE);
		if (arg->cextent == NULL) {
			rtpg_nmapalgebra_arg_destroy(arg);
			elog(ERROR, "RASTER_nMapAlgebra: Could not deserialize custom extent");
			PG_RETURN_NULL();
		}
		else if (rt_raster_is_empty(arg->cextent)) {
			elog(NOTICE, "Custom extent is an empty raster. Returning empty raster");
			rtpg_nmapalgebra_arg_destroy(arg);

			raster = rt_raster_new(0, 0);
			if (raster == NULL) {
				elog(ERROR, "RASTER_nMapAlgebra: Could not create empty raster");
				PG_RETURN_NULL();
			}

			pgraster = rt_raster_serialize(raster);
			rt_raster_destroy(raster);
			if (!pgraster) PG_RETURN_NULL();

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}
	}

	noerr = 1;
	/* all rasters are empty, return empty raster */
	if (allempty == arg->numraster) {
		elog(NOTICE, "All input rasters are empty. Returning empty raster");
		noerr = 0;
	}
	/* all rasters don't have indicated band, return empty raster */
	else if (noband == arg->numraster) {
		elog(NOTICE, "All input rasters do not have bands at indicated indexes. Returning empty raster");
		noerr = 0;
	}
	if (!noerr) {
		rtpg_nmapalgebra_arg_destroy(arg);

		raster = rt_raster_new(0, 0);
		if (raster == NULL) {
			elog(ERROR, "RASTER_nMapAlgebra: Could not create empty raster");
			PG_RETURN_NULL();
		}

		pgraster = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgraster) PG_RETURN_NULL();

		SET_VARSIZE(pgraster, pgraster->size);
		PG_RETURN_POINTER(pgraster);
	}

	/* do regprocedure last (1) */
	if (!PG_ARGISNULL(1) || get_fn_expr_argtype(fcinfo->flinfo, 1) == REGPROCEDUREOID) {
		POSTGIS_RT_DEBUG(4, "processing callbackfunc");
		arg->callback.ufc_noid = PG_GETARG_OID(1);

		/* get function info */
		fmgr_info(arg->callback.ufc_noid, &(arg->callback.ufl_info));

		/* function cannot return set */
		noerr = 0;
		if (arg->callback.ufl_info.fn_retset) {
			noerr = 1;
		}
		/* function should have correct # of args */
		else if (arg->callback.ufl_info.fn_nargs != 3) {
			noerr = 2;
		}

		/*
			TODO: consider adding checks of the userfunction parameters
				should be able to use get_fn_expr_argtype() of fmgr.c
		*/

		if (noerr > 0) {
			rtpg_nmapalgebra_arg_destroy(arg);
			if (noerr > 1)
				elog(ERROR, "RASTER_nMapAlgebra: Function provided must have three input parameters");
			else
				elog(ERROR, "RASTER_nMapAlgebra: Function provided must return double precision, not resultset");
			PG_RETURN_NULL();
		}

		if (func_volatile(arg->callback.ufc_noid) == 'v')
			elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");

		/* prep function call data */
#if POSTGIS_PGSQL_VERSION > 90
		InitFunctionCallInfoData(arg->callback.ufc_info, &(arg->callback.ufl_info), arg->callback.ufl_info.fn_nargs, InvalidOid, NULL, NULL);
#else
		InitFunctionCallInfoData(arg->callback.ufc_info, &(arg->callback.ufl_info), arg->callback.ufl_info.fn_nargs, NULL, NULL);
#endif
		memset(arg->callback.ufc_info.argnull, FALSE, sizeof(bool) * arg->callback.ufl_info.fn_nargs);

		/* userargs (7) */
		if (!PG_ARGISNULL(7))
			arg->callback.ufc_info.arg[2] = PG_GETARG_DATUM(7);
		else {
			arg->callback.ufc_info.arg[2] = (Datum) NULL;
			arg->callback.ufc_info.argnull[2] = TRUE;
			arg->callback.ufc_nullcount++;
		}
	}
	else {
		rtpg_nmapalgebra_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebra: callbackfunc must be provided");
		PG_RETURN_NULL();
	}

	/* determine nodataval and possibly pixtype */
	/* band to check */
	switch (arg->extenttype) {
		case ET_LAST:
			i = arg->numraster - 1;
			break;
		case ET_SECOND:
			if (arg->numraster > 1) {
				i = 1;
				break;
			}
		default:
			i = 0;
			break;
	}
	/* find first viable band */
	if (!arg->hasband[i]) {
		for (i = 0; i < arg->numraster; i++) {
			if (arg->hasband[i])
				break;
		}
		if (i >= arg->numraster)
			i = arg->numraster - 1;
	}
	band = rt_raster_get_band(arg->raster[i], arg->nband[i]);

	/* set pixel type if PT_END */
	if (arg->pixtype == PT_END)
		arg->pixtype = rt_band_get_pixtype(band);

	/* set hasnodata and nodataval */
	arg->hasnodata = 1;
	if (rt_band_get_hasnodata_flag(band))
		rt_band_get_nodata(band, &(arg->nodataval));
	else
		arg->nodataval = rt_band_get_min_value(band);

	POSTGIS_RT_DEBUGF(4, "pixtype, hasnodata, nodataval: %s, %d, %f", rt_pixtype_name(arg->pixtype), arg->hasnodata, arg->nodataval);

	/* init itrset */
	itrset = palloc(sizeof(struct rt_iterator_t) * arg->numraster);
	if (itrset == NULL) {
		rtpg_nmapalgebra_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebra: Could not allocate memory for iterator arguments");
		PG_RETURN_NULL();
	}

	/* set itrset */
	for (i = 0; i < arg->numraster; i++) {
		itrset[i].raster = arg->raster[i];
		itrset[i].nband = arg->nband[i];
		itrset[i].nbnodata = 1;
	}

	/* pass everything to iterator */
	noerr = rt_raster_iterator(
		itrset, arg->numraster,
		arg->extenttype, arg->cextent,
		arg->pixtype,
		arg->hasnodata, arg->nodataval,
		arg->distance[0], arg->distance[1],
		&(arg->callback),
		rtpg_nmapalgebra_callback,
		&raster
	);

	/* cleanup */
	pfree(itrset);
	rtpg_nmapalgebra_arg_destroy(arg);

	if (noerr != ES_NONE) {
		elog(ERROR, "RASTER_nMapAlgebra: Could not run raster iterator function");
		PG_RETURN_NULL();
	}
	else if (raster == NULL)
		PG_RETURN_NULL();

	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);

	POSTGIS_RT_DEBUG(3, "Finished");

	if (!pgraster)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	PG_RETURN_POINTER(pgraster);
}

/* ---------------------------------------------------------------- */
/* expression ST_MapAlgebra for n rasters                           */
/* ---------------------------------------------------------------- */

typedef struct {
	int exprcount;

	struct {
		SPIPlanPtr spi_plan;
		uint32_t spi_argcount;
		uint8_t *spi_argpos;

		int hasval;
		double val;
	} expr[3];

	struct {
		int hasval;
		double val;
	} nodatanodata;

	struct {
		int count;
		char **val;
	} kw;

} rtpg_nmapalgebraexpr_callback_arg;

typedef struct rtpg_nmapalgebraexpr_arg_t *rtpg_nmapalgebraexpr_arg;
struct rtpg_nmapalgebraexpr_arg_t {
	rtpg_nmapalgebra_arg bandarg;

	rtpg_nmapalgebraexpr_callback_arg	callback;
};

static rtpg_nmapalgebraexpr_arg rtpg_nmapalgebraexpr_arg_init(int cnt, char **kw) {
	rtpg_nmapalgebraexpr_arg arg = NULL;
	int i = 0;

	arg = palloc(sizeof(struct rtpg_nmapalgebraexpr_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_nmapalgebraexpr_arg_init: Could not allocate memory for arguments");
		return NULL;
	}

	arg->bandarg = rtpg_nmapalgebra_arg_init();
	if (arg->bandarg == NULL) {
		elog(ERROR, "rtpg_nmapalgebraexpr_arg_init: Could not allocate memory for arg->bandarg");
		return NULL;
	}

	arg->callback.kw.count = cnt;
	arg->callback.kw.val = kw;

	arg->callback.exprcount = 3;
	for (i = 0; i < arg->callback.exprcount; i++) {
		arg->callback.expr[i].spi_plan = NULL;
		arg->callback.expr[i].spi_argcount = 0;
		arg->callback.expr[i].spi_argpos = palloc(cnt * sizeof(uint8_t));
		if (arg->callback.expr[i].spi_argpos == NULL) {
			elog(ERROR, "rtpg_nmapalgebraexpr_arg_init: Could not allocate memory for spi_argpos");
			return NULL;
		}
		memset(arg->callback.expr[i].spi_argpos, 0, sizeof(uint8_t) * cnt);
		arg->callback.expr[i].hasval = 0;
		arg->callback.expr[i].val = 0;
	}

	arg->callback.nodatanodata.hasval = 0;
	arg->callback.nodatanodata.val = 0;

	return arg;
}

static void rtpg_nmapalgebraexpr_arg_destroy(rtpg_nmapalgebraexpr_arg arg) {
	int i = 0;

	rtpg_nmapalgebra_arg_destroy(arg->bandarg);

	for (i = 0; i < arg->callback.exprcount; i++) {
		if (arg->callback.expr[i].spi_plan)
			SPI_freeplan(arg->callback.expr[i].spi_plan);
		if (arg->callback.kw.count)
			pfree(arg->callback.expr[i].spi_argpos);
	}

	pfree(arg);
}

static int rtpg_nmapalgebraexpr_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	rtpg_nmapalgebraexpr_callback_arg *callback = (rtpg_nmapalgebraexpr_callback_arg *) userarg;
	SPIPlanPtr plan = NULL;
	int i = 0;
	int id = -1;

	if (arg == NULL)
		return 0;

	*value = 0;
	*nodata = 0;

	/* 2 raster */
	if (arg->rasters > 1) {
		/* nodata1 = 1 AND nodata2 = 1, nodatanodataval */
		if (arg->nodata[0][0][0] && arg->nodata[1][0][0]) {
			if (callback->nodatanodata.hasval)
				*value = callback->nodatanodata.val;
			else
				*nodata = 1;
		}
		/* nodata1 = 1 AND nodata2 != 1, nodata1expr */
		else if (arg->nodata[0][0][0] && !arg->nodata[1][0][0]) {
			id = 1;
			if (callback->expr[id].hasval)
				*value = callback->expr[id].val;
			else if (callback->expr[id].spi_plan)
				plan = callback->expr[id].spi_plan;
			else
				*nodata = 1;
		}
		/* nodata1 != 1 AND nodata2 = 1, nodata2expr */
		else if (!arg->nodata[0][0][0] && arg->nodata[1][0][0]) {
			id = 2;
			if (callback->expr[id].hasval)
				*value = callback->expr[id].val;
			else if (callback->expr[id].spi_plan)
				plan = callback->expr[id].spi_plan;
			else
				*nodata = 1;
		}
		/* expression */
		else {
			id = 0;
			if (callback->expr[id].hasval)
				*value = callback->expr[id].val;
			else if (callback->expr[id].spi_plan)
				plan = callback->expr[id].spi_plan;
			else {
				if (callback->nodatanodata.hasval)
					*value = callback->nodatanodata.val;
				else
					*nodata = 1;
			}
		}
	}
	/* 1 raster */
	else {
		/* nodata = 1, nodata1expr */
		if (arg->nodata[0][0][0]) {
			id = 1;
			if (callback->expr[id].hasval)
				*value = callback->expr[id].val;
			else if (callback->expr[id].spi_plan)
				plan = callback->expr[id].spi_plan;
			else
				*nodata = 1;
		}
		/* expression */
		else {
			id = 0;
			if (callback->expr[id].hasval)
				*value = callback->expr[id].val;
			else if (callback->expr[id].spi_plan)
				plan = callback->expr[id].spi_plan;
			else {
				/* see if nodata1expr is available */
				id = 1;
				if (callback->expr[id].hasval)
					*value = callback->expr[id].val;
				else if (callback->expr[id].spi_plan)
					plan = callback->expr[id].spi_plan;
				else
					*nodata = 1;
			}
		}
	}

	/* run prepared plan */
	if (plan != NULL) {
		Datum values[12];
		bool nulls[12];
		int err = 0;

		TupleDesc tupdesc;
		SPITupleTable *tuptable = NULL;
		HeapTuple tuple;
		Datum datum;
		bool isnull = FALSE;

		POSTGIS_RT_DEBUGF(4, "Running plan %d", id);

		/* init values and nulls */
		memset(values, (Datum) NULL, sizeof(Datum) * callback->kw.count);
		memset(nulls, FALSE, sizeof(bool) * callback->kw.count);

		if (callback->expr[id].spi_argcount) {
			int idx = 0;

			for (i = 0; i < callback->kw.count; i++) {
				idx = callback->expr[id].spi_argpos[i];
				if (idx < 1) continue;
				idx--; /* 1-based now 0-based */

				switch (i) {
					/* [rast.x] */
					case 0:
						values[idx] = Int32GetDatum(arg->src_pixel[0][0] + 1);
						break;
					/* [rast.y] */
					case 1:
						values[idx] = Int32GetDatum(arg->src_pixel[0][1] + 1);
						break;
					/* [rast.val] */
					case 2:
					/* [rast] */
					case 3:
						if (!arg->nodata[0][0][0])
							values[idx] = Float8GetDatum(arg->values[0][0][0]);
						else
							nulls[idx] = TRUE;
						break;

					/* [rast1.x] */
					case 4:
						values[idx] = Int32GetDatum(arg->src_pixel[0][0] + 1);
						break;
					/* [rast1.y] */
					case 5:
						values[idx] = Int32GetDatum(arg->src_pixel[0][1] + 1);
						break;
					/* [rast1.val] */
					case 6:
					/* [rast1] */
					case 7:
						if (!arg->nodata[0][0][0])
							values[idx] = Float8GetDatum(arg->values[0][0][0]);
						else
							nulls[idx] = TRUE;
						break;

					/* [rast2.x] */
					case 8:
						values[idx] = Int32GetDatum(arg->src_pixel[1][0] + 1);
						break;
					/* [rast2.y] */
					case 9:
						values[idx] = Int32GetDatum(arg->src_pixel[1][1] + 1);
						break;
					/* [rast2.val] */
					case 10:
					/* [rast2] */
					case 11:
						if (!arg->nodata[1][0][0])
							values[idx] = Float8GetDatum(arg->values[1][0][0]);
						else
							nulls[idx] = TRUE;
						break;
				}

			}
		}

		/* run prepared plan */
		err = SPI_execute_plan(plan, values, nulls, TRUE, 1);
		if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
			elog(ERROR, "rtpg_nmapalgebraexpr_callback: Unexpected error when running prepared statement %d", id);
			return 0;
		}

		/* get output of prepared plan */
		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
		if (SPI_result == SPI_ERROR_NOATTRIBUTE) {
			if (SPI_tuptable) SPI_freetuptable(tuptable);
			elog(ERROR, "rtpg_nmapalgebraexpr_callback: Could not get result of prepared statement %d", id);
			return 0;
		}

		if (!isnull) {
			*value = DatumGetFloat8(datum);
			POSTGIS_RT_DEBUG(4, "Getting value from Datum");
		}
		else {
			/* 2 raster, check nodatanodataval */
			if (arg->rasters > 1) {
				if (callback->nodatanodata.hasval)
					*value = callback->nodatanodata.val;
				else
					*nodata = 1;
			}
			/* 1 raster, check nodataval */
			else {
				if (callback->expr[1].hasval)
					*value = callback->expr[1].val;
				else
					*nodata = 1;
			}
		}

		if (SPI_tuptable) SPI_freetuptable(tuptable);
	}

	POSTGIS_RT_DEBUGF(4, "(value, nodata) = (%f, %d)", *value, *nodata);
	return 1;
}

PG_FUNCTION_INFO_V1(RASTER_nMapAlgebraExpr);
Datum RASTER_nMapAlgebraExpr(PG_FUNCTION_ARGS)
{
	MemoryContext mainMemCtx = CurrentMemoryContext;
	rtpg_nmapalgebraexpr_arg arg = NULL;
	rt_iterator itrset;
	uint16_t exprpos[3] = {1, 4, 5};

	int i = 0;
	int j = 0;
	int k = 0;

	int numraster = 0;
	int err = 0;
	int allnull = 0;
	int allempty = 0;
	int noband = 0;
	int len = 0;

	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	Datum datum;
	bool isnull = FALSE;

	rt_raster raster = NULL;
	rt_band band = NULL;
	rt_pgraster *pgraster = NULL;

	const int argkwcount = 12;
	char *argkw[] = {
		"[rast.x]",
		"[rast.y]",
		"[rast.val]",
		"[rast]",
		"[rast1.x]",
		"[rast1.y]",
		"[rast1.val]",
		"[rast1]",
		"[rast2.x]",
		"[rast2.y]",
		"[rast2.val]",
		"[rast2]"
	};

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* init argument struct */
	arg = rtpg_nmapalgebraexpr_arg_init(argkwcount, argkw);
	if (arg == NULL) {
		elog(ERROR, "RASTER_nMapAlgebraExpr: Could not initialize argument structure");
		PG_RETURN_NULL();
	}

	/* let helper function process rastbandarg (0) */
	if (!rtpg_nmapalgebra_rastbandarg_process(arg->bandarg, PG_GETARG_ARRAYTYPE_P(0), &allnull, &allempty, &noband)) {
		rtpg_nmapalgebraexpr_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebra: Could not process rastbandarg");
		PG_RETURN_NULL();
	}

	POSTGIS_RT_DEBUGF(4, "allnull, allempty, noband = %d, %d, %d", allnull, allempty, noband);

	/* all rasters are NULL, return NULL */
	if (allnull == arg->bandarg->numraster) {
		elog(NOTICE, "All input rasters are NULL. Returning NULL");
		rtpg_nmapalgebraexpr_arg_destroy(arg);
		PG_RETURN_NULL();
	}

	/* only work on one or two rasters */
	if (arg->bandarg->numraster > 1)
		numraster = 2;
	else
		numraster = 1;

	/* pixel type (2) */
	if (!PG_ARGISNULL(2)) {
		char *pixtypename = text_to_cstring(PG_GETARG_TEXT_P(2));

		/* Get the pixel type index */
		arg->bandarg->pixtype = rt_pixtype_index_from_name(pixtypename);
		if (arg->bandarg->pixtype == PT_END) {
			rtpg_nmapalgebraexpr_arg_destroy(arg);
			elog(ERROR, "RASTER_nMapAlgebraExpr: Invalid pixel type: %s", pixtypename);
			PG_RETURN_NULL();
		}
	}
	POSTGIS_RT_DEBUGF(4, "pixeltype: %d", arg->bandarg->pixtype);

	/* extent type (3) */
	if (!PG_ARGISNULL(3)) {
		char *extenttypename = rtpg_strtoupper(rtpg_trim(text_to_cstring(PG_GETARG_TEXT_P(3))));
		arg->bandarg->extenttype = rt_util_extent_type(extenttypename);
	}

	if (arg->bandarg->extenttype == ET_CUSTOM) {
		if (numraster < 2) {
			elog(NOTICE, "CUSTOM extent type not supported. Defaulting to FIRST");
			arg->bandarg->extenttype = ET_FIRST;
		}
		else {
			elog(NOTICE, "CUSTOM extent type not supported. Defaulting to INTERSECTION");
			arg->bandarg->extenttype = ET_INTERSECTION;
		}
	}
	else if (numraster < 2)
		arg->bandarg->extenttype = ET_FIRST;

	POSTGIS_RT_DEBUGF(4, "extenttype: %d", arg->bandarg->extenttype);

	/* nodatanodataval (6) */
	if (!PG_ARGISNULL(6)) {
		arg->callback.nodatanodata.hasval = 1;
		arg->callback.nodatanodata.val = PG_GETARG_FLOAT8(6);
	}

	err = 0;
	/* all rasters are empty, return empty raster */
	if (allempty == arg->bandarg->numraster) {
		elog(NOTICE, "All input rasters are empty. Returning empty raster");
		err = 1;
	}
	/* all rasters don't have indicated band, return empty raster */
	else if (noband == arg->bandarg->numraster) {
		elog(NOTICE, "All input rasters do not have bands at indicated indexes. Returning empty raster");
		err = 1;
	}
	if (err) {
		rtpg_nmapalgebraexpr_arg_destroy(arg);

		raster = rt_raster_new(0, 0);
		if (raster == NULL) {
			elog(ERROR, "RASTER_nMapAlgebraExpr: Could not create empty raster");
			PG_RETURN_NULL();
		}

		pgraster = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgraster) PG_RETURN_NULL();

		SET_VARSIZE(pgraster, pgraster->size);
		PG_RETURN_POINTER(pgraster);
	}

	/* connect SPI */
	if (SPI_connect() != SPI_OK_CONNECT) {
		rtpg_nmapalgebraexpr_arg_destroy(arg);
		elog(ERROR, "RASTER_nMapAlgebraExpr: Could not connect to the SPI manager");
		PG_RETURN_NULL();
	}

	/*
		process expressions

		exprpos elements are:
			1 - expression => spi_plan[0]
			4 - nodata1expr => spi_plan[1]
			5 - nodata2expr => spi_plan[2]
	*/
	for (i = 0; i < arg->callback.exprcount; i++) {
		char *expr = NULL;
		char *tmp = NULL;
		char *sql = NULL;
		char place[5] = "$1";

		if (PG_ARGISNULL(exprpos[i]))
			continue;

		expr = text_to_cstring(PG_GETARG_TEXT_P(exprpos[i]));
		POSTGIS_RT_DEBUGF(3, "raw expr of argument #%d: %s", exprpos[i], expr);

		for (j = 0, k = 1; j < argkwcount; j++) {
			/* attempt to replace keyword with placeholder */
			len = 0;
			tmp = rtpg_strreplace(expr, argkw[j], place, &len);
			pfree(expr);
			expr = tmp;

			if (len) {
				POSTGIS_RT_DEBUGF(4, "kw #%d (%s) at pos $%d", j, argkw[j], k);
				arg->callback.expr[i].spi_argcount++;
				arg->callback.expr[i].spi_argpos[j] = k++;

				sprintf(place, "$%d", k);
			}
			else
				arg->callback.expr[i].spi_argpos[j] = 0;
		}

		len = strlen("SELECT (") + strlen(expr) + strlen(")::double precision");
		sql = (char *) palloc(len + 1);
		if (sql == NULL) {
			rtpg_nmapalgebraexpr_arg_destroy(arg);
			SPI_finish();
			elog(ERROR, "RASTER_nMapAlgebraExpr: Could not allocate memory for expression parameter %d", exprpos[i]);
			PG_RETURN_NULL();
		}

		strncpy(sql, "SELECT (", strlen("SELECT ("));
		strncpy(sql + strlen("SELECT ("), expr, strlen(expr));
		strncpy(sql + strlen("SELECT (") + strlen(expr), ")::double precision", strlen(")::double precision"));
		sql[len] = '\0';

		POSTGIS_RT_DEBUGF(3, "sql #%d: %s", exprpos[i], sql);

		/* prepared plan */
		if (arg->callback.expr[i].spi_argcount) {
			Oid *argtype = (Oid *) palloc(arg->callback.expr[i].spi_argcount * sizeof(Oid));
			POSTGIS_RT_DEBUGF(3, "expression parameter %d is a prepared plan", exprpos[i]);
			if (argtype == NULL) {
				pfree(sql);
				rtpg_nmapalgebraexpr_arg_destroy(arg);
				SPI_finish();
				elog(ERROR, "RASTER_nMapAlgebraExpr: Could not allocate memory for prepared plan argtypes of expression parameter %d", exprpos[i]);
				PG_RETURN_NULL();
			}

			/* specify datatypes of parameters */
			for (j = 0, k = 0; j < argkwcount; j++) {
				if (arg->callback.expr[i].spi_argpos[j] < 1) continue;

				/* positions are INT4 */
				if (
					(strstr(argkw[j], "[rast.x]") != NULL) ||
					(strstr(argkw[j], "[rast.y]") != NULL) ||
					(strstr(argkw[j], "[rast1.x]") != NULL) ||
					(strstr(argkw[j], "[rast1.y]") != NULL) ||
					(strstr(argkw[j], "[rast2.x]") != NULL) ||
					(strstr(argkw[j], "[rast2.y]") != NULL)
				)
					argtype[k] = INT4OID;
				/* everything else is FLOAT8 */
				else
					argtype[k] = FLOAT8OID;

				k++;
			}

			arg->callback.expr[i].spi_plan = SPI_prepare(sql, arg->callback.expr[i].spi_argcount, argtype);
			pfree(argtype);
			pfree(sql);

			if (arg->callback.expr[i].spi_plan == NULL) {
				rtpg_nmapalgebraexpr_arg_destroy(arg);
				SPI_finish();
				elog(ERROR, "RASTER_nMapAlgebraExpr: Could not create prepared plan of expression parameter %d", exprpos[i]);
				PG_RETURN_NULL();
			}
		}
		/* no args, just execute query */
		else {
			POSTGIS_RT_DEBUGF(3, "expression parameter %d has no args, simply executing", exprpos[i]);
			err = SPI_execute(sql, TRUE, 0);
			pfree(sql);

			if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
				rtpg_nmapalgebraexpr_arg_destroy(arg);
				SPI_finish();
				elog(ERROR, "RASTER_nMapAlgebraExpr: Could not evaluate expression parameter %d", exprpos[i]);
				PG_RETURN_NULL();
			}

			/* get output of prepared plan */
			tupdesc = SPI_tuptable->tupdesc;
			tuptable = SPI_tuptable;
			tuple = tuptable->vals[0];

			datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
			if (SPI_result == SPI_ERROR_NOATTRIBUTE) {
				if (SPI_tuptable) SPI_freetuptable(tuptable);
				rtpg_nmapalgebraexpr_arg_destroy(arg);
				SPI_finish();
				elog(ERROR, "RASTER_nMapAlgebraExpr: Could not get result of expression parameter %d", exprpos[i]);
				PG_RETURN_NULL();
			}

			if (!isnull) {
				arg->callback.expr[i].hasval = 1;
				arg->callback.expr[i].val = DatumGetFloat8(datum);
			} 

			if (SPI_tuptable) SPI_freetuptable(tuptable);
		}
	}

	/* determine nodataval and possibly pixtype */
	/* band to check */
	switch (arg->bandarg->extenttype) {
		case ET_LAST:
		case ET_SECOND:
			if (numraster > 1)
				i = 1;
			else
				i = 0;
			break;
		default:
			i = 0;
			break;
	}
	/* find first viable band */
	if (!arg->bandarg->hasband[i]) {
		for (i = 0; i < numraster; i++) {
			if (arg->bandarg->hasband[i])
				break;
		}
		if (i >= numraster)
			i = numraster - 1;
	}
	band = rt_raster_get_band(arg->bandarg->raster[i], arg->bandarg->nband[i]);

	/* set pixel type if PT_END */
	if (arg->bandarg->pixtype == PT_END)
		arg->bandarg->pixtype = rt_band_get_pixtype(band);

	/* set hasnodata and nodataval */
	arg->bandarg->hasnodata = 1;
	if (rt_band_get_hasnodata_flag(band))
		rt_band_get_nodata(band, &(arg->bandarg->nodataval));
	else
		arg->bandarg->nodataval = rt_band_get_min_value(band);

	POSTGIS_RT_DEBUGF(4, "pixtype, hasnodata, nodataval: %s, %d, %f", rt_pixtype_name(arg->bandarg->pixtype), arg->bandarg->hasnodata, arg->bandarg->nodataval);

	/* init itrset */
	itrset = palloc(sizeof(struct rt_iterator_t) * numraster);
	if (itrset == NULL) {
		rtpg_nmapalgebraexpr_arg_destroy(arg);
		SPI_finish();
		elog(ERROR, "RASTER_nMapAlgebra: Could not allocate memory for iterator arguments");
		PG_RETURN_NULL();
	}

	/* set itrset */
	for (i = 0; i < numraster; i++) {
		itrset[i].raster = arg->bandarg->raster[i];
		itrset[i].nband = arg->bandarg->nband[i];
		itrset[i].nbnodata = 1;
	}

	/* pass everything to iterator */
	err = rt_raster_iterator(
		itrset, numraster,
		arg->bandarg->extenttype, arg->bandarg->cextent,
		arg->bandarg->pixtype,
		arg->bandarg->hasnodata, arg->bandarg->nodataval,
		0, 0,
		&(arg->callback),
		rtpg_nmapalgebraexpr_callback,
		&raster
	);

	pfree(itrset);
	rtpg_nmapalgebraexpr_arg_destroy(arg);

	if (err != ES_NONE) {
		SPI_finish();
		elog(ERROR, "RASTER_nMapAlgebraExpr: Could not run raster iterator function");
		PG_RETURN_NULL();
	}
	else if (raster == NULL) {
		SPI_finish();
		PG_RETURN_NULL();
	}

	/* switch to prior memory context to ensure memory allocated in correct context */
	MemoryContextSwitchTo(mainMemCtx);

	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);

	/* finish SPI */
	SPI_finish();

	if (!pgraster)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	PG_RETURN_POINTER(pgraster);
}

/* ---------------------------------------------------------------- */
/*  ST_Union aggregate functions                                    */
/* ---------------------------------------------------------------- */

typedef enum {
	UT_LAST = 0,
	UT_FIRST,
	UT_MIN,
	UT_MAX,
	UT_COUNT,
	UT_SUM,
	UT_MEAN,
	UT_RANGE
} rtpg_union_type;

/* internal function translating text of UNION type to enum */
static rtpg_union_type rtpg_uniontype_index_from_name(const char *cutype) {
	assert(cutype && strlen(cutype) > 0);

	if (strcmp(cutype, "LAST") == 0)
		return UT_LAST;
	else if (strcmp(cutype, "FIRST") == 0)
		return UT_FIRST;
	else if (strcmp(cutype, "MIN") == 0)
		return UT_MIN;
	else if (strcmp(cutype, "MAX") == 0)
		return UT_MAX;
	else if (strcmp(cutype, "COUNT") == 0)
		return UT_COUNT;
	else if (strcmp(cutype, "SUM") == 0)
		return UT_SUM;
	else if (strcmp(cutype, "MEAN") == 0)
		return UT_MEAN;
	else if (strcmp(cutype, "RANGE") == 0)
		return UT_RANGE;

	return UT_LAST;
}

typedef struct rtpg_union_band_arg_t *rtpg_union_band_arg;
struct rtpg_union_band_arg_t {
	int nband; /* source raster's band index, 0-based */
	rtpg_union_type uniontype;

	int numraster;
	rt_raster *raster;
};

typedef struct rtpg_union_arg_t *rtpg_union_arg;
struct rtpg_union_arg_t {
	int numband; /* number of bandargs */
	rtpg_union_band_arg bandarg;
};

static void rtpg_union_arg_destroy(rtpg_union_arg arg) {
	int i = 0;
	int j = 0;
	int k = 0;

	if (arg->bandarg != NULL) {
		for (i = 0; i < arg->numband; i++) {
			if (!arg->bandarg[i].numraster)
				continue;

			for (j = 0; j < arg->bandarg[i].numraster; j++) {
				if (arg->bandarg[i].raster[j] == NULL)
					continue;

				for (k = rt_raster_get_num_bands(arg->bandarg[i].raster[j]) - 1; k >= 0; k--)
					rt_band_destroy(rt_raster_get_band(arg->bandarg[i].raster[j], k));
				rt_raster_destroy(arg->bandarg[i].raster[j]);
			}

			pfree(arg->bandarg[i].raster);
		}

		pfree(arg->bandarg);
	}

	pfree(arg);
}

static int rtpg_union_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	rtpg_union_type *utype = (rtpg_union_type *) userarg;

	if (arg == NULL)
		return 0;

	if (
		arg->rasters != 2 ||
		arg->rows != 1 ||
		arg->columns != 1
	) {
		elog(ERROR, "rtpg_union_callback: Invalid arguments passed to callback");
		return 0;
	}

	*value = 0;
	*nodata = 0;

	/* handle NODATA situations except for COUNT, which is a special case */
	if (*utype != UT_COUNT) {
		/* both NODATA */
		if (arg->nodata[0][0][0] && arg->nodata[1][0][0]) {
			*nodata = 1;
			POSTGIS_RT_DEBUGF(4, "value, nodata = %f, %d", *value, *nodata);
			return 1;
		}
		/* second NODATA */
		else if (!arg->nodata[0][0][0] && arg->nodata[1][0][0]) {
			*value = arg->values[0][0][0];
			POSTGIS_RT_DEBUGF(4, "value, nodata = %f, %d", *value, *nodata);
			return 1;
		}
		/* first NODATA */
		else if (arg->nodata[0][0][0] && !arg->nodata[1][0][0]) {
			*value = arg->values[1][0][0];
			POSTGIS_RT_DEBUGF(4, "value, nodata = %f, %d", *value, *nodata);
			return 1;
		}
	}

	switch (*utype) {
		case UT_FIRST:
			*value = arg->values[0][0][0];
			break;
		case UT_MIN:
			if (arg->values[0][0][0] < arg->values[1][0][0])
				*value = arg->values[0][0][0];
			else
				*value = arg->values[1][0][0];
			break;
		case UT_MAX:
			if (arg->values[0][0][0] > arg->values[1][0][0])
				*value = arg->values[0][0][0];
			else
				*value = arg->values[1][0][0];
			break;
		case UT_COUNT:
			/* both NODATA */
			if (arg->nodata[0][0][0] && arg->nodata[1][0][0])
				*value = 0;
			/* second NODATA */
			else if (!arg->nodata[0][0][0] && arg->nodata[1][0][0])
				*value = arg->values[0][0][0];
			/* first NODATA */
			else if (arg->nodata[0][0][0] && !arg->nodata[1][0][0])
				*value = 1;
			/* has value, increment */
			else
				*value = arg->values[0][0][0] + 1;
			break;
		case UT_SUM:
			*value = arg->values[0][0][0] + arg->values[1][0][0];
			break;
		case UT_MEAN:
		case UT_RANGE:
			break;
		case UT_LAST:
		default:
			*value = arg->values[1][0][0];
			break;
	}

	POSTGIS_RT_DEBUGF(4, "value, nodata = %f, %d", *value, *nodata);


	return 1;
}

static int rtpg_union_mean_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	if (arg == NULL)
		return 0;

	if (
		arg->rasters != 2 ||
		arg->rows != 1 ||
		arg->columns != 1
	) {
		elog(ERROR, "rtpg_union_mean_callback: Invalid arguments passed to callback");
		return 0;
	}

	*value = 0;
	*nodata = 1;

	POSTGIS_RT_DEBUGF(4, "rast0: %f %d", arg->values[0][0][0], arg->nodata[0][0][0]);
	POSTGIS_RT_DEBUGF(4, "rast1: %f %d", arg->values[1][0][0], arg->nodata[1][0][0]);

	if (
		!arg->nodata[0][0][0] &&
		FLT_NEQ(arg->values[0][0][0], 0) &&
		!arg->nodata[1][0][0]
	) {
		*value = arg->values[1][0][0] / arg->values[0][0][0];
		*nodata = 0;
	}

	POSTGIS_RT_DEBUGF(4, "value, nodata = (%f, %d)", *value, *nodata);

	return 1;
}

static int rtpg_union_range_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	if (arg == NULL)
		return 0;

	if (
		arg->rasters != 2 ||
		arg->rows != 1 ||
		arg->columns != 1
	) {
		elog(ERROR, "rtpg_union_range_callback: Invalid arguments passed to callback");
		return 0;
	}

	*value = 0;
	*nodata = 1;

	POSTGIS_RT_DEBUGF(4, "rast0: %f %d", arg->values[0][0][0], arg->nodata[0][0][0]);
	POSTGIS_RT_DEBUGF(4, "rast1: %f %d", arg->values[1][0][0], arg->nodata[1][0][0]);

	if (
		!arg->nodata[0][0][0] &&
		!arg->nodata[1][0][0]
	) {
		*value = arg->values[1][0][0] - arg->values[0][0][0];
		*nodata = 0;
	}

	POSTGIS_RT_DEBUGF(4, "value, nodata = (%f, %d)", *value, *nodata);

	return 1;
}

/* called for ST_Union(raster, unionarg[]) */
static int rtpg_union_unionarg_process(rtpg_union_arg arg, ArrayType *array) {
	Oid etype;
	Datum *e;
	bool *nulls;
	int16 typlen;
	bool typbyval;
	char typalign;
	int n = 0;

	HeapTupleHeader tup;
	bool isnull;
	Datum tupv;

	int i;
	int nband = 1;
	char *utypename = NULL;
	rtpg_union_type utype = UT_LAST;

	etype = ARR_ELEMTYPE(array);
	get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

	deconstruct_array(
		array,
		etype,
		typlen, typbyval, typalign,
		&e, &nulls, &n
	);

	if (!n) {
		elog(ERROR, "rtpg_union_unionarg_process: Invalid argument for unionarg");
		return 0;
	}

	/* prep arg */
	arg->numband = n;
	arg->bandarg = palloc(sizeof(struct rtpg_union_band_arg_t) * arg->numband);
	if (arg->bandarg == NULL) {
		elog(ERROR, "rtpg_union_unionarg_process: Could not allocate memory for band information");
		return 0;
	}

	/* process each element */
	for (i = 0; i < n; i++) {
		if (nulls[i]) {
			arg->numband--;
			continue;
		}

		POSTGIS_RT_DEBUGF(4, "Processing unionarg at index %d", i);

		/* each element is a tuple */
		tup = (HeapTupleHeader) DatumGetPointer(e[i]);
		if (NULL == tup) {
			elog(ERROR, "rtpg_union_unionarg_process: Invalid argument for unionarg");
			return 0;
		}

		/* first element, bandnum */
		tupv = GetAttributeByName(tup, "nband", &isnull);
		if (isnull) {
			nband = i + 1;
			elog(NOTICE, "First argument (nband) of unionarg is NULL.  Assuming nband = %d", nband);
		}
		else
			nband = DatumGetInt32(tupv);

		if (nband < 1) {
			elog(ERROR, "rtpg_union_unionarg_process: Band number must be greater than zero (1-based)");
			return 0;
		}

		/* second element, uniontype */
		tupv = GetAttributeByName(tup, "uniontype", &isnull);
		if (isnull) {
			elog(NOTICE, "Second argument (uniontype) of unionarg is NULL.  Assuming uniontype = LAST");
			utype = UT_LAST;
		}
		else {
			utypename = text_to_cstring((text *) DatumGetPointer(tupv));
			utype = rtpg_uniontype_index_from_name(rtpg_strtoupper(utypename));
		}

		arg->bandarg[i].uniontype = utype;
		arg->bandarg[i].nband = nband - 1;
		arg->bandarg[i].raster = NULL;

		if (
			utype != UT_MEAN &&
			utype != UT_RANGE
		) {
			arg->bandarg[i].numraster = 1;
		}
		else
			arg->bandarg[i].numraster = 2;
	}

	if (arg->numband < n) {
		arg->bandarg = repalloc(arg->bandarg, sizeof(struct rtpg_union_band_arg_t) * arg->numband);
		if (arg->bandarg == NULL) {
			elog(ERROR, "rtpg_union_unionarg_process: Could not reallocate memory for band information");
			return 0;
		}
	}

	return 1;
}

/* called for ST_Union(raster) */
static int rtpg_union_noarg(rtpg_union_arg arg, rt_raster raster) {
	int numbands;
	int i;

	if (rt_raster_is_empty(raster))
		return 1;

	numbands = rt_raster_get_num_bands(raster);
	if (numbands <= arg->numband)
		return 1;

	/* more bands to process */
	POSTGIS_RT_DEBUG(4, "input raster has more bands, adding more bandargs");
	if (arg->numband)
		arg->bandarg = repalloc(arg->bandarg, sizeof(struct rtpg_union_band_arg_t) * numbands);
	else
		arg->bandarg = palloc(sizeof(struct rtpg_union_band_arg_t) * numbands);
	if (arg->bandarg == NULL) {
		elog(ERROR, "rtpg_union_noarg: Could not reallocate memory for band information");
		return 0;
	}

	i = arg->numband;
	arg->numband = numbands;
	for (; i < arg->numband; i++) {
		POSTGIS_RT_DEBUGF(4, "Adding bandarg for band at index %d", i);
		arg->bandarg[i].uniontype = UT_LAST;
		arg->bandarg[i].nband = i;
		arg->bandarg[i].numraster = 1;

		arg->bandarg[i].raster = (rt_raster *) palloc(sizeof(rt_raster) * arg->bandarg[i].numraster);
		if (arg->bandarg[i].raster == NULL) {
			elog(ERROR, "rtpg_union_noarg: Could not allocate memory for working rasters");
			return 0;
		}
		memset(arg->bandarg[i].raster, 0, sizeof(rt_raster) * arg->bandarg[i].numraster);

		/* add new working rt_raster but only if working raster already exists */
		if (!rt_raster_is_empty(arg->bandarg[0].raster[0])) {
			arg->bandarg[i].raster[0] = rt_raster_clone(arg->bandarg[0].raster[0], 0); /* shallow clone */
			if (arg->bandarg[i].raster[0] == NULL) {
				elog(ERROR, "rtpg_union_noarg: Could not create working raster");
				return 0;
			}
		}
	}

	return 1;
}

/* UNION aggregate transition function */
PG_FUNCTION_INFO_V1(RASTER_union_transfn);
Datum RASTER_union_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext;
	MemoryContext oldcontext;
	rtpg_union_arg iwr = NULL;
	int skiparg = 0;

	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_raster _raster = NULL;
	rt_band _band = NULL;
	int nband = 1;
	int noerr = 1;
	int isempty[2] = {0};
	int hasband[2] = {0};
	int nargs = 0;
	double _offset[4] = {0.};
	int nbnodata = 0; /* 1 if adding bands */

	int i = 0;
	int j = 0;
	int k = 0;

	rt_iterator itrset;
	char *utypename = NULL;
	rtpg_union_type utype = UT_LAST;
	rt_pixtype pixtype = PT_END;
	int hasnodata = 1;
	double nodataval = 0;

	rt_raster iraster = NULL;
	rt_band iband = NULL;
	int reuserast = 0;
	int y = 0;
	uint16_t _dim[2] = {0};
	void *vals = NULL;
	uint16_t nvals = 0;

	POSTGIS_RT_DEBUG(3, "Starting...");

	/* cannot be called directly as this is exclusive aggregate function */
	if (!AggCheckCallContext(fcinfo, &aggcontext)) {
		elog(ERROR, "RASTER_union_transfn: Cannot be called in a non-aggregate context");
		PG_RETURN_NULL();
	}

	/* switch to aggcontext */
	oldcontext = MemoryContextSwitchTo(aggcontext);

	if (PG_ARGISNULL(0)) {
		POSTGIS_RT_DEBUG(3, "Creating state variable");
		/* allocate container in aggcontext */
		iwr = palloc(sizeof(struct rtpg_union_arg_t));
		if (iwr == NULL) {
			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_union_transfn: Could not allocate memory for state variable");
			PG_RETURN_NULL();
		}

		iwr->numband = 0;
		iwr->bandarg = NULL;

		skiparg = 0;
	}
	else {
		POSTGIS_RT_DEBUG(3, "State variable already exists");
		iwr = (rtpg_union_arg) PG_GETARG_POINTER(0);
		skiparg = 1;
	}

	/* raster arg is NOT NULL */
	if (!PG_ARGISNULL(1)) {
		/* deserialize raster */
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		/* Get raster object */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (raster == NULL) {

			rtpg_union_arg_destroy(iwr);
			PG_FREE_IF_COPY(pgraster, 1);

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_union_transfn: Could not deserialize raster");
			PG_RETURN_NULL();
		}
	}

	/* process additional args if needed */
	nargs = PG_NARGS();
	POSTGIS_RT_DEBUGF(4, "nargs = %d", nargs);
	if (nargs > 2) {
		POSTGIS_RT_DEBUG(4, "processing additional arguments");

		/* if more than 2 arguments, determine the type of argument 3 */
		/* band number, UNION type or unionarg */
		if (!PG_ARGISNULL(2)) {
			Oid calltype = get_fn_expr_argtype(fcinfo->flinfo, 2);

			switch (calltype) {
				/* UNION type */
				case TEXTOID: {
					int idx = 0;
					int numband = 0;

					POSTGIS_RT_DEBUG(4, "Processing arg 3 as UNION type");
					nbnodata = 1;

					utypename = text_to_cstring(PG_GETARG_TEXT_P(2));
					utype = rtpg_uniontype_index_from_name(rtpg_strtoupper(utypename));
					POSTGIS_RT_DEBUGF(4, "Union type: %s", utypename);

					POSTGIS_RT_DEBUGF(4, "iwr->numband: %d", iwr->numband);
					/* see if we need to append new bands */
					if (raster) {
						idx = iwr->numband;
						numband = rt_raster_get_num_bands(raster);
						POSTGIS_RT_DEBUGF(4, "numband: %d", numband);

						/* only worry about appended bands */
						if (numband > iwr->numband)
							iwr->numband = numband;
					}

					if (!iwr->numband)
						iwr->numband = 1;
					POSTGIS_RT_DEBUGF(4, "iwr->numband: %d", iwr->numband);
					POSTGIS_RT_DEBUGF(4, "numband, idx: %d, %d", numband, idx);

					/* bandarg set. only possible after the first call to function */
					if (iwr->bandarg) {
						/* only reallocate if new bands need to be added */
						if (numband > idx) {
							POSTGIS_RT_DEBUG(4, "Reallocating iwr->bandarg");
							iwr->bandarg = repalloc(iwr->bandarg, sizeof(struct rtpg_union_band_arg_t) * iwr->numband);
						}
						/* prevent initial values step happening below */
						else
							idx = iwr->numband;
					}
					/* bandarg not set, first call to function */
					else {
						POSTGIS_RT_DEBUG(4, "Allocating iwr->bandarg");
						iwr->bandarg = palloc(sizeof(struct rtpg_union_band_arg_t) * iwr->numband);
					}
					if (iwr->bandarg == NULL) {

						rtpg_union_arg_destroy(iwr);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not allocate memory for band information");
						PG_RETURN_NULL();
					}

					/* set initial values for bands that are "new" */
					for (i = idx; i < iwr->numband; i++) {
						iwr->bandarg[i].uniontype = utype;
						iwr->bandarg[i].nband = i;

						if (
							utype == UT_MEAN ||
							utype == UT_RANGE
						) {
							iwr->bandarg[i].numraster = 2;
						}
						else
							iwr->bandarg[i].numraster = 1;
						iwr->bandarg[i].raster = NULL;
					}

					break;
				}
				/* band number */
				case INT2OID:
				case INT4OID:
					if (skiparg)
						break;

					POSTGIS_RT_DEBUG(4, "Processing arg 3 as band number");
					nband = PG_GETARG_INT32(2);
					if (nband < 1) {

						rtpg_union_arg_destroy(iwr);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Band number must be greater than zero (1-based)");
						PG_RETURN_NULL();
					}

					iwr->numband = 1;
					iwr->bandarg = palloc(sizeof(struct rtpg_union_band_arg_t) * iwr->numband);
					if (iwr->bandarg == NULL) {

						rtpg_union_arg_destroy(iwr);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not allocate memory for band information");
						PG_RETURN_NULL();
					}

					iwr->bandarg[0].uniontype = UT_LAST;
					iwr->bandarg[0].nband = nband - 1;

					iwr->bandarg[0].numraster = 1;
					iwr->bandarg[0].raster = NULL;
					break;
				/* only other type allowed is unionarg */
				default: 
					if (skiparg)
						break;

					POSTGIS_RT_DEBUG(4, "Processing arg 3 as unionarg");
					if (!rtpg_union_unionarg_process(iwr, PG_GETARG_ARRAYTYPE_P(2))) {

						rtpg_union_arg_destroy(iwr);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not process unionarg");
						PG_RETURN_NULL();
					}

					break;
			}
		}

		/* UNION type */
		if (nargs > 3 && !PG_ARGISNULL(3)) {
			utypename = text_to_cstring(PG_GETARG_TEXT_P(3));
			utype = rtpg_uniontype_index_from_name(rtpg_strtoupper(utypename));
			iwr->bandarg[0].uniontype = utype;
			POSTGIS_RT_DEBUGF(4, "Union type: %s", utypename);

			if (
				utype == UT_MEAN ||
				utype == UT_RANGE
			) {
				iwr->bandarg[0].numraster = 2;
			}
		}

		/* allocate space for pointers to rt_raster */
		for (i = 0; i < iwr->numband; i++) {
			POSTGIS_RT_DEBUGF(4, "iwr->bandarg[%d].raster @ %p", i, iwr->bandarg[i].raster);

			/* no need to allocate */
			if (iwr->bandarg[i].raster != NULL)
				continue;

			POSTGIS_RT_DEBUGF(4, "Allocating space for working rasters of band %d", i);

			iwr->bandarg[i].raster = (rt_raster *) palloc(sizeof(rt_raster) * iwr->bandarg[i].numraster);
			if (iwr->bandarg[i].raster == NULL) {

				rtpg_union_arg_destroy(iwr);
				if (raster != NULL) {
					rt_raster_destroy(raster);
					PG_FREE_IF_COPY(pgraster, 1);
				}

				MemoryContextSwitchTo(oldcontext);
				elog(ERROR, "RASTER_union_transfn: Could not allocate memory for working raster(s)");
				PG_RETURN_NULL();
			}

			memset(iwr->bandarg[i].raster, 0, sizeof(rt_raster) * iwr->bandarg[i].numraster);

			/* add new working rt_raster but only if working raster already exists */
			if (i > 0 && !rt_raster_is_empty(iwr->bandarg[0].raster[0])) {
				for (j = 0; j < iwr->bandarg[i].numraster; j++) {
					iwr->bandarg[i].raster[j] = rt_raster_clone(iwr->bandarg[0].raster[0], 0); /* shallow clone */
					if (iwr->bandarg[i].raster[j] == NULL) {

						rtpg_union_arg_destroy(iwr);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not create working raster");
						PG_RETURN_NULL();
					}
				}
			}
		}
	}
	/* only raster, no additional args */
	/* only do this if raster isn't empty */
	else {
		POSTGIS_RT_DEBUG(4, "no additional args, checking input raster");
		nbnodata = 1;
		if (!rtpg_union_noarg(iwr, raster)) {

			rtpg_union_arg_destroy(iwr);
			if (raster != NULL) {
				rt_raster_destroy(raster);
				PG_FREE_IF_COPY(pgraster, 1);
			}

			MemoryContextSwitchTo(oldcontext);
			elog(ERROR, "RASTER_union_transfn: Could not check and balance number of bands");
			PG_RETURN_NULL();
		}
	}

	/* init itrset */
	itrset = palloc(sizeof(struct rt_iterator_t) * 2);
	if (itrset == NULL) {

		rtpg_union_arg_destroy(iwr);
		if (raster != NULL) {
			rt_raster_destroy(raster);
			PG_FREE_IF_COPY(pgraster, 1);
		}

		MemoryContextSwitchTo(oldcontext);
		elog(ERROR, "RASTER_union_transfn: Could not allocate memory for iterator arguments");
		PG_RETURN_NULL();
	}

	/* by band to UNION */
	for (i = 0; i < iwr->numband; i++) {

		/* by raster */
		for (j = 0; j < iwr->bandarg[i].numraster; j++) {
			reuserast = 0;

			/* type of union */
			utype = iwr->bandarg[i].uniontype;

			/* raster flags */
			isempty[0] = rt_raster_is_empty(iwr->bandarg[i].raster[j]);
			isempty[1] = rt_raster_is_empty(raster);

			if (!isempty[0])
				hasband[0] = rt_raster_has_band(iwr->bandarg[i].raster[j], 0);
			if (!isempty[1])
				hasband[1] = rt_raster_has_band(raster, iwr->bandarg[i].nband);

			/* determine pixtype, hasnodata and nodataval */
			_band = NULL;
			if (!isempty[0] && hasband[0])
				_band = rt_raster_get_band(iwr->bandarg[i].raster[j], 0);
			else if (!isempty[1] && hasband[1])
				_band = rt_raster_get_band(raster, iwr->bandarg[i].nband);
			else {
				pixtype = PT_64BF;
				hasnodata = 1;
				nodataval = rt_pixtype_get_min_value(pixtype);
			}
			if (_band != NULL) {
				pixtype = rt_band_get_pixtype(_band);
				hasnodata = 1;
				if (rt_band_get_hasnodata_flag(_band))
					rt_band_get_nodata(_band, &nodataval);
				else
					nodataval = rt_band_get_min_value(_band);
			}

			/* UT_MEAN and UT_RANGE require two passes */
			/* UT_MEAN: first for UT_COUNT and second for UT_SUM */
			if (iwr->bandarg[i].uniontype == UT_MEAN) {
				/* first pass, UT_COUNT */
				if (j < 1)
					utype = UT_COUNT;
				else
					utype = UT_SUM;
			}
			/* UT_RANGE: first for UT_MIN and second for UT_MAX */
			else if (iwr->bandarg[i].uniontype == UT_RANGE) {
				/* first pass, UT_MIN */
				if (j < 1)
					utype = UT_MIN;
				else
					utype = UT_MAX;
			}

			/* force band settings for UT_COUNT */
			if (utype == UT_COUNT) {
				pixtype = PT_32BUI;
				hasnodata = 0;
				nodataval = 0;
			}

			POSTGIS_RT_DEBUGF(4, "(pixtype, hasnodata, nodataval) = (%s, %d, %f)", rt_pixtype_name(pixtype), hasnodata, nodataval);

			/* set itrset */
			itrset[0].raster = iwr->bandarg[i].raster[j];
			itrset[0].nband = 0;
			itrset[1].raster = raster;
			itrset[1].nband = iwr->bandarg[i].nband;

			/* allow use NODATA to replace missing bands */
			if (nbnodata) {
				itrset[0].nbnodata = 1;
				itrset[1].nbnodata = 1;
			}
			/* missing bands are not ignored */
			else {
				itrset[0].nbnodata = 0;
				itrset[1].nbnodata = 0;
			}

			/* if rasters AND bands are present, use copy approach */
			if (!isempty[0] && !isempty[1] && hasband[0] && hasband[1]) {
				POSTGIS_RT_DEBUG(3, "using line method");

				/* generate empty out raster */
				if (rt_raster_from_two_rasters(
					iwr->bandarg[i].raster[j], raster,
					ET_UNION,
					&iraster, _offset 
				) != ES_NONE) {

					pfree(itrset);
					rtpg_union_arg_destroy(iwr);
					if (raster != NULL) {
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 1);
					}

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_union_transfn: Could not create internal raster");
					PG_RETURN_NULL();
				}
				POSTGIS_RT_DEBUGF(4, "_offset = %f, %f, %f, %f",
					_offset[0], _offset[1], _offset[2], _offset[3]);

				/* rasters are spatially the same? */
				if (
					rt_raster_get_width(iwr->bandarg[i].raster[j]) == rt_raster_get_width(iraster) &&
					rt_raster_get_height(iwr->bandarg[i].raster[j]) == rt_raster_get_height(iraster)
				) {
					double igt[6] = {0};
					double gt[6] = {0};

					rt_raster_get_geotransform_matrix(iwr->bandarg[i].raster[j], gt);
					rt_raster_get_geotransform_matrix(iraster, igt);

					reuserast = rt_util_same_geotransform_matrix(gt, igt);
				}

				/* use internal raster */
				if (!reuserast) {
					/* create band of same type */
					if (rt_raster_generate_new_band(
						iraster,
						pixtype,
						nodataval,
						hasnodata, nodataval,
						0
					) == -1) {

						pfree(itrset);
						rtpg_union_arg_destroy(iwr);
						rt_raster_destroy(iraster);
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not add new band to internal raster");
						PG_RETURN_NULL();
					}
					iband = rt_raster_get_band(iraster, 0);

					/* copy working raster to output raster */
					_dim[0] = rt_raster_get_width(iwr->bandarg[i].raster[j]);
					_dim[1] = rt_raster_get_height(iwr->bandarg[i].raster[j]);
					for (y = 0; y < _dim[1]; y++) {
						POSTGIS_RT_DEBUGF(4, "Getting pixel line of working raster at (x, y, length) = (0, %d, %d)", y, _dim[0]);
						if (rt_band_get_pixel_line(
							_band,
							0, y,
							_dim[0],
							&vals, &nvals
						) != ES_NONE) {

							pfree(itrset);
							rtpg_union_arg_destroy(iwr);
							rt_band_destroy(iband);
							rt_raster_destroy(iraster);
							if (raster != NULL) {
								rt_raster_destroy(raster);
								PG_FREE_IF_COPY(pgraster, 1);
							}

							MemoryContextSwitchTo(oldcontext);
							elog(ERROR, "RASTER_union_transfn: Could not get pixel line from band of working raster");
							PG_RETURN_NULL();
						}

						POSTGIS_RT_DEBUGF(4, "Setting pixel line at (x, y, length) = (%d, %d, %d)", (int) _offset[0], (int) _offset[1] + y, nvals);
						if (rt_band_set_pixel_line(
							iband,
							(int) _offset[0], (int) _offset[1] + y,
							vals, nvals
						) != ES_NONE) {

							pfree(itrset);
							rtpg_union_arg_destroy(iwr);
							rt_band_destroy(iband);
							rt_raster_destroy(iraster);
							if (raster != NULL) {
								rt_raster_destroy(raster);
								PG_FREE_IF_COPY(pgraster, 1);
							}

							MemoryContextSwitchTo(oldcontext);
							elog(ERROR, "RASTER_union_transfn: Could not set pixel line to band of internal raster");
							PG_RETURN_NULL();
						}
					}
				}
				else {
					rt_raster_destroy(iraster);
					iraster = iwr->bandarg[i].raster[j];
					iband = rt_raster_get_band(iraster, 0);
				}

				/* run iterator for extent of input raster */
				noerr = rt_raster_iterator(
					itrset, 2,
					ET_LAST, NULL,
					pixtype,
					hasnodata, nodataval,
					0, 0,
					&utype,
					rtpg_union_callback,
					&_raster
				);
				if (noerr != ES_NONE) {

					pfree(itrset);
					rtpg_union_arg_destroy(iwr);
					if (!reuserast) {
						rt_band_destroy(iband);
						rt_raster_destroy(iraster);
					}
					if (raster != NULL) {
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 1);
					}

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_union_transfn: Could not run raster iterator function");
					PG_RETURN_NULL();
				}

				/* with iterator raster, copy data to output raster */
				_band = rt_raster_get_band(_raster, 0);
				_dim[0] = rt_raster_get_width(_raster);
				_dim[1] = rt_raster_get_height(_raster);
				for (y = 0; y < _dim[1]; y++) {
					POSTGIS_RT_DEBUGF(4, "Getting pixel line of iterator raster at (x, y, length) = (0, %d, %d)", y, _dim[0]);
					if (rt_band_get_pixel_line(
						_band,
						0, y,
						_dim[0],
						&vals, &nvals
					) != ES_NONE) {

						pfree(itrset);
						rtpg_union_arg_destroy(iwr);
						if (!reuserast) {
							rt_band_destroy(iband);
							rt_raster_destroy(iraster);
						}
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not get pixel line from band of working raster");
						PG_RETURN_NULL();
					}

					POSTGIS_RT_DEBUGF(4, "Setting pixel line at (x, y, length) = (%d, %d, %d)", (int) _offset[2], (int) _offset[3] + y, nvals);
					if (rt_band_set_pixel_line(
						iband,
						(int) _offset[2], (int) _offset[3] + y,
						vals, nvals
					) != ES_NONE) {

						pfree(itrset);
						rtpg_union_arg_destroy(iwr);
						if (!reuserast) {
							rt_band_destroy(iband);
							rt_raster_destroy(iraster);
						}
						if (raster != NULL) {
							rt_raster_destroy(raster);
							PG_FREE_IF_COPY(pgraster, 1);
						}

						MemoryContextSwitchTo(oldcontext);
						elog(ERROR, "RASTER_union_transfn: Could not set pixel line to band of internal raster");
						PG_RETURN_NULL();
					}
				}

				/* free _raster */
				rt_band_destroy(_band);
				rt_raster_destroy(_raster);

				/* replace working raster with output raster */
				_raster = iraster;
			}
			else {
				POSTGIS_RT_DEBUG(3, "using pixel method");

				/* pass everything to iterator */
				noerr = rt_raster_iterator(
					itrset, 2,
					ET_UNION, NULL,
					pixtype,
					hasnodata, nodataval,
					0, 0,
					&utype,
					rtpg_union_callback,
					&_raster
				);

				if (noerr != ES_NONE) {

					pfree(itrset);
					rtpg_union_arg_destroy(iwr);
					if (raster != NULL) {
						rt_raster_destroy(raster);
						PG_FREE_IF_COPY(pgraster, 1);
					}

					MemoryContextSwitchTo(oldcontext);
					elog(ERROR, "RASTER_union_transfn: Could not run raster iterator function");
					PG_RETURN_NULL();
				}
			}

			/* replace working raster */
			if (iwr->bandarg[i].raster[j] != NULL && !reuserast) {
				for (k = rt_raster_get_num_bands(iwr->bandarg[i].raster[j]) - 1; k >= 0; k--)
					rt_band_destroy(rt_raster_get_band(iwr->bandarg[i].raster[j], k));
				rt_raster_destroy(iwr->bandarg[i].raster[j]);
			}
			iwr->bandarg[i].raster[j] = _raster;
		}

	}

	pfree(itrset);
	if (raster != NULL) {
		rt_raster_destroy(raster);
		PG_FREE_IF_COPY(pgraster, 1);
	}

	/* switch back to local context */
	MemoryContextSwitchTo(oldcontext);

	POSTGIS_RT_DEBUG(3, "Finished");

	PG_RETURN_POINTER(iwr);
}

/* UNION aggregate final function */
PG_FUNCTION_INFO_V1(RASTER_union_finalfn);
Datum RASTER_union_finalfn(PG_FUNCTION_ARGS)
{
	rtpg_union_arg iwr;
	rt_raster _rtn = NULL;
	rt_raster _raster = NULL;
	rt_pgraster *pgraster = NULL;

	int i = 0;
	int j = 0;
	rt_iterator itrset = NULL;
	rt_band _band = NULL;
	int noerr = 1;
	int status = 0;
	rt_pixtype pixtype = PT_END;
	int hasnodata = 0;
	double nodataval = 0;

	POSTGIS_RT_DEBUG(3, "Starting...");

	/* cannot be called directly as this is exclusive aggregate function */
	if (!AggCheckCallContext(fcinfo, NULL)) {
		elog(ERROR, "RASTER_union_finalfn: Cannot be called in a non-aggregate context");
		PG_RETURN_NULL();
	}

	/* NULL, return null */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	iwr = (rtpg_union_arg) PG_GETARG_POINTER(0);

	/* init itrset */
	itrset = palloc(sizeof(struct rt_iterator_t) * 2);
	if (itrset == NULL) {
		rtpg_union_arg_destroy(iwr);
		elog(ERROR, "RASTER_union_finalfn: Could not allocate memory for iterator arguments");
		PG_RETURN_NULL();
	}

	for (i = 0; i < iwr->numband; i++) {
		if (
			iwr->bandarg[i].uniontype == UT_MEAN ||
			iwr->bandarg[i].uniontype == UT_RANGE
		) {
			/* raster containing the SUM or MAX is at index 1 */
			_band = rt_raster_get_band(iwr->bandarg[i].raster[1], 0);

			pixtype = rt_band_get_pixtype(_band);
			hasnodata = rt_band_get_hasnodata_flag(_band);
			if (hasnodata)
				rt_band_get_nodata(_band, &nodataval);
			POSTGIS_RT_DEBUGF(4, "(pixtype, hasnodata, nodataval) = (%s, %d, %f)", rt_pixtype_name(pixtype), hasnodata, nodataval);

			itrset[0].raster = iwr->bandarg[i].raster[0];
			itrset[0].nband = 0;
			itrset[1].raster = iwr->bandarg[i].raster[1];
			itrset[1].nband = 0;

			/* pass everything to iterator */
			if (iwr->bandarg[i].uniontype == UT_MEAN) {
				noerr = rt_raster_iterator(
					itrset, 2,
					ET_UNION, NULL,
					pixtype,
					hasnodata, nodataval,
					0, 0,
					NULL,
					rtpg_union_mean_callback,
					&_raster
				);
			}
			else if (iwr->bandarg[i].uniontype == UT_RANGE) {
				noerr = rt_raster_iterator(
					itrset, 2,
					ET_UNION, NULL,
					pixtype,
					hasnodata, nodataval,
					0, 0,
					NULL,
					rtpg_union_range_callback,
					&_raster
				);
			}

			if (noerr != ES_NONE) {
				pfree(itrset);
				rtpg_union_arg_destroy(iwr);
				if (_rtn != NULL)
					rt_raster_destroy(_rtn);
				elog(ERROR, "RASTER_union_finalfn: Could not run raster iterator function");
				PG_RETURN_NULL();
			}
		}
		else
			_raster = iwr->bandarg[i].raster[0];

		/* first band, _rtn doesn't exist */
		if (i < 1) {
			uint32_t bandNums[1] = {0};
			_rtn = rt_raster_from_band(_raster, bandNums, 1);
			status = (_rtn == NULL) ? -1 : 0;
		}
		else
			status = rt_raster_copy_band(_rtn, _raster, 0, i);

		POSTGIS_RT_DEBUG(4, "destroying source rasters");

		/* destroy source rasters */
		if (
			iwr->bandarg[i].uniontype == UT_MEAN ||
			iwr->bandarg[i].uniontype == UT_RANGE
		) {
			rt_raster_destroy(_raster);
		}
			
		for (j = 0; j < iwr->bandarg[i].numraster; j++) {
			if (iwr->bandarg[i].raster[j] == NULL)
				continue;
			rt_raster_destroy(iwr->bandarg[i].raster[j]);
			iwr->bandarg[i].raster[j] = NULL;
		}

		if (status < 0) {
			rtpg_union_arg_destroy(iwr);
			rt_raster_destroy(_rtn);
			elog(ERROR, "RASTER_union_finalfn: Could not add band to final raster");
			PG_RETURN_NULL();
		}
	}

	/* cleanup */
	pfree(itrset);
	rtpg_union_arg_destroy(iwr);

	pgraster = rt_raster_serialize(_rtn);
	rt_raster_destroy(_rtn);

	POSTGIS_RT_DEBUG(3, "Finished");

	if (!pgraster)
		PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	PG_RETURN_POINTER(pgraster);
}

/* ---------------------------------------------------------------- */
/* Clip raster with geometry                                        */
/* ---------------------------------------------------------------- */

typedef struct rtpg_clip_band_t *rtpg_clip_band;
struct rtpg_clip_band_t {
	int nband; /* band index */
	int hasnodata; /* is there a user-specified NODATA? */
	double nodataval; /* user-specified NODATA */
};

typedef struct rtpg_clip_arg_t *rtpg_clip_arg;
struct rtpg_clip_arg_t {
	rt_extenttype extenttype;
	rt_raster raster;
	rt_raster mask;

	int numbands; /* number of bandargs */
	rtpg_clip_band band;
};

static rtpg_clip_arg rtpg_clip_arg_init() {
	rtpg_clip_arg arg = NULL;

	arg = palloc(sizeof(struct rtpg_clip_arg_t));
	if (arg == NULL) {
		elog(ERROR, "rtpg_clip_arg_init: Could not allocate memory for function arguments");
		return NULL;
	}

	arg->extenttype = ET_INTERSECTION;
	arg->raster = NULL;
	arg->mask = NULL;
	arg->numbands = 0;
	arg->band = NULL;

	return arg;
}

static void rtpg_clip_arg_destroy(rtpg_clip_arg arg) {
	if (arg->band != NULL)
		pfree(arg->band);

	if (arg->raster != NULL)
		rt_raster_destroy(arg->raster);
	if (arg->mask != NULL)
		rt_raster_destroy(arg->mask);

	pfree(arg);
}

static int rtpg_clip_callback(
	rt_iterator_arg arg, void *userarg,
	double *value, int *nodata
) {
	*value = 0;
	*nodata = 0;

	/* either is NODATA, output is NODATA */
	if (arg->nodata[0][0][0] || arg->nodata[1][0][0])
		*nodata = 1;
	/* set to value */
	else
		*value = arg->values[0][0][0];

	return 1;
}

PG_FUNCTION_INFO_V1(RASTER_clip);
Datum RASTER_clip(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	LWGEOM *rastgeom = NULL;
	double gt[6] = {0};
	int srid = SRID_UNKNOWN;

	rt_pgraster *pgrtn = NULL;
	rt_raster rtn = NULL;

	GSERIALIZED *gser = NULL;
	LWGEOM *geom = NULL;
	unsigned char *wkb = NULL;
	size_t wkb_len;

	ArrayType *array;
	Oid etype;
	Datum *e;
	bool *nulls;

	int16 typlen;
	bool typbyval;
	char typalign;

	int i = 0;
	int j = 0;
	int k = 0;
	rtpg_clip_arg arg = NULL;
	LWGEOM *tmpgeom = NULL;
	rt_iterator itrset;

	rt_raster _raster = NULL;
	rt_band band = NULL;
	rt_pixtype pixtype;
	int hasnodata;
	double nodataval;
	int noerr = 0;

	POSTGIS_RT_DEBUG(3, "Starting...");

	/* raster or geometry is NULL, return NULL */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(2))
		PG_RETURN_NULL();

	/* init arg */
	arg = rtpg_clip_arg_init();
	if (arg == NULL) {
		elog(ERROR, "RASTER_clip: Could not initialize argument structure");
		PG_RETURN_NULL();
	}

	/* raster (0) */
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Get raster object */
	arg->raster = rt_raster_deserialize(pgraster, FALSE);
	if (arg->raster == NULL) {
		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_clip: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* raster is empty, return empty raster */
	if (rt_raster_is_empty(arg->raster)) {
		elog(NOTICE, "Input raster is empty. Returning empty raster");

		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);

		rtn = rt_raster_new(0, 0);
		if (rtn == NULL) {
			elog(ERROR, "RASTER_clip: Could not create empty raster");
			PG_RETURN_NULL();
		}

		pgrtn = rt_raster_serialize(rtn);
		rt_raster_destroy(rtn);
		if (NULL == pgrtn)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	/* metadata */
	rt_raster_get_geotransform_matrix(arg->raster, gt);
	srid = clamp_srid(rt_raster_get_srid(arg->raster));

	/* geometry (2) */
	gser = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(2));
	geom = lwgeom_from_gserialized(gser);

	/* Get a 2D version of the geometry if necessary */
	if (lwgeom_ndims(geom) > 2) {
		LWGEOM *geom2d = lwgeom_force_2d(geom);
		lwgeom_free(geom);
		geom = geom2d;
	}

	/* check that SRIDs match */
	if (srid != clamp_srid(gserialized_get_srid(gser))) {
		elog(NOTICE, "Geometry provided does not have the same SRID as the raster. Returning NULL");

		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		lwgeom_free(geom);
		PG_FREE_IF_COPY(gser, 2);

		PG_RETURN_NULL();
	}

	/* crop (4) */
	if (!PG_ARGISNULL(4) && !PG_GETARG_BOOL(4))
		arg->extenttype = ET_FIRST;

	/* get intersection geometry of input raster and input geometry */
	if (rt_raster_get_convex_hull(arg->raster, &rastgeom) != ES_NONE) {

		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		lwgeom_free(geom);
		PG_FREE_IF_COPY(gser, 2);

		elog(ERROR, "RASTER_clip: Could not get convex hull of raster");
		PG_RETURN_NULL();
	}

	tmpgeom = lwgeom_intersection(rastgeom, geom);
	lwgeom_free(rastgeom);
	lwgeom_free(geom);
	PG_FREE_IF_COPY(gser, 2);
	geom = tmpgeom;

	/* intersection is empty AND extent type is INTERSECTION, return empty */
	if (lwgeom_is_empty(geom) && arg->extenttype == ET_INTERSECTION) {
		elog(NOTICE, "The input raster and input geometry do not intersect. Returning empty raster");

		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		lwgeom_free(geom);

		rtn = rt_raster_new(0, 0);
		if (rtn == NULL) {
			elog(ERROR, "RASTER_clip: Could not create empty raster");
			PG_RETURN_NULL();
		}

		pgrtn = rt_raster_serialize(rtn);
		rt_raster_destroy(rtn);
		if (NULL == pgrtn)
			PG_RETURN_NULL();

		SET_VARSIZE(pgrtn, pgrtn->size);
		PG_RETURN_POINTER(pgrtn);
	}

	/* nband (1) */
	if (!PG_ARGISNULL(1)) {
		array = PG_GETARG_ARRAYTYPE_P(1);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case INT2OID:
			case INT4OID:
				break;
			default:
				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				lwgeom_free(geom);
				elog(ERROR, "RASTER_clip: Invalid data type for band indexes");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(
			array, etype,
			typlen, typbyval, typalign,
			&e, &nulls, &(arg->numbands)
		);

		arg->band = palloc(sizeof(struct rtpg_clip_band_t) * arg->numbands);
		if (arg->band == NULL) {
			rtpg_clip_arg_destroy(arg);
			PG_FREE_IF_COPY(pgraster, 0);
			lwgeom_free(geom);
			elog(ERROR, "RASTER_clip: Could not allocate memory for band arguments");
			PG_RETURN_NULL();
		}

		for (i = 0, j = 0; i < arg->numbands; i++) {
			if (nulls[i]) continue;

			switch (etype) {
				case INT2OID:
					arg->band[j].nband = DatumGetInt16(e[i]) - 1;
					break;
				case INT4OID:
					arg->band[j].nband = DatumGetInt32(e[i]) - 1;
					break;
			}

			j++;
		}

		if (j < arg->numbands) {
			arg->band = repalloc(arg->band, sizeof(struct rtpg_clip_band_t) * j);
			if (arg->band == NULL) {
				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				lwgeom_free(geom);
				elog(ERROR, "RASTER_clip: Could not reallocate memory for band arguments");
				PG_RETURN_NULL();
			}

			arg->numbands = j;
		}

		/* validate band */
		for (i = 0; i < arg->numbands; i++) {
			if (!rt_raster_has_band(arg->raster, arg->band[i].nband)) {
				elog(NOTICE, "Band at index %d not found in raster", arg->band[i].nband + 1);
				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				lwgeom_free(geom);
				PG_RETURN_NULL();
			}

			arg->band[i].hasnodata = 0;
			arg->band[i].nodataval = 0;
		}
	}
	else {
		arg->numbands = rt_raster_get_num_bands(arg->raster);

		/* raster may have no bands */
		if (arg->numbands) {
			arg->band = palloc(sizeof(struct rtpg_clip_band_t) * arg->numbands);
			if (arg->band == NULL) {

				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				lwgeom_free(geom);

				elog(ERROR, "RASTER_clip: Could not allocate memory for band arguments");
				PG_RETURN_NULL();
			}

			for (i = 0; i < arg->numbands; i++) {
				arg->band[i].nband = i;
				arg->band[i].hasnodata = 0;
				arg->band[i].nodataval = 0;
			}
		}
	}

	/* nodataval (3) */
	if (!PG_ARGISNULL(3)) {
		array = PG_GETARG_ARRAYTYPE_P(3);
		etype = ARR_ELEMTYPE(array);
		get_typlenbyvalalign(etype, &typlen, &typbyval, &typalign);

		switch (etype) {
			case FLOAT4OID:
			case FLOAT8OID:
				break;
			default:
				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				lwgeom_free(geom);
				elog(ERROR, "RASTER_clip: Invalid data type for NODATA values");
				PG_RETURN_NULL();
				break;
		}

		deconstruct_array(
			array, etype,
			typlen, typbyval, typalign,
			&e, &nulls, &k
		);

		/* it doesn't matter if there are more nodataval */
		for (i = 0, j = 0; i < arg->numbands; i++, j++) {
			/* cap j to the last nodataval element */
			if (j >= k)
				j = k - 1;

			if (nulls[j])
				continue;

			arg->band[i].hasnodata = 1;
			switch (etype) {
				case FLOAT4OID:
					arg->band[i].nodataval = DatumGetFloat4(e[j]);
					break;
				case FLOAT8OID:
					arg->band[i].nodataval = DatumGetFloat8(e[j]);
					break;
			}
		}
	}

	/* get wkb of geometry */
	POSTGIS_RT_DEBUG(3, "getting wkb of geometry");
	wkb = lwgeom_to_wkb(geom, WKB_SFSQL, &wkb_len);
	lwgeom_free(geom);

	/* rasterize geometry */
	arg->mask = rt_raster_gdal_rasterize(
		wkb, wkb_len,
		NULL,
		0, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		&(gt[1]), &(gt[5]),
		NULL, NULL,
		&(gt[0]), &(gt[3]),
		&(gt[2]), &(gt[4]),
		NULL
	);

	pfree(wkb);
	if (arg->mask == NULL) {
		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_clip: Could not rasterize intersection geometry");
		PG_RETURN_NULL();
	}

	/* set SRID */
	rt_raster_set_srid(arg->mask, srid);

	/* run iterator */

	/* init itrset */
	itrset = palloc(sizeof(struct rt_iterator_t) * 2);
	if (itrset == NULL) {
		rtpg_clip_arg_destroy(arg);
		PG_FREE_IF_COPY(pgraster, 0);
		elog(ERROR, "RASTER_clip: Could not allocate memory for iterator arguments");
		PG_RETURN_NULL();
	}

	/* one band at a time */
	for (i = 0; i < arg->numbands; i++) {
		POSTGIS_RT_DEBUGF(4, "band arg %d (nband, hasnodata, nodataval) = (%d, %d, %f)",
			i, arg->band[i].nband, arg->band[i].hasnodata, arg->band[i].nodataval);

		band = rt_raster_get_band(arg->raster, arg->band[i].nband);

		/* band metadata */
		pixtype = rt_band_get_pixtype(band);

		if (arg->band[i].hasnodata) {
			hasnodata = 1;
			nodataval = arg->band[i].nodataval;
		}
		else if (rt_band_get_hasnodata_flag(band)) {
			hasnodata = 1;
			rt_band_get_nodata(band, &nodataval);
		}
		else {
			hasnodata = 0;
			nodataval = rt_band_get_min_value(band);
		}

		/* band is NODATA, create NODATA band and continue */
		if (rt_band_get_isnodata_flag(band)) {
			/* create raster */
			if (rtn == NULL) {
				noerr = rt_raster_from_two_rasters(arg->raster, arg->mask, arg->extenttype, &rtn, NULL);
				if (noerr != ES_NONE) {
					rtpg_clip_arg_destroy(arg);
					PG_FREE_IF_COPY(pgraster, 0);
					elog(ERROR, "RASTER_clip: Could not create output raster");
					PG_RETURN_NULL();
				}
			}

			/* create NODATA band */
			if (rt_raster_generate_new_band(rtn, pixtype, nodataval, hasnodata, nodataval, i) < 0) {
				rt_raster_destroy(rtn);
				rtpg_clip_arg_destroy(arg);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_clip: Could not add NODATA band to output raster");
				PG_RETURN_NULL();
			}

			continue;
		}

		/* raster */
		itrset[0].raster = arg->raster;
		itrset[0].nband = arg->band[i].nband;
		itrset[0].nbnodata = 1;

		/* mask */
		itrset[1].raster = arg->mask;
		itrset[1].nband = 0;
		itrset[1].nbnodata = 1;

		/* pass to iterator */
		noerr = rt_raster_iterator(
			itrset, 2,
			arg->extenttype, NULL,
			pixtype,
			hasnodata, nodataval,
			0, 0,
			NULL,
			rtpg_clip_callback,
			&_raster
		);

		if (noerr != ES_NONE) {
			pfree(itrset);
			rtpg_clip_arg_destroy(arg);
			if (rtn != NULL) rt_raster_destroy(rtn);
			PG_FREE_IF_COPY(pgraster, 0);
			elog(ERROR, "RASTER_clip: Could not run raster iterator function");
			PG_RETURN_NULL();
		}

		/* new raster */
		if (rtn == NULL)
			rtn = _raster;
		/* copy band */
		else {
			band = rt_raster_get_band(_raster, 0);
			if (band == NULL) {
				pfree(itrset);
				rtpg_clip_arg_destroy(arg);
				rt_raster_destroy(_raster);
				rt_raster_destroy(rtn);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_clip: Could not get band from working raster");
				PG_RETURN_NULL();
			}

			if (rt_raster_add_band(rtn, band, i) < 0) {
				pfree(itrset);
				rtpg_clip_arg_destroy(arg);
				rt_raster_destroy(_raster);
				rt_raster_destroy(rtn);
				PG_FREE_IF_COPY(pgraster, 0);
				elog(ERROR, "RASTER_clip: Could not add new band to output raster");
				PG_RETURN_NULL();
			}

			rt_raster_destroy(_raster);
		}
	}

	pfree(itrset);
	rtpg_clip_arg_destroy(arg);
	PG_FREE_IF_COPY(pgraster, 0);

	pgrtn = rt_raster_serialize(rtn);
	rt_raster_destroy(rtn);

	POSTGIS_RT_DEBUG(3, "Finished");

	if (!pgrtn)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrtn, pgrtn->size);
	PG_RETURN_POINTER(pgrtn);
}

/* ---------------------------------------------------------------- */
/*  Memory allocation / error reporting hooks                       */
/*  TODO: reuse the ones in libpgcommon ?                           */
/* ---------------------------------------------------------------- */

static void *
rt_pg_alloc(size_t size)
{
    void * result;

    POSTGIS_RT_DEBUGF(5, "rt_pgalloc(%ld) called", (long int) size);

    result = palloc(size);

    return result;
}

static void *
rt_pg_realloc(void *mem, size_t size)
{
    void * result;

    POSTGIS_RT_DEBUGF(5, "rt_pg_realloc(%ld) called", (long int) size);

    if (mem)
        result = repalloc(mem, size);

    else
        result = palloc(size);

    return result;
}

static void
rt_pg_free(void *ptr)
{
    POSTGIS_RT_DEBUG(5, "rt_pfree called");
    pfree(ptr);
}

static void
rt_pg_error(const char *fmt, va_list ap)
{
#define ERRMSG_MAXLEN 256

    char errmsg[ERRMSG_MAXLEN+1];

    vsnprintf (errmsg, ERRMSG_MAXLEN, fmt, ap);

    errmsg[ERRMSG_MAXLEN]='\0';
    ereport(ERROR, (errmsg_internal("%s", errmsg)));
}

static void
rt_pg_notice(const char *fmt, va_list ap)
{
    char *msg;

    /*
     * This is a GNU extension.
     * Dunno how to handle errors here.
     */
    if (!lw_vasprintf (&msg, fmt, ap))
    {
        va_end (ap);
        return;
    }
    ereport(NOTICE, (errmsg_internal("%s", msg)));
    free(msg);
}


void
rt_init_allocators(void)
{
    /* raster callback - install raster handlers */
    rt_set_handlers(rt_pg_alloc, rt_pg_realloc, rt_pg_free, rt_pg_error,
            rt_pg_notice, rt_pg_notice);
}

