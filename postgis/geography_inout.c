/**********************************************************************
 * $Id$
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

#include "libgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */
#include "lwgeom_export.h"   /* For exports functions. */

Datum geography_in(PG_FUNCTION_ARGS);
Datum geography_out(PG_FUNCTION_ARGS);
Datum geography_typmod_in(PG_FUNCTION_ARGS);
Datum geography_typmod_out(PG_FUNCTION_ARGS);
Datum geography_typmod_dims(PG_FUNCTION_ARGS);
Datum geography_typmod_srid(PG_FUNCTION_ARGS);
Datum geography_typmod_type(PG_FUNCTION_ARGS);

Datum geography_enforce_typmod(PG_FUNCTION_ARGS);
Datum geography_as_text(PG_FUNCTION_ARGS); 
Datum geography_from_text(PG_FUNCTION_ARGS);
Datum geography_as_geojson(PG_FUNCTION_ARGS); 
Datum geography_as_gml(PG_FUNCTION_ARGS); 
Datum geography_as_kml(PG_FUNCTION_ARGS); 
Datum geography_as_svg(PG_FUNCTION_ARGS); 
Datum geography_as_binary(PG_FUNCTION_ARGS);
Datum geography_from_binary(PG_FUNCTION_ARGS);
Datum geography_from_geometry(PG_FUNCTION_ARGS);

/* Datum geography_gist_selectivity(PG_FUNCTION_ARGS); TBD */
/* Datum geography_gist_join_selectivity(PG_FUNCTION_ARGS); TBD */
/* Datum geography_send(PG_FUNCTION_ARGS); TBD */
/* Datum geography_recv(PG_FUNCTION_ARGS); TBD */


/**
* Utility method to call the libgeom serialization and then set the 
* PgSQL varsize header appropriately with the serialized size.
*/
GSERIALIZED* geography_serialize(LWGEOM *lwgeom)
{
	static int is_geodetic = 1;
	size_t ret_size = 0;
	GSERIALIZED *g = NULL;
	
	g = gserialized_from_lwgeom(lwgeom, is_geodetic, &ret_size);
	if( ! g ) lwerror("Unable to serialize lwgeom.");
	SET_VARSIZE(g, ret_size);
	return g;
}

/**
* The geography type only support POINT, LINESTRING, POLYGON, MULTI* variants
* of same, and GEOMETRYCOLLECTION. If the input type is not one of those, shut
* down the query.
*/
void geography_valid_type(uchar type)
{
    if( ! (
        type == POINTTYPE ||
        type == LINETYPE ||
        type == POLYGONTYPE ||
        type == MULTIPOINTTYPE ||
        type == MULTILINETYPE ||
        type == MULTIPOLYGONTYPE ||
        type == COLLECTIONTYPE
        ) )
    {
        ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Geography type does not support %s", lwgeom_typename(type) )));
		
    }
}

