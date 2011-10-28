/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

/*
 * Lower this to reduce integrity checks
 */
#define PARANOIA_LEVEL 1


/**********************************************************************
 * BOX routines
 *
 * returns the float thats very close to the input, but <=
 *  handles the funny differences in float4 and float8 reps.
 **********************************************************************/

typedef union
{
	float value;
	uint32_t word;
} ieee_float_shape_type;

#define GET_FLOAT_WORD(i,d)			\
	do {					\
		ieee_float_shape_type gf_u;	\
		gf_u.value = (d);		\
		(i) = gf_u.word;		\
	} while (0)


#define SET_FLOAT_WORD(d,i)			\
	do {					\
		ieee_float_shape_type sf_u;	\
		sf_u.word = (i);		\
		(d) = sf_u.value;		\
	} while (0)


/*
 * Returns the next smaller or next larger float
 * from x (in direction of y).
 */
static float
nextafterf_custom(float x, float y)
{
	int hx,hy,ix,iy;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hy,y);
	ix = hx&0x7fffffff;             /* |x| */
	iy = hy&0x7fffffff;             /* |y| */

	if ((ix>0x7f800000) ||   /* x is nan */
	        (iy>0x7f800000))     /* y is nan */
		return x+y;
	if (x==y) return y;              /* x=y, return y */
	if (ix==0)
	{
		/* x == 0 */
		SET_FLOAT_WORD(x,(hy&0x80000000)|1);/* return +-minsubnormal */
		y = x*x;
		if (y==x) return y;
		else return x;   /* raise underflow flag */
	}
	if (hx>=0)
	{
		/* x > 0 */
		if (hx>hy)
		{
			/* x > y, x -= ulp */
			hx -= 1;
		}
		else
		{
			/* x < y, x += ulp */
			hx += 1;
		}
	}
	else
	{
		/* x < 0 */
		if (hy>=0||hx>hy)
		{
			/* x < y, x -= ulp */
			hx -= 1;
		}
		else
		{
			/* x > y, x += ulp */
			hx += 1;
		}
	}
	hy = hx&0x7f800000;
	if (hy>=0x7f800000) return x+x;  /* overflow  */
	if (hy<0x00800000)
	{
		/* underflow */
		y = x*x;
		if (y!=x)
		{
			/* raise underflow flag */
			SET_FLOAT_WORD(y,hx);
			return y;
		}
	}
	SET_FLOAT_WORD(x,hx);
	return x;
}


float next_float_down(double d)
{
	float result  = d;

	if ( ((double) result) <=d)
		return result;

	return nextafterf_custom(result, result - 1000000);

}

/*
 * Returns the float thats very close to the input, but >=.
 * handles the funny differences in float4 and float8 reps.
 */
float
next_float_up(double d)
{
	float result  = d;

	if ( ((double) result) >=d)
		return result;

	return nextafterf_custom(result, result + 1000000);
}


/*
 * Returns the double thats very close to the input, but <.
 * handles the funny differences in float4 and float8 reps.
 */
double
next_double_down(float d)
{
	double result  = d;

	if ( result < d)
		return result;

	return nextafterf_custom(result, result - 1000000);
}

/*
 * Returns the double thats very close to the input, but >
 * handles the funny differences in float4 and float8 reps.
 */
double
next_double_up(float d)
{
	double result  = d;

	if ( result > d)
		return result;

	return nextafterf_custom(result, result + 1000000);
}



/*
 * Convert BOX3D to BOX2D
 * returned box2d is allocated with 'lwalloc'
 */
BOX2DFLOAT4 *
box3d_to_box2df(BOX3D *box)
{
	BOX2DFLOAT4 *result = (BOX2DFLOAT4*) lwalloc(sizeof(BOX2DFLOAT4));

#if PARANOIA_LEVEL > 0
	if (box == NULL)
	{
		lwerror("box3d_to_box2df got NUL box");
		return NULL;
	}
#endif

	result->xmin = next_float_down(box->xmin);
	result->ymin = next_float_down(box->ymin);

	result->xmax = next_float_up(box->xmax);
	result->ymax = next_float_up(box->ymax);

	return result;
}

