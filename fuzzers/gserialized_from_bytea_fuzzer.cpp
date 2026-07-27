/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  GSERIALIZED input fuzzer
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

static void
postgis_fuzzer_assert(int condition)
{
	if (!condition)
		abort();
}

static lwvarlena_t *
geometry_to_bytea(const LWGEOM *lwgeom)
{
	return lwgeom_to_wkb_varlena(lwgeom, WKB_NDR | WKB_EXTENDED);
}

static void
assert_equal_gserialized(const GSERIALIZED *left, size_t left_size, const GSERIALIZED *right, size_t right_size)
{
	postgis_fuzzer_assert(left_size == LWSIZE_GET(left->size));
	postgis_fuzzer_assert(right_size == LWSIZE_GET(right->size));
	postgis_fuzzer_assert(left_size == right_size);
	postgis_fuzzer_assert(memcmp(left, right, left_size) == 0);
}

static void
assert_gserialized_bytea_roundtrip(LWGEOM *lwgeom)
{
	size_t first_size = 0;
	size_t second_size = 0;
	GSERIALIZED *first = gserialized_from_lwgeom(lwgeom, &first_size);
	postgis_fuzzer_assert(first != NULL);

	lwvarlena_t *bytea = geometry_to_bytea(lwgeom);
	postgis_fuzzer_assert(bytea != NULL);

	LWGEOM *from_bytea =
	    lwgeom_from_wkb((uint8_t *)bytea->data, LWSIZE_GET(bytea->size) - LWVARHDRSZ, LW_PARSER_CHECK_ALL);
	postgis_fuzzer_assert(from_bytea != NULL);

	GSERIALIZED *second = gserialized_from_lwgeom(from_bytea, &second_size);
	postgis_fuzzer_assert(second != NULL);
	assert_equal_gserialized(first, first_size, second, second_size);

	lwgeom_free(from_bytea);
	lwfree(first);
	lwfree(second);
	lwfree(bytea);
}

int
LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	if (len < offsetof(GSERIALIZED, data))
		return 0;

	if (POSTGIS_LWGEOM_FUZZER_SETJMP())
	{
		postgis_lwgeom_fuzzer_cleanup_allocations();
		return 0;
	}

	/* Copy exactly the bytes supplied by the fuzzer and leave g->size
	 * untouched. The first four bytes are the PostgreSQL varlena size
	 * header read through LWSIZE_GET(), and they are attacker-controlled
	 * when GSERIALIZED arrives from a damaged page, binary COPY, bytea cast,
	 * or hostile dump. Rewriting the header to len would make every input
	 * self-consistent and hide the over-declared-size cases this target is
	 * meant to exercise.
	 */
	GSERIALIZED *gserialized = static_cast<GSERIALIZED *>(postgis_lwgeom_fuzzer_malloc(len));
	if (gserialized == NULL)
		return 0;
	memcpy(gserialized, buf, len);

	LWGEOM *lwgeom = lwgeom_from_gserialized(gserialized);
	if (lwgeom != NULL)
	{
		assert_gserialized_bytea_roundtrip(lwgeom);
		lwgeom_free(lwgeom);
	}

	postgis_lwgeom_fuzzer_free(gserialized);
	postgis_lwgeom_fuzzer_cleanup_allocations();
	return 0;
}
