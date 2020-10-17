/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * ^copyright^
 *
 **********************************************************************/

#include "postgres.h"

#include "../postgis_config.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "access/gist.h"
#include "access/itup.h"

#include "fmgr.h"
#include "utils/elog.h"
#include "mb/pg_wchar.h"
#include "lib/stringinfo.h" /* for binary input */
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "funcapi.h"

#include "liblwgeom.h"
#include "lwgeom_cache.h"
#include "lwgeom_pg.h"
#include "geography.h" /* for lwgeom_valid_typmod */
#include "lwgeom_transform.h"


#include "access/htup_details.h"


void elog_ERROR(const char* string);

Datum LWGEOM_in(PG_FUNCTION_ARGS);
Datum LWGEOM_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS);
Datum LWGEOM_asHEXEWKB(PG_FUNCTION_ARGS);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS);
Datum LWGEOM_recv(PG_FUNCTION_ARGS);
Datum LWGEOM_send(PG_FUNCTION_ARGS);
Datum LWGEOM_to_latlon(PG_FUNCTION_ARGS);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS);
Datum TWKBFromLWGEOM(PG_FUNCTION_ARGS);
Datum TWKBFromLWGEOMArray(PG_FUNCTION_ARGS);
Datum LWGEOMFromTWKB(PG_FUNCTION_ARGS);

/*
 * LWGEOM_in(cstring)
 * format is '[SRID=#;]wkt|wkb'
 *  LWGEOM_in( 'SRID=99;POINT(0 0)')
 *  LWGEOM_in( 'POINT(0 0)')            --> assumes SRID=SRID_UNKNOWN
 *  LWGEOM_in( 'SRID=99;0101000000000000000000F03F000000000000004')
 *  LWGEOM_in( '0101000000000000000000F03F000000000000004')
 *  LWGEOM_in( '{"type":"Point","coordinates":[1,1]}')
 *  returns a GSERIALIZED object
 */
PG_FUNCTION_INFO_V1(LWGEOM_in);
Datum LWGEOM_in(PG_FUNCTION_ARGS)
{
	char *input = PG_GETARG_CSTRING(0);
	int32 geom_typmod = -1;
	char *str = input;
	LWGEOM_PARSER_RESULT lwg_parser_result;
	LWGEOM *lwgeom;
	GSERIALIZED *ret;
	int32_t srid = 0;

	if ( (PG_NARGS()>2) && (!PG_ARGISNULL(2)) ) {
		geom_typmod = PG_GETARG_INT32(2);
	}

	lwgeom_parser_result_init(&lwg_parser_result);

	/* Empty string. */
	if ( str[0] == '\0' ) {
		ereport(ERROR,(errmsg("parse error - invalid geometry")));
		PG_RETURN_NULL();
	}

	/* Starts with "SRID=" */
	if( strncasecmp(str,"SRID=",5) == 0 )
	{
		/* Roll forward to semi-colon */
		char *tmp = str;
		while ( tmp && *tmp != ';' )
			tmp++;

		/* Check next character to see if we have WKB  */
		if ( tmp && *(tmp+1) == '0' )
		{
			/* Null terminate the SRID= string */
			*tmp = '\0';
			/* Set str to the start of the real WKB */
			str = tmp + 1;
			/* Move tmp to the start of the numeric part */
			tmp = input + 5;
			/* Parse out the SRID number */
			srid = atoi(tmp);
		}
	}

	/* WKB? Let's find out. */
	if ( str[0] == '0' )
	{
		size_t hexsize = strlen(str);
		unsigned char *wkb = bytes_from_hexbytes(str, hexsize);
		/* TODO: 20101206: No parser checks! This is inline with current 1.5 behavior, but needs discussion */
		lwgeom = lwgeom_from_wkb(wkb, hexsize/2, LW_PARSER_CHECK_NONE);
		/* If we picked up an SRID at the head of the WKB set it manually */
		if ( srid ) lwgeom_set_srid(lwgeom, srid);
		/* Add a bbox if necessary */
		if ( lwgeom_needs_bbox(lwgeom) ) lwgeom_add_bbox(lwgeom);
		lwfree(wkb);
		ret = geometry_serialize(lwgeom);
		lwgeom_free(lwgeom);
	}
	else if (str[0] == '{')
	{
		char *srs = NULL;
		lwgeom = lwgeom_from_geojson(str, &srs);
		if (srs)
		{
			srid = GetSRIDCacheBySRS(fcinfo, srs);
			lwfree(srs);
			lwgeom_set_srid(lwgeom, srid);
		}
		ret = geometry_serialize(lwgeom);
		lwgeom_free(lwgeom);
	}
	/* WKT then. */
	else
	{
		if ( lwgeom_parse_wkt(&lwg_parser_result, str, LW_PARSER_CHECK_ALL) == LW_FAILURE )
		{
			PG_PARSER_ERROR(lwg_parser_result);
			PG_RETURN_NULL();
		}
		lwgeom = lwg_parser_result.geom;
		if ( lwgeom_needs_bbox(lwgeom) )
			lwgeom_add_bbox(lwgeom);
		ret = geometry_serialize(lwgeom);
		lwgeom_parser_result_free(&lwg_parser_result);
	}

	if ( geom_typmod >= 0 )
	{
		ret = postgis_valid_typmod(ret, geom_typmod);
		POSTGIS_DEBUG(3, "typmod and geometry were consistent");
	}
	else
	{
		POSTGIS_DEBUG(3, "typmod was -1");
	}

	/* Don't free the parser result (and hence lwgeom) until we have done */
	/* the typemod check with lwgeom */

	PG_RETURN_POINTER(ret);

}