/**
* Check the consistency of the metadata we want to enforce in the typmod:
* srid, type and dimensionality. If things are inconsistent, shut down the query.
*/
void geography_valid_typmod(LWGEOM *lwgeom, int32 typmod)
{
	int32 lwgeom_srid;
	int32 lwgeom_type;
	int32 lwgeom_z;
	int32 lwgeom_m;
	int32 typmod_srid = TYPMOD_GET_SRID(typmod);
	int32 typmod_type = TYPMOD_GET_TYPE(typmod);
	int32 typmod_z = TYPMOD_GET_Z(typmod);
	int32 typmod_m = TYPMOD_GET_M(typmod);
	
	assert(lwgeom);
	
	lwgeom_type = TYPE_GETTYPE(lwgeom->type);
	lwgeom_srid = lwgeom->SRID;
	lwgeom_z = TYPE_HASZ(lwgeom->type);
	lwgeom_m = TYPE_HASM(lwgeom->type);

	POSTGIS_DEBUG(2, "Entered function");
	
	/* No typmod (-1) => no preferences */
	if (typmod < 0) return;

	POSTGIS_DEBUGF(3, "Got lwgeom(type = %d, srid = %d, hasz = %d, hasm = %d)", lwgeom_type, lwgeom_srid, lwgeom_z, lwgeom_m);
	POSTGIS_DEBUGF(3, "Got typmod(type = %d, srid = %d, hasz = %d, hasm = %d)", typmod_type, typmod_srid, typmod_z, typmod_m);	
	
	/* Typmod has a preference for SRID. */
	if( typmod_srid > 0 && typmod_srid != lwgeom_srid)
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Geography SRID (%d) does not match column SRID (%d)", lwgeom_srid, typmod_srid) ));
	}
	
	/* Typmod has a preference for geometry type. */
	if( typmod_type > 0 && 
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
	  		errmsg("Geometry type (%s) does not match column type (%s)", lwgeom_typename(lwgeom_type), lwgeom_typename(typmod_type)) ));
	}

	/* Mismatched Z dimensionality. */
	if( typmod_z && ! lwgeom_z )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Column has Z dimension but geometry does not" )));
	}

	/* Mismatched Z dimensionality (other way). */
	if( lwgeom_z && ! typmod_z )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Geometry has Z dimension but column does not" )));
	}

	/* Mismatched M dimensionality. */
	if( typmod_m && ! lwgeom_m )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Column has M dimension but geometry does not" )));
	}

	/* Mismatched M dimensionality (other way). */
	if( lwgeom_m && ! typmod_m )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Geometry has M dimension but column does not" )));
	}		
}

/*
** geography_in(cstring) returns *GSERIALIZED
*/
PG_FUNCTION_INFO_V1(geography_in);
Datum geography_in(PG_FUNCTION_ARGS)
{
	char *geog_str = PG_GETARG_CSTRING(0);
	/* Datum geog_oid = PG_GETARG_OID(1); Not needed. */
	int32 geog_typmod = PG_GETARG_INT32(2); 	
	LWGEOM_PARSER_RESULT lwg_parser_result;
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g_ser = NULL;
	int result = 0;

	/* Handles both HEXEWKB and EWKT */
	result = serialized_lwgeom_from_ewkt(&lwg_parser_result, geog_str, PARSER_CHECK_ALL);
	if (result)
		PG_PARSER_ERROR(lwg_parser_result);

	lwgeom = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

    geography_valid_type(TYPE_GETTYPE(lwgeom->type));

	if( geog_typmod >= 0 )
	{
		geography_valid_typmod(lwgeom, geog_typmod);
		POSTGIS_DEBUG(3, "typmod and geometry were consistent");
	}
	else
	{
		POSTGIS_DEBUG(3, "typmod was -1");
	}

	/*
	** Serialize our lwgeom and set the geodetic flag so subsequent
	** functions do the right thing. 
	*/
	g_ser = geography_serialize(lwgeom);
	g_ser->flags = FLAGS_SET_GEODETIC(g_ser->flags, 1);
    
	/* 
	** Replace the unaligned lwgeom with a new aligned one based on GSERIALIZED. 
	*/
	lwgeom_release(lwgeom);
	lwgeom = lwgeom_from_gserialized(g_ser);
	
	/* Check if the geography has valid coordinate range. */
	if( lwgeom_check_geodetic(lwgeom) == LW_FALSE )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Coordinate values are out of range [-180 -90, 180 90] for GEOGRAPHY type" )));
	}			
	
	PG_RETURN_POINTER(g_ser);
}

/*
** geography_out(*GSERIALIZED) returns cstring
*/
PG_FUNCTION_INFO_V1(geography_out);
Datum geography_out(PG_FUNCTION_ARGS)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	int result = 0;
	
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = lwgeom_from_gserialized(g);
	
	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, lwgeom_serialize(lwgeom), PARSER_CHECK_ALL, -1);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

	PG_RETURN_CSTRING(lwg_unparser_result.wkoutput);
}

