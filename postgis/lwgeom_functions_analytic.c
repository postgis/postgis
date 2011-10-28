/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2001-2005 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_pg.h"
#include "math.h"
#include "lwgeom_rtree.h"
#include "lwgeom_functions_analytic.h"


/***********************************************************************
 * Simple Douglas-Peucker line simplification.
 * No checks are done to avoid introduction of self-intersections.
 * No topology relations are considered.
 *
 * --strk@keybit.net;
 ***********************************************************************/


/* Prototypes */
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS);
Datum ST_LineCrossingDirection(PG_FUNCTION_ARGS);
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS);

double determineSide(POINT2D *seg1, POINT2D *seg2, POINT2D *point);
int isOnSegment(POINT2D *seg1, POINT2D *seg2, POINT2D *point);
int point_in_ring(POINTARRAY *pts, POINT2D *point);
int point_in_ring_rtree(RTREE_NODE *root, POINT2D *point);


PG_FUNCTION_INFO_V1(LWGEOM_simplify2d);
Datum LWGEOM_simplify2d(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *result;
	LWGEOM *in = lwgeom_from_gserialized(geom);
	LWGEOM *out;
	double dist = PG_GETARG_FLOAT8(1);

	out = lwgeom_simplify(in, dist);
	if ( ! out ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in->bbox ) lwgeom_add_bbox(out);

	result = pglwgeom_serialize(out);
	lwgeom_free(out);
	PG_FREE_IF_COPY(geom, 0);
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
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double distance = PG_GETARG_FLOAT8(1);
	LWLINE *line;
	LWPOINT *point;
	POINTARRAY *ipa, *opa;
	POINT4D pt;
	int nsegs, i;
	double length, slength, tlength;

	if ( distance < 0 || distance > 1 )
	{
		elog(ERROR,"line_interpolate_point: 2nd arg isnt within [0,1]");
		PG_RETURN_NULL();
	}

	if ( pglwgeom_get_type(geom) != LINETYPE )
	{
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	line = lwgeom_as_lwline(lwgeom_from_gserialized(geom));
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

		opa = ptarray_construct_reference_data(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1, (uint8_t*)&pt);
		
		point = lwpoint_construct(line->srid, NULL, opa);
		PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(point)));
	}

	/* Interpolate a point on the line */
	nsegs = ipa->npoints - 1;
	length = ptarray_length_2d(ipa);
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
			opa = ptarray_construct_reference_data(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1, (uint8_t*)&pt);
			point = lwpoint_construct(line->srid, NULL, opa);
			PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(point)));
		}
		tlength += slength;
	}

	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	getPoint4d_p(ipa, ipa->npoints-1, &pt);
	opa = ptarray_construct_reference_data(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1, (uint8_t*)&pt);
	point = lwpoint_construct(line->srid, NULL, opa);
	PG_RETURN_POINTER(pglwgeom_serialize(lwpoint_as_lwgeom(point)));
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
LWCIRCSTRING *lwcirc_grid(LWCIRCSTRING *line, gridspec *grid);
POINTARRAY *ptarray_grid(POINTARRAY *pa, gridspec *grid);
Datum LWGEOM_snaptogrid(PG_FUNCTION_ARGS);
Datum LWGEOM_snaptogrid_pointoff(PG_FUNCTION_ARGS);
static int grid_isNull(const gridspec *grid);
#if POSTGIS_DEBUG_LEVEL >=4
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

