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
 * Copyright 2011-2020 Sandro Santilli <strk@kbt.io>
 * Copyright 2015-2018 Daniel Baston <dbaston@gmail.com>
 * Copyright 2017-2018 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "lwrandom.h"

#include <stdarg.h>
#include <stdlib.h>

LWTIN* lwtin_from_geos(const GEOSGeometry* geom, uint8_t want3d);

#define AUTOFIX LW_TRUE
#define LWGEOM_GEOS_ERRMSG_MAXSIZE 256
char lwgeom_geos_errmsg[LWGEOM_GEOS_ERRMSG_MAXSIZE];

extern void
lwgeom_geos_error(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	/* Call the supplied function */
	if (LWGEOM_GEOS_ERRMSG_MAXSIZE - 1 < vsnprintf(lwgeom_geos_errmsg, LWGEOM_GEOS_ERRMSG_MAXSIZE - 1, fmt, ap))
		lwgeom_geos_errmsg[LWGEOM_GEOS_ERRMSG_MAXSIZE - 1] = '\0';

	va_end(ap);
}

/* Destroy any non-null GEOSGeometry* pointers passed as arguments */
#define GEOS_FREE(...) \
do { \
	geos_destroy((sizeof((void*[]){__VA_ARGS__})/sizeof(void*)), __VA_ARGS__); \
} while (0)

/* Pass the latest GEOS error to lwerror, then return NULL */
#define GEOS_FAIL() \
do { \
	lwerror("%s: GEOS Error: %s", __func__, lwgeom_geos_errmsg); \
	return NULL; \
} while (0)

/* Pass the latest GEOS error to lwdebug, then return NULL */
#define GEOS_FAIL_DEBUG() \
	do \
	{ \
		lwdebug(1, "%s: GEOS Error: %s", __func__, lwgeom_geos_errmsg); \
		return NULL; \
	} while (0)

#define GEOS_FREE_AND_FAIL(...) \
do { \
	GEOS_FREE(__VA_ARGS__); \
	GEOS_FAIL(); \
} while (0)

#define GEOS_FREE_AND_FAIL_DEBUG(...) \
	do \
	{ \
		GEOS_FREE(__VA_ARGS__); \
		GEOS_FAIL_DEBUG(); \
	} while (0)

/* Return the consistent SRID of all inputs, or call lwerror
 * in case of SRID mismatch. */
#define RESULT_SRID(...) \
	(get_result_srid((sizeof((const void*[]){__VA_ARGS__})/sizeof(void*)), __func__,  __VA_ARGS__))

/* Free any non-null GEOSGeometry* pointers passed as arguments *
 * Called by GEOS_FREE, which populates 'count' */
static void geos_destroy(size_t count, ...) {
	va_list ap;
	va_start(ap, count);
	while (count--)
	{
		GEOSGeometry* g = va_arg(ap, GEOSGeometry*);
		if (g)
		{
			GEOSGeom_destroy(g);
		}
	}
}

/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY*
ptarray_from_GEOSCoordSeq(const GEOSCoordSequence* cs, uint8_t want3d)
{
	uint32_t dims = 2;
	uint32_t size = 0, i;
	POINTARRAY* pa;
	POINT4D point = { 0.0, 0.0, 0.0, 0.0 };

	LWDEBUG(2, "ptarray_fromGEOSCoordSeq called");

	if (!GEOSCoordSeq_getSize(cs, &size)) lwerror("Exception thrown");

	LWDEBUGF(4, " GEOSCoordSeq size: %d", size);

	if (want3d)
	{
		if (!GEOSCoordSeq_getDimensions(cs, &dims)) lwerror("Exception thrown");

		LWDEBUGF(4, " GEOSCoordSeq dimensions: %d", dims);

		/* forget higher dimensions (if any) */
		if (dims > 3) dims = 3;
	}

	LWDEBUGF(4, " output dimensions: %d", dims);

	pa = ptarray_construct((dims == 3), 0, size);

	for (i = 0; i < size; i++)
	{
#if POSTGIS_GEOS_VERSION < 38
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if (dims >= 3) GEOSCoordSeq_getZ(cs, i, &(point.z));
#else
		if (dims >= 3)
			GEOSCoordSeq_getXYZ(cs, i, &(point.x), &(point.y), &(point.z));
		else
			GEOSCoordSeq_getXY(cs, i, &(point.x), &(point.y));
#endif
		ptarray_set_point4d(pa, i, &point);
	}

	return pa;
}

/* Return an LWGEOM from a Geometry */
LWGEOM*
GEOS2LWGEOM(const GEOSGeometry* geom, uint8_t want3d)
{
	int type = GEOSGeomTypeId(geom);
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if (SRID == 0) SRID = SRID_UNKNOWN;

	if (want3d && !GEOSHasZ(geom))
	{
		LWDEBUG(3, "Geometry has no Z, won't provide one");
		want3d = 0;
	}

	switch (type)
	{
		const GEOSCoordSequence* cs;
		POINTARRAY *pa, **ppaa;
		const GEOSGeometry* g;
		LWGEOM** geoms;
		uint32_t i, ngeoms;

	case GEOS_POINT:
		LWDEBUG(4, "lwgeom_from_geometry: it's a Point");
		cs = GEOSGeom_getCoordSeq(geom);
		if (GEOSisEmpty(geom)) return (LWGEOM*)lwpoint_construct_empty(SRID, want3d, 0);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM*)lwpoint_construct(SRID, NULL, pa);

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		LWDEBUG(4, "lwgeom_from_geometry: it's a LineString or LinearRing");
		if (GEOSisEmpty(geom)) return (LWGEOM*)lwline_construct_empty(SRID, want3d, 0);

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM*)lwline_construct(SRID, NULL, pa);

	case GEOS_POLYGON:
		LWDEBUG(4, "lwgeom_from_geometry: it's a Polygon");
		if (GEOSisEmpty(geom)) return (LWGEOM*)lwpoly_construct_empty(SRID, want3d, 0);
		ngeoms = GEOSGetNumInteriorRings(geom);
		ppaa = lwalloc(sizeof(POINTARRAY*) * (ngeoms + 1));
		g = GEOSGetExteriorRing(geom);
		cs = GEOSGeom_getCoordSeq(g);
		ppaa[0] = ptarray_from_GEOSCoordSeq(cs, want3d);
		for (i = 0; i < ngeoms; i++)
		{
			g = GEOSGetInteriorRingN(geom, i);
			cs = GEOSGeom_getCoordSeq(g);
			ppaa[i + 1] = ptarray_from_GEOSCoordSeq(cs, want3d);
		}
		return (LWGEOM*)lwpoly_construct(SRID, NULL, ngeoms + 1, ppaa);

	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
		LWDEBUG(4, "lwgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if (ngeoms)
		{
			geoms = lwalloc(sizeof(LWGEOM*) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				g = GEOSGetGeometryN(geom, i);
				geoms[i] = GEOS2LWGEOM(g, want3d);
			}
		}
		return (LWGEOM*)lwcollection_construct(type, SRID, NULL, ngeoms, geoms);

	default:
		lwerror("GEOS2LWGEOM: unknown geometry type: %d", type);
		return NULL;
	}
}

