/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../postgis_config.h"
/*#define POSTGIS_DEBUG_LEVEL 4*/
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

int
ptarray_has_z(const POINTARRAY *pa)
{
	if ( ! pa ) return LW_FALSE;
	return FLAGS_GET_Z(pa->flags);
}

int
ptarray_has_m(const POINTARRAY *pa)
{
	if ( ! pa ) return LW_FALSE;
	return FLAGS_GET_M(pa->flags);
}

POINTARRAY*
ptarray_construct(char hasz, char hasm, uint32_t npoints)
{
	POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, npoints);
	pa->npoints = npoints;
	return pa;
}

POINTARRAY*
ptarray_construct_empty(char hasz, char hasm, uint32_t maxpoints)
{
	POINTARRAY *pa = lwalloc(sizeof(POINTARRAY));
	pa->serialized_pointlist = NULL;

	/* Set our dimensionality info on the bitmap */
	pa->flags = lwflags(hasz, hasm, 0);

	/* We will be allocating a bit of room */
	pa->npoints = 0;
	pa->maxpoints = maxpoints;

	/* Allocate the coordinate array */
	if ( maxpoints > 0 )
		pa->serialized_pointlist = lwalloc(maxpoints * ptarray_point_size(pa));
	else
		pa->serialized_pointlist = NULL;

	return pa;
}

/*
* Add a point into a pointarray. Only adds as many dimensions as the
* pointarray supports.
*/
int
ptarray_insert_point(POINTARRAY *pa, const POINT4D *p, uint32_t where)
{
	if (!pa || !p)
		return LW_FAILURE;
	size_t point_size = ptarray_point_size(pa);
	LWDEBUGF(5,"pa = %p; p = %p; where = %d", pa, p, where);
	LWDEBUGF(5,"pa->npoints = %d; pa->maxpoints = %d", pa->npoints, pa->maxpoints);

	if ( FLAGS_GET_READONLY(pa->flags) )
	{
		lwerror("ptarray_insert_point: called on read-only point array");
		return LW_FAILURE;
	}

	/* Error on invalid offset value */
	if ( where > pa->npoints )
	{
		lwerror("ptarray_insert_point: offset out of range (%d)", where);
		return LW_FAILURE;
	}

	/* If we have no storage, let's allocate some */
	if( pa->maxpoints == 0 || ! pa->serialized_pointlist )
	{
		pa->maxpoints = 32;
		pa->npoints = 0;
		pa->serialized_pointlist = lwalloc(ptarray_point_size(pa) * pa->maxpoints);
	}

	/* Error out if we have a bad situation */
	if ( pa->npoints > pa->maxpoints )
	{
		lwerror("npoints (%d) is greater than maxpoints (%d)", pa->npoints, pa->maxpoints);
		return LW_FAILURE;
	}

	/* Check if we have enough storage, add more if necessary */
	if( pa->npoints == pa->maxpoints )
	{
		pa->maxpoints *= 2;
		pa->serialized_pointlist = lwrealloc(pa->serialized_pointlist, ptarray_point_size(pa) * pa->maxpoints);
	}

	/* Make space to insert the new point */
	if( where < pa->npoints )
	{
		size_t copy_size = point_size * (pa->npoints - where);
		memmove(getPoint_internal(pa, where+1), getPoint_internal(pa, where), copy_size);
		LWDEBUGF(5,"copying %d bytes to start vertex %d from start vertex %d", copy_size, where+1, where);
	}

	/* We have one more point */
	++pa->npoints;

	/* Copy the new point into the gap */
	ptarray_set_point4d(pa, where, p);
	LWDEBUGF(5,"copying new point to start vertex %d", point_size, where);

	return LW_SUCCESS;
}

int
ptarray_append_point(POINTARRAY *pa, const POINT4D *pt, int repeated_points)
{
	/* Check for pathology */
	if( ! pa || ! pt )
	{
		lwerror("ptarray_append_point: null input");
		return LW_FAILURE;
	}

	/* Check for duplicate end point */
	if ( repeated_points == LW_FALSE && pa->npoints > 0 )
	{
		POINT4D tmp;
		getPoint4d_p(pa, pa->npoints-1, &tmp);
		LWDEBUGF(4,"checking for duplicate end point (pt = POINT(%g %g) pa->npoints-q = POINT(%g %g))",pt->x,pt->y,tmp.x,tmp.y);

		/* Return LW_SUCCESS and do nothing else if previous point in list is equal to this one */
		if ( (pt->x == tmp.x) && (pt->y == tmp.y) &&
		     (FLAGS_GET_Z(pa->flags) ? pt->z == tmp.z : 1) &&
		     (FLAGS_GET_M(pa->flags) ? pt->m == tmp.m : 1) )
		{
			return LW_SUCCESS;
		}
	}

	/* Append is just a special case of insert */
	return ptarray_insert_point(pa, pt, pa->npoints);
}

int
ptarray_append_ptarray(POINTARRAY *pa1, POINTARRAY *pa2, double gap_tolerance)
{
	unsigned int poff = 0;
	unsigned int npoints;
	unsigned int ncap;
	unsigned int ptsize;

	/* Check for pathology */
	if( ! pa1 || ! pa2 )
	{
		lwerror("ptarray_append_ptarray: null input");
		return LW_FAILURE;
	}

	npoints = pa2->npoints;

	if ( ! npoints ) return LW_SUCCESS; /* nothing more to do */

	if( FLAGS_GET_READONLY(pa1->flags) )
	{
		lwerror("ptarray_append_ptarray: target pointarray is read-only");
		return LW_FAILURE;
	}

	if( FLAGS_GET_ZM(pa1->flags) != FLAGS_GET_ZM(pa2->flags) )
	{
		lwerror("ptarray_append_ptarray: appending mixed dimensionality is not allowed");
		return LW_FAILURE;
	}

	ptsize = ptarray_point_size(pa1);

	/* Check for duplicate end point */
	if ( pa1->npoints )
	{
		POINT2D tmp1, tmp2;
		getPoint2d_p(pa1, pa1->npoints-1, &tmp1);
		getPoint2d_p(pa2, 0, &tmp2);

		/* If the end point and start point are the same, then don't copy start point */
		if (p2d_same(&tmp1, &tmp2)) {
			poff = 1;
			--npoints;
		}
		else if ( gap_tolerance == 0 || ( gap_tolerance > 0 &&
		           distance2d_pt_pt(&tmp1, &tmp2) > gap_tolerance ) )
		{
			lwerror("Second line start point too far from first line end point");
			return LW_FAILURE;
		}
	}

	/* Check if we need extra space */
	ncap = pa1->npoints + npoints;
	if ( pa1->maxpoints < ncap )
	{
		pa1->maxpoints = ncap > pa1->maxpoints*2 ?
		                 ncap : pa1->maxpoints*2;
		pa1->serialized_pointlist = lwrealloc(pa1->serialized_pointlist, ptsize * pa1->maxpoints);
	}

	memcpy(getPoint_internal(pa1, pa1->npoints),
	       getPoint_internal(pa2, poff), ptsize * npoints);

	pa1->npoints = ncap;

	return LW_SUCCESS;
}

