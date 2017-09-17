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
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


/* basic LWPOLY manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


#define CHECK_POLY_RINGS_ZM 1

/* construct a new LWPOLY.  arrays (points/points per ring) will NOT be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWPOLY*
lwpoly_construct(int srid, GBOX *bbox, uint32_t nrings, POINTARRAY **points)
{
	LWPOLY *result;
	int hasz, hasm;
#ifdef CHECK_POLY_RINGS_ZM
	char zm;
	uint32_t i;
#endif

	if ( nrings < 1 ) lwerror("lwpoly_construct: need at least 1 ring");

	hasz = FLAGS_GET_Z(points[0]->flags);
	hasm = FLAGS_GET_M(points[0]->flags);

#ifdef CHECK_POLY_RINGS_ZM
	zm = FLAGS_GET_ZM(points[0]->flags);
	for (i=1; i<nrings; i++)
	{
		if ( zm != FLAGS_GET_ZM(points[i]->flags) )
			lwerror("lwpoly_construct: mixed dimensioned rings");
	}
#endif

	result = (LWPOLY*) lwalloc(sizeof(LWPOLY));
	result->type = POLYGONTYPE;
	result->flags = gflags(hasz, hasm, 0);
	FLAGS_SET_BBOX(result->flags, bbox?1:0);
	result->srid = srid;
	result->nrings = nrings;
	result->maxrings = nrings;
	result->rings = points;
	result->bbox = bbox;

	return result;
}

LWPOLY*
lwpoly_construct_rectangle(char hasz, char hasm, POINT4D *p1, POINT4D *p2,
		POINT4D *p3, POINT4D *p4)
{
	POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, 5);
	LWPOLY *lwpoly = lwpoly_construct_empty(SRID_UNKNOWN, hasz, hasm);

	ptarray_append_point(pa, p1, LW_TRUE);
	ptarray_append_point(pa, p2, LW_TRUE);
	ptarray_append_point(pa, p3, LW_TRUE);
	ptarray_append_point(pa, p4, LW_TRUE);
	ptarray_append_point(pa, p1, LW_TRUE);

	lwpoly_add_ring(lwpoly, pa);

	return lwpoly;
}

LWPOLY *
lwpoly_construct_envelope(int srid, double x1, double y1, double x2, double y2)
{
	POINT4D p1, p2, p3, p4;
	LWPOLY *poly;

	p1.x = x1;
	p1.y = y1;
	p2.x = x1;
	p2.y = y2;
	p3.x = x2;
	p3.y = y2;
	p4.x = x2;
	p4.y = y1;

	poly = lwpoly_construct_rectangle(0, 0, &p1, &p2, &p3, &p4);
	lwgeom_set_srid(lwpoly_as_lwgeom(poly), srid);
	lwgeom_add_bbox(lwpoly_as_lwgeom(poly));

	return poly;
}

LWPOLY*
lwpoly_construct_circle(int srid, double x, double y, double radius, uint32_t segments_per_quarter, char exterior)
{
	const int segments = 4*segments_per_quarter;
	const double theta = 2*M_PI / segments;
	LWPOLY *lwpoly;
	POINTARRAY *pa;
	POINT4D pt;
	uint32_t i;

	if (segments_per_quarter < 1)
	{
		lwerror("Need at least one segment per quarter-circle.");
		return NULL;
	}

	if (radius < 0)
	{
		lwerror("Radius must be positive.");
		return NULL;
	}

	lwpoly = lwpoly_construct_empty(srid, LW_FALSE, LW_FALSE);
	pa = ptarray_construct_empty(LW_FALSE, LW_FALSE, segments + 1);

	if (exterior)
		radius *= sqrt(1 + pow(tan(theta/2), 2));

	for (i = 0; i <= segments; i++)
	{
		pt.x = x + radius*sin(i * theta);
		pt.y = y + radius*cos(i * theta);
		ptarray_append_point(pa, &pt, LW_TRUE);
	}

	lwpoly_add_ring(lwpoly, pa);
	return lwpoly;
}

LWPOLY*
lwpoly_construct_empty(int srid, char hasz, char hasm)
{
	LWPOLY *result = lwalloc(sizeof(LWPOLY));
	result->type = POLYGONTYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->nrings = 0;
	result->maxrings = 1; /* Allocate room for ring, just in case. */
	result->rings = lwalloc(result->maxrings * sizeof(POINTARRAY*));
	result->bbox = NULL;
	return result;
}

void lwpoly_free(LWPOLY  *poly)
{
	int t;

	if( ! poly ) return;

	if ( poly->bbox )
		lwfree(poly->bbox);

	for (t=0; t<poly->nrings; t++)
	{
		if ( poly->rings[t] )
			ptarray_free(poly->rings[t]);
	}

	if ( poly->rings )
		lwfree(poly->rings);

	lwfree(poly);
}

