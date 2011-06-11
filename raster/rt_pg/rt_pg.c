/*
 * $Id$
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
 * Copyright (C) 2011 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@avencia.com>
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

/*#include "lwgeom_pg.h"*/
#include "liblwgeom.h"
#include "rt_pg.h"
#include "pgsql_compat.h"
#include "rt_api.h"
#include "../raster_config.h"

#include <utils/lsyscache.h> /* for get_typlenbyvalalign */
#include <utils/array.h> /* for ArrayType */
#include <catalog/pg_type.h> /* for INT2OID, INT4OID, FLOAT4OID, FLOAT8OID and TEXTOID */

#define POSTGIS_RASTER_WARN_ON_TRUNCATION

/* maximum char length required to hold any double or long long value */
#define MAX_DBL_CHARLEN (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#define MAX_INT_CHARLEN 32

/*
 * This is required for builds against pgsql 8.2
 */
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Internal funcs */
static char * replace(const char *str, const char *oldstr, const char *newstr,
        int *count);
static char *strtoupper(char *str);
static char	*chartrim(char* input, char *remove); /* for RASTER_reclass */
static char **strsplit(const char *str, const char *delimiter, int *n); /* for RASTER_reclass */
static char *removespaces(char *str); /* for RASTER_reclass */
static char *trim(char* input); /* for RASTER_asGDALRaster */

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

/* Set all the properties of a raster */
Datum RASTER_setSRID(PG_FUNCTION_ARGS);
Datum RASTER_setScale(PG_FUNCTION_ARGS);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS);
Datum RASTER_setSkew(PG_FUNCTION_ARGS);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS);

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
Datum RASTER_mapAlgebra(PG_FUNCTION_ARGS);

/* create new raster from existing raster's bands */
Datum RASTER_band(PG_FUNCTION_ARGS);

/* Get summary stats */
Datum RASTER_summaryStats(PG_FUNCTION_ARGS);

/* get histogram */
Datum RASTER_histogram(PG_FUNCTION_ARGS);

/* get quantiles */
Datum RASTER_quantile(PG_FUNCTION_ARGS);

/* get counts of values */
Datum RASTER_valueCount(PG_FUNCTION_ARGS);

/* reclassify specified bands of a raster */
Datum RASTER_reclass(PG_FUNCTION_ARGS);

/* convert raster to GDAL raster */
Datum RASTER_asGDALRaster(PG_FUNCTION_ARGS);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS);

/* transform a raster to a new projection */
Datum RASTER_transform(PG_FUNCTION_ARGS);

/* get raster's meta data */
Datum RASTER_metadata(PG_FUNCTION_ARGS);

/* get raster band's meta data */
Datum RASTER_bandmetadata(PG_FUNCTION_ARGS);

/* Replace function taken from
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
replace(const char *str, const char *oldstr, const char *newstr, int *count)
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
strtoupper(char * str)
{
    int j;

    for(j = 0; j < strlen(str); j++)
        str[j] = toupper(str[j]);

    return str;

}

static char*
chartrim(char *input, char *remove) {
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
strsplit(const char *str, const char *delimiter, int *n) {
	char *tmp = NULL;
	char **rtn = NULL;
	char *token = NULL;

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

	*n = 0;
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
removespaces(char *str) {
	char *rtn;

	rtn = replace(str, " ", "", NULL);
	rtn = replace(rtn, "\n", "", NULL);
	rtn = replace(rtn, "\t", "", NULL);
	rtn = replace(rtn, "\f", "", NULL);
	rtn = replace(rtn, "\r", "", NULL);

	return rtn;
}

static char*
trim(char *input) {
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

    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
        elog(ERROR, "RASTER_out: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    hexwkb = rt_raster_to_hexwkb(raster, &hexwkbsize);
    if ( ! hexwkb )
    {
        elog(ERROR, "RASTER_out: Could not HEX-WKBize raster");
        PG_RETURN_NULL();
    }

    /* Free the raster objects used */
    rt_raster_destroy(raster);

    /* Call free_if_copy on the "varlena" structures originally get as args */
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_CSTRING(hexwkb);
}

/**
 * Return bytea object with raster in Well-Known-Binary form.
 */
