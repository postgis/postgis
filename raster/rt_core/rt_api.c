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
#include <stdio.h>  /* for printf (default message handler) */
#include <stdarg.h> /* for va_list, va_start etc */
#include <string.h> /* for memcpy and strlen */
#include <assert.h>
#include <float.h> /* for FLT_EPSILON and float type limits */
#include <limits.h> /* for integer type limits */
#include <time.h> /* for time */
#include "rt_api.h"

#define POSTGIS_RASTER_WARN_ON_TRUNCATION


/*--- Utilities -------------------------------------------------*/

static void
swap_char(uint8_t *a, uint8_t *b) {
    uint8_t c = 0;

    assert(NULL != a && NULL != b);

    c = *a;
    *a = *b;
    *b = c;
}

static void
flip_endian_16(uint8_t *d) {
    assert(NULL != d);

    swap_char(d, d + 1);
}

static void
flip_endian_32(uint8_t *d) {
    assert(NULL != d);

    swap_char(d, d + 3);
    swap_char(d + 1, d + 2);
}

static void
flip_endian_64(uint8_t *d) {
    assert(NULL != d);

    swap_char(d + 7, d);
    swap_char(d + 6, d + 1);
    swap_char(d + 5, d + 2);
    swap_char(d + 4, d + 3);
}

uint8_t
rt_util_clamp_to_1BB(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_1BBMAX);
}

uint8_t
rt_util_clamp_to_2BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_2BUIMAX);
}

uint8_t
rt_util_clamp_to_4BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_4BUIMAX);
}

int8_t
rt_util_clamp_to_8BSI(double value) {
    return (int8_t)fmin(fmax((value), SCHAR_MIN), SCHAR_MAX);
}

uint8_t
rt_util_clamp_to_8BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), UCHAR_MAX);
}

int16_t
rt_util_clamp_to_16BSI(double value) {
    return (int16_t)fmin(fmax((value), SHRT_MIN), SHRT_MAX);
}

uint16_t
rt_util_clamp_to_16BUI(double value) {
    return (uint16_t)fmin(fmax((value), 0), USHRT_MAX);
}

int32_t
rt_util_clamp_to_32BSI(double value) {
    return (int32_t)fmin(fmax((value), INT_MIN), INT_MAX);
}

uint32_t
rt_util_clamp_to_32BUI(double value) {
    return (uint32_t)fmin(fmax((value), 0), UINT_MAX);
}

float
rt_util_clamp_to_32F(double value) {
    return (float)fmin(fmax((value), -FLT_MAX), FLT_MAX);
}

/* quicksort */
#define SWAP(x, y) { double t; t = x; x = y; y = t; }
#define ORDER(x, y) if (x > y) SWAP(x, y)

static double pivot(double *left, double *right) {
	double l, m, r, *p;

	l = *left;
	m = *(left + (right - left) / 2);
	r = *right;

	/* order */
	ORDER(l, m);
	ORDER(l, r);
	ORDER(m, r);

	/* pivot is higher of two values */
	if (l < m) return m;
	if (m < r) return r;

	/* find pivot that isn't left */
	for (p = left + 1; p <= right; ++p) {
		if (*p != *left)
			return (*p < *left) ? *left : *p;
	}

	/* all values are same */
	return -1;
}

static double *partition(double *left, double *right, double pivot) {
	while (left <= right) {
		while (*left < pivot) ++left;
		while (*right >= pivot) --right;

		if (left < right) {
			SWAP(*left, *right);
			++left;
			--right;
		}
	}

	return left;
}

static void quicksort(double *left, double *right) {
	double p = pivot(left, right);
	double *pos;

	if (p != -1) {
		pos = partition(left, right, p);
		quicksort(left, pos - 1);
		quicksort(pos, right);
	}
}

/*- rt_context -------------------------------------------------------*/

/* Functions definitions */
void * init_rt_allocator(size_t size);
void * init_rt_reallocator(void * mem, size_t size);
void init_rt_deallocator(void * mem);
void init_rt_errorreporter(const char * fmt, va_list ap);
void init_rt_warnreporter(const char * fmt, va_list ap);
void init_rt_inforeporter(const char * fmt, va_list ap);



/*
 * Default allocators
 *
 * We include some default allocators that use malloc/free/realloc
 * along with stdout/stderr since this is the most common use case
 *
 */
void *
default_rt_allocator(size_t size)
{
    void *mem = malloc(size);
    return mem;
}

void *
default_rt_reallocator(void *mem, size_t size)
{
    void *ret = realloc(mem, size);
    return ret;
}

void
default_rt_deallocator(void *mem)
{
    free(mem);
}


void
default_rt_error_handler(const char *fmt, va_list ap) {

    static const char *label = "ERROR: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}

void
default_rt_warning_handler(const char *fmt, va_list ap) {

    static const char *label = "WARNING: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}


void
default_rt_info_handler(const char *fmt, va_list ap) {

    static const char *label = "INFO: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}


/**
 * Struct definition here
 */
struct rt_context_t {
    rt_allocator alloc;
    rt_reallocator realloc;
    rt_deallocator dealloc;
    rt_message_handler err;
    rt_message_handler warn;
    rt_message_handler info;
};

/* Static variable, to be used for all rt_core functions */
static struct rt_context_t ctx_t = {
    .alloc = init_rt_allocator,
    .realloc = init_rt_reallocator,
    .dealloc = init_rt_deallocator,
    .err = init_rt_errorreporter,
    .warn = init_rt_warnreporter,
    .info = init_rt_inforeporter
};


/**
 * This function is normally called by rt_init_allocators when no special memory
 * management is needed. Useful in raster core testing and in the (future)
 * loader, when we need to use raster core functions but we don't have
 * PostgreSQL backend behind. We must take care of memory by ourselves in those
 * situations
 */
void
rt_install_default_allocators(void)
{
    ctx_t.alloc = default_rt_allocator;
    ctx_t.realloc = default_rt_reallocator;
    ctx_t.dealloc = default_rt_deallocator;
    ctx_t.err = default_rt_error_handler;
    ctx_t.info = default_rt_info_handler;
    ctx_t.warn = default_rt_warning_handler;
}


/**
 * This function is called by rt_init_allocators when the PostgreSQL backend is
 * taking care of the memory and we want to use palloc family
 */
void
rt_set_handlers(rt_allocator allocator, rt_reallocator reallocator,
        rt_deallocator deallocator, rt_message_handler error_handler,
        rt_message_handler info_handler, rt_message_handler warning_handler) {

    ctx_t.alloc = allocator;
    ctx_t.realloc = reallocator;
    ctx_t.dealloc = deallocator;

    ctx_t.err = error_handler;
    ctx_t.info = info_handler;
    ctx_t.warn = warning_handler;
}



/**
 * Initialisation allocators
 *
 * These are used the first time any of the allocators are called to enable
 * executables/libraries that link into raster to be able to set up their own
 * allocators. This is mainly useful for older PostgreSQL versions that don't
 * have functions that are called upon startup.
 **/
void *
init_rt_allocator(size_t size)
{
    rt_init_allocators();

    return ctx_t.alloc(size);
}

void
init_rt_deallocator(void *mem)
{
    rt_init_allocators();

    ctx_t.dealloc(mem);
}


void *
init_rt_reallocator(void *mem, size_t size)
{
    rt_init_allocators();

    return ctx_t.realloc(mem, size);
}

void
init_rt_inforeporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.info)(fmt, ap);
}

void
init_rt_warnreporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.warn)(fmt, ap);
}


void
init_rt_errorreporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.err)(fmt, ap);

}



/**
 * Raster core memory management functions.
 *
 * They use the functions defined by the caller.
 */
void *
rtalloc(size_t size) {
    void * mem = ctx_t.alloc(size);
    RASTER_DEBUGF(5, "rtalloc called: %d@%p", size, mem);
    return mem;
}


void *
rtrealloc(void * mem, size_t size) {
    void * result = ctx_t.realloc(mem, size);
    RASTER_DEBUGF(5, "rtrealloc called: %d@%p", size, result);
    return result;
}

void
rtdealloc(void * mem) {
    ctx_t.dealloc(mem);
    RASTER_DEBUG(5, "rtdealloc called");
}

/**
 * Raster core error and info handlers
 *
 * Since variadic functions cannot pass their parameters directly, we need
 * wrappers for these functions to convert the arguments into a va_list
 * structure.
 */
void
rterror(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.err)(fmt, ap);

    va_end(ap);
}

void
rtinfo(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.info)(fmt, ap);

    va_end(ap);
}


void
rtwarn(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.warn)(fmt, ap);

    va_end(ap);
}



int
rt_util_display_dbl_trunc_warning(double initialvalue,
                                  int32_t checkvalint,
                                  uint32_t checkvaluint,
                                  float checkvalfloat,
                                  double checkvaldouble,
                                  rt_pixtype pixtype) {
    int result = 0;



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
            if (fabs(checkvalint - initialvalue) >= 1) {
                rtwarn("Value set for %s band got clamped from %f to %d",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvalint);
                result = -1;
            }
            else if (fabs(checkvalint - initialvalue) > FLT_EPSILON) {
                rtwarn("Value set for %s band got truncated from %f to %d",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvalint);
                result = -1;
            }
            break;
        }
        case PT_32BUI:
        {
            if (fabs(checkvaluint - initialvalue) >= 1) {
                rtwarn("Value set for %s band got clamped from %f to %u",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvaluint);
                result = -1;
            }
            else if (fabs(checkvaluint - initialvalue) > FLT_EPSILON) {
                rtwarn("Value set for %s band got truncated from %f to %u",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvaluint);
                result = -1;
            }
            break;
        }
        case PT_32BF:
        {
            /* For float, because the initial value is a double,
            there is very often a difference between the desired value and the obtained one */
            if (fabs(checkvalfloat - initialvalue) > FLT_EPSILON)
                rtwarn("Value set for %s band got converted from %f to %f",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvalfloat);
            break;
        }
        case PT_64BF:
        {
            if (fabs(checkvaldouble - initialvalue) > FLT_EPSILON)
                rtwarn("Value set for %s band got converted from %f to %f",
                    rt_pixtype_name(pixtype),
                    initialvalue, checkvaldouble);
            break;
        }
        case PT_END:
            break;
    }
    return result;
}

/*--- Debug and Testing Utilities --------------------------------------------*/

#if POSTGIS_DEBUG_LEVEL > 3

static char*
d_binary_to_hex(const uint8_t * const raw, uint32_t size, uint32_t *hexsize) {
    char* hex = NULL;
    uint32_t i = 0;


    assert(NULL != raw);
    assert(NULL != hexsize);


    *hexsize = size * 2; /* hex is 2 times bytes */
    hex = (char*) rtalloc((*hexsize) + 1);
    if (!hex) {
        rterror("d_binary_to_hex: Out of memory hexifying raw binary\n");
        return NULL;
    }
    hex[*hexsize] = '\0'; /* Null-terminate */

    for (i = 0; i < size; ++i) {
        deparse_hex(raw[i], &(hex[2 * i]));
    }

    assert(NULL != hex);
    assert(0 == strlen(hex) % 2);
    return hex;
}

static void
d_print_binary_hex(const char* msg, const uint8_t * const raw, uint32_t size) {
    char* hex = NULL;
    uint32_t hexsize = 0;


    assert(NULL != msg);
    assert(NULL != raw);


    hex = d_binary_to_hex(raw, size, &hexsize);
    if (NULL != hex) {
        rtinfo("%s\t%s", msg, hex);
        rtdealloc(hex);
    }
}

static size_t
d_binptr_to_pos(const uint8_t * const ptr, const uint8_t * const end, size_t size) {
    assert(NULL != ptr && NULL != end);

    return (size - (end - ptr));
}

#define CHECK_BINPTR_POSITION(ptr, end, size, pos) \
    { if (pos != d_binptr_to_pos(ptr, end, size)) { \
    fprintf(stderr, "Check of binary stream pointer position failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %lu, expected = %lu\n", (long unsigned)d_binptr_to_pos(ptr, end, size), (long unsigned)pos); \
    } }

#else

#define CHECK_BINPTR_POSITION(ptr, end, size, pos) ((void)0);

#endif /* ifndef POSTGIS_DEBUG_LEVEL > 3 */

/*- rt_pixeltype -----------------------------------------------------*/

int
rt_pixtype_size(rt_pixtype pixtype) {
    int pixbytes = -1;



    switch (pixtype) {
        case PT_1BB:
        case PT_2BUI:
        case PT_4BUI:
        case PT_8BSI:
        case PT_8BUI:
            pixbytes = 1;
            break;
        case PT_16BSI:
        case PT_16BUI:
            pixbytes = 2;
            break;
        case PT_32BSI:
        case PT_32BUI:
        case PT_32BF:
            pixbytes = 4;
            break;
        case PT_64BF:
            pixbytes = 8;
            break;
        default:
            rterror("rt_pixtype_size: Unknown pixeltype %d", pixtype);
            pixbytes = -1;
            break;
    }

    RASTER_DEBUGF(3, "Pixel type = %s and size = %d bytes",
            rt_pixtype_name(pixtype), pixbytes);


    return pixbytes;
}

int
rt_pixtype_alignment(rt_pixtype pixtype) {

    return rt_pixtype_size(pixtype);
}

rt_pixtype
rt_pixtype_index_from_name(const char* pixname) {
    assert(pixname && strlen(pixname) > 0);


    if (strcmp(pixname, "1BB") == 0)
        return PT_1BB;
    if (strcmp(pixname, "2BUI") == 0)
        return PT_2BUI;
    if (strcmp(pixname, "4BUI") == 0)
        return PT_4BUI;
    if (strcmp(pixname, "8BSI") == 0)
        return PT_8BSI;
    if (strcmp(pixname, "8BUI") == 0)
        return PT_8BUI;
    if (strcmp(pixname, "16BSI") == 0)
        return PT_16BSI;
    if (strcmp(pixname, "16BUI") == 0)
        return PT_16BUI;
    if (strcmp(pixname, "32BSI") == 0)
        return PT_32BSI;
    if (strcmp(pixname, "32BUI") == 0)
        return PT_32BUI;
    if (strcmp(pixname, "32BF") == 0)
        return PT_32BF;
    if (strcmp(pixname, "64BF") == 0)
        return PT_64BF;
    return PT_END;
}

const char*
rt_pixtype_name(rt_pixtype pixtype) {



    switch (pixtype) {
        case PT_1BB:
            return "1BB";
        case PT_2BUI:
            return "2BUI";
        case PT_4BUI:
            return "4BUI";
        case PT_8BSI:
            return "8BSI";
        case PT_8BUI:
            return "8BUI";
        case PT_16BSI:
            return "16BSI";
        case PT_16BUI:
            return "16BUI";
        case PT_32BSI:
            return "32BSI";
        case PT_32BUI:
            return "32BUI";
        case PT_32BF:
            return "32BF";
        case PT_64BF:
            return "64BF";
        default:
            rterror("rt_pixtype_name: Unknown pixeltype %d", pixtype);
            return "Unknown";
    }
}

/*- rt_band ----------------------------------------------------------*/

struct rt_extband_t {
    uint8_t bandNum;
    char* path; /* externally owned ? */
};

struct rt_band_t {
    rt_pixtype pixtype;
    int32_t offline;
    uint16_t width;
    uint16_t height;
    int32_t hasnodata; /* a flag indicating if this band contains nodata values */
    int32_t isnodata;   /* a flag indicating if this band is filled only with
                           nodata values */
    double nodataval; /* int will be converted ... */
    int32_t ownsData; /* XXX mloskot: its behaviour needs to be documented */

    union {
        void* mem; /* actual data, externally owned */
        struct rt_extband_t offline;
    } data;

};

rt_band
rt_band_new_inline(uint16_t width, uint16_t height,
        rt_pixtype pixtype, uint32_t hasnodata, double nodataval,
        uint8_t* data) {
    rt_band band = NULL;


    assert(NULL != data);

    band = rtalloc(sizeof (struct rt_band_t));
    if (!band) {
        rterror("rt_band_new_inline: Out of memory allocating rt_band");
        return 0;
    }

    RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s",
            band, rt_pixtype_name(pixtype));


    band->pixtype = pixtype;
    band->offline = 0;
    band->width = width;
    band->height = height;
    band->hasnodata = hasnodata;
    band->nodataval = nodataval;
    band->data.mem = data;
    band->ownsData = 0;
    band->isnodata = FALSE;

    return band;
}

rt_band
rt_band_new_offline(uint16_t width, uint16_t height,
        rt_pixtype pixtype, uint32_t hasnodata, double nodataval,
        uint8_t bandNum, const char* path) {
    rt_band band = NULL;


    assert(NULL != path);

    band = rtalloc(sizeof (struct rt_band_t));
    if (!band) {
        rterror("rt_band_new_offline: Out of memory allocating rt_band");
        return 0;
    }


    RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s",
            band, rt_pixtype_name(pixtype));

    band->pixtype = pixtype;
    band->offline = 1;
    band->width = width;
    band->height = height;
    band->hasnodata = hasnodata;
    band->nodataval = nodataval;
    band->data.offline.bandNum = bandNum;

    /* memory for data.offline.path should be managed externally */
    band->data.offline.path = (char *) path;

    /* XXX QUESTION (jorgearevalo): What does exactly ownsData mean?? I think that
     * ownsData = 0 ==> the memory for band->data is externally owned
     * ownsData = 1 ==> the memory for band->data is internally owned
     */
    band->ownsData = 0;

    band->isnodata = FALSE;

    return band;
}

int
rt_band_is_offline(rt_band band) {

    assert(NULL != band);


    return band->offline;
}

void
rt_band_destroy(rt_band band) {


    RASTER_DEBUGF(3, "Destroying rt_band @ %p", band);

    /* band->data content is externally owned */
    /* XXX jorgearevalo: not really... rt_band_from_wkb allocates memory for
     * data.mem
     */
    rtdealloc(band);
}

const char*
rt_band_get_ext_path(rt_band band) {

    assert(NULL != band);


    if (!band->offline) {
        RASTER_DEBUG(3, "rt_band_get_ext_path: non-offline band doesn't have "
                "an associated path");
        return 0;
    }
    return band->data.offline.path;
}

uint8_t
rt_band_get_ext_band_num(rt_band band) {

    assert(NULL != band);


    if (!band->offline) {
        RASTER_DEBUG(3, "rt_band_get_ext_path: non-offline band doesn't have "
                "an associated band number");
        return 0;
    }
    return band->data.offline.bandNum;
}

void *
rt_band_get_data(rt_band band) {

    assert(NULL != band);


    if (band->offline) {
        RASTER_DEBUG(3, "rt_band_get_data: "
                "offline band doesn't have associated data");
        return 0;
    }
    return band->data.mem;
}

