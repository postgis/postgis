#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "profile.h"

//#define DEBUG

Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS);
Datum postgis_uses_stats(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_translate(PG_FUNCTION_ARGS);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS);
Datum LWGEOM_collect(PG_FUNCTION_ARGS);
Datum LWGEOM_accum(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_expand(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS);
Datum LWGEOM_forceRHR_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_noop(PG_FUNCTION_ARGS);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);


/*------------------------------------------------------------------*/

//find the size of geometry
PG_FUNCTION_INFO_V1(LWGEOM_mem_size);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 size = geom->size;
	int32 computed_size = lwgeom_size(SERIALIZED_FORM(geom));
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
 * Translate a pointarray.
 */
void
lwgeom_translate_ptarray(POINTARRAY *pa, double xoff, double yoff, double zoff)
{
	int i;

	if ( TYPE_HASZ(pa->dims) )
	{
		for (i=0; i<pa->npoints; i++) {
			POINT3DZ *p = (POINT3DZ *)getPoint(pa, i);
			p->x += xoff;
			p->y += yoff;
			p->z += zoff;
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++) {
			POINT2D *p = (POINT2D *)getPoint(pa, i);
			p->x += xoff;
			p->y += yoff;
		}
	}
}

void
lwgeom_translate_recursive(char *serialized,
	double xoff, double yoff, double zoff)
{
	LWGEOM_INSPECTED *inspected;
	int i, j;

	inspected = lwgeom_inspect(serialized);

	// scan each object translating it
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		char *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected, i);
		if (point !=NULL)
		{
			lwgeom_translate_ptarray(point->point,
				xoff, yoff, zoff);
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, i);
		if (poly !=NULL)
		{
			for (j=0; j<poly->nrings; j++)
			{
				lwgeom_translate_ptarray(poly->rings[j],
					xoff, yoff, zoff);
			}
			continue;
		}

		line = lwgeom_getline_inspected(inspected, i);
		if (line != NULL)
		{
			lwgeom_translate_ptarray(line->points,
				xoff, yoff, zoff);
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		if ( subgeom == NULL )
		{
			elog(ERROR, "lwgeom_getsubgeometry_inspected returned NULL??");
		}
		lwgeom_translate_recursive(subgeom, xoff, yoff, zoff);
	}

	pfree_inspected(inspected);
}

//get summary info on a GEOMETRY
PG_FUNCTION_INFO_V1(LWGEOM_summary);
Datum LWGEOM_summary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	text *mytext;
	LWGEOM *lwgeom;

	init_pg_func();

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	result = lwgeom_summary(lwgeom, 0);

	// create a text obj to return
	mytext = (text *) lwalloc(VARHDRSZ  + strlen(result) + 1);
	VARATT_SIZEP(mytext) = VARHDRSZ + strlen(result) + 1;
	VARDATA(mytext)[0] = '\n';
	memcpy(VARDATA(mytext)+1, result, strlen(result) );
	lwfree(result);
	PG_RETURN_POINTER(mytext);
}

PG_FUNCTION_INFO_V1(postgis_lib_version);
Datum postgis_lib_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIB_VERSION;
	text *result;
	result = (text *) lwalloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_scripts_released);
Datum postgis_scripts_released(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_SCRIPTS_VERSION;
	text *result;
	result = (text *) lwalloc(VARHDRSZ  + strlen(ver));
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
lwgeom_npoints(char *serialized)
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
			npoints += lwgeom_npoints(subgeom);
		}
		else
		{
	elog(ERROR, "What ? lwgeom_getsubgeometry_inspected returned NULL??");
		}
	}
	return npoints;
}

/*
 * Recursively count rings in a SERIALIZED lwgeom
 */
int32
lwgeom_nrings_recursive(char *serialized)
{
	LWGEOM_INSPECTED *inspected;
	int i;
	int nrings=0;

	inspected = lwgeom_inspect(serialized);

	//now have to do a scan of each object
	for (i=0; i<inspected->ngeometries; i++)
	{
		LWPOLY *poly=NULL;
		char *subgeom=NULL;

		subgeom = lwgeom_getsubgeometry_inspected(inspected, i);

		if ( lwgeom_getType(subgeom[0]) == POLYGONTYPE )
		{
			poly = lwpoly_deserialize(subgeom);
			nrings += poly->nrings;
			continue;
		}

		if ( lwgeom_getType(subgeom[0]) == COLLECTIONTYPE )
		{
			nrings += lwgeom_nrings_recursive(subgeom);
			continue;
		}
	}

	pfree_inspected(inspected);

	return nrings;
}

//number of points in an object
PG_FUNCTION_INFO_V1(LWGEOM_npoints);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 npoints = 0;

	npoints = lwgeom_npoints(SERIALIZED_FORM(geom));

	PG_RETURN_INT32(npoints);
}

//number of rings in an object
PG_FUNCTION_INFO_V1(LWGEOM_nrings);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 nrings = 0;

	nrings = lwgeom_nrings_recursive(SERIALIZED_FORM(geom));

	PG_RETURN_INT32(nrings);
}