GEOSCoordSeq ptarray_to_GEOSCoordSeq(const POINTARRAY*, uint8_t fix_ring);

GEOSCoordSeq
ptarray_to_GEOSCoordSeq(const POINTARRAY* pa, uint8_t fix_ring)
{
	uint32_t dims = 2;
	uint32_t i;
	int append_points = 0;
	const POINT3D *p3d = NULL;
	const POINT2D* p2d = NULL;
	GEOSCoordSeq sq;

	if (FLAGS_GET_Z(pa->flags)) dims = 3;

	if (fix_ring)
	{
		if (pa->npoints < 1)
		{
			lwerror("ptarray_to_GEOSCoordSeq called with fix_ring and 0 vertices in ring, cannot fix");
			return NULL;
		}
		else
		{
			if (pa->npoints < 4) append_points = 4 - pa->npoints;
			if (!ptarray_is_closed_2d(pa) && append_points == 0) append_points = 1;
		}
	}

	if (!(sq = GEOSCoordSeq_create(pa->npoints + append_points, dims)))
	{
		lwerror("Error creating GEOS Coordinate Sequence");
		return NULL;
	}

	for (i = 0; i < pa->npoints; i++)
	{
		if (dims == 3)
		{
			p3d = getPoint3d_cp(pa, i);
			p2d = (const POINT2D*)p3d;
			LWDEBUGF(4, "Point: %g,%g,%g", p3d->x, p3d->y, p3d->z);
		}
		else
		{
			p2d = getPoint2d_cp(pa, i);
			LWDEBUGF(4, "Point: %g,%g", p2d->x, p2d->y);
		}

#if POSTGIS_GEOS_VERSION < 38
		GEOSCoordSeq_setX(sq, i, p2d->x);
		GEOSCoordSeq_setY(sq, i, p2d->y);
		if (dims == 3) GEOSCoordSeq_setZ(sq, i, p3d->z);
#else
		if (dims == 3)
			GEOSCoordSeq_setXYZ(sq, i, p2d->x, p2d->y, p3d->z);
		else
			GEOSCoordSeq_setXY(sq, i, p2d->x, p2d->y);
#endif

	}

	if (append_points)
	{
		if (dims == 3)
		{
			p3d = getPoint3d_cp(pa, 0);
			p2d = (const POINT2D*)p3d;
		}
		else
			p2d = getPoint2d_cp(pa, 0);
		for (i = pa->npoints; i < pa->npoints + append_points; i++)
		{
#if POSTGIS_GEOS_VERSION < 38
			GEOSCoordSeq_setX(sq, i, p2d->x);
			GEOSCoordSeq_setY(sq, i, p2d->y);
#else
			GEOSCoordSeq_setXY(sq, i, p2d->x, p2d->y);
#endif

			if (dims == 3) GEOSCoordSeq_setZ(sq, i, p3d->z);
		}
	}

	return sq;
}

static inline GEOSGeometry*
ptarray_to_GEOSLinearRing(const POINTARRAY* pa, uint8_t autofix)
{
	GEOSCoordSeq sq;
	GEOSGeom g;
	sq = ptarray_to_GEOSCoordSeq(pa, autofix);
	g = GEOSGeom_createLinearRing(sq);
	return g;
}

GEOSGeometry*
GBOX2GEOS(const GBOX* box)
{
	GEOSGeometry* envelope;
	GEOSGeometry* ring;
	GEOSCoordSequence* seq = GEOSCoordSeq_create(5, 2);
	if (!seq) return NULL;

#if POSTGIS_GEOS_VERSION < 38
	GEOSCoordSeq_setX(seq, 0, box->xmin);
	GEOSCoordSeq_setY(seq, 0, box->ymin);

	GEOSCoordSeq_setX(seq, 1, box->xmax);
	GEOSCoordSeq_setY(seq, 1, box->ymin);

	GEOSCoordSeq_setX(seq, 2, box->xmax);
	GEOSCoordSeq_setY(seq, 2, box->ymax);

	GEOSCoordSeq_setX(seq, 3, box->xmin);
	GEOSCoordSeq_setY(seq, 3, box->ymax);

	GEOSCoordSeq_setX(seq, 4, box->xmin);
	GEOSCoordSeq_setY(seq, 4, box->ymin);
#else
	GEOSCoordSeq_setXY(seq, 0, box->xmin, box->ymin);
	GEOSCoordSeq_setXY(seq, 1, box->xmax, box->ymin);
	GEOSCoordSeq_setXY(seq, 2, box->xmax, box->ymax);
	GEOSCoordSeq_setXY(seq, 3, box->xmin, box->ymax);
	GEOSCoordSeq_setXY(seq, 4, box->xmin, box->ymin);
#endif

	ring = GEOSGeom_createLinearRing(seq);
	if (!ring)
	{
		GEOSCoordSeq_destroy(seq);
		return NULL;
	}

	envelope = GEOSGeom_createPolygon(ring, NULL, 0);
	if (!envelope)
	{
		GEOSGeom_destroy(ring);
		return NULL;
	}

	return envelope;
}

GEOSGeometry*
LWGEOM2GEOS(const LWGEOM* lwgeom, uint8_t autofix)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell;
	GEOSGeom* geoms = NULL;
	uint32_t ngeoms, i, j;
	int geostype;
#if LWDEBUG_LEVEL >= 4
	char* wkt;
