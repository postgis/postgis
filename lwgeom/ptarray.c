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
