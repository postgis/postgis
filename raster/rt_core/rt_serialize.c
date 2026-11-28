/*
 *
 * WKTRaster - Raster Types for PostGIS
 *
 * Copyright (C) 2011-2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@azavea.com>
 * Copyright (C) 2009-2011 Pierre Racine <pierre.racine@sbf.ulaval.ca>
 * Copyright (C) 2009-2011 Mateusz Loskot <mateusz@loskot.net>
 * Copyright (C) 2008-2009 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2025-2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "librtcore.h"
#include "librtcore_internal.h"
#include "rt_serialize.h"

/* PostgreSQL varlena and palloc size limit for serialized rasters. */
#define RT_MAX_ALLOC_SIZE 0x3fffffffUL

/******************************************************************************
 * Debug and Testing Utilities
 ******************************************************************************/

#if POSTGIS_DEBUG_LEVEL > 2

char *
d_binary_to_hex(const uint8_t *const raw, uint32_t size, uint32_t *hexsize)
{
	char *hex = NULL;
	uint32_t i = 0;

	assert(NULL != raw);
	assert(NULL != hexsize);

	*hexsize = size * 2; /* hex is 2 times bytes */
	hex = (char *)rtalloc((*hexsize) + 1);
	if (!hex)
	{
		rterror("d_binary_to_hex: Out of memory hexifying raw binary");
		return NULL;
	}
	hex[*hexsize] = '\0'; /* Null-terminate */

	for (i = 0; i < size; ++i)
	{
		deparse_hex(raw[i], &(hex[2 * i]));
	}

	assert(NULL != hex);
	assert(0 == strlen(hex) % 2);
	return hex;
}

void
d_print_binary_hex(const char *msg, const uint8_t *const raw, uint32_t size)
{
	char *hex = NULL;
	uint32_t hexsize = 0;

	assert(NULL != msg);
	assert(NULL != raw);

	hex = d_binary_to_hex(raw, size, &hexsize);
	if (NULL != hex)
	{
		rtinfo("%s\t%s", msg, hex);
		rtdealloc(hex);
	}
}

size_t
d_binptr_to_pos(const uint8_t *const ptr, const uint8_t *const end, size_t size)
{
	assert(NULL != ptr && NULL != end);

	return (size - (end - ptr));
}

#endif /* if POSTGIS_DEBUG_LEVEL > 2 */

#ifdef OPTIMIZE_SPACE

/*
 * Set given number of bits of the given byte,
 * starting from given bitOffset (from the first)
 * to the given value.
 *
 * Examples:
 *   char ch;
 *   ch=0; setBits(&ch, 1, 1, 0) -> ch==8
 *   ch=0; setBits(&ch, 3, 2, 1) -> ch==96 (0x60)
 *
 * Note that number of bits set must be <= 8-bitOffset
 *
 */
void
setBits(char *ch, double val, int bits, int bitOffset)
{
	char mask = 0xFF >> (8 - bits);
	char ival = val;

	assert(ch != NULL);
	assert(8 - bitOffset >= bits);

	RASTER_DEBUGF(4, "ival:%d bits:%d mask:%hhx bitoffset:%d\n", ival, bits, mask, bitOffset);

	/* clear all but significant bits from ival */
	ival &= mask;
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
	if (ival != val)
	{
		rtwarn(
		    "Pixel value for %d-bits band got truncated"
		    " from %g to %hhu",
		    bits,
		    val,
		    ival);
	}
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

	RASTER_DEBUGF(4, " cleared ival:%hhx\n", ival);

	/* Shift ival so the significant bits start at
	 * the first bit */
	ival <<= (8 - bitOffset - bits);

	RASTER_DEBUGF(4, " ival shifted:%hhx\n", ival);
	RASTER_DEBUGF(4, "  ch:%hhx\n", *ch);

	/* clear first bits of target */
	*ch &= ~(mask << (8 - bits - bitOffset));

	RASTER_DEBUGF(4, "  ch cleared:%hhx\n", *ch);

	/* Set the first bit of target */
	*ch |= ival;

	RASTER_DEBUGF(4, "  ch ored:%hhx\n", *ch);
}
#endif /* OPTIMIZE_SPACE */

