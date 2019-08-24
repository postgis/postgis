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
#include <setjmp.h>

#include <set>
extern "C" {
#include "liblwgeom.h"
#include "geos_stub.h"
#include "proj_stub.h"
}
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv);

extern "C" {
static void
noticereporter(const char *, va_list)
{}
}

static void
debuglogger(int, const char *, va_list)
{}

int
LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
	lwgeom_set_handlers(malloc, realloc, free, noticereporter, noticereporter);
	lwgeom_set_debuglogger(debuglogger);
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len);

int
LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
	char *srs = NULL;
	char *psz = static_cast<char *>(malloc(len + 1));
	memcpy(psz, buf, len);
	psz[len] = '\0';
	LWGEOM *lwgeom = lwgeom_from_geojson(psz, &srs);
	lwgeom_free(lwgeom);
	lwfree(srs);
	free(psz);
	return 0;
}
