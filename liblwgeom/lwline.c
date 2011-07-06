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

/* basic LWLINE functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"



/*
 * Construct a new LWLINE.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWLINE *
lwline_construct(int srid, GBOX *bbox, POINTARRAY *points)
{
	LWLINE *result;
	result = (LWLINE*) lwalloc(sizeof(LWLINE));

	LWDEBUG(2, "lwline_construct called.");

	result->type = LINETYPE;
	
	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	LWDEBUGF(3, "lwline_construct type=%d", result->type);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

LWLINE *
lwline_construct_empty(int srid, char hasz, char hasm)
{
	LWLINE *result = lwalloc(sizeof(LWLINE));
	result->type = LINETYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}


/*
 * given the LWGEOM serialized form (or a pointer into a muli* one)
 * construct a proper LWLINE.
 * serialized_form should point to the 8bit type format (with type = 2)
 * See serialized form doc
 */
LWLINE *
lwline_deserialize(uchar *serialized_form)
{
	uchar type;
	LWLINE *result;
	uchar *loc =NULL;
	uint32 npoints;
	POINTARRAY *pa;

	type = (uchar) serialized_form[0];

	if ( lwgeom_getType(type) != LINETYPE)
	{
		lwerror("lwline_deserialize: attempt to deserialize a line which is really a %s", lwtype_name(type));
		return NULL;
	}

	result = (LWLINE*) lwalloc(sizeof(LWLINE)) ;
	result->type = LINETYPE;
	result->flags = gflags(TYPE_HASZ(type),TYPE_HASM(type),0);

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		BOX2DFLOAT4 *box2df;

		LWDEBUG(3, "lwline_deserialize: input has bbox");

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
		/*lwnotice("line has NO bbox"); */
	}

	if ( lwgeom_hasSRID(type))
	{
		/*lwnotice("line has srid"); */
		result->srid = lw_get_int32(loc);
		loc +=4; /* type + SRID */
	}
	else
	{
		/*lwnotice("line has NO srid"); */
		result->srid = SRID_UNKNOWN;
	}

	/* we've read the type (1 byte) and SRID (4 bytes, if present) */

	npoints = lw_get_uint32(loc);
	/*lwnotice("line npoints = %d", npoints); */
	loc +=4;
	pa = ptarray_construct_reference_data(TYPE_HASZ(type)?1:0,
				TYPE_HASM(type)?1:0, npoints, loc);
	
	result->points = pa;

	return result;
}

/*
 * convert this line into its serialize form
 * result's first char will be the 8bit type.  See serialized form doc
 */
