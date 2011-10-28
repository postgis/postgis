/**********************************************************************
 * $Id: geography_inout.c 7248 2011-05-25 18:42:16Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"

#include "../postgis_config.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "utils/elog.h"
#include "utils/array.h"
#include "utils/builtins.h"  /* for pg_atoi */
#include "lib/stringinfo.h"  /* For binary input */
#include "catalog/pg_type.h" /* for CSTRINGOID */

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */
#include "lwgeom_export.h"   /* For export functions. */


Datum geography_typmod_in(PG_FUNCTION_ARGS);
Datum geometry_typmod_in(PG_FUNCTION_ARGS);
Datum postgis_typmod_out(PG_FUNCTION_ARGS);
Datum postgis_typmod_dims(PG_FUNCTION_ARGS);
Datum postgis_typmod_srid(PG_FUNCTION_ARGS);
Datum postgis_typmod_type(PG_FUNCTION_ARGS);
Datum geography_enforce_typmod(PG_FUNCTION_ARGS);
Datum geometry_enforce_typmod(PG_FUNCTION_ARGS);


/*
** postgis_typmod_out(int) returns cstring
*/
PG_FUNCTION_INFO_V1(postgis_typmod_out);
Datum postgis_typmod_out(PG_FUNCTION_ARGS)
{
	char *s = (char*)palloc(64);
	char *str = s;
	uint32 typmod = PG_GETARG_INT32(0);
	uint32 srid = TYPMOD_GET_SRID(typmod);
	uint32 type = TYPMOD_GET_TYPE(typmod);
	uint32 hasz = TYPMOD_GET_Z(typmod);
	uint32 hasm = TYPMOD_GET_M(typmod);

	POSTGIS_DEBUGF(3, "Got typmod(srid = %d, type = %d, hasz = %d, hasm = %d)", srid, type, hasz, hasm);

	/* No SRID or type or dimensionality? Then no typmod at all. Return empty string. */
	if ( ! ( srid || type || hasz || hasz ) )
	{
		*str = '\0';
		PG_RETURN_CSTRING(str);
	}

	/* Opening bracket. */
	str += sprintf(str, "(");

	/* Has type? */
	if ( type )
		str += sprintf(str, "%s", lwtype_name(type));
  else if ( (!type) &&  ( srid || hasz || hasm ) )
    str += sprintf(str, "Geometry");

	/* Has Z? */
	if ( hasz )
		str += sprintf(str, "%s", "Z");

	/* Has M? */
	if ( hasm )
		str += sprintf(str, "%s", "M");

	/* Comma? */
	if ( srid )
		str += sprintf(str, ",");

	/* Has SRID? */
	if ( srid )
		str += sprintf(str, "%d", srid);

	/* Closing bracket. */
	str += sprintf(str, ")");

	PG_RETURN_CSTRING(s);

}


/**
* Check the consistency of the metadata we want to enforce in the typmod:
* srid, type and dimensionality. If things are inconsistent, shut down the query.
*/
void postgis_valid_typmod(LWGEOM *lwgeom, int32 typmod)
{
	int32 lwgeom_srid;
	int32 lwgeom_type;
	int32 lwgeom_z;
	int32 lwgeom_m;
	int32 typmod_srid = TYPMOD_GET_SRID(typmod);
	int32 typmod_type = TYPMOD_GET_TYPE(typmod);
	int32 typmod_z = TYPMOD_GET_Z(typmod);
	int32 typmod_m = TYPMOD_GET_M(typmod);

	Assert(lwgeom);

	lwgeom_type = lwgeom->type;
	lwgeom_srid = lwgeom->srid;
	lwgeom_z = FLAGS_GET_Z(lwgeom->flags);
	lwgeom_m = FLAGS_GET_M(lwgeom->flags);

	POSTGIS_DEBUG(2, "Entered function");

	/* No typmod (-1) => no preferences */
	if (typmod < 0) return;

	POSTGIS_DEBUGF(3, "Got lwgeom(type = %d, srid = %d, hasz = %d, hasm = %d)", lwgeom_type, lwgeom_srid, lwgeom_z, lwgeom_m);
	POSTGIS_DEBUGF(3, "Got typmod(type = %d, srid = %d, hasz = %d, hasm = %d)", typmod_type, typmod_srid, typmod_z, typmod_m);

	/* Typmod has a preference for SRID and lwgeom has a non-default SRID? They had better match. */
	if ( typmod_srid > 0 && typmod_srid != lwgeom_srid )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Geometry SRID (%d) does not match column SRID (%d)", lwgeom_srid, typmod_srid) ));
	}

	/* Typmod has a preference for geometry type. */
	if ( typmod_type > 0 &&
	        /* GEOMETRYCOLLECTION column can hold any kind of collection */
	        ((typmod_type == COLLECTIONTYPE && ! (lwgeom_type == COLLECTIONTYPE ||
	                                              lwgeom_type == MULTIPOLYGONTYPE ||
	                                              lwgeom_type == MULTIPOINTTYPE ||
	                                              lwgeom_type == MULTILINETYPE )) ||
	         /* Other types must be strictly equal. */
	         (typmod_type != lwgeom_type)) )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Geometry type (%s) does not match column type (%s)", lwtype_name(lwgeom_type), lwtype_name(typmod_type)) ));
	}

	/* Mismatched Z dimensionality. */
	if ( typmod_z && ! lwgeom_z )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Column has Z dimension but geometry does not" )));
	}

	/* Mismatched Z dimensionality (other way). */
	if ( lwgeom_z && ! typmod_z )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Geometry has Z dimension but column does not" )));
	}

	/* Mismatched M dimensionality. */
	if ( typmod_m && ! lwgeom_m )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Column has M dimension but geometry does not" )));
	}

	/* Mismatched M dimensionality (other way). */
	if ( lwgeom_m && ! typmod_m )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Geometry has M dimension but column does not" )));
	}
}


