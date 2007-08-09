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
#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum assvg_geometry(PG_FUNCTION_ARGS);
char *geometry_to_svg(PG_LWGEOM *geometry, int svgrel, int precision);
void print_svg_coords(char *result, POINT2D *pt, int precision);
void print_svg_circle(char *result, POINT2D *pt, int precision);
void print_svg_path_abs(char *result, POINTARRAY *pa, int precision);
void print_svg_path_rel(char *result, POINTARRAY *pa, int precision);

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(assvg_geometry);
Datum assvg_geometry(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *svg;
	text *result;
	int len;
	int32 svgrel=0;
	int32 precision=MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
			svgrel = PG_GETARG_INT32(1);

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}
		
	svg = geometry_to_svg(geom, svgrel, precision);
	if ( ! svg ) PG_RETURN_NULL();

	len = strlen(svg) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), svg, len-VARHDRSZ);

	pfree(svg);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}


/*takes a GEOMETRY and returns a SVG representation */
char *
geometry_to_svg(PG_LWGEOM *geometry, int svgrel, int precision)
{
	char *result;
	LWGEOM_INSPECTED *inspected;
	int t,u;
	POINT2D	pt;
	int size;
	int npts;

	/*elog(NOTICE, "precision is %d", precision); */
	size = 30;	/*just enough to put in object type */

	if (lwgeom_getType(geometry->type) == COLLECTIONTYPE)
	{
		return NULL;
	}

	result = palloc(size);
	result[0] = '\0';

	inspected = lwgeom_inspect(SERIALIZED_FORM(geometry));
	for(t=0; t<inspected->ngeometries; t++)
	{
		uchar *subgeom = lwgeom_getsubgeometry_inspected(inspected, t);
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;

		if (lwgeom_getType(subgeom[0]) == POINTTYPE)
		{
			point = lwpoint_deserialize(subgeom);
			size +=MAX_DIGS_DOUBLE*3 + 2 +10  ;
			/*make memory bigger */
			result = repalloc(result, size );

			if (t) strcat(result, ",");

			getPoint2d_p(point->point, 0, &pt);
			if (svgrel == 1)
			{  
				/*render circle */
				print_svg_coords(result, &pt, precision);
			}
			else
			{  
				/*render circle */
				print_svg_circle(result, &pt, precision);
			}

		}
		if (lwgeom_getType(subgeom[0]) == LINETYPE)
		{
			line = lwline_deserialize(subgeom);

			size +=(MAX_DIGS_DOUBLE*3+5)*line->points->npoints+12+3;
			result = repalloc(result, size);

			/* start path with moveto */
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

			for (u=0; u<poly->nrings; u++)  /*for each ring */
			{
				strcat(result,"M ");	/*begin ring */
				if (svgrel == 1)
					print_svg_path_rel(result, 
						poly->rings[u],
						precision);
				else
					print_svg_path_abs(result,
						poly->rings[u],
						precision);
				
				strcat(result," ");	/*end ring */
			}
		}
	}
	return(result);
}


void print_svg_coords(char *result, POINT2D *pt, int precision)
{
	char temp[MAX_DIGS_DOUBLE*3+12];
	char x[MAX_DIGS_DOUBLE+3];
	char y[MAX_DIGS_DOUBLE+3];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(x, "%.*f", precision, pt->x);
	trim_trailing_zeros(x);
	sprintf(y, "%.*f", precision, pt->y * -1);
	trim_trailing_zeros(y);

	sprintf(temp, "x=\"%s\" y=\"%s\"", x, y);
	strcat(result,temp);
}


void print_svg_circle(char *result, POINT2D *pt, int precision)
{
	char temp[MAX_DIGS_DOUBLE*3 +12];
	char x[MAX_DIGS_DOUBLE+3];
	char y[MAX_DIGS_DOUBLE+3];

	if ( (pt == NULL) || (result == NULL) )
		return;

	sprintf(x, "%.*f", precision, pt->x);
	trim_trailing_zeros(x);
	sprintf(y, "%.*f", precision, pt->y * -1);
	trim_trailing_zeros(y);

	sprintf(temp, "cx=\"%s\" cy=\"%s\"", x, y);
	strcat(result,temp);
}


