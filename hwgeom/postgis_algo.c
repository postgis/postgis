/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * $Log$
 * Revision 1.1  2004/09/20 07:50:06  strk
 * prepared to contain old internal representation code
 *
 * Revision 1.3  2004/04/27 13:50:59  strk
 * Fixed bug in simplify() that was using the square of the given tolerance.
 *
 * Revision 1.2  2004/01/21 19:04:03  strk
 * Added line_interpolate_point function by jsunday@rochgrp.com
 *
 * Revision 1.1  2003/10/27 10:21:15  strk
 * Initial import.
 *
 **********************************************************************/

#include "postgres.h"
#include "postgis.h"


/***********************************************************************
 * Simple Douglas-Peucker line simplification. 
 * No checks are done to avoid introduction of self-intersections.
 * No topology relations are considered.
 *
 * --strk@keybit.net;
 ***********************************************************************/

#define SAMEPOINT(a,b) ((a)->x==(b)->x&&(a)->y==(b)->y&&(a)->z==(b)->z)
#define VERBOSE 0

#if VERBOSE > 0
#undef REPORT_POINTS_REDUCTION
#undef REPORT_RINGS_REDUCTION
#define REPORT_RINGS_ADJUSTMENTS
#endif

#undef CHECK_RING_IS_CLOSE


/*
 * Search farthest point from segment p1-p2
 * returns distance in an int pointer
 */
void DP_findsplit(POINT3D *pts, int npts, int p1, int p2,
   int *split, double *dist)
{
   int k;
   POINT3D *pa, *pb, *pk;
   double tmp;

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit called");
#endif

   *dist = -1;
   *split = p1;

   if (p1 + 1 < p2)
   {

      pa = &(pts[p1]);
      pb = &(pts[p2]);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f) to P%d(%f,%f)",
   p1, pa->x, pa->y, p2, pb->x, pb->y);