/*
** geography_enforce_typmod(*GSERIALIZED) returns *GSERIALIZED
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
	geography_valid_typmod(lwgeom, typmod);
	
	PG_RETURN_POINTER(geography_serialize(lwgeom));
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

	for (i = 0; i < n; i++) 
	{
		if( i == 1 ) /* SRID */
		{
			int srid = pg_atoi(DatumGetCString(elem_values[i]), sizeof(int32), '\0');
			if( srid > 0 )
			{
				POSTGIS_DEBUGF(3, "srid: %d", srid);
				if( srid > SRID_MAXIMUM ) 
				{
					ereport(ERROR,
                      (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                       errmsg("SRID value may not exceed %d",
                              SRID_MAXIMUM)));
				}
				else
				{
					/* TODO: Check that the value provided is in fact a lonlat entry in spatial_ref_sys. */
					/* For now, we only accept 4326. */
					if( srid != 4326 )
					{
						ereport(ERROR,
                      		(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                       		errmsg("Currently, only 4326 is accepted as an SRID for GEOGRAPHY")));
					}
					else 
					{
						typmod = TYPMOD_SET_SRID(typmod, srid);
					}
				}
			}
			else
			{
				typmod = TYPMOD_SET_SRID(typmod, 0);
			}
		}
		if( i == 0 ) /* TYPE */
		{
			char *s = DatumGetCString(elem_values[i]);
			int type = 0;
			int z = 0;
			int m = 0;
			
			if( geometry_type_from_string(s, &type, &z, &m) == G_FAILURE )
			{
				ereport(ERROR,
				    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				     errmsg("Invalid geometry type modifier: %s", s)));				
			}
			else 
			{
				typmod = TYPMOD_SET_TYPE(typmod, type);
				if ( z )
					typmod = TYPMOD_SET_Z(typmod);
				if ( m )
					typmod = TYPMOD_SET_M(typmod);
			}			
		}
	}
				
	pfree(elem_values);

	PG_RETURN_INT32(typmod);

}


/*
** geography_typmod_out(int) returns cstring
*/
PG_FUNCTION_INFO_V1(geography_typmod_out);
Datum geography_typmod_out(PG_FUNCTION_ARGS)
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
	if( ! srid && ! type && ! hasz && ! hasz )
	{
		*str = '\0';
		PG_RETURN_CSTRING(str);
	}
	
	/* Opening bracket. */
	str += sprintf(str, "(");
	
	/* Has type? */
	if( type )
		str += sprintf(str, "%s", lwgeom_typename(type));
		
	/* Need dummy type to append Z/M to? */
	if( !type & (hasz || hasz) )
		str += sprintf(str, "Geometry");
	
	/* Has Z? */
	if( hasz )
		str += sprintf(str, "%s", "Z");

	/* Has M? */
	if( hasm )
		str += sprintf(str, "%s", "M");

	/* Comma? */
	if( srid && ( type || hasz || hasm ) )
		str += sprintf(str, ",");
		
	/* Has SRID? */
	if( srid )
		str += sprintf(str, "%d", srid);

	/* Closing bracket. */
	str += sprintf(str, ")");
		
	PG_RETURN_CSTRING(s);

}

/*
** geography_as_text(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_text);
Datum geography_as_text(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	int result = 0;
	char *semicolon_loc = NULL;
	char *wkt = NULL;
	uchar *lwgeom_serialized = NULL;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	size_t len = 0;
	
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);
	lwgeom_serialized = lwgeom_serialize(lwgeom);

	/* Generate WKT */
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, lwgeom_serialized, PARSER_CHECK_ALL);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

	/* Strip SRID=NNNN;' off the front of the EWKT. */
	semicolon_loc = strchr(lwg_unparser_result.wkoutput,';');
	if (semicolon_loc == NULL) 
		semicolon_loc = lwg_unparser_result.wkoutput;
	else 
		semicolon_loc = semicolon_loc + 1;

	len = strlen(semicolon_loc) + VARHDRSZ;
	wkt = palloc(len);
	SET_VARSIZE(wkt, len);

	memcpy(VARDATA(wkt), semicolon_loc, len - VARHDRSZ);

	pfree(lwg_unparser_result.wkoutput);
	pfree(lwgeom_serialized);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(wkt);
}