static uint32 gserialized_typmod_in(ArrayType *arr, int is_geography)
{
	uint32 typmod = 0;
	Datum *elem_values;
	int n = 0;
	int	i = 0;

	if (ARR_ELEMTYPE(arr) != CSTRINGOID)
		ereport(ERROR,
		        (errcode(ERRCODE_ARRAY_ELEMENT_ERROR),
		         errmsg("typmod array must be type cstring[]")));

	if (ARR_NDIM(arr) != 1)
		ereport(ERROR,
		        (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
		         errmsg("typmod array must be one-dimensional")));

	if (ARR_HASNULL(arr))
		ereport(ERROR,
		        (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
		         errmsg("typmod array must not contain nulls")));

	deconstruct_array(arr,
	                  CSTRINGOID, -2, false, 'c', /* hardwire cstring representation details */
	                  &elem_values, NULL, &n);

	/* Set the SRID to the default value first */
	if ( is_geography) 
	    TYPMOD_SET_SRID(typmod, SRID_DEFAULT);
	else
	    TYPMOD_SET_SRID(typmod, SRID_UNKNOWN);

	for (i = 0; i < n; i++)
	{
		if ( i == 0 ) /* TYPE */
		{
			char *s = DatumGetCString(elem_values[i]);
			uint8_t type = 0;
			int z = 0;
			int m = 0;

			if ( geometry_type_from_string(s, &type, &z, &m) == LW_FAILURE )
			{
				ereport(ERROR,
				        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				         errmsg("Invalid geometry type modifier: %s", s)));
			}
			else
			{
				TYPMOD_SET_TYPE(typmod, type);
				if ( z )
					TYPMOD_SET_Z(typmod);
				if ( m )
					TYPMOD_SET_M(typmod);
			}
		}
		if ( i == 1 ) /* SRID */
		{
			int srid = pg_atoi(DatumGetCString(elem_values[i]),
			                   sizeof(int32), '\0');
			srid = clamp_srid(srid);
			POSTGIS_DEBUGF(3, "srid: %d", srid);
			if ( srid != SRID_UNKNOWN )
			{
				/* TODO: Check that the value
				 *	 provided is in fact a lonlat
				 *       entry in spatial_ref_sysj
				 */
				/* For now, we only accept SRID_DEFAULT. */
				if ( is_geography && srid != SRID_DEFAULT )
				{
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("Currently, only %d is accepted as an SRID for GEOGRAPHY", SRID_DEFAULT)));
				}
				else
				{
					TYPMOD_SET_SRID(typmod, srid);
				}
			}
			else
			{
			    if ( is_geography )
			    {
					ereport(ERROR,
					        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					         errmsg("Negative SRID is not for GEOGRAPHY")));			        
		        }
			}
		}
	}

	pfree(elem_values);

	return typmod;
}

