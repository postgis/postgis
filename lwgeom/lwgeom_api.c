
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "liblwgeom.h"

// this will change to NaN when I figure out how to
// get NaN in a platform-independent way
#define NO_VALUE 0.0
#define NO_Z_VALUE NO_VALUE
#define NO_M_VALUE NO_VALUE

//#define DEBUG 1
//#define DEBUG_EXPLODED 1

// This is an implementation of the functions defined in lwgeom.h

//forward decs
extern BOX3D *lw_geom_getBB_simple(char *serialized_form);
#ifdef DEBUG_EXPLODED
void checkexplodedsize(char *srl, LWGEOM_EXPLODED *exploded, int alloced, char wantbbox);
#endif
extern char *parse_lwg(const char* geometry, lwallocator allocfunc, lwreporter errfunc);

//*********************************************************************
// BOX routines

// returns the float thats very close to the input, but <=
//  handles the funny differences in float4 and float8 reps.


//these are taken from glibc
// some machines do *not* have these functions defined, so we give
//  an implementation of them here.

typedef int int32_tt;
typedef unsigned int u_int32_tt;


float nextafterf_custom(float x, float y);

typedef union
{
  float value;
  u_int32_tt word;
} ieee_float_shape_type;



#define GET_FLOAT_WORD(i,d)                                     \
do {                                                            \
  ieee_float_shape_type gf_u;                                   \
  gf_u.value = (d);                                             \
  (i) = gf_u.word;                                              \
} while (0)


#define SET_FLOAT_WORD(d,i)                                     \
do {                                                            \
  ieee_float_shape_type sf_u;                                   \
  sf_u.word = (i);                                              \
  (d) = sf_u.value;                                             \
} while (0)


// returns the next smaller or next larger float
// from x (in direction of y).
float nextafterf_custom(float x, float y)
{
        int32_tt hx,hy,ix,iy;

        GET_FLOAT_WORD(hx,x);
        GET_FLOAT_WORD(hy,y);
        ix = hx&0x7fffffff;             /* |x| */
        iy = hy&0x7fffffff;             /* |y| */

        if((ix>0x7f800000) ||   /* x is nan */
           (iy>0x7f800000))     /* y is nan */
           return x+y;
        if(x==y) return y;              /* x=y, return y */
        if(ix==0) {                             /* x == 0 */
            SET_FLOAT_WORD(x,(hy&0x80000000)|1);/* return +-minsubnormal */
            y = x*x;
            if(y==x) return y; else return x;   /* raise underflow flag */
        }
        if(hx>=0) {                             /* x > 0 */
            if(hx>hy) {                         /* x > y, x -= ulp */
                hx -= 1;
            } else {                            /* x < y, x += ulp */
                hx += 1;
            }
        } else {                                /* x < 0 */
            if(hy>=0||hx>hy){                   /* x < y, x -= ulp */
                hx -= 1;
            } else {                            /* x > y, x += ulp */
                hx += 1;
            }
        }
        hy = hx&0x7f800000;
        if(hy>=0x7f800000) return x+x;  /* overflow  */
        if(hy<0x00800000) {             /* underflow */
            y = x*x;
            if(y!=x) {          /* raise underflow flag */
                SET_FLOAT_WORD(y,hx);
                return y;
            }
        }
        SET_FLOAT_WORD(x,hx);
        return x;
}




float nextDown_f(double d)
{
	float result  = d;

	if ( ((double) result) <=d)
		return result;

	return nextafterf_custom(result, result - 1000000);

}

// returns the float thats very close to the input, but >=
//  handles the funny differences in float4 and float8 reps.
float nextUp_f(double d)
{
		float result  = d;

		if ( ((double) result) >=d)
			return result;

		return nextafterf_custom(result, result + 1000000);
}


// returns the double thats very close to the input, but <
//  handles the funny differences in float4 and float8 reps.
double nextDown_d(float d)
{
		double result  = d;

		if ( result < d)
			return result;

		return nextafterf_custom(result, result - 1000000);
}

// returns the double thats very close to the input, but >
//  handles the funny differences in float4 and float8 reps.
double nextUp_d(float d)
{
			double result  = d;

			if ( result > d)
				return result;

			return nextafterf_custom(result, result + 1000000);
}



// Convert BOX3D to BOX2D
// returned box2d is allocated with 'lwalloc'
BOX2DFLOAT4 *
box3d_to_box2df(BOX3D *box)
{
	BOX2DFLOAT4 *result = (BOX2DFLOAT4*) lwalloc(sizeof(BOX2DFLOAT4));

	if (box == NULL)
	{
		lwerror("box3d_to_box2df got NUL box");
		return result;
	}

	result->xmin = nextDown_f(box->xmin);
	result->ymin = nextDown_f(box->ymin);

	result->xmax = nextUp_f(box->xmax);
	result->ymax = nextUp_f(box->ymax);

	return result;
}

// Convert BOX3D to BOX2D using pre-allocated BOX2D
// returned box2d is allocated with 'lwalloc'
// return 0 on error (NULL input box)
int
box3d_to_box2df_p(BOX3D *box, BOX2DFLOAT4 *result)
{
	if (box == NULL)
	{
		lwnotice("box3d_to_box2df got NUL box");
		return 0;
	}

	result->xmin = nextDown_f(box->xmin);
	result->ymin = nextDown_f(box->ymin);

	result->xmax = nextUp_f(box->xmax);
	result->ymax = nextUp_f(box->ymax);

	return 1;
}



// convert BOX2D to BOX3D
// zmin and zmax are set to 0.0
BOX3D
box2df_to_box3d(BOX2DFLOAT4 *box)
{
	BOX3D result;

	if (box == NULL)
		return result;

	result.xmin = box->xmin;
	result.ymin = box->ymin;

	result.xmax = box->xmax;
	result.ymax = box->ymax;

	result.zmin = result.zmax = 0.0;

	return result;
}

// convert BOX2D to BOX3D, using pre-allocated BOX3D as output
// Z values are set to 0.0.
void
box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *out)
{
	if (box == NULL) return;

	out->xmin = box->xmin;
	out->ymin = box->ymin;

	out->xmax = box->xmax;
	out->ymax = box->ymax;

	out->zmin = out->zmax = 0.0;
}



