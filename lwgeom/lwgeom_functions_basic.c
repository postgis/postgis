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

Datum LWGEOM_getSRID(PG_FUNCTION_ARGS);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS);
Datum combine_box2d(PG_FUNCTION_ARGS);
Datum lwgeom_mem_size(PG_FUNCTION_ARGS);

static void *
palloc_fn(size_t size)
{
	return palloc(size);
}

// getSRID(lwgeom) :: int4
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = lwgeom_getSRID (lwgeom);

	PG_RETURN_INT32(srid);
}

//setSRID(lwgeom, int4) :: lwgeom
PG_FUNCTION_INFO_V1(LWGEOM_setSRID);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int newSRID = PG_GETARG_INT32(1);
	LWGEOM *result;
	LWGEOM_allocator oldalloc;

	oldalloc = LWGEOM_setAllocator(palloc_fn);
	result = lwgeom_setSRID(lwgeom, newSRID);
	LWGEOM_setAllocator(oldalloc);

	PG_RETURN_POINTER(result);
}

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	char *lwgeom = (char *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *text_ob = palloc(20+4);
	char *result = text_ob+4;
	int32 size;
	unsigned char type;

	type = lwgeom_getType(*(lwgeom+4));

	memset(result, 0, 20);

	if (type == POINTTYPE)
		strcpy(result,"POINT");
	else if (type == MULTIPOINTTYPE)
		strcpy(result,"MULTIPOINT");
	else if (type == LINETYPE)
		strcpy(result,"LINESTRING");
	else if (type == MULTILINETYPE)
		strcpy(result,"MULTILINESTRING");
	else if (type == POLYGONTYPE)
		strcpy(result,"POLYGON");
	else if (type == MULTIPOLYGONTYPE)
		strcpy(result,"MULTIPOLYGON");
	else if (type == COLLECTIONTYPE)
		strcpy(result,"GEOMETRYCOLLECTION");
	else
		strcpy(result,"UNKNOWN");

	size = strlen(result) +4 ;

	memcpy(text_ob, &size,4); // size of string

	PG_RETURN_POINTER(text_ob);
}

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
PG_FUNCTION_INFO_V1(lwgeom_mem_size);
Datum lwgeom_mem_size(PG_FUNCTION_ARGS)
{
	char *geom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 size = *((int32 *)geom);
	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(size);
}
