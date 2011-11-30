/*
 * $Id$
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
 * Copyright (C) 2011 Regents of the University of California
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
#include <float.h>
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

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h" /* for clamp_srid() */
#include "rt_pg.h"
#include "pgsql_compat.h"

#include <utils/lsyscache.h> /* for get_typlenbyvalalign */
#include <utils/array.h> /* for ArrayType */
#include <catalog/pg_type.h> /* for INT2OID, INT4OID, FLOAT4OID, FLOAT8OID and TEXTOID */

/* maximum char length required to hold any double or long long value */
#define MAX_DBL_CHARLEN (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#define MAX_INT_CHARLEN 32

/*
 * This is required for builds against pgsql 
 */
PG_MODULE_MAGIC;

/***************************************************************
 * Internal functions must be prefixed with rtpg_.  This is
 * keeping inline with the use of pgis_ for ./postgis C utility
 * functions.
 ****************************************************************/
/* Internal funcs */
static char *rtpg_replace(
	const char *str,
	const char *oldstr, const char *newstr,
	int *count
);
static char *rtpg_strtoupper(char *str);
static char	*rtpg_chartrim(char* input, char *remove); /* for RASTER_reclass */
static char **rtpg_strsplit(const char *str, const char *delimiter, int *n); /* for RASTER_reclass */
static char *rtpg_removespaces(char *str); /* for RASTER_reclass */
static char *rtpg_trim(char* input); /* for RASTER_asGDALRaster */
static char *rtpg_getSRTextSPI(int srid);

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
 * index for addband)
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
 *   like RASTER_dumpAsWKTPolygons (see section 34.9.9 at
 *   http://www.postgresql.org/docs/8.4/static/xfunc-c.html).
 *
 * But raster code follows the same philosophy than the rest of PostGIS: keep
 * memory as clean as possible. So, we free all allocated memory.
 *
 * TODO: In case of functions returning NULL, we should free the memory too.
 *****************************************************************************/

/* Prototypes */

/* Utility functions */
Datum RASTER_lib_version(PG_FUNCTION_ARGS);
Datum RASTER_lib_build_date(PG_FUNCTION_ARGS);
Datum RASTER_gdal_version(PG_FUNCTION_ARGS);

/* Input/output and format conversions */
Datum RASTER_in(PG_FUNCTION_ARGS);
Datum RASTER_out(PG_FUNCTION_ARGS);

Datum RASTER_to_bytea(PG_FUNCTION_ARGS);
Datum RASTER_to_binary(PG_FUNCTION_ARGS);

/* Raster as geometry operations */
Datum RASTER_convex_hull(PG_FUNCTION_ARGS);
Datum RASTER_dumpAsWKTPolygons(PG_FUNCTION_ARGS);

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
Datum RASTER_getRotation(PG_FUNCTION_ARGS);

/* Set all the properties of a raster */
Datum RASTER_setSRID(PG_FUNCTION_ARGS);
Datum RASTER_setScale(PG_FUNCTION_ARGS);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS);
Datum RASTER_setSkew(PG_FUNCTION_ARGS);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS);
Datum RASTER_setRotation(PG_FUNCTION_ARGS);

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

/* Set pixel value */
Datum RASTER_setPixelValue(PG_FUNCTION_ARGS);

/* Raster and band creation */
Datum RASTER_makeEmpty(PG_FUNCTION_ARGS);
Datum RASTER_addband(PG_FUNCTION_ARGS);
Datum RASTER_copyband(PG_FUNCTION_ARGS);

/* Raster analysis */
Datum RASTER_mapAlgebraExpr(PG_FUNCTION_ARGS);
Datum RASTER_mapAlgebraFct(PG_FUNCTION_ARGS);

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

/* convert raster to GDAL raster */
Datum RASTER_asGDALRaster(PG_FUNCTION_ARGS);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS);

/* rasterize a geometry */
Datum RASTER_asRaster(PG_FUNCTION_ARGS);

/* resample a raster */
Datum RASTER_resample(PG_FUNCTION_ARGS);

/* get raster's meta data */
Datum RASTER_metadata(PG_FUNCTION_ARGS);

/* get raster band's meta data */
Datum RASTER_bandmetadata(PG_FUNCTION_ARGS);

/* determine if two rasters intersect */
Datum RASTER_intersects(PG_FUNCTION_ARGS);

/* determine if two rasters are aligned */
Datum RASTER_sameAlignment(PG_FUNCTION_ARGS);

/* two-raster MapAlgebra */
Datum RASTER_mapAlgebra2(PG_FUNCTION_ARGS);

/* one-raster neighborhood MapAlgebra */
Datum RASTER_mapAlgebraFctNgb(PG_FUNCTION_ARGS);

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
rtpg_replace(const char *str, const char *oldstr, const char *newstr, int *count)
{
   const char *tmp = str;
   char *result;
   int   found = 0;
   int   length, reslen;
   int   oldlen = strlen(oldstr);
   int   newlen = strlen(newstr);
   int   limit = (count != NULL && *count > 0) ? *count : -1;

   tmp = str;
   while ((tmp = strstr(tmp, oldstr)) != NULL && found != limit)
      found++, tmp += oldlen;

   length = strlen(str) + found * (newlen - oldlen);

   if ( (result = (char *)palloc(length+1)) == NULL) {
      fprintf(stderr, "Not enough memory\n");
      found = -1;
   } else {
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
      strcpy(result + reslen, str); /* Copies last part and ending nul char */
   }
   if (count != NULL) *count = found;
   return result;
}


static char *
rtpg_strtoupper(char * str)
{
    int j;

    for(j = 0; j < strlen(str); j++)
        str[j] = toupper(str[j]);

    return str;

}

static char*
rtpg_chartrim(char *input, char *remove) {
	char *start;
	char *ptr;

	if (!input) {
		return NULL;
	}
	else if (!*input) {
		return input;
	}

	start = (char *) palloc(sizeof(char) * strlen(input) + 1);
	if (NULL == start) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	strcpy(start, input);

	/* trim left */
	while (strchr(remove, *start) != NULL) {
		start++;
	}

	/* trim right */
	ptr = start + strlen(start);
	while (strchr(remove, *--ptr) != NULL);
	*(++ptr) = '\0';

	return start;
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

	rtn = rtpg_replace(str, " ", "", NULL);
	rtn = rtpg_replace(rtn, "\n", "", NULL);
	rtn = rtpg_replace(rtn, "\t", "", NULL);
	rtn = rtpg_replace(rtn, "\f", "", NULL);
	rtn = rtpg_replace(rtn, "\r", "", NULL);

	return rtn;
}

static char*
rtpg_trim(char *input) {
	char *start;
	char *ptr;

	if (!input) {
		return NULL;
	}
	else if (!*input) {
		return input;
	}

	start = (char *) palloc(sizeof(char) * strlen(input) + 1);
	if (NULL == start) {
		fprintf(stderr, "Not enough memory\n");
		return NULL;
	}
	strcpy(start, input);

	/* trim left */
	while (isspace(*start)) {
		start++;
	}

	/* trim right */
	ptr = start + strlen(start);
	while (isspace(*--ptr));
	*(++ptr) = '\0';

	return start;
}

static char*
rtpg_getSRTextSPI(int srid)
{
	int len = 0;
	char *sql = NULL;
	int spi_result;
	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	char *tmp = NULL;
	char *srs = NULL;

	len = sizeof(char) * (strlen("SELECT srtext FROM spatial_ref_sys WHERE srid =  LIMIT 1") + MAX_INT_CHARLEN + 1);
	sql = (char *) palloc(len);
	if (NULL == sql) {
		elog(ERROR, "getSrtextSPI: Unable to allocate memory for sql\n");
		return NULL;
	}

	spi_result = SPI_connect();
	if (spi_result != SPI_OK_CONNECT) {
		elog(ERROR, "getSrtextSPI: Could not connect to database using SPI\n");
		pfree(sql);
		return NULL;
	}

	/* execute query */
	snprintf(sql, len, "SELECT srtext FROM spatial_ref_sys WHERE srid = %d LIMIT 1", srid);
	spi_result = SPI_execute(sql, TRUE, 0);
	pfree(sql);
	if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
		elog(ERROR, "getSrtextSPI: Cannot find SRID (%d) in spatial_ref_sys", srid);
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		return NULL;
	}

	tupdesc = SPI_tuptable->tupdesc;
	tuptable = SPI_tuptable;
	tuple = tuptable->vals[0];

	tmp = SPI_getvalue(tuple, tupdesc, 1);
	if (NULL == tmp || !strlen(tmp)) {
		elog(ERROR, "getSrtextSPI: Cannot find SRID (%d) in spatial_ref_sys", srid);
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		return NULL;
	}

	SPI_freetuptable(tuptable);
	SPI_finish();

	len = strlen(tmp);
	srs = (char *) palloc(sizeof(char) * (len + 1));
	if (NULL == srs) {
		elog(ERROR, "getSrtextSPI: Unable to allocate memory for srtext\n");
		return NULL;
	}
	srs = strncpy(srs, tmp, len + 1);
	pfree(tmp);

	return srs;
}

PG_FUNCTION_INFO_V1(RASTER_lib_version);
Datum RASTER_lib_version(PG_FUNCTION_ARGS)
{
    char *ver = POSTGIS_LIB_VERSION;
    text *result;
    result = palloc(VARHDRSZ  + strlen(ver));
    SET_VARSIZE(result, VARHDRSZ + strlen(ver));
    memcpy(VARDATA(result), ver, strlen(ver));
    PG_RETURN_POINTER(result);
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
	text *result = cstring2text(ver);
	PG_RETURN_POINTER(result);
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

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    result = rt_raster_serialize(raster);

    SET_VARSIZE(result, ((rt_pgraster*)result)->size);

    rt_raster_destroy(raster);

    if ( NULL != result )
        PG_RETURN_POINTER(result);
    else
        PG_RETURN_NULL();
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

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		elog(ERROR, "RASTER_out: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	hexwkb = rt_raster_to_hexwkb(raster, &hexwkbsize);
	if (!hexwkb) {
		elog(ERROR, "RASTER_out: Could not HEX-WKBize raster");
		PG_RETURN_NULL();
	}

	/* Free the raster objects used */
	rt_raster_destroy(raster);

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
		elog(ERROR, "RASTER_to_bytea: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* Parse raster to wkb object */
	wkb = rt_raster_to_wkb(raster, &wkb_size);
	if (!wkb) {
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
	rtdealloc(wkb);

	PG_RETURN_POINTER(result);
}

/**
 * Return bytea object with raster requested using ST_AsBinary function.
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

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Get raster object */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		elog(ERROR, "RASTER_to_binary: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* Parse raster to wkb object */
	wkb = rt_raster_to_wkb(raster, &wkb_size);
	if (!wkb) {
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
	rtdealloc(wkb);

	PG_RETURN_POINTER(result);
}

/**
 * Return the convex hull of this raster as a 4-vertices POLYGON.
 */
PG_FUNCTION_INFO_V1(RASTER_convex_hull);
Datum RASTER_convex_hull(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    LWPOLY* convexhull = NULL;
    GSERIALIZED* gser = NULL;

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    {
        raster = rt_raster_deserialize(pgraster, TRUE);
        if ( ! raster ) {
            elog(ERROR, "RASTER_convex_hull: Could not deserialize raster");
            PG_RETURN_NULL();
        }

        convexhull = rt_raster_get_convex_hull(raster);
        if ( ! convexhull ) {
            elog(ERROR, "RASTER_convex_hull: Could not get raster's convex hull");
            PG_RETURN_NULL();
        }
    }

    {
        size_t gser_size;
        gser = gserialized_from_lwgeom(lwpoly_as_lwgeom(convexhull), 0, &gser_size);
        SET_VARSIZE(gser, gser_size);
    }

    /* Free raster and lwgeom memory */
    rt_raster_destroy(raster);
    lwfree(convexhull);

    PG_RETURN_POINTER(gser);
}


PG_FUNCTION_INFO_V1(RASTER_dumpAsWKTPolygons);
Datum RASTER_dumpAsWKTPolygons(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    FuncCallContext *funcctx;
    TupleDesc tupdesc;
    int nband;
    rt_geomval geomval;
    rt_geomval geomval2;
    int call_cntr;
    int max_calls;
    int nElements;
    MemoryContext oldcontext;

    /* stuff done only on the first call of the function */
    if (SRF_IS_FIRSTCALL())
    {
        POSTGIS_RT_DEBUG(2, "RASTER_dumpAsWKTPolygons first call");

        /* create a function context for cross-call persistence */
        funcctx = SRF_FIRSTCALL_INIT();

        /* switch to memory context appropriate for multiple function calls */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /* Get input arguments */
				if (PG_ARGISNULL(0)) SRF_RETURN_DONE(funcctx);
        pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        raster = rt_raster_deserialize(pgraster, FALSE);
        if ( ! raster )
        {
            ereport(ERROR,
                    (errcode(ERRCODE_OUT_OF_MEMORY),
                    errmsg("Could not deserialize raster")));
            SRF_RETURN_DONE(funcctx);
        }

        if (PG_NARGS() == 2)
            nband = PG_GETARG_UINT32(1);
        else
            nband = 1; /* By default, first band */

        POSTGIS_RT_DEBUGF(3, "band %d", nband);

        /* Polygonize raster */

        /**
         * Dump raster
         */
        geomval = rt_raster_dump_as_wktpolygons(raster, nband - 1, &nElements);
				rt_raster_destroy(raster);
        if (NULL == geomval)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_NO_DATA_FOUND),
                    errmsg("Could not polygonize raster")));
            SRF_RETURN_DONE(funcctx);
        }

        POSTGIS_RT_DEBUGF(3, "raster dump, %d elements returned", nElements);

        /* Store needed information */
        funcctx->user_fctx = geomval;

        /* total number of tuples to be returned */
        funcctx->max_calls = nElements;

        /* Build a tuple descriptor for our result type */
        if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                            "that cannot accept type record")));

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

    if (call_cntr < max_calls)    /* do when there is more left to send */
    {
        bool *nulls = NULL;
        int values_length = 3;
        Datum values[values_length];
        HeapTuple    tuple;
        Datum        result;

        POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);

        nulls = palloc(sizeof(bool) * values_length);
				memset(nulls, FALSE, values_length);

        values[0] = CStringGetTextDatum(geomval2[call_cntr].geom);
        values[1] = Float8GetDatum(geomval2[call_cntr].val);
        values[2] = Int32GetDatum(geomval2[call_cntr].srid);

        POSTGIS_RT_DEBUGF(4, "Result %d, Polygon %s", call_cntr, geomval2[call_cntr].geom);
        POSTGIS_RT_DEBUGF(4, "Result %d, val %f", call_cntr, geomval2[call_cntr].val);
        POSTGIS_RT_DEBUGF(4, "Result %d, srid %d", call_cntr, geomval2[call_cntr].srid);

        /**
         * Free resources.
         */
        pfree(geomval2[call_cntr].geom);

        /* build a tuple */
        tuple = heap_form_tuple(tupdesc, values, nulls);

        /* make the tuple into a datum */
        result = HeapTupleGetDatum(tuple);

        /* clean up (this is not really necessary) */
        pfree(nulls);


        SRF_RETURN_NEXT(funcctx, result);
    }
    else    /* do when there is no more left */
    {
        pfree(geomval2);
        SRF_RETURN_DONE(funcctx);
    }

}


