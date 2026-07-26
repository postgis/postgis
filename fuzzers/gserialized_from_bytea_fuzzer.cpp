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

static void
assert_idempotent_serialization(LWGEOM *lwgeom)
{
	size_t first_size = 0;
	size_t second_size = 0;
	GSERIALIZED *first = gserialized_from_lwgeom(lwgeom, &first_size);
	LWGEOM *roundtrip = lwgeom_from_gserialized(first);
	GSERIALIZED *second = gserialized_from_lwgeom(roundtrip, &second_size);

	postgis_fuzzer_assert(first != NULL);
	postgis_fuzzer_assert(roundtrip != NULL);
	postgis_fuzzer_assert(second != NULL);
	postgis_fuzzer_assert(first_size == LWSIZE_GET(first->size));
	postgis_fuzzer_assert(second_size == LWSIZE_GET(second->size));
	postgis_fuzzer_assert(first_size == second_size);
	postgis_fuzzer_assert(memcmp(first, second, first_size) == 0);

	lwgeom_free(roundtrip);
	lwfree(first);
	lwfree(second);
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
		assert_idempotent_serialization(lwgeom);
		lwgeom_free(lwgeom);
	}

	postgis_lwgeom_fuzzer_free(gserialized);
	postgis_lwgeom_fuzzer_cleanup_allocations();
	return 0;
}