/*
 * LWGEOM_to_latlon(GEOMETRY, text)
 *  NOTE: Geometry must be a point.  It is assumed that the coordinates
 *        of the point are in a lat/lon projection, and they will be
 *        normalized in the output to -90-90 and -180-180.
 *
 *  The text parameter is a format string containing the format for the
 *  resulting text, similar to a date format string.  Valid tokens
 *  are "D" for degrees, "M" for minutes, "S" for seconds, and "C" for
 *  cardinal direction (NSEW).  DMS tokens may be repeated to indicate
 *  desired width and precision ("SSS.SSSS" means "  1.0023").
 *  "M", "S", and "C" are optional.  If "C" is omitted, degrees are
 *  shown with a "-" sign if south or west.  If "S" is omitted,
 *  minutes will be shown as decimal with as many digits of precision
 *  as you specify.  If "M" is omitted, degrees are shown as decimal
 *  with as many digits precision as you specify.
 *
 *  If the format string is omitted (null or 0-length) a default
 *  format will be used.
 *
 *  returns text
 */
PG_FUNCTION_INFO_V1(LWGEOM_to_latlon);
Datum LWGEOM_to_latlon(PG_FUNCTION_ARGS)
{
	/* Get the parameters */
	GSERIALIZED *pg_lwgeom = PG_GETARG_GSERIALIZED_P(0);
	text *format_text = PG_GETARG_TEXT_P(1);

	LWGEOM *lwgeom;
	char *format_str = NULL;

	char * formatted_str;
	text * formatted_text;
	char * tmp;

	/* Only supports points. */
	uint8_t geom_type = gserialized_get_type(pg_lwgeom);
	if (POINTTYPE != geom_type)
	{
		lwpgerror("Only points are supported, you tried type %s.", lwtype_name(geom_type));
	}
	/* Convert to LWGEOM type */
	lwgeom = lwgeom_from_gserialized(pg_lwgeom);

  if (format_text == NULL) {
    lwpgerror("ST_AsLatLonText: invalid format string (null");
    PG_RETURN_NULL();
  }

	format_str = text_to_cstring(format_text);
  assert(format_str != NULL);

  /* The input string supposedly will be in the database encoding,
     so convert to UTF-8. */
  tmp = (char *)pg_do_encoding_conversion(
    (uint8_t *)format_str, strlen(format_str), GetDatabaseEncoding(), PG_UTF8);
  assert(tmp != NULL);
  if ( tmp != format_str ) {
    pfree(format_str);
    format_str = tmp;
  }

	/* Produce the formatted string. */
	formatted_str = lwpoint_to_latlon((LWPOINT *)lwgeom, format_str);
  assert(formatted_str != NULL);
  pfree(format_str);

  /* Convert the formatted string from UTF-8 back to database encoding. */
  tmp = (char *)pg_do_encoding_conversion(
    (uint8_t *)formatted_str, strlen(formatted_str),
    PG_UTF8, GetDatabaseEncoding());
  assert(tmp != NULL);
  if ( tmp != formatted_str) {
    pfree(formatted_str);
    formatted_str = tmp;
  }

	/* Convert to the postgres output string type. */
	formatted_text = cstring_to_text(formatted_str);
  pfree(formatted_str);

	PG_RETURN_POINTER(formatted_text);
}

