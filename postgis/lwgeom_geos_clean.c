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
 * Attempts to make an invalid geometries valid w/out losing
 * points.
 *
 * Polygons may become lines or points or a collection of
 * polygons lines and points (collapsed ring cases).
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
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
#include "liblwgeom_internal.h"
#include "funcapi.h"

#include <string.h>
#include <assert.h>

/* #define POSTGIS_DEBUG_LEVEL 4 */


/*
 * Return Nth vertex in GEOSGeometry as a POINT.
 * May return NULL if the geometry has NO vertexex.
 */
GEOSGeometry* LWGEOM_GEOS_getPointN(const GEOSGeometry*, uint32);
GEOSGeometry*
LWGEOM_GEOS_getPointN(const GEOSGeometry* g_in, uint32 n)
{
	uint32 dims;
	const GEOSCoordSequence* seq_in;
	GEOSCoordSeq seq_out;
	double val;
	uint32 sz;
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
	LWDEBUGF(2, "lwgeom_make_geos_friendly enter (type %d)", geom->type);
	switch (geom->type)
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
		lwerror("lwgeom_make_geos_friendly: unsupported input geometry type: %s (%d)", lwtype_name(geom->type), geom->type);
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
		                           FLAGS_NDIMS(ring->flags),
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
		                        FLAGS_NDIMS(ring->flags),
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
		                                FLAGS_NDIMS(line->points->flags),
		                                line->points->npoints);
		ret = (LWGEOM*)line;
#else
		/* Turn into a point */
		ret = (LWGEOM*)lwpoint_construct(line->srid, 0, line->points);
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
LWGEOM_GEOS_nodeLines(const GEOSGeometry* lines)
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

#if POSTGIS_GEOS_VERSION >= 33
/*
 * We expect initGEOS being called already.
 * Will return NULL on error (expect error handler being called by then)
 *
 */
