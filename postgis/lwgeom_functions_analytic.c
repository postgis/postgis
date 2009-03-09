/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "math.h"
#include "lwgeom_rtree.h"
#include "lwalgorithm.h"


/***********************************************************************
 * Simple Douglas-Peucker line simplification.
 * No checks are done to avoid introduction of self-intersections.
 * No topology relations are considered.
 *
 * --strk@keybit.net;
 ***********************************************************************/


/* Prototypes */
void DP_findsplit2d(POINTARRAY *pts, int p1, int p2, int *split, double *dist);
POINTARRAY *DP_simplify2d(POINTARRAY *inpts, double epsilon);
LWLINE *simplify2d_lwline(const LWLINE *iline, double dist);
LWPOLY *simplify2d_lwpoly(const LWPOLY *ipoly, double dist);
LWCOLLECTION *simplify2d_collection(const LWCOLLECTION *igeom, double dist);
LWGEOM *simplify2d_lwgeom(const LWGEOM *igeom, double dist);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS);
Datum ST_LineCrossingDirection(PG_FUNCTION_ARGS);
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS);

double determineSide(POINT2D *seg1, POINT2D *seg2, POINT2D *point);
int isOnSegment(POINT2D *seg1, POINT2D *seg2, POINT2D *point);
int point_in_ring(POINTARRAY *pts, POINT2D *point);
int point_in_polygon(LWPOLY *polygon, LWPOINT *point);
int point_in_multipolygon(LWMPOLY *mpolygon, LWPOINT *point);
int point_in_ring_rtree(RTREE_NODE *root, POINT2D *point);
int point_in_polygon_rtree(RTREE_NODE **root, int ringCount, LWPOINT *point);
int point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int ringCount, LWPOINT *point);


/*
 * Search farthest point from segment p1-p2
 * returns distance in an int pointer
 */
void
DP_findsplit2d(POINTARRAY *pts, int p1, int p2, int *split, double *dist)
{
	int k;
	POINT2D pa, pb, pk;
	double tmp;

	LWDEBUG(4, "DP_findsplit called");

	*dist = -1;
	*split = p1;

	if (p1 + 1 < p2)
	{

		getPoint2d_p(pts, p1, &pa);
		getPoint2d_p(pts, p2, &pb);

		LWDEBUGF(4, "DP_findsplit: P%d(%f,%f) to P%d(%f,%f)",
		         p1, pa.x, pa.y, p2, pb.x, pb.y);

		for (k=p1+1; k<p2; k++)
		{
			getPoint2d_p(pts, k, &pk);

			LWDEBUGF(4, "DP_findsplit: P%d(%f,%f)", k, pk.x, pk.y);

			/* distance computation */
			tmp = distance2d_pt_seg(&pk, &pa, &pb);

			if (tmp > *dist)
			{
				*dist = tmp;	/* record the maximum */
				*split = k;

				LWDEBUGF(4, "DP_findsplit: P%d is farthest (%g)", k, *dist);
			}
		}

	} /* length---should be redone if can == 0 */

	else
	{
		LWDEBUG(3, "DP_findsplit: segment too short, no split/no dist");
	}

}


POINTARRAY *
DP_simplify2d(POINTARRAY *inpts, double epsilon)
{
	int *stack;			/* recursion stack */
	int sp=-1;			/* recursion stack pointer */
	int p1, split;
	double dist;
	POINTARRAY *outpts;
	int ptsize = pointArray_ptsize(inpts);

	/* Allocate recursion stack */
	stack = lwalloc(sizeof(int)*inpts->npoints);

	p1 = 0;
	stack[++sp] = inpts->npoints-1;

	LWDEBUGF(2, "DP_simplify called input has %d pts and %d dims (ptsize: %d)", inpts->npoints, inpts->dims, ptsize);

	/* allocate space for output POINTARRAY */
	outpts = palloc(sizeof(POINTARRAY));
	outpts->dims = inpts->dims;
	outpts->npoints=1;
	outpts->serialized_pointlist = palloc(ptsize*inpts->npoints);
	memcpy(getPoint_internal(outpts, 0), getPoint_internal(inpts, 0),
	       ptsize);

	LWDEBUG(3, "DP_simplify: added P0 to simplified point array (size 1)");

	do
	{

		DP_findsplit2d(inpts, p1, stack[sp], &split, &dist);

		LWDEBUGF(3, "DP_simplify: farthest point from P%d-P%d is P%d (dist. %g)", p1, stack[sp], split, dist);

		if (dist > epsilon)
		{
			stack[++sp] = split;
		}
		else
		{
			outpts->npoints++;
			memcpy(getPoint_internal(outpts, outpts->npoints-1),
			       getPoint_internal(inpts, stack[sp]),
			       ptsize);

			LWDEBUGF(4, "DP_simplify: added P%d to simplified point array (size: %d)", stack[sp], outpts->npoints);

			p1 = stack[sp--];
		}

		LWDEBUGF(4, "stack pointer = %d", sp);
	}
	while (! (sp<0) );

	/*
	 * If we have reduced the number of points realloc
	 * outpoints array to free up some memory.
	 * Might be turned on and off with a SAVE_MEMORY define ...
	 */
	if ( outpts->npoints < inpts->npoints )
	{
		outpts->serialized_pointlist = repalloc(
		                                       outpts->serialized_pointlist,
		                                       ptsize*outpts->npoints);
		if ( outpts->serialized_pointlist == NULL )
		{
			elog(ERROR, "Out of virtual memory");
		}
	}

	lwfree(stack);
	return outpts;
}

LWLINE *
simplify2d_lwline(const LWLINE *iline, double dist)
{
	POINTARRAY *ipts;
	POINTARRAY *opts;
	LWLINE *oline;

	LWDEBUG(2, "simplify2d_lwline called");

	ipts = iline->points;
	opts = DP_simplify2d(ipts, dist);
	oline = lwline_construct(iline->SRID, NULL, opts);

	return oline;
}

