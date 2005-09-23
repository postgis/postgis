#include <stdio.h>
#include <string.h>

#include "liblwgeom.h"

//#define PGIS_DEBUG 1

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

//POINTARRAY *
//ptarray_construct2d(uint32 npoints, const POINT2D *pts)
//{
//	POINTARRAY *pa = ptarray_construct(0, 0, npoints);
//	uint32 i;
//
//	for (i=0; i<npoints; i++)
//	{
//		POINT2D *pap = (POINT2D *)getPoint(pa, i);
//		pap->x = pts[i].x;
//		pap->y = pts[i].y;
//	}
//	
//	return pa;
//}
//
//POINTARRAY *
//ptarray_construct3dz(uint32 npoints, const POINT3DZ *pts)
//{
//	POINTARRAY *pa = ptarray_construct(1, 0, npoints);
//	uint32 i;
//
//	for (i=0; i<npoints; i++)
//	{
//		POINT3DZ *pap = (POINT3DZ *)getPoint(pa, i);
//		pap->x = pts[i].x;
//		pap->y = pts[i].y;
//		pap->z = pts[i].z;
//	}
//	
//	return pa;
//}
//
//POINTARRAY *
//ptarray_construct3dm(uint32 npoints, const POINT3DM *pts)
//{
//	POINTARRAY *pa = ptarray_construct(0, 1, npoints);
//	uint32 i;
//
//	for (i=0; i<npoints; i++)
//	{
//		POINT3DM *pap = (POINT3DM *)getPoint(pa, i);
//		pap->x = pts[i].x;
//		pap->y = pts[i].y;
//		pap->m = pts[i].m;
//	}
//	
//	return pa;
//}
//
//POINTARRAY *
//ptarray_construct4d(uint32 npoints, const POINT4D *pts)
//{
//	POINTARRAY *pa = ptarray_construct(0, 1, npoints);
//	uint32 i;
//
//	for (i=0; i<npoints; i++)
//	{
//		POINT4D *pap = (POINT4D *)getPoint(pa, i);
//		pap->x = pts[i].x;
//		pap->y = pts[i].y;
//		pap->z = pts[i].z;
//		pap->m = pts[i].m;
//	}
//	
//	return pa;
//}

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

/*
 * calculate the 2d bounding box of a set of points
 * write result to the provided BOX2DFLOAT4
 * Return 0 if bounding box is NULL (empty geom)
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

// calculate the 2d bounding box of a set of points
// return allocated BOX2DFLOAT4 or NULL (for empty array)
BOX2DFLOAT4 *
ptarray_compute_box2d(const POINTARRAY *pa)
{
	int t;
	POINT2D pt;
	BOX2DFLOAT4 *result;

	if (pa->npoints == 0) return NULL;

	result = lwalloc(sizeof(BOX2DFLOAT4));

	//pt = (POINT2D *)getPoint(pa, 0);
	getPoint2d_p(pa, 0, &pt);

	result->xmin = pt.x;
	result->xmax = pt.x;
	result->ymin = pt.y;
	result->ymax = pt.y;

	for (t=1;t<pa->npoints;t++)
	{
		//pt = (POINT2D *)getPoint(pa, t);
		getPoint2d_p(pa, t, &pt);
		if (pt.x < result->xmin) result->xmin = pt.x;
		if (pt.y < result->ymin) result->ymin = pt.y;
		if (pt.x > result->xmax) result->xmax = pt.x;
		if (pt.y > result->ymax) result->ymax = pt.y;
	}

	return result;
}

// Returns a modified POINTARRAY so that no segment is 
// longer then the given distance (computed using 2d).
// Every input point is kept.
// Z and M values for added points (if needed) are set to 0.
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
	int ipoff=0; // input point offset

	pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0;

	// Initial storage
	opa = (POINTARRAY *)lwalloc(ptsize * maxpoints);
	opa->dims = ipa->dims;
	opa->npoints = 0;
	opa->serialized_pointlist = (uchar *)lwalloc(maxpoints*ptsize);

	// Add first point
	opa->npoints++;
	getPoint4d_p(ipa, ipoff, &p1);
	op = getPoint_internal(opa, opa->npoints-1);
	memcpy(op, &p1, ptsize); 
	ipoff++;

	while (ipoff<ipa->npoints)
	{
		getPoint4d_p(ipa, ipoff, &p2);

		segdist = distance2d_pt_pt((POINT2D *)&p1, (POINT2D *)&p2);

		if (segdist > dist) // add an intermediate point
		{
			pbuf.x = p1.x + (p2.x-p1.x)/segdist * dist;
			pbuf.y = p1.y + (p2.y-p1.y)/segdist * dist;
			// might also compute z and m if available...
			ip = &pbuf;
			memcpy(&p1, ip, ptsize);
		}
		else // copy second point
		{
			ip = &p2;
			p1 = p2;
			ipoff++;
		}

		// Add point
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

/*
 * Add a point in a pointarray.
 * 'where' is the offset (starting at 0)
 * if 'where' == -1 append is required.
 */
