
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "liblwgeom_internal.h"

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


/*
 * Same as getbox2d, but modifies box instead of returning result on the stack
 */
int
getbox2d_p(uint8_t *srl, BOX2DFLOAT4 *box)
{
	uint8_t type = srl[0];
	uint8_t *loc;
	BOX3D box3d;

	LWDEBUGF(2, "getbox2d_p call on type %d -> %d", type, TYPE_GETTYPE(type));

	loc = srl+1;

	if (lwgeom_hasBBOX(type))
	{
		/*woot - this is easy */
		LWDEBUG(4, "getbox2d_p: has box");
		memcpy(box, loc, sizeof(BOX2DFLOAT4));
		return LW_TRUE;
	}

	LWDEBUG(4, "getbox2d_p: has no box - computing");

	/* We have to actually compute it! */
	if ( ! compute_serialized_box3d_p(srl, &box3d)) return LW_FALSE;

	LWDEBUGF(4, "getbox2d_p: compute_serialized_box3d returned %p", box3d);

	if ( ! box3d_to_box2df_p(&box3d, box) ) return LW_FALSE;


	LWDEBUG(4, "getbox2d_p: box3d converted to box2d");

	return LW_TRUE;
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





/***************************************************************************
 * Basic type handling
 ***************************************************************************/


/* Returns true if this type says it has an SRID (S bit set) */
char
lwgeom_hasSRID(uint8_t type)
{
	return TYPE_HASSRID(type);
}

/* has M ? */
int lwgeom_hasM(uint8_t type)
{
	return  ( (type & 0x10) >>4);
}

/* has Z ? */
int lwgeom_hasZ(uint8_t type)
{
	return  ( (type & 0x20) >>5);
}


/* get base type (ie. POLYGONTYPE) */
int
lwgeom_getType(uint8_t type)
{
	LWDEBUGF(2, "lwgeom_getType %d", type & 0x0F);

	return (type & 0x0F);
}


/* Construct a type (hasBOX=false) */
uint8_t
lwgeom_makeType(char hasz, char hasm, char has_srid, int type)
{
	return lwgeom_makeType_full(hasz, hasm, has_srid, type, 0);
}

/*
 * Construct a type
 * TODO: needs to be expanded to accept explicit MZ type
 */
uint8_t
lwgeom_makeType_full(char hasz, char hasm, char has_srid, int type, char hasBBOX)
{
	uint8_t result = (char)type;

	TYPE_SETZM(result, hasz, hasm);
	TYPE_SETHASSRID(result, has_srid);
	TYPE_SETHASBBOX(result, hasBBOX);

	return result;
}

/* Returns true if there's a bbox in this LWGEOM (B bit set) */
char
lwgeom_hasBBOX(uint8_t type)
{
	return TYPE_HASBBOX(type);
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


/*************************************************************************
 *
 * Multi-geometry support
 *
 * Note - for a simple type (ie. point), this will have
 * sub_geom[0] = serialized_form.
 *
 * For multi-geomtries sub_geom[0] will be a few bytes
 * into the serialized form.
 *
 * This function just computes the length of each sub-object and
 * pre-caches this info.
 *
 * For a geometry collection of multi* geometries, you can inspect
 * the sub-components
 * as well.
 */
LWGEOM_INSPECTED *
lwgeom_inspect(const uint8_t *serialized_form)
{
	LWGEOM_INSPECTED *result = lwalloc(sizeof(LWGEOM_INSPECTED));
	uint8_t typefl = (uint8_t)serialized_form[0];
	uint8_t type;
	uint8_t **sub_geoms;
	const uint8_t *loc;
	int 	t;

	LWDEBUGF(2, "lwgeom_inspect: serialized@%p", serialized_form);

	if (serialized_form == NULL)
		return NULL;

	result->serialized_form = serialized_form;
	result->type = (uint8_t) serialized_form[0];
	result->srid = SRID_UNKNOWN; /* assume */

	type = lwgeom_getType(typefl);

	loc = serialized_form+1;

	if ( lwgeom_hasBBOX(typefl) )
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(typefl) )
	{
		result->srid = lw_get_int32_t(loc);
		loc += 4;
	}

	if ( (type==POINTTYPE) || (type==LINETYPE) || (type==POLYGONTYPE) ||
	        (type == CIRCSTRINGTYPE) || (type == TRIANGLETYPE))
	{
		/* simple geometry (point/line/polygon/circstring/triangle)-- not multi! */
		result->ngeometries = 1;
		sub_geoms = (uint8_t**) lwalloc(sizeof(char*));
		sub_geoms[0] = (uint8_t *)serialized_form;
		result->sub_geoms = (uint8_t **)sub_geoms;
		return result;
	}

	/* its a GeometryCollection or multi* geometry */

	result->ngeometries = lw_get_uint32_t(loc);
	loc +=4;

	LWDEBUGF(3, "lwgeom_inspect: geometry is a collection of %d elements",
	         result->ngeometries);

	if ( ! result->ngeometries ) return result;

	sub_geoms = lwalloc(sizeof(uint8_t*) * result->ngeometries );
	result->sub_geoms = sub_geoms;
	sub_geoms[0] = (uint8_t *)loc;

	LWDEBUGF(3, "subgeom[0] @ %p (+%d)", sub_geoms[0], sub_geoms[0]-serialized_form);

	for (t=1; t<result->ngeometries; t++)
	{
		/* -1 = entire object */
		int sub_length = lwgeom_size_subgeom(sub_geoms[t-1], -1);
		sub_geoms[t] = sub_geoms[t-1] + sub_length;

		LWDEBUGF(3, "subgeom[%d] @ %p (+%d)",
		         t, sub_geoms[t], sub_geoms[0]-serialized_form);
	}

	return result;

}


/*
 * 1st geometry has geom_number = 0
 * if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a point (with geom_num=0),
 * multipoint or geometrycollection
 */
LWPOINT *
lwgeom_getpoint(uint8_t *serialized_form, int geom_number)
{
	int type = lwgeom_getType((uint8_t)serialized_form[0]);
	uint8_t *sub_geom;

	if ((type == POINTTYPE)  && (geom_number == 0))
	{
		/* Be nice and do as they want instead of what they say */
		return lwpoint_deserialize(serialized_form);
	}

	if ((type != MULTIPOINTTYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL)
		return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != POINTTYPE)
		return NULL;

	return lwpoint_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a point (with geom_num=0), multipoint
 * or geometrycollection
 */
LWPOINT *lwgeom_getpoint_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != POINTTYPE) return NULL;

	return lwpoint_deserialize(sub_geom);
}