// returns a BOX3D that encloses b1 and b2
// combine_boxes(NULL,A) --> A
// combine_boxes(A,NULL) --> A
// combine_boxes(A,B) --> A union B
BOX3D *combine_boxes(BOX3D *b1, BOX3D *b2)
{
	BOX3D *result;

	result =(BOX3D*) lwalloc(sizeof(BOX3D));

	if ( (b1 == NULL) && (b2 == NULL) )
	{
		return NULL;
	}

	if  (b1 == NULL)
	{
		//return b2
		memcpy(result, b2, sizeof(BOX3D));
	}
	if (b2 == NULL)
	{
		//return b1
		memcpy(result, b1, sizeof(BOX3D));
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

// returns a pointer to internal storage, or NULL
// if the serialized form does not have a BBOX.
BOX2DFLOAT4 *
getbox2d_internal(char *srl)
{
	if (TYPE_HASBBOX(srl[0])) return (BOX2DFLOAT4 *)(srl+1);
	else return NULL;
}

// same as getbox2d, but modifies box instead of returning result on the stack
int
getbox2d_p(char *serialized_form, BOX2DFLOAT4 *box)
{
	char type = serialized_form[0];
	char *loc;
	BOX3D *box3d;

#ifdef DEBUG
	lwnotice("getbox2d_p call");
#endif

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//woot - this is easy
#ifdef DEBUG
		lwnotice("getbox2d_p: has box");
#endif
		memcpy(box, loc, sizeof(BOX2DFLOAT4));
		return 1;
	}

#ifdef DEBUG
	lwnotice("getbox2d_p: has no box - computing");
#endif

	//we have to actually compute it!
//lwnotice("getbox2d_p:: computing box");
	box3d = lw_geom_getBB_simple(serialized_form);

#ifdef DEBUG
	lwnotice("getbox2d_p: lw_geom_getBB_simple returned %p", box3d);
#endif

	if ( ! box3d )
	{
		return 0;
	}

	if ( ! box3d_to_box2df_p(box3d, box) )
	{
		return 0;
	}

#ifdef DEBUG
	lwnotice("getbox2d_p: box3d converted to box2d");
#endif

	//box2 = box3d_to_box2df(box3d);
	//memcpy(box,box2, sizeof(BOX2DFLOAT4));
	//lwfree(box2);

	lwfree(box3d);

	return 1;
}

//************************************************************************
// POINTARRAY support functions


// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: point is a real POINT3D *not* a pointer
POINT4D
getPoint4d(const POINTARRAY *pa, int n)
{
	POINT4D result;
	getPoint4d_p(pa, n, &result);
	return result;
}

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: this will modify the point4d pointed to by 'point'.
int
getPoint4d_p(const POINTARRAY *pa, int n, POINT4D *point)
{
	int size;
	POINT4D *pt;

	if ( ! pa ) return 0;

	if ( (n<0) || (n>=pa->npoints))
	{
		lwerror("getPoint4d_p: point offset out of range");
		return 0; //error
	}

	size = pointArray_ptsize(pa);

	pt = (POINT4D *)getPoint(pa, n);

	// Initialize point
	point->x = pt->x;
	point->y = pt->y;
	point->z = NO_Z_VALUE;
	point->m = NO_M_VALUE;

	if (TYPE_HASZ(pa->dims))
	{
		point->z = pt->z;
		if (TYPE_HASM(pa->dims))
		{
			point->m = pt->m;
		}
	}
	else if (TYPE_HASM(pa->dims))
	{
		point->m = pt->z;
	}

	return 1;
}



// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3DZ *not* a pointer
POINT3DZ
getPoint3dz(const POINTARRAY *pa, int n)
{
	POINT3DZ result;
	getPoint3dz_p(pa, n, &result);
	return result;
}

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3DZ *not* a pointer
POINT3DM
getPoint3dm(const POINTARRAY *pa, int n)
{
	POINT3DM result;
	getPoint3dm_p(pa, n, &result);
	return result;
}

// copies a point from the point array into the parameter point
// will set point's z=NO_Z_VALUE if pa is 2d
// NOTE: this will modify the point3dz pointed to by 'point'.
int
getPoint3dz_p(const POINTARRAY *pa, int n, POINT3DZ *op)
{
	int size;
	POINT4D *ip;

	if ( ! pa ) return 0;

#ifdef DEBUG
	lwnotice("getPoint3dz_p called on array of %d-dimensions / %u pts",
		TYPE_NDIMS(pa->dims), pa->npoints);
#endif

	if ( (n<0) || (n>=pa->npoints))
	{
		lwnotice("%d out of numpoint range (%d)", n, pa->npoints);
		return 0; //error
	}

	size = pointArray_ptsize(pa);

#ifdef DEBUG
	lwnotice("getPoint3dz_p: point size: %d", size);
#endif

	ip = (POINT4D *)getPoint(pa, n);
	op->x = ip->x;
	op->y = ip->y;
	if ( TYPE_HASZ(pa->dims) ) op->z = ip->z;
	else op->z = NO_Z_VALUE;

	return 1;
}

// copies a point from the point array into the parameter point
// will set point's m=NO_Z_VALUE if pa has no M
// NOTE: this will modify the point3dm pointed to by 'point'.
int
getPoint3dm_p(const POINTARRAY *pa, int n, POINT3DM *op)
{
	int size;
	POINT4D *ip;

	if ( ! pa ) return 0;

#ifdef DEBUG
	lwnotice("getPoint3dm_p(%d) called on array of %d-dimensions / %u pts",
		n, TYPE_NDIMS(pa->dims), pa->npoints);
#endif

	if ( (n<0) || (n>=pa->npoints))
	{
		lwerror("%d out of numpoint range (%d)", n, pa->npoints);
		return 0; //error
	}

	size = pointArray_ptsize(pa);

#ifdef DEBUG
	lwnotice("getPoint3d_p: point size: %d", size);
#endif

	ip = (POINT4D *)getPoint(pa, n);
	op->x = ip->x;
	op->y = ip->y;
	op->m = NO_M_VALUE;

	if ( TYPE_HASM(pa->dims) )
	{
		if ( TYPE_HASZ(pa->dims) )
		{
			op->m = ip->m;
		}
		else op->m = ip->z;
	}

	return 1;
}


// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: point is a real POINT2D *not* a pointer
POINT2D
getPoint2d(const POINTARRAY *pa, int n)
{
	POINT2D result;
	int size;

	if ( (n<0) || (n>=pa->npoints))
	{
		return result; //error
	}

	size = pointArray_ptsize(pa);

	// this does x,y
	memcpy(&result.x, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
	return result;
}

// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: this will modify the point2d pointed to by 'point'.
int
getPoint2d_p(const POINTARRAY *pa, int n, POINT2D *point)
{
	 int size;

	if ( ! pa ) return 0;

	 if ( (n<0) || (n>=pa->npoints))
	 {
		 lwerror("getPoint2d_p: point offset out of range");
		 return 0; //error
	 }

	 size = pointArray_ptsize(pa);

	 // this does x,y
	 memcpy(point, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
	 return 1;
}

// get a pointer to nth point of a POINTARRAY
// You'll need to cast it to appropriate dimensioned point.
// Note that if you cast to a higher dimensional point you'll
// possibly corrupt the POINTARRAY.
char *
getPoint(const POINTARRAY *pa, int n)
{
	int size;

	if ( pa == NULL ) {
		lwerror("getPoint got NULL pointarray");
		return NULL;
	}

	if ( (n<0) || (n>=pa->npoints))
	{
		return NULL; //error
	}

	size = pointArray_ptsize(pa);

	// this does x,y
	return &(pa->serialized_pointlist[size*n]);
}



// constructs a POINTARRAY.
// NOTE: points is *not* copied, so be careful about modification (can be aligned/missaligned)
// NOTE: ndims is descriptive - it describes what type of data 'points'
//       points to.  No data conversion is done.
POINTARRAY *
pointArray_construct(char *points, char hasz, char hasm, uint32 npoints)
{
	POINTARRAY  *pa;
	pa = (POINTARRAY*)lwalloc(sizeof(POINTARRAY));

	pa->dims = 0;
	TYPE_SETZM(pa->dims, hasz, hasm);
	pa->npoints = npoints;
	pa->serialized_pointlist = points;

	return pa;
}

// calculate the bounding box of a set of points
// returns a lwalloced BOX3D, or NULL on empty array.
// zmin/zmax values are set to NO_Z_VALUE if not available.
BOX3D *
pointArray_bbox(const POINTARRAY *pa)
{
	int t;
	BOX3D *result;
	POINT3DZ pt;
	int npoints = pa->npoints;

#ifdef DEBUG
	lwnotice("pointArray_bbox call (array has %d points)", pa->npoints);
#endif
	if (pa->npoints == 0)
	{
#ifdef DEBUG
		lwnotice("pointArray_bbox returning NULL");
#endif
		return NULL;
	}

	if ( npoints != pa->npoints) lwerror("Messed up at %s:%d", __FILE__, __LINE__);

	result = (BOX3D*)lwalloc(sizeof(BOX3D));

	if ( ! result )
	{
		lwnotice("Out of virtual memory");
		return NULL;
	}

	getPoint3dz_p(pa, 0, &pt);


#ifdef DEBUG
	lwnotice("pointArray_bbox: got point 0");
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

#ifdef DEBUG
	lwnotice("pointArray_bbox: scanning other %d points", pa->npoints);
#endif
	for (t=1;t<pa->npoints;t++)
	{
		getPoint3dz_p(pa,t,&pt);
		if (pt.x < result->xmin) result->xmin = pt.x;
		if (pt.y < result->ymin) result->ymin = pt.y;
		if (pt.x > result->xmax) result->xmax = pt.x;
		if (pt.y > result->ymax) result->ymax = pt.y;

		if ( TYPE_HASZ(pa->dims) ) {
			if (pt.z > result->zmax) result->zmax = pt.z;
			if (pt.z < result->zmax) result->zmax = pt.z;
		}
	}

#ifdef DEBUG
	lwnotice("pointArray_bbox returning box");
#endif

	return result;
}


//size of point represeneted in the POINTARRAY
// 16 for 2d, 24 for 3d, 32 for 4d
int
pointArray_ptsize(const POINTARRAY *pa)
{
	return sizeof(double)*TYPE_NDIMS(pa->dims);
}


//***************************************************************************
// basic type handling


// returns true if this type says it has an SRID (S bit set)
char
lwgeom_hasSRID(unsigned char type)
{
	return TYPE_HASSRID(type);
}

// returns either 2,3, or 4 -- 2=2D, 3=3D, 4=4D
int
lwgeom_ndims(unsigned char type)
{
	return TYPE_NDIMS(type);
}

// has M ?
int lwgeom_hasM(unsigned char type)
{
	return  ( (type & 0x10) >>4);
}

// has Z ?
int lwgeom_hasZ(unsigned char type)
{
	return  ( (type & 0x20) >>5);
}


// get base type (ie. POLYGONTYPE)
int  lwgeom_getType(unsigned char type)
{
	return (type & 0x0F);
}


//construct a type (hasBOX=false)
unsigned char
lwgeom_makeType(char hasz, char hasm, char hasSRID, int type)
{
	return lwgeom_makeType_full(hasz, hasm, hasSRID, type, 0);
}

/*
 * Construct a type
 * TODO: needs to be expanded to accept explicit MZ type
 */
unsigned char
lwgeom_makeType_full(char hasz, char hasm, char hasSRID, int type, char hasBBOX)
{
	unsigned char result = (char)type;

	TYPE_SETZM(result, hasz, hasm);
	TYPE_SETHASSRID(result, hasSRID);
	TYPE_SETHASBBOX(result, hasBBOX);

	return result;
}

//returns true if there's a bbox in this LWGEOM (B bit set)
char
lwgeom_hasBBOX(unsigned char type)
{
	return TYPE_HASBBOX(type);
}

//*****************************************************************************
// basic sub-geometry types

// handle missaligned unsigned int32 data
uint32
get_uint32(const char *loc)
{
	uint32 result;

	memcpy(&result, loc, sizeof(uint32));
	return result;
}

// handle missaligned signed int32 data
int32
get_int32(const char *loc)
{
	int32 result;

	memcpy(&result,loc, sizeof(int32));
	return result;
}

//******************************************************************************





//*************************************************************************
// multi-geometry support
// note - for a simple type (ie. point), this will have sub_geom[0] = serialized_form.
// for multi-geomtries sub_geom[0] will be a few bytes into the serialized form
// This function just computes the length of each sub-object and pre-caches this info.
// For a geometry collection of multi* geometries, you can inspect the sub-components
// as well.
LWGEOM_INSPECTED *
lwgeom_inspect(const char *serialized_form)
{
	LWGEOM_INSPECTED *result = lwalloc(sizeof(LWGEOM_INSPECTED));
	unsigned char typefl = (unsigned char)serialized_form[0];
	unsigned char type;
	char **sub_geoms;
	const char *loc;
	int 	t;

#ifdef DEBUG
	lwnotice("lwgeom_inspect: serialized@%p", serialized_form);
#endif

	if (serialized_form == NULL)
		return NULL;

	result->serialized_form = serialized_form; 
	result->type = (unsigned char) serialized_form[0];
	result->SRID = -1; // assume

	type = lwgeom_getType(typefl);

	loc = serialized_form+1;

	if ( lwgeom_hasBBOX(typefl) )
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(typefl) )
	{
		result->SRID = get_int32(loc);
		loc += 4;
	}

	if ( (type==POINTTYPE) || (type==LINETYPE) || (type==POLYGONTYPE) )
	{
		//simple geometry (point/line/polygon)-- not multi!
		result->ngeometries = 1;
		sub_geoms = (char**) lwalloc(sizeof(char*));
		sub_geoms[0] = (char *)serialized_form;
		result->sub_geoms = (char **)sub_geoms;
		return result;
	}

	// its a GeometryCollection or multi* geometry

	result->ngeometries = get_uint32(loc);
	loc +=4;
#ifdef DEBUG
	lwnotice("lwgeom_inspect: geometry is a collection of %d elements",
		result->ngeometries);
#endif

	if ( ! result->ngeometries ) return result;

	sub_geoms = (char**) lwalloc(sizeof(char*) * result->ngeometries );
	result->sub_geoms = sub_geoms;
	sub_geoms[0] = (char *)loc;
#ifdef DEBUG
	lwnotice("subgeom[0] @ %p (+%d)", sub_geoms[0], sub_geoms[0]-serialized_form);
#endif
	for (t=1;t<result->ngeometries; t++)
	{
		int sub_length = lwgeom_size_subgeom(sub_geoms[t-1], -1);//-1 = entire object
		sub_geoms[t] = sub_geoms[t-1] + sub_length;
#ifdef DEBUG
		lwnotice("subgeom[%d] @ %p (+%d)",
			t, sub_geoms[t], sub_geoms[0]-serialized_form);
#endif
	}

	return result;

}


// 1st geometry has geom_number = 0
// if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a point (with geom_num=0), multipoint or geometrycollection
LWPOINT *lwgeom_getpoint(char *serialized_form, int geom_number)
{
	char type = lwgeom_getType((unsigned char)serialized_form[0]);
	char *sub_geom;

	if ((type == POINTTYPE)  && (geom_number == 0))
	{
		//be nice and do as they want instead of what they say
		return lwpoint_deserialize(serialized_form);
	}

	if ((type != MULTIPOINTTYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL)
		return NULL;

	type = lwgeom_getType( (unsigned char) sub_geom[0]);
	if (type != POINTTYPE)
		return NULL;

	return lwpoint_deserialize(sub_geom);
}

// 1st geometry has geom_number = 0
// if the actual sub-geometry isnt a POINT, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a point (with geom_num=0), multipoint or geometrycollection
LWPOINT *lwgeom_getpoint_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	char *sub_geom;
	unsigned char type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType( (unsigned char) sub_geom[0]);
	if (type != POINTTYPE) return NULL;

	return lwpoint_deserialize(sub_geom);
}


// 1st geometry has geom_number = 0
// if the actual geometry isnt a LINE, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a line, multiline or geometrycollection
LWLINE *lwgeom_getline(char *serialized_form, int geom_number)
{
	unsigned char type = lwgeom_getType( (unsigned char) serialized_form[0]);
	char *sub_geom;

	if ((type == LINETYPE)  && (geom_number == 0))
	{
		//be nice and do as they want instead of what they say
		return lwline_deserialize(serialized_form);
	}

	if ((type != MULTILINETYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((unsigned char) sub_geom[0]);
	if (type != LINETYPE) return NULL;

	return lwline_deserialize(sub_geom);
}

// 1st geometry has geom_number = 0
// if the actual geometry isnt a LINE, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a line, multiline or geometrycollection
LWLINE *lwgeom_getline_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	char *sub_geom;
	unsigned char type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((unsigned char) sub_geom[0]);
	if (type != LINETYPE) return NULL;

	return lwline_deserialize(sub_geom);
}

// 1st geometry has geom_number = 0
// if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a polygon, multipolygon or geometrycollection
LWPOLY *lwgeom_getpoly(char *serialized_form, int geom_number)
{
	unsigned char type = lwgeom_getType((unsigned char)serialized_form[0]);
	char *sub_geom;

	if ((type == POLYGONTYPE)  && (geom_number == 0))
	{
		//be nice and do as they want instead of what they say
		return lwpoly_deserialize(serialized_form);
	}

	if ((type != MULTIPOLYGONTYPE) && (type != COLLECTIONTYPE) )
		return NULL;

	sub_geom = lwgeom_getsubgeometry(serialized_form, geom_number);
	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((unsigned char) sub_geom[0]);
	if (type != POLYGONTYPE) return NULL;

	return lwpoly_deserialize(sub_geom);
}

// 1st geometry has geom_number = 0
// if the actual geometry isnt a POLYGON, null is returned (see _gettype()).
// if there arent enough geometries, return null.
// this is fine to call on a polygon, multipolygon or geometrycollection
LWPOLY *lwgeom_getpoly_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	char *sub_geom;
	unsigned char type;

	sub_geom = lwgeom_getsubgeometry_inspected(inspected, geom_number);

	if (sub_geom == NULL) return NULL;

	type = lwgeom_getType((unsigned char) sub_geom[0]);
	if (type != POLYGONTYPE) return NULL;

	return lwpoly_deserialize(sub_geom);
}

// this gets the serialized form of a sub-geometry
// 1st geometry has geom_number = 0
// if this isnt a multi* geometry, and geom_number ==0 then it returns
// itself
// returns null on problems.
// in the future this is how you would access a muli* portion of a
// geometry collection.
//    GEOMETRYCOLLECTION(MULTIPOINT(0 0, 1 1), LINESTRING(0 0, 1 1))
//   ie. lwgeom_getpoint( lwgeom_getsubgeometry( serialized, 0), 1)
//           --> POINT(1 1)
// you can inspect the sub-geometry as well if you wish.
char *
lwgeom_getsubgeometry(const char *serialized_form, int geom_number)
{
	//major cheat!!
	char * result;
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	result = lwgeom_getsubgeometry_inspected(inspected, geom_number);
	pfree_inspected(inspected);
	return result;
}

char *
lwgeom_getsubgeometry_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	if ((geom_number <0) || (geom_number >= inspected->ngeometries) )
	{
		lwerror("lwgeom_getsubgeometry_inspected: geom_number out of range");
		return NULL;
	}

	return inspected->sub_geoms[geom_number];
}


// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//                 --> point
// gets the 8bit type of the geometry at location geom_number
char
lwgeom_getsubtype(char *serialized_form, int geom_number)
{
	//major cheat!!
	char  result;
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	result = lwgeom_getsubtype_inspected(inspected, geom_number);
	pfree_inspected(inspected);
	return result;
}

char lwgeom_getsubtype_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	if ((geom_number <0) || (geom_number >= inspected->ngeometries) )
		return 99;

	return inspected->sub_geoms[geom_number][0]; // 1st byte is type
}


