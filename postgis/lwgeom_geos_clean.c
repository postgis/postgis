/**********************************************************************
 * $Id: lwgeom_geos.c 5258 2010-02-17 21:02:49Z strk $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2009-2010 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * ST_MakeValid
 *
 * Attempts to make an invalid geometries valid w/out loosing
 * point sets.
 *
 * Polygons may become collection of polygons and lines.
 * Collapsed rings (or portions of rings) may be dissolved in
 * polygon area or transformed to linestring if outside any other
 * ring.
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to Dr. Horst Duester for previous work on a plpgsql version
 * of the cleanup logic [1]
 *
 * Thanks to Andrea Peri for recommandations on constraints.
 *
 * [1] http://www.sogis1.so.ch/sogis/dl/postgis/cleanGeometry.sql
 *
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "funcapi.h"

#include <string.h>
#include <assert.h>

#if POSTGIS_GEOS_VERSION >= 32
#endif

/* #define POSTGIS_DEBUG_LEVEL 0 */


#define BUFSIZE 256
static char loggederror[BUFSIZE];

static void
errorlogger(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	if ( BUFSIZE-1 < vsnprintf(loggederror, BUFSIZE-1, fmt, ap) )
	{
		loggederror[BUFSIZE-1] = '\0';
	}

	va_end(ap);
}

/*
 * Return Nth vertex in GEOSGeometry as a POINT.
 * May return NULL if the geometry has NO vertexex.
 */
GEOSGeometry* LWGEOM_GEOS_getPointN(const GEOSGeometry*, unsigned int);
GEOSGeometry*
LWGEOM_GEOS_getPointN(const GEOSGeometry* g_in, unsigned int n)
{
	unsigned int dims;
	const GEOSCoordSequence* seq_in;
	GEOSCoordSeq seq_out;
	double val;
	unsigned int sz;
	int gn;
	GEOSGeometry* ret;

	switch ( GEOSGeomTypeId(g_in) )
	{
	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
	{
		for (gn=0; gn<GEOSGetNumGeometries(g_in); ++gn)
		{
			const GEOSGeometry* g = GEOSGetGeometryN(g_in, gn);
			ret = LWGEOM_GEOS_getPointN(g,n);
			if ( ret ) return ret;
		}
		break;
	}

	case GEOS_POLYGON:
	{
		ret = LWGEOM_GEOS_getPointN(GEOSGetExteriorRing(g_in), n);
		if ( ret ) return ret;
		for (gn=0; gn<GEOSGetNumInteriorRings(g_in); ++gn)
		{
			const GEOSGeometry* g = GEOSGetInteriorRingN(g_in, gn);
			ret = LWGEOM_GEOS_getPointN(g, n);
			if ( ret ) return ret;
		}
		break;
	}

	case GEOS_POINT:
	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		break;

	}

	seq_in = GEOSGeom_getCoordSeq(g_in);
	if ( ! seq_in ) return NULL;
	if ( ! GEOSCoordSeq_getSize(seq_in, &sz) ) return NULL;
	if ( ! sz ) return NULL;

	if ( ! GEOSCoordSeq_getDimensions(seq_in, &dims) ) return NULL;

	seq_out = GEOSCoordSeq_create(1, dims);
	if ( ! seq_out ) return NULL;

	if ( ! GEOSCoordSeq_getX(seq_in, n, &val) ) return NULL;
	if ( ! GEOSCoordSeq_setX(seq_out, n, val) ) return NULL;
	if ( ! GEOSCoordSeq_getY(seq_in, n, &val) ) return NULL;
	if ( ! GEOSCoordSeq_setY(seq_out, n, val) ) return NULL;
	if ( dims > 2 )
	{
		if ( ! GEOSCoordSeq_getZ(seq_in, n, &val) ) return NULL;
		if ( ! GEOSCoordSeq_setZ(seq_out, n, val) ) return NULL;
	}

	return GEOSGeom_createPoint(seq_out);
}



