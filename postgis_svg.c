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
 *
 * SVG output routines.
 * Originally written by: Klaus Förster <klaus@svg.cc>
 * Patches from: Olivier Courtin <pnine@free.fr>
 *
 **********************************************************************
 * $Log$
 * Revision 1.4  2004/09/13 14:26:27  strk
 * indentation fix
 *
 * Revision 1.3  2004/09/10 16:16:31  pramsey
 * Added Log tag to header.
 *
 *
 **********************************************************************/


#include "postgres.h"
#include "postgis.h"

Datum assvg_geometry(PG_FUNCTION_ARGS);
char *geometry_to_svg(GEOMETRY  *geometry, int svgrel, int precision);
void print_svg_coords(char *result, POINT3D *pt, int precision);
void print_svg_circle(char *result, POINT3D *pt, int precision);
void print_svg_path_abs(char *result, POINT3D *pt, int npoints, int precision);
void print_svg_path_rel(char *result, POINT3D *pt, int npoints, int precision);

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(assvg_geometry);
Datum assvg_geometry(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1;
	char *wkt;
	char *result;
	int len;
	int32 svgrel=0;
	int32 precision=15;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom1 = (GEOMETRY *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	// check for relative path notation
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
			svgrel = PG_GETARG_INT32(1);

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
		precision = PG_GETARG_INT32(2);
		
	wkt = geometry_to_svg(geom1, svgrel, precision);

	len = strlen(wkt) + 5;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, wkt, len-4);

	pfree(wkt);

	PG_RETURN_CSTRING(result);
}


//takes a GEOMETRY and returns a SVG representation
char *geometry_to_svg(GEOMETRY  *geometry, int svgrel, int precision)
{
	char		*result;
	int		t,u;
	int32		*offsets;
	char		*obj;
	POINT3D	*pts;
	POLYGON3D	*polygon;
	LINE3D	*line;
	POINT3D	*point;

	int		pt_off,size;
	bool		first_sub_obj = TRUE;
	int		npts;

	//elog(NOTICE, "precision is %d", precision);
	size = 30;	//just enough to put in object type

	// TODO BBox, from where is it called?...
	if (geometry->type == BBOXONLYTYPE)
	{
		if (svgrel == 1)
		{
			// 5 double digits+ "Mhvhz"+ spaces +null
			size = MAX_DIGS_DOUBLE*5+5+6+1;
			result = (char *) palloc(size); 

			sprintf(result, "M %.*g %.*g h%.*g v%.*g h%.*g z",
				precision, 
				geometry->bvol.LLB.x,
				precision, 
				geometry->bvol.URT.y*-1,
				precision, 
				(geometry->bvol.URT.x 
				 - geometry->bvol.LLB.x),
				precision, 
				(geometry->bvol.URT.y 
				 - geometry->bvol.LLB.y),
				precision, 
				(geometry->bvol.URT.x 
				 - geometry->bvol.LLB.x)*-1);
		}
		else
		{
			size = MAX_DIGS_DOUBLE*4+3+1;
			result = (char *) palloc(size); 
			// 4 double digits + 3 spaces +null

			sprintf(result, "%.*g %.*g %.*g %.*g",
				precision,
				geometry->bvol.LLB.x,
				precision,
				geometry->bvol.URT.y*-1,
				precision,
				(geometry->bvol.URT.x - 
geometry->bvol.LLB.x),
				precision, 
				(geometry->bvol.URT.y - 
geometry->bvol.LLB.y)
				);
		}
		return result;
	}

	if (geometry->type == COLLECTIONTYPE)
	{
		result = (char *)palloc(64); 
		sprintf(result, "GEOMETRYCOLLECTION not yet supported");
		return result;
	}

	//where are the objects?
	offsets = (int32 *) ( ((char *) &(geometry->objType[0] ))
			+ sizeof(int32) * geometry->nobjs ) ;

	result = palloc(size);
	result[0] = '\0';
	for(t=0;t<geometry->nobjs; t++)  //for each object
	{
		obj = (char *) geometry +offsets[t] ;

		if (geometry->objType[t] == 1)   //POINT
		{
			point = (POINT3D *) obj;
			size +=MAX_DIGS_DOUBLE*3 + 2 +10  ;
			//make memory bigger
			result = repalloc(result, size );

			if (!(first_sub_obj))
			{	
				// next circle ...
				strcat(result,",");
			}
			else
			{
				first_sub_obj = FALSE;
			}
			if (svgrel == 1)
			{  
				//render circle
				print_svg_coords(result, point, precision);
			}
			else
			{  
				//render circle
				print_svg_circle(result, point, precision);
			}

		}
		if (geometry->objType[t] == 2)	//LINESTRING
		{
			line = (LINE3D *) obj;

			size +=(MAX_DIGS_DOUBLE*3+5)*line->npoints +12+3;
			result = repalloc(result, size );

			// start path with moveto
			strcat(result, "M ");

			if (svgrel == 1)
				print_svg_path_rel(
						result,
						&line->points[0],
						line->npoints, 
						precision
						);
			else
				print_svg_path_abs(
						result,
						&line->points[0],
						line->npoints,
						precision
						);

			strcat(result," ");
		}
		if (geometry->objType[t] == 3)	//POLYGON
		{
			polygon = (POLYGON3D *) obj;
			pt_off = 0;	//where is the first point in this ring?

			//where are the points
			pts = (POINT3D *)
				((char *)&(polygon->npoints[polygon->nrings]));
			pts = (POINT3D *) MAXALIGN(pts);

			npts = 0;
			for (u=0; u< polygon->nrings ; u++)
				npts += polygon->npoints[u];

			size += (MAX_DIGS_DOUBLE*3+3) 
				* npts + 5* polygon->nrings;
			result = repalloc(result, size );

			for (u=0; u< polygon->nrings ; u++)  //for each ring
			{
				strcat(result,"M ");	//begin ring
				if (svgrel == 1)
					print_svg_path_rel(result, 
							&pts[pt_off] ,
							polygon->npoints[u],
							precision);
				else
					print_svg_path_abs(result,
							&pts[pt_off],
							polygon->npoints[u],
							precision);
				
				//where is first point of next ring?
				pt_off = pt_off + polygon->npoints[u]; 
				strcat(result," ");	//end ring
			}
		}
	}
	return(result);
}