// Calculate the area of all the subobj in a polygon
// area(point) = 0
// area (line) = 0
// area(polygon) = find its 2d area
PG_FUNCTION_INFO_V1(LWGEOM_area_polygon);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
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
lwgeom_force2d_recursive(char *serialized, char *optr, size_t *retsize)
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
		TYPE_SETZM(point->type, 0, 0);
		lwpoint_serialize_buf(point, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a point, size:%d", *retsize);
#endif
		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);
		TYPE_SETZM(line->type, 0, 0);
		lwline_serialize_buf(line, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force2d_recursive: it's a line, size:%d", *retsize);
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(poly->type, 0, 0);
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
	*optr = lwgeom_makeType_full(0, 0, lwgeom_hasSRID(serialized[0]),
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
 * Write to already allocated memory 'optr' a 3dz version of
 * the given serialized form. 
 * Higher dimensions in input geometry are discarder.
 * If the given version is 2d Z is set to 0.
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force3dz_recursive(char *serialized, char *optr, size_t *retsize)
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
	elog(NOTICE, "lwgeom_force3dz_recursive: call");
#endif

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DZ));
		loc = newpts.serialized_pointlist;
		getPoint3dz_p(point->point, 0, (POINT3DZ *)loc);
		point->point = &newpts;
		TYPE_SETZM(point->type, 1, 0);
		lwpoint_serialize_buf(point, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dz_recursive: it's a point, size:%d", *retsize);
#endif
		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dz_recursive: it's a line");
#endif
		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DZ)*line->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint3dz_p(line->points, j, (POINT3DZ *)loc);
			loc+=sizeof(POINT3DZ);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 1, 0);
		lwline_serialize_buf(line, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dz_recursive: it's a line, size:%d", *retsize);
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 0);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 1, 0);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
				lwalloc(ring->npoints*sizeof(POINT3DZ));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint3dz_p(ring, k, (POINT3DZ *)loc);
				loc+=sizeof(POINT3DZ);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 1, 0);
		lwpoly_serialize_buf(poly, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dz_recursive: it's a poly, size:%d", *retsize);
#endif
		return;
	}

 	// OK, this is a collection, so we write down its metadata
	// first and then call us again

#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dz_recursive: it's a collection (type:%d)", type);
#endif

	// Add type
	*optr = lwgeom_makeType_full(1, 0, lwgeom_hasSRID(serialized[0]),
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
		lwgeom_force3dz_recursive(subgeom, optr, &size);
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
 * Write to already allocated memory 'optr' a 3dm version of
 * the given serialized form. 
 * Higher dimensions in input geometry are discarder.
 * If the given version is 2d M is set to 0.
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force3dm_recursive(unsigned char *serialized, char *optr, size_t *retsize)
{
	LWGEOM_INSPECTED *inspected;
	int i,j,k;
	int totsize=0;
	int size=0;
	int type;
	unsigned char newtypefl;
	LWPOINT *point = NULL;
	LWLINE *line = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY newpts;
	POINTARRAY **nrings;
	POINT3DM *p3dm;
	char *loc;
	char check;

		
#ifdef DEBUG
	elog(NOTICE, "lwgeom_force3dm_recursive: call");
#endif

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DM));
		loc = newpts.serialized_pointlist;
		p3dm = (POINT3DM *)loc;
		getPoint3dm_p(point->point, 0, p3dm);
		point->point = &newpts;
		TYPE_SETZM(point->type, 0, 1);
		lwpoint_serialize_buf(point, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(point);

#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive returning");
#endif
		return;
	}

	if ( type == LINETYPE )
	{
		line = lwline_deserialize(serialized);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dm_recursive: it's a line with %d points", line->points->npoints);
#endif
		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT3DM)*line->points->npoints);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force3dm_recursive: %d bytes pointlist allocated", sizeof(POINT3DM)*line->points->npoints);
#endif

		loc = newpts.serialized_pointlist;
		check = TYPE_NDIMS(line->points->dims);
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint3dm_p(line->points, j, (POINT3DM *)loc);
			if ( check != TYPE_NDIMS(line->points->dims) )
			{
				lwerror("getPoint3dm_p messed with input pointarray");
				return;
			}
			loc+=sizeof(POINT3DM);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 0, 1);
		lwline_serialize_buf(line, optr, retsize);
		lwfree(newpts.serialized_pointlist);
		lwfree(line);

#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive returning");
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 0, 1);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 0, 1);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
				lwalloc(ring->npoints*sizeof(POINT3DM));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint3dm_p(ring, k, (POINT3DM *)loc);
				loc+=sizeof(POINT3DM);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 0, 1);
		lwpoly_serialize_buf(poly, optr, retsize);
		lwfree(poly);
		// TODO: free nrigs[*]->serialized_pointlist

#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive returning");
#endif
		return;
	}

	if ( type != MULTIPOINTTYPE && type != MULTIPOLYGONTYPE &&
		type != MULTILINETYPE && type != COLLECTIONTYPE )
	{
		lwerror("lwgeom_force3dm_recursive: unknown geometry: %d",
			type);
	}

 	// OK, this is a collection, so we write down its metadata
	// first and then call us again

#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive: it's a collection (%s)", lwgeom_typename(type));
#endif


	// Add type
	newtypefl = lwgeom_makeType_full(0, 1, lwgeom_hasSRID(serialized[0]),
		type, lwgeom_hasBBOX(serialized[0]));
	optr[0] = newtypefl;
	optr++;
	totsize++;
	loc=serialized+1;

#ifdef DEBUG
	lwnotice("lwgeom_force3dm_recursive: added collection type (%s[%s]) - size:%d", lwgeom_typename(type), lwgeom_typeflags(newtypefl), totsize);