LWGEOM * lwcollection_make_geos_friendly(LWCOLLECTION *g);
LWGEOM * lwline_make_geos_friendly(LWLINE *line);
LWGEOM * lwpoly_make_geos_friendly(LWPOLY *poly);
POINTARRAY* ring_make_geos_friendly(POINTARRAY* ring);

/*
 * Ensure the geometry is "structurally" valid
 * (enough for GEOS to accept it)
 * May return the input untouched (if already valid).
 * May return geometries of lower dimension (on collapses)
 */
static LWGEOM *
lwgeom_make_geos_friendly(LWGEOM *geom)
{
	LWDEBUGF(2, "lwgeom_make_geos_friendly enter (type %d)", TYPE_GETTYPE(geom->type));
	switch (TYPE_GETTYPE(geom->type))
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		/* a point is always valid */
		return geom;
		break;

	case LINETYPE:
		/* lines need at least 2 points */
		return lwline_make_geos_friendly((LWLINE *)geom);
		break;

	case POLYGONTYPE:
		/* polygons need all rings closed and with npoints > 3 */
		return lwpoly_make_geos_friendly((LWPOLY *)geom);
		break;

	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return lwcollection_make_geos_friendly((LWCOLLECTION *)geom);
		break;

	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
	default:
		lwerror("unsupported input geometry type: %d", TYPE_GETTYPE(geom->type));
		break;
	}
	return 0;
}

/*
 * Close the point array, if not already closed in 2d.
 * Returns the input if already closed in 2d, or a newly
 * constructed POINTARRAY.
 * TODO: move in ptarray.c
 */
POINTARRAY* ptarray_close2d(POINTARRAY* ring);
POINTARRAY*
ptarray_close2d(POINTARRAY* ring)
{
	POINTARRAY* newring;

	/* close the ring if not already closed (2d only) */
	if ( ! ptarray_isclosed2d(ring) )
	{
		/* close it up */
		newring = ptarray_addPoint(ring,
		                           getPoint_internal(ring, 0),
		                           TYPE_NDIMS(ring->dims),
		                           ring->npoints);
		ring = newring;
	}
	return ring;
}

/* May return the same input or a new one (never zero) */
POINTARRAY*
ring_make_geos_friendly(POINTARRAY* ring)
{
	POINTARRAY* closedring;

	/* close the ring if not already closed (2d only) */
	closedring = ptarray_close2d(ring);
	if (closedring != ring )
	{
		ptarray_free(ring); /* should we do this ? */
		ring = closedring;
	}

	/* return 0 for collapsed ring (after closeup) */

	while ( ring->npoints < 4 )
	{
		LWDEBUGF(4, "ring has %d points, adding another", ring->npoints);
		/* let's add another... */
		ring = ptarray_addPoint(ring,
		                        getPoint_internal(ring, 0),
		                        TYPE_NDIMS(ring->dims),
		                        ring->npoints);
	}


	return ring;
}

/* Make sure all rings are closed and have > 3 points.
 * May return the input untouched.
 */
LWGEOM *
lwpoly_make_geos_friendly(LWPOLY *poly)
{
	LWGEOM* ret;
	POINTARRAY **new_rings;
	int i;

	/* If the polygon has no rings there's nothing to do */
	if ( ! poly->nrings ) return (LWGEOM*)poly;

	/* Allocate enough pointers for all rings */
	new_rings = lwalloc(sizeof(POINTARRAY*)*poly->nrings);

	/* All rings must be closed and have > 3 points */
	for (i=0; i<poly->nrings; i++)
	{
		POINTARRAY* ring_in = poly->rings[i];
		POINTARRAY* ring_out = ring_make_geos_friendly(ring_in);

		if ( ring_in != ring_out )
		{
			LWDEBUGF(3, "lwpoly_make_geos_friendly: ring %d cleaned, now has %d points", i, ring_out->npoints);
			/* this may come right from
			 * the binary representation lands
			 */
			/*ptarray_free(ring_in); */
		}
		else
		{
			LWDEBUGF(3, "lwpoly_make_geos_friendly: ring %d untouched", i);
		}

		assert ( ring_out );
		new_rings[i] = ring_out;
	}

	lwfree(poly->rings);
	poly->rings = new_rings;
	ret = (LWGEOM*)poly;

	return ret;
}