/*
 * Convert BOX3D to BOX2D using pre-allocated BOX2D
 * returned box2d is allocated with 'lwalloc'
 * return 0 on error (NULL input box)
 */
int
box3d_to_box2df_p(BOX3D *box, BOX2DFLOAT4 *result)
{
#if PARANOIA_LEVEL > 0
	if (box == NULL)
	{
		lwerror("box3d_to_box2df got NUL box");
		return 0;
	}
#endif

	result->xmin = next_float_down(box->xmin);
	result->ymin = next_float_down(box->ymin);

	result->xmax = next_float_up(box->xmax);
	result->ymax = next_float_up(box->ymax);

	return 1;
}


/*
 * Convert BOX2D to BOX3D
 * zmin and zmax are set to NO_Z_VALUE
 */
BOX3D
box2df_to_box3d(BOX2DFLOAT4 *box)
{
	BOX3D result;

#if PARANOIA_LEVEL > 0
	if (box == NULL)
		lwerror("box2df_to_box3d got NULL box");
#endif

	result.xmin = box->xmin;
	result.ymin = box->ymin;

	result.xmax = box->xmax;
	result.ymax = box->ymax;

	result.zmin = result.zmax = NO_Z_VALUE;

	return result;
}

/*
 * Convert BOX2D to BOX3D, using pre-allocated BOX3D as output
 * Z values are set to NO_Z_VALUE.
 */
void
box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *out)
{
	if (box == NULL) return;

	out->xmin = box->xmin;
	out->ymin = box->ymin;

	out->xmax = box->xmax;
	out->ymax = box->ymax;

	out->zmin = out->zmax = NO_Z_VALUE;
}



/*
 * Returns a BOX3D that encloses b1 and b2
 * box3d_union(NULL,A) --> A
 * box3d_union(A,NULL) --> A
 * box3d_union(A,B) --> A union B
 */
BOX3D *
box3d_union(BOX3D *b1, BOX3D *b2)
{
	BOX3D *result;

	result = lwalloc(sizeof(BOX3D));

	if ( (b1 == NULL) && (b2 == NULL) )
	{
		return NULL;
	}

	if  (b1 == NULL)
	{
		/*return b2 */
		memcpy(result, b2, sizeof(BOX3D));
		return result;
	}
	if (b2 == NULL)
	{
		/*return b1 */
		memcpy(result, b1, sizeof(BOX3D));
		return result;
	}

	if (b1->xmin < b2->xmin)
		result->xmin = b1->xmin;
	else
		result->xmin = b2->xmin;

	if (b1->ymin < b2->ymin)
		result->ymin = b1->ymin;
	else
		result->ymin = b2->ymin;


	if (b1->xmax > b2->xmax)
		result->xmax = b1->xmax;
	else
		result->xmax = b2->xmax;

	if (b1->ymax > b2->ymax)
		result->ymax = b1->ymax;
	else
		result->ymax = b2->ymax;

	if (b1->zmax > b2->zmax)
		result->zmax = b1->zmax;
	else
		result->zmax = b2->zmax;

	if (b1->zmin > b2->zmin)
		result->zmin = b1->zmin;
	else
		result->zmin = b2->zmin;

	return result;
}

