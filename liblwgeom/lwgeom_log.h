/**********************************************************************
 * 
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Internal logging routines
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

#else /* POSTGIS_DEBUG_LEVEL <= 0 */

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUG(level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define LWDEBUGF(level, msg, ...) \
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