/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a LINE, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a line, multiline or geometrycollection
 */
LWLINE *
lwgeom_getline(uint8_t *serialized_form, int geom_number)
{
	uint8_t type = lwgeom_getType( (uint8_t) serialized_form[0]);
	uint8_t *sub_geom;

	if ((type == LINETYPE)  && (geom_number == 0))
	{
		/* be nice and do as they want instead of what they say */
		return lwline_deserialize(serialized_form);
	}

	if ((type != MULTILINETYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((uint8_t) sub_geom[0]);
	if (type != LINETYPE) return NULL;

	return lwline_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a LINE, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a line, multiline or geometrycollection
 */
LWLINE *
lwgeom_getline_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((uint8_t) sub_geom[0]);
	if (type != LINETYPE) return NULL;

	return lwline_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a polygon, multipolygon or geometrycollection
 */
LWPOLY *
lwgeom_getpoly(uint8_t *serialized_form, int geom_number)
{
	uint8_t type = lwgeom_getType((uint8_t)serialized_form[0]);
	uint8_t *sub_geom;

	if ((type == POLYGONTYPE)  && (geom_number == 0))
	{
		/* Be nice and do as they want instead of what they say */
		return lwpoly_deserialize(serialized_form);
	}

	if ((type != MULTIPOLYGONTYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != POLYGONTYPE) return NULL;

	return lwpoly_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a polygon, multipolygon or geometrycollection
 */
LWPOLY *
lwgeom_getpoly_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != POLYGONTYPE) return NULL;

	return lwpoly_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a TRIANGLE, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a Triangle, Tin or geometrycollection
 */
LWTRIANGLE *
lwgeom_gettriangle(uint8_t *serialized_form, int geom_number)
{
	uint8_t type = lwgeom_getType( (uint8_t) serialized_form[0]);
	uint8_t *sub_geom;

	if ((type == TRIANGLETYPE)  && (geom_number == 0))
	{
		/* be nice and do as they want instead of what they say */
		return lwtriangle_deserialize(serialized_form);
	}

	if ((type != TINTYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((uint8_t) sub_geom[0]);
	if (type != TRIANGLETYPE) return NULL;

	return lwtriangle_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a TRIANGLE, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a Triangle, Tin or geometrycollection
 */
LWTRIANGLE *
lwgeom_gettriangle_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != TRIANGLETYPE) return NULL;

	return lwtriangle_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if the actual geometry isnt a CIRCULARSTRING, null is returned (see _gettype()).
 * if there arent enough geometries, return null.
 * this is fine to call on a circularstring
 */
LWCIRCSTRING *
lwgeom_getcircstring_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);
	if (type != CIRCSTRINGTYPE) return NULL;

	return lwcircstring_deserialize(sub_geom);
}

/*
 * 1st geometry has geom_number = 0
 * if there arent enough geometries, return null.
 */
LWGEOM *lwgeom_getgeom_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	uint8_t *sub_geom;
	uint8_t type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType(sub_geom[0]);

	return lwgeom_deserialize(sub_geom);
}


/*
 * This gets the serialized form of a sub-geometry
 *
 * 1st geometry has geom_number = 0
 * if this isnt a multi* geometry, and geom_number ==0 then it returns
 * itself.
 *
 * Returns null on problems.
 *
 * In the future this is how you would access a muli* portion of a
 * geometry collection.
 *    GEOMETRYCOLLECTION(MULTIPOINT(0 0, 1 1), LINESTRING(0 0, 1 1))
 *   ie. lwgeom_getpoint( lwgeom_getsubgeometry( serialized, 0), 1)
 *           --> POINT(1 1)
 *
 * You can inspect the sub-geometry as well if you wish.
 *
 */
uint8_t *
lwgeom_getsubgeometry(const uint8_t *serialized_form, int geom_number)
{
	uint8_t *result;
	/*major cheat!! */
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	result = lwgeom_getsubgeometry_inspected(inspected, geom_number);
	lwinspected_release(inspected);
	return result;
}

uint8_t *
lwgeom_getsubgeometry_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	if ((geom_number <0) || (geom_number >= inspected->ngeometries) )
	{
		lwerror("lwgeom_getsubgeometry_inspected: geom_number out of range");
		return NULL;
	}

	return inspected->sub_geoms[geom_number];
}


/*
 * 1st geometry has geom_number = 0
 *  use geom_number = -1 to find the actual type of the serialized form.
 *    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
 *                 --> multipoint
 *   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
 *                 --> point
 * gets the 8bit type of the geometry at location geom_number
 */
uint8_t
lwgeom_getsubtype(uint8_t *serialized_form, int geom_number)
{
	char  result;
	/*major cheat!! */
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	result = lwgeom_getsubtype_inspected(inspected, geom_number);
	lwinspected_release(inspected);
	return result;
}

uint8_t
lwgeom_getsubtype_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	if ((geom_number <0) || (geom_number >= inspected->ngeometries) )
		return 99;

	return inspected->sub_geoms[geom_number][0]; /* 1st byte is type */
}


/*
 * How many sub-geometries are there?
 * for point,line,polygon will return 1.
 */
int
lwgeom_getnumgeometries(uint8_t *serialized_form)
{
	uint8_t type = lwgeom_getType((uint8_t)serialized_form[0]);
	uint8_t *loc;

	if ( (type==POINTTYPE) || (type==LINETYPE) || (type==POLYGONTYPE) || (type==TRIANGLETYPE) ||
	        (type==CIRCSTRINGTYPE) || (type==COMPOUNDTYPE) || (type==CURVEPOLYTYPE) )
	{
		return 1;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((uint8_t) serialized_form[0]))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID((uint8_t) serialized_form[0]) )
	{
		loc += 4;
	}
	/* its a GeometryCollection or multi* geometry */
	return lw_get_uint32_t(loc);
}

