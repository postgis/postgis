/*
 * $Id$
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
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
#include <errno.h>
#include <assert.h>

#include <postgres.h> /* for palloc */
#include <access/gist.h>
#include <access/itup.h>
#include <fmgr.h>
#include <utils/elog.h>
#include <funcapi.h>

/*#include "lwgeom_pg.h"*/
#include "liblwgeom.h"
#include "rt_pg.h"
#include "pgsql_compat.h"
#include "rt_api.h"
#include "../raster_config.h"

#define POSTGIS_RASTER_WARN_ON_TRUNCATION

/*
 * This is required for builds against pgsql 8.2
 */
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Internal funcs */
static rt_context get_rt_context(FunctionCallInfoData *fcinfo);
static void *rt_pgalloc(size_t size);
static void *rt_pgrealloc(void *mem, size_t size);
static void rt_pgfree(void *mem);


/* Prototypes */

/* Utility functions */
Datum RASTER_lib_version(PG_FUNCTION_ARGS);
Datum RASTER_lib_build_date(PG_FUNCTION_ARGS);
Datum RASTER_makeEmpty(PG_FUNCTION_ARGS);
Datum RASTER_setSRID(PG_FUNCTION_ARGS);

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
Datum RASTER_setScale(PG_FUNCTION_ARGS);
Datum RASTER_setScaleXY(PG_FUNCTION_ARGS);
Datum RASTER_getXSkew(PG_FUNCTION_ARGS);
Datum RASTER_getYSkew(PG_FUNCTION_ARGS);
Datum RASTER_setSkew(PG_FUNCTION_ARGS);
Datum RASTER_setSkewXY(PG_FUNCTION_ARGS);
Datum RASTER_getXUpperLeft(PG_FUNCTION_ARGS);
Datum RASTER_getYUpperLeft(PG_FUNCTION_ARGS);
Datum RASTER_setUpperLeftXY(PG_FUNCTION_ARGS);
Datum RASTER_getBandPixelType(PG_FUNCTION_ARGS);
Datum RASTER_getBandPixelTypeName(PG_FUNCTION_ARGS);
Datum RASTER_getBandNoDataValue(PG_FUNCTION_ARGS);
Datum RASTER_setBandNoDataValue(PG_FUNCTION_ARGS);
Datum RASTER_bandIsNoData(PG_FUNCTION_ARGS);
Datum RASTER_getBandPath(PG_FUNCTION_ARGS);
Datum RASTER_getPixelValue(PG_FUNCTION_ARGS);
Datum RASTER_setPixelValue(PG_FUNCTION_ARGS);
Datum RASTER_addband(PG_FUNCTION_ARGS);
Datum RASTER_isEmpty(PG_FUNCTION_ARGS);
Datum RASTER_hasNoBand(PG_FUNCTION_ARGS);


static void *
rt_pgalloc(size_t size)
{
    void* ret;
    POSTGIS_RT_DEBUGF(5, "rt_pgalloc(%ld) called", size);
    ret = palloc(size);
    return ret;
}

static void *
rt_pgrealloc(void *mem, size_t size)
{
    void* ret;
    
    POSTGIS_RT_DEBUGF(5, "rt_pgrealloc(%p, %ld) called", mem, size);

    if ( mem )
    {
    
        POSTGIS_RT_DEBUGF(5, "rt_pgrealloc calling repalloc(%p, %ld) called", mem, size);
        ret = repalloc(mem, size);
    }
    else
    {
        POSTGIS_RT_DEBUGF(5, "rt_pgrealloc calling palloc(%ld)", size);
        ret = palloc(size);
    }
    return ret;
}

static void
rt_pgfree(void *mem)
{
    POSTGIS_RT_DEBUGF(5, "rt_pgfree(%p) calling pfree", mem);
    pfree(mem);
}

static void
rt_pgerr(const char *fmt, ...)
{
    va_list ap;
    char msg[1024];

    va_start(ap, fmt);

    vsnprintf(msg, 1023, fmt, ap);

    elog(ERROR, "%s", msg);

    va_end(ap);
}

static void
rt_pgwarn(const char *fmt, ...)
{
    va_list ap;
    char msg[1024];

    va_start(ap, fmt);

    vsnprintf(msg, 1023, fmt, ap);

    elog(WARNING, "%s", msg);

    va_end(ap);
}