LWPOLY *
simplify2d_lwpoly(const LWPOLY *ipoly, double dist)
{
	POINTARRAY *ipts;
	POINTARRAY **orings = NULL;
	LWPOLY *opoly;
	int norings=0, ri;

	LWDEBUGF(2, "simplify_polygon3d: simplifying polygon with %d rings", ipoly->nrings);

	orings = (POINTARRAY **)palloc(sizeof(POINTARRAY *)*ipoly->nrings);

	for (ri=0; ri<ipoly->nrings; ri++)
	{
		POINTARRAY *opts;

		ipts = ipoly->rings[ri];

		opts = DP_simplify2d(ipts, dist);


		if ( opts->npoints < 2 )
		{
			/* There as to be an error in DP_simplify */
			elog(NOTICE, "DP_simplify returned a <2 pts array");
			pfree(opts);
			continue;
		}

		if ( opts->npoints < 4 )
		{
			pfree(opts);

			LWDEBUGF(3, "simplify_polygon3d: ring%d skipped ( <4 pts )", ri);

			if ( ri ) continue;
			else break;
		}


		LWDEBUGF(3, "simplify_polygon3d: ring%d simplified from %d to %d points", ri, ipts->npoints, opts->npoints);


		/*
		 * Add ring to simplified ring array
		 * (TODO: dinamic allocation of pts_per_ring)
		 */
		orings[norings] = opts;
		norings++;

	}

	LWDEBUGF(3, "simplify_polygon3d: simplified polygon with %d rings", norings);

	if ( ! norings ) return NULL;

	opoly = lwpoly_construct(ipoly->SRID, NULL, norings, orings);

	return opoly;
}

LWCOLLECTION *
simplify2d_collection(const LWCOLLECTION *igeom, double dist)
{
	unsigned int i;
	unsigned int ngeoms=0;
	LWGEOM **geoms = lwalloc(sizeof(LWGEOM *)*igeom->ngeoms);
	LWCOLLECTION *out;

	for (i=0; i<igeom->ngeoms; i++)
	{
		LWGEOM *ngeom = simplify2d_lwgeom(igeom->geoms[i], dist);
		if ( ngeom ) geoms[ngeoms++] = ngeom;
	}

	out = lwcollection_construct(TYPE_GETTYPE(igeom->type), igeom->SRID,
	                             NULL, ngeoms, geoms);

	return out;
}

LWGEOM *
simplify2d_lwgeom(const LWGEOM *igeom, double dist)
{
	switch (TYPE_GETTYPE(igeom->type))
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return lwgeom_clone(igeom);
	case LINETYPE:
		return (LWGEOM *)simplify2d_lwline(
		               (LWLINE *)igeom, dist);
	case POLYGONTYPE:
		return (LWGEOM *)simplify2d_lwpoly(
		               (LWPOLY *)igeom, dist);
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)simplify2d_collection(
		               (LWCOLLECTION *)igeom, dist);
	default:
		lwerror("simplify2d_lwgeom: unknown geometry type: %d",
		        TYPE_GETTYPE(igeom->type));
	}
	return NULL;
}

PG_FUNCTION_INFO_V1(LWGEOM_simplify2d);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *in = lwgeom_deserialize(SERIALIZED_FORM(geom));
	LWGEOM *out;
	PG_LWGEOM *result;
	double dist = PG_GETARG_FLOAT8(1);

	out = simplify2d_lwgeom(in, dist);
	if ( ! out ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in->bbox ) lwgeom_add_bbox(out);

	result = pglwgeom_serialize(out);

	PG_RETURN_POINTER(result);
}

/***********************************************************************
 * --strk@keybit.net;
 ***********************************************************************/

/***********************************************************************
 * Interpolate a point along a line, useful for Geocoding applications
 * SELECT line_interpolate_point( 'LINESTRING( 0 0, 2 2'::geometry, .5 )
 * returns POINT( 1 1 ).
 * Works in 2d space only.
 *
 * Initially written by: jsunday@rochgrp.com
 * Ported to LWGEOM by: strk@refractions.net
 ***********************************************************************/