/*
** geography_typmod_in(cstring[]) returns int32
**
** Modified from ArrayGetIntegerTypmods in PostgreSQL 8.3
*/
PG_FUNCTION_INFO_V1(geography_typmod_in);
Datum geography_typmod_in(PG_FUNCTION_ARGS)
{
	ArrayType *arr = (ArrayType *) DatumGetPointer(PG_GETARG_DATUM(0));
	uint32 typmod = gserialized_typmod_in(arr, LW_TRUE);
	PG_RETURN_INT32(typmod);
}

/*
** geometry_typmod_in(cstring[]) returns int32
**
** Modified from ArrayGetIntegerTypmods in PostgreSQL 8.3
*/
PG_FUNCTION_INFO_V1(geometry_typmod_in);
Datum geometry_typmod_in(PG_FUNCTION_ARGS)
{
	ArrayType *arr = (ArrayType *) DatumGetPointer(PG_GETARG_DATUM(0));
	uint32 typmod = gserialized_typmod_in(arr, LW_FALSE); /* Not a geography */;
	PG_RETURN_INT32(typmod);
}

/*
** geography_enforce_typmod(*GSERIALIZED, uint32) returns *GSERIALIZED
** Ensure that an incoming geometry conforms to typmod restrictions on
** type, dims and srid.
*/
PG_FUNCTION_INFO_V1(geography_enforce_typmod);
Datum geography_enforce_typmod(PG_FUNCTION_ARGS)
{
	GSERIALIZED *arg = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = NULL;
	int32 typmod = PG_GETARG_INT32(1);
	/* We don't need to have different behavior based on explicitness. */
	/* bool isExplicit = PG_GETARG_BOOL(2); */

	lwgeom = lwgeom_from_gserialized(arg);

	/* Check if geometry typmod is consistent with the supplied one. */
	postgis_valid_typmod(lwgeom, typmod);

	PG_RETURN_POINTER(geography_serialize(lwgeom));
}

/*
** geometry_enforce_typmod(*GSERIALIZED, uint32) returns *GSERIALIZED
** Ensure that an incoming geometry conforms to typmod restrictions on
** type, dims and srid.
*/
PG_FUNCTION_INFO_V1(geometry_enforce_typmod);
Datum geometry_enforce_typmod(PG_FUNCTION_ARGS)
{
	GSERIALIZED *arg = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = NULL;
	int32 typmod = PG_GETARG_INT32(1);
	/* We don't need to have different behavior based on explicitness. */
	/* bool isExplicit = PG_GETARG_BOOL(2); */

	lwgeom = lwgeom_from_gserialized(arg);

	/* Check if geometry typmod is consistent with the supplied one. */
	postgis_valid_typmod(lwgeom, typmod);

	PG_RETURN_POINTER(geometry_serialize(lwgeom));
}


/*
** postgis_typmod_type(uint32) returns cstring
** Used for geometry_columns and other views on system tables
*/
PG_FUNCTION_INFO_V1(postgis_typmod_type);
Datum postgis_typmod_type(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	int32 type = TYPMOD_GET_TYPE(typmod);
	char *s = (char*)palloc(64);
	char *ptr = s;
	text *stext;

	/* Has type? */
	if ( typmod < 0 || type == 0 )
		ptr += sprintf(ptr, "Geometry");
	else
		ptr += sprintf(ptr, "%s", lwtype_name(type));

	/* Has Z? */
	if ( typmod >= 0 && TYPMOD_GET_Z(typmod) )
		ptr += sprintf(ptr, "%s", "Z");

	/* Has M? */
	if ( typmod >= 0 && TYPMOD_GET_M(typmod) )
		ptr += sprintf(ptr, "%s", "M");

	stext = cstring2text(s);
	pfree(s);
	PG_RETURN_TEXT_P(stext);
}

/*
** postgis_typmod_dims(uint32) returns int
** Used for geometry_columns and other views on system tables
*/
PG_FUNCTION_INFO_V1(postgis_typmod_dims);
Datum postgis_typmod_dims(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	int32 dims = 2;
	if ( typmod < 0 )
		PG_RETURN_INT32(dims);
	if ( TYPMOD_GET_Z(typmod) )
		dims++;
	if ( TYPMOD_GET_M(typmod) )
		dims++;
	PG_RETURN_INT32(dims);
}

/*
** postgis_typmod_srid(uint32) returns int
** Used for geometry_columns and other views on system tables
*/
PG_FUNCTION_INFO_V1(postgis_typmod_srid);
Datum postgis_typmod_srid(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	if ( typmod < 0 )
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(TYPMOD_GET_SRID(typmod));
}