rt_pixtype
rt_band_get_pixtype(rt_band band) {

    assert(NULL != band);


    return band->pixtype;
}

uint16_t
rt_band_get_width(rt_band band) {

    assert(NULL != band);


    return band->width;
}

uint16_t
rt_band_get_height(rt_band band) {

    assert(NULL != band);


    return band->height;
}

#ifdef OPTIMIZE_SPACE

/*
 * Set given number of bits of the given byte,
 * starting from given bitOffset (from the first)
 * to the given value.
 *
 * Examples:
 *   char ch;
 *   ch=0; setBits(&ch, 1, 1, 0) -> ch==8
 *   ch=0; setBits(&ch, 3, 2, 1) -> ch==96 (0x60)
 *
 * Note that number of bits set must be <= 8-bitOffset
 *
 */
static void
setBits(char* ch, double val, int bits, int bitOffset) {
    char mask = 0xFF >> (8 - bits);
    char ival = val;


    assert(8 - bitOffset >= bits);

    RASTER_DEBUGF(4, "ival:%d bits:%d mask:%hhx bitoffset:%d\n",
            ival, bits, mask, bitOffset);

    /* clear all but significant bits from ival */
    ival &= mask;
#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    if (ival != val) {
        rtwarn("Pixel value for %d-bits band got truncated"
                " from %g to %hhu\n", bits, val, ival);
    }
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    RASTER_DEBUGF(4, " cleared ival:%hhx\n", ival);


    /* Shift ival so the significant bits start at
     * the first bit */
    ival <<= (8 - bitOffset - bits);

    RASTER_DEBUGF(4, " ival shifted:%hhx\n", ival);
    RASTER_DEBUGF(4, "  ch:%hhx\n", *ch);

    /* clear first bits of target */
    *ch &= ~(mask << (8 - bits - bitOffset));

    RASTER_DEBUGF(4, "  ch cleared:%hhx\n", *ch);

    /* Set the first bit of target */
    *ch |= ival;

    RASTER_DEBUGF(4, "  ch ored:%hhx\n", *ch);

}
#endif /* OPTIMIZE_SPACE */

int
rt_band_get_hasnodata_flag(rt_band band) {

    assert(NULL != band);


    return band->hasnodata;
}

void
rt_band_set_hasnodata_flag(rt_band band, int flag) {

    assert(NULL != band);


    band->hasnodata = (flag) ? 1 : 0;
}

void
rt_band_set_isnodata_flag(rt_band band, int flag) {

    assert(NULL != band);


    band->isnodata = (flag) ? 1 : 0;
}

int
rt_band_get_isnodata_flag(rt_band band) {

    assert(NULL != band);

    return band->isnodata;
}