/*
 * LWGEOM_out(lwgeom) --> cstring
 * output is 'SRID=#;<wkb in hex form>'
 * ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
 * WKB is machine endian
 * if SRID=-1, the 'SRID=-1;' will probably not be present.
 */
PG_FUNCTION_INFO_V1(LWGEOM_out);
Datum LWGEOM_out(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_CSTRING(lwgeom_to_hexwkb_buffer(lwgeom, WKB_EXTENDED));
}

/*
 * AsHEXEWKB(geom, string)
 */
PG_FUNCTION_INFO_V1(LWGEOM_asHEXEWKB);
Datum LWGEOM_asHEXEWKB(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom;
	uint8_t variant = 0;

	/* If user specified endianness, respect it */
	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		text *type = PG_GETARG_TEXT_P(1);

		if  ( ! strncmp(VARDATA(type), "xdr", 3) ||
		      ! strncmp(VARDATA(type), "XDR", 3) )
		{
			variant = variant | WKB_XDR;
		}
		else
		{
			variant = variant | WKB_NDR;
		}
	}

	/* Create WKB hex string */
	lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_TEXT_P(lwgeom_to_hexwkb_varlena(lwgeom, variant | WKB_EXTENDED));
}


/*
 * LWGEOM_to_text(lwgeom) --> text
 * output is 'SRID=#;<wkb in hex form>'
 * ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
 * WKB is machine endian
 * if SRID=-1, the 'SRID=-1;' will probably not be present.
 */
PG_FUNCTION_INFO_V1(LWGEOM_to_text);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_TEXT_P(lwgeom_to_hexwkb_varlena(lwgeom, WKB_EXTENDED));
}

/*
 * LWGEOMFromEWKB(wkb,  [SRID] )
 * NOTE: wkb is in *binary* not hex form.
 *
 * NOTE: this function parses EWKB (extended form)
 *       which also contains SRID info.
 */
PG_FUNCTION_INFO_V1(LWGEOMFromEWKB);
Datum LWGEOMFromEWKB(PG_FUNCTION_ARGS)
{
	bytea *bytea_wkb = PG_GETARG_BYTEA_P(0);
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	uint8_t *wkb = (uint8_t*)VARDATA(bytea_wkb);

	lwgeom = lwgeom_from_wkb(wkb, VARSIZE_ANY_EXHDR(bytea_wkb), LW_PARSER_CHECK_ALL);
	if (!lwgeom)
		lwpgerror("Unable to parse WKB");

	if ((PG_NARGS() > 1) && (!PG_ARGISNULL(1)))
	{
		int32 srid = PG_GETARG_INT32(1);
		lwgeom_set_srid(lwgeom, srid);
	}

	if ( lwgeom_needs_bbox(lwgeom) )
		lwgeom_add_bbox(lwgeom);

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(bytea_wkb, 0);
	PG_RETURN_POINTER(geom);
}
/*
 * LWGEOMFromTWKB(wkb)
 * NOTE: twkb is in *binary* not hex form.
 *
 */