void
swap_char(uint8_t *a, uint8_t *b)
{
	uint8_t c = 0;

	assert(NULL != a && NULL != b);

	c = *a;
	*a = *b;
	*b = c;
}

void
flip_endian_16(uint8_t *d)
{
	assert(NULL != d);

	swap_char(d, d + 1);
}

void
flip_endian_32(uint8_t *d)
{
	assert(NULL != d);

	swap_char(d, d + 3);
	swap_char(d + 1, d + 2);
}

void
flip_endian_64(uint8_t *d)
{
	assert(NULL != d);

	swap_char(d + 7, d);
	swap_char(d + 6, d + 1);
	swap_char(d + 5, d + 2);
	swap_char(d + 4, d + 3);
}

uint8_t
isMachineLittleEndian(void)
{
	static int endian_check_int = 1; /* dont modify this!!! */
	/* 0=big endian|xdr --  1=little endian|ndr */
	return *((uint8_t *)&endian_check_int);
}

uint8_t
read_uint8(const uint8_t **from)
{
	assert(NULL != from);

	return *(*from)++;
}

/* unused up to now
void
write_uint8(uint8_t** from, uint8_t v) {
    assert(NULL != from);

 *(*from)++ = v;
}
*/

int8_t
read_int8(const uint8_t **from)
{
	assert(NULL != from);

	return (int8_t)read_uint8(from);
}

/* unused up to now
void
write_int8(uint8_t** from, int8_t v) {
    assert(NULL != from);

 *(*from)++ = v;
}
*/

uint16_t
read_uint16(const uint8_t **from, uint8_t littleEndian)
{
	uint16_t ret = 0;

	assert(NULL != from);

	if (littleEndian)
	{
		ret = (*from)[0] | (*from)[1] << 8;
	}
	else
	{
		/* big endian */
		ret = (*from)[0] << 8 | (*from)[1];
	}
	*from += 2;
	return ret;
}

void
write_uint16(uint8_t **to, uint8_t littleEndian, uint16_t v)
{
	assert(NULL != to);

	if (littleEndian)
	{
		(*to)[0] = v & 0x00FF;
		(*to)[1] = v >> 8;
	}
	else
	{
		(*to)[1] = v & 0x00FF;
		(*to)[0] = v >> 8;
	}
	*to += 2;
}

int16_t
read_int16(const uint8_t **from, uint8_t littleEndian)
{
	assert(NULL != from);

	return (int16_t)read_uint16(from, littleEndian);
}

/* unused up to now
void
write_int16(uint8_t** to, uint8_t littleEndian, int16_t v) {
    assert(NULL != to);

    if ( littleEndian )
    {
	(*to)[0] = v & 0x00FF;
	(*to)[1] = v >> 8;
    }
    else
    {
	(*to)[1] = v & 0x00FF;
	(*to)[0] = v >> 8;
    }
 *to += 2;
}
*/

uint32_t
read_uint32(const uint8_t **from, uint8_t littleEndian)
{
	uint32_t ret = 0;

	assert(NULL != from);

	if (littleEndian)
	{
		ret = (uint32_t)((*from)[0] & 0xff) | (uint32_t)((*from)[1] & 0xff) << 8 |
		      (uint32_t)((*from)[2] & 0xff) << 16 | (uint32_t)((*from)[3] & 0xff) << 24;
	}
	else
	{
		/* big endian */
		ret = (uint32_t)((*from)[3] & 0xff) | (uint32_t)((*from)[2] & 0xff) << 8 |
		      (uint32_t)((*from)[1] & 0xff) << 16 | (uint32_t)((*from)[0] & 0xff) << 24;
	}

	*from += 4;
	return ret;
}

/* unused up to now
void
write_uint32(uint8_t** to, uint8_t littleEndian, uint32_t v) {
    assert(NULL != to);

    if ( littleEndian )
    {
	(*to)[0] = v & 0x000000FF;
	(*to)[1] = ( v & 0x0000FF00 ) >> 8;
	(*to)[2] = ( v & 0x00FF0000 ) >> 16;
	(*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
	(*to)[3] = v & 0x000000FF;
	(*to)[2] = ( v & 0x0000FF00 ) >> 8;
	(*to)[1] = ( v & 0x00FF0000 ) >> 16;
	(*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
*/