int
rt_band_set_nodata(rt_band band, double val) {
    rt_pixtype pixtype = PT_END;
    //double oldnodataval = band->nodataval;

    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    float checkvalfloat = 0;
    double checkvaldouble = 0;



    assert(NULL != band);

    pixtype = band->pixtype;

    RASTER_DEBUGF(3, "rt_band_set_nodata: setting nodata value %g with band type %s", val, rt_pixtype_name(pixtype));

    /* return -1 on out of range */
    switch (pixtype) {
        case PT_1BB:
        {
            band->nodataval = rt_util_clamp_to_1BB(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_2BUI:
        {
            band->nodataval = rt_util_clamp_to_2BUI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_4BUI:
        {
            band->nodataval = rt_util_clamp_to_4BUI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_8BSI:
        {
            band->nodataval = rt_util_clamp_to_8BSI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_8BUI:
        {
            band->nodataval = rt_util_clamp_to_8BUI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_16BSI:
        {
            band->nodataval = rt_util_clamp_to_16BSI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_16BUI:
        {
            band->nodataval = rt_util_clamp_to_16BUI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_32BSI:
        {
            band->nodataval = rt_util_clamp_to_32BSI(val);
            checkvalint = band->nodataval;
            break;
        }
        case PT_32BUI:
        {
            band->nodataval = rt_util_clamp_to_32BUI(val);
            checkvaluint = band->nodataval;
            break;
        }
        case PT_32BF:
        {
            band->nodataval = rt_util_clamp_to_32F(val);
            checkvalfloat = band->nodataval;
            break;
        }
        case PT_64BF:
        {
            band->nodataval = val;
            checkvaldouble = band->nodataval;
            break;
        }
        default:
            {
            rterror("rt_band_set_nodata: Unknown pixeltype %d", pixtype);
            band->hasnodata = 0;
            return -1;
        }
    }

    RASTER_DEBUGF(3, "rt_band_set_nodata: band->hasnodata = %d", band->hasnodata);
    RASTER_DEBUGF(3, "rt_band_set_nodata: band->nodataval = %d", band->nodataval);


    // the nodata value was just set, so this band has NODATA
    rt_band_set_hasnodata_flag(band, 1);

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    if (rt_util_display_dbl_trunc_warning(val, checkvalint, checkvaluint, checkvalfloat,
                                      checkvaldouble, pixtype))
        return -1;
#endif

    /* If the nodata value is different from the previous one, we need to check
     * again if the band is a nodata band
     * TODO: NO, THAT'S TOO SLOW!!!
     */

    /*
    if (fabs(band->nodataval - oldnodataval) > FLT_EPSILON)
        rt_band_check_is_nodata(band);
    */

    return 0;
}

int
rt_band_set_pixel(rt_band band, uint16_t x, uint16_t y,
        double val) {
    rt_pixtype pixtype = PT_END;
    unsigned char* data = NULL;
    uint32_t offset = 0;

    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    float checkvalfloat = 0;
    double checkvaldouble = 0;

    double checkval = 0;



    assert(NULL != band);

    pixtype = band->pixtype;

    if (x >= band->width || y >= band->height) {
        rterror("rt_band_set_pixel: Coordinates out of range");
        return -1;
    }

    if (band->offline) {
        rterror("rt_band_set_pixel not implemented yet for OFFDB bands");
        return -1;
    }

    data = rt_band_get_data(band);
    offset = x + (y * band->width);

    switch (pixtype) {
        case PT_1BB:
        {
            data[offset] = rt_util_clamp_to_1BB(val);
            checkvalint = data[offset];
            break;
        }
        case PT_2BUI:
        {
            data[offset] = rt_util_clamp_to_2BUI(val);
            checkvalint = data[offset];
            break;
        }
        case PT_4BUI:
        {
            data[offset] = rt_util_clamp_to_4BUI(val);
            checkvalint = data[offset];
            break;
        }
        case PT_8BSI:
        {
            data[offset] = rt_util_clamp_to_8BSI(val);
            checkvalint = (int8_t) data[offset];
            break;
        }
        case PT_8BUI:
        {
            data[offset] = rt_util_clamp_to_8BUI(val);
            checkvalint = data[offset];
            break;
        }
        case PT_16BSI:
        {
            int16_t *ptr = (int16_t*) data; /* we assume correct alignment */
            ptr[offset] = rt_util_clamp_to_16BSI(val);
            checkvalint = (int16_t) ptr[offset];
            break;
        }
        case PT_16BUI:
        {
            uint16_t *ptr = (uint16_t*) data; /* we assume correct alignment */
            ptr[offset] = rt_util_clamp_to_16BUI(val);
            checkvalint = ptr[offset];
            break;
        }
        case PT_32BSI:
        {
            int32_t *ptr = (int32_t*) data; /* we assume correct alignment */
            ptr[offset] = rt_util_clamp_to_32BSI(val);
            checkvalint = (int32_t) ptr[offset];
            break;
        }
        case PT_32BUI:
        {
            uint32_t *ptr = (uint32_t*) data; /* we assume correct alignment */
            ptr[offset] = rt_util_clamp_to_32BUI(val);
            checkvaluint = ptr[offset];
            break;
        }
        case PT_32BF:
        {
            float *ptr = (float*) data; /* we assume correct alignment */
            ptr[offset] = rt_util_clamp_to_32F(val);
            checkvalfloat = ptr[offset];
            break;
        }
        case PT_64BF:
        {
            double *ptr = (double*) data; /* we assume correct alignment */
            ptr[offset] = val;
            checkvaldouble = ptr[offset];
            break;
        }
        default:
        {
            rterror("rt_band_set_pixel: Unknown pixeltype %d", pixtype);
            return -1;
        }
    }

    /* Overflow checking */
#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    if (rt_util_display_dbl_trunc_warning(val, checkvalint, checkvaluint, checkvalfloat,
                                      checkvaldouble, pixtype))
       return -1;
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    /* If the stored value is different from no data, reset the isnodata flag */
    if (fabs(checkval - band->nodataval) > FLT_EPSILON) {
        band->isnodata = FALSE;
    }

    /*
     * If the pixel was a nodata value, now the band may be NODATA band)
     * TODO: NO, THAT'S TOO SLOW!!!
     */

    /*
    else {
        rt_band_check_is_nodata(band);
    }
    */


    return 0;
}

int
rt_band_get_pixel(rt_band band, uint16_t x, uint16_t y, double *result) {
    rt_pixtype pixtype = PT_END;
    uint8_t* data = NULL;
    uint32_t offset = 0;



    assert(NULL != band);

    pixtype = band->pixtype;

    if (x >= band->width || y >= band->height) {
        rterror("Attempting to get pixel value with out of range raster coordinates");
        return -1;
    }

    if (band->offline) {
        rterror("rt_band_get_pixel not implemented yet for OFFDB bands");
        return -1;
    }

    data = rt_band_get_data(band);
    offset = x + (y * band->width); /* +1 for the nodata value */

    switch (pixtype) {
        case PT_1BB:
#ifdef OPTIMIZE_SPACE
        {
            int byteOffset = offset / 8;
            int bitOffset = offset % 8;
            data += byteOffset;

            /* Bit to set is bitOffset into data */
            *result = getBits(data, val, 1, bitOffset);
            return 0;
        }
#endif

        case PT_2BUI:
#ifdef OPTIMIZE_SPACE
        {
            int byteOffset = offset / 4;
            int bitOffset = offset % 4;
            data += byteOffset;

            /* Bits to set start at bitOffset into data */
            *result = getBits(data, val, 2, bitOffset);
            return 0;
        }
#endif

        case PT_4BUI:
#ifdef OPTIMIZE_SPACE
        {
            int byteOffset = offset / 2;
            int bitOffset = offset % 2;
            data += byteOffset;

            /* Bits to set start at bitOffset into data */
            *result = getBits(data, val, 2, bitOffset);
            return 0;
        }
#endif

        case PT_8BSI:
        {
            int8_t val = data[offset];
            *result = val;
            return 0;
        }
        case PT_8BUI:
        {
            uint8_t val = data[offset];
            *result = val;
            return 0;
        }
        case PT_16BSI:
        {
            int16_t *ptr = (int16_t*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        case PT_16BUI:
        {
            uint16_t *ptr = (uint16_t*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        case PT_32BSI:
        {
            int32_t *ptr = (int32_t*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        case PT_32BUI:
        {
            uint32_t *ptr = (uint32_t*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        case PT_32BF:
        {
            float *ptr = (float*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        case PT_64BF:
        {
            double *ptr = (double*) data; /* we assume correct alignment */
            *result = ptr[offset];
            return 0;
        }
        default:
        {
            rterror("rt_band_get_pixel: Unknown pixeltype %d", pixtype);
            return -1;
        }
    }
}

double
rt_band_get_nodata(rt_band band) {


    assert(NULL != band);

    if (!band->hasnodata)
        RASTER_DEBUGF(3, "Getting nodata value for a band without NODATA values. Using %g", band->nodataval);


    return band->nodataval;
}

double
rt_band_get_min_value(rt_band band) {
    rt_pixtype pixtype = PT_END;



    assert(NULL != band);

    pixtype = band->pixtype;

    switch (pixtype) {
        case PT_1BB:
        case PT_2BUI:
        case PT_4BUI:
        case PT_8BUI:
        {
            return (double)CHAR_MIN;
        }
        case PT_8BSI:
        {
            return (double)SCHAR_MIN;
        }
        case PT_16BSI:
        case PT_16BUI:
        {
            return (double)SHRT_MIN;
        }
        case PT_32BSI:
        case PT_32BUI:
        {
            return (double)INT_MIN;
        }
        case PT_32BF:
        {
            return (double)-FLT_MAX;
        }
        case PT_64BF:
        {
            return (double)-DBL_MAX;
        }
        default:
        {
            rterror("rt_band_get_min_value: Unknown pixeltype %d", pixtype);
            return (double)CHAR_MIN;
        }
    }
}


int
rt_band_check_is_nodata(rt_band band)
{
    int i, j;
    double pxValue = band->nodataval;
    double dEpsilon = 0.0;



    assert(NULL != band);

    /* Check if band has nodata value */
    if (!band->hasnodata)
    {
        RASTER_DEBUG(3, "Unknown nodata value for band");
        band->isnodata = FALSE;
        return FALSE;
    }

    /* TODO: How to know it in case of offline bands? */
    if (band->offline) {
        RASTER_DEBUG(3, "Unknown nodata value for OFFDB band");
        band->isnodata = FALSE;
    }

    /* Check all pixels */
    for(i = 0; i < band->width; i++)
    {
        for(j = 0; j < band->height; j++)
        {
            rt_band_get_pixel(band, i, j, &pxValue);
            dEpsilon = fabs(pxValue - band->nodataval);
            if (dEpsilon > FLT_EPSILON) {
                band->isnodata = FALSE;
                return FALSE;
            }
        }
    }

    band->isnodata = TRUE;
    return TRUE;
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
	int sorted; /* flag indicating that values is sorted ascending by value */
};

/**
 * Compute summary statistics for a band
 *
 * @param band: the band to query for summary stats
 * @param exclude_nodata_value: if non-zero, ignore nodata values
 * @param sample: percentage of pixels to sample
 * @param inc_vals: flag to include values in return struct
 *
 * @return the summary statistics for a band
 */
rt_bandstats
rt_band_get_summary_stats(rt_band band, int exclude_nodata_value, double sample,
	int inc_vals) {
	uint8_t *data = NULL;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t z = 0;
	uint32_t offset = 0;
	uint32_t diff = 0;
	int rtn;
	int hasnodata = FALSE;
	double nodata = 0;
	double *values = NULL;
	double value;
	rt_bandstats stats = NULL;

	uint32_t do_sample = 0;
	uint32_t sample_size = 0;
	int byY = 0;
	uint32_t outer = 0;
	uint32_t inner = 0;
	uint32_t sample_per = 0;
	uint32_t sample_int = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	double sum = 0;
	uint32_t k = 0;
	double M = 0;
	double Q = 0;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != band);

	if (band->offline) {
		rterror("rt_band_get_summary_stats not implemented yet for OFFDB bands");
		return NULL;
	}

	data = rt_band_get_data(band);

	hasnodata = rt_band_get_hasnodata_flag(band);
	if (hasnodata != FALSE)
		nodata = rt_band_get_nodata(band);
	else
		exclude_nodata_value = 0;

	RASTER_DEBUGF(3, "nodata = %f", nodata);
	RASTER_DEBUGF(3, "hasnodata = %d", hasnodata);
	RASTER_DEBUGF(3, "exclude_nodata_value = %d", exclude_nodata_value);

	/* entire band is nodata */
	if (rt_band_get_isnodata_flag(band) != FALSE) {
		if (exclude_nodata_value) {
			rtwarn("All pixels of band have the NODATA value");
			return NULL;
		}
		else {
			stats = (rt_bandstats) rtalloc(sizeof(struct rt_bandstats_t));
			if (NULL == stats) {
				rterror("rt_band_get_summary_stats: Unable to allocate memory for stats");
				return NULL;
			}

			stats->sample = 1;
			stats->count = band->width * band->height;
			stats->min = stats->max = nodata;
			stats->sum = stats->count * nodata;
			stats->mean = nodata;
			stats->stddev = 0;
			stats->values = NULL;
			stats->sorted = 0;

			return stats;
		}
	}

	if (band->height > band->width) {
		byY = 1;
		outer = band->height;
		inner = band->width;
	}
	else {
		byY = 0;
		outer = band->width;
		inner = band->height;
	}

	/* clamp percentage */
	if (
		(sample < 0 || fabs(sample - 0.0) < FLT_EPSILON) ||
		(sample > 1 || fabs(sample - 1.0) < FLT_EPSILON)
	) {
		do_sample = 0;
		sample = 1;
	}
	else
		do_sample = 1;
	RASTER_DEBUGF(3, "do_sample = %d", do_sample);

	/* sample all pixels */
	if (do_sample != 1) {
		sample_size = band->width * band->height;
		sample_per = inner;
	}
	/*
	 randomly sample a percentage of available pixels
	 sampling method is known as
	 	"systematic random sample without replacement"
	*/
	else {
		sample_size = round((band->width * band->height) * sample);
		sample_per = round(sample_size / outer);
		sample_int = round(inner / sample_per);
		srand(time(NULL));
	}

	RASTER_DEBUGF(3, "sampling %d of %d available pixels w/ %d per set"
		, sample_size, (band->width * band->height), sample_per); 

	if (inc_vals) {
		values = rtalloc(sizeof(double) * sample_size);
		if (NULL == values) {
			rtwarn("Unable to allocate memory for values");
			inc_vals = 0;
		}
	}

	for (x = 0, j = 0, k = 0; x < outer; x++) {
		y = -1;
		diff = 0;

		for (i = 0, z = 0; i < sample_per; i++) {
			if (do_sample != 1)
				y = i;
			else {
				offset = (rand() % sample_int) + 1;
				y += diff + offset;
				diff = sample_int - offset;
			}
			RASTER_DEBUGF(5, "(x, y, z) = (%d, %d, %d)", (byY ? y : x), (byY ? x : y), z);
			if (y >= inner || z > sample_per) break;

			if (byY)
				rtn = rt_band_get_pixel(band, y, x, &value);
			else
				rtn = rt_band_get_pixel(band, x, y, &value);

			j++;
			if (rtn != -1) {
				if (
					!exclude_nodata_value || (
						exclude_nodata_value &&
						(hasnodata != FALSE) &&
						(fabs(value - nodata) > FLT_EPSILON)
					)
				) {

					/* inc_vals set, collect pixel values */
					if (inc_vals) values[k] = value;

					/* average */
					k++;
					sum += value;

					/*
						one-pass standard deviation
						http://www.eecs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
					*/
					if (k == 1) {
						Q = 0;
						M = value;
					}
					else {
						Q = Q + (((k  - 1) * pow(value - M, 2)) / k);
						M = M + ((value - M ) / k);
					}

					/* min/max */
					if (NULL == stats) {
						stats = (rt_bandstats) rtalloc(sizeof(struct rt_bandstats_t));
						if (NULL == stats) {
							rterror("rt_band_get_summary_stats: Unable to allocate memory for stats");
							return NULL;
						}

						stats->sample = sample;
						stats->count = 0;

						stats->sum = 0;
						stats->mean = 0;
						stats->stddev = -1;
						stats->min = stats->max = value;

						stats->values = NULL;
						stats->sorted = 0;

					}
					else {
						if (value < stats->min)
							stats->min = value;
						if (value > stats->max)
							stats->max = value;
					}

				}
			}

			z++;
		}
	}

	RASTER_DEBUG(3, "sampling complete"); 

	if (k > 0) {
		if (inc_vals) {
			/* free unused memory */
			if (sample_size != k) {
				rtrealloc(values, k * sizeof(double));
			}

			stats->values = values;
		}

		stats->count = k;
		stats->sum = sum;
		stats->mean = sum / k;

		/* standard deviation */
		if (do_sample != 1)
			stats->stddev = sqrt(Q / k);
		/* sample deviation */
		else {
			if (k < 2)
				stats->stddev = -1;
			else
				stats->stddev = sqrt(Q / (k - 1));
		}
	}
	/* inc_vals thus values allocated but not used */
	else if (inc_vals) {
		rtdealloc(values);
	}

#if POSTGIS_DEBUG_LEVEL > 0
	if (NULL != stats) {
		stop = clock();
		elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
		RASTER_DEBUGF(3, "(time, count, mean, stddev, min, max) = (%0.4f, %d, %f, %f, %f, %f)",
			elapsed, stats->count, stats->mean, stats->stddev, stats->min, stats->max);
	}
#endif

	RASTER_DEBUG(3, "done");
	return stats;
}

struct rt_histogram_t {
	uint32_t count;
	double percent;

	double min;
	double max;

	int inc_min;
	int inc_max;
};

/**
 * Count the distribution of data
 *
 * @param stats: a populated stats struct for processing
 * @param bin_count: the number of bins to group the data by
 * @param bin_width: the width of each bin as an array
 * @param bin_width_count: number of values in bin_width
 * @param right: evaluate bins by (a,b] rather than default [a,b)
 * @param rtn_count: set to the number of bins being returned
 *
 * @return the histogram of the data
 */
rt_histogram
rt_band_get_histogram(rt_bandstats stats,
	int bin_count, double *bin_width, int bin_width_count,
	int right, int *rtn_count) {
	rt_histogram bins = NULL;
	int init_width = 0;
	int i;
	int j;
	double tmp;
	double value;
	int sum = 0;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting"); 
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != stats);

	if (stats->count < 1 || NULL == stats->values) {
		rterror("rt_util_get_histogram: rt_bandstats object has no value");
		return NULL;
	}

	/* bin width must be positive numbers and not zero */
	if (NULL != bin_width && bin_width_count > 0) {
		for (i = 0; i < bin_width_count; i++) {
			if (bin_width[i] < 0 || fabs(bin_width[i] - 0.0) < FLT_EPSILON) {
				rterror("rt_util_get_histogram: bin_width element is less than or equal to zero");
				return NULL;
			}
		}
	}

	/* # of bins not provided */
	if (bin_count <= 0) {
		/*
			determine # of bins
			http://en.wikipedia.org/wiki/Histogram

			all computed bins are assumed to have equal width
		*/
		/* Square-root choice for stats->count < 30 */
		if (stats->count < 30)
			bin_count = ceil(sqrt(stats->count));
		/* Sturges' formula for stats->count >= 30 */
		else
			bin_count = ceil(log2((double) stats->count) + 1.);

		/* bin_width_count provided and bin_width has value */
		if (bin_width_count > 0 && NULL != bin_width) {
			/* user has defined something specific */
			if (bin_width_count > bin_count)
				bin_count = bin_width_count;
			else if (bin_width_count > 1) {
				tmp = 0;
				for (i = 0; i < bin_width_count; i++) tmp += bin_width[i];
				bin_count = ceil((stats->max - stats->min) / tmp) * bin_width_count;
			}
			else
				bin_count = ceil((stats->max - stats->min) / bin_width[0]);
		}
		/* set bin width count to zero so that one can be calculated */
		else {
			bin_width_count = 0;
		}
	}

	/* min and max the same */
	if (fabs(stats->max - stats->min) < FLT_EPSILON)
		bin_count = 1;

	RASTER_DEBUGF(3, "bin_count = %d", bin_count); 

	/* bin count = 1, all values are in one bin */
	if (bin_count < 2) {
		bins = rtalloc(sizeof(struct rt_histogram_t));
		if (NULL == bins) {
			rterror("rt_util_get_histogram: Unable to allocate memory for histogram");
			return NULL;
		}

		bins->count = stats->count;
		bins->percent = -1;
		bins->min = stats->min;
		bins->max = stats->max;
		bins->inc_min = bins->inc_max = 1;

		*rtn_count = bin_count;
		return bins;
	}

	/* establish bin width */
	if (bin_width_count == 0) {
		bin_width_count = 1;

		/* bin_width unallocated */
		if (NULL == bin_width) {
			bin_width = rtalloc(sizeof(double));
			if (NULL == bin_width) {
				rterror("rt_util_get_histogram: Unable to allocate memory for bin widths");
				return NULL;
			}
			init_width = 1;
		}

		bin_width[0] = (stats->max - stats->min) / bin_count;
	}

	/* initialize bins */
	bins = rtalloc(bin_count * sizeof(struct rt_histogram_t));
	if (NULL == bins) {
		rterror("rt_util_get_histogram: Unable to allocate memory for histogram");
		if (init_width) rtdealloc(bin_width);
		return NULL;
	}
	if (!right)
		tmp = stats->min;
	else
		tmp = stats->max;
	for (i = 0; i < bin_count;) {
		for (j = 0; j < bin_width_count; j++) {
			bins[i].count = 0;
			bins->percent = -1;

			if (!right) {
				bins[i].min = tmp;
				tmp += bin_width[j];
				bins[i].max = tmp;

				bins[i].inc_min = 1;
				bins[i].inc_max = 0;
			}
			else {
				bins[i].max = tmp;
				tmp -= bin_width[j];
				bins[i].min = tmp;

				bins[i].inc_min = 0;
				bins[i].inc_max = 1;
			}

			i++;
		}
	}
	if (!right) {
		bins[bin_count - 1].inc_max = 1;

		/* align last bin to the max value */
		if (bins[bin_count - 1].max < stats->max)
			bins[bin_count - 1].max = stats->max;
	}
	else {
		bins[bin_count - 1].inc_min = 1;

		/* align first bin to the min value */
		if (bins[bin_count - 1].min > stats->min)
			bins[bin_count - 1].min = stats->min;
	}

	/* process the values */
	for (i = 0; i < stats->count; i++) {
		value = stats->values[i];

		/* default, [a, b) */
		if (!right) {
			for (j = 0; j < bin_count; j++) {
				if (
					(!bins[j].inc_max && value < bins[j].max) || (
						bins[j].inc_max && (
							(value < bins[j].max) ||
							(fabs(value - bins[j].max) < FLT_EPSILON)
						)
					)
				) {
					bins[j].count++;
					sum++;
					break;
				}
			}
		}
		else {
			for (j = 0; j < bin_count; j++) {
				if (
					(!bins[j].inc_min && value > bins[j].min) || (
						bins[j].inc_min && (
							(value > bins[j].min) ||
							(fabs(value - bins[j].min) < FLT_EPSILON)
						)
					)
				) {
					bins[j].count++;
					sum++;
					break;
				}
			}
		}
	}

	/* percents */
	for (i = 0; i < bin_count; i++) {
		bins[i].percent = ((double) bins[i].count) / sum;
		if (bin_width_count > 1)
			bins[i].percent /= (bins[i].max - bins[i].min);
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);

	for (j = 0; j < bin_count; j++) {
		RASTER_DEBUGF(5, "(min, max, inc_min, inc_max, count, sum, percent) = (%f, %f, %d, %d, %d, %d, %f)",
			bins[j].min, bins[j].max, bins[j].inc_min, bins[j].inc_max, bins[j].count, sum, bins[j].percent);
	}
#endif

	if (init_width) rtdealloc(bin_width);
	*rtn_count = bin_count;
	RASTER_DEBUG(3, "done");
	return bins;
}

struct rt_quantile_t {
	double quantile;
	double value;
};

/**
 * Compute the default set of or requested quantiles for a set of data
 * the quantile formula used is same as Excel and R default method
 * 
 * @param stats: a populated stats struct for processing
 * @param quantiles: the quantiles to be computed
 * @param quantiles_count: the number of quantiles to be computed
 * @param rtn_count: set to the number of quantiles being returned
 *
 * @return the default set of or requested quantiles for a band
 */
rt_quantile
rt_band_get_quantiles(rt_bandstats stats,
	double *quantiles, int quantiles_count, int *rtn_count) {
	rt_quantile rtn;
	int init_quantiles = 0;
	int i = 0;
	double h;
	int hl;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting"); 
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != stats);

	if (stats->count < 1 || NULL == stats->values) {
		rterror("rt_util_get_quantile: rt_bandstats object has no value");
		return NULL;
	}

	/* quantiles not provided */
	if (NULL == quantiles) {
		/* quantile count not specified, default to quartiles */
		if (quantiles_count < 2)
			quantiles_count = 5;

		quantiles = rtalloc(sizeof(double) * quantiles_count);
		init_quantiles = 1;
		if (NULL == quantiles) {
			rterror("rt_util_get_quantile: Unable to allocate memory for quantile input");
			return NULL;
		}

		quantiles_count--;
		for (i = 0; i <= quantiles_count; i++)
			quantiles[i] = ((double) i) / quantiles_count;
		quantiles_count++;
	}

	/* check quantiles */
	for (i = 0; i < quantiles_count; i++) {
		if (quantiles[i] < 0. || quantiles[i] > 1.) {
			rterror("rt_util_get_quantile: Quantile value not between 0 and 1");
			if (init_quantiles) rtdealloc(quantiles);
			return NULL;
		}
	}
	quicksort(quantiles, quantiles + quantiles_count - 1);

	/* initialize rt_quantile */
	rtn = rtalloc(sizeof(struct rt_quantile_t) * quantiles_count);
	if (NULL == rtn) {
		rterror("rt_util_get_quantile: Unable to allocate memory for quantile output");
		if (init_quantiles) rtdealloc(quantiles);
		return NULL;
	}

	/* sort values */
	if (!stats->sorted) {
		quicksort(stats->values, stats->values + stats->count - 1);
		stats->sorted = 1;
	}

	/*
		make quantiles

		formula is that used in R (method 7) and Excel from
			http://en.wikipedia.org/wiki/Quantile
	*/
	for (i = 0; i < quantiles_count; i++) {
		rtn[i].quantile = quantiles[i];

		h = ((stats->count - 1.) * quantiles[i]) + 1.;
		hl = floor(h);

		/* h greater than hl, do full equation */
		if (h > hl)
			rtn[i].value = stats->values[hl - 1] + ((h - hl) * (stats->values[hl] - stats->values[hl - 1]));
		/* shortcut as second part of equation is zero */
		else
			rtn[i].value = stats->values[hl - 1];
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);
#endif

	*rtn_count = quantiles_count;
	RASTER_DEBUG(3, "done");
	return rtn;
}

/* symmetrical rounding */
#define ROUND(x, y) (((x > 0.0) ? floor((x * pow(10, y) + 0.5)) : ceil((x * pow(10, y) - 0.5))) / pow(10, y));

struct rt_valuecount_t {
	double value;
	uint32_t count;
	double percent;
};

/**
 * Count the number of times provided value(s) occur in
 * the band
 *
 * @param band: the band to query for minimum and maximum pixel values
 * @param exclude_nodata_value: if non-zero, ignore nodata values
 * @param search_values: array of values to count
 * @param search_values_count: the number of search values
 * @param roundto: the decimal place to round the values to
 * @param rtn_count: the number of value counts being returned
 *
 * @return the default set of or requested quantiles for a band
 */
rt_valuecount
rt_band_get_value_count(rt_band band, int exclude_nodata_value,
	double *search_values, uint32_t search_values_count, double roundto,
	int *rtn_count) {
	rt_valuecount vcnts = NULL;
	rt_pixtype pixtype = PT_END;
	uint8_t *data = NULL;
	int hasnodata = FALSE;
	double nodata = 0;

	int scale = 0;
	int doround = 0;
	double tmpd = 0;
	int i = 0;

	uint32_t x = 0;
	uint32_t y = 0;
	int rtn;
	double pxlval;
	double rpxlval;
	uint32_t total = 0;
	int vcnts_count = 0;
	int new_valuecount = 0;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != band);

	if (band->offline) {
		rterror("rt_band_get_count_of_values not implemented yet for OFFDB bands");
		return NULL;
	}

	data = rt_band_get_data(band);
	pixtype = band->pixtype;

	hasnodata = rt_band_get_hasnodata_flag(band);
	if (hasnodata != FALSE)
		nodata = rt_band_get_nodata(band);
	else
		exclude_nodata_value = 0;

	RASTER_DEBUGF(3, "nodata = %f", nodata);
	RASTER_DEBUGF(3, "hasnodata = %d", hasnodata);
	RASTER_DEBUGF(3, "exclude_nodata_value = %d", exclude_nodata_value);

	/* process roundto */
	if (roundto < 0 || (fabs(roundto - 0.0) < FLT_EPSILON)) {
		roundto = 0;
		scale = 0;
	}
	/* tenths, hundredths, thousandths, etc */
	else if (roundto < 1) {
    switch (pixtype) {
			/* integer band types don't have digits after the decimal place */
			case PT_1BB:
			case PT_2BUI:
			case PT_4BUI:
			case PT_8BSI:
			case PT_8BUI:
			case PT_16BSI:
			case PT_16BUI:
			case PT_32BSI:
			case PT_32BUI:
				roundto = 0;
				break;
			/* floating points, check the rounding */
			case PT_32BF:
			case PT_64BF:
				for (scale = 0; scale <= 20; scale++) {
					tmpd = roundto * pow(10, scale);
					if (fabs((tmpd - ((int) tmpd)) - 0.0) < FLT_EPSILON) break;
				}
				break;
			case PT_END:
				break;
		}
	}
	/* ones, tens, hundreds, etc */
	else {
		for (scale = 0; scale >= -20; scale--) {
			tmpd = roundto * pow(10, scale);
			if (tmpd < 1 || fabs(tmpd - 1.0) < FLT_EPSILON) {
				if (scale == 0) doround = 1;
				break;
			}
		}
	}

	if (scale != 0 || doround)
		doround = 1;
	else
		doround = 0;
	RASTER_DEBUGF(3, "scale = %d", scale);
	RASTER_DEBUGF(3, "doround = %d", doround);

	/* process search_values */
	if (search_values_count > 0 && NULL != search_values) {
		vcnts = (rt_valuecount) rtalloc(sizeof(struct rt_valuecount_t) * search_values_count);
		if (NULL == vcnts) {
			rterror("rt_band_get_count_of_values: Unable to allocate memory for value counts");
			*rtn_count = 0;
			return NULL;
		}

		for (i = 0; i < search_values_count; i++) {
			vcnts[i].count = 0;
			vcnts[i].percent = 0;
			if (!doround)
				vcnts[i].value = search_values[i];
			else
				vcnts[i].value = ROUND(search_values[i], scale);
		}
		vcnts_count = i;
	}
	else
		search_values_count = 0;
	RASTER_DEBUGF(3, "search_values_count = %d", search_values_count);

	/* entire band is nodata */
	if (rt_band_get_isnodata_flag(band) != FALSE) {
		if (exclude_nodata_value) {
			rtwarn("All pixels of band have the NODATA value");
			return NULL;
		}
		else {
			if (search_values_count > 0) {
				/* check for nodata match */
				for (i = 0; i < search_values_count; i++) {
					if (!doround)
						tmpd = nodata;
					else
						tmpd = ROUND(nodata, scale);

					if (fabs(tmpd - vcnts[i].value) > FLT_EPSILON)
						continue;

					vcnts[i].count = band->width * band->height;
					vcnts->percent = 1.0;
				}

				*rtn_count = vcnts_count;
			}
			/* no defined search values */
			else {
				vcnts = (rt_valuecount) rtalloc(sizeof(struct rt_valuecount_t));
				if (NULL == vcnts) {
					rterror("rt_band_get_count_of_values: Unable to allocate memory for value counts");
					*rtn_count = 0;
					return NULL;
				}

				vcnts->value = nodata;
				vcnts->count = band->width * band->height;
				vcnts->percent = 1.0;

				*rtn_count = 1;
			}

			return vcnts;
		}
	}

	for (x = 0; x < band->width; x++) {
		for (y = 0; y < band->height; y++) {
			rtn = rt_band_get_pixel(band, x, y, &pxlval);

			/* error getting value, continue */
			if (rtn == -1) continue;

			if (
				!exclude_nodata_value || (
					exclude_nodata_value &&
					(hasnodata != FALSE) &&
					(fabs(pxlval - nodata) > FLT_EPSILON)
				)
			) {
				total++;
				if (doround) {
					rpxlval = ROUND(pxlval, scale);
				}
				else
					rpxlval = pxlval;
				RASTER_DEBUGF(5, "(pxlval, rpxlval) => (%0.6f, %0.6f)", pxlval, rpxlval);

				new_valuecount = 1;
				/* search for match in existing valuecounts */
				for (i = 0; i < vcnts_count; i++) {
					/* match found */
					if (fabs(vcnts[i].value - rpxlval) < FLT_EPSILON) {
						vcnts[i].count++;
						new_valuecount = 0;
						RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[i].value, vcnts[i].count);
						break;
					}
				}

				/*
					don't add new valuecount either because
						- no need for new one
						- user-defined search values
				*/
				if (!new_valuecount || search_values_count > 0) continue;

				/* add new valuecount */
				vcnts = rtrealloc(vcnts, sizeof(struct rt_valuecount_t) * (vcnts_count + 1));
				if (NULL == vcnts) {
					rterror("rt_band_get_count_of_values: Unable to allocate memory for value counts");
					*rtn_count = 0;
					return NULL;
				}

				vcnts[vcnts_count].value = rpxlval;
				vcnts[vcnts_count].count = 1;
				vcnts[vcnts_count].percent = 0;
				RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[vcnts_count].value, vcnts[vcnts_count].count);
				vcnts_count++;
			}
		}
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);
#endif

	for (i = 0; i < vcnts_count; i++) {
		vcnts[i].percent = (double) vcnts[i].count / total;
		RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[i].value, vcnts[i].count);
	}

	RASTER_DEBUG(3, "done");
	*rtn_count = vcnts_count;
	return vcnts;
}

struct rt_reclassexpr_t {
	struct rt_reclassrange {
		double min;
		double max;
		int inc_min; /* include min */
		int inc_max; /* include max */
		int exc_min; /* exceed min */
		int exc_max; /* exceed max */
	} src, dst;
};

/**
 * Returns new band with values reclassified
 * 
 * @param srcband : the band who's values will be reclassified
 * @param pixtype : pixel type of the new band
 * @param hasnodata : indicates if the band has a nodata value
 * @param nodataval : nodata value for the new band
 * @param exprset : array of rt_reclassexpr structs
 * @param exprcount : number of elements in expr
 *
 * @return a new rt_band or 0 on error
 */
rt_band
rt_band_reclass(rt_band srcband, rt_pixtype pixtype,
	uint32_t hasnodata, double nodataval, rt_reclassexpr *exprset,
	int exprcount) {
	rt_band band = NULL;
	uint32_t width = 0;
	uint32_t height = 0;
	int numval = 0;
	int memsize = 0;
	void *mem = NULL;
	uint32_t src_hasnodata = 0;
	double src_nodataval = 0.0;

	int rtn;
	uint32_t x;
	uint32_t y;
	int i;
	double or = 0;
	double ov = 0;
	double nr = 0;
	double nv = 0;
	int do_nv = 0;
	rt_reclassexpr expr = NULL;

	assert(NULL != srcband);
	assert(NULL != exprset);

	/* source nodata */
	src_hasnodata = rt_band_get_hasnodata_flag(srcband);
	src_nodataval = rt_band_get_nodata(srcband);

	/* size of memory block to allocate */
	width = rt_band_get_width(srcband);
	height = rt_band_get_height(srcband);
	numval = width * height;
	memsize = rt_pixtype_size(pixtype) * numval;
	mem = (int *) rtalloc(memsize);
	if (!mem) {
		rterror("rt_band_reclass: Could not allocate memory for band");
		return 0;
	}

	/* initialize to zero */
	if (!hasnodata) {
		memset(mem, 0, memsize);
	}
	/* initialize to nodataval */
	else {
		int32_t checkvalint = 0;
		uint32_t checkvaluint = 0;
		double checkvaldouble = 0;
		float checkvalfloat = 0;

		switch (pixtype) {
			case PT_1BB:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_1BB(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_2BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_2BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_4BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_4BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_8BSI:
			{
				int8_t *ptr = mem;
				int8_t clamped_initval = rt_util_clamp_to_8BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_8BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_8BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_16BSI:
			{
				int16_t *ptr = mem;
				int16_t clamped_initval = rt_util_clamp_to_16BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_16BUI:
			{
				uint16_t *ptr = mem;
				uint16_t clamped_initval = rt_util_clamp_to_16BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_32BSI:
			{
				int32_t *ptr = mem;
				int32_t clamped_initval = rt_util_clamp_to_32BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_32BUI:
			{
				uint32_t *ptr = mem;
				uint32_t clamped_initval = rt_util_clamp_to_32BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvaluint = ptr[0];
				break;
			}
			case PT_32BF:
			{
				float *ptr = mem;
				float clamped_initval = rt_util_clamp_to_32F(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalfloat = ptr[0];
				break;
			}
			case PT_64BF:
			{
				double *ptr = mem;
				for (i = 0; i < numval; i++)
					ptr[i] = nodataval;
				checkvaldouble = ptr[0];
				break;
			}
			default:
			{
				rterror("rt_band_reclass: Unknown pixeltype %d", pixtype);
				rtdealloc(mem);
				return 0;
			}
		}

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
		/* Overflow checking */
		rt_util_display_dbl_trunc_warning(nodataval, checkvalint, checkvaluint, checkvalfloat,
			checkvaldouble, pixtype);
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */
	}
	RASTER_DEBUGF(3, "rt_band_reclass: width = %d height = %d", width, height);

	band = rt_band_new_inline(width, height, pixtype, hasnodata, nodataval, mem);
	if (!band) {
		rterror("rt_band_reclass: Could not create new band");
		rtdealloc(mem);
		return 0;
	}
	RASTER_DEBUGF(3, "rt_band_reclass: new band @ %p", band);

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			rtn = rt_band_get_pixel(srcband, x, y, &ov);

			/* error getting value, skip */
			if (rtn == -1) continue;

			do {
				do_nv = 0;

				/* no data*/
				if (src_hasnodata && hasnodata && ov == src_nodataval) {
					do_nv = 1;
					break;
				}

				for (i = 0; i < exprcount; i++) {
					expr = exprset[i];

					/* ov matches min and max*/
					if (
						fabs(expr->src.min - ov) < FLT_EPSILON &&
						fabs(expr->src.max - ov) < FLT_EPSILON
					) {
						do_nv = 1;
						break;
					}

					/* process min */
					if ((
						expr->src.exc_min && (
							expr->src.min > ov ||
							fabs(expr->src.min - ov) < FLT_EPSILON
						)) || (
						expr->src.inc_min && (
							expr->src.min < ov ||
							fabs(expr->src.min - ov) < FLT_EPSILON
						)) || (
						expr->src.min < ov
					)) {
						/* process max */
						if ((
							expr->src.exc_max && (
								ov > expr->src.max ||
								fabs(expr->src.max - ov) < FLT_EPSILON
							)) || (
								expr->src.inc_max && (
								ov < expr->src.max ||
								fabs(expr->src.max - ov) < FLT_EPSILON
							)) || (
							ov < expr->src.max
						)) {
							do_nv = 1;
							break;
						}
					}
				}
			}
			while (0);

			/* no expression matched, do not continue */
			if (!do_nv) continue;

			/* converting a value from one range to another range
			OldRange = (OldMax - OldMin)
			NewRange = (NewMax - NewMin)
			NewValue = (((OldValue - OldMin) * NewRange) / OldRange) + NewMin
			*/

			/* nodata */
			if (
				src_hasnodata &&
				hasnodata &&
				fabs(ov - src_nodataval) < FLT_EPSILON
			) {
				nv = nodataval;
			}
			/*
				"src" min and max is the same, prevent division by zero
				set nv to "dst" min, which should be the same as "dst" max
			*/
			else if (fabs(expr->src.max - expr->src.min) < FLT_EPSILON) {
				nv = expr->dst.min;
			}
			else {
				or = expr->src.max - expr->src.min;
				nr = expr->dst.max - expr->dst.min;
				nv = (((ov - expr->src.min) * nr) / or) + expr->dst.min;

				if (nv < expr->dst.min)
					nv = expr->dst.min;
				else if (nv > expr->dst.max)
					nv = expr->dst.max;
			}

			/* round the value for integers */
			switch (pixtype) {
				case PT_1BB:
				case PT_2BUI:
				case PT_4BUI:
				case PT_8BSI:
				case PT_8BUI:
				case PT_16BSI:
				case PT_16BUI:
				case PT_32BSI:
				case PT_32BUI:
					nv = round(nv);
					break;
				default:
					break;
			}

			RASTER_DEBUGF(5, "(%d, %d) ov: %f or: %f - %f nr: %f - %f nv: %f"
				, x
				, y
				, ov
				, (NULL != expr) ? expr->src.min : 0
				, (NULL != expr) ? expr->src.max : 0
				, (NULL != expr) ? expr->dst.min : 0
				, (NULL != expr) ? expr->dst.max : 0
				, nv
			);
			rtn = rt_band_set_pixel(band, x, y, nv);
			if (rtn == -1) {
				rterror("rt_band_reclass: Could not assign value to new band");
				rt_band_destroy(band);
				rtdealloc(mem);
				return 0;
			}

			expr = NULL;
		}
	}

	return band;
}

/*- rt_raster --------------------------------------------------------*/

struct rt_raster_serialized_t {
    /*---[ 8 byte boundary ]---{ */
    uint32_t size; /* required by postgresql: 4 bytes */
    uint16_t version; /* format version (this is version 0): 2 bytes */
    uint16_t numBands; /* Number of bands: 2 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double scaleX; /* pixel width: 8 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double scaleY; /* pixel height: 8 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double ipX; /* insertion point X: 8 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double ipY; /* insertion point Y: 8 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double skewX; /* skew about the X axis: 8 bytes */

    /* }---[ 8 byte boundary ]---{ */
    double skewY; /* skew about the Y axis: 8 bytes */

    /* }---[ 8 byte boundary ]--- */
    int32_t srid; /* Spatial reference id: 4 bytes */
    uint16_t width; /* pixel columns: 2 bytes */
    uint16_t height; /* pixel rows: 2 bytes */
};

/* NOTE: the initial part of this structure matches the layout
 *       of data in the serialized form version 0, starting
 *       from the numBands element
 */
struct rt_raster_t {
    uint32_t size;
    uint16_t version;

    /* Number of bands, all share the same dimension
     * and georeference */
    uint16_t numBands;

    /* Georeference (in projection units) */
    double scaleX; /* pixel width */
    double scaleY; /* pixel height */
    double ipX; /* geo x ordinate of the corner of upper-left pixel */
    double ipY; /* geo y ordinate of the corner of bottom-right pixel */
    double skewX; /* skew about the X axis*/
    double skewY; /* skew about the Y axis */

    int32_t srid; /* spatial reference id */
    uint16_t width; /* pixel columns - max 65535 */
    uint16_t height; /* pixel rows - max 65535 */
    rt_band *bands; /* actual bands */

};

rt_raster
rt_raster_new(uint16_t width, uint16_t height) {
    rt_raster ret = NULL;



    ret = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
    if (!ret) {
        rterror("rt_raster_new: Out of virtual memory creating an rt_raster");
        return 0;
    }

    RASTER_DEBUGF(3, "Created rt_raster @ %p", ret);

    assert(NULL != ret);

    ret->width = width;

    ret->height = height;
    ret->scaleX = 1;
    ret->scaleY = 1;
    ret->ipX = 0.0;
    ret->ipY = 0.0;
    ret->skewX = 0.0;
    ret->skewY = 0.0;
    ret->srid = SRID_UNKNOWN;

    ret->numBands = 0;
    ret->bands = 0;

    return ret;
}

void
rt_raster_destroy(rt_raster raster) {


    RASTER_DEBUGF(3, "Destroying rt_raster @ %p", raster);

    if (raster->bands) {
        rtdealloc(raster->bands);
    }
    rtdealloc(raster);
}

uint16_t
rt_raster_get_width(rt_raster raster) {

    assert(NULL != raster);

    return raster->width;
}

uint16_t
rt_raster_get_height(rt_raster raster) {

    assert(NULL != raster);

    return raster->height;
}

void
rt_raster_set_scale(rt_raster raster,
        double scaleX, double scaleY) {


    assert(NULL != raster);

    raster->scaleX = scaleX;
    raster->scaleY = scaleY;
}

double
rt_raster_get_x_scale(rt_raster raster) {


    assert(NULL != raster);

    return raster->scaleX;
}

double
rt_raster_get_y_scale(rt_raster raster) {


    assert(NULL != raster);

    return raster->scaleY;
}

void
rt_raster_set_skews(rt_raster raster,
        double skewX, double skewY) {


    assert(NULL != raster);

    raster->skewX = skewX;
    raster->skewY = skewY;
}

double
rt_raster_get_x_skew(rt_raster raster) {


    assert(NULL != raster);

    return raster->skewX;
}

double
rt_raster_get_y_skew(rt_raster raster) {


    assert(NULL != raster);

    return raster->skewY;
}

void
rt_raster_set_offsets(rt_raster raster, double x, double y) {


    assert(NULL != raster);

    raster->ipX = x;
    raster->ipY = y;
}

double
rt_raster_get_x_offset(rt_raster raster) {


    assert(NULL != raster);

    return raster->ipX;
}

double
rt_raster_get_y_offset(rt_raster raster) {


    assert(NULL != raster);

    return raster->ipY;
}

int32_t
rt_raster_get_srid(rt_raster raster) {


    assert(NULL != raster);

    return raster->srid;
}

void
rt_raster_set_srid(rt_raster raster, int32_t srid) {


    assert(NULL != raster);

    raster->srid = srid;
}

int
rt_raster_get_num_bands(rt_raster raster) {


    assert(NULL != raster);

    return raster->numBands;
}

rt_band
rt_raster_get_band(rt_raster raster, int n) {


    assert(NULL != raster);

    if (n >= raster->numBands || n < 0) return 0;
    return raster->bands[n];
}

int32_t
rt_raster_add_band(rt_raster raster, rt_band band, int index) {
    rt_band *oldbands = NULL;
    rt_band oldband = NULL;
    rt_band tmpband = NULL;
    uint16_t i = 0;



    assert(NULL != raster);

    RASTER_DEBUGF(3, "Adding band %p to raster %p", band, raster);

    if (band->width != raster->width || band->height != raster->height) {
        rterror("rt_raster_add_band: Can't add a %dx%d band to a %dx%d raster",
                band->width, band->height, raster->width, raster->height);
        return -1;
    }

    if (index > raster->numBands)
        index = raster->numBands;

    if (index < 0)
        index = 0;

    oldbands = raster->bands;

    RASTER_DEBUGF(3, "Oldbands at %p", oldbands);

    raster->bands = (rt_band*) rtrealloc(raster->bands,
            sizeof (rt_band)*(raster->numBands + 1)
            );

    RASTER_DEBUG(3, "Checking bands");

    if (NULL == raster->bands) {
        rterror("rt_raster_add_band: Out of virtual memory "
                "reallocating band pointers");
        raster->bands = oldbands;
        return -1;
    }

    RASTER_DEBUGF(4, "realloc returned %p", raster->bands);

    for (i = 0; i <= raster->numBands; ++i) {
        if (i == index) {
            oldband = raster->bands[i];
            raster->bands[i] = band;
        } else if (i > index) {
            tmpband = raster->bands[i];
            raster->bands[i] = oldband;
            oldband = tmpband;
        }
    }

    raster->numBands++;

    RASTER_DEBUGF(4, "now raster has %d bands", raster->numBands);

    return index;
}


int32_t
rt_raster_generate_new_band(rt_raster raster, rt_pixtype pixtype,
        double initialvalue, uint32_t hasnodata, double nodatavalue, int index)
{
    rt_band band = NULL;
    int width = 0;
    int height = 0;
    int numval = 0;
    int datasize = 0;
    int oldnumbands = 0;
    int numbands = 0;
    void * mem = NULL;
    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    double checkvaldouble = 0;
    float checkvalfloat = 0;
    int i;


    assert(NULL != raster);

    /* Make sure index is in a valid range */
    oldnumbands = rt_raster_get_num_bands(raster);
    if (index < 0)
        index = 0;
    else if (index > oldnumbands + 1)
        index = oldnumbands + 1;

    /* Determine size of memory block to allocate and allocate it */
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);
    numval = width * height;
    datasize = rt_pixtype_size(pixtype) * numval;

    mem = (int *)rtalloc(datasize);
    if (!mem) {
        rterror("rt_raster_generate_new_band: Could not allocate memory for band");
        return -1;
    }

    if (fabs(initialvalue - 0.0) < FLT_EPSILON)
        memset(mem, 0, datasize);
    else {
        switch (pixtype)
        {
            case PT_1BB:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_1BB(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_2BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_2BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_4BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_4BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BSI:
            {
                int8_t *ptr = mem;
                int8_t clamped_initval = rt_util_clamp_to_8BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_8BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BSI:
            {
                int16_t *ptr = mem;
                int16_t clamped_initval = rt_util_clamp_to_16BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BUI:
            {
                uint16_t *ptr = mem;
                uint16_t clamped_initval = rt_util_clamp_to_16BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BSI:
            {
                int32_t *ptr = mem;
                int32_t clamped_initval = rt_util_clamp_to_32BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BUI:
            {
                uint32_t *ptr = mem;
                uint32_t clamped_initval = rt_util_clamp_to_32BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvaluint = ptr[0];
                break;
            }
            case PT_32BF:
            {
                float *ptr = mem;
                float clamped_initval = rt_util_clamp_to_32F(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
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
                rterror("rt_raster_generate_new_band: Unknown pixeltype %d", pixtype);
                rtdealloc(mem);
                return -1;
            }
        }
    }

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    /* Overflow checking */
    rt_util_display_dbl_trunc_warning(initialvalue, checkvalint, checkvaluint, checkvalfloat,
                                      checkvaldouble, pixtype);
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    band = rt_band_new_inline(width, height, pixtype, hasnodata, nodatavalue, mem);
    if (! band) {
        rterror("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        rtdealloc(mem);
        return -1;
    }
    index = rt_raster_add_band(raster, band, index);
    numbands = rt_raster_get_num_bands(raster);
    if (numbands == oldnumbands || index == -1) {
        rterror("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        rt_band_destroy(band);
    }

    return index;
}


void
rt_raster_cell_to_geopoint(rt_raster raster,
        double x, double y,
        double* x1, double* y1) {


    assert(NULL != raster);
    assert(NULL != x1);
    assert(NULL != y1);

    /* Six parameters affine transformation */
    *x1 = raster->scaleX * x + raster->skewX * y + raster->ipX;
    *y1 = raster->scaleY * y + raster->skewY * x + raster->ipY;

    RASTER_DEBUGF(3, "rt_raster_cell_to_geopoint(%g,%g)", x, y);
    RASTER_DEBUGF(3, " ipx/y:%g/%g", raster->ipX, raster->ipY);
    RASTER_DEBUGF(3, "cell_to_geopoint: ipX:%g, ipY:%g, %g,%g -> %g,%g",
            raster->ipX, raster->ipY, x, y, *x1, *y1);

}

/* WKT string representing each polygon in WKT format acompagned by its
correspoding value */
struct rt_geomval_t {
    int srid;
    double val;
    char * geom;
};

rt_geomval
rt_raster_dump_as_wktpolygons(rt_raster raster, int nband, int * pnElements)
{
    char * pszQuery;
    long j;
    OGRSFDriverH ogr_drv = NULL;
    GDALDriverH gdal_drv = NULL;
    GDALDatasetH memdataset = NULL;
    GDALRasterBandH gdal_band = NULL;
    OGRDataSourceH memdatasource = NULL;
    rt_geomval pols = NULL;
    OGRLayerH hLayer = NULL;
    OGRFeatureH hFeature = NULL;
    OGRGeometryH hGeom = NULL;
    OGRFieldDefnH hFldDfn = NULL;
    char * pszSrcText = NULL;
    int nFeatureCount = 0;
    rt_band band = NULL;
    int iPixVal = -1;
    double dValue = 0.0;
    int iBandHasNodataValue = FALSE;
    double dBandNoData = 0.0;

    uint32_t bandNums[1] = {nband - 1};

    /* Checkings */


    assert(NULL != raster);
    assert(nband > 0 && nband <= rt_raster_get_num_bands(raster));

    RASTER_DEBUG(2, "In rt_raster_dump_as_polygons");

    /*******************************
     * Get band
     *******************************/
    band = rt_raster_get_band(raster, nband - 1);
    if (NULL == band) {
        rterror("rt_raster_dump_as_wktpolygons: Error getting band %d from raster", nband);
        return 0;
    }

    iBandHasNodataValue = rt_band_get_hasnodata_flag(band);
    if (iBandHasNodataValue) dBandNoData = rt_band_get_nodata(band);

    /*****************************************************
     * Convert raster to GDAL MEM dataset
     *****************************************************/
    memdataset = rt_raster_to_gdal_mem(raster, NULL, bandNums, 1, &gdal_drv);
		if (NULL == memdataset) {
				rterror("rt_raster_dump_as_wktpolygons: Couldn't convert raster to GDAL MEM dataset");

				return 0;
		}

    /*****************************
     * Register ogr mem driver
     *****************************/
    OGRRegisterAll();

    RASTER_DEBUG(3, "creating OGR MEM vector");


    /*****************************************************
     * Create an OGR in-memory vector for layers
     *****************************************************/
    ogr_drv = OGRGetDriverByName("Memory");
    memdatasource = OGR_Dr_CreateDataSource(ogr_drv, "", NULL);

    if (NULL == memdatasource) {
        rterror("rt_raster_dump_as_wktpolygons: Couldn't create a OGR Datasource to store pols\n");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);

        return 0;
    }

    /**
     * Can MEM driver create new layers?
     **/
    if (!OGR_DS_TestCapability(memdatasource, ODsCCreateLayer)) {
        rterror("rt_raster_dump_as_wktpolygons: MEM driver can't create new layers, aborting\n");
        /* xxx jorgearevalo: what should we do now? */
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);
        OGRReleaseDataSource(memdatasource);
        return 0;
    }

    RASTER_DEBUG(3, "polygonizying GDAL MEM raster band");

    /*****************************
     * Polygonize the raster band
     *****************************/

    /**
     * From GDALPolygonize function header: "Polygon features will be
     * created on the output layer, with polygon geometries representing
     * the polygons". So,the WKB geometry type should be "wkbPolygon"
     **/
    hLayer = OGR_DS_CreateLayer(memdatasource, "PolygonizedLayer", NULL,
            wkbPolygon, NULL);

    if (NULL == hLayer) {
        rterror("rt_raster_dump_as_wktpolygons: Couldn't create layer to store polygons");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);
        OGRReleaseDataSource(memdatasource);
        return 0;
    }

    /**
     * Create a new field in the layer, to store the px value
     */

    /* First, create a field definition to create the field */
    hFldDfn = OGR_Fld_Create("PixelValue", OFTReal);

    /* Second, create the field */
    if (OGR_L_CreateField(hLayer, hFldDfn, TRUE) !=
            OGRERR_NONE) {
        rtwarn("Couldn't create a field in OGR Layer. The polygons generated won't be able to store the pixel value");
        iPixVal = -1;
    }
    else {
        /* Index to the new field created in the layer */
        iPixVal = 0;
    }


    /* Get GDAL raster band */
    gdal_band = GDALGetRasterBand(memdataset, 1);
    if (NULL == gdal_band) {
        rterror("rt_raster_dump_as_wktpolygons: Couldn't get GDAL band to polygonize");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);

        OGR_Fld_Destroy(hFldDfn);
        OGR_DS_DeleteLayer(memdatasource, 0);
        OGRReleaseDataSource(memdatasource);

        return 0;
    }

    /**
     * We don't need a raster mask band. Each band has a nodata value.
     **/
#if GDALFPOLYGONIZE == 1 
    GDALFPolygonize(gdal_band, NULL, hLayer, iPixVal, NULL, NULL, NULL);
#else
    GDALPolygonize(gdal_band, NULL, hLayer, iPixVal, NULL, NULL, NULL);
#endif

    /**
     * Optimization: Apply a OGR SQL filter to the layer to select the
     * features different from NODATA value.
     *
     * Thanks to David Zwarg.
     **/
    if (iBandHasNodataValue) {
        pszQuery = (char *) rtalloc(50 * sizeof (char));
        sprintf(pszQuery, "PixelValue != %f", dBandNoData );
        OGRErr e = OGR_L_SetAttributeFilter(hLayer, pszQuery);
        if (e != OGRERR_NONE) {
                rtwarn("Error filtering NODATA values for band. All values"
                        " will be treated as data values");
        }
    }

    else {
        pszQuery = NULL;
    }



    /*********************************************************************
     * Transform OGR layers in WKT polygons
     * XXX jorgearevalo: GDALPolygonize does not set the coordinate system
     * on the output layer. Application code should do this when the layer
     * is created, presumably matching the raster coordinate system.
     * TODO: modify GDALPolygonize to directly emit polygons in WKT format?
     *********************************************************************/
    nFeatureCount = OGR_L_GetFeatureCount(hLayer, TRUE);

    /* Allocate memory for pols */
    pols = (rt_geomval) rtalloc(nFeatureCount * sizeof (struct rt_geomval_t));

    if (NULL == pols) {
        rterror("rt_raster_dump_as_wktpolygons: Couldn't allocate memory for "
                "geomval structure");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);

        OGR_Fld_Destroy(hFldDfn);
        OGR_DS_DeleteLayer(memdatasource, 0);
        OGRReleaseDataSource(memdatasource);

        return 0;
    }

    RASTER_DEBUGF(3, "storing polygons (%d)", nFeatureCount);

    if (pnElements)
        *pnElements = 0;

    /* Reset feature reading to start in the first feature */
    OGR_L_ResetReading(hLayer);

    for (j = 0; j < nFeatureCount; j++) {

        hFeature = OGR_L_GetNextFeature(hLayer);
        dValue = OGR_F_GetFieldAsDouble(hFeature, iPixVal);

	hGeom = OGR_F_GetGeometryRef(hFeature);
     	OGR_G_ExportToWkt(hGeom, &pszSrcText);

      	pols[j].val = dValue;
       	pols[j].srid = rt_raster_get_srid(raster);
     	pols[j].geom = (char *) rtalloc((1 + strlen(pszSrcText))
                    * sizeof (char));
     	strcpy(pols[j].geom, pszSrcText);

     	RASTER_DEBUGF(4, "Feature %d, Polygon: %s", j, pols[j].geom);
     	RASTER_DEBUGF(4, "Feature %d, value: %f", j, pols[j].val);
     	RASTER_DEBUGF(4, "Feature %d, srid: %d", j, pols[j].srid);

		/**
         * We can't use deallocator from rt_context, because it comes from
         * postgresql backend, that uses pfree. This function needs a
         * postgresql memory context to work with, and the memory created
         * for pszSrcText is created outside this context.
         **/
         //rtdealloc(pszSrcText);
        free(pszSrcText);
        pszSrcText = NULL;

        OGR_F_Destroy(hFeature);
    }

    if (pnElements)
        *pnElements = nFeatureCount;

    RASTER_DEBUG(3, "destroying GDAL MEM raster");

    GDALClose(memdataset);
    GDALDeregisterDriver(gdal_drv);
    GDALDestroyDriver(gdal_drv);

    RASTER_DEBUG(3, "destroying OGR MEM vector");

    OGR_Fld_Destroy(hFldDfn);
    OGR_DS_DeleteLayer(memdatasource, 0);
    OGRReleaseDataSource(memdatasource);

    return pols;
}

LWPOLY*
rt_raster_get_convex_hull(rt_raster raster) {
    POINTARRAY **rings = NULL;
    POINTARRAY *pts = NULL;
    LWPOLY* ret = NULL;
    POINT4D p4d;

    assert(NULL != raster);

    RASTER_DEBUGF(3, "rt_raster_get_convex_hull: raster is %dx%d",
            raster->width, raster->height);

    if ((!raster->width) || (!raster->height)) {
        return 0;
    }

    rings = (POINTARRAY **) rtalloc(sizeof (POINTARRAY*));
    if (!rings) {
        rterror("rt_raster_get_convex_hull: Out of memory [%s:%d]", __FILE__, __LINE__);
        return 0;
    }
    rings[0] = ptarray_construct(0, 0, 5);
    /* TODO: handle error on ptarray construction */
    /* XXX jorgearevalo: the error conditions aren't managed in ptarray_construct */
    if (!rings[0]) {
        rterror("rt_raster_get_convex_hull: Out of memory [%s:%d]", __FILE__, __LINE__);
        return 0;
    }
    pts = rings[0];

    /* Upper-left corner (first and last points) */
    rt_raster_cell_to_geopoint(raster,
            0, 0,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 0, &p4d);
    ptarray_set_point4d(pts, 4, &p4d); /* needed for closing it? */

    /* Upper-right corner (we go clockwise) */
    rt_raster_cell_to_geopoint(raster,
            raster->width, 0,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 1, &p4d);

    /* Lower-right corner */
    rt_raster_cell_to_geopoint(raster,
            raster->width, raster->height,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 2, &p4d);

    /* Lower-left corner */
    rt_raster_cell_to_geopoint(raster,
            0, raster->height,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 3, &p4d);

    ret = lwpoly_construct(raster->srid, 0, 1, rings);

    return ret;
}

/*--------- WKB I/O ---------------------------------------------------*/

static uint8_t
isMachineLittleEndian(void) {
    static int endian_check_int = 1; /* dont modify this!!! */
    /* 0=big endian|xdr --  1=little endian|ndr */
    return *((uint8_t *) & endian_check_int);
}

static uint8_t
read_uint8(const uint8_t** from) {
    assert(NULL != from);

    return *(*from)++;
}

/* unused up to now
static void
write_uint8(uint8_t** from, uint8_t v)
{
    assert(NULL != from);

 *(*from)++ = v;
}
 */

static int8_t
read_int8(const uint8_t** from) {
    assert(NULL != from);

    return (int8_t) read_uint8(from);
}

/* unused up to now
static void
write_int8(uint8_t** from, int8_t v)
{
    assert(NULL != from);

 *(*from)++ = v;
}
 */

static uint16_t
read_uint16(const uint8_t** from, uint8_t littleEndian) {
    uint16_t ret = 0;

    assert(NULL != from);

    if (littleEndian) {
        ret = (*from)[0] |
                (*from)[1] << 8;
    } else {
        /* big endian */
        ret = (*from)[0] << 8 |
                (*from)[1];
    }
    *from += 2;
    return ret;
}

static void
write_uint16(uint8_t** to, uint8_t littleEndian, uint16_t v) {
    assert(NULL != to);

    if (littleEndian) {
        (*to)[0] = v & 0x00FF;
        (*to)[1] = v >> 8;
    } else {
        (*to)[1] = v & 0x00FF;
        (*to)[0] = v >> 8;
    }
    *to += 2;
}

static int16_t
read_int16(const uint8_t** from, uint8_t littleEndian) {
    assert(NULL != from);

    return read_uint16(from, littleEndian);
}

/* unused up to now
static void
write_int16(uint8_t** to, uint8_t littleEndian, int16_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x00FF;
        (*to)[1] = v >> 8;
    }
    else
    {
        (*to)[1] = v & 0x00FF;
        (*to)[0] = v >> 8;
    }
 *to += 2;
}
 */

static uint32_t
read_uint32(const uint8_t** from, uint8_t littleEndian) {
    uint32_t ret = 0;

    assert(NULL != from);

    if (littleEndian) {
        ret = (uint32_t) ((*from)[0] & 0xff) |
                (uint32_t) ((*from)[1] & 0xff) << 8 |
                (uint32_t) ((*from)[2] & 0xff) << 16 |
                (uint32_t) ((*from)[3] & 0xff) << 24;
    } else {
        /* big endian */
        ret = (uint32_t) ((*from)[3] & 0xff) |
                (uint32_t) ((*from)[2] & 0xff) << 8 |
                (uint32_t) ((*from)[1] & 0xff) << 16 |
                (uint32_t) ((*from)[0] & 0xff) << 24;
    }

    *from += 4;
    return ret;
}

/* unused up to now
static void
write_uint32(uint8_t** to, uint8_t littleEndian, uint32_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x000000FF;
        (*to)[1] = ( v & 0x0000FF00 ) >> 8;
        (*to)[2] = ( v & 0x00FF0000 ) >> 16;
        (*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
        (*to)[3] = v & 0x000000FF;
        (*to)[2] = ( v & 0x0000FF00 ) >> 8;
        (*to)[1] = ( v & 0x00FF0000 ) >> 16;
        (*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
 */

static int32_t
read_int32(const uint8_t** from, uint8_t littleEndian) {
    assert(NULL != from);

    return read_uint32(from, littleEndian);
}

/* unused up to now
static void
write_int32(uint8_t** to, uint8_t littleEndian, int32_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x000000FF;
        (*to)[1] = ( v & 0x0000FF00 ) >> 8;
        (*to)[2] = ( v & 0x00FF0000 ) >> 16;
        (*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
        (*to)[3] = v & 0x000000FF;
        (*to)[2] = ( v & 0x0000FF00 ) >> 8;
        (*to)[1] = ( v & 0x00FF0000 ) >> 16;
        (*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
 */

static float
read_float32(const uint8_t** from, uint8_t littleEndian) {

    union {
        float f;
        uint32_t i;
    } ret;

    ret.i = read_uint32(from, littleEndian);

    return ret.f;
}

/* unused up to now
static void
write_float32(uint8_t** from, uint8_t littleEndian, float f)
{
    union {
        float f;
        uint32_t i;
    } u;

    u.f = f;
    write_uint32(from, littleEndian, u.i);
}
 */

static double
read_float64(const uint8_t** from, uint8_t littleEndian) {

    union {
        double d;
        uint64_t i;
    } ret;

    assert(NULL != from);

    if (littleEndian) {
        ret.i = (uint64_t) ((*from)[0] & 0xff) |
                (uint64_t) ((*from)[1] & 0xff) << 8 |
                (uint64_t) ((*from)[2] & 0xff) << 16 |
                (uint64_t) ((*from)[3] & 0xff) << 24 |
                (uint64_t) ((*from)[4] & 0xff) << 32 |
                (uint64_t) ((*from)[5] & 0xff) << 40 |
                (uint64_t) ((*from)[6] & 0xff) << 48 |
                (uint64_t) ((*from)[7] & 0xff) << 56;
    } else {
        /* big endian */
        ret.i = (uint64_t) ((*from)[7] & 0xff) |
                (uint64_t) ((*from)[6] & 0xff) << 8 |
                (uint64_t) ((*from)[5] & 0xff) << 16 |
                (uint64_t) ((*from)[4] & 0xff) << 24 |
                (uint64_t) ((*from)[3] & 0xff) << 32 |
                (uint64_t) ((*from)[2] & 0xff) << 40 |
                (uint64_t) ((*from)[1] & 0xff) << 48 |
                (uint64_t) ((*from)[0] & 0xff) << 56;
    }

    *from += 8;
    return ret.d;
}

/* unused up to now
static void
write_float64(uint8_t** to, uint8_t littleEndian, double v)
{
    union {
        double d;
        uint64_t i;
    } u;

    assert(NULL != to);

    u.d = v;

    if ( littleEndian )
    {
        (*to)[0] =   u.i & 0x00000000000000FFULL;
        (*to)[1] = ( u.i & 0x000000000000FF00ULL ) >> 8;
        (*to)[2] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
        (*to)[3] = ( u.i & 0x00000000FF000000ULL ) >> 24;
        (*to)[4] = ( u.i & 0x000000FF00000000ULL ) >> 32;
        (*to)[5] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
        (*to)[6] = ( u.i & 0x00FF000000000000ULL ) >> 48;
        (*to)[7] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
    else
    {
        (*to)[7] =   u.i & 0x00000000000000FFULL;
        (*to)[6] = ( u.i & 0x000000000000FF00ULL ) >> 8;
        (*to)[5] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
        (*to)[4] = ( u.i & 0x00000000FF000000ULL ) >> 24;
        (*to)[3] = ( u.i & 0x000000FF00000000ULL ) >> 32;
        (*to)[2] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
        (*to)[1] = ( u.i & 0x00FF000000000000ULL ) >> 48;
        (*to)[0] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
 *to += 8;
}
 */

#define BANDTYPE_FLAGS_MASK 0xF0
#define BANDTYPE_PIXTYPE_MASK 0x0F
#define BANDTYPE_FLAG_OFFDB     (1<<7)
#define BANDTYPE_FLAG_HASNODATA (1<<6)
#define BANDTYPE_FLAG_ISNODATA  (1<<5)
#define BANDTYPE_FLAG_RESERVED3 (1<<4)

#define BANDTYPE_PIXTYPE(x) ((x)&BANDTYPE_PIXTYPE_MASK)
#define BANDTYPE_IS_OFFDB(x) ((x)&BANDTYPE_FLAG_OFFDB)
#define BANDTYPE_HAS_NODATA(x) ((x)&BANDTYPE_FLAG_HASNODATA)
#define BANDTYPE_IS_NODATA(x) ((x)&BANDTYPE_FLAG_ISNODATA)

/* Read band from WKB as at start of band */
static rt_band
rt_band_from_wkb(uint16_t width, uint16_t height,
        const uint8_t** ptr, const uint8_t* end,
        uint8_t littleEndian) {
    rt_band band = NULL;
    int pixbytes = 0;
    uint8_t type = 0;
    unsigned long sz = 0;
    uint32_t v = 0;



    assert(NULL != ptr);
    assert(NULL != end);

    band = rtalloc(sizeof (struct rt_band_t));
    if (!band) {
        rterror("rt_band_from_wkb: Out of memory allocating rt_band during WKB parsing");
        return 0;
    }

    if (end - *ptr < 1) {
        rterror("rt_band_from_wkb: Premature end of WKB on band reading (%s:%d)",
                __FILE__, __LINE__);
        return 0;
    }
    type = read_uint8(ptr);

    if ((type & BANDTYPE_PIXTYPE_MASK) >= PT_END) {
        rterror("rt_band_from_wkb: Invalid pixtype %d", type & BANDTYPE_PIXTYPE_MASK);
        rtdealloc(band);
        return 0;
    }
    assert(NULL != band);

    band->pixtype = type & BANDTYPE_PIXTYPE_MASK;
    band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
    band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
    band->isnodata = BANDTYPE_IS_NODATA(type) ? 1 : 0;
    band->width = width;
    band->height = height;

    RASTER_DEBUGF(3, " Band pixtype:%s, offline:%d, hasnodata:%d",
            rt_pixtype_name(band->pixtype),
            band->offline,
            band->hasnodata);

    /* Check there's enough bytes to read nodata value */

    pixbytes = rt_pixtype_size(band->pixtype);
    if (((*ptr) + pixbytes) >= end) {
        rterror("rt_band_from_wkb: Premature end of WKB on band novalue reading");
        rtdealloc(band);
        return 0;
    }

    /* Read nodata value */
    switch (band->pixtype) {
        case PT_1BB:
        {
            band->nodataval = ((int) read_uint8(ptr)) & 0x01;
            break;
        }
        case PT_2BUI:
        {
            band->nodataval = ((int) read_uint8(ptr)) & 0x03;
            break;
        }
        case PT_4BUI:
        {
            band->nodataval = ((int) read_uint8(ptr)) & 0x0F;
            break;
        }
        case PT_8BSI:
        {
            band->nodataval = read_int8(ptr);
            break;
        }
        case PT_8BUI:
        {
            band->nodataval = read_uint8(ptr);
            break;
        }
        case PT_16BSI:
        {
            band->nodataval = read_int16(ptr, littleEndian);
            break;
        }
        case PT_16BUI:
        {
            band->nodataval = read_uint16(ptr, littleEndian);
            break;
        }
        case PT_32BSI:
        {
            band->nodataval = read_int32(ptr, littleEndian);
            break;
        }
        case PT_32BUI:
        {
            band->nodataval = read_uint32(ptr, littleEndian);
            break;
        }
        case PT_32BF:
        {
            band->nodataval = read_float32(ptr, littleEndian);
            break;
        }
        case PT_64BF:
        {
            band->nodataval = read_float64(ptr, littleEndian);
            break;
        }
        default:
        {
            rterror("rt_band_from_wkb: Unknown pixeltype %d", band->pixtype);
            rtdealloc(band);
            return 0;
        }
    }

    RASTER_DEBUGF(3, " Nodata value: %g, pixbytes: %d, ptr @ %p, end @ %p",
            band->nodataval, pixbytes, *ptr, end);

    if (band->offline) {
        if (((*ptr) + 1) >= end) {
            rterror("rt_band_from_wkb: Premature end of WKB on offline "
                    "band data bandNum reading (%s:%d)",
                    __FILE__, __LINE__);
            rtdealloc(band);
            return 0;
        }
        band->data.offline.bandNum = read_int8(ptr);

        {
            /* check we have a NULL-termination */
            sz = 0;
            while ((*ptr)[sz] && &((*ptr)[sz]) < end) ++sz;
            if (&((*ptr)[sz]) >= end) {
                rterror("rt_band_from_wkb: Premature end of WKB on band offline path reading");
                rtdealloc(band);
                return 0;
            }

            band->ownsData = 1;
            band->data.offline.path = rtalloc(sz + 1);

            memcpy(band->data.offline.path, *ptr, sz);
            band->data.offline.path[sz] = '\0';

            RASTER_DEBUGF(3, "OFFDB band path is %s (size is %d)",
                    band->data.offline.path, sz);

            *ptr += sz + 1;

            /* TODO: How could we know if the offline band is a nodata band? */
            band->isnodata = FALSE;
        }
        return band;
    }

    /* This is an on-disk band */
    sz = width * height * pixbytes;
    if (((*ptr) + sz) > end) {
        rterror("rt_band_from_wkb: Premature end of WKB on band data reading (%s:%d)",
                __FILE__, __LINE__);
        rtdealloc(band);
        return 0;
    }

    band->data.mem = rtalloc(sz);
    if (!band->data.mem) {
        rterror("rt_band_from_wkb: Out of memory during band creation in WKB parser");
        rtdealloc(band);
        return 0;
    }

    memcpy(band->data.mem, *ptr, sz);
    *ptr += sz;

    /* Should now flip values if > 8bit and
     * littleEndian != isMachineLittleEndian */
    if (pixbytes > 1) {
        if (isMachineLittleEndian() != littleEndian) {
            {
                void (*flipper)(uint8_t*) = 0;
                uint8_t *flipme = NULL;

                if (pixbytes == 2) flipper = flip_endian_16;
                else if (pixbytes == 4) flipper = flip_endian_32;
                else if (pixbytes == 8) flipper = flip_endian_64;
                else {
                    rterror("rt_band_from_wkb: Unexpected pix bytes %d", pixbytes);
                    rtdealloc(band);
                    rtdealloc(band->data.mem);
                    return 0;
                }

                flipme = band->data.mem;
                sz = width * height;
                for (v = 0; v < sz; ++v) {
                    flipper(flipme);
                    flipme += pixbytes;
                }
            }
        }
    }
        /* And should check for invalid values for < 8bit types */
    else if (band->pixtype == PT_1BB ||
            band->pixtype == PT_2BUI ||
            band->pixtype == PT_4BUI) {
        {
            uint8_t maxVal = band->pixtype == PT_1BB ? 1 :
                    band->pixtype == PT_2BUI ? 3 :
                    15;
            uint8_t val;

            sz = width*height;
            for (v = 0; v < sz; ++v) {
                val = ((uint8_t*) band->data.mem)[v];
                if (val > maxVal) {
                    rterror("rt_band_from_wkb: Invalid value %d for pixel of type %s",
                            val, rt_pixtype_name(band->pixtype));
                    rtdealloc(band->data.mem);
                    rtdealloc(band);
                    return 0;
                }
            }
        }
    }

    /* And we should check if the band is a nodata band */
    /* TODO: No!! This is too slow */
    //rt_band_check_is_nodata(band);

    return band;
}

/* -4 for size, +1 for endian */
#define RT_WKB_HDR_SZ (sizeof(struct rt_raster_serialized_t)-4+1)

rt_raster
rt_raster_from_wkb(const uint8_t* wkb, uint32_t wkbsize) {
    const uint8_t *ptr = wkb;
    const uint8_t *wkbend = NULL;
    rt_raster rast = NULL;
    uint8_t endian = 0;
    uint16_t version = 0;
    uint16_t i = 0;



    assert(NULL != ptr);

    /* Check that wkbsize is >= sizeof(rt_raster_serialized) */
    if (wkbsize < RT_WKB_HDR_SZ) {
        rterror("rt_raster_from_wkb: wkb size < min size (%d)",
                RT_WKB_HDR_SZ);
        return 0;
    }
    wkbend = wkb + wkbsize;

    RASTER_DEBUGF(3, "Parsing header from wkb position %d (expected 0)",
            d_binptr_to_pos(ptr, wkbend, wkbsize));


    CHECK_BINPTR_POSITION(ptr, wkbend, wkbsize, 0);

    /* Read endianness */
    endian = *ptr;
    ptr += 1;

    /* Read version of protocol */
    version = read_uint16(&ptr, endian);
    if (version != 0) {
        rterror("rt_raster_from_wkb: WKB version %d unsupported", version);
        return 0;
    }

    /* Read other components of raster header */
    rast = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
    if (!rast) {
        rterror("rt_raster_from_wkb: Out of memory allocating raster for wkb input");
        return 0;
    }
    rast->numBands = read_uint16(&ptr, endian);
    rast->scaleX = read_float64(&ptr, endian);
    rast->scaleY = read_float64(&ptr, endian);
    rast->ipX = read_float64(&ptr, endian);
    rast->ipY = read_float64(&ptr, endian);
    rast->skewX = read_float64(&ptr, endian);
    rast->skewY = read_float64(&ptr, endian);
    rast->srid = read_int32(&ptr, endian);
    rast->width = read_uint16(&ptr, endian);
    rast->height = read_uint16(&ptr, endian);

    /* Consistency checking, should have been checked before */
    assert(ptr <= wkbend);

    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster numBands: %d",
            rast->numBands);
    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster scale: %gx%g",
            rast->scaleX, rast->scaleY);
    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster ip: %gx%g",
            rast->ipX, rast->ipY);
    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster skew: %gx%g",
            rast->skewX, rast->skewY);
    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster srid: %d",
            rast->srid);
    RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster dims: %dx%d",
            rast->width, rast->height);
    RASTER_DEBUGF(3, "Parsing raster header finished at wkb position %d (expected 61)",
            d_binptr_to_pos(ptr, wkbend, wkbsize));

    CHECK_BINPTR_POSITION(ptr, wkbend, wkbsize, 61);

    /* Read all bands of raster */

    if (!rast->numBands) {
        /* Here ptr should have been left to right after last used byte */
        if (ptr < wkbend) {
            rtwarn("%d bytes of WKB remained unparsed", wkbend - ptr);
        } else if (ptr > wkbend) {
            /* Easier to get a segfault before I guess */
            rtwarn("We parsed %d bytes more then available!", ptr - wkbend);
        }
        rast->bands = 0;
        return rast;
    }

    /* Now read the bands */
    rast->bands = (rt_band*) rtalloc(sizeof (rt_band) * rast->numBands);
    if (!rast->bands) {
        rterror("rt_raster_from_wkb: Out of memory allocating bands for WKB raster decoding");
        rtdealloc(rast);
        return 0;
    }

    /* ptr should now point to start of first band */
    assert(ptr <= wkbend); /* we should have checked this before */

    for (i = 0; i < rast->numBands; ++i) {
        RASTER_DEBUGF(3, "Parsing band %d from wkb position %d", i,
                d_binptr_to_pos(ptr, wkbend, wkbsize));

        rt_band band = rt_band_from_wkb(rast->width, rast->height,
                &ptr, wkbend, endian);
        if (!band) {
            rterror("rt_raster_from_wkb: Error reading WKB form of band %d", i);
            rtdealloc(rast);
            /* TODO: dealloc any previously allocated band too ! */
            return 0;
        }
        rast->bands[i] = band;
    }

    /* Here ptr should have been left to right after last used byte */
    if (ptr < wkbend) {
        rtwarn("%d bytes of WKB remained unparsed", wkbend - ptr);
    } else if (ptr > wkbend) {
        /* Easier to get a segfault before I guess */
        rtwarn("We parsed %d bytes more then available!",
                ptr - wkbend);
    }

    assert(NULL != rast);
    return rast;
}

rt_raster
rt_raster_from_hexwkb(const char* hexwkb,
        uint32_t hexwkbsize) {
    uint8_t* wkb = NULL;
    uint32_t wkbsize = 0;
    uint32_t i = 0;



    assert(NULL != hexwkb);

    RASTER_DEBUGF(3, "rt_raster_from_hexwkb: input wkb: %s", hexwkb);

    if (hexwkbsize % 2) {
        rterror("rt_raster_from_hexwkb: Raster HEXWKB input must have an even number of characters");
        return 0;
    }
    wkbsize = hexwkbsize / 2;

    wkb = rtalloc(wkbsize);
    if (!wkb) {
        rterror("rt_raster_from_hexwkb: Out of memory allocating memory for decoding HEXWKB");
        return 0;
    }

    for (i = 0; i < wkbsize; ++i) /* parse full hex */ {
        wkb[i] = parse_hex((char*) & (hexwkb[i * 2]));
    }

    rt_raster ret = rt_raster_from_wkb(wkb, wkbsize);

    rtdealloc(wkb); /* as long as rt_raster_from_wkb copies memory */

    return ret;
}

static uint32_t
rt_raster_wkb_size(rt_raster raster) {
    uint32_t size = RT_WKB_HDR_SZ;
    uint16_t i = 0;



    assert(NULL != raster);

    RASTER_DEBUGF(3, "rt_raster_wkb_size: computing size for %d bands",
            raster->numBands);

    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(pixtype);

        RASTER_DEBUGF(3, "rt_raster_wkb_size: adding size of band %d", i);

        if (pixbytes < 1) {
            rterror("rt_raster_wkb_size: Corrupted band: unknown pixtype");
            return 0;
        }

        /* Add space for band type */
        size += 1;

        /* Add space for nodata value */
        size += pixbytes;

        if (band->offline) {
            /* Add space for band number */
            size += 1;

            /* Add space for null-terminated path */
            size += strlen(band->data.offline.path) + 1;
        } else {
            /* Add space for actual data */
            size += pixbytes * raster->width * raster->height;
        }

    }

    return size;
}

uint8_t *
rt_raster_to_wkb(rt_raster raster, uint32_t *wkbsize) {
#if POSTGIS_DEBUG_LEVEL > 0
    const uint8_t *wkbend = NULL;
#endif
    uint8_t *wkb = NULL;
    uint8_t *ptr = NULL;
    uint16_t i = 0;
    uint8_t littleEndian = isMachineLittleEndian();



    assert(NULL != raster);
    assert(NULL != wkbsize);

    RASTER_DEBUG(2, "rt_raster_to_wkb: about to call rt_raster_wkb_size");

    *wkbsize = rt_raster_wkb_size(raster);

    RASTER_DEBUGF(3, "rt_raster_to_wkb: found size: %d", *wkbsize);

    wkb = (uint8_t*) rtalloc(*wkbsize);
    if (!wkb) {
        rterror("rt_raster_to_wkb: Out of memory allocating WKB for raster");
        return 0;
    }

    ptr = wkb;

#if POSTGIS_DEBUG_LEVEL > 2
    wkbend = ptr + (*wkbsize);
#endif
    RASTER_DEBUGF(3, "Writing raster header to wkb on position %d (expected 0)",
            d_binptr_to_pos(ptr, wkbend, *wkbsize));

    /* Write endianness */
    *ptr = littleEndian;
    ptr += 1;

    /* Write version(size - (end - ptr)) */
    write_uint16(&ptr, littleEndian, 0);

    /* Copy header (from numBands up) */
    memcpy(ptr, &(raster->numBands), sizeof (struct rt_raster_serialized_t) - 6);
    ptr += sizeof (struct rt_raster_serialized_t) - 6;

    RASTER_DEBUGF(3, "Writing bands header to wkb position %d (expected 61)",
            d_binptr_to_pos(ptr, wkbend, *wkbsize));

    /* Serialize bands now */
    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(pixtype);

        RASTER_DEBUGF(3, "Writing WKB for band %d", i);
        RASTER_DEBUGF(3, "Writing band pixel type to wkb position %d",
                d_binptr_to_pos(ptr, wkbend, *wkbsize));

        if (pixbytes < 1) {
            rterror("rt_raster_to_wkb: Corrupted band: unknown pixtype");
            return 0;
        }

        /* Add band type */
        *ptr = band->pixtype;
        if (band->offline) *ptr |= BANDTYPE_FLAG_OFFDB;
        if (band->hasnodata) *ptr |= BANDTYPE_FLAG_HASNODATA;
        if (band->isnodata) *ptr |= BANDTYPE_FLAG_ISNODATA;
        ptr += 1;

#if 0 // no padding required for WKB
        /* Add padding (if needed) */
        if (pixbytes > 1) {
            memset(ptr, '\0', pixbytes - 1);
            ptr += pixbytes - 1;
        }
        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!(((uint64_t) ptr) % pixbytes));
#endif

        RASTER_DEBUGF(3, "Writing band nodata to wkb position %d",
                d_binptr_to_pos(ptr, wkbend, *wkbsize));

        /* Add nodata value */
        switch (pixtype) {
            case PT_1BB:
            case PT_2BUI:
            case PT_4BUI:
            case PT_8BUI:
            {
                uint8_t v = band->nodataval;
                *ptr = v;
                ptr += 1;
                break;
            }
            case PT_8BSI:
            {
                int8_t v = band->nodataval;
                *ptr = v;
                ptr += 1;
                break;
            }
            case PT_16BSI:
            case PT_16BUI:
            {
                uint16_t v = band->nodataval;
                memcpy(ptr, &v, 2);
                ptr += 2;
                break;
            }
            case PT_32BSI:
            case PT_32BUI:
            {
                uint32_t v = band->nodataval;
                memcpy(ptr, &v, 4);
                ptr += 4;
                break;
            }
            case PT_32BF:
            {
                float v = band->nodataval;
                memcpy(ptr, &v, 4);
                ptr += 4;
                break;
            }
            case PT_64BF:
            {
                memcpy(ptr, &band->nodataval, 8);
                ptr += 8;
                break;
            }
            default:
                rterror("rt_raster_to_wkb: Fatal error caused by unknown pixel type. Aborting.");
                abort(); /* shoudn't happen */
                return 0;
        }

#if 0 // no padding for WKB
        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uint64_t) ptr % pixbytes));
#endif

        if (band->offline) {
            /* Write band number */
            *ptr = band->data.offline.bandNum;
            ptr += 1;

            /* Write path */
            strcpy((char*) ptr, band->data.offline.path);
            ptr += strlen(band->data.offline.path) + 1;
        } else {
            /* Write data */
            {
                uint32_t datasize = raster->width * raster->height * pixbytes;
                RASTER_DEBUGF(4, "rt_raster_to_wkb: Copying %d bytes", datasize);
                memcpy(ptr, band->data.mem, datasize);
                ptr += datasize;
            }
        }

#if 0 // no padding for WKB
        /* Pad up to 8-bytes boundary */
        while ((uint64_t) ptr % 8) {
            *ptr = 0;
            ++ptr;
        }

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uint64_t) ptr % pixbytes));
#endif
    }

    return wkb;
}