// how many sub-geometries are there?
//  for point,line,polygon will return 1.
int lwgeom_getnumgeometries(char *serialized_form)
{
	unsigned char type = lwgeom_getType((unsigned char)serialized_form[0]);
	char *loc;

	if ( (type==POINTTYPE) || (type==LINETYPE) || (type==POLYGONTYPE) )
	{
		return 1;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((unsigned char) serialized_form[0]))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID((unsigned char) serialized_form[0]) )
	{
		loc += 4;
	}
	//its a GeometryCollection or multi* geometry
	return get_uint32(loc);
}

// how many sub-geometries are there?
//  for point,line,polygon will return 1.
int lwgeom_getnumgeometries_inspected(LWGEOM_INSPECTED *inspected)
{
	return inspected->ngeometries;
}


// set finalType to COLLECTIONTYPE or 0 (0 means choose a best type)
//   (ie. give it 2 points and ask it to be a multipoint)
//  use SRID=-1 for unknown SRID  (will have 8bit type's S = 0)
// all subgeometries must have the same SRID
// if you want to construct an inspected, call this then inspect the result...
char *
lwgeom_serialized_construct(int SRID, int finalType, char hasz, char hasm, int nsubgeometries, char **serialized_subs)
{
	uint32 *lengths;
	int t;
	int total_length = 0;
	char type = -1;
	char this_type = -1;
	char *result;
	char *loc;

	if (nsubgeometries == 0)
		return lwgeom_constructempty(SRID, hasz, hasm);

	lengths = lwalloc(sizeof(int32) * nsubgeometries);

	for (t=0;t<nsubgeometries;t++)
	{
		lengths[t] = lwgeom_size_subgeom(serialized_subs[t],-1);
		total_length += lengths[t];
		this_type = lwgeom_getType((unsigned char) (serialized_subs[t][0]));
		if (type == -1)
		{
			type = this_type;
		}
		else if (type == COLLECTIONTYPE)
		{
				//still a collection type...
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
					;//nop
				else if ( (this_type == LINETYPE)  && (type==MULTILINETYPE) )
					;//nop
				else if ( (this_type == POINTTYPE)  && (type==MULTIPOINTTYPE) )
					;//nop
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

	// now we have a mutli* or GEOMETRYCOLLECTION, lets serialize it

	if (SRID != -1)
		total_length +=4; // space for SRID

	total_length +=1 ;   // main type;
	total_length +=4 ;   // nsubgeometries

	result = lwalloc(total_length);
	result[0] = (unsigned char) lwgeom_makeType(hasz, hasm, SRID != -1,  type);
	if (SRID != -1)
	{
		memcpy(&result[1],&SRID,4);
		loc = result+5;
	}
	else
		loc = result+1;

	memcpy(loc,&nsubgeometries,4);
	loc +=4;

	for (t=0;t<nsubgeometries;t++)
	{
		memcpy(loc, serialized_subs[t], lengths[t] );
		loc += lengths[t] ;
	}

	lwfree(lengths);
	return result;
}


// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
//returns serialized form
char *
lwgeom_constructempty(int SRID, char hasz, char hasm)
{
	int size = 0;
	char *result;
	int ngeoms = 0;
	char *loc;

	if (SRID != -1)
		size +=4;

	size += 5;

	result = lwalloc(size);

	result[0] = lwgeom_makeType(hasz, hasm, SRID != -1,  COLLECTIONTYPE);
	if (SRID != -1)
	{
		memcpy(&result[1],&SRID,4);
		loc = result+5;
	}
	else
		loc = result+1;

	memcpy(loc,&ngeoms,4);
	return result;
}

int
lwgeom_empty_length(int SRID)
{
	int size = 5;
	if ( SRID != 1 ) size += 4;
	return size;
}

// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
// writing it into the provided buffer.
void
lwgeom_constructempty_buf(int SRID, char hasz, char hasm, char *buf, int *retsize)
{
	int ngeoms = 0;

	buf[0] =(unsigned char) lwgeom_makeType( hasz, hasm, SRID != -1,  COLLECTIONTYPE);
	if (SRID != -1)
	{
		memcpy(&buf[1],&SRID,4);
		buf += 5;
	}
	else
		buf += 1;

	memcpy(buf, &ngeoms, 4);

	if (retsize) *retsize = lwgeom_empty_length(SRID);
}

// helper function (not for general use)
// find the size a geometry (or a sub-geometry)
// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> size of the multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//         --> size of the point

// take a geometry, and find its length
size_t
lwgeom_size(const char *serialized_form)
{
	unsigned char type = lwgeom_getType((unsigned char) serialized_form[0]);
	int t;
	const char *loc;
	uint32 ngeoms;
	int sub_size;
	int result = 1; //"type"

#ifdef DEBUG
	lwnotice("lwgeom_size called");
#endif

	if (type == POINTTYPE)
	{
#ifdef DEBUG
		lwnotice("lwgeom_size: is a point");
#endif
		return lwgeom_size_point(serialized_form);
	}
	else if (type == LINETYPE)
	{
#ifdef DEBUG
		lwnotice("lwgeom_size: is a line");
#endif
		return lwgeom_size_line(serialized_form);
	}
	else if (type == POLYGONTYPE)
	{
#ifdef DEBUG
		lwnotice("lwgeom_size: is a polygon");
#endif
		return lwgeom_size_poly(serialized_form);
	}

	if ( type == 0 )
	{
		lwerror("lwgeom_size called with unknown-typed serialized geometry");
		return 0;
	}

	//handle all the multi* and geometrycollections the same
	//NOTE: for a geometry collection of GC of GC of GC we will be recursing...

#ifdef DEBUG
	lwnotice("lwgeom_size called on a geoemtry with type %d", type);
#endif

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((unsigned char) serialized_form[0]))
	{
#ifdef DEBUG
		lwnotice("lwgeom_size: has bbox");
#endif

		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID( (unsigned char) serialized_form[0]) )
	{
#ifdef DEBUG
		lwnotice("lwgeom_size: has srid");
#endif
		result +=4;
		loc +=4;
	}


	ngeoms = get_uint32(loc);
	loc +=4;
	result += 4; // numgeoms

#ifdef DEBUG
	lwnotice("lwgeom_size called on a geoemtry with %d elems (result so far: %d)", ngeoms, result);
#endif

	for (t=0;t<ngeoms;t++)
	{
		sub_size = lwgeom_size(loc);
#ifdef DEBUG
		lwnotice(" subsize %d", sub_size);
#endif
		loc += sub_size;
		result += sub_size;
	}

#ifdef DEBUG
	lwnotice("lwgeom_size returning %d", result);
#endif
	return result;
}

size_t
lwgeom_size_subgeom(const char *serialized_form, int geom_number)
{
	if (geom_number == -1)
	{
		return lwgeom_size(serialized_form);
	}
	return lwgeom_size( lwgeom_getsubgeometry(serialized_form,geom_number));
}



int
lwgeom_size_inspected(const LWGEOM_INSPECTED *inspected, int geom_number)
{
	return lwgeom_size(inspected->sub_geoms[geom_number]);
}


//get bounding box of LWGEOM (automatically calls the sub-geometries bbox generators)
//dont forget to lwfree() result
BOX3D *
lw_geom_getBB(char *serialized_form)
{
	  LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	  BOX3D *result = lw_geom_getBB_inspected(inspected);

	  pfree_inspected(inspected);
	  return result;
}

// Compute bounding box of a serialized LWGEOM, even if it is
// already cached. The computed BOX2DFLOAT4 is stored at
// the given location, the function returns 0 is the geometry
// does not have a bounding box (EMPTY GEOM)
int
compute_serialized_bbox_p(char *serialized_form, BOX2DFLOAT4 *out)
{
	  LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	  BOX3D *result = lw_geom_getBB_inspected(inspected);
	  if ( ! result ) return 0;
	  out->xmin = result->xmin;
	  out->xmax = result->xmax;
	  out->ymin = result->ymin;
	  out->ymax = result->ymax;
	  lwfree(result);

	  return 1;
}

//dont forget to lwfree() result
BOX3D *lw_geom_getBB_simple(char *serialized_form)
{
	int type = lwgeom_getType((unsigned char) serialized_form[0]);
	int t;
	char *loc;
	uint32 ngeoms;
	BOX3D *result;
	BOX3D *b1,*b2;
	int sub_size;

#ifdef DEBUG
lwnotice("lw_geom_getBB_simple called on type %d", type);
#endif

	if (type == POINTTYPE)
	{
		LWPOINT *pt = lwpoint_deserialize(serialized_form);
#ifdef DEBUG
lwnotice("lw_geom_getBB_simple: lwpoint deserialized");
#endif
		result = lwpoint_findbbox(pt);
#ifdef DEBUG
lwnotice("lw_geom_getBB_simple: bbox found");
#endif
		pfree_point(pt);
		return result;
	/*
		result = lwalloc(sizeof(BOX3D));
		memcpy(result, serialized_form+1, sizeof(BOX2DFLOAT4));
		memcpy(( (char *)result)+24, serialized_form+1, sizeof(BOX2DFLOAT4));
		return result;
	*/
	}

	else if (type == LINETYPE)
	{
		LWLINE *line = lwline_deserialize(serialized_form);
		result = lwline_findbbox(line);
		pfree_line(line);
		return result;

	}
	else if (type == POLYGONTYPE)
	{
		LWPOLY *poly = lwpoly_deserialize(serialized_form);
		result = lwpoly_findbbox(poly);
		pfree_polygon(poly);
		return result;
	}

	if ( ! ( type == MULTIPOINTTYPE || type == MULTILINETYPE ||
		type == MULTIPOLYGONTYPE || type == COLLECTIONTYPE ) )
	{
		lwnotice("lw_geom_getBB_simple called on unknown type %d", type);
		return NULL;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((unsigned char) serialized_form[0]))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID((unsigned char) serialized_form[0]) )
	{
		loc +=4;
	}

	ngeoms =  get_uint32(loc);
	loc +=4;

	result = NULL;
		// each sub-type
	for (t=0;t<ngeoms;t++)
	{
		b1 = lw_geom_getBB_simple(loc);
		sub_size = lwgeom_size(loc);
		loc += sub_size;
		if (result != NULL)
		{
			b2= result;
			result = combine_boxes(b2, b1);
			lwfree(b1);
			lwfree(b2);
		}
		else
		{
			result = b1;
		}
	}

	return result;

}


