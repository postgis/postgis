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





//#define DEBUG

#include "wktparse.h"

Datum combine_box2d(PG_FUNCTION_ARGS);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS);
Datum postgis_uses_stats(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS);

// internal
char * lwgeom_summary_recursive(char *serialized, int offset);
int32 lwgeom_npoints_recursive(char *serialized);

// general utilities (might be moved in lwgeom_api.c)
double lwgeom_polygon_area(LWPOLY *poly);
double lwgeom_polygon_perimeter(LWPOLY *poly);
double lwgeom_polygon_perimeter2d(LWPOLY *poly);
double lwgeom_pointarray_length2d(POINTARRAY *pts);
double lwgeom_pointarray_length(POINTARRAY *pts);
void lwgeom_force2d_recursive(char *serialized, char *optr, int *retsize);
void lwgeom_force3d_recursive(char *serialized, char *optr, int *retsize);

/*------------------------------------------------------------------*/

//find the 2d length of the given POINTARRAY (even if it's 3d)
double lwgeom_pointarray_length2d(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;

	if ( pts->npoints < 2 ) return 0.0;
	for (i=0; i<pts->npoints-1;i++)
	{
		POINT2D *frm = (POINT2D *)getPoint(pts, i);
		POINT2D *to = (POINT2D *)getPoint(pts, i+1);
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) );
	}
	return dist;
}

//find the 3d/2d length of the given POINTARRAY (depending on its dimensions)
double lwgeom_pointarray_length(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;

	if ( pts->npoints < 2 ) return 0.0;

	// compute 2d length if 3d is not available
	if ( pts->ndims < 3 ) return lwgeom_pointarray_length2d(pts);

	for (i=0; i<pts->npoints-1;i++)
	{
		POINT3D *frm = (POINT3D *)getPoint(pts, i);
		POINT3D *to = (POINT3D *)getPoint(pts, i+1);
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) +
				((frm->z - to->z)*(frm->z - to->z) ) );
	}

	return dist;
}

//find the area of the outer ring - sum (area of inner rings)
// Could use a more numerically stable calculator...
double lwgeom_polygon_area(LWPOLY *poly)
{
	double poly_area=0.0;
	int i;

//elog(NOTICE,"in lwgeom_polygon_area (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
	{
		int j;
		POINTARRAY *ring = poly->rings[i];
		double ringarea = 0.0;

//elog(NOTICE," rings %d has %d points", i, ring->npoints);
		for (j=0; j<ring->npoints-1; j++)
    		{
			POINT2D *p1 = (POINT2D *)getPoint(ring, j);
			POINT2D *p2 = (POINT2D *)getPoint(ring, j+1);
			ringarea += ( p1->x * p2->y ) - ( p1->y * p2->x );
		}

		ringarea  /= 2.0;
//elog(NOTICE," ring 1 has area %lf",ringarea);
		ringarea  = fabs(ringarea );
		if (i != 0)	//outer
			ringarea  = -1.0*ringarea ; // its a hole

		poly_area += ringarea;
	}

	return poly_area;
}

// Compute the sum of polygon rings length.
// Could use a more numerically stable calculator...
double lwgeom_polygon_perimeter(LWPOLY *poly)
{
	double result=0.0;
	int i;

//elog(NOTICE,"in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length(poly->rings[i]);

	return result;
}

// Compute the sum of polygon rings length (forcing 2d computation).
// Could use a more numerically stable calculator...
double lwgeom_polygon_perimeter2d(LWPOLY *poly)
{
	double result=0.0;
	int i;

//elog(NOTICE,"in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length2d(poly->rings[i]);

	return result;
}


/*------------------------------------------------------------------*/