char *
rt_raster_to_hexwkb(rt_raster raster, uint32_t *hexwkbsize) {
    uint8_t *wkb = NULL;
    char* hexwkb = NULL;
    uint32_t i = 0;
    uint32_t wkbsize = 0;



    assert(NULL != raster);
    assert(NULL != hexwkbsize);

    RASTER_DEBUG(2, "rt_raster_to_hexwkb: calling rt_raster_to_wkb");

    wkb = rt_raster_to_wkb(raster, &wkbsize);

    RASTER_DEBUG(3, "rt_raster_to_hexwkb: rt_raster_to_wkb returned");

    *hexwkbsize = wkbsize * 2; /* hex is 2 times bytes */
    hexwkb = (char*) rtalloc((*hexwkbsize) + 1);
    if (!hexwkb) {
        rtdealloc(wkb);
        rterror("rt_raster_to_hexwkb: Out of memory hexifying raster WKB");
        return 0;
    }
    hexwkb[*hexwkbsize] = '\0'; /* Null-terminate */

    for (i = 0; i < wkbsize; ++i) {
        deparse_hex(wkb[i], &(hexwkb[2 * i]));
    }

    rtdealloc(wkb); /* we don't need this anymore */

    RASTER_DEBUGF(3, "rt_raster_to_hexwkb: output wkb: %s", hexwkb);

    return hexwkb;
}