//dont forget to lwfree() result
BOX3D *lw_geom_getBB_inspected(LWGEOM_INSPECTED *inspected)
{
	int t;
	BOX3D *b1,*b2,*result;

	result = NULL;

	// handle all the multi* and geometrycollections the same
	// NOTE: for a geometry collection of GC of GC of GC
	// we will be recursing...
	for (t=0;t<inspected->ngeometries;t++)
	{
		b1 = lw_geom_getBB_simple( inspected->sub_geoms[t] );

		//lwnotice("%i has box :: BBOX3D(%g %g, %g %g)",t,b1->xmin, b1->ymin,b1->xmax, b1->ymax);

		if (result != NULL)
		{
			b2= result;
			result = combine_boxes(b2, b1);
//	lwnotice("combined has :: BBOX3D(%g %g, %g %g)",result->xmin, result->ymin,result->xmax, result->ymax);

			lwfree(b1);
			lwfree(b2);
		}
		else
		{
			result = b1;
		}

	}

	return result;
}






//****************************************************************
// memory management -- these only delete the memory associated
//  directly with the structure - NOT the stuff pointing into
//  the original de-serialized info

void pfree_inspected(LWGEOM_INSPECTED *inspected)
{
	if ( inspected->ngeometries )
		lwfree(inspected->sub_geoms);
	lwfree(inspected);
}