/* Make given ubox a union of b1 and b2 */
int
box3d_union_p(BOX3D *b1, BOX3D *b2, BOX3D *ubox)
{

	LWDEBUG(2, "box3d_union_p called: (xmin, xmax), (ymin, ymax), (zmin, zmax)");
	LWDEBUGF(4, "b1: (%.16f, %.16f),(%.16f, %.16f),(%.16f, %.16f)", b1->xmin, b1->xmax, b1->ymin, b1->ymax, b1->zmin, b1->zmax);
	LWDEBUGF(4, "b2: (%.16f, %.16f),(%.16f, %.16f),(%.16f, %.16f)", b2->xmin, b2->xmax, b2->ymin, b2->ymax, b2->zmin, b2->zmax);

	if ( (b1 == NULL) && (b2 == NULL) )
	{
		return 0;
	}

	if  (b1 == NULL)
	{
		memcpy(ubox, b2, sizeof(BOX3D));
		return 1;
	}
	if (b2 == NULL)
	{
		memcpy(ubox, b1, sizeof(BOX3D));
		return 1;
	}

	if (b1->xmin < b2->xmin)
		ubox->xmin = b1->xmin;
	else
		ubox->xmin = b2->xmin;

	if (b1->ymin < b2->ymin)
		ubox->ymin = b1->ymin;
	else
		ubox->ymin = b2->ymin;


	if (b1->xmax > b2->xmax)
		ubox->xmax = b1->xmax;
	else
		ubox->xmax = b2->xmax;

	if (b1->ymax > b2->ymax)
		ubox->ymax = b1->ymax;
	else
		ubox->ymax = b2->ymax;

	if (b1->zmax > b2->zmax)
		ubox->zmax = b1->zmax;
	else
		ubox->zmax = b2->zmax;

	if (b1->zmin < b2->zmin)
		ubox->zmin = b1->zmin;
	else
		ubox->zmin = b2->zmin;

	return 1;
}



/************************************************************************
 * POINTARRAY support functions
 *
 * TODO: should be moved to ptarray.c probably
 *
 ************************************************************************/

/*
 * Copies a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 * will set point's m=NO_M_VALUE if pa is 3d or 2d
 *
 * NOTE: point is a real POINT3D *not* a pointer
 */
POINT4D
getPoint4d(const POINTARRAY *pa, int n)
{
	POINT4D result;
	getPoint4d_p(pa, n, &result);
	return result;
}

/*
 * Copies a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE  if pa is 2d
 * will set point's m=NO_M_VALUE  if pa is 3d or 2d
 *
 * NOTE: this will modify the point4d pointed to by 'point'.
 */
int
getPoint4d_p(const POINTARRAY *pa, int n, POINT4D *op)
{
	uint8_t *ptr;
	int zmflag;

#if PARANOIA_LEVEL > 0
	if ( ! pa ) lwerror("getPoint4d_p: NULL pointarray");

	if ( (n<0) || (n>=pa->npoints))
	{
		lwerror("getPoint4d_p: point offset out of range");
	}
#endif

	LWDEBUG(4, "getPoint4d_p called.");

	/* Get a pointer to nth point offset and zmflag */
	ptr=getPoint_internal(pa, n);
	zmflag=FLAGS_GET_ZM(pa->flags);

	LWDEBUGF(4, "ptr %p, zmflag %d", ptr, zmflag);

	switch (zmflag)
	{
	case 0: /* 2d  */
		memcpy(op, ptr, sizeof(POINT2D));
		op->m=NO_M_VALUE;
		op->z=NO_Z_VALUE;
		break;

	case 3: /* ZM */
		memcpy(op, ptr, sizeof(POINT4D));
		break;

	case 2: /* Z */
		memcpy(op, ptr, sizeof(POINT3DZ));
		op->m=NO_M_VALUE;
		break;

	case 1: /* M */
		memcpy(op, ptr, sizeof(POINT3DM));
		op->m=op->z; /* we use Z as temporary storage */
		op->z=NO_Z_VALUE;
		break;

	default:
		lwerror("Unknown ZM flag ??");
	}
	return 1;

}



/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 * NOTE: point is a real POINT3DZ *not* a pointer
 */
POINT3DZ
getPoint3dz(const POINTARRAY *pa, int n)
{
	POINT3DZ result;
	getPoint3dz_p(pa, n, &result);
	return result;
}

