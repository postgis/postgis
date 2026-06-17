/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  TWKB input fuzzer
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

extern "C" {
#include "geos_stub.h"
#include "proj_stub.h"
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
	/* Avoid inputs too short to contain TWKB metadata. If the metadata says an
	 * extended precision byte follows, make sure that byte exists too.
	 */
	if (len < 2)
		return 0;
	if ((buf[1] & 0x08) && len < 3)
		return 0;

	if (POSTGIS_LWGEOM_FUZZER_SETJMP())
	{
		postgis_lwgeom_fuzzer_cleanup_allocations();
		return 0;
	}

	LWGEOM *lwgeom = lwgeom_from_twkb(buf, len, LW_PARSER_CHECK_NONE);
	lwgeom_free(lwgeom);
	postgis_lwgeom_fuzzer_cleanup_allocations();
	return 0;
}