#endif

	if ( lwgeom_hasBBOX(serialized[0]) != lwgeom_hasBBOX(newtypefl) )
		lwerror("typeflag mismatch in BBOX");
	if ( lwgeom_hasSRID(serialized[0]) != lwgeom_hasSRID(newtypefl) )
		lwerror("typeflag mismatch in SRID");

	// Add BBOX if any
	if (lwgeom_hasBBOX(serialized[0]))
	{
		memcpy(optr, loc, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
		totsize += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
#ifdef DEBUG
		lwnotice("lwgeom_force3dm_recursive: added collection bbox - size:%d", totsize);
#endif
	}

	// Add SRID if any
	if (lwgeom_hasSRID(serialized[0]))
	{
		memcpy(optr, loc, 4);
		optr += 4;
		totsize += 4;
		loc += 4;
#ifdef DEBUG
		lwnotice("lwgeom_force3dm_recursive: added collection SRID - size:%d", totsize);
#endif
	}

	// Add numsubobjects
	memcpy(optr, loc, sizeof(uint32));
	optr += sizeof(uint32);
	totsize += sizeof(uint32);
	loc += sizeof(uint32);
#ifdef DEBUG
	lwnotice("lwgeom_force3dm_recursive: added collection ngeoms - size:%d", totsize);
#endif

#ifdef DEBUG
	lwnotice("lwgeom_force3dm_recursive: inspecting subgeoms");
#endif
	// Now recurse for each subobject
	inspected = lwgeom_inspect(serialized);
	for (i=0; i<inspected->ngeometries; i++)
	{
		char *subgeom = lwgeom_getsubgeometry_inspected(inspected, i);
		lwgeom_force3dm_recursive(subgeom, optr, &size);
		totsize += size;
		optr += size;
#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive: added elem %d size: %d (tot: %d)",
	i, size, totsize);
#endif
	}
	pfree_inspected(inspected);

#ifdef DEBUG
lwnotice("lwgeom_force3dm_recursive returning");
#endif

	if ( retsize ) *retsize = totsize;
}


/*
 * Write to already allocated memory 'optr' a 4d version of
 * the given serialized form. 
 * Pad dimensions are set to 0 (this might be z, m or both).
 * Return number bytes written in given int pointer.
 */
void
lwgeom_force4d_recursive(char *serialized, char *optr, size_t *retsize)
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
	elog(NOTICE, "lwgeom_force4d_recursive: call");
