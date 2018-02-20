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
 * Copyright 2011-2014 Sandro Santilli <strk@kbt.io>
 * Copyright 2017-2018 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

#include <stdlib.h>
#include <time.h>

LWTIN* lwtin_from_geos(const GEOSGeometry* geom, int want3d);

#undef LWGEOM_PROFILE_BUILDAREA

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

/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY*
ptarray_from_GEOSCoordSeq(const GEOSCoordSequence* cs, char want3d)
{
	uint32_t dims = 2;
	uint32_t size, i;
	POINTARRAY* pa;
	POINT4D point;

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
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if (dims >= 3) GEOSCoordSeq_getZ(cs, i, &(point.z));
		ptarray_set_point4d(pa, i, &point);
	}

	return pa;
}

/* Return an LWGEOM from a Geometry */
LWGEOM*
GEOS2LWGEOM(const GEOSGeometry* geom, char want3d)
{
	int type = GEOSGeomTypeId(geom);
	int hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if (SRID == 0) SRID = SRID_UNKNOWN;

	if (want3d)
	{
		hasZ = GEOSHasZ(geom);
		if (!hasZ)
		{
			LWDEBUG(3, "Geometry has no Z, won't provide one");
			want3d = 0;
		}
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

GEOSCoordSeq ptarray_to_GEOSCoordSeq(const POINTARRAY*, int fix_ring);

GEOSCoordSeq
ptarray_to_GEOSCoordSeq(const POINTARRAY* pa, int fix_ring)
{
	uint32_t dims = 2;
	uint32_t i;
	int append_points = 0;
	const POINT3DZ* p3d;
	const POINT2D* p2d;
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
			p3d = getPoint3dz_cp(pa, i);
			p2d = (const POINT2D*)p3d;
			LWDEBUGF(4, "Point: %g,%g,%g", p3d->x, p3d->y, p3d->z);
		}
		else
		{
			p2d = getPoint2d_cp(pa, i);
			LWDEBUGF(4, "Point: %g,%g", p2d->x, p2d->y);
		}

		GEOSCoordSeq_setX(sq, i, p2d->x);
		GEOSCoordSeq_setY(sq, i, p2d->y);

		if (dims == 3) GEOSCoordSeq_setZ(sq, i, p3d->z);
	}

	if (append_points)
	{
		if (dims == 3)
		{
			p3d = getPoint3dz_cp(pa, 0);
			p2d = (const POINT2D*)p3d;
		}
		else
			p2d = getPoint2d_cp(pa, 0);
		for (i = pa->npoints; i < pa->npoints + append_points; i++)
		{
			GEOSCoordSeq_setX(sq, i, p2d->x);
			GEOSCoordSeq_setY(sq, i, p2d->y);

			if (dims == 3) GEOSCoordSeq_setZ(sq, i, p3d->z);
		}
	}

	return sq;
}

static inline GEOSGeometry*
ptarray_to_GEOSLinearRing(const POINTARRAY* pa, int autofix)
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
LWGEOM2GEOS(const LWGEOM* lwgeom, int autofix)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell;
	GEOSGeom* geoms = NULL;
	uint32_t ngeoms, i, j;
	int geostype;
#if LWDEBUG_LEVEL >= 4
	char* wkt;
