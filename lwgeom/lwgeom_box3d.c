/**********************************************************************
 *
 * BOX3D IO and conversions
 *
 **********************************************************************/

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"


//#define DEBUG
// basic implementation of BOX2D

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

//forward defs
Datum BOX3D_in(PG_FUNCTION_ARGS);
Datum BOX3D_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS);
Datum BOX3D_expand(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX2DFLOAT4(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS);
Datum BOX3D_xmin(PG_FUNCTION_ARGS);
Datum BOX3D_ymin(PG_FUNCTION_ARGS);
Datum BOX3D_zmin(PG_FUNCTION_ARGS);
Datum BOX3D_xmax(PG_FUNCTION_ARGS);
Datum BOX3D_ymax(PG_FUNCTION_ARGS);
Datum BOX3D_zmax(PG_FUNCTION_ARGS);
Datum BOX3D_combine(PG_FUNCTION_ARGS);

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

PG_FUNCTION_INFO_V1(BOX3D_to_BOX);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *box2d = box3d_to_box2df(in);
	BOX *box = palloc(sizeof(BOX));

	box2df_to_box_p(box2d, box);
	PG_RETURN_POINTER(box);
}

PG_FUNCTION_INFO_V1(BOX3D_to_LWGEOM);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
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
	TYPE_SETZM(pa[0]->dims, 0, 0);
	pa[0]->npoints = 5;

	// Construct polygon
	poly = lwpoly_construct(-1, wantbbox, 1, pa);

	// Serialize polygon
	ser = lwpoly_serialize(poly);

	// Construct PG_LWGEOM 
	result = PG_LWGEOM_construct(ser, -1, wantbbox);
	
	PG_RETURN_POINTER(result);
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

//convert a PG_LWGEOM to BOX3D
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX3D);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D *result;

	result = lw_geom_getBB(SERIALIZED_FORM(lwgeom));

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_xmin);
Datum BOX3D_xmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->xmin);
}

PG_FUNCTION_INFO_V1(BOX3D_ymin);
Datum BOX3D_ymin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->ymin);
}

PG_FUNCTION_INFO_V1(BOX3D_zmin);
Datum BOX3D_zmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->zmin);
}

PG_FUNCTION_INFO_V1(BOX3D_xmax);
Datum BOX3D_xmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->xmax);
}

PG_FUNCTION_INFO_V1(BOX3D_ymax);
Datum BOX3D_ymax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->ymax);
}

PG_FUNCTION_INFO_V1(BOX3D_zmax);
Datum BOX3D_zmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(box->zmax);
}


PG_FUNCTION_INFO_V1(BOX3D_combine);
Datum BOX3D_combine(PG_FUNCTION_ARGS)
{
	Pointer box3d_ptr = PG_GETARG_POINTER(0);
	Pointer geom_ptr = PG_GETARG_POINTER(1);
	BOX3D *a,*b;
	PG_LWGEOM *lwgeom;
	BOX3D *box, *result;

	if  ( (box3d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL(); 
	}

	result = (BOX3D *)palloc(sizeof(BOX3D));

	if (box3d_ptr == NULL)
	{
		lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		box = lw_geom_getBB(SERIALIZED_FORM(lwgeom));
		if ( ! box ) PG_RETURN_NULL(); // must be the empty geom
		memcpy(result, box, sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	// combine_bbox(BOX3D, null) => BOX3D
	if (geom_ptr == NULL)
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	box = lw_geom_getBB(SERIALIZED_FORM(lwgeom));
	if ( ! box ) // must be the empty geom
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	a = (BOX3D *)PG_GETARG_DATUM(0);
	b = box;

	result->xmax = LWGEOM_Maxd(a->xmax, b->xmax);
	result->ymax = LWGEOM_Maxd(a->ymax, b->ymax);
	result->zmax = LWGEOM_Maxd(a->zmax, b->zmax);
	result->xmin = LWGEOM_Mind(a->xmin, b->xmin);
	result->ymin = LWGEOM_Mind(a->ymin, b->ymin);
	result->zmin = LWGEOM_Mind(a->zmin, b->zmin);

	PG_RETURN_POINTER(result);
}

//min(a,b)
double LWGEOM_Mind(double a, double b)
{
	if (a<b)
		return a;
	return b;
}

//max(a,b)
double LWGEOM_Maxd(double a, double b)
{
	if (b>a)
		return b;
	return a;
}