#endif

	type = lwgeom_getType(serialized[0]);

	if ( type == POINTTYPE )
	{
		point = lwpoint_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = 1;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT4D));
		loc = newpts.serialized_pointlist;
		getPoint4d_p(point->point, 0, (POINT4D *)loc);
		point->point = &newpts;
		TYPE_SETZM(point->type, 1, 1);
		lwpoint_serialize_buf(point, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force4d_recursive: it's a point, size:%d", *retsize);
#endif
		return;
	}

	if ( type == LINETYPE )
	{
#ifdef DEBUG
elog(NOTICE, "lwgeom_force4d_recursive: it's a line");
#endif
		line = lwline_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = line->points->npoints;
		newpts.serialized_pointlist = lwalloc(sizeof(POINT4D)*line->points->npoints);
		loc = newpts.serialized_pointlist;
		for (j=0; j<line->points->npoints; j++)
		{
			getPoint4d_p(line->points, j, (POINT4D *)loc);
			loc+=sizeof(POINT4D);
		}
		line->points = &newpts;
		TYPE_SETZM(line->type, 1, 1);
		lwline_serialize_buf(line, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force4d_recursive: it's a line, size:%d", *retsize);
#endif
		return;
	}

	if ( type == POLYGONTYPE )
	{
		poly = lwpoly_deserialize(serialized);
		TYPE_SETZM(newpts.dims, 1, 1);
		newpts.npoints = 0;
		newpts.serialized_pointlist = lwalloc(1);
		nrings = lwalloc(sizeof(POINTARRAY *)*poly->nrings);
		loc = newpts.serialized_pointlist;
		for (j=0; j<poly->nrings; j++)
		{
			POINTARRAY *ring = poly->rings[j];
			POINTARRAY *nring = lwalloc(sizeof(POINTARRAY));
			TYPE_SETZM(nring->dims, 1, 1);
			nring->npoints = ring->npoints;
			nring->serialized_pointlist =
				lwalloc(ring->npoints*sizeof(POINT4D));
			loc = nring->serialized_pointlist;
			for (k=0; k<ring->npoints; k++)
			{
				getPoint4d_p(ring, k, (POINT4D *)loc);
				loc+=sizeof(POINT4D);
			}
			nrings[j] = nring;
		}
		poly->rings = nrings;
		TYPE_SETZM(poly->type, 1, 1);
		lwpoly_serialize_buf(poly, optr, retsize);
#ifdef DEBUG
elog(NOTICE, "lwgeom_force4d_recursive: it's a poly, size:%d", *retsize);
#endif
		return;
	}

 	// OK, this is a collection, so we write down its metadata
	// first and then call us again

#ifdef DEBUG
elog(NOTICE, "lwgeom_force4d_recursive: it's a collection (type:%d)", type);
#endif

	// Add type
	*optr = lwgeom_makeType_full(
		1, 1,
		lwgeom_hasSRID(serialized[0]),
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
		lwgeom_force4d_recursive(subgeom, optr, &size);
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
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int32 size = 0;

	// already 2d
	if ( lwgeom_ndims(geom->type) == 2 ) PG_RETURN_POINTER(geom);

	// allocate a larger for safety and simplicity
	result = (PG_LWGEOM *) lwalloc(geom->size);

	lwgeom_force2d_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = lwrealloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to 3dz if not 3dz already
PG_FUNCTION_INFO_V1(LWGEOM_force_3dz);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int olddims;
	int32 size = 0;

	olddims = lwgeom_ndims(geom->type);
	
	// already 3d
	if ( olddims == 3 && TYPE_HASZ(geom->type) ) PG_RETURN_POINTER(geom);

	if ( olddims > 3 ) {
		result = (PG_LWGEOM *) lwalloc(geom->size);
	} else {
		// allocate double as memory a larger for safety 
		result = (PG_LWGEOM *) lwalloc(geom->size*1.5);
	}

	lwgeom_force3dz_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = lwrealloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to 3dm if not 3dm already
PG_FUNCTION_INFO_V1(LWGEOM_force_3dm);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int olddims;
	size_t size = 0;

	olddims = lwgeom_ndims(geom->type);
	
	// already 3dm
	if ( olddims == 3 && TYPE_HASM(geom->type) ) PG_RETURN_POINTER(geom);

	if ( olddims > 3 ) {
		size = geom->size;
	} else {
		// allocate double as memory a larger for safety 
		size = geom->size * 1.5;
	}
	result = (PG_LWGEOM *)lwalloc(size);

#ifdef DEBUG
	lwnotice("LWGEOM_force_3dm: allocated %d bytes for result", size);
#endif

	lwgeom_force3dm_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

#ifdef DEBUG
	lwnotice("LWGEOM_force_3dm: lwgeom_force3dm_recursive returned a %d sized geom", size);
#endif

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = lwrealloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to 4d if not 4d already
PG_FUNCTION_INFO_V1(LWGEOM_force_4d);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int olddims;
	int32 size = 0;

	olddims = lwgeom_ndims(geom->type);
	
	// already 4d
	if ( olddims == 4 ) PG_RETURN_POINTER(geom);

	// allocate double as memory a larger for safety 
	result = (PG_LWGEOM *) lwalloc(geom->size*2);

	lwgeom_force4d_recursive(SERIALIZED_FORM(geom),
		SERIALIZED_FORM(result), &size);

	// we can safely avoid this... memory will be freed at
	// end of query processing anyway.
	//result = lwrealloc(result, size+4);

	result->size = size+4;

	PG_RETURN_POINTER(result);
}

// transform input geometry to a collection type
PG_FUNCTION_INFO_V1(LWGEOM_force_collection);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
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
		result = (PG_LWGEOM *)lwalloc(geom->size);
		result->size = geom->size;
		result->type = geom->type;
		TYPE_SETTYPE(result->type, COLLECTIONTYPE);
		memcpy(result->data, geom->data, geom->size-5);
		PG_RETURN_POINTER(result);
	}

	// not a multi*, must add header and 
	// transfer eventual BBOX and SRID to first object

	size = geom->size+5; // 4 for numgeoms, 1 for type

	result = (PG_LWGEOM *)lwalloc(size); // 4 for numgeoms, 1 for type
	result->size = size;

	result->type = geom->type;
	TYPE_SETTYPE(result->type, COLLECTIONTYPE);
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
	optr[0] = geom->type;
	TYPE_SETHASSRID(optr[0], 0);
	TYPE_SETHASBBOX(optr[0], 0);
	optr++;

	// write remaining stuff
	memcpy(optr, iptr, size);

	PG_RETURN_POINTER(result);
}

// transform input geometry to a multi* type
PG_FUNCTION_INFO_V1(LWGEOM_force_multi);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	int oldtype, newtype;
	int32 size = 0;
	char *iptr, *optr;
	int32 nsubgeoms = 1;

	oldtype = lwgeom_getType(geom->type);
	
	// already a multi
	if ( oldtype >= 4 ) PG_RETURN_POINTER(geom);

	// not a multi*, must add header and 
	// transfer eventual BBOX and SRID to first object

	newtype = oldtype+3; // see defines

	size = geom->size+5; // 4 for numgeoms, 1 for type

	result = (PG_LWGEOM *)lwalloc(size); // 4 for numgeoms, 1 for type
	result->size = size;

	result->type = geom->type;
	TYPE_SETTYPE(result->type, newtype);
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
	optr[0] = geom->type;
	TYPE_SETHASSRID(optr[0], 0);
	TYPE_SETHASBBOX(optr[0], 0);
	optr++;

	// write remaining stuff
	memcpy(optr, iptr, size);

	PG_RETURN_POINTER(result);
}

// Minimum 2d distance between objects in geom1 and geom2.
PG_FUNCTION_INFO_V1(LWGEOM_mindistance2d);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	double mindist;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance2d_recursive(SERIALIZED_FORM(geom1),
		SERIALIZED_FORM(geom2));

#ifdef PROFILE
	profstop(PROF_QRUN);
	profreport("dist",geom1, geom2, NULL);
#endif

	PG_RETURN_FLOAT8(mindist);
}

