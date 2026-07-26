/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  GeoHash input fuzzer
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "geos_stub.h"
#include "proj_stub.h"
#include "liblwgeom_internal.h"
}

#include "liblwgeom_fuzzer.hpp"

extern "C" int
LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
	postgis_lwgeom_fuzzer_initialize();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len);

int
LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	if (len < 2)
		return 0;

	char *geohash = static_cast<char *>(malloc(len));
	if (geohash == NULL)
		return 0;

	memcpy(geohash, buf + 1, len - 1);
	geohash[len - 1] = '\0';
	int precision = (buf[0] % 2) ? -1 : (int)(buf[0] % len);
	double lat[2];
	double lon[2];

	if (!POSTGIS_LWGEOM_FUZZER_SETJMP())
		decode_geohash_bbox(geohash, lat, lon, precision);
	postgis_lwgeom_fuzzer_cleanup_allocations();
	free(geohash);
	return 0;
}
