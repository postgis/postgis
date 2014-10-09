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

#include "varint.h"
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

static unsigned
_varint_u64_encoded_size(uint64_t q)
{
	int n=0;
	while ((q>>(7*(n+1))) >0) ++n;
	return ++n;
}

static int
_varint_u64_encode_buf(uint64_t val, uint8_t **buf)
{
	uint8_t grp;	
	uint64_t q=val;
	while (1) 
	{
		grp=127&q; //We put the 7 least significant bits in grp
		q=q>>7;	//We rightshift our input value 7 bits which means that the 7 next least significant bits becomes the 7 least significant
		if(q>0) // Check if, after our rightshifting, we still have anything to read in our input value.
		{
			/*In the next line quite a lot is happening.
			Since there is more to read in our input value we signalize that by setting the most siginicant bit in our byte to 1.
			Then we put that byte in our buffer (**buf) and move the cursor to our buffer (*buf) one step*/
			*((*buf)++)=128^grp;
		}
		else
		{
			/*The same as above, but since there is nothing more to read in our input value we leave the most significant bit unset*/
			*((*buf)++)=grp;
	//		printf("grp1:%d\n",(int) grp);
			return 0;
		}
	}
	return 0;
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

int
varint_u32_encode_buf(uint32_t val, uint8_t **buf)
{
  LWDEBUGF(2, "Entered varint_u32_encode_buf, value %u", val);
  _varint_u64_encode_buf(val, buf); /* implicit upcast to 64bit */
return 0;
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

int
varint_s32_encode_buf(int32_t val, uint8_t **buf)
{
	
	uint32_t q;
	q = (val << 1) ^ (val >> 31);/* zig-zag encode */
	_varint_u64_encode_buf(q, buf);/* implicit upcast to 64bit */
	return 0;
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

int
varint_s64_encode_buf(int64_t val, uint8_t **buf)
{
	uint64_t q;
	q = (val << 1) ^ (val >> 63);
	varint_u64_encode_buf(q, buf);
	return 0;
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

int
varint_u64_encode_buf(uint64_t val, uint8_t **buf)
{
  LWDEBUGF(2, "Entered varint_u64_encode_buf, value %lu", val);
  _varint_u64_encode_buf(val, buf);
  return 0;
}

/*Read varint*/

/* Read from unsigned 64bit varint */
uint64_t varint_u64_read(uint8_t **data, uint8_t *the_end)
{
    uint64_t nVal = 0;
    int nShift = 0;
    uint8_t nByte;
    
    while(*data<=the_end)//Check so we don't read beyond the twkb
    {
        nByte = (uint8_t) **data; //read a byte
        if (!(nByte & 0x80)) //If it is the last byte in the varInt ....
        {
            (*data) ++; //move the "cursor" one step
            return nVal | ((uint64_t)nByte << nShift);	 //Move the last read byte to the most significant place in the result and return the whole result
        }
        /*We get here when there is more to read in the input varInt*/
        nVal |= ((uint64_t)(nByte & 0x7f)) << nShift; //Here we take the least significant 7 bits of the read byte and put it in the most significant place in the result variable. 
        (*data) ++; //move the "cursor" of the input buffer step (8 bits)
        nShift += 7; //move the cursor in the resulting variable (7 bits)
    }
     lwerror("VarInt value goes beyond TWKB");
    return 0;
}

/* Read from signed 64bit varint */
uint64_t varint_s64_read(uint8_t **data, uint8_t *the_end)
{
    uint64_t nVal = varint_u64_read(data,the_end);
    /* un-zig-zag-ging */
    if ((nVal & 1) == 0) 
        return (((uint64_t)nVal) >> 1);
    else
        return (uint64_t) (-(nVal >> 1)-1);
}

/* Used to jump over varint values as efficient as possible*/
void varint_64_jump_n(uint8_t **data, int nValues, uint8_t *the_end)
{
    uint8_t nByte;
    while(nValues>0)//Check so we don't read beyond the twkb
    {
	 if(*data>the_end)
		 lwerror("VarInt value goes beyond TWKB");
        nByte = (uint8_t) **data; //read a byte
        if (!(nByte & 0x80)) //If it is the last byte in the varInt ....
        {
             nValues--;//...We count one more varint
        }       
        (*data) ++; //move the "cursor" of the input buffer step (8 bits)
    }
     return;
}
