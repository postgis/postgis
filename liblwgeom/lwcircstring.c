/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* basic LWCIRCSTRING functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

BOX3D *lwcircle_compute_box3d(POINT4D *p1, POINT4D *p2, POINT4D *p3);
void printLWCIRCSTRING(LWCIRCSTRING *curve);
void lwcircstring_reverse(LWCIRCSTRING *curve);
void lwcircstring_release(LWCIRCSTRING *lwcirc);
char lwcircstring_same(const LWCIRCSTRING *me, const LWCIRCSTRING *you);
LWCIRCSTRING *lwcircstring_from_lwpointarray(int srid, uint32_t npoints, LWPOINT **points);
LWCIRCSTRING *lwcircstring_from_lwmpoint(int srid, LWMPOINT *mpoint);
LWCIRCSTRING *lwcircstring_addpoint(LWCIRCSTRING *curve, LWPOINT *point, uint32_t where);
LWCIRCSTRING *lwcircstring_removepoint(LWCIRCSTRING *curve, uint32_t index);
void lwcircstring_setPoint4d(LWCIRCSTRING *curve, uint32_t index, POINT4D *newpoint);



/*
 * Construct a new LWCIRCSTRING.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWCIRCSTRING *
lwcircstring_construct(int srid, GBOX *bbox, POINTARRAY *points)
{
	LWCIRCSTRING *result;

	/*
	        * The first arc requires three points.  Each additional
	        * arc requires two more points.  Thus the minimum point count
	        * is three, and the count must be odd.
	        */
	if (points->npoints % 2 != 1 || points->npoints < 3)
	{
		lwnotice("lwcircstring_construct: invalid point count %d", points->npoints);
	}

	result = (LWCIRCSTRING*) lwalloc(sizeof(LWCIRCSTRING));

	result->type = CIRCSTRINGTYPE;
	
	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

LWCIRCSTRING *
lwcircstring_construct_empty(int srid, char hasz, char hasm)
{
	LWCIRCSTRING *result = lwalloc(sizeof(LWCIRCSTRING));
	result->type = CIRCSTRINGTYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}

void
lwcircstring_release(LWCIRCSTRING *lwcirc)
{
	lwgeom_release(lwcircstring_as_lwgeom(lwcirc));
}



/*
 * convert this circularstring into its serialized form
 * result's first char will be the 8bit type. See serialized form doc
 */
uint8_t *
lwcircstring_serialize(LWCIRCSTRING *curve)
{
	size_t size, retsize;
	uint8_t * result;

	if (curve == NULL)
	{
		lwerror("lwcircstring_serialize:: given null curve");
		return NULL;
	}

	size = lwcircstring_serialize_size(curve);
	result = lwalloc(size);
	lwcircstring_serialize_buf(curve, result, &retsize);
	if (retsize != size)
		lwerror("lwcircstring_serialize_size returned %d, ..serialize_buf returned %d", size, retsize);
	return result;
}

/*
 * convert this circularstring into its serialized form writing it into
 * the given buffer, and returning number of bytes written into
 * the given int pointer.
 * result's first char will be the 8bit type.  See serialized form doc
 */