/*
 * How many sub-geometries are there?
 * for point,line,polygon will return 1.
 */
int
lwgeom_getnumgeometries_inspected(LWGEOM_INSPECTED *inspected)
{
	return inspected->ngeometries;
}


/*
 * Set finalType to COLLECTIONTYPE or 0 (0 means choose a best type)
 *   (ie. give it 2 points and ask it to be a multipoint)
 *  use SRID=SRID_UNKNOWN for unknown SRID  (will have 8bit type's S = 0)
 * all subgeometries must have the same SRID
 * if you want to construct an inspected, call this then inspect the result...
 */
uint8_t *
lwgeom_serialized_construct(int srid, int finalType, char hasz, char hasm,
                            int nsubgeometries, uint8_t **serialized_subs)
{
	uint32_t *lengths;
	int t;
	int total_length = 0;
	char type = (char)-1;
	char this_type = -1;
	uint8_t *result;
	uint8_t *loc;

	if (nsubgeometries == 0)
		return lwgeom_constructempty(srid, hasz, hasm);

	lengths = lwalloc(sizeof(int32_t) * nsubgeometries);

	for (t=0; t<nsubgeometries; t++)
	{
		lengths[t] = lwgeom_size_subgeom(serialized_subs[t],-1);
		total_length += lengths[t];
		this_type = lwgeom_getType((uint8_t) (serialized_subs[t][0]));
		if (type == (char)-1)
		{
			type = this_type;
		}
		else if (type == COLLECTIONTYPE)
		{
			/* still a collection type... */
		}
		else
		{
			if ( (this_type == MULTIPOINTTYPE) || (this_type == MULTILINETYPE)  || (this_type == MULTIPOLYGONTYPE) || (this_type == COLLECTIONTYPE) )
			{
				type = COLLECTIONTYPE;
			}
			else
			{
				if ( (this_type == POINTTYPE)  && (type==POINTTYPE) )
					type=MULTIPOINTTYPE;
				else if ( (this_type == LINETYPE)  && (type==LINETYPE) )
					type=MULTILINETYPE;
				else if ( (this_type == POLYGONTYPE)  && (type==POLYGONTYPE) )
					type=MULTIPOLYGONTYPE;
				else if ( (this_type == POLYGONTYPE)  && (type==MULTIPOLYGONTYPE) )
					; /* nop */
				else if ( (this_type == LINETYPE)  && (type==MULTILINETYPE) )
					; /* nop */
				else if ( (this_type == POINTTYPE)  && (type==MULTIPOINTTYPE) )
					; /* nop */
				else
					type = COLLECTIONTYPE;
			}
		}
	}

	if (type == POINTTYPE)
		type = MULTIPOINTTYPE;
	if (type == LINETYPE)
		type = MULTILINETYPE;
	if (type == POINTTYPE)
		type = MULTIPOINTTYPE;

	if (finalType == COLLECTIONTYPE)
		type = COLLECTIONTYPE;

	/* now we have a multi* or GEOMETRYCOLLECTION, let's serialize it */

	if (srid != SRID_UNKNOWN)
		total_length +=4; /* space for SRID */

	total_length +=1 ;   /* main type; */
	total_length +=4 ;   /* nsubgeometries */

	result = lwalloc(total_length);
	result[0] = (uint8_t) lwgeom_makeType(hasz, hasm, srid != SRID_UNKNOWN,  type);
	if (srid != SRID_UNKNOWN)
	{
		memcpy(&result[1],&srid,4);
		loc = result+5;
	}
	else
		loc = result+1;

	memcpy(loc,&nsubgeometries,4);
	loc +=4;

	for (t=0; t<nsubgeometries; t++)
	{
		memcpy(loc, serialized_subs[t], lengths[t] );
		loc += lengths[t] ;
	}

	lwfree(lengths);
	return result;
}