/*--------- Serializer/Deserializer --------------------------------------*/

static uint32_t
rt_raster_serialized_size(rt_raster raster) {
    uint32_t size = sizeof (struct rt_raster_serialized_t);
    uint16_t i = 0;



    assert(NULL != raster);

    RASTER_DEBUGF(3, "Serialized size with just header:%d - now adding size of %d bands",
            size, raster->numBands);

    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(pixtype);

        if (pixbytes < 1) {
            rterror("rt_raster_serialized_size: Corrupted band: unknown pixtype");
            return 0;
        }

        /* Add space for band type, hasnodata flag and data padding */
        size += pixbytes;

        /* Add space for nodata value */
        size += pixbytes;

        if (band->offline) {
            /* Add space for band number */
            size += 1;

            /* Add space for null-terminated path */
            size += strlen(band->data.offline.path) + 1;
        } else {
            /* Add space for raster band data */
            size += pixbytes * raster->width * raster->height;
        }

        RASTER_DEBUGF(3, "Size before alignment is %d", size);

        /* Align size to 8-bytes boundary (trailing padding) */
        /* XXX jorgearevalo: bug here. If the size is actually 8-bytes aligned,
         this line will add 8 bytes trailing padding, and it's not necessary */
        //size += 8 - (size % 8);
        if (size % 8)
            size += 8 - (size % 8);

        RASTER_DEBUGF(3, "Size after alignment is %d", size);
    }

    return size;
}

