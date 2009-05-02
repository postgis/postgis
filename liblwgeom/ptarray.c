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

#include <stdio.h>
#include <string.h>

#include "liblwgeom.h"


POINTARRAY *
ptarray_construct(char hasz, char hasm, unsigned int npoints)
{
	uchar dims = 0;
	size_t size;
	uchar *ptlist;
	POINTARRAY *pa;

	TYPE_SETZM(dims, hasz?1:0, hasm?1:0);
	size = TYPE_NDIMS(dims)*npoints*sizeof(double);

	ptlist = (uchar *)lwalloc(size);
	pa = lwalloc(sizeof(POINTARRAY));
	pa->dims = dims;
	pa->serialized_pointlist = ptlist;
	pa->npoints = npoints;

	return pa;

}

void ptarray_free(POINTARRAY *pa)
{
	/*!
	*  	TODO: \todo	Turn this on after retrofitting all calls to lwfree_ in /lwgeom
	*		if( pa->serialized_pointlist )
	*		lwfree(pa->serialized_pointlist);
  */

	lwfree(pa);
}

void
ptarray_reverse(POINTARRAY *pa)
{
	POINT4D pbuf;
	uint32 i;
	int ptsize = pointArray_ptsize(pa);
	int last = pa->npoints-1;
	int mid = last/2;

	for (i=0; i<=mid; i++)
	{
		uchar *from, *to;
		from = getPoint_internal(pa, i);
		to = getPoint_internal(pa, (last-i));
		memcpy((uchar *)&pbuf, to, ptsize);
		memcpy(to, from, ptsize);
		memcpy(from, (uchar *)&pbuf, ptsize);
	}

}

/*!
 * \brief calculate the 2d bounding box of a set of points
 * 		write result to the provided BOX2DFLOAT4
 * 		Return 0 if bounding box is NULL (empty geom)
 */
int
ptarray_compute_box2d_p(const POINTARRAY *pa, BOX2DFLOAT4 *result)
{
	int t;
	POINT2D pt;
	BOX3D box;

	if (pa->npoints == 0) return 0;

	getPoint2d_p(pa, 0, &pt);

	box.xmin = pt.x;
	box.xmax = pt.x;
	box.ymin = pt.y;
	box.ymax = pt.y;

	for (t=1; t<pa->npoints; t++)
	{
		getPoint2d_p(pa, t, &pt);
		if (pt.x < box.xmin) box.xmin = pt.x;
		if (pt.y < box.ymin) box.ymin = pt.y;
		if (pt.x > box.xmax) box.xmax = pt.x;
		if (pt.y > box.ymax) box.ymax = pt.y;
	}

	box3d_to_box2df_p(&box, result);

	return 1;
}

/*!
 * \brief Calculate the 2d bounding box of a set of points.
 * \return allocated #BOX2DFLOAT4 or NULL (for empty array).
 */
BOX2DFLOAT4 *
ptarray_compute_box2d(const POINTARRAY *pa)
{
	int t;
	POINT2D pt;
	BOX2DFLOAT4 *result;

	if (pa->npoints == 0) return NULL;

	result = lwalloc(sizeof(BOX2DFLOAT4));

	getPoint2d_p(pa, 0, &pt);

	result->xmin = pt.x;
	result->xmax = pt.x;
	result->ymin = pt.y;
	result->ymax = pt.y;

	for (t=1;t<pa->npoints;t++)
	{
		getPoint2d_p(pa, t, &pt);
		if (pt.x < result->xmin) result->xmin = pt.x;
		if (pt.y < result->ymin) result->ymin = pt.y;
		if (pt.x > result->xmax) result->xmax = pt.x;
		if (pt.y > result->ymax) result->ymax = pt.y;
	}

	return result;
}

/*!
 * \brief Returns a modified #POINTARRAY so that no segment is
 * 		longer than the given distance (computed using 2d).
 *
 * Every input point is kept.
 * Z and M values for added points (if needed) are set to 0.
 */
