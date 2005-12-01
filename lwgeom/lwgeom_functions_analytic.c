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

/***********************************************************************
 * Simple Douglas-Peucker line simplification. 
 * No checks are done to avoid introduction of self-intersections.
 * No topology relations are considered.
 *
 * --strk@keybit.net;
 ***********************************************************************/

#define VERBOSE 0

#if VERBOSE > 0
#define REPORT_POINTS_REDUCTION
#define REPORT_RINGS_REDUCTION
#define REPORT_RINGS_ADJUSTMENTS
#endif

/* Prototypes */
void DP_findsplit2d(POINTARRAY *pts, int p1, int p2, int *split, double *dist);
POINTARRAY *DP_simplify2d(POINTARRAY *inpts, double epsilon);
LWLINE *simplify2d_lwline(const LWLINE *iline, double dist);
LWPOLY *simplify2d_lwpoly(const LWPOLY *ipoly, double dist);
LWCOLLECTION *simplify2d_collection(const LWCOLLECTION *igeom, double dist);
LWGEOM *simplify2d_lwgeom(const LWGEOM *igeom, double dist);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS);


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

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit called");
#endif

   *dist = -1;
   *split = p1;

   if (p1 + 1 < p2)
   {

      getPoint2d_p(pts, p1, &pa);
      getPoint2d_p(pts, p2, &pb);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f) to P%d(%f,%f)",
   p1, pa.x, pa.y, p2, pb.x, pb.y);
#endif

      for (k=p1+1; k<p2; k++)
      {
         getPoint2d_p(pts, k, &pk);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f)", k, pk.x, pk.y);
#endif

         /* distance computation */
         tmp = distance2d_pt_seg(&pk, &pa, &pb);

         if (tmp > *dist) 
         {
            *dist = tmp;	/* record the maximum */
            *split = k;
#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d is farthest (%g)", k, *dist);
#endif
         }
      }

   } /* length---should be redone if can == 0 */

   else
   {
#if VERBOSE > 3
elog(NOTICE, "DP_findsplit: segment too short, no split/no dist");
#endif
   }

}


POINTARRAY *
DP_simplify2d(POINTARRAY *inpts, double epsilon)
{
	int stack[inpts->npoints];	/* recursion stack */
	int sp=-1;			/* recursion stack pointer */
	int p1, split; 
	double dist;
	POINTARRAY *outpts;
	int ptsize = pointArray_ptsize(inpts);

	p1 = 0;
	stack[++sp] = inpts->npoints-1;

#if VERBOSE > 4
	elog(NOTICE, "DP_simplify called input has %d pts and %d dims (ptsize: %d)", inpts->npoints, inpts->ndims, ptsize);
#endif

	// allocate space for output POINTARRAY
	outpts = palloc(sizeof(POINTARRAY));
	outpts->dims = inpts->dims;
	outpts->npoints=1;
	outpts->serialized_pointlist = (char *)palloc(ptsize*inpts->npoints);
	memcpy(getPoint_internal(outpts, 0), getPoint_internal(inpts, 0),
		ptsize);

#if VERBOSE > 3
	elog(NOTICE, "DP_simplify: added P0 to simplified point array (size 1)");
#endif


	do
	{

		DP_findsplit2d(inpts, p1, stack[sp], &split, &dist);
#if VERBOSE > 3
		elog(NOTICE, "DP_simplify: farthest point from P%d-P%d is P%d (dist. %g)", p1, stack[sp], split, dist);
#endif

		if (dist > epsilon) {
			stack[++sp] = split;
		} else {
			outpts->npoints++;
			memcpy(getPoint_internal(outpts, outpts->npoints-1),
				getPoint_internal(inpts, stack[sp]),
				ptsize);
#if VERBOSE > 3
			elog(NOTICE, "DP_simplify: added P%d to simplified point array (size: %d)", stack[sp], outpts->npoints);
#endif
			p1 = stack[sp--];
		}
#if VERBOSE > 5
		elog(NOTICE, "stack pointer = %d", sp);
#endif
	}
	while (! (sp<0) );

	/*
	 * If we have reduced the number of points realloc
	 * outpoints array to free up some memory.
	 * Might be turned on and off with a SAVE_MEMORY define ...
	 */
	if ( outpts->npoints < inpts->npoints )
	{
		outpts->serialized_pointlist = (char *)repalloc(
			outpts->serialized_pointlist,
			ptsize*outpts->npoints);
		if ( outpts->serialized_pointlist == NULL ) {
			elog(ERROR, "Out of virtual memory");
		}
	}

	return outpts;
}

LWLINE *
simplify2d_lwline(const LWLINE *iline, double dist)
{
	POINTARRAY *ipts;
	POINTARRAY *opts;
	LWLINE *oline;

#if VERBOSE
   elog(NOTICE, "simplify2d_lwline called");
#endif

	ipts = iline->points;
	opts = DP_simplify2d(ipts, dist);
	oline = lwline_construct(iline->SRID, NULL, opts);

	return oline;
}

