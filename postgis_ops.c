/*************************************************************
 postGIS - geometric types for postgres

 This software is copyrighted (2001).
 
 This is free software; you can redistribute it and/or modify
 it under the GNU General Public Licence.  See the file "COPYING".

 More Info?  See the documentation, join the mailing list 
 (postgis@yahoogroups.com), or see the web page
 (http://postgis.refractions.net).

 *************************************************************/



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


//Norman Vine found this problem for compiling under cygwin
//  it defines BYTE_ORDER and LITTLE_ENDIAN 

#ifdef __CYGWIN__
#include <sys/param.h>       // FOR ENDIAN DEFINES
#endif


#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// #define DEBUG_GIST
// #define DEBUG_GIST2



PG_FUNCTION_INFO_V1(ggeometry_compress);
PG_FUNCTION_INFO_V1(ggeometry_union);
PG_FUNCTION_INFO_V1(ggeometry_picksplit);
PG_FUNCTION_INFO_V1(ggeometry_consistent);
PG_FUNCTION_INFO_V1(ggeometry_penalty);
PG_FUNCTION_INFO_V1(ggeometry_same);
PG_FUNCTION_INFO_V1(ggeometry_inter);
PG_FUNCTION_INFO_V1(rtree_decompress);


//restriction in the GiST && operator

Datum postgis_gist_sel(PG_FUNCTION_ARGS)
{
        PG_RETURN_FLOAT8(0.000005);
}


//given a bvol, make a "fake" geometry that contains it (for indexing)
GEOMETRY *make_bvol_geometry(BOX3D *box)
{
	return (GEOMETRY *) DatumGetPointer(DirectFunctionCall1(get_geometry_of_bbox,PointerGetDatum(box) ) );
}

//   ***********************************
//          GEOMETRY indexing (operator) fns
//	NOTE:
//		These work on the bbox of the geometry, not the 
//		individual components (except same)
//   ***********************************


PG_FUNCTION_INFO_V1(box3d_same);
Datum box3d_same(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *)  PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool		result;

	result = (	   FPeq(box1->LLB.x	, box2->LLB.x) &&
				   FPeq(box1->LLB.y	, box2->LLB.y) &&
				   FPeq(box1->URT.x	, box2->URT.x) &&
				   FPeq(box1->URT.y	, box2->URT.y)
				);
	PG_RETURN_BOOL(result);
}