// Maximum 2d distance between linestrings.
// Returns NULL if given geoms are not linestrings.
// This is very bogus (or I'm missing its meaning)
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance2d_linestring);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS)
{

	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	LWLINE *line1;
	LWLINE *line2;
	double maxdist = 0;
	int i;

	elog(ERROR, "This function is unimplemented yet");
	PG_RETURN_NULL();

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	line1 = lwline_deserialize(SERIALIZED_FORM(geom1));
	if ( line1 == NULL ) PG_RETURN_NULL(); // not a linestring

	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	line2 = lwline_deserialize(SERIALIZED_FORM(geom2));
	if ( line2 == NULL ) PG_RETURN_NULL(); // not a linestring

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	for (i=0; i<line1->points->npoints; i++)
	{
		POINT2D *p = (POINT2D *)getPoint(line1->points, i);
		double dist = distance2d_pt_ptarray(p, line2->points);

		if (dist > maxdist) maxdist = dist;
	}

	PG_RETURN_FLOAT8(maxdist);
}

//translate geometry
PG_FUNCTION_INFO_V1(LWGEOM_translate);
Datum LWGEOM_translate(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	double xoff =  PG_GETARG_FLOAT8(1);
	double yoff =  PG_GETARG_FLOAT8(2);
	double zoff =  PG_GETARG_FLOAT8(3);

	lwgeom_translate_recursive(SERIALIZED_FORM(geom), xoff, yoff, zoff);

	PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(LWGEOM_inside_circle_point);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	double cx = PG_GETARG_FLOAT8(1);
	double cy = PG_GETARG_FLOAT8(2);
	double rr = PG_GETARG_FLOAT8(3);
	LWPOINT *point;
	POINT2D *pt;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	point = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( point == NULL ) PG_RETURN_NULL(); // not a point

	pt = (POINT2D *)getPoint(point->point, 0);

	PG_RETURN_BOOL(lwgeom_pt_inside_circle(pt, cx, cy, rr));
}

void
dump_lwexploded(LWGEOM_EXPLODED *exploded)
{
	int i;

	elog(NOTICE, "SRID=%d ndims=%d", exploded->SRID,
		TYPE_NDIMS(exploded->dims));
	elog(NOTICE, "%d points, %d lines, %d polygons",
		exploded->npoints, exploded->nlines, exploded->npolys);

	for (i=0; i<exploded->npoints; i++)
	{
		elog(NOTICE, "Point%d @ %p", i, exploded->points[i]);
		if ( (int)exploded->points[i] < 100 )
		{
			elog(ERROR, "dirty point pointer");
		}
	}

	for (i=0; i<exploded->nlines; i++)
	{
		elog(NOTICE, "Line%d @ %p", i, exploded->lines[i]);
		if ( (int)exploded->lines[i] < 100 )
		{
			elog(ERROR, "dirty line pointer");
		}
	}

	for (i=0; i<exploded->npolys; i++)
	{
		elog(NOTICE, "Poly%d @ %p", i, exploded->polys[i]);
		if ( (int)exploded->polys[i] < 100 )
		{
			elog(ERROR, "dirty poly pointer");
		}
	}

}

// collect( geom, geom ) returns a geometry which contains
// all the sub_objects from both of the argument geometries
// returned geometry is the simplest possible, based on the types
// of the collected objects
// ie. if all are of either X or multiX, then a multiX is returned.
PG_FUNCTION_INFO_V1(LWGEOM_collect);
Datum LWGEOM_collect(PG_FUNCTION_ARGS)
{
	elog(ERROR, "memcollect() is obsoleted, use collect() instead");
	PG_RETURN_NULL();
}

/*
 * This is a geometry array constructor
 * for use as aggregates sfunc.
 * Will have as input an array of Geometry pointers and a Geometry.
 * Will DETOAST given geometry and put a pointer to it
 * in the given array. DETOASTED value is first copied
 * to a safe memory context to avoid premature deletion.
 */
PG_FUNCTION_INFO_V1(LWGEOM_accum);
Datum LWGEOM_accum(PG_FUNCTION_ARGS)
{
	ArrayType *array = NULL;
	int nelems, nbytes;
	Datum datum;
	PG_LWGEOM *geom;
	ArrayType *result;
	Pointer **pointers;
	MemoryContext oldcontext;

//elog(NOTICE, "LWGEOM_accum called");

	datum = PG_GETARG_DATUM(0);
	if ( (Pointer *)datum == NULL ) {
		array = NULL;
		nelems = 0;
		//elog(NOTICE, "geom_accum: NULL array, nelems=%d", nelems);
	} else {
		array = (ArrayType *) PG_DETOAST_DATUM_COPY(datum);
		nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
	}

	datum = PG_GETARG_DATUM(1);
	// Do nothing, return state array
	if ( (Pointer *)datum == NULL )
	{
		//elog(NOTICE, "geom_accum: NULL geom, nelems=%d", nelems);
		PG_RETURN_ARRAYTYPE_P(array);
	}

	/*
	 * Switch to * flinfo->fcinfo->fn_mcxt
	 * memory context to be sure both detoasted
	 * geometry AND array of pointers to it
	 * last till the call to unite_finalfunc.
	 */
	oldcontext = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);

	/* Make a DETOASTED copy of input geometry */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(datum);

	//elog(NOTICE, "geom_accum: adding %p (nelems=%d)", geom, nelems);

	/*
	 * Might use a more optimized version instead of lwrealloc'ing
	 * at every iteration. This is not the bottleneck anyway.
	 * 		--strk(TODO);
	 */
	++nelems;
	nbytes = ARR_OVERHEAD(1) + sizeof(Pointer *) * nelems;
	if ( ! array ) {
		result = (ArrayType *) lwalloc(nbytes);
		result->size = nbytes;
		result->ndim = 1;
		*((int *) ARR_DIMS(result)) = nelems;
	} else {
		result = (ArrayType *) lwrealloc(array, nbytes);
		result->size = nbytes;
		result->ndim = 1;
		*((int *) ARR_DIMS(result)) = nelems;
	}

	pointers = (Pointer **)ARR_DATA_PTR(result);
	pointers[nelems-1] = (Pointer *)geom;

	/* Go back to previous memory context */
	MemoryContextSwitchTo(oldcontext);


	PG_RETURN_ARRAYTYPE_P(result);

}

