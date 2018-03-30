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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <set>

extern "C"
{
#include "liblwgeom.h"

void GEOSCoordSeq_destroy() { assert(0); }
void GEOSClipByRect() { assert(0); }
void GEOSUnion() { assert(0); }
void GEOSCoordSeq_getDimensions() { assert(0); }
void GEOSPreparedCovers() { assert(0); }
void GEOSPreparedContains() { assert(0); }
void GEOSSymDifference() { assert(0); }
void GEOSUnionCascaded() { assert(0); }
void GEOSGetExteriorRing() { assert(0); }
void GEOSCoordSeq_setX() { assert(0); }
void GEOSGeom_createLineString() { assert(0); }
void GEOSCoordSeq_getY() { assert(0); }
void GEOSEquals() { assert(0); }
void GEOSRelatePatternMatch() { assert(0); }
void GEOSGeom_createCollection() { assert(0); }
void GEOSGeom_extractUniquePoints() { assert(0); }
void GEOSNormalize() { assert(0); }
void GEOSVoronoiDiagram() { assert(0); }
void GEOSArea() { assert(0); }
void GEOSLineMerge() { assert(0); }
void GEOSGeom_createPolygon() { assert(0); }
void GEOSGetCentroid() { assert(0); }
void GEOSCoordSeq_create() { assert(0); }
void GEOSFree() { assert(0); }
void initGEOS() { assert(0); }
void GEOSIntersection() { assert(0); }
void GEOSEnvelope() { assert(0); }
void GEOSGetGeometryN() { assert(0); }
void GEOSSTRtree_insert() { assert(0); }
void GEOSGeomTypeId() { assert(0); }
void GEOSBoundary() { assert(0); }
void GEOSversion() { assert(0); }
void GEOSGetInteriorRingN() { assert(0); }
void GEOSCoordSeq_setY() { assert(0); }
void GEOSGetSRID() { assert(0); }
void GEOSGeom_destroy() { assert(0); }
void GEOSGeom_createEmptyPolygon() { assert(0); }
void GEOSPolygonize() { assert(0); }
void GEOSCoordSeq_getX() { assert(0); }
void GEOSSharedPaths() { assert(0); }
void GEOSSTRtree_create() { assert(0); }
void GEOSGeom_clone() { assert(0); }
void GEOSRelateBoundaryNodeRule() { assert(0); }
void GEOSSnap() { assert(0); }
void GEOSRelatePattern() { assert(0); }
void GEOSSetSRID() { assert(0); }
void GEOSisValid() { assert(0); }
void GEOSContains() { assert(0); }
void GEOSPreparedGeom_destroy() { assert(0); }
void GEOSCoordSeq_setZ() { assert(0); }
void GEOSOffsetCurve() { assert(0); }
void GEOSUnaryUnion() { assert(0); }
void GEOSPrepare() { assert(0); }
void GEOSCoordSeq_getSize() { assert(0); }
void GEOSGetNumInteriorRings() { assert(0); }
void GEOSGetNumGeometries() { assert(0); }
void GEOSisSimple() { assert(0); }
void GEOSDifference() { assert(0); }
void GEOSPreparedIntersects() { assert(0); }
void GEOSisEmpty() { assert(0); }
void GEOSPointOnSurface() { assert(0); }
void GEOSSTRtree_query() { assert(0); }
void GEOSGeom_createPoint() { assert(0); }
void GEOSSTRtree_destroy() { assert(0); }
void GEOSIntersects() { assert(0); }
void GEOSHasZ() { assert(0); }
void GEOSGeom_getCoordSeq() { assert(0); }
void GEOSCoordSeq_getZ() { assert(0); }
void GEOSGeom_createLinearRing() { assert(0); }
void GEOSGeomType() { assert(0); }
void GEOSDelaunayTriangulation() { assert(0); }
void GEOSNode() { assert(0); }

void geod_init() { assert(0); }
void geod_inverse() { assert(0); }
void geod_direct() { assert(0); }
void geod_polygon_init() { assert(0); }
void geod_polygon_addpoint() { assert(0); }
void geod_polygon_compute() { assert(0); }

}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv);

// Keep active heap allocated memory corresponding to returns of allocator()
// and reallocator()
std::set<void*> oSetPointers;
jmp_buf jmpBuf;

extern "C"
{
    static void *
    allocator(size_t size)
    {
            void *mem = malloc(size);
            oSetPointers.insert(mem);
            return mem;
    }

    static void
    freeor(void *mem)
    {
            oSetPointers.erase(mem);
            free(mem);
    }

    static void *
    reallocator(void *mem, size_t size)
    {
            oSetPointers.erase(mem);
            void *ret = realloc(mem, size);
            oSetPointers.insert(ret);
            return ret;
    }

    static void
    noticereporter(const char *, va_list )
    {
    }

    static void
    errorreporter(const char *, va_list )
    {
        // Cleanup any heap-allocated memory still active
        for(std::set<void*>::iterator oIter = oSetPointers.begin();
            oIter != oSetPointers.end(); ++oIter )
        {
            free(*oIter);
        }
        oSetPointers.clear();
        // Abort everything to jump to setjmp() call
        longjmp(jmpBuf, 1);
    }

    static void
    debuglogger(int, const char *, va_list)
    {
    }

}

int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    lwgeom_set_handlers(allocator, reallocator, freeor, errorreporter, noticereporter);
    lwgeom_set_debuglogger(debuglogger);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len);

int LLVMFuzzerTestOneInput(const uint8_t *buf, size_t len)
{
    char* pszWKT = static_cast<char*>(malloc( len + 1 ));
    memcpy(pszWKT, buf, len);
    pszWKT[len] = '\0';
    if( !setjmp(jmpBuf) )
    {
        LWGEOM* lwgeom = lwgeom_from_wkt(pszWKT, LW_PARSER_CHECK_NONE);
        lwgeom_free(lwgeom);
        //assert( oSetPointers.empty() );
    }
    free(pszWKT);
    return 0;
}
