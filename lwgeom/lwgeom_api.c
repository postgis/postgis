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

extern float  nextDown_f(double d);
extern float  nextUp_f(double d);
extern double nextDown_d(float d);
extern double nextUp_d(float d);


extern  BOX3D *lw_geom_getBB_simple(char *serialized_form);


// this will change to NaN when I figure out how to
// get NaN in a platform-independent way

#define NO_Z_VALUE 0

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
		elog(NOTICE, "box3d_to_box2df got NUL box");
		return result;
	}

	result->xmin = nextDown_f(box->xmin);
	result->ymin = nextDown_f(box->ymin);

	result->xmax = nextUp_f(box->xmax);
	result->ymax = nextUp_f(box->ymax);

	return result;
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
BOX3D box2df_to_box3d(BOX2DFLOAT4 *box)
{
	BOX3D result;

	if (box == NULL)
		return result;

	result.xmin = nextDown_d(box->xmin);
	result.ymin = nextDown_d(box->ymin);

	result.xmax = nextUp_d(box->xmax);
	result.ymax = nextUp_d(box->ymax);

	return result;
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
	BOX2DFLOAT4 *box;

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
	box = box3d_to_box2df(box3d);
//elog(NOTICE,"box3d made box2d(%.15g %.15g,%.15g %.15g)",box->xmin,box->ymin,box->xmax,box->ymax);
	memcpy(&result,box, sizeof(BOX2DFLOAT4));
	pfree(box3d);
	pfree(box);
	return result;
}