PG_FUNCTION_INFO_V1(combine_box2d);
Datum combine_box2d(PG_FUNCTION_ARGS)
{
	Pointer box2d_ptr = PG_GETARG_POINTER(0);
	Pointer geom_ptr = PG_GETARG_POINTER(1);
	BOX2DFLOAT4 *a,*b;
	char *lwgeom;
	BOX2DFLOAT4 box, *result;

	if  ( (box2d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL(); // combine_box2d(null,null) => null
	}

	result = (BOX2DFLOAT4 *)palloc(sizeof(BOX2DFLOAT4));

	if (box2d_ptr == NULL)
	{
		lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		box = getbox2d(lwgeom+4);
		memcpy(result, &box, sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	// combine_bbox(BOX3D, null) => BOX3D
	if (geom_ptr == NULL)
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX2DFLOAT4));
		PG_RETURN_POINTER(result);
	}

	//combine_bbox(BOX3D, geometry) => union(BOX3D, geometry->bvol)

	lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	box = getbox2d(lwgeom+4);

	a = (BOX2DFLOAT4 *)PG_GETARG_DATUM(0);
	b = &box;

	result->xmax = LWGEOM_Maxf(a->xmax, b->xmax);
	result->ymax = LWGEOM_Maxf(a->ymax, b->ymax);
	result->xmin = LWGEOM_Minf(a->xmin, b->xmin);
	result->ymin = LWGEOM_Minf(a->ymin, b->ymin);

	PG_RETURN_POINTER(result);
}

//find the size of geometry
PG_FUNCTION_INFO_V1(LWGEOM_mem_size);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 size = geom->size;
	int32 computed_size = lwgeom_seralizedformlength_simple(SERIALIZED_FORM(geom));
	computed_size += 4; // varlena size
	if ( size != computed_size )
	{
		elog(NOTICE, "varlena size (%d) != computed size+4 (%d)",
				size, computed_size);
	}

	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(size);
}

/*
 * Returns a palloced string containing summary for the serialized
 * LWGEOM object
 */
char *
lwgeom_summary_recursive(char *serialized, int offset)
{
	static int idx = 0;
	LWGEOM_INSPECTED *inspected;
	char *result;
	char *ptr;
	char tmp[100];
	int size;
	int32 j,i;

	size = 1;
	result = palloc(1);
	result[0] = '\0';

	if ( offset == 0 ) idx = 0;

	inspected = lwgeom_inspect(serialized);

	//now have to do a scan of each object
	for (j=0; j<inspected->ngeometries; j++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		char *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected,j);
		if (point !=NULL)
		{
			size += 30;
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POINT()\n",
				idx++);
			strcat(result,tmp);
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, j);
		if (poly !=NULL)
		{
			size += 57*(poly->nrings+1);
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POLYGON() with %i rings\n",
					idx++, poly->nrings);
			strcat(result,tmp);
			for (i=0; i<poly->nrings;i++)
			{
				sprintf(tmp,"     + ring %i has %i points\n",
					i, poly->rings[i]->npoints);
				strcat(result,tmp);
			}
			continue;
		}

		line = lwgeom_getline_inspected(inspected, j);
		if (line != NULL)
		{
			size += 57;
			result = repalloc(result,size);
			sprintf(tmp,
				"Object %i is a LINESTRING() with %i points\n",
				idx++, line->points->npoints);
			strcat(result,tmp);
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, j);
		if ( subgeom != NULL )
		{
			ptr = lwgeom_summary_recursive(subgeom, 1);
			size += strlen(ptr);
			result = repalloc(result,size);
			strcat(result, ptr);
			pfree(ptr);
		}
		else
		{
			elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}
	}

	pfree_inspected(inspected);
	return result;
}

//get summary info on a GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_summary);
Datum LWGEOM_summary(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	text *mytext;

	result = lwgeom_summary_recursive(SERIALIZED_FORM(geom), 0);

	// create a text obj to return
	mytext = (text *) palloc(VARHDRSZ  + strlen(result) );
	VARATT_SIZEP(mytext) = VARHDRSZ + strlen(result) ;
	memcpy(VARDATA(mytext) , result, strlen(result) );
	pfree(result);
	PG_RETURN_POINTER(mytext);
}