// TODO
LWPOLY *
simplify2d_lwpoly(const LWPOLY *ipoly, double dist)
{
	POINTARRAY *ipts;
	POINTARRAY **orings = NULL;
	LWPOLY *opoly;
	int norings=0, ri;

#ifdef REPORT_RINGS_REDUCTION
	elog(NOTICE, "simplify_polygon3d: simplifying polygon with %d rings", ipoly->nrings);
#endif

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
#ifdef REPORT_RINGS_ADJUSTMENTS
			elog(NOTICE, "simplify_polygon3d: ring%d skipped ( <4 pts )", ri);
#endif

			if ( ri ) continue;
			else break;
		}


#ifdef REPORT_POINTS_REDUCTION
		elog(NOTICE, "simplify_polygon3d: ring%d simplified from %d to %d points", ri, ipts->npoints, opts->npoints);
#endif


		/*
		 * Add ring to simplified ring array
		 * (TODO: dinamic allocation of pts_per_ring)
		 */
		orings[norings] = opts;
		norings++;

	}

#ifdef REPORT_RINGS_REDUCTION
elog(NOTICE, "simplify_polygon3d: simplified polygon with %d rings", norings);
#endif

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
	switch(TYPE_GETTYPE(igeom->type))
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
	if ( in->bbox ) lwgeom_addBBOX(out);

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
	char *srl;
	int nsegs, i;
	double length, slength, tlength;

	if( distance < 0 || distance > 1 ) {
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if( lwgeom_getType(geom->type) != LINETYPE ) {
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

		opa = pointArray_construct((char *)&pt,
			TYPE_HASZ(line->type),
			TYPE_HASM(line->type),
			1);
		point = lwpoint_construct(line->SRID, 0, opa);
		srl = lwpoint_serialize(point);
		pfree_point(point);
		PG_RETURN_POINTER(PG_LWGEOM_construct(srl, line->SRID, 0));
	}

	/* Interpolate a point on the line */
	nsegs = ipa->npoints - 1;
	length = lwgeom_pointarray_length2d(ipa);
	tlength = 0;
	for( i = 0; i < nsegs; i++ ) {
		POINT2D p1, p2;

		getPoint2d_p(ipa, i, &p1);
		getPoint2d_p(ipa, i+1, &p2);

		/* Find the relative length of this segment */
		slength = distance2d_pt_pt(&p1, &p2)/length;

		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if( distance < tlength + slength ) {
			double dseg = (distance - tlength) / slength;
			pt.x = (p1.x) + ((p2.x - p1.x) * dseg);
			pt.y = (p1.y) + ((p2.y - p1.y) * dseg);
			pt.z = 0;
			pt.m = 0;
			opa = pointArray_construct((char *)&pt,
				TYPE_HASZ(line->type),
				TYPE_HASM(line->type),
				1);
			point = lwpoint_construct(line->SRID, 0, opa);
			srl = lwpoint_serialize(point);
			pfree_point(point);
			PG_RETURN_POINTER(PG_LWGEOM_construct(srl, line->SRID, 0));
		}
		tlength += slength;
	}

	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	getPoint4d_p(ipa, ipa->npoints-1, &pt);
	opa = pointArray_construct((char *)&pt,
		TYPE_HASZ(line->type),
		TYPE_HASM(line->type),
		1);
	point = lwpoint_construct(line->SRID, 0, opa);
	srl = lwpoint_serialize(point);
	pfree_point(point);
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

typedef struct gridspec_t {
   double ipx;
   double ipy;
   double ipz;
   double ipm;
   double xsize;
   double ysize;
   double zsize;
   double msize;
} gridspec;

/* Forward declarations */
LWGEOM *lwgeom_grid(LWGEOM *lwgeom, gridspec *grid);
LWCOLLECTION *lwcollection_grid(LWCOLLECTION *coll, gridspec *grid);
LWPOINT * lwpoint_grid(LWPOINT *point, gridspec *grid);
LWPOLY * lwpoly_grid(LWPOLY *poly, gridspec *grid);
LWLINE *lwline_grid(LWLINE *line, gridspec *grid);
POINTARRAY *ptarray_grid(POINTARRAY *pa, gridspec *grid);
Datum LWGEOM_snaptogrid(PG_FUNCTION_ARGS);

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

#if VERBOSE
	elog(NOTICE, "ptarray_grid called on %p", pa);
#endif

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

	// TODO: grid bounding box...
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

#ifdef REPORT_RINGS_REDUCTION
	elog(NOTICE, "grid_polygon3d: applying grid to polygon with %d rings",
   		poly->nrings);
#endif

	for (ri=0; ri<poly->nrings; ri++)
	{
		POINTARRAY *ring = poly->rings[ri];
		POINTARRAY *newring;

#ifdef CHECK_RING_IS_CLOSE
		POINT2D p1, p2;
		getPoint2d_p(ring, 0, &p1);
		getPoint2d_p(ring, ring->npoints-1, &p2);
		if ( ! SAMEPOINT(&p1, &p2) )
			elog(NOTICE, "Before gridding: first point != last point");
#endif

		newring = ptarray_grid(ring, grid);

		/* Skip ring if not composed by at least 4 pts (3 segments) */
		if ( newring->npoints < 4 )
		{
			pfree(newring);
#ifdef REPORT_RINGS_ADJUSTMENTS
			elog(NOTICE, "grid_polygon3d: ring%d skipped ( <4 pts )", ri);
#endif
			if ( ri ) continue;
			else break; /* this is the external ring, no need to work on holes */
		}

#ifdef CHECK_RING_IS_CLOSE
		getPoint2d_p(newring, 0, &p1);
		getPoint2d_p(newring, newring->npoints-1, &p2);
		if ( ! SAMEPOINT(&p1, &p2) )
			elog(NOTICE, "After gridding: first point != last point");
#endif



#ifdef REPORT_POINTS_REDUCTION
elog(NOTICE, "grid_polygon3d: ring%d simplified from %d to %d points", ri,
	ring->npoints, newring->npoints);
#endif


		/*
		 * Add ring to simplified ring array
		 * (TODO: dinamic allocation of pts_per_ring)
		 */
		if ( ! nrings ) {
			newrings = palloc(sizeof(POINTARRAY *));
		} else {
			newrings = repalloc(newrings, sizeof(POINTARRAY *)*(nrings+1));
		}
		if ( ! newrings ) {
			elog(ERROR, "Out of virtual memory");
			return NULL;
		}
		newrings[nrings++] = newring;
	}

#ifdef REPORT_RINGS_REDUCTION
elog(NOTICE, "grid_polygon3d: simplified polygon with %d rings", nrings);
#endif

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

	// TODO: grid bounding box ?
	opoint = lwpoint_construct(point->SRID, NULL, opa);

#if VERBOSE
	elog(NOTICE, "lwpoint_grid called");
#endif

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
	switch(TYPE_GETTYPE(lwgeom->type))
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
			elog(ERROR, "lwgeom_grid: Unknown geometry type: %d",
				TYPE_GETTYPE(lwgeom->type));
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
	BOX2DFLOAT4 *box;

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

	/* 0-sided grid == grid */
	if ( grid.xsize == 0 && grid.ysize == 0 ) PG_RETURN_POINTER(in_geom);

	in_lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in_geom));