#endif

	LWDEBUGF(4, "LWGEOM2GEOS got a %s", lwtype_name(lwgeom->type));

	if (lwgeom_has_arc(lwgeom))
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
			g = GEOSGeom_createEmptyPolygon();
		else
		{
			sq = ptarray_to_GEOSCoordSeq(lwp->point, 0);
			g = GEOSGeom_createPoint(sq);
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
			if (ngeoms > 0) geoms = malloc(sizeof(GEOSGeom) * ngeoms);

			for (i = 1; i < lwpoly->nrings; i++)
			{
				geoms[i - 1] = ptarray_to_GEOSLinearRing(lwpoly->rings[i], autofix);
				if (!geoms[i - 1])
				{
					uint32_t k;
					for (k = 0; k < i - 1; k++)
						GEOSGeom_destroy(geoms[k]);
					free(geoms);
					GEOSGeom_destroy(shell);
					return NULL;
				}
			}
			g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
			if (geoms) free(geoms);
		}
		if (!g) return NULL;
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
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
		if (ngeoms > 0) geoms = malloc(sizeof(GEOSGeom) * ngeoms);

		j = 0;
		for (i = 0; i < ngeoms; ++i)
		{
			GEOSGeometry* g;

			if (lwgeom_is_empty(lwc->geoms[i])) continue;

			g = LWGEOM2GEOS(lwc->geoms[i], 0);
			if (!g)
			{
				uint32_t k;
				for (k = 0; k < j; k++)
					GEOSGeom_destroy(geoms[k]);
				free(geoms);
				return NULL;
			}
			geoms[j++] = g;
		}
		g = GEOSGeom_createCollection(geostype, geoms, j);
		if (ngeoms > 0) free(geoms);
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

	GEOSCoordSeq_setX(seq, 0, x);
	GEOSCoordSeq_setY(seq, 0, y);

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

	GEOSCoordSeq_setX(seq, 0, x1);
	GEOSCoordSeq_setY(seq, 0, y1);
	GEOSCoordSeq_setX(seq, 1, x2);
	GEOSCoordSeq_setY(seq, 1, y2);

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