/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 *
 * NOTE: point is a real POINT3DZ *not* a pointer
 */
POINT3DM
getPoint3dm(const POINTARRAY *pa, int n)
{
	POINT3DM result;
	getPoint3dm_p(pa, n, &result);
	return result;
}

/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 *
 * NOTE: this will modify the point3dz pointed to by 'point'.
 */
int
getPoint3dz_p(const POINTARRAY *pa, int n, POINT3DZ *op)
{
	uint8_t *ptr;

#if PARANOIA_LEVEL > 0
	if ( ! pa ) return 0;

	if ( (n<0) || (n>=pa->npoints))
	{
		LWDEBUGF(4, "%d out of numpoint range (%d)", n, pa->npoints);
		return 0; /*error */
	}
#endif

	LWDEBUGF(2, "getPoint3dz_p called on array of %d-dimensions / %u pts",
	         FLAGS_NDIMS(pa->flags), pa->npoints);

	/* Get a pointer to nth point offset */
	ptr=getPoint_internal(pa, n);

	/*
	 * if input POINTARRAY has the Z, it is always
	 * at third position so make a single copy
	 */
	if ( FLAGS_GET_Z(pa->flags) )
	{
		memcpy(op, ptr, sizeof(POINT3DZ));
	}

	/*
	 * Otherwise copy the 2d part and initialize
	 * Z to NO_Z_VALUE
	 */
	else
	{
		memcpy(op, ptr, sizeof(POINT2D));
		op->z=NO_Z_VALUE;
	}

	return 1;

}

/*
 * Copy a point from the point array into the parameter point
 * will set point's m=NO_Z_VALUE if pa has no M
 *
 * NOTE: this will modify the point3dm pointed to by 'point'.
 */
int
getPoint3dm_p(const POINTARRAY *pa, int n, POINT3DM *op)
{
	uint8_t *ptr;
	int zmflag;

#if PARANOIA_LEVEL > 0
	if ( ! pa ) return 0;

	if ( (n<0) || (n>=pa->npoints))
	{
		lwerror("%d out of numpoint range (%d)", n, pa->npoints);
		return 0; /*error */
	}
#endif

	LWDEBUGF(2, "getPoint3dm_p(%d) called on array of %d-dimensions / %u pts",
	         n, FLAGS_NDIMS(pa->flags), pa->npoints);


	/* Get a pointer to nth point offset and zmflag */
	ptr=getPoint_internal(pa, n);
	zmflag=FLAGS_GET_ZM(pa->flags);

	/*
	 * if input POINTARRAY has the M and NO Z,
	 * we can issue a single memcpy
	 */
	if ( zmflag == 1 )
	{
		memcpy(op, ptr, sizeof(POINT3DM));
		return 1;
	}

	/*
	 * Otherwise copy the 2d part and
	 * initialize M to NO_M_VALUE
	 */
	memcpy(op, ptr, sizeof(POINT2D));

	/*
	 * Then, if input has Z skip it and
	 * copy next double, otherwise initialize
	 * M to NO_M_VALUE
	 */
	if ( zmflag == 3 )
	{
		ptr+=sizeof(POINT3DZ);
		memcpy(&(op->m), ptr, sizeof(double));
	}
	else
	{
		op->m=NO_M_VALUE;
	}

	return 1;
}


/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: point is a real POINT2D *not* a pointer
 */
POINT2D
getPoint2d(const POINTARRAY *pa, int n)
{
	POINT2D result;
	getPoint2d_p(pa, n, &result);
	return result;
}

/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: this will modify the point2d pointed to by 'point'.
 */
int
getPoint2d_p(const POINTARRAY *pa, int n, POINT2D *point)
{
#if PARANOIA_LEVEL > 0
	if ( ! pa ) return 0;

	if ( (n<0) || (n>=pa->npoints))
	{
		lwerror("getPoint2d_p: point offset out of range");
		return 0; /*error */
	}
#endif

	/* this does x,y */
	memcpy(point, getPoint_internal(pa, n), sizeof(POINT2D));
	return 1;
}