#endif

	if (autofix)
	{
		/* cross fingers and try without autofix, maybe it'll work? */
		g = LWGEOM2GEOS(lwgeom, LW_FALSE);
		if (g) return g;
	}

	LWDEBUGF(4, "LWGEOM2GEOS got a %s", lwtype_name(lwgeom->type));

	if (lwgeom_type_arc(lwgeom))
	{
		LWGEOM* lwgeom_stroked = lwgeom_stroke(lwgeom, 32);
		GEOSGeometry* g = LWGEOM2GEOS(lwgeom_stroked, autofix);
		lwgeom_free(lwgeom_stroked);
		return g;
	}

	LWPOINT* lwp = NULL;
	LWPOLY* lwpoly = NULL;
	LWLINE* lwl = NULL;
	LWCOLLECTION* lwc = NULL;

	switch (lwgeom->type)
	{
	case POINTTYPE:
		lwp = (LWPOINT*)lwgeom;

		if (lwgeom_is_empty(lwgeom))
			g = GEOSGeom_createEmptyPoint();
		else
		{
#if POSTGIS_GEOS_VERSION < 38
			sq = ptarray_to_GEOSCoordSeq(lwp->point, 0);
			g = GEOSGeom_createPoint(sq);
#else
			if (lwgeom_has_z(lwgeom))
			{
				sq = ptarray_to_GEOSCoordSeq(lwp->point, 0);
				g = GEOSGeom_createPoint(sq);
			}
			else
			{
				const POINT2D* p = getPoint2d_cp(lwp->point, 0);
				g = GEOSGeom_createPointFromXY(p->x, p->y);
			}
#endif
		}
		if (!g) return NULL;
		break;

	case LINETYPE:
		lwl = (LWLINE*)lwgeom;
		/* TODO: if (autofix) */
		if (lwl->points->npoints == 1)
		{
			/* Duplicate point, to make geos-friendly */
			lwl->points = ptarray_addPoint(lwl->points,
						       getPoint_internal(lwl->points, 0),
						       FLAGS_NDIMS(lwl->points->flags),
						       lwl->points->npoints);
		}
		sq = ptarray_to_GEOSCoordSeq(lwl->points, 0);
		g = GEOSGeom_createLineString(sq);
		if (!g) return NULL;
		break;

	case POLYGONTYPE:
		lwpoly = (LWPOLY*)lwgeom;
		if (lwgeom_is_empty(lwgeom))
			g = GEOSGeom_createEmptyPolygon();
		else
		{
			shell = ptarray_to_GEOSLinearRing(lwpoly->rings[0], autofix);
			if (!shell) return NULL;
			ngeoms = lwpoly->nrings - 1;
			if (ngeoms > 0) geoms = lwalloc(sizeof(GEOSGeom) * ngeoms);

			for (i = 1; i < lwpoly->nrings; i++)
			{
				geoms[i - 1] = ptarray_to_GEOSLinearRing(lwpoly->rings[i], autofix);
				if (!geoms[i - 1])
				{
					uint32_t k;
					for (k = 0; k < i - 1; k++)
						GEOSGeom_destroy(geoms[k]);
					lwfree(geoms);
					GEOSGeom_destroy(shell);
					return NULL;
				}
			}
			g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
			if (geoms) lwfree(geoms);
		}
		if (!g) return NULL;
		break;

	case TRIANGLETYPE:
		if (lwgeom_is_empty(lwgeom))
			g = GEOSGeom_createEmptyPolygon();
		else
		{
			LWTRIANGLE *lwt = (LWTRIANGLE *)lwgeom;
			shell = ptarray_to_GEOSLinearRing(lwt->points, autofix);
			if (!shell)
				return NULL;
			g = GEOSGeom_createPolygon(shell, NULL, 0);
		}
		if (!g)
			return NULL;
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		if (lwgeom->type == MULTIPOINTTYPE)
			geostype = GEOS_MULTIPOINT;
		else if (lwgeom->type == MULTILINETYPE)
			geostype = GEOS_MULTILINESTRING;
		else if (lwgeom->type == MULTIPOLYGONTYPE)
			geostype = GEOS_MULTIPOLYGON;
		else
			geostype = GEOS_GEOMETRYCOLLECTION;

		lwc = (LWCOLLECTION*)lwgeom;

		ngeoms = lwc->ngeoms;
		if (ngeoms > 0) geoms = lwalloc(sizeof(GEOSGeom) * ngeoms);

		j = 0;
		for (i = 0; i < ngeoms; ++i)
		{
			GEOSGeometry* g;

			/* if (lwgeom_is_empty(lwc->geoms[i])) continue; */

			g = LWGEOM2GEOS(lwc->geoms[i], 0);
			if (!g)
			{
				uint32_t k;
				for (k = 0; k < j; k++)
					GEOSGeom_destroy(geoms[k]);
				lwfree(geoms);
				return NULL;
			}
			geoms[j++] = g;
		}
		g = GEOSGeom_createCollection(geostype, geoms, j);
		if (ngeoms > 0) lwfree(geoms);
		if (!g) return NULL;
		break;

	default:
		lwerror("Unknown geometry type: %d - %s", lwgeom->type, lwtype_name(lwgeom->type));
		return NULL;
	}

	GEOSSetSRID(g, lwgeom->srid);

#if LWDEBUG_LEVEL >= 4
	wkt = GEOSGeomToWKT(g);
	LWDEBUGF(4, "LWGEOM2GEOS: GEOSGeom: %s", wkt);
	free(wkt);
#endif

	return g;
}

GEOSGeometry*
make_geos_point(double x, double y)
{
	GEOSCoordSequence* seq = GEOSCoordSeq_create(1, 2);
	GEOSGeometry* geom = NULL;

	if (!seq) return NULL;

#if POSTGIS_GEOS_VERSION < 38
	GEOSCoordSeq_setX(seq, 0, x);
	GEOSCoordSeq_setY(seq, 0, y);
#else
	GEOSCoordSeq_setXY(seq, 0, x, y);
#endif

	geom = GEOSGeom_createPoint(seq);
	if (!geom) GEOSCoordSeq_destroy(seq);
	return geom;
}

GEOSGeometry*
make_geos_segment(double x1, double y1, double x2, double y2)
{
	GEOSCoordSequence* seq = GEOSCoordSeq_create(2, 2);
	GEOSGeometry* geom = NULL;

	if (!seq) return NULL;

#if POSTGIS_GEOS_VERSION < 38
	GEOSCoordSeq_setX(seq, 0, x1);
	GEOSCoordSeq_setY(seq, 0, y1);
	GEOSCoordSeq_setX(seq, 1, x2);
	GEOSCoordSeq_setY(seq, 1, y2);
#else
	GEOSCoordSeq_setXY(seq, 0, x1, y1);
	GEOSCoordSeq_setXY(seq, 1, x2, y2);
#endif

	geom = GEOSGeom_createLineString(seq);
	if (!geom) GEOSCoordSeq_destroy(seq);
	return geom;
}

const char*
lwgeom_geos_version()
{
	const char* ver = GEOSversion();
	return ver;
}

/* Return the consistent SRID of all input.
 * Intended to be called from RESULT_SRID macro */
static int32_t
get_result_srid(size_t count, const char* funcname, ...)
{
	va_list ap;
	va_start(ap, funcname);
	int32_t srid = SRID_INVALID;
	size_t i;
	for(i = 0; i < count; i++)
	{
		LWGEOM* g = va_arg(ap, LWGEOM*);
		if (!g)
		{
			lwerror("%s: Geometry is null", funcname);
			return SRID_INVALID;
		}
		if (i == 0)
		{
			srid = g->srid;
		}
		else
		{
			if (g->srid != srid)
			{
				lwerror("%s: Operation on mixed SRID geometries (%d != %d)", funcname, srid, g->srid);
				return SRID_INVALID;
			}
		}
	}
	return srid;
}

LWGEOM*
lwgeom_normalize(const LWGEOM* geom)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry* g;

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	if (GEOSNormalize(g) == -1) GEOS_FREE_AND_FAIL(g);
	GEOSSetSRID(g, srid);

	if (!(result = GEOS2LWGEOM(g, is3d))) GEOS_FREE_AND_FAIL(g);

	GEOSGeom_destroy(g);
	return result;
}

LWGEOM*
lwgeom_intersection(const LWGEOM* g1, const LWGEOM* g2)
{
	return lwgeom_intersection_prec(g1, g2, -1.0);
}

LWGEOM*
lwgeom_intersection_prec(const LWGEOM* geom1, const LWGEOM* geom2, double prec)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	GEOSGeometry* g3;

	if (srid == SRID_INVALID) return NULL;

	/* A.Intersection(Empty) == Empty */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom2); /* match empty type? */

	/* Empty.Intersection(A) == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1); /* match empty type? */

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	if ( prec >= 0) {
#if POSTGIS_GEOS_VERSION < 39
		lwerror("Fixed-precision intersection requires GEOS-3.9 or higher");
		GEOS_FREE_AND_FAIL(g1, g2);
		return NULL;
#else
		g3 = GEOSIntersectionPrec(g1, g2, prec);
#endif
	}
	else
	{
		g3 = GEOSIntersection(g1, g2);
	}

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d))) GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