void print_svg_coords(char *result, POINT3D *pt, int precision)
{
	char	temp[MAX_DIGS_DOUBLE*3 +12];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(temp, "x=\"%.*g\" y=\"%.*g\"", 
			precision, pt->x, 
			precision, pt->y*-1);
	strcat(result,temp);
}


void print_svg_circle(char *result, POINT3D *pt, int precision)
{
	char	temp[MAX_DIGS_DOUBLE*3 +12];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(temp, "cx=\"%.*g\" cy=\"%.*g\"", 
			precision, pt->x, 
			precision, pt->y*-1);
	strcat(result,temp);
}


void print_svg_path_abs(char *result, POINT3D *pt ,int npoints, int
precision){
	int	u;

	result += strlen(result);
	for (u=0;u<npoints;u++)
	{
		if (u != 0)
		{
			result[0] = ' ';
			result++;
		}
		result+= sprintf(result,"%.*g %.*g", 
				precision, pt[u].x, 
				precision, pt[u].y*-1);
	}
}


void print_svg_path_rel(char *result, POINT3D *pt ,int npoints, int
precision){
	int	u;

	result += strlen(result);
	for (u=0;u<npoints;u++)
	{
		if (u == 0)
		{
			result+= sprintf(result,"%.*g %.*g l", 
					precision, pt[u].x, 
					precision, pt[u].y*-1);
		}
		else
		{
			result+= sprintf(result," %.*g %.*g", 
					precision, (pt[u].x-pt[u-1].x), 
					precision, (pt[u].y-pt[u-1].y)*-1);
		}
	}
}


/**********************************************************************
 * $Log$
 * Revision 1.4  2004/09/13 14:26:27  strk
 * indentation fix
 *
 * Revision 1.3  2004/09/10 16:16:31  pramsey
 * Added Log tag to header.
 *
 * Revision 1.2  2004/09/10 13:25:36  strk
 * fixed a memory fault
 *
 * Revision 1.1  2004/09/10 12:49:29  strk
 * Included SVG output function, modified to have precision expressed
 * in terms of significant digits.
 *
 **********************************************************************/