PG_FUNCTION_INFO_V1(geometry_overleft);
Datum geometry_overleft(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

//printf("in geometry_overleft\n");
//print_box(&geom1->bvol);
//print_box(&geom2->bvol);

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	result = FPle(geom1->bvol.URT.x, geom2->bvol.URT.x);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_left);
Datum geometry_left(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

//printf("in geometry_left\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPlt(geom1->bvol.URT.x, geom2->bvol.LLB.x);


	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(geometry_right);
Datum geometry_right(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_right\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPgt(geom1->bvol.LLB.x, geom2->bvol.URT.x);
	
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_overright);
Datum geometry_overright(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_overright\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPge(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
	


	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_contained);
Datum geometry_contained(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


//printf("in geometry_contained\n");


	result = FPle(geom1->bvol.URT.x, geom2->bvol.URT.x) &&
				   FPge(geom1->bvol.LLB.x, geom2->bvol.LLB.x) &&
				   FPle(geom1->bvol.URT.y, geom2->bvol.URT.y) &&
				   FPge(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
	
//printf("returning geometry_contained\n");
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_contain);
Datum geometry_contain(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_contain\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPge(geom1->bvol.URT.x, geom2->bvol.URT.x) &&
				   FPle(geom1->bvol.LLB.x, geom2->bvol.LLB.x) &&
				   FPge(geom1->bvol.URT.y, geom2->bvol.URT.y) &&
				   FPle(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
	


	PG_RETURN_BOOL(result);
}

bool box3d_ov(BOX3D *box1, BOX3D *box2)
{
	bool	result;

	result = 

	/*overlap in x*/
		 ((    FPge(box1->URT.x, box2->URT.x) &&
			 FPle(box1->LLB.x, box2->URT.x)) ||
			(FPge(box2->URT.x, box1->URT.x) &&
			 FPle(box2->LLB.x, box1->URT.x)))
	&&
	/*overlap in y*/
	((FPge(box1->URT.y, box2->URT.y) &&
	  FPle(box1->LLB.y, box2->URT.y)) ||
	 (FPge(box2->URT.y, box1->URT.y) &&
	  FPle(box2->LLB.y, box1->URT.y)));

//printf("box3d_ov about to return %i\n",(int) result);	
	return result;
}


PG_FUNCTION_INFO_V1(geometry_overlap);
Datum geometry_overlap(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

//printf("in geometry_overlap\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	PG_RETURN_BOOL(box3d_ov(&(geom1->bvol), &(geom2->bvol)));
}


bool is_same_point(POINT3D	*p1, POINT3D	*p2)
{
	return ( FPeq(p1->x, p2->x ) && FPeq(p1->y, p2->y ) &&FPeq(p1->z, p2->z ) );
}

bool is_same_line(LINE3D	*l1, LINE3D	*l2)
{
	int i;
		//lines are directed, so all the points should be the same

	if (l1->npoints != l2->npoints)
		return FALSE; //simple case, not the same # of points

	for (i=0;i<l2->npoints; i++)
	{
		if (!( is_same_point( &l1->points[i], &l2->points[i] ) ) )
		{
			return FALSE;
		}
	}
	return TRUE;
}



PG_FUNCTION_INFO_V1(geometry_lt);
Datum geometry_lt(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	if  (geom1->bvol.LLB.x < geom2->bvol.LLB.x)  
		PG_RETURN_BOOL(TRUE);

	if ( FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x)   )
	{ 
		if ( geom1->bvol.LLB.y < geom2->bvol.LLB.y )  
			PG_RETURN_BOOL(TRUE);

		if ( FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y)   )
		{ 
			if ( geom1->bvol.LLB.z < geom2->bvol.LLB.z )  
				PG_RETURN_BOOL(TRUE);
			else
				PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}

	}
	else
	{
		PG_RETURN_BOOL(FALSE);
	}
}

PG_FUNCTION_INFO_V1(geometry_gt);
Datum geometry_gt(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	if  (geom1->bvol.LLB.x > geom2->bvol.LLB.x)  
		PG_RETURN_BOOL(TRUE);

	if ( FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x)   )
	{ 
		if ( geom1->bvol.LLB.y > geom2->bvol.LLB.y )  
			PG_RETURN_BOOL(TRUE);

		if ( FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y)   )
		{ 
			if ( geom1->bvol.LLB.z > geom2->bvol.LLB.z )  
				PG_RETURN_BOOL(TRUE);
			else
				PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}

	}
	else
	{
		PG_RETURN_BOOL(FALSE);
	}
}

PG_FUNCTION_INFO_V1(geometry_eq);
Datum geometry_eq(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	if  ( FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x)  && FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y)  && FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z)  )
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);
}







//order of points in a ring IS important
//order of rings is also important
bool is_same_polygon(POLYGON3D	*poly1, POLYGON3D	*poly2)
{
	int	i;
	int	numb_points;
	POINT3D	*pts1, *pts2;


	if (poly1->nrings != poly2->nrings)
		return FALSE;
	
	numb_points = 0;
	for (i=0; i< poly1->nrings; i++)
	{
		if (poly1->npoints[i] != poly2->npoints[i])
			return FALSE;
		numb_points += poly1->npoints[i];
	}

	pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);
	

	pts2 = (POINT3D *) ( (char *)&(poly2->npoints[poly2->nrings] )  );
	pts2 = (POINT3D *) MAXALIGN(pts2);

	for (i=0; i< numb_points; i++)
	{
		if (!( is_same_point( &pts1[i], &pts2[i] ) ) )
		{
			return FALSE;
		}

	}
	return TRUE;
}