/*
 * Construct the empty geometry (GEOMETRYCOLLECTION EMPTY).
 *
 * Returns serialized form
 */
uint8_t *
lwgeom_constructempty(int srid, char hasz, char hasm)
{
	int size = 0;
	uint8_t *result;
	int ngeoms = 0;
	uint8_t *loc;

	if (srid != SRID_UNKNOWN)
		size +=4;

	size += 5;

	result = lwalloc(size);

	result[0] = lwgeom_makeType(hasz, hasm, srid != SRID_UNKNOWN,  COLLECTIONTYPE);
	if (srid != SRID_UNKNOWN)
	{
		memcpy(&result[1],&srid,4);
		loc = result+5;
	}
	else
		loc = result+1;

	memcpy(loc,&ngeoms,4);
	return result;
}

size_t
lwgeom_empty_length(int srid)
{
	int size = 5;
	if ( srid != 1 ) size += 4;
	return size;
}

/**
 * Construct the empty geometry (GEOMETRYCOLLECTION EMPTY)
 * writing it into the provided buffer.
 */
void
lwgeom_constructempty_buf(int srid, char hasz, char hasm,
                          uint8_t *buf, size_t *retsize)
{
	int ngeoms = 0;

	buf[0] =(uint8_t) lwgeom_makeType( hasz, hasm, srid != SRID_UNKNOWN,  COLLECTIONTYPE);
	if (srid != SRID_UNKNOWN)
	{
		memcpy(&buf[1],&srid,4);
		buf += 5;
	}
	else
		buf += 1;

	memcpy(buf, &ngeoms, 4);

	if (retsize) *retsize = lwgeom_empty_length(srid);
}