Datum LWGEOM_line_interpolate_point(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(LWGEOM_line_interpolate_point);
Datum LWGEOM_line_interpolate_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double distance = PG_GETARG_FLOAT8(1);
	LWLINE *line;
	LWPOINT *point;
	POINTARRAY *ipa, *opa;
	POINT4D pt;
	uchar *srl;
	int nsegs, i;
	double length, slength, tlength;

	if ( distance < 0 || distance > 1 )
	{
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if ( lwgeom_getType(geom->type) != LINETYPE )
	{
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	line = lwline_deserialize(SERIALIZED_FORM(geom));
	ipa = line->points;

	/* If distance is one of the two extremes, return the point on that
	 * end rather than doing any expensive computations
	 */
	if ( distance == 0.0 || distance == 1.0 )
	{
		if ( distance == 0.0 )
			getPoint4d_p(ipa, 0, &pt);
		else
			getPoint4d_p(ipa, ipa->npoints-1, &pt);

		opa = pointArray_construct((uchar *)&pt,
		                           TYPE_HASZ(line->type),
		                           TYPE_HASM(line->type),
		                           1);
		point = lwpoint_construct(line->SRID, 0, opa);
		srl = lwpoint_serialize(point);
		/* We shouldn't need this, the memory context is getting freed on the next line.
		lwpoint_free(point); */
		PG_RETURN_POINTER(PG_LWGEOM_construct(srl, line->SRID, 0));
	}

	/* Interpolate a point on the line */
	nsegs = ipa->npoints - 1;
	length = lwgeom_pointarray_length2d(ipa);
	tlength = 0;
	for ( i = 0; i < nsegs; i++ )
	{
		POINT4D p1, p2;
		POINT4D *p1ptr=&p1, *p2ptr=&p2; /* don't break
				                                 * strict-aliasing rules
				                                 */

		getPoint4d_p(ipa, i, &p1);
		getPoint4d_p(ipa, i+1, &p2);

		/* Find the relative length of this segment */
		slength = distance2d_pt_pt((POINT2D*)p1ptr, (POINT2D*)p2ptr)/length;

		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if ( distance < tlength + slength )
		{
			double dseg = (distance - tlength) / slength;
			interpolate_point4d(&p1, &p2, &pt, dseg);
			opa = pointArray_construct((uchar *)&pt,
			                           TYPE_HASZ(line->type),
			                           TYPE_HASM(line->type),
			                           1);
			point = lwpoint_construct(line->SRID, 0, opa);
			srl = lwpoint_serialize(point);
			/* We shouldn't need this, the memory context is getting freed on the next line
			lwpoint_free(point); */
			PG_RETURN_POINTER(PG_LWGEOM_construct(srl, line->SRID, 0));
		}
		tlength += slength;
	}

	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	getPoint4d_p(ipa, ipa->npoints-1, &pt);
	opa = pointArray_construct((uchar *)&pt,
	                           TYPE_HASZ(line->type),
	                           TYPE_HASM(line->type),
	                           1);
	point = lwpoint_construct(line->SRID, 0, opa);
	srl = lwpoint_serialize(point);
	/* We shouldn't need this, the memory context is getting freed on the next line
	lwpoint_free(point); */
	PG_RETURN_POINTER(PG_LWGEOM_construct(srl, line->SRID, 0));
}
/***********************************************************************
 * --jsunday@rochgrp.com;
 ***********************************************************************/

/***********************************************************************
 *
 *  Grid application function for postgis.
 *
 *  WHAT IS
 *  -------
 *
 *  This is a grid application function for postgis.
 *  You use it to stick all of a geometry points to
 *  a custom grid defined by its origin and cell size
 *  in geometry units.
 *
 *  Points reduction is obtained by collapsing all
 *  consecutive points falling on the same grid node and
 *  removing all consecutive segments S1,S2 having
 *  S2.startpoint = S1.endpoint and S2.endpoint = S1.startpoint.
 *
 *  ISSUES
 *  ------
 *
 *  Only 2D is contemplated in grid application.
 *
 *  Consecutive coincident segments removal does not work
 *  on first/last segments (They are not considered consecutive).
 *
 *  Grid application occurs on a geometry subobject basis, thus no
 *  points reduction occurs for multipoint geometries.
 *
 *  USAGE TIPS
 *  ----------
 *
 *  Run like this:
 *
 *     SELECT SnapToGrid(<geometry>, <originX>, <originY>, <sizeX>, <sizeY>);
 *
 *     Grid cells are not visible on a screen as long as the screen
 *     point size is equal or bigger then the grid cell size.
 *     This assumption may be used to reduce the number of points in
 *     a map for a given display scale.
 *
 *     Keeping multiple resolution versions of the same data may be used
 *     in conjunction with MINSCALE/MAXSCALE keywords of mapserv to speed
 *     up rendering.
 *
 *     Check also the use of DP simplification to reduce grid visibility.
 *     I'm still researching about the relationship between grid size and
 *     DP epsilon values - please tell me if you know more about this.
 *
 *
 * --strk@keybit.net;
 *
 ***********************************************************************/

#define CHECK_RING_IS_CLOSE
#define SAMEPOINT(a,b) ((a)->x==(b)->x&&(a)->y==(b)->y)

typedef struct gridspec_t
{
	double ipx;
	double ipy;
	double ipz;
	double ipm;
	double xsize;
	double ysize;
	double zsize;
	double msize;
}
gridspec;


/* Forward declarations */
LWGEOM *lwgeom_grid(LWGEOM *lwgeom, gridspec *grid);
LWCOLLECTION *lwcollection_grid(LWCOLLECTION *coll, gridspec *grid);
LWPOINT * lwpoint_grid(LWPOINT *point, gridspec *grid);
LWPOLY * lwpoly_grid(LWPOLY *poly, gridspec *grid);
LWLINE *lwline_grid(LWLINE *line, gridspec *grid);
POINTARRAY *ptarray_grid(POINTARRAY *pa, gridspec *grid);
Datum LWGEOM_snaptogrid(PG_FUNCTION_ARGS);
Datum LWGEOM_snaptogrid_pointoff(PG_FUNCTION_ARGS);
static int grid_isNull(const gridspec *grid);
#if POSTGIS_DEBUG_LEVEL > 0
static void grid_print(const gridspec *grid);
#endif

/* A NULL grid is a grid in which size in all dimensions is 0 */
static int
grid_isNull(const gridspec *grid)
{
	if ( grid->xsize==0 &&
	                grid->ysize==0 &&
	                grid->zsize==0 &&
	                grid->msize==0 ) return 1;
	else return 0;
}

#if POSTGIS_DEBUG_LEVEL > 0
/* Print grid using given reporter */
static void
grid_print(const gridspec *grid)
{
	lwnotice("GRID(%g %g %g %g, %g %g %g %g)",
	         grid->ipx, grid->ipy, grid->ipz, grid->ipm,
	         grid->xsize, grid->ysize, grid->zsize, grid->msize);
}
#endif

/*
 * Stick an array of points to the given gridspec.
 * Return "gridded" points in *outpts and their number in *outptsn.
 *
 * Two consecutive points falling on the same grid cell are collapsed
 * into one single point.
 *
 */
POINTARRAY *
ptarray_grid(POINTARRAY *pa, gridspec *grid)
{
	POINT4D pbuf;
	int ipn, opn; /* point numbers (input/output) */
	DYNPTARRAY *dpa;
	POINTARRAY *opa;

	LWDEBUGF(2, "ptarray_grid called on %p", pa);

	dpa=dynptarray_create(pa->npoints, pa->dims);

	for (ipn=0, opn=0; ipn<pa->npoints; ++ipn)
	{

		getPoint4d_p(pa, ipn, &pbuf);

		if ( grid->xsize )
			pbuf.x = rint((pbuf.x - grid->ipx)/grid->xsize) *
			         grid->xsize + grid->ipx;

		if ( grid->ysize )
			pbuf.y = rint((pbuf.y - grid->ipy)/grid->ysize) *
			         grid->ysize + grid->ipy;

		if ( TYPE_HASZ(pa->dims) && grid->zsize )
			pbuf.z = rint((pbuf.z - grid->ipz)/grid->zsize) *
			         grid->zsize + grid->ipz;

		if ( TYPE_HASM(pa->dims) && grid->msize )
			pbuf.m = rint((pbuf.m - grid->ipm)/grid->msize) *
			         grid->msize + grid->ipm;

		dynptarray_addPoint4d(dpa, &pbuf, 0);

	}

	opa = dpa->pa;
	lwfree(dpa);

	return opa;
}

LWLINE *
lwline_grid(LWLINE *line, gridspec *grid)
{
	LWLINE *oline;
	POINTARRAY *opa;

	opa = ptarray_grid(line->points, grid);

	/* Skip line3d with less then 2 points */
	if ( opa->npoints < 2 ) return NULL;

	/* TODO: grid bounding box... */
	oline = lwline_construct(line->SRID, NULL, opa);

	return oline;
}

LWPOLY *
lwpoly_grid(LWPOLY *poly, gridspec *grid)
{
	LWPOLY *opoly;
	int ri;
	POINTARRAY **newrings = NULL;
	int nrings = 0;
	double minvisiblearea;

	/*
	 * TODO: control this assertion
	 * it is assumed that, since the grid size will be a pixel,
	 * a visible ring should show at least a white pixel inside,
	 * thus, for a square, that would be grid_xsize*grid_ysize
	 */
	minvisiblearea = grid->xsize * grid->ysize;

	nrings = 0;

	LWDEBUGF(3, "grid_polygon3d: applying grid to polygon with %d rings",
	         poly->nrings);

	for (ri=0; ri<poly->nrings; ri++)
	{
		POINTARRAY *ring = poly->rings[ri];
		POINTARRAY *newring;

#if POSTGIS_DEBUG_LEVEL >= 4
		POINT2D p1, p2;
		getPoint2d_p(ring, 0, &p1);
		getPoint2d_p(ring, ring->npoints-1, &p2);
		if ( ! SAMEPOINT(&p1, &p2) )
			LWDEBUG(4, "Before gridding: first point != last point");
#endif

		newring = ptarray_grid(ring, grid);

		/* Skip ring if not composed by at least 4 pts (3 segments) */
		if ( newring->npoints < 4 )
		{
			pfree(newring);

			LWDEBUGF(3, "grid_polygon3d: ring%d skipped ( <4 pts )", ri);

			if ( ri ) continue;
			else break; /* this is the external ring, no need to work on holes */
		}

#if POSTGIS_DEBUG_LEVEL >= 4
		getPoint2d_p(newring, 0, &p1);
		getPoint2d_p(newring, newring->npoints-1, &p2);
		if ( ! SAMEPOINT(&p1, &p2) )
			LWDEBUG(4, "After gridding: first point != last point");
#endif

		LWDEBUGF(3, "grid_polygon3d: ring%d simplified from %d to %d points", ri,
		         ring->npoints, newring->npoints);

		/*
		 * Add ring to simplified ring array
		 * (TODO: dinamic allocation of pts_per_ring)
		 */
		if ( ! nrings )
		{
			newrings = palloc(sizeof(POINTARRAY *));
		}
		else
		{
			newrings = repalloc(newrings, sizeof(POINTARRAY *)*(nrings+1));
		}
		if ( ! newrings )
		{
			elog(ERROR, "Out of virtual memory");
			return NULL;
		}
		newrings[nrings++] = newring;
	}

	LWDEBUGF(3, "grid_polygon3d: simplified polygon with %d rings", nrings);

	if ( ! nrings ) return NULL;

	opoly = lwpoly_construct(poly->SRID, NULL, nrings, newrings);
	return opoly;
}

LWPOINT *
lwpoint_grid(LWPOINT *point, gridspec *grid)
{
	LWPOINT *opoint;
	POINTARRAY *opa;

	opa = ptarray_grid(point->point, grid);

	/* TODO: grid bounding box ? */
	opoint = lwpoint_construct(point->SRID, NULL, opa);

	LWDEBUG(2, "lwpoint_grid called");

	return opoint;
}

LWCOLLECTION *
lwcollection_grid(LWCOLLECTION *coll, gridspec *grid)
{
	unsigned int i;
	LWGEOM **geoms;
	unsigned int ngeoms=0;

	geoms = palloc(coll->ngeoms * sizeof(LWGEOM *));

	for (i=0; i<coll->ngeoms; i++)
	{
		LWGEOM *g = lwgeom_grid(coll->geoms[i], grid);
		if ( g ) geoms[ngeoms++] = g;
	}

	if ( ! ngeoms ) return lwcollection_construct_empty(coll->SRID, 0, 0);

	return lwcollection_construct(TYPE_GETTYPE(coll->type), coll->SRID,
	                              NULL, ngeoms, geoms);
}

LWGEOM *
lwgeom_grid(LWGEOM *lwgeom, gridspec *grid)
{
	switch (TYPE_GETTYPE(lwgeom->type))
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_grid((LWPOINT *)lwgeom, grid);
	case LINETYPE:
		return (LWGEOM *)lwline_grid((LWLINE *)lwgeom, grid);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_grid((LWPOLY *)lwgeom, grid);
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)lwcollection_grid((LWCOLLECTION *)lwgeom, grid);
	default:
		elog(ERROR, "lwgeom_grid: Unsupported geometry type: %s",
		     lwgeom_typename(TYPE_GETTYPE(lwgeom->type)));
		return NULL;
	}
}

