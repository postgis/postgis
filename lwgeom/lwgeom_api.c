#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "fmgr.h"
#include "utils/elog.h"


#include "lwgeom.h"


// This is an implementation of the functions defined in lwgeom.h

//forward decs



extern  BOX3D *lw_geom_getBB_simple(char *serialized_form);


// this will change to NaN when I figure out how to
// get NaN in a platform-independent way

#define NO_Z_VALUE 0.0

//#define DEBUG 1

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
// returned box2d is allocated with 'palloc'
BOX2DFLOAT4 *box3d_to_box2df(BOX3D *box)
{
	BOX2DFLOAT4 *result = (BOX2DFLOAT4*) palloc(sizeof(BOX2DFLOAT4));

	if (box == NULL)
	{
		elog(ERROR, "box3d_to_box2df got NUL box");
		return result;
	}

	result->xmin = nextDown_f(box->xmin);
	result->ymin = nextDown_f(box->ymin);

	result->xmax = nextUp_f(box->xmax);
	result->ymax = nextUp_f(box->ymax);

	return result;
}

// Convert BOX3D to BOX2D using pre-allocated BOX2D
// returned box2d is allocated with 'palloc'
// return 0 on error (NULL input box)
int box3d_to_box2df_p(BOX3D *box, BOX2DFLOAT4 *result)
{
	if (box == NULL)
	{
		elog(NOTICE, "box3d_to_box2df got NUL box");
		return 0;
	}

	result->xmin = nextDown_f(box->xmin);
	result->ymin = nextDown_f(box->ymin);

	result->xmax = nextUp_f(box->xmax);
	result->ymax = nextUp_f(box->ymax);

	return 1;
}


//convert postgresql BOX to BOX2D
BOX2DFLOAT4 *box_to_box2df(BOX *box)
{
	BOX2DFLOAT4 *result = (BOX2DFLOAT4*) palloc(sizeof(BOX2DFLOAT4));

	if (box == NULL)
		return result;

	result->xmin = nextDown_f(box->low.x);
	result->ymin = nextDown_f(box->low.y);

	result->xmax = nextUp_f(box->high.x);
	result->ymax = nextUp_f(box->high.x);

	return result;
}

// convert BOX2D to BOX3D
// zmin and zmax are set to 0.0
BOX3D box2df_to_box3d(BOX2DFLOAT4 *box)
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
void box2df_to_box3d_p(BOX2DFLOAT4 *box, BOX3D *out)
{
	if (box == NULL) return;

	out->xmin = box->xmin;
	out->ymin = box->ymin;

	out->xmax = box->xmax;
	out->ymax = box->ymax;

	out->zmin = out->zmax = 0.0;
}


// convert BOX2D to postgresql BOX
BOX   box2df_to_box(BOX2DFLOAT4 *box)
{
	BOX result;

	if (box == NULL)
		return result;

	result.low.x = nextDown_d(box->xmin);
	result.low.y = nextDown_d(box->ymin);

	result.high.x = nextUp_d(box->xmax);
	result.high.y = nextUp_d(box->ymax);

	return result;
}

// convert BOX2D to postgresql BOX
void
box2df_to_box_p(BOX2DFLOAT4 *box, BOX *out)
{
	if (box == NULL) return;

	out->low.x = nextDown_d(box->xmin);
	out->low.y = nextDown_d(box->ymin);

	out->high.x = nextUp_d(box->xmax);
	out->high.y = nextUp_d(box->ymax);
}


// returns a BOX3D that encloses b1 and b2
// combine_boxes(NULL,A) --> A
// combine_boxes(A,NULL) --> A
// combine_boxes(A,B) --> A union B
BOX3D *combine_boxes(BOX3D *b1, BOX3D *b2)
{
	BOX3D *result;

	result =(BOX3D*) palloc(sizeof(BOX3D));

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

// returns a real entity so it doesnt leak
// if this has a pre-built BOX2d, then we use it,
// otherwise we need to compute it.
BOX2DFLOAT4 getbox2d(char *serialized_form)
{
	int type = (unsigned char) serialized_form[0];
	char *loc;
	BOX2DFLOAT4 result;
	BOX3D *box3d;

	loc = serialized_form+1;

//elog(NOTICE,"getbox2d: type is %d", type);

	if (lwgeom_hasBBOX(type))
	{
		//woot - this is easy
//elog(NOTICE,"getbox2d has box");
		memcpy(&result,loc, sizeof(BOX2DFLOAT4));
		return result;
	}

	//we have to actually compute it!
//elog(NOTICE,"getbox2d -- computing bbox");
	box3d = lw_geom_getBB_simple(serialized_form);
//elog(NOTICE,"lw_geom_getBB_simple got bbox3d(%.15g %.15g,%.15g %.15g)",box3d->xmin,box3d->ymin,box3d->xmax,box3d->ymax);

	if ( ! box3d_to_box2df_p(box3d, &result) )
	{
		elog(ERROR, "Error converting box3d to box2df");
	}

	//box = box3d_to_box2df(box3d);
//elog(NOTICE,"box3d made box2d(%.15g %.15g,%.15g %.15g)",box->xmin,box->ymin,box->xmax,box->ymax);
	//memcpy(&result,box, sizeof(BOX2DFLOAT4));
	//pfree(box);

	pfree(box3d);

	return result;
}

// same as getbox2d, but modifies box instead of returning result on the stack
int
getbox2d_p(char *serialized_form, BOX2DFLOAT4 *box)
{
	char type = serialized_form[0];
	char *loc;
	BOX3D *box3d;

#ifdef DEBUG
	elog(NOTICE,"getbox2d_p call");
#endif

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//woot - this is easy
#ifdef DEBUG
		elog(NOTICE,"getbox2d_p: has box");
#endif
		memcpy(box, loc, sizeof(BOX2DFLOAT4));
		return 1;
	}

#ifdef DEBUG
	elog(NOTICE,"getbox2d_p: has no box - computing");
#endif

	//we have to actually compute it!
//elog(NOTICE,"getbox2d_p:: computing box");
	box3d = lw_geom_getBB_simple(serialized_form);

#ifdef DEBUG
	elog(NOTICE, "getbox2d_p: lw_geom_getBB_simple returned %p", box3d);
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
	elog(NOTICE, "getbox2d_p: box3d converted to box2d");
#endif

	//box2 = box3d_to_box2df(box3d);
	//memcpy(box,box2, sizeof(BOX2DFLOAT4));
	//pfree(box2);

	pfree(box3d);

	return 1;
}

// this function returns a pointer to the 'internal' bounding
// box of a serialized-form geometry. If the geometry does
// not have an embedded bounding box the function returns NULL.
// READ-ONLY!
const BOX2DFLOAT4 *
getbox2d_internal(char *serialized_form)
{
	unsigned char type = (unsigned char) serialized_form[0];

	// No embedded bounding box ...
	if (!lwgeom_hasBBOX(type)) return NULL;

	return (BOX2DFLOAT4 *)(serialized_form+1);
}

//************************************************************************
// POINTARRAY support functions


// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: point is a real POINT3D *not* a pointer
POINT4D getPoint4d(POINTARRAY *pa, int n)
{
		 POINT4D result;
		 int size;

		 if ( (n<0) || (n>=pa->npoints))
		 {
			 return result; //error
		 }

		 size = pointArray_ptsize(pa);

		 	// this does x,y
		 memcpy(&result.x, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
		 if (pa->ndims >2)
		 	memcpy(&result.z, &pa->serialized_pointlist[size*n + sizeof(double)*2],sizeof(double) );
		 else
		 	result.z = NO_Z_VALUE;
		 if (pa->ndims >3)
		 	memcpy(&result.m, &pa->serialized_pointlist[size*n + sizeof(double)*3],sizeof(double) );
		 else
		 	result.m = NO_Z_VALUE;

		 return result;
}

// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// will set point's m=0 (or NaN( if pa is 3d or 2d
// NOTE: this will modify the point4d pointed to by 'point'.
void getPoint4d_p(POINTARRAY *pa, int n, char *point)
{
		int size;

		 if ( (n<0) || (n>=pa->npoints))
		 {
			 return ; //error
		 }

		size = pointArray_ptsize(pa);

		 	// this does x,y
		 memcpy(point, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
		 if (pa->ndims >2)
		 	memcpy(point+16, &pa->serialized_pointlist[size*n + sizeof(double)*2],sizeof(double) );
		 else
		 {
			 double bad=NO_Z_VALUE;
			 memcpy(point+16, &bad,sizeof(double) );
		 	 //point->z = NO_Z_VALUE;
	 	 }

		 if (pa->ndims >3)
			memcpy(point+24, &pa->serialized_pointlist[size*n + sizeof(double)*3],sizeof(double) );
		 else
		 {
			 double bad=NO_Z_VALUE;
			 memcpy(point+24, &bad,sizeof(double) );
			//point->z = NO_Z_VALUE;
	 	 }
}



// copies a point from the point array into the parameter point
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: point is a real POINT3D *not* a pointer
POINT3D getPoint3d(POINTARRAY *pa, int n)
{
	POINT3D result;
	int size;

	if ( (n<0) || (n>=pa->npoints))
	{
		return result; //error
	}

	size = pointArray_ptsize(pa);

	// this does x,y
	memcpy(&result.x, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
	if (pa->ndims >2)
		memcpy(&result.z, &pa->serialized_pointlist[size*n + sizeof(double)*2],sizeof(double) );
	 else
	 	result.z = NO_Z_VALUE;
	 return result;
}

// copies a point from the point array into the parameter point
// will set point's z=NO_Z_VALUE if pa is 2d
// NOTE: this will modify the point3d pointed to by 'point'.
// we use a char* instead of a POINT3D because we want to use this function
//  for serialization/de-serialization
void getPoint3d_p(POINTARRAY *pa, int n, char *point)
{
	int size;
	double bad=NO_Z_VALUE;
	POINT3D *op, *ip;

	op = (POINT3D *)point;

#ifdef DEBUG
	elog(NOTICE, "getPoint3d_p called on array of %d-dimensions / %u pts",
		pa->ndims, pa->npoints);
	if ( pa->npoints > 1000 ) elog(ERROR, "More then 1000 points in pointarray ??");
#endif

	if ( (n<0) || (n>=pa->npoints))
	{
		elog(NOTICE, "%d out of numpoint range (%d)", n, pa->npoints);
		return ; //error
	}

	size = pointArray_ptsize(pa);

#ifdef DEBUG
	elog(NOTICE, "getPoint3d_p: point size: %d", size);
#endif

	ip = (POINT3D *)getPoint(pa, n);
	op->x = ip->x;
	op->y = ip->y;
	if (pa->ndims >2) op->z = ip->z;
	else op->z = NO_Z_VALUE;
	return;

#ifdef _KEEP_TRASH
	// this does x,y
	memcpy(point,
		&pa->serialized_pointlist[size*n],
		sizeof(double)*2 );

#ifdef DEBUG
	elog(NOTICE, "getPoint3d_p: x/y copied");
#endif

	if (pa->ndims >2)
	{
		memcpy(point+sizeof(double)*2,
			&pa->serialized_pointlist[size*n + sizeof(double)*2],
			sizeof(double) );
#ifdef DEBUG
		elog(NOTICE, "getPoint3d_p: z copied");
#endif
	}
	else
	{
		memcpy(point+sizeof(double)*2, &bad, sizeof(double) );
	 	//point->z = NO_Z_VALUE;
#ifdef DEBUG
		elog(NOTICE, "getPoint3d_p: z set to %f", NO_Z_VALUE);
#endif
 	}

#endif // _KEEP_TRASH

}


// copies a point from the point array into the parameter point
// z value (if present is not returned)
// NOTE: point is a real POINT3D *not* a pointer
POINT2D getPoint2d(POINTARRAY *pa, int n)
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
// we use a char* instead of a POINT2D because we want to use this function
//  for serialization/de-serialization
void getPoint2d_p(POINTARRAY *pa, int n, char *point)
{
	 int size;

	 if ( (n<0) || (n>=pa->npoints))
	 {
		 return; //error
	 }

	 size = pointArray_ptsize(pa);

	 // this does x,y
	 memcpy(point, &pa->serialized_pointlist[size*n],sizeof(double)*2 );
}

// get a pointer to nth point of a POINTARRAY
// You'll need to cast it to appropriate dimensioned point.
// Note that if you cast to a higher dimensional point you'll
// possibly corrupt the POINTARRAY.
char *
getPoint(POINTARRAY *pa, int n)
{
	int size;

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
POINTARRAY *pointArray_construct(char *points, int ndims, uint32 npoints)
{
	POINTARRAY  *pa;
	pa = (POINTARRAY*)palloc(sizeof(POINTARRAY));

	if (ndims>4)
		elog(ERROR,"pointArray_construct:: called with dims = %i", (int) ndims);

	pa->ndims = ndims;
	pa->npoints = npoints;
	pa->serialized_pointlist = points;

	return pa;
}

// calculate the bounding box of a set of points
// returns a palloced BOX3D, or NULL on empty array.
// zmin/zmax values are set to NO_Z_VALUE if not available.
BOX3D *pointArray_bbox(POINTARRAY *pa)
{
	int t;
	BOX3D *result;
	POINT3D *pt;
	int npoints = pa->npoints;

#ifdef DEBUG
	elog(NOTICE, "pointArray_bbox call (array has %d points)", pa->npoints);
#endif
	if (pa->npoints == 0)
	{
#ifdef DEBUG
		elog(NOTICE, "pointArray_bbox returning NULL");
#endif
		return NULL;
	}

	if ( npoints != pa->npoints) elog(ERROR, "Messed up at %s:%d", __FILE__, __LINE__);

	result = (BOX3D*)palloc(sizeof(BOX3D));

	if ( npoints != pa->npoints) elog(ERROR, "Messed up at %s:%d", __FILE__, __LINE__);

	if ( ! result )
	{
		elog(NOTICE, "Out of virtual memory");
		return NULL;
	}

	//getPoint3d_p(pa, 0, (char*)pt);
	pt = (POINT3D *)getPoint(pa, 0);

	if ( npoints != pa->npoints) elog(ERROR, "Messed up at %s:%d", __FILE__, __LINE__);

#ifdef DEBUG
	elog(NOTICE, "pointArray_bbox: got point 0");
#endif

	result->xmin = pt->x;
	result->xmax = pt->x;
	result->ymin = pt->y;
	result->ymax = pt->y;

	if ( pa->ndims > 2 ) {
		result->zmin = pt->z;
		result->zmax = pt->z;
	} else {
		result->zmin = NO_Z_VALUE;
		result->zmax = NO_Z_VALUE;
	}

#ifdef DEBUG
	elog(NOTICE, "pointArray_bbox: scanning other %d points", pa->npoints);
#endif
	for (t=1;t<pa->npoints;t++)
	{
		//getPoint3d_p(pa,t,(char*)&pt);
		pt = (POINT3D *)getPoint(pa, t);
		if (pt->x < result->xmin) result->xmin = pt->x;
		if (pt->y < result->ymin) result->ymin = pt->y;
		if (pt->x > result->xmax) result->xmax = pt->x;
		if (pt->y > result->ymax) result->ymax = pt->y;

		if ( pa->ndims > 2 ) {
			if (pt->z > result->zmax) result->zmax = pt->z;
			if (pt->z < result->zmax) result->zmax = pt->z;
		}
	}

#ifdef DEBUG
	elog(NOTICE, "pointArray_bbox returning box");
#endif

	return result;
}

//size of point represeneted in the POINTARRAY
// 16 for 2d, 24 for 3d, 32 for 4d
int pointArray_ptsize(POINTARRAY *pa)
{
	if ( pa->ndims < 2 || pa->ndims > 4 )
	{
		elog(ERROR,"pointArray_ptsize:: ndims isnt 2,3, or 4");
		return 0; // never get here
	}

	return sizeof(double)*pa->ndims;
}


//***************************************************************************
// basic type handling


// returns true if this type says it has an SRID (S bit set)
bool lwgeom_hasSRID(unsigned char type)
{
	return (type & 0x40);
}

// returns either 2,3, or 4 -- 2=2D, 3=3D, 4=4D
int lwgeom_ndims(unsigned char type)
{
	return  ( (type & 0x30) >>4) +2;
}


// get base type (ie. POLYGONTYPE)
int  lwgeom_getType(unsigned char type)
{
	return (type & 0x0F);
}


//construct a type (hasBOX=false)
unsigned char lwgeom_makeType(int ndims, char hasSRID, int type)
{
	unsigned char result = type;

	if (ndims == 3)
		result = result | 0x10;
	if (ndims == 4)
		result = result | 0x20;
	if (hasSRID)
		result = result | 0x40;

	return result;
}

//construct a type
unsigned char lwgeom_makeType_full(int ndims, char hasSRID, int type, bool hasBBOX)
{
		unsigned char result = type;

		if (ndims == 3)
			result = result | 0x10;
		if (ndims == 4)
			result = result | 0x20;
		if (hasSRID)
			result = result | 0x40;
		if (hasBBOX)
			result = result | 0x80;

	return result;
}

//returns true if there's a bbox in this LWGEOM (B bit set)
bool lwgeom_hasBBOX(unsigned char type)
{
	return (type & 0x80);
}

//*****************************************************************************
// basic sub-geometry types

// handle missaligned unsigned int32 data
uint32 get_uint32(char *loc)
{
	uint32 result;

	memcpy(&result, loc, sizeof(uint32));
	return result;
}

// handle missaligned signed int32 data
int32 get_int32(char *loc)
{
	int32 result;

	memcpy(&result,loc, sizeof(int32));
	return result;
}

//******************************************************************************
// basic LWLINE functions


// construct a new LWLINE.  points will *NOT* be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWLINE *lwline_construct(int ndims, int SRID,  POINTARRAY *points)
{
	LWLINE *result;
	result = (LWLINE*) palloc( sizeof(LWLINE));

	result->ndims =ndims;
	result->SRID = SRID;
	result->points = points;

	return result;
}

// given the LWGEOM serialized form (or a pointer into a muli* one)
// construct a proper LWLINE.
// serialized_form should point to the 8bit type format (with type = 2)
// See serialized form doc
LWLINE *lwline_deserialize(char *serialized_form)
{
	unsigned char type;
	LWLINE *result;
	char *loc =NULL;
	uint32 npoints;
	POINTARRAY *pa;

	result = (LWLINE*) palloc(sizeof(LWLINE)) ;

	type = (unsigned char) serialized_form[0];
	if ( lwgeom_getType(type) != LINETYPE)
	{
		elog(ERROR,"lwline_deserialize: attempt to deserialize a line when its not really a line");
		return NULL;
	}

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//elog(NOTICE, "line has bbox");
		loc += sizeof(BOX2DFLOAT4);
	}
	else
	{
		//elog(NOTICE, "line has NO bbox");
	}

	if ( lwgeom_hasSRID(type))
	{
		//elog(NOTICE, "line has srid");
		result->SRID = get_int32(loc);
		loc +=4; // type + SRID
	}
	else
	{
		//elog(NOTICE, "line has NO srid");
		result->SRID = -1;
	}

	// we've read the type (1 byte) and SRID (4 bytes, if present)

	npoints = get_uint32(loc);
	//elog(NOTICE, "line npoints = %d", npoints);
	loc +=4;
	pa = pointArray_construct( loc, lwgeom_ndims(type), npoints);

	result->points = pa;
	result->ndims = lwgeom_ndims(type);

	return result;
}

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char  *lwline_serialize(LWLINE *line)
{
	int size=1;  // type byte
	char hasSRID;
	unsigned char * result;
	int t;
	char *loc;

if (line == NULL)
	elog(ERROR,"lwline_serialize:: given null line");

	hasSRID = (line->SRID != -1);

	if (hasSRID)
		size +=4;  //4 byte SRID

	if (line->ndims == 3)
	{
		size += 24 * line->points->npoints; //x,y,z
	}
	else if (line->ndims == 2)
	{
		size += 16 * line->points->npoints; //x,y
	}
	else if (line->ndims == 4)
	{
			size += 32 * line->points->npoints; //x,y
	}


	size+=4; // npoints

	result = palloc(size);

	result[0] = (unsigned char) lwgeom_makeType(line->ndims,hasSRID, LINETYPE);
	loc = result+1;

	if (hasSRID)
	{
		memcpy(loc, &line->SRID, sizeof(int32));
		loc += 4;
	}

	memcpy(loc, &line->points->npoints, sizeof(int32));
	loc +=4;
	//copy in points

//elog(NOTICE," line serialize - size = %i", size);

	if (line->ndims == 3)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint3d_p(line->points, t, loc);
			loc += 24; // size of a 3d point
		}
	}
	else if (line->ndims == 2)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint2d_p(line->points, t, loc);
			loc += 16; // size of a 2d point
		}
	}
	else if (line->ndims == 4)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint4d_p(line->points, t, loc);
			loc += 32; // size of a 2d point
		}
	}
	//printBYTES((unsigned char *)result, size);
	return result;
}

