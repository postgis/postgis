
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

Datum transform_geom(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);

// if USE_PROJECTION undefined, we get a do-nothing transform() function
#ifndef USE_PROJ

PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{

	elog(ERROR,"PostGIS transform() called, but support not compiled in.  Modify your makefile to add proj support, remake and re-install");
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

#else // defined USE_PROJ 

#include "projects.h"

PJ *make_project(char *str1);
void to_rad(POINT2D *pt);
void to_dec(POINT2D *pt);
int pj_transform_nodatum(PJ *srcdefn, PJ *dstdefn, long point_count, int point_offset, double *x, double *y, double *z );
int transform_point(POINT2D *pt, PJ *srcdefn, PJ *dstdefn);
int lwgeom_transform_recursive(char *geom, PJ *inpj, PJ *outpj);

//this is *exactly* the same as PROJ.4's pj_transform(), but it doesnt do the
// datum shift.
int
pj_transform_nodatum(PJ *srcdefn, PJ *dstdefn, long point_count,
int point_offset, double *x, double *y, double *z )
{
    long      i;
    //int       need_datum_shift;

    pj_errno = 0;

    if( point_offset == 0 )
        point_offset = 1;

    if( !srcdefn->is_latlong )
    {
        for( i = 0; i < point_count; i++ )
        {
            XY         projected_loc;
            LP	       geodetic_loc;

            projected_loc.u = x[point_offset*i];
            projected_loc.v = y[point_offset*i];

            geodetic_loc = pj_inv( projected_loc, srcdefn );
            if( pj_errno != 0 )
                return pj_errno;

            x[point_offset*i] = geodetic_loc.u;
            y[point_offset*i] = geodetic_loc.v;
        }
    }

    if( !dstdefn->is_latlong )
    {
        for( i = 0; i < point_count; i++ )
        {
            XY         projected_loc;
            LP	       geodetic_loc;

            geodetic_loc.u = x[point_offset*i];
            geodetic_loc.v = y[point_offset*i];

            projected_loc = pj_fwd( geodetic_loc, dstdefn );
            if( pj_errno != 0 )
                return pj_errno;

            x[point_offset*i] = projected_loc.u;
            y[point_offset*i] = projected_loc.v;
        }
    }

    return 0;
}



// convert decimal degress to radians
void to_rad(POINT2D *pt)
{
	pt->x *= PI/180.0;
	pt->y *= PI/180.0;
}

// convert radians to decimal degress
void to_dec(POINT2D *pt)
{
	pt->x *= 180.0/PI;
	pt->y *= 180.0/PI;
}

//given a string, make a PJ object
PJ *make_project(char *str1)
{
	int t;
	char	*params[1024];  //one for each parameter
	char  *loc;
	char 	*str;
	PJ *result;


	if (str1 == NULL)
		return NULL;

	if (strlen(str1) ==0)
		return NULL;

	str = palloc(1+strlen(str1) );
	strcpy(str,str1);

	//first we split the string into a bunch of smaller strings, based on the " " separator

	params[0] = str; //1st param, we'll null terminate at the " " soon

	loc = str;
	t =1;
	while  ((loc != NULL) && (*loc != 0) )
	{
		loc = strchr( loc,' ');
		if (loc != NULL)
		{
			*loc = 0; // null terminate
			params[t] = loc +1;
			loc++; // next char
			t++; //next param
		}
	}

	if (!(result= pj_init ( t , params)))
	{
		pfree(str);
		return NULL;
	}
	pfree(str);
	return result;
}

/*
 * Transform given SERIALIZED geometry
 * from inpj projection to outpj projection
 */
int
lwgeom_transform_recursive(char *geom, PJ *inpj, PJ *outpj)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(geom);
	int j, i;

	for (j=0; j<inspected->ngeometries; j++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		POINT2D p;
		char *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected,j);
		if (point != NULL)
		{
			getPoint2d_p(point->point, 0, &p);
			transform_point(&p, inpj, outpj);
			memcpy(getPoint_internal(point->point, 0),
				&p, sizeof(POINT2D));
			continue;
		}

		line = lwgeom_getline_inspected(inspected, j);
		if (line != NULL)
		{
			POINTARRAY *pts = line->points;
			for (i=0; i<pts->npoints; i++)
			{
				getPoint2d_p(pts, i, &p);
				transform_point(&p, inpj, outpj);
				memcpy(getPoint_internal(pts, i),
					&p, sizeof(POINT2D));
			}
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, j);
		if (poly !=NULL)
		{
			for (i=0; i<poly->nrings;i++)
			{
				int pi;
				POINTARRAY *pts = poly->rings[i];
				for (pi=0; pi<pts->npoints; pi++)
				{
					getPoint2d_p(pts, pi, &p);
					transform_point(&p, inpj, outpj);
					memcpy(getPoint_internal(pts, pi),
						&p, sizeof(POINT2D));
				}
			}
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, j);
		if ( subgeom != NULL )
		{
			if (!lwgeom_transform_recursive(subgeom, inpj, outpj))
			{
				pfree_inspected(inspected);
				return 0;
			}
			continue;
		}
		else
		{
	elog(NOTICE, "lwgeom_getsubgeometry_inspected returned NULL");
			return 0;
		}
	}

	pfree_inspected(inspected);
	return 1;
}