PG_FUNCTION_INFO_V1(LWGEOM_snaptogrid);
Datum LWGEOM_snaptogrid(PG_FUNCTION_ARGS)
{
	Datum datum;
	PG_LWGEOM *in_geom;
	LWGEOM *in_lwgeom;
	PG_LWGEOM *out_geom = NULL;
	LWGEOM *out_lwgeom;
	gridspec grid;
	/* BOX3D box3d; */

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(0);
	in_geom = (PG_LWGEOM *)PG_DETOAST_DATUM(datum);

	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	grid.ipx = PG_GETARG_FLOAT8(1);

	if ( PG_ARGISNULL(2) ) PG_RETURN_NULL();
	grid.ipy = PG_GETARG_FLOAT8(2);

	if ( PG_ARGISNULL(3) ) PG_RETURN_NULL();
	grid.xsize = PG_GETARG_FLOAT8(3);

	if ( PG_ARGISNULL(4) ) PG_RETURN_NULL();
	grid.ysize = PG_GETARG_FLOAT8(4);

	/* Do not support gridding Z and M values for now */
	grid.ipz=grid.ipm=grid.zsize=grid.msize=0;

	/* Return input geometry if grid is null */
	if ( grid_isNull(&grid) )
	{
		PG_RETURN_POINTER(in_geom);
	}

	in_lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in_geom));

	POSTGIS_DEBUGF(3, "SnapToGrid got a %s", lwgeom_typename(TYPE_GETTYPE(in_lwgeom->type)));

	out_lwgeom = lwgeom_grid(in_lwgeom, &grid);
	if ( out_lwgeom == NULL ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in_lwgeom->bbox ) lwgeom_add_bbox(out_lwgeom);

