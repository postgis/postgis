/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2014 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * 
 * Handle varInt values, as described here:
 * http://developers.google.com/protocol-buffers/docs/encoding#varints
 *
 **********************************************************************/

#ifndef _LIBLWGEOM_VARINT_H
#define _LIBLWGEOM_VARINT_H 1

#include <stdint.h>
/*Write varInt to buffer*/

/* Find encoded size for unsigned 32bit integer */
unsigned varint_u32_encoded_size(uint32_t val);

/* Encode unsigned 32bit integer */
int varint_u32_encode_buf(uint32_t val, uint8_t **buf);

/* Find encoded size for signed 32bit integer */
unsigned varint_s32_encoded_size(int32_t val);

/* Encode signed 32bit integer */
int varint_s32_encode_buf(int32_t val, uint8_t **buf);

/* Find encoded size for unsigned 64bit integer */
unsigned varint_u64_encoded_size(uint64_t val);

/* Encode unsigned 64bit integer */
int varint_u64_encode_buf(uint64_t val, uint8_t **buf);

/* Find encoded size for signed 64bit integer */
unsigned varint_s64_encoded_size(int64_t val);

/* Encode unsigned 64bit integer */
int varint_s64_encode_buf(int64_t val, uint8_t **buf);

/*Read varint*/

/* Read from unsigned 64bit varint */
uint64_t varint_u64_read(uint8_t **buf, uint8_t *the_end);

/* Read from signed 64bit varint */
uint64_t varint_s64_read(uint8_t **buf, uint8_t *the_end);

/*To jump over values like id, when we don't care about id
should be faster than reading it properly
nValues tells how many varInts to jump over*/
void varint_64_jump_n(uint8_t **data, int nValues, uint8_t *the_end);

#endif /* !defined _LIBLWGEOM_VARINT_H  */