//FIXED: 
// geom1 same as geom2
//  iff  
//	+ have same type
//	+ have same # objects
//	+ have same bvol
//	+ each object in geom1 has a corresponding object in geom2 (see above)
PG_FUNCTION_INFO_V1(geometry_same);
Datum geometry_same(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool				result;
	int				i,j;
	bool				*already_hit;
	int32				type1,type2;
	int32				*offsets1,*offsets2;
	char				*o1,*o2;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


//printf("in geometry_same\n");

	//This was removed because this doesnt actually refer to the geometry, but how it is represented
	// to the user
	//easy tests
	//if (geom1->type != geom2-> type)
	//	PG_RETURN_BOOL(FALSE);

	

	if (geom1->nobjs != geom2->nobjs)
		PG_RETURN_BOOL(FALSE);	

	result = DatumGetBool(DirectFunctionCall2(box3d_same,
								PointerGetDatum(&geom1->bvol), 
								PointerGetDatum(&geom2->bvol)   )     );

	if (result == FALSE)
		PG_RETURN_BOOL(FALSE);

	if (geom1->nobjs<1)
		return FALSE; // no objects (probably a BOXONLYTYPE) and bvols dont match

//	printf(" 	+have to do it the hard way");

	already_hit = (bool *) palloc (geom2->nobjs * sizeof(bool) );
	memset(already_hit, 0, geom2->nobjs * sizeof(bool) ); //all false
	

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;
	offsets2 = (int32 *) ( ((char *) &(geom2->objType[0] ))+ sizeof(int32) * geom2->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];

		for(i=0;i<geom1->nobjs; i++)		//for each object in geom2
		{
			o2 = (char *) geom2 +offsets2[i] ;  
			type2=  geom2->objType[j];
			
			if (  (type1 == type2) && (!(already_hit[i])) )
			{
				//see if they match
				if (type1 == POINTTYPE)
				{
					if (is_same_point((POINT3D *) o1,(POINT3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 
				}
				if (type1 == LINETYPE)
				{
					if (is_same_line((LINE3D *) o1,(LINE3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 

				}
				if (type2 == POLYGONTYPE)
				{
					if (is_same_polygon((POLYGON3D *) o1,(POLYGON3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 
				}	
			}
		}
		if (i == geom1->nobjs)
			PG_RETURN_BOOL(FALSE);	 // couldnt find match
	}
	PG_RETURN_BOOL(TRUE);
}

/*******************************************************
 *Box3d routines
 *******************************************************/



/*		box_overlap		-		does box1 overlap box2?
 */
PG_FUNCTION_INFO_V1(box3d_overlap);
Datum box3d_overlap(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(box3d_ov(box1, box2));
}

/*		box_overleft	-		is the right edge of box1 to the left of
 *								the right edge of box2?
 *
 *		This is "less than or equal" for the end of a time range,
 *		when time ranges are stored as rectangles.
 */
PG_FUNCTION_INFO_V1(box3d_overleft);
Datum box3d_overleft(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool	result;

	result = FPle(box1->URT.x, box2->LLB.x);

//printf("box3d_overleft about to return %i\n",(int) result);	

	PG_RETURN_BOOL(result);
}

/*		box_right		-		is box1 strictly right of box2?
 */
PG_FUNCTION_INFO_V1(box3d_right);
Datum box3d_right(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool	result;

	result = FPgt(box1->LLB.x, box2->URT.x);
//printf("box3d_rightabout to return %i\n",(int) result);	

	PG_RETURN_BOOL(result);
}

/*		box_contain		-		does box1 contain box2?
 */
PG_FUNCTION_INFO_V1(box3d_contain);
Datum box3d_contain(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);


bool	result;

	result = FPge(box1->URT.x, box2->URT.x) &&
				   FPle(box1->LLB.x, box2->LLB.x) &&
				   FPge(box1->URT.y, box2->URT.y) &&
				   FPle(box1->LLB.y, box2->LLB.y);
	//printf("box3d_contain about to return %i\n",(int) result);	


	PG_RETURN_BOOL(result);
}


/******************************************************
 * RTREE index requires these 3 functions
 ******************************************************/

PG_FUNCTION_INFO_V1(geometry_union);
Datum geometry_union(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GEOMETRY		   *result = (GEOMETRY *) palloc(sizeof(GEOMETRY) );
	BOX3D				*n;

#ifdef DEBUG_GIST2
	printf("GIST: geometry_union called\n");
#endif



	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result->size = sizeof(GEOMETRY);
	result->type = BBOXONLYTYPE;
	result->nobjs = -1;
	result->SRID = geom1->SRID;
	result->scale = geom1->scale;
	result->offsetX = geom1->offsetX;
	result->offsetY = geom1->offsetY;


	n = &result->bvol;

	//cheat - just change the bbox

        n->URT.x = max(geom1->bvol.URT.x, geom2->bvol.URT.x);
        n->URT.y = max(geom1->bvol.URT.y, geom2->bvol.URT.y);
        n->LLB.x = min(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
        n->LLB.y = min(geom1->bvol.LLB.y, geom2->bvol.LLB.y);

	
        n->URT.z = max(geom1->bvol.URT.z, geom2->bvol.URT.z);
        n->LLB.z = min(geom1->bvol.LLB.z, geom2->bvol.LLB.z);

	  PG_FREE_IF_COPY(geom1, 0);
        PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(geometry_inter);
Datum geometry_inter(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GEOMETRY		   *result = (GEOMETRY *) palloc(sizeof(GEOMETRY) );

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
#ifdef DEBUG_GIST2
	printf("GIST: geometry_inter called\n");
#endif


	result->size = sizeof(GEOMETRY);
	result->type = BBOXONLYTYPE;
	result->nobjs = -1;
	result->SRID = geom1->SRID;
	result->scale = geom1->scale;
	result->offsetX = geom1->offsetX;
	result->offsetY = geom1->offsetY;

		result->bvol.URT.x = min(geom1->bvol.URT.x, geom2->bvol.URT.x);
		result->bvol.URT.y = min(geom1->bvol.URT.y, geom2->bvol.URT.y);
		result->bvol.URT.z = min(geom1->bvol.URT.z, geom2->bvol.URT.z);

		result->bvol.LLB.x = max(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
		result->bvol.LLB.y = max(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
		result->bvol.LLB.z = max(geom1->bvol.LLB.z, geom2->bvol.LLB.z);

        if (result->bvol.URT.x < result->bvol.LLB.x || result->bvol.URT.y < result->bvol.LLB.y)
        {
                pfree(result);
                /* Indicate "no intersection" by returning NULL pointer */
                result = NULL;
        }
	  PG_FREE_IF_COPY(geom1, 0);
        PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(geometry_size);
Datum geometry_size(PG_FUNCTION_ARGS)
{
	 Pointer        	 aptr = PG_GETARG_POINTER(0);

       float      	*size = (float *) PG_GETARG_POINTER(1);
       GEOMETRY    	*a;
	float			xdim,ydim;


//printf("entering rt_points3d_size \n");
#ifdef DEBUG_GIST2
	printf("GIST: geometry_size called\n");
#endif


        if (aptr == NULL)
        {
                *size = 0.0;
		//printf("	+ aprt==null return in 0.0\n");
                PG_RETURN_VOID();
        }

	        a = (GEOMETRY *)   PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

        if (a->bvol.URT.x <= a->bvol.LLB.x ||
                a->bvol.URT.y <= a->bvol.LLB.y)
                *size = 0.0;
        else
        {
                xdim = (a->bvol.URT.x - a->bvol.LLB.x);
                ydim = (a->bvol.URT.y - a->bvol.LLB.y);

                *size = (float) (xdim * ydim);
        }

		//printf("	+ about to return (and free) with %e\n",*size);

        /* Avoid leaking memory when handed toasted input. */
        PG_FREE_IF_COPY(a, 0);

        PG_RETURN_VOID();
}


/******************************************************
 * GiST index requires these functions
 *
 *  ORGININAL used BOXONLYTYPE geomery as index
 *
 *	NEW version uses geo_decs.h BOX (2d) type
 *
 *	Based on;
 *
 *	Taken from http://www.sai.msu.su/~megera/postgres/gist/ 
 *   {Slightley Modified}
 *
 ******************************************************/

BOX	*convert_box3d_to_box(BOX3D *in)
{
		BOX	*out = palloc (sizeof (BOX) );

		out->high.x = in->URT.x;
		out->high.y = in->URT.y;

		out->low.x = in->LLB.x;
		out->low.y = in->LLB.y;

	return out;
}



GISTENTRY *ggeometry_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry=(GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *retval;

	if ( entry->leafkey) {
		retval = palloc(sizeof(GISTENTRY));
		if ( entry->pred ) {

			GEOMETRY *in;
			GEOMETRYKEY *r;
			BOX	*thebox;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_compress called on geometry\n");
#endif

			in = (GEOMETRY*)PG_DETOAST_DATUM(PointerGetDatum(entry->pred));
			r = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
			r->size = sizeof(GEOMETRYKEY);
			r->SRID = in->SRID;
			thebox = convert_box3d_to_box(&in->bvol);
			memcpy( (void*)&(r->key), (void*)thebox, sizeof(BOX) );
			if ( (char*)in != entry->pred ) 
			{
				pfree( in );
				pfree(thebox);
			}

			gistentryinit(*retval, (char*)r, entry->rel, entry->page,
				entry->offset, sizeof(GEOMETRYKEY),FALSE);

		} else {
			gistentryinit(*retval, NULL, entry->rel, entry->page,
				entry->offset, 0,FALSE);
		} 
	} else {
		retval = entry;
	}
	return( retval );
}

bool ggeometry_consistent(PG_FUNCTION_ARGS)
{
    GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
    GEOMETRY *query       = (GEOMETRY*)	    PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
    StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
    BOX		*thebox;
    /*
    ** if entry is not leaf, use gbox_internal_consistent,
    ** else use gbox_leaf_consistent
    */

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_consistent called\n");
#endif

    if ( ! (entry->pred && query) )
	return FALSE;

	thebox = convert_box3d_to_box( &(query->bvol) );

	if(    ((GEOMETRYKEY *)(entry->pred))->SRID != query->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs (ggeometry_consistent)\n");
		PG_RETURN_BOOL(FALSE);
	}

    PG_RETURN_BOOL(rtree_internal_consistent((BOX*)&( ((GEOMETRYKEY *)(entry->pred))->key ), 
		thebox, strategy));
}


GEOMETRYKEY *ggeometry_union(PG_FUNCTION_ARGS)
{
	GEOMETRYKEY		*result;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_union called\n");
#endif

	result = (GEOMETRYKEY*) 
		rtree_union(
			(bytea*) PG_GETARG_POINTER(0),
			(int*) PG_GETARG_POINTER(1),
			ggeometry_binary_union
		);

    return result;
}


float *ggeometry_penalty(PG_FUNCTION_ARGS)
{
#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_penalty called\n");
#endif

    return rtree_penalty(
	(GISTENTRY*) PG_GETARG_POINTER(0),
	(GISTENTRY*) PG_GETARG_POINTER(1),
	(float*) PG_GETARG_POINTER(2),
	ggeometry_binary_union,
	size_geometrykey
    );
}

GIST_SPLITVEC *ggeometry_picksplit(PG_FUNCTION_ARGS)
{
#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_picksplit called\n");
#endif


    return rtree_picksplit(
	(bytea*)PG_GETARG_POINTER(0),
	(GIST_SPLITVEC*)PG_GETARG_POINTER(1),
	sizeof(GEOMETRYKEY),
	ggeometry_binary_union,
	ggeometry_inter,
	size_geometrykey
    );
}

bool *ggeometry_same(PG_FUNCTION_ARGS)
{


  GEOMETRYKEY *b1 = (GEOMETRYKEY*) PG_GETARG_POINTER(0);
  GEOMETRYKEY *b2 = (GEOMETRYKEY*) PG_GETARG_POINTER(1);

  bool *result = (bool*) PG_GETARG_POINTER(2);

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_same called\n");
#endif


  if ( b1 && b2 )
   	*result = DatumGetBool( DirectFunctionCall2( box_same, 
		PointerGetDatum(&(b1->key)), 
		PointerGetDatum(&(b2->key))) );
  else
	*result = ( b1==NULL && b2==NULL ) ? TRUE : FALSE; 
  return(result);
}

Datum ggeometry_inter(PG_FUNCTION_ARGS) {
  	GEOMETRYKEY *b1 = (GEOMETRYKEY*) PG_GETARG_POINTER(0);
  	GEOMETRYKEY*b2 = (GEOMETRYKEY*) PG_GETARG_POINTER(1);
	char *interd;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_inter called\n");
#endif


    	interd = DatumGetPointer(DirectFunctionCall2(
			rt_box_inter,
			PointerGetDatum( &(b1->key) ),
			PointerGetDatum( &(b2->key) )) );
	
	if (interd) {
		GEOMETRYKEY *tmp = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		tmp->size = sizeof(GEOMETRYKEY);
	
		memcpy( (void*)&(tmp->key), (void*)interd, sizeof(BOX) );
		tmp->SRID = b1->SRID;
		pfree( interd );
		PG_RETURN_POINTER( tmp );
	} else 
		PG_RETURN_POINTER( NULL );
}

char *ggeometry_binary_union(char *r1, char *r2, int *sizep)
{
    GEOMETRYKEY *retval;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_binary_union called\n");
#endif



    if ( ! (r1 && r2) ) {
	if ( r1 ) {
		retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		memcpy( (void*)retval, (void*)r1, sizeof(GEOMETRYKEY) );
    		*sizep = sizeof(GEOMETRYKEY);
	} else if ( r2 ) {
		retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		memcpy( (void*)retval, (void*)r2, sizeof(GEOMETRYKEY) );
    		*sizep = sizeof(GEOMETRYKEY);
	} else {
		*sizep = 0;
		retval = NULL;
	} 
    } else {
    	BOX *key = (BOX*)DatumGetPointer( DirectFunctionCall2(
		rt_box_union,
		PointerGetDatum( &(((GEOMETRYKEY*)r1)->key) ),
		PointerGetDatum( &(((GEOMETRYKEY*)r2)->key) )) );
	retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
	retval->SRID = ((GEOMETRYKEY *) r1)->SRID;
	memcpy( (void*)&(retval->key), (void*)key, sizeof(BOX) );
	pfree( key );
    	*sizep = retval->size = sizeof(GEOMETRYKEY);
    }
    return (char*)retval;
}


float size_geometrykey( char *pk ) {

#ifdef DEBUG_GIST2
	printf("GIST: size_geometrykey called\n");
#endif


    if ( pk ) {
	float size;
    	DirectFunctionCall2( rt_box_size,
		PointerGetDatum( &(((GEOMETRYKEY*)pk)->key) ),
		PointerGetDatum( &size ) );
	return size;
    } else
	return 0.0;
}

char *rtree_union(bytea *entryvec, int *sizep, BINARY_UNION bu)
{
    int numranges, i;
    char *out, *tmp;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_union called\n");
#endif



    numranges = (VARSIZE(entryvec) - VARHDRSZ)/sizeof(GISTENTRY); 
    tmp = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[0]).pred;
    out = NULL;

    for (i = 1; i < numranges; i++) {
	out = (*bu)(tmp, (char *)
				 (((GISTENTRY *)(VARDATA(entryvec)))[i]).pred,
				 sizep);
	if (i > 1 && tmp) pfree(tmp);
	tmp = out;
    }

    return(out);
}

float *rtree_penalty(GISTENTRY *origentry, GISTENTRY *newentry, float *result, BINARY_UNION bu, SIZE_BOX sb)
{
    char * ud;
    float tmp1;
    int sizep;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_penalty called\n");
#endif


   
    ud = (*bu)( origentry->pred, newentry->pred, &sizep );
    tmp1 = (*sb)( ud ); 
    if (ud) pfree(ud);

    *result = tmp1 - (*sb)( origentry->pred );
    return(result);
}

/*
** The GiST PickSplit method
** We use Guttman's poly time split algorithm 
*/
GIST_SPLITVEC *rtree_picksplit(bytea *entryvec, GIST_SPLITVEC *v, int keylen, BINARY_UNION bu, RDF interop, SIZE_BOX sb)
{
    OffsetNumber i, j;
    char *datum_alpha, *datum_beta;
    char *datum_l, *datum_r;
    char *union_d, *union_dl, *union_dr;
    char *inter_d;
    bool firsttime;
    float size_alpha, size_beta, size_union, size_inter;
    float size_waste, waste;
    float size_l, size_r;
    int nbytes;
    int sizep;
    OffsetNumber seed_1 = 0, seed_2 = 0;
    OffsetNumber *left, *right;
    OffsetNumber maxoff;


#ifdef DEBUG_GIST2
	printf("GIST: rtree_picsplit called\n");
#endif


    maxoff = ((VARSIZE(entryvec) - VARHDRSZ)/sizeof(GISTENTRY)) - 2;
    nbytes =  (maxoff + 2) * sizeof(OffsetNumber);
    v->spl_left = (OffsetNumber *) palloc(nbytes);
    v->spl_right = (OffsetNumber *) palloc(nbytes);
    
    firsttime = true;
    waste = 0.0;
    
    for (i = FirstOffsetNumber; i < maxoff; i = OffsetNumberNext(i)) {
	datum_alpha = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[i].pred);
	for (j = OffsetNumberNext(i); j <= maxoff; j = OffsetNumberNext(j)) {
	    datum_beta = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[j].pred);
	    
	    /* compute the wasted space by unioning these guys */
	    /* size_waste = size_union - size_inter; */	
	    union_d = (*bu)( datum_alpha, datum_beta, &sizep );
	    if ( union_d ) {
		size_union = (*sb)(union_d);
		pfree(union_d);
	    } else
		size_union = 0.0;

	    if ( datum_alpha && datum_beta ) {
    	    	inter_d = DatumGetPointer(DirectFunctionCall2(
			interop,
			PointerGetDatum( datum_alpha ),
			PointerGetDatum( datum_beta )) );
		if ( inter_d ) {
			size_inter = (*sb)(inter_d);
			pfree(inter_d);
		} else 
			size_inter = 0.0;
	    } else
		size_inter = 0.0;

	    size_waste = size_union - size_inter;
	    
	    /*
	     *  are these a more promising split that what we've
	     *  already seen?
	     */
	    
	    if (size_waste > waste || firsttime) {
		waste = size_waste;
		seed_1 = i;
		seed_2 = j;
		firsttime = false;
	    }
	}
    }
    
    left = v->spl_left;
    v->spl_nleft = 0;
    right = v->spl_right;
    v->spl_nright = 0;
  
    if ( ((GISTENTRY *)(VARDATA(entryvec)))[seed_1].pred ) {
	datum_l = (char*) palloc( keylen );
	memcpy( (void*)datum_l, (void*)(((GISTENTRY *)(VARDATA(entryvec)))[seed_1].pred ), keylen );
    } else 
	datum_l = NULL; 
    size_l  = (*sb)( datum_l ); 
    if ( ((GISTENTRY *)(VARDATA(entryvec)))[seed_2].pred ) {
	datum_r = (char*) palloc( keylen );
	memcpy( (void*)datum_r, (void*)(((GISTENTRY *)(VARDATA(entryvec)))[seed_2].pred ), keylen );
    } else 
	datum_r = NULL; 
    size_r  = (*sb)( datum_r ); 
    
    /*
     *  Now split up the regions between the two seeds.  An important
     *  property of this split algorithm is that the split vector v
     *  has the indices of items to be split in order in its left and
     *  right vectors.  We exploit this property by doing a merge in
     *  the code that actually splits the page.
     *
     *  For efficiency, we also place the new index tuple in this loop.
     *  This is handled at the very end, when we have placed all the
     *  existing tuples and i == maxoff + 1.
     */
    
    maxoff = OffsetNumberNext(maxoff);
    for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
	
	/*
	 *  If we've already decided where to place this item, just
	 *  put it on the right list.  Otherwise, we need to figure
	 *  out which page needs the least enlargement in order to
	 *  store the item.
	 */
	
	if (i == seed_1) {
	    *left++ = i;
	    v->spl_nleft++;
	    continue;
	} else if (i == seed_2) {
	    *right++ = i;
	    v->spl_nright++;
	    continue;
	}
	
	/* okay, which page needs least enlargement? */ 
	datum_alpha = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[i].pred);
	union_dl = (*bu)( datum_l, datum_alpha, &sizep );
	union_dr = (*bu)( datum_r, datum_alpha, &sizep );
	size_alpha = (*sb)( union_dl ); 
	size_beta  = (*sb)( union_dr ); 
	
	/* pick which page to add it to */
	if (size_alpha - size_l < size_beta - size_r) {
	    pfree(datum_l);
	    pfree(union_dr);
	    datum_l = union_dl;
	    size_l = size_alpha;
	    *left++ = i;
	    v->spl_nleft++;
	} else {
	    pfree(datum_r);
	    pfree(union_dl);
	    datum_r = union_dr;
	    size_r = size_alpha;
	    *right++ = i;
	    v->spl_nright++;
	}
    }
    *left = *right = FirstOffsetNumber;	/* sentinel value, see dosplit() */
    
    v->spl_ldatum = datum_l;
    v->spl_rdatum = datum_r;

    return( v );
}


bool rtree_internal_consistent(BOX *key,
			BOX *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_internal_consist called\n");
#endif



    switch(strategy) {
    case RTLeftStrategyNumber:
    case RTOverLeftStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overleft, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTOverlapStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTOverRightStrategyNumber:
    case RTRightStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_right, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTSameStrategyNumber:
    case RTContainsStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_contain, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTContainedByStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    default:
      retval = FALSE;
    }
    return(retval);
}

GISTENTRY *rtree_decompress(PG_FUNCTION_ARGS)
{
    return((GISTENTRY*)PG_GETARG_POINTER(0));
}


PG_FUNCTION_INFO_V1(box3d_xmin);
Datum box3d_xmin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.x);
}

PG_FUNCTION_INFO_V1(box3d_ymin);
Datum box3d_ymin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.y);
}
PG_FUNCTION_INFO_V1(box3d_zmin);
Datum box3d_zmin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.z);
}
PG_FUNCTION_INFO_V1(box3d_xmax);
Datum box3d_xmax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.x);
}
PG_FUNCTION_INFO_V1(box3d_ymax);
Datum box3d_ymax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.y);
}
PG_FUNCTION_INFO_V1(box3d_zmax);
Datum box3d_zmax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.z);
}
