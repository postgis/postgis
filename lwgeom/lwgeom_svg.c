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
 **********************************************************************/


#include "postgres.h"
#include "lwgeom.h"

Datum assvg_geometry(PG_FUNCTION_ARGS);
char *geometry_to_svg(PG_LWGEOM *geometry, int svgrel, int precision);
void print_svg_coords(char *result, POINT2D *pt, int precision);
void print_svg_circle(char *result, POINT2D *pt, int precision);
void print_svg_path_abs(char *result, POINTARRAY *pa, int precision);
void print_svg_path_rel(char *result, POINTARRAY *pa, int precision);

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(assvg_geometry);
Datum assvg_geometry(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *svg;
	char *result;
	int len;
	int32 svgrel=0;
	int32 precision=15;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	// check for relative path notation
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
			svgrel = PG_GETARG_INT32(1);

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
		precision = PG_GETARG_INT32(2);
		
	svg = geometry_to_svg(geom, svgrel, precision);

	len = strlen(svg) + 5;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, svg, len-4);

	pfree(svg);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_CSTRING(result);
}


//takes a GEOMETRY and returns a SVG representation
char *geometry_to_svg(PG_LWGEOM *geometry, int svgrel, int precision)
{
	char *result;
	LWGEOM_INSPECTED *inspected;
	int t,u;
	POINT2D	*pt;
	int size;
	int npts;

	//elog(NOTICE, "precision is %d", precision);
	size = 30;	//just enough to put in object type

	if (lwgeom_getType(geometry->type) == COLLECTIONTYPE)
	{
		result = (char *)palloc(64); 
		sprintf(result, "GEOMETRYCOLLECTION not yet supported");
		return result;
	}

	result = palloc(size);
	result[0] = '\0';

	inspected = lwgeom_inspect(SERIALIZED_FORM(geometry));
	for(t=0; t<inspected->ngeometries; t++)
	{
		char *subgeom = lwgeom_getsubgeometry_inspected(inspected, t);
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if (lwgeom_getType(subgeom[0]) == POINTTYPE)
		{
			point = lwpoint_deserialize(subgeom);
			size +=MAX_DIGS_DOUBLE*3 + 2 +10  ;
			//make memory bigger
			result = repalloc(result, size );

			if (t) strcat(result, ",");

			pt = (POINT2D *)getPoint(point->point, 0);
			if (svgrel == 1)
			{  
				//render circle
				print_svg_coords(result, pt, precision);
			}
			else
			{  
				//render circle
				print_svg_circle(result, pt, precision);
			}

		}
		if (lwgeom_getType(subgeom[0]) == LINETYPE)
		{
			line = lwline_deserialize(subgeom);

			size +=(MAX_DIGS_DOUBLE*3+5)*line->points->npoints+12+3;
			result = repalloc(result, size);

			// start path with moveto
			strcat(result, "M ");

			if (svgrel == 1)
				print_svg_path_rel(result, line->points,
					precision);
			else
				print_svg_path_abs( result, line->points,
					precision);

			strcat(result, " ");
		}
		if (lwgeom_getType(subgeom[0]) == POLYGONTYPE)
		{
			poly = lwpoly_deserialize(subgeom);

			npts = 0;
			for (u=0; u<poly->nrings; u++)
				npts += poly->rings[u]->npoints;

			size += (MAX_DIGS_DOUBLE*3+3) * npts +
				5 * poly->nrings;
			result = repalloc(result, size);

			for (u=0; u<poly->nrings; u++)  //for each ring
			{
				strcat(result,"M ");	//begin ring
				if (svgrel == 1)
					print_svg_path_rel(result, 
						poly->rings[u],
						precision);
				else
					print_svg_path_abs(result,
						poly->rings[u],
						precision);
				
				strcat(result," ");	//end ring
			}
		}
	}
	return(result);
}


void print_svg_coords(char *result, POINT2D *pt, int precision)
{
	char	temp[MAX_DIGS_DOUBLE*3+12];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(temp, "x=\"%.*g\" y=\"%.*g\"", 
			precision, pt->x, 
			precision, pt->y*-1);
	strcat(result,temp);
}


void print_svg_circle(char *result, POINT2D *pt, int precision)
{
	char	temp[MAX_DIGS_DOUBLE*3 +12];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(temp, "cx=\"%.*g\" cy=\"%.*g\"", 
			precision, pt->x, 
			precision, pt->y*-1);
	strcat(result,temp);
}


void
print_svg_path_abs(char *result, POINTARRAY *pa, int precision)
{
	int u;
	POINT2D *pt;

	result += strlen(result);
	for (u=0; u<pa->npoints; u++)
	{
		pt = (POINT2D *)getPoint(pa, u);
		if (u != 0)
		{
			result[0] = ' ';
			result++;
		}
		result+= sprintf(result,"%.*g %.*g", 
				precision, pt->x, 
				precision, pt->y*-1);
	}
}


void
print_svg_path_rel(char *result, POINTARRAY *pa, int precision)
{
	int u;
	POINT2D *pt, *lpt;

	result += strlen(result);

	pt = (POINT2D *)getPoint(pa, 0);
	result += sprintf(result,"%.*g %.*g l", 
		precision, pt->x, 
		precision, pt->y*-1);

	lpt = pt;
	for (u=1; u<pa->npoints; u++)
	{
		pt = (POINT2D *)getPoint(pa, u);
		result+= sprintf(result," %.*g %.*g", 
				precision, (pt->x-lpt->x), 
				precision, (pt->y-lpt->y)*-1);
		lpt = pt;
	}
}


/**********************************************************************
 * $Log$
 * Revision 1.2  2004/09/29 06:31:42  strk
 * Changed LWGEOM to PG_LWGEOM.
 * Changed LWGEOM_construct to PG_LWGEOM_construct.
 *
 * Revision 1.1  2004/09/13 13:32:01  strk
 * Added AsSVG().
 *
 **********************************************************************/