void*
rt_raster_serialize(rt_raster raster) {
    uint32_t size = rt_raster_serialized_size(raster);
    uint8_t* ret = NULL;
    uint8_t* ptr = NULL;
    uint16_t i = 0;


    assert(NULL != raster);

    ret = (uint8_t*) rtalloc(size);
    if (!ret) {
        rterror("rt_raster_serialize: Out of memory allocating %d bytes for serializing a raster",
                size);
        return 0;
    }
    memset(ret, '-', size);

    ptr = ret;

    RASTER_DEBUGF(3, "sizeof(struct rt_raster_serialized_t):%u",
            sizeof (struct rt_raster_serialized_t));
    RASTER_DEBUGF(3, "sizeof(struct rt_raster_t):%u",
            sizeof (struct rt_raster_t));
    RASTER_DEBUGF(3, "serialized size:%lu", (long unsigned) size);

    /* Set size */
    /* NOTE: Value of rt_raster.size may be updated in
     * returned object, for instance, by rt_pg layer to
     * store value calculated by SET_VARSIZE.
     */
    raster->size = size;

    /* Set version */
    raster->version = 0;

    /* Copy header */
    memcpy(ptr, raster, sizeof (struct rt_raster_serialized_t));

    RASTER_DEBUG(3, "Start hex dump of raster being serialized using 0x2D to mark non-written bytes");

#if POSTGIS_DEBUG_LEVEL > 2
    uint8_t* dbg_ptr = ptr;
    d_print_binary_hex("HEADER", dbg_ptr, size);
#endif

    ptr += sizeof (struct rt_raster_serialized_t);

    /* Serialize bands now */
    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        assert(NULL != band);

        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(pixtype);
        if (pixbytes < 1) {
            rterror("rt_raster_serialize: Corrupted band: unknown pixtype");
            return 0;
        }

        /* Add band type */
        *ptr = band->pixtype;
        if (band->offline) {
            *ptr |= BANDTYPE_FLAG_OFFDB;
        }
        if (band->hasnodata) {
            *ptr |= BANDTYPE_FLAG_HASNODATA;
        }

        if (band->isnodata) {
            *ptr |= BANDTYPE_FLAG_ISNODATA;
        }

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex("PIXTYPE", dbg_ptr, size);
#endif

        ptr += 1;

        /* Add padding (if needed) */
        if (pixbytes > 1) {
            memset(ptr, '\0', pixbytes - 1);
            ptr += pixbytes - 1;
        }

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex("PADDING", dbg_ptr, size);
#endif

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!(((uintptr_t) ptr) % pixbytes));

        /* Add nodata value */
        switch (pixtype) {
            case PT_1BB:
            case PT_2BUI:
            case PT_4BUI:
            case PT_8BUI:
            {
                uint8_t v = band->nodataval;
                *ptr = v;
                ptr += 1;
                break;
            }
            case PT_8BSI:
            {
                int8_t v = band->nodataval;
                *ptr = v;
                ptr += 1;
                break;
            }
            case PT_16BSI:
            case PT_16BUI:
            {
                uint16_t v = band->nodataval;
                memcpy(ptr, &v, 2);
                ptr += 2;
                break;
            }
            case PT_32BSI:
            case PT_32BUI:
            {
                uint32_t v = band->nodataval;
                memcpy(ptr, &v, 4);
                ptr += 4;
                break;
            }
            case PT_32BF:
            {
                float v = band->nodataval;
                memcpy(ptr, &v, 4);
                ptr += 4;
                break;
            }
            case PT_64BF:
            {
                memcpy(ptr, &band->nodataval, 8);
                ptr += 8;
                break;
            }
            default:
                rterror("rt_raster_serialize: Fatal error caused by unknown pixel type. Aborting.");
                abort(); /* shouldn't happen */
                return 0;
        }

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uintptr_t) ptr % pixbytes));

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex("nodata", dbg_ptr, size);
#endif

        if (band->offline) {
            /* Write band number */
            *ptr = band->data.offline.bandNum;
            ptr += 1;

            /* Write path */
            strcpy((char*) ptr, band->data.offline.path);
            ptr += strlen(band->data.offline.path) + 1;
        } else {
            /* Write data */
            uint32_t datasize = raster->width * raster->height * pixbytes;
            memcpy(ptr, band->data.mem, datasize);
            ptr += datasize;
        }

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex("BAND", dbg_ptr, size);
#endif

        /* Pad up to 8-bytes boundary */
        while ((uintptr_t) ptr % 8) {
            *ptr = 0;
            ++ptr;

            RASTER_DEBUGF(3, "PAD at %d", (uint64_t) ptr % 8);

        }

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uintptr_t) ptr % pixbytes));

    } /* for-loop over bands */

