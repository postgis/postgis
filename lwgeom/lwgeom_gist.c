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
//#define DEBUG_GIST
//#define DEBUG_GIST2
//#define DEBUG_GIST3
//#define DEBUG_GIST4
//#define DEBUG_GIST5
//#define DEBUG_GIST6

Datum lwgeom_same(PG_FUNCTION_ARGS);
Datum lwgeom_overlap(PG_FUNCTION_ARGS);
Datum lwgeom_overleft(PG_FUNCTION_ARGS);
Datum lwgeom_left(PG_FUNCTION_ARGS);
Datum lwgeom_right(PG_FUNCTION_ARGS);
Datum lwgeom_overright(PG_FUNCTION_ARGS);
Datum lwgeom_contained(PG_FUNCTION_ARGS);
Datum lwgeom_contain(PG_FUNCTION_ARGS);



static float size_box2d(Datum box);

static bool lwgeom_rtree_internal_consistent(BOX2DFLOAT4 *key,BOX2DFLOAT4 *query,StrategyNumber strategy);
static bool lwgeom_rtree_leaf_consistent(BOX2DFLOAT4 *key,BOX2DFLOAT4 *query,	StrategyNumber strategy);


static char *bbox_cache_index =NULL;
static BOX2DFLOAT4 *bbox_cache_value = NULL;
static BOX2DFLOAT4 *getBOX2D_cache(char *lwgeom);
static int counter = 0;
static int counter2 = 0;
static int counter1 = 0;

int counter_leaf = 0;
int counter_intern = 0;

static BOX2DFLOAT4 *getBOX2D_cache(char *lwgeom)
{
	if (bbox_cache_index != lwgeom)
	{
			BOX2DFLOAT4 box;

#ifdef DEBUG_GIST4
		elog(NOTICE,"GIST: getBOX2D_cache -- cache miss %i times",counter);
		counter++;
#endif

			if (bbox_cache_value == NULL)
			   	bbox_cache_value = malloc(sizeof(BOX2DFLOAT4)); // initial value

			box = getbox2d(lwgeom+4);

			memcpy(bbox_cache_value, &box, sizeof(BOX2DFLOAT4));
			bbox_cache_index = lwgeom;
			return bbox_cache_value;
	}
	else
	{
		return bbox_cache_value;
	}
}

// all the lwgeom_<same,overlpa,overleft,left,right,overright,contained,contain>
//  work the same.
//  1. get lwgeom1
//  2. get lwgeom2
//  3. get box3d for lwgeom1
//  4. get box3d for lwgeom2
//  5. convert box3d (for lwgeom1) to BOX2DFLOAT4
//  6. convert box3d (for lwgeom2) to BOX2DFLOAT4
//  7. call the appropriate BOX2DFLOAT4 function
//  8. return result;

// lwgeom_same(lwgeom1, lwgeom2)
PG_FUNCTION_INFO_V1(lwgeom_same);
Datum lwgeom_same(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);