PG_FUNCTION_INFO_V1(postgis_lib_version);
Datum postgis_lib_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIB_VERSION;
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_scripts_released);
Datum postgis_scripts_released(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_SCRIPTS_VERSION;
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_uses_stats);
Datum postgis_uses_stats(PG_FUNCTION_ARGS)
{
#ifdef USE_STATS
	PG_RETURN_BOOL(TRUE);
#else
	PG_RETURN_BOOL(FALSE);
#endif
}

/*
 * Recursively count points in a SERIALIZED lwgeom
 */
int32
lwgeom_npoints_recursive(char *serialized)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(serialized);
	int i, j;
	int npoints=0;

	//now have to do a scan of each object
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		char *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected, i);
		if (point !=NULL)
		{
			npoints++;
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, i);
		if (poly !=NULL)
		{
			for (j=0; j<poly->nrings; j++)
			{
				npoints += poly->rings[j]->npoints;
			}
			continue;
		}

		line = lwgeom_getline_inspected(inspected, i);
		if (line != NULL)
		{
			npoints += line->points->npoints;
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom != NULL )
		{
			npoints += lwgeom_npoints_recursive(subgeom);
		}
		else
		{
	elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}
	}
	return npoints;
}

//number of points in an object
PG_FUNCTION_INFO_V1(LWGEOM_npoints);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 npoints = 0;

	npoints = lwgeom_npoints_recursive(SERIALIZED_FORM(geom));

	PG_RETURN_INT32(npoints);
}

// Calculate the area of all the subobj in a polygon
// area(point) = 0
// area (line) = 0
// area(polygon) = find its 2d area
PG_FUNCTION_INFO_V1(LWGEOM_area_polygon);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWPOLY *poly;
	double area = 0.0;
	int i;

//elog(NOTICE, "in LWGEOM_area_polygon");

	for (i=0; i<inspected->ngeometries; i++)
	{
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly == NULL ) continue;
		area += lwgeom_polygon_area(poly);
//elog(NOTICE, " LWGEOM_area_polygon found a poly (%f)", area);
	}
	
	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(area);
}

//find the "length of a geometry"
// length2d(point) = 0
// length2d(line) = length of line
// length2d(polygon) = 0  -- could make sense to return sum(ring perimeter)
// uses euclidian 2d length (even if input is 3d)
PG_FUNCTION_INFO_V1(LWGEOM_length2d_linestring);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

//elog(NOTICE, "in LWGEOM_length2d");

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length2d(line->points);
//elog(NOTICE, " LWGEOM_length2d found a line (%f)", dist);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(dist);
}

//find the "length of a geometry"
// length(point) = 0
// length(line) = length of line
// length(polygon) = 0  -- could make sense to return sum(ring perimeter)
// uses euclidian 3d/2d length depending on input dimensions.
PG_FUNCTION_INFO_V1(LWGEOM_length_linestring);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length(line->points);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(dist);
}

// find the "perimeter of a geometry"
// perimeter(point) = 0
// perimeter(line) = 0
// perimeter(polygon) = sum of ring perimeters
// uses euclidian 3d/2d computation depending on input dimension.
PG_FUNCTION_INFO_V1(LWGEOM_perimeter_poly);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	double ret = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly;
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly == NULL ) continue;
		ret += lwgeom_polygon_perimeter(poly);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(ret);
}

// find the "perimeter of a geometry"
// perimeter(point) = 0
// perimeter(line) = 0
// perimeter(polygon) = sum of ring perimeters
// uses euclidian 2d computation even if input is 3d
PG_FUNCTION_INFO_V1(LWGEOM_perimeter2d_poly);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	double ret = 0.0;
	int i;

	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly;
		poly = lwgeom_getpoly_inspected(inspected, i);
		if ( poly == NULL ) continue;
		ret += lwgeom_polygon_perimeter2d(poly);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(ret);
}


