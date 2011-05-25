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

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */
#include "lwgeom_export.h"   /* For export functions. */

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
Datum geometry_from_geography(PG_FUNCTION_ARGS);

/* Datum geography_gist_selectivity(PG_FUNCTION_ARGS); TBD */
/* Datum geography_gist_join_selectivity(PG_FUNCTION_ARGS); TBD */
/* Datum geography_send(PG_FUNCTION_ARGS); TBD */
/* Datum geography_recv(PG_FUNCTION_ARGS); TBD */


/**
* Utility method to call the serialization and then set the
* PgSQL varsize header appropriately with the serialized size.
*/
GSERIALIZED* geography_serialize(LWGEOM *lwgeom)
{
	static int is_geodetic = 1;
	size_t ret_size = 0;
	GSERIALIZED *g = NULL;

	g = gserialized_from_lwgeom(lwgeom, is_geodetic, &ret_size);
	if ( ! g ) lwerror("Unable to serialize lwgeom.");
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
	if ( ! (
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
		            errmsg("Geography type does not support %s", lwtype_name(type) )));

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
		            errmsg("Geography SRID (%d) does not match column SRID (%d)", lwgeom_srid, typmod_srid) ));
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

/*
** geography_in(cstring) returns *GSERIALIZED
*/
PG_FUNCTION_INFO_V1(geography_in);
Datum geography_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	/* Datum geog_oid = PG_GETARG_OID(1); Not needed. */
	int32 geog_typmod = PG_GETARG_INT32(2);
	LWGEOM_PARSER_RESULT lwg_parser_result;
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g_ser = NULL;

	lwgeom_parser_result_init(&lwg_parser_result);

	/* Empty string. */
	if ( str[0] == '\0' )
		ereport(ERROR,(errmsg("parse error - invalid geometry")));

	/* WKB? Let's find out. */
	if ( str[0] == '0' )
	{
		/* TODO: 20101206: No parser checks! This is inline with current 1.5 behavior, but needs discussion */
		lwgeom = lwgeom_from_hexwkb(str, PARSER_CHECK_NONE);
		/* Error out if something went sideways */
		if ( ! lwgeom ) 
			ereport(ERROR,(errmsg("parse error - invalid geometry")));
	}
	/* WKT then. */
	else
	{
		if ( lwgeom_parse_wkt(&lwg_parser_result, str, PARSER_CHECK_ALL) == LW_FAILURE )
			PG_PARSER_ERROR(lwg_parser_result);

		lwgeom = lwg_parser_result.geom;
	}

	/* Set geodetic flag */
	lwgeom_set_geodetic(lwgeom, true);

	/* Check that this is a type we can handle */
	geography_valid_type(lwgeom->type);

	/* Check that the coordinates are in range */
	if ( lwgeom_check_geodetic(lwgeom) == LW_FALSE )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Coordinate values are out of range [-180 -90, 180 90] for GEOGRAPHY type" )));
	}

	/* Force default SRID to the default */
	if ( (int)lwgeom->srid <= 0 )
		lwgeom->srid = SRID_DEFAULT;

	if ( geog_typmod >= 0 )
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

	/* Clean up temporary object */
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(g_ser);

}

