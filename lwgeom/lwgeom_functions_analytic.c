#include "postgres.h"
#include "fmgr.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

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
LWLINE *simplify2d_lwline(LWLINE *iline, double dist);
LWPOLY *simplify2d_lwpoly(LWPOLY *ipoly, double dist);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS);


/*
 * Search farthest point from segment p1-p2
 * returns distance in an int pointer
 */
void
DP_findsplit2d(POINTARRAY *pts, int p1, int p2, int *split, double *dist)
{
   int k;
   POINT2D *pa, *pb, *pk;
   double tmp;

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit called");
#endif

   *dist = -1;
   *split = p1;

   if (p1 + 1 < p2)
   {

      pa = (POINT2D *)getPoint(pts, p1);
      pb = (POINT2D *)getPoint(pts, p2);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f) to P%d(%f,%f)",
   p1, pa->x, pa->y, p2, pb->x, pb->y);
#endif

      for (k=p1+1; k<p2; k++)
      {
         pk = (POINT2D *)getPoint(pts, k);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f)", k, pk->x, pk->y);
#endif

         /* distance computation */
         tmp = distance2d_pt_seg(pk, pa, pb);

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
	memcpy(getPoint(outpts, 0), getPoint(inpts, 0), ptsize);

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
			memcpy(getPoint(outpts, outpts->npoints-1),
				getPoint(inpts, stack[sp]),
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
simplify2d_lwline(LWLINE *iline, double dist)
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
simplify2d_lwpoly(LWPOLY *ipoly, double dist)
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

PG_FUNCTION_INFO_V1(LWGEOM_simplify2d);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_EXPLODED *exp = lwgeom_explode(SERIALIZED_FORM(geom));
	double dist = PG_GETARG_FLOAT8(1);
	int i;
	char **newlines;
	int newlinesnum=0;
	char **newpolys;
	int newpolysnum=0;
	PG_LWGEOM *result;
	char *serialized;

	// no lines, no points... return input
	if ( exp->nlines + exp->npolys == 0 )
	{
		pfree_exploded(exp);
	 	PG_RETURN_POINTER(geom);
	}

	if ( exp->nlines )
	{
#if VERBOSE
		elog(NOTICE, "%d lines in exploded geom", exp->nlines);
#endif
		newlines = palloc(sizeof(char *)*exp->nlines);
		for ( i=0; i<exp->nlines; i++ )
		{
			LWLINE *iline = lwline_deserialize(exp->lines[i]);
#if VERBOSE
			elog(NOTICE, " line %d deserialized", i);
#endif
			LWLINE *oline = simplify2d_lwline(iline, dist);
#if VERBOSE
			elog(NOTICE, " line %d simplified", i);
#endif
			if ( oline == NULL ) continue;
			newlines[newlinesnum] = lwline_serialize(oline);
			newlinesnum++;
		}
		pfree(exp->lines);
		exp->lines = newlines;
		exp->nlines = newlinesnum;
	}

	if ( exp->npolys )
	{
		newpolys = palloc(sizeof(char *)*exp->npolys);
		for ( i=0; i<exp->npolys; i++ )
		{
			LWPOLY *ipoly = lwpoly_deserialize(exp->polys[i]);
			LWPOLY *opoly = simplify2d_lwpoly(ipoly, dist);
			if ( opoly == NULL ) continue;
			newpolys[newpolysnum] = lwpoly_serialize(opoly);
			newpolysnum++;
		}
		pfree(exp->polys);
		exp->polys = newpolys;
		exp->npolys = newpolysnum;
	}

	// copy 1 (when lwexploded_serialize_buf will be implemented this
	// can be avoided)
	serialized = lwexploded_serialize(exp, lwgeom_hasBBOX(geom->type));
	pfree_exploded(exp);


	// copy 2 (see above)
	result = PG_LWGEOM_construct(serialized,
		lwgeom_getSRID(geom), lwgeom_hasBBOX(geom->type));

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
		POINT2D *p1, *p2;

		p1 = (POINT2D *)getPoint(ipa, i);
		p2 = (POINT2D *)getPoint(ipa, i+1);

		/* Find the relative length of this segment */
		slength = distance2d_pt_pt(p1, p2)/length;

		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if( distance < tlength + slength ) {
			double dseg = (distance - tlength) / slength;
			pt.x = (p1->x) + ((p2->x - p1->x) * dseg);
			pt.y = (p1->y) + ((p2->y - p1->y) * dseg);
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
