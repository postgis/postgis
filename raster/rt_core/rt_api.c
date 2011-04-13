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
#include <stdio.h>  /* for printf (default message handler) */
#include <stdarg.h> /* for va_list, va_start etc */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <float.h> /* for FLT_EPSILON and float type limits */
#include <limits.h> /* for integer type limits */
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

/*- rt_context -------------------------------------------------------*/

static void
default_error_handler(const char *fmt, ...) {
    va_list ap;

    static const char *label = "ERROR: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    va_start(ap, fmt);

    vprintf(newfmt, ap);

    va_end(ap);
}

static void
default_warning_handler(const char *fmt, ...) {
    va_list ap;

    static const char *label = "WARNING: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    va_start(ap, fmt);

    vprintf(newfmt, ap);

    va_end(ap);
}

static void
default_info_handler(const char *fmt, ...) {
    va_list ap;

    static const char *label = "INFO: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    va_start(ap, fmt);

    vprintf(newfmt, ap);

    va_end(ap);
}

struct rt_context_t {
    rt_allocator alloc;
    rt_reallocator realloc;
    rt_deallocator dealloc;
    rt_message_handler err;
    rt_message_handler warn;
    rt_message_handler info;
};

rt_context
rt_context_new(rt_allocator allocator, rt_reallocator reallocator,
        rt_deallocator deallocator) {
    rt_context ret;

    if (!allocator) allocator = malloc;
    if (!reallocator) reallocator = realloc;
    if (!deallocator) deallocator = free;

    ret = (rt_context) allocator(sizeof (struct rt_context_t));
    if (!ret) {
        default_error_handler("Out of virtual memory creating an rt_context");
        return 0;
    }

    // Can not be used here
    //RASTER_DEBUGF(3, "Created rt_context @ %p", ret);

    ret->alloc = allocator;
    ret->realloc = reallocator;
    ret->dealloc = deallocator;
    ret->err = default_error_handler;
    ret->warn = default_warning_handler;
    ret->info = default_info_handler;

    assert(NULL != ret->alloc);
    assert(NULL != ret->realloc);
    assert(NULL != ret->dealloc);
    assert(NULL != ret->err);
    assert(NULL != ret->warn);
    assert(NULL != ret->info);
    return ret;
}

void
rt_context_set_message_handlers(rt_context ctx,
        rt_message_handler error_handler,
        rt_message_handler warning_handler,
        rt_message_handler info_handler) {
    ctx->err = error_handler;
    ctx->warn = warning_handler;
    ctx->info = info_handler;

    assert(NULL != ctx->err);
    assert(NULL != ctx->warn);
    assert(NULL != ctx->info);
}

void
rt_context_destroy(rt_context ctx) {
    RASTER_DEBUGF(3, "Destroying rt_context @ %p", ctx);

    ctx->dealloc(ctx);
}