void pfree_POINTARRAY(POINTARRAY *pa)
{
	lwfree(pa);
}




//************************************************
//** debugging routines


void printPA(POINTARRAY *pa)
{
	int t;
	POINT4D *pt;
	char *mflag;

	if ( TYPE_HASM(pa->dims) ) mflag = "M";
	else mflag = "";

	lwnotice("      POINTARRAY%s{", mflag);
	lwnotice("                 ndims=%i,   ptsize=%i",
		TYPE_NDIMS(pa->dims), pointArray_ptsize(pa));
	lwnotice("                 npoints = %i", pa->npoints);

	for (t =0; t<pa->npoints;t++)
	{
		pt = (POINT4D *)getPoint(pa,t);
		if (TYPE_NDIMS(pa->dims) == 2)
		{
			lwnotice("                    %i : %lf,%lf",t,pt->x,pt->y);
		}
		if (TYPE_NDIMS(pa->dims) == 3)
		{
			lwnotice("                    %i : %lf,%lf,%lf",t,pt->x,pt->y,pt->z);
		}
		if (TYPE_NDIMS(pa->dims) == 4)
		{
			lwnotice("                    %i : %lf,%lf,%lf,%lf",t,pt->x,pt->y,pt->z,pt->m);
		}
	}

	lwnotice("      }");
}

void printBYTES(unsigned char *a, int n)
{
	int t;
	char buff[3];

	buff[2] = 0; //null terminate

	lwnotice(" BYTE ARRAY (n=%i) IN HEX: {", n);
	for (t=0;t<n;t++)
	{
		deparse_hex(a[t], buff);
		lwnotice("    %i : %s", t,buff );
	}
	lwnotice("  }");
}