/**
 * rt_MakeEmptyRaster( <width>, <height>, <ipx>, <ipy>,
 *                                        <scalex>, <scaley>,
 *                                        <skewx>, <skewy>,
 *                                        <srid>)
 */
PG_FUNCTION_INFO_V1(RASTER_makeEmpty);
Datum RASTER_makeEmpty(PG_FUNCTION_ARGS)
{
    uint16 width, height;
    double ipx, ipy, scalex, scaley, skewx, skewy;
    int32_t srid;
    rt_pgraster *pgraster;
    rt_raster raster;

    if ( PG_NARGS() < 9 )
    {
        elog(ERROR, "RASTER_makeEmpty: ST_MakeEmptyRaster requires 9 args");
        PG_RETURN_NULL();
    }

    if (PG_ARGISNULL(0))
        width = 0;
    else
        width = PG_GETARG_UINT16(0);

    if (PG_ARGISNULL(1))
        height = 0;
    else
        height = PG_GETARG_UINT16(1);

    if (PG_ARGISNULL(2))
        ipx = 0;
    else
        ipx = PG_GETARG_FLOAT8(2);

    if (PG_ARGISNULL(3))
        ipy = 0;
    else
        ipy = PG_GETARG_FLOAT8(3);

    if (PG_ARGISNULL(4))
        scalex = 0;
    else
        scalex = PG_GETARG_FLOAT8(4);

    if (PG_ARGISNULL(5))
        scaley = 0;
    else
        scaley = PG_GETARG_FLOAT8(5);

    if (PG_ARGISNULL(6))
        skewx = 0;
    else
        skewx = PG_GETARG_FLOAT8(6);

    if (PG_ARGISNULL(7))
        skewy = 0;
    else
        skewy = PG_GETARG_FLOAT8(7);

    if (PG_ARGISNULL(8))
        srid = SRID_UNKNOWN;
    else
        srid = PG_GETARG_INT32(8);

    POSTGIS_RT_DEBUGF(4, "%dx%d, ip:%g,%g, scale:%g,%g, skew:%g,%g srid:%d",
                  width, height, ipx, ipy, scalex, scaley,
                  skewx, skewy, srid);

    raster = rt_raster_new(width, height);
    if ( ! raster ) {
        PG_RETURN_NULL(); /* error was supposedly printed already */
    }

    rt_raster_set_scale(raster, scalex, scaley);
    rt_raster_set_offsets(raster, ipx, ipy);
    rt_raster_set_skews(raster, skewx, skewy);
    rt_raster_set_srid(raster, srid);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getSRID: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    srid = rt_raster_get_srid(raster);

    rt_raster_destroy(raster);

    PG_RETURN_INT32(srid);
}

/**
 * Set the SRID associated with the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSRID);
Datum RASTER_setSRID(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster;
	int32_t newSRID = PG_GETARG_INT32(1);

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		elog(ERROR, "RASTER_setSRID: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	rt_raster_set_srid(raster, newSRID);

	pgraster = rt_raster_serialize(raster);
	if (!pgraster) PG_RETURN_NULL();

	SET_VARSIZE(pgraster, pgraster->size);
	rt_raster_destroy(raster);
	PG_RETURN_POINTER(pgraster);
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
        elog(ERROR, "RASTER_getWidth: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    width = rt_raster_get_width(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getHeight: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    height = rt_raster_get_height(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getNumBands: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    num_bands = rt_raster_get_num_bands(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getXScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xsize = rt_raster_get_x_scale(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getYScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    ysize = rt_raster_get_y_scale(raster);

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(ysize);
}

/**
 * Set the scale of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setScale);
Datum RASTER_setScale(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster;
    double size = PG_GETARG_FLOAT8(1);

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(raster, size, size);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
}

/**
 * Set the pixel size of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setScaleXY);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster;
    double xscale = PG_GETARG_FLOAT8(1);
    double yscale = PG_GETARG_FLOAT8(2);

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setScaleXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(raster, xscale, yscale);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
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
        elog(ERROR, "RASTER_getXSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xskew = rt_raster_get_x_skew(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getYSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yskew = rt_raster_get_y_skew(raster);

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(yskew);
}

/**
 * Set the skew of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSkew);
Datum RASTER_setSkew(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster;
    double skew = PG_GETARG_FLOAT8(1);

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(raster, skew, skew);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
}

/**
 * Set the skew of the raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setSkewXY);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster;
    double xskew = PG_GETARG_FLOAT8(1);
    double yskew = PG_GETARG_FLOAT8(2);

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setSkewXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(raster, xskew, yskew);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
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
        elog(ERROR, "RASTER_getXUpperLeft: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xul = rt_raster_get_x_offset(raster);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getYUpperLeft: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yul = rt_raster_get_y_offset(raster);

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(yul);
}

/**
 * Set the raster offset in the X and Y dimension.
 */
PG_FUNCTION_INFO_V1(RASTER_setUpperLeftXY);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster;
    double xoffset = PG_GETARG_FLOAT8(1);
    double yoffset = PG_GETARG_FLOAT8(2);

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setUpperLeftXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_offsets(raster, xoffset, yoffset);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
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
        elog(ERROR, "RASTER_getPixelWidth: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xscale = rt_raster_get_x_scale(raster);
    yskew = rt_raster_get_y_skew(raster);
    pwidth = sqrt(xscale*xscale + yskew*yskew);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getPixelHeight: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yscale = rt_raster_get_y_scale(raster);
    xskew = rt_raster_get_x_skew(raster);
    pheight = sqrt(yscale*yscale + xskew*xskew);

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(pheight);
}

/**
 * Return the raster rotation. The raster rotation is calculated from
 * the scale and skew values stored in the georeference. If the scale
 * and skew values indicate that the raster is not uniformly rotated
 * (the pixels are diamond-shaped), this function will return NaN.
 */
