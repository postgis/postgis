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





#define DEBUG

#include "wktparse.h"

Datum combine_box2d(PG_FUNCTION_ARGS);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum postgis_uses_stats(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);


// internal
char * lwgeom_summary_recursive(char *serialized, int offset);
int32 lwgeom_npoints_recursive(char *serialized);

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