void
printMULTI(char *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	LWLINE  *line;
	LWPOINT *point;
	LWPOLY  *poly;
	int t;

	lwnotice("MULTI* geometry (type = %i), with %i sub-geoms",lwgeom_getType((unsigned char)serialized[0]), inspected->ngeometries);

	for (t=0;t<inspected->ngeometries;t++)
	{
		lwnotice("      sub-geometry %i:", t);
		line = NULL; point = NULL; poly = NULL;

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

	pfree_inspected(inspected);
}

void printType(unsigned char type)
{
	lwnotice("type 0x%x ==> hasBBOX=%i, hasSRID=%i, ndims=%i, type=%i",(unsigned int) type, lwgeom_hasBBOX(type), lwgeom_hasSRID(type),lwgeom_ndims(type), lwgeom_getType(type));
}

// get the SRID from the LWGEOM
// none present => -1
int lwgeom_getsrid(char *serialized)
{
	unsigned char type = serialized[0];
	char *loc = serialized+1;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return get_int32(loc);
}

// get the SRID from the LWGEOM
// none present => -1
int lwgeom_getSRID(PG_LWGEOM *lwgeom)
{
	unsigned char type = lwgeom->type;
	char *loc = lwgeom->data;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return get_int32(loc);
}

// Set the SRID of a LWGEOM
// Returns a newly allocated LWGEOM object.
// Allocation will be done using the lwalloc.
PG_LWGEOM *
lwgeom_setSRID(PG_LWGEOM *lwgeom, int32 newSRID)
{
	unsigned char type = lwgeom->type;
	int bbox_offset=0; //0=no bbox, otherwise sizeof(BOX2DFLOAT4)
	int len,len_new,len_left;
	PG_LWGEOM *result;
	char *loc_new, *loc_old;

	if (lwgeom_hasBBOX(type))
		bbox_offset = sizeof(BOX2DFLOAT4);

	len = lwgeom->size;

	if (lwgeom_hasSRID(type))
	{
		if ( newSRID != -1 ) {
			//we create a new one and copy the SRID in
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
			memcpy(result->data+bbox_offset, &newSRID,4);
		} else {
			//we create a new one dropping the SRID
			result = lwalloc(len-4);
			result->size = len-4;
			result->type = lwgeom_makeType_full(
				TYPE_HASZ(type), TYPE_HASM(type),
				0, lwgeom_getType(type),
				lwgeom_hasBBOX(type));
			loc_new = result->data;
			loc_old = lwgeom->data;
			len_left = len-4-1;

			// handle bbox (if there)
			if (lwgeom_hasBBOX(type))
			{
				memcpy(loc_new, loc_old, sizeof(BOX2DFLOAT4));
				loc_new += sizeof(BOX2DFLOAT4);
				loc_old += sizeof(BOX2DFLOAT4);
				len_left -= sizeof(BOX2DFLOAT4);
			}

			// skip SRID, copy the remaining
			loc_old += 4;
			len_left -= 4;
			memcpy(loc_new, loc_old, len_left);

		}

	}
	else 
	{
		// just copy input, already w/out a SRID
		if ( newSRID == -1 ) {
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
		}
		// need to add one
		else {
			len_new = len + 4;//+4 for SRID
			result = lwalloc(len_new);
			memcpy(result, &len_new, 4); // size copy in
			result->type = lwgeom_makeType_full(
				TYPE_HASZ(type), TYPE_HASM(type),
				1, lwgeom_getType(type),lwgeom_hasBBOX(type));

			loc_new = result->data;
			loc_old = lwgeom->data;

			len_left = len -4-1;// old length - size - type

			// handle bbox (if there)

			if (lwgeom_hasBBOX(type))
			{
				memcpy(loc_new, loc_old, sizeof(BOX2DFLOAT4)) ;//copy in bbox
				loc_new += sizeof(BOX2DFLOAT4);
				loc_old += sizeof(BOX2DFLOAT4);
				len_left -= sizeof(BOX2DFLOAT4);
			}

			//put in SRID

			memcpy(loc_new, &newSRID,4);
			loc_new +=4;
			memcpy(loc_new, loc_old, len_left);
		}
	}
	return result;
}

PG_LWGEOM *
PG_LWGEOM_construct(char *ser, int SRID, int wantbbox)
{
	int size;
	char *iptr, *optr, *eptr;
	int wantsrid = 0;
	BOX2DFLOAT4 box;
	PG_LWGEOM *result;

	size = lwgeom_size(ser);
	eptr = ser+size; // end of subgeom

	iptr = ser+1; // skip type
	if ( lwgeom_hasSRID(ser[0]) )
	{
		iptr += 4; // skip SRID
		size -= 4;
	}
	if ( lwgeom_hasBBOX(ser[0]) )
	{
		iptr += sizeof(BOX2DFLOAT4); // skip BBOX
		size -= sizeof(BOX2DFLOAT4);
	}

	if ( SRID != -1 )
	{
		wantsrid = 1;
		size += 4;
	}
	if ( wantbbox )
	{
		size += sizeof(BOX2DFLOAT4);
		getbox2d_p(ser, &box);
	}

	size+=4; // size header

	result = lwalloc(size);
	result->size = size;

	result->type = lwgeom_makeType_full(
		TYPE_HASZ(ser[0]), TYPE_HASM(ser[0]),
		wantsrid, lwgeom_getType(ser[0]), wantbbox);
	optr = result->data;
	if ( wantbbox )
	{
		memcpy(optr, &box, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
	}
	if ( wantsrid )
	{
		memcpy(optr, &SRID, 4);
		optr += 4;
	}
	memcpy(optr, iptr, eptr-iptr);

	return result;
}

void
pfree_exploded(LWGEOM_EXPLODED *exploded)
{
	if ( exploded->npoints )
		lwfree(exploded->points);
	if ( exploded->nlines )
		lwfree(exploded->lines);
	if ( exploded->npolys )
		lwfree(exploded->polys);
	lwfree(exploded);
};

/*
 * This function recursively scan the given serialized geometry
 * and returns a list of _all_ subgeoms in it (deep-first)
 */
LWGEOM_EXPLODED *
lwgeom_explode(char *serialized)
{
	LWGEOM_INSPECTED *inspected;
	LWGEOM_EXPLODED *subexploded, *result;
	int i;

#ifdef DEBUG
	lwnotice("lwgeom_explode called");
#endif

	inspected = lwgeom_inspect(serialized);


#ifdef DEBUG
lwnotice("lwgeom_explode: serialized inspected");
#endif

	result = lwalloc(sizeof(LWGEOM_EXPLODED));
	result->points = lwalloc(1);
	result->lines = lwalloc(1);
	result->polys = lwalloc(1);
	result->npoints = 0;
	result->nlines = 0;
	result->npolys = 0;

	if ( ! inspected->ngeometries )
	{
		lwfree(result->points);
		lwfree(result->lines);
		lwfree(result->polys);
		result->SRID = -1;
		result->dims = inspected->type;
		pfree_inspected(inspected);
		//lwnotice("lwgeom_explode: no geometries");
		return result;
	}

	result->SRID = lwgeom_getsrid(serialized);
	result->dims = inspected->type; // will use ZM only

	for (i=0; i<inspected->ngeometries; i++)
	{

		char *subgeom = inspected->sub_geoms[i];
		int type = lwgeom_getType(subgeom[0]);

		if ( type == POINTTYPE )
		{
#ifdef DEBUG
lwnotice("lwgeom_explode: it's a point");
#endif
			result->points = lwrealloc(result->points,
				(result->npoints+1)*sizeof(char *));
			result->points[result->npoints] = subgeom;
			result->npoints++;
			continue;
		}

		if ( type == LINETYPE )
		{
#ifdef DEBUG
lwnotice("lwgeom_explode: it's a line");
#endif
			result->lines = lwrealloc(result->lines,
				(result->nlines+1)*sizeof(char *));
			result->lines[result->nlines] = subgeom;
			result->nlines++;
			continue;
		}

		if ( type == POLYGONTYPE )
		{
#ifdef DEBUG
lwnotice("lwgeom_explode: it's a polygon");
#endif
			result->polys = lwrealloc(result->polys,
				(result->npolys+1)*sizeof(char *));
			result->polys[result->npolys] = subgeom;
			result->npolys++;
			continue;
		}

#ifdef DEBUG
		lwnotice("type of subgeom %d is %d, recursing", i, type);
#endif

		// it's a multi geometry, recurse
		subexploded = lwgeom_explode(subgeom);

#ifdef DEBUG
		lwnotice("subgeom %d, exploded: %d point, %d lines, %d polys", i, subexploded->npoints, subexploded->nlines, subexploded->npolys);
#endif

		// Re-allocate adding space for new exploded geoms
		// (-1 because 1 was already allocated for the collection)
		// Copy subgeom pointers from subexploded to current
		// exploded.

		if ( subexploded->npoints )
		{
			result->points = lwrealloc(result->points,
				sizeof(char *)*(result->npoints+subexploded->npoints-1));
			if ( ! result )
				lwerror("Out of virtual memory");

#ifdef DEBUG
			lwnotice("lwrealloc'ed exploded->points");
#endif

			memcpy(&(result->points[result->npoints]),
				subexploded->points,
				subexploded->npoints*sizeof(char *));

#ifdef DEBUG
			lwnotice("memcpied exploded->points");
#endif

			result->npoints += subexploded->npoints;

#ifdef DEBUG
			lwnotice("memcopied %d points from subexploded (exploded points: %d", subexploded->npoints, result->npoints);
#endif
		}

		if ( subexploded->nlines )
		{
			result->lines = lwrealloc(result->lines,
				sizeof(char *)*
				(result->nlines+subexploded->nlines-1));

			memcpy(&(result->lines[result->nlines]),
				subexploded->lines,
				subexploded->nlines*sizeof(char *));

			result->nlines += subexploded->nlines;
		}

		if ( subexploded->npolys )
		{
			result->polys = lwrealloc(result->polys,
				sizeof(char *)*
				(result->npolys+subexploded->npolys-1));

			memcpy(&(result->polys[result->npolys]),
				subexploded->polys,
				subexploded->npolys*sizeof(char *));

			result->npolys += subexploded->npolys;
		}

		// release subexploded memory
		pfree_exploded(subexploded);

	}

	pfree_inspected(inspected);

#ifdef DEBUG
lwnotice("lwgeom_explode: returning");
#endif

	return result;
}

// Returns a 'lwalloced' union of the two input exploded geoms
// Returns NULL if SRID or ndims do not match.
LWGEOM_EXPLODED *
lwexploded_sum(LWGEOM_EXPLODED *exp1, LWGEOM_EXPLODED *exp2)
{
	LWGEOM_EXPLODED *expcoll;
	char *loc;

	if ( TYPE_GETZM(exp1->dims) != TYPE_GETZM(exp2->dims) )
	{
		lwerror("lwexploded_sum: can't sum mixed DIMS geoms (%d/%d)",
			TYPE_GETZM(exp1->dims), TYPE_GETZM(exp2->dims));
		return NULL;
	}
	if ( exp1->SRID != exp2->SRID )
	{
		lwerror("lwexploded_sum: can't sum mixed SRID geoms (%d/%d)",
			exp1->SRID, exp2->SRID);
		return NULL;
	}

	expcoll = lwalloc(sizeof(LWGEOM_EXPLODED));

	expcoll->npoints = exp1->npoints + exp2->npoints;
	if ( expcoll->npoints ) {
		expcoll->points = (char **)lwalloc(expcoll->npoints*sizeof(char *));
		loc = (char *)&(expcoll->points[0]);
		if ( exp1->npoints ) {
			memcpy(loc, exp1->points,
				exp1->npoints*sizeof(char *));
			loc += exp1->npoints*sizeof(char *);
		}
		if ( exp2->npoints ) {
			memcpy(loc, exp2->points,
				exp2->npoints*sizeof(char *));
		}
	}

	expcoll->nlines = exp1->nlines + exp2->nlines;
	if ( expcoll->nlines ) {
		expcoll->lines = lwalloc(expcoll->nlines*sizeof(char *));
		loc = (char *)&(expcoll->lines[0]);
		if ( exp1->nlines ) {
			memcpy(loc, exp1->lines,
				exp1->nlines*sizeof(char *));
			loc += exp1->nlines*sizeof(char *);
		}
		if ( exp2->nlines ) {
			memcpy(loc, exp2->lines,
				exp2->nlines*sizeof(char *));
		}
	}

	expcoll->npolys = exp1->npolys + exp2->npolys;
	if ( expcoll->npolys ) {
		expcoll->polys = lwalloc(expcoll->npolys*sizeof(char *));
		loc = (char *)&(expcoll->polys[0]);
		if ( exp1->npolys ) {
			memcpy(loc, exp1->polys,
				exp1->npolys*sizeof(char *));
			loc += exp1->npolys*sizeof(char *);
		}
		if ( exp2->npolys ) {
			memcpy(loc, exp2->polys,
				exp2->npolys*sizeof(char *));
		}
	}

	expcoll->dims = exp1->dims;
	expcoll->SRID = exp1->SRID;

	return expcoll;
}

/*
 * Serialized a LWGEOM_EXPLODED structure
 */
char *
lwexploded_serialize(LWGEOM_EXPLODED *exploded, int wantbbox)
{
	int sizecom = 0;
	int size = lwexploded_findlength(exploded, wantbbox);
	char *result = lwalloc(size);
	lwexploded_serialize_buf(exploded, wantbbox, result, &sizecom);
#ifdef DEBUG
	lwnotice("lwexploded_serialize: findlength:%d, serialize_buf:%d", size, sizecom);
#endif
	return result;
}

/*
 * Serialized a LWGEOM_EXPLODED structure into a
 * pre-allocated memory space.
 * Use lwexploded_findlength to know the required size 
 * of the provided buffer.
 */
void
lwexploded_serialize_buf(LWGEOM_EXPLODED *exploded, int wantbbox,
	char *buf, size_t *retsize)
{
	size_t size=0;
	int i;
	int ntypes = 0;
	int ngeoms = 0;
	char *loc;
	int outtype = 0;
	LWPOLY *poly;
	LWLINE *line;
	LWPOINT *point;
	BOX2DFLOAT4 *box2d;
	BOX3D *box3d;

	ngeoms = exploded->npoints + exploded->nlines + exploded->npolys;

	if ( ngeoms == 0 )
	{
		lwgeom_constructempty_buf(exploded->SRID,
			TYPE_HASZ(exploded->dims), TYPE_HASM(exploded->dims),
			buf, retsize);
		return;
	}


	// For a single geometry just set SRID and BBOX (if requested)
	if ( ngeoms == 1 )
	{
		loc = buf;

		if ( exploded->npoints ) {

			if ( wantbbox && !
				lwgeom_hasBBOX(exploded->points[0][0]) )
			{
				loc += sizeof(BOX2DFLOAT4);
			}
			point = lwpoint_deserialize(exploded->points[0]);
			point->SRID = exploded->SRID;
			lwpoint_serialize_buf(point, loc, &size);
			pfree_point(point);
		}
		else if ( exploded->nlines ) {
			if ( wantbbox && !
				lwgeom_hasBBOX(exploded->lines[0][0]) )
			{
				loc += sizeof(BOX2DFLOAT4);
			}
			line = lwline_deserialize(exploded->lines[0]);
			line->SRID = exploded->SRID;
			lwline_serialize_buf(line, loc, &size);
			pfree_line(line);
		}
		else if ( exploded->npolys ) {
			if ( wantbbox && !
				lwgeom_hasBBOX(exploded->polys[0][0]) )
			{
				loc += sizeof(BOX2DFLOAT4);
			}
			poly = lwpoly_deserialize(exploded->polys[0]);
			poly->SRID = exploded->SRID;
			lwpoly_serialize_buf(poly, loc, &size);
			pfree_polygon(poly);

		}
		else {
			if ( retsize ) *retsize = 0;
			return; // ERROR !!
		}


		// Now compute the bounding box and write it
		if ( wantbbox && ! lwgeom_hasBBOX(loc[0]) )
		{
			buf[0] = loc[0];
			TYPE_SETHASBBOX(buf[0], 1);
			box3d = lw_geom_getBB_simple(loc);
			box2d = box3d_to_box2df(box3d);
			loc = buf+1;
			memcpy(loc, box2d, sizeof(BOX2DFLOAT4));
			size += sizeof(BOX2DFLOAT4);
		}

		if (retsize) *retsize = size;
		return;
	}

	if ( exploded->npoints ) {
		ntypes++;
		outtype = (exploded->npoints>1) ? MULTIPOINTTYPE : POINTTYPE;
	}
	if ( exploded->nlines ) {
		ntypes++;
		if ( outtype ) outtype = COLLECTIONTYPE;
		else outtype = (exploded->nlines>1) ? MULTILINETYPE : LINETYPE;
	}
	if ( exploded->npolys ) {
		ntypes++;
		if ( outtype ) outtype = COLLECTIONTYPE;
		else outtype = (exploded->npolys>1) ? MULTIPOLYGONTYPE : POLYGONTYPE;
	}

#ifdef DEBUG
	lwnotice(" computed outtype: %d, ngeoms: %d", outtype, ngeoms);
#endif


	loc = buf+1; // skip type

	if ( wantbbox ) loc += sizeof(BOX2DFLOAT4); // skip box
	if ( exploded->SRID != -1 ) loc += 4; // skip SRID

	// If we have more then one type of geom
	// write that number in the 'ngeoms' field of the
	// output serialized form (internal geoms would be multi themself).
	// Else rewind location pointer so to overwrite result type.
	if ( ntypes > 1 ) {
		memcpy(loc, &ntypes, 4);
		loc += 4; 
	} else {
		loc--; // let the type be specified later.
	}
	
	if ( exploded->npoints > 1 )
	{
		loc[0] = lwgeom_makeType_full(
			TYPE_HASZ(exploded->dims), TYPE_HASM(exploded->dims),
			0, MULTIPOINTTYPE, 0);
		loc++;
		memcpy(loc, &exploded->npoints, 4); // numpoints
		loc += 4; 
	}
	// Serialize points stripping BBOX and SRID if any
	for (i=0; i<exploded->npoints; i++)
	{
		int subsize;
		point = lwpoint_deserialize(exploded->points[i]);
		point->SRID = -1;
		lwpoint_serialize_buf(point, loc, &subsize);
		pfree_point(point);
		loc += subsize;
	}

	if ( exploded->nlines > 1 )
	{
		loc[0] = lwgeom_makeType_full(
			TYPE_HASZ(exploded->dims),
			TYPE_HASM(exploded->dims),
			0, MULTILINETYPE, 0);
		loc++;
		memcpy(loc, &exploded->nlines, 4); // numlines
		loc += 4; 
	}
	// Serialize lines stripping BBOX and SRID if any
	for (i=0; i<exploded->nlines; i++)
	{
		int subsize;

		line = lwline_deserialize(exploded->lines[i]);
		if ( line == NULL )
		{
	lwerror("Error deserializing %dnt line from exploded geom", i);
	return;
		}
		line->SRID = -1;
		lwline_serialize_buf(line, loc, &subsize);
		pfree_line(line);
		loc += subsize;
	}

	if ( exploded->npolys > 1 )
	{
		loc[0] = lwgeom_makeType_full(
			TYPE_HASZ(exploded->dims),
			TYPE_HASM(exploded->dims),
			0, MULTIPOLYGONTYPE, 0);
		loc++;
		memcpy(loc, &exploded->npolys, 4); // numpolys
		loc += 4; 
	}
	// Serialize polys stripping BBOX and SRID if any
	for (i=0; i<exploded->npolys; i++)
	{
		int subsize;

		poly = lwpoly_deserialize(exploded->polys[i]);
		if ( poly == NULL )
		{
	lwerror("Error deserializing %dnt polygon from exploded geom", i);
	return;
		}
		poly->SRID = -1;
		lwpoly_serialize_buf(poly, loc, &subsize);
		pfree_polygon(poly);
		loc += subsize;
	}

	// Register now the number of written bytes
	if (retsize) *retsize = (loc-buf);

	// Ok. now we need to add type, SRID and bbox 
	buf[0] = lwgeom_makeType_full(
		TYPE_HASZ(exploded->dims),
		TYPE_HASM(exploded->dims),
		(exploded->SRID!=-1), outtype, wantbbox);
	loc = buf+1;

	if ( wantbbox )
	{
		box3d = lw_geom_getBB_simple(buf);
		box2d = box3d_to_box2df(box3d);
		memcpy(loc, box2d, sizeof(BOX2DFLOAT4));
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( exploded->SRID != -1 )
	{
		memcpy(loc, &(exploded->SRID), 4);
		loc += 4; // useless.. we've finished
	}

#ifdef DEBUG
	lwnotice("lwexploded_serialize finished");
	lwnotice(" type: %d", lwgeom_getType(buf[0]));
	lwnotice(" SRID: %d", lwgeom_getsrid(buf));
	if ( lwgeom_hasBBOX(buf[0]) )
	{
		{
			BOX2DFLOAT4 boxbuf;
			getbox2d_p(buf, &boxbuf);
			lwnotice(" BBOX: %f,%f %f,%f",
				boxbuf.xmin, boxbuf.ymin,
				boxbuf.xmax, boxbuf.ymax);
		}
	}
	lwnotice(" numgeoms: %d", lwgeom_getnumgeometries(buf));
#endif

	return;
}

#ifdef DEBUG_EXPLODED
void
checkexplodedsize(char *srl, LWGEOM_EXPLODED *exp, int alloced, char wantbbox)
{
	lwnotice("exploded len: serialized:%d computed:%d alloced:%d",
		lwgeom_size(srl), lwexploded_findlength(exp, wantbbox),
		alloced);
}
#endif

uint32
lwexploded_findlength(LWGEOM_EXPLODED *exploded, int wantbbox)
{
	uint32 size=0;
	char ntypes=0;
	uint32 i;
	

	// find sum of sizes of all geoms.
	// substract size of eventually embedded SRID and BBOXes
	if ( exploded->npoints )
	{
		ntypes++;
		for (i=0; i<exploded->npoints; i++)
		{
			size += lwgeom_size_point(exploded->points[i]);
			if ( lwgeom_hasBBOX(exploded->points[i][0]) )
				size -= sizeof(BOX2DFLOAT4);
			if ( lwgeom_hasSRID(exploded->points[i][0]) )
				size -= 4;
		}
		// add multigeom header size
		if ( exploded->npoints > 1 )
		{
			size += 1; // type
			size += 4; // numgeometries
		}
	}
	if ( exploded->nlines )
	{
		ntypes++;
		for (i=0; i<exploded->nlines; i++)
		{
			size += lwgeom_size_line(exploded->lines[i]);
			if ( lwgeom_hasBBOX(exploded->lines[i][0]) )
				size -= sizeof(BOX2DFLOAT4);
			if ( lwgeom_hasSRID(exploded->lines[i][0]) )
				size -= 4;
		}
		// add multigeom header size
		if ( exploded->nlines > 1 )
		{
			size += 1; // type
			size += 4; // numgeometries
		}
	}
	if ( exploded->npolys )
	{
		ntypes++;
		for (i=0; i<exploded->npolys; i++)
		{
			size += lwgeom_size_poly(exploded->polys[i]);
			if ( lwgeom_hasBBOX(exploded->polys[i][0]) )
				size -= sizeof(BOX2DFLOAT4);
			if ( lwgeom_hasSRID(exploded->polys[i][0]) )
				size -= 4;
		}
		// add multigeom header size
		if ( exploded->npolys > 1 )
		{
			size += 1; // type
			size += 4; // numgeometries
		}
	}

	/* structure is empty */
	if ( ! ntypes ) return lwgeom_empty_length(exploded->SRID);

	/* multi-typed geom (collection), add collection header */
	if ( ntypes > 1 )
	{
		size += 1; // type
		size += 4; // numgeometries
	}

	/*
	 * Add BBOX and SRID if required
	 */
	if ( exploded->SRID != -1 ) size += 4;
	if ( wantbbox ) size += sizeof(BOX2DFLOAT4);

	return size;
}

char
ptarray_isccw(const POINTARRAY *pa)
{
	int i;
	double area = 0;
	POINT2D *p1, *p2;

	for (i=0; i<pa->npoints-1; i++)
	{
		p1 = (POINT2D *)getPoint(pa, i);
		p2 = (POINT2D *)getPoint(pa, i+1);
		area += (p1->x * p2->y) - (p1->y * p2->x);
	}
	if ( area > 0 ) return 0;
	else return 1;
}

// returns a BOX2DFLOAT4 that encloses b1 and b2
// combine_boxes(NULL,A) --> A
// combine_boxes(A,NULL) --> A
// combine_boxes(A,B) --> A union B
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
		memcpy(result, b2, sizeof(BOX3D));
		return result;
	}
	if (b2 == NULL)
	{
		memcpy(result, b1, sizeof(BOX3D));
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

// combine_boxes(NULL,A) --> A
// combine_boxes(A,NULL) --> A
// combine_boxes(A,B) --> A union B
// ubox may be one of the two args...
int
box2d_union_p(BOX2DFLOAT4 *b1, BOX2DFLOAT4 *b2, BOX2DFLOAT4 *ubox)
{
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
lwgeom_typeflags(unsigned char type)
{
	static char flags[5];
	int flagno=0;
	if ( TYPE_HASZ(type) ) flags[flagno++] = 'Z';
	if ( TYPE_HASM(type) ) flags[flagno++] = 'M';
	if ( TYPE_HASBBOX(type) ) flags[flagno++] = 'B';
	if ( TYPE_HASSRID(type) ) flags[flagno++] = 'S';
	flags[flagno] = '\0';
	//lwnotice("Flags: %s - returning %p", flags, flags);
	return flags;
}

//given a string with at least 2 chars in it, convert them to
// a byte value.  No error checking done!
unsigned char
parse_hex(char *str)
{
	//do this a little brute force to make it faster

	unsigned char		result_high = 0;
	unsigned char		result_low = 0;

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
			result_high = 10;
			break;
		case 'B' :
			result_high = 11;
			break;
		case 'C' :
			result_high = 12;
			break;
		case 'D' :
			result_high = 13;
			break;
		case 'E' :
			result_high = 14;
			break;
		case 'F' :
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
			result_low = 10;
			break;
		case 'B' :
			result_low = 11;
			break;
		case 'C' :
			result_low = 12;
			break;
		case 'D' :
			result_low = 13;
			break;
		case 'E' :
			result_low = 14;
			break;
		case 'F' :
			result_low = 15;
			break;
	}
	return (unsigned char) ((result_high<<4) + result_low);
}


//given one byte, populate result with two byte representing
// the hex number
// ie deparse_hex( 255, mystr)
//		-> mystr[0] = 'F' and mystr[1] = 'F'
// no error checking done
void
deparse_hex(unsigned char str, unsigned char *result)
{
	int	input_high;
	int  input_low;

	input_high = (str>>4);
	input_low = (str & 0x0F);

	switch (input_high)
	{
		case 0:
			result[0] = '0';
			break;
		case 1:
			result[0] = '1';
			break;
		case 2:
			result[0] = '2';
			break;
		case 3:
			result[0] = '3';
			break;
		case 4:
			result[0] = '4';
			break;
		case 5:
			result[0] = '5';
			break;
		case 6:
			result[0] = '6';
			break;
		case 7:
			result[0] = '7';
			break;
		case 8:
			result[0] = '8';
			break;
		case 9:
			result[0] = '9';
			break;
		case 10:
			result[0] = 'A';
			break;
		case 11:
			result[0] = 'B';
			break;
		case 12:
			result[0] = 'C';
			break;
		case 13:
			result[0] = 'D';
			break;
		case 14:
			result[0] = 'E';
			break;
		case 15:
			result[0] = 'F';
			break;
	}

	switch (input_low)
	{
		case 0:
			result[1] = '0';
			break;
		case 1:
			result[1] = '1';
			break;
		case 2:
			result[1] = '2';
			break;
		case 3:
			result[1] = '3';
			break;
		case 4:
			result[1] = '4';
			break;
		case 5:
			result[1] = '5';
			break;
		case 6:
			result[1] = '6';
			break;
		case 7:
			result[1] = '7';
			break;
		case 8:
			result[1] = '8';
			break;
		case 9:
			result[1] = '9';
			break;
		case 10:
			result[1] = 'A';
			break;
		case 11:
			result[1] = 'B';
			break;
		case 12:
			result[1] = 'C';
			break;
		case 13:
			result[1] = 'D';
			break;
		case 14:
			result[1] = 'E';
			break;
		case 15:
			result[1] = 'F';
			break;
	}
}

char *
parse_lwgeom_wkt(char *wkt_input)
{
	char *serialized_form = parse_lwg((const char *)wkt_input,
		lwalloc, lwerror);


#ifdef DEBUG
	lwnotice("parse_lwgeom_wkt with %s",wkt_input);
#endif

	if (serialized_form == NULL)
	{
		lwerror("parse_WKT:: couldnt parse!");
		return NULL;
	}

	return serialized_form;

}