/*
 * set point N to the given value
 * NOTE that the pointarray can be of any
 * dimension, the appropriate ordinate values
 * will be extracted from it
 *
 */
void
ptarray_set_point4d(POINTARRAY *pa, int n, POINT4D *p4d)
{
	uint8_t *ptr=getPoint_internal(pa, n);
	switch ( FLAGS_GET_ZM(pa->flags) )
	{
	case 3:
		memcpy(ptr, p4d, sizeof(POINT4D));
		break;
	case 2:
		memcpy(ptr, p4d, sizeof(POINT3DZ));
		break;
	case 1:
		memcpy(ptr, p4d, sizeof(POINT2D));
		ptr+=sizeof(POINT2D);
		memcpy(ptr, &(p4d->m), sizeof(double));
		break;
	case 0:
		memcpy(ptr, p4d, sizeof(POINT2D));
		break;
	}
}




/*****************************************************************************
 * Basic sub-geometry types
 *****************************************************************************/

/* handle missaligned uint32_t32 data */
uint32_t
lw_get_uint32_t(const uint8_t *loc)
{
	uint32_t result;

	memcpy(&result, loc, sizeof(uint32_t));
	return result;
}

/* handle missaligned signed int32_t data */
int32_t
lw_get_int32_t(const uint8_t *loc)
{
	int32_t result;

	memcpy(&result,loc, sizeof(int32_t));
	return result;
}


/************************************************
 * debugging routines
 ************************************************/

void printBOX3D(BOX3D *box)
{
	lwnotice("BOX3D: %g %g, %g %g", box->xmin, box->ymin,
	         box->xmax, box->ymax);
}

void printPA(POINTARRAY *pa)
{
	int t;
	POINT4D pt;
	char *mflag;


	if ( FLAGS_GET_M(pa->flags) ) mflag = "M";
	else mflag = "";

	lwnotice("      POINTARRAY%s{", mflag);
	lwnotice("                 ndims=%i,   ptsize=%i",
	         FLAGS_NDIMS(pa->flags), ptarray_point_size(pa));
	lwnotice("                 npoints = %i", pa->npoints);

	for (t =0; t<pa->npoints; t++)
	{
		getPoint4d_p(pa, t, &pt);
		if (FLAGS_NDIMS(pa->flags) == 2)
		{
			lwnotice("                    %i : %lf,%lf",t,pt.x,pt.y);
		}
		if (FLAGS_NDIMS(pa->flags) == 3)
		{
			lwnotice("                    %i : %lf,%lf,%lf",t,pt.x,pt.y,pt.z);
		}
		if (FLAGS_NDIMS(pa->flags) == 4)
		{
			lwnotice("                    %i : %lf,%lf,%lf,%lf",t,pt.x,pt.y,pt.z,pt.m);
		}
	}

	lwnotice("      }");
}


char
ptarray_isccw(const POINTARRAY *pa)
{
	int i;
	double area = 0;
	POINT2D p1, p2;

	for (i=0; i<pa->npoints-1; i++)
	{
		getPoint2d_p(pa, i, &p1);
		getPoint2d_p(pa, i+1, &p2);
		area += (p1.y * p2.x) - (p1.x * p2.y);
	}
	if ( area > 0 ) return 0;
	else return 1;
}


const char *
lwtype_zmflags(uint8_t flags)
{
	static char tflags[4];
	int flagno=0;
	if ( FLAGS_GET_Z(flags) ) tflags[flagno++] = 'Z';
	if ( FLAGS_GET_M(flags) ) tflags[flagno++] = 'M';
	if ( FLAGS_GET_BBOX(flags) ) tflags[flagno++] = 'B';
	tflags[flagno] = '\0';

	LWDEBUGF(4, "Flags: %s - returning %p", flags, tflags);

	return tflags;
}