int32_t
read_int32(const uint8_t **from, uint8_t littleEndian)
{
	assert(NULL != from);

	return (int32_t)read_uint32(from, littleEndian);
}

/* unused up to now
void
write_int32(uint8_t** to, uint8_t littleEndian, int32_t v) {
    assert(NULL != to);

    if ( littleEndian )
    {
	(*to)[0] = v & 0x000000FF;
	(*to)[1] = ( v & 0x0000FF00 ) >> 8;
	(*to)[2] = ( v & 0x00FF0000 ) >> 16;
	(*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
	(*to)[3] = v & 0x000000FF;
	(*to)[2] = ( v & 0x0000FF00 ) >> 8;
	(*to)[1] = ( v & 0x00FF0000 ) >> 16;
	(*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
*/

float
read_float32(const uint8_t **from, uint8_t littleEndian)
{

	union {
		float f;
		uint32_t i;
	} ret;

	ret.i = read_uint32(from, littleEndian);

	return ret.f;
}

/* unused up to now
void
write_float32(uint8_t** from, uint8_t littleEndian, float f) {
    union {
	float f;
	uint32_t i;
    } u;

    u.f = f;
    write_uint32(from, littleEndian, u.i);
}
*/

double
read_float64(const uint8_t **from, uint8_t littleEndian)
{

	union {
		double d;
		uint64_t i;
	} ret;

	assert(NULL != from);

	if (littleEndian)
	{
		ret.i = (uint64_t)((*from)[0] & 0xff) | (uint64_t)((*from)[1] & 0xff) << 8 |
			(uint64_t)((*from)[2] & 0xff) << 16 | (uint64_t)((*from)[3] & 0xff) << 24 |
			(uint64_t)((*from)[4] & 0xff) << 32 | (uint64_t)((*from)[5] & 0xff) << 40 |
			(uint64_t)((*from)[6] & 0xff) << 48 | (uint64_t)((*from)[7] & 0xff) << 56;
	}
	else
	{
		/* big endian */
		ret.i = (uint64_t)((*from)[7] & 0xff) | (uint64_t)((*from)[6] & 0xff) << 8 |
			(uint64_t)((*from)[5] & 0xff) << 16 | (uint64_t)((*from)[4] & 0xff) << 24 |
			(uint64_t)((*from)[3] & 0xff) << 32 | (uint64_t)((*from)[2] & 0xff) << 40 |
			(uint64_t)((*from)[1] & 0xff) << 48 | (uint64_t)((*from)[0] & 0xff) << 56;
	}

	*from += 8;
	return ret.d;
}

/* unused up to now
void
write_float64(uint8_t** to, uint8_t littleEndian, double v) {
    union {
	double d;
	uint64_t i;
    } u;

    assert(NULL != to);

    u.d = v;

    if ( littleEndian )
    {
	(*to)[0] =   u.i & 0x00000000000000FFULL;
	(*to)[1] = ( u.i & 0x000000000000FF00ULL ) >> 8;
	(*to)[2] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
	(*to)[3] = ( u.i & 0x00000000FF000000ULL ) >> 24;
	(*to)[4] = ( u.i & 0x000000FF00000000ULL ) >> 32;
	(*to)[5] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
	(*to)[6] = ( u.i & 0x00FF000000000000ULL ) >> 48;
	(*to)[7] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
    else
    {
	(*to)[7] =   u.i & 0x00000000000000FFULL;
	(*to)[6] = ( u.i & 0x000000000000FF00ULL ) >> 8;
	(*to)[5] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
	(*to)[4] = ( u.i & 0x00000000FF000000ULL ) >> 24;
	(*to)[3] = ( u.i & 0x000000FF00000000ULL ) >> 32;
	(*to)[2] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
	(*to)[1] = ( u.i & 0x00FF000000000000ULL ) >> 48;
	(*to)[0] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
 *to += 8;
}
*/