LWGEOM*
lwgeom_linemerge(const LWGEOM* geom)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry* g1;
	GEOSGeometry* g3;

	if (srid == SRID_INVALID) return NULL;

	/* Empty.Linemerge() == Empty */
	if (lwgeom_is_empty(geom)) return lwgeom_clone_deep(geom); /* match empty type to linestring? */

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	g3 = GEOSLineMerge(g1);

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);

	return result;
}

LWGEOM*
lwgeom_unaryunion(const LWGEOM* geom)
{
	return lwgeom_unaryunion_prec(geom, -1.0);
}

LWGEOM*
lwgeom_unaryunion_prec(const LWGEOM* geom, double prec)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry* g1;
	GEOSGeometry* g3;

	if (srid == SRID_INVALID) return NULL;

	/* Empty.UnaryUnion() == Empty */
	if (lwgeom_is_empty(geom)) return lwgeom_clone_deep(geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	if ( prec >= 0) {
#if POSTGIS_GEOS_VERSION < 39
		lwerror("Fixed-precision union requires GEOS-3.9 or higher");
		GEOS_FREE_AND_FAIL(g1);
		return NULL;
#else
		g3 = GEOSUnaryUnionPrec(g1, prec);
#endif
	}
	else
	{
		g3 = GEOSUnaryUnion(g1);
	}

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);

	return result;
}

LWGEOM*
lwgeom_difference(const LWGEOM* geom1, const LWGEOM* geom2)
{
	return lwgeom_difference_prec(geom1, geom2, -1.0);
}

LWGEOM*
lwgeom_difference_prec(const LWGEOM* geom1, const LWGEOM* geom2, double prec)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry *g1, *g2, *g3;

	if (srid == SRID_INVALID) return NULL;

	/* A.Intersection(Empty) == Empty */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1); /* match empty type? */

	/* Empty.Intersection(A) == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1); /* match empty type? */

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	if ( prec >= 0) {
#if POSTGIS_GEOS_VERSION < 39
		lwerror("Fixed-precision difference requires GEOS-3.9 or higher");
		GEOS_FREE_AND_FAIL(g1, g2);
		return NULL;
#else
		g3 = GEOSDifferencePrec(g1, g2, prec);
#endif
	}
	else
	{
		g3 = GEOSDifference(g1, g2);
	}

	if (!g3) GEOS_FREE_AND_FAIL(g1, g2);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

LWGEOM*
lwgeom_symdifference(const LWGEOM* geom1, const LWGEOM* geom2)
{
	return lwgeom_symdifference_prec(geom1, geom2, -1.0);
}

LWGEOM*
lwgeom_symdifference_prec(const LWGEOM* geom1, const LWGEOM* geom2, double prec)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry *g1, *g2, *g3;

	if (srid == SRID_INVALID) return NULL;

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom2);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	if ( prec >= 0) {
#if POSTGIS_GEOS_VERSION < 39
		lwerror("Fixed-precision difference requires GEOS-3.9 or higher");
		GEOS_FREE_AND_FAIL(g1, g2);
		return NULL;
#else
		g3 = GEOSSymDifferencePrec(g1, g2, prec);
#endif
	}
	else
	{
		g3 = GEOSSymDifference(g1, g2);
	}

	if (!g3) GEOS_FREE_AND_FAIL(g1, g2);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

LWGEOM*
lwgeom_centroid(const LWGEOM* geom)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (srid == SRID_INVALID) return NULL;

	if (lwgeom_is_empty(geom))
	{
		LWPOINT* lwp = lwpoint_construct_empty(srid, is3d, lwgeom_has_m(geom));
		return lwpoint_as_lwgeom(lwp);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	g3 = GEOSGetCentroid(g1);

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1);

	GEOS_FREE(g1, g3);

	return result;
}

LWGEOM*
lwgeom_reduceprecision(const LWGEOM* geom, double gridSize)
{
#if POSTGIS_GEOS_VERSION < 39
	lwerror("Precision reduction requires GEOS-3.9 or higher");
	return NULL;
#else

	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (srid == SRID_INVALID) return NULL;

	if (lwgeom_is_empty(geom))
		return lwgeom_clone_deep(geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	g3 = GEOSGeom_setPrecision(g1, gridSize, 0);

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1);

	GEOS_FREE(g1, g3);

	return result;
#endif
}

LWGEOM *
lwgeom_pointonsurface(const LWGEOM *geom)
{
	LWGEOM *result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (srid == SRID_INVALID) return NULL;

	if (lwgeom_is_empty(geom))
	{
		LWPOINT *lwp = lwpoint_construct_empty(srid, is3d, lwgeom_has_m(geom));
		return lwpoint_as_lwgeom(lwp);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	g3 = GEOSPointOnSurface(g1);

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);

	return result;
}

LWGEOM*
lwgeom_union(const LWGEOM* g1, const LWGEOM* g2)
{
	return lwgeom_union_prec(g1, g2, -1.0);
}

LWGEOM*
lwgeom_union_prec(const LWGEOM* geom1, const LWGEOM* geom2, double gridSize)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry *g1, *g2, *g3;

	if (srid == SRID_INVALID) return NULL;

	/* A.Union(empty) == A */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom2);

	/* B.Union(empty) == B */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	if ( gridSize >= 0) {
#if POSTGIS_GEOS_VERSION < 39
		lwerror("Fixed-precision union requires GEOS-3.9 or higher");
		GEOS_FREE_AND_FAIL(g1, g2);
		return NULL;
#else
		g3 = GEOSUnionPrec(g1, g2, gridSize);
#endif
	}
	else
	{
		g3 = GEOSUnion(g1, g2);
	}

	if (!g3) GEOS_FREE_AND_FAIL(g1, g2);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

LWGEOM *
lwgeom_clip_by_rect(const LWGEOM *geom1, double x1, double y1, double x2, double y2)
{
	LWGEOM *result;
	GEOSGeometry *g1, *g3;
	int is3d;

	/* A.Intersection(Empty) == Empty */
	if ( lwgeom_is_empty(geom1) )
		return lwgeom_clone_deep(geom1);

	is3d = FLAGS_GET_Z(geom1->flags);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX)))
		GEOS_FAIL_DEBUG();

	if (!(g3 = GEOSClipByRect(g1, x1, y1, x2, y2)))
		GEOS_FREE_AND_FAIL_DEBUG(g1);

	GEOS_FREE(g1);
	result = GEOS2LWGEOM(g3, is3d);
	GEOS_FREE(g3);

	if (!result)
		GEOS_FAIL_DEBUG();

	result->srid = geom1->srid;

	return result;
}

/* ------------ BuildArea stuff ---------------------------------------------------------------------{ */
#if POSTGIS_GEOS_VERSION < 38
typedef struct Face_t
{
	const GEOSGeometry* geom;
	GEOSGeometry* env;
	double envarea;
	struct Face_t* parent; /* if this face is an hole of another one, or NULL */
} Face;