void
print_svg_path_abs(char *result, POINTARRAY *pa, int precision)
{
	int u;
	POINT2D pt;
	char x[MAX_DIGS_DOUBLE+3];
	char y[MAX_DIGS_DOUBLE+3];

	result += strlen(result);
	for (u=0; u<pa->npoints; u++)
	{
		getPoint2d_p(pa, u, &pt);
		if (u != 0)
		{
			result[0] = ' ';
			result++;
		}
		sprintf(x, "%.*f", precision, pt.x);
		trim_trailing_zeros(x);
		sprintf(y, "%.*f", precision, pt.y * -1);
		trim_trailing_zeros(y);
		result+= sprintf(result,"%s %s", x, y);
	}
}


void
print_svg_path_rel(char *result, POINTARRAY *pa, int precision)
{
	int u;
	POINT2D pt, lpt;
	char x[MAX_DIGS_DOUBLE+3];
	char y[MAX_DIGS_DOUBLE+3];

	result += strlen(result);

	getPoint2d_p(pa, 0, &pt);

	sprintf(x, "%.*f", precision, pt.x);
	trim_trailing_zeros(x);
	sprintf(y, "%.*f", precision, pt.y * -1);
	trim_trailing_zeros(y);

	result += sprintf(result,"%s %s l", x, y); 

	lpt = pt;
	for (u=1; u<pa->npoints; u++)
	{
		getPoint2d_p(pa, u, &pt);
		sprintf(x, "%.*f", precision, pt.x - lpt.x);
		trim_trailing_zeros(x);
		sprintf(y, "%.*f", precision, (pt.y - lpt.y) * -1);
		trim_trailing_zeros(y);
		result+= sprintf(result," %s %s", x, y);
		lpt = pt;
	}
}


/**********************************************************************
 * $Log$
 * Revision 1.11  2006/01/09 15:55:55  strk
 * ISO C90 comments (finished in lwgeom/)
 *
 * Revision 1.10  2005/12/30 18:14:53  strk
 * Fixed all signedness warnings
 *
 * Revision 1.9  2005/11/18 10:16:21  strk
 * Removed casts on lwalloc return.
 * Used varlena macros when appropriate.
 *
 * Revision 1.8  2005/02/10 17:41:55  strk
 * Dropped getbox2d_internal().
 * Removed all castings of getPoint() output, which has been renamed
 * to getPoint_internal() and commented about danger of using it.
 * Changed SERIALIZED_FORM() macro to use VARDATA() macro.
 * All this changes are aimed at taking into account memory alignment
 * constraints which might be the cause of recent crash bug reports.
 *
 * Revision 1.7  2004/10/27 12:30:53  strk
 * AsSVG returns NULL on GEOMETRY COLLECTION input.
 *
 * Revision 1.6  2004/10/25 14:20:57  strk
 * Y axis reverse and relative path fixes from Olivier Courtin.
 *
 * Revision 1.5  2004/10/15 11:48:48  strk
 * Fixed a bug making asSVG return a spurious char at the end.
 *
 * Revision 1.4  2004/10/15 09:41:22  strk
 * changed precision semantic back to number of decimal digits
 *
 * Revision 1.3  2004/09/29 10:50:30  strk
 * Big layout change.
 * lwgeom.h is public API
 * liblwgeom.h is private header
 * lwgeom_pg.h is for PG-links
 * lw<type>.c contains type-specific functions
 *
 * Revision 1.2  2004/09/29 06:31:42  strk
 * Changed LWGEOM to PG_LWGEOM.
 * Changed LWGEOM_construct to PG_LWGEOM_construct.
 *
 * Revision 1.1  2004/09/13 13:32:01  strk
 * Added AsSVG().
 *
 **********************************************************************/