#if POSTGIS_DEBUG_LEVEL >= 4
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
	POINTARRAY *dpa;

	POSTGIS_DEBUGF(2, "ptarray_grid called on %p", pa);

	dpa = ptarray_construct_empty(FLAGS_GET_Z(pa->flags),FLAGS_GET_M(pa->flags), pa->npoints);

	for (ipn=0, opn=0; ipn<pa->npoints; ++ipn)
	{

		getPoint4d_p(pa, ipn, &pbuf);

		if ( grid->xsize )
			pbuf.x = rint((pbuf.x - grid->ipx)/grid->xsize) *
			         grid->xsize + grid->ipx;

		if ( grid->ysize )
			pbuf.y = rint((pbuf.y - grid->ipy)/grid->ysize) *
			         grid->ysize + grid->ipy;

		if ( FLAGS_GET_Z(pa->flags) && grid->zsize )
			pbuf.z = rint((pbuf.z - grid->ipz)/grid->zsize) *
			         grid->zsize + grid->ipz;

		if ( FLAGS_GET_M(pa->flags) && grid->msize )
			pbuf.m = rint((pbuf.m - grid->ipm)/grid->msize) *
			         grid->msize + grid->ipm;

		ptarray_append_point(dpa, &pbuf, LW_FALSE);

	}

	return dpa;
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
	oline = lwline_construct(line->srid, NULL, opa);

	return oline;
}

