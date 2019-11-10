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
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2017 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/



#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

#include <stdio.h>
#include <assert.h>
#include "../postgis_revision.h"

#define xstr(s) str(s)
#define str(s) #s

const char *
lwgeom_version()
{
  static char *ptr = NULL;
  static char buf[256];
  if ( ! ptr )
  {
    ptr = buf;
    snprintf(ptr, 256, LIBLWGEOM_VERSION" " xstr(POSTGIS_REVISION));
  }

  return ptr;
}


inline float
next_float_down(double d)
{
	float result;
	if (d > (double)FLT_MAX)
		return FLT_MAX;
	if (d <= (double)-FLT_MAX)
		return -FLT_MAX;
	result = d;

	if ( ((double)result) <=d )
		return result;

	return nextafterf(result, -1*FLT_MAX);

}

/*
 * Returns the float that's very close to the input, but >=.
 * handles the funny differences in float4 and float8 reps.
 */
inline float
next_float_up(double d)
{
	float result;
	if (d >= (double)FLT_MAX)
		return FLT_MAX;
	if (d < (double)-FLT_MAX)
		return -FLT_MAX;
	result = d;

	if ( ((double)result) >=d )
		return result;

	return nextafterf(result, FLT_MAX);
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
getPoint4d(const POINTARRAY *pa, uint32_t n)
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
 *
 * @return 0 on error, 1 on success
 */
int
getPoint4d_p(const POINTARRAY *pa, uint32_t n, POINT4D *op)
{
	uint8_t *ptr;
	int zmflag;

	if ( ! pa )
	{
		lwerror("%s [%d] NULL POINTARRAY input", __FILE__, __LINE__);
		return 0;
	}

	if ( n>=pa->npoints )
	{
		lwnotice("%s [%d] called with n=%d and npoints=%d", __FILE__, __LINE__, n, pa->npoints);
		return 0;
	}

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
		return 0;
	}
	return 1;

}

/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 * NOTE: point is a real POINT3DZ *not* a pointer
 */