static GEOSGeometry*
LWGEOM_GEOS_makeValidPolygon(const GEOSGeometry* gin)
{
	GEOSGeom gout;
	GEOSGeom geos_bound;
	GEOSGeom geos_cut_edges, geos_area, collapse_points;
	GEOSGeometry *vgeoms[3]; /* One for area, one for cut-edges */
	unsigned int nvgeoms=0;

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

	/* Use noded boundaries as initial "cut" edges */


	geos_cut_edges = LWGEOM_GEOS_nodeLines(geos_bound);
	if ( NULL == geos_cut_edges )
	{
		GEOSGeom_destroy(geos_bound);
		lwnotice("LWGEOM_GEOS_nodeLines(): %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* NOTE: the noding process may drop lines collapsing to points.
	 *       We want to retrive any of those */
	{
		GEOSGeometry* pi;
		GEOSGeometry* po;

		pi = GEOSGeom_extractUniquePoints(geos_bound);
		if ( NULL == pi )
		{
			GEOSGeom_destroy(geos_bound);
			lwnotice("GEOSGeom_extractUniquePoints(): %s",
			         lwgeom_geos_errmsg);
			return NULL;
		}

		POSTGIS_DEBUGF(3,
		               "Boundaries input points %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(pi, 0),
		                              PARSER_CHECK_NONE));

		po = GEOSGeom_extractUniquePoints(geos_cut_edges);
		if ( NULL == po )
		{
			GEOSGeom_destroy(geos_bound);
			GEOSGeom_destroy(pi);
			lwnotice("GEOSGeom_extractUniquePoints(): %s",
			         lwgeom_geos_errmsg);
			return NULL;
		}

		POSTGIS_DEBUGF(3,
		               "Boundaries output points %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(po, 0),
		                              PARSER_CHECK_NONE));

		collapse_points = GEOSDifference(pi, po);
		if ( NULL == collapse_points )
		{
			GEOSGeom_destroy(geos_bound);
			GEOSGeom_destroy(pi);
			GEOSGeom_destroy(po);
			lwnotice("GEOSDifference(): %s", lwgeom_geos_errmsg);
			return NULL;
		}

		POSTGIS_DEBUGF(3,
		               "Collapse points: %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(collapse_points, 0),
		                              PARSER_CHECK_NONE));

		GEOSGeom_destroy(pi);
		GEOSGeom_destroy(po);
	}
	GEOSGeom_destroy(geos_bound);

	POSTGIS_DEBUGF(3,
	               "Noded Boundaries: %s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_cut_edges, 0),
	                              PARSER_CHECK_NONE));

	/* And use an empty geometry as initial "area" */
	geos_area = GEOSGeom_createEmptyPolygon();
	if ( ! geos_area )
	{
		lwnotice("GEOSGeom_createEmptyPolygon(): %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(geos_cut_edges);
		return NULL;
	}


	/*
	 * See if an area can be build with the remaining edges
	 * and if it can, symdifference with the original area.
	 * Iterate this until no more polygons can be created
	 * with left-over edges.
	 */
	while (GEOSGetNumGeometries(geos_cut_edges))
	{
		GEOSGeometry* new_area=0;
		GEOSGeometry* new_area_bound=0;
		GEOSGeometry* symdif=0;
		GEOSGeometry* new_cut_edges=0;

		/*
		 * ASSUMPTION: cut_edges should already be fully noded
		 */

		new_area = LWGEOM_GEOS_buildArea(geos_cut_edges);
		if ( ! new_area )   /* must be an exception */
		{
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(geos_area);
			lwnotice("LWGEOM_GEOS_buildArea() threw an error: %s",
			         lwgeom_geos_errmsg);
			return NULL;
		}

		if ( GEOSisEmpty(new_area) )
		{
			/* no more rings can be build with thes edges */
			GEOSGeom_destroy(new_area);
			break;
		}

		/*
		 * We succeeded in building a ring !
		 */


		/*
		 * Save the new ring boundaries first (to compute
		 * further cut edges later)
		 */
		new_area_bound = GEOSBoundary(new_area);
		if ( ! new_area_bound )
		{
			/* We did check for empty area already so
			 * this must be some other error */
			lwnotice("GEOSBoundary('%s') threw an error: %s",
			         lwgeom_to_ewkt(GEOS2LWGEOM(new_area, 0),
			                        PARSER_CHECK_NONE),
			         lwgeom_geos_errmsg);
			GEOSGeom_destroy(new_area);
			GEOSGeom_destroy(geos_area);
			return NULL;
		}

		/*
		 * Now symdif new and old area
		 */
		symdif = GEOSSymDifference(geos_area, new_area);
		if ( ! symdif )   /* must be an exception */
		{
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(new_area);
			GEOSGeom_destroy(new_area_bound);
			GEOSGeom_destroy(geos_area);
			lwnotice("GEOSSymDifference() threw an error: %s",
			         lwgeom_geos_errmsg);
			return NULL;
		}

		GEOSGeom_destroy(geos_area);
		GEOSGeom_destroy(new_area);
		geos_area = symdif;
		symdif = 0;

		/*
		 * Now let's re-set geos_cut_edges with what's left
		 * from the original boundary.
		 * ASSUMPTION: only the previous cut-edges can be
		 *             left, so we don't need to reconsider
		 *             the whole original boundaries
		 */

		new_cut_edges = GEOSDifference(geos_cut_edges, new_area_bound);
		GEOSGeom_destroy(new_area_bound);
		if ( ! new_cut_edges )   /* an exception ? */
		{
			/* cleanup and throw */
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(geos_area);
			lwnotice("GEOSDifference() threw an error: %s",
			         lwgeom_geos_errmsg);
			return NULL;
		}
		GEOSGeom_destroy(geos_cut_edges);
		geos_cut_edges = new_cut_edges;
	}

	if ( ! GEOSisEmpty(geos_area) )
	{
		vgeoms[nvgeoms++] = geos_area;
	}
	else
	{
		GEOSGeom_destroy(geos_area);
	}

	if ( ! GEOSisEmpty(geos_cut_edges) )
	{
		vgeoms[nvgeoms++] = geos_cut_edges;
	}
	else
	{
		GEOSGeom_destroy(geos_cut_edges);
	}

	if ( ! GEOSisEmpty(collapse_points) )
	{
		vgeoms[nvgeoms++] = collapse_points;
	}
	else
	{
		GEOSGeom_destroy(collapse_points);
	}

	if ( 1 == nvgeoms )
	{
		/* Return cut edges */
		gout = vgeoms[0];
	}
	else
	{
		/* Collect areas and lines (if any line) */
		gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, vgeoms, nvgeoms);
		if ( ! gout )   /* an exception again */
		{
			/* cleanup and throw */
			lwnotice("GEOSGeom_createCollection() threw an error: %s",
			         lwgeom_geos_errmsg);
			/* TODO: cleanup! */
			return NULL;
		}
	}

	return gout;

}