POINTARRAY *
ptarray_segmentize2d(POINTARRAY *ipa, double dist)
{
	double	segdist;
	POINT4D	p1, p2;
	void *ip, *op;
	POINT4D pbuf;
	POINTARRAY *opa;
	int maxpoints = ipa->npoints;
	int ptsize = pointArray_ptsize(ipa);
	int ipoff=0; /* input point offset */

	pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0;

	/* Initial storage */
	opa = (POINTARRAY *)lwalloc(ptsize * maxpoints);
	opa->dims = ipa->dims;
	opa->npoints = 0;
	opa->serialized_pointlist = (uchar *)lwalloc(maxpoints*ptsize);

	/* Add first point */
	opa->npoints++;
	getPoint4d_p(ipa, ipoff, &p1);
	op = getPoint_internal(opa, opa->npoints-1);
	memcpy(op, &p1, ptsize);
	ipoff++;

	while (ipoff<ipa->npoints)
	{
		/*
		 * We use these pointers to avoid
		 * "strict-aliasing rules break" warning raised
		 * by gcc (3.3 and up).
		 *
		 * It looks that casting a variable address (also
		 * referred to as "type-punned pointer")
		 * breaks those "strict" rules.
		 *
		 */
		POINT4D *p1ptr=&p1, *p2ptr=&p2;

		getPoint4d_p(ipa, ipoff, &p2);

		segdist = distance2d_pt_pt((POINT2D *)p1ptr, (POINT2D *)p2ptr);

		if (segdist > dist) /* add an intermediate point */
		{
			pbuf.x = p1.x + (p2.x-p1.x)/segdist * dist;
			pbuf.y = p1.y + (p2.y-p1.y)/segdist * dist;
			/* might also compute z and m if available... */
			ip = &pbuf;
			memcpy(&p1, ip, ptsize);
		}
		else /* copy second point */
		{
			ip = &p2;
			p1 = p2;
			ipoff++;
		}

		/* Add point */
		if ( ++(opa->npoints) > maxpoints ) {
			maxpoints *= 1.5;
			opa->serialized_pointlist = (uchar *)lwrealloc(
				opa->serialized_pointlist,
				maxpoints*ptsize
			);
		}
		op = getPoint_internal(opa, opa->npoints-1);
		memcpy(op, ip, ptsize);
	}

	return opa;
}

char
ptarray_same(const POINTARRAY *pa1, const POINTARRAY *pa2)
{
	unsigned int i;
	size_t ptsize;

	if ( TYPE_GETZM(pa1->dims) != TYPE_GETZM(pa2->dims) ) return 0;

	if ( pa1->npoints != pa2->npoints ) return 0;

	ptsize = pointArray_ptsize(pa1);

	for (i=0; i<pa1->npoints; i++)
	{
		if ( memcmp(getPoint_internal(pa1, i), getPoint_internal(pa2, i), ptsize) )
			return 0;
	}

	return 1;
}

/*!
 * \brief Add a point in a pointarray.
 * 		'where' is the offset (starting at 0)
 * 		if 'where' == -1 append is required.
 */
POINTARRAY *
ptarray_addPoint(POINTARRAY *pa, uchar *p, size_t pdims, unsigned int where)
{
	POINTARRAY *ret;
	POINT4D pbuf;
	size_t ptsize = pointArray_ptsize(pa);

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
	memcpy((uchar *)&pbuf, p, pdims*sizeof(double));

	LWDEBUG(3, "initialized point buffer");

	ret = ptarray_construct(TYPE_HASZ(pa->dims),
		TYPE_HASM(pa->dims), pa->npoints+1);

	if ( where == -1 ) where = pa->npoints;

	if ( where )
	{
		memcpy(getPoint_internal(ret, 0), getPoint_internal(pa, 0), ptsize*where);
	}

	memcpy(getPoint_internal(ret, where), (uchar *)&pbuf, ptsize);

	if ( where+1 != ret->npoints )
	{
		memcpy(getPoint_internal(ret, where+1),
			getPoint_internal(pa, where),
			ptsize*(pa->npoints-where));
	}

	return ret;
}