uchar *
lwline_serialize(LWLINE *line)
{
	size_t size, retsize;
	uchar * result;

	if (line == NULL) lwerror("lwline_serialize:: given null line");

	size = lwline_serialize_size(line);
	result = lwalloc(size);
	lwline_serialize_buf(line, result, &retsize);

	if ( retsize != size )
	{
		lwerror("lwline_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	}

	return result;
}

/*
 * convert this line into its serialize form writing it into
 * the given buffer, and returning number of bytes written into
 * the given int pointer.
 * result's first char will be the 8bit type.  See serialized form doc
 */
void
lwline_serialize_buf(LWLINE *line, uchar *buf, size_t *retsize)
{
	char has_srid;
	uchar *loc;
	int ptsize;
	size_t size;

	LWDEBUGF(2, "lwline_serialize_buf(%p, %p, %p) called",
	         line, buf, retsize);

	if (line == NULL)
		lwerror("lwline_serialize:: given null line");

	if ( FLAGS_GET_ZM(line->flags) != FLAGS_GET_ZM(line->points->flags) )
		lwerror("Dimensions mismatch in lwline");

	ptsize = ptarray_point_size(line->points);

	has_srid = (line->srid != SRID_UNKNOWN);

	buf[0] = (uchar) lwgeom_makeType_full(
	             FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags),
	             has_srid, LINETYPE, line->bbox ? 1 : 0);
	loc = buf+1;

	LWDEBUGF(3, "lwline_serialize_buf added type (%d)", line->type);

	if (line->bbox)
	{
		BOX2DFLOAT4 *box2df;
	
		box2df = box2df_from_gbox(line->bbox);
		memcpy(loc, box2df, sizeof(BOX2DFLOAT4));
		lwfree(box2df);
		loc += sizeof(BOX2DFLOAT4);

		LWDEBUG(3, "lwline_serialize_buf added BBOX");
	}

	if (has_srid)
	{
		memcpy(loc, &line->srid, sizeof(int32));
		loc += sizeof(int32);

		LWDEBUG(3, "lwline_serialize_buf added SRID");
	}

	memcpy(loc, &line->points->npoints, sizeof(uint32));
	loc += sizeof(uint32);

	LWDEBUGF(3, "lwline_serialize_buf added npoints (%d)",
	         line->points->npoints);

	/*copy in points */
	size = line->points->npoints*ptsize;
	memcpy(loc, getPoint_internal(line->points, 0), size);
	loc += size;

	LWDEBUGF(3, "lwline_serialize_buf copied serialized_pointlist (%d bytes)",
	         ptsize * line->points->npoints);

	if (retsize) *retsize = loc-buf;

	/*printBYTES((uchar *)result, loc-buf); */

	LWDEBUGF(3, "lwline_serialize_buf returning (loc: %p, size: %d)",
	         loc, loc-buf);
}

/*
 * Find bounding box (standard one)
 * zmin=zmax=NO_Z_VALUE if 2d
 */
BOX3D *
lwline_compute_box3d(LWLINE *line)
{
	BOX3D *ret;

	if (line == NULL) return NULL;

	ret = ptarray_compute_box3d(line->points);
	return ret;
}


/* find length of this deserialized line */
size_t
lwline_serialize_size(LWLINE *line)
{
	size_t size = 1;  /* type */

	LWDEBUG(2, "lwline_serialize_size called");

	if ( line->srid != SRID_UNKNOWN ) size += 4; /* SRID */
	if ( line->bbox ) size += sizeof(BOX2DFLOAT4);

	size += 4; /* npoints */
	size += ptarray_point_size(line->points)*line->points->npoints;

	LWDEBUGF(3, "lwline_serialize_size returning %d", size);

	return size;
}

void lwline_free (LWLINE  *line)
{
	if ( line->bbox )
		lwfree(line->bbox);
	if ( line->points )
		ptarray_free(line->points);
	lwfree(line);
}

/* find length of this serialized line */
size_t
lwgeom_size_line(const uchar *serialized_line)
{
	int type = (uchar) serialized_line[0];
	uint32 result = 1;  /*type */
	const uchar *loc;
	uint32 npoints;

	LWDEBUG(2, "lwgeom_size_line called");

	if ( lwgeom_getType(type) != LINETYPE)
		lwerror("lwgeom_size_line::attempt to find the length of a non-line");


	loc = serialized_line+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(type))
	{
		loc += 4; /* type + SRID */
		result +=4;
	}

	/* we've read the type (1 byte) and SRID (4 bytes, if present) */
	npoints = lw_get_uint32(loc);
	result += sizeof(uint32); /* npoints */

	result += TYPE_NDIMS(type) * sizeof(double) * npoints;

	LWDEBUGF(3, "lwgeom_size_line returning %d", result);

	return result;
}