/*
* Add a point into a pointarray. Only adds as many dimensions as the
* pointarray supports.
*/
int
ptarray_remove_point(POINTARRAY *pa, uint32_t where)
{
	/* Check for pathology */
	if( ! pa )
	{
		lwerror("ptarray_remove_point: null input");
		return LW_FAILURE;
	}

	/* Error on invalid offset value */
	if ( where >= pa->npoints )
	{
		lwerror("ptarray_remove_point: offset out of range (%d)", where);
		return LW_FAILURE;
	}

	/* If the point is any but the last, we need to copy the data back one point */
	if (where < pa->npoints - 1)
		memmove(getPoint_internal(pa, where),
			getPoint_internal(pa, where + 1),
			ptarray_point_size(pa) * (pa->npoints - where - 1));

	/* We have one less point */
	pa->npoints--;

	return LW_SUCCESS;
}

/**
* Build a new #POINTARRAY, but on top of someone else's ordinate array.
* Flag as read-only, so that ptarray_free() does not free the serialized_ptlist
*/
POINTARRAY* ptarray_construct_reference_data(char hasz, char hasm, uint32_t npoints, uint8_t *ptlist)
{
	POINTARRAY *pa = lwalloc(sizeof(POINTARRAY));
	LWDEBUGF(5, "hasz = %d, hasm = %d, npoints = %d, ptlist = %p", hasz, hasm, npoints, ptlist);
	pa->flags = lwflags(hasz, hasm, 0);
	FLAGS_SET_READONLY(pa->flags, 1); /* We don't own this memory, so we can't alter or free it. */
	pa->npoints = npoints;
	pa->maxpoints = npoints;
	pa->serialized_pointlist = ptlist;
	return pa;
}


POINTARRAY*
ptarray_construct_copy_data(char hasz, char hasm, uint32_t npoints, const uint8_t *ptlist)
{
	POINTARRAY *pa = lwalloc(sizeof(POINTARRAY));

	pa->flags = lwflags(hasz, hasm, 0);
	pa->npoints = npoints;
	pa->maxpoints = npoints;

	if ( npoints > 0 )
	{
		pa->serialized_pointlist = lwalloc(ptarray_point_size(pa) * npoints);
		memcpy(pa->serialized_pointlist, ptlist, ptarray_point_size(pa) * npoints);
	}
	else
	{
		pa->serialized_pointlist = NULL;
	}

	return pa;
}

void
ptarray_free(POINTARRAY *pa)
{
	if (pa)
	{
		if (pa->serialized_pointlist && (!FLAGS_GET_READONLY(pa->flags)))
			lwfree(pa->serialized_pointlist);
		lwfree(pa);
	}
}


void
ptarray_reverse_in_place(POINTARRAY *pa)
{
	if (!pa->npoints)
		return;
	uint32_t i;
	uint32_t last = pa->npoints - 1;
	uint32_t mid = pa->npoints / 2;

	double *d = (double*)(pa->serialized_pointlist);
	int j;
	int ndims = FLAGS_NDIMS(pa->flags);
	for (i = 0; i < mid; i++)
	{
		for (j = 0; j < ndims; j++)
		{
			double buf;
			buf = d[i*ndims+j];
			d[i*ndims+j] = d[(last-i)*ndims+j];
			d[(last-i)*ndims+j] = buf;
		}
	}
	return;
}


/**
 * Reverse X and Y axis on a given POINTARRAY
 */
POINTARRAY*
ptarray_flip_coordinates(POINTARRAY *pa)
{
	uint32_t i;
	double d;
	POINT4D p;

	for (i=0 ; i < pa->npoints ; i++)
	{
		getPoint4d_p(pa, i, &p);
		d = p.y;
		p.y = p.x;
		p.x = d;
		ptarray_set_point4d(pa, i, &p);
	}

	return pa;
}

void
ptarray_swap_ordinates(POINTARRAY *pa, LWORD o1, LWORD o2)
{
	uint32_t i;
	double d, *dp1, *dp2;
	POINT4D p;

	dp1 = ((double*)&p)+(unsigned)o1;
	dp2 = ((double*)&p)+(unsigned)o2;
	for (i=0 ; i < pa->npoints ; i++)
	{
		getPoint4d_p(pa, i, &p);
		d = *dp2;
		*dp2 = *dp1;
		*dp1 = d;
		ptarray_set_point4d(pa, i, &p);
	}
}

/**
 * @brief Returns a modified #POINTARRAY so that no segment is
 * 		longer than the given distance (computed using 2d).
 *
 * Every input point is kept.
 * Z and M values for added points (if needed) are set proportionally.
 */
POINTARRAY *
ptarray_segmentize2d(const POINTARRAY *ipa, double dist)
{
	double segdist;
	POINT4D	p1, p2;
	POINT4D pbuf;
	POINTARRAY *opa;
	uint32_t i, j, nseg;
	int hasz = FLAGS_GET_Z(ipa->flags);
	int hasm = FLAGS_GET_M(ipa->flags);

	pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0;

	/* Initial storage */
	opa = ptarray_construct_empty(hasz, hasm, ipa->npoints);

	/* Add first point */
	getPoint4d_p(ipa, 0, &p1);
	ptarray_append_point(opa, &p1, LW_FALSE);

	/* Loop on all other input points */
	for (i = 1; i < ipa->npoints; i++)
	{
		/*
		 * We use these pointers to avoid
		 * "strict-aliasing rules break" warning raised
		 * by gcc (3.3 and up).
		 *
		 * It looks that casting a variable address (also
		 * referred to as "type-punned pointer")
		 * breaks those "strict" rules.
		 */
		POINT4D *p1ptr=&p1, *p2ptr=&p2;
		double segments;

		getPoint4d_p(ipa, i, &p2);

		segdist = distance2d_pt_pt((POINT2D *)p1ptr, (POINT2D *)p2ptr);
		/* Split input segment into shorter even chunks */
		segments = ceil(segdist / dist);

		/* Uses INT32_MAX instead of UINT32_MAX to be safe that it fits */
		if (segments >= INT32_MAX)
		{
			lwnotice("%s:%d - %s: Too many segments required (%e)",
				__FILE__, __LINE__,__func__, segments);
			ptarray_free(opa);
			return NULL;
		}
		nseg = segments;

		for (j = 1; j < nseg; j++)
		{
			pbuf.x = p1.x + (p2.x - p1.x) * j / nseg;
			pbuf.y = p1.y + (p2.y - p1.y) * j / nseg;
			if (hasz)
				pbuf.z = p1.z + (p2.z - p1.z) * j / nseg;
			if (hasm)
				pbuf.m = p1.m + (p2.m - p1.m) * j / nseg;
			ptarray_append_point(opa, &pbuf, LW_FALSE);
			LW_ON_INTERRUPT(ptarray_free(opa); return NULL);
		}

		ptarray_append_point(opa, &p2, (ipa->npoints == 2) ? LW_TRUE : LW_FALSE);
		p1 = p2;
		LW_ON_INTERRUPT(ptarray_free(opa); return NULL);
	}

	return opa;
}

char
ptarray_same(const POINTARRAY *pa1, const POINTARRAY *pa2)
{
	uint32_t i;
	size_t ptsize;

	if ( FLAGS_GET_ZM(pa1->flags) != FLAGS_GET_ZM(pa2->flags) ) return LW_FALSE;
	LWDEBUG(5,"dimensions are the same");

	if ( pa1->npoints != pa2->npoints ) return LW_FALSE;
	LWDEBUG(5,"npoints are the same");

	ptsize = ptarray_point_size(pa1);
	LWDEBUGF(5, "ptsize = %d", ptsize);

	for (i=0; i<pa1->npoints; i++)
	{
		if ( memcmp(getPoint_internal(pa1, i), getPoint_internal(pa2, i), ptsize) )
			return LW_FALSE;
		LWDEBUGF(5,"point #%d is the same",i);
	}

	return LW_TRUE;
}