void lwcircstring_serialize_buf(LWCIRCSTRING *curve, uint8_t *buf, size_t *retsize)
{
	char has_srid;
	uint8_t *loc;
	int ptsize;
	size_t size;

	LWDEBUGF(2, "lwcircstring_serialize_buf(%p, %p, %p) called",
	         curve, buf, retsize);

	if (curve == NULL)
	{
		lwerror("lwcircstring_serialize:: given null curve");
		return;
	}

	if (FLAGS_GET_ZM(curve->flags) != FLAGS_GET_ZM(curve->points->flags))
	{
		lwerror("Dimensions mismatch in lwcircstring");
		return;
	}

	ptsize = ptarray_point_size(curve->points);

	has_srid = (curve->srid != SRID_UNKNOWN);

	buf[0] = (uint8_t)lwgeom_makeType_full(
	             FLAGS_GET_Z(curve->flags), FLAGS_GET_M(curve->flags),
	             has_srid, CIRCSTRINGTYPE, curve->bbox ? 1 : 0);
	loc = buf+1;

	LWDEBUGF(3, "lwcircstring_serialize_buf added type (%d)", curve->type);

	if (curve->bbox)
	{
		BOX2DFLOAT4 *box2df;
		
		box2df = box2df_from_gbox(curve->bbox);
		memcpy(loc, box2df, sizeof(BOX2DFLOAT4));
		lwfree(box2df);
		loc += sizeof(BOX2DFLOAT4);

		LWDEBUG(3, "lwcircstring_serialize_buf added BBOX");
	}

	if (has_srid)
	{
		memcpy(loc, &curve->srid, sizeof(int32_t));
		loc += sizeof(int32_t);

		LWDEBUG(3, "lwcircstring_serialize_buf added SRID");
	}

	memcpy(loc, &curve->points->npoints, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	LWDEBUGF(3, "lwcircstring_serialize_buf added npoints (%d)",
	         curve->points->npoints);

	/* copy in points */
	size = curve->points->npoints * ptsize;
	memcpy(loc, getPoint_internal(curve->points, 0), size);
	loc += size;

	LWDEBUGF(3, "lwcircstring_serialize_buf copied serialized_pointlist (%d bytes)",
	         ptsize * curve->points->npoints);

	if (retsize) *retsize = loc-buf;

	LWDEBUGF(3, "lwcircstring_serialize_buf returning (loc: %p, size: %d)",
	         loc, loc-buf);
}

/* find length of this deserialized circularstring */
size_t
lwcircstring_serialize_size(LWCIRCSTRING *curve)
{
	size_t size = 1; /* type */

	LWDEBUG(2, "lwcircstring_serialize_size called");

	if (curve->srid != SRID_UNKNOWN) size += 4; /* SRID */
	if (curve->bbox) size += sizeof(BOX2DFLOAT4);

	size += 4; /* npoints */
	size += ptarray_point_size(curve->points) * curve->points->npoints;

	LWDEBUGF(3, "lwcircstring_serialize_size returning %d", size);

	return size;
}


BOX3D *
lwcircle_compute_box3d(POINT4D *p1, POINT4D *p2, POINT4D *p3)
{
	double x1, x2, y1, y2, z1, z2;
	double angle, radius, sweep;
	/* angles from center */
	double a1, a2, a3;
	/* angles from center once a1 is rotated to zero */
	double r2, r3;
	double xe = 0.0, ye = 0.0;
	POINT4D center;
	int i;
	BOX3D *box;

	LWDEBUG(2, "lwcircle_compute_box3d called.");

	box = lwalloc(sizeof(BOX3D));
	radius = lwcircle_center(p1, p2, p3, &center);
	if (radius < 0.0) 
	{
        box->xmin = FP_MIN(p1->x, p3->x);
        box->ymin = FP_MIN(p1->y, p3->y);
        box->zmin = FP_MIN(p1->z, p3->z);
        box->xmax = FP_MAX(p1->x, p3->x);
        box->ymax = FP_MAX(p1->y, p3->y);
        box->zmax = FP_MAX(p1->z, p3->z);
        return box;
	}

	/*
	top = center.y + radius;
	left = center.x - radius;

	LWDEBUGF(3, "lwcircle_compute_box3d: center (%.16f, %.16f)", center.x, center.y);
	*/

	x1 = MAXFLOAT;
	x2 = -1 * MAXFLOAT;
	y1 = MAXFLOAT;
	y2 = -1 * MAXFLOAT;

	a1 = atan2(p1->y - center.y, p1->x - center.x);
	a2 = atan2(p2->y - center.y, p2->x - center.x);
	a3 = atan2(p3->y - center.y, p3->x - center.x);

	/* Rotate a2 and a3 such that a1 = 0 */
	r2 = a2 - a1;
	r3 = a3 - a1;

	/*
	 * There are six cases here I'm interested in
	 * Clockwise:
	 *   1. a1-a2 < 180 == r2 < 0 && (r3 > 0 || r3 < r2)
	 *   2. a1-a2 > 180 == r2 > 0 && (r3 > 0 && r3 < r2)
	 *   3. a1-a2 > 180 == r2 > 0 && (r3 > r2 || r3 < 0)
	 * Counter-clockwise:
	 *   4. a1-a2 < 180 == r2 > 0 && (r3 < 0 || r3 > r2)
	 *   5. a1-a2 > 180 == r2 < 0 && (r3 < 0 && r3 > r2)
	 *   6. a1-a2 > 180 == r2 < 0 && (r3 < r2 || r3 > 0)
	 * 3 and 6 are invalid cases where a3 is the midpoint.
	 * BBOX is fundamental, so these cannot error out and will instead
	 * calculate the sweep using a3 as the middle point.
	 */

	/* clockwise 1 */
	if (FP_LT(r2, 0) && (FP_GT(r3, 0) || FP_LT(r3, r2)))
	{
		sweep = (FP_GT(r3, 0)) ? (r3 - 2 * M_PI) : r3;
	}
	/* clockwise 2 */
	else if (FP_GT(r2, 0) && FP_GT(r3, 0) && FP_LT(r3, r2))
	{
		sweep = (FP_GT(r3, 0)) ? (r3 - 2 * M_PI) : r3;
	}
	/* counter-clockwise 4 */
	else if (FP_GT(r2, 0) && (FP_LT(r3, 0) || FP_GT(r3, r2)))
	{
		sweep = (FP_LT(r3, 0)) ? (r3 + 2 * M_PI) : r3;
	}
	/* counter-clockwise 5 */
	else if (FP_LT(r2, 0) && FP_LT(r3, 0) && FP_GT(r3, r2))
	{
		sweep = (FP_LT(r3, 0)) ? (r3 + 2 * M_PI) : r3;
	}
	/* clockwise invalid 3 */
	else if (FP_GT(r2, 0) && (FP_GT(r3, r2) || FP_LT(r3, 0)))
	{
		sweep = (FP_GT(r2, 0)) ? (r2 - 2 * M_PI) : r2;
	}
	/* clockwise invalid 6 */
	else
	{
		sweep = (FP_LT(r2, 0)) ? (r2 + 2 * M_PI) : r2;
	}

	LWDEBUGF(3, "a1 %.16f, a2 %.16f, a3 %.16f, sweep %.16f", a1, a2, a3, sweep);

	angle = 0.0;
	for (i=0; i < 6; i++)
	{
		switch (i)
		{
			/* right extent */
		case 0:
			angle = 0.0;
			xe = center.x + radius;
			ye = center.y;
			break;
			/* top extent */
		case 1:
			angle = M_PI_2;
			xe = center.x;
			ye = center.y + radius;
			break;
			/* left extent */
		case 2:
			angle = M_PI;
			xe = center.x - radius;
			ye = center.y;
			break;
			/* bottom extent */
		case 3:
			angle = -1 * M_PI_2;
			xe = center.x;
			ye = center.y - radius;
			break;
			/* first point */
		case 4:
			angle = a1;
			xe = p1->x;
			ye = p1->y;
			break;
			/* last point */
		case 5:
			angle = a3;
			xe = p3->x;
			ye = p3->y;
			break;
		}
		/* determine if the extents are outside the arc */
		if (i < 4)
		{
			if (FP_GT(sweep, 0.0))
			{
				if (FP_LT(a3, a1))
				{
					if (FP_GT(angle, (a3 + 2 * M_PI)) || FP_LT(angle, a1)) continue;
				}
				else
				{
					if (FP_GT(angle, a3) || FP_LT(angle, a1)) continue;
				}
			}
			else
			{
				if (FP_GT(a3, a1))
				{
					if (FP_LT(angle, (a3 - 2 * M_PI)) || FP_GT(angle, a1)) continue;
				}
				else
				{
					if (FP_LT(angle, a3) || FP_GT(angle, a1)) continue;
				}
			}
		}

		LWDEBUGF(3, "lwcircle_compute_box3d: potential extreame %d (%.16f, %.16f)", i, xe, ye);

		x1 = (FP_LT(x1, xe)) ? x1 : xe;
		y1 = (FP_LT(y1, ye)) ? y1 : ye;
		x2 = (FP_GT(x2, xe)) ? x2 : xe;
		y2 = (FP_GT(y2, ye)) ? y2 : ye;
	}

	LWDEBUGF(3, "lwcircle_compute_box3d: extreames found (%.16f %.16f, %.16f %.16f)", x1, y1, x2, y2);

	/*
	x1 = center.x + x1 * radius;
	x2 = center.x + x2 * radius;
	y1 = center.y + y1 * radius;
	y2 = center.y + y2 * radius;
	*/
	z1 = (FP_LT(p1->z, p2->z)) ? p1->z : p2->z;
	z1 = (FP_LT(z1, p3->z)) ? z1 : p3->z;
	z2 = (FP_GT(p1->z, p2->z)) ? p1->z : p2->z;
	z2 = (FP_GT(z2, p3->z)) ? z2 : p3->z;

	box->xmin = x1;
	box->xmax = x2;
	box->ymin = y1;
	box->ymax = y2;
	box->zmin = z1;
	box->zmax = z2;

	return box;
}

/*
 * Find bounding box (standard one)
 * zmin=zmax=NO_Z_VALUE if 2d
 * TODO: This ignores curvature, which should be taken into account.
 */
BOX3D *
lwcircstring_compute_box3d(const LWCIRCSTRING *curve)
{
	BOX3D *box, *tmp;
	int i;
	POINT4D *p1 = lwalloc(sizeof(POINT4D));
	POINT4D *p2 = lwalloc(sizeof(POINT4D));
	POINT4D *p3 = lwalloc(sizeof(POINT4D));

	LWDEBUG(2, "lwcircstring_compute_box3d called.");

	/* initialize box values */
	box = lwalloc(sizeof(BOX3D));
	box->xmin = MAXFLOAT;
	box->xmax = -1 * MAXFLOAT;
	box->ymin = MAXFLOAT;
	box->ymax = -1 * MAXFLOAT;
	box->zmin = MAXFLOAT;
	box->zmax = -1 * MAXFLOAT;

	for (i = 2; i < curve->points->npoints; i+=2)
	{
		getPoint4d_p(curve->points, i-2, p1);
		getPoint4d_p(curve->points, i-1, p2);
		getPoint4d_p(curve->points, i, p3);
		tmp = lwcircle_compute_box3d(p1, p2, p3);
		if (tmp == NULL) continue;
		box->xmin = (box->xmin < tmp->xmin) ? box->xmin : tmp->xmin;
		box->xmax = (box->xmax > tmp->xmax) ? box->xmax : tmp->xmax;
		box->ymin = (box->ymin < tmp->ymin) ? box->ymin : tmp->ymin;
		box->ymax = (box->ymax > tmp->ymax) ? box->ymax : tmp->ymax;
		box->zmin = (box->zmin < tmp->zmin) ? box->zmin : tmp->zmin;
		box->zmax = (box->zmax > tmp->zmax) ? box->zmax : tmp->zmax;

		LWDEBUGF(4, "circularstring %d x=(%.16f,%.16f) y=(%.16f,%.16f) z=(%.16f,%.16f)", i/2, box->xmin, box->xmax, box->ymin, box->ymax, box->zmin, box->zmax);
	}

	return box;
}


void lwcircstring_free(LWCIRCSTRING *curve)
{
	if ( curve->bbox )
		lwfree(curve->bbox);
	if ( curve->points )
		ptarray_free(curve->points);
	lwfree(curve);
}



void printLWCIRCSTRING(LWCIRCSTRING *curve)
{
	lwnotice("LWCIRCSTRING {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(curve->flags));
	lwnotice("    srid = %i", (int)curve->srid);
	printPA(curve->points);
	lwnotice("}");
}

/* @brief Clone LWCIRCSTRING object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
LWCIRCSTRING *
lwcircstring_clone(const LWCIRCSTRING *g)
{
	return (LWCIRCSTRING *)lwline_clone((LWLINE *)g);
}


void lwcircstring_reverse(LWCIRCSTRING *curve)
{
	ptarray_reverse(curve->points);
}

/* check coordinate equality */
char
lwcircstring_same(const LWCIRCSTRING *me, const LWCIRCSTRING *you)
{
	return ptarray_same(me->points, you->points);
}

/*
 * Construct a LWCIRCSTRING from an array of LWPOINTs
 * LWCIRCSTRING dimensions are large enough to host all input dimensions.
 */
LWCIRCSTRING *
lwcircstring_from_lwpointarray(int srid, uint32_t npoints, LWPOINT **points)
{
	int zmflag=0;
	uint32_t i;
	POINTARRAY *pa;
	uint8_t *newpoints, *ptr;
	size_t ptsize, size;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i = 0; i < npoints; i++)
	{
		if (points[i]->type != POINTTYPE)
		{
			lwerror("lwcurve_from_lwpointarray: invalid input type: %s",
			        lwtype_name(points[i]->type));
			return NULL;
		}
		if (FLAGS_GET_Z(points[i]->flags)) zmflag |= 2;
		if (FLAGS_GET_M(points[i]->flags)) zmflag |= 1;
		if (zmflag == 3) break;
	}

	if (zmflag == 0) ptsize = 2 * sizeof(double);
	else if (zmflag == 3) ptsize = 4 * sizeof(double);
	else ptsize = 3 * sizeof(double);

	/*
	 * Allocate output points array
	 */
	size = ptsize * npoints;
	newpoints = lwalloc(size);
	memset(newpoints, 0, size);

	ptr = newpoints;
	for (i = 0; i < npoints; i++)
	{
		size = ptarray_point_size(points[i]->point);
		memcpy(ptr, getPoint_internal(points[i]->point, 0), size);
		ptr += ptsize;
	}
	pa = ptarray_construct_reference_data(zmflag&2, zmflag&1, npoints, newpoints);
	
	return lwcircstring_construct(srid, NULL, pa);
}

/*
 * Construct a LWCIRCSTRING from a LWMPOINT
 */
LWCIRCSTRING *
lwcircstring_from_lwmpoint(int srid, LWMPOINT *mpoint)
{
	uint32_t i;
	POINTARRAY *pa;
	char zmflag = FLAGS_GET_ZM(mpoint->flags);
	size_t ptsize, size;
	uint8_t *newpoints, *ptr;

	if (zmflag == 0) ptsize = 2 * sizeof(double);
	else if (zmflag == 3) ptsize = 4 * sizeof(double);
	else ptsize = 3 * sizeof(double);

	/* Allocate space for output points */
	size = ptsize * mpoint->ngeoms;
	newpoints = lwalloc(size);
	memset(newpoints, 0, size);

	ptr = newpoints;
	for (i = 0; i < mpoint->ngeoms; i++)
	{
		memcpy(ptr,
		       getPoint_internal(mpoint->geoms[i]->point, 0),
		       ptsize);
		ptr += ptsize;
	}

	pa = ptarray_construct_reference_data(zmflag&2, zmflag&1, mpoint->ngeoms, newpoints);
	
	LWDEBUGF(3, "lwcurve_from_lwmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);

	return lwcircstring_construct(srid, NULL, pa);
}

LWCIRCSTRING *
lwcircstring_addpoint(LWCIRCSTRING *curve, LWPOINT *point, uint32_t where)
{
	POINTARRAY *newpa;
	LWCIRCSTRING *ret;

	newpa = ptarray_addPoint(curve->points,
	                         getPoint_internal(point->point, 0),
	                         FLAGS_NDIMS(point->flags), where);
	ret = lwcircstring_construct(curve->srid, NULL, newpa);

	return ret;
}

LWCIRCSTRING *
lwcircstring_removepoint(LWCIRCSTRING *curve, uint32_t index)
{
	POINTARRAY *newpa;
	LWCIRCSTRING *ret;

	newpa = ptarray_removePoint(curve->points, index);
	ret = lwcircstring_construct(curve->srid, NULL, newpa);

	return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 * */
void
lwcircstring_setPoint4d(LWCIRCSTRING *curve, uint32_t index, POINT4D *newpoint)
{
	ptarray_set_point4d(curve->points, index, newpoint);
}

int
lwcircstring_is_closed(const LWCIRCSTRING *curve)
{
	if (FLAGS_GET_Z(curve->flags))
		return ptarray_isclosed3d(curve->points);

	return ptarray_isclosed2d(curve->points);
}

int lwcircstring_is_empty(const LWCIRCSTRING *circ)
{
	if ( !circ->points || circ->points->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

double lwcircstring_length(const LWCIRCSTRING *circ)
{
	double length = 0.0;
	LWLINE *line;
	if ( lwcircstring_is_empty(circ) )
		return 0.0;
	line = lwcircstring_segmentize(circ, 32);
	length = lwline_length(line);
	lwline_free(line);
	return length;
}

double lwcircstring_length_2d(const LWCIRCSTRING *circ)
{
	double length = 0.0;
	LWLINE *line;
	if ( lwcircstring_is_empty(circ) )
		return 0.0;
	line = lwcircstring_segmentize(circ, 32);
	length = lwline_length_2d(line);
	lwline_free(line);
	return length;
}
