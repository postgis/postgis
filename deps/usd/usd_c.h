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
 * Copyright 2023 J CUBE Inc and Jadason Technology Ltd
 *
 **********************************************************************/

#ifndef USD_C_H
#define USD_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "liblwgeom.h"
#include "lwgeom_log.h"

typedef enum usd_format
{
	USDA = 1,
	USDC = 2
} usd_format;

#define USD_ROOT_NAME "World"
#define USD_GEOM_NAME "_Geoemtry"

#define USD_WIDTH_MIN 1e-6f
#define USD_WIDTH_MAX 1e6f

struct usd_write_context;

struct usd_write_context *usd_write_create(usd_format format, const char *root_name, const char *geom_name, float points_curves_widths);

void usd_write_destroy(struct usd_write_context *ctx);

void usd_write_convert(struct usd_write_context *ctx, LWGEOM *geom);

void usd_write_save(struct usd_write_context *ctx);

void usd_write_data(struct usd_write_context *ctx, size_t *size, void **data);

struct usd_read_context;

struct usd_read_context *usd_read_create();

void usd_read_convert(struct usd_read_context *ctx, const char *data, size_t size, usd_format format);

struct usd_read_iterator;

struct usd_read_iterator *usd_read_iterator_create(struct usd_read_context *ctx);

LWGEOM *usd_read_iterator_next(struct usd_read_iterator *itr);

void usd_read_iterator_destroy(struct usd_read_iterator *itr);

void usd_read_destroy(struct usd_read_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