POINTARRAY *
ptarray_addPoint(const POINTARRAY *pa, uint8_t *p, size_t pdims, uint32_t where)
{
	POINTARRAY *ret;
	POINT4D pbuf;
	size_t ptsize = ptarray_point_size(pa);

	LWDEBUGF(3, "pa %x p %x size %d where %d",
	         pa, p, pdims, where);

	if ( pdims < 2 || pdims > 4 )
	{
		lwerror("ptarray_addPoint: point dimension out of range (%d)",
		        pdims);
		return NULL;
	}

	if ( where > pa->npoints )
	{
		lwerror("ptarray_addPoint: offset out of range (%d)",
		        where);
		return NULL;
	}

	LWDEBUG(3, "called with a %dD point");

	pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0.0;
	memcpy((uint8_t *)&pbuf, p, pdims*sizeof(double));

	LWDEBUG(3, "initialized point buffer");

	ret = ptarray_construct(FLAGS_GET_Z(pa->flags),
	                        FLAGS_GET_M(pa->flags), pa->npoints+1);


	if ( where )
	{
		memcpy(getPoint_internal(ret, 0), getPoint_internal(pa, 0), ptsize*where);
	}

	memcpy(getPoint_internal(ret, where), (uint8_t *)&pbuf, ptsize);

	if ( where+1 != ret->npoints )
	{
		memcpy(getPoint_internal(ret, where+1),
		       getPoint_internal(pa, where),
		       ptsize*(pa->npoints-where));
	}

	return ret;
}

POINTARRAY *
ptarray_removePoint(POINTARRAY *pa, uint32_t which)
{
	POINTARRAY *ret;
	size_t ptsize = ptarray_point_size(pa);

	LWDEBUGF(3, "pa %x which %d", pa, which);

#if PARANOIA_LEVEL > 0
	if ( which > pa->npoints-1 )
	{
		lwerror("%s [%d] offset (%d) out of range (%d..%d)", __FILE__, __LINE__,
		        which, 0, pa->npoints-1);
		return NULL;
	}

	if ( pa->npoints < 3 )
	{
		lwerror("%s [%d] can't remove a point from a 2-vertex POINTARRAY", __FILE__, __LINE__);
		return NULL;
	}
#endif

	ret = ptarray_construct(FLAGS_GET_Z(pa->flags),
	                        FLAGS_GET_M(pa->flags), pa->npoints-1);

	/* copy initial part */
	if ( which )
	{
		memcpy(getPoint_internal(ret, 0), getPoint_internal(pa, 0), ptsize*which);
	}

	/* copy final part */
	if ( which < pa->npoints-1 )
	{
		memcpy(getPoint_internal(ret, which), getPoint_internal(pa, which+1),
		       ptsize*(pa->npoints-which-1));
	}

	return ret;
}

POINTARRAY *
ptarray_merge(POINTARRAY *pa1, POINTARRAY *pa2)
{
	POINTARRAY *pa;
	size_t ptsize = ptarray_point_size(pa1);

	if (FLAGS_GET_ZM(pa1->flags) != FLAGS_GET_ZM(pa2->flags))
		lwerror("ptarray_cat: Mixed dimension");

	pa = ptarray_construct( FLAGS_GET_Z(pa1->flags),
	                        FLAGS_GET_M(pa1->flags),
	                        pa1->npoints + pa2->npoints);

	memcpy(         getPoint_internal(pa, 0),
	                getPoint_internal(pa1, 0),
	                ptsize*(pa1->npoints));

	memcpy(         getPoint_internal(pa, pa1->npoints),
	                getPoint_internal(pa2, 0),
	                ptsize*(pa2->npoints));

	ptarray_free(pa1);
	ptarray_free(pa2);

	return pa;
}


/**
 * @brief Deep clone a pointarray (also clones serialized pointlist)
 */
POINTARRAY *
ptarray_clone_deep(const POINTARRAY *in)
{
	POINTARRAY *out = lwalloc(sizeof(POINTARRAY));

	LWDEBUG(3, "ptarray_clone_deep called.");

	out->flags = in->flags;
	out->npoints = in->npoints;
	out->maxpoints = in->npoints;

	FLAGS_SET_READONLY(out->flags, 0);

	if (!in->npoints)
	{
		// Avoid calling lwalloc of 0 bytes
		out->serialized_pointlist = NULL;
	}
	else
	{
		size_t size = in->npoints * ptarray_point_size(in);
		out->serialized_pointlist = lwalloc(size);
		memcpy(out->serialized_pointlist, in->serialized_pointlist, size);
	}

	return out;
}

/**
 * @brief Clone a POINTARRAY object. Serialized pointlist is not copied.
 */
POINTARRAY *
ptarray_clone(const POINTARRAY *in)
{
	POINTARRAY *out = lwalloc(sizeof(POINTARRAY));

	LWDEBUG(3, "ptarray_clone called.");

	out->flags = in->flags;
	out->npoints = in->npoints;
	out->maxpoints = in->maxpoints;

	FLAGS_SET_READONLY(out->flags, 1);

	out->serialized_pointlist = in->serialized_pointlist;

	return out;
}

/**
* Check for ring closure using whatever dimensionality is declared on the
* pointarray.
*/
int
ptarray_is_closed(const POINTARRAY *in)
{
	if (!in)
	{
		lwerror("ptarray_is_closed: called with null point array");
		return 0;
	}
	if (in->npoints <= 1 ) return in->npoints; /* single-point are closed, empty not closed */

	return 0 == memcmp(getPoint_internal(in, 0), getPoint_internal(in, in->npoints-1), ptarray_point_size(in));
}


int
ptarray_is_closed_2d(const POINTARRAY *in)
{
	if (!in)
	{
		lwerror("ptarray_is_closed_2d: called with null point array");
		return 0;
	}
	if (in->npoints <= 1 ) return in->npoints; /* single-point are closed, empty not closed */

	return 0 == memcmp(getPoint_internal(in, 0), getPoint_internal(in, in->npoints-1), sizeof(POINT2D) );
}

int
ptarray_is_closed_3d(const POINTARRAY *in)
{
	if (!in)
	{
		lwerror("ptarray_is_closed_3d: called with null point array");
		return 0;
	}
	if (in->npoints <= 1 ) return in->npoints; /* single-point are closed, empty not closed */

	return 0 == memcmp(getPoint_internal(in, 0), getPoint_internal(in, in->npoints-1), sizeof(POINT3D) );
}

int
ptarray_is_closed_z(const POINTARRAY *in)
{
	if ( FLAGS_GET_Z(in->flags) )
		return ptarray_is_closed_3d(in);
	else
		return ptarray_is_closed_2d(in);
}

/**
* Return 1 if the point is inside the POINTARRAY, -1 if it is outside,
* and 0 if it is on the boundary.
*/
int
ptarray_contains_point(const POINTARRAY *pa, const POINT2D *pt)
{
	return ptarray_contains_point_partial(pa, pt, LW_TRUE, NULL);
}