// same as getbox2d, but modifies box instead of returning result on the stack
void
getbox2d_p(char *serialized_form, BOX2DFLOAT4 *box)
{
	unsigned char type = (unsigned char) serialized_form[0];
	char *loc;
	BOX3D *box3d;
	BOX2DFLOAT4 *box2;

	loc = serialized_form+1;

	if (lwgeom_hasBBOX(type))
	{
		//woot - this is easy
//elog(NOTICE,"getbox2d has box");
		memcpy(box,loc, sizeof(BOX2DFLOAT4));
		return ;
	}

	//we have to actually compute it!
//elog(NOTICE,"getbox2d_p:: computing box");
	box3d = lw_geom_getBB_simple(serialized_form);
	box2 = box3d_to_box2df(box3d);

	memcpy(box,box2, sizeof(BOX2DFLOAT4));
	pfree(box3d);
	pfree(box2);
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
// will set point's z=0 (or NaN) if pa is 2d
// NOTE: this will modify the point3d pointed to by 'point'.
// we use a char* instead of a POINT3D because we want to use this function
//  for serialization/de-serialization
void getPoint3d_p(POINTARRAY *pa, int n, char *point)
{
	int size;

	if ( (n<0) || (n>=pa->npoints))
	{
		elog(NOTICE, "%d out of numpoint range (%d)", n, pa->npoints);
		return ; //error
	}

	size = pointArray_ptsize(pa);

	// this does x,y
	memcpy(point,
		&pa->serialized_pointlist[size*n],
		sizeof(double)*2 );

	if (pa->ndims >2)
		memcpy(point+16, &pa->serialized_pointlist[size*n + sizeof(double)*2],sizeof(double) );
	else
	{
		double bad=NO_Z_VALUE;
		memcpy(point+16, &bad,sizeof(double) );
	 	//point->z = NO_Z_VALUE;
 	}
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
	pa = (POINTARRAY*)palloc(sizeof(pa));

	if (ndims>4)
		elog(ERROR,"pointArray_construct:: called with dims = %i", (int) ndims);

	pa->ndims = ndims;
	pa->npoints = npoints;
	pa->serialized_pointlist = points;

	return pa;
}

//calculate the bounding box of a set of points
// returns a postgresql box
BOX3D *pointArray_bbox(POINTARRAY *pa)
{
	int t;
	BOX3D *result;
	POINT3D pt;

	result = (BOX3D*) palloc(sizeof(BOX3D));

	if (pa->npoints == 0)
		return result;

	getPoint3d_p(pa,0,(char*)&pt);
	result->xmin = pt.x;
	result->ymin = pt.y;
	result->zmin = pt.z;

	result->xmax = pt.x;
	result->ymax = pt.y;
	result->zmax = pt.z;

	for (t=1;t<pa->npoints;t++)
	{
		getPoint3d_p(pa,t,(char*)&pt);
		if (pt.x < result->xmin)
			result->xmin = pt.x;
		if (pt.y <
			result->ymin)
			result->ymin = pt.y;
		if (pt.x > result->xmax)
			result->xmax = pt.x;
		if (pt.y > result->ymax)
			result->ymax = pt.y;

		if (pt.z > result->zmax)
			result->zmax = pt.z;
		if (pt.z < result->zmax)
			result->zmax = pt.z;
	}
	return result;
}

//size of point represeneted in the POINTARRAY
// 16 for 2d, 24 for 3d, 32 for 4d
int pointArray_ptsize(POINTARRAY *pa)
{
	if (pa->ndims == 3)
		return 24;
	else if (pa->ndims == 2)
		return 16;
	else if (pa->ndims == 4)
		return 32;
	elog(ERROR,"pointArray_ptsize:: ndims isnt 2,3, or 4");
	return 0; // never get here
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


// construct a new LWLINE.  points will be copied
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

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwline_findbbox(LWLINE *line)
{
	if (line == NULL)
		return NULL;

	return pointArray_bbox(line->points);
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

		result = (LWPOINT*) palloc( sizeof(LWPOINT)) ;

		type = (unsigned char) serialized_form[0];


		if ( lwgeom_getType(type) != POINTTYPE)
			return NULL;



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

		// we've read the type (1 byte) and SRID (4 bytes, if present)

		pa = pointArray_construct( loc, lwgeom_ndims(type), 1);

		result->point = pa;
		result->ndims = lwgeom_ndims(type);

	return result;

}

// convert this line into its serialize form
// result's first char will be the 8bit type.  See serialized form doc
char  *lwpoint_serialize(LWPOINT *point)
{
		int size=1;
		char hasSRID;
		char * result;
		char *loc;

		hasSRID = (point->SRID != -1);

		if (hasSRID)
			size +=4;  //4 byte SRID

		if (point->ndims == 3)
		{
			size += 24; //x,y,z
		}
		else if (point->ndims == 2)
		{
			size += 16 ; //x,y,z
		}
		else if (point->ndims == 4)
		{
			size += 32 ; //x,y,z,m
		}

		result = palloc(size);

		result[0] = (unsigned char) lwgeom_makeType(point->ndims,hasSRID, POINTTYPE);
		loc = result+1;

		if (hasSRID)
		{
			memcpy(loc, &point->SRID, sizeof(int32));
			loc += 4;
		}

		//copy in points

		if (point->ndims == 3)
		{
				getPoint3d_p(point->point, 0, loc);
		}
		else if (point->ndims == 2)
		{

				getPoint2d_p(point->point, 0, loc);
		}
		else if (point->ndims == 4)
		{

			getPoint4d_p(point->point, 0, loc);
		}
	return result;
}

// find bounding box (standard one)  zmin=zmax=0 if 2d (might change to NaN)
BOX3D *lwpoint_findbbox(LWPOINT *point)
{
	if (point == NULL)
			return NULL;

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

	if ( lwgeom_getType(type) != POINTTYPE) return -9999;

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

// construct a new LWLINE.  arrays (points/points per ring) will NOT be copied
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
		return NULL;

	result = (LWPOLY*) palloc(sizeof(LWPOLY));


	type = (unsigned  char) serialized_form[0];
	ndims = lwgeom_ndims(type);
	loc = serialized_form;

	if ( lwgeom_getType(type) != POLYGONTYPE)
		return NULL;


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

	sub_geoms = (char**) palloc(sizeof(char*) * result->ngeometries );
	result->sub_geoms = sub_geoms;
	sub_geoms[0] = loc;
#ifdef DEBUG
	elog(NOTICE, "subgeom[0] @ %i", sub_geoms[0]);
#endif
	for (t=1;t<result->ngeometries; t++)
	{
		int sub_length = lwgeom_seralizedformlength(sub_geoms[t-1], -1);//-1 = entire object
		sub_geoms[t] = sub_geoms[t-1] + sub_length;
#ifdef DEBUG
		elog(NOTICE, "subgeom[%d] @ %i (sub_length: %d)",
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
	char type = lwgeom_getType((unsigned char) serialized_form[0]);
	int t;
	char *loc;
	uint32 ngeoms;
	BOX3D *result;
	BOX3D *b1,*b2;
	int sub_size;

	if (type == POINTTYPE)
	{
		LWPOINT *pt = lwpoint_deserialize(serialized_form);
		result = lwpoint_findbbox(pt);
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

		//handle all the multi* and geometrycollections the same
		    //NOTE: for a geometry collection of GC of GC of GC we will be recursing...
		for (t=0;t<inspected->ngeometries;t++)
		{
			b1 = lw_geom_getBB_simple( inspected->sub_geoms[t] );

//	elog(NOTICE,"%i has box :: BBOX3D(%g %g, %g %g)",t,b1->xmin, b1->ymin,b1->xmax, b1->ymax);

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
int lwgeom_getSRID(LWGEOM *lwgeom)
{
	unsigned char type = lwgeom->type;
	char *loc = lwgeom->data;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return get_int32(lwgeom->data);
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