static GEOSGeometry*
LWGEOM_GEOS_makeValidLine(const GEOSGeometry* gin)
{
	GEOSGeometry* noded;
	noded = LWGEOM_GEOS_nodeLines(gin);
	return noded;
}

static GEOSGeometry*
LWGEOM_GEOS_makeValidMultiLine(const GEOSGeometry* gin)
{
	GEOSGeometry** lines;
	GEOSGeometry** points;
	GEOSGeometry* mline_out=0;
	GEOSGeometry* mpoint_out=0;
	GEOSGeometry* gout=0;
	uint32 nlines=0, nlines_alloc;
	uint32 npoints=0;
	uint32 ngeoms=0, nsubgeoms;
	uint32 i, j;

	ngeoms = GEOSGetNumGeometries(gin);

	nlines_alloc = ngeoms;
	lines = lwalloc(sizeof(GEOSGeometry*)*nlines_alloc);
	points = lwalloc(sizeof(GEOSGeometry*)*ngeoms);

	for (i=0; i<ngeoms; ++i)
	{
		const GEOSGeometry* g = GEOSGetGeometryN(gin, i);
		GEOSGeometry* vg;
		vg = LWGEOM_GEOS_makeValidLine(g);
		if ( GEOSisEmpty(vg) )
		{
			/* we don't care about this one */
			GEOSGeom_destroy(vg);
		}
		if ( GEOSGeomTypeId(vg) == GEOS_POINT )
		{
			points[npoints++] = vg;
		}
		else if ( GEOSGeomTypeId(vg) == GEOS_LINESTRING )
		{
			lines[nlines++] = vg;
		}
		else if ( GEOSGeomTypeId(vg) == GEOS_MULTILINESTRING )
		{
			nsubgeoms=GEOSGetNumGeometries(vg);
			nlines_alloc += nsubgeoms;
			lines = lwrealloc(lines, sizeof(GEOSGeometry*)*nlines_alloc);
			for (j=0; j<nsubgeoms; ++j)
			{
				const GEOSGeometry* gc = GEOSGetGeometryN(vg, j);
				/* NOTE: ownership of the cloned geoms will be
				 *       taken by final collection */
				lines[nlines++] = GEOSGeom_clone(gc);
			}
		}
		else
		{
			/* NOTE: return from GEOSGeomType will leak
			 * but we really don't expect this to happen */
			lwerror("unexpected geom type returned "
			        "by LWGEOM_GEOS_makeValid: %s",
			        GEOSGeomType(vg));
		}
	}

	if ( npoints )
	{
		if ( npoints > 1 )
		{
			mpoint_out = GEOSGeom_createCollection(GEOS_MULTIPOINT,
			                                       points, npoints);
		}
		else
		{
			mpoint_out = points[0];
		}
	}

	if ( nlines )
	{
		if ( nlines > 1 )
		{
			mline_out = GEOSGeom_createCollection(
			                GEOS_MULTILINESTRING, lines, nlines);
		}
		else
		{
			mline_out = lines[0];
		}
	}

	lwfree(lines);

	if ( mline_out && mpoint_out )
	{
		points[0] = mline_out;
		points[1] = mpoint_out;
		gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION,
		                                 points, 2);
	}
	else if ( mline_out )
	{
		gout = mline_out;
	}
	else if ( mpoint_out )
	{
		gout = mpoint_out;
	}

	lwfree(points);

	return gout;
}