static void
rt_pginfo(const char *fmt, ...)
{
    va_list ap;
    char msg[1024];

    va_start(ap, fmt);

    vsnprintf(msg, 1023, fmt, ap);

    elog(INFO, "%s", msg);

    va_end(ap);
}


static rt_context
get_rt_context(FunctionCallInfoData *fcinfo)
{
    rt_context ctx = 0;
    MemoryContext old_context;

    if ( ctx ) return ctx;

    /* We switch memory context info so the rt_context
     * is not freed by the end of function call
     * TODO: check _which_ context we should be using
     * for a whole-plugin-lifetime persistence */
    old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
    ctx = rt_context_new(rt_pgalloc, rt_pgrealloc, rt_pgfree);
    MemoryContextSwitchTo(old_context);

    rt_context_set_message_handlers(ctx, rt_pgerr, rt_pgwarn, rt_pginfo);

    return ctx;
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

/*
 * Input is a string with hex chars in it.
 * Convert to binary and put in the result
 */
PG_FUNCTION_INFO_V1(RASTER_in);
Datum RASTER_in(PG_FUNCTION_ARGS)
{
    rt_raster raster;
    char *hexwkb = PG_GETARG_CSTRING(0);
    void *result = NULL;
    rt_context ctx = get_rt_context(fcinfo);

    raster = rt_raster_from_hexwkb(ctx, hexwkb, strlen(hexwkb));
    result = rt_raster_serialize(ctx, raster);

    SET_VARSIZE(result, ((rt_pgraster*)result)->size);

    if ( NULL != result )
        PG_RETURN_POINTER(result);
    else
        PG_RETURN_NULL();
}

/*given a RASTER structure, convert it to Hex and put it in a string */
PG_FUNCTION_INFO_V1(RASTER_out);
Datum RASTER_out(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster = NULL;
    uint32_t hexwkbsize = 0;
    char *hexwkb = NULL;
    rt_context ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /*elog(NOTICE, "RASTER_out: rt_raster_deserialize returned %p", raster);*/

    hexwkb = rt_raster_to_hexwkb(ctx, raster, &hexwkbsize);
    if ( ! hexwkb )
    {
        elog(ERROR, "Could not HEX-WKBize raster");
        PG_RETURN_NULL();
    }

    /*elog(NOTICE, "RASTER_out: rt_raster_to_hexwkb returned");*/

    PG_RETURN_CSTRING(hexwkb);
}

/*
 * Return bytea object with raster in Well-Known-Binary form.
 */
PG_FUNCTION_INFO_V1(RASTER_to_bytea);
Datum RASTER_to_bytea(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster = NULL;
    rt_context ctx = get_rt_context(fcinfo);
    uint8_t *wkb = NULL;
    uint32_t wkb_size = 0;
    bytea *result = NULL;
    int result_size = 0;

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }
    /*elog(NOTICE, "RASTER_to_binary: rt_raster_deserialize returned %p", raster);*/

    /* Uses context allocator */
    wkb = rt_raster_to_wkb(ctx, raster, &wkb_size);
    if ( ! wkb ) {
        elog(ERROR, "Could not allocate and generate WKB data");
        PG_RETURN_NULL();
    }

    /* TODO: Replace all palloc/pfree, malloc/free, etc. with rt_context_t->{alloc/dealloc}
     * FIXME: ATM, impossible as rt_context_t is incomplete type for rt_pg layer. */
    result_size = wkb_size + VARHDRSZ;
    result = (bytea *)palloc(result_size);
    SET_VARSIZE(result, result_size);
    memcpy(VARDATA(result), wkb, VARSIZE(result) - VARHDRSZ);
    pfree(wkb);

    PG_RETURN_POINTER(result);
}

/*
 * Return bytea object with raster requested using ST_AsBinary function.
 */