// convert this line into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
void lwline_serialize_buf(LWLINE *line, char *buf, int *retsize)
{
	int size=1;  // type byte
	char hasSRID;
	int t;
	char *loc;

	if (line == NULL)
		elog(ERROR,"lwline_serialize:: given null line");

	hasSRID = (line->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	if (line->ndims == 3)
	{
		size += 24 * line->points->npoints; //x,y,z
	}
	else if (line->ndims == 2)
	{
		size += 16 * line->points->npoints; //x,y
	}
	else if (line->ndims == 4)
	{
			size += 32 * line->points->npoints; //x,y
	}

	size+=4; // npoints

	buf[0] = (unsigned char) lwgeom_makeType(line->ndims,
		hasSRID, LINETYPE);
	loc = buf+1;

	if (hasSRID)
	{
		memcpy(loc, &line->SRID, sizeof(int32));
		loc += 4;
	}

	memcpy(loc, &line->points->npoints, sizeof(int32));
	loc +=4;
	//copy in points

//elog(NOTICE," line serialize - size = %i", size);

	if (line->ndims == 3)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint3d_p(line->points, t, loc);
			loc += 24; // size of a 3d point
		}
	}
	else if (line->ndims == 2)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint2d_p(line->points, t, loc);
			loc += 16; // size of a 2d point
		}
	}
	else if (line->ndims == 4)
	{
		for (t=0; t< line->points->npoints;t++)
		{
			getPoint4d_p(line->points, t, loc);
			loc += 32; // size of a 2d point
		}
	}
	//printBYTES((unsigned char *)result, size);

	*retsize = size;
}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwline_findbbox(LWLINE *line)
{
	BOX3D *ret;

	if (line == NULL)
		return NULL;

	ret = pointArray_bbox(line->points);
	return ret;
}

//find length of this serialized line
uint32 lwline_findlength(char *serialized_line)
{
	int type = (unsigned char) serialized_line[0];
	uint32 result =1;  //type
	char *loc;
	uint32 npoints;

	if ( lwgeom_getType(type) != LINETYPE)
		elog(ERROR,"lwline_findlength::attempt to find the length of a non-line");


	loc = serialized_line+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

		if ( lwgeom_hasSRID(type))
		{
			loc += 4; // type + SRID
			result +=4;
		}

		// we've read the type (1 byte) and SRID (4 bytes, if present)

		npoints = get_uint32(loc);
		result += 4; //npoints

		if (lwgeom_ndims(type) ==3)
		{
			return result + npoints * 24;
		}
		else if (lwgeom_ndims(type) ==2)
		{
			return result+ npoints * 16;
		}
		else if (lwgeom_ndims(type) ==4)
		{
			return result+ npoints * 32;
		}
		elog(ERROR,"lwline_findlength :: invalid ndims");
		return 0; //never get here
}

//********************************************************************
// support for the LWPOINT sub-type

// construct a new point.  point will not be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOINT  *lwpoint_construct(int ndims, int SRID, POINTARRAY *point)
{
	LWPOINT *result ;

	if (point == NULL)
		return NULL; // error

	result = palloc(sizeof(LWPOINT));
	result->ndims = ndims;
	result->SRID = SRID;

	result->point = point;

	return result;
}