#endif

      for (k=p1+1; k<p2; k++)
      {
         pk = &(pts[k]);

#if VERBOSE > 4
elog(NOTICE, "DP_findsplit: P%d(%f,%f)", k, pk->x, pk->y);
#endif

         /* distance computation */
         tmp = distance_pt_seg(pk, pa, pb);

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


void
DP_simplify(POINT3D *inpts, int inptsn, POINT3D **outpts, int *outptsn, double epsilon)
{
   int stack[inptsn];		/* recursion stack */
   int sp=-1;              /* recursion stack pointer */
   int p1, split; 
   double dist;
   POINT3D *outpoints;
   int numoutpoints=0;

   p1 = 0;
   stack[++sp] = inptsn-1;

#if VERBOSE > 4
   elog(NOTICE, "DP_simplify called");
#endif

   outpoints = (POINT3D *)palloc( sizeof(POINT3D) * inptsn);
   memcpy(outpoints, inpts, sizeof(POINT3D));
   numoutpoints++;

#if VERBOSE > 3
elog(NOTICE, "DP_simplify: added P0 to simplified point array (size 1)");
#endif


   do
   {

      DP_findsplit(inpts, inptsn, p1, stack[sp], &split, &dist);
#if VERBOSE > 3
      elog(NOTICE, "DP_simplify: farthest point from P%d-P%d is P%d (dist. %g)", p1, stack[sp], split, dist);
#endif

      if (dist > epsilon) {
         stack[++sp] = split;
      } else {
         /*
         outpoints = repalloc( outpoints, sizeof(POINT3D) * numoutpoints+1 );
         if ( outpoints == NULL ) elog(NOTICE, "Out of virtual memory");
         */
         memcpy(outpoints+numoutpoints, &(inpts[stack[sp]]),
            sizeof(POINT3D));
         numoutpoints++;
#if VERBOSE > 3
elog(NOTICE, "DP_simplify: added P%d to simplified point array (size: %d)",
   stack[sp], numoutpoints);
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
    * Might be turned on and of with a SAVE_MEMORY define ...
    */
   if ( numoutpoints < inptsn )
   {
      outpoints = (POINT3D *)repalloc(outpoints, sizeof(POINT3D)*numoutpoints);
      if ( outpoints == NULL ) {
         elog(ERROR, "Out of virtual memory");
      }
   }

   *outpts = outpoints;
   *outptsn = numoutpoints;
}

char *
simplify_line3d(LINE3D *iline, double dist)
{
   POINT3D *ipts;
   POINT3D *opts;
   int iptsn, optsn, olinesize;
   LINE3D *oline;

   ipts = iline->points;
   iptsn = iline->npoints;

   DP_simplify(ipts, iptsn, &opts, &optsn, dist);

   oline = make_line(optsn, opts, &olinesize);

   return (char *)oline;
}

char *
simplify_polygon3d(POLYGON3D *ipoly, double dist)
{
   POINT3D *ipts;
   POINT3D *opts;
   int iptsn, optsn, size;
   int nrings;
   int pts_per_ring[ipoly->nrings];
   POLYGON3D *opoly;
   int ri;
   POINT3D *allpts = NULL;
   int allptsn = 0;
   int pt_off = 0; /* point offset for each ring */

   nrings = 0;

#ifdef REPORT_RINGS_REDUCTION
elog(NOTICE, "simplify_polygon3d: simplifying polygon with %d rings",
   ipoly->nrings);
#endif

   /* Points start here */
   ipts = (POINT3D *) ((char *)&(ipoly->npoints[ipoly->nrings] ));

   pt_off=0;
   for (ri=0; ri<ipoly->nrings; pt_off += ipoly->npoints[ri++])
   {
      iptsn = ipoly->npoints[ri];

#ifdef CHECK_RING_IS_CLOSE
      if ( ! SAMEPOINT(ipts+pt_off, ipts+pt_off+iptsn-1) )
         elog(NOTICE, "First point != Last point");
#endif

      DP_simplify(ipts+pt_off, iptsn, &opts, &optsn, dist);
      if ( optsn < 2 )
      {
         /* There as to be an error in DP_simplify */
         elog(NOTICE, "DP_simplify returned a <2 pts array");
         pfree(opts);
         continue;
      }

#ifdef CHECK_RING_IS_CLOSE
      if ( ! SAMEPOINT(opts, opts+optsn-1) )
         elog(NOTICE, "First point != Last point");
#endif


      if ( optsn < 4 )
      {
         pfree(opts);
#ifdef REPORT_RINGS_ADJUSTMENTS
         elog(NOTICE, "simplify_polygon3d: ring%d skipped ( <4 pts )", ri);
#endif
         if ( ri ) continue;
         else break;
      }


#ifdef REPORT_POINTS_REDUCTION
elog(NOTICE, "simplify_polygon3d: ring%d simplified from %d to %d points", ri, iptsn, optsn);
#endif


      /*
       * Add ring to simplified ring array
       * (TODO: dinamic allocation of pts_per_ring)
       */
      pts_per_ring[nrings++] = optsn;
      if ( ! allptsn ) {
         allptsn = optsn;
         allpts = palloc(sizeof(POINT3D)*allptsn);
         memcpy(allpts, opts, sizeof(POINT3D)*optsn);
      } else {
         allptsn += optsn;
         allpts = repalloc(allpts, sizeof(POINT3D)*allptsn);
         memcpy(allpts+(allptsn-optsn), opts, sizeof(POINT3D)*optsn);
      }
      pfree(opts);

      if ( ! allpts ) {
         elog(NOTICE, "Error allocating memory for all ring points");
         return NULL;
      }

   }

#ifdef REPORT_RINGS_REDUCTION
elog(NOTICE, "simplify_polygon3d: simplified polygon with %d rings", nrings);
#endif

   if ( nrings )
   {
      opoly = make_polygon(nrings, pts_per_ring, allpts, allptsn, &size);
      pfree(allpts);
      return (char *)opoly;
   }
   else
   {
      return NULL;
   }

}

char *
simplify_point3d(POINT3D *ipoint, double dist)
{
   return (char *)ipoint;
}

PG_FUNCTION_INFO_V1(simplify);
Datum simplify(PG_FUNCTION_ARGS)
{
   Datum datum;
   BOX3D *bbox;
   GEOMETRY *orig_geom;
   GEOMETRY *simp_geom = NULL;
   char *orig_obj;       /* pointer to each object in orig_geom */
   char *simp_obj;       /* pointer to each simplified object */
   int simp_obj_size;    /* size of simplified object */
   int32 *offsets;
   int i;
   double dist;

   if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();
   datum = PG_GETARG_DATUM(0);
   orig_geom = (GEOMETRY *)PG_DETOAST_DATUM(datum);

   if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
   dist = PG_GETARG_FLOAT8(1);

   /*
    * Three or more points on a straight line will still collapse!
    */
   // if ( dist == 0 ) PG_RETURN_POINTER(orig_geom);

   offsets = (int32 *) ( ((char *) &(orig_geom->objType[0] )) +
      sizeof(int32) * orig_geom->nobjs );


   /*
    * Simplify each subobject indipendently.
    * No topology relations kept.
    */
   for(i=0;i<orig_geom->nobjs; i++)
   {
      orig_obj = (char *) orig_geom+offsets[i];

      if ( orig_geom->objType[i] == LINETYPE )
      {
         simp_obj = simplify_line3d((LINE3D *)orig_obj, dist);
      }
      else if ( orig_geom->objType[i] == POLYGONTYPE )
      {
         simp_obj = simplify_polygon3d((POLYGON3D *)orig_obj, dist);
      }
      else if ( orig_geom->objType[i] == POINTTYPE )
      {
         simp_obj = simplify_point3d((POINT3D *)orig_obj, dist);
      }
      else
      {
         elog(NOTICE, "Unknown geometry type");
         PG_RETURN_NULL();
      }

      /* Simplified object degenerated to empty set */
      if ( ! simp_obj ) continue;

      /* Get size of simplified object */
      simp_obj_size = size_subobject(simp_obj, orig_geom->objType[i]);

      /* Create one-object geometry (will add objects later) */
      if ( simp_geom == NULL )
      {
         simp_geom = make_oneobj_geometry(
            simp_obj_size, simp_obj, orig_geom->objType[i],
            orig_geom->is3d, orig_geom->SRID, orig_geom->scale,
            orig_geom->offsetX, orig_geom->offsetY);
      }

      /* Add object to already initialized geometry  */
      else
      {
         simp_geom = add_to_geometry(
                  simp_geom, simp_obj_size, simp_obj, 
                  orig_geom->objType[i] );
      }

      /* Error in simplified geometry construction */
      if ( ! simp_geom )
      {
         elog(ERROR, "geometry construction failed at iteration %d", i);
         PG_RETURN_NULL();
      }

   }

   if ( simp_geom == NULL ) PG_RETURN_NULL();

   /* Calculate the bounding box */
   bbox = bbox_of_geometry(simp_geom);
   memcpy(&(simp_geom->bvol), bbox, sizeof(BOX3D));
   pfree(bbox);


   simp_geom->type = orig_geom->type;
   PG_RETURN_POINTER(simp_geom);
}
/***********************************************************************
 * --strk@keybit.net;
 ***********************************************************************/

/***********************************************************************
 * Interpolate a point along a line, useful for Geocoding applications
 * SELECT line_interpolate_point( 'LINESTRING( 0 0, 2 2'::geometry, .5 )
 * returns POINT( 1 1 )
 *
 * -- jsunday@rochgrp.com;
 ***********************************************************************/
PG_FUNCTION_INFO_V1(line_interpolate_point);
Datum line_interpolate_point(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double distance = PG_GETARG_FLOAT8(1);

	int32 *offsets1;
	LINE3D *line;
	POINT3D point;
	int nsegs, i;
	double length, slength, tlength;

	if( distance < 0 || distance > 1 ) {
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if( geom->type != LINETYPE ) {
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] )) +
			sizeof(int32) * geom->nobjs );
	line = (LINE3D*) ( (char *)geom + offsets1[0] );

	/* If distance is one of the two extremes, return the point on that
	 * end rather than doing any expensive computations
	 */
	if( distance == 0.0 ) {
		PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D),
					/*(char*)&(line->points[0]), POINTTYPE, */
					(char*)&(line->points[0]), POINTTYPE,
					geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY
					)
				);
	}

	if( distance == 1.0 ) {
		PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D),
					(char*)&(line->points[line->npoints-1]), POINTTYPE, 
					geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY
					)
				);
	}

	/* Interpolate a point on the line */
	nsegs = line->npoints - 1;
	length = line_length2d( line );
	tlength = 0;
	for( i = 0; i < nsegs; i++ ) {
		POINT3D *p1, *p2;
		p1 = &(line->points[i]);
		p2 = &(line->points[i+1]);
		/* Find the relative length of this segment */
		slength = distance_pt_pt( p1, p2 )/length;
		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if( distance < tlength + slength ) {
			double dseg = (distance - tlength) / slength;
			point.x = (p1->x) + ((p2->x - p1->x) * dseg);
			point.y = (p1->y) + ((p2->y - p1->y) * dseg);
			point.z = 0;
			PG_RETURN_POINTER(
					make_oneobj_geometry(sizeof(POINT3D),
						(char*)&point, POINTTYPE, 
						geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY
						)
					);
		}
		tlength += slength;
	}
	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	PG_RETURN_POINTER(
			make_oneobj_geometry(sizeof(POINT3D),
				(char*)&(line->points[line->npoints-1]), POINTTYPE, 
				geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY
				)
			);
}
/***********************************************************************
 * --jsunday@rochgrp.com;
 ***********************************************************************/
