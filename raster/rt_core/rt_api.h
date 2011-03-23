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

#ifndef RT_API_H_INCLUDED
#define RT_API_H_INCLUDED

/* define the systems */
#if defined(__linux__)  /* (predefined) */
#if !defined(LINUX)
#define LINUX
#endif
#if !defined(UNIX)
#define UNIX        /* make sure this is defined */
#endif
#endif


#if defined(__FreeBSD__) || defined(__OpenBSD__)    /* seems to work like Linux... */
#if !defined(LINUX)
#define LINUX
#endif
#if !defined(UNIX)
#define UNIX        /* make sure this is defined */
#endif
#endif

#if defined(__MSDOS__)
#if !defined(MSDOS)
#define MSDOS       /* make sure this is defined */
#endif
#endif

#if defined(__WIN32__) || defined(__NT__) || defined(_WIN32)
#if !defined(WIN32)
#define WIN32
#endif
#if defined(__BORLANDC__) && defined(MSDOS) /* Borland always defines MSDOS */
#undef  MSDOS
#endif
#endif

#if defined(__APPLE__)
#if !defined(UNIX)
#define UNIX
#endif
#endif

/* if we are in Unix define stricmp to be strcasecmp and strnicmp to */
/* be strncasecmp. I'm not sure if all Unices have these, but Linux */
/* does. */
#if defined(UNIX)
#if !defined(HAVE_STRICMP)
#define stricmp     strcasecmp
#endif
#if !defined(HAVE_STRNICMP)
#define strnicmp    strncasecmp
#endif
#endif



#include <stdlib.h> /* For size_t */
#include <stdint.h> /* For C99 int types */

#include "liblwgeom.h"

#include "gdal_alg.h"
#include "gdal_frmts.h"
#include "gdal.h"
#include "ogr_api.h"
#include "../../postgis_config.h"


typedef struct rt_context_t* rt_context;
typedef struct rt_raster_t* rt_raster;
typedef struct rt_band_t* rt_band;
typedef struct rt_geomval_t* rt_geomval;


/*- rt_context -------------------------------------------------------*/

typedef void* (*rt_allocator)(size_t size);
typedef void* (*rt_reallocator)(void *mem, size_t size);
typedef void  (*rt_deallocator)(void *mem);
typedef void  (*rt_message_handler)(const char* string, ...);