/**
 * Given a string with at least 2 chars in it, convert them to
 * a byte value.  No error checking done!
 */
uint8_t
parse_hex(char *str)
{
	/* do this a little brute force to make it faster */

	uint8_t		result_high = 0;
	uint8_t		result_low = 0;

	switch (str[0])
	{
	case '0' :
		result_high = 0;
		break;
	case '1' :
		result_high = 1;
		break;
	case '2' :
		result_high = 2;
		break;
	case '3' :
		result_high = 3;
		break;
	case '4' :
		result_high = 4;
		break;
	case '5' :
		result_high = 5;
		break;
	case '6' :
		result_high = 6;
		break;
	case '7' :
		result_high = 7;
		break;
	case '8' :
		result_high = 8;
		break;
	case '9' :
		result_high = 9;
		break;
	case 'A' :
	case 'a' :
		result_high = 10;
		break;
	case 'B' :
	case 'b' :
		result_high = 11;
		break;
	case 'C' :
	case 'c' :
		result_high = 12;
		break;
	case 'D' :
	case 'd' :
		result_high = 13;
		break;
	case 'E' :
	case 'e' :
		result_high = 14;
		break;
	case 'F' :
	case 'f' :
		result_high = 15;
		break;
	}
	switch (str[1])
	{
	case '0' :
		result_low = 0;
		break;
	case '1' :
		result_low = 1;
		break;
	case '2' :
		result_low = 2;
		break;
	case '3' :
		result_low = 3;
		break;
	case '4' :
		result_low = 4;
		break;
	case '5' :
		result_low = 5;
		break;
	case '6' :
		result_low = 6;
		break;
	case '7' :
		result_low = 7;
		break;
	case '8' :
		result_low = 8;
		break;
	case '9' :
		result_low = 9;
		break;
	case 'A' :
	case 'a' :
		result_low = 10;
		break;
	case 'B' :
	case 'b' :
		result_low = 11;
		break;
	case 'C' :
	case 'c' :
		result_low = 12;
		break;
	case 'D' :
	case 'd' :
		result_low = 13;
		break;
	case 'E' :
	case 'e' :
		result_low = 14;
		break;
	case 'F' :
	case 'f' :
		result_low = 15;
		break;
	}
	return (uint8_t) ((result_high<<4) + result_low);
}


/**
 * Given one byte, populate result with two byte representing
 * the hex number.
 *
 * Ie. deparse_hex( 255, mystr)
 *		-> mystr[0] = 'F' and mystr[1] = 'F'
 *
 * No error checking done
 */
void
deparse_hex(uint8_t str, char *result)
{
	int	input_high;
	int  input_low;
	static char outchr[]=
	{
		"0123456789ABCDEF"
	};

	input_high = (str>>4);
	input_low = (str & 0x0F);

	result[0] = outchr[input_high];
	result[1] = outchr[input_low];

}


/**
 * Find interpolation point I
 * between point A and point B
 * so that the len(AI) == len(AB)*F
 * and I falls on AB segment.
 *
 * Example:
 *
 *   F=0.5  :    A----I----B
 *   F=1    :    A---------B==I
 *   F=0    : A==I---------B
 *   F=.2   :    A-I-------B
 */
void
interpolate_point4d(POINT4D *A, POINT4D *B, POINT4D *I, double F)
{
#if PARANOIA_LEVEL > 0
	double absF=fabs(F);
	if ( absF < 0 || absF > 1 )
	{
		lwerror("interpolate_point4d: invalid F (%g)", F);
	}
#endif
	I->x=A->x+((B->x-A->x)*F);
	I->y=A->y+((B->y-A->y)*F);
	I->z=A->z+((B->z-A->z)*F);
	I->m=A->m+((B->m-A->m)*F);
}



