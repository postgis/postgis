/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * $Log$
 * Revision 1.10  2004/10/25 17:07:09  strk
 * Obsoleted getbox2d(). Use getbox2d_p() or getbox2d_internal() instead.
 *
 * Revision 1.9  2004/10/08 13:20:54  strk
 *
 * Changed LWGEOM structure to point to an actual BOX2DFLOAT4.
 * Renamed most function to reflect a TYPE_method naming convention.
 * (you'll need a dump/reload for it to work)
 * Added more manipulation functions.
 *
 * Revision 1.8  2004/10/05 21:54:48  strk
 * Yes another change in SPI_cursor_open
 *
 * Revision 1.7  2004/09/29 10:50:30  strk
 * Big layout change.
 * lwgeom.h is public API
 * liblwgeom.h is private header
 * lwgeom_pg.h is for PG-links
 * lw<type>.c contains type-specific functions
 *
 * Revision 1.6  2004/09/27 08:26:03  strk
 * Debugging defines set to NODEBUG.
 *
 * Revision 1.5  2004/09/16 09:06:12  strk
 * Changed SPI_cursor_open call changes to be used for USE_VERSION > 80
 * (change seems to be intended for future releases)
 *
 * Revision 1.4  2004/09/14 07:43:00  strk
 * Updated call to SPI_cursor_open to 8.0 (beta2) interface.
 *
 * Revision 1.3  2004/08/19 13:54:15  strk
 * cpp checks updated to use 80 instead of 75 for USE_VERSION
 *
 * Revision 1.2  2004/08/19 13:18:01  strk
 * Added selectivity estimation
 *
 **********************************************************************/

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "postgres.h"
#include "utils/geo_decls.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "parser/parsetree.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

#if USE_VERSION >= 80

#include "commands/vacuum.h"
#include "utils/lsyscache.h"

/*
 * 	Assign a number to the postgis statistics kind
 *
 * 	tgl suggested:
 *
 * 	1-100:	reserved for assignment by the core Postgres project
 * 	100-199: reserved for assignment by PostGIS
 * 	200-9999: reserved for other globally-known stats kinds
 * 	10000-32767: reserved for private site-local use
 *
 */
#define STATISTIC_KIND_GEOMETRY 100

//#define DEBUG_GEOMETRY_STATS 1

/*
 * Define this if you want to use standard deviation based
 * histogram extent computation. If you do, you can also 
 * tweak the deviation factor used in computation with
 * SDFACTOR.
 */
#define USE_STANDARD_DEVIATION 1
#define SDFACTOR 2

typedef struct GEOM_STATS_T
{
	// cols * rows = total boxes in grid
	float4 cols;
	float4 rows;
	
	// average bounding box area of not-null features 
	float4 avgFeatureArea;

	// average number of histogram cells
	// covered by the sample not-null features
	float4 avgFeatureCells;

	// BOX of area
	float4 xmin,ymin, xmax, ymax;
	// variable length # of floats for histogram
	float4 value[1];
} GEOM_STATS;

#endif

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/*
 * Default geometry selectivity factor
 */
#define DEFAULT_GEOMETRY_SEL 0.000005 

#define min(a,b)  ((a) <= (b) ? (a) : (b))
#define max(a,b)  ((a) >  (b) ? (a) : (b))

// --------------------------------------------
// lwhistogram2d type

// 2d histogram is a bounding box with a bunch of cells in it.
// The cells will have width (xmax-xmin)/boxesPerSide
//                 and height(ymax-ymin)/boxesPerSide
// The first box is the ll corner's box, the send is directly to the right
//   (row-major).
//
//  Size of structure is:
//        4 (size) + 32 (box) + 4 (boxesPerSide) +
//		    boxesPerSide*boxesPerSide*4 (data)
typedef struct histotag
{
	int32 size;	  // postgres variable-length type requirement
	int boxesPerSide; // boxesPerSide * boxesPerSide = total boxes in grid
	double avgFeatureArea; // average bbox area of features in this histogram
	double xmin,ymin, xmax, ymax; // BOX of area
	unsigned int value[1]; // variable length # of ints for histogram
} LWHISTOGRAM2D;

Datum lwhistogram2d_in(PG_FUNCTION_ARGS);
Datum lwhistogram2d_out(PG_FUNCTION_ARGS);
Datum create_lwhistogram2d(PG_FUNCTION_ARGS);
Datum build_lwhistogram2d(PG_FUNCTION_ARGS);
Datum explode_lwhistogram2d(PG_FUNCTION_ARGS);
Datum estimate_lwhistogram2d(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_sel(PG_FUNCTION_ARGS);
#if USE_VERSION >= 80
Datum LWGEOM_analyze(PG_FUNCTION_ARGS);
#endif

//text form of LWHISTOGRAM2D is:
// 'HISTOGRAM2D(xmin,ymin,xmax,ymax,boxesPerSide;value[0],value[1],...')
//    note the ";" in the middle (for easy parsing)
//  I dont expect anyone to actually create one by hand
PG_FUNCTION_INFO_V1(lwhistogram2d_in);
Datum lwhistogram2d_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	LWHISTOGRAM2D *histo ;
	int nitems;
	double xmin,ymin,xmax,ymax;
	int boxesPerSide;
	double avgFeatureArea;
	char *str2,*str3;
	long datum;

	//elog(NOTICE, "lwhistogram2d parser called");

	int t;

	while (isspace((unsigned char) *str))
		str++;

	if (strstr(str,"HISTOGRAM2D(") != str)
	{
		elog(ERROR, "lwhistogram2d parser - doesnt start with 'HISTOGRAM2D(\n");
		PG_RETURN_NULL() ;
	}
	if (strstr(str,";") == NULL)
	{
		elog(ERROR, "lwhistogram2d parser - doesnt have a ; in sring!\n");
		PG_RETURN_NULL() ;
	}

	nitems = sscanf(str,"HISTOGRAM2D(%lf,%lf,%lf,%lf,%i,%lf;",&xmin,&ymin,&xmax,&ymax,&boxesPerSide,&avgFeatureArea);

	if (nitems != 6)
	{
		elog(ERROR, "lwhistogram2d parser - couldnt parse initial portion of histogram!\n");
		PG_RETURN_NULL() ;
	}

	if  ( (boxesPerSide > 50) || (boxesPerSide <1) )
	{
		elog(ERROR, "lwhistogram2d parser - boxesPerSide is too big or too small\n");
		PG_RETURN_NULL() ;
	}

	str2 = strstr(str,";");
	str2++;

	if (str2[0] ==0)
	{
		elog(ERROR, "lwhistogram2d parser - no histogram values\n");
		PG_RETURN_NULL() ;
	}

	histo = (LWHISTOGRAM2D *) palloc (sizeof(LWHISTOGRAM2D) + (boxesPerSide*boxesPerSide-1)*4 );
	histo->size = sizeof(LWHISTOGRAM2D) + (boxesPerSide*boxesPerSide-1)*4;

	for (t=0;t<boxesPerSide*boxesPerSide;t++)
	{
		datum = strtol(str2,&str3,10); // str2=start of int, str3=end of int, base 10
		// str3 points to "," or ")"
		if (str3[0] ==0)
		{
			elog(ERROR, "lwhistogram2d parser - histogram values prematurely ended!\n");
			PG_RETURN_NULL() ;
		}
		histo->value[t] = (unsigned int) datum;
		str2= str3+1; //move past the "," or ")"
	}
	histo->xmin = xmin;
	histo->xmax = xmax;
	histo->ymin = ymin;
	histo->ymax = ymax;
	histo->avgFeatureArea = avgFeatureArea;
	histo->boxesPerSide = boxesPerSide;

	PG_RETURN_POINTER(histo);
}



//text version
PG_FUNCTION_INFO_V1(lwhistogram2d_out);
Datum lwhistogram2d_out(PG_FUNCTION_ARGS)
{
	LWHISTOGRAM2D *histo = (LWHISTOGRAM2D *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	int t;
	char temp[100];
	int size;

	size = 26+6*MAX_DIGS_DOUBLE + histo->boxesPerSide*histo->boxesPerSide* (MAX_DIGS_DOUBLE+1);
	result = palloc(size);

	//elog(NOTICE, "result@%x", result);

	sprintf(result,"HISTOGRAM2D(%.15g,%.15g,%.15g,%.15g,%i,%.15g;",
		histo->xmin,histo->ymin,histo->xmax,histo->ymax,histo->boxesPerSide,histo->avgFeatureArea );

	//elog(NOTICE,"so far: %s",result);
	//elog(NOTICE,"buffsize=%i, size=%i",size,histo->size);

	for (t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
	{
		if (t) sprintf(temp, ",%u", histo->value[t]);
		else sprintf(temp, "%u", histo->value[t]);
		strcat(result,temp);
	}

	strcat(result,")");
	//elog(NOTICE,"about to return string (len=%i): -%s-",strlen(result),result);
	//elog(NOTICE, "result@%x", result);


	PG_RETURN_CSTRING(result);
}

//create_lwhistogram2d(BOX2D, boxesPerSide)
// returns a histgram with 0s in all the boxes.
PG_FUNCTION_INFO_V1(create_lwhistogram2d);
Datum create_lwhistogram2d(PG_FUNCTION_ARGS)
{
	//BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *bbox = (BOX2DFLOAT4 *)PG_GETARG_DATUM(0);
	int32	boxesPerSide = PG_GETARG_INT32(1);
	LWHISTOGRAM2D *histo;
	int size,t;

	if ( (boxesPerSide <1) || (boxesPerSide >50) )
	{
		elog(ERROR, "create_lwhistogram2d - boxesPerSide is too small or big.\n");
		PG_RETURN_NULL() ;
	}

	size =  sizeof(LWHISTOGRAM2D) + (boxesPerSide*boxesPerSide-1)*4  ;

	histo = (LWHISTOGRAM2D *) palloc(size);
	histo->size = size;

	histo->xmin = bbox->xmin;
	histo->ymin = bbox->ymin;
	histo->xmax = bbox->xmax;
	histo->ymax = bbox->ymax;

	histo->avgFeatureArea = 0;

	histo->boxesPerSide = boxesPerSide;

	for (t=0;t<boxesPerSide*boxesPerSide; t++)
	{
		histo->value[t] = 0;
	}

	//elog(NOTICE,"create_lwhistogram2d returning");

	PG_RETURN_POINTER(histo);
}

//build_histogram2d (LWHISTOGRAM2D, tablename, columnname)
// executes the SPI 'SELECT box3d(columnname) FROM tablename'
// and sticks all the results in the histogram
PG_FUNCTION_INFO_V1(build_lwhistogram2d);
Datum build_lwhistogram2d(PG_FUNCTION_ARGS)
{
	LWHISTOGRAM2D *histo = (LWHISTOGRAM2D *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *tablename, *columnname;
	LWHISTOGRAM2D *result;
	int SPIcode;
	char	sql[1000];
	SPITupleTable *tuptable;
	TupleDesc tupdesc ;
	int ntuples,t;
	Datum	datum;
	bool	isnull;
	HeapTuple tuple ;
	BOX2DFLOAT4 *box;
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

	//elog(NOTICE,"build_lwhistogram2d called");

	xmin = histo->xmin;
	ymin = histo->ymin;
	xmax = histo->xmax;
	ymax = histo->ymax;


	result = (LWHISTOGRAM2D *) malloc(histo->size);
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


	sprintf(sql,"SELECT box2d(\"%s\") FROM \"%s\"",columnname,tablename);
	//elog(NOTICE,"executing %s",sql);

	SPIplan = SPI_prepare(sql, 0, NULL);
	if (SPIplan  == NULL)
	{
		elog(ERROR,"build_histogram2d: couldnt create query plan via SPI");
		PG_RETURN_NULL() ;
	}

#if USE_VERSION >= 80
	SPIportal = SPI_cursor_open(NULL, SPIplan, NULL, NULL, 1);
#else
	SPIportal = SPI_cursor_open(NULL, SPIplan, NULL, NULL);
#endif
	if (SPIportal == NULL)
	{
		elog(ERROR,"build_histogram2d: couldn't create cursor via SPI");
		PG_RETURN_NULL() ;
	}

		
	moredata = TRUE;
	while (moredata==TRUE)
	{

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
					box = (BOX2DFLOAT4 *)DatumGetPointer(datum);
					//box_area = (box->high.x-box->low.x)*(box->high.y-box->low.y);
					box_area = (box->xmax-box->xmin)*(box->ymax-box->ymin);

					sum_area_new += box_area;
					sum_area_numb_new ++;

					if (box_area > cell_area )
						box_area = cell_area;
					if (box_area<0)
						box_area =0;  // for precision!

					//check to see which boxes this intersects
					//x_idx_min = (box->low->x-xmin)/(xmax-xmin)*histo->boxesPerSide;
					x_idx_min = (box->xmin-xmin)/(xmax-xmin)*histo->boxesPerSide;
					if (x_idx_min <0)
						x_idx_min = 0;
					if (x_idx_min >= histo->boxesPerSide)
						x_idx_min = histo->boxesPerSide-1;
					//y_idx_min = (box->low.y-ymin)/(ymax-ymin)*histo->boxesPerSide;
					y_idx_min = (box->ymin-ymin)/(ymax-ymin)*histo->boxesPerSide;
					if (y_idx_min <0)
						y_idx_min = 0;
					if (y_idx_min >= histo->boxesPerSide)
						y_idx_min = histo->boxesPerSide-1;

					//x_idx_max = (box->high.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
					x_idx_max = (box->xmax-xmin)/(xmax-xmin)*histo->boxesPerSide;
					if (x_idx_max <0)
						x_idx_max = 0;
					if (x_idx_max >= histo->boxesPerSide)
						x_idx_max = histo->boxesPerSide-1;
					//y_idx_max = (box->high.y-ymin)/(ymax-ymin)*histo->boxesPerSide ;
					y_idx_max = (box->ymax-ymin)/(ymax-ymin)*histo->boxesPerSide ;
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
							intersect_x = min(box->xmax, xmin+ (x+1) * (xmax-xmin)/histo->boxesPerSide ) - max(box->xmin, xmin+ x*(xmax-xmin)/histo->boxesPerSide ) ;
							intersect_y = min(box->ymax, ymin+ (y+1) * (ymax-ymin)/histo->boxesPerSide ) - max(box->ymin, ymin+ y*(ymax-ymin)/histo->boxesPerSide ) ;

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

//explode_lwhistogram2d(histogram2d, tablename::text)
// executes CREATE TABLE tablename (the_geom geometry, id int, hits int, percent float)
// then populates it
//  DOES NOT UPDATE GEOMETRY_COLUMNS
PG_FUNCTION_INFO_V1(explode_lwhistogram2d);
Datum explode_lwhistogram2d(PG_FUNCTION_ARGS)
{
	LWHISTOGRAM2D *histo = (LWHISTOGRAM2D *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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

//estimate_histogram2d(histogram2d, box2d)
// returns a % estimate of the # of features that will be returned by that box query
//
//For each grid cell that intersects the query box
//	  Calculate area of intersection (AOI)
//    IF AOI < avgFeatureArea THEN set AOI = avgFeatureArea
//    SUM AOI/area-of-cell*value-of-cell
//
// change : instead of avgFeatureArea, use avgFeatureArea or 10% of a grid cell (whichever is smaller)

PG_FUNCTION_INFO_V1(estimate_lwhistogram2d);
Datum estimate_lwhistogram2d(PG_FUNCTION_ARGS)
{
	LWHISTOGRAM2D *histo = (LWHISTOGRAM2D *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 *box = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
	double box_area;
	int x_idx_min, x_idx_max, y_idx_min, y_idx_max;
	double intersect_x, intersect_y, AOI;
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

	//box_area = (box->high.x-box->low.x)*(box->high.y-box->low.y);
	box_area = (box->xmax-box->xmin)*(box->ymax-box->ymin);

	if (box_area<0) box_area = 0;  // for precision!

	//check to see which boxes this intersects
	//x_idx_min = (box->low.x-xmin)/(xmax-xmin)*histo->boxesPerSide;
	x_idx_min = (box->xmin-xmin)/(xmax-xmin)*histo->boxesPerSide;
	if (x_idx_min <0) x_idx_min = 0;
	if (x_idx_min >= histo->boxesPerSide)
		x_idx_min = histo->boxesPerSide-1;
	y_idx_min = (box->ymin-ymin)/(ymax-ymin)*histo->boxesPerSide;
	if (y_idx_min <0) y_idx_min = 0;
	if (y_idx_min >= histo->boxesPerSide)
		y_idx_min = histo->boxesPerSide-1;

	x_idx_max = (box->xmax-xmin)/(xmax-xmin)*histo->boxesPerSide;
	if (x_idx_max <0) x_idx_max = 0;
	if (x_idx_max >= histo->boxesPerSide)
		x_idx_max = histo->boxesPerSide-1;
	y_idx_max = (box->ymax-ymin)/(ymax-ymin)*histo->boxesPerSide ;
	if (y_idx_max <0) y_idx_max = 0;
	if (y_idx_max >= histo->boxesPerSide)
		y_idx_max = histo->boxesPerSide-1;

	//the {x,y}_idx_{min,max} define the grid squares that the box intersects


//elog(NOTICE," search is in x: %i to %i   y: %i to %i",x_idx_min, x_idx_max, y_idx_min,y_idx_max);
	for (y= y_idx_min; y<=y_idx_max;y++)
	{
		for (x=x_idx_min;x<= x_idx_max;x++)
		{
			//intersect_x = min(box->high.x, xmin+ (x+1) * (xmax-xmin)/histo->boxesPerSide ) - max(box->low.x, xmin+ x*(xmax-xmin)/histo->boxesPerSide ) ;
			intersect_x = min(box->xmax, xmin+ (x+1) * (xmax-xmin)/histo->boxesPerSide ) - max(box->xmin, xmin+ x*(xmax-xmin)/histo->boxesPerSide ) ;
			//intersect_y = min(box->high.y, ymin+ (y+1) * (ymax-ymin)/histo->boxesPerSide ) - max(box->low.y, ymin+ y*(ymax-ymin)/histo->boxesPerSide ) ;
			intersect_y = min(box->ymax, ymin+ (y+1) * (ymax-ymin)/histo->boxesPerSide ) - max(box->ymin, ymin+ y*(ymax-ymin)/histo->boxesPerSide ) ;

// for a point, intersect_x=0, intersect_y=0, box_area =0
//elog(NOTICE,"x=%i,y=%i, intersect_x= %.15g, intersect_y = %.15g",x,y,intersect_x,intersect_y);
			if ( (intersect_x>=0) && (intersect_y>=0) )
			{
				AOI = intersect_x*intersect_y;
				if (AOI< avg_feature_size)
						AOI = avg_feature_size;
				result_sum += AOI/cell_area *
					histo->value[x+y*histo->boxesPerSide];
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


/**************************** FROM POSTGIS ****************/


#if USE_VERSION < 80
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
get_restriction_var(List *args, int varRelid, Var **var,
	Node **other, bool *varonleft)
{
	Node *left, *right;

	if (length(args) != 2) return false;

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
PG_FUNCTION_INFO_V1(LWGEOM_gist_sel);
Datum LWGEOM_gist_sel(PG_FUNCTION_ARGS)
{
	Query *root = (Query *) PG_GETARG_POINTER(0);
	List *args = (List *) PG_GETARG_POINTER(2);
	int varRelid = PG_GETARG_INT32(3);
	char *in;
	BOX2DFLOAT4 search_box;
	char sql[1000];

	SPITupleTable *tuptable;
	TupleDesc tupdesc ;
	HeapTuple tuple ;

	Datum datum;
	bool isnull;

	Var *var;
	Node *other;
	bool varonleft;
	Oid relid;
	int SPIcode;

	double myest;

#ifndef USE_STATS
	PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
#endif

	//elog(NOTICE,"LWGEOM_gist_sel was called");

	if (!get_restriction_var(args, varRelid, &var, &other, &varonleft))
	{
		//elog(NOTICE,"get_restriction_var FAILED -returning early");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	relid = getrelid(var->varno, root->rtable);
	if (relid == InvalidOid)
	{
		//elog(NOTICE,"getrelid FAILED (invalid oid) -returning early");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	//elog(NOTICE,"operator's oid = %i (this should be GEOMETRY && GEOMETRY)",operator);
	//elog(NOTICE,"relations' oid = %i (this should be the relation that the && is working on) ",relid);
	//elog(NOTICE,"varatt oid = %i (basically relations column #) ",var->varattno);


	if (IsA(other, Const) &&((Const *) other)->constisnull)
	{
		//elog(NOTICE,"other operand of && is NULL - returning early");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	if (IsA(other, Const))
	{
		//elog(NOTICE,"The other side of the && is a constant with type (oid) = %i and length %i.  This should be GEOMETRY with length -1 (variable length)",((Const*)other)->consttype,((Const*)other)->constlen);

	}
	else
	{
		//elog(NOTICE,"the other side of && isnt a constant - returning early");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	//get the BOX thats being searched in
	in = (char *)PG_DETOAST_DATUM( ((Const*)other)->constvalue );

	//search_box = convert_box3d_to_box(&in->bvol);
	search_box = getbox2d(in+4);

	//elog(NOTICE,"requested search box is : (%.15g %.15g, %.15g %.15g)",search_box->xmin,search_box->ymin,search_box->xmax,search_box->ymax);


	SPIcode = SPI_connect();
	if (SPIcode  != SPI_OK_CONNECT)
	{
		elog(NOTICE,"LWGEOM_gist_sel: couldnt open a connection to SPI:%i",SPIcode);
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}

	sprintf(sql,"SELECT stats FROM GEOMETRY_COLUMNS WHERE attrelid=%u AND varattnum=%i",relid,var->varattno);
	//elog(NOTICE,"sql:%s",sql);
	SPIcode = SPI_exec(sql, 1 );
	if (SPIcode  != SPI_OK_SELECT )
	{
		SPI_finish();
		elog(NOTICE,"LWGEOM_gist_sel: couldnt execute sql via SPI");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}

	if (SPI_processed !=1)
	{
		SPI_finish();
		//elog(NOTICE,"LWGEOM_gist_sel: geometry_columns didnt return a unique value");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}

	tuptable = SPI_tuptable;
	tupdesc = SPI_tuptable->tupdesc;
	tuple = tuptable->vals[0];
	datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
	if (isnull)
	{
		SPI_finish();
		//elog(NOTICE,"LWGEOM_gist_sel: geometry_columns returned a null histogram");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}
//elog(NOTICE,"LWGEOM_gist_sel: checking against estimate_histogram2d");

	// now we have the histogram, and our search box - use the estimate_histogram2d(histo,box) to get the result!
	myest = DatumGetFloat8( DirectFunctionCall2( estimate_lwhistogram2d, datum, PointerGetDatum(&search_box) ) );

	if ( (myest<0) || (myest!=myest) ) // <0?  or NaN?
	{
		//elog(NOTICE,"LWGEOM_gist_sel: got something crazy back from estimate_histogram2d");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}

	SPIcode =SPI_finish();
	if (SPIcode  != SPI_OK_FINISH )
	{
		//elog(NOTICE,"LWGEOM_gist_sel: couldnt disconnect from SPI");
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL) ;
	}

//elog(NOTICE,"LWGEOM_gist_sel: finished, returning with %lf",myest);
        PG_RETURN_FLOAT8(myest);
}

#else // USE_VERSION >= 80

/*
 * This function returns an estimate of the selectivity
 * of a search_box looking at data in the GEOM_STATS 
 * structure.
 *
 * TODO: handle box dimension collapses
 */
static float8
estimate_selectivity(BOX2DFLOAT4 *box, GEOM_STATS *geomstats)
{
	int x, y;
	int x_idx_min, x_idx_max, y_idx_min, y_idx_max;
	double intersect_x, intersect_y, AOI;
	double cell_area, box_area;
	double geow, geoh; // width and height of histogram
	int histocols, historows; // histogram grid size
	double value;
	float overlapping_cells;
	float avg_feat_cells;
	double gain;
	float8 selectivity;


	/*
	 * Search box completely miss histogram extent
	 */
	if ( box->xmax < geomstats->xmin ||
		box->xmin > geomstats->xmax ||
		box->ymax < geomstats->ymin ||
		box->ymin > geomstats->ymax )
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box does not overlaps histogram, returning 0");
#endif
		return 0.0;
	}

	/*
	 * Search box completely contains histogram extent
	 */
	if ( box->xmax >= geomstats->xmax &&
		box->xmin <= geomstats->xmin &&
		box->ymax >= geomstats->ymax &&
		box->ymin <= geomstats->ymin )
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box contains histogram, returning 1");
#endif
		return 1.0;
	}

	geow = geomstats->xmax-geomstats->xmin;
	geoh = geomstats->ymax-geomstats->ymin;

	histocols = geomstats->cols;
	historows = geomstats->rows;

	cell_area = (geow*geoh) / (histocols*historows);
	//box_area = (box->high.x-box->low.x)*(box->high.y-box->low.y);
	box_area = (box->xmax-box->xmin)*(box->ymax-box->ymin);
	value = 0;

	/* Find first overlapping column */
	x_idx_min = (box->xmin-geomstats->xmin) / geow * histocols;
	if (x_idx_min < 0) {
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box overlaps %d columns on the left of histogram grid", -x_idx_min);
#endif
		// should increment the value somehow
		x_idx_min = 0;
	}
	if (x_idx_min >= histocols)
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box overlaps %d columns on the right of histogram grid", x_idx_min-histocols+1);
#endif
		// should increment the value somehow
		x_idx_min = histocols-1;
	}

	/* Find first overlapping row */
	y_idx_min = (box->ymin-geomstats->ymin) / geoh * historows;
	if (y_idx_min <0)
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box overlaps %d columns on the bottom of histogram grid", -y_idx_min);
#endif
		// should increment the value somehow
		y_idx_min = 0;
	}
	if (y_idx_min >= historows)
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " search_box overlaps %d columns on the top of histogram grid", y_idx_min-historows+1);
#endif
		// should increment the value somehow
		y_idx_min = historows-1;
	}

	/* Find last overlapping column */
	//x_idx_max = (box->high.x-geomstats->xmin) / geow * histocols;
	x_idx_max = (box->xmax-geomstats->xmin) / geow * histocols;
	if (x_idx_max <0)
	{
		// should increment the value somehow
		x_idx_max = 0;
	}
	if (x_idx_max >= histocols )
	{
		// should increment the value somehow
		x_idx_max = histocols-1;
	}

	/* Find last overlapping row */
	//y_idx_max = (box->high.y-geomstats->ymin) / geoh * historows;
	y_idx_max = (box->ymax-geomstats->ymin) / geoh * historows;
	if (y_idx_max <0)
	{
		// should increment the value somehow
		y_idx_max = 0;
	}
	if (y_idx_max >= historows)
	{
		// should increment the value somehow
		y_idx_max = historows-1;
	}

	/*
	 * the {x,y}_idx_{min,max}
	 * define the grid squares that the box intersects
	 */
	for (y=y_idx_min; y<=y_idx_max; y++)
	{
		for (x=x_idx_min; x<=x_idx_max; x++)
		{
			double val;
			double gain;

			val = geomstats->value[x+y*histocols];

			/*
			 * Of the cell value we get
			 * only the overlap fraction.
			 */

			//intersect_x = min(box->high.x, geomstats->xmin + (x+1) * geow / histocols) - max(box->low.x, geomstats->xmin + x * geow / histocols );
			intersect_x = min(box->xmax, geomstats->xmin + (x+1) * geow / histocols) - max(box->xmin, geomstats->xmin + x * geow / histocols );
			//intersect_y = min(box->high.y, geomstats->ymin + (y+1) * geoh / historows) - max(box->low.y, geomstats->ymin+ y * geoh / historows) ;
			intersect_y = min(box->ymax, geomstats->ymin + (y+1) * geoh / historows) - max(box->ymin, geomstats->ymin+ y * geoh / historows) ;
			
			AOI = intersect_x*intersect_y;
			gain = AOI/cell_area;

#if DEBUG_GEOMETRY_STATS > 1
			elog(NOTICE, " [%d,%d] cell val %.15f",
					x, y, val);
			elog(NOTICE, " [%d,%d] AOI %.15f",
					x, y, AOI);
			elog(NOTICE, " [%d,%d] gain %.15f",
					x, y, gain);
#endif

			val *= gain;

#if DEBUG_GEOMETRY_STATS > 1
			elog(NOTICE, " [%d,%d] adding %.15f to value",
				x, y, val);
#endif
			value += val;
		}
	}


	/*
	 * If the search_box is a point, it will
	 * overlap a single cell and thus get
	 * it's value, which is the fraction of
	 * samples (we can presume of row set also)
	 * which bumped to that cell.
	 *
	 * If the table features are points, each
	 * of them will overlap a single histogram cell.
	 * Our search_box value would then be correctly
	 * computed as the sum of the bumped cells values.
	 *
	 * If both our search_box AND the sample features
	 * overlap more then a single histogram cell we
	 * need to consider the fact that our sum computation
	 * will have many duplicated included. E.g. each
	 * single sample feature would have contributed to
	 * raise the search_box value by as many times as
	 * many cells in the histogram are commonly overlapped
	 * by both searc_box and feature. We should then
	 * divide our value by the number of cells in the virtual
	 * 'intersection' between average feature cell occupation
	 * and occupation of the search_box. This is as 
	 * fuzzy as you understand it :)
	 *
	 * Consistency check: whenever the number of cells is
	 * one of whichever part (search_box_occupation,
	 * avg_feature_occupation) the 'intersection' must be 1.
	 * If sounds that our 'intersaction' is actually the
	 * minimun number between search_box_occupation and
	 * avg_feat_occupation.
	 *
	 */
	overlapping_cells = (x_idx_max-x_idx_min+1) *
		(y_idx_max-y_idx_min+1);
	avg_feat_cells = geomstats->avgFeatureCells;

#if DEBUG_GEOMETRY_STATS
elog(NOTICE, " search_box overlaps %f cells", overlapping_cells);
elog(NOTICE, " avg feat overlaps %f cells", avg_feat_cells);
#endif

	gain = 1/min(overlapping_cells, avg_feat_cells);
	selectivity = value*gain;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " SUM(ov_histo_cells)=%f", value);
	elog(NOTICE, " gain=%f", gain);
	elog(NOTICE, " selectivity=%f", selectivity);
#endif

	/* prevent rounding overflows */
	if (selectivity > 1.0) selectivity = 1.0;
	else if (selectivity < 0) selectivity = 0.0;

	return selectivity;
}

/*
 * This function should return an estimation of the number of
 * rows returned by a query involving an overlap check 
 * ( it's the restrict function for the && operator )
 *
 * It can make use (if available) of the statistics collected
 * by the geometry analyzer function.
 *
 * Note that the good work is done by estimate_selectivity() above.
 * This function just tries to find the search_box, loads the statistics
 * and invoke the work-horse.
 *
 * This is the one used for PG version >= 7.5
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_gist_sel);
Datum LWGEOM_gist_sel(PG_FUNCTION_ARGS)
{
	Query *root = (Query *) PG_GETARG_POINTER(0);
	//Oid operator = PG_GETARG_OID(1);
	List *args = (List *) PG_GETARG_POINTER(2);
	int varRelid = PG_GETARG_INT32(3);
	Oid relid;
	HeapTuple stats_tuple;
	GEOM_STATS *geomstats;
	int geomstats_nvalues=0;
	Node *other;
	Var *self;
	char *in;
	BOX2DFLOAT4 search_box;
	float8 selectivity=0;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, "LWGEOM_gist_sel called");
#endif

        /* Fail if not a binary opclause (probably shouldn't happen) */
	if (list_length(args) != 2)
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, "LWGEOM_gist_sel: not a binary opclause");
#endif
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}


	/*
	 * Find the constant part
	 */
	other = (Node *) linitial(args);
	if ( ! IsA(other, Const) ) 
	{
		self = (Var *)other;
		other = (Node *) lsecond(args);
	}
	else
	{
		self = (Var *) lsecond(args);
	}

	if ( ! IsA(other, Const) ) 
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " no constant arguments - returning default selectivity");
#endif
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	/*
	 * We are working on two constants..
	 * TODO: check if expression is true,
	 *       returned set would be either 
	 *       the whole or none.
	 */
	if ( ! IsA(self, Var) ) 
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " no variable argument ? - returning default selectivity");
#endif
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}

	/*
	 * Convert the constant to a BOX
	 */

	in = (char *)PG_DETOAST_DATUM( ((Const*)other)->constvalue );
	if ( ! getbox2d_p(in+4, &search_box) )
	{
#if DEBUG_GEOMETRY_STATS 
		elog(NOTICE, "search box is EMPTY");
#endif
		PG_RETURN_FLOAT8(0.0);
	}

#if DEBUG_GEOMETRY_STATS > 1
	elog(NOTICE," requested search box is : %.15g %.15g, %.15g %.15g",search_box->xmin,search_box->ymin,search_box->xmax,search_box->ymax);
#endif

	/*
	 * Get pg_statistic row
	 */

	relid = getrelid(varRelid, root->rtable);

	stats_tuple = SearchSysCache(STATRELATT, ObjectIdGetDatum(relid), Int16GetDatum(self->varattno), 0, 0);
	if ( ! stats_tuple )
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " No statistics, returning default estimate");
#endif
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);
	}


	if ( ! get_attstatsslot(stats_tuple, 0, 0,
		STATISTIC_KIND_GEOMETRY, InvalidOid, NULL, NULL,
		(float4 **)&geomstats, &geomstats_nvalues) )
	{
#if DEBUG_GEOMETRY_STATS
		elog(NOTICE, " STATISTIC_KIND_GEOMETRY stats not found - returning default geometry selectivity");
#endif
		ReleaseSysCache(stats_tuple);
		PG_RETURN_FLOAT8(DEFAULT_GEOMETRY_SEL);

	}

#if DEBUG_GEOMETRY_STATS > 1
		elog(NOTICE, " %d read from stats", geomstats_nvalues);
#endif

#if DEBUG_GEOMETRY_STATS > 1
	elog(NOTICE, " histo: xmin,ymin: %f,%f",
		geomstats->xmin, geomstats->ymin);
	elog(NOTICE, " histo: xmax,ymax: %f,%f",
		geomstats->xmax, geomstats->ymax);
	elog(NOTICE, " histo: cols: %f", geomstats->rows);
	elog(NOTICE, " histo: rows: %f", geomstats->cols);
	elog(NOTICE, " histo: avgFeatureArea: %f", geomstats->avgFeatureArea);
	elog(NOTICE, " histo: avgFeatureCells: %f", geomstats->avgFeatureCells);
#endif

	/*
	 * Do the estimation
	 */
	selectivity = estimate_selectivity(&search_box, geomstats);


#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " returning computed value: %f", selectivity);
#endif

	free_attstatsslot(0, NULL, 0, (float *)geomstats, geomstats_nvalues);
	ReleaseSysCache(stats_tuple);
	PG_RETURN_FLOAT8(selectivity);

}

/*
 * This function is called by the analyze function iff
 * the geometry_analyze() function give it its pointer
 * (this is always the case so far).
 * The geometry_analyze() function is also responsible
 * of deciding the number of "sample" rows we will receive
 * here. It is able to give use other 'custom' data, but we
 * won't use them so far.
 *
 * Our job is to build some statistics on the sample data
 * for use by operator estimators.
 *
 * Currently we only need statistics to estimate the number of rows
 * overlapping a given extent (estimation function bound
 * to the && operator).
 *
 */
static void
compute_geometry_stats(VacAttrStats *stats, AnalyzeAttrFetchFunc fetchfunc,
		int samplerows, double totalrows)
{
	MemoryContext old_context;
	int i;
	int geom_stats_size;
	BOX2DFLOAT4 **sampleboxes;
	GEOM_STATS *geomstats;
	bool isnull;
	int null_cnt=0, notnull_cnt=0, examinedsamples=0;
	BOX2DFLOAT4 *sample_extent=NULL;
	double total_width=0;
	double total_boxes_area=0;
	int total_boxes_cells=0;
	double cell_area;
	double cell_width;
	double cell_height;
#if USE_STANDARD_DEVIATION
	/* for standard deviation */
	double avgLOWx, avgLOWy, avgHIGx, avgHIGy;
	double sumLOWx=0, sumLOWy=0, sumHIGx=0, sumHIGy=0;
	double sdLOWx=0, sdLOWy=0, sdHIGx=0, sdHIGy=0;
	BOX2DFLOAT4 *newhistobox=NULL;
#endif
	double geow, geoh; // width and height of histogram
	int histocells;
	int cols, rows; // histogram grid size
	BOX2DFLOAT4 histobox;

	/*
	 * This is where geometry_analyze
	 * should put its' custom parameters.
	 */
	//void *mystats = stats->extra_data;
	
	/*
	 * We'll build an histogram having from 40 to 400 boxesPerSide
	 * Total number of cells is determined by attribute stat
	 * target. It can go from  1600 to 160000 (stat target: 10,1000)
	 */
	histocells = 160*stats->attr->attstattarget;


#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, "compute_geometry_stats called");
	elog(NOTICE, " samplerows: %d", samplerows);
	elog(NOTICE, " histogram cells: %d", histocells);
#endif

	/*
	 * We might need less space, but don't think
	 * its worth saving...
	 */
	sampleboxes = palloc(sizeof(BOX2DFLOAT4 *)*samplerows);

	/*
	 * First scan:
	 *  o find extent of the sample rows
	 *  o count null-infinite/not-null values
	 *  o compute total_width
	 *  o compute total features's box area (for avgFeatureArea)
	 *  o sum features box coordinates (for standard deviation)
	 */
	for (i=0; i<samplerows; i++)
	{
		Datum datum;
		char *geom;
		BOX2DFLOAT4 box;

		datum = fetchfunc(stats, i, &isnull);

		/*
		 * Skip nulls
		 */
		if ( isnull ) {
			null_cnt++;
			continue;
		}

		geom = (char *) PG_DETOAST_DATUM(datum);

		if ( ! getbox2d_p(geom+4, &box) )
		{
			// Skip empty geometry
#if DEBUG_GEOMETRY_STATS 
			elog(NOTICE, " skipped empty geometry %d", i);
#endif
			continue;
		}

		/*
		 * Skip infinite geoms
		 */
		if ( ! finite(box.xmin) ||
			! finite(box.xmax) ||
			! finite(box.ymin) ||
			! finite(box.ymax) )
		{
#if DEBUG_GEOMETRY_STATS 
			elog(NOTICE, " skipped infinite geometry %d", i);
#endif
			continue;
		}

		/*
		 * Cache bounding box
		 * TODO: reduce BOX2DFLOAT4 copies
		 */
		//box = convert_box3d_to_box(&(geom->bvol));
		sampleboxes[notnull_cnt] = palloc(sizeof(BOX2DFLOAT4));
		memcpy(sampleboxes[notnull_cnt], &box, sizeof(BOX2DFLOAT4));

		/*
		 * Add to sample extent union
		 */
		if ( ! sample_extent )
		{
			sample_extent = palloc(sizeof(BOX2DFLOAT4));
			memcpy(sample_extent, &box, sizeof(BOX2DFLOAT4));
		}
		else
		{
			sample_extent->xmax = LWGEOM_Maxf(sample_extent->xmax,
				box.xmax);
			sample_extent->ymax = LWGEOM_Maxf(sample_extent->ymax,
				box.ymax);
			sample_extent->xmin = LWGEOM_Minf(sample_extent->xmin,
				box.xmin);
			sample_extent->ymin = LWGEOM_Minf(sample_extent->ymin,
				box.ymin);
		}
		
		// TODO: ask if we need geom or bvol size for stawidth
		total_width += (int32)*geom;
		total_boxes_area += (box.xmax-box.xmin)*(box.ymax-box.ymin);

#if USE_STANDARD_DEVIATION
		/*
		 * Add bvol coordinates to sum for standard deviation 
		 * computation.
		 */
		sumLOWx += box.xmin;
		sumLOWy += box.ymin;
		sumHIGx += box.xmax;
		sumHIGy += box.ymax;
#endif

		notnull_cnt++;

		/* give backend a chance of interrupting us */
		vacuum_delay_point();

	}

	if ( ! notnull_cnt ) {
		elog(NOTICE, " no notnull values, invalid stats");
		stats->stats_valid = false;
		return;
	}

#if USE_STANDARD_DEVIATION

#if DEBUG_GEOMETRY_STATS 
	elog(NOTICE, " sample_extent: xmin,ymin: %f,%f",
		sample_extent->xmin, sample_extent->ymin);
	elog(NOTICE, " sample_extent: xmax,ymax: %f,%f",
		sample_extent->xmax, sample_extent->ymax);
#endif

	/*
	 * Second scan:
	 *  o compute standard deviation
	 */
	avgLOWx = sumLOWx/notnull_cnt;
	avgLOWy = sumLOWy/notnull_cnt;
	avgHIGx = sumHIGx/notnull_cnt;
	avgHIGy = sumHIGy/notnull_cnt;
	for (i=0; i<notnull_cnt; i++)
	{
		BOX2DFLOAT4 *box;
		box = (BOX2DFLOAT4 *)sampleboxes[i];

		sdLOWx += (box->xmin - avgLOWx) * (box->xmin - avgLOWx);
		sdLOWy += (box->ymin - avgLOWy) * (box->ymin - avgLOWy);
		sdHIGx += (box->xmax - avgHIGx) * (box->xmax - avgHIGx);
		sdHIGy += (box->ymax - avgHIGy) * (box->ymax - avgHIGy);
	}
	sdLOWx = sqrt(sdLOWx/(notnull_cnt-1));
	sdLOWy = sqrt(sdLOWy/(notnull_cnt-1));
	sdHIGx = sqrt(sdHIGx/(notnull_cnt-1));
	sdHIGy = sqrt(sdHIGy/(notnull_cnt-1));

#if DEBUG_GEOMETRY_STATS 
	elog(NOTICE, " standard deviations:");
	elog(NOTICE, "  LOWx - avg:%f sd:%f", avgLOWx, sdLOWx);
	elog(NOTICE, "  LOWy - avg:%f sd:%f", avgLOWy, sdLOWy);
	elog(NOTICE, "  HIGx - avg:%f sd:%f", avgHIGx, sdHIGx);
	elog(NOTICE, "  HIGy - avg:%f sd:%f", avgHIGy, sdHIGy);
#endif

	histobox.xmin = max((avgLOWx - SDFACTOR * sdLOWx),
		sample_extent->xmin);
	histobox.ymin = max((avgLOWy - SDFACTOR * sdLOWy),
		sample_extent->ymin);
	histobox.xmax = min((avgHIGx + SDFACTOR * sdHIGx),
		sample_extent->xmax); 
	histobox.ymax = min((avgHIGy + SDFACTOR * sdHIGy),
		sample_extent->ymax); 

#if DEBUG_GEOMETRY_STATS 
	elog(NOTICE, " sd_extent: xmin,ymin: %f,%f",
		histobox.xmin, histobox.ymin);
	elog(NOTICE, " sd_extent: xmax,ymax: %f,%f",
		histobox.xmin, histobox.ymax);
#endif

	/*
	 * Third scan:
	 *   o skip hard deviants
	 *   o compute new histogram box
	 */
	for (i=0; i<notnull_cnt; i++)
	{
		BOX2DFLOAT4 *box;
		box = (BOX2DFLOAT4 *)sampleboxes[i];

		if ( box->xmin > histobox.xmax ||
			box->xmax < histobox.xmin ||
			box->ymin > histobox.ymax ||
			box->ymax < histobox.ymin )
		{
#if DEBUG_GEOMETRY_STATS > 1
	elog(NOTICE, " feat %d is an hard deviant, skipped", i);
#endif
			sampleboxes[i] = NULL;
			continue;
		}
		if ( ! newhistobox ) {
			newhistobox = palloc(sizeof(BOX));
			memcpy(newhistobox, box, sizeof(BOX));
		} else {
			if ( box->xmin < newhistobox->xmin )
				newhistobox->xmin = box->xmin;
			if ( box->ymin < newhistobox->ymin )
				newhistobox->ymin = box->ymin;
			if ( box->xmax > newhistobox->xmax )
				newhistobox->xmax = box->xmax;
			if ( box->ymax > newhistobox->ymax )
				newhistobox->ymax = box->ymax;
		}
	}

	/*
	 * Set histogram extent as the intersection between
	 * standard deviation based histogram extent 
	 * and computed sample extent after removal of
	 * hard deviants (there might be no hard deviants).
	 */
	if ( histobox.xmin < newhistobox->xmin )
		histobox.xmin = newhistobox->xmin;
	if ( histobox.ymin < newhistobox->ymin )
		histobox.ymin = newhistobox->ymin;
	if ( histobox.xmax > newhistobox->xmax )
		histobox.xmax = newhistobox->xmax;
	if ( histobox.ymax > newhistobox->ymax )
		histobox.ymax = newhistobox->ymax;


#else // ! USE_STANDARD_DEVIATION

	/*
	 * Set histogram extent box
	 */
	histobox.xmin = sample_extent->xmin;
	histobox.ymin = sample_extent->ymin;
	histobox.xmax = sample_extent->xmax;
	histobox.ymax = sample_extent->ymax;
#endif // USE_STANDARD_DEVIATION


#if DEBUG_GEOMETRY_STATS 
	elog(NOTICE, " histogram_extent: xmin,ymin: %f,%f",
		histobox.xmin, histobox.ymin);
	elog(NOTICE, " histogram_extent: xmax,ymax: %f,%f",
		histobox.xmax, histobox.ymax);
#endif


	geow = histobox.xmax - histobox.xmin;
	geoh = histobox.ymax - histobox.ymin;

	/*
	 * Compute histogram cols and rows based on aspect ratio
	 * of histogram extent
	 */
	if ( ! geow && ! geoh ) {
		cols = 1;
		rows = 1;
		histocells = 1;
	} else if ( ! geow ) {
		cols = 1;
		rows = histocells;
	} else if ( ! geoh ) {
		cols = histocells;
		rows = 1;
	} else {
		if ( geow<geoh) {
			cols = ceil(sqrt((double)histocells*(geow/geoh)));
			rows = ceil((double)histocells/cols);
		} else {
			rows = ceil(sqrt((double)histocells*(geoh/geow)));
			cols = ceil((double)histocells/rows);
		}
		histocells = cols*rows;
	}
#if DEBUG_GEOMETRY_STATS 
	elog(NOTICE, " computed histogram grid size (CxR): %dx%d (%d cells)", cols, rows, histocells);
#endif


	/*
	 * Create the histogram (GEOM_STATS)
	 */
	old_context = MemoryContextSwitchTo(stats->anl_context);
	geom_stats_size=sizeof(GEOM_STATS)+(histocells-1)*sizeof(float4);
	geomstats = palloc(geom_stats_size);
	MemoryContextSwitchTo(old_context);

	geomstats->avgFeatureArea = total_boxes_area/notnull_cnt;
	geomstats->xmin = histobox.xmin;
	geomstats->ymin = histobox.ymin;
	geomstats->xmax = histobox.xmax;
	geomstats->ymax = histobox.ymax;
	geomstats->cols = cols;
	geomstats->rows = rows;

	// Initialize all values to 0
	for (i=0;i<histocells; i++) geomstats->value[i] = 0;

	cell_width = geow/cols;
	cell_height = geoh/rows;
	cell_area = cell_width*cell_height;

#if DEBUG_GEOMETRY_STATS > 2
	elog(NOTICE, "cell_width: %f", cell_width);
	elog(NOTICE, "cell_height: %f", cell_height);
#endif
	

	/*
	 * Fourth scan:
	 *  o fill histogram values with the number of
	 *    features' bbox overlaps: a feature's bvol
	 *    can fully overlap (1) or partially overlap
	 *    (fraction of 1) an histogram cell.
	 *
	 *  o compute total cells occupation
	 *
	 */
	for (i=0; i<notnull_cnt; i++)
	{
		BOX *box;
		int x_idx_min, x_idx_max, x;
		int y_idx_min, y_idx_max, y;
		int numcells=0;

		box = (BOX *)sampleboxes[i];
		if ( ! box ) continue; // hard deviant..

		/* give backend a chance of interrupting us */
		vacuum_delay_point();

#if DEBUG_GEOMETRY_STATS > 2
		elog(NOTICE, " feat %d box is %f %f, %f %f",
				i, box->high.x, box->high.y,
				box->low.x, box->low.y);
#endif
							
		/* Find first overlapping column */
		x_idx_min = (box->low.x-geomstats->xmin) / geow * cols;
		if (x_idx_min <0) x_idx_min = 0;
		if (x_idx_min >= cols) x_idx_min = cols-1;

		/* Find first overlapping row */
		y_idx_min = (box->low.y-geomstats->ymin) / geoh * rows;
		if (y_idx_min <0) y_idx_min = 0;
		if (y_idx_min >= rows) y_idx_min = rows-1;

		/* Find last overlapping column */
		x_idx_max = (box->high.x-geomstats->xmin) / geow * cols;
		if (x_idx_max <0) x_idx_max = 0;
		if (x_idx_max >= cols ) x_idx_max = cols-1;

		/* Find last overlapping row */
		y_idx_max = (box->high.y-geomstats->ymin) / geoh * rows;
		if (y_idx_max <0) y_idx_max = 0;
		if (y_idx_max >= rows) y_idx_max = rows-1;
#if DEBUG_GEOMETRY_STATS > 2
		elog(NOTICE, " feat %d overlaps columns %d-%d, rows %d-%d",
				i, x_idx_min, x_idx_max, y_idx_min, y_idx_max);
#endif

		/*
		 * the {x,y}_idx_{min,max}
		 * define the grid squares that the box intersects
		 */
		for (y=y_idx_min; y<=y_idx_max; y++)
		{
			for (x=x_idx_min; x<=x_idx_max; x++)
			{
				geomstats->value[x+y*cols] += 1;
				numcells++;
			}
		}

		// before adding to the total cells
		// we could decide if we really 
		// want this feature to count
		total_boxes_cells += numcells;

		examinedsamples++;
	}
#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " examined_samples: %d/%d", examinedsamples, samplerows);
#endif

	if ( ! examinedsamples ) {
		elog(NOTICE, " no examined values, invalid stats");
		stats->stats_valid = false;
#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " no stats have been gathered");
#endif
		return;
	}

	// what about null features (TODO) ?
	geomstats->avgFeatureCells = (float4)total_boxes_cells/examinedsamples;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " histo: total_boxes_cells: %d", total_boxes_cells);
	elog(NOTICE, " histo: avgFeatureArea: %f", geomstats->avgFeatureArea);
	elog(NOTICE, " histo: avgFeatureCells: %f", geomstats->avgFeatureCells);
#endif


	/*
	 * Normalize histogram
	 *
	 * We divide each histogram cell value
	 * by the number of samples examined.
	 *
	 */
	for (i=0; i<histocells; i++)
		geomstats->value[i] /= examinedsamples;

#if DEBUG_GEOMETRY_STATS > 1
	{
		int x, y;
		for (x=0; x<cols; x++)
		{
			for (y=0; y<rows; y++)
			{
				elog(NOTICE, " histo[%d,%d] = %.15f", x, y, geomstats->value[x+y*cols]);
			}
		}
	}
#endif

	/*
	 * Write the statistics data 
	 */
	stats->stakind[0] = STATISTIC_KIND_GEOMETRY;
	stats->staop[0] = InvalidOid;
	stats->stanumbers[0] = (float4 *)geomstats;
	stats->numnumbers[0] = geom_stats_size/sizeof(float4);

	stats->stanullfrac = null_cnt/samplerows;
	stats->stawidth = total_width/notnull_cnt;
	stats->stadistinct = -1.0;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " out: slot 0: kind %d (STATISTIC_KIND_GEOMETRY)",
		stats->stakind[0]);
	elog(NOTICE, " out: slot 0: op %d (InvalidOid)", stats->staop[0]);
	elog(NOTICE, " out: slot 0: numnumbers %d", stats->numnumbers[0]);
	elog(NOTICE, " out: null fraction: %d/%d", null_cnt, samplerows);
	elog(NOTICE, " out: average width: %d bytes", stats->stawidth);
	elog(NOTICE, " out: distinct values: all (no check done)");
#endif

	stats->stats_valid = true;
}

/*
 * This function will be called when the ANALYZE command is run
 * on a column of the "geometry" type.
 * 
 * It will need to return a stats builder function reference
 * and a "minimum" sample rows to feed it.
 * If we want analisys to be completely skipped we can return
 * FALSE and leave output vals untouched.
 *
 * What we know from this call is:
 *
 * 	o The pg_attribute row referring to the specific column.
 * 	  Could be used to get reltuples from pg_class (which 
 * 	  might quite inexact though...) and use them to set an
 * 	  appropriate minimum number of sample rows to feed to
 * 	  the stats builder. The stats builder will also receive
 * 	  a more accurate "estimation" of the number or rows.
 *
 * 	o The pg_type row for the specific column.
 * 	  Could be used to set stat builder / sample rows
 * 	  based on domain type (when postgis will be implemented
 * 	  that way).
 *
 * Being this experimental we'll stick to a static stat_builder/sample_rows
 * value for now.
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_analyze);
Datum LWGEOM_analyze(PG_FUNCTION_ARGS)
{
	VacAttrStats *stats = (VacAttrStats *)PG_GETARG_POINTER(0);
	Form_pg_attribute attr = stats->attr;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, "lwgeom_analyze called");
#endif

	/* If the attstattarget column is negative, use the default value */
	/* NB: it is okay to scribble on stats->attr since it's a copy */
        if (attr->attstattarget < 0)
                attr->attstattarget = default_statistics_target;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " attribute stat target: %d", attr->attstattarget);
#endif

	/*
	 * There might be a reason not to analyze this column
	 * (can we detect the absence of an index?)
	 */
	//elog(NOTICE, "compute_geometry_stats not implemented yet");
	//PG_RETURN_BOOL(false);

	/* Setup the minimum rows and the algorithm function */
	stats->minrows = 300 * stats->attr->attstattarget;
	stats->compute_stats = compute_geometry_stats;

#if DEBUG_GEOMETRY_STATS
	elog(NOTICE, " minrows: %d", stats->minrows);
#endif

	/* Indicate we are done successfully */
	PG_RETURN_BOOL(true);
}
	


#endif