PG_FUNCTION_INFO_V1(RASTER_getRotation);
Datum RASTER_getRotation(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster;
    rt_raster raster;
    double xscale, xskew, yscale, yskew, xrot, yrot;

    if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

    raster = rt_raster_deserialize(pgraster, TRUE);
    if (!raster) {
        elog(ERROR, "RASTER_getRotation: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xscale = rt_raster_get_x_scale(raster);
    yscale = rt_raster_get_y_scale(raster);

    if (xscale == 0 || yscale == 0) {
        rt_raster_destroy(raster);

        /* cannot compute scale with a zero denominator */
        elog(NOTICE, "RASTER_getRotation: Could not divide by zero scale; cannot determine raster rotation.");
        PG_RETURN_FLOAT8(NAN);
    }

    xskew = rt_raster_get_x_skew(raster);
    yskew = rt_raster_get_y_skew(raster);

    xrot = atan(yskew/xscale);
    yrot = atan(xskew/yscale);

    rt_raster_destroy(raster);

    if (xrot == yrot) {
        PG_RETURN_FLOAT8(xrot);
    }

    PG_RETURN_FLOAT8(NAN);
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
    rt_raster raster;
    double rotation = PG_GETARG_FLOAT8(1);
    double xscale, yscale, xskew, yskew, psize;

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
		pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    /* no matter what, we don't rotate more than once around */
    if (rotation < 0) {
        rotation = (-2*M_PI) + fmod(rotation, (2*M_PI));
    }
    else {
        rotation = fmod(rotation, (2 * M_PI));
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if (! raster ) {
        elog(ERROR, "RASTER_setRotation: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xscale = rt_raster_get_x_scale(raster);
    yskew = rt_raster_get_y_skew(raster);
    psize = sqrt(xscale*xscale + yskew*yskew);
    xscale = psize * cos(rotation);
    yskew = psize * sin(rotation);
    
    yscale = rt_raster_get_y_scale(raster);
    xskew = rt_raster_get_x_skew(raster);
    psize = sqrt(yscale*yscale + xskew*xskew);
    yscale = psize * cos(rotation);
    xskew = psize * sin(rotation); 

    rt_raster_set_scale(raster, xscale, yscale);
    rt_raster_set_skews(raster, xskew, yskew);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
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
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        elog(ERROR, "RASTER_getBandPixelType: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel type. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(band);

    rt_raster_destroy(raster);

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
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        elog(ERROR, "RASTER_getBandPixelTypeName: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel type name. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(band);

    result = palloc(VARHDRSZ + name_size);
    /* We don't need to check for NULL pointer, because if out of memory, palloc
     * exit via elog(ERROR). It never returns NULL.
     */

    memset(VARDATA(result), 0, name_size);
    ptr = (char *)result + VARHDRSZ;

    switch (pixtype)
    {
        case PT_1BB:  /* 1-bit boolean            */
            strcpy(ptr, "1BB");
            break;
        case PT_2BUI: /* 2-bit unsigned integer   */
            strcpy(ptr, "2BUI");
            break;
        case PT_4BUI: /* 4-bit unsigned integer   */
            strcpy(ptr, "4BUI");
            break;
        case PT_8BSI: /* 8-bit signed integer     */
            strcpy(ptr, "8BSI");
            break;
        case PT_8BUI: /* 8-bit unsigned integer   */
            strcpy(ptr, "8BUI");
            break;
        case PT_16BSI:/* 16-bit signed integer    */
            strcpy(ptr, "16BSI");
            break;
        case PT_16BUI:/* 16-bit unsigned integer  */
            strcpy(ptr, "16BUI");
            break;
        case PT_32BSI:/* 32-bit signed integer    */
            strcpy(ptr, "32BSI");
            break;
        case PT_32BUI:/* 32-bit unsigned integer  */
            strcpy(ptr, "32BUI");
            break;
        case PT_32BF: /* 32-bit float             */
            strcpy(ptr, "32BF");
            break;
        case PT_64BF: /* 64-bit float             */

            strcpy(ptr, "64BF");
            break;
        default:      /* PT_END=13 */
            elog(ERROR, "RASTER_getBandPixelTypeName: Invalid value of pixel type");
            pfree(result);
            PG_RETURN_NULL();
    }

    size = VARHDRSZ + strlen(ptr);
    SET_VARSIZE(result, size);

    rt_raster_destroy(raster);

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
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        elog(ERROR, "RASTER_getBandNoDataValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting band nodata value. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }

    if ( ! rt_band_get_hasnodata_flag(band) ) {
        /* Raster does not have a nodata value set so we return NULL */
        PG_RETURN_NULL();
    }

    nodata = rt_band_get_nodata(band);

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(nodata);
}


/**
 * Set the nodata value of the specified band of raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setBandNoDataValue);
Datum RASTER_setBandNoDataValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    double nodata;
    int32_t bandindex;
    bool forcechecking = FALSE;
    bool skipset = FALSE;

    /* Deserialize raster */
		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

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
    if (! raster) {
        elog(ERROR, "RASTER_setBandNoDataValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    if (! skipset) {
        assert(0 <= (bandindex - 1));
         /* Fetch requested band */
        band = rt_raster_get_band(raster, bandindex - 1);
        if (! band) {
            elog(NOTICE, "Could not find raster band of index %d when setting pixel value. Nodata value not set. Returning original raster", bandindex);
        }
        else {
            if (!PG_ARGISNULL(3))
                forcechecking = PG_GETARG_BOOL(3);

            if (PG_ARGISNULL(2)) {
                /* Set the hasnodata flag to FALSE */
                rt_band_set_hasnodata_flag(band, FALSE);

                POSTGIS_RT_DEBUGF(3, "Raster band %d does not have a nodata value",
                        bandindex);
            }
            else {

                /* Get the nodata value */
                nodata = PG_GETARG_FLOAT8(2);

                /* Set the band's nodata value */
                rt_band_set_nodata(band, nodata);

                /* Set the hasnodata flag to TRUE */
                rt_band_set_hasnodata_flag(band, TRUE);

                /* Recheck all pixels if requested */
                if (forcechecking)
                   rt_band_check_is_nodata(band);
            }
        }
    }

    pgraster = rt_raster_serialize(raster);
    if (! pgraster) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
}

PG_FUNCTION_INFO_V1(RASTER_setBandIsNoData);
Datum RASTER_setBandIsNoData(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    int32_t bandindex;

		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
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

        if ( ! band )
            elog(NOTICE, "Could not find raster band of index %d. Isnodata flag not set. Returning original raster", bandindex);
        else
            /* Set the band's nodata value */
            rt_band_set_isnodata_flag(band, 1);
    }

    /* Serialize raster again */
    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
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
        elog(ERROR, "RASTER_bandIsNoData: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when determining if band is nodata. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }

    forcechecking = PG_GETARG_BOOL(2);

    bandisnodata = (forcechecking) ?
        rt_band_check_is_nodata(band) : rt_band_get_isnodata_flag(band);

    rt_raster_destroy(raster);

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
        elog(ERROR, "RASTER_getBandPath: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its nodata value */
    band = rt_raster_get_band(raster, bandindex - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d when getting band path. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }

    bandpath = rt_band_get_ext_path(band);
    if ( ! bandpath )
    {
        PG_RETURN_NULL();
    }

    result = (text *) palloc(VARHDRSZ + strlen(bandpath) + 1);

    SET_VARSIZE(result, VARHDRSZ + strlen(bandpath) + 1);

    strcpy((char *) VARDATA(result), bandpath);

    rt_raster_destroy(raster);

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
    bool hasnodata = TRUE;

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    x = PG_GETARG_INT32(2);

    y = PG_GETARG_INT32(3);

    hasnodata = PG_GETARG_BOOL(4);

    POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

    /* Deserialize raster */
		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if (!raster) {
        elog(ERROR, "RASTER_getPixelValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch Nth band using 0-based internal index */
    band = rt_raster_get_band(raster, bandindex - 1);
    if (! band) {
        elog(NOTICE, "Could not find raster band of index %d when getting pixel "
                "value. Returning NULL", bandindex);
        PG_RETURN_NULL();
    }
    /* Fetch pixel using 0-based coordinates */
    result = rt_band_get_pixel(band, x - 1, y - 1, &pixvalue);

    /* If the result is -1 or the value is nodata and we take nodata into account
     * then return nodata = NULL */
    if (result == -1 || (hasnodata && rt_band_get_hasnodata_flag(band) &&
            pixvalue == rt_band_get_nodata(band))) {
        PG_RETURN_NULL();
    }

    rt_raster_destroy(raster);

    PG_RETURN_FLOAT8(pixvalue);
}

/**
 * Write value of raster sample on given position and in specified band.
 */
PG_FUNCTION_INFO_V1(RASTER_setPixelValue);
Datum RASTER_setPixelValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    double pixvalue = 0;
    int32_t bandindex = 0;
    int32_t x = 0;
    int32_t y = 0;
    bool skipset = FALSE;

    if (PG_ARGISNULL(0)) {
        PG_RETURN_NULL();
    }

    /* Check index is not NULL or < 1 */
    if (PG_ARGISNULL(1))
        bandindex = -1;
    else
        bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Value not set. "
                "Returning original raster");
        skipset = TRUE;
    }

    /* Validate pixel coordinates are not null */
    if (PG_ARGISNULL(2)) {
        elog(NOTICE, "X coordinate can not be NULL when getting pixel value. "
                "Value not set. Returning original raster");
        skipset = TRUE;
    }
    else
        x = PG_GETARG_INT32(2);

    if (PG_ARGISNULL(3)) {
        elog(NOTICE, "Y coordinate can not be NULL when getting pixel value. "
                "Value not set. Returning original raster");
        skipset = TRUE;
    }
    else
        y = PG_GETARG_INT32(3);

    POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

    /* Deserialize raster */
		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        elog(ERROR, "RASTER_setPixelValue: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    if (! skipset) {
        /* Fetch requested band */
        band = rt_raster_get_band(raster, bandindex - 1);
        if ( ! band ) {
            elog(NOTICE, "Could not find raster band of index %d when setting "
                    "pixel value. Value not set. Returning original raster",
                    bandindex);
        }
        else {
            /* Set the pixel value */
            if (PG_ARGISNULL(4)) {
                if (!rt_band_get_hasnodata_flag(band)) {
                    elog(NOTICE, "Raster do not have a nodata value defined. "
                            "Set band nodata value first. Nodata value not set. "
                            "Returning original raster");
                }
                else {
                    pixvalue = rt_band_get_nodata(band);
                    rt_band_set_pixel(band, x - 1, y - 1, pixvalue);
                }
            }
            else {
                pixvalue = PG_GETARG_FLOAT8(4);
                rt_band_set_pixel(band, x - 1, y - 1, pixvalue);
            }
        }
    }

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
}

/**
 * Add a band to the given raster at the given position.
 */
PG_FUNCTION_INFO_V1(RASTER_addband);
Datum RASTER_addband(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;

    int bandindex = 0;
    double initialvalue = 0;
    double nodatavalue = 0;
    bool hasnodata = FALSE;
    bool skipaddband = FALSE;

    text *pixeltypename = NULL;
    char *new_pixeltypename = NULL;

    rt_pixtype pixtype = PT_END;
    int32_t oldnumbands = 0;
    int32_t numbands = 0;

    /* Get the initial pixel value */
    if (PG_ARGISNULL(3))
        initialvalue = 0;
    else
        initialvalue = PG_GETARG_FLOAT8(3);

    /* Get the nodata value */
    if (PG_ARGISNULL(4))
        nodatavalue = 0;
    else
    {
        nodatavalue = PG_GETARG_FLOAT8(4);
        hasnodata = TRUE;
    }

    /* Deserialize raster */
		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    /* Get the pixel type in text form */
    if (PG_ARGISNULL(2)) {
        elog(ERROR, "RASTER_addband: Pixel type can not be NULL");
        PG_RETURN_NULL();
    }

    pixeltypename = PG_GETARG_TEXT_P(2);
		new_pixeltypename = text_to_cstring(pixeltypename);

    /* Get the pixel type index */
    pixtype = rt_pixtype_index_from_name(new_pixeltypename);
    if ( pixtype == PT_END ) {
        elog(ERROR, "RASTER_addband: Invalid pixel type: %s", new_pixeltypename);
        PG_RETURN_NULL();
    }

    raster = rt_raster_deserialize(pgraster, FALSE);
    if ( ! raster ) {
        elog(ERROR, "RASTER_addband: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Make sure index (1 based) is in a valid range */
    oldnumbands = rt_raster_get_num_bands(raster);
    if (PG_ARGISNULL(1))
        bandindex = oldnumbands + 1;
    else
    {
        bandindex = PG_GETARG_INT32(1);
        if (bandindex < 1) {
            elog(NOTICE, "Invalid band index (must use 1-based). Band not added. "
                    "Returning original raster");
            skipaddband = TRUE;
        }
        if (bandindex > oldnumbands + 1) {
            elog(NOTICE, "RASTER_addband: Band index number exceed possible "
                    "values, truncated to number of band (%u) + 1", oldnumbands);
            bandindex = oldnumbands + 1;
        }
    }

    if (!skipaddband) {
        bandindex = rt_raster_generate_new_band(raster, pixtype, initialvalue,
                                                hasnodata, nodatavalue,
                bandindex - 1);

        numbands = rt_raster_get_num_bands(raster);
        if (numbands == oldnumbands || bandindex == -1) {
            elog(ERROR, "RASTER_addband: Could not add band to raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }
    }

    pgraster = rt_raster_serialize(raster);
    if (!pgraster) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(raster);

    PG_RETURN_POINTER(pgraster);
}


/**
 * Copy a band from one raster to another one at the given position.
 */
PG_FUNCTION_INFO_V1(RASTER_copyband);
Datum RASTER_copyband(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster torast = NULL;
    rt_raster fromrast = NULL;
    int toindex = 0;
    int fromband = 0;
    int oldtorastnumbands = 0;
    int newtorastnumbands = 0;
    int newbandindex = 0;

    /* Deserialize torast */
		if (PG_ARGISNULL(0)) PG_RETURN_NULL();
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    torast = rt_raster_deserialize(pgraster, FALSE);
    if ( ! torast ) {
        elog(ERROR, "RASTER_copyband: Could not deserialize first raster");
        PG_RETURN_NULL();
    }

    /* Deserialize fromrast */
    if (!PG_ARGISNULL(1)) {
        pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

        fromrast = rt_raster_deserialize(pgraster, FALSE);
        if ( ! fromrast ) {
            elog(ERROR, "RASTER_copyband: Could not deserialize second raster");
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
        newbandindex = rt_raster_copy_band(torast, fromrast, fromband - 1,
            toindex - 1);

        newtorastnumbands = rt_raster_get_num_bands(torast);
        if (newtorastnumbands == oldtorastnumbands || newbandindex == -1) {
            elog(NOTICE, "RASTER_copyband: Could not add band to raster. "
                    "Returning original raster.");
        }
    }

    /* Serialize and return torast */
    pgraster = rt_raster_serialize(torast);
    if (!pgraster) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);

    rt_raster_destroy(fromrast);
    rt_raster_destroy(torast);

    PG_RETURN_POINTER(pgraster);
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
        PG_RETURN_NULL();
    }

    isempty = rt_raster_is_empty(raster);

    rt_raster_destroy(raster);

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
        PG_RETURN_NULL();
    }

    /* Get band number */
    bandindex = PG_GETARG_INT32(1);
    hasnoband = rt_raster_has_no_band(raster, bandindex - 1);

    rt_raster_destroy(raster);

    PG_RETURN_BOOL(hasnoband);
}

PG_FUNCTION_INFO_V1(RASTER_mapAlgebraExpr);
Datum RASTER_mapAlgebraExpr(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
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
    char strnewval[50];
    int count = 0;
    int len = 0;
    int ret = -1;
    TupleDesc tupdesc;
    SPITupleTable * tuptable = NULL;
    HeapTuple tuple;
    char * strFromText = NULL;

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
        elog(ERROR, "RASTER_mapAlgebraExpr: Could not create a new raster. "
                "Returning NULL");
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

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {

            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }


    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (rt_raster_has_no_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
                "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
                "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

     /*
     * Get NODATA value
     */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        newnodatavalue = rt_band_get_nodata(band);
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
        lwfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }

    if (newpixeltype == PT_END) {
        elog(ERROR, "RASTER_mapAlgebraExpr: Invalid pixeltype. Returning NULL");
        PG_RETURN_NULL();
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Pixeltype set to %s",
        rt_pixtype_name(newpixeltype));


    /* Construct expression for raster values */
    if (!PG_ARGISNULL(3)) {
        expression = text_to_cstring(PG_GETARG_TEXT_P(3));
        len = strlen("SELECT ") + strlen(expression);
        initexpr = (char *)palloc(len + 1);

        strncpy(initexpr, "SELECT ", strlen("SELECT "));
        strncpy(initexpr + strlen("SELECT "), rtpg_strtoupper(expression),
            strlen(expression));
        initexpr[len] = '\0';

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Expression is %s", initexpr);

        /* We don't need this memory */
        /*
				lwfree(expression);
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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }


    /**
     * Optimization: If expression resume to 'RAST' and hasnodataval is zero,
		 * we can just return the band from the original raster
     **/
    if (initexpr != NULL && !strcmp(initexpr, "SELECT RAST") && !hasnodataval) {

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Expression resumes to RAST. "
                "Returning raster with band %d from original raster", nband);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebraExpr: New raster has %d bands",
                rt_raster_get_num_bands(newrast));

        rt_raster_copy_band(raster, newrast, nband - 1, 0);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebraExpr: New raster now has %d bands",
                rt_raster_get_num_bands(newrast));

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /**
     * Optimization: If expression resume to a constant (it does not contain
     * RAST)
     **/
    if (initexpr != NULL && strstr(initexpr, "RAST") == NULL) {

        SPI_connect();

        /* Execute the expresion into newval */
        ret = SPI_execute(initexpr, FALSE, 0);

        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Invalid construction for expression."
                    " Aborting");

            /* Free memory allocated out of the current context */
            if (SPI_tuptable)
                SPI_freetuptable(tuptable);

            SPI_finish();
            PG_RETURN_NULL();
        }

        tupdesc = SPI_tuptable->tupdesc;
        tuptable = SPI_tuptable;

        tuple = tuptable->vals[0];
        newval = atof(SPI_getvalue(tuple, tupdesc, 1));

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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    RASTER_DEBUG(3, "RASTER_mapAlgebraExpr: Creating new raster band...");

    /* Get the new raster band */
    newband = rt_raster_get_band(newrast, 0);
    if ( NULL == newband ) {
        elog(NOTICE, "Could not modify band for new raster. Returning new "
                "raster with the original band");

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraExpr: Could not serialize raster. "
                    "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);


        PG_RETURN_POINTER(pgraster);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: Main computing loop (%d x %d)",
            width, height);

    for (x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            ret = rt_band_get_pixel(band, x, y, &r);

            /**
             * We compute a value only for the withdata value pixel since the
             * nodata value has already been set by the first optimization
             **/
            if (ret != -1 && FLT_NEQ(r, newnodatavalue)) {
                if (skipcomputation == 0) {
                    sprintf(strnewval, "%f", r);

                    if (initexpr != NULL) {
												count = 0;
                        newexpr = rtpg_replace(initexpr, "RAST", strnewval, &count);

                        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: (%dx%d), "
                                "r = %s, newexpr = %s",
                                x, y, strnewval, newexpr);

                        SPI_connect();

                        ret = SPI_execute(newexpr, FALSE, 0);

                        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL ||
                                SPI_processed != 1) {
                            elog(ERROR, "RASTER_mapAlgebraExpr: Invalid construction"
                                    " for expression. Aborting");

                            pfree(newexpr);
                            if (SPI_tuptable)
                                SPI_freetuptable(tuptable);

                            SPI_finish();
                            PG_RETURN_NULL();
                        }

                        tupdesc = SPI_tuptable->tupdesc;
                        tuptable = SPI_tuptable;

                        tuple = tuptable->vals[0];
                        newval = atof(SPI_getvalue(tuple, tupdesc, 1));

                        SPI_freetuptable(tuptable);

                        SPI_finish();

                        pfree(newexpr);
                    }

                    else
                        newval = newinitialvalue;

                    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraExpr: new value = %f",
                        newval);
                }


                rt_band_set_pixel(newband, x, y, newval);
            }

        }
    }

    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: raster modified, serializing it.");
    /* Serialize created raster */

    pgraster = rt_raster_serialize(newrast);
    if (NULL == pgraster) {
        /* Free memory allocated out of the current context */
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    SET_VARSIZE(pgraster, pgraster->size);

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraExpr: raster serialized");

    /* Free memory */
    if (initexpr)
        pfree(initexpr);

    rt_raster_destroy(raster);
    rt_raster_destroy(newrast);

    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraExpr: returning raster");


    PG_RETURN_POINTER(pgraster);
}

/*
 * One raster user function MapAlgebra.
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebraFct);
Datum RASTER_mapAlgebraFct(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
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

    POSTGIS_RT_DEBUG(2, "RASTER_mapAlgebraFct: STARTING...");

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
        elog(ERROR, "RASTER_mapAlgebraFct: Could not create a new raster. "
            "Returning NULL");

        rt_raster_destroy(raster);

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

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (rt_raster_has_no_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /*
    * Get NODATA value
    */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        newnodatavalue = rt_band_get_nodata(band);
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
        lwfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }
    
    if (newpixeltype == PT_END) {
        elog(ERROR, "RASTER_mapAlgebraFct: Invalid pixeltype. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }    
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Pixeltype set to %s",
        rt_pixtype_name(newpixeltype));

    /* Get the name of the callback user function for raster values */
    if (PG_ARGISNULL(3)) {
        elog(ERROR, "RASTER_mapAlgebraFct: Required function is missing. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    oid = PG_GETARG_OID(3);
    if (oid == InvalidOid) {
        elog(ERROR, "RASTER_mapAlgebraFct: Got invalid function object id. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    fmgr_info(oid, &cbinfo);

    /* function cannot return set */
    if (cbinfo.fn_retset) {
        elog(ERROR, "RASTER_mapAlgebraFct: Function provided must return double precision not resultset");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }
    /* function should have correct # of args */
    else if (cbinfo.fn_nargs != 2) {
        elog(ERROR, "RASTER_mapAlgebraFct: Function does not have two input parameters");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    if (func_volatile(oid) == 'v') {
        elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");
    }

    /* prep function call data */
#if POSTGIS_PGSQL_VERSION <= 90
    InitFunctionCallInfoData(cbdata, &cbinfo, 2, InvalidOid, NULL);
#else
    InitFunctionCallInfoData(cbdata, &cbinfo, 2, InvalidOid, NULL, NULL);
#endif
    memset(cbdata.argnull, FALSE, 2);
    
    /* check that the function isn't strict if the args are null. */
    if (PG_ARGISNULL(4)) {
        if (cbinfo.fn_strict) {
            elog(ERROR, "RASTER_mapAlgebraFct: Strict callback functions cannot have null parameters");

            rt_raster_destroy(raster);
            rt_raster_destroy(newrast);

            PG_RETURN_NULL();
        }

        cbdata.arg[1] = (Datum)NULL;
        cbdata.argnull[1] = TRUE;
    }
    else {
        cbdata.arg[1] = PG_GETARG_DATUM(4);
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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);
        
        PG_RETURN_POINTER(pgraster);               
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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFct: Could not serialize raster. "
                "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);      
    }
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFct: Main computing loop (%d x %d)",
            width, height);

    for (x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            ret = rt_band_get_pixel(band, x, y, &r);

            /**
             * We compute a value only for the withdata value pixel since the
             * nodata value has already been set by the first optimization
             **/
            if (ret != -1) {
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
                
                rt_band_set_pixel(newband, x, y, newval);
            }

        }
    }
    
    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: raster modified, serializing it.");
    /* Serialize created raster */

    pgraster = rt_raster_serialize(newrast);
    if (NULL == pgraster) {
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    SET_VARSIZE(pgraster, pgraster->size);    

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFct: raster serialized");

    rt_raster_destroy(raster);
    rt_raster_destroy(newrast);

    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraFct: returning raster");
    
    PG_RETURN_POINTER(pgraster);
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
	int ndims = 1;
	int *dims;
	int *lbs;

	uint32_t numBands;
	uint32_t *bandNums;
	uint32 idx = 0;
	int n;
	int i = 0;
	int j = 0;

	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
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
				elog(ERROR, "RASTER_band: Invalid data type for band number(s)");
				rt_raster_destroy(raster);
				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

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
		if (!rast) {
			elog(ERROR, "RASTER_band: Could not create new raster");
			PG_RETURN_NULL();
		}

		pgrast = rt_raster_serialize(rast);
		rt_raster_destroy(rast);

		if (!pgrast) PG_RETURN_NULL();

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
	bool *nulls = NULL;
	int values_length = 6;
	Datum values[values_length];
	HeapTuple tuple;
	Datum result;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
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
		PG_RETURN_NULL();
	}

	/* we don't need the raw values, hence the zero parameter */
	stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, NULL, NULL, NULL);
	rt_band_destroy(band);
	rt_raster_destroy(raster);
	if (NULL == stats) {
		elog(NOTICE, "Unable to compute summary statistics for band at index %d. Returning NULL", bandindex);
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

	nulls = palloc(sizeof(bool) * values_length);
	memset(nulls, FALSE, values_length);

	values[0] = Int64GetDatum(stats->count);
	values[1] = Float8GetDatum(stats->sum);
	values[2] = Float8GetDatum(stats->mean);
	values[3] = Float8GetDatum(stats->stddev);
	values[4] = Float8GetDatum(stats->min);
	values[5] = Float8GetDatum(stats->max);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	pfree(nulls);
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

	bool *nulls = NULL;
	Datum values[6];
	int values_length = 6;
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
	/* create sql */
	len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
	sql = (char *) palloc(len);
	if (NULL == sql) {
		elog(ERROR, "RASTER_summaryStatsCoverage: Unable to allocate memory for sql\n");
		PG_RETURN_NULL();
	}

	/* connect to database */
	spi_result = SPI_connect();
	if (spi_result != SPI_OK_CONNECT) {
		elog(ERROR, "RASTER_summaryStatsCoverage: Could not connect to database using SPI\n");
		pfree(sql);
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
			elog(ERROR, "RASTER_summaryStatsCoverage: Unable to get raster of coverage\n");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);

			PG_RETURN_NULL();
		}
		else if (isNull) {
			SPI_cursor_fetch(portal, TRUE, 1);
			continue;
		}

		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			elog(ERROR, "RASTER_summaryStatsCoverage: Could not deserialize raster");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);

			PG_RETURN_NULL();
		}

		/* inspect number of bands */
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			rt_raster_destroy(raster);
			if (NULL != rtn) pfree(rtn);

			PG_RETURN_NULL();
		}

		/* get band */
		band = rt_raster_get_band(raster, bandindex - 1);
		if (!band) {
			elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			rt_raster_destroy(raster);
			if (NULL != rtn) pfree(rtn);

			PG_RETURN_NULL();
		}

		/* we don't need the raw values, hence the zero parameter */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, &cK, &cM, &cQ);

		rt_band_destroy(band);
		rt_raster_destroy(raster);

		if (NULL == stats) {
			elog(NOTICE, "Unable to compute summary statistics for band at index %d. Returning NULL", bandindex);

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_cursor_close(portal);
			SPI_finish();

			if (NULL != rtn) pfree(rtn);

			PG_RETURN_NULL();
		}

		/* initialize rtn */
		if (stats->count > 0) {
			if (NULL == rtn) {
				rtn = (rt_bandstats) palloc(sizeof(struct rt_bandstats_t));
				if (NULL == rtn) {
					elog(ERROR, "RASTER_summaryStatsCoverage: Unable to allocate memory for summary stats of coverage\n");

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

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
		elog(ERROR, "RASTER_summaryStatsCoverage: Unable to compute coverage summary stats\n");
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

	nulls = palloc(sizeof(bool) * values_length);
	memset(nulls, FALSE, values_length);

	values[0] = Int64GetDatum(rtn->count);
	values[1] = Float8GetDatum(rtn->sum);
	values[2] = Float8GetDatum(rtn->mean);
	values[3] = Float8GetDatum(rtn->stddev);
	values[4] = Float8GetDatum(rtn->min);
	values[5] = Float8GetDatum(rtn->max);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	pfree(nulls);
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
		int ndims = 1;
		int *dims;
		int *lbs;

		POSTGIS_RT_DEBUG(3, "RASTER_histogram: Starting");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) SRF_RETURN_DONE(funcctx);
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
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
					elog(ERROR, "RASTER_histogram: Invalid data type for width");
					rt_raster_destroy(raster);
					PG_RETURN_NULL();
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Unable to compute summary statistics for band at index %d", bandindex);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Unable to compute histogram for band at index %d as the band has no values", bandindex);
			SRF_RETURN_DONE(funcctx);
		}

		/* get histogram */
		hist = rt_band_get_histogram(stats, bin_count, bin_width, bin_width_count, right, min, max, &count);
		if (bin_width_count) pfree(bin_width);
		pfree(stats);
		if (NULL == hist || !count) {
			elog(NOTICE, "Unable to compute histogram for band at index %d", bandindex);
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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(hist2[call_cntr].min);
		values[1] = Float8GetDatum(hist2[call_cntr].max);
		values[2] = Int64GetDatum(hist2[call_cntr].count);
		values[3] = Float8GetDatum(hist2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

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
		int ndims = 1;
		int *dims;
		int *lbs;

		POSTGIS_RT_DEBUG(3, "RASTER_histogramCoverage: first call of function");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* tablename is null, return null */
		if (PG_ARGISNULL(0)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		tablenametext = PG_GETARG_TEXT_P(0);
		tablename = text_to_cstring(tablenametext);
		if (!strlen(tablename)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: tablename = %s", tablename);

		/* column name is null, return null */
		if (PG_ARGISNULL(1)) {
			elog(NOTICE, "Column name must be provided");
			PG_RETURN_NULL();
		}
		colnametext = PG_GETARG_TEXT_P(1);
		colname = text_to_cstring(colnametext);
		if (!strlen(colname)) {
			elog(NOTICE, "Column name must be provided");
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
					elog(ERROR, "RASTER_histogramCoverage: Invalid data type for width");
					SRF_RETURN_DONE(funcctx);
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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

		/* coverage stats */
		len = sizeof(char) * (strlen("SELECT min, max FROM _st_summarystats('','',,::boolean,)") + strlen(tablename) + strlen(colname) + (MAX_INT_CHARLEN * 2) + MAX_DBL_CHARLEN + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {
			elog(ERROR, "RASTER_histogramCoverage: Unable to allocate memory for sql\n");
			if (bin_width_count) pfree(bin_width);
			SRF_RETURN_DONE(funcctx);
		}

		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {
			elog(ERROR, "RASTER_histogramCoverage: Could not connect to database using SPI\n");

			pfree(sql);
			if (bin_width_count) pfree(bin_width);

			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		snprintf(sql, len, "SELECT min, max FROM _st_summarystats('%s','%s',%d,%d::boolean,%f)", tablename, colname, bandindex, (exclude_nodata_value ? 1 : 0), sample);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: %s", sql);
		spi_result = SPI_execute(sql, TRUE, 0);
		pfree(sql);
		if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			SRF_RETURN_DONE(funcctx);
		}

		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		tmp = SPI_getvalue(tuple, tupdesc, 1);
		if (NULL == tmp || !strlen(tmp)) {
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

			SRF_RETURN_DONE(funcctx);
		}
		min = strtod(tmp, NULL);
		POSTGIS_RT_DEBUGF(3, "RASTER_histogramCoverage: min = %f", min);
		pfree(tmp);

		tmp = SPI_getvalue(tuple, tupdesc, 2);
		if (NULL == tmp || !strlen(tmp)) {
			elog(ERROR, "RASTER_histogramCoverage: Could not get summary stats of coverage");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);

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
			elog(ERROR, "RASTER_histogramCoverage: Unable to allocate memory for sql\n");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			if (bin_width_count) pfree(bin_width);
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
				elog(ERROR, "RASTER_histogramCoverage: Unable to get raster of coverage\n");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) free(covhist);
				if (bin_width_count) pfree(bin_width);

				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {
				elog(ERROR, "RASTER_histogramCoverage: Could not deserialize raster");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) free(covhist);
				if (bin_width_count) pfree(bin_width);

				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);
				if (NULL != covhist) free(covhist);
				if (bin_width_count) pfree(bin_width);

				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);
				if (NULL != covhist) free(covhist);
				if (bin_width_count) pfree(bin_width);

				SRF_RETURN_DONE(funcctx);
			}

			/* we need the raw values, hence the non-zero parameter */
			stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);

			rt_band_destroy(band);
			rt_raster_destroy(raster);

			if (NULL == stats) {
				elog(NOTICE, "Unable to compute summary statistics for band at index %d. Returning NULL", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covhist) free(covhist);
				if (bin_width_count) pfree(bin_width);

				SRF_RETURN_DONE(funcctx);
			}

			/* get histogram */
			if (stats->count > 0) {
				hist = rt_band_get_histogram(stats, bin_count, bin_width, bin_width_count, right, min, max, &count);
				pfree(stats);
				if (NULL == hist || !count) {
					elog(NOTICE, "Unable to compute histogram for band at index %d", bandindex);

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					if (NULL != covhist) free(covhist);
					if (bin_width_count) pfree(bin_width);

					SRF_RETURN_DONE(funcctx);
				}

				POSTGIS_RT_DEBUGF(3, "%d bins returned", count);

				/* coverage histogram */
				if (NULL == covhist) {
					/*
						dustymugs 2011-08-25
						covhist is initialized using malloc instead of palloc due to
							strange memory issues where covvcnts is corrupted in
							subsequent calls of SRF
					*/
					covhist = (rt_histogram) malloc(sizeof(struct rt_histogram_t) * count);
					if (NULL == covhist) {
						elog(ERROR, "RASTER_histogramCoverage: Unable to allocate memory for histogram of coverage");

						if (SPI_tuptable) SPI_freetuptable(tuptable);
						SPI_cursor_close(portal);
						SPI_finish();

						if (bin_width_count) pfree(bin_width);
						pfree(hist);

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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(covhist2[call_cntr].min);
		values[1] = Float8GetDatum(covhist2[call_cntr].max);
		values[2] = Int64GetDatum(covhist2[call_cntr].count);
		values[3] = Float8GetDatum(covhist2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		free(covhist2);
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
		int ndims = 1;
		int *dims;
		int *lbs;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) SRF_RETURN_DONE(funcctx);
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			elog(ERROR, "RASTER_quantile: Could not deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* band index is 1-based */
		bandindex = PG_GETARG_INT32(1);
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			SRF_RETURN_DONE(funcctx);
		}
		assert(0 <= (bandindex - 1));

		/* exclude_nodata_value flag */
		if (!PG_ARGISNULL(2))
			exclude_nodata_value = PG_GETARG_BOOL(2);

		/* sample % */
		if (!PG_ARGISNULL(3)) {
			sample = PG_GETARG_FLOAT8(3);
			if (sample < 0 || sample > 1) {
				elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
				rt_raster_destroy(raster);
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
					elog(ERROR, "RASTER_quantile: Invalid data type for quantiles");
					rt_raster_destroy(raster);
					PG_RETURN_NULL();
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Could not retrieve summary statistics for band at index %d", bandindex);
			SRF_RETURN_DONE(funcctx);
		}
		else if (stats->count < 1) {
			elog(NOTICE, "Unable to compute quantiles for band at index %d as the band has no values", bandindex);
			SRF_RETURN_DONE(funcctx);
		}

		/* get quantiles */
		quant = rt_band_get_quantiles(stats, quantiles, quantiles_count, &count);
		if (quantiles_count) pfree(quantiles);
		pfree(stats);
		if (NULL == quant || !count) {
			elog(NOTICE, "Unable to compute quantiles for band at index %d", bandindex);
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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(quant2[call_cntr].quantile);
		values[1] = Float8GetDatum(quant2[call_cntr].value);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

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
		int ndims = 1;
		int *dims;
		int *lbs;

		POSTGIS_RT_DEBUG(3, "RASTER_quantileCoverage: first call of function");

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* tablename is null, return null */
		if (PG_ARGISNULL(0)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		tablenametext = PG_GETARG_TEXT_P(0);
		tablename = text_to_cstring(tablenametext);
		if (!strlen(tablename)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		POSTGIS_RT_DEBUGF(3, "RASTER_quantileCoverage: tablename = %s", tablename);

		/* column name is null, return null */
		if (PG_ARGISNULL(1)) {
			elog(NOTICE, "Column name must be provided");
			PG_RETURN_NULL();
		}
		colnametext = PG_GETARG_TEXT_P(1);
		colname = text_to_cstring(colnametext);
		if (!strlen(colname)) {
			elog(NOTICE, "Column name must be provided");
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
					elog(ERROR, "RASTER_quantileCoverage: Invalid data type for quantiles");
					rt_raster_destroy(raster);
					PG_RETURN_NULL();
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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
		len = sizeof(char) * (strlen("SELECT count FROM _st_summarystats('','',,::boolean,)") + strlen(tablename) + strlen(colname) + (MAX_INT_CHARLEN * 2) + MAX_DBL_CHARLEN + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {
			elog(ERROR, "RASTER_quantileCoverage: Unable to allocate memory for sql\n");
			SRF_RETURN_DONE(funcctx);
		}

		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {
			elog(ERROR, "RASTER_quantileCoverage: Could not connect to database using SPI\n");
			pfree(sql);
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		snprintf(sql, len, "SELECT count FROM _st_summarystats('%s','%s',%d,%d::boolean,%f)", tablename, colname, bandindex, (exclude_nodata_value ? 1 : 0), sample);
		POSTGIS_RT_DEBUGF(3, "stats sql:  %s", sql);
		spi_result = SPI_execute(sql, TRUE, 0);
		pfree(sql);
		if (spi_result != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
			elog(ERROR, "RASTER_quantileCoverage: Could not get summary stats of coverage");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

			SRF_RETURN_DONE(funcctx);
		}

		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];

		tmp = SPI_getvalue(tuple, tupdesc, 1);
		if (NULL == tmp || !strlen(tmp)) {
			elog(ERROR, "RASTER_quantileCoverage: Could not get summary stats of coverage");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

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
			elog(ERROR, "RASTER_quantileCoverage: Unable to allocate memory for sql\n");

			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();

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
				elog(ERROR, "RASTER_quantileCoverage: Unable to get raster of coverage\n");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {
				elog(ERROR, "RASTER_quantileCoverage: Could not deserialize raster");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);

				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);

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
				elog(NOTICE, "Unable to compute quantiles for band at index %d", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				SRF_RETURN_DONE(funcctx);
			}

			/* next record */
			SPI_cursor_fetch(portal, TRUE, 1);
		}

		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_cursor_close(portal);
		SPI_finish();

		quantile_llist_destroy(&qlls, qlls_count);
		if (quantiles_count) pfree(quantiles);

		/*
			dustymugs 2011-08-23
			covquant2 is initialized using malloc instead of palloc due to
				strange memory issues where covvcnts is corrupted in
				subsequent calls of SRF
		*/
		covquant2 = malloc(sizeof(struct rt_quantile_t) * count);
		for (i = 0; i < count; i++) {
			covquant2[i].quantile = covquant[i].quantile;
			covquant2[i].value = covquant[i].value;
		}
		pfree(covquant);

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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(covquant2[call_cntr].quantile);
		values[1] = Float8GetDatum(covquant2[call_cntr].value);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		POSTGIS_RT_DEBUG(3, "done");
		free(covquant2);
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
		int ndims = 1;
		int *dims;
		int *lbs;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* pgraster is null, return nothing */
		if (PG_ARGISNULL(0)) SRF_RETURN_DONE(funcctx);
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			elog(ERROR, "RASTER_valueCount: Could not deserialize raster");
			PG_RETURN_NULL();
		}

		/* band index is 1-based */
		bandindex = PG_GETARG_INT32(1);
		num_bands = rt_raster_get_num_bands(raster);
		if (bandindex < 1 || bandindex > num_bands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			SRF_RETURN_DONE(funcctx);
		}
		assert(0 <= (bandindex - 1));

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
					elog(ERROR, "RASTER_valueCount: Invalid data type for values");
					rt_raster_destroy(raster);
					PG_RETURN_NULL();
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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
			SRF_RETURN_DONE(funcctx);
		}

		/* get counts of values */
		vcnts = rt_band_get_value_count(band, (int) exclude_nodata_value, search_values, search_values_count, roundto, NULL, &count);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == vcnts || !count) {
			elog(NOTICE, "Unable to count the values for band at index %d", bandindex);
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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(vcnts2[call_cntr].value);
		values[1] = UInt32GetDatum(vcnts2[call_cntr].count);
		values[2] = Float8GetDatum(vcnts2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

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
		int ndims = 1;
		int *dims;
		int *lbs;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/* switch to memory context appropriate for multiple function calls */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* tablename is null, return null */
		if (PG_ARGISNULL(0)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		tablenametext = PG_GETARG_TEXT_P(0);
		tablename = text_to_cstring(tablenametext);
		if (!strlen(tablename)) {
			elog(NOTICE, "Table name must be provided");
			SRF_RETURN_DONE(funcctx);
		}
		POSTGIS_RT_DEBUGF(3, "tablename = %s", tablename);

		/* column name is null, return null */
		if (PG_ARGISNULL(1)) {
			elog(NOTICE, "Column name must be provided");
			PG_RETURN_NULL();
		}
		colnametext = PG_GETARG_TEXT_P(1);
		colname = text_to_cstring(colnametext);
		if (!strlen(colname)) {
			elog(NOTICE, "Column name must be provided");
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
					elog(ERROR, "RASTER_valueCountCoverage: Invalid data type for values");
					rt_raster_destroy(raster);
					PG_RETURN_NULL();
					break;
			}

			ndims = ARR_NDIM(array);
			dims = ARR_DIMS(array);
			lbs = ARR_LBOUND(array);

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
		/* create sql */
		len = sizeof(char) * (strlen("SELECT \"\" FROM \"\" WHERE \"\" IS NOT NULL") + (strlen(colname) * 2) + strlen(tablename) + 1);
		sql = (char *) palloc(len);
		if (NULL == sql) {
			elog(ERROR, "RASTER_valueCountCoverage: Unable to allocate memory for sql\n");

			if (search_values_count) pfree(search_values);
			SRF_RETURN_DONE(funcctx);
		}

		/* connect to database */
		spi_result = SPI_connect();
		if (spi_result != SPI_OK_CONNECT) {
			elog(ERROR, "RASTER_valueCountCoverage: Could not connect to database using SPI\n");

			pfree(sql);
			if (search_values_count) pfree(search_values);

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
				elog(ERROR, "RASTER_valueCountCoverage: Unable to get raster of coverage\n");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) free(covvcnts);
				if (search_values_count) pfree(search_values);

				SRF_RETURN_DONE(funcctx);
			}
			else if (isNull) {
				SPI_cursor_fetch(portal, TRUE, 1);
				continue;
			}

			pgraster = (rt_pgraster *) PG_DETOAST_DATUM(datum);

			raster = rt_raster_deserialize(pgraster, FALSE);
			if (!raster) {
				elog(ERROR, "RASTER_valueCountCoverage: Could not deserialize raster");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				if (NULL != covvcnts) free(covvcnts);
				if (search_values_count) pfree(search_values);

				SRF_RETURN_DONE(funcctx);
			}

			/* inspect number of bands*/
			num_bands = rt_raster_get_num_bands(raster);
			if (bandindex < 1 || bandindex > num_bands) {
				elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);
				if (NULL != covvcnts) free(covvcnts);
				if (search_values_count) pfree(search_values);

				SRF_RETURN_DONE(funcctx);
			}

			/* get band */
			band = rt_raster_get_band(raster, bandindex - 1);
			if (!band) {
				elog(NOTICE, "Could not find band at index %d. Returning NULL", bandindex);

				if (SPI_tuptable) SPI_freetuptable(tuptable);
				SPI_cursor_close(portal);
				SPI_finish();

				rt_raster_destroy(raster);
				if (NULL != covvcnts) free(covvcnts);
				if (search_values_count) pfree(search_values);

				SRF_RETURN_DONE(funcctx);
			}

			/* get counts of values */
			vcnts = rt_band_get_value_count(band, (int) exclude_nodata_value, search_values, search_values_count, roundto, &total, &count);
			rt_band_destroy(band);
			rt_raster_destroy(raster);
			if (NULL == vcnts || !count) {
				elog(NOTICE, "Unable to count the values for band at index %d", bandindex);
				SRF_RETURN_DONE(funcctx);
			}

			POSTGIS_RT_DEBUGF(3, "%d value counts returned", count);

			if (NULL == covvcnts) {
				/*
					dustymugs 2011-08-23
					covvcnts is initialized using malloc instead of palloc due to
						strange memory issues where covvcnts is corrupted in
						subsequent calls of SRF
				*/
				covvcnts = (rt_valuecount) malloc(sizeof(struct rt_valuecount_t) * count);
				if (NULL == covvcnts) {
					elog(ERROR, "RASTER_valueCountCoverage: Unable to allocate memory for value counts of coverage");

					if (SPI_tuptable) SPI_freetuptable(tuptable);
					SPI_cursor_close(portal);
					SPI_finish();

					if (search_values_count) pfree(search_values);
					pfree(vcnts);

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
						covvcnts = realloc(covvcnts, sizeof(struct rt_valuecount_t) * covcount);
						if (NULL == covvcnts) {
							elog(ERROR, "RASTER_valueCountCoverage: Unable to change allocated memory for value counts of coverage");

							if (SPI_tuptable) SPI_freetuptable(tuptable);
							SPI_cursor_close(portal);
							SPI_finish();

							if (search_values_count) pfree(search_values);
							if (NULL != covvcnts) free(covvcnts);
							pfree(vcnts);

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
		bool *nulls = NULL;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = Float8GetDatum(covvcnts2[call_cntr].value);
		values[1] = UInt32GetDatum(covvcnts2[call_cntr].count);
		values[2] = Float8GetDatum(covvcnts2[call_cntr].percent);

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		free(covvcnts2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Reclassify the specified bands of the raster
 */
PG_FUNCTION_INFO_V1(RASTER_reclass);
Datum RASTER_reclass(PG_FUNCTION_ARGS) {
	rt_pgraster *pgraster = NULL;
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
	int ndims = 1;
	int *dims;
	int *lbs;
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
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
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

	ndims = ARR_NDIM(array);
	dims = ARR_DIMS(array);
	lbs = ARR_LBOUND(array);

	deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
		&nulls, &n);

	if (!n) {
		elog(NOTICE, "Invalid argument for reclassargset. Returning original raster");
		rt_raster_destroy(raster);

		SET_VARSIZE(pgraster, pgraster->size);
		PG_RETURN_POINTER(pgraster);
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
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}

		/* band index (1-based) */
		tupv = GetAttributeByName(tup, "nband", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of nband for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}
		nband = DatumGetInt32(tupv);
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: expression for band %d", nband);

		/* valid band index? */
		if (nband < 1 || nband > numBands) {
			elog(NOTICE, "Invalid argument for reclassargset. Invalid band index (must use 1-based) for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}

		/* reclass expr */
		tupv = GetAttributeByName(tup, "reclassexpr", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}
		exprtext = (text *) DatumGetPointer(tupv);
		if (NULL == exprtext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
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
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
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
				rt_raster_destroy(raster);

				SET_VARSIZE(pgraster, pgraster->size);
				PG_RETURN_POINTER(pgraster);
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
					rt_raster_destroy(raster);

					SET_VARSIZE(pgraster, pgraster->size);
					PG_RETURN_POINTER(pgraster);
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
							elog(ERROR, "RASTER_reclass: Insufficient memory. Returning NULL");
							for (k = 0; k <= j; k++) pfree(exprset[k]);
							pfree(exprset);
							rt_raster_destroy(raster);

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
						rt_raster_destroy(raster);

						SET_VARSIZE(pgraster, pgraster->size);
						PG_RETURN_POINTER(pgraster);
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
						rt_raster_destroy(raster);

						SET_VARSIZE(pgraster, pgraster->size);
						PG_RETURN_POINTER(pgraster);
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
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}
		pixeltypetext = (text *) DatumGetPointer(tupv);
		if (NULL == pixeltypetext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of pixeltype for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
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
			rt_raster_destroy(raster);

			SET_VARSIZE(pgraster, pgraster->size);
			PG_RETURN_POINTER(pgraster);
		}
		newband = rt_band_reclass(band, pixtype, hasnodata, nodataval, exprset, j);
		if (!newband) {
			elog(ERROR, "RASTER_reclass: Could not reclassify raster band of index %d. Returning NULL", nband);
			rt_raster_destroy(raster);
			for (k = 0; k < j; k++) pfree(exprset[k]);
			pfree(exprset);

			PG_RETURN_NULL();
		}

		/* replace old band with new band */
		if (rt_raster_replace_band(raster, newband, nband - 1) == NULL) {
			elog(ERROR, "RASTER_reclass: Could not replace raster band of index %d with reclassified band. Returning NULL", nband);
			rt_band_destroy(newband);
			rt_raster_destroy(raster);
			for (k = 0; k < j; k++) pfree(exprset[k]);
			pfree(exprset);

			PG_RETURN_NULL();
		}

		/* free exprset */
		for (k = 0; k < j; k++) pfree(exprset[k]);
		pfree(exprset);
	}

	pgraster = rt_raster_serialize(raster);
	rt_raster_destroy(raster);

	if (!pgraster) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_reclass: Finished");
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
	int ndims = 1;
	int *dims;
	int *lbs;
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
		elog(ERROR, "RASTER_asGDALRaster: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* format is required */
	if (PG_ARGISNULL(1)) {
		elog(NOTICE, "Format must be provided");
		rt_raster_destroy(raster);
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
				elog(ERROR, "RASTER_asGDALRaster: Invalid data type for options");
				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

		deconstruct_array(array, etype, typlen, typbyval, typalign, &e,
			&nulls, &n);

		if (n) {
			options = (char **) palloc(sizeof(char *) * (n + 1));
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
				/* add NULL to end */
				options[j] = NULL;

				/* trim allocation */
				options = repalloc(options, j * sizeof(char *));
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
		srs = rtpg_getSRTextSPI(srid);
		if (NULL == srs) {
			elog(ERROR, "RASTER_asGDALRaster: Could not find srtext for SRID (%d)", srid);
			if (NULL != options) {
				for (i = j - 1; i >= 0; i--) pfree(options[i]);
				pfree(options);
			}
			rt_raster_destroy(raster);
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

		drv_set = rt_raster_gdal_drivers(&drv_count);
		if (NULL == drv_set || !drv_count) {
			elog(NOTICE, "No GDAL drivers found");
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
		bool *nulls;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

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
		pfree(nulls);

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
	GSERIALIZED *pggeom = NULL;

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
	int ndims = 1;
	int *dims;
	int *lbs;
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

	pggeom = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom = lwgeom_from_gserialized(pggeom);

	/* Get a 2D version of the geometry if necessary */
	if (lwgeom_ndims(geom) > 2) {
		LWGEOM *geom2d = lwgeom_force_2d(geom);
		lwgeom_free(geom);
		geom = geom2d;
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
				elog(ERROR, "RASTER_asRaster: Invalid data type for pixeltype");

				lwgeom_free(geom);

				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

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
						elog(ERROR, "RASTER_asRaster: Invalid pixel type provided: %s", pixeltype);

						pfree(pixtypes);

						lwgeom_free(geom);

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
				elog(ERROR, "RASTER_asRaster: Invalid data type for value");

				if (pixtypes_len) pfree(pixtypes);

				lwgeom_free(geom);

				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

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
				elog(ERROR, "RASTER_asRaster: Invalid data type for nodataval");

				if (pixtypes_len) pfree(pixtypes);
				if (values_len) pfree(values);

				lwgeom_free(geom);

				PG_RETURN_NULL();
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

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
	srid = gserialized_get_srid(pggeom);

	POSTGIS_RT_DEBUGF(3, "RASTER_asRaster: srid = %d", srid);
	if (clamp_srid(srid) != SRID_UNKNOWN) {
		srs = rtpg_getSRTextSPI(srid);
		if (NULL == srs) {
			elog(ERROR, "RASTER_asRaster: Could not find srtext for SRID (%d)", srid);

			if (pixtypes_len) pfree(pixtypes);
			if (values_len) pfree(values);
			if (nodatavals_len) {
				pfree(hasnodatas);
				pfree(nodatavals);
			}
			if (options_len) pfree(options);

			lwgeom_free(geom);

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
 * Resample a raster
 */
PG_FUNCTION_INFO_V1(RASTER_resample);
Datum RASTER_resample(PG_FUNCTION_ARGS)
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

	POSTGIS_RT_DEBUG(3, "RASTER_resample: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster, FALSE);
	if (!raster) {
		elog(ERROR, "RASTER_resample: Could not deserialize raster");
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

	/* source srid */
	src_srid = rt_raster_get_srid(raster);
	if (clamp_srid(src_srid) == SRID_UNKNOWN) {
		elog(ERROR, "RASTER_resample: Input raster has unknown (%d) SRID", src_srid);
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(4, "source srid: %d", src_srid);

	/* target srid */
	if (!PG_ARGISNULL(3)) {
		dst_srid = PG_GETARG_INT32(3);
		if (clamp_srid(dst_srid) == SRID_UNKNOWN) {
			elog(ERROR, "RASTER_resample: %d is an invalid target SRID", dst_srid);
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
		}
	}
	else
		dst_srid = src_srid;
	POSTGIS_RT_DEBUGF(4, "destination srid: %d", dst_srid);

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
		(clamp_srid(dst_srid) == SRID_UNKNOWN) &&
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
	/* source srs */
	src_srs = rtpg_getSRTextSPI(src_srid);
	if (NULL == src_srs) {
		elog(ERROR, "RASTER_resample: Input raster has unknown SRID (%d)", src_srid);
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(4, "src srs: %s", src_srs);

	/* target srs */
	if (clamp_srid(dst_srid) != SRID_UNKNOWN) {
		dst_srs = rtpg_getSRTextSPI(dst_srid);
		if (NULL == dst_srs) {
			elog(ERROR, "RASTER_resample: Target SRID (%d) is unknown", dst_srid);
			rt_raster_destroy(raster);
			if (NULL != src_srs) pfree(src_srs);
			PG_RETURN_NULL();
		}
		POSTGIS_RT_DEBUGF(4, "dst srs: %s", dst_srs);
	}

	rast = rt_raster_gdal_warp(raster, src_srs,
		dst_srs,
		scale_x, scale_y,
		dim_x, dim_y,
		NULL, NULL,
		grid_xw, grid_yw,
		skew_x, skew_y,
		alg, max_err);
	rt_raster_destroy(raster);
	if (NULL != src_srs) pfree(src_srs);
	if (NULL != dst_srs) pfree(dst_srs);
	if (!rast) {
		elog(ERROR, "RASTER_band: Could not create transformed raster");
		PG_RETURN_NULL();
	}

	/* add target srid */
	rt_raster_set_srid(rast, dst_srid);

	pgrast = rt_raster_serialize(rast);
	rt_raster_destroy(rast);

	if (NULL == pgrast) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_resample: done");

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
	bool *nulls = NULL;
	int values_length = 10;
	Datum values[values_length];
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_metadata: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0), 0, sizeof(struct rt_raster_serialized_t));

	/* raster */
	raster = rt_raster_deserialize(pgraster, TRUE);
	if (!raster) {
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

	nulls = palloc(sizeof(bool) * values_length);
	memset(nulls, FALSE, values_length);

	/* build a tuple */
	tuple = heap_form_tuple(tupdesc, values, nulls);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	pfree(nulls);

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

	bool *nulls = NULL;
	int values_length = 6;
	Datum values[values_length];
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
		int ndims = 1;
		int *dims;
		int *lbs;
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
		if (PG_ARGISNULL(0)) SRF_RETURN_DONE(funcctx);
		pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		/* raster */
		raster = rt_raster_deserialize(pgraster, FALSE);
		if (!raster) {
			elog(ERROR, "RASTER_bandmetadata: Could not deserialize raster");
			SRF_RETURN_DONE(funcctx);
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(raster);
		if (numBands < 1) {
			elog(NOTICE, "Raster provided has no bands");
			rt_raster_destroy(raster);
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
				elog(ERROR, "RASTER_bandmetadata: Invalid data type for band number(s)");
				rt_raster_destroy(raster);
				SRF_RETURN_DONE(funcctx);
				break;
		}

		ndims = ARR_NDIM(array);
		dims = ARR_DIMS(array);
		lbs = ARR_LBOUND(array);

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
				SRF_RETURN_DONE(funcctx);
			}

			bandNums[j] = idx;
			POSTGIS_RT_DEBUGF(3, "bandNums[%d] = %d", j, bandNums[j]);
			j++;
		}

		if (j < n)
			bandNums = repalloc(bandNums, sizeof(uint32_t) * j);
		bmd = (struct bandmetadata *) palloc(sizeof(struct bandmetadata) * j);

		for (i = 0; i < j; i++) {
			band = rt_raster_get_band(raster, bandNums[i] - 1);
			if (NULL == band) {
				elog(NOTICE, "Could not get raster band at index %d", bandNums[i]);
				rt_raster_destroy(raster);
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
				bmd[i].nodataval = rt_band_get_nodata(band);
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

		/* Store needed information */
		funcctx->user_fctx = bmd;

		/* total number of tuples to be returned */
		funcctx->max_calls = j;

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
	bmd2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		nulls = palloc(sizeof(bool) * values_length);
		memset(nulls, FALSE, values_length);

		values[0] = UInt32GetDatum(bmd2[call_cntr].bandnum);
		values[1] = CStringGetTextDatum(bmd2[call_cntr].pixeltype);
		values[2] = BoolGetDatum(bmd2[call_cntr].hasnodata);
		values[3] = Float8GetDatum(bmd2[call_cntr].nodataval);
		values[4] = BoolGetDatum(bmd2[call_cntr].isoutdb);
		if (bmd2[call_cntr].bandpath && strlen(bmd2[call_cntr].bandpath))
			values[5] = CStringGetTextDatum(bmd2[call_cntr].bandpath);
		else
			nulls[5] = TRUE;

		/* build a tuple */
		tuple = heap_form_tuple(tupdesc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up */
		pfree(nulls);
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

/**
 * See if two rasters intersect
 */
PG_FUNCTION_INFO_V1(RASTER_intersects);
Datum RASTER_intersects(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast;
	rt_raster rast[2] = {NULL};
	uint32_t bandindex[2] = {0};
	uint32_t hasbandindex[2] = {0};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t numBands;
	int rtn;
	int intersects;

	LWPOLY *hull[2] = {NULL};
	GEOSGeometry *ghull[2] = {NULL};

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
			PG_RETURN_NULL();
		}
		pgrast = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast, FALSE);
		if (!rast[i]) {
			elog(ERROR, "RASTER_intersects: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
			PG_RETURN_NULL();
		}

		/* numbands */
		numBands = rt_raster_get_num_bands(rast[i]);
		if (numBands < 1) {
			elog(NOTICE, "The %s raster provided has no bands", i < 1 ? "first" : "second");
			for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
			PG_RETURN_NULL();
		}

		/* band index */
		if (!PG_ARGISNULL(j)) {
			bandindex[i] = PG_GETARG_INT32(j);
			if (bandindex[i] < 1 || bandindex[i] > numBands) {
				elog(NOTICE, "Invalid band index (must use 1-based) for the %s raster. Returning NULL", i < 1 ? "first" : "second");
				for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
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
		for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
		PG_RETURN_NULL();
	}

	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		elog(ERROR, "The two rasters provided have different SRIDs");
		for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);
		PG_RETURN_NULL();
	}

	/* raster extents need to intersect */
	do {
		initGEOS(lwnotice, lwgeom_geos_error);

		rtn = 1;
		for (i = 0; i < 2; i++) {
			hull[i] = rt_raster_get_convex_hull(rast[i]);
			if (NULL == hull[i]) {
				for (j = 0; j < i; j++) {
					GEOSGeom_destroy(ghull[j]);
					lwpoly_free(hull[j]);
				}
				rtn = 0;
				break;
			}
			ghull[i] = (GEOSGeometry *) LWGEOM2GEOS((LWGEOM *) hull[i]);
			if (NULL == ghull[i]) {
				for (j = 0; j < i; j++) {
					GEOSGeom_destroy(ghull[j]);
					lwpoly_free(hull[j]);
				}
				lwpoly_free(hull[i]);
				rtn = 0;
				break;
			}
		}
		if (!rtn) break;

		rtn = GEOSIntersects(ghull[0], ghull[1]);

		for (i = 0; i < 2; i++) {
			GEOSGeom_destroy(ghull[i]);
			lwpoly_free(hull[i]);
		}

		if (rtn != 2) {
			if (rtn != 1) {
				for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);
				PG_RETURN_BOOL(0);
			}
			/* band isn't specified */
			else if (!hasbandindex[0]) {
				for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);
				PG_RETURN_BOOL(1);
			}
		}
	}
	while (0);

	rtn = rt_raster_intersects(
		rast[0], (hasbandindex[0] ? bandindex[0] - 1 : -1),
		rast[1], (hasbandindex[1] ? bandindex[1] - 1 : -1),
		&intersects
	);
	for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);

	if (!rtn) {
		elog(ERROR, "RASTER_intersects: Unable to test for intersection on the two rasters");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(intersects);
}

/**
 * See if two rasters are aligned
 */
PG_FUNCTION_INFO_V1(RASTER_sameAlignment);
Datum RASTER_sameAlignment(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast;
	rt_raster rast[2] = {NULL};

	uint32_t i;
	uint32_t j;
	uint32_t k;
	int rtn;
	int aligned = 0;
	int err = 0;

	for (i = 0, j = 0; i < set_count; i++) {
		/* pgrast is null, return null */
		if (PG_ARGISNULL(j)) {
			for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
			PG_RETURN_NULL();
		}
		pgrast = (rt_pgraster *) PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(j), 0, sizeof(struct rt_raster_serialized_t));
		j++;

		/* raster */
		rast[i] = rt_raster_deserialize(pgrast, FALSE);
		if (!rast[i]) {
			elog(ERROR, "RASTER_sameAlignment: Could not deserialize the %s raster", i < 1 ? "first" : "second");
			for (k = 0; k < i; k++) rt_raster_destroy(rast[k]);
			PG_RETURN_NULL();
		}
	}

	err = 0;
	/* SRID must match */
	if (rt_raster_get_srid(rast[0]) != rt_raster_get_srid(rast[1])) {
		elog(NOTICE, "The two rasters provided have different SRIDs");
		err = 1;
	}
	/* scales must match */
	else if (FLT_NEQ(rt_raster_get_x_scale(rast[0]), rt_raster_get_x_scale(rast[1]))) {
		elog(NOTICE, "The two rasters provided have different scales on the X axis");
		err = 1;
	}
	else if (FLT_NEQ(rt_raster_get_y_scale(rast[0]), rt_raster_get_y_scale(rast[1]))) {
		elog(NOTICE, "The two rasters provided have different scales on the Y axis");
		err = 1;
	}
	/* skews must match */
	else if (FLT_NEQ(rt_raster_get_x_skew(rast[0]), rt_raster_get_x_skew(rast[1]))) {
		elog(NOTICE, "The two rasters provided have different skews on the X axis");
		err = 1;
	}
	else if (FLT_NEQ(rt_raster_get_y_skew(rast[0]), rt_raster_get_y_skew(rast[1]))) {
		elog(NOTICE, "The two rasters provided have different skews on the Y axis");
		err = 1;
	}

	if (err) {
		for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);
		PG_RETURN_BOOL(0);
	}

	rtn = rt_raster_same_alignment(
		rast[0],
		rast[1],
		&aligned
	);
	for (k = 0; k < set_count; k++) rt_raster_destroy(rast[k]);

	if (!rtn) {
		elog(ERROR, "RASTER_sameAlignment: Unable to test for alignment on the two rasters");
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(aligned);
}

/**
 * Two raster MapAlgebra
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebra2);
Datum RASTER_mapAlgebra2(PG_FUNCTION_ARGS)
{
	const int set_count = 2;
	rt_pgraster *pgrast;
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

	int spicount = 3;
	uint16_t exprpos[3] = {4, 7, 8};
	char *expr = NULL;
	char *sql = NULL;
	SPIPlanPtr spiplan[3] = {NULL};
	uint16_t spiempty = 0;
	Oid *argtype = NULL;
	uint32_t argcount[3] = {0};
	int argexists[3][2] = {{0}};
	Datum *values = NULL;
	bool *nulls = NULL;
	TupleDesc tupdesc;
	SPITupleTable *tuptable = NULL;
	HeapTuple tuple;
	Datum datum;
	bool isnull = FALSE;
	int hasargval[3] = {0};
	double argval[3] = {0.};
	int hasnodatanodataval = 0;
	double nodatanodataval = 0;

	Oid ufcnoid = InvalidOid;
	FmgrInfo uflinfo;
	FunctionCallInfoData ufcinfo;
	int ufcnullcount = 0;

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
			pgrast = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(j));
			j++;

			/* raster */
			rast[i] = rt_raster_deserialize(pgrast, FALSE);
			if (!rast[i]) {
				elog(ERROR, "RASTER_mapAlgebra2: Could not deserialize the %s raster", i < 1 ? "first" : "second");
				for (k = 0; k < i; k++) {
					if (rast[k] != NULL) rt_raster_destroy(rast[k]);
				}
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
		PG_RETURN_NULL();
	}

	/* both rasters are empty */
	if (_isempty[0] && _isempty[1]) {
		elog(NOTICE, "The two rasters provided are empty.  Returning empty raster");

		raster = rt_raster_new(0, 0);
		if (raster == NULL) {
			elog(ERROR, "RASTER_mapAlgebra2: Unable to create empty raster");
			for (k = 0; k < i; k++) {
				if (rast[k] != NULL) rt_raster_destroy(rast[k]);
			}
			PG_RETURN_NULL();
		}
		rt_raster_set_scale(raster, 0, 0);

		pgrast = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgrast) PG_RETURN_NULL();

		SET_VARSIZE(pgrast, pgrast->size);
		PG_RETURN_POINTER(pgrast);
	}

	/* replace the empty or NULL raster with one matching the other */
	if (
		(rast[0] == NULL || _isempty[0]) ||
		(rast[1] == NULL || _isempty[1])
	) {
		if (rast[0] == NULL || _isempty[0]) {
			i = 0;
			j = 1;
		}
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
			elog(ERROR, "RASTER_mapAlgebra2: Unable to create nodata raster");
			rt_raster_destroy(_rast[j]);
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
	if (rt_raster_get_srid(_rast[0]) != rt_raster_get_srid(_rast[1])) {
		elog(NOTICE, "The two rasters provided have different SRIDs.  Returning NULL");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
		PG_RETURN_NULL();
	}

	/* same alignment */
	if (!rt_raster_same_alignment(_rast[0], _rast[1], &aligned)) {
		elog(ERROR, "RASTER_mapAlgebra2: Unable to test for alignment on the two rasters");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
		PG_RETURN_NULL();
	}
	if (!aligned) {
		elog(NOTICE, "The two rasters provided do not have the same alignment.  Returning NULL");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
		PG_RETURN_NULL();
	}

	/* pixel type */
	if (!PG_ARGISNULL(5)) {
		pixtypename = text_to_cstring(PG_GETARG_TEXT_P(5));
		/* Get the pixel type index */
		pixtype = rt_pixtype_index_from_name(pixtypename);
		if (pixtype == PT_END ) {
			elog(ERROR, "RASTER_mapAlgebra2: Invalid pixel type: %s", pixtypename);
			for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
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
	raster = rt_raster_from_two_rasters(
		_rast[0], _rast[1],
		extenttype,
		&err, _offset
	);
	if (!err) {
		elog(ERROR, "RASTER_mapAlgebra2: Unable to get output raster of correct extent");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
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
				for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
				rt_raster_destroy(raster);
				PG_RETURN_NULL();
			}

			/* specified band not found */
			if (rt_raster_has_no_band(_rast[i], bandindex[i] - 1)) {
				elog(NOTICE, "The %s raster does not have the band at index %d.  Returning no band raster of correct extent",
					(i != 1 ? "FIRST" : "SECOND"), bandindex[i]
				);

				for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);

				pgrast = rt_raster_serialize(raster);
				rt_raster_destroy(raster);
				if (!pgrast) PG_RETURN_NULL();

				SET_VARSIZE(pgrast, pgrast->size);
				PG_RETURN_POINTER(pgrast);
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
						elog(ERROR, "RASTER_mapAlgebra2: Unable to create no band raster");
						for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
						PG_RETURN_NULL();
					}
					rt_raster_set_scale(raster, 0, 0);
					rt_raster_set_srid(raster, rt_raster_get_srid(_rast[0]));
				}

				for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);

				pgrast = rt_raster_serialize(raster);
				rt_raster_destroy(raster);
				if (!pgrast) PG_RETURN_NULL();

				SET_VARSIZE(pgrast, pgrast->size);
				PG_RETURN_POINTER(pgrast);
			}
			break;
	}

	/* both rasters do not have specified bands */
	if (
		(!_isempty[0] && rt_raster_has_no_band(_rast[0], bandindex[0] - 1)) &&
		(!_isempty[1] && rt_raster_has_no_band(_rast[1], bandindex[1] - 1))
	) {
		elog(NOTICE, "The two rasters provided do not have the respectively specified band indices.  Returning no band raster of correct extent");

		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);

		pgrast = rt_raster_serialize(raster);
		rt_raster_destroy(raster);
		if (!pgrast) PG_RETURN_NULL();

		SET_VARSIZE(pgrast, pgrast->size);
		PG_RETURN_POINTER(pgrast);
	}

	/* get bands */
	for (i = 0; i < set_count; i++) {
		if (_isempty[i] || rt_raster_has_no_band(_rast[i], bandindex[i] - 1)) {
			_hasnodata[i] = 1;
			_nodataval[i] = 0;

			continue;
		}

		_band[i] = rt_raster_get_band(_rast[i], bandindex[i] - 1);
		if (_band[i] == NULL) {
			elog(ERROR, "RASTER_mapAlgebra2: Unable to get band %d of the %s raster",
				bandindex[i],
				(i < 1 ? "FIRST" : "SECOND")
			);
			for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
		}

		_hasnodata[i] = rt_band_get_hasnodata_flag(_band[i]);
		if (_hasnodata[i])
			_nodataval[i] = rt_band_get_nodata(_band[i]);
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
		pixtype, nodataval,
		1, nodataval,
		0
	) < 0) {
		elog(ERROR, "RASTER_mapAlgebra2: Unable to add new band to output raster");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}

	/* get output band */
	band = rt_raster_get_band(raster, 0);
	if (band == NULL) {
		elog(ERROR, "RASTER_mapAlgebra2: Unable to get newly added band of output raster");
		for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
		rt_raster_destroy(raster);
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

	POSTGIS_RT_DEBUGF(4, "bandmetadata = (%s, %d, %f)",
		rt_pixtype_name(rt_band_get_pixtype(band)),
		rt_band_get_hasnodata_flag(band),
		rt_band_get_nodata(band)
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
				elog(ERROR, "RASTER_mapAlgebra2: Unable to connect to the SPI manager");
				for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
				rt_raster_destroy(raster);
				PG_RETURN_NULL();
			}

			/* reset hasargval */
			memset(hasargval, 0, spicount);

			/*
				process expressions

				exprpos elements are:
					4 - expression => spiplan[0]
					7 - nodata1expr => spiplan[1]
					8 - nodata2expr => spiplan[2]
			*/
			for (i = 0; i < spicount; i++) {
				if (!PG_ARGISNULL(exprpos[i])) {
					expr = rtpg_strtoupper(text_to_cstring(PG_GETARG_TEXT_P(exprpos[i])));
					POSTGIS_RT_DEBUGF(3, "raw expr #%d: %s", i, expr);
					len = 0;
					expr = rtpg_replace(expr, "RAST1", "$1", &len);
					if (len) {
						argcount[i]++;
						argexists[i][0] = 1;
					}
					len = 0;
					expr = rtpg_replace(expr, "RAST2", (argexists[i][0] ? "$2" : "$1"), &len);
					if (len) {
						argcount[i]++;
						argexists[i][1] = 1;
					}

					len = strlen("SELECT (") + strlen(expr) + strlen(")::double precision");
					sql = (char *) palloc(len + 1);
					if (sql == NULL) {
						elog(ERROR, "RASTER_mapAlgebra2: Unable to allocate memory for expression parameter %d", exprpos[i]);
						for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
						rt_raster_destroy(raster);
						for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
						SPI_finish();
						PG_RETURN_NULL();
					}

					strncpy(sql, "SELECT (", strlen("SELECT ("));
					strncpy(sql + strlen("SELECT ("), expr, strlen(expr));
					strncpy(sql + strlen("SELECT (") + strlen(expr), ")::double precision", strlen(")::double precision"));
					sql[len] = '\0';

					POSTGIS_RT_DEBUGF(3, "sql #%d: %s", i, sql);

					/* create prepared plan */
					if (argcount[i]) {
						argtype = (Oid *) palloc(argcount[i] * sizeof(Oid));
						if (argtype == NULL) {
							elog(ERROR, "RASTER_mapAlgebra2: Unable to allocate memory for prepared plan argtypes of expression parameter %d", exprpos[i]);

							for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
							rt_raster_destroy(raster);

							for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
							SPI_finish();

							pfree(sql);

							PG_RETURN_NULL();
						}
						for (j = 0; j < argcount[i]; j++) argtype[j] = FLOAT8OID;

						spiplan[i] = SPI_prepare(sql, argcount[i], argtype);
						if (spiplan[i] == NULL) {
							elog(ERROR, "RASTER_mapAlgebra2: Unable to create prepared plan of expression parameter %d", exprpos[i]);

							for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
							rt_raster_destroy(raster);

							for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
							SPI_finish();

							pfree(sql);
							pfree(argtype);

							PG_RETURN_NULL();
						}

						pfree(argtype);
					}
					/* no args, just execute query */
					else {
						err = SPI_execute(sql, TRUE, 0);
						if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
							elog(ERROR, "RASTER_mapAlgebra2: Unable to evaluate expression parameter %d", exprpos[i]);

							for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
							rt_raster_destroy(raster);

							for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
							SPI_finish();

							pfree(sql);

							PG_RETURN_NULL();
						}

						/* get output of prepared plan */
						tupdesc = SPI_tuptable->tupdesc;
						tuptable = SPI_tuptable;
						tuple = tuptable->vals[0];

						datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
						if (SPI_result == SPI_ERROR_NOATTRIBUTE) {
							elog(ERROR, "RASTER_mapAlgebra2: Unable to get result of expression parameter %d", exprpos[i]);

							for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
							rt_raster_destroy(raster);

							if (SPI_tuptable) SPI_freetuptable(tuptable);
							for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
							SPI_finish();

							pfree(sql);

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
					spiempty++;
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
				ufcnullcount = 0;
				ufcnoid = PG_GETARG_OID(4);

				/* get function info */
				fmgr_info(ufcnoid, &uflinfo);

				/* function cannot return set */
				err = 0;
				if (uflinfo.fn_retset) {
					elog(ERROR, "RASTER_mapAlgebra2: Function provided must return double precision not resultset");
					err = 1;
				}
				/* function should have correct # of args */
				else if (uflinfo.fn_nargs != 3) {
					elog(ERROR, "RASTER_mapAlgebra2: Function does not have three input parameters");
					err = 1;
				}

				if (err) {
					for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
					rt_raster_destroy(raster);

					PG_RETURN_NULL();
				}

				if (func_volatile(ufcnoid) == 'v') {
					elog(NOTICE, "Function provided is VOLATILE. Unless required and for best performance, function should be IMMUTABLE or STABLE");
				}

				/* prep function call data */
#if POSTGIS_PGSQL_VERSION <= 90
				InitFunctionCallInfoData(ufcinfo, &uflinfo, 3, InvalidOid, NULL);
#else
				InitFunctionCallInfoData(ufcinfo, &uflinfo, 3, InvalidOid, NULL, NULL);
#endif
				memset(ufcinfo.argnull, FALSE, 3);

				if (!PG_ARGISNULL(7)) {
				 ufcinfo.arg[2] = PG_GETARG_DATUM(7);
				}
				else {
				 ufcinfo.arg[2] = (Datum) NULL;
				 ufcinfo.argnull[2] = TRUE;
				 ufcnullcount++;
				}
			}
			break;
		}
		default:
			elog(ERROR, "RASTER_mapAlgebra2: Invalid data type for expression or userfunction");
			for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
			break;
	}

	/* loop over pixels */
	/* if any expression present, run */
	if ((
		(calltype == TEXTOID) && (
			(spiempty != spicount) || hasnodatanodataval
		)
	) || (
		(calltype == REGPROCEDUREOID) && (ufcnoid != InvalidOid)
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
						err = rt_band_get_pixel(_band[i], _x, _y, &(_pixel[i]));
						if (err < 0) {
							elog(ERROR, "RASTER_mapAlgebra2: Unable to get pixel of %s raster", (i < 1 ? "FIRST" : "SECOND"));

							for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
							rt_raster_destroy(raster);

							if (calltype == TEXTOID) {
								for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
								SPI_finish();
							}

							PG_RETURN_NULL();
						}

						if (!_hasnodata[i] || FLT_NEQ(_nodataval[i], _pixel[i]))
							_haspixel[i] = 1;
					}

					POSTGIS_RT_DEBUGF(5, "r%d(%d, %d) = %d, %f",
						i,
						_x,
						_y,
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
						/* run spiplan[2] (nodata2expr) */
						else if (_haspixel[0] && !_haspixel[1])
							i = 2;
						/* !pixel0 && pixel1 */
						/* run spiplan[1] (nodata1expr) */
						else if (!_haspixel[0] && _haspixel[1])
							i = 1;
						/* pixel0 && pixel1 */
						/* run spiplan[0] (expression) */
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
						else if (spiplan[i] != NULL) {
							POSTGIS_RT_DEBUGF(5, "Using prepared plan: %d", i);

							/* expression has argument(s) */
							if (argcount[i]) {
								values = (Datum *) palloc(sizeof(Datum) * argcount[i]);
								if (values == NULL) {
									elog(ERROR, "RASTER_mapAlgebra2: Unable to allocate memory for value parameters of prepared statement %d", i);

									for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
									rt_raster_destroy(raster);

									for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
									SPI_finish();

									PG_RETURN_NULL();
								}

								nulls = (bool *) palloc(sizeof(bool) * argcount[i]);
								if (nulls == NULL) {
									elog(ERROR, "RASTER_mapAlgebra2: Unable to allocate memory for NULL parameters of prepared statement %d", i);

									pfree(values);

									for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
									rt_raster_destroy(raster);

									for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
									SPI_finish();

									PG_RETURN_NULL();
								}
								memset(nulls, FALSE, argcount[i]);

								/* two arguments */
								if (argcount[i] > 1) {
									for (j = 0; j < argcount[i]; j++) {
										if (_isempty[j] || !_haspixel[j])
											nulls[j] = TRUE;
										else
											values[j] = Float8GetDatum(_pixel[j]);
									}
								}
								/* only one argument */
								else {
									if (argexists[i][0])
										j = 0;
									else
										j = 1;

									if (_isempty[j] || !_haspixel[j]) {
										POSTGIS_RT_DEBUG(5, "using null");
										nulls[0] = TRUE;
									}
									else {
										POSTGIS_RT_DEBUGF(5, "using value %f", _pixel[j]);
										values[0] = Float8GetDatum(_pixel[j]);
									}
								}
							}

							/* run prepared plan */
							err = SPI_execute_plan(spiplan[i], values, nulls, TRUE, 1);
							if (values != NULL) pfree(values);
							if (nulls != NULL) pfree(nulls);
							if (err != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
								elog(ERROR, "RASTER_mapAlgebra2: Unexpected error when running prepared statement %d", i);

								for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
								rt_raster_destroy(raster);

								for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
								SPI_finish();

								PG_RETURN_NULL();
							}

							/* get output of prepared plan */
							tupdesc = SPI_tuptable->tupdesc;
							tuptable = SPI_tuptable;
							tuple = tuptable->vals[0];

							datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
							if (SPI_result == SPI_ERROR_NOATTRIBUTE) {
								elog(ERROR, "RASTER_mapAlgebra2: Unable to get result of prepared statement %d", i);

								for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
								rt_raster_destroy(raster);

								if (SPI_tuptable) SPI_freetuptable(tuptable);
								for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
								SPI_finish();

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
						/* build fcnarg */
						for (i = 0; i < set_count; i++) {
							ufcinfo.arg[i] = Float8GetDatum(_pixel[i]);

							if (_haspixel[i]) {
								ufcinfo.argnull[i] = FALSE;
								ufcnullcount--;
							}
							else {
								ufcinfo.argnull[i] = TRUE;
				 				ufcnullcount++;
							}
						}

						/* function is strict and null parameter is passed */
						/* http://archives.postgresql.org/pgsql-general/2011-11/msg00424.php */
						if (uflinfo.fn_strict && ufcnullcount)
							break;

						datum = FunctionCallInvoke(&ufcinfo);

						/* result is not null*/
						if (!ufcinfo.isnull) {
							haspixel = 1;
							pixel = DatumGetFloat8(datum);
						}
					}	break;
				}

				/* burn pixel if haspixel != 0 */
				if (haspixel) {
					if (rt_band_set_pixel(band, x, y, pixel) < 0) {
						elog(ERROR, "RASTER_mapAlgebra2: Unable to set pixel value of output raster");

						for (k = 0; k < set_count; k++) rt_raster_destroy(_rast[k]);
						rt_raster_destroy(raster);

						if (calltype == TEXTOID) {
							for (k = 0; k < spicount; k++) SPI_freeplan(spiplan[k]);
							SPI_finish();
						}

						PG_RETURN_NULL();
					}
				}

				POSTGIS_RT_DEBUGF(5, "(x, y, val) = (%d, %d, %f)", x, y, haspixel ? pixel : nodataval);

			} /* y: height */
		} /* x: width */
	}

	/* CLEANUP */
	if (calltype == TEXTOID) {
		for (i = 0; i < spicount; i++) {
			if (spiplan[i] != NULL) SPI_freeplan(spiplan[i]);
		}
		SPI_finish();
	}

	for (k = 0; k < set_count; k++) {
		if (_rast[k] != NULL) rt_raster_destroy(_rast[k]);
	}

	pgrast = rt_raster_serialize(raster);
	rt_raster_destroy(raster);
	if (!pgrast) PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "Finished RASTER_mapAlgebra2");

	SET_VARSIZE(pgrast, pgrast->size);
	PG_RETURN_POINTER(pgrast);
}

/**
 * One raster neighborhood MapAlgebra
 */
PG_FUNCTION_INFO_V1(RASTER_mapAlgebraFctNgb);
Datum RASTER_mapAlgebraFctNgb(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
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
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not create a new raster. "
            "Returning NULL");
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

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (rt_raster_has_no_band(raster, nband - 1)) {
        elog(NOTICE, "Raster does not have the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /* Get the raster band */
    band = rt_raster_get_band(raster, nband - 1);
    if ( NULL == band ) {
        elog(NOTICE, "Could not get the required band. Returning a raster "
            "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    /*
    * Get NODATA value
    */
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        newnodatavalue = rt_band_get_nodata(band);
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
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Invalid pixeltype. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }    
    
    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebraFctNgb: Pixeltype set to %s (%d)",
        rt_pixtype_name(newpixeltype), newpixeltype);

    /* Get the name of the callback userfunction */
    if (PG_ARGISNULL(5)) {
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Required function is missing. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    oid = PG_GETARG_OID(5);
    if (oid == InvalidOid) {
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Got invalid function object id. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    fmgr_info(oid, &cbinfo);

    /* function cannot return set */
    if (cbinfo.fn_retset) {
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Function provided must return double precision not resultset. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }
    /* function should have correct # of args */
    else if (cbinfo.fn_nargs != 3) {
        elog(ERROR, "RASTER_mapAlgebraFctNgb: Function does not have three input parameters. Returning NULL");

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

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
    memset(cbdata.argnull, FALSE, 3);

    /* check that the function isn't strict if the args are null. */
    if (PG_ARGISNULL(7)) {
        if (cbinfo.fn_strict) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Strict callback functions cannot have null parameters. Returning NULL");

            rt_raster_destroy(raster);
            rt_raster_destroy(newrast);

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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);
        
        PG_RETURN_POINTER(pgraster);               
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

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);      
    }

    /* Get the width of the neighborhood */
    if (PG_ARGISNULL(3) || PG_GETARG_INT32(3) <= 0) {
        elog(NOTICE, "Neighborhood width is NULL or <= 0. Returning new "
            "raster with the original band");

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);      
    }

    ngbwidth = PG_GETARG_INT32(3);
    winwidth = ngbwidth * 2 + 1;

    /* Get the height of the neighborhood */
    if (PG_ARGISNULL(4) || PG_GETARG_INT32(4) <= 0) {
        elog(NOTICE, "Neighborhood height is NULL or <= 0. Returning new "
            "raster with the original band");

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);      
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
        // if the text is not "IGNORE" or "NULL", it may be a numerical value
        if (sscanf(strFromText, "%d", &intReplace) <= 0 && sscanf(strFromText, "%f", &fltReplace) <= 0) {
            // the value is NOT an integer NOR a floating point
            elog(NOTICE, "Neighborhood NODATA mode is not recognized. Must be one of 'value', 'ignore', "
                "'NULL', or a numeric value. Returning new raster with the original band");

            /* clean up the nodatamode string */
            pfree(txtCallbackParam);
            pfree(strFromText);

            /* Serialize created raster */
            pgraster = rt_raster_serialize(newrast);
            if (NULL == pgraster) {
                elog(ERROR, "RASTER_mapAlgebraFctNgb: Could not serialize raster. "
                    "Returning NULL");

                PG_RETURN_NULL();
            }

            SET_VARSIZE(pgraster, pgraster->size);

            rt_raster_destroy(raster);
            rt_raster_destroy(newrast);

            PG_RETURN_POINTER(pgraster);      
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
                ret = rt_band_get_pixel(band, x, y, &rpix);
                if (ret != -1 && FLT_NEQ(rpix, newnodatavalue)) {
                    pixelreplace = true;
                }
            }
            for (u = x - ngbwidth; u <= x + ngbwidth; u++) {
                for (v = y - ngbheight; v <= y + ngbheight; v++) {
                    ret = rt_band_get_pixel(band, u, v, &r);
                    if (ret != -1) {
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
                                /* do not increment nNullItems */
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
            if (!nNodataOnly && !(nNullSkip && nNullItems > 0) && !(valuereplace && nNullItems > 0)) {
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
                
                rt_band_set_pixel(newband, x, y, newval);
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
    
    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: raster modified, serializing it.");
    /* Serialize created raster */

    pgraster = rt_raster_serialize(newrast);
    if (NULL == pgraster) {
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    SET_VARSIZE(pgraster, pgraster->size);    

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebraFctNgb: raster serialized");

    rt_raster_destroy(raster);
    rt_raster_destroy(newrast);

    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebraFctNgb: returning raster");
    
    PG_RETURN_POINTER(pgraster);
}

/* ---------------------------------------------------------------- */
/*  Memory allocation / error reporting hooks                       */
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


#if 0 /* defined by libpgcommon { */
/* This is needed by liblwgeom */
void
lwgeom_init_allocators(void)
{
    /* liblwgeom callback - install PostgreSQL handlers */
    lwalloc_var = rt_pg_alloc;
    lwrealloc_var = rt_pg_realloc;
    lwfree_var = rt_pg_free;
    lwerror_var = rt_pg_error;
    lwnotice_var = rt_pg_notice;
}
#endif /* defined by libpgcommon } */


void
rt_init_allocators(void)
{
    /* raster callback - install raster handlers */
    rt_set_handlers(rt_pg_alloc, rt_pg_realloc, rt_pg_free, rt_pg_error,
            rt_pg_notice, rt_pg_notice);
}