LWGEOM*
lwgeom_normalize(const LWGEOM* geom1)
{
	LWGEOM* result;
	GEOSGeometry* g1;
	int is3d;
	int srid;

	srid = (int)(geom1->srid);
	is3d = FLAGS_GET_Z(geom1->flags);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	if (GEOSNormalize(g1) == -1)
	{
		lwerror("Error in GEOSNormalize: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSSetSRID(g1, srid); /* needed ? */
	result = GEOS2LWGEOM(g1, is3d);
	GEOSGeom_destroy(g1);

	if (!result)
	{
		lwerror("Error performing normalize: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}
	return result;
}

LWGEOM*
lwgeom_intersection(const LWGEOM* geom1, const LWGEOM* geom2)
{
	LWGEOM* result;
	GEOSGeometry *g1, *g2, *g3;
	int is3d;
	int srid;

	/* A.Intersection(Empty) == Empty */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom2);

	/* Empty.Intersection(A) == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	LWDEBUG(3, "intersection() START");

	g1 = LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = LWGEOM2GEOS(geom2, 0);
	if (!g2) /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS.");
		GEOSGeom_destroy(g1);
		return NULL;
	}

	LWDEBUG(3, " constructed geometrys - calling geos");
	LWDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	LWDEBUGF(3, " g2 = %s", GEOSGeomToWKT(g2));

	g3 = GEOSIntersection(g1, g2);

	LWDEBUG(3, " intersection finished");

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("Error performing intersection: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("Error performing intersection: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	return result;
}

LWGEOM*
lwgeom_linemerge(const LWGEOM* geom1)
{
	LWGEOM* result;
	GEOSGeometry *g1, *g3;
	int is3d = FLAGS_GET_Z(geom1->flags);
	int srid = geom1->srid;

	/* Empty.Linemerge() == Empty */
	if (lwgeom_is_empty(geom1))
		return (LWGEOM*)lwcollection_construct_empty(COLLECTIONTYPE, srid, is3d, lwgeom_has_m(geom1));

	initGEOS(lwnotice, lwgeom_geos_error);

	LWDEBUG(3, "linemerge() START");

	g1 = LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUG(3, " constructed geometrys - calling geos");
	LWDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	/*LWDEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	g3 = GEOSLineMerge(g1);

	LWDEBUG(3, " linemerge finished");

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		lwerror("Error performing linemerge: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		lwerror("Error performing linemerge: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	return result;
}

LWGEOM*
lwgeom_unaryunion(const LWGEOM* geom1)
{
	LWGEOM* result;
	GEOSGeometry *g1, *g3;
	int is3d = FLAGS_GET_Z(geom1->flags);
	int srid = geom1->srid;

	/* Empty.UnaryUnion() == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g3 = GEOSUnaryUnion(g1);

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		lwerror("Error performing unaryunion: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		lwerror("Error performing unaryunion: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	return result;
}

LWGEOM*
lwgeom_difference(const LWGEOM* geom1, const LWGEOM* geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM* result;
	int is3d;
	int srid;

	/* A.Difference(Empty) == A */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1);

	/* Empty.Intersection(A) == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = LWGEOM2GEOS(geom2, 0);
	if (!g2) /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g3 = GEOSDifference(g1, g2);

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSDifference: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("Error performing difference: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	return result;
}

LWGEOM*
lwgeom_symdifference(const LWGEOM* geom1, const LWGEOM* geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM* result;
	int is3d;
	int srid;

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom2);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(geom1, 0);

	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = LWGEOM2GEOS(geom2, 0);

	if (!g2) /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSymDifference(g1, g2);

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSSymDifference: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("GEOS symdifference() threw an error (result postgis geometry formation)!");
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	return result;
}

LWGEOM*
lwgeom_centroid(const LWGEOM* geom)
{
	GEOSGeometry *g, *g_centroid;
	LWGEOM* centroid;
	int srid, is3d;

	if (lwgeom_is_empty(geom))
	{
		LWPOINT* lwp = lwpoint_construct_empty(lwgeom_get_srid(geom), lwgeom_has_z(geom), lwgeom_has_m(geom));
		return lwpoint_as_lwgeom(lwp);
	}

	srid = lwgeom_get_srid(geom);
	is3d = lwgeom_has_z(geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	g = LWGEOM2GEOS(geom, 0);

	if (!g) /* exception thrown at construction */
	{
		lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g_centroid = GEOSGetCentroid(g);
	GEOSGeom_destroy(g);

	if (!g_centroid)
	{
		lwerror("GEOSGetCentroid: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g_centroid));

	GEOSSetSRID(g_centroid, srid);

	centroid = GEOS2LWGEOM(g_centroid, is3d);
	GEOSGeom_destroy(g_centroid);

	if (!centroid)
	{
		lwerror("GEOS GEOSGetCentroid() threw an error (result postgis geometry formation)!");
		return NULL; /* never get here */
	}

	return centroid;
}

LWGEOM*
lwgeom_union(const LWGEOM* geom1, const LWGEOM* geom2)
{
	int is3d;
	int srid;
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM* result;

	LWDEBUG(2, "in geomunion");

	/* A.Union(empty) == A */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom2);

	/* B.Union(empty) == B */
	if (lwgeom_is_empty(geom2)) return lwgeom_clone_deep(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(geom1, 0);

	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = LWGEOM2GEOS(geom2, 0);

	if (!g2) /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));
	LWDEBUGF(3, "g2=%s", GEOSGeomToWKT(g2));

	g3 = GEOSUnion(g1, g2);

	LWDEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (!g3)
	{
		lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	GEOSSetSRID(g3, srid);

	result = GEOS2LWGEOM(g3, is3d);

	GEOSGeom_destroy(g3);

	if (!result)
	{
		lwerror("Error performing union: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	return result;
}

LWGEOM*
lwgeom_clip_by_rect(const LWGEOM* geom1, double x0, double y0, double x1, double y1)
{
#if POSTGIS_GEOS_VERSION < 35
	lwerror(
	    "The GEOS version this postgis binary was compiled against (%d) doesn't support 'GEOSClipByRect' function "
	    "(3.5.0+ required)",
	    POSTGIS_GEOS_VERSION);
	return NULL;
#else  /* POSTGIS_GEOS_VERSION >= 35 */
	LWGEOM* result;
	GEOSGeometry *g1, *g3;
	int is3d;

	/* A.Intersection(Empty) == Empty */
	if (lwgeom_is_empty(geom1)) return lwgeom_clone_deep(geom1);

	is3d = FLAGS_GET_Z(geom1->flags);

	initGEOS(lwnotice, lwgeom_geos_error);

	LWDEBUG(3, "clip_by_rect() START");

	g1 = LWGEOM2GEOS(geom1, 1); /* auto-fix structure */
	if (!g1)                    /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUG(3, " constructed geometrys - calling geos");
	LWDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));

	g3 = GEOSClipByRect(g1, x0, y0, x1, y1);
	GEOSGeom_destroy(g1);

	LWDEBUG(3, " clip_by_rect finished");

	if (!g3)
	{
		lwnotice("Error performing rectangular clipping: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	result = GEOS2LWGEOM(g3, is3d);
	GEOSGeom_destroy(g3);

	if (!result)
	{
		lwerror("Error performing rectangular clipping: GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}

	result->srid = geom1->srid;

	return result;
#endif /* POSTGIS_GEOS_VERSION >= 35 */
}

/* ------------ BuildArea stuff ---------------------------------------------------------------------{ */

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

	/* We sort by envelope area so that we know holes are only
	 * after their shells */
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

	vgeoms[0] = geom_in;
#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Polygonizing");
#endif
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
#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Num geometries from polygonizer: %d", ngeoms);
#endif

	LWDEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
	LWDEBUGF(3, "GEOSpolygonize: polygonized:%s", lwgeom_to_ewkt(GEOS2LWGEOM(geos_result, 0)));

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

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Preparing face structures");
#endif

	/* Prepare face structures for later analysis */
	geoms = lwalloc(sizeof(Face**) * ngeoms);
	for (i = 0; i < ngeoms; ++i)
		geoms[i] = newFace(GEOSGetGeometryN(geos_result, i));

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Finding face holes");
#endif

	/* Find faces representing other faces holes */
	findFaceHoles(geoms, ngeoms);

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Colletting even ancestor faces");
#endif

	/* Build a MultiPolygon composed only by faces with an even number of ancestors */
	tmp = collectFacesWithEvenAncestors(geoms, ngeoms);

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Cleaning up");
#endif

	/* Cleanup face structures */
	for (i = 0; i < ngeoms; ++i)
		delFace(geoms[i]);
	lwfree(geoms);

	/* Faces referenced memory owned by geos_result. It is safe to destroy geos_result after deleting them. */
	GEOSGeom_destroy(geos_result);

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Self-unioning");
#endif

	/* Run a single overlay operation to dissolve shared edges */
	shp = GEOSUnionCascaded(tmp);
	if (!shp)
	{
		GEOSGeom_destroy(tmp);
		return 0; /* exception */
	}

#ifdef LWGEOM_PROFILE_BUILDAREA
	lwnotice("Final cleanup");
#endif

	GEOSGeom_destroy(tmp);

	GEOSSetSRID(shp, srid);

	return shp;
}

LWGEOM*
lwgeom_buildarea(const LWGEOM* geom)
{
	GEOSGeometry* geos_in;
	GEOSGeometry* geos_out;
	LWGEOM* geom_out;
	int SRID = (int)(geom->srid);
	int is3d = FLAGS_GET_Z(geom->flags);

	/* Can't build an area from an empty! */
	if (lwgeom_is_empty(geom)) return (LWGEOM*)lwpoly_construct_empty(SRID, is3d, 0);

	LWDEBUG(3, "buildarea called");

	LWDEBUGF(3, "ST_BuildArea got geom @ %p", geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	geos_in = LWGEOM2GEOS(geom, 0);

	if (!geos_in) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	geos_out = LWGEOM_GEOS_buildArea(geos_in);
	GEOSGeom_destroy(geos_in);

	if (!geos_out) /* exception thrown.. */
	{
		lwerror("LWGEOM_GEOS_buildArea: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* If no geometries are in result collection, return NULL */
	if (GEOSGetNumGeometries(geos_out) == 0)
	{
		GEOSGeom_destroy(geos_out);
		return NULL;
	}

	geom_out = GEOS2LWGEOM(geos_out, is3d);
	GEOSGeom_destroy(geos_out);

#if PARANOIA_LEVEL > 0
	if (!geom_out)
	{
		lwerror("%s [%s] serialization error", __FILE__, __LINE__);
		return NULL;
	}
#endif

	return geom_out;
}

int
lwgeom_is_simple(const LWGEOM* geom)
{
	GEOSGeometry* geos_in;
	int simple;

	/* Empty is always simple */
	if (lwgeom_is_empty(geom)) return LW_TRUE;

	initGEOS(lwnotice, lwgeom_geos_error);

	geos_in = LWGEOM2GEOS(geom, 0);
	if (!geos_in) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return -1;
	}
	simple = GEOSisSimple(geos_in);
	GEOSGeom_destroy(geos_in);

	if (simple == 2) /* exception thrown */
	{
		lwerror("lwgeom_is_simple: %s", lwgeom_geos_errmsg);
		return -1;
	}

	return simple ? LW_TRUE : LW_FALSE;
}

/* ------------ end of BuildArea stuff ---------------------------------------------------------------------} */

LWGEOM*
lwgeom_geos_noop(const LWGEOM* geom_in)
{
	GEOSGeometry* geosgeom;
	LWGEOM* geom_out;

	int is3d = FLAGS_GET_Z(geom_in->flags);

	initGEOS(lwnotice, lwgeom_geos_error);
	geosgeom = LWGEOM2GEOS(geom_in, 0);
	if (!geosgeom)
	{
		lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	geom_out = GEOS2LWGEOM(geosgeom, is3d);
	GEOSGeom_destroy(geosgeom);
	if (!geom_out) lwerror("GEOS Geometry could not be converted to LWGEOM: %s", lwgeom_geos_errmsg);

	return geom_out;
}

LWGEOM*
lwgeom_snap(const LWGEOM* geom1, const LWGEOM* geom2, double tolerance)
{
	int srid, is3d;
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM* out;

	srid = geom1->srid;
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry*)LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = (GEOSGeometry*)LWGEOM2GEOS(geom2, 0);
	if (!g2) /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSnap(g1, g2, tolerance);
	if (!g3)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSSnap: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	GEOSSetSRID(g3, srid);
	out = GEOS2LWGEOM(g3, is3d);
	if (!out)
	{
		GEOSGeom_destroy(g3);
		lwerror("GEOSSnap() threw an error (result LWGEOM geometry formation)!");
		return NULL;
	}
	GEOSGeom_destroy(g3);

	return out;
}

LWGEOM*
lwgeom_sharedpaths(const LWGEOM* geom1, const LWGEOM* geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM* out;
	int is3d, srid;

	srid = geom1->srid;
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry*)LWGEOM2GEOS(geom1, 0);
	if (!g1) /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = (GEOSGeometry*)LWGEOM2GEOS(geom2, 0);
	if (!g2) /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	g3 = GEOSSharedPaths(g1, g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (!g3)
	{
		lwerror("GEOSSharedPaths: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	GEOSSetSRID(g3, srid);
	out = GEOS2LWGEOM(g3, is3d);
	GEOSGeom_destroy(g3);

	if (!out)
	{
		lwerror("GEOS2LWGEOM threw an error");
		return NULL;
	}

	return out;
}

LWGEOM*
lwgeom_offsetcurve(const LWLINE* lwline, double size, int quadsegs, int joinStyle, double mitreLimit)
{
	GEOSGeometry *g1, *g3;
	LWGEOM* lwgeom_result;
	LWGEOM* lwgeom_in = lwline_as_lwgeom(lwline);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry*)LWGEOM2GEOS(lwgeom_in, 0);
	if (!g1)
	{
		lwerror("lwgeom_offsetcurve: Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g3 = GEOSOffsetCurve(g1, size, quadsegs, joinStyle, mitreLimit);

	/* Don't need input geometry anymore */
	GEOSGeom_destroy(g1);

	if (!g3)
	{
		lwerror("GEOSOffsetCurve: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, lwgeom_get_srid(lwgeom_in));
	lwgeom_result = GEOS2LWGEOM(g3, lwgeom_has_z(lwgeom_in));
	GEOSGeom_destroy(g3);

	if (!lwgeom_result)
	{
		lwerror("lwgeom_offsetcurve: GEOS2LWGEOM returned null");
		return NULL;
	}

	return lwgeom_result;
}

LWMPOINT*
lwpoly_to_points(const LWPOLY* lwpoly, uint32_t npoints)
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
	int srid = lwgeom_get_srid(lwgeom);
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
	if (sample_sqrt == 0) sample_sqrt = 1;

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

	/* Init random number generator */
	srand(time(NULL));

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

	/* shuffle */
	{
		n = sample_height * sample_width;
		if (n > 1)
		{
			for (i = 0; i < n - 1; ++i)
			{
				size_t rnd = (size_t)rand();
				size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

				memcpy(tmp, (char*)cells + j * stride, size);
				memcpy((char*)cells + j * stride, (char*)cells + i * stride, size);
				memcpy((char*)cells + i * stride, tmp, size);
			}
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
			x += rand() * sample_cell_size / RAND_MAX;
			y += rand() * sample_cell_size / RAND_MAX;
			if (x >= bbox.xmax || y >= bbox.ymax) continue;

			gseq = GEOSCoordSeq_create(1, 2);
			GEOSCoordSeq_setX(gseq, 0, x);
			GEOSCoordSeq_setY(gseq, 0, y);
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
lwmpoly_to_points(const LWMPOLY* lwmpoly, uint32_t npoints)
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
			LWMPOINT* sub_mpt = lwpoly_to_points(lwmpoly->geoms[i], sub_npoints);
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
lwgeom_to_points(const LWGEOM* lwgeom, uint32_t npoints)
{
	switch (lwgeom_get_type(lwgeom))
	{
	case MULTIPOLYGONTYPE:
		return lwmpoly_to_points((LWMPOLY*)lwgeom, npoints);
	case POLYGONTYPE:
		return lwpoly_to_points((LWPOLY*)lwgeom, npoints);
	default:
		lwerror("%s: unsupported geometry type '%s'", __func__, lwtype_name(lwgeom_get_type(lwgeom)));
		return NULL;
	}
}

LWTIN*
lwtin_from_geos(const GEOSGeometry* geom, int want3d)
{
	int type = GEOSGeomTypeId(geom);
	int hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if (SRID == 0) SRID = SRID_UNKNOWN;

	if (want3d)
	{
		hasZ = GEOSHasZ(geom);
		if (!hasZ)
		{
			LWDEBUG(3, "Geometry has no Z, won't provide one");
			want3d = 0;
		}
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
lwgeom_delaunay_triangulation(const LWGEOM* lwgeom_in, double tolerance, int output)
{
#if POSTGIS_GEOS_VERSION < 34
	lwerror("lwgeom_delaunay_triangulation: GEOS 3.4 or higher required");
	return NULL;
#else
	GEOSGeometry *g1, *g3;
	LWGEOM* lwgeom_result;

	if (output < 0 || output > 2)
	{
		lwerror("lwgeom_delaunay_triangulation: invalid output type specified %d", output);
		return NULL;
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry*)LWGEOM2GEOS(lwgeom_in, 0);
	if (!g1)
	{
		lwerror("lwgeom_delaunay_triangulation: Geometry could not be converted to GEOS: %s",
			lwgeom_geos_errmsg);
		return NULL;
	}

	/* if output != 1 we want polys */
	g3 = GEOSDelaunayTriangulation(g1, tolerance, output == 1);

	/* Don't need input geometry anymore */
	GEOSGeom_destroy(g1);

	if (!g3)
	{
		lwerror("GEOSDelaunayTriangulation: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3)); */

	GEOSSetSRID(g3, lwgeom_get_srid(lwgeom_in));

	if (output == 2)
		lwgeom_result = (LWGEOM*)lwtin_from_geos(g3, lwgeom_has_z(lwgeom_in));
	else
		lwgeom_result = GEOS2LWGEOM(g3, lwgeom_has_z(lwgeom_in));

	GEOSGeom_destroy(g3);

	if (!lwgeom_result)
	{
		if (output != 2)
			lwerror("lwgeom_delaunay_triangulation: GEOS2LWGEOM returned null");
		else
			lwerror("lwgeom_delaunay_triangulation: lwtin_from_geos returned null");

		return NULL;
	}

	return lwgeom_result;

#endif /* POSTGIS_GEOS_VERSION < 34 */
}

#if POSTGIS_GEOS_VERSION < 35
LWGEOM*
lwgeom_voronoi_diagram(const LWGEOM* g, const GBOX* env, double tolerance, int output_edges)
{
	lwerror("lwgeom_voronoi_diagram: GEOS 3.5 or higher required");
	return NULL;
}
#else  /* POSTGIS_GEOS_VERSION >= 35 */
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

		if (!GEOSCoordSeq_setX(coords, i, tmp.x) || !GEOSCoordSeq_setY(coords, i, tmp.y))
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
	int srid = lwgeom_get_srid(g);
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
#endif /* POSTGIS_GEOS_VERSION >= 35 */