//tranform_geom( GEOMETRY, TEXT (input proj4), TEXT (output proj4), INT (output srid)
// tmpPts - if there is a nadgrid error (-38), we re-try the transform on a copy of points.  The transformed points
//          are in an indeterminate state after the -38 error is thrown.
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	PG_LWGEOM *result=NULL;
	LWGEOM *lwgeom;
	PJ *input_pj,*output_pj;
	char *input_proj4, *output_proj4;
	text *input_proj4_text;
	text *output_proj4_text;
	int32 result_srid ;
	char *srl;

	result_srid   = PG_GETARG_INT32(3);
	if (result_srid == -1)
	{
		elog(ERROR,"tranform: destination SRID = -1");
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	if (pglwgeom_getSRID(geom) == -1)
	{
		pfree(geom);
		elog(ERROR,"tranform: source SRID = -1");
		PG_RETURN_NULL();
	}

	input_proj4_text  = (PG_GETARG_TEXT_P(1));
	output_proj4_text = (PG_GETARG_TEXT_P(2));

	input_proj4 = (char *) palloc(input_proj4_text->vl_len +1-4);
	memcpy(input_proj4, input_proj4_text->vl_dat,
		input_proj4_text->vl_len-4);
	input_proj4[input_proj4_text->vl_len-4] = 0; //null terminate

	output_proj4 = (char *) palloc(output_proj4_text->vl_len +1-4);
	memcpy(output_proj4, output_proj4_text->vl_dat,
		output_proj4_text->vl_len-4);
	output_proj4[output_proj4_text->vl_len-4] = 0; //null terminate

	//make input and output projection objects
	input_pj = make_project(input_proj4);
	if ( (input_pj == NULL) || pj_errno)
	{
		//pfree(input_proj4);
		pfree(output_proj4);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 input string: '%s': %s", input_proj4, pj_strerrno(pj_errno));
		PG_RETURN_NULL();
	}

	output_pj = make_project(output_proj4);
	if ((output_pj == NULL)|| pj_errno)
	{
		pfree(input_proj4);
		//pfree(output_proj4);
		pj_free(input_pj);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 output string: '%s': %s", output_proj4, pj_strerrno(pj_errno));
		PG_RETURN_NULL();
	}

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom_transform_recursive(SERIALIZED_FORM(geom),
		input_pj, output_pj);

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);
	pfree(input_proj4); pfree(output_proj4);

	srl = SERIALIZED_FORM(geom);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( TYPE_HASBBOX(geom->type) )
	{
		lwgeom = lwgeom_deserialize(srl);
		lwgeom_dropBBOX(lwgeom);
		lwgeom->bbox = lwgeom_compute_bbox(lwgeom);
		lwgeom->SRID = result_srid;
		result = pglwgeom_serialize(lwgeom);
	}
	else
	{
		result = PG_LWGEOM_construct(srl, result_srid, 0);
	}

	PG_RETURN_POINTER(result); // new geometry
}

PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	const char *ver = pj_release;
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

int
transform_point(POINT2D *pt, PJ *srcpj, PJ *dstpj)
{
	if (srcpj->is_latlong) to_rad(pt);
	pj_transform(srcpj, dstpj, 1, 2, &(pt->x), &(pt->y), NULL);
	if (pj_errno)
	{
		if (pj_errno == -38)  //2nd chance
		{
			//couldnt do nadshift - do it without the datum
			pj_transform_nodatum(srcpj, dstpj, 1, 2,
				&(pt->x), &(pt->y), NULL);
		}

		if (pj_errno)
		{
			elog(ERROR,"transform: couldn't project point: %i (%s)",
				pj_errno,pj_strerrno(pj_errno));
			return 0;
		}
	}

	if (dstpj->is_latlong) to_dec(pt);
	return 1;
}


#endif

