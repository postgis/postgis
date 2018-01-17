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
 * Copyright (C) 2014 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2013 Nicklas Avén
 *
 **********************************************************************/


#ifndef _LIBLWGEOM_VARINT_H
#define _LIBLWGEOM_VARINT_H 1

#include <stdint.h>
#include <stdlib.h>


/* NEW SIGNATURES */

size_t varint_u32_encode_buf(uint32_t val, uint8_t *buf);
size_t varint_s32_encode_buf(int32_t val, uint8_t *buf);
size_t varint_u64_encode_buf(uint64_t val, uint8_t *buf);
size_t varint_s64_encode_buf(int64_t val, uint8_t *buf);
int64_t varint_s64_decode(const uint8_t *the_start, const uint8_t *the_end, size_t *size);
uint64_t varint_u64_decode(const uint8_t *the_start, const uint8_t *the_end, size_t *size);

size_t varint_size(const uint8_t *the_start, const uint8_t *the_end);

/* Support from -INT{8,32,64}_MAX to INT{8,32,64}_MAX),
 * e.g INT8_MIN is not supported in zigzag8 */
uint64_t zigzag64(int64_t val);
uint32_t zigzag32(int32_t val);
uint8_t zigzag8(int8_t val);
int64_t unzigzag64(uint64_t val);
int32_t unzigzag32(uint32_t val);
int8_t unzigzag8(uint8_t val);

#endif /* !defined _LIBLWGEOM_VARINT_H  */