int
ptarray_contains_point_partial(const POINTARRAY *pa, const POINT2D *pt, int check_closed, int *winding_number)
{
	int wn = 0;
	uint32_t i;
	double side;
	const POINT2D *seg1;
	const POINT2D *seg2;
	double ymin, ymax;

	seg1 = getPoint2d_cp(pa, 0);
	seg2 = getPoint2d_cp(pa, pa->npoints-1);
	if ( check_closed && ! p2d_same(seg1, seg2) )
		lwerror("ptarray_contains_point called on unclosed ring");

	for ( i=1; i < pa->npoints; i++ )
	{
		seg2 = getPoint2d_cp(pa, i);

		/* Zero length segments are ignored. */
		if ( seg1->x == seg2->x && seg1->y == seg2->y )
		{
			seg1 = seg2;
			continue;
		}

		ymin = FP_MIN(seg1->y, seg2->y);
		ymax = FP_MAX(seg1->y, seg2->y);

		/* Only test segments in our vertical range */
		if ( pt->y > ymax || pt->y < ymin )
		{
			seg1 = seg2;
			continue;
		}

		side = lw_segment_side(seg1, seg2, pt);

		/*
		* A point on the boundary of a ring is not contained.
		* WAS: if (fabs(side) < 1e-12), see #852
		*/
		if ( (side == 0) && lw_pt_in_seg(pt, seg1, seg2) )
		{
			return LW_BOUNDARY;
		}

		/*
		* If the point is to the left of the line, and it's rising,
		* then the line is to the right of the point and
		* circling counter-clockwise, so increment.
		*/
		if ( (side < 0) && (seg1->y <= pt->y) && (pt->y < seg2->y) )
		{
			wn++;
		}

		/*
		* If the point is to the right of the line, and it's falling,
		* then the line is to the right of the point and circling
		* clockwise, so decrement.
		*/
		else if ( (side > 0) && (seg2->y <= pt->y) && (pt->y < seg1->y) )
		{
			wn--;
		}

		seg1 = seg2;
	}

	/* Sent out the winding number for calls that are building on this as a primitive */
	if ( winding_number )
		*winding_number = wn;

	/* Outside */
	if (wn == 0)
	{
		return LW_OUTSIDE;
	}

	/* Inside */
	return LW_INSIDE;
}

/**
* For POINTARRAYs representing CIRCULARSTRINGS. That is, linked triples
* with each triple being control points of a circular arc. Such
* POINTARRAYs have an odd number of vertices.
*
* Return 1 if the point is inside the POINTARRAY, -1 if it is outside,
* and 0 if it is on the boundary.
*/

int
ptarrayarc_contains_point(const POINTARRAY *pa, const POINT2D *pt)
{
	return ptarrayarc_contains_point_partial(pa, pt, LW_TRUE /* Check closed*/, NULL);
}

int
ptarrayarc_contains_point_partial(const POINTARRAY *pa, const POINT2D *pt, int check_closed, int *winding_number)
{
	int wn = 0;
	uint32_t i;
	int side;
	const POINT2D *seg1;
	const POINT2D *seg2;
	const POINT2D *seg3;
	GBOX gbox;

	/* Check for not an arc ring (always have odd # of points) */
	if ( (pa->npoints % 2) == 0 )
	{
		lwerror("ptarrayarc_contains_point called with even number of points");
		return LW_OUTSIDE;
	}

	/* Check for not an arc ring (always have >= 3 points) */
	if ( pa->npoints < 3 )
	{
		lwerror("ptarrayarc_contains_point called too-short pointarray");
		return LW_OUTSIDE;
	}

	/* Check for unclosed case */
	seg1 = getPoint2d_cp(pa, 0);
	seg3 = getPoint2d_cp(pa, pa->npoints-1);
	if ( check_closed && ! p2d_same(seg1, seg3) )
	{
		lwerror("ptarrayarc_contains_point called on unclosed ring");
		return LW_OUTSIDE;
	}
	/* OK, it's closed. Is it just one circle? */
	else if ( p2d_same(seg1, seg3) && pa->npoints == 3 )
	{
		double radius, d;
		POINT2D c;
		seg2 = getPoint2d_cp(pa, 1);

		/* Wait, it's just a point, so it can't contain anything */
		if ( lw_arc_is_pt(seg1, seg2, seg3) )
			return LW_OUTSIDE;

		/* See if the point is within the circle radius */
		radius = lw_arc_center(seg1, seg2, seg3, &c);
		d = distance2d_pt_pt(pt, &c);
		if ( FP_EQUALS(d, radius) )
			return LW_BOUNDARY; /* Boundary of circle */
		else if ( d < radius )
			return LW_INSIDE; /* Inside circle */
		else
			return LW_OUTSIDE; /* Outside circle */
	}
	else if ( p2d_same(seg1, pt) || p2d_same(seg3, pt) )
	{
		return LW_BOUNDARY; /* Boundary case */
	}

	/* Start on the ring */
	seg1 = getPoint2d_cp(pa, 0);
	for ( i=1; i < pa->npoints; i += 2 )
	{
		seg2 = getPoint2d_cp(pa, i);
		seg3 = getPoint2d_cp(pa, i+1);

		/* Catch an easy boundary case */
		if( p2d_same(seg3, pt) )
			return LW_BOUNDARY;

		/* Skip arcs that have no size */
		if ( lw_arc_is_pt(seg1, seg2, seg3) )
		{
			seg1 = seg3;
			continue;
		}

		/* Only test segments in our vertical range */
		lw_arc_calculate_gbox_cartesian_2d(seg1, seg2, seg3, &gbox);
		if ( pt->y > gbox.ymax || pt->y < gbox.ymin )
		{
			seg1 = seg3;
			continue;
		}

		/* Outside of horizontal range, and not between end points we also skip */
		if ( (pt->x > gbox.xmax || pt->x < gbox.xmin) &&
			 (pt->y > FP_MAX(seg1->y, seg3->y) || pt->y < FP_MIN(seg1->y, seg3->y)) )
		{
			seg1 = seg3;
			continue;
		}

		side = lw_arc_side(seg1, seg2, seg3, pt);

		/* On the boundary */
		if ( (side == 0) && lw_pt_in_arc(pt, seg1, seg2, seg3) )
		{
			return LW_BOUNDARY;
		}

		/* Going "up"! Point to left of arc. */
		if ( side < 0 && (seg1->y <= pt->y) && (pt->y < seg3->y) )
		{
			wn++;
		}

		/* Going "down"! */
		if ( side > 0 && (seg2->y <= pt->y) && (pt->y < seg1->y) )
		{
			wn--;
		}

		/* Inside the arc! */
		if ( pt->x <= gbox.xmax && pt->x >= gbox.xmin )
		{
			POINT2D C;
			double radius = lw_arc_center(seg1, seg2, seg3, &C);
			double d = distance2d_pt_pt(pt, &C);

			/* On the boundary! */
			if ( d == radius )
				return LW_BOUNDARY;

			/* Within the arc! */
			if ( d  < radius )
			{
				/* Left side, increment winding number */
				if ( side < 0 )
					wn++;
				/* Right side, decrement winding number */
				if ( side > 0 )
					wn--;
			}
		}

		seg1 = seg3;
	}

	/* Sent out the winding number for calls that are building on this as a primitive */
	if ( winding_number )
		*winding_number = wn;

	/* Outside */
	if (wn == 0)
	{
		return LW_OUTSIDE;
	}

	/* Inside */
	return LW_INSIDE;
}