#if POSTGIS_DEBUG_LEVEL > 2
    d_print_binary_hex("SERIALIZED RASTER", dbg_ptr, size);
#endif

    return ret;
}

rt_raster
rt_raster_deserialize(void* serialized) {
    rt_raster rast = NULL;
    const uint8_t *ptr = NULL;
    const uint8_t *beg = NULL;
    uint16_t i = 0;
    uint8_t littleEndian = isMachineLittleEndian();



    assert(NULL != serialized);

    RASTER_DEBUG(2, "rt_raster_deserialize: Entering...");

    /* NOTE: Value of rt_raster.size may be different
     * than actual size of raster data being read.
     * See note on SET_VARSIZE in rt_raster_serialize function above.
     */

    /* Allocate memory for deserialized raster header */

    RASTER_DEBUG(3, "rt_raster_deserialize: Allocationg memory for deserialized raster header");
    rast = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
    if (!rast) {
        rterror("rt_raster_deserialize: Out of memory allocating raster for deserialization");
        return 0;
    }

    /* Deserialize raster header */
    RASTER_DEBUG(3, "rt_raster_deserialize: Deserialize raster header");
    memcpy(rast, serialized, sizeof (struct rt_raster_serialized_t));

    if (0 == rast->numBands) {
        rast->bands = 0;
        return rast;
    }
    beg = (const uint8_t*) serialized;

    RASTER_DEBUG(3, "rt_raster_deserialize: Allocating memory for bands");
    /* Allocate registry of raster bands */
    rast->bands = rtalloc(rast->numBands * sizeof (rt_band));

    RASTER_DEBUGF(3, "rt_raster_deserialize: %d bands", rast->numBands);

    /* Move to the beginning of first band */
    ptr = beg;
    ptr += sizeof (struct rt_raster_serialized_t);

    /* Deserialize bands now */
    for (i = 0; i < rast->numBands; ++i) {
        rt_band band = NULL;
        uint8_t type = 0;
        int pixbytes = 0;

        band = rtalloc(sizeof (struct rt_band_t));
        if (!band) {
            rterror("rt_raster_deserialize: Out of memory allocating rt_band during deserialization");
            return 0;
        }

        rast->bands[i] = band;

        type = *ptr;
        ptr++;
        band->pixtype = type & BANDTYPE_PIXTYPE_MASK;

        RASTER_DEBUGF(3, "rt_raster_deserialize: band %d with pixel type %s", i, rt_pixtype_name(band->pixtype));

        band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
        band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
        band->isnodata = BANDTYPE_IS_NODATA(type) ? 1 : 0;
        band->width = rast->width;
        band->height = rast->height;
        band->ownsData = 0;

        /* Advance by data padding */
        pixbytes = rt_pixtype_size(band->pixtype);
        ptr += pixbytes - 1;

        /* Read nodata value */
        switch (band->pixtype) {
            case PT_1BB:
            {
                band->nodataval = ((int) read_uint8(&ptr)) & 0x01;
                break;
            }
            case PT_2BUI:
            {
                band->nodataval = ((int) read_uint8(&ptr)) & 0x03;
                break;
            }
            case PT_4BUI:
            {
                band->nodataval = ((int) read_uint8(&ptr)) & 0x0F;
                break;
            }
            case PT_8BSI:
            {
                band->nodataval = read_int8(&ptr);
                break;
            }
            case PT_8BUI:
            {
                band->nodataval = read_uint8(&ptr);
                break;
            }
            case PT_16BSI:
            {
                band->nodataval = read_int16(&ptr, littleEndian);
                break;
            }
            case PT_16BUI:
            {
                band->nodataval = read_uint16(&ptr, littleEndian);
                break;
            }
            case PT_32BSI:
            {
                band->nodataval = read_int32(&ptr, littleEndian);
                break;
            }
            case PT_32BUI:
            {
                band->nodataval = read_uint32(&ptr, littleEndian);
                break;
            }
            case PT_32BF:
            {
                band->nodataval = read_float32(&ptr, littleEndian);
                break;
            }
            case PT_64BF:
            {
                band->nodataval = read_float64(&ptr, littleEndian);
                break;
            }
            default:
            {
                rterror("rt_raster_deserialize: Unknown pixeltype %d", band->pixtype);
                rtdealloc(band);
                rtdealloc(rast);
                return 0;
            }
        }

        RASTER_DEBUGF(3, "rt_raster_deserialize: has nodata flag %d", band->hasnodata);
        RASTER_DEBUGF(3, "rt_raster_deserialize: nodata value %g", band->nodataval);

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!(((uintptr_t) ptr) % pixbytes));

        if (band->offline) {
            /* Read band number */
            band->data.offline.bandNum = *ptr;
            ptr += 1;

            /* Register path */
            band->data.offline.path = (char*) ptr;
            ptr += strlen(band->data.offline.path) + 1;
        } else {
            /* Register data */
            const uint32_t datasize = rast->width * rast->height * pixbytes;
            band->data.mem = (uint8_t*) ptr;
            ptr += datasize;
        }

        /* Skip bytes of padding up to 8-bytes boundary */
#if POSTGIS_DEBUG_LEVEL > 0
        const uint8_t *padbeg = ptr;
#endif
        while (0 != ((ptr - beg) % 8)){
            ++ptr;
        }

        RASTER_DEBUGF(3, "rt_raster_deserialize: skip %d bytes of 8-bytes boundary padding", ptr - padbeg);

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uintptr_t) ptr % pixbytes));
    }

    return rast;
}

int rt_raster_is_empty(rt_raster raster) {


    return (NULL == raster || raster->height <= 0 || raster->width <= 0);
}

int rt_raster_has_no_band(rt_raster raster, int nband) {


    return (NULL == raster || raster->numBands < nband);
}

int32_t rt_raster_copy_band(rt_raster torast,
        rt_raster fromrast, int fromindex, int toindex)
{
    rt_band newband = NULL;


    assert(NULL != torast);
    assert(NULL != fromrast);

    /* Check raster dimensions */
    if (torast->height != fromrast->height || torast->width != fromrast->width)
    {
        rtwarn("rt_raster_copy_band: Attempting to add a band with different width or height");
        return -1;
    }

    /* Check bands limits */
    if (fromrast->numBands < 1)
    {
        rtwarn("rt_raster_copy_band: Second raster has no band");
        return -1;
    }
    else if (fromindex < 0)
    {
        rtwarn("rt_raster_copy_band: Band index for second raster < 0. Defaulted to 1");
        fromindex = 0;
    }
    else if (fromindex >= fromrast->numBands)
    {
        rtwarn("rt_raster_copy_band: Band index for second raster > number of bands, truncated from %u to %u", fromindex - 1, fromrast->numBands);
        fromindex = fromrast->numBands - 1;
    }

    if (toindex < 0)
    {
        rtwarn("rt_raster_copy_band: Band index for first raster < 0. Defaulted to 1");
        toindex = 0;
    }
    else if (toindex > torast->numBands)
    {
        rtwarn("rt_raster_copy_band: Band index for first raster > number of bands, truncated from %u to %u", toindex - 1, torast->numBands);
        toindex = torast->numBands;
    }

    /* Get band from source raster */
    newband = rt_raster_get_band(fromrast, fromindex);

    /* Add band to the second raster */
    return rt_raster_add_band(torast, newband, toindex);
}

/**
 * Construct a new rt_raster from an existing rt_raster and an array
 * of band numbers
 *
 * @param raster : the source raster
 * @param bandNums : array of band numbers to extract from source raster
 *                   and add to the new raster (0 based)
 * @param count : number of elements in bandNums
 *
 * @return a new rt_raster or 0 on error
 */
rt_raster
rt_raster_from_band(rt_raster raster, uint32_t *bandNums, int count) {
	rt_raster rast = NULL;
	int i = 0;
	int idx;
	int32_t flag;

	assert(NULL != raster);
	assert(NULL != bandNums);

	RASTER_DEBUGF(3, "rt_raster_from_band: source raster has %d bands",
		rt_raster_get_num_bands(raster));

	/* create new raster */
	rast = rt_raster_new(raster->width, raster->height);
	if (NULL == rast) {
		rterror("rt_raster_from_band: Out of memory allocating new raster\n");
		return 0;
	}

	/* copy raster attributes */
	/* scale */
	rt_raster_set_scale(rast, raster->scaleX, raster->scaleY);
	/* offset */
	rt_raster_set_offsets(rast, raster->ipX, raster->ipY);
	/* skew */
	rt_raster_set_skews(rast, raster->skewX, raster->skewY);
	/* srid */
	rt_raster_set_srid(rast, raster->srid);

	/* copy bands */
	for (i = 0; i < count; i++) {
		idx = bandNums[i];
		flag = rt_raster_copy_band(rast, raster, idx, i);

		if (flag < 0) {
			rterror("rt_raster_from_band: Unable to copy band\n");
			rt_raster_destroy(rast);
			return 0;
		}

		RASTER_DEBUGF(3, "rt_raster_from_band: band created at index %d",
			flag);
	}

	RASTER_DEBUGF(3, "rt_raster_from_band: new raster has %d bands",
		rt_raster_get_num_bands(rast));
	return rast;
}

/**
 * Replace band at provided index with new band
 * 
 * @param raster: raster of band to be replaced
 * @param band : new band to add to raster
 * @param index : index of band to replace (1-based)
 *
 * @return 0 on error or replaced band
 */
int
rt_raster_replace_band(rt_raster raster, rt_band band, int index) {
	rt_band oldband = NULL;
	assert(NULL != raster);

	if (band->width != raster->width || band->height != raster->height) {
		rterror("rt_raster_replace_band: Band does not match raster's dimensions: %dx%d band to %dx%d raster",
			band->width, band->height, raster->width, raster->height);
		return 0;
	}

	if (index > raster->numBands || index < 0) {
		rterror("rt_raster_replace_band: Band index is not valid");
		return 0;
	}

	oldband = rt_raster_get_band(raster, index);
	RASTER_DEBUGF(3, "rt_raster_replace_band: old band at %p", oldband);
	RASTER_DEBUGF(3, "rt_raster_replace_band: new band at %p", band);

	raster->bands[index] = band;
	RASTER_DEBUGF(3, "rt_raster_replace_band: new band at %p", raster->bands[index]);

	rt_band_destroy(oldband);
	return 1;
}

/**
 * Return formatted GDAL raster from raster
 *
 * @param raster : the raster to convert
 * @param srs : the raster's coordinate system in OGC WKT or PROJ.4
 * @param format : format to convert to. GDAL driver short name
 * @param options : list of format creation options. array of strings
 * @param gdalsize : will be set to the size of returned bytea
 *
 * @return formatted GDAL raster
 */
uint8_t*
rt_raster_to_gdal(rt_raster raster, char *srs,
	char *format, char **options, uint64_t *gdalsize) {
	GDALDriverH src_drv = NULL;
	GDALDatasetH src_ds = NULL;

	vsi_l_offset rtn_lenvsi;
	uint64_t rtn_len = 0;

	GDALDriverH rtn_drv = NULL;
	GDALDatasetH rtn_ds = NULL;
	uint8_t *rtn = NULL;

	assert(NULL != raster);

	/* any supported format is possible */
	GDALAllRegister();
	RASTER_DEBUG(3, "rt_raster_to_gdal: loaded all supported GDAL formats");

	/* output format not specified */
	if (format == NULL || !strlen(format))
		format = "GTiff";
	RASTER_DEBUGF(3, "rt_raster_to_gdal: output format is %s", format);

	/* load raster into a GDAL MEM raster */
	src_ds = rt_raster_to_gdal_mem(raster, srs, NULL, 0, &src_drv);
	if (NULL == src_ds) {
		rterror("rt_raster_to_gdal: Unable to convert raster to GDAL MEM format\n");
		if (NULL != src_drv) {
			GDALDeregisterDriver(src_drv);
			GDALDestroyDriver(src_drv);
		}
		return 0;
	}

	/* load driver */
	rtn_drv = GDALGetDriverByName(format);
	if (NULL == rtn_drv) {
		rterror("rt_raster_to_gdal: Unable to load the output GDAL driver\n");
		GDALClose(src_ds);
		GDALDeregisterDriver(src_drv);
		GDALDestroyDriver(src_drv);
		return 0;
	}
	RASTER_DEBUG(3, "rt_raster_to_gdal: Output driver loaded");

	/* convert GDAL MEM raster to output format */
	RASTER_DEBUG(3, "rt_raster_to_gdal: Copying GDAL MEM raster to memory file in output format");
	rtn_ds = GDALCreateCopy(
		rtn_drv,
		"/vsimem/out.dat", /* should be fine assuming this is in a process */
		src_ds,
		FALSE, /* should copy be strictly equivelent? */
		options, /* format options */
		NULL, /* progress function */
		NULL /* progress data */
	);
	if (NULL == rtn_ds) {
		rterror("rt_raster_to_gdal: Unable to create the output GDAL dataset\n");
		GDALClose(src_ds);
		GDALDeregisterDriver(rtn_drv);
		GDALDestroyDriver(rtn_drv);
		GDALDeregisterDriver(src_drv);
		GDALDestroyDriver(src_drv);
		return 0;
	}

	/* close source dataset */
	GDALClose(src_ds);
	GDALDeregisterDriver(src_drv);
	GDALDestroyDriver(src_drv);
	RASTER_DEBUG(3, "rt_raster_to_gdal: Closed GDAL MEM raster");

	/* close dataset, this also flushes any pending writes */
	GDALClose(rtn_ds);
	GDALDeregisterDriver(rtn_drv);
	GDALDestroyDriver(rtn_drv);
	RASTER_DEBUG(3, "rt_raster_to_gdal: Closed GDAL output raster");

	RASTER_DEBUG(3, "rt_raster_to_gdal: Done copying GDAL MEM raster to memory file in output format");

	/* from memory file to buffer */
	RASTER_DEBUG(3, "rt_raster_to_gdal: Copying GDAL memory file to buffer");
	rtn = VSIGetMemFileBuffer("/vsimem/out.dat", &rtn_lenvsi, TRUE);
	RASTER_DEBUG(3, "rt_raster_to_gdal: Done copying GDAL memory file to buffer");
	if (NULL == rtn) {
		rterror("rt_raster_to_gdal: Unable to create the output GDAL raster\n");
		return 0;
	}

	rtn_len = (uint64_t) rtn_lenvsi;
	*gdalsize = rtn_len;

	return rtn;
}