/* Debugging macros */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a simple message at NOTICE level */
#define RASTER_DEBUG(level, msg) \
    do { \
        if (POSTGIS_DEBUG_LEVEL >= level) \
            ctx->warn("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__); \
    } while (0);

/* Display a formatted message at NOTICE level (like printf, with variadic arguments) */
#define RASTER_DEBUGF(level, msg, ...) \
    do { \
        if (POSTGIS_DEBUG_LEVEL >= level) \
        ctx->warn("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    } while (0);

#else

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define RASTER_DEBUG(level, msg) \
    ((void) 0)

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define RASTER_DEBUGF(level, msg, ...) \
    ((void) 0)

#endif

/*- memory context -------------------------------------------------------*/

/* Initialize a context object
 * @param allocator memory allocator to use, 0 to use malloc
 * @param reallocator memory reallocator to use, 0 to use realloc
 * @param deallocator memory deallocator to use, 0 to use free
 * @return an opaque rt_context, or 0 on failure (out of memory)
 */
rt_context rt_context_new(rt_allocator allocator,
                          rt_reallocator reallocator,
                          rt_deallocator deallocator);

/* Destroy context */
void rt_context_destroy(rt_context ctx);

/* Set message handlers */
void rt_context_set_message_handlers(rt_context ctx,
                                    rt_message_handler error_handler,
                                    rt_message_handler warning_handler,
                                    rt_message_handler info_handler);

/*- rt_pixtype --------------------------------------------------------*/

/* Pixel types */
typedef enum {
    PT_1BB=0,     /* 1-bit boolean            */
    PT_2BUI=1,    /* 2-bit unsigned integer   */
    PT_4BUI=2,    /* 4-bit unsigned integer   */
    PT_8BSI=3,    /* 8-bit signed integer     */
    PT_8BUI=4,    /* 8-bit unsigned integer   */
    PT_16BSI=5,   /* 16-bit signed integer    */
    PT_16BUI=6,   /* 16-bit unsigned integer  */
    PT_32BSI=7,   /* 32-bit signed integer    */
    PT_32BUI=8,   /* 32-bit unsigned integer  */
    PT_32BF=10,   /* 32-bit float             */
    PT_64BF=11,   /* 64-bit float             */
    PT_END=13
} rt_pixtype;

/**
 * Return size in bytes of a value in the given pixtype
 */
int rt_pixtype_size(rt_context ctx, rt_pixtype pixtype);

/**
 * Return alignment requirements for data in the given pixel type.
 * Fast access to pixel values of this type must be aligned to as
 * many bytes as returned by this function.
 */
int rt_pixtype_alignment(rt_context ctx, rt_pixtype pixtype);

/* Return human-readable name of pixel type */
const char* rt_pixtype_name(rt_context ctx, rt_pixtype pixtype);

/* Return pixel type index from human-readable name */
rt_pixtype rt_pixtype_index_from_name(rt_context ctx, const char* pixname);

/*- rt_band ----------------------------------------------------------*/

/**
 * Create an in-buffer rt_band with no data
 *
 * @param ctx       : context, for thread safety
 * @param width     : number of pixel columns
 * @param height    : number of pixel rows
 * @param pixtype   : pixel type for the band
 * @param hasnodata : indicates if the band has nodata value
 * @param nodataval : the nodata value, will be appropriately
 *                    truncated to fit the pixtype size.
 * @param data      : pointer to actual band data, required to
 *                    be aligned accordingly to
 *                    rt_pixtype_aligment(pixtype) and big enough
 *                    to hold raster width*height values.
 *                    Data will NOT be copied, ownership is left
 *                    to caller which is responsible to keep it
 *                    allocated for the whole lifetime of the returned
 *                    rt_band.
 *
 * @return an rt_band, or 0 on failure
 */
rt_band rt_band_new_inline(rt_context ctx,
                           uint16_t width, uint16_t height,
                           rt_pixtype pixtype, uint32_t hasnodata,
                           double nodataval, uint8_t* data);

/**
 * Create an on-disk rt_band
 *
 * @param ctx       : context, for thread safety
 * @param width     : number of pixel columns
 * @param height    : number of pixel rows
 * @param pixtype   : pixel type for the band
 * @param nodataval : the nodata value, will be appropriately
 *                    truncated to fit the pixtype size.
 * @param bandNum   : 0-based band number in the external file
 *                    to associate this band with.
 * @param path      : NULL-terminated path string pointing to the file
 *                    containing band data. The string will NOT be
 *                    copied, ownership is left to caller which is
 *                    responsible to keep it allocated for the whole
 *                    lifetime of the returned rt_band.
 *
 * @return an rt_band, or 0 on failure
 */
rt_band rt_band_new_offline(rt_context ctx,
                            uint16_t width, uint16_t height,
                            rt_pixtype pixtype, uint32_t hasnodata,
                            double nodataval, uint8_t bandNum, const char* path);

/**
 * Return non-zero if the given band data is on
 * the filesystem.
 *
 * @param ctx : context, for thread safety
 * @param band : the band
 *
 * @return non-zero if the given band data is on
 *         the filesystem.
 */
int rt_band_is_offline(rt_context ctx, rt_band band);

/**
 * Return bands' external path (only valid when rt_band_is_offline
 * returns non-zero).
 */
const char* rt_band_get_ext_path(rt_context ctx, rt_band band);

/**
 * Return bands' external band number (only valid when
 * rt_band_is_offline returns non-zero).
 */
uint8_t rt_band_get_ext_band_num(rt_context ctx, rt_band band);


/* Get pixeltype of this band */
rt_pixtype rt_band_get_pixtype(rt_context ctx, rt_band band);

/* Get width of this band */
uint16_t rt_band_get_width(rt_context ctx, rt_band band);

/* Get height of this band */
uint16_t rt_band_get_height(rt_context ctx, rt_band band);

/* Get pointer to inline raster band data
 * @@deprecate ?
 */
void* rt_band_get_data(rt_context ctx, rt_band band);

/* Destroy a raster band */
void rt_band_destroy(rt_context ctx, rt_band band);

/**
 * Get hasnodata flag value
 * @param ctx : context, for thread safety
 * @param band : the band on which to check the hasnodata flag
 * @return the hasnodata flag.
 */
int rt_band_get_hasnodata_flag(rt_context ctx, rt_band band);

/**
 * Set hasnodata flag value
 * @param ctx : context, for thread safety
 * @param band : the band on which to set the hasnodata flag
 * @param flag : the new hasnodata flag value. Must be 1 or 0.
 */
void rt_band_set_hasnodata_flag(rt_context ctx, rt_band band, int flag);

/**
 * Set isnodata flag value
 * @param ctx : context, for thread safety
 * @param band : the band on which to set the isnodata flag
 * @param flag : the new isnodata flag value. Must be 1 or 0
 */
void rt_band_set_isnodata_flag(rt_context ctx, rt_band band, int flag);

/**
 * Get hasnodata flag value
 * @param ctx : context, for thread safety
 * @param band : the band on which to check the isnodata flag
 * @return the hasnodata flag.
 */
int rt_band_get_isnodata_flag(rt_context ctx, rt_band band);

/**
 * Set nodata value
 * @param ctx : context, for thread safety
 * @param band : the band to set nodata value to
 * @param val : the nodata value, must be in the range
 *              of values supported by this band's pixeltype
 *              or a warning will be printed and non-zero
 *              returned.
 *
 * @return 0 on success, -1 on error (value out of valid range).
 *
 */
int rt_band_set_nodata(rt_context ctx, rt_band band, double val);

/**
 * Get nodata value
 * @param ctx : context, for thread safety
 * @param band : the band to set nodata value to
 * @return nodata value
 */
double rt_band_get_nodata(rt_context ctx, rt_band band);

/**
 * Set pixel value
 * @param ctx : context, for thread safety
 * @param band : the band to set nodata value to
 * @param x : x ordinate
 * @param y : x ordinate
 * @param val : the pixel value, must be in the range
 *              of values supported by this band's pixeltype
 *              or a warning will be printed and non-zero
 *              returned.
 *
 * @return 0 on success, -1 on error (value out of valid range).
 */
int rt_band_set_pixel(rt_context ctx, rt_band band,
                      uint16_t x, uint16_t y, double val);

/**
 * Get pixel value
 *
 * @param ctx : context, for thread safety
 * @param band : the band to set nodata value to
 * @param x : x ordinate
 * @param y : x ordinate
 * @param *result: result if there is a value
 * @return the pixel value, as a double.
 */
int rt_band_get_pixel(rt_context ctx, rt_band band,
                         uint16_t x, uint16_t y, double *result );


/**
 * Returns the minimal possible value for the band according to the pixel type.
 * @param ctx: context, for thread safety
 * @param band: the band to get info from
 * @return the minimal possible value for the band.
 */
double rt_band_get_min_value(rt_context ctx, rt_band band);

/**
 * Returns TRUE if the band is only nodata values
 * @param ctx: context, for thread safety
 * @param band: the band to get info from
 * @return TRUE if the band is only nodata values, FALSE otherwise
 */
int rt_band_is_nodata(rt_context ctx, rt_band band);

/**
 * Returns TRUE if the band is only nodata values
 * @param ctx: context, for thread safety
 * @param band: the band to get info from
 * @return TRUE if the band is only nodata values, FALSE otherwise
 */
int rt_band_check_is_nodata(rt_context ctx, rt_band band);



/*- rt_raster --------------------------------------------------------*/

/**
 * Construct a raster with given dimensions.
 *
 * Transform will be set to identity.
 * Will contain no bands.
 *
 * @param ctx : context, for thread safety
 * @param width : number of pixel columns
 * @param height : number of pixel rows
 *
 * @return an rt_raster or 0 if out of memory
 */
rt_raster rt_raster_new(rt_context ctx, uint16_t width, uint16_t height);

/**
 * Construct an rt_raster from a binary WKB representation
 *
 * @param ctx : context, for thread safety
 * @param wkb : an octet stream
 * @param wkbsize : size (in bytes) of the wkb octet stream
 *
 * @return an rt_raster or 0 on error (out of memory or
 *         malformed WKB).
 *
 */
rt_raster rt_raster_from_wkb(rt_context ctx, const uint8_t* wkb,
                             uint32_t wkbsize);

/**
 * Construct an rt_raster from a text HEXWKB representation
 *
 * @param ctx : context, for thread safety
 * @param hexwkb : an hex-encoded stream
 * @param hexwkbsize : size (in bytes) of the hexwkb stream
 *
 * @return an rt_raster or 0 on error (out of memory or
 *         malformed WKB).
 *
 */
rt_raster rt_raster_from_hexwkb(rt_context ctx, const char* hexwkb,
                             uint32_t hexwkbsize);

/**
 * Return this raster in WKB form
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster
 * @param wkbsize : will be set to the size of returned wkb form
 */
uint8_t *rt_raster_to_wkb(rt_context ctx, rt_raster raster,
                                    uint32_t *wkbsize);

/**
 * Return this raster in HEXWKB form (null-terminated hex)
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster
 * @param hexwkbsize : will be set to the size of returned wkb form,
 *                     not including the null termination
 */
char *rt_raster_to_hexwkb(rt_context ctx, rt_raster raster,
                                    uint32_t *hexwkbsize);

/**
 * Release memory associated to a raster
 *
 * Note that this will not release data
 * associated to the band themselves (but only
 * the one associated with the pointers pointing
 * at them).
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to destroy
 */
void rt_raster_destroy(rt_context ctx, rt_raster raster);

/* Get number of bands */
int rt_raster_get_num_bands(rt_context ctx, rt_raster raster);

/* Return Nth band, or 0 if unavailable */
rt_band rt_raster_get_band(rt_context ctx, rt_raster raster, int bandNum);

/* Get number of rows */
uint16_t rt_raster_get_width(rt_context ctx, rt_raster raster);

/* Get number of columns */
uint16_t rt_raster_get_height(rt_context ctx, rt_raster raster);

/**
 * Add band data to a raster.
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to add a band to
 * @param band : the band to add, ownership left to caller.
 *               Band dimensions are required to match with raster ones.
 * @param index : the position where to insert the new band (0 based)
 *
 * @return identifier (position) for the just-added raster, or -1 on error
 */
int32_t rt_raster_add_band(rt_context ctx, rt_raster raster, rt_band band, int index);



/**
 * Generate a new band data and add it to a raster.
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to add a band to
 * @param pixtype: the pixel type for the new band
 * @param initialvalue: initial value for pixels
 * @param hasnodata: indicates if the band has a nodata value
 * @param nodatavalue: nodata value for the new band
 * @param index: position to add the new band in the raster
 *
 * @return identifier (position) for the just-added raster, or -1 on error
 */
int32_t rt_raster_generate_new_band(rt_context ctx, rt_raster raster, rt_pixtype pixtype,
        double initialvalue, uint32_t hasnodata, double nodatavalue, int index);

/**
 * Set scale in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set georeference of
 * @param scaleX : scale X in projection units
 * @param scaleY : scale Y height in projection units
 *
 * NOTE: doesn't recompute offsets
 */
void rt_raster_set_scale(rt_context ctx, rt_raster raster,
                               double scaleX, double scaleY);

/**
 * Get scale X in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to get georeference of
 *
 * @return scale X in projection units
 */
double rt_raster_get_x_scale(rt_context ctx, rt_raster raster);

/**
 * Get scale Y in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to get georeference of
 *
 * @return scale Y in projection units
 */
double rt_raster_get_y_scale(rt_context ctx, rt_raster raster);

/**
 * Set insertion points in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set georeference of
 * @param x : x ordinate of the upper-left corner of upper-left pixel,
 *            in projection units
 * @param y : y ordinate of the upper-left corner of upper-left pixel,
 *            in projection units
 */
void rt_raster_set_offsets(rt_context ctx, rt_raster raster,
                           double x, double y);

/**
 * Get raster x offset, in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to get georeference of
 *
 * @return  x ordinate of the upper-left corner of upper-left pixel,
 *          in projection units
 */
double rt_raster_get_x_offset(rt_context ctx, rt_raster raster);

/**
 * Get raster y offset, in projection units
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to get georeference of
 *
 * @return  y ordinate of the upper-left corner of upper-left pixel,
 *          in projection units
 */
double rt_raster_get_y_offset(rt_context ctx, rt_raster raster);

/**
 * Set skews about the X and Y axis
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set georeference of
 * @param skewX : skew about the x axis
 * @param skewY : skew about the y axis
 */
void rt_raster_set_skews(rt_context ctx, rt_raster raster,
                             double skewX, double skewY);

/**
 * Get skew about the X axis
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set georeference of
 * @return skew about the Y axis
 */
double rt_raster_get_x_skew(rt_context ctx, rt_raster raster);

/**
 * Get skew about the Y axis
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set georeference of
 * @return skew about the Y axis
 */
double rt_raster_get_y_skew(rt_context ctx, rt_raster raster);

/**
 * Set raster's SRID
 *
 * @param ctx : context, for thread safety
 * @param raster : the raster to set SRID of
 * @param srid : the SRID to set for the raster
 */
void rt_raster_set_srid(rt_context ctx, rt_raster raster, int32_t srid);

/**
 * Get raster's SRID
 * @param ctx : context, for thread safety
 * @param raster : the raster to set SRID of
 *
 * @return the raster's SRID
 */
int32_t rt_raster_get_srid(rt_context ctx, rt_raster raster);

/**
 * Convert an x,y raster point to an x1,y1 point on map
 *
 * @param ctx : context for thread safety
 * @param raster : the raster to get info from
 * @param x : the pixel's column
 * @param y : the pixel's row
 * @param x1 : output parameter, X ordinate of the geographical point
 * @param y1 : output parameter, Y ordinate of the geographical point
 */
void rt_raster_cell_to_geopoint(rt_context ctx, rt_raster raster,
                                double x, double y,
                                double* x1, double* y1);

/**
 * Get raster's polygon convex hull.
 *
 * The convex hull is a 4 vertices (5 to be closed) single
 * ring polygon bearing the raster's rotation
 * and using projection coordinates
 *
 * @param ctx : context for thread safety
 * @param raster : the raster to get info from
 *
 * @return the convex hull, or NULL on error.
 *
 */
LWPOLY* rt_raster_get_convex_hull(rt_context ctx, rt_raster raster);


/**
 * Returns a set of "geomval" value, one for each group of pixel
 * sharing the same value for the provided band.
 *
 * A "geomval " value is a complex type composed of a the wkt
 * representation of a geometry (one for each group of pixel sharing
 * the same value) and the value associated with this geometry.
 *
 * @param ctx: context for thread safety.
 * @param raster: the raster to get info from.
 * @param nband: the band to polygonize. From 1 to rt_raster_get_num_bands
 *
 * @return A set of "geomval" values, one for each group of pixels
 * sharing the same value for the provided band. The returned values are
 * WKT geometries, not real PostGIS geometries (this may change in the
 * future, and the function returns real geometries)
 */
rt_geomval
rt_raster_dump_as_wktpolygons(rt_context ctx, rt_raster raster, int nband,
        int * pnElements);


/**
 * Return this raster in serialized form.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 *
 */
void* rt_raster_serialize(rt_context ctx, rt_raster raster);

/**
 * Return a raster from a serialized form.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 *
 * NOTE: the raster will contain pointer to the serialized
 *       form, which must be kept alive.
 */
rt_raster rt_raster_deserialize(rt_context ctx, void* serialized);


/**
 * Return TRUE if the raster is empty. i.e. is NULL, width = 0 or height = 0
 * @param ctx: context, for thread safety
 * @param raster: the raster to get info from
 * @return TRUE if the raster is empty, FALSE otherwise
 */
int rt_raster_is_empty(rt_context ctx, rt_raster raster);

/**
 * Return TRUE if the raster do not have a band of this number.
 * @param ctx: context, for thread safety
 * @param raster: the raster to get info from
 * @param nband: the band number.
 * @return TRUE if the raster do not have a band of this number, FALSE otherwise
 */
int rt_raster_has_no_band(rt_context ctx, rt_raster raster, int nband);


/**
 * Copy one band from one raster to another
 * @param ctx: context, for thread safety
 * @param torast: raster to copy band to
 * @param fromrast: raster to copy band from
 * @param fromindex: index of band in source raster
 * @param toindex: index of new band in destination raster
 * @return The band index of the second raster where the new band is copied.
 */
int32_t rt_raster_copy_band(rt_context ctx, rt_raster torast,
        rt_raster fromrast, int fromindex, int toindex);

/*- utilities -------------------------------------------------------*/

/* Set of functions to clamp double to int of different size
 */

#define POSTGIS_RT_1BBMAX 1
#define POSTGIS_RT_2BUIMAX 3
#define POSTGIS_RT_4BUIMAX 15

uint8_t
rt_util_clamp_to_1BB(double value);

uint8_t
rt_util_clamp_to_2BUI(double value);

uint8_t
rt_util_clamp_to_4BUI(double value);

int8_t
rt_util_clamp_to_8BSI(double value);

uint8_t
rt_util_clamp_to_8BUI(double value);

int16_t
rt_util_clamp_to_16BSI(double value);

uint16_t
rt_util_clamp_to_16BUI(double value);

int32_t
rt_util_clamp_to_32BSI(double value);

uint32_t
rt_util_clamp_to_32BUI(double value);

float
rt_util_clamp_to_32F(double value);

int
rt_util_display_dbl_trunc_warning(rt_context ctx,
                                  double initialvalue,
                                  int32_t checkvalint,
                                  uint32_t checkvaluint,
                                  float checkvalfloat,
                                  double checkvaldouble,
                                  rt_pixtype pixtype);

#endif /* RT_API_H_INCLUDED */
