/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* basic LWPOLY manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"


#define CHECK_POLY_RINGS_ZM 1

/* construct a new LWPOLY.  arrays (points/points per ring) will NOT be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWPOLY*
lwpoly_construct(int srid, GBOX *bbox, uint32 nrings, POINTARRAY **points)
{
	LWPOLY *result;
	int hasz, hasm;
#ifdef CHECK_POLY_RINGS_ZM
	char zm;
	uint32 i;
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


/*
 * given the LWPOLY serialized form (or a pointer into a muli* one)
 * construct a proper LWPOLY.
 * serialized_form should point to the 8bit type format (with type = 3)
 * See serialized form doc
 */
LWPOLY *
lwpoly_deserialize(uchar *serialized_form)
{

	LWPOLY *result;
	uint32 nrings;
	int ndims, hasz, hasm;
	uint32 npoints;
	uchar type;
	uchar *loc;
	int t;

	if (serialized_form == NULL)
	{
		lwerror("lwpoly_deserialize called with NULL arg");
		return NULL;
	}

	type = serialized_form[0];
	ndims = TYPE_NDIMS(type);
	hasz = TYPE_HASZ(type);
	hasm = TYPE_HASM(type);

	result = (LWPOLY*) lwalloc(sizeof(LWPOLY));
	result->type = TYPE_GETTYPE(type);
	result->flags = gflags(hasz, hasm, 0);

	loc = serialized_form;

	if ( TYPE_GETTYPE(type) != POLYGONTYPE)
	{
		lwerror("lwpoly_deserialize: attempt to deserialize a poly which is really a %s", lwtype_name(type));
		return NULL;
	}


	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		BOX2DFLOAT4 *box2df;

		LWDEBUG(3, "lwpoly_deserialize: input has bbox");

		FLAGS_SET_BBOX(result->flags, 1);
		box2df = lwalloc(sizeof(BOX2DFLOAT4));
                memcpy(box2df, loc, sizeof(BOX2DFLOAT4));
                result->bbox = gbox_from_box2df(result->flags, box2df);
                lwfree(box2df);
		loc += sizeof(BOX2DFLOAT4);
	}
	else
	{
		result->bbox = NULL;
	}

	if ( lwgeom_hasSRID(type))
	{
		result->srid = lw_get_int32(loc);
		loc +=4; /* type + SRID */
	}
	else
	{
		result->srid = SRID_UNKNOWN;
	}

	nrings = lw_get_uint32(loc);
	result->nrings = nrings;
	result->maxrings = nrings;
	loc +=4;
	if ( nrings )
	{
		result->rings = (POINTARRAY**) lwalloc(nrings* sizeof(POINTARRAY*));
	}
	else
	{
		result->rings = NULL;
	}

	for (t =0; t<nrings; t++)
	{
		/* read in a single ring and make a PA */
		npoints = lw_get_uint32(loc);
		loc +=4;
		result->rings[t] = ptarray_construct_reference_data(hasz, hasm, npoints, loc);
		loc += sizeof(double)*ndims*npoints;
	}

	return result;
}

/*
 * create the serialized form of the polygon
 * result's first char will be the 8bit type.  See serialized form doc
 * points copied
 */
