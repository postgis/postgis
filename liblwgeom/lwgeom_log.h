/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#ifndef LWGEOM_LOG_H
#define LWGEOM_LOG_H 1

#include <stdarg.h>

/*
 * Debug macros
 */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a notice at the given debug level */
#define LWDEBUG(level, msg) \
        do { \
            if (POSTGIS_DEBUG_LEVEL >= level) \
              lwdebug(level, "[%s:%s:%d] " msg, __FILE__, __func__, __LINE__); \
        } while (0);

/* Display a formatted notice at the given debug level
 * (like printf, with variadic arguments) */
#define LWDEBUGF(level, msg, ...) \
        do { \
            if (POSTGIS_DEBUG_LEVEL >= level) \
              lwdebug(level, "[%s:%s:%d] " msg, \
                __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } while (0);

/* Display a notice and a WKT representation of a geometry
 * at the given debug level */
#define LWDEBUGG(level, geom, msg) \
  if (POSTGIS_DEBUG_LEVEL >= level) \
  do { \
    size_t sz; \
    char *wkt = lwgeom_to_wkt(geom, WKT_EXTENDED, 15, &sz); \
    /* char *wkt = lwgeom_to_hexwkb(geom, WKT_EXTENDED, &sz); */ \
    LWDEBUGF(level, msg ": %s", wkt); \
    lwfree(wkt); \
  } while (0);

/* Display a formatted notice and a WKT representation of a geometry
 * at the given debug level */
#define LWDEBUGGF(level, geom, fmt, ...) \
  if (POSTGIS_DEBUG_LEVEL >= level) \
  do { \
    size_t sz; \
    char *wkt = lwgeom_to_wkt(geom, WKT_EXTENDED, 15, &sz); \
    /* char *wkt = lwgeom_to_hexwkb(geom, WKT_EXTENDED, &sz); */ \
    LWDEBUGF(level, fmt ": %s", __VA_ARGS__, wkt); \
    lwfree(wkt); \
  } while (0);

#else /* POSTGIS_DEBUG_LEVEL <= 0 */

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUG(level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUGF(level, msg, ...) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUGG(level, geom, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUGGF(level, geom, fmt, ...) \
        ((void) 0)

#endif /* POSTGIS_DEBUG_LEVEL <= 0 */

/**
 * Write a notice out to the notice handler.
 *
 * Uses standard printf() substitutions.
 * Use for messages you always want output.
 * For debugging, use LWDEBUG() or LWDEBUGF().
 * @ingroup logging
 */
void lwnotice(const char *fmt, ...);

/**
 * Write a notice out to the error handler.
 *
 * Uses standard printf() substitutions.
 * Use for errors you always want output.
 * For debugging, use LWDEBUG() or LWDEBUGF().
 * @ingroup logging
 */
void lwerror(const char *fmt, ...);

/**
 * Write a debug message out.
 * Don't call this function directly, use the
 * macros, LWDEBUG() or LWDEBUGF(), for
 * efficiency.
 * @ingroup logging
 */
void lwdebug(int level, const char *fmt, ...);



#endif /* LWGEOM_LOG_H */