#if VERBOSE
	elog(NOTICE, "SnapToGrid got a %s", lwgeom_typename(TYPE_GETTYPE(in_lwgeom->type)));
#endif

   	out_lwgeom = lwgeom_grid(in_lwgeom, &grid);
	if ( out_lwgeom == NULL ) PG_RETURN_NULL();

	/* COMPUTE_BBOX WHEN_SIMPLE */
	if ( in_lwgeom->bbox )
	{
		box = palloc(sizeof(BOX2DFLOAT4));
		box->xmin = rint((in_lwgeom->bbox->xmin - grid.ipx)/grid.xsize)
			* grid.xsize + grid.ipx;
		box->xmax = rint((in_lwgeom->bbox->xmax - grid.ipx)/grid.xsize)
			* grid.xsize + grid.ipx;
		box->ymin = rint((in_lwgeom->bbox->ymin - grid.ipy)/grid.ysize)
			* grid.ysize + grid.ipy;
		box->ymax = rint((in_lwgeom->bbox->ymax - grid.ipy)/grid.ysize)
			* grid.ysize + grid.ipy;
		out_lwgeom->bbox = box;
	}

#if VERBOSE
	elog(NOTICE, "SnapToGrid made a %s", lwgeom_typename(TYPE_GETTYPE(out_lwgeom->type)));
#endif

	out_geom = pglwgeom_serialize(out_lwgeom);

	PG_RETURN_POINTER(out_geom);
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
	LWLINE *oline;
	POINTARRAY *ipa, *opa;
	PG_LWGEOM *ret;

	if( from < 0 || from > 1 ) {
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if( to < 0 || to > 1 ) {
		elog(ERROR,"line_interpolate_point: 3rd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if ( from > to ) {
		elog(ERROR, "2nd arg must be smaller then 3rd arg");
		PG_RETURN_NULL();
	}

	if( lwgeom_getType(geom->type) != LINETYPE ) {
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	iline = lwline_deserialize(SERIALIZED_FORM(geom));
	ipa = iline->points;

	opa = ptarray_substring(ipa, from, to);

	oline = lwline_construct(iline->SRID, 0, opa);
	ret = pglwgeom_serialize((LWGEOM *)oline);
	PG_FREE_IF_COPY(geom, 0);
	lwgeom_release((LWGEOM *)oline);
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

	if( lwgeom_getType(geom1->type) != LINETYPE ) {
		elog(ERROR,"line_locate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}
	if( lwgeom_getType(geom2->type) != POINTTYPE ) {
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