uchar *
lwpoly_serialize(LWPOLY *poly)
{
	size_t size, retsize;
	uchar *result;

	size = lwpoly_serialize_size(poly);
	result = lwalloc(size);
	lwpoly_serialize_buf(poly, result, &retsize);

	if ( retsize != size )
	{
		lwerror("lwpoly_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	}

	return result;
}

/*
 * create the serialized form of the polygon writing it into the
 * given buffer, and returning number of bytes written into
 * the given int pointer.
 * result's first char will be the 8bit type.  See serialized form doc
 * points copied
 */
void
lwpoly_serialize_buf(LWPOLY *poly, uchar *buf, size_t *retsize)
{
	size_t size=1;  /* type byte */
	char has_srid;
	int t;
	uchar *loc;
	int ptsize;

	LWDEBUG(2, "lwpoly_serialize_buf called");

	ptsize = sizeof(double)*FLAGS_NDIMS(poly->flags);

	has_srid = (poly->srid != SRID_UNKNOWN);

	size += 4; /* nrings */
	size += 4*poly->nrings; /* npoints/ring */

	buf[0] = (uchar) lwgeom_makeType_full(
	             FLAGS_GET_Z(poly->flags), FLAGS_GET_M(poly->flags),
	             has_srid, POLYGONTYPE, poly->bbox ? 1 : 0);
	loc = buf+1;

	if (poly->bbox)
	{
		BOX2DFLOAT4 *box2df;
		
		box2df = box2df_from_gbox(poly->bbox);
		memcpy(loc, box2df, sizeof(BOX2DFLOAT4));
		lwfree(box2df);
		size += sizeof(BOX2DFLOAT4); /* bvol */
		loc += sizeof(BOX2DFLOAT4);
	}

	if (has_srid)
	{
		memcpy(loc, &poly->srid, sizeof(int32));
		loc += 4;
		size +=4;  /* 4 byte SRID */
	}

	memcpy(loc, &poly->nrings, sizeof(int32));  /* nrings */
	loc+=4;

	for (t=0; t<poly->nrings; t++)
	{
		POINTARRAY *pa = poly->rings[t];
		size_t pasize;
		uint32 npoints;

		LWDEBUGF(4, "FLAGS_GET_ZM(poly->type) == %d", FLAGS_GET_ZM(poly->flags));
		LWDEBUGF(4, "FLAGS_GET_ZM(pa->flags) == %d", FLAGS_GET_ZM(pa->flags));

		if ( FLAGS_GET_ZM(poly->flags) != FLAGS_GET_ZM(pa->flags) )
			lwerror("Dimensions mismatch in lwpoly");

		npoints = pa->npoints;

		memcpy(loc, &npoints, sizeof(uint32)); /* npoints this ring */
		loc+=4;

		pasize = npoints*ptsize;
		size += pasize;

		/* copy points */
		memcpy(loc, getPoint_internal(pa, 0), pasize);
		loc += pasize;

	}

	if (retsize) *retsize = size;
}


/* find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN) */
BOX3D *
lwpoly_compute_box3d(LWPOLY *poly)
{
	BOX3D *result;

	/* just need to check outer ring -- interior rings are inside */
	POINTARRAY *pa = poly->rings[0];
	result  = ptarray_compute_box3d(pa);

	return result;
}


/* find length of this serialized polygon */
size_t
lwgeom_size_poly(const uchar *serialized_poly)
{
	uint32 result = 1; /* char type */
	uint32 nrings;
	int ndims;
	int t;
	uchar type;
	uint32 npoints;
	const uchar *loc;

	if (serialized_poly == NULL)
		return -9999;


	type = (uchar) serialized_poly[0];
	ndims = TYPE_NDIMS(type);

	if ( lwgeom_getType(type) != POLYGONTYPE)
		return -9999;


	loc = serialized_poly+1;

	if (lwgeom_hasBBOX(type))
	{
		LWDEBUG(3, "lwgeom_size_poly: has bbox");

		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}


	if ( lwgeom_hasSRID(type))
	{
		LWDEBUG(3, "lwgeom_size_poly: has srid");

		loc +=4; /* type + SRID */
		result += 4;
	}


	nrings = lw_get_uint32(loc);
	loc +=4;
	result +=4;

	LWDEBUGF(3, "lwgeom_size_poly contains %d rings", nrings);

	for (t =0; t<nrings; t++)
	{
		/* read in a single ring and make a PA */
		npoints = lw_get_uint32(loc);
		loc += 4;
		result += 4;

		if (ndims == 3)
		{
			loc += 24*npoints;
			result += 24*npoints;
		}
		else if (ndims == 2)
		{
			loc += 16*npoints;
			result += 16*npoints;
		}
		else if (ndims == 4)
		{
			loc += 32*npoints;
			result += 32*npoints;
		}
	}

	LWDEBUGF(3, "lwgeom_size_poly returning %d", result);

	return result;
}

/* find length of this deserialized polygon */
size_t
lwpoly_serialize_size(LWPOLY *poly)
{
	size_t size = 1; /* type */
	uint32 i;

	if ( poly->srid != SRID_UNKNOWN ) size += 4; /* SRID */
	if ( poly->bbox ) size += sizeof(BOX2DFLOAT4);

	LWDEBUGF(2, "lwpoly_serialize_size called with poly[%p] (%d rings)",
	         poly, poly->nrings);

	size += 4; /* nrings */

	for (i=0; i<poly->nrings; i++)
	{
		size += 4; /* npoints */
		size += poly->rings[i]->npoints*FLAGS_NDIMS(poly->flags)*sizeof(double);
	}

	LWDEBUGF(3, "lwpoly_serialize_size returning %d", size);

	return size;
}

void lwpoly_free(LWPOLY  *poly)
{
	int t;

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

int
lwpoly_compute_box2d_p(const LWPOLY *poly, BOX2DFLOAT4 *box)
{
	BOX2DFLOAT4 boxbuf;
	uint32 i;

	if ( ! poly->nrings ) return 0;
	if ( ! ptarray_compute_box2d_p(poly->rings[0], box) ) return 0;
	for (i=1; i<poly->nrings; i++)
	{
		if ( ! ptarray_compute_box2d_p(poly->rings[0], &boxbuf) )
			return 0;
		if ( ! box2d_union_p(box, &boxbuf, box) )
			return 0;
	}
	return 1;
}

/* Clone LWLINE object. POINTARRAY are not copied, it's ring array is. */
LWPOLY *
lwpoly_clone(const LWPOLY *g)
{
	LWPOLY *ret = lwalloc(sizeof(LWPOLY));
	memcpy(ret, g, sizeof(LWPOLY));
	ret->rings = lwalloc(sizeof(POINTARRAY *)*g->nrings);
	memcpy(ret->rings, g->rings, sizeof(POINTARRAY *)*g->nrings);
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
		ret->rings[i] = ptarray_clone(g->rings[i]);
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
	uint32 i;

	newrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
	for (i=0; i<poly->nrings; i++)
	{
		newrings[i] = ptarray_segmentize2d(poly->rings[i], dist);
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
	uint32 i;

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
                    uint32 nholes, const LWLINE **holes)
{
	uint32 nrings;
	POINTARRAY **rings = lwalloc((nholes+1)*sizeof(POINTARRAY *));
	int srid = shell->srid;
	LWPOLY *ret;

	if ( shell->points->npoints < 4 )
		lwerror("lwpoly_from_lwlines: shell must have at least 4 points");
	if ( ! ptarray_isclosed2d(shell->points) )
		lwerror("lwpoly_from_lwlines: shell must be closed");
	rings[0] = ptarray_clone(shell->points);

	for (nrings=1; nrings<=nholes; nrings++)
	{
		const LWLINE *hole = holes[nrings-1];

		if ( hole->srid != srid )
			lwerror("lwpoly_from_lwlines: mixed SRIDs in input lines");

		if ( hole->points->npoints < 4 )
			lwerror("lwpoly_from_lwlines: holes must have at least 4 points");
		if ( ! ptarray_isclosed2d(hole->points) )
			lwerror("lwpoly_from_lwlines: holes must be closed");

		rings[nrings] = ptarray_clone(hole->points);
	}

	ret = lwpoly_construct(srid, NULL, nrings, rings);
	return ret;
}

LWGEOM*
lwpoly_remove_repeated_points(LWPOLY *poly)
{
	uint32 i;
	POINTARRAY **newrings;

	newrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
	for (i=0; i<poly->nrings; i++)
	{
		newrings[i] = ptarray_remove_repeated_points(poly->rings[i]);
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
	if ( (poly->nrings == 0) || (!poly->rings) )
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

LWPOLY* lwpoly_simplify(const LWPOLY *ipoly, double dist)
{
	int i;
	LWPOLY *opoly = lwpoly_construct_empty(ipoly->srid, FLAGS_GET_Z(ipoly->flags), FLAGS_GET_M(ipoly->flags));

	LWDEBUGF(2, "simplify_polygon3d: simplifying polygon with %d rings", ipoly->nrings);

	if( lwpoly_is_empty(ipoly) )
		return opoly;

	for (i = 0; i < ipoly->nrings; i++)
	{
		POINTARRAY *opts = ptarray_simplify(ipoly->rings[i], dist);

		/* One point implies an error in the ptarray_simplify */
		if ( opts->npoints < 2 )
		{
			lwnotice("ptarray_simplify returned a <2 pts array");
			ptarray_free(opts);
			continue;
		}

		/* Less points than are needed to form a closed ring, we can't use this */
		if ( opts->npoints < 4 )
		{
			LWDEBUGF(3, "ring%d skipped (<4 pts)", i);
			ptarray_free(opts);
			if ( i ) continue;
			else break;
		}

		LWDEBUGF(3, "ring%d simplified from %d to %d points", i, ipoly->rings[i]->npoints, opts->npoints);

		/* Add ring to simplified polygon */
		if( lwpoly_add_ring(opoly, opts) == LW_FAILURE )
			return NULL;
	}

	LWDEBUGF(3, "simplified polygon with %d rings", ipoly->nrings);
	opoly->type = ipoly->type;
	return opoly;
}

/**
 * Find the area of the outer ring - sum (area of inner rings).
 * Could use a more numerically stable calculator...
 */
double
lwpoly_area(const LWPOLY *poly)
{
	double poly_area=0.0;
	int i;
	POINT2D p1;
	POINT2D p2;

	LWDEBUGF(2, "in lwpoly_area (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
	{
		int j;
		POINTARRAY *ring = poly->rings[i];
		double ringarea = 0.0;

		LWDEBUGF(4, " rings %d has %d points", i, ring->npoints);

		if ( ! ring->npoints ) continue; /* empty ring */
		for (j=0; j<ring->npoints-1; j++)
		{
			getPoint2d_p(ring, j, &p1);
			getPoint2d_p(ring, j+1, &p2);
			ringarea += ( p1.x * p2.y ) - ( p1.y * p2.x );
		}

		ringarea  /= 2.0;

		LWDEBUGF(4, " ring 1 has area %lf",ringarea);

		ringarea  = fabs(ringarea);
		if (i != 0)	/*outer */
			ringarea  = -1.0*ringarea ; /* its a hole */

		poly_area += ringarea;
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