/*
** geography_as_gml(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_gml);
Datum geography_as_gml(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	char *gml;
	text *result;
	int len;
	int version;
	char *srs;
	int SRID=4326;
	int precision = MAX_DOUBLE_PRECISION;
	int option=0;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2 && version != 3 )
	{
		elog(ERROR, "Only GML 2 and GML 3 are supported");
		PG_RETURN_NULL();
	}

	/* Get the geography */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* retrieve option */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);

	if (option & 1) srs = getSRSbySRID(SRID, false);
	else srs = getSRSbySRID(SRID, true);
        if (!srs)
	{
                elog(ERROR, "SRID 4326 unknown in spatial_ref_sys table");
                PG_RETURN_NULL();
        }

	if (version == 2)
		gml = geometry_to_gml2(lwgeom_serialize(lwgeom), srs, precision);
	else
		gml = geometry_to_gml3(lwgeom_serialize(lwgeom), srs, precision, true);

	PG_FREE_IF_COPY(lwgeom, 1);

	len = strlen(gml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), gml, len-VARHDRSZ);

	pfree(gml);

	PG_RETURN_POINTER(result);
}


/*
** geography_as_kml(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_kml);
Datum geography_as_kml(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g = NULL;
	LWGEOM *lwgeom = NULL;
	char *kml;
	text *result;
	int len;
	int version;
	int precision = MAX_DOUBLE_PRECISION;


	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2)
	{
		elog(ERROR, "Only KML 2 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	kml = geometry_to_kml2(lwgeom_serialize(lwgeom), precision);

	PG_FREE_IF_COPY(lwgeom, 1);

	len = strlen(kml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), kml, len-VARHDRSZ);

	pfree(kml);

	PG_RETURN_POINTER(result);
}


/*
** geography_as_svg(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_svg);
Datum geography_as_svg(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g = NULL;
	LWGEOM *lwgeom = NULL;
	char *svg;
	text *result;
	int len;
	bool relative = false;
	int precision=MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		relative = PG_GETARG_INT32(1) ? true:false;

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	svg = geometry_to_svg(lwgeom_serialize(lwgeom), relative, precision);
	PG_FREE_IF_COPY(lwgeom, 0);

	len = strlen(svg) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), svg, len-VARHDRSZ);

	pfree(svg);

	PG_RETURN_POINTER(result);
}


/*
** geography_as_geojson(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_geojson);
Datum geography_as_geojson(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
        char *geojson;
        text *result;
        int len;
        int version;
        int option = 0;
        bool has_bbox = 0;
        int precision = MAX_DOUBLE_PRECISION;
        char * srs = NULL;

        /* Get the version */
        version = PG_GETARG_INT32(0);
        if ( version != 1)
        {
                elog(ERROR, "Only GeoJSON 1 is supported");
                PG_RETURN_NULL();
        }

        /* Get the geography */
        if (PG_ARGISNULL(1) ) PG_RETURN_NULL();
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

       /* Retrieve precision if any (default is max) */
        if (PG_NARGS() >2 && !PG_ARGISNULL(2))
        {
                precision = PG_GETARG_INT32(2);
                if ( precision > MAX_DOUBLE_PRECISION )
                        precision = MAX_DOUBLE_PRECISION;
                else if ( precision < 0 ) precision = 0;
        }

        /* Retrieve output option
         * 0 = without option (default)
         * 1 = bbox
         * 2 = short crs
         * 4 = long crs
         */
        if (PG_NARGS() >3 && !PG_ARGISNULL(3))
                option = PG_GETARG_INT32(3);

        if (option & 2 || option & 4)
        {
		/* Geography only handle srid 4326 */
                if (option & 2) srs = getSRSbySRID(4326, true);
                if (option & 4) srs = getSRSbySRID(4326, false);

                if (!srs)
		{
                     elog(ERROR, "SRID 4326 unknown in spatial_ref_sys table");
                     PG_RETURN_NULL();
                }
        }

        if (option & 1) has_bbox = 1;

        geojson = geometry_to_geojson(lwgeom_serialize(lwgeom), srs, has_bbox, precision);
        PG_FREE_IF_COPY(lwgeom, 1);
        if (srs) pfree(srs);

        len = strlen(geojson) + VARHDRSZ;
        result = palloc(len);
        SET_VARSIZE(result, len);
        memcpy(VARDATA(result), geojson, len-VARHDRSZ);

        pfree(geojson);

        PG_RETURN_POINTER(result);
}