/*!
 * \brief Remove a point from a pointarray.
 * 	\param which -  is the offset (starting at 0)
 * \return #POINTARRAY is newly allocated
 */
POINTARRAY *
ptarray_removePoint(POINTARRAY *pa, unsigned int which)
{
	POINTARRAY *ret;
	size_t ptsize = pointArray_ptsize(pa);

	LWDEBUGF(3, "pa %x which %d", pa, which);

#if PARANOIA_LEVEL > 0
	if ( which > pa->npoints-1 )
	{
		lwerror("ptarray_removePoint: offset (%d) out of range (%d..%d)",
			which, 0, pa->npoints-1);
		return NULL;
	}

	if ( pa->npoints < 3 )
	{
		lwerror("ptarray_removePointe: can't remove a point from a 2-vertex POINTARRAY");
	}
#endif

	ret = ptarray_construct(TYPE_HASZ(pa->dims),
		TYPE_HASM(pa->dims), pa->npoints-1);

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

/*!
 * \brief Clone a pointarray
 */
POINTARRAY *
ptarray_clone(const POINTARRAY *in)
{
	POINTARRAY *out = lwalloc(sizeof(POINTARRAY));
	size_t size;

		LWDEBUG(3, "ptarray_clone called.");

	out->dims = in->dims;
	out->npoints = in->npoints;

	size = in->npoints*sizeof(double)*TYPE_NDIMS(in->dims);
	out->serialized_pointlist = lwalloc(size);
	memcpy(out->serialized_pointlist, in->serialized_pointlist, size);

	return out;
}

int
ptarray_isclosed2d(const POINTARRAY *in)
{
	if ( memcmp(getPoint_internal(in, 0), getPoint_internal(in, in->npoints-1), sizeof(POINT2D)) ) return 0;
	return 1;
}

/*!
 * \brief calculate the #BOX3D bounding box of a set of points
 * \return  a lwalloced #BOX3D, or NULL on empty array.
 * 		#zmin / #zmax values are set to #NO_Z_VALUE if not available.
 */
BOX3D *
ptarray_compute_box3d(const POINTARRAY *pa)
{
	BOX3D *result = lwalloc(sizeof(BOX3D));

	if ( ! ptarray_compute_box3d_p(pa, result) )
	{
		lwfree(result);
		return NULL;
	}

	return result;
}

/*!
 * \brief calculate the #BOX3D bounding box of a set of points
 * 		zmin/zmax values are set to #NO_Z_VALUE if not available.
 * write result to the provided #BOX3D
 * \return 0 if bounding box is NULL (empty geom) and 1 otherwise
 */
int
ptarray_compute_box3d_p(const POINTARRAY *pa, BOX3D *result)
{
	int t;
	POINT3DZ pt;

	LWDEBUGF(3, "ptarray_compute_box3d call (array has %d points)", pa->npoints);

	if (pa->npoints == 0) return 0;

	getPoint3dz_p(pa, 0, &pt);

	LWDEBUG(3, "got point 0");

	result->xmin = pt.x;
	result->xmax = pt.x;
	result->ymin = pt.y;
	result->ymax = pt.y;

	if ( TYPE_HASZ(pa->dims) ) {
		result->zmin = pt.z;
		result->zmax = pt.z;
	} else {
		result->zmin = NO_Z_VALUE;
		result->zmax = NO_Z_VALUE;
	}

	LWDEBUGF(3, "scanning other %d points", pa->npoints);

	for (t=1; t<pa->npoints; t++)
	{
		getPoint3dz_p(pa,t,&pt);
		if (pt.x < result->xmin) result->xmin = pt.x;
		if (pt.y < result->ymin) result->ymin = pt.y;
		if (pt.x > result->xmax) result->xmax = pt.x;
		if (pt.y > result->ymax) result->ymax = pt.y;

		if ( TYPE_HASZ(pa->dims) ) {
			if (pt.z > result->zmax) result->zmax = pt.z;
			if (pt.z < result->zmin) result->zmin = pt.z;
		}
	}

	LWDEBUG(3, "returning box");

	return 1;
}

/*!
 * TODO: \todo implement point interpolation
 */
POINTARRAY *
ptarray_substring(POINTARRAY *ipa, double from, double to)
{
	DYNPTARRAY *dpa;
	POINTARRAY *opa;
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
	dpa = dynptarray_create(ipa->npoints, ipa->dims);

	/* Compute total line length */
	length = lwgeom_pointarray_length2d(ipa);


	LWDEBUGF(3, "Total length: %g", length);


	/* Get 'from' and 'to' lengths */
	from = length*from;
	to = length*to;


	LWDEBUGF(3, "From/To: %g/%g", from, to);


	tlength = 0;
	getPoint4d_p(ipa, 0, &p1);
	nsegs = ipa->npoints - 1;
	for( i = 0; i < nsegs; i++ )
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

			/*
			 * Didn't reach the 'from' point,
			 * nothing to do
			 */
			if ( from > tlength + slength ) goto END;

			else if ( from == tlength + slength )
			{

				LWDEBUG(3, "  Second point is our start");

				/*
				 * Second point is our start
				 */
				dynptarray_addPoint4d(dpa, &p2, 1);
				state=1; /* we're inside now */
				goto END;
			}

			else if ( from == tlength )
			{

				LWDEBUG(3, "  First point is our start");

				/*
				 * First point is our start
				 */
				dynptarray_addPoint4d(dpa, &p1, 1);

				/*
				 * We're inside now, but will check
				 * 'to' point as well
				 */
				state=1;
			}

			else  /* tlength < from < tlength+slength */
			{

				LWDEBUG(3, "  Seg contains first point");

				/*
				 * Our start is between first and
				 * second point
				 */
				dseg = (from - tlength) / slength;
				interpolate_point4d(&p1, &p2, &pt, dseg);

				dynptarray_addPoint4d(dpa, &pt, 1);

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
			 * Didn't reach the 'end' point,
			 * just copy second point
			 */
			if ( to > tlength + slength )
			{
				dynptarray_addPoint4d(dpa, &p2, 0);
				goto END;
			}

			/*
			 * 'to' point is our second point.
			 */
			else if ( to == tlength + slength )
			{

				LWDEBUG(3, " Second point is our end");

				dynptarray_addPoint4d(dpa, &p2, 0);
				break; /* substring complete */
			}

			/*
			 * 'to' point is our first point.
			 * (should only happen if 'to' is 0)
			 */
			else if ( to == tlength )
			{

				LWDEBUG(3, " First point is our end");

				dynptarray_addPoint4d(dpa, &p1, 0);

				break; /* substring complete */
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

				dynptarray_addPoint4d(dpa, &pt, 0);

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

	/* Get constructed pointarray and release memory associated
	 * with the dynamic pointarray
	 */
	opa = dpa->pa;
	lwfree(dpa);

	LWDEBUGF(3, "Out of loop, ptarray has %d points", opa->npoints);

	return opa;
}

/*
 * Write into the *ret argument coordinates of the closes point on
 * the given segment to the reference input point.
 */
void
closest_point_on_segment(POINT2D *p, POINT2D *A, POINT2D *B, POINT2D *ret)
{
	double r;

	if (  ( A->x == B->x) && (A->y == B->y) )
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

	if (r<0) {
		*ret = *A; return;
	}
	if (r>1) {
		*ret = *B;
		return;
	}

	ret->x = A->x + ( (B->x - A->x) * r );
	ret->y = A->y + ( (B->y - A->y) * r );
}

/*
 * Given a point, returns the location of closest point on pointarray
 */
double
ptarray_locate_point(POINTARRAY *pa, POINT2D *p)
{
	double mindist=-1;
	double tlen, plen;
	int t, seg=-1;
	POINT2D	start, end;
	POINT2D proj;

	getPoint2d_p(pa, 0, &start);
	for (t=1; t<pa->npoints; t++)
	{
		double dist;
		getPoint2d_p(pa, t, &end);
		dist = distance2d_pt_seg(p, &start, &end);

		if (t==1 || dist < mindist ) {
			mindist = dist;
			seg=t-1;
		}

		if ( mindist == 0 ) break;

		start = end;
	}

	LWDEBUGF(3, "Closest segment: %d", seg);

	/*
	 * If mindist is not 0 we need to project the
	 * point on the closest segment.
	 */
	if ( mindist > 0 )
	{
		getPoint2d_p(pa, seg, &start);
		getPoint2d_p(pa, seg+1, &end);
		closest_point_on_segment(p, &start, &end, &proj);
	} else {
		proj = *p;
	}

	LWDEBUGF(3, "Closest point on segment: %g,%g", proj.x, proj.y);

	tlen = lwgeom_pointarray_length2d(pa);

	LWDEBUGF(3, "tlen %g", tlen);

	plen=0;
	getPoint2d_p(pa, 0, &start);
	for (t=0; t<seg; t++, start=end)
	{
		getPoint2d_p(pa, t+1, &end);
		plen += distance2d_pt_pt(&start, &end);

		LWDEBUGF(4, "Segment %d made plen %g", t, plen);
	}

	plen+=distance2d_pt_pt(&proj, &start);

	LWDEBUGF(3, "plen %g, tlen %g", plen, tlen);
	LWDEBUGF(3, "mindist: %g", mindist);

	return plen/tlen;
}

/*!
 * \brief Longitude shift for a pointarray.
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
	int i;
	double x;

	for (i=0; i<pa->npoints; i++) {
		memcpy(&x, getPoint_internal(pa, i), sizeof(double));
		if ( x < 0 ) x+= 360;
		else if ( x > 180 ) x -= 360;
		memcpy(getPoint_internal(pa, i), &x, sizeof(double));
	}
}

DYNPTARRAY *
dynptarray_create(size_t initial_capacity, int dims)
{
	DYNPTARRAY *ret=lwalloc(sizeof(DYNPTARRAY));

		LWDEBUGF(3, "dynptarray_create called, dims=%d.", dims);

	if ( initial_capacity < 1 ) initial_capacity=1;

	ret->pa=lwalloc(sizeof(POINTARRAY));
	ret->pa->dims=dims;
	ret->ptsize=pointArray_ptsize(ret->pa);
	ret->capacity=initial_capacity;
	ret->pa->serialized_pointlist=lwalloc(ret->ptsize*ret->capacity);
	ret->pa->npoints=0;

	return ret;
}

/*!
 * \brief Add a #POINT4D to the dynamic pointarray.
 *
 * The dynamic pointarray may be of any dimension, only
 * accepted dimensions will be copied.
 *
 * If allow_duplicates is set to 0 (false) a check
 * is performed to see if last point in array is equal to the
 * provided one. NOTE that the check is 4d based, with missing
 * ordinates in the pointarray set to #NO_Z_VALUE and #NO_M_VALUE
 * respectively.
 */
int
dynptarray_addPoint4d(DYNPTARRAY *dpa, POINT4D *p4d, int allow_duplicates)
{
	POINTARRAY *pa=dpa->pa;
	POINT4D tmp;

		LWDEBUG(3, "dynptarray_addPoint4d called.");

	if ( ! allow_duplicates && pa->npoints > 0 )
	{
		getPoint4d_p(pa, pa->npoints-1, &tmp);

		/*
		 * return 0 and do nothing else if previous point in list is
		 * equal to this one  (4D equality)
		 */
		if (tmp.x == p4d->x && tmp.y == p4d->y && tmp.z == p4d->z && tmp.m == p4d->m) return 0;
	}

	++pa->npoints;
	if ( pa->npoints > dpa->capacity )
	{
		dpa->capacity*=2;
		pa->serialized_pointlist = lwrealloc(
			pa->serialized_pointlist,
			dpa->capacity*dpa->ptsize);
	}

	setPoint4d(pa, pa->npoints-1, p4d);

	return 1;
}

