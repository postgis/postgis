#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "liblwgeom.h"

#ifndef EPSILON
#define EPSILON        1.0E-06
#endif
#ifndef FPeq
#define FPeq(A,B)     (fabs((A) - (B)) <= EPSILON)
#endif


/* Expand given box of 'd' units in all directions */
void
expand_box2d(BOX2DFLOAT4 *box, double d)
{
	box->xmin -= d;
	box->ymin -= d;

	box->xmax += d;
	box->ymax += d;
}


char
box2d_same(BOX2DFLOAT4 *box1, BOX2DFLOAT4 *box2)
{
	return(FPeq(box1->xmax, box2->xmax) &&
				   FPeq(box1->xmin, box2->xmin) &&
				   FPeq(box1->ymax, box2->ymax) &&
				   FPeq(box1->ymin, box2->ymin));
}

BOX2DFLOAT4 *
box2d_clone(const BOX2DFLOAT4 *in)
{
	BOX2DFLOAT4 *ret = lwalloc(sizeof(BOX2DFLOAT4));
	memcpy(ret, in, sizeof(BOX2DFLOAT4));
	return ret;
}
