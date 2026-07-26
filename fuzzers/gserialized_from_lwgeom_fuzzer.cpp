/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  GSERIALIZED output property fuzzer
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

extern "C" {
#include "geos_stub.h"
#include "proj_stub.h"
}

#include "liblwgeom_fuzzer.hpp"

extern "C" size_t gserialized_from_lwgeom_size(const LWGEOM *geom);

extern "C" int
LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
	postgis_lwgeom_fuzzer_initialize();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len);

static void
postgis_fuzzer_assert(int condition)
{
	if (!condition)
		abort();
}

static void
assert_matching_gbox(const LWGEOM *input, const LWGEOM *roundtrip)
{
	if (input->bbox == NULL || roundtrip->bbox == NULL)
	{
		postgis_fuzzer_assert(input->bbox == NULL);
		postgis_fuzzer_assert(roundtrip->bbox == NULL);
		return;
	}

	postgis_fuzzer_assert(gbox_same(input->bbox, roundtrip->bbox));
}

static void
assert_matching_metadata(const LWGEOM *input, const LWGEOM *roundtrip)
{
	postgis_fuzzer_assert(input->type == roundtrip->type);
	postgis_fuzzer_assert(input->srid == roundtrip->srid);
	postgis_fuzzer_assert(FLAGS_GET_Z(input->flags) == FLAGS_GET_Z(roundtrip->flags));
	postgis_fuzzer_assert(FLAGS_GET_M(input->flags) == FLAGS_GET_M(roundtrip->flags));
	postgis_fuzzer_assert(FLAGS_GET_GEODETIC(input->flags) == FLAGS_GET_GEODETIC(roundtrip->flags));
}

int
LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	if (POSTGIS_LWGEOM_FUZZER_SETJMP())
	{
		postgis_lwgeom_fuzzer_cleanup_allocations();
		return 0;
	}

	LWGEOM *input = lwgeom_from_wkb(buf, len, LW_PARSER_CHECK_NONE);
	if (input == NULL)
	{
		postgis_lwgeom_fuzzer_cleanup_allocations();
		return 0;
	}

	size_t serialized_size = 0;
	GSERIALIZED *serialized = gserialized_from_lwgeom(input, &serialized_size);
	const size_t expected_size = gserialized_from_lwgeom_size(input);
	postgis_fuzzer_assert(serialized != NULL);
	postgis_fuzzer_assert(serialized_size == expected_size);
	postgis_fuzzer_assert(serialized_size == LWSIZE_GET(serialized->size));
	postgis_fuzzer_assert(gserialized_cmp(serialized, serialized) == 0);

	LWGEOM *roundtrip = lwgeom_from_gserialized(serialized);
	postgis_fuzzer_assert(roundtrip != NULL);
	assert_matching_metadata(input, roundtrip);
	assert_matching_gbox(input, roundtrip);
	postgis_fuzzer_assert(lwgeom_same(input, roundtrip));

	lwgeom_free(input);
	lwgeom_free(roundtrip);
	lwfree(serialized);
	postgis_lwgeom_fuzzer_cleanup_allocations();
	return 0;
}