static Face* newFace(const GEOSGeometry* g);
static void delFace(Face* f);
static unsigned int countParens(const Face* f);
static void findFaceHoles(Face** faces, int nfaces);

static Face*
newFace(const GEOSGeometry* g)
{
	Face* f = lwalloc(sizeof(Face));
	f->geom = g;
	f->env = GEOSEnvelope(f->geom);
	GEOSArea(f->env, &f->envarea);
	f->parent = NULL;
	return f;
}

static unsigned int
countParens(const Face* f)
{
	unsigned int pcount = 0;
	while (f->parent)
	{
		++pcount;
		f = f->parent;
	}
	return pcount;
}

/* Destroy the face and release memory associated with it */
static void
delFace(Face* f)
{
	GEOSGeom_destroy(f->env);
	lwfree(f);
}

static int
compare_by_envarea(const void* g1, const void* g2)
{
	Face* f1 = *(Face**)g1;
	Face* f2 = *(Face**)g2;
	double n1 = f1->envarea;
	double n2 = f2->envarea;

	if (n1 < n2) return 1;
	if (n1 > n2) return -1;
	return 0;
}

/* Find holes of each face */
static void
findFaceHoles(Face** faces, int nfaces)
{
	int i, j, h;

	/* We sort by envelope area so that we know holes are only after their shells */
	qsort(faces, nfaces, sizeof(Face*), compare_by_envarea);
	for (i = 0; i < nfaces; ++i)
	{
		Face* f = faces[i];
		int nholes = GEOSGetNumInteriorRings(f->geom);
		LWDEBUGF(2, "Scanning face %d with env area %g and %d holes", i, f->envarea, nholes);
		for (h = 0; h < nholes; ++h)
		{
			const GEOSGeometry* hole = GEOSGetInteriorRingN(f->geom, h);
			LWDEBUGF(2,
				 "Looking for hole %d/%d of face %d among %d other faces",
				 h + 1,
				 nholes,
				 i,
				 nfaces - i - 1);
			for (j = i + 1; j < nfaces; ++j)
			{
				const GEOSGeometry* f2er;
				Face* f2 = faces[j];
				if (f2->parent) continue; /* hole already assigned */
				f2er = GEOSGetExteriorRing(f2->geom);
				/* TODO: can be optimized as the ring would have the same vertices, possibly in
				 * different order. Maybe comparing number of points could already be useful. */
				if (GEOSEquals(f2er, hole))
				{
					LWDEBUGF(2, "Hole %d/%d of face %d is face %d", h + 1, nholes, i, j);
					f2->parent = f;
					break;
				}
			}
		}
	}
}

static GEOSGeometry*
collectFacesWithEvenAncestors(Face** faces, int nfaces)
{
	GEOSGeometry** geoms = lwalloc(sizeof(GEOSGeometry*) * nfaces);
	GEOSGeometry* ret;
	unsigned int ngeoms = 0;
	int i;

	for (i = 0; i < nfaces; ++i)
	{
		Face* f = faces[i];
		if (countParens(f) % 2) continue; /* we skip odd parents geoms */
		geoms[ngeoms++] = GEOSGeom_clone(f->geom);
	}

	ret = GEOSGeom_createCollection(GEOS_MULTIPOLYGON, geoms, ngeoms);
	lwfree(geoms);
	return ret;
}

GEOSGeometry*
LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in)
{
	GEOSGeometry* tmp;
	GEOSGeometry *geos_result, *shp;
	GEOSGeometry const* vgeoms[1];
	uint32_t i, ngeoms;
	int srid = GEOSGetSRID(geom_in);
	Face** geoms;
#if POSTGIS_DEBUG_LEVEL >= 3
	LWGEOM *geos_geom;
	char *geom_ewkt;
#endif

	vgeoms[0] = geom_in;
	geos_result = GEOSPolygonize(vgeoms, 1);

	LWDEBUGF(3, "GEOSpolygonize returned @ %p", geos_result);

	/* Null return from GEOSpolygonize (an exception) */
	if (!geos_result) return 0;

		/* We should now have a collection */
#if PARANOIA_LEVEL > 0
	if (GEOSGeomTypeId(geos_result) != COLLECTIONTYPE)
	{
		GEOSGeom_destroy(geos_result);
		lwerror("%s [%d] Unexpected return from GEOSpolygonize", __FILE__, __LINE__);
		return 0;
	}
#endif

	ngeoms = GEOSGetNumGeometries(geos_result);

#if POSTGIS_DEBUG_LEVEL >= 3
	LWDEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
	geos_geom = GEOS2LWGEOM(geos_result, 0);
	geom_ewkt = lwgeom_to_ewkt(geos_geom);
	LWDEBUGF(3, "GEOSpolygonize: polygonized:%s", geom_ewkt);
	lwgeom_free(geos_geom);
	lwfree(geom_ewkt);
#endif

	/* No geometries in collection, early out */
	if (ngeoms == 0)
	{
		GEOSSetSRID(geos_result, srid);
		return geos_result;
	}

	/* Return first geometry if we only have one in collection, to avoid the unnecessary Geometry clone below. */
	if (ngeoms == 1)
	{
		tmp = (GEOSGeometry*)GEOSGetGeometryN(geos_result, 0);
		if (!tmp)
		{
			GEOSGeom_destroy(geos_result);
			return 0; /* exception */
		}
		shp = GEOSGeom_clone(tmp);
		GEOSGeom_destroy(geos_result); /* only safe after the clone above */
		GEOSSetSRID(shp, srid);
		return shp;
	}

	LWDEBUGF(2, "Polygonize returned %d geoms", ngeoms);

	/*
	 * Polygonizer returns a polygon for each face in the built topology.
	 *
	 * This means that for any face with holes we'll have other faces representing each hole. We can imagine a
	 * parent-child relationship between these faces.
	 *
	 * In order to maximize the number of visible rings in output we only use those faces which have an even number
	 * of parents.
	 *
	 * Example:
	 *
	 *   +---------------+
	 *   |     L0        |  L0 has no parents
	 *   |  +---------+  |
	 *   |  |   L1    |  |  L1 is an hole of L0
	 *   |  |  +---+  |  |
	 *   |  |  |L2 |  |  |  L2 is an hole of L1 (which is an hole of L0)
	 *   |  |  |   |  |  |
	 *   |  |  +---+  |  |
	 *   |  +---------+  |
	 *   |               |
	 *   +---------------+
	 *
	 * See http://trac.osgeo.org/postgis/ticket/1806
	 *
	 */

	/* Prepare face structures for later analysis */
	geoms = lwalloc(sizeof(Face**) * ngeoms);
	for (i = 0; i < ngeoms; ++i)
		geoms[i] = newFace(GEOSGetGeometryN(geos_result, i));

	/* Find faces representing other faces holes */
	findFaceHoles(geoms, ngeoms);

	/* Build a MultiPolygon composed only by faces with an even number of ancestors */
	tmp = collectFacesWithEvenAncestors(geoms, ngeoms);

	/* Cleanup face structures */
	for (i = 0; i < ngeoms; ++i)
		delFace(geoms[i]);
	lwfree(geoms);

	/* Faces referenced memory owned by geos_result. It is safe to destroy geos_result after deleting them. */
	GEOSGeom_destroy(geos_result);

	/* Run a single overlay operation to dissolve shared edges */
	shp = GEOSUnionCascaded(tmp);
	if (!shp)
	{
		GEOSGeom_destroy(tmp);
		return 0; /* exception */
	}

	GEOSGeom_destroy(tmp);

	GEOSSetSRID(shp, srid);

	return shp;
}
#endif