void printLWLINE(LWLINE *line)
{
	lwnotice("LWLINE {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(line->flags));
	lwnotice("    srid = %i", (int)line->srid);
	printPA(line->points);
	lwnotice("}");
}

int
lwline_compute_box2d_p(const LWLINE *line, BOX2DFLOAT4 *box)
{
	return ptarray_compute_box2d_p(line->points, box);
}

/* Clone LWLINE object. POINTARRAY is not copied. */
LWLINE *
lwline_clone(const LWLINE *g)
{
	LWLINE *ret = lwalloc(sizeof(LWLINE));

	LWDEBUGF(2, "lwline_clone called with %p", g);

	memcpy(ret, g, sizeof(LWLINE));
	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}

/* Deep clone LWLINE object. POINTARRAY *is* copied. */
LWLINE *
lwline_clone_deep(const LWLINE *g)
{
	LWLINE *ret = lwalloc(sizeof(LWLINE));

	LWDEBUGF(2, "lwline_clone_deep called with %p", g);
	memcpy(ret, g, sizeof(LWLINE));

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	if ( g->points ) ret->points = ptarray_clone_deep(g->points);
	FLAGS_SET_READONLY(ret->flags,0);

	return ret;
}


void
lwline_release(LWLINE *lwline)
{
	lwgeom_release(lwline_as_lwgeom(lwline));
}

void
lwline_reverse(LWLINE *line)
{
	if ( lwline_is_empty(line) ) return;
	ptarray_reverse(line->points);
}

LWLINE *
lwline_segmentize2d(LWLINE *line, double dist)
{
	return lwline_construct(line->srid, NULL,
	                        ptarray_segmentize2d(line->points, dist));
}

/* check coordinate equality  */
char
lwline_same(const LWLINE *l1, const LWLINE *l2)
{
	return ptarray_same(l1->points, l2->points);
}

/*
 * Construct a LWLINE from an array of LWPOINTs
 * LWLINE dimensions are large enough to host all input dimensions.
 */
LWLINE *
lwline_from_lwpointarray(int srid, uint32 npoints, LWPOINT **points)
{
 	int i;
	int hasz = LW_FALSE;
	int hasm = LW_FALSE;
	POINTARRAY *pa;
	LWLINE *line;
	POINT4D pt;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<npoints; i++)
	{
		if ( points[i]->type != POINTTYPE )
		{
			lwerror("lwline_from_lwpointarray: invalid input type: %s", lwtype_name(points[i]->type));
			return NULL;
		}
		if ( FLAGS_GET_Z(points[i]->flags) ) hasz = LW_TRUE;
		if ( FLAGS_GET_M(points[i]->flags) ) hasm = LW_TRUE;
		if ( hasz && hasm ) break; /* Nothing more to learn! */
	}

	pa = ptarray_construct_empty(hasz, hasm, npoints);
	
	for ( i=0; i < npoints; i++ )
	{
		if ( ! lwpoint_is_empty(points[i]) )
		{
			lwpoint_getPoint4d_p(points[i], &pt);
			ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		}
	}

	if ( pa->npoints > 0 )
		line = lwline_construct(srid, NULL, pa);
	else 
		line = lwline_construct_empty(srid, hasz, hasm);
	
	return line;
}

/*
 * Construct a LWLINE from a LWMPOINT
 */