LWCIRCSTRING *
lwcirc_grid(LWCIRCSTRING *line, gridspec *grid)
{
	LWCIRCSTRING *oline;
	POINTARRAY *opa;

	opa = ptarray_grid(line->points, grid);

	/* Skip line3d with less then 2 points */
	if ( opa->npoints < 2 ) return NULL;

	/* TODO: grid bounding box... */
	oline = lwcircstring_construct(line->srid, NULL, opa);

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

	POSTGIS_DEBUGF(3, "grid_polygon3d: applying grid to polygon with %d rings",
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
			POSTGIS_DEBUG(4, "Before gridding: first point != last point");
#endif

		newring = ptarray_grid(ring, grid);

		/* Skip ring if not composed by at least 4 pts (3 segments) */
		if ( newring->npoints < 4 )
		{
			pfree(newring);

			POSTGIS_DEBUGF(3, "grid_polygon3d: ring%d skipped ( <4 pts )", ri);

			if ( ri ) continue;
			else break; /* this is the external ring, no need to work on holes */
		}

#if POSTGIS_DEBUG_LEVEL >= 4
		getPoint2d_p(newring, 0, &p1);
		getPoint2d_p(newring, newring->npoints-1, &p2);
		if ( ! SAMEPOINT(&p1, &p2) )
			POSTGIS_DEBUG(4, "After gridding: first point != last point");
#endif

		POSTGIS_DEBUGF(3, "grid_polygon3d: ring%d simplified from %d to %d points", ri,
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

	POSTGIS_DEBUGF(3, "grid_polygon3d: simplified polygon with %d rings", nrings);

	if ( ! nrings ) return NULL;

	opoly = lwpoly_construct(poly->srid, NULL, nrings, newrings);
	return opoly;
}

LWPOINT *
lwpoint_grid(LWPOINT *point, gridspec *grid)
{
	LWPOINT *opoint;
	POINTARRAY *opa;

	opa = ptarray_grid(point->point, grid);

	/* TODO: grid bounding box ? */
	opoint = lwpoint_construct(point->srid, NULL, opa);

	POSTGIS_DEBUG(2, "lwpoint_grid called");

	return opoint;
}

LWCOLLECTION *
lwcollection_grid(LWCOLLECTION *coll, gridspec *grid)
{
	uint32 i;
	LWGEOM **geoms;
	uint32 ngeoms=0;

	geoms = palloc(coll->ngeoms * sizeof(LWGEOM *));

	for (i=0; i<coll->ngeoms; i++)
	{
		LWGEOM *g = lwgeom_grid(coll->geoms[i], grid);
		if ( g ) geoms[ngeoms++] = g;
	}

	if ( ! ngeoms ) return lwcollection_construct_empty(COLLECTIONTYPE, coll->srid, 0, 0);

	return lwcollection_construct(coll->type, coll->srid,
	                              NULL, ngeoms, geoms);
}

LWGEOM *
lwgeom_grid(LWGEOM *lwgeom, gridspec *grid)
{
	switch (lwgeom->type)
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
	case COMPOUNDTYPE:
		return (LWGEOM *)lwcollection_grid((LWCOLLECTION *)lwgeom, grid);
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcirc_grid((LWCIRCSTRING *)lwgeom, grid);
	default:
		elog(ERROR, "lwgeom_grid: Unsupported geometry type: %s",
		     lwtype_name(lwgeom->type));
		return NULL;
	}
}

PG_FUNCTION_INFO_V1(LWGEOM_snaptogrid);
Datum LWGEOM_snaptogrid(PG_FUNCTION_ARGS)
{
	Datum datum;
	GSERIALIZED *in_geom;
	LWGEOM *in_lwgeom;
	GSERIALIZED *out_geom = NULL;
	LWGEOM *out_lwgeom;
	gridspec grid;
	/* BOX3D box3d; */

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(0);
	in_geom = (GSERIALIZED *)PG_DETOAST_DATUM(datum);

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

	in_lwgeom = lwgeom_from_gserialized(in_geom);

	POSTGIS_DEBUGF(3, "SnapToGrid got a %s", lwtype_name(in_lwgeom->type));

	out_lwgeom = lwgeom_grid(in_lwgeom, &grid);
	if ( out_lwgeom == NULL ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in_lwgeom->bbox ) lwgeom_add_bbox(out_lwgeom);


	POSTGIS_DEBUGF(3, "SnapToGrid made a %s", lwtype_name(out_lwgeom->type));

	out_geom = pglwgeom_serialize(out_lwgeom);

	PG_RETURN_POINTER(out_geom);
}

PG_FUNCTION_INFO_V1(LWGEOM_snaptogrid_pointoff);
Datum LWGEOM_snaptogrid_pointoff(PG_FUNCTION_ARGS)
{
	Datum datum;
	GSERIALIZED *in_geom, *in_point;
	LWGEOM *in_lwgeom;
	LWPOINT *in_lwpoint;
	GSERIALIZED *out_geom = NULL;
	LWGEOM *out_lwgeom;
	gridspec grid;
	/* BOX3D box3d; */
	POINT4D offsetpoint;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(0);
	in_geom = (GSERIALIZED *)PG_DETOAST_DATUM(datum);

	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	datum = PG_GETARG_DATUM(1);
	in_point = (GSERIALIZED *)PG_DETOAST_DATUM(datum);
	in_lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(in_point));
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
	if (FLAGS_GET_Z(in_lwpoint->flags) ) grid.ipz = offsetpoint.z;
	else grid.ipz=0;
	if (FLAGS_GET_M(in_lwpoint->flags) ) grid.ipm = offsetpoint.m;
	else grid.ipm=0;

#if POSTGIS_DEBUG_LEVEL >= 4
	grid_print(&grid);
#endif

	/* Return input geometry if grid is null */
	if ( grid_isNull(&grid) )
	{
		PG_RETURN_POINTER(in_geom);
	}

	in_lwgeom = lwgeom_from_gserialized(in_geom);

	POSTGIS_DEBUGF(3, "SnapToGrid got a %s", lwtype_name(in_lwgeom->type));

	out_lwgeom = lwgeom_grid(in_lwgeom, &grid);
	if ( out_lwgeom == NULL ) PG_RETURN_NULL();

	/* COMPUTE_BBOX TAINTING */
	if ( in_lwgeom->bbox ) lwgeom_add_bbox(out_lwgeom);

	POSTGIS_DEBUGF(3, "SnapToGrid made a %s", lwtype_name(out_lwgeom->type));

	out_geom = pglwgeom_serialize(out_lwgeom);

	PG_RETURN_POINTER(out_geom);
}