/*
 * collect_garray ( GEOMETRY[] ) returns a geometry which contains
 * all the sub_objects from all of the geometries in given array.
 *
 * returned geometry is the simplest possible, based on the types
 * of the collected objects
 * ie. if all are of either X or multiX, then a multiX is returned
 * bboxonly types are treated as null geometries (no sub_objects)
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect_garray);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	PG_LWGEOM **geoms;
	PG_LWGEOM *result=NULL;
	LWGEOM **lwgeoms, *outlwg;
	size_t size;
	unsigned int outtype;
	int i;

//elog(NOTICE, "LWGEOM_collect_garray called");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/* Get pointer to GEOMETRY pointers array */
	geoms = (PG_LWGEOM **)ARR_DATA_PTR(array);

	/*
	 * Deserialize all geometries in array into the lwgeoms pointers
	 * array. Check input types to form output type.
	 */
	lwgeoms = palloc(sizeof(LWGEOM *)*nelems);
	outtype = 0;
	for (i=0; i<nelems; i++)
	{
		unsigned int intype = TYPE_GETTYPE(geoms[i]->type);
		lwgeoms[i] = lwgeom_deserialize(SERIALIZED_FORM(geoms[i]));

		// Output type not initialized
		if ( ! outtype ) {
			// Input is single, make multi
			if ( outtype < 4 ) outtype = intype+3;
			// Input is multi, make collection
			else outtype = COLLECTIONTYPE;
		}

		// Input type not compatible with output
		// make output type a collection
		else if ( outtype != COLLECTIONTYPE && intype != outtype-3 )
		{
			outtype = COLLECTIONTYPE;
		}

	}

	outlwg = (LWGEOM *)lwcollection_construct(
		outtype,
		lwgeom_getSRID(geoms[0]),
		NULL, nelems, lwgeoms);

	size = lwgeom_serialize_size(outlwg);
	//lwnotice("lwgeom_serialize_size returned %d", size);
	result = palloc(size+4);
	result->size = (size+4);
	lwgeom_serialize_buf(outlwg, SERIALIZED_FORM(result), &size);
	if ( size != result->size-4 )
	{
		lwerror("lwgeom_serialize size:%d, lwgeom_serialize_size:%d",
			size, result->size-4);
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

/*
 * makeline_garray ( GEOMETRY[] ) returns a LINE formed by
 * all the point geometries in given array.
 * array elements that are NOT points are discarded..
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline_garray);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	PG_LWGEOM **geoms;
	PG_LWGEOM *result=NULL;
	LWPOINT **lwpoints;
	LWGEOM *outlwg;
	size_t size;
	unsigned int npoints;
	int i;

//elog(NOTICE, "LWGEOM_makeline_garray called");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = (ArrayType *) PG_DETOAST_DATUM(datum);

	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/* Get pointer to GEOMETRY pointers array */
	geoms = (PG_LWGEOM **)ARR_DATA_PTR(array);

	/*
	 * Deserialize all point geometries in array into the
	 * lwpoints pointers array.
	 * Count actual number of points.
	 */
	
	// possibly more then required
	lwpoints = palloc(sizeof(LWGEOM *)*nelems);
	npoints = 0;
	for (i=0; i<nelems; i++)
	{
		if ( TYPE_GETTYPE(geoms[i]->type) != POINTTYPE ) continue;
		lwpoints[npoints++] =
			lwpoint_deserialize(SERIALIZED_FORM(geoms[i]));
	}

	/* Return null on 0-points input array */
	if ( npoints == 0 )
	{
		elog(NOTICE, "No points in input array");
		PG_RETURN_NULL();
	}

	outlwg = (LWGEOM *)make_lwline(-1, npoints, lwpoints);

	size = lwgeom_serialize_size(outlwg);
	//lwnotice("lwgeom_serialize_size returned %d", size);
	result = palloc(size+4);
	result->size = (size+4);
	lwgeom_serialize_buf(outlwg, SERIALIZED_FORM(result), &size);
	if ( size != result->size-4 )
	{
		lwerror("lwgeom_serialize size:%d, lwgeom_serialize_size:%d",
			size, result->size-4);
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

// makes a polygon of the expanded features bvol - 1st point = LL 3rd=UR
// 2d only. (3d might be worth adding).
// create new geometry of type polygon, 1 ring, 5 points
PG_FUNCTION_INFO_V1(LWGEOM_expand);
Datum LWGEOM_expand(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double d = PG_GETARG_FLOAT8(1);
	BOX2DFLOAT4 box;
	POINT2D *pts = lwalloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	int SRID;
	PG_LWGEOM *result;
	char *ser;

	// get geometry box 
	if ( ! getbox2d_p(SERIALIZED_FORM(geom), &box) )
	{
		// must be an EMPTY geometry
		PG_RETURN_POINTER(geom);
	}

	// get geometry SRID
	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));

	// expand it
	expand_box2d(&box, d);

	// Assign coordinates to POINT2D array
	pts[0].x = box.xmin; pts[0].y = box.ymin;
	pts[1].x = box.xmin; pts[1].y = box.ymax;
	pts[2].x = box.xmax; pts[2].y = box.ymax;
	pts[3].x = box.xmax; pts[3].y = box.ymin;
	pts[4].x = box.xmin; pts[4].y = box.ymin;

	// Construct point array
	pa[0] = lwalloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (char *)pts;
	TYPE_SETZM(pa[0]->dims, 0, 0);
	pa[0]->npoints = 5;

	// Construct polygon 
	poly = lwpoly_construct(SRID, box2d_clone(&box), 1, pa);

	// Serialize polygon
	ser = lwpoly_serialize(poly);

	// Construct PG_LWGEOM 
	result = PG_LWGEOM_construct(ser, SRID, 1);
	
	PG_RETURN_POINTER(result);
}