/* Need NO or >1 points. Duplicate first if only one. */
LWGEOM *
lwline_make_geos_friendly(LWLINE *line)
{
	LWGEOM *ret;

	if (line->points->npoints == 1) /* 0 is fine, 2 is fine */
	{
#if 1
		/* Duplicate point */
		line->points = ptarray_addPoint(line->points,
		                                getPoint_internal(line->points, 0),
		                                TYPE_NDIMS(line->points->dims),
		                                line->points->npoints);
		ret = (LWGEOM*)line;
#else
		/* Turn into a point */
		ret = (LWGEOM*)lwpoint_construct(line->SRID, 0, line->points);
#endif
		return ret;
	}
	else
	{
		return (LWGEOM*)line;
		/* return lwline_clone(line); */
	}
}

LWGEOM *
lwcollection_make_geos_friendly(LWCOLLECTION *g)
{
	LWGEOM **new_geoms;
	uint32 i, new_ngeoms=0;
	LWCOLLECTION *ret;

	/* enough space for all components */
	new_geoms = lwalloc(sizeof(LWGEOM *)*g->ngeoms);

	ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, g, sizeof(LWCOLLECTION));

	for (i=0; i<g->ngeoms; i++)
	{
		LWGEOM* newg = lwgeom_make_geos_friendly(g->geoms[i]);
		if ( newg ) new_geoms[new_ngeoms++] = newg;
	}

	ret->bbox = 0; /* recompute later... */

	ret->ngeoms = new_ngeoms;
	if ( new_ngeoms )
	{
		ret->geoms = new_geoms;
	}
	else
	{
		free(new_geoms);
		ret->geoms = 0;
	}

	return (LWGEOM*)ret;
}

/*
 * Fully node given linework
 * TODO: move to GEOS capi
 */
static GEOSGeometry*
LWGEOM_GEOS_nodeLines(GEOSGeometry* lines)
{
	GEOSGeometry* noded;
	GEOSGeometry* point;

	/*
	 * Union with first geometry point, obtaining full noding
	 * and dissolving of duplicated repeated points
	 *
	 * TODO: substitute this with UnaryUnion?
	 */

	point = LWGEOM_GEOS_getPointN(lines, 0);
	if ( ! point ) return NULL;

	POSTGIS_DEBUGF(3,
	               "Boundary point: %s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(point, 0),
	                              PARSER_CHECK_NONE));

	noded = GEOSUnion(lines, point);
	if ( NULL == noded )
	{
		GEOSGeom_destroy(point);
		return NULL;
	}

	GEOSGeom_destroy(point);

	POSTGIS_DEBUGF(3,
	               "LWGEOM_GEOS_nodeLines: in[%s] out[%s]",
	               lwgeom_to_ewkt(GEOS2LWGEOM(lines, 0),
	                              PARSER_CHECK_NONE),
	               lwgeom_to_ewkt(GEOS2LWGEOM(noded, 0),
	                              PARSER_CHECK_NONE));

	return noded;
}

/*
 * We expect initGEOS being called already.
 * Will return NULL on error (expect error handler being called by then)
 *
 */