#if 0
	/*
	 * COMPUTE_BBOX WHEN SIMPLE
	 *
	 * WARNING: this is not SIMPLE, as snapping
	 * an existing bbox to a grid does not
	 * give the same result as computing a
	 * new bounding box on the snapped coordinates.
	 *
	 * This bug has been fixed in postgis-1.1.2
	 */
	if ( in_lwgeom->bbox )
	{
		box2df_to_box3d_p(in_lwgeom->bbox, &box3d);

		box3d.xmin = rint((box3d.xmin - grid.ipx)/grid.xsize)
		             * grid.xsize + grid.ipx;
		box3d.xmax = rint((box3d.xmax - grid.ipx)/grid.xsize)
		             * grid.xsize + grid.ipx;
		box3d.ymin = rint((box3d.ymin - grid.ipy)/grid.ysize)
		             * grid.ysize + grid.ipy;
		box3d.ymax = rint((box3d.ymax - grid.ipy)/grid.ysize)
		             * grid.ysize + grid.ipy;

		out_lwgeom->bbox = box3d_to_box2df(&box3d);
	}
#endif /* 0 */

	POSTGIS_DEBUGF(3, "SnapToGrid made a %s", lwgeom_typename(TYPE_GETTYPE(out_lwgeom->type)));

	out_geom = pglwgeom_serialize(out_lwgeom);

	PG_RETURN_POINTER(out_geom);
}

PG_FUNCTION_INFO_V1(LWGEOM_snaptogrid_pointoff);
Datum LWGEOM_snaptogrid_pointoff(PG_FUNCTION_ARGS)
{
	Datum datum;
	PG_LWGEOM *in_geom, *in_point;
	LWGEOM *in_lwgeom;
	LWPOINT *in_lwpoint;
	PG_LWGEOM *out_geom = NULL;
	LWGEOM *out_lwgeom;
	gridspec grid;
	/* BOX3D box3d; */
	POINT4D offsetpoint;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(0);
	in_geom = (PG_LWGEOM *)PG_DETOAST_DATUM(datum);

	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(1);
	in_point = (PG_LWGEOM *)PG_DETOAST_DATUM(datum);
	in_lwpoint = lwpoint_deserialize(SERIALIZED_FORM(in_point));
	if ( in_lwpoint == NULL )
	{
		lwerror("Offset geometry must be a point");
	}

	if ( PG_ARGISNULL(2) ) PG_RETURN_NULL();
	grid.xsize = PG_GETARG_FLOAT8(2);

	if ( PG_ARGISNULL(3) ) PG_RETURN_NULL();
	grid.ysize = PG_GETARG_FLOAT8(3);

	if ( PG_ARGISNULL(4) ) PG_RETURN_NULL();
	grid.zsize = PG_GETARG_FLOAT8(4);

	if ( PG_ARGISNULL(5) ) PG_RETURN_NULL();
	grid.msize = PG_GETARG_FLOAT8(5);

	/* Take offsets from point geometry */
	getPoint4d_p(in_lwpoint->point, 0, &offsetpoint);
	grid.ipx = offsetpoint.x;
	grid.ipy = offsetpoint.y;
	if (TYPE_HASZ(in_lwpoint->type) ) grid.ipz = offsetpoint.z;
	else grid.ipz=0;
	if (TYPE_HASM(in_lwpoint->type) ) grid.ipm = offsetpoint.m;
	else grid.ipm=0;

#if POSTGIS_DEBUG_LEVEL >= 4
	grid_print(&grid);
#endif

	/* Return input geometry if grid is null */
	if ( grid_isNull(&grid) )
	{
		PG_RETURN_POINTER(in_geom);
	}

	in_lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in_geom));

	POSTGIS_DEBUGF(3, "SnapToGrid got a %s", lwgeom_typename(TYPE_GETTYPE(in_lwgeom->type)));

	out_lwgeom = lwgeom_grid(in_lwgeom, &grid);
	if ( out_lwgeom == NULL ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in_lwgeom->bbox ) lwgeom_add_bbox(out_lwgeom);