// Convert geometry to BOX (internal postgres type)
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 box2d;
	BOX *result = (BOX *)lwalloc(sizeof(BOX));

	if ( ! getbox2d_p(SERIALIZED_FORM(lwgeom), &box2d) )
	{
		PG_RETURN_NULL(); // must be the empty geometry
	}
	box2df_to_box_p(&box2d, result);

	PG_RETURN_POINTER(result);
}

// makes a polygon of the features bvol - 1st point = LL 3rd=UR
// 2d only. (3d might be worth adding).
// create new geometry of type polygon, 1 ring, 5 points
PG_FUNCTION_INFO_V1(LWGEOM_envelope);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX2DFLOAT4 box;
	POINT2D *pts = lwalloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	int SRID;
	PG_LWGEOM *result;
	char *ser;

	// get bounding box 
	if ( ! getbox2d_p(SERIALIZED_FORM(geom), &box) )
	{
		// must be the EMPTY geometry
		PG_RETURN_POINTER(geom);
	}

	// get geometry SRID
	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));

	// Assign coordinates to POINT2D array
	pts[0].x = box.xmin; pts[0].y = box.ymin;
	pts[1].x = box.xmin; pts[1].y = box.ymax;
	pts[2].x = box.xmax; pts[2].y = box.ymax;
	pts[3].x = box.xmax; pts[3].y = box.ymin;
	pts[4].x = box.xmin; pts[4].y = box.ymin;

	// Construct point array
	pa[0] = lwalloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (char *)pts;
	TYPE_SETZM(pa[0]->dims, 0, 0);
	pa[0]->npoints = 5;

	// Construct polygon 
	poly = lwpoly_construct(SRID, box2d_clone(&box), 1, pa);

	// Serialize polygon
	ser = lwpoly_serialize(poly);

	// Construct PG_LWGEOM 
	result = PG_LWGEOM_construct(ser, SRID, 1);
	
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_isempty);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0 )
		PG_RETURN_BOOL(TRUE);
	PG_RETURN_BOOL(FALSE);
}


#if ! USE_GEOS
Datum centroid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int type = lwgeom_getType(geom->type);
	int SRID = lwgeom_getSRID(geom);
	LWGEOM_EXPLODED *exp = lwgeom_explode(SERIALIZED_FORM(geom));
	LWPOLY *poly=NULL;
	LWPOINT *point;
	PG_LWGEOM *result;
	POINTARRAY *ring, *pa;
	POINT3DZ *p, cent;
	int i,j,k;
	uint32 num_points_tot = 0;
	char *srl;
	char wantbbox = 0;
	double tot_x=0, tot_y=0, tot_z=0;

	if  (type != POLYGONTYPE && type != MULTIPOLYGONTYPE)
		PG_RETURN_NULL();

	//find the centroid
	for (i=0; i<exp->npolys; i++)
	{
		poly = lwpoly_deserialize(exp->polys[i]);
		for (j=0; j<poly->nrings; j++)
		{
			ring = poly->rings[j];
			for (k=0; k<ring->npoints-1; k++)
			{
				p = (POINT3DZ *)getPoint(ring, k);
				tot_x += p->x;
				tot_y += p->y;
				if ( TYPE_HASZ(ring->dims) ) tot_z += p->z;
			}
			num_points_tot += ring->npoints-1;
		}
		pfree_polygon(poly);
	}
	pfree_exploded(exp);

	// Setup point
	cent.x = tot_x/num_points_tot;
	cent.y = tot_y/num_points_tot;
	cent.z = tot_z/num_points_tot;

	// Construct POINTARRAY (paranoia?)
	pa = pointArray_construct((char *)&cent, 1, 0, 1);

	// Construct LWPOINT
	point = lwpoint_construct(SRID, NULL, pa);

	// Serialize LWPOINT 
	srl = lwpoint_serialize(point);

	pfree_point(point);
	pfree_POINTARRAY(pa);

	// Construct output PG_LWGEOM
	result = PG_LWGEOM_construct(srl, SRID, wantbbox);

	PG_RETURN_POINTER(result);
}
#endif // ! USE_GEOS