PG_FUNCTION_INFO_V1(LWGEOMFromTWKB);
Datum LWGEOMFromTWKB(PG_FUNCTION_ARGS)
{
	bytea *bytea_twkb = PG_GETARG_BYTEA_P(0);
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	uint8_t *twkb = (uint8_t*)VARDATA(bytea_twkb);

	lwgeom = lwgeom_from_twkb(twkb, VARSIZE_ANY_EXHDR(bytea_twkb), LW_PARSER_CHECK_ALL);

	if (lwgeom_needs_bbox(lwgeom))
		lwgeom_add_bbox(lwgeom);

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(bytea_twkb, 0);
	PG_RETURN_POINTER(geom);
}

/*
 * WKBFromLWGEOM(lwgeom) --> wkb
 * this will have no 'SRID=#;'
 */
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom;
	uint8_t variant = 0;

	/* If user specified endianness, respect it */
	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		text *type = PG_GETARG_TEXT_P(1);

		if  ( ! strncmp(VARDATA(type), "xdr", 3) ||
		      ! strncmp(VARDATA(type), "XDR", 3) )
		{
			variant = variant | WKB_XDR;
		}
		else
		{
			variant = variant | WKB_NDR;
		}
	}

	/* Create WKB hex string */
	lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_BYTEA_P(lwgeom_to_wkb_varlena(lwgeom, variant | WKB_EXTENDED));
}

PG_FUNCTION_INFO_V1(TWKBFromLWGEOM);
Datum TWKBFromLWGEOM(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	uint8_t variant = 0;
	srs_precision sp;

	/*check for null input since we cannot have the sql-function as strict.
	That is because we use null as default for optional ID*/
	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = PG_GETARG_GSERIALIZED_P(0);

	/* Read sensible precision defaults (about one meter) given the srs */
	sp = srid_axis_precision(fcinfo, gserialized_get_srid(geom), TWKB_DEFAULT_PRECISION);

	/* If user specified XY precision, use it */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		sp.precision_xy = PG_GETARG_INT32(1);

	/* If user specified Z precision, use it */
	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
		sp.precision_z = PG_GETARG_INT32(2);

	/* If user specified M precision, use it */
	if ( PG_NARGS() > 3 && ! PG_ARGISNULL(3) )
		sp.precision_m = PG_GETARG_INT32(3);

	/* We don't permit ids for single geoemtries */
	variant = variant & ~TWKB_ID;

	/* If user wants registered twkb sizes */
	if ( PG_NARGS() > 4 && ! PG_ARGISNULL(4) && PG_GETARG_BOOL(4) )
		variant |= TWKB_SIZE;

	/* If user wants bounding boxes */
	if ( PG_NARGS() > 5 && ! PG_ARGISNULL(5) && PG_GETARG_BOOL(5) )
		variant |= TWKB_BBOX;

	/* Create TWKB binary string */
	lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_BYTEA_P(lwgeom_to_twkb(lwgeom, variant, sp.precision_xy, sp.precision_z, sp.precision_m));
}