// given the LWPOINT serialized form (or a pointer into a muli* one)
// construct a proper LWPOINT.
// serialized_form should point to the 8bit type format (with type = 1)
// See serialized form doc
LWPOINT *lwpoint_deserialize(char *serialized_form)
{
	unsigned char type;
	LWPOINT *result;
	char *loc = NULL;
	POINTARRAY *pa;

#ifdef DEBUG
	elog(NOTICE, "lwpoint_deserialize called");
#endif

	result = (LWPOINT*) palloc(sizeof(LWPOINT)) ;

	type = (unsigned char) serialized_form[0];

	if ( lwgeom_getType(type) != POINTTYPE) return NULL;

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
#ifdef DEBUG
		elog(NOTICE, "lwpoint_deserialize: input has bbox");
#endif
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
		elog(NOTICE, "lwpoint_deserialize: input has SRID");
#endif
		result->SRID = get_int32(loc);
		loc += 4; // type + SRID
	}
	else
	{
		result->SRID = -1;
	}

	// we've read the type (1 byte) and SRID (4 bytes, if present)

	pa = pointArray_construct(loc, lwgeom_ndims(type), 1);

	result->point = pa;
	result->ndims = lwgeom_ndims(type);

	return result;
}

// convert this point into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char  *lwpoint_serialize(LWPOINT *point)
{
	int size=1;
	char hasSRID;
	char *result;
	char *loc;

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	if (point->ndims == 3) size += 24; //x,y,z
	else if (point->ndims == 2) size += 16 ; //x,y,z
	else if (point->ndims == 4) size += 32 ; //x,y,z,m

	result = palloc(size);

	result[0] = (unsigned char) lwgeom_makeType(point->ndims,
		hasSRID, POINTTYPE);
	loc = result+1;

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points

	if (point->ndims == 3) getPoint3d_p(point->point, 0, loc);
	else if (point->ndims == 2) getPoint2d_p(point->point, 0, loc);
	else if (point->ndims == 4) getPoint4d_p(point->point, 0, loc);
	return result;
}

// convert this point into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
void lwpoint_serialize_buf(LWPOINT *point, char *buf, int *retsize)
{
	int size=1;
	char hasSRID;
	char *loc;

	hasSRID = (point->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	if (point->ndims == 3) size += 24; //x,y,z
	else if (point->ndims == 2) size += 16 ; //x,y,z
	else if (point->ndims == 4) size += 32 ; //x,y,z,m

	buf[0] = (unsigned char) lwgeom_makeType(point->ndims,
		hasSRID, POINTTYPE);
	loc = buf+1;

	if (hasSRID)
	{
		memcpy(loc, &point->SRID, sizeof(int32));
		loc += 4;
	}

	//copy in points

	if (point->ndims == 3) getPoint3d_p(point->point, 0, loc);
	else if (point->ndims == 2) getPoint2d_p(point->point, 0, loc);
	else if (point->ndims == 4) getPoint4d_p(point->point, 0, loc);

	*retsize = size;
}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwpoint_findbbox(LWPOINT *point)
{
#ifdef DEBUG
	elog(NOTICE, "lwpoint_findbbox called with point %p", point);
#endif
	if (point == NULL)
	{
#ifdef DEBUG
		elog(NOTICE, "lwpoint_findbbox returning NULL");
#endif
		return NULL;
	}

#ifdef DEBUG
	elog(NOTICE, "lwpoint_findbbox returning pointArray_bbox return");
#endif

	return pointArray_bbox(point->point);
}

// convenience functions to hide the POINTARRAY
POINT2D lwpoint_getPoint2d(LWPOINT *point)
{
	POINT2D result;

	if (point == NULL)
			return result;

	return getPoint2d(point->point,0);
}

// convenience functions to hide the POINTARRAY
POINT3D lwpoint_getPoint3d(LWPOINT *point)
{
	POINT3D result;

	if (point == NULL)
			return result;

	return getPoint3d(point->point,0);
}


//find length of this serialized point
uint32 lwpoint_findlength(char *serialized_point)
{
	uint  result = 1;
	unsigned char type;
	char *loc;

	type = (unsigned char) serialized_point[0];

	if ( lwgeom_getType(type) != POINTTYPE) return 0;

#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength called (%d)", result);
#endif

	loc = serialized_point+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength: has bbox (%d)", result);
#endif
	}

	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength: has srid (%d)", result);
#endif
		loc +=4; // type + SRID
		result +=4;
	}

	if (lwgeom_ndims(type) == 3)
	{
#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength: returning (%d)", result+24);
#endif
		return result + 24;
	}
	else if (lwgeom_ndims(type) == 2)
	{
#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength: returning (%d)", result+16);
#endif
		return result + 16;
	}
	else if (lwgeom_ndims(type) == 4)
	{
#ifdef DEBUG
elog(NOTICE, "lwpoint_findlength: returning (%d)", result+32);
#endif
		return result + 32;
	}

    	elog(ERROR,"lwpoint_findlength :: invalid ndims = %i",
		lwgeom_ndims(type));
	return 0; //never get here
}


//********************************************************************
// basic polygon manipulation

// construct a new LWPOLY.  arrays (points/points per ring) will NOT be copied
// use SRID=-1 for unknown SRID (will have 8bit type's S = 0)
LWPOLY *lwpoly_construct(int ndims, int SRID, int nrings,POINTARRAY **points)
{
	LWPOLY *result;

	result = (LWPOLY*) palloc(sizeof(LWPOLY));
	result->ndims = ndims;
	result->SRID = SRID;
	result->nrings = nrings;
	result->rings = points;

	return result;
}


// given the LWPOLY serialized form (or a pointer into a muli* one)
// construct a proper LWPOLY.
// serialized_form should point to the 8bit type format (with type = 3)
// See serialized form doc
LWPOLY *lwpoly_deserialize(char *serialized_form)
{

	LWPOLY *result;
	uint32 nrings;
	int ndims;
	uint32 npoints;
	unsigned char type;
	char  *loc;
	int t;

	if (serialized_form == NULL)
	{
		elog(ERROR, "lwpoly_deserialize called with NULL arg");
		return NULL;
	}

	result = (LWPOLY*) palloc(sizeof(LWPOLY));


	type = (unsigned  char) serialized_form[0];
	ndims = lwgeom_ndims(type);
	loc = serialized_form;

	if ( lwgeom_getType(type) != POLYGONTYPE)
	{
		elog(ERROR, "lwpoly_deserialize called with arg of type %d",
			lwgeom_getType(type));
		return NULL;
	}


	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( lwgeom_hasSRID(type))
	{
		result->SRID = get_int32(loc);
		loc +=4; // type + SRID
	}
	else
	{
		result->SRID = -1;
	}

	nrings = get_uint32(loc);
	result->nrings = nrings;
	loc +=4;
	result->rings = (POINTARRAY**) palloc(nrings* sizeof(POINTARRAY*));

	for (t =0;t<nrings;t++)
	{
		//read in a single ring and make a PA
		npoints = get_uint32(loc);
		loc +=4;

		result->rings[t] = pointArray_construct(loc, ndims, npoints);
		if (ndims == 3)
			loc += 24*npoints;
		else if (ndims == 2)
			loc += 16*npoints;
		else if (ndims == 4)
			loc += 32*npoints;
	}
	result->ndims = ndims;

	return result;
}

// create the serialized form of the polygon
// result's first char will be the 8bit type.  See serialized form doc
// points copied
char *lwpoly_serialize(LWPOLY *poly)
{
	int size=1;  // type byte
	char hasSRID;
	char *result;
	int t,u;
	int total_points = 0;
	int npoints;
	char *loc;

	hasSRID = (poly->SRID != -1);

	if (hasSRID)
		size +=4;  //4 byte SRID

	size += 4; // nrings
	size += 4*poly->nrings; //npoints/ring


	for (t=0;t<poly->nrings;t++)
	{
		total_points  += poly->rings[t]->npoints;
	}
	if (poly->ndims == 3)
		size += 24*total_points;
	else if (poly->ndims == 2)
		size += 16*total_points;
	else if (poly->ndims == 4)
		size += 32*total_points;

	result = palloc(size);

	result[0] = (unsigned char) lwgeom_makeType(poly->ndims,hasSRID, POLYGONTYPE);
	loc = result+1;

	if (hasSRID)
	{
		memcpy(loc, &poly->SRID, sizeof(int32));
		loc += 4;
	}

	memcpy(loc, &poly->nrings, sizeof(int32));  // nrings
	loc+=4;



	for (t=0;t<poly->nrings;t++)
	{
		POINTARRAY *pa = poly->rings[t];
		npoints = poly->rings[t]->npoints;
		memcpy(loc, &npoints, sizeof(int32)); //npoints this ring
		loc+=4;
		if (poly->ndims == 3)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint3d_p(pa, u, loc);
				loc+= 24;
			}
		}
		else if (poly->ndims == 2)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint2d_p(pa, u, loc);
				loc+= 16;
			}
		}
		else if (poly->ndims == 4)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint4d_p(pa, u, loc);
				loc+= 32;
			}
		}
	}

	return result;
}

