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
Datum LWGEOM_setSRID(PG_FUNCTION_ARGS);


// getSRID(lwgeom) :: int4
PG_FUNCTION_INFO_V1(LWGEOM_getSRID);
Datum LWGEOM_getSRID(PG_FUNCTION_ARGS)
{
		char		        *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int					srid = lwgeom_getSRID (lwgeom+4);

	//	printBYTES(lwgeom, *((int*)lwgeom) );

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
