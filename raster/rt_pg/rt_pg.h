/*
 * $Id: rt_pg.h 10860 2012-12-19 18:06:17Z strk $
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
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

#ifndef RT_PG_H_INCLUDED
#define RT_PG_H_INCLUDED

#include <stdint.h> /* for int16_t and friends */

#include "rt_api.h"
#include "../../postgis_config.h"
#include "../raster_config.h"

/* Debugging macros */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a simple message at NOTICE level */
#define POSTGIS_RT_DEBUG(level, msg) \
    do { \
        if (POSTGIS_DEBUG_LEVEL >= level) \
            ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__))); \
    } while (0);

/* Display a formatted message at NOTICE level (like printf, with variadic arguments) */
#define POSTGIS_RT_DEBUGF(level, msg, ...) \
    do { \
        if (POSTGIS_DEBUG_LEVEL >= level) \
        ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__))); \
    } while (0);

#else

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_RT_DEBUG(level, msg) \
    ((void) 0)

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_RT_DEBUGF(level, msg, ...) \
    ((void) 0)

#endif


typedef struct rt_pgband8_t {
    uint8_t pixtype;
    uint8_t data[1];
} rt_pgband8;

typedef struct rt_pgband16_t {
    uint8_t pixtype;
    uint8_t pad;
    uint8_t data[1];
} rt_pgband16;

typedef struct rt_pgband32_t {
    uint8_t pixtype;
    uint8_t pad0;
    uint8_t pad1;
    uint8_t pad2;
    uint8_t data[1];
} rt_pgband32;

typedef struct rt_pgband64_t {
    uint8_t pixtype;
    uint8_t pad[7];
    uint8_t data[1];
} rt_pgband64;

typedef struct rt_pgband_t {
    uint8_t pixtype;
    uint8_t data[1];
} rt_pgband;

/* Header of PostgreSQL-stored RASTER value,
 * and binary representation of it */
typedef struct rt_raster_serialized_t rt_pgraster;

#endif /* RT_PG_H_INCLUDED */
