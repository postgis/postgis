
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * $Log$
 * Revision 1.7  2003/11/11 10:14:57  strk
 * Added support for PG74
 *
 * Revision 1.6  2003/07/25 17:08:37  pramsey
 * Moved Cygwin endian define out of source files into postgis.h common
 * header file.
 *
 * Revision 1.5  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/

 // If you're modifiying this file you should read the postgis mail list as it has
 // detailed descriptions of whats happening here and why.

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

//--------------------------------------------------------------------------

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_statistic.h"
#include "catalog/pg_type.h"
#include "mb/pg_wchar.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/plancat.h"
#include "optimizer/prep.h"
#include "parser/parse_func.h"
#include "parser/parse_oper.h"
#include "parser/parsetree.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/int8.h"
#include "utils/lsyscache.h"
#include "utils/selfuncs.h"
#include "utils/syscache.h"



#include "executor/spi.h"


//estimate_histogram2d(histogram2d, box)
// returns a % estimate of the # of features that will be returned by that box query
//
//For each grid cell that intersects the query box
//	  Calculate area of intersection (AOI)
//    IF AOI < avgFeatureArea THEN set AOI = avgFeatureArea
//    SUM AOI/area-of-cell*value-of-cell
//
// change : instead of avgFeatureArea, use avgFeatureArea or 10% of a grid cell (whichever is smaller)