#if 0
	/*
	 * COMPUTE_BBOX WHEN SIMPLE
	 *
	 * WARNING: this is not SIMPLE, as snapping
	 * an existing bbox to a grid does not
	 * give the same result as computing a
	 * new bounding box on the snapped coordinates.
	 *
	 * This bug has been fixed in postgis-1.1.2
	 */
	if ( in_lwgeom->bbox )
	{
		box2df_to_box3d_p(in_lwgeom->bbox, &box3d);

		box3d.xmin = rint((box3d.xmin - grid.ipx)/grid.xsize)
		             * grid.xsize + grid.ipx;
		box3d.xmax = rint((box3d.xmax - grid.ipx)/grid.xsize)
		             * grid.xsize + grid.ipx;
		box3d.ymin = rint((box3d.ymin - grid.ipy)/grid.ysize)
		             * grid.ysize + grid.ipy;
		box3d.ymax = rint((box3d.ymax - grid.ipy)/grid.ysize)
		             * grid.ysize + grid.ipy;

		out_lwgeom->bbox = box3d_to_box2df(&box3d);
	}
#endif /* 0 */

	POSTGIS_DEBUGF(3, "SnapToGrid made a %s", lwgeom_typename(TYPE_GETTYPE(out_lwgeom->type)));

	out_geom = pglwgeom_serialize(out_lwgeom);

	PG_RETURN_POINTER(out_geom);
}


/* 
** crossingDirection(line1, line2)
** 
** Determines crossing direction of line2 relative to line1.
** Only accepts LINESTRING ass parameters!
*/
PG_FUNCTION_INFO_V1(ST_LineCrossingDirection);
Datum ST_LineCrossingDirection(PG_FUNCTION_ARGS)
{
	int type1, type2, rv;
	BOX2DFLOAT4 box1, box2;
	LWLINE *l1 = NULL;
	LWLINE *l2 = NULL;
	PG_LWGEOM *geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));
	
	/*
	** If the bounding boxes don't interact, then there can't be any 
	** crossing, return right away.
	*/
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	     getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		     ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_INT32(LINE_NO_CROSS); 
		}
	}

	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);

	if ( type1 != LINETYPE || type2 != LINETYPE ) 
	{
			elog(ERROR,"This function only accepts LINESTRING as arguments.");
			PG_RETURN_NULL();
	}

	l1 = lwline_deserialize(SERIALIZED_FORM(geom1));
	l2 = lwline_deserialize(SERIALIZED_FORM(geom2));

	rv = lwline_crossing_direction(l1, l2);
	
	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 0);	

	PG_RETURN_INT32(rv);

}

PG_FUNCTION_INFO_V1(ST_LocateBetweenElevations);
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	LWCOLLECTION *geom_out = NULL;
	LWGEOM *line_in = NULL;
	uchar type = (uchar)SERIALIZED_FORM(geom_in)[0];
	char geomtype = TYPE_GETTYPE(type);
	char hasz = TYPE_HASZ(type);
	static int ordinate = 2; /* Z */
	
	if ( ! ( geomtype == LINETYPE || geomtype == MULTILINETYPE ) )
	{
		elog(ERROR,"This function only accepts LINESTRING or MULTILINESTRING as arguments.");
		PG_RETURN_NULL();
	}

	if( ! hasz ) 
	{
		elog(ERROR,"This function only accepts LINESTRING or MULTILINESTRING with Z values as arguments.");
		PG_RETURN_NULL();
	}

	line_in = lwgeom_deserialize(SERIALIZED_FORM(geom_in));
	if ( geomtype == LINETYPE ) 
	{
		geom_out = lwline_clip_to_ordinate_range((LWLINE*)line_in, ordinate, from, to);
	}
	else if ( geomtype == MULTILINETYPE )
	{
		geom_out = lwmline_clip_to_ordinate_range((LWMLINE*)line_in, ordinate, from, to);
	}
	lwgeom_free(line_in);

	if( ! geom_out ) {
		elog(ERROR,"The lwline_clip_to_ordinate_range returned null.");
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(geom_in, 0);
	PG_RETURN_POINTER(pglwgeom_serialize((LWGEOM*)geom_out));
}

/***********************************************************************
 * --strk@keybit.net
 ***********************************************************************/

