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


// getSRID(lwgeom) :: int4
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
	char *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int srid = lwgeom_getSRID (lwgeom+4);

	//printBYTES(lwgeom, *((int*)lwgeom) );

	PG_RETURN_INT32(srid);
}

//setSRID(lwgeom, int4) :: lwgeom
PG_FUNCTION_INFO_V1(LWGEOM_setSRID);
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS)
{
		char		        *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int					newSRID = PG_GETARG_INT32(1);
		char				type = lwgeom[4];
		int					bbox_offset =0; //0=no bbox, otherwise sizeof(BOX2DFLOAT4)
		int					len,len_new,len_left;
		char				*result;
		char				*loc_new, *loc_old;

	// I'm totally cheating here
	//    If it already has an SRID, just overwrite it.
	//    If it doesnt have an SRID, we'll create it

	if (lwgeom_hasBBOX(type))
		bbox_offset =  sizeof(BOX2DFLOAT4);

	len = *((int *)lwgeom);

	if (lwgeom_hasSRID(type))
	{
		//we create a new one and copy the SRID in
		result = palloc(len);
		memcpy(result, lwgeom, len);
		memcpy(result+5+bbox_offset, &newSRID,4);
		PG_RETURN_POINTER(result);
	}
	else  // need to add one
	{
		len_new = len + 4;//+4 for SRID
		result = palloc(len_new);
		memcpy(result, &len_new, 4); // size copy in
		result[4] = lwgeom_makeType_full(lwgeom_ndims(type), true, lwgeom_getType(type),lwgeom_hasBBOX(type));


		loc_new = result+5;
		loc_old = lwgeom+5;

		len_left = len -4-1;// old length - size - type

			// handle bbox (if there)

		if (lwgeom_hasBBOX(type))
		{
			memcpy(loc_new, loc_old, sizeof(BOX2DFLOAT4)) ;//copy in bbox
			loc_new+=	sizeof(BOX2DFLOAT4);
			loc_old+=	sizeof(BOX2DFLOAT4);
			len_left -= sizeof(BOX2DFLOAT4);
		}

		//put in SRID

		memcpy(loc_new, &newSRID,4);
		loc_new +=4;
		memcpy(loc_new, loc_old, len_left);
	    PG_RETURN_POINTER(result);
	}
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
	//char *geom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	//struct varlena *vl = (struct varlena *)(PG_GETARG_POINTER(0));
	PG_RETURN_INT32(VARSIZE(PG_GETARG_POINTER(0)));
}