POINT3DZ
getPoint3dz(const POINTARRAY *pa, uint32_t n)
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
getPoint3dm(const POINTARRAY *pa, uint32_t n)
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
getPoint3dz_p(const POINTARRAY *pa, uint32_t n, POINT3DZ *op)
{
	uint8_t *ptr;

	if ( ! pa )
	{
		lwerror("%s [%d] NULL POINTARRAY input", __FILE__, __LINE__);
		return 0;
	}

	//assert(n < pa->npoints); --causes point emtpy/point empty to crash
	if ( n>=pa->npoints )
	{
		lwnotice("%s [%d] called with n=%d and npoints=%d", __FILE__, __LINE__, n, pa->npoints);
		return 0;
	}

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
getPoint3dm_p(const POINTARRAY *pa, uint32_t n, POINT3DM *op)
{
	uint8_t *ptr;
	int zmflag;

	if (!pa)
	{
		lwerror("%s [%d] NULL POINTARRAY input", __FILE__, __LINE__);
		return LW_FALSE;
	}

	if (n >= pa->npoints)
	{
		lwerror("%s [%d] called with n=%d and npoints=%d", __FILE__, __LINE__, n, pa->npoints);
		return LW_FALSE;
	}

	/* Get a pointer to nth point offset and zmflag */
	ptr = getPoint_internal(pa, n);
	zmflag = FLAGS_GET_ZM(pa->flags);

	/*
	 * if input POINTARRAY has the M and NO Z,
	 * we can issue a single memcpy
	 */
	if (zmflag == 1)
	{
		memcpy(op, ptr, sizeof(POINT3DM));
		return LW_TRUE;
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
	if (zmflag == 3)
	{
		ptr += sizeof(POINT3DZ);
		memcpy(&(op->m), ptr, sizeof(double));
	}
	else
		op->m = NO_M_VALUE;

	return LW_TRUE;
}

/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: point is a real POINT2D *not* a pointer
 */
POINT2D
getPoint2d(const POINTARRAY *pa, uint32_t n)
{
	const POINT2D *result;
	result = getPoint2d_cp(pa, n);
	return *result;
}

/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: this will modify the point2d pointed to by 'point'.
 */
int
getPoint2d_p(const POINTARRAY *pa, uint32_t n, POINT2D *point)
{
	if ( ! pa )
	{
		lwerror("%s [%d] NULL POINTARRAY input", __FILE__, __LINE__);
		return 0;
	}

	if ( n>=pa->npoints )
	{
		lwnotice("%s [%d] called with n=%d and npoints=%d", __FILE__, __LINE__, n, pa->npoints);
		return 0;
	}

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
ptarray_set_point4d(POINTARRAY *pa, uint32_t n, const POINT4D *p4d)
{
	uint8_t *ptr;
	assert(n < pa->npoints);
	ptr = getPoint_internal(pa, n);
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

void
ptarray_copy_point(POINTARRAY *pa, uint32_t from, uint32_t to)
{
	int ndims = FLAGS_NDIMS(pa->flags);
	switch (ndims)
	{
		case 2:
		{
			POINT2D *p_from = (POINT2D*)(getPoint_internal(pa, from));
			POINT2D *p_to = (POINT2D*)(getPoint_internal(pa, to));
			*p_to = *p_from;
			return;
		}
		case 3:
		{
			POINT3D *p_from = (POINT3D*)(getPoint_internal(pa, from));
			POINT3D *p_to = (POINT3D*)(getPoint_internal(pa, to));
			*p_to = *p_from;
			return;
		}
		case 4:
		{
			POINT4D *p_from = (POINT4D*)(getPoint_internal(pa, from));
			POINT4D *p_to = (POINT4D*)(getPoint_internal(pa, to));
			*p_to = *p_from;
			return;
		}
		default:
		{
			lwerror("%s: unsupported number of dimensions - %d", __func__, ndims);
			return;
		}
	}
	return;
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
	uint32_t t;
	POINT4D pt;
	char *mflag;


	if ( FLAGS_GET_M(pa->flags) ) mflag = "M";
	else mflag = "";

	lwnotice("      POINTARRAY%s{", mflag);
	lwnotice("                 ndims=%i,   ptsize=%i",
	         FLAGS_NDIMS(pa->flags), ptarray_point_size(pa));
	lwnotice("                 npoints = %i", pa->npoints);

	if (!pa)
	{
		lwnotice("                    PTARRAY is null pointer!");
	}
	else
	{

		for (t = 0; t < pa->npoints; t++)
		{
			getPoint4d_p(pa, t, &pt);
			if (FLAGS_NDIMS(pa->flags) == 2)
				lwnotice("                    %i : %lf,%lf", t, pt.x, pt.y);
			if (FLAGS_NDIMS(pa->flags) == 3)
				lwnotice("                    %i : %lf,%lf,%lf", t, pt.x, pt.y, pt.z);
			if (FLAGS_NDIMS(pa->flags) == 4)
				lwnotice("                    %i : %lf,%lf,%lf,%lf", t, pt.x, pt.y, pt.z, pt.m);
		}
	}
	lwnotice("      }");
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
interpolate_point4d(const POINT4D *A, const POINT4D *B, POINT4D *I, double F)
{
#if PARANOIA_LEVEL > 0
	if (F < 0 || F > 1) lwerror("interpolate_point4d: invalid F (%g)", F);
#endif
	I->x=A->x+((B->x-A->x)*F);
	I->y=A->y+((B->y-A->y)*F);
	I->z=A->z+((B->z-A->z)*F);
	I->m=A->m+((B->m-A->m)*F);
}


int _lwgeom_interrupt_requested = 0;
void
lwgeom_request_interrupt() {
  _lwgeom_interrupt_requested = 1;
}
void
lwgeom_cancel_interrupt() {
  _lwgeom_interrupt_requested = 0;
}

lwinterrupt_callback *_lwgeom_interrupt_callback = 0;
lwinterrupt_callback *
lwgeom_register_interrupt_callback(lwinterrupt_callback *cb) {
  lwinterrupt_callback *old = _lwgeom_interrupt_callback;
  _lwgeom_interrupt_callback = cb;
  return old;
}