/*
** geography_from_text(*char) returns *GSERIALIZED
**
** Convert text (varlena) to cstring and then call geography_in().
*/
PG_FUNCTION_INFO_V1(geography_from_text);
Datum geography_from_text(PG_FUNCTION_ARGS)
{
	text *wkt_text = PG_GETARG_TEXT_P(0);
	size_t size = VARSIZE(wkt_text) - VARHDRSZ;
	/* Extract the cstring from the varlena */
	char *wkt = palloc(size + 1);
	memcpy(wkt, VARDATA(wkt_text), size);
	/* Null terminate it */
	wkt[size] = '\0';

	if ( size < 10 )
	{
		lwerror("Invalid OGC WKT (too short)");
		PG_RETURN_NULL();
	}
	
	/* Pass the cstring to the input parser, and magic occurs! */
	PG_RETURN_DATUM(DirectFunctionCall3(geography_in, PointerGetDatum(wkt), Int32GetDatum(0), Int32GetDatum(-1)));
}

/*
** geography_as_binary(*GSERIALIZED) returns bytea
*/
PG_FUNCTION_INFO_V1(geography_as_binary);
Datum geography_as_binary(PG_FUNCTION_ARGS)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	LWGEOM *lwgeom = NULL;
	uchar *lwgeom_serialized = NULL;
	size_t lwgeom_serialized_size = 0;
	uchar *lwgeom_serialized_2d = NULL;
	int result = 0;
	char *wkb = NULL;
	size_t wkb_size = 0;
	GSERIALIZED *g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	
	/* Drop SRID so that WKB does not contain SRID. */
	gserialized_set_srid(g, 0);

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);
	lwgeom_serialized_size = lwgeom_serialize_size(lwgeom);
	lwgeom_serialized = lwgeom_serialize(lwgeom);

	/* Force to 2D */
	lwgeom_serialized_2d = lwalloc(lwgeom_serialized_size);
	lwgeom_force2d_recursive(lwgeom_serialized, lwgeom_serialized_2d, &lwgeom_serialized_size);

	/* Create WKB */
	result = serialized_lwgeom_to_ewkb(&lwg_unparser_result, lwgeom_serialized_2d, PARSER_CHECK_ALL, NDR);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

	/* Copy to varlena pointer */
	wkb_size = lwg_unparser_result.size + VARHDRSZ;
	wkb = palloc(wkb_size);
	SET_VARSIZE(wkb, wkb_size);
	memcpy(VARDATA(wkb), lwg_unparser_result.wkoutput, lwg_unparser_result.size);
	
	/* Clean up */
	pfree(lwg_unparser_result.wkoutput);
	lwgeom_release(lwgeom);
	lwfree(lwgeom_serialized);
	lwfree(lwgeom_serialized_2d);

	PG_RETURN_POINTER(wkb);
}