static size_t
rt_raster_serialized_size(rt_raster raster)
{
	size_t size = sizeof(struct rt_raster_serialized_t);

	assert(raster);

	for (uint32_t i = 0; i < raster->numBands; ++i)
	{
		rt_band band = raster->bands[i];
		int pixbytes = rt_pixtype_size_for(__func__, band->pixtype);
		size_t payload;
		size_t overhead;

		if (pixbytes < 1)
			return 0;

		overhead = 2 * rt_util_size_from_signed(pixbytes); /* type byte, its padding and nodata */
		if (band->offline)
		{
			overhead += 2; /* band number and path terminator */
			payload = strlen(band->data.offline.path);
		}
		else if (!rt_band_data_size(__func__, raster->width, raster->height, pixbytes, SIZE_MAX, &payload))
			return 0;

		if (!rt_util_size_add(&size, overhead, RT_MAX_ALLOC_SIZE) ||
		    !rt_util_size_add(&size, payload, RT_MAX_ALLOC_SIZE) ||
		    !rt_util_size_add(&size, (8 - size % 8) % 8, RT_MAX_ALLOC_SIZE))
		{
			rterror("%s: Raster serialized size exceeds PostgreSQL maximum", __func__);
			return 0;
		}
	}

	return size;
}

/**
 * Return this raster in serialized form.
 * Memory (band data included) is copied from rt_raster.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 */
void *
rt_raster_serialize(rt_raster raster)
{
	size_t size = 0;
	uint8_t *ret = NULL;
	uint8_t *ptr = NULL;

	assert(NULL != raster);

	size = rt_raster_serialized_size(raster);
	if (size == 0)
		return NULL;
	ret = rtalloc(size);
	if (!ret)
	{
		rterror("%s: Out of memory allocating %zu bytes for serializing a raster", __func__, size);
		return NULL;
	}
	memset(ret, '-', size);
	ptr = ret;

	RASTER_DEBUGF(3, "sizeof(struct rt_raster_serialized_t):%zu", sizeof(struct rt_raster_serialized_t));
	RASTER_DEBUGF(3, "sizeof(struct rt_raster_t):%zu", sizeof(struct rt_raster_t));
	RASTER_DEBUGF(3, "serialized size:%zu", size);

	/* Set size */
	/* NOTE: Value of rt_raster.size may be updated in
	 * returned object, for instance, by rt_pg layer to
	 * store value calculated by SET_VARSIZE.
	 */
	raster->size = (uint32_t)size;

	/* Set version */
	raster->version = 0;

	/* Copy header */
	memcpy(ptr, raster, sizeof(struct rt_raster_serialized_t));

	RASTER_DEBUG(3, "Start hex dump of raster being serialized using 0x2D to mark non-written bytes");

#if POSTGIS_DEBUG_LEVEL > 2
	uint8_t *dbg_ptr = ptr;
	d_print_binary_hex("HEADER", dbg_ptr, size);
#endif

	ptr += sizeof(struct rt_raster_serialized_t);

	/* Serialize bands now */
	for (uint32_t i = 0; i < raster->numBands; ++i)
	{
		rt_band band = raster->bands[i];
		assert(NULL != band);

		rt_pixtype pixtype = band->pixtype;
		int pixbytes = rt_pixtype_size_for(__func__, pixtype);
		if (pixbytes < 1)
		{
			rtdealloc(ret);
			return NULL;
		}

		/* Add band type */
		*ptr = band->pixtype;
		if (band->offline)
		{
#ifdef POSTGIS_RASTER_DISABLE_OFFLINE
			rterror("rt_raster_serialize: offdb raster support disabled at compile-time");
			return NULL;
#endif
			*ptr |= BANDTYPE_FLAG_OFFDB;
		}
		if (band->hasnodata)
		{
			*ptr |= BANDTYPE_FLAG_HASNODATA;
		}

		if (band->isnodata)
		{
			*ptr |= BANDTYPE_FLAG_ISNODATA;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("PIXTYPE", dbg_ptr, size);
#endif

		ptr += 1;

		/* Add padding (if needed) */
		if (pixbytes > 1)
		{
			memset(ptr, '\0', pixbytes - 1);
			ptr += pixbytes - 1;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("PADDING", dbg_ptr, size);
#endif

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));

		/* Add nodata value */
		switch (pixtype)
		{
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BUI: {
			uint8_t v = band->nodataval;
			*ptr = v;
			ptr += 1;
			break;
		}
		case PT_8BSI: {
			int8_t v = band->nodataval;
			*ptr = (uint8_t)v;
			ptr += 1;
			break;
		}
		case PT_16BSI: {
			int16_t v = band->nodataval;
			memcpy(ptr, &v, 2);
			ptr += 2;
			break;
		}
		case PT_16BUI: {
			uint16_t v = band->nodataval;
			memcpy(ptr, &v, 2);
			ptr += 2;
			break;
		}
		case PT_32BSI: {
			int32_t v = band->nodataval;
			memcpy(ptr, &v, 4);
			ptr += 4;
			break;
		}
		case PT_32BUI: {
			uint32_t v = band->nodataval;
			memcpy(ptr, &v, 4);
			ptr += 4;
			break;
		}
		case PT_16BF: {
			uint16_t v = rt_util_float_to_float16((float)band->nodataval);
			memcpy(ptr, &v, 2);
			ptr += 2;
			break;
		}
		case PT_32BF: {
			float v = band->nodataval;
			memcpy(ptr, &v, 4);
			ptr += 4;
			break;
		}
		case PT_64BF: {
			memcpy(ptr, &band->nodataval, 8);
			ptr += 8;
			break;
		}
		default:
			rterror("rt_raster_serialize: Fatal error caused by unknown pixel type. Aborting.");
			rtdealloc(ret);
			return NULL;
		}

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("nodata", dbg_ptr, size);
#endif

		if (band->offline)
		{
			/* Write band number */
			*ptr = band->data.offline.bandNum;
			ptr += 1;

			/* Write path */
			strcpy((char *)ptr, band->data.offline.path);
			ptr += strlen(band->data.offline.path) + 1;
		}
		else
		{
			/* Write data */
			size_t datasize;
			if (!rt_band_data_size(__func__, raster->width, raster->height, pixbytes, SIZE_MAX, &datasize))
			{
				rtdealloc(ret);
				return NULL;
			}
			memcpy(ptr, band->data.mem, datasize);
			ptr += datasize;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("BAND", dbg_ptr, size);
#endif

		/* Pad up to 8-bytes boundary */
		while ((ptr - ret) % 8)
		{
			*ptr = 0;
			++ptr;
		}

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));
	} /* for-loop over bands */

