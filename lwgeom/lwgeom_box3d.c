/**********************************************************************
 *
 * BOX3D IO and conversions
 *
 **********************************************************************/

#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "fmgr.h"
#include "utils/elog.h"

#include "lwgeom.h"


//#define DEBUG
// basic implementation of BOX2D

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

//forward defs
Datum BOX3D_in(PG_FUNCTION_ARGS);
Datum BOX3D_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX3D_expand(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX2DFLOAT4(PG_FUNCTION_ARGS);

/*
 *  BOX3D_in - takes a string rep of BOX3D and returns internal rep
 *
 *  example:
 *     "BOX3D(x1 y1 z1,x2 y2 z2)"
 * or  "BOX3D(x1 y1,x2 y2)"   z1 and z2 = 0.0
 *
 *
 */

PG_FUNCTION_INFO_V1(BOX3D_in);
Datum BOX3D_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	BOX3D *box = (BOX3D *) palloc(sizeof(BOX3D));
	box->zmin = 0;
	box->zmax = 0;


//printf( "box3d_in gets '%s'\n",str);

	if (strstr(str,"BOX3D(") !=  str )
	{
		 pfree(box);
		 elog(ERROR,"BOX3D parser - doesnt start with BOX3D(");
		 PG_RETURN_NULL();
	}

	nitems = sscanf(str,"BOX3D(%le %le %le,%le %le %le)",
		&box->xmin, &box->ymin, &box->zmin,
		&box->xmax, &box->ymax, &box->zmax);
	if (nitems != 6 )
	{
		nitems = sscanf(str,"BOX3D(%le %le,%le %le)",
			&box->xmin, &box->ymin, &box->xmax, &box->ymax);
		if (nitems != 4)
		{
			 pfree(box);
			 elog(ERROR,"BOX3D parser - couldnt parse.  It should look like: BOX3D(xmin ymin zmin,xmax ymax zmax) or BOX3D(xmin ymin,xmax ymax)");
			 PG_RETURN_NULL();
		}
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
	if (box->zmin > box->zmax)
	{
		float tmp = box->zmin;
		box->zmin = box->zmax;
		box->zmax = tmp;
	}
	PG_RETURN_POINTER(box);
}


/*
 *  Takes an internal rep of a BOX3D and returns a string rep.
 *
 *  example:
 *     "BOX3D(xmin ymin zmin, xmin ymin zmin)"
 */
PG_FUNCTION_INFO_V1(BOX3D_out);
Datum BOX3D_out(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	int size;
	char *result;

	if (bbox == NULL)
	{
		result = palloc(5);
		strcat(result,"NULL");
		PG_RETURN_CSTRING(result);
	}

	size = MAX_DIGS_DOUBLE*6+5+2+4+5+1;
	result = (char *) palloc(size); //double digits+ "BOX3D"+ "()" + commas +null
	sprintf(result, "BOX3D(%.15g %.15g %.15g,%.15g %.15g %.15g)",
			bbox->xmin, bbox->ymin, bbox->zmin,
			bbox->xmax,bbox->ymax,bbox->zmax);

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(BOX3D_to_BOX2DFLOAT4);
Datum BOX3D_to_BOX2DFLOAT4(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *out = box3d_to_box2df(in);
	PG_RETURN_POINTER(out);
}

/* Expand given box of 'd' units in all directions */
void
expand_box3d(BOX3D *box, double d)
{
	box->xmin -= d;
	box->ymin -= d;
	box->zmin -= d;

	box->xmax += d;
	box->ymax += d;
	box->zmax += d;
}

PG_FUNCTION_INFO_V1(BOX3D_expand);
Datum BOX3D_expand(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	double d = PG_GETARG_FLOAT8(1);
	BOX3D *result = (BOX3D *)palloc(sizeof(BOX3D));

	memcpy(result, box, sizeof(BOX3D));
	expand_box3d(result, d);

	PG_RETURN_POINTER(result);
}

//convert a LWGEOM to BOX3D
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX3D);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 box;
	BOX3D *result = palloc(sizeof(BOX3D));

	getbox2d_p(SERIALIZED_FORM(lwgeom), &box);
	box2df_to_box3d_p(&box, result);

	PG_RETURN_POINTER(result);
}