static GEOSGeometry*
LWGEOM_GEOS_makeValid(const GEOSGeometry* gin)
{
	GEOSGeometry* gout;
	char ret_char;

	/*
	 * Step 2: return what we got so far if already valid
	 */

	ret_char = GEOSisValid(gin);
	if ( ret_char == 2 )
	{
		/* I don't think should ever happen */
		lwerror("GEOSisValid(): %s", lwgeom_geos_errmsg);
		return NULL;
	}
	else if ( ret_char )
	{
		POSTGIS_DEBUGF(3,
		               "Geometry [%s] is valid. ",
		               lwgeom_to_ewkt(GEOS2LWGEOM(gin, 0),
		                              PARSER_CHECK_NONE));

		/* It's valid at this step, return what we have */
		return GEOSGeom_clone(gin);
	}

	POSTGIS_DEBUGF(3,
	               "Geometry [%s] is still not valid: %s. "
	               "Will try to clean up further.",
	               lwgeom_to_ewkt(GEOS2LWGEOM(gin, 0),
	                              PARSER_CHECK_NONE), lwgeom_geos_errmsg);



	/*
	 * Step 3 : make what we got valid
	 */

	switch (GEOSGeomTypeId(gin))
	{
	case GEOS_MULTIPOINT:
	case GEOS_POINT:
		/* points are always valid, but we might have invalid ordinate values */
		lwnotice("PUNTUAL geometry resulted invalid to GEOS -- dunno how to clean that up");
		return NULL;
		break;

	case GEOS_LINESTRING:
		gout = LWGEOM_GEOS_makeValidLine(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			lwerror("%s", lwgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */

	case GEOS_MULTILINESTRING:
		gout = LWGEOM_GEOS_makeValidMultiLine(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			lwerror("%s", lwgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */

	case GEOS_POLYGON:
	case GEOS_MULTIPOLYGON:
	{
		gout = LWGEOM_GEOS_makeValidPolygon(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			lwerror("%s", lwgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */
	}

	default:
	{
		char* tmp = GEOSGeomType(gin);
		char* typname = pstrdup(tmp);
		GEOSFree(tmp);
		lwnotice("ST_MakeValid: doesn't support geometry type: %s",
		         typname);
		return NULL;
		break;
	}
	}

	/*
	 * Now check if every point of input is also found
	 * in output, or abort by returning NULL
	 *
	 * Input geometry was lwgeom_in
	 */
	{
		const int paranoia = 2;
		/* TODO: check if the result is valid */
		if (paranoia)
		{
			int loss;
			GEOSGeometry *pi, *po, *pd;

			/* TODO: handle some errors here...
			 * Lack of exceptions is annoying indeed,
			 * I'm getting old --strk;
			 */
			pi = GEOSGeom_extractUniquePoints(gin);
			po = GEOSGeom_extractUniquePoints(gout);
			pd = GEOSDifference(pi, po); /* input points - output points */
			GEOSGeom_destroy(pi);
			GEOSGeom_destroy(po);
			loss = !GEOSisEmpty(pd);
			GEOSGeom_destroy(pd);
			if ( loss )
			{
				lwnotice("Vertices lost in LWGEOM_GEOS_makeValid");
				/* return NULL */
			}
		}
	}


	return gout;
}

/* Uses GEOS internally */
static LWGEOM* lwgeom_make_valid(LWGEOM* lwgeom_in);
static LWGEOM*
lwgeom_make_valid(LWGEOM* lwgeom_in)
{
	int is3d;
	GEOSGeom geosgeom;
	GEOSGeometry* geosout;
	LWGEOM *lwgeom_out;

	is3d = FLAGS_GET_Z(lwgeom_in->type);

	/*
	 * Step 1 : try to convert to GEOS, if impossible, clean that up first
	 *          otherwise (adding only duplicates of existing points)
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	lwgeom_out = lwgeom_in;
	geosgeom = LWGEOM2GEOS(lwgeom_out);
	if ( ! geosgeom )
	{
		POSTGIS_DEBUGF(4,
		               "Original geom can't be converted to GEOS (%s)"
		               " - will try cleaning that up first",
		               lwgeom_geos_errmsg);


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
			        lwgeom_geos_errmsg);
			return NULL;
		}

	}
	else
	{
		POSTGIS_DEBUG(4, "original geom converted to GEOS");
		lwgeom_out = lwgeom_in;
	}

	geosout = LWGEOM_GEOS_makeValid(geosgeom);
	GEOSGeom_destroy(geosgeom);
	if ( ! geosout )
	{
		return NULL;
	}

	lwgeom_out = GEOS2LWGEOM(geosout, is3d);
	if ( lwgeom_is_collection(lwgeom_in) && ! lwgeom_is_collection(lwgeom_out) )
	{
		POSTGIS_DEBUG(3, "lwgeom_make_valid: forcing multi");
		lwgeom_out = lwgeom_as_multi(lwgeom_out);
	}

	GEOSGeom_destroy(geosout);

	lwgeom_out->srid = lwgeom_in->srid;
	return lwgeom_out;
}

/* Uses GEOS internally */
static LWGEOM* lwgeom_clean(LWGEOM* lwgeom_in);
static LWGEOM*
lwgeom_clean(LWGEOM* lwgeom_in)
{
	LWGEOM* lwgeom_out;

	lwgeom_out = lwgeom_make_valid(lwgeom_in);
	if ( ! lwgeom_out )
	{
		return NULL;
	}

	/* Check dimensionality is the same as input */
	if ( lwgeom_dimensionality(lwgeom_in) != lwgeom_dimensionality(lwgeom_out) )
	{
		lwnotice("lwgeom_clean: dimensional collapse (%d to %d)",
		         lwgeom_dimensionality(lwgeom_in), lwgeom_dimensionality(lwgeom_out));

		return NULL;
	}

	/* Check that the output is not a collection if the input wasn't */
	if ( lwgeom_out->type == COLLECTIONTYPE &&
	        lwgeom_in->type != COLLECTIONTYPE )
	{
		lwnotice("lwgeom_clean: mixed-type output (%s) "
		         "from single-type input (%s)",
		         lwtype_name(lwgeom_out->type),
		         lwtype_name(lwgeom_in->type));
		return NULL;
	}

	/* Force right-hand-rule (will only affect polygons) */
	/* gout := ST_ForceRHR(gout); */

	/* Remove repeated duplicated points ? */
	/* gout = ST_RemoveRepeatedPoints(gout); */

	return lwgeom_out;
}

#endif /* POSTGIS_GEOS_VERSION >= 33 */

Datum ST_MakeValid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_MakeValid);
Datum ST_MakeValid(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 33
	elog(ERROR, "You need GEOS-3.3.0 or up for ST_MakeValid");
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */

	PG_LWGEOM *in, *out;
	LWGEOM *lwgeom_in, *lwgeom_out;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom_in = pglwgeom_deserialize(in);

	switch ( lwgeom_in->type )
	{
	case LINETYPE:
	case POLYGONTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		break;

	default:
		lwerror("ST_MakeValid: unsupported geometry type %s",
		        lwtype_name(lwgeom_in->type));
		PG_RETURN_NULL();
		break;
	}

	lwgeom_out = lwgeom_make_valid(lwgeom_in);
	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_NULL();
	}

	out = pglwgeom_serialize(lwgeom_out);

	PG_RETURN_POINTER(out);
#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

Datum ST_CleanGeometry(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CleanGeometry);
Datum ST_CleanGeometry(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 33
	elog(ERROR, "You need GEOS-3.3.0 or up for ST_CleanGeometry");
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */

	PG_LWGEOM *in, *out;
	LWGEOM *lwgeom_in, *lwgeom_out;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom_in = pglwgeom_deserialize(in);

	/* Short-circuit: empty geometry are the cleanest ! */
#if 0
	if ( lwgeom_is_empty(lwgeom_in) )
	{
		out = pglwgeom_serialize(lwgeom_in);
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_POINTER(out);
	}
#endif

	lwgeom_out = lwgeom_clean(lwgeom_in);
	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(in, 0);
		PG_RETURN_NULL();
	}

	out = pglwgeom_serialize(lwgeom_out);
	PG_RETURN_POINTER(out);

#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