POINTARRAY *
ptarray_addPoint(POINTARRAY *pa, uchar *p, size_t pdims, unsigned int where)
{
	POINTARRAY *ret;
	POINT4D pbuf;
	size_t ptsize = pointArray_ptsize(pa);

	if ( pdims < 2 || pdims > 4 )
	{
		lwerror("ptarray_addPoint: point dimension out of range (%d)",
			pdims);
		return NULL;
	}

#if PGIS_DEBUG
	lwnotice("ptarray_addPoint: called with a %dD point");
#endif
	
	pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0.0;
	memcpy((uchar *)&pbuf, p, pdims*sizeof(double));

#if PGIS_DEBUG
	lwnotice("ptarray_addPoint: initialized point buffer");
#endif

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

/* 
 * Clone a pointarray 
 */
POINTARRAY *
ptarray_clone(const POINTARRAY *in)
{
	POINTARRAY *out = lwalloc(sizeof(POINTARRAY));
	size_t size;

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
	//POINT2D *p1, *p2;
	if ( memcmp(getPoint_internal(in, 0), getPoint_internal(in, in->npoints-1), sizeof(POINT2D)) ) return 0;
	return 1;
	//p1 = (POINT2D *)getPoint(in, 0);
	//p2 = (POINT2D *)getPoint(in, in->npoints-1);
	//if ( p1->x != p2->x || p1->y != p2->y ) return 0;
	//else return 1;
}

/*
 * calculate the BOX3D bounding box of a set of points
 * returns a lwalloced BOX3D, or NULL on empty array.
 * zmin/zmax values are set to NO_Z_VALUE if not available.
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

/*
 * calculate the BOX3D bounding box of a set of points
 * zmin/zmax values are set to NO_Z_VALUE if not available.
 * write result to the provided BOX3D
 * Return 0 if bounding box is NULL (empty geom)
 */
int
ptarray_compute_box3d_p(const POINTARRAY *pa, BOX3D *result)
{
	int t;
	POINT3DZ pt;

#ifdef PGIS_DEBUG
	lwnotice("ptarray_compute_box3d call (array has %d points)", pa->npoints);
#endif
	if (pa->npoints == 0) return 0;

	getPoint3dz_p(pa, 0, &pt);

#ifdef PGIS_DEBUG
	lwnotice("ptarray_compute_box3d: got point 0");
#endif

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

#ifdef PGIS_DEBUG
	lwnotice("ptarray_compute_box3d: scanning other %d points", pa->npoints);
#endif
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

#ifdef PGIS_DEBUG
	lwnotice("ptarray_compute_box3d returning box");
#endif

	return 1;
}

POINTARRAY *
ptarray_substring(POINTARRAY *ipa, double from, double to)
{
	POINTARRAY *opa;
	int nopts=0;
	uchar *opts;
	uchar *optsptr;
	POINT4D pt;
	POINT2D p1, p2;
	int nsegs, i;
	int ptsize;
	double length, slength, tlength;
	int state = 0; // 0=before, 1=inside, 2=after

	/* Allocate memory for a full copy of input points */
	ptsize=TYPE_NDIMS(ipa->dims)*sizeof(double);
	opts = lwalloc(ptsize*ipa->npoints);
	optsptr = opts;

	/* Interpolate a point on the line */
	nsegs = ipa->npoints - 1;
	length = lwgeom_pointarray_length2d(ipa);

	//lwnotice("Total length: %g", length);

	from = length*from;
	to = length*to;

	//lwnotice("From/To: %g/%g", from, to);

	tlength = 0;
	getPoint2d_p(ipa, 0, &p1);
	for( i = 0; i < nsegs; i++ )
	{
		double dseg;

		getPoint2d_p(ipa, i+1, &p2);

		/* Find the relative length of this segment */
		slength = distance2d_pt_pt(&p1, &p2);

		/*
		 * If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if ( state == 0 )
		{
			if ( from < tlength + slength ) {
				dseg = (from - tlength) / slength;
				pt.x = (p1.x) + ((p2.x - p1.x) * dseg);
				pt.y = (p1.y) + ((p2.y - p1.y) * dseg);
				pt.z = 0; pt.m = 0;
				memcpy(optsptr, &pt, ptsize);
				optsptr+=ptsize;
				nopts++; state++;
			}
		}

		if ( state == 1 )
		{
			if ( to < tlength + slength ) {
				dseg = (to - tlength) / slength;
				pt.x = (p1.x) + ((p2.x - p1.x) * dseg);
				pt.y = (p1.y) + ((p2.y - p1.y) * dseg);
				pt.z = 0; pt.m = 0;
				memcpy(optsptr, &pt, ptsize);
				optsptr+=ptsize;
				nopts++; break;
			} else {
				memcpy(optsptr, &p2, ptsize);
				optsptr+=ptsize;
				nopts++;
			}
		}


		tlength += slength;
		memcpy(&p1, &p2, sizeof(POINT2D));
	}

	//lwnotice("Out of loop, constructing ptarray with %d points", nopts);

	opa = pointArray_construct(opts,
		TYPE_HASZ(ipa->dims),
		TYPE_HASM(ipa->dims),
		nopts);

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

	// we use comp.graphics.algorithms Frequently Asked Questions method

	/*(1)     	AC dot AB
                   r = ----------
                         ||AB||^2
		r has the following meaning:
		r=0 P = A
		r=1 P = B
		r<0 P is on the backward extension of AB
		r>1 P is on the forward extension of AB
		0<r<1 P is interior to AB
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

	//lwnotice("Closest segment: %d", seg);

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

	//lwnotice("Closest point on segment: %g,%g", proj.x, proj.y);

	tlen = lwgeom_pointarray_length2d(pa);
	//lwnotice("tlen %g", tlen);

	plen=0;
	getPoint2d_p(pa, 0, &start);
	for (t=1; t<seg; t++, start=end)
	{
		getPoint2d_p(pa, t+1, &end);
		plen += distance2d_pt_pt(&start, &end);
		//lwnotice("Segment %d made plen %g", t, plen);
	}

	plen+=distance2d_pt_pt(&proj, &start);

	//lwnotice("plen %g, tlen %g", plen, tlen);
	//lwnotice("mindist: %g", mindist);

	return plen/tlen;
}