LWLINE *
lwline_from_lwmpoint(int srid, LWMPOINT *mpoint)
{
	uint32 i;
	POINTARRAY *pa;
	char zmflag = FLAGS_GET_ZM(mpoint->flags);
	size_t ptsize, size;
	uchar *newpoints, *ptr;

	if ( zmflag == 0 ) ptsize=2*sizeof(double);
	else if ( zmflag == 3 ) ptsize=4*sizeof(double);
	else ptsize=3*sizeof(double);

	/* Allocate space for output points */
	size = ptsize*mpoint->ngeoms;
	newpoints = lwalloc(size);
	memset(newpoints, 0, size);

	ptr=newpoints;
	for (i=0; i<mpoint->ngeoms; i++)
	{
		memcpy(ptr,
		       getPoint_internal(mpoint->geoms[i]->point, 0),
		       ptsize);
		ptr+=ptsize;
	}

	pa = ptarray_construct_reference_data(zmflag&2, zmflag&1, mpoint->ngeoms, newpoints);
	
	LWDEBUGF(3, "lwline_from_lwmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);

	return lwline_construct(srid, NULL, pa);
}

/**
* Returns freshly allocated #LWPOINT that corresponds to the index where.
* Returns NULL if the geometry is empty or the index invalid.
*/
LWPOINT*
lwline_get_lwpoint(LWLINE *line, int where)
{
	POINT4D pt;
	LWPOINT *lwpoint;
	POINTARRAY *pa;

	if ( lwline_is_empty(line) || where < 0 || where >= line->points->npoints )
		return NULL;

	pa = ptarray_construct_empty(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1);
	pt = getPoint4d(line->points, where);
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
	lwpoint = lwpoint_construct(line->srid, NULL, pa);
	return lwpoint;
}


int
lwline_add_lwpoint(LWLINE *line, LWPOINT *point, int where)
{
	POINT4D pt;	
	getPoint4d_p(point->point, 0, &pt);
	return ptarray_insert_point(line->points, &pt, where);
}



LWLINE *
lwline_removepoint(LWLINE *line, uint32 index)
{
	POINTARRAY *newpa;
	LWLINE *ret;

	newpa = ptarray_removePoint(line->points, index);

	ret = lwline_construct(line->srid, NULL, newpa);
	lwgeom_add_bbox((LWGEOM *) ret);

	return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 */
void
lwline_setPoint4d(LWLINE *line, uint32 index, POINT4D *newpoint)
{
	ptarray_set_point4d(line->points, index, newpoint);
	/* Update the box, if there is one to update */
	if ( line->bbox )
	{
		lwgeom_drop_bbox((LWGEOM*)line);
		lwgeom_add_bbox((LWGEOM*)line);
	}
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
LWLINE*
lwline_measured_from_lwline(const LWLINE *lwline, double m_start, double m_end)
{
	int i = 0;
	int hasm = 0, hasz = 0;
	int npoints = 0;
	double length = 0.0;
	double length_so_far = 0.0;
	double m_range = m_end - m_start;
	double m;
	POINTARRAY *pa = NULL;
	POINT3DZ p1, p2;

	if ( lwline->type != LINETYPE )
	{
		lwerror("lwline_construct_from_lwline: only line types supported");
		return NULL;
	}

	hasz = FLAGS_GET_Z(lwline->flags);
	hasm = 1;

	/* Null points or npoints == 0 will result in empty return geometry */
	if ( lwline->points )
	{
		npoints = lwline->points->npoints;
		length = ptarray_length_2d(lwline->points);
		getPoint3dz_p(lwline->points, 0, &p1);
	}

	pa = ptarray_construct(hasz, hasm, npoints);

	for ( i = 0; i < npoints; i++ )
	{
		POINT4D q;
		POINT2D a, b;
		getPoint3dz_p(lwline->points, i, &p2);
		a.x = p1.x;
		a.y = p1.y;
		b.x = p2.x;
		b.y = p2.y;
		length_so_far += distance2d_pt_pt(&a, &b);
		if ( length > 0.0 )
			m = m_start + m_range * length_so_far / length;
		else
			m = 0.0;
		q.x = p2.x;
		q.y = p2.y;
		q.z = p2.z;
		q.m = m;
		ptarray_set_point4d(pa, i, &q);
		p1 = p2;
	}

	return lwline_construct(lwline->srid, NULL, pa);
}

LWGEOM*
lwline_remove_repeated_points(LWLINE *lwline)
{
	POINTARRAY* npts = ptarray_remove_repeated_points(lwline->points);

	LWDEBUGF(3, "lwline_remove_repeated_points: npts %p", npts);

	return (LWGEOM*)lwline_construct(lwline->srid,
	                                 lwline->bbox ? gbox_copy(lwline->bbox) : 0,
	                                 npts);
}

int
lwline_is_closed(const LWLINE *line)
{
	if (FLAGS_GET_Z(line->type))
		return ptarray_isclosed3d(line->points);

	return ptarray_isclosed2d(line->points);
}


LWLINE*
lwline_force_dims(const LWLINE *line, int hasz, int hasm)
{
	POINTARRAY *pdims = NULL;
	LWLINE *lineout;
	
	/* Return 2D empty */
	if( lwline_is_empty(line) )
	{
		lineout = lwline_construct_empty(line->srid, hasz, hasm);
	}
	else
	{	
		pdims = ptarray_force_dims(line->points, hasz, hasm);
		lineout = lwline_construct(line->srid, NULL, pdims);
	}
	lineout->type = line->type;
	return lineout;
}

int lwline_is_empty(const LWLINE *line)
{
	if ( !line->points || line->points->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}


int lwline_count_vertices(LWLINE *line)
{
	assert(line);
	if ( ! line->points )
		return 0;
	return line->points->npoints;
}

LWLINE* lwline_simplify(const LWLINE *iline, double dist)
{
	LWLINE *oline;

	LWDEBUG(2, "function called");

	/* Skip empty case */
	if( lwline_is_empty(iline) )
		return lwline_clone(iline);
		
	oline = lwline_construct(iline->srid, NULL, ptarray_simplify(iline->points, dist));
	oline->type = iline->type;
	return oline;
}

double lwline_length(const LWLINE *line)
{
	if ( lwline_is_empty(line) )
		return 0.0;
	return ptarray_length(line->points);
}

double lwline_length_2d(const LWLINE *line)
{
	if ( lwline_is_empty(line) )
		return 0.0;
	return ptarray_length_2d(line->points);
}