static GEOSGeometry*
LWGEOM_GEOS_makeValidPolygon(const GEOSGeometry* gin)
{
	GEOSGeom gout;
	GEOSGeom geos_bound, geos_bound_noded;
	GEOSGeom geos_cut_edges, geos_area;
	GEOSGeometry *vgeoms[2]; /* One for area, one for cut-edges */
	int num_cut_edges;

	assert (GEOSGeomTypeId(gin) == GEOS_POLYGON ||
	        GEOSGeomTypeId(gin) == GEOS_MULTIPOLYGON);

	geos_bound = GEOSBoundary(gin);
	if ( NULL == geos_bound )
	{
		return NULL;
	}

	POSTGIS_DEBUGF(3,
	               "Boundaries: %s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_bound, 0),
	                              PARSER_CHECK_NONE));

	/* Node boundaries */

	geos_bound_noded = LWGEOM_GEOS_nodeLines(geos_bound);
	if ( NULL == geos_bound_noded )
	{
		lwnotice("LWGEOM_GEOS_nodeLines(): %s", loggederror);
		GEOSGeom_destroy(geos_bound);
		return NULL;
	}

	GEOSGeom_destroy(geos_bound);

	POSTGIS_DEBUGF(3,
	               "Noded: %s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_bound_noded, 0),
	                              PARSER_CHECK_NONE));

	geos_area = LWGEOM_GEOS_buildArea(geos_bound_noded);
	if ( ! geos_area ) /* must be an exception ... */
	{
		/* cleanup */
		GEOSGeom_destroy(geos_bound_noded);
		lwnotice("LWGEOM_GEOS_buildArea() threw an error: %s",
		         loggederror);
		return NULL;
	}

	if ( GEOSisEmpty(geos_area) )
	{
		/* no area, it's all boundary */
		POSTGIS_DEBUG(3, "No area found, returning noded bounds");
		GEOSGeom_destroy(geos_area);
		return geos_bound_noded;
	}

	/* Compute what's left out from original boundary
	 * (this would be the so called 'cut lines' */
	geos_bound = GEOSBoundary(geos_area);
	if ( ! geos_bound )
	{
		/* Empty area most likely,
		 * noded original bounds are the result */
		lwnotice("GEOSBoundary('%s') threw an error: %s",
		         lwgeom_to_ewkt(GEOS2LWGEOM(geos_area, 0),
		                        PARSER_CHECK_NONE),
		         loggederror);
		return geos_bound_noded;
	}

	geos_cut_edges = GEOSDifference(geos_bound_noded, geos_bound);
#if 0
	{
		GEOSGeometry const *vgeoms[1];
		vgeoms[0] = geos_bound_noded;
		geos_cut_edges = GEOSPolygonizer_getCutEdges(vgeoms, 1);
	}
