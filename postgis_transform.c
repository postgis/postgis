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


#include "postgis.h"
#include "utils/elog.h"



#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// if USE_PROJECTION undefined, we get a do-nothing transform() function
#ifdef USE_PROJ

#include "projects.h"


PJ *make_project(char *str1);
void to_rad(POINT3D *pts, int num_points);
void to_dec(POINT3D *pts, int num_points);

int pj_transform_nodatum( PJ *srcdefn, PJ *dstdefn, long point_count, int point_offset,
                  double *x, double *y, double *z );

//this is *exactly* the same as PROJ.4's pj_transform(), but it doesnt do the
// datum shift.
int pj_transform_nodatum( PJ *srcdefn, PJ *dstdefn, long point_count, int point_offset,
                  double *x, double *y, double *z )

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
void to_rad(POINT3D *pts, int num_points)
{
	int t;
	for(t=0;t<num_points;t++)
	{
		pts[t].x *= PI/180.0;
		pts[t].y *= PI/180.0;
	}
}

// convert radians to decimal degress
void to_dec(POINT3D *pts, int num_points)
{
	int t;
	for(t=0;t<num_points;t++)
	{
		pts[t].x *= 180.0/PI;
		pts[t].y *= 180.0/PI;
	}

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


//tranform_geom( GEOMETRY, TEXT (input proj4), TEXT (input proj4), INT (output srid)
// tmpPts - if there is a nadgrid error (-38), we re-try the transform on a copy of points.  The transformed points
//          are in an indeterminate state after the -38 error is thrown.
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *result;
	PJ			   *input_pj,*output_pj;

	char				*o1;
	int32				*offsets1;
	int				j,type1,gtype,i,poly_points;

	POLYGON3D			*poly;
	LINE3D			*line;
	POINT3D			*pt,*poly_pts;

	char				*input_proj4, *output_proj4;

	BOX3D				*bbox;
	POINT3D			*tmpPts;



	text	   *input_proj4_text  = (PG_GETARG_TEXT_P(1));
	text	   *output_proj4_text = (PG_GETARG_TEXT_P(2));
	int32	   result_srid   = PG_GETARG_INT32(3);

	input_proj4 = (char *) palloc(input_proj4_text->vl_len +1-4);
      memcpy(input_proj4,input_proj4_text->vl_dat, input_proj4_text->vl_len-4);
	input_proj4[input_proj4_text->vl_len-4] = 0; //null terminate

	output_proj4 = (char *) palloc(output_proj4_text->vl_len +1-4);
      memcpy(output_proj4,output_proj4_text->vl_dat, output_proj4_text->vl_len-4);
	output_proj4[output_proj4_text->vl_len-4] = 0; //null terminate

	if (geom->SRID == -1)
	{
		pfree(input_proj4); pfree(output_proj4);
		elog(ERROR,"tranform: source SRID = -1");
		PG_RETURN_NULL();			// no srid, cannot convert
	}

	if (result_srid   == -1)
	{
		pfree(input_proj4); pfree(output_proj4);
		elog(ERROR,"tranform: destination SRID = -1");
		PG_RETURN_NULL();			// no srid, cannot convert
	}

	//make input and output projection objects
	input_pj = make_project(input_proj4);
	if ( (input_pj == NULL) || pj_errno)
	{
		pfree(input_proj4); pfree(output_proj4);
		elog(ERROR,"tranform: couldnt parse proj4 input string");
		PG_RETURN_NULL();
	}

	output_pj = make_project(output_proj4);
	if ((output_pj == NULL)|| pj_errno)
	{
		pfree(input_proj4); pfree(output_proj4);
		pj_free(input_pj);
		elog(ERROR,"tranform: couldnt parse proj4 output string");

		PG_RETURN_NULL();
	}
	//great, now we have a geometry, and input/output PJ* structs.  Excellent.

	//copy the geometry structure - we're only going to change the points, not the structures
	result = (GEOMETRY *) palloc (geom->size);
	memcpy(result,geom, geom->size);

	gtype = result->type;

	// allow transformations of the BOX3D type - the loop below won't be entered since for a BOX3D
	// type result->nobjs will be -1 so we check for it here
	if (gtype == BBOXONLYTYPE)
	{
		bbox = &(result->bvol);
		pt = (POINT3D *)bbox;

		if (input_pj->is_latlong)
			to_rad(pt,2);

		tmpPts = palloc(sizeof(POINT3D)*2);
		memcpy(tmpPts, pt, sizeof(POINT3D)*2);

		pj_transform(input_pj,output_pj, 2,3, &pt->x,&pt->y, &pt->z);

		if (pj_errno)
		{
			if (pj_errno == -38)  //2nd chance
			{
				//couldnt do nadshift - do it without the datum
				memcpy(pt,tmpPts, sizeof(POINT3D)*2);
				pj_transform_nodatum(input_pj,output_pj, 2 ,3, &pt->x,&pt->y, &pt->z);
			}

			if (pj_errno)
			{
				pfree(input_proj4); pfree(output_proj4);
				pj_free(input_pj); pj_free(output_pj);
				elog(ERROR,"transform: couldnt project bbox point: %i (%s)",pj_errno,pj_strerrno(pj_errno));
				PG_RETURN_NULL();
			}

		}
		pfree(tmpPts);
		if (output_pj->is_latlong)
			to_dec(pt,2);

	}else{

		//handle each sub-geometry
		offsets1 = (int32 *) ( ((char *) &(result->objType[0] ))+ sizeof(int32) * result->nobjs ) ;
		for (j=0; j< result->nobjs; j++)		//for each object in geom1
		{
			o1 = (char *) result +offsets1[j] ;
			type1=  result->objType[j];

			if (type1 == POINTTYPE)	//point
			{
				pt = (POINT3D *) o1;
				if (input_pj->is_latlong)
					to_rad(pt,1);

				tmpPts = palloc(sizeof(POINT3D) );
				memcpy(tmpPts,pt, sizeof(POINT3D));

				pj_transform(input_pj,output_pj, 1,3, &pt->x,&pt->y, &pt->z);
				if (pj_errno)
				{
					if (pj_errno == -38)  //2nd chance
					{
						//couldnt do nadshift - do it without the datum
						memcpy(pt,tmpPts, sizeof(POINT3D));
						pj_transform_nodatum(input_pj,output_pj, 1,3, &pt->x,&pt->y, &pt->z);
					}

					if (pj_errno)
					{
						pfree(input_proj4); pfree(output_proj4);
						pj_free(input_pj); pj_free(output_pj);
						elog(ERROR,"transform: couldnt project point: %i (%s)",pj_errno,pj_strerrno(pj_errno));
						PG_RETURN_NULL();
					}

				}
				pfree(tmpPts);
				if (output_pj->is_latlong)
					to_dec(pt,1);
			}
			if (type1 == LINETYPE)	//line
			{
				line = (LINE3D *) o1;
				if (input_pj->is_latlong)
					to_rad(&line->points[0],line->npoints);

				tmpPts = palloc(sizeof(POINT3D)*line->npoints );
				memcpy(tmpPts,&line->points[0], sizeof(POINT3D)*line->npoints);

				pj_transform(input_pj,output_pj, line->npoints ,3,
							&line->points[0].x,&line->points[0].y, &line->points[0].z);
				if (pj_errno)
				{
					if (pj_errno == -38)  //2nd chance
					{
						//couldnt do nadshift - do it without the datum
						memcpy(&line->points[0],tmpPts, sizeof(POINT3D)*line->npoints);
						pj_transform_nodatum(input_pj,output_pj, line->npoints ,3,
							&line->points[0].x,&line->points[0].y, &line->points[0].z);
					}

					if (pj_errno)
					{

						pfree(input_proj4); pfree(output_proj4);
						pj_free(input_pj); pj_free(output_pj);
						elog(ERROR,"transform: couldnt project line");
						PG_RETURN_NULL();
					}
				}
				pfree(tmpPts);
				if (output_pj->is_latlong)
					to_dec(&line->points[0],line->npoints);
			}
			if (type1 == POLYGONTYPE)	//POLYGON
			{
				poly = (POLYGON3D *) o1;
				poly_points = 0;

				for (i=0; i<poly->nrings;i++)
				{
					poly_points  += poly->npoints[i];
				}
				poly_pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
				if (input_pj->is_latlong)
					to_rad(poly_pts , poly_points);

				tmpPts = palloc(sizeof(POINT3D)* poly_points );
				memcpy(tmpPts,&poly_pts[0].x, sizeof(POINT3D)* poly_points);



				pj_transform(input_pj,output_pj, poly_points,3,
							&poly_pts[0].x,&poly_pts[0].y, &poly_pts[0].z);
				if (pj_errno)
				{
					if (pj_errno == -38)  //2nd chance
					{
						//couldnt do nadshift - do it without the datum
						memcpy(&poly_pts[0].x,tmpPts, sizeof(POINT3D)*poly_points);
						pj_transform_nodatum(input_pj,output_pj, poly_points,3,
							&poly_pts[0].x,&poly_pts[0].y, &poly_pts[0].z);
					}

					if (pj_errno)
					{

						pfree(input_proj4); pfree(output_proj4);
						pj_free(input_pj); pj_free(output_pj);
						elog(ERROR,"transform: couldnt project polygon");
						PG_RETURN_NULL();
					}
				}
				pfree(tmpPts);
				if (output_pj->is_latlong)
					to_dec(poly_pts , poly_points);
			}
		}

	}

	// clean up
	pj_free(input_pj);
	pj_free(output_pj);
	pfree(input_proj4); pfree(output_proj4);

	// Generate the bounding box if necessary
	if (gtype != BBOXONLYTYPE)
	{
		bbox = bbox_of_geometry(result);
		memcpy(&result->bvol,bbox, sizeof(BOX3D) ); //make bounding box
	}

	result->SRID = result_srid;

	PG_RETURN_POINTER(result); // new geometry
}


#else

	// return the original geometry
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{

	elog(ERROR,"PostGIS transform() called, but support not compiled in.  Modify your makefile to add proj support, remake and re-install");

	//GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	//GEOMETRY		   *result;

	//result = (GEOMETRY *) palloc (geom1->size);
	//memcpy(result,geom1, geom1->size);
	///elog(NOTICE,"PostGIS transform
	//PG_RETURN_POINTER(result);
}
#endif