PG_FUNCTION_INFO_V1(RASTER_to_bytea);
Datum RASTER_to_bytea(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster = NULL;
    uint8_t *wkb = NULL;
    uint32_t wkb_size = 0;
    bytea *result = NULL;
    int result_size = 0;

    /* Get raster object */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
        elog(ERROR, "RASTER_to_bytea: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Parse raster to wkb object */
    wkb = rt_raster_to_wkb(raster, &wkb_size);
    if ( ! wkb ) {
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

     /* Call free_if_copy on the "varlena" structures originally get as args */
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_POINTER(result);
}

/**
 * Return bytea object with raster requested using ST_AsBinary function.
 */
PG_FUNCTION_INFO_V1(RASTER_to_binary);
Datum RASTER_to_binary(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster = NULL;
    uint8_t *wkb = NULL;
    uint32_t wkb_size = 0;
    char *result = NULL;
    int result_size = 0;

    /* Get raster object */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
        elog(ERROR, "RASTER_to_binary: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Parse raster to wkb object */
    wkb = rt_raster_to_wkb(raster, &wkb_size);
    if ( ! wkb ) {
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

     /* Call free_if_copy on the "varlena" structures originally get as args */
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_POINTER(result);
}

/**
 * Return the convex hull of this raster as a 4-vertices POLYGON.
 */
PG_FUNCTION_INFO_V1(RASTER_convex_hull);
Datum RASTER_convex_hull(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    LWPOLY* convexhull;
    uchar* pglwgeom;

    { /* TODO: can be optimized to only detoast the header! */
        raster = rt_raster_deserialize(pgraster);
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
#ifdef GSERIALIZED_ON
        size_t gser_size;
        GSERIALIZED *gser;
        gser = gserialized_from_lwgeom(lwpoly_as_lwgeom(convexhull), 0, &gser_size);
        SET_VARSIZE(gser, gser_size);
        pglwgeom = (uchar*)gser;
#else
        size_t sz = lwpoly_serialize_size(convexhull);
        pglwgeom = palloc(VARHDRSZ+sz);
        lwpoly_serialize_buf(convexhull, (uchar*)VARDATA(pglwgeom), &sz);
        SET_VARSIZE(pglwgeom, VARHDRSZ+sz);
#endif
    }

    /* Free raster and lwgeom memory */
    rt_raster_destroy(raster);
    lwfree(convexhull);

    /* Free input varlena object */
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_POINTER(pglwgeom);
}


/**
 * Needed for sizeof
 */
struct rt_geomval_t {
    int srid;
    double val;
    char * geom;
};


PG_FUNCTION_INFO_V1(RASTER_dumpAsWKTPolygons);
Datum RASTER_dumpAsWKTPolygons(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    FuncCallContext *funcctx;
    TupleDesc tupdesc;
    AttInMetadata *attinmeta;
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
        pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        raster = rt_raster_deserialize(pgraster);
        if ( ! raster )
        {
            ereport(ERROR,
                    (errcode(ERRCODE_OUT_OF_MEMORY),
                    errmsg("Could not deserialize raster")));
            PG_RETURN_NULL();
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
        geomval = rt_raster_dump_as_wktpolygons(raster, nband, &nElements);
        if (NULL == geomval)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_NO_DATA_FOUND),
                    errmsg("Could not polygonize raster")));
            PG_RETURN_NULL();
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

        /*
         * generate attribute metadata needed later to produce tuples from raw
         * C strings
         */
        attinmeta = TupleDescGetAttInMetadata(tupdesc);
        funcctx->attinmeta = attinmeta;

        MemoryContextSwitchTo(oldcontext);
    }

    /* stuff done on every call of the function */
    funcctx = SRF_PERCALL_SETUP();

    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;
    attinmeta = funcctx->attinmeta;
    geomval2 = funcctx->user_fctx;

    if (call_cntr < max_calls)    /* do when there is more left to send */
    {
        char       **values;
        HeapTuple    tuple;
        Datum        result;

        POSTGIS_RT_DEBUGF(3, "call number %d", call_cntr);

        /*
         * Prepare a values array for building the returned tuple.
         * This should be an array of C strings which will
         * be processed later by the type input functions.
         */
        values = (char **) palloc(3 * sizeof(char *));

        values[0] = (char *) palloc(
                (strlen(geomval2[call_cntr].geom) + 1) * sizeof(char));
        values[1] = (char *) palloc(18 * sizeof(char));
        values[2] = (char *) palloc(16 * sizeof(char));

        snprintf(values[0],
            (strlen(geomval2[call_cntr].geom) + 1) * sizeof(char), "%s",
            geomval2[call_cntr].geom);
        snprintf(values[1], 18 * sizeof(char), "%f", geomval2[call_cntr].val);
        snprintf(values[2], 16 * sizeof(char), "%d", geomval2[call_cntr].srid);

        POSTGIS_RT_DEBUGF(4, "Result %d, Polygon %s", call_cntr, values[0]);
        POSTGIS_RT_DEBUGF(4, "Result %d, val %s", call_cntr, values[1]);
        POSTGIS_RT_DEBUGF(4, "Result %d, val %s", call_cntr, values[2]);

        /**
         * Free resources.
         */
        pfree(geomval2[call_cntr].geom);

        /* build a tuple */
        tuple = BuildTupleFromCStrings(attinmeta, values);

        /* make the tuple into a datum */
        result = HeapTupleGetDatum(tuple);

        /* clean up (this is not really necessary) */
        pfree(values[0]);
        pfree(values[1]);
        pfree(values[2]);
        pfree(values);


        SRF_RETURN_NEXT(funcctx, result);
    }
    else    /* do when there is no more left */
    {
        pfree(geomval2);
        //rt_raster_destroy(raster);
        //PG_FREE_IF_COPY(pgraster, 0);
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    int32_t srid;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    int32_t newSRID = PG_GETARG_INT32(1);

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
        elog(ERROR, "RASTER_setSRID: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_srid(raster, newSRID);

    //PG_FREE_IF_COPY(pgraster, 0);

    pgraster = rt_raster_serialize(raster);
    if ( ! pgraster ) PG_RETURN_NULL();

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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    uint16_t width;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    uint16_t height;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    int32_t num_bands;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xsize;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double ysize;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double size = PG_GETARG_FLOAT8(1);

    raster = rt_raster_deserialize(pgraster);
    if (! raster ) {
        elog(ERROR, "RASTER_setScale: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(raster, size, size);

    //PG_FREE_IF_COPY(pgraster, 0);

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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xscale = PG_GETARG_FLOAT8(1);
    double yscale = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(pgraster);
    if (! raster ) {
        elog(ERROR, "RASTER_setScaleXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(raster, xscale, yscale);

    //PG_FREE_IF_COPY(pgraster, 0);

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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xskew;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double yskew;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double skew = PG_GETARG_FLOAT8(1);

    raster = rt_raster_deserialize(pgraster);
    if (! raster ) {
        elog(ERROR, "RASTER_setSkew: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(raster, skew, skew);

    //PG_FREE_IF_COPY(raster, 0);

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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xskew = PG_GETARG_FLOAT8(1);
    double yskew = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(pgraster);
    if (! raster ) {
        elog(ERROR, "RASTER_setSkewXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(raster, xskew, yskew);

    //PG_FREE_IF_COPY(pgraster, 0);

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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xul;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double yul;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster ) {
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
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster;
    double xoffset = PG_GETARG_FLOAT8(1);
    double yoffset = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(pgraster);
    if (! raster ) {
        elog(ERROR, "RASTER_setUpperLeftXY: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_offsets(raster, xoffset, yoffset);

    //PG_FREE_IF_COPY(pgraster, 0);

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

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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

    /* Index is 1-based */
    bandindex = PG_GETARG_INT32(1);
    if ( bandindex < 1 ) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
        PG_RETURN_NULL();
    }

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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
        // Raster does not have a nodata value set so we return NULL
        PG_RETURN_NULL();
    }

    nodata = rt_band_get_nodata(band);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_FLOAT4(nodata);
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

    /* Check index is not NULL or smaller than 1 */
    if (PG_ARGISNULL(1))
        bandindex = -1;
    else
        bandindex = PG_GETARG_INT32(1);
    if (bandindex < 1) {
        elog(NOTICE, "Invalid band index (must use 1-based). Nodata value not set. Returning original raster");
        skipset = TRUE;
    }

    /* Deserialize raster */
    if (PG_ARGISNULL(0)) {
        /* Simply return NULL */
        PG_RETURN_NULL();
    }
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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

    //PG_FREE_IF_COPY(pgraster, 0);

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

    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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

    //PG_FREE_IF_COPY(pgraster, 0);
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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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
    PG_FREE_IF_COPY(pgraster, 0);

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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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
    PG_FREE_IF_COPY(pgraster, 0);

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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

    raster = rt_raster_deserialize(pgraster);
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

    //PG_FREE_IF_COPY(pgraster, 0);

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

    int pixtype = PT_END;
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
    if (PG_ARGISNULL(0)) {
        /* Simply return NULL */
        PG_RETURN_NULL();
    }
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

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

    raster = rt_raster_deserialize(pgraster);
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

    //PG_FREE_IF_COPY(pgraster, 0);

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
    if (PG_ARGISNULL(0)) {
        /* Simply return NULL */
        PG_RETURN_NULL();
    }
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

    torast = rt_raster_deserialize(pgraster);
    if ( ! torast ) {
        elog(ERROR, "RASTER_copyband: Could not deserialize first raster");
        PG_RETURN_NULL();
    }

    /* Deserialize fromrast */
    if (!PG_ARGISNULL(1)) {
        pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1));

        fromrast = rt_raster_deserialize(pgraster);
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

    //PG_FREE_IF_COPY(pgraster, 0);

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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster )
    {
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("RASTER_isEmpty: Could not deserialize raster")));
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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster);
    if ( ! raster )
    {
        ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("RASTER_hasNoBand: Could not deserialize raster")));
        PG_RETURN_NULL();
    }

    /* Get band number */
    bandindex = PG_GETARG_INT32(1);
    hasnoband = rt_raster_has_no_band(raster, bandindex);

    rt_raster_destroy(raster);
    PG_FREE_IF_COPY(pgraster, 0);

    PG_RETURN_BOOL(hasnoband);
}

PG_FUNCTION_INFO_V1(RASTER_mapAlgebra);
Datum RASTER_mapAlgebra(PG_FUNCTION_ARGS)
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
    char *initndvexpr = NULL;
    char *expression = NULL;
    char *nodatavalueexpr = NULL;
    rt_pixtype newpixeltype;
    int skipcomputation = 0;
    char strnewnodatavalue[50];
    char strnewval[50];
    int count = 0;
    int len = 0;
    int ret = -1;
    TupleDesc tupdesc;
    SPITupleTable * tuptable = NULL;
    HeapTuple tuple;
    char * strFromText = NULL;
    bool freemem = FALSE;

    POSTGIS_RT_DEBUG(2, "RASTER_mapAlgebra: Starting...");

    /* Check raster */
    if (PG_ARGISNULL(0)) {
        elog(NOTICE, "Raster is NULL. Returning NULL");
        PG_RETURN_NULL();
    }


    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    raster = rt_raster_deserialize(pgraster);
    if (NULL == raster) {
        elog(ERROR, "RASTER_mapAlgebra: Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* We don't need this */
    //PG_FREE_IF_COPY(pgraster, 0);

	POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: Getting arguments...");

	if (PG_ARGISNULL(1))
		nband = 1;
	else
		nband = PG_GETARG_INT32(1);

	if (nband < 1)
		nband = 1;


    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: Creating new empty raster...");

    /**
     * Create a new empty raster with having the same georeference as the
     * provided raster
     **/
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);

    newrast = rt_raster_new(width, height);

    if ( NULL == newrast ) {
        elog(ERROR, "RASTER_mapAlgebra: Could not create a new raster. "
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

            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }


    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: Getting raster band %d...", nband);

    /**
     * Check if the raster has the required band. Otherwise, return a raster
     * without band
     **/
    if (rt_raster_has_no_band(raster, nband)) {
        elog(NOTICE, "Raster do not have the required band. Returning a raster "
                "without a band");
        rt_raster_destroy(raster);

        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
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
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
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
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: Getting NODATA value for band...");

    if (rt_band_get_hasnodata_flag(band)) {
        newnodatavalue = rt_band_get_nodata(band);
    }

    else {
        newnodatavalue = rt_band_get_min_value(band);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: NODATA value for band: = %f",
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
    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: Setting pixeltype...");

    if (PG_ARGISNULL(4)) {
        newpixeltype = rt_band_get_pixtype(band);
    }

    else {
        strFromText = text_to_cstring(PG_GETARG_TEXT_P(4));
        newpixeltype = rt_pixtype_index_from_name(strFromText);
        lwfree(strFromText);
        if (newpixeltype == PT_END)
            newpixeltype = rt_band_get_pixtype(band);
    }

    if (newpixeltype == PT_END) {
        elog(ERROR, "RASTER_mapAlgebra: Invalid pixeltype. Returning NULL");
        PG_RETURN_NULL();
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: Pixeltype set to %s",
        rt_pixtype_name(newpixeltype));


	/* Construct expression for raster values */
    if (!PG_ARGISNULL(2)) {
        expression = text_to_cstring(PG_GETARG_TEXT_P(2));
        len = strlen("SELECT ") + strlen(expression);
        initexpr = (char *)palloc(len + 1);

        strncpy(initexpr, "SELECT ", strlen("SELECT "));
        strncpy(initexpr + strlen("SELECT "), strtoupper(expression),
            strlen(expression));
        initexpr[len] = '\0';

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: Expression is %s", initexpr);

        /* We don't need this memory */
        //lwfree(expression);
        //expression = NULL;
    }



    /**
     * Optimization: If a nodatavalueexpr is provided, recompute the initial
     * value. Then, we can initialize the raster with this value and skip the
     * computation of nodata values one by one in the main computing loop
     **/
    if (!PG_ARGISNULL(3)) {
        nodatavalueexpr = text_to_cstring(PG_GETARG_TEXT_P(3));
        len = strlen("SELECT ") + strlen(nodatavalueexpr);
        initndvexpr = (char *)palloc(len + 1);
        strncpy(initndvexpr, "SELECT ", strlen("SELECT "));
        strncpy(initndvexpr + strlen("SELECT "), strtoupper(nodatavalueexpr),
                strlen(nodatavalueexpr));
        initndvexpr[len] = '\0';

        //lwfree(nodatavalueexpr);
        //nodatavalueexpr = NULL;

        /* Replace RAST, if present, for NODATA value, to eval the expression */
        if (strstr(initndvexpr, "RAST")) {
            sprintf(strnewnodatavalue, "%f", newnodatavalue);

            newexpr = replace(initndvexpr, "RAST", strnewnodatavalue, &count);
            freemem = TRUE;
        }

        /* If newexpr reduces to a constant, simply eval it */
        else {
            newexpr = initndvexpr;
        }

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: initndvexpr = %s", newexpr);

        /* Eval the NODATA expression to get new NODATA. Connect with SPI manager
        * NOTE: This creates a NEW memory context and makes it current.
        */
        SPI_connect();

        /**
        * Execute the expression for nodata value and store the result as new
        * initial value
        **/
        ret = SPI_execute(newexpr, FALSE, 0);

        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL ||
                SPI_processed != 1) {
            elog(ERROR, "RASTER_mapAlgebra: Invalid construction for nodata "
                "expression. Aborting");

            if (SPI_tuptable)
                SPI_freetuptable(tuptable);

            /* Disconnect from SPI manager */
            SPI_finish();

            PG_RETURN_NULL();
        }

        tupdesc = SPI_tuptable->tupdesc;
        tuptable = SPI_tuptable;

        tuple = tuptable->vals[0];
        newinitialvalue = atof(SPI_getvalue(tuple, tupdesc, 1));

        SPI_freetuptable(tuptable);

        /* Close the connection to SPI manager.
         * NOTE: This restores the previous memory context
         */
        SPI_finish();

        if (freemem)
            pfree(newexpr);

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: new initial value = %f",
            newinitialvalue);

    }



    /**
     * Optimization: If the raster is only filled with nodata values return
     * right now a raster filled with the nodatavalueexpr
     * TODO: Call rt_band_check_isnodata instead?
     **/
    if (rt_band_get_isnodata_flag(band)) {

        POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: Band is a nodata band, returning "
                "a raster filled with nodata");

        ret = rt_raster_generate_new_band(newrast, newpixeltype,
                newinitialvalue, TRUE, newnodatavalue, 0);

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        if (initndvexpr)
            pfree(initndvexpr);
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }


    /**
     * Optimization: If expression resume to 'RAST' and nodatavalueexpr is NULL
     * or also equal to 'RAST', we can just return the band from the original
     * raster
     **/
    if (initexpr != NULL && !strcmp(initexpr, "SELECT RAST") &&
		    (nodatavalueexpr  == NULL || !strcmp(initndvexpr, "SELECT RAST"))) {
            //(initndvexpr == NULL || !strcmp(initndvexpr, "SELECT RAST"))) {

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: Expression resumes to RAST. "
                "Returning raster with band %d from original raster", nband);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebra: New raster has %d bands",
                rt_raster_get_num_bands(newrast));

        rt_raster_copy_band(raster, newrast, nband - 1, 0);

        POSTGIS_RT_DEBUGF(4, "RASTER_mapAlgebra: New raster now has %d bands",
                rt_raster_get_num_bands(newrast));

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
                    "Returning NULL");
            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        if (initndvexpr)
            pfree(initndvexpr);
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
            elog(ERROR, "RASTER_mapAlgebra: Invalid construction for expression."
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

        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: New raster value = %f",
                newval);

        SPI_finish();

        skipcomputation = 1;

        /**
         * Compute the new value, set it and we will return after creating the
         * new raster
         **/
        //if (initndvexpr == NULL) {
        if (nodatavalueexpr == NULL) {
            newinitialvalue = newval;
            skipcomputation = 2;
        }

        /* Return the new raster as it will be before computing pixel by pixel */
        else if (fabs(newval - newinitialvalue) > FLT_EPSILON) {
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
    //if (initexpr == NULL || skipcomputation == 2) {
    if (expression == NULL || skipcomputation == 2) {

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
                    "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        /* Free memory */
        if (initndvexpr)
            pfree(initndvexpr);
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_POINTER(pgraster);
    }

    RASTER_DEBUG(3, "RASTER_mapAlgebra: Creating new raster band...");

    /* Get the new raster band */
    newband = rt_raster_get_band(newrast, 0);
    if ( NULL == newband ) {
        elog(NOTICE, "Could not modify band for new raster. Returning new "
                "raster with the original band");

        /* Serialize created raster */
        pgraster = rt_raster_serialize(newrast);
        if (NULL == pgraster) {
            elog(ERROR, "RASTER_mapAlgebra: Could not serialize raster. "
                    "Returning NULL");

            PG_RETURN_NULL();
        }

        SET_VARSIZE(pgraster, pgraster->size);

        if (initndvexpr)
            pfree(initndvexpr);
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);


        PG_RETURN_POINTER(pgraster);
    }

    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: Main computing loop (%d x %d)",
            width, height);

    for (x = 0; x < width; x++) {
        for(y = 0; y < height; y++) {
            ret = rt_band_get_pixel(band, x, y, &r);

            /**
             * We compute a value only for the withdata value pixel since the
             * nodata value has already been set by the first optimization
             **/
            if (ret != -1 && fabs(r - newnodatavalue) > FLT_EPSILON) {
                if (skipcomputation == 0) {
                    sprintf(strnewval, "%f", r);

                    if (initexpr != NULL) {
                        newexpr = replace(initexpr, "RAST", strnewval, &count);

                        POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: (%dx%d), "
                                "r = %s, newexpr = %s",
                                x, y, strnewval, newexpr);

                        SPI_connect();

                        ret = SPI_execute(newexpr, FALSE, 0);

                        if (ret != SPI_OK_SELECT || SPI_tuptable == NULL ||
                                SPI_processed != 1) {
                            elog(ERROR, "RASTER_mapAlgebra: Invalid construction"
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

                    POSTGIS_RT_DEBUGF(3, "RASTER_mapAlgebra: new value = %f",
                        newval);
                }


                rt_band_set_pixel(newband, x, y, newval);
            }

        }
    }

    /* The newrast band has been modified */

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: raster modified, serializing it.");
    /* Serialize created raster */

    pgraster = rt_raster_serialize(newrast);
    if (NULL == pgraster) {
        /* Free memory allocated out of the current context */
        if (initndvexpr)
            pfree(initndvexpr);
        if (initexpr)
            pfree(initexpr);
        rt_raster_destroy(raster);
        rt_raster_destroy(newrast);

        PG_RETURN_NULL();
    }

    SET_VARSIZE(pgraster, pgraster->size);

    POSTGIS_RT_DEBUG(3, "RASTER_mapAlgebra: raster serialized");

    /* Free memory */
    if (initndvexpr)
        pfree(initndvexpr);
    if (initexpr)
        pfree(initexpr);

    rt_raster_destroy(raster);
    rt_raster_destroy(newrast);

    POSTGIS_RT_DEBUG(4, "RASTER_mapAlgebra: returning raster");


    PG_RETURN_POINTER(pgraster);
}

/**
 * Return new raster from selected bands of existing raster through ST_Band.
 * second argument is an array of band numbers (1 based)
 */
PG_FUNCTION_INFO_V1(RASTER_band);
Datum RASTER_band(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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

	uint32_t *bandNums;
	uint32 idx = 0;
	int n;
	int i = 0;
	int j = 0;

	raster = rt_raster_deserialize(pgraster);
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

		bandNums = (uint32_t *) palloc(sizeof(uint32_t) * n);
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
			if (idx > pgraster->numBands || idx < 1) {
        elog(NOTICE, "Invalid band index (must use 1-based). Returning original raster");
				pfree(bandNums);
				skip = TRUE;
			}
			if (skip) break;

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
	}
	else {
		pgrast = pgraster;
	}

	if (!pgrast)
		PG_RETURN_NULL();

	SET_VARSIZE(pgrast, pgrast->size);
	PG_RETURN_POINTER(pgrast);
}

struct rt_bandstats_t {
	double sample;
	uint32_t count;

	double min;
	double max;
	double sum;
	double mean;
	double stddev;

	double *values;
	int sorted;
};

/**
 * Get summary stats of a band
 */
PG_FUNCTION_INFO_V1(RASTER_summaryStats);
Datum RASTER_summaryStats(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;
	int32_t bandindex = 0;
	bool exclude_nodata_value = TRUE;
	int num_bands = 0;
	double sample = 0;
	int cstddev = 0;
	uint64_t cK = 0;
	double cM = 0;
	double cQ = 0;
	rt_bandstats stats = NULL;

	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	int i = 0;
	char **values = NULL;
	int values_length = 0;
	HeapTuple tuple;
	Datum result;

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	raster = rt_raster_deserialize(pgraster);
	if (!raster) {
		elog(ERROR, "RASTER_summaryStats: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* band index is 1-based */
	bandindex = PG_GETARG_INT32(1);
	num_bands = rt_raster_get_num_bands(raster);
	if (bandindex < 1 || bandindex > num_bands) {
		elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
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
			PG_RETURN_NULL();
		}
		else if (fabs(sample - 0.0) < FLT_EPSILON)
			sample = 1;
	}
	else
		sample = 1;

	/* one-pass standard deviation variables */
	if (PG_NARGS()> 4) {
		cstddev = 1;
		if (!PG_ARGISNULL(4)) cK = PG_GETARG_INT64(4);
		if (!PG_ARGISNULL(5)) cM = PG_GETARG_FLOAT8(5);
		if (!PG_ARGISNULL(6)) cQ = PG_GETARG_FLOAT8(6);
	}

	/* get band */
	band = rt_raster_get_band(raster, bandindex - 1);
	if (!band) {
		elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}

	/* we don't need the raw values, hence the zero parameter */
	if (!cstddev)
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, NULL, NULL, NULL);
	else
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 0, &cK, &cM, &cQ);
	rt_band_destroy(band);
	rt_raster_destroy(raster);
	if (NULL == stats) {
		elog(NOTICE, "Could not retrieve summary statistics of band of index %d. Returning NULL", bandindex);
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

	/*
	 * generate attribute metadata needed later to produce tuples from raw
	 * C strings
	 */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	/*
	 * Prepare a values array for building the returned tuple.
	 * This should be an array of C strings which will
	 * be processed later by the type input functions.
	 */
	if (!cstddev)
		values_length = 6;
	else
		values_length = 8;
	values = (char **) palloc(values_length * sizeof(char *));

	values[0] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	values[1] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[2] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[3] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[4] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[5] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	if (cstddev) {
		values[6] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
		values[7] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	}

	snprintf(
		values[0],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		stats->count
	);
	snprintf(
		values[1],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		stats->sum
	);
	snprintf(
		values[2],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		stats->mean
	);
	snprintf(
		values[3],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		stats->stddev
	);
	snprintf(
		values[4],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		stats->min
	);
	snprintf(
		values[5],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		stats->max
	);
	if (cstddev) {
		snprintf(
			values[6],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			cM
		);
		snprintf(
			values[7],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			cQ
		);
	}

	/* build a tuple */
	tuple = BuildTupleFromCStrings(attinmeta, values);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	for (i = 0; i < values_length; i++) pfree(values[i]);
	pfree(values);
	pfree(stats);

	PG_RETURN_DATUM(result);
}

/* get histogram */
struct rt_histogram_t {
	uint32_t count;
	double percent;

	double min;
	double max;

	int inc_min;
	int inc_max;
};

/**
 * Returns histogram for a band
 */
PG_FUNCTION_INFO_V1(RASTER_histogram);
Datum RASTER_histogram(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	int count;
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
		int32_t bandindex = 0;
		int num_bands = 0;
		bool exclude_nodata_value = TRUE;
		double sample = 0;
		int bin_count = 0;
		double *bin_width = NULL;
		int bin_width_count = 0;
		double width = 0;
		bool right = FALSE;
		double min = 0;
		double max = 0;
		rt_bandstats stats = NULL;

		int i;
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

		raster = rt_raster_deserialize(pgraster);
		if (!raster) {
			elog(ERROR, "RASTER_histogram: Could not deserialize raster");
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

		/* sample % */
		if (!PG_ARGISNULL(3)) {
			sample = PG_GETARG_FLOAT8(3);
			if (sample < 0 || sample > 1) {
				elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
				rt_raster_destroy(raster);
				SRF_RETURN_DONE(funcctx);
			}
			else if (fabs(sample - 0.0) < FLT_EPSILON)
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

				if (width < 0 || fabs(width - 0.0) < FLT_EPSILON) {
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
			elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Could not retrieve summary statistics of raster band of index %d", bandindex);
			SRF_RETURN_DONE(funcctx);
		}

		/* get histogram */
		hist = rt_band_get_histogram(stats, bin_count, bin_width, bin_width_count, right, min, max, &count);
		if (bin_width_count) pfree(bin_width);
		pfree(stats);
		if (NULL == hist || !count) {
			elog(NOTICE, "Could not retrieve histogram of raster band of index %d", bandindex);
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

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;
		MemoryContextSwitchTo(oldcontext);
	}

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;
	hist2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		char **values;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		/*
		 * Prepare a values array for building the returned tuple.
		 * This should be an array of C strings which will
		 * be processed later by the type input functions.
		 */
		values = (char **) palloc(4 * sizeof(char *));

		values[0] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
		values[1] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
		values[2] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
		values[3] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));

		snprintf(
			values[0],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			hist2[call_cntr].min
		);
		snprintf(
			values[1],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			hist2[call_cntr].max
		);
		snprintf(
			values[2],
			sizeof(char) * (MAX_INT_CHARLEN + 1),
			"%d",
			hist2[call_cntr].count
		);
		snprintf(
			values[3],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			hist2[call_cntr].percent
		);

		/* build a tuple */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up (this is not really necessary) */
		pfree(values[3]);
		pfree(values[2]);
		pfree(values[1]);
		pfree(values[0]);
		pfree(values);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(hist2);
		SRF_RETURN_DONE(funcctx);
	}
}

/* get quantiles */
struct rt_quantile_t {
	double quantile;
	double value;
};

/**
 * Returns quantiles for a band
 */
PG_FUNCTION_INFO_V1(RASTER_quantile);
Datum RASTER_quantile(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	int count;
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
		int quantiles_count = 0;
		double quantile = 0;
		rt_bandstats stats = NULL;

		int i;
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

		raster = rt_raster_deserialize(pgraster);
		if (!raster) {
			elog(ERROR, "RASTER_quantile: Could not deserialize raster");
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

		/* sample % */
		if (!PG_ARGISNULL(3)) {
			sample = PG_GETARG_FLOAT8(3);
			if (sample < 0 || sample > 1) {
				elog(NOTICE, "Invalid sample percentage (must be between 0 and 1). Returning NULL");
				rt_raster_destroy(raster);
				SRF_RETURN_DONE(funcctx);
			}
			else if (fabs(sample - 0.0) < FLT_EPSILON)
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
					elog(ERROR, "RASTER_quantile: Invalid data type for quanitiles");
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
			elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			SRF_RETURN_DONE(funcctx);
		}

		/* get stats */
		stats = rt_band_get_summary_stats(band, (int) exclude_nodata_value, sample, 1, NULL, NULL, NULL);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == stats || NULL == stats->values) {
			elog(NOTICE, "Could not retrieve summary statistics of raster band of index %d", bandindex);
			SRF_RETURN_DONE(funcctx);
		}

		/* get quantiles */
		quant = rt_band_get_quantiles(stats, quantiles, quantiles_count, &count);
		if (quantiles_count) pfree(quantiles);
		pfree(stats);
		if (NULL == quant || !count) {
			elog(NOTICE, "Could not retrieve quantiles of raster band of index %d", bandindex);
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

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;
		MemoryContextSwitchTo(oldcontext);
	}

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;
	quant2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		char **values;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		/*
		 * Prepare a values array for building the returned tuple.
		 * This should be an array of C strings which will
		 * be processed later by the type input functions.
		 */
		values = (char **) palloc(2 * sizeof(char *));

		values[0] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
		values[1] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));

		snprintf(
			values[0],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			quant2[call_cntr].quantile
		);
		snprintf(
			values[1],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			quant2[call_cntr].value
		);

		/* build a tuple */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up (this is not really necessary) */
		pfree(values[1]);
		pfree(values[0]);
		pfree(values);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(quant2);
		SRF_RETURN_DONE(funcctx);
	}
}

