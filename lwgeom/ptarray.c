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