PG_FUNCTION_INFO_V1(TWKBFromLWGEOMArray);
Datum TWKBFromLWGEOMArray(PG_FUNCTION_ARGS)
{
	ArrayType *arr_geoms = NULL;
	ArrayType *arr_ids = NULL;
	int num_geoms, num_ids, i = 0;

	ArrayIterator iter_geoms, iter_ids;
	bool null_geom, null_id;
	Datum val_geom, val_id;

	int is_homogeneous = true;
	uint32_t subtype = 0;
	int has_z = 0;
	int has_m  = 0;
	LWCOLLECTION *col = NULL;
	int64_t *idlist = NULL;
	uint8_t variant = 0;

	srs_precision sp;

	/* The first two arguments are required */
	if ( PG_NARGS() < 2 || PG_ARGISNULL(0) || PG_ARGISNULL(1) )
		PG_RETURN_NULL();

	arr_geoms = PG_GETARG_ARRAYTYPE_P(0);
	arr_ids = PG_GETARG_ARRAYTYPE_P(1);

	num_geoms = ArrayGetNItems(ARR_NDIM(arr_geoms), ARR_DIMS(arr_geoms));
	num_ids = ArrayGetNItems(ARR_NDIM(arr_ids), ARR_DIMS(arr_ids));

	if ( num_geoms != num_ids )
	{
		elog(ERROR, "size of geometry[] and integer[] arrays must match");
		PG_RETURN_NULL();
	}

	/* Loop through array and build a collection of geometry and */
	/* a simple array of ids. If either side is NULL, skip it */

	iter_geoms = array_create_iterator(arr_geoms, 0, NULL);
	iter_ids = array_create_iterator(arr_ids, 0, NULL);

	while( array_iterate(iter_geoms, &val_geom, &null_geom) &&
	       array_iterate(iter_ids, &val_id, &null_id) )
	{
		LWGEOM *geom;
		int32_t uid;

		if ( null_geom || null_id )
		{
			elog(NOTICE, "ST_AsTWKB skipping NULL entry at position %d", i);
			continue;
		}

		geom = lwgeom_from_gserialized((GSERIALIZED*)DatumGetPointer(val_geom));
		uid = DatumGetInt64(val_id);

		/* Construct collection/idlist first time through */
		if ( ! col )
		{
			has_z = lwgeom_has_z(geom);
			has_m = lwgeom_has_m(geom);
			col = lwcollection_construct_empty(COLLECTIONTYPE, lwgeom_get_srid(geom), has_z, has_m);
		}
		if ( ! idlist )
			idlist = palloc0(num_geoms * sizeof(int64_t));


		/* Check if there is differences in dimensionality*/
		if( lwgeom_has_z(geom)!=has_z || lwgeom_has_m(geom)!=has_m)
		{
			elog(ERROR, "Geometries have different dimensionality");
			PG_FREE_IF_COPY(arr_geoms, 0);
			PG_FREE_IF_COPY(arr_ids, 1);
			PG_RETURN_NULL();
		}
		/* Store the values */
		lwcollection_add_lwgeom(col, geom);
		idlist[i++] = uid;

		/* Grab the geometry type and note if all geometries share it */
		/* If so, we can make this a homogeneous collection and save some space */
		if ( lwgeom_get_type(geom) != subtype && subtype )
		{
			is_homogeneous = false;
		}
		else
		{
			subtype = lwgeom_get_type(geom);
		}

	}
	array_free_iterator(iter_geoms);
	array_free_iterator(iter_ids);

	if(i==0)
	{
		elog(NOTICE, "No valid geometry - id pairs found");
		PG_FREE_IF_COPY(arr_geoms, 0);
		PG_FREE_IF_COPY(arr_ids, 1);
		PG_RETURN_NULL();
	}
	if ( is_homogeneous )
	{
		col->type = lwtype_get_collectiontype(subtype);
	}

	/* Read sensible precision defaults (about one meter) given the srs */
	sp = srid_axis_precision(fcinfo, lwgeom_get_srid(lwcollection_as_lwgeom(col)), TWKB_DEFAULT_PRECISION);

	/* If user specified XY precision, use it */
	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
		sp.precision_xy = PG_GETARG_INT32(2);

	/* If user specified Z precision, use it */
	if ( PG_NARGS() > 3 && ! PG_ARGISNULL(3) )
		sp.precision_z = PG_GETARG_INT32(3);

	/* If user specified M precision, use it */
	if ( PG_NARGS() > 4 && ! PG_ARGISNULL(4) )
		sp.precision_m = PG_GETARG_INT32(4);

	/* We are building an ID'ed output */
	variant = TWKB_ID;

	/* If user wants registered twkb sizes */
	if ( PG_NARGS() > 5 && ! PG_ARGISNULL(5) && PG_GETARG_BOOL(5) )
		variant |= TWKB_SIZE;

	/* If user wants bounding boxes */
	if ( PG_NARGS() > 6 && ! PG_ARGISNULL(6) && PG_GETARG_BOOL(6) )
		variant |= TWKB_BBOX;

	/* Write out the TWKB */
	PG_RETURN_BYTEA_P(lwgeom_to_twkb_with_idlist(
	    lwcollection_as_lwgeom(col), idlist, variant, sp.precision_xy, sp.precision_z, sp.precision_m));
}