void printLWPOLY(LWPOLY *poly)
{
	int t;
	lwnotice("LWPOLY {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(poly->flags));
	lwnotice("    SRID = %i", (int)poly->srid);
	lwnotice("    nrings = %i", (int)poly->nrings);
	for (t=0; t<poly->nrings; t++)
	{
		lwnotice("    RING # %i :",t);
		printPA(poly->rings[t]);
	}
	lwnotice("}");
}

/* @brief Clone LWLINE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWPOLY *
lwpoly_clone(const LWPOLY *g)
{
	int i;
	LWPOLY *ret = lwalloc(sizeof(LWPOLY));
	memcpy(ret, g, sizeof(LWPOLY));
	ret->rings = lwalloc(sizeof(POINTARRAY *)*g->nrings);
	for ( i = 0; i < g->nrings; i++ ) {
		ret->rings[i] = ptarray_clone(g->rings[i]);
	}
	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}

/* Deep clone LWPOLY object. POINTARRAY are copied, as is ring array */
LWPOLY *
lwpoly_clone_deep(const LWPOLY *g)
{
	int i;
	LWPOLY *ret = lwalloc(sizeof(LWPOLY));
	memcpy(ret, g, sizeof(LWPOLY));
	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	ret->rings = lwalloc(sizeof(POINTARRAY *)*g->nrings);
	for ( i = 0; i < ret->nrings; i++ )
	{
		ret->rings[i] = ptarray_clone_deep(g->rings[i]);
	}
	FLAGS_SET_READONLY(ret->flags,0);
	return ret;
}

/**
* Add a ring to a polygon. Point array will be referenced, not copied.
*/
int
lwpoly_add_ring(LWPOLY *poly, POINTARRAY *pa)
{
	if( ! poly || ! pa )
		return LW_FAILURE;

	/* We have used up our storage, add some more. */
	if( poly->nrings >= poly->maxrings )
	{
		int new_maxrings = 2 * (poly->nrings + 1);
		poly->rings = lwrealloc(poly->rings, new_maxrings * sizeof(POINTARRAY*));
		poly->maxrings = new_maxrings;
	}

	/* Add the new ring entry. */
	poly->rings[poly->nrings] = pa;
	poly->nrings++;

	return LW_SUCCESS;
}

void
lwpoly_force_clockwise(LWPOLY *poly)
{
	int i;

	/* No-op empties */
	if ( lwpoly_is_empty(poly) )
		return;

	/* External ring */
	if ( ptarray_isccw(poly->rings[0]) )
		ptarray_reverse(poly->rings[0]);

	/* Internal rings */
	for (i=1; i<poly->nrings; i++)
		if ( ! ptarray_isccw(poly->rings[i]) )
			ptarray_reverse(poly->rings[i]);

}

int
lwpoly_is_clockwise(LWPOLY *poly)
{
	int i;

	if ( lwpoly_is_empty(poly) )
		return LW_TRUE;

	if ( ptarray_isccw(poly->rings[0]) )
		return LW_FALSE;

	for ( i = 1; i < poly->nrings; i++)
		if ( !ptarray_isccw(poly->rings[i]) )
			return LW_FALSE;

	return LW_TRUE;
}

void
lwpoly_release(LWPOLY *lwpoly)
{
	lwgeom_release(lwpoly_as_lwgeom(lwpoly));
}

void
lwpoly_reverse(LWPOLY *poly)
{
	int i;
	if ( lwpoly_is_empty(poly) ) return;
	for (i=0; i<poly->nrings; i++)
		ptarray_reverse(poly->rings[i]);
}

LWPOLY *
lwpoly_segmentize2d(LWPOLY *poly, double dist)
{
	POINTARRAY **newrings;
	uint32_t i;

	newrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
	for (i=0; i<poly->nrings; i++)
	{
		newrings[i] = ptarray_segmentize2d(poly->rings[i], dist);
		if ( ! newrings[i] ) {
			while (i--) ptarray_free(newrings[i]);
			lwfree(newrings);
			return NULL;
		}
	}
	return lwpoly_construct(poly->srid, NULL,
	                        poly->nrings, newrings);
}

/*
 * check coordinate equality
 * ring and coordinate order is considered
 */
char
lwpoly_same(const LWPOLY *p1, const LWPOLY *p2)
{
	uint32_t i;

	if ( p1->nrings != p2->nrings ) return 0;
	for (i=0; i<p1->nrings; i++)
	{
		if ( ! ptarray_same(p1->rings[i], p2->rings[i]) )
			return 0;
	}
	return 1;
}

/*
 * Construct a polygon from a LWLINE being
 * the shell and an array of LWLINE (possibly NULL) being holes.
 * Pointarrays from intput geoms are cloned.
 * SRID must be the same for each input line.
 * Input lines must have at least 4 points, and be closed.
 */
LWPOLY *
lwpoly_from_lwlines(const LWLINE *shell,
                    uint32_t nholes, const LWLINE **holes)
{
	uint32_t nrings;
	POINTARRAY **rings = lwalloc((nholes+1)*sizeof(POINTARRAY *));
	int srid = shell->srid;
	LWPOLY *ret;

	if ( shell->points->npoints < 4 )
		lwerror("lwpoly_from_lwlines: shell must have at least 4 points");
	if ( ! ptarray_is_closed_2d(shell->points) )
		lwerror("lwpoly_from_lwlines: shell must be closed");
	rings[0] = ptarray_clone_deep(shell->points);

	for (nrings=1; nrings<=nholes; nrings++)
	{
		const LWLINE *hole = holes[nrings-1];

		if ( hole->srid != srid )
			lwerror("lwpoly_from_lwlines: mixed SRIDs in input lines");

		if ( hole->points->npoints < 4 )
			lwerror("lwpoly_from_lwlines: holes must have at least 4 points");
		if ( ! ptarray_is_closed_2d(hole->points) )
			lwerror("lwpoly_from_lwlines: holes must be closed");

		rings[nrings] = ptarray_clone_deep(hole->points);
	}

	ret = lwpoly_construct(srid, NULL, nrings, rings);
	return ret;
}

LWGEOM*
lwpoly_remove_repeated_points(const LWPOLY *poly, double tolerance)
{
	uint32_t i;
	POINTARRAY **newrings;

	newrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
	for (i=0; i<poly->nrings; i++)
	{
		newrings[i] = ptarray_remove_repeated_points_minpoints(poly->rings[i], tolerance, 4);
	}

	return (LWGEOM*)lwpoly_construct(poly->srid,
	                                 poly->bbox ? gbox_copy(poly->bbox) : NULL,
	                                 poly->nrings, newrings);

}


LWPOLY*
lwpoly_force_dims(const LWPOLY *poly, int hasz, int hasm)
{
	LWPOLY *polyout;

	/* Return 2D empty */
	if( lwpoly_is_empty(poly) )
	{
		polyout = lwpoly_construct_empty(poly->srid, hasz, hasm);
	}
	else
	{
		POINTARRAY **rings = NULL;
		int i;
		rings = lwalloc(sizeof(POINTARRAY*) * poly->nrings);
		for( i = 0; i < poly->nrings; i++ )
		{
			rings[i] = ptarray_force_dims(poly->rings[i], hasz, hasm);
		}
		polyout = lwpoly_construct(poly->srid, NULL, poly->nrings, rings);
	}
	polyout->type = poly->type;
	return polyout;
}

int lwpoly_is_empty(const LWPOLY *poly)
{
	if ( (poly->nrings < 1) || (!poly->rings) || (!poly->rings[0]) || (poly->rings[0]->npoints < 1) )
		return LW_TRUE;
	return LW_FALSE;
}

int lwpoly_count_vertices(LWPOLY *poly)
{
	int i = 0;
	int v = 0; /* vertices */
	assert(poly);
	for ( i = 0; i < poly->nrings; i ++ )
	{
		v += poly->rings[i]->npoints;
	}
	return v;
}

LWPOLY* lwpoly_simplify(const LWPOLY *ipoly, double dist, int preserve_collapsed)
{
	int i;
	LWPOLY *opoly = lwpoly_construct_empty(ipoly->srid, FLAGS_GET_Z(ipoly->flags), FLAGS_GET_M(ipoly->flags));

	LWDEBUGF(2, "%s: simplifying polygon with %d rings", __func__, ipoly->nrings);

	if ( lwpoly_is_empty(ipoly) )
	{
		lwpoly_free(opoly);
		return NULL;
	}

	for ( i = 0; i < ipoly->nrings; i++ )
	{
		POINTARRAY *opts;
		int minvertices = 0;

		/* We'll still let holes collapse, but if we're preserving */
		/* and this is a shell, we ensure it is kept */
		if ( preserve_collapsed && i == 0 )
			minvertices = 4;

		opts = ptarray_simplify(ipoly->rings[i], dist, minvertices);

		LWDEBUGF(3, "ring%d simplified from %d to %d points", i, ipoly->rings[i]->npoints, opts->npoints);

		/* Less points than are needed to form a closed ring, we can't use this */
		if ( opts->npoints < 4 )
		{
			LWDEBUGF(3, "ring%d skipped (% pts)", i, opts->npoints);
			ptarray_free(opts);
			if ( i ) continue;
			else break; /* Don't scan holes if shell is collapsed */
		}

		/* Add ring to simplified polygon */
		if( lwpoly_add_ring(opoly, opts) == LW_FAILURE )
		{
			lwpoly_free(opoly);
			return NULL;
		}
	}

	LWDEBUGF(3, "simplified polygon with %d rings", ipoly->nrings);
	opoly->type = ipoly->type;

	if( lwpoly_is_empty(opoly) )
	{
		lwpoly_free(opoly);
		return NULL;
	}

	return opoly;
}

/**
* Find the area of the outer ring - sum (area of inner rings).
*/
double
lwpoly_area(const LWPOLY *poly)
{
	double poly_area = 0.0;
	int i;

	if ( ! poly )
		lwerror("lwpoly_area called with null polygon pointer!");

	for ( i=0; i < poly->nrings; i++ )
	{
		POINTARRAY *ring = poly->rings[i];
		double ringarea = 0.0;

		/* Empty or messed-up ring. */
		if ( ring->npoints < 3 )
			continue;

		ringarea = fabs(ptarray_signed_area(ring));
		if ( i == 0 ) /* Outer ring, positive area! */
			poly_area += ringarea;
		else /* Inner ring, negative area! */
			poly_area -= ringarea;
	}

	return poly_area;
}


/**
 * Compute the sum of polygon rings length.
 * Could use a more numerically stable calculator...
 */
double
lwpoly_perimeter(const LWPOLY *poly)
{
	double result=0.0;
	int i;

	LWDEBUGF(2, "in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += ptarray_length(poly->rings[i]);

	return result;
}

/**
 * Compute the sum of polygon rings length (forcing 2d computation).
 * Could use a more numerically stable calculator...
 */
double
lwpoly_perimeter_2d(const LWPOLY *poly)
{
	double result=0.0;
	int i;

	LWDEBUGF(2, "in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += ptarray_length_2d(poly->rings[i]);

	return result;
}

int
lwpoly_is_closed(const LWPOLY *poly)
{
	int i = 0;

	if ( poly->nrings == 0 )
		return LW_TRUE;

	for ( i = 0; i < poly->nrings; i++ )
	{
		if (FLAGS_GET_Z(poly->flags))
		{
			if ( ! ptarray_is_closed_3d(poly->rings[i]) )
				return LW_FALSE;
		}
		else
		{
			if ( ! ptarray_is_closed_2d(poly->rings[i]) )
				return LW_FALSE;
		}
	}

	return LW_TRUE;
}

int
lwpoly_startpoint(const LWPOLY* poly, POINT4D* pt)
{
	if ( poly->nrings < 1 )
		return LW_FAILURE;
	return ptarray_startpoint(poly->rings[0], pt);
}

int
lwpoly_contains_point(const LWPOLY *poly, const POINT2D *pt)
{
	int i;

	if ( lwpoly_is_empty(poly) )
		return LW_FALSE;

	if ( ptarray_contains_point(poly->rings[0], pt) == LW_OUTSIDE )
		return LW_FALSE;

	for ( i = 1; i < poly->nrings; i++ )
	{
		if ( ptarray_contains_point(poly->rings[i], pt) == LW_INSIDE )
			return LW_FALSE;
	}
	return LW_TRUE;
}



LWPOLY* lwpoly_grid(const LWPOLY *poly, const gridspec *grid)
{
	LWPOLY *opoly;
	int ri;

#if 0
	/*
	 * TODO: control this assertion
	 * it is assumed that, since the grid size will be a pixel,
	 * a visible ring should show at least a white pixel inside,
	 * thus, for a square, that would be grid_xsize*grid_ysize
	 */
	double minvisiblearea = grid->xsize * grid->ysize;
#endif

	LWDEBUGF(3, "lwpoly_grid: applying grid to polygon with %d rings", poly->nrings);

	opoly = lwpoly_construct_empty(poly->srid, lwgeom_has_z((LWGEOM*)poly), lwgeom_has_m((LWGEOM*)poly));

	for (ri=0; ri<poly->nrings; ri++)
	{
		POINTARRAY *ring = poly->rings[ri];
		POINTARRAY *newring;

		newring = ptarray_grid(ring, grid);

		/* Skip ring if not composed by at least 4 pts (3 segments) */
		if ( newring->npoints < 4 )
		{
			ptarray_free(newring);

			LWDEBUGF(3, "grid_polygon3d: ring%d skipped ( <4 pts )", ri);

			if ( ri ) continue;
			else break; /* this is the external ring, no need to work on holes */
		}

		if ( ! lwpoly_add_ring(opoly, newring) )
		{
			lwerror("lwpoly_grid, memory error");
			return NULL;
		}
	}

	LWDEBUGF(3, "lwpoly_grid: simplified polygon with %d rings", opoly->nrings);

	if ( ! opoly->nrings )
	{
		lwpoly_free(opoly);
		return NULL;
	}

	return opoly;
}