/**
 * helper function (not for general use)
 * find the size a geometry (or a sub-geometry)
 * 1st geometry has geom_number = 0
 *  use geom_number = -1 to find the actual type of the serialized form.
 *    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
 *                 --> size of the multipoint
 *   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
 *         --> size of the point
 * take a geometry, and find its length
 */
size_t
serialized_lwgeom_size(const uint8_t *serialized_form)
{
	uint8_t type = lwgeom_getType((uint8_t) serialized_form[0]);
	int t;
	const uint8_t *loc;
	uint32_t ngeoms;
	int sub_size;
	int result = 1; /* type */

	LWDEBUG(2, "lwgeom_size called");

	if (type == POINTTYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a point");

		return lwgeom_size_point(serialized_form);
	}
	else if (type == LINETYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a line");

		return lwgeom_size_line(serialized_form);
	}
	else if (type == CIRCSTRINGTYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a circularstring");

		return lwgeom_size_circstring(serialized_form);
	}
	else if (type == POLYGONTYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a polygon");

		return lwgeom_size_poly(serialized_form);
	}
	else if (type == TRIANGLETYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a triangle");

		return lwgeom_size_triangle(serialized_form);
	}
	else if (type == COMPOUNDTYPE)
	{
		LWDEBUG(3, "lwgeom_size: is a compound curve");
	}

	if ( type == 0 )
	{
		lwerror("lwgeom_size called with unknown-typed serialized geometry");
		return 0;
	}

	/*
	 * Handle all the multi* and geometrycollections the same
	 *
	 * NOTE: for a geometry collection of GC of GC of GC we will
	 *       be recursing...
	 */

	LWDEBUGF(3, "lwgeom_size called on a geoemtry with type %d", type);

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((uint8_t) serialized_form[0]))
	{
		LWDEBUG(3, "lwgeom_size: has bbox");

		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID( (uint8_t) serialized_form[0]) )
	{
		LWDEBUG(3, "lwgeom_size: has srid");

		result +=4;
		loc +=4;
	}


	ngeoms = lw_get_uint32_t(loc);
	loc +=4;
	result += 4; /* numgeoms */

	LWDEBUGF(3, "lwgeom_size called on a geoemtry with %d elems (result so far: %d)", ngeoms, result);

	for (t=0; t<ngeoms; t++)
	{
		sub_size = serialized_lwgeom_size(loc);

		LWDEBUGF(3, " subsize %d", sub_size);

		loc += sub_size;
		result += sub_size;
	}

	LWDEBUGF(3, "lwgeom_size returning %d", result);

	return result;
}

size_t
lwgeom_size_subgeom(const uint8_t *serialized_form, int geom_number)
{
	if (geom_number == -1)
	{
		return serialized_lwgeom_size(serialized_form);
	}
	return serialized_lwgeom_size( lwgeom_getsubgeometry(serialized_form,geom_number));
}


