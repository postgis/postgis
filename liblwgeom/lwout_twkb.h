
/**********************************************************************
 * $Id: lwout_twkb.h 4715 2009-11-01 17:58:42Z nicklas $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include <limits.h>

//Maximum number of geometry dimmensions that internal arrays can hold
#define MAX_N_DIMS 4

/**
* If the twkb includes an ID 1 bit
* If the twkb includes its size 1 bit
* If the twkb includes bboxes 1 bit
* Precision  4 bits
*/


#define FIRST_BYTE_SET_ID(flag, id) ((flag) = (flag & 0xFE) | ((id & 0x01)))
#define FIRST_BYTE_SET_SIZES(flag, sizes) ((flag) = (flag & 0xFD) | ((sizes & 0x02)))
#define FIRST_BYTE_SET_BBOXES(flag, bboxes) ((flag) = (flag & 0xFB) | ((bboxes & 0x04)))

#define FIRST_BYTE_SET_PRECISION(flag, prec) ((flag) = (flag & 0x0F) | ((prec<<4) & 0xF0))


/**
* Macros for manipulating the 'type_dim' int. An int8_t used as follows:
* Type 5 bits
* NDims  3 bits
*/

#define TYPE_DIM_SET_TYPE(flag, type) ((flag) = (flag & 0xE0) | ((type & 0x1F)))
#define TYPE_DIM_SET_DIM(flag, dim) ((flag) = (flag & 0x1F) | ((dim & 0x07)<<5))

static size_t ptarray_to_twkb_size(const POINTARRAY *pa, uint8_t variant,int prec,int64_t accum_rel[3][4]);
static int ptarray_to_twkb_buf(const POINTARRAY *pa, uint8_t **buf, uint8_t variant,int64_t factor,int64_t accum_rel[]);

static size_t  lwgeom_agg_to_twkbpoint_size(lwgeom_id *geom_array, uint8_t *variant,int n,int64_t factor,int64_t refpoint[3][4]);
static size_t lwpoint_to_twkb_size(const LWPOINT *pt, uint8_t *variant, int64_t factor, int64_t id,int64_t refpoint[3][4]);
static int lwgeom_agg_to_twkbpoint_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t refpoint[3][4]);
static int lwpoint_to_twkb_buf(const LWPOINT *pt, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[]);

static size_t  lwgeom_agg_to_twkbline_size(lwgeom_id *geom_array, uint8_t *variant,int n,int64_t factor,int64_t refpoint[3][4]);
static size_t lwline_to_twkb_size(const LWLINE *line, uint8_t *variant, int64_t factor, int64_t id,int64_t refpoint[3][4]);
static int lwgeom_agg_to_twkbline_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t refpoint[3][4]);
static int lwline_to_twkb_buf(const LWLINE *line, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[]);

static size_t  lwgeom_agg_to_twkbpoly_size(lwgeom_id *geom_array, uint8_t *variant,int n,int64_t factor,int64_t refpoint[3][4]);
static size_t lwpoly_to_twkb_size(const LWPOLY *poly, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[3][4]);
static int lwgeom_agg_to_twkbpoly_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t refpoint[3][4]);
static int lwpoly_to_twkb_buf(const LWPOLY *poly, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[]);

static size_t  lwgeom_agg_to_twkbcollection_size(lwgeom_id *geom_array, uint8_t *variant,int n,int64_t factor,int64_t refpoint[3][4]);
static size_t lwcollection_to_twkb_size(const LWCOLLECTION *col, uint8_t *variant, int64_t factor, int64_t id,int64_t refpoint[3][4]);
static int lwgeom_agg_to_twkbcollection_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t refpoint[3][4]);
static int lwcollection_to_twkb_buf(const LWCOLLECTION *col, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[3][4]);

static size_t lwgeom_to_twkb_size(const LWGEOM *geom, uint8_t *variant, int64_t factor, int64_t id,int64_t refpoint[3][4]);
static int lwgeom_to_twkb_buf(const LWGEOM *geom, uint8_t **buf, uint8_t *variant,int64_t factor, int64_t id,int64_t refpoint[3][4]);


//static size_t lwgeom_to_twkb_size(const LWGEOM *geom, uint8_t variant,int64_t factor);

