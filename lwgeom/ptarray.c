#include <stdio.h>
#include <string.h>

#include "liblwgeom.h"

POINTARRAY *
ptarray_construct(char hasz, char hasm, unsigned int npoints)
{
	unsigned char dims = 0;
	size_t size; 
	char *ptlist;
	POINTARRAY *pa;
	
	TYPE_SETZM(dims, hasz?1:0, hasm?1:0);
	size = TYPE_NDIMS(dims)*npoints*sizeof(double);
	ptlist = (char *)lwalloc(size);
	pa = lwalloc(sizeof(POINTARRAY));
	pa->dims = dims;
	pa->serialized_pointlist = ptlist;
	pa->npoints = npoints;

	return pa;

}

POINTARRAY *
ptarray_construct2d(uint32 npoints, const POINT2D *pts)
{
	POINTARRAY *pa = ptarray_construct(0, 0, npoints);
	uint32 i;

	for (i=0; i<npoints; i++)
	{
		POINT2D *pap = (POINT2D *)getPoint(pa, i);
		pap->x = pts[i].x;
		pap->y = pts[i].y;
	}
	
	return pa;
}

POINTARRAY *
ptarray_construct3dz(uint32 npoints, const POINT3DZ *pts)
{
	POINTARRAY *pa = ptarray_construct(1, 0, npoints);
	uint32 i;

	for (i=0; i<npoints; i++)
	{
		POINT3DZ *pap = (POINT3DZ *)getPoint(pa, i);
		pap->x = pts[i].x;
		pap->y = pts[i].y;
		pap->z = pts[i].z;
	}
	
	return pa;
}

POINTARRAY *
ptarray_construct3dm(uint32 npoints, const POINT3DM *pts)
{
	POINTARRAY *pa = ptarray_construct(0, 1, npoints);
	uint32 i;

	for (i=0; i<npoints; i++)
	{
		POINT3DM *pap = (POINT3DM *)getPoint(pa, i);
		pap->x = pts[i].x;
		pap->y = pts[i].y;
		pap->m = pts[i].m;
	}
	
	return pa;
}

POINTARRAY *
ptarray_construct4d(uint32 npoints, const POINT4D *pts)
{
	POINTARRAY *pa = ptarray_construct(0, 1, npoints);
	uint32 i;

	for (i=0; i<npoints; i++)
	{
		POINT4D *pap = (POINT4D *)getPoint(pa, i);
		pap->x = pts[i].x;
		pap->y = pts[i].y;
		pap->z = pts[i].z;
		pap->m = pts[i].m;
	}
	
	return pa;
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
		char *from, *to;
		from = getPoint(pa, i);
		to = getPoint(pa, (last-i));
		memcpy((char *)&pbuf, to, ptsize);
		memcpy(to, from, ptsize);
		memcpy(from, (char *)&pbuf, ptsize);
	}

}

// calculate the 2d bounding box of a set of points
// write result to the provided BOX2DFLOAT4
// Return 0 if bounding box is NULL (empty geom)
int
ptarray_compute_bbox_p(const POINTARRAY *pa, BOX2DFLOAT4 *result)
{
	int t;
	POINT2D *pt;

	if (pa->npoints == 0) return 0;

	pt = (POINT2D *)getPoint(pa, 0);

	result->xmin = pt->x;
	result->xmax = pt->x;
	result->ymin = pt->y;
	result->ymax = pt->y;

	for (t=1;t<pa->npoints;t++)
	{
		pt = (POINT2D *)getPoint(pa, t);
		if (pt->x < result->xmin) result->xmin = pt->x;
		if (pt->y < result->ymin) result->ymin = pt->y;
		if (pt->x > result->xmax) result->xmax = pt->x;
		if (pt->y > result->ymax) result->ymax = pt->y;
	}

	return 1;
}

// calculate the 2d bounding box of a set of points
// return allocated BOX2DFLOAT4 or NULL (for empty array)
BOX2DFLOAT4 *
ptarray_compute_bbox(const POINTARRAY *pa)
{
	int t;
	POINT2D *pt;
	BOX2DFLOAT4 *result;

	if (pa->npoints == 0) return NULL;

	result = lwalloc(sizeof(BOX2DFLOAT4));

	pt = (POINT2D *)getPoint(pa, 0);

	result->xmin = pt->x;
	result->xmax = pt->x;
	result->ymin = pt->y;
	result->ymax = pt->y;

	for (t=1;t<pa->npoints;t++)
	{
		pt = (POINT2D *)getPoint(pa, t);
		if (pt->x < result->xmin) result->xmin = pt->x;
		if (pt->y < result->ymin) result->ymin = pt->y;
		if (pt->x > result->xmax) result->xmax = pt->x;
		if (pt->y > result->ymax) result->ymax = pt->y;
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
	POINT4D	*p1, *p2, *ip, *op;
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
	opa->serialized_pointlist = (char *)lwalloc(maxpoints*ptsize);

	// Add first point
	opa->npoints++;
	p1 = (POINT4D *)getPoint(ipa, ipoff);
	op = (POINT4D *)getPoint(opa, opa->npoints-1);
	memcpy(op, p1, ptsize); 
	ipoff++;

	while (ipoff<ipa->npoints)
	{
		p2 = (POINT4D *)getPoint(ipa, ipoff);

		segdist = distance2d_pt_pt((POINT2D *)p1, (POINT2D *)p2);

		if (segdist > dist) // add an intermediate point
		{
			pbuf.x = p1->x + (p2->x-p1->x)/segdist * dist;
			pbuf.y = p1->y + (p2->y-p1->y)/segdist * dist;
			// might also compute z and m if available...
			ip = &pbuf;
			p1 = ip;
		}
		else // copy second point
		{
			ip = p2;
			p1 = p2;
			ipoff++;
		}

		// Add point
		if ( ++(opa->npoints) > maxpoints ) {
			maxpoints *= 1.5;
			opa->serialized_pointlist = (char *)lwrealloc(
				opa->serialized_pointlist,
				maxpoints*ptsize
			);
		}
		op = (POINT4D *)getPoint(opa, opa->npoints-1);
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
		if ( memcmp(getPoint(pa1, i), getPoint(pa2, i), ptsize) )
			return 0;
	}

	return 1;
}