int
compute_serialized_box3d_p(uint8_t *srl, BOX3D *out)
{
	BOX3D *box = compute_serialized_box3d(srl);
	if ( ! box ) return 0;
	out->xmin = box->xmin;
	out->ymin = box->ymin;
	out->zmin = box->zmin;
	out->xmax = box->xmax;
	out->ymax = box->ymax;
	out->zmax = box->zmax;
	lwfree(box);

	return 1;
}


/**
 * Don't forget to lwfree() result !
 */
BOX3D *
compute_serialized_box3d(uint8_t *srl)
{
	int type = lwgeom_getType(srl[0]);
	int t;
	uint8_t *loc = srl;
	uint32_t nelems;
	BOX3D *result;
	BOX3D b1;
	int sub_size;
	char nboxes=0;

	LWDEBUGF(2, "compute_serialized_box3d called on type %d", type);

	loc += 1; /* Move past the 'type' byte. */

	if (lwgeom_hasBBOX(srl[0]))
	{
		loc += sizeof(BOX2DFLOAT4); /* Move past the bbox */
	}

	if (lwgeom_hasSRID(srl[0]) )
	{
		loc +=4; /* Move past the SRID */
	}

	if (type == POINTTYPE)
	{
		LWPOINT *pt = lwpoint_deserialize(srl);

		LWDEBUG(3, "compute_serialized_box3d: lwpoint deserialized");

		result = lwpoint_compute_box3d(pt);

		LWDEBUG(3, "compute_serialized_box3d: bbox found");

		lwpoint_free(pt);
		return result;
	}

	/*
	** For items that have elements (everything except points),
	** nelems == 0 => EMPTY geometry
	*/
	nelems = lw_get_uint32_t(loc);
	if ( nelems == 0 ) return NULL;

	if (type == LINETYPE)
	{
		LWLINE *line = lwline_deserialize(srl);
		result = lwline_compute_box3d(line);
		lwline_free(line);
		return result;

	}
	else if (type == CIRCSTRINGTYPE)
	{
		LWCIRCSTRING *curve = lwcircstring_deserialize(srl);
		result = lwcircstring_compute_box3d(curve);
		lwcircstring_free(curve);
		return result;
	}
	else if (type == POLYGONTYPE)
	{
		LWPOLY *poly = lwpoly_deserialize(srl);
		result = lwpoly_compute_box3d(poly);
		lwpoly_free(poly);
		return result;
	}
	else if (type == TRIANGLETYPE)
	{
		LWTRIANGLE *triangle = lwtriangle_deserialize(srl);
		result = lwtriangle_compute_box3d(triangle);
		lwtriangle_free(triangle);
		return result;
	}

	if ( ! ( type == MULTIPOINTTYPE || type == MULTILINETYPE ||
	         type == MULTIPOLYGONTYPE || type == COLLECTIONTYPE ||
	         type == COMPOUNDTYPE || type == CURVEPOLYTYPE ||
	         type == MULTICURVETYPE || type == MULTISURFACETYPE ||
	         type == POLYHEDRALSURFACETYPE || type == TINTYPE ))
	{
		lwnotice("compute_serialized_box3d called on unknown type %d", type);
		return NULL;
	}

	loc += 4;

	/* each sub-type */
	result = NULL;
	for (t=0; t<nelems; t++)
	{
		if ( compute_serialized_box3d_p(loc, &b1) )
		{
			LWDEBUG(3, "Geom %d have box:");
#if POSTGIS_DEBUG_LEVEL >= 3
			printBOX3D(&b1);
#endif

			if (result)
			{
				nboxes += box3d_union_p(result, &b1, result);
			}
			else
			{
				result = lwalloc(sizeof(BOX3D));
				memcpy(result, &b1, sizeof(BOX3D));
			}
		}

		sub_size = serialized_lwgeom_size(loc);
		loc += sub_size;
	}

	return result;
}

/****************************************************************
 * memory management
 *
 *  these only delete the memory associated
 *  directly with the structure - NOT the stuff pointing into
 *  the original de-serialized info
 *
 ****************************************************************/