struct rt_valuecount_t {
	double value;
	uint32_t count;
	double percent;
};

/* get counts of values */
PG_FUNCTION_INFO_V1(RASTER_valueCount);
Datum RASTER_valueCount(PG_FUNCTION_ARGS) {
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	int count;
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
		int search_values_count = 0;
		double roundto = 0;

		int i;
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

		raster = rt_raster_deserialize(pgraster);
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
			elog(NOTICE, "Could not find raster band of index %d. Returning NULL", bandindex);
			rt_raster_destroy(raster);
			SRF_RETURN_DONE(funcctx);
		}

		/* get counts of values */
		vcnts = rt_band_get_value_count(band, (int) exclude_nodata_value, search_values, search_values_count, roundto, &count);
		rt_band_destroy(band);
		rt_raster_destroy(raster);
		if (NULL == vcnts || !count) {
			elog(NOTICE, "Could not count the values of raster band of index %d", bandindex);
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

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;
		MemoryContextSwitchTo(oldcontext);
	}

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;
	vcnts2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		char **values;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		/*
		 * Prepare a values array for building the returned tuple.
		 * This should be an array of C strings which will
		 * be processed later by the type input functions.
		 */
		values = (char **) palloc(3 * sizeof(char *));

		values[0] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
		values[1] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
		values[2] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));

		snprintf(
			values[0],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			vcnts2[call_cntr].value
		);
		snprintf(
			values[1],
			sizeof(char) * (MAX_INT_CHARLEN + 1),
			"%d",
			vcnts2[call_cntr].count
		);
		snprintf(
			values[2],
			sizeof(char) * (MAX_DBL_CHARLEN + 1),
			"%f",
			vcnts2[call_cntr].percent
		);

		/* build a tuple */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up (this is not really necessary) */
		pfree(values[2]);
		pfree(values[1]);
		pfree(values[0]);
		pfree(values);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(vcnts2);
		SRF_RETURN_DONE(funcctx);
	}
}

