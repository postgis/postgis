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



//parser - "BOX(xmin ymin,xmax ymax)"
PG_FUNCTION_INFO_V1(BOX2DFLOAT4_in);
Datum BOX2DFLOAT4_in(PG_FUNCTION_ARGS)
{
	    char                    *str = PG_GETARG_CSTRING(0);
		int nitems;
	    BOX2DFLOAT4      *box = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));



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


//convert a LWGEOM to BOX2D
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX2DFLOAT4);
Datum LWGEOM_to_BOX2DFLOAT4(PG_FUNCTION_ARGS)
{
	char *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 box, *result;

	box = getbox2d(lwgeom+4);
	result = palloc(sizeof(BOX2DFLOAT4));
	memcpy(result,&box, sizeof(BOX2DFLOAT4));
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