#endif
	GEOSGeom_destroy(geos_bound);
	if ( ! geos_cut_edges )   /* an exception ? */
	{
		/* cleanup and throw */
		GEOSGeom_destroy(geos_bound_noded);
		GEOSGeom_destroy(geos_area);
		lwnotice("GEOSDifference() threw an error: %s",
		         loggederror);
		return NULL;
	}

	POSTGIS_DEBUGF(3,
	               "Cut-edges: %s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_cut_edges, 0),
	                              PARSER_CHECK_NONE));

	num_cut_edges = GEOSGetNumGeometries(geos_cut_edges);
	if ( -1 == num_cut_edges )
	{
		lwnotice("GEOSGetNumGeometries() threw an error: %s",
		         loggederror);
		GEOSGeom_destroy(geos_cut_edges);
		GEOSGeom_destroy(geos_bound_noded);
		return geos_area;
	}

	/* If we have no cut edges the result is the area */
	/* NOTE: this is a short-cut we may want to drop */
	if ( 0 == num_cut_edges )
	{
		GEOSGeom_destroy(geos_cut_edges);
		GEOSGeom_destroy(geos_bound_noded);
		return geos_area;
	}

	/*
	 * Drop cut-edge segments for which all
	 * vertices are already vertices of geos_bound_noded
	 * which fall completely in the area
	 */
	{

		GEOSGeometry** cutlines;
		GEOSGeometry* geos_bound_noded_points;
		int kept_edges = 0;
		int i;

		geos_bound_noded_points = GEOSGeom_extractUniquePoints(geos_area);
		if ( ! geos_bound_noded_points )
		{
			/* Exception I guess ? */
			lwnotice("GEOSGeom_extractUniquePoints: %s", loggederror);
			return 0;
		}

		/* Allocate enough space for keeping all cut edges */
		cutlines = (GEOSGeometry**)lwalloc(sizeof(GEOSGeometry*)*num_cut_edges);
		for (i=0; i<num_cut_edges; ++i)
		{
			char ret;
			GEOSGeometry* xset;
			const GEOSGeometry* cutline = GEOSGetGeometryN(geos_cut_edges, i);
			GEOSGeometry* cutline_points = GEOSGeom_extractUniquePoints(cutline);
			if ( ! cutline_points )
			{
				/* Exception I guess ? */
				lwnotice("GEOSGeom_extractUniquePoints: %s", loggederror);
				GEOSGeom_destroy(geos_cut_edges);
				GEOSGeom_destroy(geos_bound_noded_points);
				lwfree(cutlines);
				return 0;
			}

			xset = GEOSDifference(cutline_points, geos_bound_noded_points);
			GEOSGeom_destroy(cutline_points);
			if ( ! xset )
			{
				/* exception */
				lwnotice("GEOSDifference: %s", loggederror);
				GEOSGeom_destroy(geos_cut_edges);
				GEOSGeom_destroy(geos_bound_noded_points);
				lwfree(cutlines);
				return 0;
			}
			ret = GEOSisEmpty(xset);
			GEOSGeom_destroy(xset);
			if ( 2 == ret )   /* exception */
			{
				lwnotice("GEOSisEmpty: %s", loggederror);
				GEOSGeom_destroy(geos_cut_edges);
				GEOSGeom_destroy(geos_bound_noded_points);
				lwfree(cutlines);
				return 0;
			}
			if ( 1 == ret )
			{
				/* all points already represented */
				POSTGIS_DEBUGF(3,
				               "Cut-edge: %s - vertices all found, drop",
				               lwgeom_to_ewkt(GEOS2LWGEOM(cutline, 0),
				                              PARSER_CHECK_NONE));
				continue;
			}

			POSTGIS_DEBUGF(3,
			               "Cut-edge: %s - missing vertices, keep",
			               lwgeom_to_ewkt(GEOS2LWGEOM(cutline, 0),
			                              PARSER_CHECK_NONE));

			cutlines[kept_edges++] = GEOSGeom_clone(cutline);
		}

		GEOSGeom_destroy(geos_bound_noded_points);
		GEOSGeom_destroy(geos_cut_edges);

		if ( kept_edges )
		{
			geos_cut_edges = GEOSGeom_createCollection(GEOS_MULTILINESTRING, cutlines, kept_edges);
			if ( ! geos_cut_edges )
			{
				lwnotice("GEOSGeom_createCollection: %s", loggederror);
				return 0;
			}

			POSTGIS_DEBUGF(3,
			               "Trimmed cut-edges: %s",
			               lwgeom_to_ewkt(GEOS2LWGEOM(geos_cut_edges, 0),
			                              PARSER_CHECK_NONE));
		}
		else
		{
			geos_cut_edges = 0;
		}

	}

	GEOSGeom_destroy(geos_bound_noded); /* TODO: move up */

	/* Collect areas and lines (if any line) */
	if ( geos_cut_edges )
	{
		vgeoms[1] = geos_area;
		vgeoms[0] = geos_cut_edges;
		gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, vgeoms, 2);
		if ( ! gout )   /* an exception again */
		{
			/* cleanup and throw */
			lwnotice("GEOSGeom_createCollection() threw an error: %s",
			         loggederror);
			return NULL;
		}

	}
	else
	{
		gout = geos_area;
	}

	return gout;
}

