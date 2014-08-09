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
#include "lwgeom_log.h"
#include "liblwgeom.h"

/**
 * Constants to find the range of Varint. 
 * Since Varint only uses 7 bits instead of 8 in each byte a 8 byte varint
 * only uses 56 bits for the number and 8 for telling if next byte is used
 */
const int64_t varint_u64_max = ((int64_t) 1<<56) - 1;
const int64_t varint_s64_min = (int64_t) -1<<55;
const int64_t varint_s64_max = ((int64_t)1<<55) - 1;
const int32_t varint_u32_max = ((int32_t) 1<<28) - 1;
const int32_t varint_s32_min = (int32_t) -1<<27;
const int32_t varint_s32_max = ((int32_t)1<<27) - 1;

unsigned _varint_u64_encoded_size(uint64_t q);
uint8_t* _varint_u64_encode_buf(uint64_t q, uint8_t *buf);
unsigned varint_u32_encoded_size(uint32_t val);
uint8_t* varint_u32_encode_buf(uint32_t val, uint8_t *buf);
unsigned varint_s32_encoded_size(int32_t val);
uint8_t* varint_s32_encode_buf(int32_t val, uint8_t *buf);
unsigned varint_s64_encoded_size(int64_t val);
uint8_t* varint_s64_encode_buf(int64_t val, uint8_t *buf);
unsigned varint_u64_encoded_size(uint64_t val);
uint8_t* varint_u64_encode_buf(uint64_t val, uint8_t *buf);


unsigned
_varint_u64_encoded_size(uint64_t q)
{
    int n=0;
    while ((q>>(7*(n+1))) >0) ++n;
    return ++n;
}

uint8_t*
_varint_u64_encode_buf(uint64_t q, uint8_t *buf)
{
    int n=0, grp;
    while ((q>>(7*(n+1))) >0)
    {
        grp=128^(127&(q>>(7*n)));
        buf[n++]=grp;	
    }
    grp=127&(q>>(7*n));
    buf[n++]=grp;	

    return buf+=n;
}

unsigned
varint_u32_encoded_size(uint32_t val)
{
  LWDEBUGF(2, "Entered varint_u32_encoded_size, value %u", val);

  if( val>varint_u32_max ) {
    lwerror("Value is out of range for unsigned 32bit varint (0 to %ld)",
      varint_u32_max);
  }

  return _varint_u64_encoded_size(val); /* implicit upcast to 64bit int */
}

uint8_t*
varint_u32_encode_buf(uint32_t val, uint8_t *buf)
{
  LWDEBUGF(2, "Entered varint_u32_encode_buf, value %u", val);
  return _varint_u64_encode_buf(val, buf); /* implicit upcast to 64bit */
}

unsigned
varint_s32_encoded_size(int32_t val)
{
  LWDEBUGF(2, "Entered varint_s32_encoded_size, value %d", val);

  if(val<varint_s32_min||val>varint_s32_max) {
    lwerror("Value is out of range for signed 32bit varint (%d to %d)",
      varint_s32_min, varint_s32_max);
  }

  uint32_t q = (val << 1) ^ (val >> 31); /* zig-zag encode */
  return _varint_u64_encoded_size(q); /* implicit upcast to 64bit int */
}

uint8_t*
varint_s32_encode_buf(int32_t val, uint8_t *buf)
{
  LWDEBUGF(2, "Entered varint_s32_encode_buf, value %d", val);
  uint32_t q = (val << 1) ^ (val >> 31); /* zig-zag encode */
  return _varint_u64_encode_buf(q, buf); /* implicit upcast to 64bit */
}

unsigned
varint_s64_encoded_size(int64_t val)
{
  LWDEBUGF(2, "Entered varint_s64_encoded_size, value %ld", val);

  if(val<varint_s64_min||val>varint_s64_max) {
    lwerror("Value is out of range for signed 64bit varint (%ld to %ld)",
      varint_s64_min, varint_s64_max);
  }

  uint64_t q = (val << 1) ^ (val >> 63); /* zig-zag encode */
  return _varint_u64_encoded_size(q);
}

uint8_t*
varint_s64_encode_buf(int64_t val, uint8_t *buf)
{
  LWDEBUGF(2, "Entered varint_s64_encode_buf, value %ld", val);

  uint64_t q = (val << 1) ^ (val >> 63); /* zig-zag encode */
  return _varint_u64_encode_buf(q, buf);
}

unsigned
varint_u64_encoded_size(uint64_t val)
{
  LWDEBUGF(2, "Entered varint_u64_encoded_size, value %lu", val);

  if( val>varint_u64_max ) {
    lwerror("Value is out of range for unsigned 64bit varint (0 to %ld)",
      varint_u64_max);
  }

  return _varint_u64_encoded_size(val);
}

uint8_t*
varint_u64_encode_buf(uint64_t val, uint8_t *buf)
{
  LWDEBUGF(2, "Entered varint_u64_encode_buf, value %lu", val);
  return _varint_u64_encode_buf(val, buf);
}


#endif /* !defined _LIBLWGEOM_VARINT_H  */