// create the serialized form of the polygon writing it into the
// given buffer, and returning number of bytes written into
// the given int pointer.
// result's first char will be the 8bit type.  See serialized form doc
// points copied
void lwpoly_serialize_buf(LWPOLY *poly, char *buf, int *retsize)
{
	int size=1;  // type byte
	char hasSRID;
	int t,u;
	int total_points = 0;
	int npoints;
	char *loc;

	hasSRID = (poly->SRID != -1);

	if (hasSRID) size +=4;  //4 byte SRID

	size += 4; // nrings
	size += 4*poly->nrings; //npoints/ring

	for (t=0;t<poly->nrings;t++)
	{
		total_points  += poly->rings[t]->npoints;
	}
	if (poly->ndims == 3) size += 24*total_points;
	else if (poly->ndims == 2) size += 16*total_points;
	else if (poly->ndims == 4) size += 32*total_points;

	buf[0] = (unsigned char) lwgeom_makeType(poly->ndims,
		hasSRID, POLYGONTYPE);
	loc = buf+1;

	if (hasSRID)
	{
		memcpy(loc, &poly->SRID, sizeof(int32));
		loc += 4;
	}

	memcpy(loc, &poly->nrings, sizeof(int32));  // nrings
	loc+=4;

	for (t=0;t<poly->nrings;t++)
	{
		POINTARRAY *pa = poly->rings[t];
		npoints = poly->rings[t]->npoints;
		memcpy(loc, &npoints, sizeof(int32)); //npoints this ring
		loc+=4;
		if (poly->ndims == 3)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint3d_p(pa, u, loc);
				loc+= 24;
			}
		}
		else if (poly->ndims == 2)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint2d_p(pa, u, loc);
				loc+= 16;
			}
		}
		else if (poly->ndims == 4)
		{
			for (u=0;u<npoints;u++)
			{
				getPoint4d_p(pa, u, loc);
				loc+= 32;
			}
		}
	}

	*retsize = size;
}


// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwpoly_findbbox(LWPOLY *poly)
{
//	int t;

	BOX3D *result;
//	BOX3D *abox,*abox2;

	POINTARRAY *pa = poly->rings[0];   // just need to check outer ring -- interior rings are inside
	result  = pointArray_bbox(pa);

//	for (t=1;t<poly->nrings;t++)
	//{
//		pa = poly->rings[t];
//		abox  = pointArray_bbox(pa);
//		abox2 = result;
//		result = combine_boxes( abox, abox2);
//		pfree(abox);
//		pfree(abox2);
  //  }

    return result;
}

//find length of this serialized polygon
uint32 lwpoly_findlength(char *serialized_poly)
{
	uint32 result = 1; // char type
	uint32 nrings;
	int   ndims;
	int t;
	unsigned char type;
	uint32 npoints;
	char *loc;

	if (serialized_poly == NULL)
		return -9999;


	type = (unsigned char) serialized_poly[0];
	ndims = lwgeom_ndims(type);

	if ( lwgeom_getType(type) != POLYGONTYPE)
		return -9999;


	loc = serialized_poly+1;

	if (lwgeom_hasBBOX(type))
	{
#ifdef DEBUG
		elog(NOTICE, "lwpoly_findlength: has bbox");
#endif
		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}


	if ( lwgeom_hasSRID(type))
	{
#ifdef DEBUG
		elog(NOTICE, "lwpoly_findlength: has srid");
#endif
		loc +=4; // type + SRID
		result += 4;
	}


	nrings = get_uint32(loc);
	loc +=4;
	result +=4;


	for (t =0;t<nrings;t++)
	{
		//read in a single ring and make a PA
		npoints = get_uint32(loc);
		loc +=4;
		result +=4;

		if (ndims == 3)
		{
			loc += 24*npoints;
			result += 24*npoints;
		}
		else if (ndims == 2)
		{
			loc += 16*npoints;
			result += 16*npoints;
		}
		else if (ndims == 4)
		{
			loc += 32*npoints;
			result += 32*npoints;
		}
	}
	return result;
}


//*************************************************************************
// multi-geometry support