PG_FUNCTION_INFO_V1(RASTER_to_binary);
Datum RASTER_to_binary(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    rt_raster raster = NULL;
    rt_context ctx = get_rt_context(fcinfo);
    uint8_t *wkb = NULL;
    uint32_t wkb_size = 0;
    char *result = NULL;
    int result_size = 0;

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }
    /*elog(NOTICE, "RASTER_to_binary: rt_raster_deserialize returned %p", raster);*/

    /* Uses context allocator */
    wkb = rt_raster_to_wkb(ctx, raster, &wkb_size);
    if ( ! wkb ) {
        elog(ERROR, "Could not allocate and generate WKB data");
        PG_RETURN_NULL();
    }

    /* TODO: Replace all palloc/pfree, malloc/free, etc. with rt_context_t->{alloc/dealloc}
     * FIXME: ATM, impossible as rt_context_t is incomplete type for rt_pg layer. */
    result_size = wkb_size + VARHDRSZ;
    result = (char *)palloc(result_size);
    SET_VARSIZE(result, result_size);
    memcpy(VARDATA(result), wkb, VARSIZE(result) - VARHDRSZ);
    pfree(wkb);

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
    rt_context ctx = get_rt_context(fcinfo);
    LWPOLY* convexhull;
    uchar* pglwgeom;

    { /* TODO: can be optimized to only detoast the header! */
        raster = rt_raster_deserialize(ctx, pgraster);
        if ( ! raster ) {
            elog(ERROR, "Could not deserialize raster");
            PG_RETURN_NULL();
        }

        /*elog(NOTICE, "rt_raster_deserialize returned %p", raster);*/

        convexhull = rt_raster_get_convex_hull(ctx, raster);
        if ( ! convexhull ) {
            elog(ERROR, "Could not get raster's convex hull");
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

    /* PG_FREE_IF_COPY(pgraster, 0); */
    /* lwfree(convexhull) */
    /* ... more deallocs ... */


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
    rt_pgraster *pgraster;
    rt_raster raster;
    rt_context ctx;
    FuncCallContext *funcctx;
    TupleDesc tupdesc;
    AttInMetadata *attinmeta;
    int nband;
    rt_geomval geomval;
    rt_geomval geomval2;
    int call_cntr;
    int max_calls;
    int nElements;
    MemoryContext   oldcontext;


    /* stuff done only on the first call of the function */
    if (SRF_IS_FIRSTCALL())
    {
        POSTGIS_RT_DEBUG(2, "RASTER_dumpAsWKTPolygons first call");

        /* create a function context for cross-call persistence */
        funcctx = SRF_FIRSTCALL_INIT();

        /* switch to memory context appropriate for multiple function calls */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /**
         * Create a new context. We don't call get_rt_context, because this
         * function changes the memory context to the one pointed by
         * fcinfo->flinfo->fn_mcxt, and we want to keep the current one
         */
        ctx = rt_context_new(rt_pgalloc, rt_pgrealloc, rt_pgfree);
        rt_context_set_message_handlers(ctx, rt_pgerr, rt_pgwarn, rt_pginfo);

        /* Get input arguments */
        pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
        raster = rt_raster_deserialize(ctx, pgraster);
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
        geomval = rt_raster_dump_as_wktpolygons(ctx, raster, nband, &nElements);
        if (NULL == geomval)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_NO_DATA_FOUND), 
                    errmsg("Could not polygonize raster")));
            PG_RETURN_NULL();
        }
		
        POSTGIS_RT_DEBUGF(3, "raster dump, %d elements returned", nElements);

        /**
         * Not needed to check geomval. It was allocated by the new
         * rt_context, that uses palloc. It never returns NULL
         */

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
		 * TODO: Do we have to use the same context we used
		 * for creating them?
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
    rt_context ctx;

    if ( PG_NARGS() < 9 )
    {
        elog(ERROR, "MakeEmptyRaster requires 9 args");
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

    ctx = get_rt_context(fcinfo);

    raster = rt_raster_new(ctx, width, height);
    if ( ! raster ) {
        PG_RETURN_NULL(); /* error was supposedly printed already */
    }

    rt_raster_set_scale(ctx, raster, scalex, scaley);
    rt_raster_set_offsets(ctx, raster, ipx, ipy);
    rt_raster_set_skews(ctx, raster, skewx, skewy);
    rt_raster_set_srid(ctx, raster, srid);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    int32_t srid;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    srid = rt_raster_get_srid(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    int32_t newSRID = PG_GETARG_INT32(1);

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_srid(ctx, raster, newSRID);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    uint16_t width;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    width = rt_raster_get_width(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    uint16_t height;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    height = rt_raster_get_height(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    int32_t num_bands;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    num_bands = rt_raster_get_num_bands(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xsize;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xsize = rt_raster_get_x_scale(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double ysize;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    ysize = rt_raster_get_y_scale(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double size = PG_GETARG_FLOAT8(1);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(ctx, raster, size, size);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xscale = PG_GETARG_FLOAT8(1);
    double yscale = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_scale(ctx, raster, xscale, yscale);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xskew;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xskew = rt_raster_get_x_skew(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double yskew;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yskew = rt_raster_get_y_skew(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double skew = PG_GETARG_FLOAT8(1);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(ctx, raster, skew, skew);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xskew = PG_GETARG_FLOAT8(1);
    double yskew = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_skews(ctx, raster, xskew, yskew);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xul;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    xul = rt_raster_get_x_offset(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double yul;

    /* TODO: can be optimized to only detoast the header! */
    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    yul = rt_raster_get_y_offset(ctx, raster);
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
    rt_context ctx = get_rt_context(fcinfo);
    double xoffset = PG_GETARG_FLOAT8(1);
    double yoffset = PG_GETARG_FLOAT8(2);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    rt_raster_set_offsets(ctx, raster, xoffset, yoffset);

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = NULL;
    rt_pixtype pixtype;
    int32_t index;

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(ctx, band);
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
    rt_context ctx = NULL;
    rt_pixtype pixtype;
    int32_t index;
    const size_t name_size = 8; /* size of type name */
    size_t size = 0;
    char *ptr = NULL;
    text *result = NULL;

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its pixel type */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }

    pixtype = rt_band_get_pixtype(ctx, band);

    result = palloc(VARHDRSZ + name_size);
    if ( ! result ) {
        elog(ERROR, "Could not allocate memory for output text object");
        PG_RETURN_NULL();
    }
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
            elog(ERROR, "Invalid value of pixel type");
            pfree(result);
            PG_RETURN_NULL();
    }

    size = VARHDRSZ + strlen(ptr);
    SET_VARSIZE(result, size);

    PG_RETURN_TEXT_P(result);
}

/**
 * Return NODATA value of the specified band of raster.
 * The value is always returned as FLOAT32 even if the pixel type is INTEGER.
 */
PG_FUNCTION_INFO_V1(RASTER_getBandNoDataValue);
Datum RASTER_getBandNoDataValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    rt_context ctx = NULL;
    double nodata;
    int32_t index;

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its NODATA value */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }

    if ( ! rt_band_get_hasnodata_flag(ctx, band) ) {
        //elog(WARNING, "Raster band %d does not have a NODATA value", index);
        PG_RETURN_NULL();
    }

    nodata = rt_band_get_nodata(ctx, band);
    PG_RETURN_FLOAT4(nodata);
}

  
/**
 * Set the NODATA value of the specified band of raster.
 */
PG_FUNCTION_INFO_V1(RASTER_setBandNoDataValue);
Datum RASTER_setBandNoDataValue(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    rt_context ctx = NULL;
    double nodata;
    int32_t index;
    bool forceChecking = FALSE;

    /* Check index is not NULL */
    if ((Pointer *)PG_GETARG_DATUM(1) == NULL) {
        /* Simply return NULL */
        PG_RETURN_NULL();
    }

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    if ((Pointer *)PG_GETARG_DATUM(0) == NULL) {
        /* Simply return NULL */
        PG_RETURN_NULL();
    }
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }
    
    
    forceChecking = PG_GETARG_BOOL(3);


    if ((Pointer *)PG_GETARG_DATUM(2) == NULL) {
        /* Set the hasnodata flag to FALSE */
        rt_band_set_hasnodata_flag(ctx, band, FALSE);

        POSTGIS_RT_DEBUGF(3, "Raster band %d does not have a NODATA value",
                index);
    }

    else {

        /* Get the NODATA value */
        nodata = PG_GETARG_FLOAT8(2);


        /* Set the band's NODATA value */
        rt_band_set_nodata(ctx, band, nodata);

        /* Set the hasnodata flag to TRUE */
        rt_band_set_hasnodata_flag(ctx, band, TRUE);

        /* Recheck all pixels if requested */
        if (forceChecking)
           rt_band_check_is_nodata(ctx, band); 

    }

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
    PG_RETURN_POINTER(pgraster);
}

PG_FUNCTION_INFO_V1(RASTER_bandIsNoData);
Datum RASTER_bandIsNoData(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    rt_band band = NULL;
    rt_context ctx = NULL;
    int32_t index;
    bool forceChecking = FALSE;

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its NODATA value */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }

    forceChecking = PG_GETARG_BOOL(2);
    
    if (forceChecking)    
	    PG_RETURN_BOOL(rt_band_check_is_nodata(ctx, band));
    else
        PG_RETURN_BOOL(rt_band_get_isnodata_flag(ctx, band));
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
    rt_context ctx = NULL;
    int32_t index;
    const char *bandpath;
	text *result;

    /* Index is 1-based */
    index = PG_GETARG_INT32(1);
    if ( index < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (index - 1));

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band and its NODATA value */
    band = rt_raster_get_band(ctx, raster, index - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", index);
        PG_RETURN_NULL();
    }

    bandpath = rt_band_get_ext_path(ctx, band);
    if ( ! bandpath )
    {
        PG_RETURN_NULL();
    }

    result = (text *) palloc(VARHDRSZ + strlen(bandpath) + 1);
    if ( ! result ) {
        elog(ERROR, "Could not allocate memory for output text object");
        PG_RETURN_NULL();
    }
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
    rt_context ctx = NULL;
    double pixvalue = 0;
    int32_t nband = 0;
    int32_t x = 0;
    int32_t y = 0;
    int result = 0;
    bool hasnodata = TRUE;

    /* Index is 1-based */
    nband = PG_GETARG_INT32(1);
    if ( nband < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (nband - 1));

    /* Validate pixel coordinates are in range */
    if (PG_ARGISNULL(2)) {
        elog(NOTICE, "x coordinate can not be NULL. Returning NULL");
        PG_RETURN_NULL();
    }
    x = PG_GETARG_INT32(2);

    if (PG_ARGISNULL(3)) {
        elog(NOTICE, "y coordinate can not be NULL. Returning NULL");
        PG_RETURN_NULL();
    }
    y = PG_GETARG_INT32(3);

    if (!PG_ARGISNULL(4))
        hasnodata = PG_GETARG_BOOL(4);

    POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if (!raster) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch Nth band using 0-based internal index */
    band = rt_raster_get_band(ctx, raster, nband - 1);
    if (! band) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", nband);
        PG_RETURN_NULL();
    }
    /* Fetch pixel using 0-based coordiantes */
    result = rt_band_get_pixel(ctx, band, x - 1, y - 1, &pixvalue);
    if (result == -1 || (hasnodata && rt_band_get_hasnodata_flag(ctx, band) && pixvalue == rt_band_get_nodata(ctx, band))) {
        //elog(WARNING, "Raster band %d does not have a NODATA value", index);
        PG_RETURN_NULL();
    }

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
    rt_context ctx = NULL;
    double pixvalue = 0;
    int32_t nband = 0;
    int32_t x = 0;
    int32_t y = 0;

    /* nband is 1-based */
    nband = PG_GETARG_INT32(1);
    if ( nband < 1 ) {
        elog(ERROR, "Invalid band index (must use 1-based)");
        PG_RETURN_NULL();
    }
    assert(0 <= (nband - 1));

    /* Validate pixel coordinates are in range */
    x = PG_GETARG_INT32(2);
    y = PG_GETARG_INT32(3);
    
    POSTGIS_RT_DEBUGF(3, "Pixel coordinates (%d, %d)", x, y);

    /* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }

    /* Fetch requested band */
    band = rt_raster_get_band(ctx, raster, nband - 1);
    if ( ! band ) {
        elog(NOTICE, "Could not find raster band of index %d. Returning NULL", nband);
        PG_RETURN_NULL();
    }

    /* Set the pixel value */
    if (PG_ARGISNULL(4)) {
        if (!rt_band_get_hasnodata_flag(ctx, band)) {
            elog(NOTICE, "Raster do not have a nodata value defined. Pixel value not set. Returning raster");
        }
        else {
            pixvalue = rt_band_get_nodata(ctx, band);
            rt_band_set_pixel(ctx, band, x - 1, y - 1, pixvalue);
        }
    }
    else {
        pixvalue = PG_GETARG_FLOAT8(4);
        rt_band_set_pixel(ctx, band, x - 1, y - 1, pixvalue);
    }

    pgraster = rt_raster_serialize(ctx, raster);
    if ( ! pgraster ) PG_RETURN_NULL();
 
    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_band band = NULL;
    rt_context ctx = NULL;
    
    int index = 0;
    double initialvalue = 0;
    double nodatavalue = 0;
    bool hasnodata = FALSE;

    text *pixeltypename = NULL;
    char *new_pixeltypename = NULL;

    int pixtype = PT_END;
    int width = 0;
    int height = 0;
    int32_t oldnumbands = 0;
    int32_t numbands = 0;
    
    int datasize = 0;
    int numval = 0;
    void *mem;
    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    double checkvaldouble = 0;
    float checkvalfloat = 0;
    int i;
    
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
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);

    /* Get the pixel type in text form */
    if (PG_ARGISNULL(2)) {
        elog(ERROR, "Pixel type can not be null");
        PG_RETURN_NULL();
    }
      
    pixeltypename = PG_GETARG_TEXT_P(2);
    new_pixeltypename = (char *) palloc(VARSIZE(pixeltypename) + 1 - VARHDRSZ);
    SET_VARSIZE(new_pixeltypename, VARSIZE(pixeltypename));
    memcpy(new_pixeltypename, VARDATA(pixeltypename), VARSIZE(pixeltypename) - VARHDRSZ);
    new_pixeltypename[VARSIZE(pixeltypename) - VARHDRSZ] = 0; /* null terminate */

    /* Get the pixel type index */
    pixtype = rt_pixtype_index_from_name(ctx, new_pixeltypename);
    if ( pixtype == PT_END ) {
        elog(ERROR, "Invalid pixel type: %s", new_pixeltypename);
        PG_RETURN_NULL();
    }
    assert(-1 < pixtype && pixtype < PT_END);

    raster = rt_raster_deserialize(ctx, pgraster);
    if ( ! raster ) {
        elog(ERROR, "Could not deserialize raster");
        PG_RETURN_NULL();
    }
    
    /* Make sure index (1 based) is in a valid range */
    oldnumbands = rt_raster_get_num_bands(ctx, raster);
    if (PG_ARGISNULL(1))
        index = oldnumbands + 1;
    else
    {
        index = PG_GETARG_UINT16(1);
        if (index < 1) {
            elog(ERROR, "Invalid band index (must be 1-based)");
            PG_RETURN_NULL();
        }
        if (index > oldnumbands + 1) {   
            elog(WARNING, "Band index number exceed possible values, truncated to number of band (%u) + 1", oldnumbands);
            index = oldnumbands + 1;
        }
    }
    
    /* Determine size of memory block to allocate and allocate*/
    width = rt_raster_get_width(ctx, raster);
    height = rt_raster_get_height(ctx, raster);
    numval = width * height;
    datasize = rt_pixtype_size(ctx, pixtype) * numval;
    
    
    mem = palloc((int) datasize);
    if (!mem)
    {
        elog(ERROR, "Could not allocate memory for band");
        PG_RETURN_NULL();
    }
        
    /* Initialize block of memory to 0 or to the provided initial value */
    if (initialvalue == 0)
        memset(mem, 0, datasize);
    else
    {
        switch (pixtype)
        {
            case PT_1BB:
            {
                uint8_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint8_t) initialvalue&0x01;
                checkvalint = ptr[0];
                break;
            }
            case PT_2BUI:
            {
                uint8_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint8_t) initialvalue&0x03;
                checkvalint = ptr[0];
                break;
            }
            case PT_4BUI:
            {
                uint8_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint8_t) initialvalue&0x0F;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BSI:
            {
                int8_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (int8_t) initialvalue;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BUI:
            {
                uint8_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint8_t) initialvalue;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BSI:
            {
                int16_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (int16_t) initialvalue;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BUI:
            {
                uint16_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint16_t) initialvalue;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BSI:
            {
                int32_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (int32_t) initialvalue;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BUI:
            {
                uint32_t *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (uint32_t) initialvalue;
                checkvaluint = ptr[0];
                break;
            }
            case PT_32BF:
            {
                float *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = (float) initialvalue;
                checkvalfloat = ptr[0];
                break;
            }
            case PT_64BF:
            {
                double *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = initialvalue;
                checkvaldouble = ptr[0];
                break;
            }
            default:
            {
                elog(ERROR, "Unknown pixeltype %d", pixtype);
                PG_RETURN_NULL();
            }
        }
    }

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    /* Overflow checking */
    switch (pixtype)
    {
        case PT_1BB:
        case PT_2BUI:
        case PT_4BUI:
        case PT_8BSI:
        case PT_8BUI:
        case PT_16BSI:
        case PT_16BUI:
        case PT_32BSI:
        {
            if (fabs(checkvalint - initialvalue) > 0.0001)
                elog(WARNING, "Initial pixel value for %s band got truncated from %f to %d",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvalint);
            break;
        }
        case PT_32BUI:
        {
            if (fabs(checkvaluint - initialvalue) > 0.0001)
                elog(WARNING, "Initial pixel value for %s band got truncated from %f to %u",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvaluint);
            break;
        }
        case PT_32BF:
        {
            /* For float, because the initial value is a double, 
            there is very often a difference between the desired value and the obtained one */
            if (checkvalfloat != initialvalue)
                elog(WARNING, "Initial pixel value for %s band got truncated from %f to %g",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvalfloat);
            break;
        }
        case PT_64BF:
        {
            if (checkvaldouble != initialvalue)
                elog(WARNING, "Initial pixel value for %s band got truncated from %f to %g",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvaldouble);
            break;
        }
    }
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    band = rt_band_new_inline(ctx, width, height, pixtype, hasnodata, nodatavalue, mem);
    if (! band) {
        elog(ERROR, "Could not add band to raster. Returning NULL");
        PG_RETURN_NULL();
    }
    index = rt_raster_add_band(ctx, raster, band, index - 1);
    numbands = rt_raster_get_num_bands(ctx, raster);
    if (numbands == oldnumbands || index == -1) {
        elog(ERROR, "Could not add band to raster. Returning NULL");
        PG_RETURN_NULL();
    }
    pgraster = rt_raster_serialize(ctx, raster);
    if (!pgraster) PG_RETURN_NULL();

    SET_VARSIZE(pgraster, pgraster->size);
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
    rt_context ctx = NULL;
	
	/* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);
	raster = rt_raster_deserialize(ctx, pgraster);
	if ( ! raster ) 
	{
		ereport(ERROR,
			(errcode(ERRCODE_OUT_OF_MEMORY), 
				errmsg("Could not deserialize raster")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_BOOL(rt_raster_is_empty(ctx, raster));
}

/**
 * Check if the raster has a given band or not
 */
PG_FUNCTION_INFO_V1(RASTER_hasNoBand);
Datum RASTER_hasNoBand(PG_FUNCTION_ARGS)
{
    rt_pgraster *pgraster = NULL;
    rt_raster raster = NULL;
    int nBand = 0;
    rt_context ctx = NULL;
	
	/* Deserialize raster */
    pgraster = (rt_pgraster *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
    ctx = get_rt_context(fcinfo);
	raster = rt_raster_deserialize(ctx, pgraster);
	if ( ! raster ) 
	{
		ereport(ERROR,
			(errcode(ERRCODE_OUT_OF_MEMORY), 
				errmsg("Could not deserialize raster")));
		PG_RETURN_NULL();
	}
	
	/* Get band number */
	nBand = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(rt_raster_has_no_band(ctx, raster, nBand));
}


/* ---------------------------------------------------------------- */
/*  Memory allocation / error reporting hooks                       */
/* ---------------------------------------------------------------- */

static void *
rt_pg_alloc(size_t size)
{
	void * result;

	result = palloc(size);

	if ( ! result )
	{
		ereport(ERROR, (errmsg_internal("Out of virtual memory")));
		return NULL;
	}
	return result;
}

static void *
rt_pg_realloc(void *mem, size_t size)
{
	void * result;

	result = repalloc(mem, size);

	return result;
}

static void
rt_pg_free(void *ptr)
{
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

/* ---------------------------------------------------------------- */
/*  Memory allocation / error reporting hooks                       */
/* ---------------------------------------------------------------- */
