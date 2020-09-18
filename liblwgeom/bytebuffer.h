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
 * Copyright 2015 Nicklas Avén <nicklas.aven@jordogskog.no>
 *
 **********************************************************************/


#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "varint.h"

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

#define BYTEBUFFER_STARTSIZE 512
#define BYTEBUFFER_STATICSIZE 1024

typedef struct
{
	size_t capacity;
	uint8_t *buf_start;
	uint8_t *writecursor;
	uint8_t *readcursor;
	uint8_t buf_static[BYTEBUFFER_STATICSIZE];
}
bytebuffer_t;

void bytebuffer_init_with_size(bytebuffer_t *b, size_t size);
void bytebuffer_destroy_buffer(bytebuffer_t *s);
void bytebuffer_append_byte(bytebuffer_t *s, const uint8_t val);
void bytebuffer_append_bytebuffer(bytebuffer_t *write_to, bytebuffer_t *write_from);
void bytebuffer_append_varint(bytebuffer_t *s, const int64_t val);
void bytebuffer_append_uvarint(bytebuffer_t *s, const uint64_t val);
size_t bytebuffer_getlength(const bytebuffer_t *s);
lwvarlena_t *bytebuffer_get_buffer_varlena(const bytebuffer_t *s);
const uint8_t* bytebuffer_get_buffer(const bytebuffer_t *s, size_t *buffer_length);

/* Unused functions */
#if 0
void bytebuffer_destroy(bytebuffer_t *s);
bytebuffer_t *bytebuffer_create_with_size(size_t size);
bytebuffer_t *bytebuffer_create(void);
void bytebuffer_clear(bytebuffer_t *s);
uint8_t* bytebuffer_get_buffer_copy(const bytebuffer_t *s, size_t *buffer_length);
uint64_t bytebuffer_read_uvarint(bytebuffer_t *s);
int64_t bytebuffer_read_varint(bytebuffer_t *s);
bytebuffer_t* bytebuffer_merge(bytebuffer_t **buff_array, int nbuffers);
void bytebuffer_reset_reading(bytebuffer_t *s);
void bytebuffer_append_bytebuffer(bytebuffer_t *write_to,bytebuffer_t *write_from);
void bytebuffer_append_bulk(bytebuffer_t *s, void * start, size_t size);
void bytebuffer_append_int(bytebuffer_t *buf, const int val, int swap);
void bytebuffer_append_double(bytebuffer_t *buf, const double val, int swap);
#endif

#endif /* _BYTEBUFFER_H */
