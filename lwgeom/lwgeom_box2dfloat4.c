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
#include "stringBuffer.h"


//#define DEBUG

// basic implementation of BOX2D


//forward defs
Datum BOX2DFLOAT4_in(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX2DFLOAT4(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_expand(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_xmin(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_ymin(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_xmax(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_ymax(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_combine(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_to_LWGEOM(PG_FUNCTION_ARGS);



//parser - "BOX(xmin ymin,xmax ymax)"
PG_FUNCTION_INFO_V1(BOX2DFLOAT4_in);
Datum BOX2DFLOAT4_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));



//printf( "box3d_in gets '%s'\n",str);

	if (strstr(str,"BOX(") !=  str )
	{
		 pfree(box);
		 elog(ERROR,"BOX2DFLOAT4 parser - doesnt start with BOX(");
		 PG_RETURN_NULL();
	}
	nitems = sscanf(str,"BOX(%f %f,%f %f)", &box->xmin,&box->ymin,&box->xmax,&box->ymax);
	if (nitems != 4)
	{
		 pfree(box);
		 elog(ERROR,"BOX2DFLOAT4 parser - couldnt parse.  It should look like: BOX(xmin ymin,xmax ymax)");
		 PG_RETURN_NULL();
	}

	if (box->xmin > box->xmax)
	{
		float tmp = box->xmin;
		box->xmin = box->xmax;
		box->xmax = tmp;
	}
	if (box->ymin > box->ymax)
	{
		float tmp = box->ymin;
		box->ymin = box->ymax;
		box->ymax = tmp;
	}
	PG_RETURN_POINTER(box);
}

//writer  "BOX(xmin ymin,xmax ymax)"
PG_FUNCTION_INFO_V1(BOX2DFLOAT4_out);
Datum BOX2DFLOAT4_out(PG_FUNCTION_ARGS)
{
		BOX2DFLOAT4		        *box = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
		char		 tmp[500]; // big enough
		char		 *result;
		int			size;

		size  = sprintf(tmp,"BOX(%.15g %.15g,%.15g %.15g)", box->xmin, box->ymin, box->xmax, box->ymax);

		result= palloc(size+1); // +1= null term

		memcpy(result,tmp,size+1);
		PG_RETURN_CSTRING(result);
}


//convert a PG_LWGEOM to BOX2D
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX2DFLOAT4);
Datum LWGEOM_to_BOX2DFLOAT4(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 *result;

	result = palloc(sizeof(BOX2DFLOAT4));
	if ( ! getbox2d_p(SERIALIZED_FORM(lwgeom), result) )
	{
		PG_RETURN_NULL(); // must be the empty geometry
	}
	PG_RETURN_POINTER(result);
}

/*----------------------------------------------------------
 *	Relational operators for BOXes.
 *		<, >, <=, >=, and == are based on box area.
 *---------------------------------------------------------*/

/*		box_same		-		are two boxes identical?
 */
PG_FUNCTION_INFO_V1(box2d_same);
Datum box2d_same(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPeq(box1->xmax, box2->xmax) &&
				   FPeq(box1->xmin, box2->xmin) &&
				   FPeq(box1->ymax, box2->ymax) &&
				   FPeq(box1->ymin, box2->ymin));
}

/*		box_overlap		-		does box1 overlap box2?
 */
PG_FUNCTION_INFO_V1(box2d_overlap);
Datum box2d_overlap(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
	bool       result;


	result = ((FPge(box1->xmax, box2->xmax) &&
			 FPle(box1->xmin, box2->xmax)) ||
			(FPge(box2->xmax, box1->xmax) &&
			 FPle(box2->xmin, box1->xmax)))
		&&
		((FPge(box1->ymax, box2->ymax) &&
		  FPle(box1->ymin, box2->ymax)) ||
		 (FPge(box2->ymax, box1->ymax) &&
		  FPle(box2->ymin, box1->ymax)));

	PG_RETURN_BOOL(result);
}


/*		box_overleft	-		is the right edge of box1 to the left of
 *								the right edge of box2?
 *
 *		This is "less than or equal" for the end of a time range,
 *		when time ranges are stored as rectangles.
 */
 PG_FUNCTION_INFO_V1(box2d_overleft);
Datum box2d_overleft(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPle(box1->xmax, box2->xmax));
}

/*		box_left		-		is box1 strictly left of box2?
 */
 PG_FUNCTION_INFO_V1(box2d_left);
Datum box2d_left(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPlt(box1->xmax, box2->xmin));
}

/*		box_right		-		is box1 strictly right of box2?
 */
 PG_FUNCTION_INFO_V1(box2d_right);
Datum box2d_right(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPgt(box1->xmin, box2->xmax));
}

/*		box_overright	-		is the left edge of box1 to the right of
 *								the left edge of box2?
 *
 *		This is "greater than or equal" for time ranges, when time ranges
 *		are stored as rectangles.
 */
 PG_FUNCTION_INFO_V1(box2d_overright);
Datum box2d_overright(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPge(box1->xmin, box2->xmin));
}

/*		box_contained	-		is box1 contained by box2?
 */
 PG_FUNCTION_INFO_V1(box2d_contained);
Datum box2d_contained(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 =(BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPle(box1->xmax, box2->xmax) &&
				   FPge(box1->xmin, box2->xmin) &&
				   FPle(box1->ymax, box2->ymax) &&
				   FPge(box1->ymin, box2->ymin));
}

/*		box_contain		-		does box1 contain box2?
 */
 PG_FUNCTION_INFO_V1(box2d_contain);