void
lwinspected_release(LWGEOM_INSPECTED *inspected)
{
	if ( inspected->ngeometries )
		lwfree(inspected->sub_geoms);
	lwfree(inspected);
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

void printBYTES(uint8_t *a, int n)
{
	int t;
	char buff[3];

	buff[2] = 0; /* null terminate */

	lwnotice(" BYTE ARRAY (n=%i) IN HEX: {", n);
	for (t=0; t<n; t++)
	{
		deparse_hex(a[t], buff);
		lwnotice("    %i : %s", t,buff );
	}
	lwnotice("  }");
}


void
printMULTI(uint8_t *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	LWLINE  *line;
	LWPOINT *point;
	LWPOLY  *poly;
	int t;

	lwnotice("MULTI* geometry (type = %i), with %i sub-geoms",lwgeom_getType((uint8_t)serialized[0]), inspected->ngeometries);

	for (t=0; t<inspected->ngeometries; t++)
	{
		lwnotice("      sub-geometry %i:", t);
		line = NULL;
		point = NULL;
		poly = NULL;

		line = lwgeom_getline_inspected(inspected,t);
		if (line !=NULL)
		{
			printLWLINE(line);
		}
		poly = lwgeom_getpoly_inspected(inspected,t);
		if (poly !=NULL)
		{
			printLWPOLY(poly);
		}
		point = lwgeom_getpoint_inspected(inspected,t);
		if (point !=NULL)
		{
			printPA(point->point);
		}
	}

	lwnotice("end multi*");

	lwinspected_release(inspected);
}


/**
 * Get the srid from the LWGEOM.
 * None present => -1
 */
int
lwgeom_getsrid(uint8_t *serialized)
{
	uint8_t type = serialized[0];
	uint8_t *loc = serialized+1;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return lw_get_int32_t(loc);
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

/**
 * Returns a BOX2DFLOAT4 that encloses b1 and b2
 *
 * box2d_union(NULL,A) --> A
 * box2d_union(A,NULL) --> A
 * box2d_union(A,B) --> A union B
 */
BOX2DFLOAT4 *
box2d_union(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2)
{
	BOX2DFLOAT4 *result;

	if ( (b1 == NULL) && (b2 == NULL) )
	{
		return NULL;
	}

	result = lwalloc(sizeof(BOX2DFLOAT4));

	if  (b1 == NULL)
	{
		memcpy(result, b2, sizeof(BOX2DFLOAT4));
		return result;
	}
	if (b2 == NULL)
	{
		memcpy(result, b1, sizeof(BOX2DFLOAT4));
		return result;
	}

	if (b1->xmin < b2->xmin) result->xmin = b1->xmin;
	else result->xmin = b2->xmin;

	if (b1->ymin < b2->ymin) result->ymin = b1->ymin;
	else result->ymin = b2->ymin;

	if (b1->xmax > b2->xmax) result->xmax = b1->xmax;
	else result->xmax = b2->xmax;

	if (b1->ymax > b2->ymax) result->ymax = b1->ymax;
	else result->ymax = b2->ymax;

	return result;
}

/**
 * ubox may be one of the two args...
 * return 1 if done something to ubox, 0 otherwise.
 */
int
box2d_union_p(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2, BOX2DFLOAT4 *ubox)
{
	if ( (b1 == NULL) && (b2 == NULL) )
	{
		return 0;
	}

	if  (b1 == NULL)
	{
		memcpy(ubox, b2, sizeof(BOX2DFLOAT4));
		return 1;
	}
	if (b2 == NULL)
	{
		memcpy(ubox, b1, sizeof(BOX2DFLOAT4));
		return 1;
	}

	if (b1->xmin < b2->xmin) ubox->xmin = b1->xmin;
	else ubox->xmin = b2->xmin;

	if (b1->ymin < b2->ymin) ubox->ymin = b1->ymin;
	else ubox->ymin = b2->ymin;

	if (b1->xmax > b2->xmax) ubox->xmax = b1->xmax;
	else ubox->xmax = b2->xmax;

	if (b1->ymax > b2->ymax) ubox->ymax = b1->ymax;
	else ubox->ymax = b2->ymax;

	return 1;
}

const char *
lwgeom_typeflags(uint8_t flags)
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



