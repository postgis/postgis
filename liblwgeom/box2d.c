#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "liblwgeom_internal.h"

#ifndef EPSILON
#define EPSILON        1.0E-06
#endif
#ifndef FPeq
#define FPeq(A,B)     (fabs((A) - (B)) <= EPSILON)
#endif



BOX2DFLOAT4 *
box2d_clone(const BOX2DFLOAT4 *in)
{
	BOX2DFLOAT4 *ret = lwalloc(sizeof(BOX2DFLOAT4));
	memcpy(ret, in, sizeof(BOX2DFLOAT4));
	return ret;
}
