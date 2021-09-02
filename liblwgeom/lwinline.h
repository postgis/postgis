/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2018 Darafei Praliaskouski <me@komzpa.net>
 * Copyright 2017-2018 Daniel Baston <dbaston@gmail.com>
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/

#if PARANOIA_LEVEL > 0
#include <assert.h>
#endif

inline static double
distance2d_sqr_pt_pt(const POINT2D *p1, const POINT2D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;

	return hside * hside + vside * vside;
}

inline static double
distance3d_sqr_pt_pt(const POINT3D *p1, const POINT3D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;
	double zside = p2->z - p1->z;

	return hside * hside + vside * vside + zside * zside;
}

/*
 * Size of point represeneted in the POINTARRAY
 * 16 for 2d, 24 for 3d, 32 for 4d
 */
static inline size_t
ptarray_point_size(const POINTARRAY *pa)
{
	return sizeof(double) * FLAGS_NDIMS(pa->flags);
}

/*
 * Get a pointer to Nth point of a POINTARRAY
 * You'll need to cast it to appropriate dimensioned point.
 * Note that if you cast to a higher dimensional point you'll
 * possibly corrupt the POINTARRAY.
 *
 * Casting to returned pointer to POINT2D* should be safe,
 * as gserialized format always keeps the POINTARRAY pointer
 * aligned to double boundary.
 *
 * WARNING: Don't cast this to a POINT!
 * it would not be reliable due to memory alignment constraints
 */
static inline uint8_t *
getPoint_internal(const POINTARRAY *pa, uint32_t n)
{
	size_t size;
	uint8_t *ptr;

#if PARANOIA_LEVEL > 0
	assert(pa);
	assert(n <= pa->npoints);
	assert(n <= pa->maxpoints);
#endif

	size = ptarray_point_size(pa);
	ptr = pa->serialized_pointlist + size * n;

	return ptr;
}

/**
 * Returns a POINT2D pointer into the POINTARRAY serialized_ptlist,
 * suitable for reading from. This is very high performance
 * and declared const because you aren't allowed to muck with the
 * values, only read them.
 */
static inline const POINT2D *
getPoint2d_cp(const POINTARRAY *pa, uint32_t n)
{
	return (const POINT2D *)getPoint_internal(pa, n);
}

/**
 * Returns a POINT3D pointer into the POINTARRAY serialized_ptlist,
 * suitable for reading from. This is very high performance
 * and declared const because you aren't allowed to muck with the
 * values, only read them.
 */
static inline const POINT3D *
getPoint3d_cp(const POINTARRAY *pa, uint32_t n)
{
	return (const POINT3D *)getPoint_internal(pa, n);
}

/**
 * Returns a POINT4D pointer into the POINTARRAY serialized_ptlist,
 * suitable for reading from. This is very high performance
 * and declared const because you aren't allowed to muck with the
 * values, only read them.
 */
static inline const POINT4D *
getPoint4d_cp(const POINTARRAY *pa, uint32_t n)
{
	return (const POINT4D *)getPoint_internal(pa, n);
}

static inline LWPOINT *
lwgeom_as_lwpoint(const LWGEOM *lwgeom)
{
	if (!lwgeom)
		return NULL;
	if (lwgeom->type == POINTTYPE)
		return (LWPOINT *)lwgeom;
	else
		return NULL;
}

/**
 * Return LWTYPE number
 */
static inline uint32_t
lwgeom_get_type(const LWGEOM *geom)
{
	if (!geom)
		return 0;
	return geom->type;
}

static inline int
lwpoint_is_empty(const LWPOINT *point)
{
	return !point->point || point->point->npoints < 1;
}

static inline int
lwline_is_empty(const LWLINE *line)
{
	return !line->points || line->points->npoints < 1;
}

static inline int
lwcircstring_is_empty(const LWCIRCSTRING *circ)
{
	return !circ->points || circ->points->npoints < 1;
}

static inline int
lwpoly_is_empty(const LWPOLY *poly)
{
	return poly->nrings < 1 || !poly->rings || !poly->rings[0] || poly->rings[0]->npoints < 1;
}

static inline int
lwtriangle_is_empty(const LWTRIANGLE *triangle)
{
	return !triangle->points || triangle->points->npoints < 1;
}

static inline int lwgeom_is_empty(const LWGEOM *geom);

static inline int
lwcollection_is_empty(const LWCOLLECTION *col)
{
	uint32_t i;
	if (col->ngeoms == 0 || !col->geoms)
		return LW_TRUE;
	for (i = 0; i < col->ngeoms; i++)
	{
		if (!lwgeom_is_empty(col->geoms[i]))
			return LW_FALSE;
	}
	return LW_TRUE;
}

/**
 * Return true or false depending on whether a geometry is an "empty"
 * geometry (no vertices members)
 */