// note - for a simple type (ie. point), this will have sub_geom[0] = serialized_form.
// for multi-geomtries sub_geom[0] will be a few bytes into the serialized form
// This function just computes the length of each sub-object and pre-caches this info.
// For a geometry collection of multi* geometries, you can inspect the sub-components
// as well.
LWGEOM_INSPECTED *lwgeom_inspect(char *serialized_form)
{
	LWGEOM_INSPECTED *result = palloc(sizeof(LWGEOM_INSPECTED));
	unsigned char type;
	char **sub_geoms;
	char *loc;
	int 	t;

	if (serialized_form == NULL)
		return NULL;

	result->serialized_form = serialized_form;
	result->type = (unsigned char) serialized_form[0];
	result->SRID = -1; // assume

	type = lwgeom_getType( (unsigned char) serialized_form[0]);


	loc = serialized_form+1;

	if (lwgeom_hasBBOX((unsigned char) serialized_form[0]))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( (type==POINTTYPE) || (type==LINETYPE) || (type==POLYGONTYPE) )
	{
		if (lwgeom_hasSRID((unsigned char) serialized_form[0]) )
		{
			result->SRID = get_int32(loc);
		}
		//simple geometry (point/line/polygon)-- not multi!
		result->ngeometries = 1;
		sub_geoms = (char**) palloc(sizeof(char*));
		sub_geoms[0] = serialized_form;
		result->sub_geoms = sub_geoms;
		return result;
	}

	// its a GeometryCollection or multi* geometry

	if (lwgeom_hasSRID((unsigned char) serialized_form[0]) )
	{
		result->SRID=  get_int32(loc);
		loc += 4;
	}

	result->ngeometries = get_uint32(loc);
	loc +=4;
#ifdef DEBUG
	elog(NOTICE, "lwgeom_inspect: geometry is a collection of %d elements",
		result->ngeometries);
#endif

	if ( ! result->ngeometries ) return result;

	sub_geoms = (char**) palloc(sizeof(char*) * result->ngeometries );
	result->sub_geoms = sub_geoms;
	sub_geoms[0] = loc;
#ifdef DEBUG
	elog(NOTICE, "subgeom[0] @ %p", sub_geoms[0]);
#endif
	for (t=1;t<result->ngeometries; t++)
	{
		int sub_length = lwgeom_seralizedformlength(sub_geoms[t-1], -1);//-1 = entire object
		sub_geoms[t] = sub_geoms[t-1] + sub_length;
#ifdef DEBUG
		elog(NOTICE, "subgeom[%d] @ %p (sub_length: %d)",
			t, sub_geoms[t], sub_length);
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
char *lwgeom_getsubgeometry(char *serialized_form, int geom_number)
{
	//major cheat!!
	char * result;
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	result = lwgeom_getsubgeometry_inspected(inspected, geom_number);
	pfree_inspected(inspected);
	return result;
}

char *lwgeom_getsubgeometry_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
	if ((geom_number <0) || (geom_number >= inspected->ngeometries) )
		return NULL;

	return inspected->sub_geoms[geom_number];
}


// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//                 --> point
// gets the 8bit type of the geometry at location geom_number
char lwgeom_getsubtype(char *serialized_form, int geom_number)
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
extern char *lwgeom_construct(int SRID,int finalType,int ndims, int nsubgeometries, char **serialized_subs)
{
	uint32 *lengths;
	int t;
	int total_length = 0;
	char type = -1;
	char this_type = -1;
	char *result;
	char *loc;

	if (nsubgeometries == 0)
		return lwgeom_constructempty(SRID,ndims);

	lengths = palloc(sizeof(int32) * nsubgeometries);

	for (t=0;t<nsubgeometries;t++)
	{
		lengths[t] = lwgeom_seralizedformlength(serialized_subs[t],-1);
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

	result = palloc(total_length);
	result[0] = (unsigned char) lwgeom_makeType( ndims, SRID != -1,  type);
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

	pfree(lengths);
	return result;
}


// construct the empty geometry (GEOMETRYCOLLECTION(EMPTY))
//returns serialized form
char *lwgeom_constructempty(int SRID,int ndims)
{
	int size = 0;
	char *result;
	int ngeoms = 0;
	char *loc;

	if (SRID != -1)
		size +=4;

	size += 5;

	result = palloc(size);

	result[0] =(unsigned char) lwgeom_makeType( ndims, SRID != -1,  COLLECTIONTYPE);
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

// helper function (not for general use)
// find the size a geometry (or a sub-geometry)
// 1st geometry has geom_number = 0
//  use geom_number = -1 to find the actual type of the serialized form.
//    ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, -1)
//                 --> size of the multipoint
//   ie lwgeom_gettype( <'MULTIPOINT(0 0, 1 1)'>, 0)
//         --> size of the point

// take a geometry, and find its length
int lwgeom_seralizedformlength_simple(char *serialized_form)
{
	unsigned char type = lwgeom_getType((unsigned char) serialized_form[0]);
	int t;
	char *loc;
	uint32 ngeoms;
	int sub_size;
	int result = 1; //"type"

#ifdef DEBUG
	elog(NOTICE, "lwgeom_seralizedformlength_simple called");
#endif

	if (type == POINTTYPE)
	{
#ifdef DEBUG
		elog(NOTICE, "lwgeom_seralizedformlength_simple: is a point");
#endif
		return lwpoint_findlength(serialized_form);
	}
	else if (type == LINETYPE)
	{
#ifdef DEBUG
		elog(NOTICE, "lwgeom_seralizedformlength_simple: is a line");
#endif
		return lwline_findlength(serialized_form);
	}
	else if (type == POLYGONTYPE)
	{
#ifdef DEBUG
		elog(NOTICE, "lwgeom_seralizedformlength_simple: is a polygon");
#endif
		return lwpoly_findlength(serialized_form);
	}

	if ( type == 0 )
	{
		elog(ERROR, "lwgeom_seralizedformlength_simple called with unknown-typed serialized geometry");
		return 0;
	}

	//handle all the multi* and geometrycollections the same
	//NOTE: for a geometry collection of GC of GC of GC we will be recursing...

#ifdef DEBUG
	elog(NOTICE, "lwgeom_seralizedformlength_simple called on a geoemtry with type %d", type);
#endif

	loc = serialized_form+1;

	if (lwgeom_hasBBOX((unsigned char) serialized_form[0]))
	{
#ifdef DEBUG
		elog(NOTICE, "lwgeom_seralizedformlength_simple: has bbox");
#endif

		loc += sizeof(BOX2DFLOAT4);
		result +=sizeof(BOX2DFLOAT4);
	}

	if (lwgeom_hasSRID( (unsigned char) serialized_form[0]) )
	{
#ifdef DEBUG
		elog(NOTICE, "lwgeom_seralizedformlength_simple: has srid");
#endif
		result +=4;
		loc +=4;
	}


	ngeoms =  get_uint32(loc);
	loc +=4;
	result += 4; // numgeoms

#ifdef DEBUG
	elog(NOTICE, "lwgeom_seralizedformlength_simple called on a geoemtry with %d elems (result so far: %d)", ngeoms, result);
#endif

	for (t=0;t<ngeoms;t++)
	{
		sub_size = lwgeom_seralizedformlength_simple(loc);
#ifdef DEBUG
		elog(NOTICE, " subsize %d", sub_size);
#endif
		loc += sub_size;
		result += sub_size;
	}

#ifdef DEBUG
	elog(NOTICE, "lwgeom_seralizedformlength_simple returning %d", result);
#endif
	return result;
}

int lwgeom_seralizedformlength(char *serialized_form, int geom_number)
{
	if (geom_number == -1)
	{
		return lwgeom_seralizedformlength_simple(serialized_form);
	}
	return lwgeom_seralizedformlength_simple( lwgeom_getsubgeometry(serialized_form,geom_number));
}



int lwgeom_seralizedformlength_inspected(LWGEOM_INSPECTED *inspected, int geom_number)
{
		return lwgeom_seralizedformlength( inspected->sub_geoms[geom_number], -1);
}


//get bounding box of LWGEOM (automatically calls the sub-geometries bbox generators)
//dont forget to pfree() result
BOX3D *lw_geom_getBB(char *serialized_form)
{
	  LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized_form);

	  BOX3D *result = lw_geom_getBB_inspected(inspected);

	  pfree_inspected(inspected);
	  return result;
}

//dont forget to pfree() result
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
elog(NOTICE, "lw_geom_getBB_simple called on type %d", type);
#endif

	if (type == POINTTYPE)
	{
		LWPOINT *pt = lwpoint_deserialize(serialized_form);
#ifdef DEBUG
elog(NOTICE, "lw_geom_getBB_simple: lwpoint deserialized");
#endif
		result = lwpoint_findbbox(pt);
#ifdef DEBUG
elog(NOTICE, "lw_geom_getBB_simple: bbox found");
#endif
		pfree_point(pt);
		return result;
	/*
		result = palloc(sizeof(BOX3D));
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
		elog(NOTICE, "lw_geom_getBB_simple called on unknown type %d", type);
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
		sub_size = lwgeom_seralizedformlength_simple(loc);
		loc += sub_size;
		if (result != NULL)
		{
			b2= result;
			result = combine_boxes(b2, b1);
			pfree(b1);
			pfree(b2);
		}
		else
		{
			result = b1;
		}
	}

	return result;

}


//dont forget to pfree() result
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

		//elog(NOTICE,"%i has box :: BBOX3D(%g %g, %g %g)",t,b1->xmin, b1->ymin,b1->xmax, b1->ymax);

		if (result != NULL)
		{
			b2= result;
			result = combine_boxes(b2, b1);
//	elog(NOTICE,"combined has :: BBOX3D(%g %g, %g %g)",result->xmin, result->ymin,result->xmax, result->ymax);

			pfree(b1);
			pfree(b2);
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
		pfree(inspected->sub_geoms);
	pfree(inspected);
}

void pfree_point    (LWPOINT *pt)
{
	pfree_POINTARRAY(pt->point);
	pfree(pt);
}

void pfree_line     (LWLINE  *line)
{
	pfree(line->points);
	pfree(line);
}

void pfree_polygon  (LWPOLY  *poly)
{
	int t;

	for (t=0;t<poly->nrings;t++)
	{
		pfree_POINTARRAY(poly->rings[t]);
	}

	pfree(poly);
}

void pfree_POINTARRAY(POINTARRAY *pa)
{
	pfree(pa);
}




//************************************************
//** debugging routines


void printLWLINE(LWLINE *line)
{
	elog(NOTICE,"LWLINE {");
	elog(NOTICE,"    ndims = %i", (int)line->ndims);
	elog(NOTICE,"    SRID = %i", (int)line->SRID);
	printPA(line->points);
	elog(NOTICE,"}");
}

void printLWPOINT(LWPOINT *point)
{
	elog(NOTICE,"LWPOINT {");
	elog(NOTICE,"    ndims = %i", (int)point->ndims);
	elog(NOTICE,"    SRID = %i", (int)point->SRID);
	printPA(point->point);
	elog(NOTICE,"}");
}

void printPA(POINTARRAY *pa)
{
	int t;
	POINT2D pt2;
	POINT3D pt3;
	POINT4D pt4;

	elog(NOTICE,"      POINTARRAY{");
	elog(NOTICE,"                 ndims =%i,   ptsize=%i", (int) pa->ndims,pointArray_ptsize(pa));
	elog(NOTICE,"                 npoints = %i", pa->npoints);

	for (t =0; t<pa->npoints;t++)
	{
		if (pa->ndims == 2)
		{
			pt2 = getPoint2d(pa,t);
			elog(NOTICE,"                    %i : %lf,%lf",t,pt2.x,pt2.y);
		}
		if (pa->ndims == 3)
		{
			pt3 = getPoint3d(pa,t);
			elog(NOTICE,"                    %i : %lf,%lf,%lf",t,pt3.x,pt3.y,pt3.z);
		}
		if (pa->ndims == 4)
		{
			pt4 = getPoint4d(pa,t);
			elog(NOTICE,"                    %i : %lf,%lf,%lf,%lf",t,pt3.x,pt4.y,pt4.z,pt4.m);
		}
	}

	elog(NOTICE,"      }");
}

void printBYTES(unsigned char *a, int n)
{
	int t;
	char buff[3];

	buff[2] = 0; //null terminate

	elog(NOTICE," BYTE ARRAY (n=%i) IN HEX: {", n);
	for (t=0;t<n;t++)
	{
		deparse_hex(a[t], buff);
		elog(NOTICE, "    %i : %s", t,buff );
	}
	elog(NOTICE, "  }");
}


void printLWPOLY(LWPOLY *poly)
{
	int t;
	elog(NOTICE,"LWPOLY {");
	elog(NOTICE,"    ndims = %i", (int)poly->ndims);
	elog(NOTICE,"    SRID = %i", (int)poly->SRID);
	elog(NOTICE,"    nrings = %i", (int)poly->nrings);
	for (t=0;t<poly->nrings;t++)
	{
		elog(NOTICE,"    RING # %i :",t);
		printPA(poly->rings[t]);
	}
	elog(NOTICE,"}");
}

void printMULTI(char *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	LWLINE  *line;
	LWPOINT *point;
	LWPOLY  *poly;
	int t;

	elog(NOTICE,"MULTI* geometry (type = %i), with %i sub-geoms",lwgeom_getType((unsigned char)serialized[0]), inspected->ngeometries);

	for (t=0;t<inspected->ngeometries;t++)
	{
		elog(NOTICE,"      sub-geometry %i:", t);
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

    elog(NOTICE,"end multi*");

	pfree_inspected(inspected);
}

void printType(unsigned char type)
{
	elog(NOTICE,"type 0x%x ==> hasBBOX=%i, hasSRID=%i, ndims=%i, type=%i",(unsigned int) type, lwgeom_hasBBOX(type), lwgeom_hasSRID(type),lwgeom_ndims(type), lwgeom_getType(type));
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
int lwgeom_getSRID(LWGEOM *lwgeom)
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
// Allocation will be done using the palloc.
LWGEOM *lwgeom_setSRID(LWGEOM *lwgeom, int32 newSRID)
{
	unsigned char type = lwgeom->type;
	int bbox_offset=0; //0=no bbox, otherwise sizeof(BOX2DFLOAT4)
	int len,len_new,len_left;
	LWGEOM *result;
	char *loc_new, *loc_old;

	if (lwgeom_hasBBOX(type))
		bbox_offset = sizeof(BOX2DFLOAT4);

	len = lwgeom->size;

	if (lwgeom_hasSRID(type))
	{
		//we create a new one and copy the SRID in
		result = palloc(len);
		memcpy(result, lwgeom, len);
		memcpy(result->data+bbox_offset, &newSRID,4);
	}
	else  // need to add one
	{
		len_new = len + 4;//+4 for SRID
		result = palloc(len_new);
		memcpy(result, &len_new, 4); // size copy in
		result->type = lwgeom_makeType_full(lwgeom_ndims(type), true, lwgeom_getType(type),lwgeom_hasBBOX(type));

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

		// TODO: add SRID presence flag in type.
	}
	return result;
}

LWGEOM *
LWGEOM_construct(char *ser, int SRID, int wantbbox)
{
	int size;
	char *iptr, *optr, *eptr;
	int wantsrid = 0;
	BOX2DFLOAT4 box;
	LWGEOM *result;

	size = lwgeom_seralizedformlength_simple(ser);
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

	result = palloc(size);
	result->size = size;

	result->type = lwgeom_makeType_full(lwgeom_ndims(ser[0]),
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
		pfree(exploded->points);
	if ( exploded->nlines )
		pfree(exploded->lines);
	if ( exploded->npolys )
		pfree(exploded->polys);
	pfree(exploded);
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
	elog(NOTICE, "lwgeom_explode called");
#endif

	inspected = lwgeom_inspect(serialized);


#ifdef DEBUG
elog(NOTICE, "lwgeom_explode: serialized inspected");
#endif

	result = palloc(sizeof(LWGEOM_EXPLODED));
	result->points = palloc(1);
	result->lines = palloc(1);
	result->polys = palloc(1);
	result->npoints = 0;
	result->nlines = 0;
	result->npolys = 0;

	if ( ! inspected->ngeometries )
	{
		pfree(result->points);
		pfree(result->lines);
		pfree(result->polys);
		result->SRID = -1;
		result->ndims = 0;
		pfree_inspected(inspected);
		elog(NOTICE, "lwgeom_explode: no geometries");
		return result;
	}

	result->SRID = lwgeom_getsrid(serialized);
	result->ndims = lwgeom_ndims(serialized[0]);

	for (i=0; i<inspected->ngeometries; i++)
	{

		char *subgeom = inspected->sub_geoms[i];
		int type = lwgeom_getType(subgeom[0]);

		if ( type == POINTTYPE )
		{
#ifdef DEBUG
elog(NOTICE, "lwgeom_explode: it's a point");
#endif
			result->points = repalloc(result->points,
				(result->npoints+1)*sizeof(char *));
			result->points[result->npoints] = subgeom;
			result->npoints++;
			continue;
		}

		if ( type == LINETYPE )
		{
#ifdef DEBUG
elog(NOTICE, "lwgeom_explode: it's a line");
#endif
			result->lines = repalloc(result->lines,
				(result->nlines+1)*sizeof(char *));
			result->lines[result->nlines] = subgeom;
			result->nlines++;
			continue;
		}

		if ( type == POLYGONTYPE )
		{
#ifdef DEBUG
elog(NOTICE, "lwgeom_explode: it's a polygon");
#endif
			result->polys = repalloc(result->polys,
				(result->npolys+1)*sizeof(char *));
			result->polys[result->npolys] = subgeom;
			result->npolys++;
			continue;
		}

#ifdef DEBUG
		elog(NOTICE, "type of subgeom %d is %d, recursing", i, type);
#endif

		// it's a multi geometry, recurse
		subexploded = lwgeom_explode(subgeom);

#ifdef DEBUG
		elog(NOTICE, "subgeom %d, exploded: %d point, %d lines, %d polys", i, subexploded->npoints, subexploded->nlines, subexploded->npolys);
#endif

		// Re-allocate adding space for new exploded geoms
		// (-1 because 1 was already allocated for the collection)
		// Copy subgeom pointers from subexploded to current
		// exploded.

		if ( subexploded->npoints )
		{
			result->points = repalloc(result->points,
				sizeof(char *)*(result->npoints+subexploded->npoints-1));
			if ( ! result )
				elog(ERROR, "Out of virtual memory");

#ifdef DEBUG
			elog(NOTICE, "repalloc'ed exploded->points");
#endif

			memcpy(&(result->points[result->npoints]),
				subexploded->points,
				subexploded->npoints*sizeof(char *));

#ifdef DEBUG
			elog(NOTICE, "memcpied exploded->points");
#endif

			result->npoints += subexploded->npoints;

#ifdef DEBUG
			elog(NOTICE, "memcopied %d points from subexploded (exploded points: %d", subexploded->npoints, result->npoints);
#endif
		}

		if ( subexploded->nlines )
		{
			result->lines = repalloc(result->lines,
				sizeof(char *)*
				(result->nlines+subexploded->nlines-1));

			memcpy(&(result->lines[result->nlines]),
				subexploded->lines,
				subexploded->nlines*sizeof(char *));

			result->nlines += subexploded->nlines;
		}

		if ( subexploded->npolys )
		{
			result->polys = repalloc(result->polys,
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
elog(NOTICE, "lwgeom_explode: returning");
#endif

	return result;
}

// Returns a 'palloced' union of the two input exploded geoms
// Returns NULL if SRID or ndims do not match.
LWGEOM_EXPLODED *
lwexploded_sum(LWGEOM_EXPLODED *exp1, LWGEOM_EXPLODED *exp2)
{
	LWGEOM_EXPLODED *expcoll;
	char *loc;

	if ( exp1->ndims != exp2->ndims ) return NULL;
	if ( exp1->SRID != exp2->SRID ) return NULL;

	expcoll = palloc(sizeof(LWGEOM_EXPLODED));

	expcoll->npoints = exp1->npoints + exp2->npoints;
	if ( expcoll->npoints ) {
		expcoll->points = (char **)palloc(expcoll->npoints*sizeof(char *));
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
		expcoll->lines = palloc(expcoll->nlines*sizeof(char *));
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
		expcoll->polys = palloc(expcoll->npolys*sizeof(char *));
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

	expcoll->ndims = exp1->ndims;
	expcoll->SRID = exp1->SRID;

	return expcoll;
}


/*
 * Serialized a LWGEOM_EXPLODED structure
 */
char *
lwexploded_serialize(LWGEOM_EXPLODED *exploded, int wantbbox)
{
	unsigned int size=0;
	int i;
	int ntypes = 0;
	int ngeoms = 0;
	char *result, *loc;
	int outtype = 0;
	LWPOLY *poly;
	LWLINE *line;
	LWPOINT *point;
	BOX2DFLOAT4 *box2d;
	BOX3D *box3d;
	char *ser;

	if ( exploded->npoints + exploded->nlines + exploded->npolys == 0 )
	{
		return lwgeom_constructempty(exploded->SRID, exploded->ndims);
	}

	// find size of all geoms.
	// If BBOX and SRID are included this size could be
	// larger then needed, but that should not be a problem
	for (i=0; i<exploded->npoints; i++)
		size += lwpoint_findlength(exploded->points[i]);
	for (i=0; i<exploded->nlines; i++)
		size += lwline_findlength(exploded->lines[i]);
	for (i=0; i<exploded->npolys; i++)
		size += lwpoly_findlength(exploded->polys[i]);

	if ( exploded->npoints )
	{
		ntypes++;
		outtype = (exploded->npoints>1) ? MULTIPOINTTYPE : POINTTYPE;
	}
	if ( exploded->nlines )
	{
		ntypes++;
		if ( outtype ) outtype = COLLECTIONTYPE;
		else outtype = (exploded->nlines>1) ? MULTILINETYPE : LINETYPE;
	}
	if ( exploded->npolys )
	{
		ntypes++;
		if ( outtype ) outtype = COLLECTIONTYPE;
		else outtype = (exploded->npolys>1) ? MULTIPOLYGONTYPE : POLYGONTYPE;
	}

	ngeoms = exploded->npoints + exploded->nlines + exploded->npolys;

#ifdef DEBUG
	elog(NOTICE, " computed outtype: %d, ngeoms: %d", outtype, ngeoms);
#endif

	if ( ! ngeoms )
	{
		return lwgeom_constructempty(exploded->SRID, exploded->ndims);
	}


	// For a single geometry just set SRID and BBOX (if requested)
	if ( ngeoms < 2 )
	{
		if ( exploded->npoints )
		{
			point = lwpoint_deserialize(exploded->points[0]);
			point->SRID = exploded->SRID;
			ser = lwpoint_serialize(point);
			pfree_point(point);
			size = lwpoint_findlength(ser);
		}
		else if ( exploded->nlines )
		{
			line = lwline_deserialize(exploded->lines[0]);
			line->SRID = exploded->SRID;
			ser = lwline_serialize(line);
			pfree_line(line);
			size = lwline_findlength(ser);
		}
		else if ( exploded->npolys )
		{
			poly = lwpoly_deserialize(exploded->polys[0]);
			poly->SRID = exploded->SRID;
			ser = lwpoly_serialize(poly);
			pfree_polygon(poly);
			size = lwpoly_findlength(ser);
		}
		else return NULL;
		if ( wantbbox && ! lwgeom_hasBBOX(ser[0]) )
		{
			result = palloc(size+4);
			result[0] = TYPE_SETHASBBOX(ser[0], 1);
			loc = result+1;
			box3d = lw_geom_getBB_simple(ser);
			box2d = box3d_to_box2df(box3d);
			memcpy(loc, box2d, sizeof(BOX2DFLOAT4));
			loc += sizeof(BOX2DFLOAT4);
			memcpy(loc, (ser+1), size-1);
			pfree(ser);
			return result;
		}
		else
		{
			return ser;
		}
	}

	// Add size for 3 multigeoms + root geom + bbox and srid.
	// Also in this case we are considering worst case.
	size += 24+sizeof(BOX2DFLOAT4); 

#ifdef DEBUG
	elog(NOTICE, " computed totsize: %d", size);
#endif

	result = palloc(size*2);
	loc = result+1; // skip type

	if ( wantbbox ) loc += sizeof(BOX2DFLOAT4); // skip box
	if ( exploded->SRID != -1 ) loc += 4; // skip SRID

	// If we have more then one type of geom
	// write that number in the 'ngeoms' field of the
	// output serialized form (internal geoms would be multi themself)
	if ( ntypes > 1 )
	{
		memcpy(loc, &ntypes, 4);
		loc += 4; 
	}

	else
	{
		loc--; // let the type be specified later.
	}
	
	if ( exploded->npoints > 1 )
	{
		loc[0] = lwgeom_makeType_full(exploded->ndims, 0,
			MULTIPOINTTYPE, 0);
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
		ser = lwpoint_serialize(point);
		subsize = lwpoint_findlength(ser);
		memcpy(loc, ser, subsize);
		loc += subsize;
	}

	if ( exploded->nlines > 1 )
	{
		loc[0] = lwgeom_makeType_full(exploded->ndims, 0,
			MULTILINETYPE, 0);
		loc++;
		memcpy(loc, &exploded->nlines, 4); // numlines
		loc += 4; 
	}
	// Serialize lines stripping BBOX and SRID if any
	for (i=0; i<exploded->nlines; i++)
	{
		char *ser;
		int subsize;

		line = lwline_deserialize(exploded->lines[i]);
		if ( line == NULL )
		{
	elog(ERROR, "Error deserializing %dnt line from exploded geom", i);
	return NULL;
		}
		line->SRID = -1;
		ser = lwline_serialize(line);
		pfree_line(line);
		subsize = lwline_findlength(ser);
		memcpy(loc, ser, subsize);
		pfree(ser);
		loc += subsize;
	}

	if ( exploded->npolys > 1 )
	{
		loc[0] = lwgeom_makeType_full(exploded->ndims, 0,
			MULTIPOLYGONTYPE, 0);
		loc++;
		memcpy(loc, &exploded->npolys, 4); // numpolys
		loc += 4; 
	}
	// Serialize polys stripping BBOX and SRID if any
	for (i=0; i<exploded->npolys; i++)
	{
		char *ser;
		int subsize;

		poly = lwpoly_deserialize(exploded->polys[i]);
		if ( poly == NULL )
		{
	elog(ERROR, "Error deserializing %dnt polygon from exploded geom", i);
	return NULL;
		}
		poly->SRID = -1;
		ser = lwpoly_serialize(poly);
		pfree_polygon(poly);
		subsize = lwpoly_findlength(ser);
#ifdef DEBUG
		elog(NOTICE, "size of polygon %d: %d", i, subsize);
#endif
		memcpy(loc, ser, subsize);
		pfree(ser);
		loc += subsize;
	}

	// Ok. now we need to add type, SRID and bbox 
	result[0] = lwgeom_makeType_full(exploded->ndims,
		(exploded->SRID!=-1), outtype, wantbbox);
	loc = result+1;

	if ( wantbbox )
	{
		box3d = lw_geom_getBB_simple(result);
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
	elog(NOTICE, "lwexploded_serialize finished");
	elog(NOTICE, " type: %d", lwgeom_getType(result[0]));
	elog(NOTICE, " SRID: %d", lwgeom_getsrid(result));
	if ( lwgeom_hasBBOX(result[0]) )
	{
		{
			BOX2DFLOAT4 boxbuf;
			getbox2d_p(result, &boxbuf);
			elog(NOTICE, " BBOX: %f,%f %f,%f",
				boxbuf.xmin, boxbuf.ymin,
				boxbuf.xmax, boxbuf.ymax);
		}
	}
	elog(NOTICE, " numgeoms: %d", lwgeom_getnumgeometries(result));
#endif
		
	return result;
}