LWGEOM*
lwgeom_buildarea(const LWGEOM* geom)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (srid == SRID_INVALID) return NULL;

	/* Can't build an area from an empty! */
	if (lwgeom_is_empty(geom)) return (LWGEOM*)lwpoly_construct_empty(srid, is3d, 0);

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

#if POSTGIS_GEOS_VERSION < 38
	g3 = LWGEOM_GEOS_buildArea(g1);
#else
	g3 = GEOSBuildArea(g1);
#endif

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	/* If no geometries are in result collection, return NULL */
	if (GEOSGetNumGeometries(g3) == 0)
	{
		GEOS_FREE(g1);
		return NULL;
	}

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);

	return result;
}

/* ------------ end of BuildArea stuff ---------------------------------------------------------------------} */

int
lwgeom_is_simple(const LWGEOM* geom)
{
	GEOSGeometry* g;
	int simple;

	/* Empty is always simple */
	if (lwgeom_is_empty(geom)) return LW_TRUE;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g = LWGEOM2GEOS(geom, AUTOFIX))) return -1;

	simple = GEOSisSimple(g);
	GEOSGeom_destroy(g);

	if (simple == 2) /* exception thrown */
	{
		lwerror("lwgeom_is_simple: %s", lwgeom_geos_errmsg);
		return -1;
	}

	return simple ? LW_TRUE : LW_FALSE;
}

LWGEOM*
lwgeom_geos_noop(const LWGEOM* geom)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry* g;

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	if (!g) GEOS_FREE_AND_FAIL(g);
	GEOSSetSRID(g, srid);

	if (!(result = GEOS2LWGEOM(g, is3d)))
		GEOS_FREE_AND_FAIL(g);

	GEOS_FREE(g);

	return result;
}

LWGEOM*
lwgeom_snap(const LWGEOM* geom1, const LWGEOM* geom2, double tolerance)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry *g1, *g2, *g3;

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	g3 = GEOSSnap(g1, g2, tolerance);

	if (!g3) GEOS_FREE_AND_FAIL(g1, g2);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

LWGEOM*
lwgeom_sharedpaths(const LWGEOM* geom1, const LWGEOM* geom2)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom1, geom2);
	uint8_t is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));
	GEOSGeometry *g1, *g2, *g3;

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom1, AUTOFIX))) GEOS_FAIL();
	if (!(g2 = LWGEOM2GEOS(geom2, AUTOFIX))) GEOS_FREE_AND_FAIL(g1);

	g3 = GEOSSharedPaths(g1, g2);

	if (!g3) GEOS_FREE_AND_FAIL(g1, g2);
	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g2, g3);

	GEOS_FREE(g1, g2, g3);
	return result;
}

static LWGEOM *
lwline_offsetcurve(const LWLINE *lwline, double size, int quadsegs, int joinStyle, double mitreLimit)
{
	LWGEOM* result;
	LWGEOM* geom = lwline_as_lwgeom(lwline);
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	g3 = GEOSOffsetCurve(g1, size, quadsegs, joinStyle, mitreLimit);

	if (!g3)
	{
		GEOS_FREE(g1);
		return NULL;
	}

	GEOSSetSRID(g3, srid);

	if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);
	return result;
}

static LWGEOM *
lwcollection_offsetcurve(const LWCOLLECTION *col, double size, int quadsegs, int joinStyle, double mitreLimit)
{
	const LWGEOM *geom = lwcollection_as_lwgeom(col);
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(col->flags);
	LWCOLLECTION *result;
	LWGEOM *tmp;
	uint32_t i;
	if (srid == SRID_INVALID) return NULL;

	result = lwcollection_construct_empty(MULTILINETYPE, srid, is3d, LW_FALSE);

	for (i = 0; i < col->ngeoms; i++)
	{
		tmp = lwgeom_offsetcurve(col->geoms[i], size, quadsegs, joinStyle, mitreLimit);

		if (!tmp)
		{
			lwcollection_free(result);
			return NULL;
		}

		if (!lwgeom_is_empty(tmp))
		{
			if (lwgeom_is_collection(tmp))
				result = lwcollection_concat_in_place(result, lwgeom_as_lwcollection(tmp));
			else
				result = lwcollection_add_lwgeom(result, tmp);

			if (!result)
			{
				lwgeom_free(tmp);
				return NULL;
			}
		}
	}

	if (result->ngeoms == 1)
	{
		tmp = result->geoms[0];
		lwcollection_release(result);
		return tmp;
	}
	else
		return lwcollection_as_lwgeom(result);
}

LWGEOM*
lwgeom_offsetcurve(const LWGEOM* geom, double size, int quadsegs, int joinStyle, double mitreLimit)
{
	int32_t srid = RESULT_SRID(geom);
	LWGEOM *result = NULL;
	LWGEOM *noded = NULL;
	if (srid == SRID_INVALID) return NULL;

	if (lwgeom_dimension(geom) != 1)
	{
		lwerror("%s: input is not linear", __func__, lwtype_name(geom->type));
		return NULL;
	}

	while (!result)
	{
		switch (geom->type)
		{
		case LINETYPE:
			result = lwline_offsetcurve(lwgeom_as_lwline(geom), size, quadsegs, joinStyle, mitreLimit);
			break;
		case COLLECTIONTYPE:
		case MULTILINETYPE:
			result = lwcollection_offsetcurve(lwgeom_as_lwcollection(geom), size, quadsegs, joinStyle, mitreLimit);
			break;
		default:
			lwerror("%s: unsupported geometry type: %s", __func__, lwtype_name(geom->type));
			return NULL;
		}

		if (result)
		{
			if (noded) lwgeom_free(noded);
			return result;
		}
		else if (!noded)
		{
			noded = lwgeom_node(geom);
			if (!noded)
			{
				lwerror("lwgeom_offsetcurve: cannot node input");
				return NULL;
			}
			geom = noded;
		}
		else
		{
			lwgeom_free(noded);
			lwerror("lwgeom_offsetcurve: noded geometry cannot be offset");
			return NULL;
		}
	}

	return result;
}