static inline int
lwgeom_is_empty(const LWGEOM *geom)
{
	switch (geom->type)
	{
	case POINTTYPE:
		return lwpoint_is_empty((LWPOINT *)geom);
		break;
	case LINETYPE:
		return lwline_is_empty((LWLINE *)geom);
		break;
	case CIRCSTRINGTYPE:
		return lwcircstring_is_empty((LWCIRCSTRING *)geom);
		break;
	case POLYGONTYPE:
		return lwpoly_is_empty((LWPOLY *)geom);
		break;
	case TRIANGLETYPE:
		return lwtriangle_is_empty((LWTRIANGLE *)geom);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return lwcollection_is_empty((LWCOLLECTION *)geom);
		break;
	default:
		return LW_FALSE;
		break;
	}
}

inline static uint64_t
uint64_interleave_2(uint64_t x, uint64_t y)
{
	x = (x | (x << 16)) & 0x0000FFFF0000FFFFULL;
	x = (x | (x << 8)) & 0x00FF00FF00FF00FFULL;
	x = (x | (x << 4)) & 0x0F0F0F0F0F0F0F0FULL;
	x = (x | (x << 2)) & 0x3333333333333333ULL;
	x = (x | (x << 1)) & 0x5555555555555555ULL;

	y = (y | (y << 16)) & 0x0000FFFF0000FFFFULL;
	y = (y | (y << 8)) & 0x00FF00FF00FF00FFULL;
	y = (y | (y << 4)) & 0x0F0F0F0F0F0F0F0FULL;
	y = (y | (y << 2)) & 0x3333333333333333ULL;
	y = (y | (y << 1)) & 0x5555555555555555ULL;

	return x | (y << 1);
}

/* Based on https://github.com/rawrunprotected/hilbert_curves Public Domain code */
inline static uint64_t
uint32_hilbert(uint32_t px, uint32_t py)
{
	uint64_t x = px;
	uint64_t y = py;

	uint64_t A, B, C, D;
	uint64_t a, b, c, d;
	uint64_t i0, i1;

	// Initial prefix scan round, prime with x and y
	{
		a = x ^ y;
		b = 0xFFFFFFFFULL ^ a;
		c = 0xFFFFFFFFULL ^ (x | y);
		d = x & (y ^ 0xFFFFFFFFULL);

		A = a | (b >> 1);
		B = (a >> 1) ^ a;
		C = ((c >> 1) ^ (b & (d >> 1))) ^ c;
		D = ((a & (c >> 1)) ^ (d >> 1)) ^ d;
	}

	{
		a = A;
		b = B;
		c = C;
		d = D;

		A = ((a & (a >> 2)) ^ (b & (b >> 2)));
		B = ((a & (b >> 2)) ^ (b & ((a ^ b) >> 2)));
		C ^= ((a & (c >> 2)) ^ (b & (d >> 2)));
		D ^= ((b & (c >> 2)) ^ ((a ^ b) & (d >> 2)));
	}

	{
		a = A;
		b = B;
		c = C;
		d = D;

		A = ((a & (a >> 4)) ^ (b & (b >> 4)));
		B = ((a & (b >> 4)) ^ (b & ((a ^ b) >> 4)));
		C ^= ((a & (c >> 4)) ^ (b & (d >> 4)));
		D ^= ((b & (c >> 4)) ^ ((a ^ b) & (d >> 4)));
	}

	{
		a = A;
		b = B;
		c = C;
		d = D;

		A = ((a & (a >> 8)) ^ (b & (b >> 8)));
		B = ((a & (b >> 8)) ^ (b & ((a ^ b) >> 8)));
		C ^= ((a & (c >> 8)) ^ (b & (d >> 8)));
		D ^= ((b & (c >> 8)) ^ ((a ^ b) & (d >> 8)));
	}

	{
		a = A;
		b = B;
		c = C;
		d = D;

		C ^= ((a & (c >> 16)) ^ (b & (d >> 16)));
		D ^= ((b & (c >> 16)) ^ ((a ^ b) & (d >> 16)));
	}

	// Undo transformation prefix scan
	a = C ^ (C >> 1);
	b = D ^ (D >> 1);

	// Recover index bits
	i0 = x ^ y;
	i1 = b | (0xFFFFFFFFULL ^ (i0 | a));

	return uint64_interleave_2(i0, i1);
}

/*
 * This macro is based on PG_FREE_IF_COPY, except that it accepts two pointers.
 * See PG_FREE_IF_COPY comment in src/include/fmgr.h in postgres source code
 * for more details.
 */
#define POSTGIS_FREE_IF_COPY_P(ptrsrc, ptrori) \
	do \
	{ \
		if ((Pointer)(ptrsrc) != (Pointer)(ptrori)) \
			pfree(ptrsrc); \
	} while (0)
