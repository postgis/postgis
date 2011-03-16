/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://www.postgis.org
 * adapted from lwout_asgml.c
 * Copyright 2011 Arrival 3D 
 * 				Regina Obe with input from Dave Arendash
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/**
* @file X3D output routines.
*
**********************************************************************/


#include <string.h>
#include <math.h> /* fabs */
#include "liblwgeom_internal.h"
/** defid is the id of the coordinate can be used to hold other elements DEF='abc' transform='' etc. **/
static size_t asx3d3_point_size(const LWPOINT *point, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_point(const LWPOINT *point, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_line_size(const LWLINE *line, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_line(const LWLINE *line, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_poly_size(const LWPOLY *poly, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_poly(const LWPOLY *poly, char *srs, int precision, int opts, int is_patch, const char *defid);
static size_t asx3d3_triangle_size(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_triangle(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_multi_size(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_multi(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_psurface(const LWPSURFACE *psur, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_tin(const LWTIN *tin, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_collection_size(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_collection(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static size_t pointArray_toX3D3(POINTARRAY *pa, char *buf, int precision, int opts, int type);

static size_t pointArray_X3Dsize(POINTARRAY *pa, int precision);


/*
 * VERSION X3D 3.0.2 http://www.web3d.org/specifications/x3d-3.0.dtd
 */


/* takes a GEOMETRY and returns an X3D representation */
extern char *
lwgeom_to_x3d3(const LWGEOM *geom, char *srs, int precision, int opts, const char *defid)
{
	int type = geom->type;

	switch (type)
	{
	case POINTTYPE:
		return asx3d3_point((LWPOINT*)geom, srs, precision, opts, defid);

	case LINETYPE:
		return asx3d3_line((LWLINE*)geom, srs, precision, opts, defid);

	case POLYGONTYPE:
		return asx3d3_poly((LWPOLY*)geom, srs, precision, opts, 0, defid);

	case TRIANGLETYPE:
		return asx3d3_triangle((LWTRIANGLE*)geom, srs, precision, opts, defid);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		return asx3d3_multi((LWCOLLECTION*)geom, srs, precision, opts, defid);

	case POLYHEDRALSURFACETYPE:
		return asx3d3_psurface((LWPSURFACE*)geom, srs, precision, opts, defid);

	case TINTYPE:
		return asx3d3_tin((LWTIN*)geom, srs, precision, opts, defid);

	case COLLECTIONTYPE:
		return asx3d3_collection((LWCOLLECTION*)geom, srs, precision, opts, defid);

	default:
		lwerror("lwgeom_to_x3d3: '%s' geometry type not supported", lwtype_name(type));
		return NULL;
	}
}

static size_t
asx3d3_point_size(const LWPOINT *point, char *srs, int precision, int opts, const char *defid)
{
	int size;
	//size_t defidlen = strlen(defid);

	size = pointArray_X3Dsize(point->point, precision);
	//size += ( sizeof("<point><pos>/") + (defidlen*2) ) * 2;
	//if (srs)     size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asx3d3_point_buf(const LWPOINT *point, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr = output;
	//int dimension=2;

	//if (FLAGS_GET_Z(point->flags)) dimension = 3;
/*	if ( srs )
	{
		ptr += sprintf(ptr, "<%sPoint srsName=\"%s\">", defid, srs);
	}
	else*/
	//ptr += sprintf(ptr, "%s", defid);
	
	//ptr += sprintf(ptr, "<%spos>", defid);
	ptr += pointArray_toX3D3(point->point, ptr, precision, opts, point->type);
	//ptr += sprintf(ptr, "</%spos></%sPoint>", defid, defid);

	return (ptr-output);
}

static char *
asx3d3_point(const LWPOINT *point, char *srs, int precision, int opts, const char *defid)
{
	char *output;
	int size;

	size = asx3d3_point_size(point, srs, precision, opts, defid);
	output = lwalloc(size);
	asx3d3_point_buf(point, srs, output, precision, opts, defid);
	return output;
}


static size_t
asx3d3_line_size(const LWLINE *line, char *srs, int precision, int opts, const char *defid)
{
	int size;
	size_t defidlen = strlen(defid);

	size = pointArray_X3Dsize(line->points, precision)*2;

	size += (
		sizeof("<LineSet vertexCount=''><Coordinate point='' /></LineSet>")  + defidlen  
	) * 2;
	
	//if (srs)     size += strlen(srs) + sizeof(" srsName=..");
	return size;
}

static size_t
asx3d3_line_buf(const LWLINE *line, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr=output;
	//int dimension=2;
	POINTARRAY *pa;


	//if (FLAGS_GET_Z(line->flags)) dimension = 3;

	pa = line->points;
	ptr += sprintf(ptr, "<LineSet %s vertexCount='%d'>", defid, pa->npoints);


	ptr += sprintf(ptr, "<Coordinate point='");
	ptr += pointArray_toX3D3(line->points, ptr, precision, opts, line->type);

	ptr += sprintf(ptr, "' />");

	ptr += sprintf(ptr, "</LineSet>");
	return (ptr-output);
}

static size_t
asx3d3_line_coords(const LWLINE *line, char *output, int precision, int opts)
{
	char *ptr=output;
	
	ptr += sprintf(ptr, "");
	ptr += pointArray_toX3D3(line->points, ptr, precision, opts, line->type);
	return (ptr-output);
}

/* Calculate the coordIndex property of the IndexedLineSet for the multilinestring */
static size_t
asx3d3_mline_coordindex(const LWCOLLECTION *mgeom, char *output)
{
	char *ptr=output;
	LWLINE *geom;
	int i, j, k;
	POINTARRAY *pa;
	int np;
	
	ptr += sprintf(ptr, "");
	j = 0;
	for (i=0; i < mgeom->ngeoms; i++)
	{
		geom = (LWLINE *) mgeom->geoms[i];
		pa = geom->points;
		np = pa->npoints - 1;
		for(k=0; k < np ; k++){
            if (k) {
                ptr += sprintf(ptr, " ");    
            }
            ptr += sprintf(ptr, "%d", (j + k));
		}
		if (i < (mgeom->ngeoms - 1) ){
				ptr += sprintf(ptr, " -1 "); //separator for each subgeom
		}
		j += k;
	}
	return (ptr-output);
}

/* Return the linestring as an X3D LineSet */
static char *
asx3d3_line(const LWLINE *line, char *srs, int precision, int opts, const char *defid)
{
	char *output;
	int size;

	size = sizeof("<LineSet><CoordIndex ='' /></LineSet>") + asx3d3_line_size(line, srs, precision, opts, defid);
	output = lwalloc(size);
	asx3d3_line_buf(line, srs, output, precision, opts, defid);
	return output;
}

/* Compute the string space needed for the IndexedFaceSet representation of the polygon **/
static size_t
asx3d3_poly_size(const LWPOLY *poly,  char *srs, int precision, int opts, const char *defid)
{
	size_t size;
	size_t defidlen = strlen(defid);
	int i;

	size = ( sizeof("<IndexedFaceSet></IndexedFaceSet>") + (defidlen*3) ) * 2 + 6 * (poly->nrings - 1);

	for (i=0; i<poly->nrings; i++)
		size += pointArray_X3Dsize(poly->rings[i], precision);

	return size;
}

/** Compute the X3D coordinates of the polygon **/
static size_t
asx3d3_poly_buf(const LWPOLY *poly, char *srs, char *output, int precision, int opts, int is_patch, const char *defid)
{
	int i;
	char *ptr=output;
	int dimension=2;

	if (FLAGS_GET_Z(poly->flags)) dimension = 3;
	ptr += pointArray_toX3D3(poly->rings[0], ptr, precision, opts, poly->type);
	for (i=1; i<poly->nrings; i++)
	{
		ptr += pointArray_toX3D3(poly->rings[i], ptr, precision, opts, poly->type);
		ptr += sprintf(ptr, " ");
	}
	return (ptr-output);
}

static char *
asx3d3_poly(const LWPOLY *poly, char *srs, int precision, int opts, int is_patch, const char *defid)
{
	char *output;
	int size;

	size = asx3d3_poly_size(poly, srs, precision, opts, defid);
	output = lwalloc(size);
	asx3d3_poly_buf(poly, srs, output, precision, opts, is_patch, defid);
	return output;
}


static size_t
asx3d3_triangle_size(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid)
{
	size_t size;
	size_t defidlen = strlen(defid);

	/** 6 for the 3 sides and space to separate each side **/ 
	size = sizeof("<IndexedTriangleSet index=''></IndexedTriangleSet>") + defidlen + 6; 
	size += pointArray_X3Dsize(triangle->points, precision);

	return size;
}

static size_t
asx3d3_triangle_buf(const LWTRIANGLE *triangle, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr=output;
	ptr += pointArray_toX3D3(triangle->points, ptr, precision, opts, triangle->type);

	return (ptr-output);
}

static char *
asx3d3_triangle(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid)
{
	char *output;
	int size;

	size = asx3d3_triangle_size(triangle, srs, precision, opts, defid);
	output = lwalloc(size);
	asx3d3_triangle_buf(triangle, srs, output, precision, opts, defid);
	return output;
}


/*
 * Compute max size required for X3D version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asx3d3_multi_size(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
	int i;
	size_t size;
	size_t defidlen = strlen(defid);
	LWGEOM *subgeom;

	/* the longest possible multi version needs to hold DEF=defid and coordinate breakout */
	size = sizeof("<PointSet><Coordinate point='' /></PointSet>") + defidlen;

	//if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			//size += ( sizeof("point=''") + defidlen ) * 2;
			size += asx3d3_point_size((LWPOINT*)subgeom, 0, precision, opts, defid);
		}
		else if (subgeom->type == LINETYPE)
		{
			//size += ( sizeof("<curveMember>/") + defidlen ) * 2;
			size += asx3d3_line_size((LWLINE*)subgeom, 0, precision, opts, defid);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			//size += ( sizeof("<surfaceMember>/") + defidlen ) * 2;
			size += asx3d3_poly_size((LWPOLY*)subgeom, 0, precision, opts, defid);
		}
	}

	return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_multi_buf(const LWCOLLECTION *col, char *srs, char *output, int precision, int opts, const char *defid)
{
	int type = col->type;
	char *ptr, *x3dtype, *coordIndex;
	int i;
	//int numvertices;
	LWGEOM *subgeom;
	POINTARRAY *pa;

	ptr = output;
	x3dtype="";
	coordIndex = lwalloc(1000);

	for (i=0; i<col->ngeoms; i++){
		/** TODO: This is wrong, but haven't quite figured out how to correct. Involves ring order and bunch of other stuff **/
		coordIndex += sprintf(coordIndex, "-1 ");
	}
			
	if 	(type == MULTIPOINTTYPE) {
		x3dtype = "PointSet";
		ptr += sprintf(ptr, "<%s %s>", x3dtype, defid);
	}
	else if (type == MULTILINETYPE) {
		x3dtype = "IndexedLineSet";
		ptr += sprintf(ptr, "<%s %s coordIndex='", x3dtype, defid);
		ptr += asx3d3_mline_coordindex(col, ptr);
		ptr += sprintf(ptr, "'>");
	}
	else if (type == MULTIPOLYGONTYPE) {
		x3dtype = "IndexedFaceSet";
		ptr += sprintf(ptr, "<%s %s coordIndex='%s'>", x3dtype, defid, coordIndex);
	}

	ptr += sprintf(ptr, "<Coordinate point='");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			ptr += asx3d3_point_buf((LWPOINT*)subgeom, 0, ptr, precision, opts, defid);
			ptr += sprintf(ptr, " ");
		}
		else if (subgeom->type == LINETYPE)
		{
			ptr += asx3d3_line_coords((LWLINE*)subgeom, ptr, precision, opts);
			ptr += sprintf(ptr, " ");
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			ptr += asx3d3_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, opts, 0, defid);
			ptr += sprintf(ptr, " ");
		}
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "' /></%s>", x3dtype);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_multi(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
	char *x3d;
	size_t size;

	size = asx3d3_multi_size(col, srs, precision, opts, defid);
	x3d = lwalloc(size);
	asx3d3_multi_buf(col, srs, x3d, precision, opts, defid);
	return x3d;
}


static size_t
asx3d3_psurface_size(const LWPSURFACE *psur, char *srs, int precision, int opts, const char *defid)
{
	int i;
	size_t size;
	size_t defidlen = strlen(defid);

	size = sizeof("<IndexedFaceSet coordIndex=''><Coordinate point='' />") + defidlen;
	
	for (i=0; i<psur->ngeoms; i++)
	{
		size += asx3d3_poly_size(psur->geoms[i], 0, precision, opts, defid)*5; /** need to make space for coordIndex values too including -1 separating each poly**/
	}

	return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_psurface_buf(const LWPSURFACE *psur, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr;
	int i;
	int j;
	int k;
	int np;
	LWPOLY *patch;

	ptr = output;

	/* Open outmost tag */
	ptr += sprintf(ptr, "<IndexedFaceSet %s coordIndex='",defid);
	
	j = 0;
	for (i=0; i<psur->ngeoms; i++)
	{
		patch = (LWPOLY *) psur->geoms[i];
		np = patch->rings[0]->npoints - 1;
	    for(k=0; k < np ; k++){
	        if (k) {
	            ptr += sprintf(ptr, " ");    
	        }
	        ptr += sprintf(ptr, "%d", (j + k));
	    }
	    if (i < (psur->ngeoms - 1) ){
	        ptr += sprintf(ptr, " -1 "); //separator for each subgeom
	    }
	    j += k;
	}
	
	ptr += sprintf(ptr, "'><Coordinate point='");

	for (i=0; i<psur->ngeoms; i++)
	{
		ptr += asx3d3_poly_buf(psur->geoms[i], 0, ptr, precision, opts, 1, defid);
		if (i < (psur->ngeoms - 1) ){
	        ptr += sprintf(ptr, " "); //only add a trailing space if its not the last polygon in the set
	    }
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "' /></IndexedFaceSet>");

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_psurface(const LWPSURFACE *psur, char *srs, int precision, int opts, const char *defid)
{
	char *x3d;
	size_t size;

	size = asx3d3_psurface_size(psur, srs, precision, opts, defid);
	x3d = lwalloc(size);
	asx3d3_psurface_buf(psur, srs, x3d, precision, opts, defid);
	return x3d;
}


static size_t
asx3d3_tin_size(const LWTIN *tin, char *srs, int precision, int opts, const char *defid)
{
	int i;
	size_t size;
	size_t defidlen = strlen(defid);
	//int dimension=2;

	/** Need to make space for size of additional attributes, 
	** the coordIndex has a value for each edge for each triangle plus a space to separate so we need at least that much extra room ***/
	size = sizeof("<IndexedTriangleSet coordIndex=''></IndexedTriangleSet>") + defidlen + tin->ngeoms*12;

	for (i=0; i<tin->ngeoms; i++)
	{
		size += (asx3d3_triangle_size(tin->geoms[i], 0, precision, opts, defid) * 20); /** 3 is to make space for coordIndex **/
	}

	return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_tin_buf(const LWTIN *tin, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr;
	int i;
	int k;
	//int dimension=2;

	ptr = output;

	ptr += sprintf(ptr, "<IndexedTriangleSet %s index='",defid);
	k = 0;
	/** Fill in triangle index **/
	for (i=0; i<tin->ngeoms; i++)
	{
		ptr += sprintf(ptr, "%d %d %d", k, (k+1), (k+2));
		if (i < (tin->ngeoms - 1) ){
		    ptr += sprintf(ptr, " ");
		}
		k += 3;
	}
	
	ptr += sprintf(ptr, "'><Coordinate point='");
	for (i=0; i<tin->ngeoms; i++)
	{
		ptr += asx3d3_triangle_buf(tin->geoms[i], 0, ptr, precision,
		                           opts, defid);
		if (i < (tin->ngeoms - 1) ){
		    ptr += sprintf(ptr, " ");
		}
	}

	/* Close outmost tag */

	ptr += sprintf(ptr, "'/></IndexedTriangleSet>");
	
	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_tin(const LWTIN *tin, char *srs, int precision, int opts, const char *defid)
{
	char *x3d;
	size_t size;

	size = asx3d3_tin_size(tin, srs, precision, opts, defid);
	x3d = lwalloc(size);
	asx3d3_tin_buf(tin, srs, x3d, precision, opts, defid);
	return x3d;
}

static size_t
asx3d3_collection_size(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
	int i;
	size_t size;
	size_t defidlen = strlen(defid);
	LWGEOM *subgeom;

	size = sizeof("<MultiGeometry></MultiGeometry>") + defidlen*2;

	if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		size += ( sizeof("<geometryMember>/") + defidlen ) * 2;
		if ( subgeom->type == POINTTYPE )
		{
			size += asx3d3_point_size((LWPOINT*)subgeom, 0, precision, opts, defid);
		}
		else if ( subgeom->type == LINETYPE )
		{
			size += asx3d3_line_size((LWLINE*)subgeom, 0, precision, opts, defid);
		}
		else if ( subgeom->type == POLYGONTYPE )
		{
			size += asx3d3_poly_size((LWPOLY*)subgeom, 0, precision, opts, defid);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			size += asx3d3_multi_size((LWCOLLECTION*)subgeom, 0, precision, opts, defid);
		}
		else
			lwerror("asx3d3_collection_size: unknown geometry type");
	}

	return size;
}

static size_t
asx3d3_collection_buf(const LWCOLLECTION *col, char *srs, char *output, int precision, int opts, const char *defid)
{
	char *ptr;
	int i;
	LWGEOM *subgeom;

	ptr = output;

	/* Open outmost tag */
	if ( srs )
	{
		ptr += sprintf(ptr, "<%sMultiGeometry srsName=\"%s\">", defid, srs);
	}
	else
	{
		ptr += sprintf(ptr, "<%sMultiGeometry>", defid);
	}

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		ptr += sprintf(ptr, "<%sgeometryMember>", defid);
		if ( subgeom->type == POINTTYPE )
		{
			ptr += asx3d3_point_buf((LWPOINT*)subgeom, 0, ptr, precision, opts, defid);
		}
		else if ( subgeom->type == LINETYPE )
		{
			ptr += asx3d3_line_buf((LWLINE*)subgeom, 0, ptr, precision, opts, defid);
		}
		else if ( subgeom->type == POLYGONTYPE )
		{
			ptr += asx3d3_poly_buf((LWPOLY*)subgeom, 0, ptr, precision, opts, 0, defid);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			if ( subgeom->type == COLLECTIONTYPE )
				ptr += asx3d3_collection_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, opts, defid);
			else
				ptr += asx3d3_multi_buf((LWCOLLECTION*)subgeom, 0, ptr, precision, opts, defid);
		}
		else 
			lwerror("asx3d3_collection_buf: unknown geometry type");
			
		ptr += sprintf(ptr, "</%sgeometryMember>", defid);
	}

	/* Close outmost tag */
	ptr += sprintf(ptr, "</%sMultiGeometry>", defid);

	return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_collection(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
	char *x3d;
	size_t size;

	size = asx3d3_collection_size(col, srs, precision, opts, defid);
	x3d = lwalloc(size);
	asx3d3_collection_buf(col, srs, x3d, precision, opts, defid);
	return x3d;
}


/* In X3D3, coordinates are separated by a space separator
 */
static size_t
pointArray_toX3D3(POINTARRAY *pa, char *output, int precision, int opts, int type)
{
	int i;
	char *ptr;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char z[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if ( ! FLAGS_GET_Z(pa->flags) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			/** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
			if (!(type == POLYGONTYPE || type == TRIANGLETYPE) || i < (pa->npoints - 1) ){
				POINT2D pt;
				getPoint2d_p(pa, i, &pt);
	
				if (fabs(pt.x) < OUT_MAX_DOUBLE)
					sprintf(x, "%.*f", precision, pt.x);
				else
					sprintf(x, "%g", pt.x);
				trim_trailing_zeros(x);
	
				if (fabs(pt.y) < OUT_MAX_DOUBLE)
					sprintf(y, "%.*f", precision, pt.y);
				else
					sprintf(y, "%g", pt.y);
				trim_trailing_zeros(y);
	
				if ( i ) ptr += sprintf(ptr, " ");
				ptr += sprintf(ptr, "%s %s", x, y);
			}
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			/** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
			if (!(type == POLYGONTYPE || type == TRIANGLETYPE) || i < (pa->npoints - 1) ){
				POINT4D pt;
				getPoint4d_p(pa, i, &pt);
	
				if (fabs(pt.x) < OUT_MAX_DOUBLE)
					sprintf(x, "%.*f", precision, pt.x);
				else
					sprintf(x, "%g", pt.x);
				trim_trailing_zeros(x);
	
				if (fabs(pt.y) < OUT_MAX_DOUBLE)
					sprintf(y, "%.*f", precision, pt.y);
				else
					sprintf(y, "%g", pt.y);
				trim_trailing_zeros(y);
	
				if (fabs(pt.z) < OUT_MAX_DOUBLE)
					sprintf(z, "%.*f", precision, pt.z);
				else
					sprintf(z, "%g", pt.z);
				trim_trailing_zeros(z);
	
				if ( i ) ptr += sprintf(ptr, " ");
				
				ptr += sprintf(ptr, "%s %s %s", x, y, z);
			}
		}
	}

	return ptr-output;
}



/*
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_X3Dsize(POINTARRAY *pa, int precision)
{
	if (FLAGS_NDIMS(pa->flags) == 2)
		return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(" "))
		       * 2 * pa->npoints;

	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(" ")) * 3 * pa->npoints;
}