int
rt_util_display_dbl_trunc_warning(rt_context ctx,
                                  double initialvalue,
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
                ctx->warn("Value set for %s band got clamped from %f to %d",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvalint);
                result = -1;
            }
            else if (fabs(checkvalint - initialvalue) > FLT_EPSILON) {
                ctx->warn("Value set for %s band got truncated from %f to %d",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvalint);
                result = -1;
            }
            break;
        }
        case PT_32BUI:
        {
            if (fabs(checkvaluint - initialvalue) >= 1) {
                ctx->warn("Value set for %s band got clamped from %f to %u",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvaluint);
                result = -1;
            }
            else if (fabs(checkvaluint - initialvalue) > FLT_EPSILON) {
                ctx->warn("Value set for %s band got truncated from %f to %u",
                    rt_pixtype_name(ctx, pixtype),
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
                ctx->warn("Value set for %s band got converted from %f to %f",
                    rt_pixtype_name(ctx, pixtype),
                    initialvalue, checkvalfloat);
            break;
        }
        case PT_64BF:
        {
            if (fabs(checkvaldouble - initialvalue) > FLT_EPSILON)
                ctx->warn("Value set for %s band got converted from %f to %f",
                    rt_pixtype_name(ctx, pixtype),
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
d_binary_to_hex(rt_context ctx, const uint8_t * const raw, uint32_t size, uint32_t *hexsize) {
    char* hex = NULL;
    uint32_t i = 0;

    assert(NULL != ctx);
    assert(NULL != raw);
    assert(NULL != hexsize);

    *hexsize = size * 2; /* hex is 2 times bytes */
    hex = (char*) ctx->alloc((*hexsize) + 1);
    if (!hex) {
        ctx->err("d_binary_to_hex: Out of memory hexifying raw binary\n");
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
d_print_binary_hex(rt_context ctx, const char* msg, const uint8_t * const raw, uint32_t size) {
    char* hex = NULL;
    uint32_t hexsize = 0;

    assert(NULL != ctx);
    assert(NULL != msg);
    assert(NULL != raw);

    hex = d_binary_to_hex(ctx, raw, size, &hexsize);
    if (NULL != hex) {
        ctx->info("%s\t%s", msg, hex);
        ctx->dealloc(hex);
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
rt_pixtype_size(rt_context ctx, rt_pixtype pixtype) {
    int pixbytes = -1;

    assert(NULL != ctx);

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
            ctx->err("rt_pixtype_size: Unknown pixeltype %d", pixtype);
            pixbytes = -1;
            break;
    }

    RASTER_DEBUGF(3, "Pixel type = %s and size = %d bytes",
            rt_pixtype_name(ctx, pixtype), pixbytes);


    return pixbytes;
}

int
rt_pixtype_alignment(rt_context ctx, rt_pixtype pixtype) {
    return rt_pixtype_size(ctx, pixtype);
}

rt_pixtype
rt_pixtype_index_from_name(rt_context ctx, const char* pixname) {
    assert(strlen(pixname) > 0);

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
rt_pixtype_name(rt_context ctx, rt_pixtype pixtype) {
    assert(NULL != ctx);

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
            ctx->err("rt_pixtype_name: Unknown pixeltype %d", pixtype);
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
rt_band_new_inline(rt_context ctx, uint16_t width, uint16_t height,
        rt_pixtype pixtype, uint32_t hasnodata, double nodataval,
        uint8_t* data) {
    rt_band band = NULL;

    assert(NULL != ctx);
    assert(NULL != data);

    band = ctx->alloc(sizeof (struct rt_band_t));
    if (!band) {
        ctx->err("rt_band_new_inline: Out of memory allocating rt_band");
        return 0;
    }

    RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s",
            band, rt_pixtype_name(ctx, pixtype));


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
rt_band_new_offline(rt_context ctx, uint16_t width, uint16_t height,
        rt_pixtype pixtype, uint32_t hasnodata, double nodataval,
        uint8_t bandNum, const char* path) {
    rt_band band = NULL;

    assert(NULL != ctx);
    assert(NULL != path);

    band = ctx->alloc(sizeof (struct rt_band_t));
    if (!band) {
        ctx->err("rt_band_new_offline: Out of memory allocating rt_band");
        return 0;
    }


    RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s",
            band, rt_pixtype_name(ctx, pixtype));

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
rt_band_is_offline(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    return band->offline;
}

void
rt_band_destroy(rt_context ctx, rt_band band) {
    RASTER_DEBUGF(3, "Destroying rt_band @ %p", band);


    /* band->data content is externally owned */
    /* XXX jorgearevalo: not really... rt_band_from_wkb allocates memory for
     * data.mem
     */
    ctx->dealloc(band);
}

const char*
rt_band_get_ext_path(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    if (!band->offline) {
        RASTER_DEBUG(3, "rt_band_get_ext_path: non-offline band doesn't have "
                "an associated path");
        return 0;
    }
    return band->data.offline.path;
}

uint8_t
rt_band_get_ext_band_num(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    if (!band->offline) {
        RASTER_DEBUG(3, "rt_band_get_ext_path: non-offline band doesn't have "
                "an associated band number");
        return 0;
    }
    return band->data.offline.bandNum;
}

void *
rt_band_get_data(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    if (band->offline) {
        RASTER_DEBUG(3, "rt_band_get_data: "
                "offline band doesn't have associated data");
        return 0;
    }
    return band->data.mem;
}

rt_pixtype
rt_band_get_pixtype(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    return band->pixtype;
}

uint16_t
rt_band_get_width(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    return band->width;
}

uint16_t
rt_band_get_height(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
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
        ctx->warn("Pixel value for %d-bits band got truncated"
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
rt_band_get_hasnodata_flag(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    return band->hasnodata;
}

void
rt_band_set_hasnodata_flag(rt_context ctx, rt_band band, int flag) {
    assert(NULL != ctx);
    assert(NULL != band);

    band->hasnodata = (flag) ? 1 : 0;
}

void
rt_band_set_isnodata_flag(rt_context ctx, rt_band band, int flag) {
    assert(NULL != ctx);
    assert(NULL != band);

    band->isnodata = (flag) ? 1 : 0;
}

int
rt_band_get_isnodata_flag(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    return band->isnodata;
}

int
rt_band_set_nodata(rt_context ctx, rt_band band, double val) {
    rt_pixtype pixtype = PT_END;
    //double oldnodataval = band->nodataval;

    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    float checkvalfloat = 0;
    double checkvaldouble = 0;

    assert(NULL != ctx);
    assert(NULL != band);

    pixtype = band->pixtype;

    RASTER_DEBUGF(3, "rt_band_set_nodata: setting nodata value %g with band type %s", val, rt_pixtype_name(ctx, pixtype));

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
            ctx->err("rt_band_set_nodata: Unknown pixeltype %d", pixtype);
            band->hasnodata = 0;
            return -1;
        }
    }

    RASTER_DEBUGF(3, "rt_band_set_nodata: band->hasnodata = %d", band->hasnodata);
    RASTER_DEBUGF(3, "rt_band_set_nodata: band->nodataval = %d", band->nodataval);


    // the nodata value was just set, so this band has NODATA
    rt_band_set_hasnodata_flag(ctx, band, 1);

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    if (rt_util_display_dbl_trunc_warning(ctx, val, checkvalint, checkvaluint, checkvalfloat,
                                      checkvaldouble, pixtype))
        return -1;
#endif

    /* If the nodata value is different from the previous one, we need to check
     * again if the band is a nodata band
     * TODO: NO, THAT'S TOO SLOW!!!
     */

    /*
    if (fabs(band->nodataval - oldnodataval) > FLT_EPSILON)
        rt_band_check_is_nodata(ctx, band);
    */

    return 0;
}

int
rt_band_set_pixel(rt_context ctx, rt_band band, uint16_t x, uint16_t y,
        double val) {
    rt_pixtype pixtype = PT_END;
    unsigned char* data = NULL;
    uint32_t offset = 0;

    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    float checkvalfloat = 0;
    double checkvaldouble = 0;

    double checkval = 0;

    assert(NULL != ctx);
    assert(NULL != band);

    pixtype = band->pixtype;

    if (x >= band->width || y >= band->height) {
        ctx->err("rt_band_set_pixel: Coordinates out of range");
        return -1;
    }

    if (band->offline) {
        ctx->err("rt_band_set_pixel not implemented yet for OFFDB bands");
        return -1;
    }

    data = rt_band_get_data(ctx, band);
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
            ctx->err("rt_band_set_pixel: Unknown pixeltype %d", pixtype);
            return -1;
        }
    }

    /* Overflow checking */
#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    if (rt_util_display_dbl_trunc_warning(ctx, val, checkvalint, checkvaluint, checkvalfloat,
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
        rt_band_check_is_nodata(ctx, band);
    }
    */


    return 0;
}

int
rt_band_get_pixel(rt_context ctx, rt_band band, uint16_t x, uint16_t y, double *result) {
    rt_pixtype pixtype = PT_END;
    uint8_t* data = NULL;
    uint32_t offset = 0;

    assert(NULL != ctx);
    assert(NULL != band);

    pixtype = band->pixtype;

    if (x >= band->width || y >= band->height) {
        ctx->warn("Attempting to get pixel value with out of range raster coordinates");
        return -1;
    }

    if (band->offline) {
        ctx->err("rt_band_get_pixel not implemented yet for OFFDB bands");
        return -1;
    }

    data = rt_band_get_data(ctx, band);
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
            ctx->err("rt_band_get_pixel: Unknown pixeltype %d", pixtype);
            return -1;
        }
    }
}

double
rt_band_get_nodata(rt_context ctx, rt_band band) {
    assert(NULL != ctx);
    assert(NULL != band);

    if (!band->hasnodata)
        RASTER_DEBUGF(3, "Getting nodata value for a band without NODATA values. Using %g", band->nodataval);


    return band->nodataval;
}

double
rt_band_get_min_value(rt_context ctx, rt_band band) {
    rt_pixtype pixtype = PT_END;

    assert(NULL != ctx);
    assert(NULL != band);

    pixtype = band->pixtype;

    switch (pixtype) {
        case PT_1BB: case PT_2BUI: case PT_4BUI: case PT_8BUI:
        {
            return (double)CHAR_MIN;
        }
        case PT_8BSI:
        {
            return (double)SCHAR_MIN;
        }
        case PT_16BSI: case PT_16BUI:
        {
            return (double)SHRT_MIN;
        }
        case PT_32BSI: case PT_32BUI:
        {
            return (double)INT_MIN;
        }
        case PT_32BF:
        {
            return (double)FLT_MIN;
        }
        case PT_64BF:
        {
            return (double)DBL_MIN;
        }
        default:
        {
            ctx->err("rt_band_get_min_value: Unknown pixeltype %d", pixtype);
            return (double)CHAR_MIN;
        }
    }
}


int
rt_band_check_is_nodata(rt_context ctx, rt_band band)
{
    int i, j;
    double pxValue = band->nodataval;
    double dEpsilon = 0.0;

    assert(NULL != ctx);
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
            rt_band_get_pixel(ctx, band, i, j, &pxValue);
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
rt_raster_new(rt_context ctx, uint16_t width, uint16_t height) {
    rt_raster ret = NULL;

    assert(NULL != ctx);
    assert(NULL != ctx->alloc);

    ret = (rt_raster) ctx->alloc(sizeof (struct rt_raster_t));
    if (!ret) {
        ctx->err("rt_raster_new: Out of virtual memory creating an rt_raster");
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
rt_raster_destroy(rt_context ctx, rt_raster raster) {
    RASTER_DEBUGF(3, "Destroying rt_raster @ %p", ctx);

    if (raster->bands) {
        ctx->dealloc(raster->bands);
    }
    ctx->dealloc(raster);
}

uint16_t
rt_raster_get_width(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->width;
}

uint16_t
rt_raster_get_height(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->height;
}

void
rt_raster_set_scale(rt_context ctx, rt_raster raster,
        double scaleX, double scaleY) {
    assert(NULL != ctx);
    assert(NULL != raster);

    raster->scaleX = scaleX;
    raster->scaleY = scaleY;
}

double
rt_raster_get_x_scale(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->scaleX;
}

double
rt_raster_get_y_scale(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->scaleY;
}

void
rt_raster_set_skews(rt_context ctx, rt_raster raster,
        double skewX, double skewY) {
    assert(NULL != ctx);
    assert(NULL != raster);

    raster->skewX = skewX;
    raster->skewY = skewY;
}

double
rt_raster_get_x_skew(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->skewX;
}

double
rt_raster_get_y_skew(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->skewY;
}

void
rt_raster_set_offsets(rt_context ctx, rt_raster raster, double x, double y) {
    assert(NULL != ctx);
    assert(NULL != raster);

    raster->ipX = x;
    raster->ipY = y;
}

double
rt_raster_get_x_offset(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->ipX;
}

double
rt_raster_get_y_offset(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->ipY;
}

int32_t
rt_raster_get_srid(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->srid;
}

void
rt_raster_set_srid(rt_context ctx, rt_raster raster, int32_t srid) {
    assert(NULL != ctx);
    assert(NULL != raster);

    raster->srid = srid;
}

int
rt_raster_get_num_bands(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);
    assert(NULL != raster);

    return raster->numBands;
}

rt_band
rt_raster_get_band(rt_context ctx, rt_raster raster, int n) {
    assert(NULL != ctx);
    assert(NULL != raster);

    if (n >= raster->numBands || n < 0) return 0;
    return raster->bands[n];
}

int32_t
rt_raster_add_band(rt_context ctx, rt_raster raster, rt_band band, int index) {
    rt_band *oldbands = NULL;
    rt_band oldband = NULL;
    rt_band tmpband = NULL;
    uint16_t i = 0;

    assert(NULL != ctx);
    assert(NULL != raster);

    RASTER_DEBUGF(3, "Adding band %p to raster %p", band, raster);

    if (band->width != raster->width || band->height != raster->height) {
        ctx->err("rt_raster_add_band: Can't add a %dx%d band to a %dx%d raster",
                band->width, band->height, raster->width, raster->height);
        return -1;
    }

    if (index > raster->numBands)
        index = raster->numBands;

    if (index < 0)
        index = 0;

    oldbands = raster->bands;

    RASTER_DEBUGF(3, "Oldbands at %p", oldbands);

    raster->bands = (rt_band*) ctx->realloc(raster->bands,
            sizeof (rt_band)*(raster->numBands + 1)
            );

    if (!raster->bands) {
        ctx->err("rt_raster_add_band: Out of virtual memory "
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
rt_raster_generate_new_band(rt_context ctx, rt_raster raster, rt_pixtype pixtype,
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


    assert(NULL != ctx);
    assert(NULL != raster);

    /* Make sure index is in a valid range */
    oldnumbands = rt_raster_get_num_bands(ctx, raster);
    if (index < 0)
        index = 0;
    else if (index > rt_raster_get_num_bands(ctx, raster) + 1)
        index = rt_raster_get_num_bands(ctx, raster) + 1;

    /* Determine size of memory block to allocate and allocate it */
    width = rt_raster_get_width(ctx, raster);
    height = rt_raster_get_height(ctx, raster);
    numval = width * height;
    datasize = rt_pixtype_size(ctx, pixtype) * numval;

    mem = (int *)ctx->alloc(datasize);
    if (!mem) {
        ctx->err("rt_raster_generate_new_band: Could not allocate memory for band");
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
                ctx->err("rt_raster_generate_new_band: Unknown pixeltype %d", pixtype);
                ctx->dealloc(mem);
                return -1;
            }
        }
    }

#ifdef POSTGIS_RASTER_WARN_ON_TRUNCATION
    /* Overflow checking */
    rt_util_display_dbl_trunc_warning(ctx, initialvalue, checkvalint, checkvaluint, checkvalfloat,
                                      checkvaldouble, pixtype);
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    band = rt_band_new_inline(ctx, width, height, pixtype, hasnodata, nodatavalue, mem);
    if (! band) {
        ctx->err("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        ctx->dealloc(mem);
        return -1;
    }
    index = rt_raster_add_band(ctx, raster, band, index);
    numbands = rt_raster_get_num_bands(ctx, raster);
    if (numbands == oldnumbands || index == -1) {
        ctx->err("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        rt_band_destroy(ctx, band);
    }

    return index;
}


void
rt_raster_cell_to_geopoint(rt_context ctx, rt_raster raster,
        double x, double y,
        double* x1, double* y1) {
    assert(NULL != ctx);
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
rt_raster_dump_as_wktpolygons(rt_context ctx, rt_raster raster, int nband,
        int * pnElements) {

	char * pszQuery;
    char * pszDataPointer;
    char szGdalOption[50];
    long j;
    GDALDataType nPixelType = GDT_Unknown;
    char * apszOptions[4];
    OGRSFDriverH ogr_drv = NULL;
    GDALDriverH gdal_drv = NULL;
    GDALDatasetH memdataset = NULL;
    GDALRasterBandH gdal_band = NULL;
    OGRDataSourceH memdatasource = NULL;
    rt_pixtype pt = PT_END;
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

    /* Checkings */
    assert(NULL != ctx);
    assert(NULL != raster);
    assert(nband > 0 && nband <= rt_raster_get_num_bands(ctx, raster));

    RASTER_DEBUG(2, "In rt_raster_dump_as_polygons");

    /*******************************
     * Get band
     *******************************/
    band = rt_raster_get_band(ctx, raster, nband - 1);
    if (NULL == band) {
        ctx->err("rt_raster_dump_as_wktpolygons: Error getting band %d from raster", nband);
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
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't create a OGR Datasource to store pols\n");
        return 0;
    }

    /**
     * Can MEM driver create new layers?
     **/
    if (!OGR_DS_TestCapability(memdatasource, ODsCCreateLayer)) {
        ctx->err("rt_raster_dump_as_wktpolygons: MEM driver can't create new layers, aborting\n");
        /* xxx jorgearevalo: what should we do now? */
        OGRReleaseDataSource(memdatasource);
        return 0;
    }

    RASTER_DEBUG(3, "creating GDAL MEM raster");


    /****************************************************************
     * Create a GDAL MEM raster with one band, from rt_band object
     ****************************************************************/
    GDALRegister_MEM();


    /**
     * First, create a Dataset with no bands using MEM driver
     **/
    gdal_drv = GDALGetDriverByName("MEM");
    memdataset = GDALCreate(gdal_drv, "", rt_band_get_width(ctx, band),
            rt_band_get_height(ctx, band), 0, GDT_Byte, NULL);

    if (NULL == memdataset) {
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't create a GDALDataset to polygonize it\n");
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);
        OGRReleaseDataSource(memdatasource);
        return 0;
    }

    /**
     * Add geotransform
     */
    double adfGeoTransform[6] = {0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    adfGeoTransform[0] = rt_raster_get_x_offset(ctx, raster);
    adfGeoTransform[1] = rt_raster_get_x_scale(ctx, raster);
    adfGeoTransform[2] = rt_raster_get_x_skew(ctx, raster);
    adfGeoTransform[3] = rt_raster_get_y_offset(ctx, raster);
    adfGeoTransform[4] = rt_raster_get_y_skew(ctx, raster);
    adfGeoTransform[5] = rt_raster_get_y_scale(ctx, raster);
    GDALSetGeoTransform(memdataset, adfGeoTransform);

    RASTER_DEBUG(3, "Adding GDAL MEM raster band");

    /**
     * Now, add the raster band
     */
    pt = rt_band_get_pixtype(ctx, band);

    switch (pt) {
        case PT_1BB: case PT_2BUI: case PT_4BUI: case PT_8BSI: case PT_8BUI:
            nPixelType = GDT_Byte;
            break;

        case PT_16BSI: case PT_16BUI:
            if (pt == PT_16BSI)
                nPixelType = GDT_Int16;
            else
                nPixelType = GDT_UInt16;
            break;

        case PT_32BSI: case PT_32BUI: case PT_32BF:
            if (pt == PT_32BSI)
                nPixelType = GDT_Int32;
            else if (pt == PT_32BUI)
                nPixelType = GDT_UInt32;
            else
                nPixelType = GDT_Float32;
            break;

        case PT_64BF:
            nPixelType = GDT_Float64;
            break;

        default:
            ctx->warn("Unknown pixel type for band\n");
            nPixelType = GDT_Unknown;
            break;
    }

    void * pVoid = rt_band_get_data(ctx, band);

    RASTER_DEBUGF(4, "Band data is at pos %p", pVoid);

    /**
     * Be careful!! If this pointer is defined as szDataPointer[20]
     * the sprintf crash! Probable because the memory is taken from
     * an invalid memory context for PostgreSQL.
     * And be careful with size too: 10 characters may be insufficient
     * to store 64bits memory addresses
     */
    pszDataPointer = (char *) ctx->alloc(20 * sizeof (char));
    sprintf(pszDataPointer, "%p", pVoid);

    RASTER_DEBUGF(4, "rt_raster_dump_as_polygons: szDatapointer is %p",
            pszDataPointer);

    if (strnicmp(pszDataPointer, "0x", 2) == 0)
        sprintf(szGdalOption, "DATAPOINTER=%s", pszDataPointer);
    else
        sprintf(szGdalOption, "DATAPOINTER=0x%s", pszDataPointer);

    RASTER_DEBUG(3, "Storing info for GDAL MEM raster band");

    apszOptions[0] = szGdalOption;
    apszOptions[1] = NULL; // pixel offset, not needed
    apszOptions[2] = NULL; // line offset, not needed
    apszOptions[3] = NULL;

    /**
     * This memory must be deallocated because we own it. The GDALRasterBand
     * destructor will not deallocate it
     **/
    ctx->dealloc(pszDataPointer);

    if (GDALAddBand(memdataset, nPixelType, apszOptions) == CE_Failure) {
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't transform raster band in GDALRasterBand format to polygonize it");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);
        OGRReleaseDataSource(memdatasource);

        return 0;
    }

    /* Checking */
    if (GDALGetRasterCount(memdataset) != 1) {
        ctx->err("rt_raster_dump_as_wktpolygons: Error creating GDAL MEM raster bands");
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
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't create layer to store polygons");
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
    hFldDfn = OGR_Fld_Create("PixelValue", OFTInteger);

    /* Second, create the field */
    if (OGR_L_CreateField(hLayer, hFldDfn, TRUE) !=
            OGRERR_NONE) {
        ctx->warn("Couldn't create a field in OGR Layer. The polygons generated won't be able to store the pixel value");
        iPixVal = -1;
    }
    else {
        /* Index to the new field created in the layer */
        iPixVal = 0;
    }


    /* Get GDAL raster band */
    gdal_band = GDALGetRasterBand(memdataset, 1);
    if (NULL == gdal_band) {
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't get GDAL band to polygonize");
        GDALClose(memdataset);
        GDALDeregisterDriver(gdal_drv);
        GDALDestroyDriver(gdal_drv);

        OGR_Fld_Destroy(hFldDfn);
        OGR_DS_DeleteLayer(memdatasource, 0);
        OGRReleaseDataSource(memdatasource);

        return 0;
    }

    iBandHasNodataValue = rt_band_get_hasnodata_flag(ctx, band);
    if (iBandHasNodataValue) {
        /* Add nodata value for band */
        dBandNoData = rt_band_get_nodata(ctx, band);
        if (GDALSetRasterNoDataValue(gdal_band, dBandNoData) != CE_None)
            ctx->warn("Couldn't set nodata value for band.");
    }


    /**
     * We don't need a raster mask band. Each band has a nodata value.
     **/
    GDALPolygonize(gdal_band, NULL, hLayer, iPixVal, NULL, NULL, NULL);

	/**
	 * Optimization: Apply a OGR SQL filter to the layer to select the
 	 * features different from NODATA value.
 	 *
 	 * Thanks to David Zwarg.
 	 **/
	if (iBandHasNodataValue) {
		pszQuery = (char *) ctx->alloc(50 * sizeof (char));
		sprintf(pszQuery, "PixelValue != %f", dBandNoData );		
		OGRErr e = OGR_L_SetAttributeFilter(hLayer, pszQuery);
		if (e != OGRERR_NONE) {
			ctx->warn("Error filtering NODATA values for band. All values will be treated as data values\n");
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
     * XXX jorgearevalo: modify GDALPolygonize to directly emit polygons
     * in WKT format?
     *********************************************************************/
    nFeatureCount = OGR_L_GetFeatureCount(hLayer, TRUE);

    /* Allocate memory for pols */
    pols = (rt_geomval) ctx->alloc(nFeatureCount * sizeof (struct rt_geomval_t));

    if (NULL == pols) {
        ctx->err("rt_raster_dump_as_wktpolygons: Couldn't allocate memory for geomval structure");
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
       	pols[j].srid = rt_raster_get_srid(ctx, raster);
     	pols[j].geom = (char *) ctx->alloc((1 + strlen(pszSrcText))
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
         //ctx->dealloc(pszSrcText);
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
rt_raster_get_convex_hull(rt_context ctx, rt_raster raster) {
    POINTARRAY **rings = NULL;
    POINTARRAY *pts = NULL;
    LWPOLY* ret = NULL;
    POINT4D p4d;

    assert(NULL != ctx);
    assert(NULL != raster);

    RASTER_DEBUGF(3, "rt_raster_get_convex_hull: raster is %dx%d",
            raster->width, raster->height);

    if ((!raster->width) || (!raster->height)) {
        return 0;
    }

    rings = (POINTARRAY **) ctx->alloc(sizeof (POINTARRAY*));
    if (!rings) {
        ctx->err("rt_raster_get_convex_hull: Out of memory [%s:%d]", __FILE__, __LINE__);
        return 0;
    }
    rings[0] = ptarray_construct(0, 0, 5);
    /* TODO: handle error on ptarray construction */
    /* XXX jorgearevalo: the error conditions aren't managed in ptarray_construct */
    if (!rings[0]) {
        ctx->err("rt_raster_get_convex_hull: Out of memory [%s:%d]", __FILE__, __LINE__);
        return 0;
    }
    pts = rings[0];

    /* Upper-left corner (first and last points) */
    rt_raster_cell_to_geopoint(ctx, raster,
            0, 0,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 0, &p4d);
    ptarray_set_point4d(pts, 4, &p4d); /* needed for closing it? */

    /* Upper-right corner (we go clockwise) */
    rt_raster_cell_to_geopoint(ctx, raster,
            raster->width, 0,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 1, &p4d);

    /* Lower-right corner */
    rt_raster_cell_to_geopoint(ctx, raster,
            raster->width, raster->height,
            &p4d.x, &p4d.y);
    ptarray_set_point4d(pts, 2, &p4d);

    /* Lower-left corner */
    rt_raster_cell_to_geopoint(ctx, raster,
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
rt_band_from_wkb(rt_context ctx, uint16_t width, uint16_t height,
        const uint8_t** ptr, const uint8_t* end,
        uint8_t littleEndian) {
    rt_band band = NULL;
    int pixbytes = 0;
    uint8_t type = 0;
    unsigned long sz = 0;
    uint32_t v = 0;

    assert(NULL != ctx);
    assert(NULL != ptr);
    assert(NULL != end);

    band = ctx->alloc(sizeof (struct rt_band_t));
    if (!band) {
        ctx->err("rt_band_from_wkb: Out of memory allocating rt_band during WKB parsing");
        return 0;
    }

    if (end - *ptr < 1) {
        ctx->err("rt_band_from_wkb: Premature end of WKB on band reading (%s:%d)",
                __FILE__, __LINE__);
        return 0;
    }
    type = read_uint8(ptr);

    if ((type & BANDTYPE_PIXTYPE_MASK) >= PT_END) {
        ctx->err("rt_band_from_wkb: Invalid pixtype %d", type & BANDTYPE_PIXTYPE_MASK);
        ctx->dealloc(band);
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
            rt_pixtype_name(ctx, band->pixtype),
            band->offline,
            band->hasnodata);

    /* Check there's enough bytes to read nodata value */

    pixbytes = rt_pixtype_size(ctx, band->pixtype);
    if (((*ptr) + pixbytes) >= end) {
        ctx->err("rt_band_from_wkb: Premature end of WKB on band novalue reading");
        ctx->dealloc(band);
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
            ctx->err("rt_band_from_wkb: Unknown pixeltype %d", band->pixtype);
            ctx->dealloc(band);
            return 0;
        }
    }

    RASTER_DEBUGF(3, " Nodata value: %g, pixbytes: %d, ptr @ %p, end @ %p",
            band->nodataval, pixbytes, *ptr, end);

    if (band->offline) {
        if (((*ptr) + 1) >= end) {
            ctx->err("rt_band_from_wkb: Premature end of WKB on offline "
                    "band data bandNum reading (%s:%d)",
                    __FILE__, __LINE__);
            ctx->dealloc(band);
            return 0;
        }
        band->data.offline.bandNum = read_int8(ptr);

        {
            /* check we have a NULL-termination */
            sz = 0;
            while ((*ptr)[sz] && &((*ptr)[sz]) < end) ++sz;
            if (&((*ptr)[sz]) >= end) {
                ctx->err("rt_band_from_wkb: Premature end of WKB on band offline path reading");
                ctx->dealloc(band);
                return 0;
            }

            band->ownsData = 1;
            band->data.offline.path = ctx->alloc(sz + 1);

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
        ctx->err("rt_band_from_wkb: Premature end of WKB on band data reading (%s:%d)",
                __FILE__, __LINE__);
        ctx->dealloc(band);
        return 0;
    }

    band->data.mem = ctx->alloc(sz);
    if (!band->data.mem) {
        ctx->err("rt_band_from_wkb: Out of memory during band creation in WKB parser");
        ctx->dealloc(band);
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
                    ctx->err("rt_band_from_wkb: Unexpected pix bytes %d", pixbytes);
                    ctx->dealloc(band);
                    ctx->dealloc(band->data.mem);
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
                    ctx->err("rt_band_from_wkb: Invalid value %d for pixel of type %s",
                            val, rt_pixtype_name(ctx, band->pixtype));
                    ctx->dealloc(band->data.mem);
                    ctx->dealloc(band);
                    return 0;
                }
            }
        }
    }

    /* And we should check if the band is a nodata band */
    /* TODO: No!! This is too slow */
    //rt_band_check_is_nodata(ctx, band);

    return band;
}

/* -4 for size, +1 for endian */
#define RT_WKB_HDR_SZ (sizeof(struct rt_raster_serialized_t)-4+1)

rt_raster
rt_raster_from_wkb(rt_context ctx, const uint8_t* wkb, uint32_t wkbsize) {
    const uint8_t *ptr = wkb;
    const uint8_t *wkbend = NULL;
    rt_raster rast = NULL;
    uint8_t endian = 0;
    uint16_t version = 0;
    uint16_t i = 0;

    assert(NULL != ctx);
    assert(NULL != ptr);

    /* Check that wkbsize is >= sizeof(rt_raster_serialized) */
    if (wkbsize < RT_WKB_HDR_SZ) {
        ctx->err("rt_raster_from_wkb: wkb size < min size (%d)",
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
        ctx->err("rt_raster_from_wkb: WKB version %d unsupported", version);
        return 0;
    }

    /* Read other components of raster header */
    rast = (rt_raster) ctx->alloc(sizeof (struct rt_raster_t));
    if (!rast) {
        ctx->err("rt_raster_from_wkb: Out of memory allocating raster for wkb input");
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
            ctx->info("%d bytes of WKB remained unparsed", wkbend - ptr);
        } else if (ptr > wkbend) {
            /* Easier to get a segfault before I guess */
            ctx->warn("We parsed %d bytes more then available!", ptr - wkbend);
        }
        rast->bands = 0;
        return rast;
    }

    /* Now read the bands */
    rast->bands = (rt_band*) ctx->alloc(sizeof (rt_band) * rast->numBands);
    if (!rast->bands) {
        ctx->err("rt_raster_from_wkb: Out of memory allocating bands for WKB raster decoding");
        ctx->dealloc(rast);
        return 0;
    }

    /* ptr should now point to start of first band */
    assert(ptr <= wkbend); /* we should have checked this before */

    for (i = 0; i < rast->numBands; ++i) {
        RASTER_DEBUGF(3, "Parsing band %d from wkb position %d", i,
                d_binptr_to_pos(ptr, wkbend, wkbsize));

        rt_band band = rt_band_from_wkb(ctx, rast->width, rast->height,
                &ptr, wkbend, endian);
        if (!band) {
            ctx->err("rt_raster_from_wkb: Error reading WKB form of band %d", i);
            ctx->dealloc(rast);
            /* TODO: dealloc any previously allocated band too ! */
            return 0;
        }
        rast->bands[i] = band;
    }

    /* Here ptr should have been left to right after last used byte */
    if (ptr < wkbend) {
        ctx->info("%d bytes of WKB remained unparsed", wkbend - ptr);
    } else if (ptr > wkbend) {
        /* Easier to get a segfault before I guess */
        ctx->warn("We parsed %d bytes more then available!",
                ptr - wkbend);
    }

    assert(NULL != rast);
    return rast;
}

rt_raster
rt_raster_from_hexwkb(rt_context ctx, const char* hexwkb,
        uint32_t hexwkbsize) {
    uint8_t* wkb = NULL;
    uint32_t wkbsize = 0;
    uint32_t i = 0;

    assert(NULL != ctx);
    assert(NULL != hexwkb);

    RASTER_DEBUGF(3, "rt_raster_from_hexwkb: input wkb: %s", hexwkb);

    if (hexwkbsize % 2) {
        ctx->err("rt_raster_from_hexwkb: Raster HEXWKB input must have an even number of characters");
        return 0;
    }
    wkbsize = hexwkbsize / 2;

    wkb = ctx->alloc(wkbsize);
    if (!wkb) {
        ctx->err("rt_raster_from_hexwkb: Out of memory allocating memory for decoding HEXWKB");
        return 0;
    }

    for (i = 0; i < wkbsize; ++i) /* parse full hex */ {
        wkb[i] = parse_hex((char*) & (hexwkb[i * 2]));
    }

    rt_raster ret = rt_raster_from_wkb(ctx, wkb, wkbsize);

    ctx->dealloc(wkb); /* as long as rt_raster_from_wkb copies memory */

    return ret;
}

static uint32_t
rt_raster_wkb_size(rt_context ctx, rt_raster raster) {
    uint32_t size = RT_WKB_HDR_SZ;
    uint16_t i = 0;

    assert(NULL != ctx);
    assert(NULL != raster);

    RASTER_DEBUGF(3, "rt_raster_wkb_size: computing size for %d bands",
            raster->numBands);

    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(ctx, pixtype);

        RASTER_DEBUGF(3, "rt_raster_wkb_size: adding size of band %d", i);

        if (pixbytes < 1) {
            ctx->err("rt_raster_wkb_size: Corrupted band: unknown pixtype");
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
rt_raster_to_wkb(rt_context ctx, rt_raster raster, uint32_t *wkbsize) {
#if POSTGIS_DEBUG_LEVEL > 0
    const uint8_t *wkbend = NULL;
#endif
    uint8_t *wkb = NULL;
    uint8_t *ptr = NULL;
    uint16_t i = 0;
    uint8_t littleEndian = isMachineLittleEndian();

    assert(NULL != ctx);
    assert(NULL != raster);
    assert(NULL != wkbsize);

    RASTER_DEBUG(2, "rt_raster_to_wkb: about to call rt_raster_wkb_size");

    *wkbsize = rt_raster_wkb_size(ctx, raster);

    RASTER_DEBUGF(3, "rt_raster_to_wkb: found size: %d", *wkbsize);

    wkb = (uint8_t*) ctx->alloc(*wkbsize);
    if (!wkb) {
        ctx->err("rt_raster_to_wkb: Out of memory allocating WKB for raster");
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
        int pixbytes = rt_pixtype_size(ctx, pixtype);

        RASTER_DEBUGF(3, "Writing WKB for band %d", i);
        RASTER_DEBUGF(3, "Writing band pixel type to wkb position %d",
                d_binptr_to_pos(ptr, wkbend, *wkbsize));

        if (pixbytes < 1) {
            ctx->err("rt_raster_to_wkb: Corrupted band: unknown pixtype");
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
                ctx->err("rt_raster_to_wkb: Fatal error caused by unknown pixel type. Aborting.");
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
rt_raster_to_hexwkb(rt_context ctx, rt_raster raster, uint32_t *hexwkbsize) {
    uint8_t *wkb = NULL;
    char* hexwkb = NULL;
    uint32_t i = 0;
    uint32_t wkbsize = 0;

    assert(NULL != ctx);
    assert(NULL != raster);
    assert(NULL != hexwkbsize);

    RASTER_DEBUG(2, "rt_raster_to_hexwkb: calling rt_raster_to_wkb");

    wkb = rt_raster_to_wkb(ctx, raster, &wkbsize);

    RASTER_DEBUG(3, "rt_raster_to_hexwkb: rt_raster_to_wkb returned");

    *hexwkbsize = wkbsize * 2; /* hex is 2 times bytes */
    hexwkb = (char*) ctx->alloc((*hexwkbsize) + 1);
    if (!hexwkb) {
        ctx->dealloc(wkb);
        ctx->err("rt_raster_to_hexwkb: Out of memory hexifying raster WKB");
        return 0;
    }
    hexwkb[*hexwkbsize] = '\0'; /* Null-terminate */

    for (i = 0; i < wkbsize; ++i) {
        deparse_hex(wkb[i], &(hexwkb[2 * i]));
    }

    ctx->dealloc(wkb); /* we don't need this anymore */

    RASTER_DEBUGF(3, "rt_raster_to_hexwkb: output wkb: %s", hexwkb);

    return hexwkb;
}

/*--------- Serializer/Deserializer --------------------------------------*/

static uint32_t
rt_raster_serialized_size(rt_context ctx, rt_raster raster) {
    uint32_t size = sizeof (struct rt_raster_serialized_t);
    uint16_t i = 0;

    assert(NULL != ctx);
    assert(NULL != raster);

    RASTER_DEBUGF(3, "Serialized size with just header:%d - now adding size of %d bands",
            size, raster->numBands);

    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(ctx, pixtype);

        if (pixbytes < 1) {
            ctx->err("rt_raster_serialized_size: Corrupted band: unknown pixtype");
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
rt_raster_serialize(rt_context ctx, rt_raster raster) {
    uint32_t size = rt_raster_serialized_size(ctx, raster);
    uint8_t* ret = NULL;
    uint8_t* ptr = NULL;
    uint16_t i = 0;

    assert(NULL != ctx);
    assert(NULL != ctx->alloc);
    assert(NULL != raster);

    ret = (uint8_t*) ctx->alloc(size);
    if (!ret) {
        ctx->err("rt_raster_serialize: Out of memory allocating %d bytes for serializing a raster",
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
    d_print_binary_hex(ctx, "HEADER", dbg_ptr, size);
#endif

    ptr += sizeof (struct rt_raster_serialized_t);

    /* Serialize bands now */
    for (i = 0; i < raster->numBands; ++i) {
        rt_band band = raster->bands[i];
        assert(NULL != band);

        rt_pixtype pixtype = band->pixtype;
        int pixbytes = rt_pixtype_size(ctx, pixtype);
        if (pixbytes < 1) {
            ctx->err("rt_raster_serialize: Corrupted band: unknown pixtype");
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
        d_print_binary_hex(ctx, "PIXTYPE", dbg_ptr, size);
#endif

        ptr += 1;

        /* Add padding (if needed) */
        if (pixbytes > 1) {
            memset(ptr, '\0', pixbytes - 1);
            ptr += pixbytes - 1;
        }

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex(ctx, "PADDING", dbg_ptr, size);
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
                ctx->err("rt_raster_serialize: Fatal error caused by unknown pixel type. Aborting.");
                abort(); /* shouldn't happen */
                return 0;
        }

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uintptr_t) ptr % pixbytes));

#if POSTGIS_DEBUG_LEVEL > 2
        d_print_binary_hex(ctx, "nodata", dbg_ptr, size);
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
        d_print_binary_hex(ctx, "BAND", dbg_ptr, size);
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
    d_print_binary_hex(ctx, "SERIALIZED RASTER", dbg_ptr, size);
#endif

    return ret;
}

rt_raster
rt_raster_deserialize(rt_context ctx, void* serialized) {
    rt_raster rast = NULL;
    const uint8_t *ptr = NULL;
    const uint8_t *beg = NULL;
    uint16_t i = 0;
    uint8_t littleEndian = isMachineLittleEndian();

    assert(NULL != ctx);
    assert(NULL != serialized);

    RASTER_DEBUG(2, "rt_raster_deserialize: Entering...");

    /* NOTE: Value of rt_raster.size may be different
     * than actual size of raster data being read.
     * See note on SET_VARSIZE in rt_raster_serialize function above.
     */

    /* Allocate memory for deserialized raster header */

    RASTER_DEBUG(3, "rt_raster_deserialize: Allocationg memory for deserialized raster header");
    rast = (rt_raster) ctx->alloc(sizeof (struct rt_raster_t));
    if (!rast) {
        ctx->err("rt_raster_deserialize: Out of memory allocating raster for deserialization");
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
    rast->bands = ctx->alloc(rast->numBands * sizeof (rt_band));

    RASTER_DEBUGF(3, "rt_raster_deserialize: %d bands", rast->numBands);

    /* Move to the beginning of first band */
    ptr = beg;
    ptr += sizeof (struct rt_raster_serialized_t);

    /* Deserialize bands now */
    for (i = 0; i < rast->numBands; ++i) {
        rt_band band = NULL;
        uint8_t type = 0;
        int pixbytes = 0;

        band = ctx->alloc(sizeof (struct rt_band_t));
        if (!band) {
            ctx->err("rt_raster_deserialize: Out of memory allocating rt_band during deserialization");
            return 0;
        }

        rast->bands[i] = band;

        type = *ptr;
        ptr++;
        band->pixtype = type & BANDTYPE_PIXTYPE_MASK;

        RASTER_DEBUGF(3, "rt_raster_deserialize: band %d with pixel type %s", i, rt_pixtype_name(ctx, band->pixtype));

        band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
        band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
        band->isnodata = BANDTYPE_IS_NODATA(type) ? 1 : 0;
        band->width = rast->width;
        band->height = rast->height;
        band->ownsData = 0;

        /* Advance by data padding */
        pixbytes = rt_pixtype_size(ctx, band->pixtype);
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
                ctx->err("rt_raster_deserialize: Unknown pixeltype %d", band->pixtype);
                ctx->dealloc(band);
                ctx->dealloc(rast);
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
        while (0 != ((ptr - beg) % 8))
 {
            ++ptr;
        }

        RASTER_DEBUGF(3, "rt_raster_deserialize: skip %d bytes of 8-bytes boundary padding", ptr - padbeg);

        /* Consistency checking (ptr is pixbytes-aligned) */
        assert(!((uintptr_t) ptr % pixbytes));
    }

    return rast;
}

int rt_raster_is_empty(rt_context ctx, rt_raster raster) {
    assert(NULL != ctx);

    return (NULL == raster || raster->height <= 0 || raster->width <= 0);
}

int rt_raster_has_no_band(rt_context ctx, rt_raster raster, int nband) {
    assert(NULL != ctx);

    return (NULL == raster || raster->numBands < nband);
}

int32_t rt_raster_copy_band(rt_context ctx, rt_raster torast,
        rt_raster fromrast, int fromindex, int toindex)
{
    rt_band newband = NULL;
    assert(NULL != ctx);
    assert(NULL != torast);
    assert(NULL != fromrast);

    /* Check raster dimensions */
    if (torast->height != fromrast->height || torast->width != fromrast->width)
    {
        ctx->err("rt_raster_copy_band: Attempting to add a band with different width or height");
        return -1;
    }

    /* Check bands limits */
    if (fromrast->numBands < 1)
    {
        ctx->warn("rt_raster_copy_band: Second raster has no band");
        return -1;
    }
    else if (fromindex < 0)
    {
        ctx->warn("rt_raster_copy_band: Band index for second raster < 0. Defaulted to 1");
        fromindex = 0;
    }
    else if (fromindex >= fromrast->numBands)
    {
        ctx->warn("rt_raster_copy_band: Band index for second raster > number of bands, truncated from %u to %u", fromindex - 1, fromrast->numBands);
        fromindex = fromrast->numBands - 1;
    }

    if (toindex < 0)
    {
        ctx->warn("rt_raster_copy_band: Band index for first raster < 0. Defaulted to 1");
        toindex = 0;
    }
    else if (toindex > torast->numBands)
    {
        ctx->warn("rt_raster_copy_band: Band index for first raster > number of bands, truncated from %u to %u", toindex - 1, torast->numBands);
        toindex = torast->numBands;
    }

    /* Get band from source raster */
    newband = rt_raster_get_band(ctx, fromrast, fromindex);

    /* Add band to the second raster */
    return rt_raster_add_band(ctx, torast, newband, toindex);
}