/**
* Returns the area in cartesian units. Area is negative if ring is oriented CCW,
* positive if it is oriented CW and zero if the ring is degenerate or flat.
* http://en.wikipedia.org/wiki/Shoelace_formula
*/
double
ptarray_signed_area(const POINTARRAY *pa)
{
	const POINT2D *P1;
	const POINT2D *P2;
	const POINT2D *P3;
	double sum = 0.0;
	double x0, x, y1, y2;
	uint32_t i;

	if (! pa || pa->npoints < 3 )
		return 0.0;

	P1 = getPoint2d_cp(pa, 0);
	P2 = getPoint2d_cp(pa, 1);
	x0 = P1->x;
	for ( i = 2; i < pa->npoints; i++ )
	{
		P3 = getPoint2d_cp(pa, i);
		x = P2->x - x0;
		y1 = P3->y;
		y2 = P1->y;
		sum += x * (y2-y1);

		/* Move forwards! */
		P1 = P2;
		P2 = P3;
	}
	return sum / 2.0;
}

int
ptarray_isccw(const POINTARRAY *pa)
{
	double area = 0;
	area = ptarray_signed_area(pa);
	if ( area > 0 ) return LW_FALSE;
	else return LW_TRUE;
}

POINTARRAY*
ptarray_force_dims(const POINTARRAY *pa, int hasz, int hasm, double zval, double mval)
{
	/* TODO handle zero-length point arrays */
	uint32_t i;
	int in_hasz = FLAGS_GET_Z(pa->flags);
	int in_hasm = FLAGS_GET_M(pa->flags);
	POINT4D pt;
	POINTARRAY *pa_out = ptarray_construct_empty(hasz, hasm, pa->npoints);

	for( i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &pt);
		if( hasz && ! in_hasz )
			pt.z = zval;
		if( hasm && ! in_hasm )
			pt.m = mval;
		ptarray_append_point(pa_out, &pt, LW_TRUE);
	}

	return pa_out;
}

POINTARRAY *
ptarray_substring(POINTARRAY *ipa, double from, double to, double tolerance)
{
	POINTARRAY *dpa;
	POINT4D pt;
	POINT4D p1, p2;
	POINT4D *p1ptr=&p1; /* don't break strict-aliasing rule */
	POINT4D *p2ptr=&p2;
	int nsegs, i;
	double length, slength, tlength;
	int state = 0; /* 0=before, 1=inside */

	/*
	 * Create a dynamic pointarray with an initial capacity
	 * equal to full copy of input points
	 */
	dpa = ptarray_construct_empty(FLAGS_GET_Z(ipa->flags), FLAGS_GET_M(ipa->flags), ipa->npoints);

	/* Compute total line length */
	length = ptarray_length_2d(ipa);


	LWDEBUGF(3, "Total length: %g", length);


	/* Get 'from' and 'to' lengths */
	from = length*from;
	to = length*to;


	LWDEBUGF(3, "From/To: %g/%g", from, to);


	tlength = 0;
	getPoint4d_p(ipa, 0, &p1);
	nsegs = ipa->npoints - 1;
	for ( i = 0; i < nsegs; i++ )
	{
		double dseg;

		getPoint4d_p(ipa, i+1, &p2);


		LWDEBUGF(3 ,"Segment %d: (%g,%g,%g,%g)-(%g,%g,%g,%g)",
		         i, p1.x, p1.y, p1.z, p1.m, p2.x, p2.y, p2.z, p2.m);


		/* Find the length of this segment */
		slength = distance2d_pt_pt((POINT2D *)p1ptr, (POINT2D *)p2ptr);

		/*
		 * We are before requested start.
		 */
		if ( state == 0 ) /* before */
		{

			LWDEBUG(3, " Before start");

			if ( fabs ( from - ( tlength + slength ) ) <= tolerance )
			{

				LWDEBUG(3, "  Second point is our start");

				/*
				 * Second point is our start
				 */
				ptarray_append_point(dpa, &p2, LW_FALSE);
				state=1; /* we're inside now */
				goto END;
			}

			else if ( fabs(from - tlength) <= tolerance )
			{

				LWDEBUG(3, "  First point is our start");

				/*
				 * First point is our start
				 */
				ptarray_append_point(dpa, &p1, LW_FALSE);

				/*
				 * We're inside now, but will check
				 * 'to' point as well
				 */
				state=1;
			}

			/*
			 * Didn't reach the 'from' point,
			 * nothing to do
			 */
			else if ( from > tlength + slength ) goto END;

			else  /* tlength < from < tlength+slength */
			{

				LWDEBUG(3, "  Seg contains first point");

				/*
				 * Our start is between first and
				 * second point
				 */
				dseg = (from - tlength) / slength;

				interpolate_point4d(&p1, &p2, &pt, dseg);

				ptarray_append_point(dpa, &pt, LW_FALSE);

				/*
				 * We're inside now, but will check
				 * 'to' point as well
				 */
				state=1;
			}
		}

		if ( state == 1 ) /* inside */
		{

			LWDEBUG(3, " Inside");

			/*
			 * 'to' point is our second point.
			 */
			if ( fabs(to - ( tlength + slength ) ) <= tolerance )
			{

				LWDEBUG(3, " Second point is our end");

				ptarray_append_point(dpa, &p2, LW_FALSE);
				break; /* substring complete */
			}

			/*
			 * 'to' point is our first point.
			 * (should only happen if 'to' is 0)
			 */
			else if ( fabs(to - tlength) <= tolerance )
			{

				LWDEBUG(3, " First point is our end");

				ptarray_append_point(dpa, &p1, LW_FALSE);

				break; /* substring complete */
			}

			/*
			 * Didn't reach the 'end' point,
			 * just copy second point
			 */
			else if ( to > tlength + slength )
			{
				ptarray_append_point(dpa, &p2, LW_FALSE);
				goto END;
			}

			/*
			 * 'to' point falls on this segment
			 * Interpolate and break.
			 */
			else if ( to < tlength + slength )
			{

				LWDEBUG(3, " Seg contains our end");

				dseg = (to - tlength) / slength;
				interpolate_point4d(&p1, &p2, &pt, dseg);

				ptarray_append_point(dpa, &pt, LW_FALSE);

				break;
			}

			else
			{
				LWDEBUG(3, "Unhandled case");
			}
		}


END:

		tlength += slength;
		memcpy(&p1, &p2, sizeof(POINT4D));
	}

	LWDEBUGF(3, "Out of loop, ptarray has %d points", dpa->npoints);

	return dpa;
}

/*
 * Write into the *ret argument coordinates of the closes point on
 * the given segment to the reference input point.
 */
void
closest_point_on_segment(const POINT4D *p, const POINT4D *A, const POINT4D *B, POINT4D *ret)
{
	double r;

	if (  FP_EQUALS(A->x, B->x) && FP_EQUALS(A->y, B->y) )
	{
		*ret = *A;
		return;
	}

	/*
	 * We use comp.graphics.algorithms Frequently Asked Questions method
	 *
	 * (1)           AC dot AB
	 *           r = ----------
	 *                ||AB||^2
	 *	r has the following meaning:
	 *	r=0 P = A
	 *	r=1 P = B
	 *	r<0 P is on the backward extension of AB
	 *	r>1 P is on the forward extension of AB
	 *	0<r<1 P is interior to AB
	 *
	 */
	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0)
	{
		*ret = *A;
		return;
	}
	if (r>1)
	{
		*ret = *B;
		return;
	}

	ret->x = A->x + ( (B->x - A->x) * r );
	ret->y = A->y + ( (B->y - A->y) * r );
	ret->z = A->z + ( (B->z - A->z) * r );
	ret->m = A->m + ( (B->m - A->m) * r );
}