#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_same --entry");
#endif


			result = DatumGetBool(DirectFunctionCall2(box2d_same,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(lwgeom_overlap);
Datum lwgeom_overlap(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);


#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_overlap --entry");
#endif

			result = DatumGetBool(DirectFunctionCall2(box2d_overlap,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(lwgeom_overleft);
Datum lwgeom_overleft(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);
#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_overleft --entry");
#endif

			result = DatumGetBool(DirectFunctionCall2(box2d_overleft,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(lwgeom_left);
Datum lwgeom_left(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;

				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);

#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_left --entry");
#endif

			result = DatumGetBool(DirectFunctionCall2(box2d_left,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(lwgeom_right);
Datum lwgeom_right(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);
#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_right --entry");
#endif
			result = DatumGetBool(DirectFunctionCall2(box2d_right,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(lwgeom_overright);
Datum lwgeom_overright(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);

#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_overright --entry");
#endif

			result = DatumGetBool(DirectFunctionCall2(box2d_overright,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(lwgeom_contained);
Datum lwgeom_contained(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);

#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_contained --entry");
#endif
			result = DatumGetBool(DirectFunctionCall2(box2d_contained,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(lwgeom_contain);
Datum lwgeom_contain(PG_FUNCTION_ARGS)
{
			char		        *lwgeom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
			char		        *lwgeom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
			bool				result;


				BOX2DFLOAT4				box1 = getbox2d(lwgeom1+4);
				BOX2DFLOAT4				box2 = getbox2d(lwgeom2+4);

#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: lwgeom_contain --entry");
#endif

			result = DatumGetBool(DirectFunctionCall2(box2d_contain,
											          PointerGetDatum(&box1),
								                      PointerGetDatum(&box2)   )     );

		PG_FREE_IF_COPY(lwgeom1, 0);
        PG_FREE_IF_COPY(lwgeom2, 1);

        PG_RETURN_BOOL(result);
}


// These functions are taken from the postgis_gist_72.c file



PG_FUNCTION_INFO_V1(gist_lwgeom_compress);
Datum gist_lwgeom_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry=(GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *retval;


#ifdef DEBUG_GIST4
	elog(NOTICE,"GIST: gist_lwgeom_compress called on lwgeom");
#endif

	if ( entry->leafkey)
	{
		retval = palloc(sizeof(GISTENTRY));
		if ( DatumGetPointer(entry->key) != NULL )
		{

			char        *in; // lwgeom serialized
			BOX2DFLOAT4	r,*rr;

			in = (char*)PG_DETOAST_DATUM( entry->key );  // lwgeom serialized form

			if (in == NULL)
				PG_RETURN_POINTER(entry);

			if (lwgeom_getnumgeometries(in+4) ==0)  // this is the EMPTY geometry
			{
elog(NOTICE,"found an empty geometry");
				// dont bother adding this to the index
				PG_RETURN_POINTER(entry);
			}
			else
			{
				r = getbox2d(in+4);
				rr = (BOX2DFLOAT4*) palloc(sizeof(BOX2DFLOAT4));
				memcpy(rr,&r,sizeof(BOX2DFLOAT4));


#ifdef DEBUG_GIST2
	elog(NOTICE,"GIST: gist_lwgeom_compress -- got box2d BOX(%.15g %.15g,%.15g %.15g)", r.xmin, r.ymin, r.xmax, r.ymax);
#endif
				//r = convert_box3d_to_box(&in->bvol);

				if ( in != (char*)DatumGetPointer(entry->key) )
				{
					pfree( in );  // PG_FREE_IF_COPY
				}

				gistentryinit(*retval, PointerGetDatum(rr), entry->rel, entry->page,
					entry->offset, sizeof(BOX2DFLOAT4), FALSE);
			}

		}
		else
		{
			gistentryinit(*retval, (Datum) 0, entry->rel, entry->page,
				entry->offset, 0,FALSE);
		}
	}
	else
	{
		retval = entry;
	}

	PG_RETURN_POINTER(retval);
}


PG_FUNCTION_INFO_V1(gist_lwgeom_consistent);
Datum gist_lwgeom_consistent(PG_FUNCTION_ARGS)
{
    GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
    char *query ; // lwgeom serialized form
    StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
   // BOX		thebox;
	bool result;
	BOX2DFLOAT4 *thebox;
	BOX2DFLOAT4  box;

	/*
    ** if entry is not leaf, use rtree_internal_consistent,
    ** else use rtree_leaf_consistent
    */
#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gist_lwgeom_consistent called");
#endif

   if (   ( (Pointer *) PG_GETARG_DATUM(1) ) == NULL)
    {
		//elog(NOTICE,"gist_lwgeom_consistent:: got null query!");
		PG_RETURN_BOOL(false); // null query - this is screwy!
	}

	query = (char*) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


    if ( ! (DatumGetPointer(entry->key) != NULL && query) )
		PG_RETURN_BOOL(FALSE);


	//box3d = lw_geom_getBB(query+4);
	//thebox = box3d_to_box2df(box3d);
	//pfree(box3d);
		//convert_box3d_to_box_p( &(query->bvol) , &thebox);
	//thebox = getBOX2D_cache(query);
  getbox2d_p(query+4, &box);

	if (GIST_LEAF(entry))
		result = lwgeom_rtree_leaf_consistent((BOX2DFLOAT4 *) DatumGetPointer(entry->key), &box, strategy );
	else
		result = lwgeom_rtree_internal_consistent((BOX2DFLOAT4 *) DatumGetPointer(entry->key), &box, strategy );

	//pfree(thebox);
	PG_FREE_IF_COPY(query, 1);
	PG_RETURN_BOOL(result);
}


static bool lwgeom_rtree_internal_consistent(BOX2DFLOAT4 *key,
			BOX2DFLOAT4 *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: lwgeom_rtree_internal_consistent called with strategy=%i",strategy);
#endif

    switch(strategy) {
    case RTLeftStrategyNumber:
    case RTOverLeftStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box2d_overleft, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTOverlapStrategyNumber:  //optimized for speed

        retval = (((key->xmax>= query->xmax) &&
			 (key->xmin <= query->xmax)) ||
			((query->xmax>= key->xmax) &&
			 (query->xmin<= key->xmax)))
		&&
		(((key->ymax>= query->ymax) &&
		  (key->ymin<= query->ymax)) ||
		 ((query->ymax>= key->ymax) &&
		  (query->ymin<= key->ymax)));



#ifdef DEBUG_GIST5
					if (counter_intern == 0)
					{
									 elog(NOTICE,"search bounding box is: <%.16g %.16g,%.16g %.16g> - size box2d= %i",
									 	query->xmin,query->ymin,query->xmax,query->ymax,sizeof(BOX2DFLOAT4));

					}



elog(NOTICE,"%i:(int)<%.8g %.8g,%.8g %.8g>&&<%.8g %.8g,%.8g %.8g> %i",counter_intern,key->xmin,key->ymin,key->xmax,key->ymax,
		  						query->xmin,query->ymin,query->xmax,query->ymax,   (int) retval);
#endif
			counter_intern++;
		  return(retval);
      break;
    case RTOverRightStrategyNumber:
    case RTRightStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box2d_overright, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTSameStrategyNumber:
    case RTContainsStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box2d_contain, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTContainedByStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box2d_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    default:
      retval = FALSE;
    }
    return(retval);
}


static bool lwgeom_rtree_leaf_consistent(BOX2DFLOAT4 *key,
			BOX2DFLOAT4 *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: rtree_leaf_consist called with strategy=%i",strategy);
#endif

	switch (strategy)
	{
		case RTLeftStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_left, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTOverLeftStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_overleft, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTOverlapStrategyNumber://optimized for speed
			           retval = (((key->xmax>= query->xmax) &&
			   			 (key->xmin <= query->xmax)) ||
			   			((query->xmax>= key->xmax) &&
			   			 (query->xmin<= key->xmax)))
			   		&&
			   		(((key->ymax>= query->ymax) &&
			   		  (key->ymin<= query->ymax)) ||
			   		 ((query->ymax>= key->ymax) &&
			   		  (query->ymin<= key->ymax)));
#ifdef DEBUG_GIST5
			   elog(NOTICE,"%i:gist test (leaf) <%.6g %.6g,%.6g %.6g> &&  <%.6g %.6g,%.6g %.6g> --> %i",counter_leaf,key->xmin,key->ymin,key->xmax,key->ymax,
			   		  						query->xmin,query->ymin,query->xmax,query->ymax,   (int) retval);
#endif
			   counter_leaf++;
		  return(retval);
			break;
		case RTOverRightStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_overright, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTRightStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_right, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTSameStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_same, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTContainsStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_contain, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTContainedByStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box2d_contained, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		default:
			retval = FALSE;
	}
	return (retval);
}



PG_FUNCTION_INFO_V1(gist_rtree_decompress);
Datum gist_rtree_decompress(PG_FUNCTION_ARGS)
{
#ifdef DEBUG_GIST
		elog(NOTICE,"GIST: gist_rtree_decompress called %i",counter2);
		counter2++;
#endif

    PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}



/*
** The GiST Union method for boxes
** returns the minimal bounding box that encloses all the entries in entryvec
*/
PG_FUNCTION_INFO_V1(lwgeom_box_union);
Datum lwgeom_box_union(PG_FUNCTION_ARGS)
{
	bytea	   *entryvec = (bytea *) PG_GETARG_POINTER(0);
	int		   *sizep = (int *) PG_GETARG_POINTER(1);
	int			numranges,
				i;
	BOX2DFLOAT4		   *cur,
			   *pageunion;


	numranges = (VARSIZE(entryvec) - VARHDRSZ) / sizeof(GISTENTRY);
	pageunion = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	cur = (BOX2DFLOAT4 *) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[0].key);
	memcpy((void *) pageunion, (void *) cur, sizeof(BOX2DFLOAT4));




	for (i = 1; i < numranges; i++)
	{
		cur = (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[i].key);

		if (pageunion->xmax < cur->xmax)
			pageunion->xmax = cur->xmax;
		if (pageunion->xmin > cur->xmin)
			pageunion->xmin = cur->xmin;
		if (pageunion->ymax < cur->ymax)
			pageunion->ymax = cur->ymax;
		if (pageunion->ymin > cur->ymin)
			pageunion->ymin = cur->ymin;
	}

	*sizep = sizeof(BOX2DFLOAT4);

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gbox_union called with numranges=%i pageunion is: <%.16g %.16g,%.16g %.16g>", numranges,pageunion->xmin, pageunion->ymin, pageunion->xmax, pageunion->ymax);
#endif


	PG_RETURN_POINTER(pageunion);
}

static float size_box2d(Datum box2d)
{

	float result;


	if (DatumGetPointer(box2d) != NULL)
	{
		BOX2DFLOAT4 *a = (BOX2DFLOAT4*) DatumGetPointer(box2d);

		    if (a == (BOX2DFLOAT4 *) NULL || a->xmax <= a->xmin || a->ymax <= a->ymin)
		        result =  (float) 0.0;
		    else
		    {
				result = (((double) a->xmax)-((double) a->xmin)) * (((double) a->ymax)-((double) a->ymin));
			}
		      //  result= (float) ((a->xmax - a->xmin) * (a->ymax - a->ymin));
	}
	else
		result = (float) 0.0;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: size_box2d called - returning %.15g",result);
#endif
	return result;
}

static double size_box2d_double(Datum box2d);
static double size_box2d_double(Datum box2d)
{

	double result;


	if (DatumGetPointer(box2d) != NULL)
	{
		BOX2DFLOAT4 *a = (BOX2DFLOAT4*) DatumGetPointer(box2d);

		    if (a == (BOX2DFLOAT4 *) NULL || a->xmax <= a->xmin || a->ymax <= a->ymin)
		        result =  (double) 0.0;
		    else
		    {
				result = (((double) a->xmax)-((double) a->xmin)) * (((double) a->ymax)-((double) a->ymin));
			}
		      //  result= (float) ((a->xmax - a->xmin) * (a->ymax - a->ymin));
	}
	else
		result = (double) 0.0;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: size_box2d_double called - returning %.15g",result);
#endif
	return result;
}



/*
** The GiST Penalty method for boxes
** As in the R-tree paper, we use change in area as our penalty metric
*/
PG_FUNCTION_INFO_V1(lwgeom_box_penalty);
Datum lwgeom_box_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY  *origentry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *newentry = (GISTENTRY *) PG_GETARG_POINTER(1);
	float	   *result = (float *) PG_GETARG_POINTER(2);
	Datum		ud;
	BOX2DFLOAT4	*b1, *b2;
	double		tmp1;



	ud = DirectFunctionCall2(box2d_union, origentry->key, newentry->key);
	tmp1 = size_box2d_double(ud);
	if (DatumGetPointer(ud) != NULL)
		pfree(DatumGetPointer(ud));

	*result = tmp1 - size_box2d_double(origentry->key);
/*
	if ( (*result <0) && (*result > -0.00001))
		*result = 0;
	if ( (*result >0) && (*result < 0.00001))
		*result = 0;

*/

#ifdef DEBUG_GIST6
	elog(NOTICE,"GIST: lwgeom_box_penalty called and returning %.15g", *result);
	if (*result<0)
	{
		BOX2DFLOAT4 *a, *b,*c;
		a = (BOX2DFLOAT4*) DatumGetPointer(origentry->key);
		b = (BOX2DFLOAT4*) DatumGetPointer(newentry->key);
		c = (BOX2DFLOAT4*) DatumGetPointer(DirectFunctionCall2(box2d_union, origentry->key, newentry->key));
		//elog(NOTICE,"lwgeom_box_penalty -- a = <%.16g %.16g,%.16g %.16g>", a->xmin, a->ymin, a->xmax, a->ymax);
		//elog(NOTICE,"lwgeom_box_penalty -- b = <%.16g %.16g,%.16g %.16g>", b->xmin, b->ymin, b->xmax, b->ymax);
		//elog(NOTICE,"lwgeom_box_penalty -- c = <%.16g %.16g,%.16g %.16g>", c->xmin, c->ymin, c->xmax, c->ymax);
	}
#endif

	PG_RETURN_POINTER(result);
}

typedef struct {
	BOX2DFLOAT4 	*key;
	int 	pos;
} KBsort;

static int
compare_KB(const void* a, const void* b) {
	BOX2DFLOAT4 *abox = ((KBsort*)a)->key;
	BOX2DFLOAT4 *bbox = ((KBsort*)b)->key;
	float sa = (abox->xmax - abox->xmin) * (abox->ymax - abox->ymin);
	float sb = (bbox->xmax - bbox->xmin) * (bbox->ymax - bbox->ymin);

	if ( sa==sb ) return 0;
	return ( sa>sb ) ? 1 : -1;
}

/*
** Equality method
*/
PG_FUNCTION_INFO_V1(lwgeom_gbox_same);
Datum lwgeom_gbox_same(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4		   *b1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4		   *b2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: lwgeom_gbox_same called");
#endif

	if (b1 && b2)
		*result = DatumGetBool(DirectFunctionCall2(box2d_same, PointerGetDatum(b1), PointerGetDatum(b2)));
	else
		*result = (b1 == NULL && b2 == NULL) ? TRUE : FALSE;
	PG_RETURN_POINTER(result);
}




/*
** The GiST PickSplit method
** New linear algorithm, see 'New Linear Node Splitting Algorithm for R-tree',
** C.H.Ang and T.C.Tan
*/
PG_FUNCTION_INFO_V1(lwgeom_gbox_picksplit);
Datum lwgeom_gbox_picksplit(PG_FUNCTION_ARGS)
{
	bytea	   *entryvec = (bytea *) PG_GETARG_POINTER(0);
	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i;
	OffsetNumber *listL,
			   *listR,
			   *listB,
			   *listT;
	BOX2DFLOAT4		   *unionL,
			   *unionR,
			   *unionB,
			   *unionT;
	int			posL,
				posR,
				posB,
				posT;
	BOX2DFLOAT4			pageunion;
	BOX2DFLOAT4		   *cur;
	char		direction = ' ';
	bool		allisequal = true;
	OffsetNumber maxoff;
	int			nbytes;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: lwgeom_gbox_picksplit called");
#endif

#ifdef DEBUG_GIST6
	elog(NOTICE,"GIST: lwgeom_gbox_picksplit called -- analysing");
#endif


	posL = posR = posB = posT = 0;
	maxoff = ((VARSIZE(entryvec) - VARHDRSZ) / sizeof(GISTENTRY)) - 1;



	cur = (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[FirstOffsetNumber].key);
	memcpy((void *) &pageunion, (void *) cur, sizeof(BOX2DFLOAT4));

#ifdef DEBUG_GIST6
elog(NOTICE,"   cur is: <%.16g %.16g,%.16g %.16g>", cur->xmin, cur->ymin, cur->xmax, cur->ymax);
#endif


	/* find MBR */
	for (i = OffsetNumberNext(FirstOffsetNumber); i <= maxoff; i = OffsetNumberNext(i))
	{
		cur =  (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[i].key);
		if ( allisequal == true &&  (
				pageunion.xmax != cur->xmax ||
				pageunion.ymax != cur->ymax ||
				pageunion.xmin != cur->xmin ||
				pageunion.ymin != cur->ymin
			) )
			allisequal = false;

		if (pageunion.xmax < cur->xmax)
			pageunion.xmax = cur->xmax;
		if (pageunion.xmin > cur->xmin)
			pageunion.xmin = cur->xmin;
		if (pageunion.ymax < cur->ymax)
			pageunion.ymax = cur->ymax;
		if (pageunion.ymin> cur->ymin)
			pageunion.ymin = cur->ymin;
	}

#ifdef DEBUG_GIST6
elog(NOTICE,"   pageunion is: <%.16g %.16g,%.16g %.16g>", pageunion.xmin, pageunion.ymin, pageunion.xmax, pageunion.ymax);
#endif


	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	listL = (OffsetNumber *) palloc(nbytes);
	listR = (OffsetNumber *) palloc(nbytes);
	unionL = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	unionR = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

	if (allisequal)
	{
#ifdef DEBUG_GIST6
elog(NOTICE," AllIsEqual!");
#endif
		cur = (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[OffsetNumberNext(FirstOffsetNumber)].key);
		if (memcmp((void *) cur, (void *) &pageunion, sizeof(BOX2DFLOAT4)) == 0)
		{
			v->spl_left = listL;
			v->spl_right = listR;
			v->spl_nleft = v->spl_nright = 0;
			memcpy((void *) unionL, (void *) &pageunion, sizeof(BOX2DFLOAT4));
			memcpy((void *) unionR, (void *) &pageunion, sizeof(BOX2DFLOAT4));

			for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
			{
				if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
				{
					v->spl_left[v->spl_nleft] = i;
					v->spl_nleft++;
				}
				else
				{
					v->spl_right[v->spl_nright] = i;
					v->spl_nright++;
				}
			}
			v->spl_ldatum = PointerGetDatum(unionL);
			v->spl_rdatum = PointerGetDatum(unionR);

			PG_RETURN_POINTER(v);
		}
	}




	listB = (OffsetNumber *) palloc(nbytes);
	listT = (OffsetNumber *) palloc(nbytes);
	unionB = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	unionT = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

#define ADDLIST( list, unionD, pos, num ) do { \
	if ( pos ) { \
		if ( unionD->xmax < cur->xmax )    unionD->xmax	= cur->xmax; \
		if ( unionD->xmin	> cur->xmin  ) unionD->xmin	= cur->xmin; \
		if ( unionD->ymax < cur->ymax )    unionD->ymax	= cur->ymax; \
		if ( unionD->ymin	> cur->ymin  ) unionD->ymin	= cur->ymin; \
	} else { \
			memcpy( (void*)unionD, (void*) cur, sizeof( BOX2DFLOAT4 ) );  \
	} \
	list[pos] = num; \
	(pos)++; \
} while(0)

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[i].key);
		if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
			ADDLIST(listL, unionL, posL,i);
		else
			ADDLIST(listR, unionR, posR,i);
		if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
			ADDLIST(listB, unionB, posB,i);
		else
			ADDLIST(listT, unionT, posT,i);
	}

#ifdef DEBUG_GIST6
elog(NOTICE,"   unionL is: <%.16g %.16g,%.16g %.16g>", unionL->xmin, unionL->ymin, unionL->xmax, unionL->ymax);
elog(NOTICE,"   unionR is: <%.16g %.16g,%.16g %.16g>", unionR->xmin, unionR->ymin, unionR->xmax, unionR->ymax);
elog(NOTICE,"   unionT is: <%.16g %.16g,%.16g %.16g>", unionT->xmin, unionT->ymin, unionT->xmax, unionT->ymax);
elog(NOTICE,"   unionB is: <%.16g %.16g,%.16g %.16g>", unionB->xmin, unionB->ymin, unionB->xmax, unionB->ymax);
#endif



	/* bad disposition, sort by ascending and resplit */
	if ( (posR==0 || posL==0) && (posT==0 || posB==0) ) {
		KBsort *arr = (KBsort*)palloc( sizeof(KBsort) * maxoff );
		posL = posR = posB = posT = 0;
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
			arr[i-1].key = (BOX2DFLOAT4*) DatumGetPointer(((GISTENTRY *) VARDATA(entryvec))[i].key);
			arr[i-1].pos = i;
		}
		qsort( arr, maxoff, sizeof(KBsort), compare_KB );
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
			cur = arr[i-1].key;
			if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
				ADDLIST(listL, unionL, posL,arr[i-1].pos);
			else if ( cur->xmin - pageunion.xmin == pageunion.xmax - cur->xmax ) {
				if ( posL>posR )
					ADDLIST(listR, unionR, posR,arr[i-1].pos);
				else
					ADDLIST(listL, unionL, posL,arr[i-1].pos);
			} else
				ADDLIST(listR, unionR, posR,arr[i-1].pos);

			if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
				ADDLIST(listB, unionB, posB,arr[i-1].pos);
			else if ( cur->ymin - pageunion.ymin == pageunion.ymax - cur->ymax ) {
				if ( posB>posT )
					ADDLIST(listT, unionT, posT,arr[i-1].pos);
				else
					ADDLIST(listB, unionB, posB,arr[i-1].pos);
			} else
				ADDLIST(listT, unionT, posT,arr[i-1].pos);
		}
		pfree(arr);
	}

	/* which split more optimal? */
	if (Max(posL, posR) < Max(posB, posT))
		direction = 'x';
	else if (Max(posL, posR) > Max(posB, posT))
		direction = 'y';
	else
	{
		Datum		interLR = DirectFunctionCall2(box2d_inter,
												  PointerGetDatum(unionL),
												  PointerGetDatum(unionR));
		Datum		interBT = DirectFunctionCall2(box2d_inter,
												  PointerGetDatum(unionB),
												  PointerGetDatum(unionT));
		float		sizeLR,
					sizeBT;

//elog(NOTICE,"direction is abigeous");

		sizeLR = size_box2d(interLR);
		sizeBT = size_box2d(interBT);

		if (sizeLR < sizeBT)
			direction = 'x';
		else
			direction = 'y';
	}

	if (direction == 'x')
	{
		pfree(unionB);
		pfree(listB);
		pfree(unionT);
		pfree(listT);

		v->spl_left = listL;
		v->spl_right = listR;
		v->spl_nleft = posL;
		v->spl_nright = posR;
		v->spl_ldatum = PointerGetDatum(unionL);
		v->spl_rdatum = PointerGetDatum(unionR);

#ifdef DEBUG_GIST6
elog(NOTICE,"   split direction was '%c'", direction);
elog(NOTICE,"   posL = %i, posR=%i", posL,posR);
elog(NOTICE,"   posL's (nleft) offset numbers:");
{
		char		aaa[5000],bbb[100];
		aaa[0] =0;

for (i=0;i<posL;i++)
{
	sprintf(bbb," %i", listL[i]);
	strcat(aaa,bbb);
}
elog(NOTICE,aaa);
aaa[0]=0;
elog(NOTICE,"   posR's (nright) offset numbers:");
for (i=0;i<posR;i++)
{
	sprintf(bbb," %i", listR[i]);
	strcat(aaa,bbb);
}
elog(NOTICE,aaa);
}

#endif


	}
	else
	{
		pfree(unionR);
		pfree(listR);
		pfree(unionL);
		pfree(listL);

		v->spl_left = listB;
		v->spl_right = listT;
		v->spl_nleft = posB;
		v->spl_nright = posT;
		v->spl_ldatum = PointerGetDatum(unionB);
		v->spl_rdatum = PointerGetDatum(unionT);

#ifdef DEBUG_GIST6
elog(NOTICE,"   split direction was '%c'", direction);
elog(NOTICE,"   posB = %i, posT=%i", posB,posT);
elog(NOTICE,"   posB's (nleft) offset numbers:");
{
	char		aaa[5000],bbb[100];
	aaa[0] =0;
for (i=0;i<posB;i++)
{
	sprintf(bbb," %i", listB[i]);
	strcat(aaa,bbb);
}
elog(NOTICE,aaa);
aaa[0]=0;
elog(NOTICE,"   posT's (nright) offset numbers:");
for (i=0;i<posT;i++)
{
	sprintf(bbb," %i", listT[i]);
	strcat(aaa,bbb);
}
elog(NOTICE,aaa);
}
#endif


	}
	PG_RETURN_POINTER(v);
}

Datum report_lwgeom_gist_activity(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(report_lwgeom_gist_activity);
Datum report_lwgeom_gist_activity(PG_FUNCTION_ARGS)
{
	elog(NOTICE,"lwgeom gist activity - internal consistency= %i, leaf consistency = %i",counter_intern,counter_leaf);
	PG_RETURN_NULL();
}