/*
** geography_out(*GSERIALIZED) returns cstring
*/
PG_FUNCTION_INFO_V1(geography_out);
Datum geography_out(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	char *hexwkb;

	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = lwgeom_from_gserialized(g);
	hexwkb = lwgeom_to_hexwkb(lwgeom, WKB_EXTENDED, 0);
	lwgeom_free(lwgeom);

	PG_RETURN_CSTRING(hexwkb);
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

	/* Set the SRID to the default value first */
	TYPMOD_SET_SRID(typmod, SRID_DEFAULT);

	for (i = 0; i < n; i++)
	{
		if ( i == 1 ) /* SRID */
		{
			int srid = pg_atoi(DatumGetCString(elem_values[i]), sizeof(int32), '\0');
			if ( srid > 0 )
			{
				POSTGIS_DEBUGF(3, "srid: %d", srid);
				if ( srid > SRID_MAXIMUM )
				{
					ereport(ERROR,
					        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					         errmsg("SRID value may not exceed %d",
					                SRID_MAXIMUM)));
				}
				else
				{
					/* TODO: Check that the value provided is in fact a lonlat entry in spatial_ref_sys. */
					/* For now, we only accept SRID_DEFAULT. */
					if ( srid != SRID_DEFAULT )
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
			}
			else
			{
			}
		}
		if ( i == 0 ) /* TYPE */
		{
			char *s = DatumGetCString(elem_values[i]);
			int type = 0;
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

/*
** geography_as_text(*GSERIALIZED) returns text
*/
PG_FUNCTION_INFO_V1(geography_as_text);
Datum geography_as_text(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g = NULL;
	LWGEOM *lwgeom = NULL;
	char *wkt = NULL;
	text *result = NULL;
	size_t len = 0;

	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

	/* Generate WKT */
	wkt = lwgeom_to_wkt(lwgeom, WKT_ISO, DBL_DIG, &len);

	/* Copy into text obect */
	result = cstring2text(wkt);
	pfree(wkt);
	lwgeom_free(lwgeom);

	PG_RETURN_TEXT_P(result);
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
	int version;
	char *srs;
	int srid = SRID_DEFAULT;
	int precision = OUT_MAX_DOUBLE_PRECISION;
	int option=0;
	int lwopts = LW_GML_IS_DIMS;
	static const char *default_prefix = "gml:";
	char *prefixbuf;
	const char* prefix = default_prefix;
	text *prefix_text;


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
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* retrieve option */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);


	/* retrieve prefix */
	if (PG_NARGS() >4 && !PG_ARGISNULL(4))
	{
		prefix_text = PG_GETARG_TEXT_P(4);
		if ( VARSIZE(prefix_text)-VARHDRSZ == 0 )
		{
			prefix = "";
		}
		else
		{
			/* +2 is one for the ':' and one for term null */
			prefixbuf = palloc(VARSIZE(prefix_text)-VARHDRSZ+2);
			memcpy(prefixbuf, VARDATA(prefix_text),
			       VARSIZE(prefix_text)-VARHDRSZ);
			/* add colon and null terminate */
			prefixbuf[VARSIZE(prefix_text)-VARHDRSZ] = ':';
			prefixbuf[VARSIZE(prefix_text)-VARHDRSZ+1] = '\0';
			prefix = prefixbuf;
		}
	}

	if (option & 1) srs = getSRSbySRID(srid, false);
	else srs = getSRSbySRID(srid, true);
	if (!srs)
	{
		elog(ERROR, "SRID %d unknown in spatial_ref_sys table", SRID_DEFAULT);
		PG_RETURN_NULL();
	}

	/* Revert lat/lon only with long SRS */
	if (option & 1) lwopts |= LW_GML_IS_DEGREE;
	if (option & 2) lwopts &= ~LW_GML_IS_DIMS; 

	if (version == 2)
		gml = lwgeom_to_gml2(lwgeom, srs, precision, prefix);
	else
		gml = lwgeom_to_gml3(lwgeom, srs, precision, lwopts, prefix);

    lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(g, 1);

	result = cstring2text(gml);
	lwfree(gml);

	PG_RETURN_TEXT_P(result);
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
	int version;
	int precision = OUT_MAX_DOUBLE_PRECISION;
	static const char *default_prefix = "";
	char *prefixbuf;
	const char* prefix = default_prefix;
	text *prefix_text;


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
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* retrieve prefix */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
	{
		prefix_text = PG_GETARG_TEXT_P(3);
		if ( VARSIZE(prefix_text)-VARHDRSZ == 0 )
		{
			prefix = "";
		}
		else
		{
			/* +2 is one for the ':' and one for term null */
			prefixbuf = palloc(VARSIZE(prefix_text)-VARHDRSZ+2);
			memcpy(prefixbuf, VARDATA(prefix_text),
			       VARSIZE(prefix_text)-VARHDRSZ);
			/* add colon and null terminate */
			prefixbuf[VARSIZE(prefix_text)-VARHDRSZ] = ':';
			prefixbuf[VARSIZE(prefix_text)-VARHDRSZ+1] = '\0';
			prefix = prefixbuf;
		}
	}

	kml = lwgeom_to_kml2(lwgeom, precision, prefix);

    lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(g, 1);

	if ( ! kml )
		PG_RETURN_NULL();

	result = cstring2text(kml);
	lwfree(kml);

	PG_RETURN_TEXT_P(result);
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
	int relative = 0;
	int precision=OUT_MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Convert to lwgeom so we can run the old functions */
	lwgeom = lwgeom_from_gserialized(g);

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		relative = PG_GETARG_INT32(1) ? 1:0;

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	svg = lwgeom_to_svg(lwgeom, precision, relative);
	
    lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(g, 0);

	result = cstring2text(svg);
	lwfree(svg);

	PG_RETURN_TEXT_P(result);
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
	int version;
	int option = 0;
	int has_bbox = 0;
	int precision = OUT_MAX_DOUBLE_PRECISION;
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
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
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
		/* Geography only handle srid SRID_DEFAULT */
		if (option & 2) srs = getSRSbySRID(SRID_DEFAULT, true);
		if (option & 4) srs = getSRSbySRID(SRID_DEFAULT, false);

		if (!srs)
		{
			elog(ERROR, "SRID SRID_DEFAULT unknown in spatial_ref_sys table");
			PG_RETURN_NULL();
		}
	}

	if (option & 1) has_bbox = 1;

	geojson = lwgeom_to_geojson(lwgeom, srs, precision, has_bbox);
    lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(g, 1);
	if (srs) pfree(srs);

	result = cstring2text(geojson);
	lwfree(geojson);

	PG_RETURN_TEXT_P(result);
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
	/* Extract the cstring from the varlena */
	char *wkt = text2cstring(wkt_text);
	/* Pass the cstring to the input parser, and magic occurs! */
	Datum rv = DirectFunctionCall3(geography_in, PointerGetDatum(wkt), Int32GetDatum(0), Int32GetDatum(-1));
	/* Clean up and return */
	pfree(wkt);
	PG_RETURN_DATUM(rv);
}

/*
** geography_as_binary(*GSERIALIZED) returns bytea
*/
PG_FUNCTION_INFO_V1(geography_as_binary);
Datum geography_as_binary(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	uchar *wkb = NULL;
	bytea *wkb_result;
	size_t wkb_size = 0;
	GSERIALIZED *g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Get our lwgeom form */
	lwgeom = lwgeom_from_gserialized(g);

	if ( FLAGS_NDIMS(lwgeom->flags) > 2 )
	{
		/* Strip out the higher dimensions */
		LWGEOM *tmp = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = tmp;
	}
	
	/* Create WKB */
	wkb = lwgeom_to_wkb(lwgeom, WKB_SFSQL, &wkb_size);
	
	/* Copy to varlena pointer */
	wkb_result = palloc(wkb_size + VARHDRSZ);
	SET_VARSIZE(wkb_result, wkb_size + VARHDRSZ);
	memcpy(VARDATA(wkb_result), wkb, wkb_size);

	/* Clean up */
	pfree(wkb);
	lwgeom_free(lwgeom);

	PG_RETURN_BYTEA_P(wkb_result);
}

/*
** geography_from_binary(*char) returns *GSERIALIZED
*/
PG_FUNCTION_INFO_V1(geography_from_binary);
Datum geography_from_binary(PG_FUNCTION_ARGS)
{
	char *wkb_cstring = NULL;
	text *wkb_hex = NULL;
	char *wkb_bytea = (char*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *hexarg = palloc(4 + VARHDRSZ);

	/* Create our second argument to binary_encode */
	SET_VARSIZE(hexarg, 4 + VARHDRSZ);
	memcpy(VARDATA(hexarg), "hex", 4);

	/* Convert the bytea into a hex representation cstring */
	wkb_hex = (text*)DatumGetPointer(DirectFunctionCall2(binary_encode, PointerGetDatum(wkb_bytea), PointerGetDatum(hexarg)));
	wkb_cstring = text2cstring(wkb_hex);
	pfree(hexarg);

	/* Pass the cstring to the input parser, and magic occurs! */
	PG_RETURN_DATUM(DirectFunctionCall3(geography_in, PointerGetDatum(wkb_cstring), Int32GetDatum(0), Int32GetDatum(-1)));
}

PG_FUNCTION_INFO_V1(geography_typmod_dims);
Datum geography_typmod_dims(PG_FUNCTION_ARGS)
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

PG_FUNCTION_INFO_V1(geography_typmod_srid);
Datum geography_typmod_srid(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	if ( typmod < 0 )
		PG_RETURN_INT32(0);
	PG_RETURN_INT32(TYPMOD_GET_SRID(typmod));
}

PG_FUNCTION_INFO_V1(geography_typmod_type);
Datum geography_typmod_type(PG_FUNCTION_ARGS)
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

PG_FUNCTION_INFO_V1(geography_from_geometry);
Datum geography_from_geometry(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g_ser = NULL;

	geography_valid_type(pglwgeom_get_type(geom));

	lwgeom = pglwgeom_deserialize(geom);

	/* Force default SRID */
	if ( (int)lwgeom->srid <= 0 )
	{
		lwgeom->srid = SRID_DEFAULT;
	}

	/* Error on any SRID != default */
	if ( lwgeom->srid != SRID_DEFAULT )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Only SRID %d is currently supported in geography.", SRID_DEFAULT)));
	}

	/* Check if the geography has valid coordinate range. */
	if ( lwgeom_check_geodetic(lwgeom) == LW_FALSE )
	{
		ereport(ERROR, (
		            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		            errmsg("Coordinate values are out of range [-180 -90, 180 90] for GEOGRAPHY type" )));
	}

	/*
	** Serialize our lwgeom and set the geodetic flag so subsequent
	** functions do the right thing.
	*/
	lwgeom_set_geodetic(lwgeom, true);
	/* Recalculate the boxes after re-setting the geodetic bit */
	lwgeom_drop_bbox(lwgeom);
	lwgeom_add_bbox(lwgeom);
	g_ser = geography_serialize(lwgeom);

	/*
	** Replace the unaligned lwgeom with a new aligned one based on GSERIALIZED.
	*/
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(g_ser);

}

PG_FUNCTION_INFO_V1(geometry_from_geography);
Datum geometry_from_geography(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	PG_LWGEOM *ret = NULL;
	GSERIALIZED *g_ser = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_from_gserialized(g_ser);

	/* Recalculate the boxes after re-setting the geodetic bit */
	lwgeom_set_geodetic(lwgeom, false);	
	lwgeom_drop_bbox(lwgeom);
	lwgeom_add_bbox(lwgeom);

	/* We want "geometry" to think all our "geography" has an SRID, and the
	   implied SRID is the default, so we fill that in if our SRID is actually unknown. */
	if ( (int)lwgeom->srid <= 0 )
		lwgeom->srid = SRID_DEFAULT;

	ret = pglwgeom_serialize(lwgeom);
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(ret);
}