/*
** crossingDirection(line1, line2)
**
** Determines crossing direction of line2 relative to line1.
** Only accepts LINESTRING as parameters!
*/
PG_FUNCTION_INFO_V1(ST_LineCrossingDirection);
Datum ST_LineCrossingDirection(PG_FUNCTION_ARGS)
{
	int type1, type2, rv;
	LWLINE *l1 = NULL;
	LWLINE *l2 = NULL;
	GSERIALIZED *geom1 = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);

	if ( type1 != LINETYPE || type2 != LINETYPE )
	{
		elog(ERROR,"This function only accepts LINESTRING as arguments.");
		PG_RETURN_NULL();
	}

	l1 = lwgeom_as_lwline(lwgeom_from_gserialized(geom1));
	l2 = lwgeom_as_lwline(lwgeom_from_gserialized(geom2));

	rv = lwline_crossing_direction(l1, l2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_INT32(rv);

}

PG_FUNCTION_INFO_V1(ST_LocateBetweenElevations);
Datum ST_LocateBetweenElevations(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom_in = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	LWCOLLECTION *geom_out = NULL;
	LWGEOM *line_in = NULL;
	char geomtype = pglwgeom_get_type(geom_in);
	static int ordinate = 2; /* Z */

	if ( ! ( geomtype == LINETYPE || geomtype == MULTILINETYPE ) )
	{
		elog(ERROR,"This function only accepts LINESTRING or MULTILINESTRING as arguments.");
		PG_RETURN_NULL();
	}

	line_in = lwgeom_from_gserialized(geom_in);

	if ( ! FLAGS_GET_Z(line_in->flags) )
	{
		elog(ERROR,"This function only accepts LINESTRING or MULTILINESTRING with Z values as arguments.");
		PG_RETURN_NULL();
	}

	if ( geomtype == LINETYPE )
	{
		geom_out = lwline_clip_to_ordinate_range((LWLINE*)line_in, ordinate, from, to);
	}
	else if ( geomtype == MULTILINETYPE )
	{
		geom_out = lwmline_clip_to_ordinate_range((LWMLINE*)line_in, ordinate, from, to);
	}
	lwgeom_free(line_in);

	if ( ! geom_out )
	{
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
	GSERIALIZED *geom = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double from = PG_GETARG_FLOAT8(1);
	double to = PG_GETARG_FLOAT8(2);
	LWGEOM *olwgeom;
	POINTARRAY *ipa, *opa;
	GSERIALIZED *ret;
	int type = pglwgeom_get_type(geom);

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

	if ( type == LINETYPE )
	{
		LWLINE *iline = lwgeom_as_lwline(lwgeom_from_gserialized(geom));

		if ( lwgeom_is_empty((LWGEOM*)iline) )
		{
			/* TODO return empty line */
			lwline_release(iline);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_NULL();
		}

		ipa = iline->points;

		opa = ptarray_substring(ipa, from, to);

		if ( opa->npoints == 1 ) /* Point returned */
			olwgeom = (LWGEOM *)lwpoint_construct(iline->srid, NULL, opa);
		else
			olwgeom = (LWGEOM *)lwline_construct(iline->srid, NULL, opa);

	}
	else if ( type == MULTILINETYPE )
	{
		LWMLINE *iline;
		int i = 0, g = 0;
		int homogeneous = LW_TRUE;
		LWGEOM **geoms = NULL;
		double length = 0.0, sublength = 0.0, minprop = 0.0, maxprop = 0.0;

		iline = lwgeom_as_lwmline(lwgeom_from_gserialized(geom));

		if ( lwgeom_is_empty((LWGEOM*)iline) )
		{
			/* TODO return empty collection */
			lwmline_release(iline);
			PG_FREE_IF_COPY(geom, 0);
			PG_RETURN_NULL();
		}

		/* Calculate the total length of the mline */
		for ( i = 0; i < iline->ngeoms; i++ )
		{
			LWLINE *subline = (LWLINE*)iline->geoms[i];
			if ( subline->points && subline->points->npoints > 1 )
				length += ptarray_length_2d(subline->points);
		}

		geoms = lwalloc(sizeof(LWGEOM*) * iline->ngeoms);

		/* Slice each sub-geometry of the multiline */
		for ( i = 0; i < iline->ngeoms; i++ )
		{
			LWLINE *subline = (LWLINE*)iline->geoms[i];
			double subfrom = 0.0, subto = 0.0;

			if ( subline->points && subline->points->npoints > 1 )
				sublength += ptarray_length_2d(subline->points);

			/* Calculate proportions for this subline */
			minprop = maxprop;
			maxprop = sublength / length;

			/* This subline doesn't reach the lowest proportion requested
			   or is beyond the highest proporton */
			if ( from > maxprop || to < minprop )
				continue;

			if ( from <= minprop )
				subfrom = 0.0;
			if ( to >= maxprop )
				subto = 1.0;

			if ( from > minprop && from <= maxprop )
				subfrom = (from - minprop) / (maxprop - minprop);

			if ( to < maxprop && to >= minprop )
				subto = (to - minprop) / (maxprop - minprop);


			opa = ptarray_substring(subline->points, subfrom, subto);
			if ( opa && opa->npoints > 0 )
			{
				if ( opa->npoints == 1 ) /* Point returned */
				{
					geoms[g] = (LWGEOM *)lwpoint_construct(iline->srid, NULL, opa);
					homogeneous = LW_FALSE;
				}
				else
				{
					geoms[g] = (LWGEOM *)lwline_construct(iline->srid, NULL, opa);
				}
				g++;
			}



		}
		/* If we got any points, we need to return a GEOMETRYCOLLECTION */
		if ( ! homogeneous )
			type = COLLECTIONTYPE;

		olwgeom = (LWGEOM*)lwcollection_construct(type, iline->srid, NULL, g, geoms);
	}
	else
	{
		elog(ERROR,"line_interpolate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}

	ret = pglwgeom_serialize(olwgeom);
	lwgeom_free(olwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(ret);

}

Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(LWGEOM_line_locate_point);
Datum LWGEOM_line_locate_point(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWLINE *lwline;
	LWPOINT *lwpoint;
	POINTARRAY *pa;
	POINT2D p;
	double ret;

	if ( pglwgeom_get_type(geom1) != LINETYPE )
	{
		elog(ERROR,"line_locate_point: 1st arg isnt a line");
		PG_RETURN_NULL();
	}
	if ( pglwgeom_get_type(geom2) != POINTTYPE )
	{
		elog(ERROR,"line_locate_point: 2st arg isnt a point");
		PG_RETURN_NULL();
	}
	if ( gserialized_get_srid(geom1) != gserialized_get_srid(geom2) )
	{
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	lwline = lwgeom_as_lwline(lwgeom_from_gserialized(geom1));
	lwpoint = lwgeom_as_lwpoint(lwgeom_from_gserialized(geom2));

	pa = lwline->points;
	lwpoint_getPoint2d_p(lwpoint, &p);

	ret = ptarray_locate_point(pa, &p, NULL);

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

	POSTGIS_DEBUGF(3, "maxX minX/maxY minY: %.8f %.8f/%.8f %.8f", maxX, minX, maxY, minY);

	if (maxX < point->x || minX > point->x)
	{
		POSTGIS_DEBUGF(3, "X value %.8f falls outside the range %.8f-%.8f", point->x, minX, maxX);

		return 0;
	}
	else if (maxY < point->y || minY > point->y)
	{
		POSTGIS_DEBUGF(3, "Y value %.8f falls outside the range %.8f-%.8f", point->y, minY, maxY);

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

	POSTGIS_DEBUG(2, "point_in_ring called.");

	lines = findLineSegments(root, point->y);
	if (!lines)
		return -1;

	for (i=0; i<lines->ngeoms; i++)
	{
		getPoint2d_p(lines->geoms[i]->points, 0, &seg1);
		getPoint2d_p(lines->geoms[i]->points, 1, &seg2);


		side = determineSide(&seg1, &seg2, point);

		POSTGIS_DEBUGF(3, "segment: (%.8f, %.8f),(%.8f, %.8f)", seg1.x, seg1.y, seg2.x, seg2.y);
		POSTGIS_DEBUGF(3, "side result: %.8f", side);
		POSTGIS_DEBUGF(3, "counterclockwise wrap %d, clockwise wrap %d", FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y), FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y));

		/* zero length segments are ignored. */
		if (((seg2.x-seg1.x)*(seg2.x-seg1.x)+(seg2.y-seg1.y)*(seg2.y-seg1.y)) < 1e-12*1e-12)
		{
			POSTGIS_DEBUG(3, "segment is zero length... ignoring.");

			continue;
		}

		/* a point on the boundary of a ring is not contained. */
		if (fabs(side) < 1e-12)
		{
			if (isOnSegment(&seg1, &seg2, point) == 1)
			{
				POSTGIS_DEBUGF(3, "point on ring boundary between points %d, %d", i, i+1);

				return 0;
			}
		}

		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and
		 * circling counter-clockwise, so incremement.
		 */
		if (FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y) && side>0)
		{
			POSTGIS_DEBUG(3, "incrementing winding number.");

			++wn;
		}
		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y) && side<0)
		{
			POSTGIS_DEBUG(3, "decrementing winding number.");

			--wn;
		}
	}

	POSTGIS_DEBUGF(3, "winding number %d", wn);

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

	POSTGIS_DEBUG(2, "point_in_ring called.");


	for (i=0; i<pts->npoints-1; i++)
	{
		getPoint2d_p(pts, i, &seg1);
		getPoint2d_p(pts, i+1, &seg2);


		side = determineSide(&seg1, &seg2, point);

		POSTGIS_DEBUGF(3, "segment: (%.8f, %.8f),(%.8f, %.8f)", seg1.x, seg1.y, seg2.x, seg2.y);
		POSTGIS_DEBUGF(3, "side result: %.8f", side);
		POSTGIS_DEBUGF(3, "counterclockwise wrap %d, clockwise wrap %d", FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y), FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y));

		/* zero length segments are ignored. */
		if (((seg2.x-seg1.x)*(seg2.x-seg1.x)+(seg2.y-seg1.y)*(seg2.y-seg1.y)) < 1e-12*1e-12)
		{
			POSTGIS_DEBUG(3, "segment is zero length... ignoring.");

			continue;
		}

		/* a point on the boundary of a ring is not contained. */
		if (fabs(side) < 1e-12)
		{
			if (isOnSegment(&seg1, &seg2, point) == 1)
			{
				POSTGIS_DEBUGF(3, "point on ring boundary between points %d, %d", i, i+1);

				return 0;
			}
		}

		/*
		 * If the point is to the left of the line, and it's rising,
		 * then the line is to the right of the point and
		 * circling counter-clockwise, so incremement.
		 */
		if (FP_CONTAINS_BOTTOM(seg1.y,point->y,seg2.y) && side>0)
		{
			POSTGIS_DEBUG(3, "incrementing winding number.");

			++wn;
		}
		/*
		 * If the point is to the right of the line, and it's falling,
		 * then the line is to the right of the point and circling
		 * clockwise, so decrement.
		 */
		else if (FP_CONTAINS_BOTTOM(seg2.y,point->y,seg1.y) && side<0)
		{
			POSTGIS_DEBUG(3, "decrementing winding number.");

			--wn;
		}
	}

	POSTGIS_DEBUGF(3, "winding number %d", wn);

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

	POSTGIS_DEBUGF(2, "point_in_polygon called for %p %d %p.", root, ringCount, point);

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	if (point_in_ring_rtree(root[0], &pt) != 1)
	{
		POSTGIS_DEBUG(3, "point_in_polygon_rtree: outside exterior ring.");

		return 0;
	}

	for (i=1; i<ringCount; i++)
	{
		if (point_in_ring_rtree(root[i], &pt) != -1)
		{
			POSTGIS_DEBUGF(3, "point_in_polygon_rtree: within hole %d.", i);

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
 * Expected **root order is each exterior ring followed by its holes, eg. EIIEIIEI
 */
int point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int *ringCounts, LWPOINT *point)
{
	int i, p, r, in_ring;
	POINT2D pt;
	int result = -1;

	POSTGIS_DEBUGF(2, "point_in_multipolygon_rtree called for %p %d %p.", root, polyCount, point);

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

        i = 0; /* the current index into the root array */

	/* is the point inside any of the sub-polygons? */
	for ( p = 0; p < polyCount; p++ )
	{
		in_ring = point_in_ring_rtree(root[i], &pt);
		POSTGIS_DEBUGF(4, "point_in_multipolygon_rtree: exterior ring (%d), point_in_ring returned %d", p, in_ring);
		if ( in_ring == -1 ) /* outside the exterior ring */
		{
			POSTGIS_DEBUG(3, "point_in_multipolygon_rtree: outside exterior ring.");
		}
         	else if ( in_ring == 0 ) /* on the boundary */
		{
			POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: on edge of exterior ring %d", p);
                        return 0;
		} else {
                	result = in_ring;

	                for(r=1; r<ringCounts[p]; r++)
     	                {
                        	in_ring = point_in_ring_rtree(root[i+r], &pt);
		        	POSTGIS_DEBUGF(4, "point_in_multipolygon_rtree: interior ring (%d), point_in_ring returned %d", r, in_ring);
                        	if (in_ring == 1) /* inside a hole => outside the polygon */
                        	{
                                	POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: within hole %d of exterior ring %d", r, p);
                                	result = -1;
                                	break;
                        	}
                        	if (in_ring == 0) /* on the edge of a hole */
                        	{
			        	POSTGIS_DEBUGF(3, "point_in_multipolygon_rtree: on edge of hole %d of exterior ring %d", r, p);
                                	return 0;
		        	}
                	}
                	/* if we have a positive result, we can short-circuit and return it */
                	if ( result != -1)
                	{
                        	return result;
                	}
                }
                /* increment the index by the total number of rings in the sub-poly */
                /* we do this here in case we short-cutted out of the poly before looking at all the rings */ 
                i += ringCounts[p];
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

	POSTGIS_DEBUG(2, "point_in_polygon called.");

	getPoint2d_p(point->point, 0, &pt);
	/* assume bbox short-circuit has already been attempted */

	ring = polygon->rings[0];
	in_ring = point_in_ring(polygon->rings[0], &pt);
	if ( in_ring == -1) /* outside the exterior ring */
	{
		POSTGIS_DEBUG(3, "point_in_polygon: outside exterior ring.");
		return -1;
	}
	result = in_ring;

	for (i=1; i<polygon->nrings; i++)
	{
		ring = polygon->rings[i];
		in_ring = point_in_ring(polygon->rings[i], &pt);
		if (in_ring == 1) /* inside a hole => outside the polygon */
		{
			POSTGIS_DEBUGF(3, "point_in_polygon: within hole %d.", i);
			return -1;
		}
		if (in_ring == 0) /* on the edge of a hole */
		{
			POSTGIS_DEBUGF(3, "point_in_polygon: on edge of hole %d.", i);
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

	POSTGIS_DEBUG(2, "point_in_polygon called.");

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
			POSTGIS_DEBUG(3, "point_in_polygon: outside exterior ring.");
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
				POSTGIS_DEBUGF(3, "point_in_polygon: within hole %d.", i);
				result = -1;
				break;
			}
			if (in_ring == 0) /* on the edge of a hole */
			{
				POSTGIS_DEBUGF(3, "point_in_polygon: on edge of hole %d.", i);
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