LWMPOINT*
lwpoly_to_points(const LWPOLY* lwpoly, uint32_t npoints, int32_t seed)
{
	double area, bbox_area, bbox_width, bbox_height;
	GBOX bbox;
	const LWGEOM* lwgeom = (LWGEOM*)lwpoly;
	uint32_t sample_npoints, sample_sqrt, sample_width, sample_height;
	double sample_cell_size;
	uint32_t i, j, n;
	uint32_t iterations = 0;
	uint32_t npoints_generated = 0;
	uint32_t npoints_tested = 0;
	GEOSGeometry* g;
	const GEOSPreparedGeometry* gprep;
	GEOSGeometry* gpt;
	GEOSCoordSequence* gseq;
	LWMPOINT* mpt;
	int32_t srid = lwgeom_get_srid(lwgeom);
	int done = 0;
	int* cells;
	const size_t size = 2 * sizeof(int);
	char tmp[2 * sizeof(int)];
	const size_t stride = 2 * sizeof(int);

	if (lwgeom_get_type(lwgeom) != POLYGONTYPE)
	{
		lwerror("%s: only polygons supported", __func__);
		return NULL;
	}

	if (npoints == 0 || lwgeom_is_empty(lwgeom)) return NULL;

	if (!lwpoly->bbox)
		lwgeom_calculate_gbox(lwgeom, &bbox);
	else
		bbox = *(lwpoly->bbox);

	area = lwpoly_area(lwpoly);
	bbox_width = bbox.xmax - bbox.xmin;
	bbox_height = bbox.ymax - bbox.ymin;
	bbox_area = bbox_width * bbox_height;

	if (area == 0.0 || bbox_area == 0.0)
	{
		lwerror("%s: zero area input polygon, TBD", __func__);
		return NULL;
	}

	/* Gross up our test set a bit to increase odds of getting coverage in one pass */
	sample_npoints = npoints * bbox_area / area;

	/* We're going to generate points using a sample grid as described
	 * http://lin-ear-th-inking.blogspot.ca/2010/05/more-random-points-in-jts.html to try and get a more uniform
	 * "random" set of points. So we have to figure out how to stick a grid into our box */
	sample_sqrt = lround(sqrt(sample_npoints));
	if (sample_sqrt == 0)
		sample_sqrt = 1;

	/* Calculate the grids we're going to randomize within */
	if (bbox_width > bbox_height)
	{
		sample_width = sample_sqrt;
		sample_height = ceil((double)sample_npoints / (double)sample_width);
		sample_cell_size = bbox_width / sample_width;
	}
	else
	{
		sample_height = sample_sqrt;
		sample_width = ceil((double)sample_npoints / (double)sample_height);
		sample_cell_size = bbox_height / sample_height;
	}

	/* Prepare the polygon for fast true/false testing */
	initGEOS(lwnotice, lwgeom_geos_error);
	g = (GEOSGeometry*)LWGEOM2GEOS(lwgeom, 0);
	if (!g)
	{
		lwerror("%s: Geometry could not be converted to GEOS: %s", __func__, lwgeom_geos_errmsg);
		return NULL;
	}
	gprep = GEOSPrepare(g);

	/* Get an empty multi-point ready to return */
	mpt = lwmpoint_construct_empty(srid, 0, 0);

	/* Initiate random number generator.
	 * Repeatable numbers are generated with seed values >= 1.
	 * When seed is zero and has not previously been set, it is based on
	 * Unix time (seconds) and process ID. */
	lwrandom_set_seed(seed);

	/* Now we fill in an array of cells, and then shuffle that array, */
	/* so we can visit the cells in random order to avoid visual ugliness */
	/* caused by visiting them sequentially */
	cells = lwalloc(2 * sizeof(int) * sample_height * sample_width);
	for (i = 0; i < sample_width; i++)
	{
		for (j = 0; j < sample_height; j++)
		{
			cells[2 * (i * sample_height + j)] = i;
			cells[2 * (i * sample_height + j) + 1] = j;
		}
	}

	/* Fisher-Yates shuffle */
	n = sample_height * sample_width;
	if (n > 1)
	{
		for (i = n - 1; i > 0; i--)
		{
			size_t j = (size_t)(lwrandom_uniform() * (i + 1));

			memcpy(tmp, (char *)cells + j * stride, size);
			memcpy((char *)cells + j * stride, (char *)cells + i * stride, size);
			memcpy((char *)cells + i * stride, tmp, size);
		}
	}

	/* Start testing points */
	while (npoints_generated < npoints)
	{
		iterations++;
		for (i = 0; i < sample_width * sample_height; i++)
		{
			int contains = 0;
			double y = bbox.ymin + cells[2 * i] * sample_cell_size;
			double x = bbox.xmin + cells[2 * i + 1] * sample_cell_size;
			x += lwrandom_uniform() * sample_cell_size;
			y += lwrandom_uniform() * sample_cell_size;
			if (x >= bbox.xmax || y >= bbox.ymax) continue;

			gseq = GEOSCoordSeq_create(1, 2);
#if POSTGIS_GEOS_VERSION < 38
			GEOSCoordSeq_setX(gseq, 0, x);
			GEOSCoordSeq_setY(gseq, 0, y);
#else
			GEOSCoordSeq_setXY(gseq, 0, x, y);
#endif
			gpt = GEOSGeom_createPoint(gseq);

			contains = GEOSPreparedIntersects(gprep, gpt);

			GEOSGeom_destroy(gpt);

			if (contains == 2)
			{
				GEOSPreparedGeom_destroy(gprep);
				GEOSGeom_destroy(g);
				lwerror("%s: GEOS exception on PreparedContains: %s", __func__, lwgeom_geos_errmsg);
				return NULL;
			}
			if (contains)
			{
				npoints_generated++;
				mpt = lwmpoint_add_lwpoint(mpt, lwpoint_make2d(srid, x, y));
				if (npoints_generated == npoints)
				{
					done = 1;
					break;
				}
			}

			/* Short-circuit check for ctrl-c occasionally */
			npoints_tested++;
			if (npoints_tested % 10000 == 0)
				LW_ON_INTERRUPT(GEOSPreparedGeom_destroy(gprep); GEOSGeom_destroy(g); return NULL);

			if (done) break;
		}
		if (done || iterations > 100) break;
	}

	GEOSPreparedGeom_destroy(gprep);
	GEOSGeom_destroy(g);
	lwfree(cells);

	return mpt;
}

/* Allocate points to sub-geometries by area, then call lwgeom_poly_to_points and bundle up final result in a single
 * multipoint. */
LWMPOINT*
lwmpoly_to_points(const LWMPOLY* lwmpoly, uint32_t npoints, int32_t seed)
{
	const LWGEOM* lwgeom = (LWGEOM*)lwmpoly;
	double area;
	uint32_t i;
	LWMPOINT* mpt = NULL;

	if (lwgeom_get_type(lwgeom) != MULTIPOLYGONTYPE)
	{
		lwerror("%s: only multipolygons supported", __func__);
		return NULL;
	}
	if (npoints == 0 || lwgeom_is_empty(lwgeom)) return NULL;

	area = lwgeom_area(lwgeom);

	for (i = 0; i < lwmpoly->ngeoms; i++)
	{
		double sub_area = lwpoly_area(lwmpoly->geoms[i]);
		int sub_npoints = lround(npoints * sub_area / area);
		if (sub_npoints > 0)
		{
			LWMPOINT* sub_mpt = lwpoly_to_points(lwmpoly->geoms[i], sub_npoints, seed);
			if (!mpt)
				mpt = sub_mpt;
			else
			{
				uint32_t j;
				for (j = 0; j < sub_mpt->ngeoms; j++)
					mpt = lwmpoint_add_lwpoint(mpt, sub_mpt->geoms[j]);
				/* Just free the shell, leave the underlying lwpoints alone, as they are now owned by
				 * the returning multipoint */
				lwfree(sub_mpt->geoms);
				lwgeom_release((LWGEOM*)sub_mpt);
			}
		}
	}
	return mpt;
}