#if POSTGIS_DEBUG_LEVEL > 2
	d_print_binary_hex("SERIALIZED RASTER", dbg_ptr, size);
#endif
	return ret;
}

/* Only the initialized prefix of the band array belongs to this reader. */
static rt_raster
rt_raster_deserialize_failed(rt_raster raster, uint32_t nbands)
{
	for (uint32_t i = 0; i < nbands; i++)
		rt_band_destroy(raster->bands[i]);
	rt_raster_destroy(raster);
	return NULL;
}

static rt_raster
rt_raster_deserialize_truncated(const char *caller, rt_raster raster, uint32_t band_index, const char *field)
{
	rterror("%s: Premature end of serialized raster reading band %u %s", caller, band_index, field);
	return rt_raster_deserialize_failed(raster, band_index + 1);
}

/**
 * Return a raster from a serialized form.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 *
 * NOTE: the raster will contain pointer to the serialized
 * form (including band data), which must be kept alive.
 */
static rt_raster
rt_raster_deserialize_with_size_internal(const void *serialized,
					 size_t serialized_size,
					 int header_only,
					 int pg_varlena)
{
	rt_raster rast = NULL;
	const uint8_t *ptr = NULL;
	const uint8_t *beg = NULL;
	const uint8_t *end = NULL;
	size_t bands_size;
#ifdef WORDS_BIGENDIAN
	uint8_t littleEndian = LW_FALSE;
#else
	uint8_t littleEndian = LW_TRUE;
#endif

	assert(serialized);
	if (serialized_size < sizeof(struct rt_raster_serialized_t) || serialized_size > UINT32_MAX)
	{
		rterror("%s: Serialized raster length is out of range", __func__);
		return NULL;
	}

	RASTER_DEBUG(2, "rt_raster_deserialize: Entering...");

	/* NOTE: Value of rt_raster.size may be different
	 * than actual size of raster data being read.
	 * See note on SET_VARSIZE in rt_raster_serialize function above.
	 */

	/* Allocate memory for deserialized raster header */
	RASTER_DEBUG(3, "rt_raster_deserialize: Allocating memory for deserialized raster header");
	rast = rtalloc(sizeof(*rast));
	if (!rast)
	{
		rterror("%s: Out of memory allocating raster for deserialization", __func__);
		return NULL;
	}

	/* Deserialize raster header */
	RASTER_DEBUG(3, "rt_raster_deserialize: Deserialize raster header");
	memcpy(rast, serialized, sizeof(struct rt_raster_serialized_t));
	if (!pg_varlena && rast->size != serialized_size)
	{
		rterror("%s: Serialized raster size does not match input length", __func__);
		rtdealloc(rast);
		return NULL;
	}
	if (pg_varlena)
		rast->size = (uint32_t)serialized_size;

	if (0 == rast->numBands || header_only)
	{
		rast->bands = 0;
		return rast;
	}

	beg = serialized;
	end = beg + serialized_size;

	/* Allocate registry of raster bands */
	RASTER_DEBUG(3, "rt_raster_deserialize: Allocating memory for bands");
	if (!rt_util_size_mul(rast->numBands, sizeof(rt_band), &bands_size))
	{
		rterror("%s: Too many bands", __func__);
		rtdealloc(rast);
		return NULL;
	}
	rast->bands = rtalloc(bands_size);
	if (!rast->bands)
	{
		rterror("%s: Out of memory allocating bands", __func__);
		rtdealloc(rast);
		return NULL;
	}

	RASTER_DEBUGF(3, "rt_raster_deserialize: %u bands", rast->numBands);

	/* Move to the beginning of first band */
	ptr = beg + sizeof(struct rt_raster_serialized_t);

	/* Deserialize bands now */
	for (uint32_t i = 0; i < rast->numBands; ++i)
	{
		uint8_t type;
		int pixbytes;
		rt_band band = rtalloc(sizeof(*band));
		if (!band)
		{
			rterror("%s: Out of memory allocating rt_band during deserialization", __func__);
			return rt_raster_deserialize_failed(rast, i);
		}
		memset(band, 0, sizeof(struct rt_band_t));

		rast->bands[i] = band;
		if (ptr == end)
			return rt_raster_deserialize_truncated(__func__, rast, i, "type");

		type = *ptr;
		ptr++;
		band->pixtype = type & BANDTYPE_PIXTYPE_MASK;
		pixbytes = rt_pixtype_size_for(__func__, band->pixtype);
		if (pixbytes < 1)
			return rt_raster_deserialize_failed(rast, i + 1);

		RASTER_DEBUGF(
		    3, "rt_raster_deserialize: band %u with pixel type %s", i, rt_pixtype_name(band->pixtype));

		band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
		band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
		band->isnodata = band->hasnodata ? (BANDTYPE_IS_NODATA(type) ? 1 : 0) : 0;
		band->width = rast->width;
		band->height = rast->height;
		band->ownsdata = 0; /* we do NOT own this data!!! */
		band->raster = rast;

		/* Check the combined padding and nodata length before reading either. */
		if (2 * pixbytes - 1 > end - ptr)
			return rt_raster_deserialize_truncated(__func__, rast, i, "nodata");
		ptr += pixbytes - 1;

		/* Read nodata value */
		switch (band->pixtype)
		{
		case PT_1BB: {
			band->nodataval = ((int)read_uint8(&ptr)) & 0x01;
			break;
		}
		case PT_2BUI: {
			band->nodataval = ((int)read_uint8(&ptr)) & 0x03;
			break;
		}
		case PT_4BUI: {
			band->nodataval = ((int)read_uint8(&ptr)) & 0x0F;
			break;
		}
		case PT_8BSI: {
			band->nodataval = read_int8(&ptr);
			break;
		}
		case PT_8BUI: {
			band->nodataval = read_uint8(&ptr);
			break;
		}
		case PT_16BSI: {
			band->nodataval = read_int16(&ptr, littleEndian);
			break;
		}
		case PT_16BUI: {
			band->nodataval = read_uint16(&ptr, littleEndian);
			break;
		}
		case PT_32BSI: {
			band->nodataval = read_int32(&ptr, littleEndian);
			break;
		}
		case PT_32BUI: {
			band->nodataval = read_uint32(&ptr, littleEndian);
			break;
		}
		case PT_16BF: {
			band->nodataval = rt_util_float16_to_float(read_uint16(&ptr, littleEndian));
			break;
		}
		case PT_32BF: {
			band->nodataval = read_float32(&ptr, littleEndian);
			break;
		}
		case PT_64BF: {
			band->nodataval = read_float64(&ptr, littleEndian);
			break;
		}
		default: {
			rterror("%s: Unknown pixeltype %d", __func__, band->pixtype);
			return rt_raster_deserialize_failed(rast, i + 1);
		}
		}

		RASTER_DEBUGF(3, "rt_raster_deserialize: has nodata flag %d", band->hasnodata);
		RASTER_DEBUGF(3, "rt_raster_deserialize: nodata value %g", band->nodataval);

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - beg) % pixbytes));

		if (band->offline)
		{
			size_t pathlen = 0;
			const uint8_t *pathend;

			/* Read band number */
			if (ptr == end)
				return rt_raster_deserialize_truncated(__func__, rast, i, "external band number");
			band->data.offline.bandNum = *ptr;
			ptr += 1;

			/* Register path */
			pathend = memchr(ptr, '\0', rt_util_size_from_signed(end - ptr));
			if (!pathend)
			{
				rterror("%s: Unterminated offline band path", __func__);
				return rt_raster_deserialize_failed(rast, i + 1);
			}
			pathlen = rt_util_size_from_signed(pathend - ptr);
			band->data.offline.path = rtalloc(pathlen + 1);
			if (!band->data.offline.path)
			{
				rterror("%s: Could not allocate memory for offline band path", __func__);
				return rt_raster_deserialize_failed(rast, i + 1);
			}

			memcpy(band->data.offline.path, ptr, pathlen);
			band->data.offline.path[pathlen] = '\0';
			ptr += pathlen + 1;

			band->data.offline.mem = NULL;
		}
		else
		{
			/* Register data */
			size_t datasize;
			if (!rt_band_data_size(__func__, rast->width, rast->height, pixbytes, SIZE_MAX, &datasize))
				return rt_raster_deserialize_failed(rast, i + 1);
			if (datasize > rt_util_size_from_signed(end - ptr))
				return rt_raster_deserialize_truncated(__func__, rast, i, "data");
			band->data.mem = (uint8_t *)ptr;
			ptr += datasize;
		}

		/* Skip bytes of padding up to 8-bytes boundary */
#if POSTGIS_DEBUG_LEVEL > 0
		const uint8_t *padbeg = ptr;
#endif
		{
			size_t padding = (8 - (rt_util_size_from_signed(ptr - beg) % 8)) % 8;
			if (padding > rt_util_size_from_signed(end - ptr))
				return rt_raster_deserialize_truncated(__func__, rast, i, "padding");
			ptr += padding;
		}

		RASTER_DEBUGF(3, "rt_raster_deserialize: skip %td bytes of 8-bytes boundary padding", ptr - padbeg);

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - beg) % pixbytes));
	}

	return rast;
}

rt_raster
rt_raster_deserialize_with_size(const void *serialized, size_t serialized_size, int header_only)
{ return rt_raster_deserialize_with_size_internal(serialized, serialized_size, header_only, FALSE); }

rt_raster
rt_raster_deserialize_pgraster(const void *serialized, size_t serialized_size, int header_only)
{ return rt_raster_deserialize_with_size_internal(serialized, serialized_size, header_only, TRUE); }

rt_raster
rt_raster_deserialize(void *serialized, int header_only)
{
	const struct rt_raster_serialized_t *rast = serialized;

	assert(serialized);
	return rt_raster_deserialize_with_size(serialized, rast->size, header_only);
}