PG_FUNCTION_INFO_V1(estimate_histogram2d);
Datum estimate_histogram2d(PG_FUNCTION_ARGS)
{
	HISTOGRAM2D   *histo = (HISTOGRAM2D *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX			  *box = (BOX *) PG_GETARG_POINTER(1);
	double box_area;
	int			x_idx_min, x_idx_max, y_idx_min, y_idx_max;
	double intersect_x, intersect_y,AOI;
	int x,y;
	double xmin,ymin,xmax,ymax;
	int32 result_sum;
	double cell_area;
	int total,t;
	double 	avg_feature_size;



	result_sum = 0;
    xmin = histo->xmin;
    ymin = histo->ymin;
    xmax = histo->xmax;
    ymax = histo->ymax;

			cell_area = ( (xmax-xmin)*(ymax-ymin)/(histo->boxesPerSide*histo->boxesPerSide) );

			avg_feature_size = histo->avgFeatureArea;
			if ( avg_feature_size > cell_area*0.1)
			{
				avg_feature_size = cell_area*0.1;
			}


//elog(NOTICE,"start estimate_histogram2d: ");
//elog(NOTICE,"box is : (%.15g,%.15g to %.15g,%.15g)",box->low.x,box->low.y, box->high.x, box->high.y);

					box_area = (box->high.x-box->low.x)*(box->high.y-box->low.y);

					if (box_area<0)
					    box_area =0;  // for precision!

					//check to see which boxes this intersects
					x_idx_min = (box->low.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
					if (x_idx_min <0)
						x_idx_min = 0;
					if (x_idx_min >= histo->boxesPerSide)
						x_idx_min = histo->boxesPerSide-1;
					y_idx_min = (box->low.y-ymin)/(ymax-ymin)*histo->boxesPerSide;
					if (y_idx_min <0)
						y_idx_min = 0;
					if (y_idx_min >= histo->boxesPerSide)
						y_idx_min = histo->boxesPerSide-1;

					x_idx_max = (box->high.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
					if (x_idx_max <0)
						x_idx_max = 0;
					if (x_idx_max >= histo->boxesPerSide)
						x_idx_max = histo->boxesPerSide-1;
					y_idx_max = (box->high.y-ymin)/(ymax-ymin)*histo->boxesPerSide ;
					if (y_idx_max <0)
						y_idx_max = 0;
					if (y_idx_max >= histo->boxesPerSide)
						y_idx_max = histo->boxesPerSide-1;

					//the {x,y}_idx_{min,max} define the grid squares that the box intersects


//elog(NOTICE,"        search is in x: %i to %i   y: %i to %i",x_idx_min, x_idx_max, y_idx_min,y_idx_max);
					for (y= y_idx_min; y<=y_idx_max;y++)
					{
						for (x=x_idx_min;x<= x_idx_max;x++)
						{
								intersect_x = min(box->high.x, xmin+ (x+1) * (xmax-xmin)/histo->boxesPerSide ) -
											max(box->low.x, xmin+ x*(xmax-xmin)/histo->boxesPerSide ) ;
								intersect_y = min(box->high.y, ymin+ (y+1) * (ymax-ymin)/histo->boxesPerSide ) -
											max(box->low.y, ymin+ y*(ymax-ymin)/histo->boxesPerSide ) ;

									// for a point, intersect_x=0, intersect_y=0, box_area =0
//elog(NOTICE,"x=%i,y=%i, intersect_x= %.15g, intersect_y = %.15g",x,y,intersect_x,intersect_y);
								if ( (intersect_x>=0) && (intersect_y>=0) )
								{
									AOI = intersect_x*intersect_y;
									if (AOI< avg_feature_size)
											AOI = avg_feature_size;
									result_sum += AOI/cell_area* histo->value[x+y*histo->boxesPerSide];
								}
						}
					}
			total = 0;
			for(t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
			{
				total+=histo->value[t];
			}

			if ( (histo->avgFeatureArea <=0) && (box_area <=0) )
				PG_RETURN_FLOAT8(1.0/((double)(total)));
			else
				PG_RETURN_FLOAT8(result_sum/((double)total));

}

//explode_histogram2d(histogram2d, tablename::text)
// executes CREATE TABLE tablename (the_geom geometry, id int, hits int, percent float)
// then populates it
//  DOES NOT UPDATE GEOMETRY_COLUMNS
PG_FUNCTION_INFO_V1(explode_histogram2d);
Datum explode_histogram2d(PG_FUNCTION_ARGS)
{
	HISTOGRAM2D   *histo = (HISTOGRAM2D *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char	*tablename;
	char	sql[1000];
	char	geom[1000];
	int t;
    int total;
	double cellx,celly;
	int x,y;
	int SPIcode;


	cellx = (histo->xmax-histo->xmin)/histo->boxesPerSide;
	celly = (histo->ymax-histo->ymin)/histo->boxesPerSide;

	tablename  = DatumGetCString(DirectFunctionCall1(textout,
	                PointerGetDatum(PG_GETARG_DATUM(1))));

	total = 0;
	for(t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
	{
		total+=histo->value[t];
	}
	if (total==0)
		total=1;

		SPIcode = SPI_connect();
		if (SPIcode  != SPI_OK_CONNECT)
		{
			elog(ERROR,"build_histogram2d: couldnt open a connection to SPI");
			PG_RETURN_NULL() ;

		}

		sprintf(sql,"CREATE TABLE %s (the_geom geometry, id int, hits int, percent float)",tablename);

		SPIcode = SPI_exec(sql, 2147483640 ); // max signed int32

			if (SPIcode  != SPI_OK_UTILITY )
			{
					elog(ERROR,"explode_histogram2d: couldnt create table");
					PG_RETURN_NULL() ;
			}
			t=0;
			for(y=0;y<histo->boxesPerSide;y++)
			{
					for(x=0;x<histo->boxesPerSide;x++)
					{

						sprintf(geom,"POLYGON((%.15g %.15g, %.15g %.15g, %.15g %.15g, %.15g %.15g, %.15g %.15g ))",
							histo->xmin + x*cellx,     histo->ymin+y*celly,
							histo->xmin + (x)*cellx,   histo->ymin+ (y+1)*celly,
							histo->xmin + (x+1)*cellx, histo->ymin+ (y+1)*celly,
							histo->xmin + (x+1)*cellx, histo->ymin+y*celly,
							histo->xmin + x*cellx,     histo->ymin+y*celly
							);
						sprintf(sql,"INSERT INTO %s VALUES('%s'::geometry,%i,%i,%.15g)",tablename,geom,t,histo->value[t],histo->value[t]/((double)total)*100.0);
//elog(NOTICE,"SQL:%s",sql);
						t++;
						SPIcode = SPI_exec(sql, 2147483640 ); // max signed int32
						if (SPIcode  != SPI_OK_INSERT )
						{
								elog(ERROR,"explode_histogram2d: couldnt insert into");
								PG_RETURN_NULL() ;
						}
					}
			}

	    SPIcode =SPI_finish();
	    if (SPIcode  != SPI_OK_FINISH )
		{
					elog(ERROR,"build_histogram2d: couldnt disconnect from SPI");
					PG_RETURN_NULL() ;
		}

	PG_RETURN_POINTER(histo) ;
}


//build_histogram2d (HISTOGRAM2D, tablename, columnname)
// executes the SPI 'SELECT box3d(columnname) FROM tablename'
// and sticks all the results in the histogram
PG_FUNCTION_INFO_V1(build_histogram2d);
Datum build_histogram2d(PG_FUNCTION_ARGS)
{
	HISTOGRAM2D   *histo = (HISTOGRAM2D *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	char	*tablename, *columnname;
	HISTOGRAM2D   *result;
	int SPIcode;
	char	sql[1000];
	SPITupleTable *tuptable;
	TupleDesc tupdesc ;
	int ntuples,t;
	Datum	datum;
	bool	isnull;
	HeapTuple tuple ;
	BOX	*box;
    double box_area, area_intersect, cell_area;
    int x_idx_min, x_idx_max;
    int y_idx_min, y_idx_max;
    double xmin,ymin, xmax,ymax;
    double intersect_x, intersect_y;
    int x,y;
    int total;
    double sum_area;
    int    sum_area_numb;

  	double sum_area_new      = 0;
    int    sum_area_numb_new =0;
	int bump=0;

	int tuplimit = 500000;	// No. of tuples returned on each cursor fetch
	bool moredata;
	void *SPIplan;
	void *SPIportal;


    xmin = histo->xmin;
    ymin = histo->ymin;
    xmax = histo->xmax;
    ymax = histo->ymax;


	result = (HISTOGRAM2D *) malloc(histo->size);
	memcpy(result,histo,histo->size);


		total = 0;
		for(t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
		{
			total+=histo->value[t];
		}



		sum_area = histo->avgFeatureArea * total;
		sum_area_numb = total;



	    tablename  = DatumGetCString(DirectFunctionCall1(textout,
	                                                PointerGetDatum(PG_GETARG_DATUM(1))));

		columnname = DatumGetCString(DirectFunctionCall1(textout,
	                                                PointerGetDatum(PG_GETARG_DATUM(2))));

	//elog(NOTICE,"Start build_histogram2d with %i items already existing", sum_area_numb);
	//elog(NOTICE,"table=\"%s\", column = \"%s\"", tablename, columnname);


		SPIcode = SPI_connect();

		if (SPIcode  != SPI_OK_CONNECT)
		{
			elog(ERROR,"build_histogram2d: couldnt open a connection to SPI");
			PG_RETURN_NULL() ;

		}


			sprintf(sql,"SELECT box(\"%s\") FROM \"%s\"",columnname,tablename);
			//elog(NOTICE,"executing %s",sql);

			SPIplan = SPI_prepare(sql, 0, NULL);
			if (SPIplan  == NULL)
			{
					elog(ERROR,"build_histogram2d: couldnt create query plan via SPI");
					PG_RETURN_NULL() ;
			}

			SPIportal = SPI_cursor_open(NULL, SPIplan, NULL, NULL);
			if (SPIportal == NULL)
			{
					elog(ERROR,"build_histogram2d: couldn't create cursor via SPI");
					PG_RETURN_NULL() ;
			}

			
			moredata = TRUE;
			while (moredata==TRUE) {

				//elog(NOTICE,"about to fetch...");
				SPI_cursor_fetch(SPIportal, TRUE, tuplimit);

				ntuples = SPI_processed;
				//elog(NOTICE,"processing %d records", ntuples);

				if (ntuples > 0) {

					tuptable = SPI_tuptable;
					tupdesc = SPI_tuptable->tupdesc;

					cell_area = ( (xmax-xmin)*(ymax-ymin)/(histo->boxesPerSide*histo->boxesPerSide) );

					for (t=0;t<ntuples;t++)
					{
						tuple = tuptable->vals[t];
						datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
						if (!(isnull))
						{
							box = (BOX *)DatumGetPointer(datum);
							box_area = (box->high.x-box->low.x)*(box->high.y-box->low.y);

							sum_area_new += box_area;
							sum_area_numb_new ++;

							if (box_area > cell_area )
								box_area = cell_area;
							if (box_area<0)
								box_area =0;  // for precision!

							//check to see which boxes this intersects
							x_idx_min = (box->low.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
							if (x_idx_min <0)
								x_idx_min = 0;
							if (x_idx_min >= histo->boxesPerSide)
								x_idx_min = histo->boxesPerSide-1;
							y_idx_min = (box->low.y-ymin)/(ymax-ymin)*histo->boxesPerSide;
							if (y_idx_min <0)
								y_idx_min = 0;
							if (y_idx_min >= histo->boxesPerSide)
								y_idx_min = histo->boxesPerSide-1;

							x_idx_max = (box->high.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
							if (x_idx_max <0)
								x_idx_max = 0;
							if (x_idx_max >= histo->boxesPerSide)
								x_idx_max = histo->boxesPerSide-1;
							y_idx_max = (box->high.y-ymin)/(ymax-ymin)*histo->boxesPerSide ;
							if (y_idx_max <0)
								y_idx_max = 0;
							if (y_idx_max >= histo->boxesPerSide)
								y_idx_max = histo->boxesPerSide-1;

							//the {x,y}_idx_{min,max} define the grid squares that the box intersects
							// if the area of the intersect between the box and the grid square > 5% of

			//elog(NOTICE,"box is : (%.15g,%.15g to %.15g,%.15g)",box->low.x,box->low.y, box->high.x, box->high.y);
		//elog(NOTICE,"        search is in x: %i to %i   y: %i to %i",x_idx_min, x_idx_max, y_idx_min,y_idx_max);
							for (y= y_idx_min; y<=y_idx_max;y++)
							{
								for (x=x_idx_min;x<= x_idx_max;x++)
								{
										intersect_x = min(box->high.x, xmin+ (x+1) * (xmax-xmin)/histo->boxesPerSide ) -
													max(box->low.x, xmin+ x*(xmax-xmin)/histo->boxesPerSide ) ;
										intersect_y = min(box->high.y, ymin+ (y+1) * (ymax-ymin)/histo->boxesPerSide ) -
													max(box->low.y, ymin+ y*(ymax-ymin)/histo->boxesPerSide ) ;

											// for a point, intersect_x=0, intersect_y=0, box_area =0
		//elog(NOTICE,"x=%i,y=%i, intersect_x= %.15g, intersect_y = %.15g",x,y,intersect_x,intersect_y);
										if ( (intersect_x>=0) && (intersect_y>=0) )
										{
											area_intersect = intersect_x*intersect_y;
											if (area_intersect >= box_area*0.05)
											{
		//elog(NOTICE,"bump");
												bump++;
												result->value[x+y*histo->boxesPerSide]++;
											}
										}
								}
							} // End of y

						} // End isnull

					} // End of for loop

					// Free all the results after each fetch, otherwise all tuples stay
					// in memory until the end of the table...
					SPI_freetuptable(tuptable);

				} else {
						moredata = FALSE;
				} // End of if ntuples > 0

			} // End of while loop


		// Close the cursor
		SPI_cursor_close(SPIportal);

	    SPIcode =SPI_finish();
	    if (SPIcode  != SPI_OK_FINISH )
		{
					elog(ERROR,"build_histogram2d: couldnt disconnect from SPI");
					PG_RETURN_NULL() ;
		}

		//elog(NOTICE,"finishing up build_histogram2d ");

	//pfree(tablename);
	//pfree(columnname);

	total = 0;
	for(t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
	{
		total+=result->value[t];
	}
	//elog(NOTICE ,"histogram finishes with %i items in it - acutally added %i rows and %i bumps\n",total,sum_area_numb_new,bump);
	//elog(NOTICE,"done build_histogram2d ");


	//re-calculate statistics on avg bbox size
	if (sum_area_numb_new >0)
		result->avgFeatureArea = (sum_area_new+sum_area)/((double)(sum_area_numb_new+sum_area_numb));

	PG_RETURN_POINTER(result) ;
}





/*
 * get_restriction_var
 *		Examine the args of a restriction clause to see if it's of the
 *		form (var op something) or (something op var).	If so, extract
 *		and return the var and the other argument.
 *
 * Inputs:
 *	args: clause argument list
 *	varRelid: see specs for restriction selectivity functions
 *
 * Outputs: (these are set only if TRUE is returned)
 *	*var: gets Var node
 *	*other: gets other clause argument
 *	*varonleft: set TRUE if var is on the left, FALSE if on the right
 *
 * Returns TRUE if a Var is identified, otherwise FALSE.
 */
static bool
get_restriction_var(List *args,
					int varRelid,
					Var **var,
					Node **other,
					bool *varonleft)
{
	Node	   *left,
			   *right;

	if (length(args) != 2)
		return false;

	left = (Node *) lfirst(args);
	right = (Node *) lsecond(args);

	/* Ignore any binary-compatible relabeling */

	if (IsA(left, RelabelType))
		left = (Node *)((RelabelType *) left)->arg;
	if (IsA(right, RelabelType))
		right = (Node *)((RelabelType *) right)->arg;

	/* Look for the var */

	if (IsA(left, Var) &&
		(varRelid == 0 || varRelid == ((Var *) left)->varno))
	{
		*var = (Var *) left;
		*other = right;
		*varonleft = true;
	}
	else if (IsA(right, Var) &&
			 (varRelid == 0 || varRelid == ((Var *) right)->varno))
	{
		*var = (Var *) right;
		*other = left;
		*varonleft = false;
	}
	else
	{
		/* Duh, it's too complicated for me... */
		return false;
	}

	return true;
}




//restriction in the GiST && operator
PG_FUNCTION_INFO_V1(postgis_gist_sel);
Datum postgis_gist_sel(PG_FUNCTION_ARGS)
{
		Query	   *root = (Query *) PG_GETARG_POINTER(0);
	//	Oid			operator = PG_GETARG_OID(1);
		List	   *args = (List *) PG_GETARG_POINTER(2);
		int			varRelid = PG_GETARG_INT32(3);
		GEOMETRY	*in;
		BOX			*search_box;
		char		sql[1000];

		SPITupleTable *tuptable;
		TupleDesc tupdesc ;
		HeapTuple tuple ;

		Datum datum;
		bool isnull;


		Var	*var;
		Node	*other;
		bool varonleft;
		Oid relid;
		int SPIcode;

		double myest;

#ifndef USE_STATS
	PG_RETURN_FLOAT8(0.000005);
#endif

		  //PG_RETURN_FLOAT8(0.000005);

		//elog(NOTICE,"postgis_gist_sel was called");

		if (!get_restriction_var(args, varRelid,
							 &var, &other, &varonleft))
		{
			//elog(NOTICE,"get_restriction_var FAILED -returning early");
			PG_RETURN_FLOAT8(0.000005);
		}

		relid = getrelid(var->varno, root->rtable);
		if (relid == InvalidOid)
		{
			//elog(NOTICE,"getrelid FAILED (invalid oid) -returning early");
			PG_RETURN_FLOAT8(0.000005);
		}

		//elog(NOTICE,"operator's oid = %i (this should be GEOMETRY && GEOMETRY)",operator);
		//elog(NOTICE,"relations' oid = %i (this should be the relation that the && is working on) ",relid);
		//elog(NOTICE,"varatt oid = %i (basically relations column #) ",var->varattno);


		if (IsA(other, Const) &&((Const *) other)->constisnull)
		{
			//elog(NOTICE,"other operand of && is NULL - returning early");
			PG_RETURN_FLOAT8(0.000005);
		}

		if (IsA(other, Const))
		{
			//elog(NOTICE,"The other side of the && is a constant with type (oid) = %i and length %i.  This should be GEOMETRY with length -1 (variable length)",((Const*)other)->consttype,((Const*)other)->constlen);

		}
		else
		{
			//elog(NOTICE,"the other side of && isnt a constant - returning early");
			PG_RETURN_FLOAT8(0.000005);
		}

		//get the BOX thats being searched in
		in = (GEOMETRY*)PG_DETOAST_DATUM( ((Const*)other)->constvalue );
		search_box = convert_box3d_to_box(&in->bvol);

		//elog(NOTICE,"requested search box is : (%.15g %.15g, %.15g %.15g)",search_box->low.x,search_box->low.y,search_box->high.x,search_box->high.y);


			SPIcode = SPI_connect();
			if (SPIcode  != SPI_OK_CONNECT)
			{
				elog(NOTICE,"postgis_gist_sel: couldnt open a connection to SPI:%i",SPIcode);
				PG_RETURN_FLOAT8(0.000005) ;
			}

			sprintf(sql,"SELECT stats FROM GEOMETRY_COLUMNS WHERE attrelid=%u AND varattnum=%i",relid,var->varattno);
			//elog(NOTICE,"sql:%s",sql);
			SPIcode = SPI_exec(sql, 1 );
			if (SPIcode  != SPI_OK_SELECT )
			{
				SPI_finish();
				elog(NOTICE,"postgis_gist_sel: couldnt execute sql via SPI");
				PG_RETURN_FLOAT8(0.000005) ;
			}

			if (SPI_processed !=1)
			{
				SPI_finish();
				//elog(NOTICE,"postgis_gist_sel: geometry_columns didnt return a unique value");
				PG_RETURN_FLOAT8(0.000005) ;
			}

			tuptable = SPI_tuptable;
			tupdesc = SPI_tuptable->tupdesc;
			tuple = tuptable->vals[0];
			datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
			if (isnull)
			{
				SPI_finish();
				//elog(NOTICE,"postgis_gist_sel: geometry_columns returned a null histogram");
				PG_RETURN_FLOAT8(0.000005) ;
			}
//elog(NOTICE,"postgis_gist_sel: checking against estimate_histogram2d");
			// now we have the histogram, and our search box - use the estimate_histogram2d(histo,box) to get the result!
			myest =
					DatumGetFloat8( DirectFunctionCall2( estimate_histogram2d, datum, PointerGetDatum(search_box) ) );

			if ( (myest<0) || (myest!=myest) ) // <0?  or NaN?
			{
				//elog(NOTICE,"postgis_gist_sel: got something crazy back from estimate_histogram2d");
				PG_RETURN_FLOAT8(0.000005) ;
			}




			SPIcode =SPI_finish();
			if (SPIcode  != SPI_OK_FINISH )
			{
				//elog(NOTICE,"postgis_gist_sel: couldnt disconnect from SPI");
				PG_RETURN_FLOAT8(0.000005) ;
			}
//elog(NOTICE,"postgis_gist_sel: finished, returning with %lf",myest);
        PG_RETURN_FLOAT8(myest);
}

static void
genericcostestimate2(Query *root, RelOptInfo *rel,
					IndexOptInfo *index, List *indexQuals,
					Cost *indexStartupCost,
					Cost *indexTotalCost,
					Selectivity *indexSelectivity,
					double *indexCorrelation)
{
	double		numIndexTuples;
	double		numIndexPages;
	List		*selectivityQuals = indexQuals;
#if USE_VERSION >= 74
	QualCost	index_qual_cost;
#endif


//elog(NOTICE,"in genericcostestimate");


	/*
	 * If the index is partial, AND the index predicate with the
	 * explicitly given indexquals to produce a more accurate idea of the
	 * index restriction.  This may produce redundant clauses, which we
	 * hope that cnfify and clauselist_selectivity will deal with
	 * intelligently.
	 *
	 * Note that index->indpred and indexQuals are both in implicit-AND form
	 * to start with, which we have to make explicit to hand to
	 * canonicalize_qual, and then we get back implicit-AND form again.
	 */
	if (index->indpred != NIL)
	{
		Expr	   *andedQuals;

		andedQuals = make_ands_explicit(nconc(listCopy(index->indpred),
											  indexQuals));
		selectivityQuals = canonicalize_qual(andedQuals, true);
	}



	/* Estimate the fraction of main-table tuples that will be visited */
	*indexSelectivity = clauselist_selectivity(root, selectivityQuals,
#if USE_VERSION < 74
		lfirsti(rel->relids));
#else
		rel->relid, JOIN_INNER);
#endif

	/*
	 * Estimate the number of tuples that will be visited.	We do it in
	 * this rather peculiar-looking way in order to get the right answer
	 * for partial indexes.  We can bound the number of tuples by the
	 * index size, in any case.
	 */
	numIndexTuples = *indexSelectivity * rel->tuples;

	//elog(NOTICE,"relation has %li pages",rel->pages);
	//elog(NOTICE,"indexselectivity is %lf, ntuples = %lf, numindexTuples = %lf, index->tuples = %lf",*indexSelectivity, rel->tuples, numIndexTuples,index->tuples);


	if (numIndexTuples > index->tuples)
		numIndexTuples = index->tuples;

	/*
	 * Always estimate at least one tuple is touched, even when
	 * indexSelectivity estimate is tiny.
	 */
	if (numIndexTuples < 1.0)
		numIndexTuples = 1.0;

	/*
	 * Estimate the number of index pages that will be retrieved.
	 *
	 * For all currently-supported index types, the first page of the index
	 * is a metadata page, and we should figure on fetching that plus a
	 * pro-rated fraction of the remaining pages.
	 */


	 //elog(NOTICE,"index->pages =  %li ",index->pages);

	if (index->pages > 1 && index->tuples > 0)
	{
		numIndexPages = (numIndexTuples / index->tuples) * (index->pages - 1);
		numIndexPages += 1;		/* count the metapage too */
		numIndexPages = ceil(numIndexPages);
	}
	else
		numIndexPages = 1.0;


//elog(NOTICE,"numIndexPages =  %lf ",numIndexPages);
	/*
	 * Compute the index access cost.
	 *
	 * Our generic assumption is that the index pages will be read
	 * sequentially, so they have cost 1.0 each, not random_page_cost.
	 * Also, we charge for evaluation of the indexquals at each index
	 * tuple. All the costs are assumed to be paid incrementally during
	 * the scan.
	 */

#if USE_VERSION < 74
	*indexStartupCost = 0;
	*indexTotalCost = numIndexPages +
		(cpu_index_tuple_cost + cost_qual_eval(indexQuals)) *
		numIndexTuples;
#else
	cost_qual_eval(&index_qual_cost, indexQuals);
	*indexStartupCost = index_qual_cost.startup;
	*indexTotalCost = numIndexPages +
		(cpu_index_tuple_cost + index_qual_cost.per_tuple) *
		numIndexTuples;
#endif


	//elog(NOTICE,"cpu_index_tuple_cost = %lf, cost_qual_eval(indexQuals)) = %lf",
	//	cpu_index_tuple_cost,cost_qual_eval(indexQuals));

	//elog(NOTICE,"indexTotalCost =  %lf ",*indexTotalCost);

	/*
	 * Generic assumption about index correlation: there isn't any.
	 */
	*indexCorrelation = 0.97;
	//elog(NOTICE,"indexcorrelation =  %lf ",*indexCorrelation);
}


PG_FUNCTION_INFO_V1(postgisgistcostestimate);
Datum
postgisgistcostestimate(PG_FUNCTION_ARGS)
{
    Query      *root = (Query *) PG_GETARG_POINTER(0);
    RelOptInfo *rel = (RelOptInfo *) PG_GETARG_POINTER(1);
    IndexOptInfo *index = (IndexOptInfo *) PG_GETARG_POINTER(2);
    List       *indexQuals = (List *) PG_GETARG_POINTER(3);
    Cost       *indexStartupCost = (Cost *) PG_GETARG_POINTER(4);
    Cost       *indexTotalCost = (Cost *) PG_GETARG_POINTER(5);
    Selectivity *indexSelectivity = (Selectivity *) PG_GETARG_POINTER(6);
    double     *indexCorrelation = (double *) PG_GETARG_POINTER(7);

//elog(NOTICE,"postgisgistcostestimate was called");

    genericcostestimate2(root, rel, index, indexQuals,
                        indexStartupCost, indexTotalCost,
                        indexSelectivity, indexCorrelation);
//elog(NOTICE,"postgisgistcostestimate is going to return void");

    PG_RETURN_VOID();
}