LWMPOINT*
lwgeom_to_points(const LWGEOM* lwgeom, uint32_t npoints, int32_t seed)
{
	switch (lwgeom_get_type(lwgeom))
	{
	case MULTIPOLYGONTYPE:
		return lwmpoly_to_points((LWMPOLY*)lwgeom, npoints, seed);
	case POLYGONTYPE:
		return lwpoly_to_points((LWPOLY*)lwgeom, npoints, seed);
	default:
		lwerror("%s: unsupported geometry type '%s'", __func__, lwtype_name(lwgeom_get_type(lwgeom)));
		return NULL;
	}
}

LWTIN*
lwtin_from_geos(const GEOSGeometry* geom, uint8_t want3d)
{
	int type = GEOSGeomTypeId(geom);
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if (SRID == 0) SRID = SRID_UNKNOWN;

	if (want3d && !GEOSHasZ(geom))
	{
		LWDEBUG(3, "Geometry has no Z, won't provide one");
		want3d = 0;
	}

	switch (type)
	{
		LWTRIANGLE** geoms;
		uint32_t i, ngeoms;
	case GEOS_GEOMETRYCOLLECTION:
		LWDEBUG(4, "lwgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if (ngeoms)
		{
			geoms = lwalloc(ngeoms * sizeof *geoms);
			if (!geoms)
			{
				lwerror("lwtin_from_geos: can't allocate geoms");
				return NULL;
			}
			for (i = 0; i < ngeoms; i++)
			{
				const GEOSGeometry *poly, *ring;
				const GEOSCoordSequence* cs;
				POINTARRAY* pa;

				poly = GEOSGetGeometryN(geom, i);
				ring = GEOSGetExteriorRing(poly);
				cs = GEOSGeom_getCoordSeq(ring);
				pa = ptarray_from_GEOSCoordSeq(cs, want3d);

				geoms[i] = lwtriangle_construct(SRID, NULL, pa);
			}
		}
		return (LWTIN*)lwcollection_construct(TINTYPE, SRID, NULL, ngeoms, (LWGEOM**)geoms);
	case GEOS_POLYGON:
	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
	case GEOS_POINT:
		lwerror("lwtin_from_geos: invalid geometry type for tin: %d", type);
		break;

	default:
		lwerror("GEOS2LWGEOM: unknown geometry type: %d", type);
		return NULL;
	}

	/* shouldn't get here */
	return NULL;
}
/*
 * output = 1 for edges, 2 for TIN, 0 for polygons
 */
LWGEOM*
lwgeom_delaunay_triangulation(const LWGEOM* geom, double tolerance, int32_t output)
{
	LWGEOM* result;
	int32_t srid = RESULT_SRID(geom);
	uint8_t is3d = FLAGS_GET_Z(geom->flags);
	GEOSGeometry *g1, *g3;

	if (output < 0 || output > 2)
	{
		lwerror("%s: invalid output type specified %d", __func__, output);
		return NULL;
	}

	if (srid == SRID_INVALID) return NULL;

	initGEOS(lwnotice, lwgeom_geos_error);

	if (!(g1 = LWGEOM2GEOS(geom, AUTOFIX))) GEOS_FAIL();

	/* if output != 1 we want polys */
	g3 = GEOSDelaunayTriangulation(g1, tolerance, output == 1);

	if (!g3) GEOS_FREE_AND_FAIL(g1);
	GEOSSetSRID(g3, srid);

	if (output == 2)
	{
		result = (LWGEOM*)lwtin_from_geos(g3, is3d);
		if (!result)
		{
			GEOS_FREE(g1, g3);
			lwerror("%s: cannot convert output geometry", __func__);
			return NULL;
		}
		lwgeom_set_srid(result, srid);
	}
	else if (!(result = GEOS2LWGEOM(g3, is3d)))
		GEOS_FREE_AND_FAIL(g1, g3);

	GEOS_FREE(g1, g3);
	return result;
}

static GEOSCoordSequence*
lwgeom_get_geos_coordseq_2d(const LWGEOM* g, uint32_t num_points)
{
	uint32_t i = 0;
	uint8_t num_dims = 2;
	LWPOINTITERATOR* it;
	GEOSCoordSequence* coords;
	POINT4D tmp;

	coords = GEOSCoordSeq_create(num_points, num_dims);
	if (!coords) return NULL;

	it = lwpointiterator_create(g);
	while (lwpointiterator_next(it, &tmp))
	{
		if (i >= num_points)
		{
			lwerror("Incorrect num_points provided to lwgeom_get_geos_coordseq_2d");
			GEOSCoordSeq_destroy(coords);
			lwpointiterator_destroy(it);
			return NULL;
		}

#if POSTGIS_GEOS_VERSION < 38
		if (!GEOSCoordSeq_setX(coords, i, tmp.x) || !GEOSCoordSeq_setY(coords, i, tmp.y))
#else
		if (!GEOSCoordSeq_setXY(coords, i, tmp.x, tmp.y))
#endif
		{
			GEOSCoordSeq_destroy(coords);
			lwpointiterator_destroy(it);
			return NULL;
		}
		i++;
	}
	lwpointiterator_destroy(it);

	return coords;
}

LWGEOM*
lwgeom_voronoi_diagram(const LWGEOM* g, const GBOX* env, double tolerance, int output_edges)
{
	uint32_t num_points = lwgeom_count_vertices(g);
	LWGEOM* lwgeom_result;
	char is_3d = LW_FALSE;
	int32_t srid = lwgeom_get_srid(g);
	GEOSCoordSequence* coords;
	GEOSGeometry* geos_geom;
	GEOSGeometry* geos_env = NULL;
	GEOSGeometry* geos_result;

	if (num_points < 2)
	{
		LWCOLLECTION* empty = lwcollection_construct_empty(COLLECTIONTYPE, lwgeom_get_srid(g), 0, 0);
		return lwcollection_as_lwgeom(empty);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	/* Instead of using the standard LWGEOM2GEOS transformer, we read the vertices of the LWGEOM directly and put
	 * them into a single GEOS CoordinateSeq that can be used to define a LineString.  This allows us to process
	 * geometry types that may not be supported by GEOS, and reduces the memory requirements in cases of many
	 * geometries with few points (such as LWMPOINT).*/
	coords = lwgeom_get_geos_coordseq_2d(g, num_points);
	if (!coords) return NULL;

	geos_geom = GEOSGeom_createLineString(coords);
	if (!geos_geom)
	{
		GEOSCoordSeq_destroy(coords);
		return NULL;
	}

	if (env) geos_env = GBOX2GEOS(env);

	geos_result = GEOSVoronoiDiagram(geos_geom, geos_env, tolerance, output_edges);

	GEOSGeom_destroy(geos_geom);
	if (env) GEOSGeom_destroy(geos_env);

	if (!geos_result)
	{
		lwerror("GEOSVoronoiDiagram: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	lwgeom_result = GEOS2LWGEOM(geos_result, is_3d);
	GEOSGeom_destroy(geos_result);

	lwgeom_set_srid(lwgeom_result, srid);

	return lwgeom_result;
}