struct rt_gdaldriver_t {
    int idx;
    char *short_name;
    char *long_name;
		char *create_options;
};

/**
 * Returns a set of available GDAL drivers
 *
 * @param drv_count : number of GDAL drivers available
 *
 * @return set of "gdaldriver" values of available GDAL drivers
 */
rt_gdaldriver
rt_raster_gdal_drivers(uint32_t *drv_count) {
	const char *state;
	const char *txt;
	int txt_len;
	GDALDriverH *drv = NULL;
	rt_gdaldriver rtn = NULL;
	int count;
	int i;
	uint32_t j;

	GDALAllRegister();
	count = GDALGetDriverCount();
	rtn = (rt_gdaldriver) rtalloc(count * sizeof(struct rt_gdaldriver_t));
	if (NULL == rtn) {
		rterror("rt_raster_gdal_drivers: Unable to allocate memory for gdaldriver structure");
		return 0;
	}

	for (i = 0, j = 0; i < count; i++) {
		drv = GDALGetDriver(i);

		/* CreateCopy support */
		state = GDALGetMetadataItem(drv, GDAL_DCAP_CREATECOPY, NULL);
		if (state == NULL) continue;

		/* VirtualIO support */
		state = GDALGetMetadataItem(drv, GDAL_DCAP_VIRTUALIO, NULL);
		if (state == NULL) continue;

		/* index of driver */
		rtn[j].idx = i;

		/* short name */
		txt = GDALGetDriverShortName(drv);
		txt_len = strlen(txt);

		RASTER_DEBUGF(3, "rt_raster_gdal_driver: driver %s (%d) supports CreateCopy() and VirtualIO()", txt, i);

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].short_name = (char *) rtalloc(txt_len);
		memcpy(rtn[j].short_name, txt, txt_len);

		/* long name */
		txt = GDALGetDriverLongName(drv);
		txt_len = strlen(txt);

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].long_name = (char *) rtalloc(txt_len);
		memcpy(rtn[j].long_name, txt, txt_len);

		/* creation options */
		txt = GDALGetDriverCreationOptionList(drv);
		txt_len = strlen(txt);

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].create_options = (char *) rtalloc(txt_len);
		memcpy(rtn[j].create_options, txt, txt_len);

		j++;
	}

	/* free unused memory */
	rtrealloc(rtn, j * sizeof(struct rt_gdaldriver_t));
	*drv_count = j;

	return rtn;
}

/**
 * Return GDAL dataset using GDAL MEM driver from raster
 *
 * @param raster : raster to convert to GDAL MEM
 * @param srs : the raster's coordinate system in OGC WKT or PROJ.4
 * @param bandNums : array of band numbers to extract from raster
 *                   and include in the GDAL dataset (0 based)
 * @param count : number of elements in bandNums
 * @param rtn_drv : is set to the GDAL driver object
 *
 * @return GDAL dataset using GDAL MEM driver
 */
GDALDatasetH
rt_raster_to_gdal_mem(rt_raster raster, char *srs,
	uint32_t *bandNums, int count, GDALDriverH *rtn_drv) {
	GDALDriverH drv = NULL;
	int drv_gen = 0;
	GDALDatasetH ds = NULL;
	double gt[] = {0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
	CPLErr cplerr;
	GDALDataType gdal_pt = GDT_Unknown;
	GDALRasterBandH band;
	void *pVoid;
	char *pszDataPointer;
	char szGDALOption[50];
	char *apszOptions[4];
	double nodata = 0.0;
	int allocBandNums = 0;

	int i;
	int numBands;
	rt_band rtband = NULL;
	rt_pixtype pt = PT_END;

	assert(NULL != raster);

	/* store raster in GDAL MEM raster */
	GDALRegister_MEM();
	if (NULL == *rtn_drv) {
		drv = GDALGetDriverByName("MEM");
		if (NULL == drv) {
			rterror("rt_raster_to_gdal_mem: Unable to load the MEM GDAL driver\n");
			return 0;
		}

		*rtn_drv = drv;
		drv_gen = 1;
	}

	ds = GDALCreate(drv, "", rt_raster_get_width(raster),
		rt_raster_get_height(raster), 0, GDT_Byte, NULL);
	if (NULL == ds) {
		rterror("rt_raster_to_gdal_mem: Couldn't create a GDALDataset to convert it\n");
		if (drv_gen) {
			GDALDeregisterDriver(drv);
			GDALDestroyDriver(drv);
		}
		return 0;
	}

	/* add geotransform */
	gt[0] = rt_raster_get_x_offset(raster);
	gt[1] = rt_raster_get_x_scale(raster);
	gt[2] = rt_raster_get_x_skew(raster);
	gt[3] = rt_raster_get_y_offset(raster);
	gt[4] = rt_raster_get_y_skew(raster);
	gt[5] = rt_raster_get_y_scale(raster);
	cplerr = GDALSetGeoTransform(ds, gt);
	if (cplerr != CE_None) {
		rterror("rt_raster_to_gdal_mem: Unable to set geotransformation\n");
		GDALClose(ds);
		if (drv_gen) {
			GDALDeregisterDriver(drv);
			GDALDestroyDriver(drv);
		}
		return 0;
	}

	/* set spatial reference */
	if (NULL != srs && strlen(srs)) {
		cplerr = GDALSetProjection(ds, srs);
		if (cplerr != CE_None) {
			rterror("rt_raster_to_gdal_mem: Unable to set projection\n");
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}
	}

	/* process bandNums */
	numBands = rt_raster_get_num_bands(raster);
	if (NULL != bandNums && count > 0) {
		for (i = 0; i < count; i++) {
			if (bandNums[i] < 0 || bandNums[i] >= numBands) {
				rterror("rt_raster_to_gdal_mem: The band index %d is invalid\n", bandNums[i]);
				GDALClose(ds);
				if (drv_gen) {
					GDALDeregisterDriver(drv);
					GDALDestroyDriver(drv);
				}
				return 0;
			}
		}
	}
	else {
		count = numBands;
		bandNums = (uint32_t *) rtalloc(sizeof(uint32_t) * count);
		if (NULL == bandNums) {
			rterror("rt_raster_to_gdal_mem: Unable to allocate memory for band indices\n");
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}
		allocBandNums = 1;
		for (i = 0; i < count; i++) bandNums[i] = i;
	}
	
	/* add band(s) */
	for (i = 0; i < numBands; i++) {
		rtband = rt_raster_get_band(raster, bandNums[i]);
		if (NULL == rtband) {
			rterror("rt_raster_to_gdal_mem: Unable to get requested band\n");
			if (allocBandNums) rtdealloc(bandNums);
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}

		pt = rt_band_get_pixtype(rtband);
		switch (pt) {
			case PT_1BB: case PT_2BUI: case PT_4BUI: case PT_8BSI: case PT_8BUI:
				gdal_pt = GDT_Byte;
				break;
			case PT_16BSI: case PT_16BUI:
				if (pt == PT_16BSI)
					gdal_pt = GDT_Int16;
				else
					gdal_pt = GDT_UInt16;
				break;
			case PT_32BSI: case PT_32BUI: case PT_32BF:
				if (pt == PT_32BSI)
					gdal_pt = GDT_Int32;
				else if (pt == PT_32BUI)
					gdal_pt = GDT_UInt32;
				else
					gdal_pt = GDT_Float32;
				break;
			case PT_64BF:
				gdal_pt = GDT_Float64;
				break;
			default:
				rtwarn("rt_raster_to_gdal_mem: Unknown pixel type for band\n");
				gdal_pt = GDT_Unknown;
				break;
		}

		pVoid = rt_band_get_data(rtband);
		RASTER_DEBUGF(4, "Band data is at pos %p", pVoid);

		pszDataPointer = (char *) rtalloc(20 * sizeof (char));
		sprintf(pszDataPointer, "%p", pVoid);
		RASTER_DEBUGF(4, "rt_raster_to_gdal_mem: szDatapointer is %p",
			pszDataPointer);

		if (strnicmp(pszDataPointer, "0x", 2) == 0)
			sprintf(szGDALOption, "DATAPOINTER=%s", pszDataPointer);
		else
			sprintf(szGDALOption, "DATAPOINTER=0x%s", pszDataPointer);

		RASTER_DEBUG(3, "Storing info for GDAL MEM raster band");

		apszOptions[0] = szGDALOption;
		apszOptions[1] = NULL; /* pixel offset, not needed */
		apszOptions[2] = NULL; /* line offset, not needed */
		apszOptions[3] = NULL;

		/* free */
		rtdealloc(pszDataPointer);

		if (GDALAddBand(ds, gdal_pt, apszOptions) == CE_Failure) {
			rterror("rt_raster_to_gdal_mem: Couldn't add GDALRasterBand\n");
			if (allocBandNums) rtdealloc(bandNums);
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}

		/* check band count */
		if (GDALGetRasterCount(ds) != i + 1) {
			rterror("rt_raster_to_gdal_mem: Error creating GDAL MEM raster band\n");
			if (allocBandNums) rtdealloc(bandNums);
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}

		/* get band to write data to */
		band = NULL;
		band = GDALGetRasterBand(ds, i + 1);
		if (NULL == band) {
			rterror("rt_raster_to_gdal_mem: Couldn't get GDAL band for additional processing\n");
			if (allocBandNums) rtdealloc(bandNums);
			GDALClose(ds);
			if (drv_gen) {
				GDALDeregisterDriver(drv);
				GDALDestroyDriver(drv);
			}
			return 0;
		}

		/* Add nodata value for band */
		if (rt_band_get_hasnodata_flag(rtband) != FALSE) {
			nodata = rt_band_get_nodata(rtband);
			RASTER_DEBUGF(3, "Setting nodata value to %f", nodata);
			if (GDALSetRasterNoDataValue(band, nodata) != CE_None)
				rtwarn("rt_raster_to_gdal_mem: Couldn't set nodata value for band\n");
		}
	}

	return ds;
}

/**
 * Return a raster from a GDAL dataset
 *
 * @param ds : the GDAL dataset to convert to a raster
 *
 * @return raster
 */
rt_raster
rt_raster_from_gdal_dataset(GDALDatasetH ds) {
	rt_raster rast = NULL;
	double gt[6] = {0};
	CPLErr cplerr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numBands = 0;
	int i = 0;
	int *status = NULL;

	GDALRasterBandH gdband = NULL;
	GDALDataType gdpixtype = GDT_Unknown;
	rt_band band;
	int32_t idx;
	rt_pixtype pt = PT_END;
	uint32_t hasnodata = 0;
	double nodataval;

	int x;
	int y;
	double value;

	int nXBlocks, nYBlocks;
	int nXBlockSize, nYBlockSize;
	int iXBlock, iYBlock;
	int nXValid, nYValid;
	int iY, iX;
	double *values = NULL;
	int byblock = 0;

	assert(NULL != ds);

	/* raster size */
	width = GDALGetRasterXSize(ds);
	height = GDALGetRasterYSize(ds);
	RASTER_DEBUGF(3, "Raster dimensions (width x height): %d x %d", width, height);

	/* create new raster */
	RASTER_DEBUG(3, "Creating new raster");
	rast = rt_raster_new(width, height);
	if (NULL == rast) {
		rterror("rt_raster_from_gdal_dataset: Out of memory allocating new raster\n");
		return NULL;
	}

	/* get raster attributes */
	cplerr = GDALGetGeoTransform(ds, gt);
	if (cplerr != CE_None) {
		rterror("rt_raster_from_gdal_dataset: Unable to get geotransformation\n");
		rt_raster_destroy(rast);
		return NULL;
	}
	RASTER_DEBUGF(3, "Raster geotransform (%f, %f, %f, %f, %f, %f)",
		gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);

	/* apply raster attributes */
	/* scale */
	rt_raster_set_scale(rast, gt[1], gt[5]);
	/* offset */
	rt_raster_set_offsets(rast, gt[0], gt[3]);
	/* skew */
	rt_raster_set_skews(rast, gt[2], gt[4]);
	/* srid */
	/* CANNOT SET srid AS srid IS NOT AVAILABLE IN GDAL dataset */

	/* copy bands */
	numBands = GDALGetRasterCount(ds);
	for (i = 1; i <= numBands; i++) {
		RASTER_DEBUGF(3, "Processing band %d of %d", i, numBands);
		gdband = NULL;
		gdband = GDALGetRasterBand(ds, i);
		if (NULL == gdband) {
			rterror("rt_raster_from_gdal_dataset: Unable to get transformed band\n");
			rt_raster_destroy(rast);
			return NULL;
		}

		/* pixtype */
		gdpixtype = GDALGetRasterDataType(gdband);
		switch (gdpixtype) {
			case GDT_Byte:
				pt = PT_8BUI;
				RASTER_DEBUG(3, "Pixel type is GDT_Byte");
				break;
			case GDT_UInt16:
				pt = PT_16BUI;
				RASTER_DEBUG(3, "Pixel type is GDT_UInt16");
				break;
			case GDT_Int16:
				pt = PT_16BSI;
				RASTER_DEBUG(3, "Pixel type is GDT_Int16");
				break;
			case GDT_UInt32:
				pt = PT_32BUI;
				RASTER_DEBUG(3, "Pixel type is GDT_UInt32");
				break;
			case GDT_Int32:
				pt = PT_32BSI;
				RASTER_DEBUG(3, "Pixel type is GDT_Int32");
				break;
			case GDT_Float32:
				pt = PT_32BF;
				RASTER_DEBUG(3, "Pixel type is GDT_Float32");
				break;
			case GDT_Float64:
				pt = PT_64BF;
				RASTER_DEBUG(3, "Pixel type is GDT_Float64");
				break;
			default:
				rterror("rt_raster_from_gdal_dataset: Unknown pixel type for transformed raster\n");
				rt_raster_destroy(rast);
				return NULL;
				break;
		}

		/* size: width and height */
		width = GDALGetRasterBandXSize(gdband);
		height = GDALGetRasterBandYSize(gdband);
		RASTER_DEBUGF(3, "Band dimensions (width x height): %d x %d", width, height);

		/* nodata */
		nodataval = GDALGetRasterNoDataValue(gdband, status);
		if (NULL == status || !*status) {
			nodataval = 0;
			hasnodata = 0;
		}
		else
			hasnodata = 1;
		RASTER_DEBUGF(3, "(hasnodata, nodataval) = (%d, %f)", hasnodata, nodataval);

		idx = rt_raster_generate_new_band(
			rast, pt,
			(hasnodata ? nodataval : 0),
			hasnodata, nodataval, rt_raster_get_num_bands(rast)
		);
		if (idx < 0) {
			rterror("rt_raster_from_gdal_dataset: Could not allocate memory for band\n");
			rt_raster_destroy(rast);
			return NULL;
		}
		band = rt_raster_get_band(rast, idx);

		/* copy data from gdband to band */
		/* try to duplicate entire band data at once */
		do {
			values = rtalloc(width * height * sizeof(double));
			if (NULL == values) {
				byblock = 1;
				break;
			}

			cplerr = GDALRasterIO(
				gdband, GF_Read,
				0, 0,
				width, height,
				values, width, height,
				GDT_Float64,
				0, 0
			);

			if (cplerr != CE_None) {
				byblock = 1;
				rtdealloc(values);
				break;
			}

			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					value = values[x + y * width];

					RASTER_DEBUGF(5, "(x, y, value) = (%d, %d, %f)", x, y, value); 

					if (rt_band_set_pixel(band, x, y, value) < 0) {
						rterror("rt_raster_from_gdal_dataset: Unable to save data from transformed raster\n");
						rt_raster_destroy(rast);
						return NULL;
					}
				}
			}

			rtdealloc(values);
		}
		while (0);

		/* this makes use of GDAL's "natural" blocks */
		if (byblock) {
			GDALGetBlockSize(gdband, &nXBlockSize, &nYBlockSize);
			nXBlocks = (width + nXBlockSize - 1) / nXBlockSize;
			nYBlocks = (height + nYBlockSize - 1) / nYBlockSize;
			RASTER_DEBUGF(4, "(nXBlockSize, nYBlockSize) = (%d, %d)", nXBlockSize, nYBlockSize);
			RASTER_DEBUGF(4, "(nXBlocks, nYBlocks) = (%d, %d)", nXBlocks, nYBlocks);

			values = rtalloc(nXBlockSize * nYBlockSize * sizeof(double));
			for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
				for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
					x = iXBlock * nXBlockSize;
					y = iYBlock * nYBlockSize;

					cplerr = GDALRasterIO(
						gdband, GF_Read,
						x, y,
						nXBlockSize, nYBlockSize,
						values, nXBlockSize, nYBlockSize,
						GDT_Float64,
						0, 0
					);
					if (cplerr != CE_None) {
						rterror("rt_raster_from_gdal_dataset: Unable to get data from transformed raster\n");
						rt_raster_destroy(rast);
						return NULL;
					}

					if ((iXBlock + 1) * nXBlockSize > width)
						nXValid = width - (iXBlock * nXBlockSize);
					else
						nXValid = nXBlockSize;

					if ((iYBlock + 1) * nYBlockSize > height)
						nYValid = height - (iYBlock * nYBlockSize);
					else
						nYValid = nYBlockSize;

					for (iY = 0; iY < nYValid; iY++) {
						for (iX = 0; iX < nXValid; iX++) {
							x = iX + (nXBlockSize * iXBlock);
							y = iY + (nYBlockSize * iYBlock);
							value = values[iX + iY * nXBlockSize];

							RASTER_DEBUGF(5, "(x, y, value) = (%d, %d, %f)", x, y, value);

							if (rt_band_set_pixel(band, x, y, value) < 0) {
								rterror("rt_raster_from_gdal_dataset: Unable to save data from transformed raster\n");
								rt_raster_destroy(rast);
								return NULL;
							}
						}
					}
				}
			}

			rtdealloc(values);
		}
	}

	return rast;
}