/*
** geography_from_binary(*char) returns *GSERIALIZED
*/
PG_FUNCTION_INFO_V1(geography_from_binary);
Datum geography_from_binary(PG_FUNCTION_ARGS)
{
	char *wkb_cstring = NULL;
	char *wkb_hex = NULL;
	char *wkb_bytea = (char*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *hexarg = palloc(4 + VARHDRSZ);
	size_t wkb_hex_size;

	/* Create our second argument to binary_encode */
	SET_VARSIZE(hexarg, 4 + VARHDRSZ);
	memcpy(VARDATA(hexarg), "hex", 4);

	/* Convert the bytea into a hex representation cstring */
	wkb_hex = DatumGetPointer(DirectFunctionCall2(binary_encode, PointerGetDatum(wkb_bytea), PointerGetDatum(hexarg)));
	wkb_hex_size = VARSIZE(wkb_hex) - VARHDRSZ;
	wkb_cstring = palloc(wkb_hex_size + 1);
	memcpy(wkb_cstring, VARDATA(wkb_hex), wkb_hex_size);
	wkb_cstring[wkb_hex_size] = '\0'; /* Null terminate the cstring */
	pfree(hexarg);
	
	/* Pass the cstring to the input parser, and magic occurs! */
	PG_RETURN_DATUM(DirectFunctionCall3(geography_in, PointerGetDatum(wkb_cstring), Int32GetDatum(0), Int32GetDatum(-1)));
}

PG_FUNCTION_INFO_V1(geography_typmod_dims);
Datum geography_typmod_dims(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	int32 dims = 2;
	if( typmod < 0 )
		PG_RETURN_INT32(dims);
	if( TYPMOD_GET_Z(typmod) )
		dims++;
	if( TYPMOD_GET_M(typmod) )
		dims++;
	PG_RETURN_INT32(dims);
}

PG_FUNCTION_INFO_V1(geography_typmod_srid);
Datum geography_typmod_srid(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	if( typmod < 0 )
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(TYPMOD_GET_SRID(typmod));
}

PG_FUNCTION_INFO_V1(geography_typmod_type);
Datum geography_typmod_type(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	int32 type = TYPMOD_GET_TYPE(typmod);
	char *s = (char*)palloc(64);
	char *str = s;
	int slen = 0;

	/* Has type? */
	if( typmod < 0 || type == 0 )
		str += sprintf(str, "Geometry");
	else
		str += sprintf(str, "%s", lwgeom_typename(type));
	
	/* Has Z? */
	if( typmod >= 0 && TYPMOD_GET_Z(typmod) )
		str += sprintf(str, "%s", "Z");

	/* Has M? */
	if( typmod >= 0 && TYPMOD_GET_M(typmod) )
		str += sprintf(str, "%s", "M");
	
	slen = strlen(s) + 1;
	str = palloc(slen + VARHDRSZ);
	SET_VARSIZE(str, slen + VARHDRSZ);
	memcpy(VARDATA(str), s, slen);
	pfree(s);
	PG_RETURN_POINTER(str);	
}

PG_FUNCTION_INFO_V1(geography_from_geometry);
Datum geography_from_geometry(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g_ser = NULL;
	uchar *lwgeom_serialized = (uchar*)VARDATA(PG_DETOAST_DATUM(PG_GETARG_DATUM(0)));

    geography_valid_type(TYPE_GETTYPE(lwgeom_serialized[0]));

	lwgeom = lwgeom_deserialize(lwgeom_serialized);

	/*
    ** Serialize our lwgeom and set the geodetic flag so subsequent
    ** functions do the right thing. 
    */
	g_ser = geography_serialize(lwgeom);
    g_ser->flags = FLAGS_SET_GEODETIC(g_ser->flags, 1);
    
	/* 
	** Replace the unaligned lwgeom with a new aligned one based on GSERIALIZED. 
	*/
	lwgeom_release(lwgeom);
	lwgeom = lwgeom_from_gserialized(g_ser);
	
	/* Check if the geography has valid coordinate range. */
	if( lwgeom_check_geodetic(lwgeom) == LW_FALSE )
	{
		ereport(ERROR, (
	  		errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	  		errmsg("Coordinate values are out of range [-180 -90, 180 90] for GEOGRAPHY type" )));
	}			
	
	PG_RETURN_POINTER(g_ser);
	
}