/* puts a bbox inside the geometry */
PG_FUNCTION_INFO_V1(LWGEOM_addBBOX);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *result;
	LWGEOM *lwgeom;

	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_add_bbox(lwgeom);
	result = geometry_serialize(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/* removes a bbox from a geometry */
PG_FUNCTION_INFO_V1(LWGEOM_dropBBOX);
Datum LWGEOM_dropBBOX(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);

	/* No box? we're done already! */
	if ( ! gserialized_has_bbox(geom) )
		PG_RETURN_POINTER(geom);

	PG_RETURN_POINTER(gserialized_drop_gbox(geom));
}


/* for the wkt parser */
void elog_ERROR(const char* string)
{
	elog(ERROR, "%s", string);
}

/*
* This just does the same thing as the _in function,
* except it has to handle a 'text' input. First
* unwrap the text into a cstring, then call
* geometry_in
*/
PG_FUNCTION_INFO_V1(parse_WKT_lwgeom);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS)
{
	text *wkt_text = PG_GETARG_TEXT_P(0);
	char *wkt;
	Datum result;

	/* Unwrap the PgSQL text type into a cstring */
	wkt = text_to_cstring(wkt_text);

	/* Now we call over to the geometry_in function
	 * We need to initialize the fcinfo since cache might be used
	 */
	result = CallerFInfoFunctionCall1(LWGEOM_in, fcinfo->flinfo, InvalidOid, CStringGetDatum(wkt));

	/* Return null on null */
	if ( ! result )
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}


/*
 * This function must advance the StringInfo.cursor pointer
 * and leave it at the end of StringInfo.buf. If it fails
 * to do so the backend will raise an exception with message:
 * ERROR:  incorrect binary data format in bind parameter #
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_recv);
Datum LWGEOM_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
	int32 geom_typmod = -1;
	GSERIALIZED *geom;
	LWGEOM *lwgeom;

	if ( (PG_NARGS()>2) && (!PG_ARGISNULL(2)) ) {
		geom_typmod = PG_GETARG_INT32(2);
	}

	lwgeom = lwgeom_from_wkb((uint8_t*)buf->data, buf->len, LW_PARSER_CHECK_ALL);

	if ( lwgeom_needs_bbox(lwgeom) )
		lwgeom_add_bbox(lwgeom);

	/* Set cursor to the end of buffer (so the backend is happy) */
	buf->cursor = buf->len;

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	if ( geom_typmod >= 0 )
	{
		geom = postgis_valid_typmod(geom, geom_typmod);
		POSTGIS_DEBUG(3, "typmod and geometry were consistent");
	}
	else
	{
		POSTGIS_DEBUG(3, "typmod was -1");
	}


	PG_RETURN_POINTER(geom);
}



PG_FUNCTION_INFO_V1(LWGEOM_send);
Datum LWGEOM_send(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(2, "LWGEOM_send called");

	PG_RETURN_POINTER(
	  DatumGetPointer(
	    DirectFunctionCall1(
	      WKBFromLWGEOM,
	      PG_GETARG_DATUM(0)
	    )));
}

PG_FUNCTION_INFO_V1(LWGEOM_to_bytea);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(2, "LWGEOM_to_bytea called");

	PG_RETURN_POINTER(
	  DatumGetPointer(
	    DirectFunctionCall1(
	      WKBFromLWGEOM,
	      PG_GETARG_DATUM(0)
	    )));
}

PG_FUNCTION_INFO_V1(LWGEOM_from_bytea);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;

	POSTGIS_DEBUG(2, "LWGEOM_from_bytea start");

	result = (GSERIALIZED *)DatumGetPointer(DirectFunctionCall1(
	                                          LWGEOMFromEWKB, PG_GETARG_DATUM(0)));

	PG_RETURN_POINTER(result);
}

