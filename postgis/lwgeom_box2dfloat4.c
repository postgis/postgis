#include "postgres.h"
#include "access/gist.h"
#include "access/itup.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


/* forward defs */
Datum BOX2DFLOAT4_in(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX2DFLOAT4(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_expand(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_combine(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_to_LWGEOM(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_construct(PG_FUNCTION_ARGS);

/* parser - "BOX(xmin ymin,xmax ymax)" */
PG_FUNCTION_INFO_V1(BOX2DFLOAT4_in);
Datum BOX2DFLOAT4_in(PG_FUNCTION_ARGS)
{
#ifdef GSERIALIZED_ON
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	double tmp;
	GBOX box;
	
	gbox_init(&box);

	if (strstr(str,"BOX(") !=  str )
	{
		elog(ERROR,"box2d parser - doesnt start with BOX(");
		PG_RETURN_NULL();
	}
	nitems = sscanf(str,"BOX(%f %f,%f %f)", &box.xmin, &box.ymin, &box.xmax, &box.ymax);
	if (nitems != 4)
	{
		elog(ERROR,"box2d parser - couldnt parse.  It should look like: BOX(xmin ymin,xmax ymax)");
		PG_RETURN_NULL();
	}

	if (box.xmin > box.xmax)
	{
		tmp = box.xmin;
		box.xmin = box.xmax;
		box.xmax = tmp;
	}
	if (box.ymin > box.ymax)
	{
		tmp = box.ymin;
		box.ymin = box.ymax;
		box.ymax = tmp;
	}
	PG_RETURN_POINTER(gbox_copy(&box));
#else
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

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
#endif
}

/*writer  "BOX(xmin ymin,xmax ymax)" */
PG_FUNCTION_INFO_V1(BOX2DFLOAT4_out);
Datum BOX2DFLOAT4_out(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	char tmp[500]; /* big enough */
	char *result;
	int size;

	size  = sprintf(tmp,"BOX(%.15g %.15g,%.15g %.15g)",
	                box->xmin, box->ymin, box->xmax, box->ymax);

	result= palloc(size+1); /* +1= null term */
	memcpy(result,tmp,size+1);
	result[size] = '\0';

	PG_RETURN_CSTRING(result);
}


/*convert a PG_LWGEOM to BOX2D */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX2DFLOAT4);
Datum LWGEOM_to_BOX2DFLOAT4(PG_FUNCTION_ARGS)
{
#ifdef GSERIALIZED_ON
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	GBOX gbox;

	/* Cannot box empty! */
	if ( lwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL(); 

	/* Cannot calculate box? */
	if ( lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE )
		PG_RETURN_NULL();
		
	/* Strip out higher dimensions */
	FLAGS_SET_Z(gbox.flags, 0);
	FLAGS_SET_M(gbox.flags, 0);

	PG_RETURN_POINTER(gbox_copy(&gbox));
#else
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 *result;

	/* Cannot box empty! */
	if ( pglwgeom_is_empty(lwgeom) )
		PG_RETURN_NULL(); 

	result = palloc(sizeof(BOX2DFLOAT4));
	if ( ! pglwgeom_getbox2d_p(lwgeom, result) )
	{
		PG_RETURN_NULL(); /* must be the empty geometry */
	}

	PG_RETURN_POINTER(result);
#endif
}

/*----------------------------------------------------------
 *	Relational operators for BOXes.
 *		<, >, <=, >=, and == are based on box area.
 *---------------------------------------------------------*/

/*
 * box_same - are two boxes identical?
 */
PG_FUNCTION_INFO_V1(BOX2D_same);
Datum BOX2D_same(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPeq(box1->xmax, box2->xmax) &&
	               FPeq(box1->xmin, box2->xmin) &&
	               FPeq(box1->ymax, box2->ymax) &&
	               FPeq(box1->ymin, box2->ymin));
}

/*
 * box_overlap - does box1 overlap box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_overlap);
Datum BOX2D_overlap(PG_FUNCTION_ARGS)
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


/*
 * box_overleft - is the right edge of box1 to the left of
 *                the right edge of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_overleft);
Datum BOX2D_overleft(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPle(box1->xmax, box2->xmax));
}

/*
 * box_left - is box1 strictly left of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_left);
Datum BOX2D_left(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPlt(box1->xmax, box2->xmin));
}

/*
 * box_right - is box1 strictly right of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_right);
Datum BOX2D_right(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPgt(box1->xmin, box2->xmax));
}

/*
 * box_overright - is the left edge of box1 to the right of
 *                 the left edge of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_overright);
Datum BOX2D_overright(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPge(box1->xmin, box2->xmin));
}

/*
 * box_overbelow - is the bottom edge of box1 below
 *                 the bottom edge of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_overbelow);
Datum BOX2D_overbelow(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPle(box1->ymax, box2->ymax));
}

/*
 * box_below - is box1 strictly below box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_below);
Datum BOX2D_below(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPlt(box1->ymax, box2->ymin));
}

/*
 * box_above - is box1 strictly above box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_above);
Datum BOX2D_above(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPgt(box1->ymin, box2->ymax));
}

/*
 * box_overabove - the top edge of box1 above
 *                 the top edge of box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_overabove);
Datum BOX2D_overabove(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPge(box1->ymin, box2->ymin));
}

/*
 * box_contained - is box1 contained by box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_contained);
Datum BOX2D_contained(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 =(BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPle(box1->xmax, box2->xmax) &&
	               FPge(box1->xmin, box2->xmin) &&
	               FPle(box1->ymax, box2->ymax) &&
	               FPge(box1->ymin, box2->ymin));
}

/*
 * box_contain - does box1 contain box2?
 */
PG_FUNCTION_INFO_V1(BOX2D_contain);
Datum BOX2D_contain(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *box1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *box2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(FPge(box1->xmax, box2->xmax) &&
	               FPle(box1->xmin, box2->xmin) &&
	               FPge(box1->ymax, box2->ymax) &&
	               FPle(box1->ymin, box2->ymin));

}

PG_FUNCTION_INFO_V1(BOX2D_intersects);
Datum BOX2D_intersects(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *a = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *b = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
	BOX2DFLOAT4 *n;


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


/*
 * union of two BOX2Ds
 */
PG_FUNCTION_INFO_V1(BOX2D_union);
Datum BOX2D_union(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *a = (BOX2DFLOAT4*) PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *b = (BOX2DFLOAT4*) PG_GETARG_POINTER(1);
	BOX2DFLOAT4 *n;

	n = (BOX2DFLOAT4 *) lwalloc(sizeof(BOX2DFLOAT4));
	if ( ! box2d_union_p(a,b,n) ) PG_RETURN_NULL();
	PG_RETURN_POINTER(n);
}


/*
 * min(a,b)
 */
float LWGEOM_Minf(float a, float b)
{
	if (a<b)
		return a;
	return b;
}

/*
 * max(a,b)
 */
float LWGEOM_Maxf(float a, float b)
{
	if (b>a) return b;
	return a;
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
#ifdef GSERIALIZED_ON
	BOX3D *result = box3d_from_gbox(box);
#else
	BOX3D *result = palloc(sizeof(BOX3D));
	box2df_to_box3d_p(box, result);
#endif

	PG_RETURN_POINTER(result);
}


#define KEEP_OBSOLETED_FUNX 1
#if KEEP_OBSOLETED_FUNX
Datum BOX2DFLOAT4_xmin(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_ymin(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_xmax(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_ymax(PG_FUNCTION_ARGS);
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
#endif

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_combine);
Datum BOX2DFLOAT4_combine(PG_FUNCTION_ARGS)
{
	Pointer box2d_ptr = PG_GETARG_POINTER(0);
	Pointer geom_ptr = PG_GETARG_POINTER(1);
	BOX2DFLOAT4 *a,*b;
	PG_LWGEOM *lwgeom;
	BOX2DFLOAT4 box, *result;

	if  ( (box2d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL(); /* combine_box2d(null,null) => null */
	}

	result = (BOX2DFLOAT4 *)palloc(sizeof(BOX2DFLOAT4));

	if (box2d_ptr == NULL)
	{
		lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		/* empty geom would make getbox2d_p return NULL */
		if ( ! pglwgeom_getbox2d_p(lwgeom, &box) ) PG_RETURN_NULL();
		memcpy(result, &box, sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	/* combine_bbox(BOX3D, null) => BOX3D */
	if (geom_ptr == NULL)
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	/*combine_bbox(BOX3D, geometry) => union(BOX3D, geometry->bvol) */

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	if ( ! pglwgeom_getbox2d_p(lwgeom, &box) )
	{
		/* must be the empty geom */
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
	POINTARRAY *pa = ptarray_construct_empty(0, 0, 5);
	POINT4D pt;
	PG_LWGEOM *result;


	/*
	 * Alter BOX2D cast so that a valid geometry is always
	 * returned depending upon the size of the BOX2D. The
	 * code makes the following assumptions:
	 *     - If the BOX2D is a single point then return a
	 *     POINT geometry
	 *     - If the BOX2D represents either a horizontal or
	 *     vertical line, return a LINESTRING geometry
	 *     - Otherwise return a POLYGON
	 */

	if ( (box->xmin == box->xmax) && (box->ymin == box->ymax) )
	{
		/* Construct and serialize point */
		LWPOINT *point = lwpoint_make2d(SRID_UNKNOWN, box->xmin, box->ymin);
		result = pglwgeom_serialize(lwpoint_as_lwgeom(point));
		lwpoint_free(point);
	}
	else if ( (box->xmin == box->xmax) || (box->ymin == box->ymax) )
	{
		LWLINE *line;

		/* Assign coordinates to point array */
		pt.x = box->xmin;
		pt.y = box->ymin;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		pt.x = box->xmax;
		pt.y = box->ymax;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);

		/* Construct and serialize linestring */
		line = lwline_construct(SRID_UNKNOWN, NULL, pa);
		result = pglwgeom_serialize(lwline_as_lwgeom(line));
		lwline_free(line);
	}
	else
	{
		LWPOLY *poly;
		POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY*));

		/* Assign coordinates to point array */
		pt.x = box->xmin;
		pt.y = box->ymin;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		pt.x = box->xmin;
		pt.y = box->ymax;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		pt.x = box->xmax;
		pt.y = box->ymax;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		pt.x = box->xmax;
		pt.y = box->ymin;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
		pt.x = box->xmin;
		pt.y = box->ymin;
		ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);

		/* Construct polygon */
		ppa[0] = pa;
		poly = lwpoly_construct(SRID_UNKNOWN, NULL, 1, ppa);
		result = pglwgeom_serialize(lwpoly_as_lwgeom(poly));
		lwpoly_free(poly);
	}

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX2DFLOAT4_construct);
Datum BOX2DFLOAT4_construct(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pgmin = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *pgmax = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 *result;
	LWPOINT *minpoint, *maxpoint;
	double min, max, tmp;

	minpoint = (LWPOINT*)pglwgeom_deserialize(pgmin);
	maxpoint = (LWPOINT*)pglwgeom_deserialize(pgmax);

	if ( (minpoint->type != POINTTYPE) || (maxpoint->type != POINTTYPE) )
	{
		elog(ERROR, "BOX2DFLOAT4_construct: arguments must be points");
		PG_RETURN_NULL();
	}

	error_if_srid_mismatch(minpoint->srid, maxpoint->srid);

#ifdef GSERIALIZED_ON
	result = gbox_new(gflags(0, 0, 0));
#else
	result = palloc(sizeof(BOX2DFLOAT4));
#endif
	/* Process X min/max */
	min = lwpoint_get_x(minpoint);
	max = lwpoint_get_x(maxpoint);
	if ( min > max ) 
	{
		tmp = min;
		min = max;
		max = tmp;
	}
	result->xmin = min;
	result->xmax = max;

	/* Process Y min/max */
	min = lwpoint_get_y(minpoint);
	max = lwpoint_get_y(maxpoint);
	if ( min > max ) 
	{
		tmp = min;
		min = max;
		max = tmp;
	}
	result->ymin = min;
	result->ymax = max;

	PG_RETURN_POINTER(result);
}

