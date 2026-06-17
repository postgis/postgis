/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  Raster serialized input fuzzer
 *
 ******************************************************************************
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "librtcore.h"
#include "rt_serialize.h"

static jmp_buf jmpBuf;
static void *current_serialized = NULL;

static void
cleanup_serialized(void)
{
	free(current_serialized);
	current_serialized = NULL;
}

static void
rt_message_handler_noop(const char *fmt, va_list ap)
{
	(void)fmt;
	(void)ap;
}

static void
rt_error_handler(const char *fmt, va_list ap)
{
	(void)fmt;
	(void)ap;
	cleanup_serialized();
	longjmp(jmpBuf, 1);
}

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
	(void)argc;
	(void)argv;
	rt_set_handlers(malloc, realloc, free, rt_error_handler, rt_message_handler_noop, rt_message_handler_noop);
	return 0;
}

static int
checked_add(size_t *total, size_t value, size_t limit)
{
	if (value > limit || *total > limit - value)
		return 0;
	*total += value;
	return 1;
}

static int
has_complete_serialized_raster(const uint8_t *buf, size_t len)
{
	struct rt_raster_serialized_t header;
	size_t pos = sizeof(header);

	/* rt_raster_deserialize() accepts a pointer but no buffer length. Before
	 * calling it, walk the serialized layout far enough to prove each band,
	 * nodata value, optional out-db path, pixel payload, and 8-byte padding byte
	 * is inside the fuzzer-provided buffer.
	 */
	if (len < sizeof(header))
		return 0;

	memcpy(&header, buf, sizeof(header));

	if (header.numBands > 256)
		return 0;

	for (uint16_t i = 0; i < header.numBands; i++)
	{
		if (pos >= len)
			return 0;

		const uint8_t band_type = buf[pos];
		const rt_pixtype pixtype = (rt_pixtype)(band_type & BANDTYPE_PIXTYPE_MASK);
		const int pixbytes = rt_pixtype_size(pixtype);
		if (pixbytes <= 0)
			return 0;

		/* Band type byte, nodata padding bytes, and nodata value. */
		if (!checked_add(&pos, 1, len))
			return 0;
		if (!checked_add(&pos, (size_t)(pixbytes - 1), len))
			return 0;
		if (!checked_add(&pos, (size_t)pixbytes, len))
			return 0;

		if (BANDTYPE_IS_OFFDB(band_type))
		{
			/* Out-db band number plus NUL-terminated path. */
			if (!checked_add(&pos, 1, len))
				return 0;

			const void *nul = memchr(buf + pos, '\0', len - pos);
			if (nul == NULL)
				return 0;

			const size_t path_len = (const uint8_t *)nul - (buf + pos);
			if (!checked_add(&pos, path_len + 1, len))
				return 0;
		}
		else
		{
			/* Inline raster data is width * height pixels in this band's
			 * pixel type.
			 */
			const size_t pixels = (size_t)header.width * (size_t)header.height;
			if (header.width != 0 && pixels / header.width != header.height)
				return 0;
			if (pixbytes > 0 && pixels > len / (size_t)pixbytes)
				return 0;
			if (!checked_add(&pos, pixels * (size_t)pixbytes, len))
				return 0;
		}

		while (pos % 8 != 0)
		{
			if (!checked_add(&pos, 1, len))
				return 0;
		}
	}

	return 1;
}

static void
destroy_raster_with_bands(rt_raster raster)
{
	uint16_t num_bands;

	if (raster == NULL)
		return;

	num_bands = rt_raster_get_num_bands(raster);
	for (uint16_t i = 0; i < num_bands; i++)
	{
		rt_band band = rt_raster_get_band(raster, i);
		if (band != NULL)
			rt_band_destroy(band);
	}

	rt_raster_destroy(raster);
}

int
LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	if (setjmp(jmpBuf))
	{
		cleanup_serialized();
		return 0;
	}

	if (!has_complete_serialized_raster(buf, len))
		return 0;

	current_serialized = malloc(len);
	if (current_serialized == NULL)
		return 0;

	memcpy(current_serialized, buf, len);
	rt_raster raster = rt_raster_deserialize(current_serialized, 0);
	destroy_raster_with_bands(raster);
	cleanup_serialized();
	return 0;
}