/*
 * Given a point, returns the location of closest point on pointarray
 * and, optionally, it's actual distance from the point array.
 */
double
ptarray_locate_point(const POINTARRAY *pa, const POINT4D *p4d, double *mindistout, POINT4D *proj4d)
{
	double mindist=DBL_MAX;
	double tlen, plen;
	uint32_t t, seg=0;
	POINT4D	start4d, end4d, projtmp;
	POINT2D proj, p;
	const POINT2D *start = NULL, *end = NULL;

	/* Initialize our 2D copy of the input parameter */
	p.x = p4d->x;
	p.y = p4d->y;

	if ( ! proj4d ) proj4d = &projtmp;

	/* Check for special cases (length 0 and 1) */
	if ( pa->npoints <= 1 )
	{
		if ( pa->npoints == 1 )
		{
			getPoint4d_p(pa, 0, proj4d);
			if ( mindistout )
				*mindistout = distance2d_pt_pt(&p, getPoint2d_cp(pa, 0));
		}
		return 0.0;
	}

	start = getPoint2d_cp(pa, 0);
	/* Loop through pointarray looking for nearest segment */
	for (t=1; t<pa->npoints; t++)
	{
		double dist_sqr;
		end = getPoint2d_cp(pa, t);
		dist_sqr = distance2d_sqr_pt_seg(&p, start, end);

		if (dist_sqr < mindist)
		{
			mindist = dist_sqr;
			seg=t-1;
			if ( mindist == 0 )
			{
				LWDEBUG(3, "Breaking on mindist=0");
				break;
			}
		}

		start = end;
	}
	mindist = sqrt(mindist);

	if ( mindistout ) *mindistout = mindist;

	LWDEBUGF(3, "Closest segment: %d", seg);
	LWDEBUGF(3, "mindist: %g", mindist);

	/*
	 * We need to project the
	 * point on the closest segment.
	 */
	getPoint4d_p(pa, seg, &start4d);
	getPoint4d_p(pa, seg+1, &end4d);
	closest_point_on_segment(p4d, &start4d, &end4d, proj4d);

	/* Copy 4D values into 2D holder */
	proj.x = proj4d->x;
	proj.y = proj4d->y;

	LWDEBUGF(3, "Closest segment:%d, npoints:%d", seg, pa->npoints);

	/* For robustness, force 1 when closest point == endpoint */
	if ( (seg >= (pa->npoints-2)) && p2d_same(&proj, end) )
	{
		return 1.0;
	}

	LWDEBUGF(3, "Closest point on segment: %g,%g", proj.x, proj.y);

	tlen = ptarray_length_2d(pa);

	LWDEBUGF(3, "tlen %g", tlen);

	/* Location of any point on a zero-length line is 0 */
	/* See http://trac.osgeo.org/postgis/ticket/1772#comment:2 */
	if ( tlen == 0 ) return 0;

	plen=0;
	start = getPoint2d_cp(pa, 0);
	for (t=0; t<seg; t++, start=end)
	{
		end = getPoint2d_cp(pa, t+1);
		plen += distance2d_pt_pt(start, end);

		LWDEBUGF(4, "Segment %d made plen %g", t, plen);
	}

	plen+=distance2d_pt_pt(&proj, start);

	LWDEBUGF(3, "plen %g, tlen %g", plen, tlen);

	return plen/tlen;
}

/**
 * @brief Longitude shift for a pointarray.
 *  	Y remains the same
 *  	X is converted:
 *	 		from -180..180 to 0..360
 *	 		from 0..360 to -180..180
 *  	X < 0 becomes X + 360
 *  	X > 180 becomes X - 360
 */
void
ptarray_longitude_shift(POINTARRAY *pa)
{
	uint32_t i;
	double x;

	for (i=0; i<pa->npoints; i++)
	{
		memcpy(&x, getPoint_internal(pa, i), sizeof(double));
		if ( x < 0 ) x+= 360;
		else if ( x > 180 ) x -= 360;
		memcpy(getPoint_internal(pa, i), &x, sizeof(double));
	}
}


/*
 * Returns a POINTARRAY with consecutive equal points
 * removed. Equality test on all dimensions of input.
 *
 * Always returns a newly allocated object.
 */
static POINTARRAY *
ptarray_remove_repeated_points_minpoints(const POINTARRAY *in, double tolerance, int minpoints)
{
	POINTARRAY *out = ptarray_clone_deep(in);
	ptarray_remove_repeated_points_in_place(out, tolerance, minpoints);
	return out;
}

POINTARRAY *
ptarray_remove_repeated_points(const POINTARRAY *in, double tolerance)
{
	return ptarray_remove_repeated_points_minpoints(in, tolerance, 2);
}


void
ptarray_remove_repeated_points_in_place(POINTARRAY *pa, double tolerance, uint32_t min_points)
{
	uint32_t i;
	double tolsq = tolerance * tolerance;
	const POINT2D *last = NULL;
	const POINT2D *pt;
	uint32_t n_points = pa->npoints;
	uint32_t n_points_out = 1;
	size_t pt_size = ptarray_point_size(pa);

	double dsq = FLT_MAX;

	/* No-op on short inputs */
	if ( n_points <= min_points ) return;

	last = getPoint2d_cp(pa, 0);
	void *p_to = ((char *)last) + pt_size;
	for (i = 1; i < n_points; i++)
	{
		int last_point = (i == n_points - 1);

		/* Look straight into the abyss */
		pt = getPoint2d_cp(pa, i);

		/* Don't drop points if we are running short of points */
		if (n_points + n_points_out > min_points + i)
		{
			if (tolerance > 0.0)
			{
				/* Only drop points that are within our tolerance */
				dsq = distance2d_sqr_pt_pt(last, pt);
				/* Allow any point but the last one to be dropped */
				if (!last_point && dsq <= tolsq)
				{
					continue;
				}
			}
			else
			{
				/* At tolerance zero, only skip exact dupes */
				if (memcmp((char*)pt, (char*)last, pt_size) == 0)
					continue;
			}

			/* Got to last point, and it's not very different from */
			/* the point that preceded it. We want to keep the last */
			/* point, not the second-to-last one, so we pull our write */
			/* index back one value */
			if (last_point && n_points_out > 1 && tolerance > 0.0 && dsq <= tolsq)
			{
				n_points_out--;
				p_to = (char*)p_to - pt_size;
			}
		}

		/* Compact all remaining values to front of array */
		memcpy(p_to, pt, pt_size);
		n_points_out++;
		p_to = (char*)p_to + pt_size;
		last = pt;
	}
	/* Adjust array length */
	pa->npoints = n_points_out;
	return;
}

/* Out of the points in pa [itfist .. itlast], finds the one that's farthest away from
 * the segment determined by pts[itfist] and pts[itlast].
 * Returns itfirst if no point was found futher away than max_distance_sqr
 */
