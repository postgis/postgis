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

static const size_t POSTGIS_FUZZER_MAX_GSERIALIZED_SIZE = 16 * 1024 * 1024;

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
	postgis_fuzzer_assert(first_size == LWSIZE_GET(first->size));
	postgis_fuzzer_assert(second_size == LWSIZE_GET(second->size));
	postgis_fuzzer_assert(lwgeom->type == from_bytea->type);
	postgis_fuzzer_assert(clamp_srid(lwgeom->srid) == from_bytea->srid);
	postgis_fuzzer_assert(FLAGS_GET_Z(lwgeom->flags) == FLAGS_GET_Z(from_bytea->flags));
	postgis_fuzzer_assert(FLAGS_GET_M(lwgeom->flags) == FLAGS_GET_M(from_bytea->flags));
	postgis_fuzzer_assert(FLAGS_GET_GEODETIC(lwgeom->flags) == FLAGS_GET_GEODETIC(from_bytea->flags));

	/* WKB bytea does not preserve GSERIALIZED-only metadata. Arbitrary
	 * GSERIALIZED input can carry a bbox or out-of-range SRID that is
	 * normalized after geometry->bytea->geometry, so compare the semantic
	 * geometry after applying the same normalization.
	 */
	lwgeom_drop_bbox(lwgeom);
	lwgeom_drop_bbox(from_bytea);
	lwgeom_set_srid(lwgeom, from_bytea->srid);
	postgis_fuzzer_assert(lwgeom_same(lwgeom, from_bytea));

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

	uint32_t size_header;
	memcpy(&size_header, buf, sizeof(size_header));
	const size_t declared_size = LWSIZE_GET(size_header);
	if (declared_size > POSTGIS_FUZZER_MAX_GSERIALIZED_SIZE)
		return 0;

	const size_t allocation_size = declared_size > len ? declared_size : len;

	/* Leave the varlena size header untouched. The first four bytes are
	 * attacker-controlled when GSERIALIZED arrives from a damaged page,
	 * binary COPY, bytea cast, or hostile dump. When the declared size is
	 * larger than the supplied testcase, zero-fill the missing tail so the
	 * parser can validate the declared buffer without UBSAN builds reading
	 * past the fuzzer allocation.
	 */
	GSERIALIZED *gserialized = static_cast<GSERIALIZED *>(postgis_lwgeom_fuzzer_malloc(allocation_size));
	if (gserialized == NULL)
		return 0;
	memset(gserialized, 0, allocation_size);
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
