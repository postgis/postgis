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
Datum lwgeom_summary(PG_FUNCTION_ARGS);

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

	result = lwgeom_setSRID(lwgeom, newSRID);

	PG_RETURN_POINTER(result);
}

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(LWGEOM_getTYPE);
Datum LWGEOM_getTYPE(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *text_ob = palloc(20+4);
	char *result = text_ob+4;
	int32 size;
	unsigned char type;

	//type = lwgeom_getType(*(lwgeom+4));
	type = lwgeom_getType(lwgeom->type);

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
PG_FUNCTION_INFO_V1(lwgeom_summary);
Datum lwgeom_summary(PG_FUNCTION_ARGS)
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