// Returns a modified [multi]polygon so that no ring segment is 
// longer then the given distance (computed using 2d).
// Every input point is kept.
// Z and M values for added points (if needed) are set to 0.
PG_FUNCTION_INFO_V1(LWGEOM_segmentize2d);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *outgeom, *ingeom;
	double dist;
	LWGEOM *inlwgeom, *outlwgeom;
	size_t size, retsize;

	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	dist = PG_GETARG_FLOAT8(1);

	// Avoid deserialize/serialize steps
	if ( (TYPE_GETTYPE(ingeom->type) == POINTTYPE) ||
		(TYPE_GETTYPE(ingeom->type) == MULTIPOINTTYPE) )
		PG_RETURN_POINTER(ingeom);

	inlwgeom = lwgeom_deserialize(SERIALIZED_FORM(ingeom));
	outlwgeom = lwgeom_segmentize2d(inlwgeom, dist);

	size = lwgeom_serialize_size(outlwgeom);
	outgeom = palloc(size+4);
	outgeom->size = size+4;
	lwgeom_serialize_buf(outlwgeom, SERIALIZED_FORM(outgeom), &retsize);

	if ( size != retsize )
	{
		lwerror ("lwgeom_serialize_buf returned size(%d) != lwgeom_serialize_size (%d)", retsize, size);
	}

	PG_RETURN_POINTER(outgeom);
}

// Reverse vertex order of geometry
PG_FUNCTION_INFO_V1(LWGEOM_reverse);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	lwgeom_reverse(lwgeom);

	PG_RETURN_POINTER(geom);
}

// Force polygons of the collection to obey Right-Hand-Rule
PG_FUNCTION_INFO_V1(LWGEOM_forceRHR_poly);
Datum LWGEOM_forceRHR_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	lwgeom_forceRHR(lwgeom);

	PG_RETURN_POINTER(geom);
}

// Test deserialize/serialize operations
PG_FUNCTION_INFO_V1(LWGEOM_noop);
Datum LWGEOM_noop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in, *out;
	LWGEOM *lwgeom;
	size_t size, retsize;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in));

	lwnotice("Deserialized: %s", lwgeom_summary(lwgeom, 0));

	size = lwgeom_serialize_size(lwgeom);
	out = palloc(size+4);
	out->size = size+4;
	lwgeom_serialize_buf(lwgeom, SERIALIZED_FORM(out), &retsize);

	if ( size != retsize )
	{
		lwerror ("lwgeom_serialize_buf returned size(%d) != lwgeom_serialize_size (%d)", retsize, size);
	}

	PG_RETURN_POINTER(out);
}

// Return:
//  0==2d
//  1==3dm
//  2==3dz
//  3==4d
PG_FUNCTION_INFO_V1(LWGEOM_zmflag);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	unsigned char type;
	int ret = 0;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	type = in->type;
	if ( TYPE_HASZ(type) ) ret += 2;
	if ( TYPE_HASM(type) ) ret += 1;
	PG_RETURN_INT16(ret);
}

// lwgeom_same(lwgeom1, lwgeom2)
PG_FUNCTION_INFO_V1(LWGEOM_same);
Datum LWGEOM_same(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *g1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *g2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWGEOM *lwg1, *lwg2;
	bool result;

	if ( TYPE_GETTYPE(g1->type) != TYPE_GETTYPE(g2->type) )
		PG_RETURN_BOOL(FALSE); // different types

	if ( TYPE_GETZM(g1->type) != TYPE_GETZM(g2->type) )
		PG_RETURN_BOOL(FALSE); // different dimensions

	// ok, deserialize.
	lwg1 = lwgeom_deserialize(SERIALIZED_FORM(g1));
	lwg2 = lwgeom_deserialize(SERIALIZED_FORM(g2));

	// invoke appropriate function
	result = lwgeom_same(lwg1, lwg2);

	// Relase memory
	lwgeom_release(lwg1);
	lwgeom_release(lwg2);
	PG_FREE_IF_COPY(g1, 0);
        PG_FREE_IF_COPY(g2, 1);

        PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS)
{
	double x,y,z,m;
	LWPOINT *point;
	PG_LWGEOM *result;
	size_t size;

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);

	if ( PG_NARGS() == 2 ) point = make_lwpoint2d(-1, x, y);
	else if ( PG_NARGS() == 3 ) {
		z = PG_GETARG_FLOAT8(2);
		point = make_lwpoint3dz(-1, x, y, z);
	}
	else if ( PG_NARGS() == 4 ) {
		z = PG_GETARG_FLOAT8(2);
		m = PG_GETARG_FLOAT8(3);
		point = make_lwpoint4d(-1, x, y, z, m);
	}
	else {
		elog(ERROR, "LWGEOM_makepoint: unsupported number of args: %d",
			PG_NARGS());
		PG_RETURN_NULL();
	}

	size = lwpoint_serialize_size(point);
	result = (PG_LWGEOM *)palloc(size+4);
	result->size = (size+4);
	lwpoint_serialize_buf(point, SERIALIZED_FORM(result), NULL);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint3dm);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS)
{
	double x,y,m;
	LWPOINT *point;
	PG_LWGEOM *result;
	size_t size;

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);
	m = PG_GETARG_FLOAT8(2);

	point = make_lwpoint3dm(-1, x, y, m);

	size = lwpoint_serialize_size(point);
	result = (PG_LWGEOM *)palloc(size+4);
	result->size = (size+4);
	lwpoint_serialize_buf(point, SERIALIZED_FORM(result), NULL);

	PG_RETURN_POINTER(result);
}

