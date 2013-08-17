
/**********************************************************************
 * $Id: lwout_twkb.h 4715 2009-11-01 17:58:42Z nicklas $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include <limits.h>

/**
* Macros for manipulating the 'endian_precision' int. An int8_t used as follows:
* Endianess 1 bit
* 3 unused bits
* Precision  4 bits
*/

#define END_PREC_SET__ENDIANESS(flag, endianess) ((flag) = (flag & 0xFE) | ((endianess & 0x01)))
#define END_PREC_SET__METHOD(flag, method) ((flag) = (flag & 0xF1) | ((method<<1) & 0x0E))
#define END_PREC_SET__PRECISION(flag, prec) ((flag) = (flag & 0x0F) | ((prec<<4) & 0xF0))


/**
* Macros for manipulating the 'type_dim' int. An int8_t used as follows:
* Type 5 bits
* NDims  3 bits
*/

#define TYPE_DIM_SET_TYPE(flag, type) ((flag) = (flag & 0xE0) | ((type & 0x1F)))
#define TYPE_DIM_SET_DIM(flag, dim) ((flag) = (flag & 0x1F) | ((dim & 0x07)<<5))

/*For variant variable that holds options when building the twkb*/
#define WKB_NO_ID 0x01 /* This position has another meaning when building wkb! */
#define WKB_NO_TYPE 0x02 /* This position has another meaning when building wkb! */

int s_getvarint_size(long val);
int u_getvarint_size(unsigned long val);

static size_t ptarray_to_twkb_size(const POINTARRAY *pa, uint8_t variant,int prec,int accum_rel[],int method);
static uint8_t* ptarray_to_twkb_buf(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int accum_rel[],int method);

static size_t ptarray_to_twkb_size_m0(const POINTARRAY *pa, uint8_t variant,int prec,int accum_rel[]);
static uint8_t* ptarray_to_twkb_buf_m0(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int accum_rel[]);
static size_t ptarray_to_twkb_size_m1(const POINTARRAY *pa, uint8_t variant,int prec,int accum_rel[]);
static uint8_t* ptarray_to_twkb_buf_m1(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int accum_rel[]);

static size_t  lwgeom_agg_to_twkbpoint_size(lwgeom_id *geom_array, uint8_t variant,int n,int8_t prec,int refpoint[],int method);
static size_t lwpoint_to_twkb_size(const LWPOINT *pt, uint8_t variant, int8_t prec, uint32_t id,int refpoint[],int method);
static uint8_t* lwgeom_agg_to_twkbpoint_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method);
static uint8_t* lwpoint_to_twkb_buf(const LWPOINT *pt, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);

static size_t  lwgeom_agg_to_twkbline_size(lwgeom_id *geom_array, uint8_t variant,int n,int8_t prec,int refpoint[],int method);
static size_t lwline_to_twkb_size(const LWLINE *line, uint8_t variant, int8_t prec, uint32_t id,int refpoint[],int method);
static uint8_t* lwgeom_agg_to_twkbline_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method);
static uint8_t* lwline_to_twkb_buf(const LWLINE *line, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);

static size_t  lwgeom_agg_to_twkbpoly_size(lwgeom_id *geom_array, uint8_t variant,int n,int8_t prec,int refpoint[],int method);
static size_t lwpoly_to_twkb_size(const LWPOLY *poly, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);
static uint8_t* lwgeom_agg_to_twkbpoly_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method);
static uint8_t* lwpoly_to_twkb_buf(const LWPOLY *poly, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);

static size_t  lwgeom_agg_to_twkbcollection_size(lwgeom_id *geom_array, uint8_t variant,int n,int8_t prec,int refpoint[],int method);
static size_t lwcollection_to_twkb_size(const LWCOLLECTION *col, uint8_t variant, int8_t prec, uint32_t id,int refpoint[],int method);
static uint8_t* lwgeom_agg_to_twkbcollection_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method);
static uint8_t* lwcollection_to_twkb_buf(const LWCOLLECTION *col, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);

static size_t lwgeom_to_twkb_size(const LWGEOM *geom, uint8_t variant, int8_t prec, uint32_t id,int refpoint[],int method);
static uint8_t* lwgeom_to_twkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method);


//static size_t lwgeom_to_twkb_size(const LWGEOM *geom, uint8_t variant,int8_t prec);