static uint32_t
ptarray_dp_findsplit_in_place(const POINTARRAY *pts, uint32_t it_first, uint32_t it_last, double max_distance_sqr)
{
	uint32_t split = it_first;
	if ((it_first - it_last) < 2)
		return it_first;

	const POINT2D *A = getPoint2d_cp(pts, it_first);
	const POINT2D *B = getPoint2d_cp(pts, it_last);

	if (distance2d_sqr_pt_pt(A, B) < DBL_EPSILON)
	{
		/* If p1 == p2, we can just calculate the distance from each point to A */
		for (uint32_t itk = it_first + 1; itk < it_last; itk++)
		{
			const POINT2D *pk = getPoint2d_cp(pts, itk);
			double distance_sqr = distance2d_sqr_pt_pt(pk, A);
			if (distance_sqr > max_distance_sqr)
			{
				split = itk;
				max_distance_sqr = distance_sqr;
			}
		}
		return split;
	}

	/* This is based on distance2d_sqr_pt_seg, but heavily inlined here to avoid recalculations */
	double ba_x = (B->x - A->x);
	double ba_y = (B->y - A->y);
	double ab_length_sqr = (ba_x * ba_x + ba_y * ba_y);
	/* To avoid the division by ab_length_sqr in the 3rd path, we normalize here
	 * and multiply in the first two paths [(dot_ac_ab < 0) and (> ab_length_sqr)] */
	max_distance_sqr *= ab_length_sqr;
	for (uint32_t itk = it_first + 1; itk < it_last; itk++)
	{
		const POINT2D *C = getPoint2d_cp(pts, itk);
		double distance_sqr;
		double ca_x = (C->x - A->x);
		double ca_y = (C->y - A->y);
		double dot_ac_ab = (ca_x * ba_x + ca_y * ba_y);

		if (dot_ac_ab <= 0.0)
		{
			distance_sqr = distance2d_sqr_pt_pt(C, A) * ab_length_sqr;
		}
		else if (dot_ac_ab >= ab_length_sqr)
		{
			distance_sqr = distance2d_sqr_pt_pt(C, B) * ab_length_sqr;
		}
		else
		{
			double s_numerator = ca_x * ba_y - ca_y * ba_x;
			distance_sqr = s_numerator * s_numerator; /* Missing division by ab_length_sqr on purpose */
		}

		if (distance_sqr > max_distance_sqr)
		{
			split = itk;
			max_distance_sqr = distance_sqr;
		}
	}
	return split;
}

/* O(N) simplification for tolearnce = 0 */
static void
ptarray_simplify_in_place_tolerance0(POINTARRAY *pa)
{
	uint32_t kept_it = 0;
	uint32_t last_it = pa->npoints - 1;
	const POINT2D *kept_pt = getPoint2d_cp(pa, 0);
	const size_t pt_size = ptarray_point_size(pa);

	for (uint32_t i = 1; i < last_it; i++)
	{
		const POINT2D *curr_pt = getPoint2d_cp(pa, i);
		const POINT2D *next_pt = getPoint2d_cp(pa, i + 1);

		double ba_x = next_pt->x - kept_pt->x;
		double ba_y = next_pt->y - kept_pt->y;
		double ab_length_sqr = ba_x * ba_x + ba_y * ba_y;

		double ca_x = curr_pt->x - kept_pt->x;
		double ca_y = curr_pt->y - kept_pt->y;
		double dot_ac_ab = ca_x * ba_x + ca_y * ba_y;
		double s_numerator = ca_x * ba_y - ca_y * ba_x;

		if (dot_ac_ab < 0.0 || dot_ac_ab > ab_length_sqr || s_numerator != 0)
		{
			kept_it++;
			kept_pt = curr_pt;
			if (kept_it != i)
				memcpy(pa->serialized_pointlist + pt_size * kept_it,
				       pa->serialized_pointlist + pt_size * i,
				       pt_size);
		}
	}

	/* Append last point */
	kept_it++;
	if (kept_it != last_it)
		memcpy(pa->serialized_pointlist + pt_size * kept_it,
		       pa->serialized_pointlist + pt_size * last_it,
		       pt_size);
	pa->npoints = kept_it + 1;
}

void
ptarray_simplify_in_place(POINTARRAY *pa, double tolerance, uint32_t minpts)
{
	/* Do not try to simplify really short things */
	if (pa->npoints < 3 || pa->npoints <= minpts)
		return;

	if (tolerance == 0 && minpts <= 2)
	{
		ptarray_simplify_in_place_tolerance0(pa);
		return;
	}

	/* We use this array to keep track of the points we are keeping, so
	 * we store just TRUE / FALSE in their position */
	uint8_t *kept_points = lwalloc(sizeof(uint8_t) * pa->npoints);
	memset(kept_points, LW_FALSE, sizeof(uint8_t) * pa->npoints);
	kept_points[0] = LW_TRUE;
	kept_points[pa->npoints - 1] = LW_TRUE;
	uint32_t keptn = 2;

	/* We use this array as a stack to store the iterators that we are going to need
	 * in the following steps.
	 * This is ~10% faster than iterating over @kept_points looking for them
	 */
	uint32_t *iterator_stack = lwalloc(sizeof(uint32_t) * pa->npoints);
	iterator_stack[0] = 0;
	uint32_t iterator_stack_size = 1;

	uint32_t it_first = 0;
	uint32_t it_last = pa->npoints - 1;

	const double tolerance_sqr = tolerance * tolerance;
	/* For the first @minpts points we ignore the tolerance */
	double it_tol = keptn >= minpts ? tolerance_sqr : -1.0;

	while (iterator_stack_size)
	{
		uint32_t split = ptarray_dp_findsplit_in_place(pa, it_first, it_last, it_tol);
		if (split == it_first)
		{
			it_first = it_last;
			it_last = iterator_stack[--iterator_stack_size];
		}
		else
		{
			kept_points[split] = LW_TRUE;
			keptn++;

			iterator_stack[iterator_stack_size++] = it_last;
			it_last = split;
			it_tol = keptn >= minpts ? tolerance_sqr : -1.0;
		}
	}

	const size_t pt_size = ptarray_point_size(pa);
	/* The first point is already in place, so we don't need to copy it */
	size_t kept_it = 1;
	if (keptn == 2)
	{
		/* If there are 2 points remaining, it has to be first and last as
		 * we added those at the start */
		memcpy(pa->serialized_pointlist + pt_size * kept_it,
		       pa->serialized_pointlist + pt_size * (pa->npoints - 1),
		       pt_size);
	}
	else if (pa->npoints != keptn) /* We don't need to move any points if we are keeping them all */
	{
		for (uint32_t i = 1; i < pa->npoints; i++)
		{
			if (kept_points[i])
			{
				memcpy(pa->serialized_pointlist + pt_size * kept_it,
				       pa->serialized_pointlist + pt_size * i,
				       pt_size);
				kept_it++;
			}
		}
	}
	pa->npoints = keptn;

	lwfree(kept_points);
	lwfree(iterator_stack);
}

/************************************************************************/

/**
* Find the 2d length of the given #POINTARRAY, using circular
* arc interpolation between each coordinate triple.
* Length(A1, A2, A3, A4, A5) = Length(A1, A2, A3)+Length(A3, A4, A5)
*/
double
ptarray_arc_length_2d(const POINTARRAY *pts)
{
	double dist = 0.0;
	uint32_t i;
	const POINT2D *a1;
	const POINT2D *a2;
	const POINT2D *a3;

	if ( pts->npoints % 2 != 1 )
		lwerror("arc point array with even number of points");

	a1 = getPoint2d_cp(pts, 0);

	for ( i=2; i < pts->npoints; i += 2 )
	{
		a2 = getPoint2d_cp(pts, i-1);
		a3 = getPoint2d_cp(pts, i);
		dist += lw_arc_length(a1, a2, a3);
		a1 = a3;
	}
	return dist;
}