/*
 * Write to already allocated memory 'optr' a 2d version of
 * the given serialized form. 
 * Higher dimensions in input geometry are discarder.
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force2d_recursive(char *serialized, char *optr, int *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i;
	int totsize=0;
	int size=0;
	int type;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWPOLY *poly = NULL;
	char *loc;

		
#ifdef DEBUG
	elog(NOTICE, "lwgeom_force2d_recursive: call");
#endif

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		point->ndims = 2;
		lwpoint_serialize_buf(point, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a point, size:%d", *retsize);
#endif
		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);
		line->ndims = 2;
		lwline_serialize_buf(line, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a line, size:%d", *retsize);
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		poly->ndims = 2;
		lwpoly_serialize_buf(poly, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a poly, size:%d", *retsize);
#endif
		return;
	}

 	// OK, this is a collection, so we write down its metadata
	// first and then call us again

#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a collection (type:%d)", type);
#endif

	// Add type
	*optr = lwgeom_makeType_full(2, lwgeom_hasSRID(serialized[0]),
		type, lwgeom_hasBBOX(serialized[0]));
	optr++;
	totsize++;
	loc=serialized+1;

	// Add BBOX if any
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	// Add SRID if any
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;
	}

	// Add numsubobjects
	memcpy(optr, loc, 4);
	optr += 4;
	totsize += 4;

#ifdef DEBUG
elog(NOTICE, " collection header size:%d", totsize);
#endif

	// Now recurse for each suboject
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		char *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force2d_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;
#ifdef DEBUG
elog(NOTICE, " elem %d size: %d (tot: %d)", i, size, totsize);
#endif
	}
	pfree_inspected(inspected);

	*retsize = totsize;
}

/*
 * Write to already allocated memory 'optr' a 3d version of
 * the given serialized form. 
 * Higher dimensions in input geometry are discarder.
 * If the given version is 2d Z is set to 0.
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force3d_recursive(char *serialized, char *optr, int *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	int totsize=0;
	int size=0;
	int type;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	char *loc;

		
#ifdef DEBUG
	elog(NOTICE, "lwgeom_force3d_recursive: call");
#endif

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		if ( point->ndims < 3 )
		{
			newpts.ndims = 3;
			newpts.npoints = 1;
			newpts.serialized_pointlist = palloc(sizeof(POINT3D));
			loc = newpts.serialized_pointlist;
			getPoint3d_p(point->point, 0, loc);
			point->point = &newpts;
		}
		point->ndims = 3;
		lwpoint_serialize_buf(point, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3d_recursive: it's a point, size:%d", *retsize);
#endif
		return;
	}

	if ( type == LINETYPE )
	{
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3d_recursive: it's a line");
#endif
		line = lwline_deserialize(serialized);
		if ( line->ndims < 3 )
		{
			newpts.ndims = 3;
			newpts.npoints = line->points->npoints;
			newpts.serialized_pointlist = palloc(24*line->points->npoints);
			loc = newpts.serialized_pointlist;
			for (j=0; j<line->points->npoints; j++)
			{
				getPoint3d_p(line->points, j, loc);
				loc+=24;
			}
			line->points = &newpts;
		}

		line->ndims = 3;
		lwline_serialize_buf(line, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3d_recursive: it's a line, size:%d", *retsize);
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		if ( poly->ndims < 3 )
		{
			newpts.ndims = 3;
			newpts.npoints = 0;
			newpts.serialized_pointlist = palloc(1);
			nrings = palloc(sizeof(POINTARRAY *)*poly->nrings);
			loc = newpts.serialized_pointlist;
			for (j=0; j<poly->nrings; j++)
			{
				POINTARRAY *ring = poly->rings[j];
				POINTARRAY *nring = palloc(sizeof(POINTARRAY));
				nring->ndims = 2;
				nring->npoints = ring->npoints;
				nring->serialized_pointlist =
					palloc(ring->npoints*24);
				loc = nring->serialized_pointlist;
				for (k=0; k<ring->npoints; k++)
				{
					getPoint3d_p(ring, k, loc);
					loc+=24;
				}
				nrings[j] = nring;
			}
			poly->rings = nrings;
		}
		poly->ndims = 3;
		lwpoly_serialize_buf(poly, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3d_recursive: it's a poly, size:%d", *retsize);
#endif
		return;
	}

 	// OK, this is a collection, so we write down its metadata
	// first and then call us again

#ifdef DEBUG
elog(NOTICE, "lwgeom_force3d_recursive: it's a collection (type:%d)", type);
#endif

	// Add type
	*optr = lwgeom_makeType_full(3, lwgeom_hasSRID(serialized[0]),
		type, lwgeom_hasBBOX(serialized[0]));
	optr++;
	totsize++;
	loc=serialized+1;

	// Add BBOX if any
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	// Add SRID if any
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;
	}

	// Add numsubobjects
	memcpy(optr, loc, 4);
	optr += 4;
	totsize += 4;

#ifdef DEBUG
elog(NOTICE, " collection header size:%d", totsize);
#endif

	// Now recurse for each suboject
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		char *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force3d_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;
#ifdef DEBUG
elog(NOTICE, " elem %d size: %d (tot: %d)", i, size, totsize);
#endif
	}
	pfree_inspected(inspected);

	*retsize = totsize;
}

// transform input geometry to 2d if not 2d already
PG_FUNCTION_INFO_V1(LWGEOM_force_2d);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *result;
	int32 size = 0;

	// already 2d
	if ( lwgeom_ndims(geom->type) == 2 ) PG_RETURN_POINTER(geom);

	// allocate a larger for safety and simplicity
	result = (LWGEOM *) palloc(geom->size);

	lwgeom_force2d_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = repalloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to 3d if not 2d already
PG_FUNCTION_INFO_V1(LWGEOM_force_3d);
Datum LWGEOM_force_3d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *result;
	int olddims;
	int32 size = 0;

	olddims = lwgeom_ndims(geom->type);
	
	// already 3d
	if ( olddims == 3 ) PG_RETURN_POINTER(geom);

	if ( olddims > 3 ) {
		result = (LWGEOM *) palloc(geom->size);
	} else {
		// allocate double as memory a larger for safety 
		result = (LWGEOM *) palloc(geom->size*1.5);
	}

	lwgeom_force3d_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = repalloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to a collection type
PG_FUNCTION_INFO_V1(LWGEOM_force_collection);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS)
{
	LWGEOM *geom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *result;
	int oldtype;
	int32 size = 0;
	char *iptr, *optr;
	int32 nsubgeoms = 1;

	oldtype = lwgeom_getType(geom->type);
	
	// already a collection
	if ( oldtype == COLLECTIONTYPE ) PG_RETURN_POINTER(geom);

	// alread a multi*, just make it a collection
	if ( oldtype > 3 )
	{
		result = (LWGEOM *)palloc(geom->size);
		result->size = geom->size;
		result->type = TYPE_SETTYPE(geom->type, COLLECTIONTYPE);
		memcpy(result->data, geom->data, geom->size-5);
		PG_RETURN_POINTER(result);
	}

	// not a multi*, must add header and 
	// transfer eventual BBOX and SRID to first object

	size = geom->size+5; // 4 for numgeoms, 1 for type

	result = (LWGEOM *)palloc(size); // 4 for numgeoms, 1 for type
	result->size = size;

	result->type = TYPE_SETTYPE(geom->type, COLLECTIONTYPE);
	iptr = geom->data;
	optr = result->data;

	// reset size to bare serialized input
	size = geom->size - 4;

	// transfer bbox
	if ( lwgeom_hasBBOX(geom->type) )
	{
		memcpy(optr, iptr, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		iptr += sizeof(BOX2DFLOAT4);
		size -= sizeof(BOX2DFLOAT4);
	}

	// transfer SRID
	if ( lwgeom_hasSRID(geom->type) )
	{
		memcpy(optr, iptr, 4);
		optr += 4;
		iptr += 4;
		size -= 4;
	}

	// write number of geometries (1)
	memcpy(optr, &nsubgeoms, 4);
	optr+=4;

	// write type of first geometry w/out BBOX and SRID
	optr[0] = TYPE_SETHASSRID(geom->type, 0);
	optr[0] = TYPE_SETHASBBOX(optr[0], 0);
	optr++;

	// write remaining stuff
	memcpy(optr, iptr, size);

	PG_RETURN_POINTER(result);
}