Datum st_makevalid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(st_makevalid);
Datum st_makevalid(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 33
	elog(ERROR, "You need GEOS-3.3.0 or up for ST_MakeValid");
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */

	PG_LWGEOM *in, *out;
	GEOSGeom geosgeom;
	LWGEOM *lwgeom_in, *lwgeom_out;
	/* LWGEOM *lwgeom_pointset; */
	char ret_char;
	int is3d;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom_in = lwgeom_deserialize(SERIALIZED_FORM(in));

	is3d = TYPE_HASZ(lwgeom_in->type);

	/*
	 * Step 1 : try to convert to GEOS, if impossible, clean that up first
	 *          otherwise (adding only duplicates of existing points)
	 */

	initGEOS(errorlogger, errorlogger);


	lwgeom_out = lwgeom_in;
	geosgeom = LWGEOM2GEOS(lwgeom_out);
	if ( ! geosgeom )
	{
		POSTGIS_DEBUGF(4,
		               "Original geom can't be converted to GEOS (%s)"
		               " - will try cleaning that up first",
		               loggederror);


		lwgeom_out = lwgeom_make_geos_friendly(lwgeom_out);
		if ( ! lwgeom_out )
		{
			lwerror("Could not make a valid geometry out of input");
		}

		/* try again as we did cleanup now */
		geosgeom = LWGEOM2GEOS(lwgeom_out);
		if ( ! geosgeom )
		{
			lwerror("Couldn't convert POSTGIS geom to GEOS: %s",
			        loggederror);
			PG_RETURN_NULL();
		}

	}
	else
	{
		POSTGIS_DEBUG(4, "original geom converted to GEOS");
		lwgeom_out = lwgeom_in;
	}


	/*
	 * Step 2 : return the resulting geometry if it's now valid
	 */

	ret_char = GEOSisValid(geosgeom);
	if ( ret_char == 2 )
	{
		GEOSGeom_destroy(geosgeom);
		geosgeom=0;

		PG_FREE_IF_COPY(in, 0);

		lwerror("GEOSisValid() threw an error: %s", loggederror);
		PG_RETURN_NULL(); /* I don't think should ever happen */
	}
	else if ( ret_char )
	{
		/* It's valid at this step, return what we have */

		GEOSGeom_destroy(geosgeom);
		geosgeom=0;

		out = pglwgeom_serialize(lwgeom_out);
		lwgeom_release(lwgeom_out);
		lwgeom_out=0;

		PG_FREE_IF_COPY(in, 0);

		PG_RETURN_POINTER(out);
	}

	POSTGIS_DEBUGF(3,
	               "Geometry [%s] is still not valid:",
	               lwgeom_to_ewkt(lwgeom_out, PARSER_CHECK_NONE));
	POSTGIS_DEBUGF(3, " %s", loggederror);
	POSTGIS_DEBUG(3, " will try to clean up further");

	/*
	 * Step 3 : make what we got now (geosgeom) valid
	 */

	switch (GEOSGeomTypeId(geosgeom))
	{
	case GEOS_MULTIPOINT:
	case GEOS_POINT:
		/* points are always valid, but we might have invalid ordinate values */
		lwnotice("PUNTUAL geometry resulted invalid to GEOS -- dunno how to clean that up");
		PG_RETURN_NULL();
		break;

	case GEOS_LINESTRING:
	case GEOS_MULTILINESTRING:
		lwnotice("LINEAL geometries resulted invalid to GEOS -- dunno how to clean that up");
		PG_RETURN_NULL();
		break;

	case GEOS_POLYGON:
	case GEOS_MULTIPOLYGON:
	{
		GEOSGeom tmp = LWGEOM_GEOS_makeValidPolygon(geosgeom);
		if ( ! tmp )  /* an exception or something */
		{
			/* cleanup and throw */
			GEOSGeom_destroy(geosgeom);
			lwgeom_release(lwgeom_in);
			if ( lwgeom_in != lwgeom_out )
			{
				lwgeom_release(lwgeom_out);
			}
			PG_FREE_IF_COPY(in, 0);
			lwerror("%s", loggederror);
			PG_RETURN_NULL(); /* never get here */
		}
		GEOSGeom_destroy(geosgeom); /* input one */
		geosgeom = tmp;
		break; /* we've done (geosgeom is the output) */
	}

	default:
	{
		char* tmp = GEOSGeomType(geosgeom);
		char* typname = pstrdup(tmp);
		GEOSFree(tmp);
		lwnotice("ST_MakeValid: doesn't support geometry type: %s",
		         typname);
		PG_RETURN_NULL();
		break;
	}
	}


	if ( ! geosgeom ) PG_RETURN_NULL();

	/* Now check if every point of input is also found
	 * in output, or abort by returning NULL
	 *
	 * Input geometry was lwgeom_in
	 */

	out = GEOS2POSTGIS(geosgeom, is3d);
	GEOSGeom_destroy(geosgeom);
	geosgeom=0;

	PG_RETURN_POINTER(out);
#endif /* POSTGIS_GEOS_VERSION >= 33 */
}