struct rt_reclassexpr_t {
	struct rt_reclassrange {
		double min;
		double max;
		int inc_min;
		int inc_max;
		int exc_min;
		int exc_max;
	} src, dst;
};

/**
 * Reclassify the specified bands of the raster
 */
PG_FUNCTION_INFO_V1(RASTER_reclass);
Datum RASTER_reclass(PG_FUNCTION_ARGS) {
	rt_pgraster *pgrast = NULL;
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
	pgrast = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgrast);
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

		SET_VARSIZE(pgrast, pgrast->size);
		PG_RETURN_POINTER(pgrast);
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

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}

		/* band index (1-based) */
		tupv = GetAttributeByName(tup, "nband", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of nband for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}
		nband = DatumGetInt32(tupv);
		POSTGIS_RT_DEBUGF(3, "RASTER_reclass: expression for band %d", nband);

		/* valid band index? */
		if (nband < 1 || nband > numBands) {
			elog(NOTICE, "Invalid argument for reclassargset. Invalid band index (must use 1-based) for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}

		/* reclass expr */
		tupv = GetAttributeByName(tup, "reclassexpr", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}
		exprtext = (text *) DatumGetPointer(tupv);
		if (NULL == exprtext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of reclassexpr for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}
		expr = text_to_cstring(exprtext);
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: expr (raw) %s", expr);
		expr = removespaces(expr);
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: expr (clean) %s", expr);

		/* split string to its components */
		/* comma (,) separating rangesets */
		comma_set = strsplit(expr, ",", &comma_n);
		if (comma_n < 1) {
			elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}

		/* set of reclass expressions */
		POSTGIS_RT_DEBUGF(5, "RASTER_reclass: %d possible expressions", comma_n);
		exprset = palloc(comma_n * sizeof(rt_reclassexpr));

		for (a = 0, j = 0; a < comma_n; a++) {
			POSTGIS_RT_DEBUGF(5, "RASTER_reclass: map %s", comma_set[a]);

			/* colon (:) separating range "src" and "dst" */
			colon_set = strsplit(comma_set[a], ":", &colon_n);
			if (colon_n != 2) {
				elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
				for (k = 0; k < j; k++) pfree(exprset[k]);
				pfree(exprset);
				rt_raster_destroy(raster);

				SET_VARSIZE(pgrast, pgrast->size);
				PG_RETURN_POINTER(pgrast);
			}

			/* allocate mem for reclass expression */
			exprset[j] = palloc(sizeof(struct rt_reclassexpr_t));

			for (b = 0; b < colon_n; b++) {
				POSTGIS_RT_DEBUGF(5, "RASTER_reclass: range %s", colon_set[b]);

				/* dash (-) separating "min" and "max" */
				dash_set = strsplit(colon_set[b], "-", &dash_n);
				if (dash_n < 1 || dash_n > 3) {
					elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
					for (k = 0; k < j; k++) pfree(exprset[k]);
					pfree(exprset);
					rt_raster_destroy(raster);

					SET_VARSIZE(pgrast, pgrast->size);
					PG_RETURN_POINTER(pgrast);
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

						SET_VARSIZE(pgrast, pgrast->size);
						PG_RETURN_POINTER(pgrast);
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
					dash_set[c] = chartrim(dash_set[c], "()[]");
					POSTGIS_RT_DEBUGF(5, "RASTER_reclass: min/max (char) %s", dash_set[c]);

					/* value from string to double */
					errno = 0;
					val = strtod(dash_set[c], &junk);
					if (errno != 0 || dash_set[c] == junk) {
						elog(NOTICE, "Invalid argument for reclassargset. Invalid expression of reclassexpr for reclassarg of index %d . Returning original raster", i);
						for (k = 0; k < j; k++) pfree(exprset[k]);
						pfree(exprset);
						rt_raster_destroy(raster);

						SET_VARSIZE(pgrast, pgrast->size);
						PG_RETURN_POINTER(pgrast);
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
			}

			POSTGIS_RT_DEBUGF(3, "RASTER_reclass: or: %f - %f nr: %f - %f"
				, exprset[j]->src.min
				, exprset[j]->src.max
				, exprset[j]->dst.min
				, exprset[j]->dst.max
			);
			j++;
		}

		/* pixel type */
		tupv = GetAttributeByName(tup, "pixeltype", &isnull);
		if (isnull) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of pixeltype for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
		}
		pixeltypetext = (text *) DatumGetPointer(tupv);
		if (NULL == pixeltypetext) {
			elog(NOTICE, "Invalid argument for reclassargset. Missing value of pixeltype for reclassarg of index %d . Returning original raster", i);
			rt_raster_destroy(raster);

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
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

			SET_VARSIZE(pgrast, pgrast->size);
			PG_RETURN_POINTER(pgrast);
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

	pgrast = rt_raster_serialize(raster);
	rt_raster_destroy(raster);

	if (!pgrast)
		PG_RETURN_NULL();

	POSTGIS_RT_DEBUG(3, "RASTER_reclass: Finished");
	SET_VARSIZE(pgrast, pgrast->size);
	PG_RETURN_POINTER(pgrast);
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
	int srid = -1;
	char *srs = NULL;

	int sqllen = 0;
	char *sql = NULL;
	int ret;
	TupleDesc tupdesc;
	SPITupleTable * tuptable = NULL;
	HeapTuple tuple;

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

	raster = rt_raster_deserialize(pgraster);
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
						option = trim(option);
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
				options[j] = (char *) palloc(sizeof(char));
				options[j] = '\0';

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
	if (srid != SRID_UNKNOWN) {
		sqllen = sizeof(char) * (strlen("SELECT _ST_srtext()") + MAX_INT_CHARLEN);
		sql = (char *) palloc(sqllen);
		if (NULL == sql) {
			elog(ERROR, "RASTER_asGDALRaster: Unable to allocate memory for SRID query");
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
		}

		SPI_connect();

		/* srs */
		snprintf(sql, sqllen, "SELECT _ST_srtext(%d)", srid);
		POSTGIS_RT_DEBUGF(4, "srs sql: %s", sql);
		ret = SPI_execute(sql, TRUE, 0);
		if (ret != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
			elog(ERROR, "RASTER_asGDALRaster: SRID provided is unknown");
			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();
			rt_raster_destroy(raster);
			pfree(sql);
			PG_RETURN_NULL();
		}
		tupdesc = SPI_tuptable->tupdesc;
		tuptable = SPI_tuptable;
		tuple = tuptable->vals[0];
		srs = SPI_getvalue(tuple, tupdesc, 1);
		if (NULL == srs || !strlen(srs)) {
			elog(ERROR, "RASTER_asGDALRaster: SRID provided (%d) is invalid", srid);
			if (SPI_tuptable) SPI_freetuptable(tuptable);
			SPI_finish();
			rt_raster_destroy(raster);
			pfree(sql);
			PG_RETURN_NULL();
		}
		SPI_freetuptable(tuptable);
		SPI_finish();
		pfree(sql);

		POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: Arg 3 (srs) is %s", srs);
	}
	else
		srs = NULL;

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Generating GDAL raster");
	gdal = rt_raster_to_gdal(raster, srs, format, options, &gdal_size);

	/* free memory */
	if (NULL != options) {
		for (i = j - 1; i >= 0; i--)
			pfree(options[i]);
		pfree(options);
	}
	rt_raster_destroy(raster);

	if (!gdal) {
		elog(ERROR, "RASTER_asGDALRaster: Could not allocate and generate GDAL raster");
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(3, "RASTER_asGDALRaster: GDAL raster generated with %d bytes", (int) gdal_size);

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

	POSTGIS_RT_DEBUG(3, "RASTER_asGDALRaster: Returning pointer to GDAL raster");
	PG_RETURN_POINTER(result);
}

/**
 * Needed for sizeof
 */
struct rt_gdaldriver_t {
    int idx;
    char *short_name;
    char *long_name;
		char *create_options;
};

/**
 * Returns available GDAL drivers
 */
PG_FUNCTION_INFO_V1(RASTER_getGDALDrivers);
Datum RASTER_getGDALDrivers(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

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

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;
		MemoryContextSwitchTo(oldcontext);
	}

  /* stuff done on every call of the function */
  funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	attinmeta = funcctx->attinmeta;
	drv_set2 = funcctx->user_fctx;

	/* do when there is more left to send */
	if (call_cntr < max_calls) {
		char **values;
		HeapTuple tuple;
		Datum result;

		POSTGIS_RT_DEBUGF(3, "Result %d", call_cntr);

		/*
		 * Prepare a values array for building the returned tuple.
		 * This should be an array of C strings which will
		 * be processed later by the type input functions.
		 */
		values = (char **) palloc(4 * sizeof(char *));

		values[0] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
		values[1] = (char *) palloc(
			(strlen(drv_set2[call_cntr].short_name) + 1) * sizeof(char)
		);
		values[2] = (char *) palloc(
			(strlen(drv_set2[call_cntr].long_name) + 1) * sizeof(char)
		);
		values[3] = (char *) palloc(
			(strlen(drv_set2[call_cntr].create_options) + 1) * sizeof(char)
		);

		snprintf(
			values[0],
			sizeof(char) * (MAX_INT_CHARLEN + 1),
			"%d",
			drv_set2[call_cntr].idx
		);
		snprintf(
			values[1],
			(strlen(drv_set2[call_cntr].short_name) + 1) * sizeof(char),
			"%s",
			drv_set2[call_cntr].short_name
		);
		snprintf(
			values[2],
			(strlen(drv_set2[call_cntr].long_name) + 1) * sizeof(char),
			"%s",
			drv_set2[call_cntr].long_name
		);
		snprintf(
			values[3],
			(strlen(drv_set2[call_cntr].create_options) + 1) * sizeof(char),
			"%s",
			drv_set2[call_cntr].create_options
		);

		POSTGIS_RT_DEBUGF(4, "Result %d, Index %s", call_cntr, values[0]);
		POSTGIS_RT_DEBUGF(4, "Result %d, Short Name %s", call_cntr, values[1]);
		POSTGIS_RT_DEBUGF(4, "Result %d, Full Name %s", call_cntr, values[2]);
		POSTGIS_RT_DEBUGF(5, "Result %d, Create Options %s", call_cntr, values[3]);

		/* build a tuple */
		tuple = BuildTupleFromCStrings(attinmeta, values);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* clean up (this is not really necessary) */
		pfree(values[3]);
		pfree(values[2]);
		pfree(values[1]);
		pfree(values[0]);
		pfree(values);

		SRF_RETURN_NEXT(funcctx, result);
	}
	/* do when there is no more left */
	else {
		pfree(drv_set2);
		SRF_RETURN_DONE(funcctx);
	}
}

/**
 * Transform a raster to a new projection
 */
PG_FUNCTION_INFO_V1(RASTER_transform);
Datum RASTER_transform(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_pgraster *pgrast = NULL;
	rt_raster raster = NULL;
	rt_raster rast = NULL;

	text *algtext = NULL;
	char *algchar = NULL;
	GDALResampleAlg alg = GRA_NearestNeighbour;

	int src_srid = -1;
	char *src_srs = NULL;
	int dst_srid = -1;
	char *dst_srs = NULL;
	double max_err = 0.125;

	int sqllen = 0;
	char *sql = NULL;
	int ret;
	TupleDesc tupdesc;
	SPITupleTable * tuptable = NULL;
	HeapTuple tuple;

	POSTGIS_RT_DEBUG(3, "RASTER_transform: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	raster = rt_raster_deserialize(pgraster);
	if (!raster) {
		elog(ERROR, "RASTER_transform: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* source srid */
	src_srid = rt_raster_get_srid(raster);
	if (src_srid == SRID_UNKNOWN) {
		elog(ERROR, "RASTER_transform: Input raster has unknown (-1) SRID");
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(4, "source srid: %d", src_srid);

	/* target srid */
	if (PG_ARGISNULL(1)) PG_RETURN_NULL();
	dst_srid = PG_GETARG_INT32(1);
	if (dst_srid == SRID_UNKNOWN) {
		elog(ERROR, "RASTER_transform: -1 is an invalid target SRID");
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}
	POSTGIS_RT_DEBUGF(4, "destination srid: %d", dst_srid);

	/* resampling algorithm */
	if (!PG_ARGISNULL(2)) {
		algtext = PG_GETARG_TEXT_P(2);
		algchar = text_to_cstring(algtext);
		alg = rt_util_gdal_resample_alg(algchar);
	}
	POSTGIS_RT_DEBUGF(4, "Resampling algorithm: %d", alg);

	/* max error */
	if (!PG_ARGISNULL(3)) {
		max_err = PG_GETARG_FLOAT8(3);
		if (max_err < 0.) max_err = 0.;
	}
	POSTGIS_RT_DEBUGF(4, "max_err: %f", max_err);

	/* get srses from srids */
	sqllen = sizeof(char) * (strlen("SELECT _ST_srtext()") + MAX_INT_CHARLEN);
	sql = (char *) palloc(sqllen);
	if (NULL == sql) {
		elog(ERROR, "RASTER_transform: Unable to allocate memory for SRID queries");
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}

	SPI_connect();

	/* source srs */
	snprintf(sql, sqllen, "SELECT _ST_srtext(%d)", src_srid);
	POSTGIS_RT_DEBUGF(4, "sourse srs sql: %s", sql);
	ret = SPI_execute(sql, TRUE, 0);
	if (ret != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
		elog(ERROR, "RASTER_transform: Input raster has unknown SRID");
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		rt_raster_destroy(raster);
		pfree(sql);
		PG_RETURN_NULL();
	}
	tupdesc = SPI_tuptable->tupdesc;
	tuptable = SPI_tuptable;
	tuple = tuptable->vals[0];
	src_srs = SPI_getvalue(tuple, tupdesc, 1);
	if (NULL == src_srs || !strlen(src_srs)) {
		elog(ERROR, "RASTER_transform: Input raster has invalid SRID");
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		rt_raster_destroy(raster);
		pfree(sql);
		PG_RETURN_NULL();
	}
	SPI_freetuptable(tuptable);

	/* target srs */
	sprintf(sql, "SELECT _ST_srtext(%d)", dst_srid);
	POSTGIS_RT_DEBUGF(4, "destination srs sql: %s", sql);
	ret = SPI_execute(sql, TRUE, 0);
	if (ret != SPI_OK_SELECT || SPI_tuptable == NULL || SPI_processed != 1) {
		elog(ERROR, "RASTER_transform: Target SRID is unknown");
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		rt_raster_destroy(raster);
		pfree(sql);
		PG_RETURN_NULL();
	}
	tupdesc = SPI_tuptable->tupdesc;
	tuptable = SPI_tuptable;
	tuple = tuptable->vals[0];
	dst_srs = SPI_getvalue(tuple, tupdesc, 1);
	if (NULL == dst_srs || !strlen(dst_srs)) {
		elog(ERROR, "RASTER_transform: Target SRID is unknown");
		if (SPI_tuptable) SPI_freetuptable(tuptable);
		SPI_finish();
		rt_raster_destroy(raster);
		pfree(sql);
		PG_RETURN_NULL();
	}
	SPI_freetuptable(tuptable);

	SPI_finish();
	pfree(sql);

	rast = rt_raster_transform(raster, src_srs, dst_srs, alg, max_err);
	rt_raster_destroy(raster);
	if (!rast) {
		elog(ERROR, "RASTER_band: Could not create transformed raster");
		PG_RETURN_NULL();
	}

	/* add target srid */
	rt_raster_set_srid(rast, dst_srid);

	pgrast = rt_raster_serialize(rast);
	rt_raster_destroy(rast);
	if (NULL == pgrast) PG_RETURN_NULL();

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

	uint16_t numBands;
	double scaleX;
	double scaleY;
	double ipX;
	double ipY;
	double skewX;
	double skewY;
	int32_t srid;
	uint16_t width;
	uint16_t height;

	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	int i = 0;
	char **values = NULL;
	int values_length = 10;
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_metadata: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	/* TODO: can be optimized to only detoast the header! */
	raster = rt_raster_deserialize(pgraster);
	if (!raster) {
		elog(ERROR, "RASTER_transform: Could not deserialize raster");
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

	/*
	 * generate attribute metadata needed later to produce tuples from raw
	 * C strings
	 */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	/*
	 * Prepare a values array for building the returned tuple.
	 * This should be an array of C strings which will
	 * be processed later by the type input functions.
	 */
	values = (char **) palloc(values_length * sizeof(char *));

	values[0] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[1] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[2] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	values[3] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	values[4] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[5] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[6] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[7] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[8] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	values[9] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));

	snprintf(
		values[0],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		ipX
	);
	snprintf(
		values[1],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		ipY
	);
	snprintf(
		values[2],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		width
	);
	snprintf(
		values[3],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		height
	);
	snprintf(
		values[4],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		scaleX
	);
	snprintf(
		values[5],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		scaleY
	);
	snprintf(
		values[6],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		skewX
	);
	snprintf(
		values[7],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		skewY
	);
	snprintf(
		values[8],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		srid
	);
	snprintf(
		values[9],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		numBands
	);

	/* build a tuple */
	tuple = BuildTupleFromCStrings(attinmeta, values);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	for (i = 0; i < values_length; i++) pfree(values[i]);
	pfree(values);

	PG_RETURN_DATUM(result);
}

/**
 * Get raster band's meta data
 */
PG_FUNCTION_INFO_V1(RASTER_bandmetadata);
Datum RASTER_bandmetadata(PG_FUNCTION_ARGS)
{
	rt_pgraster *pgraster = NULL;
	rt_raster raster = NULL;
	rt_band band = NULL;

	TupleDesc tupdesc;
	AttInMetadata *attinmeta;

	uint32_t numBands;
	uint32_t bandindex = 1;
	const char *pixtypename = NULL;
	bool hasnodatavalue = FALSE;
	double nodatavalue;
	const char *bandpath = NULL;
	bool isoutdb = FALSE;

	int i = 0;
	char **values = NULL;
	int values_length = 5;
	HeapTuple tuple;
	Datum result;

	POSTGIS_RT_DEBUG(3, "RASTER_bandmetadata: Starting");

	/* pgraster is null, return null */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	pgraster = (rt_pgraster *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* raster */
	/* TODO: can be optimized to only detoast the header! */
	raster = rt_raster_deserialize(pgraster);
	if (!raster) {
		elog(ERROR, "RASTER_transform: Could not deserialize raster");
		PG_RETURN_NULL();
	}

	/* numbands */
	numBands = rt_raster_get_num_bands(raster);
	if (numBands < 1) {
		elog(NOTICE, "Raster provided has no bands");
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}

	/* band index */
	if (!PG_ARGISNULL(1)) {
		bandindex = PG_GETARG_INT32(1);
		if (bandindex < 1 || bandindex > numBands) {
			elog(NOTICE, "Invalid band index (must use 1-based). Returning NULL");
			rt_raster_destroy(raster);
			PG_RETURN_NULL();
		}
	}

	band = rt_raster_get_band(raster, bandindex - 1);
	if (NULL == band) {
		elog(NOTICE, "Could not get raster band at index %d", bandindex);
		rt_raster_destroy(raster);
		PG_RETURN_NULL();
	}

	/* pixeltype */
	pixtypename = rt_pixtype_name(rt_band_get_pixtype(band));

	/* hasnodatavalue */
	if (rt_band_get_hasnodata_flag(band)) hasnodatavalue = TRUE;

	/* nodatavalue */
	nodatavalue = rt_band_get_nodata(band);

	/* path */
	bandpath = rt_band_get_ext_path(band);

	/* isoutdb */
	isoutdb = bandpath ? TRUE : FALSE;

	rt_band_destroy(band);
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

	/*
	 * generate attribute metadata needed later to produce tuples from raw
	 * C strings
	 */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	/*
	 * Prepare a values array for building the returned tuple.
	 * This should be an array of C strings which will
	 * be processed later by the type input functions.
	 */
	values = (char **) palloc(values_length * sizeof(char *));

	values[0] = (char *) palloc(sizeof(char) * (strlen(pixtypename) + 1));
	values[1] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	values[2] = (char *) palloc(sizeof(char) * (MAX_DBL_CHARLEN + 1));
	values[3] = (char *) palloc(sizeof(char) * (MAX_INT_CHARLEN + 1));
	if (bandpath)
		values[4] = (char *) palloc(sizeof(char) * (strlen(bandpath) + 1));
	else
		values[4] = NULL;

	snprintf(
		values[0],
		sizeof(char) * (strlen(pixtypename) + 1),
		"%s",
		pixtypename
	);
	snprintf(
		values[1],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		hasnodatavalue
	);
	snprintf(
		values[2],
		sizeof(char) * (MAX_DBL_CHARLEN + 1),
		"%f",
		nodatavalue
	);
	snprintf(
		values[3],
		sizeof(char) * (MAX_INT_CHARLEN + 1),
		"%d",
		isoutdb
	);
	if (bandpath) {
		snprintf(
			values[4],
			sizeof(char) * (strlen(bandpath) + 1),
			"%s",
			bandpath
		);
	}

	/* build a tuple */
	tuple = BuildTupleFromCStrings(attinmeta, values);

	/* make the tuple into a datum */
	result = HeapTupleGetDatum(tuple);

	/* clean up */
	for (i = 0; i < values_length; i++) {
		if (NULL != values[i]) pfree(values[i]);
	}
	pfree(values);

	PG_RETURN_DATUM(result);
}

/* ---------------------------------------------------------------- */
/*  Memory allocation / error reporting hooks                       */
/* ---------------------------------------------------------------- */

static void *
rt_pg_alloc(size_t size)
{
    void * result;

    POSTGIS_RT_DEBUGF(5, "rt_pgalloc(%ld) called", size);

    result = palloc(size);

    return result;
}

static void *
rt_pg_realloc(void *mem, size_t size)
{
    void * result;

    POSTGIS_RT_DEBUGF(5, "rt_pg_realloc(%ld) called", size);

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


void
rt_init_allocators(void)
{
    /* raster callback - install raster handlers */
    rt_set_handlers(rt_pg_alloc, rt_pg_realloc, rt_pg_free, rt_pg_error,
            rt_pg_notice, rt_pg_notice);
}