Datum LWGEOM_line_substring(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(LWGEOM_line_substring);
Datum LWGEOM_line_substring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	LWLINE *iline;
	LWGEOM *olwgeom;
	POINTARRAY *ipa, *opa;
	PG_LWGEOM *ret;

	if ( from < 0 || from > 1 )
	{
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if ( to < 0 || to > 1 )
	{
		elog(ERROR,"line_interpolate_point: 3rd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if ( from > to )
	{
		elog(ERROR, "2nd arg must be smaller then 3rd arg");
		PG_RETURN_NULL();
	}

	if ( lwgeom_getType(geom->type) != LINETYPE )
	{
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	iline = lwline_deserialize(SERIALIZED_FORM(geom));
	ipa = iline->points;

	opa = ptarray_substring(ipa, from, to);

	if ( opa->npoints == 1 ) /* Point returned */
		olwgeom = (LWGEOM *)lwpoint_construct(iline->SRID, NULL, opa);
	else
		olwgeom = (LWGEOM *)lwline_construct(iline->SRID, NULL, opa);

	ret = pglwgeom_serialize(olwgeom);
	PG_FREE_IF_COPY(geom, 0);
	lwgeom_release(olwgeom);
	PG_RETURN_POINTER(ret);
}

Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(LWGEOM_line_locate_point);
Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWLINE *lwline;
	LWPOINT *lwpoint;
	POINTARRAY *pa;
	POINT2D p;
	double ret;

	if ( lwgeom_getType(geom1->type) != LINETYPE )
	{
		elog(ERROR,"line_locate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}
	if ( lwgeom_getType(geom2->type) != POINTTYPE )
	{
		elog(ERROR,"line_locate_point: 2st arg isnt a point");
		PG_RETURN_NULL();
	}
	if ( pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	lwline = lwline_deserialize(SERIALIZED_FORM(geom1));
	lwpoint = lwpoint_deserialize(SERIALIZED_FORM(geom2));

	pa = lwline->points;
	lwpoint_getPoint2d_p(lwpoint, &p);

	ret = ptarray_locate_point(pa, &p);

	PG_RETURN_FLOAT8(ret);
}

/*******************************************************************************
 * The following is based on the "Fast Winding Number Inclusion of a Point
 * in a Polygon" algorithm by Dan Sunday.
 * http://softsurfer.com/Archive/algorithm_0103/algorithm_0103.htm#Winding%20Number
 ******************************************************************************/

/*
 * returns: >0 for a point to the left of the segment,
 *          <0 for a point to the right of the segment,
 *          0 for a point on the segment
 */
double determineSide(POINT2D *seg1, POINT2D *seg2, POINT2D *point)
{
	return ((seg2->x-seg1->x)*(point->y-seg1->y)-(point->x-seg1->x)*(seg2->y-seg1->y));
}

/*
 * This function doesn't test that the point falls on the line defined by
 * the two points.  It assumes that that has already been determined
 * by having determineSide return within the tolerance.  It simply checks
 * that if the point is on the line, it is within the endpoints.
 *
 * returns: 1 if the point is not outside the bounds of the segment
 *          0 if it is
 */
int isOnSegment(POINT2D *seg1, POINT2D *seg2, POINT2D *point)
{
	double maxX;
	double maxY;
	double minX;
	double minY;

	if (seg1->x > seg2->x)
	{
		maxX = seg1->x;
		minX = seg2->x;
	}
	else
	{
		maxX = seg2->x;
		minX = seg1->x;
	}
	if (seg1->y > seg2->y)
	{
		maxY = seg1->y;
		minY = seg2->y;
	}
	else
	{
		maxY = seg2->y;
		minY = seg1->y;
	}

	LWDEBUGF(3, "maxX minX/maxY minY: %.8f %.8f/%.8f %.8f", maxX, minX, maxY, minY);

	if (maxX < point->x || minX > point->x)
	{
		LWDEBUGF(3, "X value %.8f falls outside the range %.8f-%.8f", point->x, minX, maxX);

		return 0;
	}
	else if (maxY < point->y || minY > point->y)
	{
		LWDEBUGF(3, "Y value %.8f falls outside the range %.8f-%.8f", point->y, minY, maxY);

		return 0;
	}
	return 1;
}

/*
 * return -1 iff point is outside ring pts
 * return 1 iff point is inside ring pts
 * return 0 iff point is on ring pts
 */
int point_in_ring_rtree(RTREE_NODE *root, POINT2D *point)
{
	int wn = 0;
	int i;
	double side;
	POINT2D seg1;
	POINT2D seg2;
	LWMLINE *lines;

	LWDEBUG(2, "point_in_ring called.");

	lines = findLineSegments(root, point->y);
	if (!lines)
		return -1;

	for (i=0; i<lines->ngeoms; i++)
	{
		getPoint2d_p(lines->geoms[i]->points, 0, &seg1);
		getPoint2d_p(lines->geoms[i]->points, 1, &seg2);


		side = determineSide(&seg1, &seg2, point);

		LWDEBUGF(3, "segment: (%.8f, %.8f),(%.8f, %.8f)", seg1.x, seg1.y, seg2.x, seg2.y);
		LWDEBUGF(3, "side result: %.8f", side);
		LWDEBUGF(3, "counterclockwise wrap %d, clockwise wrap %d", FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y), FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y));

		/* zero length segments are ignored. */
		if (((seg2.x-seg1.x)*(seg2.x-seg1.x)+(seg2.y-seg1.y)*(seg2.y-seg1.y)) < 1e-12*1e-12)
		{
			LWDEBUG(3, "segment is zero length... ignoring.");

			continue;
		}

		/* a point on the boundary of a ring is not contained. */
		if (fabs(side) < 1e-12)
		{
			if (isOnSegment(&seg1, &seg2, point) == 1)
			{
				LWDEBUGF(3, "point on ring boundary between points %d, %d", i, i+1);

				return 0;
			}
		}
		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and 
		 * circling counter-clockwise, so incremement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y) && side>0)
		{
			LWDEBUG(3, "incrementing winding number.");

			++wn;
		}
		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y) && side<0)
		{
			LWDEBUG(3, "decrementing winding number.");

			--wn;
		}
	}

	LWDEBUGF(3, "winding number %d", wn);

	if (wn == 0)
		return -1;
	return 1;
}


/*
 * return -1 iff point is outside ring pts
 * return 1 iff point is inside ring pts
 * return 0 iff point is on ring pts
 */
int point_in_ring(POINTARRAY *pts, POINT2D *point)
{
	int wn = 0;
	int i;
	double side;
	POINT2D seg1;
	POINT2D seg2;

	LWDEBUG(2, "point_in_ring called.");


	for (i=0; i<pts->npoints-1; i++)
	{
		getPoint2d_p(pts, i, &seg1);
		getPoint2d_p(pts, i+1, &seg2);


		side = determineSide(&seg1, &seg2, point);

		LWDEBUGF(3, "segment: (%.8f, %.8f),(%.8f, %.8f)", seg1.x, seg1.y, seg2.x, seg2.y);
		LWDEBUGF(3, "side result: %.8f", side);
		LWDEBUGF(3, "counterclockwise wrap %d, clockwise wrap %d", FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y), FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y));

		/* zero length segments are ignored. */
		if (((seg2.x-seg1.x)*(seg2.x-seg1.x)+(seg2.y-seg1.y)*(seg2.y-seg1.y)) < 1e-12*1e-12)
		{
			LWDEBUG(3, "segment is zero length... ignoring.");

			continue;
		}

		/* a point on the boundary of a ring is not contained. */
		if (fabs(side) < 1e-12)
		{
			if (isOnSegment(&seg1, &seg2, point) == 1)
			{
				LWDEBUGF(3, "point on ring boundary between points %d, %d", i, i+1);

				return 0;
			}
		}
		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and 
		 * circling counter-clockwise, so incremement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y) && side>0)
		{
			LWDEBUG(3, "incrementing winding number.");

			++wn;
		}
		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y) && side<0)
		{
			LWDEBUG(3, "decrementing winding number.");

			--wn;
		}
	}

	LWDEBUGF(3, "winding number %d", wn);

	if (wn == 0)
		return -1;
	return 1;
}