/**
* Find the 2d length of the given #POINTARRAY (even if it's 3d)
*/
double
ptarray_length_2d(const POINTARRAY *pts)
{
	double dist = 0.0;
	uint32_t i;
	const POINT2D *frm;
	const POINT2D *to;

	if ( pts->npoints < 2 ) return 0.0;

	frm = getPoint2d_cp(pts, 0);

	for ( i=1; i < pts->npoints; i++ )
	{
		to = getPoint2d_cp(pts, i);

		dist += sqrt( ((frm->x - to->x)*(frm->x - to->x))  +
		              ((frm->y - to->y)*(frm->y - to->y)) );

		frm = to;
	}
	return dist;
}

/**
* Find the 3d/2d length of the given #POINTARRAY
* (depending on its dimensionality)
*/
double
ptarray_length(const POINTARRAY *pts)
{
	double dist = 0.0;
	uint32_t i;
	POINT3DZ frm;
	POINT3DZ to;

	if ( pts->npoints < 2 ) return 0.0;

	/* compute 2d length if 3d is not available */
	if ( ! FLAGS_GET_Z(pts->flags) ) return ptarray_length_2d(pts);

	getPoint3dz_p(pts, 0, &frm);
	for ( i=1; i < pts->npoints; i++ )
	{
		getPoint3dz_p(pts, i, &to);
		dist += sqrt( ((frm.x - to.x)*(frm.x - to.x)) +
		              ((frm.y - to.y)*(frm.y - to.y)) +
		              ((frm.z - to.z)*(frm.z - to.z)) );
		frm = to;
	}
	return dist;
}



/**
 * Affine transform a pointarray.
 */
void
ptarray_affine(POINTARRAY *pa, const AFFINE *a)
{
	if (FLAGS_GET_Z(pa->flags))
	{
		for (uint32_t i = 0; i < pa->npoints; i++)
		{
			POINT4D *p4d = (POINT4D *)(getPoint_internal(pa, i));
			double x = p4d->x;
			double y = p4d->y;
			double z = p4d->z;
			p4d->x = a->afac * x + a->bfac * y + a->cfac * z + a->xoff;
			p4d->y = a->dfac * x + a->efac * y + a->ffac * z + a->yoff;
			p4d->z = a->gfac * x + a->hfac * y + a->ifac * z + a->zoff;
		}
	}
	else
	{
		for (uint32_t i = 0; i < pa->npoints; i++)
		{
			POINT2D *pt = (POINT2D *)(getPoint_internal(pa, i));
			double x = pt->x;
			double y = pt->y;
			pt->x = a->afac * x + a->bfac * y + a->xoff;
			pt->y = a->dfac * x + a->efac * y + a->yoff;
		}
	}
}

/**
* WARNING, make sure you send in only 16-member double arrays
* or obviously things will go pear-shaped fast.
*/
#if 0
static int gluInvertMatrix(const double *m, double *invOut)
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
              m[4]  * m[11] * m[14] +
              m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] -
              m[12] * m[6]  * m[11] +
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] -
               m[12] * m[5] * m[10] +
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
              m[1]  * m[11] * m[14] +
              m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] -
              m[13] * m[2] * m[11] +
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
              m[0]  * m[11] * m[13] +
              m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] -
              m[12] * m[1] * m[11] +
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
              m[0]  * m[7] * m[14] +
              m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] -
              m[12] * m[2] * m[7] +
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
               m[0]  * m[6] * m[13] +
               m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] -
               m[12] * m[1] * m[6] +
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
              m[1] * m[7] * m[10] +
              m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] -
              m[9] * m[2] * m[7] +
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
               m[0] * m[7] * m[9] +
               m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] -
               m[8] * m[1] * m[7] +
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return LW_FALSE;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return LW_TRUE;
}
#endif

/**
 * Scale a pointarray.
 */
void
ptarray_scale(POINTARRAY *pa, const POINT4D *fact)
{
	uint32_t i;
	POINT4D p4d;
	LWDEBUG(3, "ptarray_scale start");
	for (i=0; i<pa->npoints; i++)
	{
		getPoint4d_p(pa, i, &p4d);
		p4d.x *= fact->x;
		p4d.y *= fact->y;
		p4d.z *= fact->z;
		p4d.m *= fact->m;
		ptarray_set_point4d(pa, i, &p4d);
	}
	LWDEBUG(3, "ptarray_scale end");
}

int
ptarray_startpoint(const POINTARRAY *pa, POINT4D *pt)
{
	return getPoint4d_p(pa, 0, pt);
}


/*
 * Stick an array of points to the given gridspec.
 * Return "gridded" points in *outpts and their number in *outptsn.
 *
 * Two consecutive points falling on the same grid cell are collapsed
 * into one single point.
 *
 */
void
ptarray_grid_in_place(POINTARRAY *pa, const gridspec *grid)
{
	uint32_t j = 0;
	POINT4D *p, *p_out = NULL;
	double x, y, z = 0, m = 0;
	uint32_t ndims = FLAGS_NDIMS(pa->flags);
	uint32_t has_z = FLAGS_GET_Z(pa->flags);
	uint32_t has_m = FLAGS_GET_M(pa->flags);

	for (uint32_t i = 0; i < pa->npoints; i++)
	{
		/* Look straight into the abyss */
		p = (POINT4D *)(getPoint_internal(pa, i));
		x = p->x;
		y = p->y;
		if (ndims > 2)
			z = p->z;
		if (ndims > 3)
			m = p->m;

		if (grid->xsize > 0)
			x = rint((x - grid->ipx) / grid->xsize) * grid->xsize + grid->ipx;

		if (grid->ysize > 0)
			y = rint((y - grid->ipy) / grid->ysize) * grid->ysize + grid->ipy;

		/* Read and round this point */
		/* Z is always in third position */
		if (has_z && grid->zsize > 0)
			z = rint((z - grid->ipz) / grid->zsize) * grid->zsize + grid->ipz;

		/* M might be in 3rd or 4th position */
		if (has_m && grid->msize > 0)
		{
			/* In POINT ZM, M is in 4th position, in POINT M, M is in 3rd position which is Z in POINT4D */
			if (has_z)
				m = rint((m - grid->ipm) / grid->msize) * grid->msize + grid->ipm;
			else
				z = rint((z - grid->ipm) / grid->msize) * grid->msize + grid->ipm;
		}

		/* Skip duplicates */
		if (p_out && p_out->x == x && p_out->y == y && (ndims > 2 ? p_out->z == z : 1) &&
		    (ndims > 3 ? p_out->m == m : 1))
			continue;

		/* Write rounded values into the next available point */
		p_out = (POINT4D *)(getPoint_internal(pa, j++));
		p_out->x = x;
		p_out->y = y;
		if (ndims > 2)
			p_out->z = z;
		if (ndims > 3)
			p_out->m = m;
	}

	/* Update output ptarray length */
	pa->npoints = j;
	return;
}

int
ptarray_npoints_in_rect(const POINTARRAY *pa, const GBOX *gbox)
{
	const POINT2D *pt;
	int n = 0;
	uint32_t i;
	for ( i = 0; i < pa->npoints; i++ )
	{
		pt = getPoint2d_cp(pa, i);
		if ( gbox_contains_point2d(gbox, pt) )
			n++;
	}
	return n;
}


