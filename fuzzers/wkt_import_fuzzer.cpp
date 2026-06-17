/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  Fuzzer
 * Author:   Even Rouault, even.rouault at spatialys.com
 *
 ******************************************************************************
 * Copyright (c) 2017, Even Rouault <even.rouault at spatialys.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

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

int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	char *pszWKT = static_cast<char *>(malloc(len + 1));
	if (pszWKT == NULL)
		return 0;

	memcpy(pszWKT, buf, len);
	pszWKT[len] = '\0';
	if (!POSTGIS_LWGEOM_FUZZER_SETJMP())
	{
		LWGEOM *lwgeom = lwgeom_from_wkt(pszWKT, LW_PARSER_CHECK_NONE);
		lwgeom_free(lwgeom);
	}
	postgis_lwgeom_fuzzer_cleanup_allocations();
	free(pszWKT);
	return 0;
}