Datum box2d_contain(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPge(box1->xmax, box2->xmax) &&
				   FPle(box1->xmin, box2->xmin) &&
				   FPge(box1->ymax, box2->ymax) &&
				   FPle(box1->ymin, box2->ymin));

}

PG_FUNCTION_INFO_V1(box2d_inter);
Datum box2d_inter(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *a = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *b = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
    BOX2DFLOAT4        *n;


    n = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

    n->xmax = LWGEOM_Minf(a->xmax, b->xmax);
    n->ymax = LWGEOM_Minf(a->ymax, b->ymax);
    n->xmin = LWGEOM_Maxf(a->xmin, b->xmin);
    n->ymin = LWGEOM_Maxf(a->ymin, b->ymin);


    if (n->xmax < n->xmin || n->ymax < n->ymin)
    {
        pfree(n);
        /* Indicate "no intersection" by returning NULL pointer */
        n = NULL;
    }

    PG_RETURN_POINTER(n);
}


//union of two BOX2Ds
PG_FUNCTION_INFO_V1(box2d_union);
Datum box2d_union(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4        *a = (BOX2DFLOAT4*) PG_GETARG_POINTER(0);
	BOX2DFLOAT4        *b = (BOX2DFLOAT4*) PG_GETARG_POINTER(1);
	BOX2DFLOAT4        *n;

	n = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

	n->xmax = LWGEOM_Maxf(a->xmax, b->xmax);
	n->ymax = LWGEOM_Maxf(a->ymax, b->ymax);
	n->xmin = LWGEOM_Minf(a->xmin, b->xmin);
	n->ymin = LWGEOM_Minf(a->ymin, b->ymin);


	PG_RETURN_POINTER(n);
}


//min(a,b)
float LWGEOM_Minf(float a, float b)
{
	if (a<b)
		return a;
	return b;
}

//max(a,b)
float LWGEOM_Maxf(float a, float b)
{
	if (b>a)
		return b;
	return a;
}


/* Expand given box of 'd' units in all directions */
void
expand_box2d(BOX2DFLOAT4 *box, double d)
{
	box->xmin -= d;
	box->ymin -= d;

	box->xmax += d;
	box->ymax += d;
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_expand);
Datum BOX2DFLOAT4_expand(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	double d = PG_GETARG_FLOAT8(1);
	BOX2DFLOAT4 *result = (BOX2DFLOAT4 *)palloc(sizeof(BOX2DFLOAT4));

	memcpy(result, box, sizeof(BOX2DFLOAT4));
	expand_box2d(result, d);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_to_BOX3D);
Datum BOX2DFLOAT4_to_BOX3D(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	BOX3D *result = palloc(sizeof(BOX3D));

	box2df_to_box3d_p(box, result);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_xmin);
Datum BOX2DFLOAT4_xmin(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT4(box->xmin);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_ymin);
Datum BOX2DFLOAT4_ymin(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT4(box->ymin);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_xmax);
Datum BOX2DFLOAT4_xmax(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT4(box->xmax);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_ymax);
Datum BOX2DFLOAT4_ymax(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT4(box->ymax);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_combine);
Datum BOX2DFLOAT4_combine(PG_FUNCTION_ARGS)
{
	Pointer box2d_ptr = PG_GETARG_POINTER(0);
	Pointer geom_ptr = PG_GETARG_POINTER(1);
	BOX2DFLOAT4 *a,*b;
	char *lwgeom;
	BOX2DFLOAT4 box, *result;

	if  ( (box2d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL(); // combine_box2d(null,null) => null
	}

	result = (BOX2DFLOAT4 *)palloc(sizeof(BOX2DFLOAT4));

	if (box2d_ptr == NULL)
	{
		lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		// empty geom would make getbox2d_p return NULL
		if ( ! getbox2d_p(lwgeom+4, &box) ) PG_RETURN_NULL();
		memcpy(result, &box, sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	// combine_bbox(BOX3D, null) => BOX3D
	if (geom_ptr == NULL)
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	//combine_bbox(BOX3D, geometry) => union(BOX3D, geometry->bvol)

	lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	if ( ! getbox2d_p(lwgeom+4, &box) )
	{
		// must be the empty geom
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	a = (BOX2DFLOAT4 *)PG_GETARG_DATUM(0);
	b = &box;

	result->xmax = LWGEOM_Maxf(a->xmax, b->xmax);
	result->ymax = LWGEOM_Maxf(a->ymax, b->ymax);
	result->xmin = LWGEOM_Minf(a->xmin, b->xmin);
	result->ymin = LWGEOM_Minf(a->ymin, b->ymin);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_to_LWGEOM);
Datum BOX2DFLOAT4_to_LWGEOM(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *)PG_GETARG_POINTER(0);
	POINT2D *pts = palloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	int wantbbox = 0;
	PG_LWGEOM *result;
	char *ser;

	// Assign coordinates to POINT2D array
	pts[0].x = box->xmin; pts[0].y = box->ymin;
	pts[1].x = box->xmin; pts[1].y = box->ymax;
	pts[2].x = box->xmax; pts[2].y = box->ymax;
	pts[3].x = box->xmax; pts[3].y = box->ymin;
	pts[4].x = box->xmin; pts[4].y = box->ymin;

	// Construct point array
	pa[0] = palloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (char *)pts;
	pa[0]->ndims = 2;
	pa[0]->npoints = 5;

	// Construct polygon
	poly = lwpoly_construct(2, -1, 1, pa);

	// Serialize polygon
	ser = lwpoly_serialize(poly);

	// Construct PG_LWGEOM 
	result = PG_LWGEOM_construct(ser, -1, wantbbox);
	
	PG_RETURN_POINTER(result);
}