/*
 * return 0 iff point outside polygon or on boundary
 * return 1 iff point inside polygon
 */
int point_in_polygon_rtree(RTREE_NODE **root, int ringCount, LWPOINT *point)
{
	int i;
	POINT2D pt;

	LWDEBUGF(2, "point_in_polygon called for %p %d %p.", root, ringCount, point);

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	if (point_in_ring_rtree(root[0], &pt) != 1)
	{
		LWDEBUG(3, "point_in_polygon_rtree: outside exterior ring.");

		return 0;
	}

	for (i=1; i<ringCount; i++)
	{
		if (point_in_ring_rtree(root[i], &pt) != -1)
		{
			LWDEBUGF(3, "point_in_polygon_rtree: within hole %d.", i);

			return 0;
		}
	}
	return 1;
}

/*
 * return -1 if point outside polygon
 * return 0 if point on boundary
 * return 1 if point inside polygon
 *
 * Expected **root order is all the exterior rings first, then all the holes
 *
 * TODO: this could be made slightly more efficient by ordering the rings in
 * EIIIEIIIEIEI order (exterior/interior) and including list of exterior ring
 * positions on the cache object.
 */
int point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int ringCount, LWPOINT *point)
{
	int i;
	POINT2D pt;
	int result = -1;

	LWDEBUGF(2, "point_in_multipolygon_rtree called for %p %d %d %p.", root, polyCount, ringCount, point);

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	/* is the point inside (not outside) any of the exterior rings? */
	for ( i = 0; i < polyCount; i++ )
	{
		int in_ring = point_in_ring_rtree(root[i], &pt);
		LWDEBUGF(4, "point_in_multipolygon_rtree: exterior ring (%d), point_in_ring returned %d", i, in_ring);
		if ( in_ring != -1 ) /* not outside this ring */
		{
			LWDEBUG(3, "point_in_multipolygon_rtree: inside exterior ring.");
			result = in_ring;
			break;
		}
	}

	if ( result == -1 ) /* strictly outside all rings */
		return result;

	/* ok, it's in a ring, but if it's in a hole it's still outside */
	for ( i = polyCount; i < ringCount; i++ )
	{
		int in_ring = point_in_ring_rtree(root[i], &pt);
		LWDEBUGF(4, "point_in_multipolygon_rtree: hole (%d), point_in_ring returned %d", i, in_ring);
		if ( in_ring == 1 ) /* completely inside hole */
		{
			LWDEBUGF(3, "point_in_multipolygon_rtree: within hole %d.", i);
			return -1;
		}
		if ( in_ring == 0 ) /* on the boundary of a hole */
		{
			result = 0;
		}
	}
	return result; /* -1 = outside, 0 = boundary, 1 = inside */

}

/*
 * return -1 iff point outside polygon
 * return 0 iff point on boundary
 * return 1 iff point inside polygon
 */
int point_in_polygon(LWPOLY *polygon, LWPOINT *point)
{
	int i, result, in_ring;
	POINTARRAY *ring;
	POINT2D pt;

	LWDEBUG(2, "point_in_polygon called.");

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	ring = polygon->rings[0];
	in_ring = point_in_ring(polygon->rings[0], &pt);
	if ( in_ring == -1) /* outside the exterior ring */
	{
		LWDEBUG(3, "point_in_polygon: outside exterior ring.");
		return -1;
	}
	result = in_ring;

	for (i=1; i<polygon->nrings; i++)
	{
		ring = polygon->rings[i];
		in_ring = point_in_ring(polygon->rings[i], &pt);
		if (in_ring == 1) /* inside a hole => outside the polygon */
		{
			LWDEBUGF(3, "point_in_polygon: within hole %d.", i);
			return -1;
		}
		if (in_ring == 0) /* on the edge of a hole */
		{
			LWDEBUGF(3, "point_in_polygon: on edge of hole %d.", i);
			return 0;
		}
	}
	return result; /* -1 = outside, 0 = boundary, 1 = inside */
}

/*
 * return -1 iff point outside multipolygon
 * return 0 iff point on multipolygon boundary
 * return 1 iff point inside multipolygon
 */
int point_in_multipolygon(LWMPOLY *mpolygon, LWPOINT *point)
{
	int i, j, result, in_ring;
	POINTARRAY *ring;
	POINT2D pt;

	LWDEBUG(2, "point_in_polygon called.");

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	result = -1;

	for (j = 0; j < mpolygon->ngeoms; j++ )
	{

		LWPOLY *polygon = mpolygon->geoms[j];
		ring = polygon->rings[0];
		in_ring = point_in_ring(polygon->rings[0], &pt);
		if ( in_ring == -1) /* outside the exterior ring */
		{
			LWDEBUG(3, "point_in_polygon: outside exterior ring.");
			continue;
		}
		if ( in_ring == 0 )
		{
			return 0;
		}

		result = in_ring;

		for (i=1; i<polygon->nrings; i++)
		{
			ring = polygon->rings[i];
			in_ring = point_in_ring(polygon->rings[i], &pt);
			if (in_ring == 1) /* inside a hole => outside the polygon */
			{
				LWDEBUGF(3, "point_in_polygon: within hole %d.", i);
				result = -1;
				break;
			}
			if (in_ring == 0) /* on the edge of a hole */
			{
				LWDEBUGF(3, "point_in_polygon: on edge of hole %d.", i);
				return 0;
			}
		}
		if ( result != -1)
		{
			return result;
		}
	}
	return result;
}


/*******************************************************************************
 * End of "Fast Winding Number Inclusion of a Point in a Polygon" derivative.
 ******************************************************************************/

