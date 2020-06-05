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
 * Copyright 2009-2011 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 **********************************************************************/



/** @file
 *  Commons functions for all export functions
 */

#include "postgres.h"
#include "catalog/pg_type.h" /* for INT4OID */
#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/jsonb.h"

#include "../postgis_config.h"
#include "lwgeom_cache.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"

Datum LWGEOM_asGML(PG_FUNCTION_ARGS);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS);
Datum LWGEOM_asGeoJson_old(PG_FUNCTION_ARGS);
Datum LWGEOM_asSVG(PG_FUNCTION_ARGS);
Datum LWGEOM_asX3D(PG_FUNCTION_ARGS);
Datum LWGEOM_asEncodedPolyline(PG_FUNCTION_ARGS);
Datum geometry_to_json(PG_FUNCTION_ARGS);
Datum geometry_to_jsonb(PG_FUNCTION_ARGS);

/**
 * Encode feature in GML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGML);
Datum LWGEOM_asGML(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	lwvarlena_t *v = NULL;
	int version;
	const char *srs;
	int32_t srid;
	int option = 0;
	int lwopts = LW_GML_IS_DIMS;
	int precision = OUT_DEFAULT_DECIMAL_DIGITS;
	static const char* default_prefix = "gml:"; /* default prefix */
	const char* prefix = default_prefix;
	const char* gml_id = NULL;
	size_t len;
	char *gml_id_buf, *prefix_buf;
	text *prefix_text, *gml_id_text;

	/*
	 * Two potential callers, one starts with GML version,
	 * one starts with geometry, and we check for initial
	 * argument type and then dynamically change what args
	 * we read based on presence/absence
	 */
	Oid first_type = get_fn_expr_argtype(fcinfo->flinfo, 0);
	int argnum = 0;
	if (first_type != INT4OID)
	{
		version = 2;
	}
	else
	{
		/* Get the version */
		version = PG_GETARG_INT32(argnum++);
		if (version != 2 && version != 3)
		{
			elog(ERROR, "Only GML 2 and GML 3 are supported");
			PG_RETURN_NULL();
		}
	}

	/* Get the geometry */
	if (PG_ARGISNULL(argnum))
		PG_RETURN_NULL();
	geom = PG_GETARG_GSERIALIZED_P(argnum++);

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() > argnum && !PG_ARGISNULL(argnum))
	{
		precision = PG_GETARG_INT32(argnum);
	}
	argnum++;

	/* retrieve option */
	if (PG_NARGS() > argnum && !PG_ARGISNULL(argnum))
		option = PG_GETARG_INT32(argnum);
	argnum++;

	/* retrieve prefix */
	if (PG_NARGS() > argnum && !PG_ARGISNULL(argnum))
	{
		prefix_text = PG_GETARG_TEXT_P(argnum);
		if ( VARSIZE(prefix_text) == VARHDRSZ )
		{
			prefix = "";
		}
		else
		{
			len = VARSIZE_ANY_EXHDR(prefix_text);
			prefix_buf = palloc(len + 2); /* +2 is one for the ':' and one for term null */
			memcpy(prefix_buf, VARDATA(prefix_text), len);
			/* add colon and null terminate */
			prefix_buf[len] = ':';
			prefix_buf[len+1] = '\0';
			prefix = prefix_buf;
		}
	}
	argnum++;

	if (PG_NARGS() > argnum && !PG_ARGISNULL(argnum))
	{
		gml_id_text = PG_GETARG_TEXT_P(argnum);
		if ( VARSIZE(gml_id_text) == VARHDRSZ )
		{
			gml_id = "";
		}
		else
		{
			len = VARSIZE_ANY_EXHDR(gml_id_text);
			gml_id_buf = palloc(len+1);
			memcpy(gml_id_buf, VARDATA(gml_id_text), len);
			gml_id_buf[len] = '\0';
			gml_id = gml_id_buf;
		}
	}
	argnum++;

	srid = gserialized_get_srid(geom);
	if (srid == SRID_UNKNOWN)      srs = NULL;
	else if (option & 1)
		srs = GetSRSCacheBySRID(fcinfo, srid, false);
	else
		srs = GetSRSCacheBySRID(fcinfo, srid, true);

	if (option & 2) lwopts &= ~LW_GML_IS_DIMS;
	if (option & 4) lwopts |= LW_GML_SHORTLINE;
	if (option & 8)
	{
		elog(ERROR,
		     "Options %d passed to ST_AsGML(geometry) sets "
		     "unsupported value 8",
		     option);
		PG_RETURN_NULL();
	}
	if (option & 16) lwopts |= LW_GML_IS_DEGREE;
	if (option & 32) lwopts |= LW_GML_EXTENT;

	lwgeom = lwgeom_from_gserialized(geom);

	if (version == 2)
	{
		if (lwopts & LW_GML_EXTENT)
			v = lwgeom_extent_to_gml2(lwgeom, srs, precision, prefix);
		else
			v = lwgeom_to_gml2(lwgeom, srs, precision, prefix);
	}
	if (version == 3)
	{
		if (lwopts & LW_GML_EXTENT)
			v = lwgeom_extent_to_gml3(lwgeom, srs, precision, lwopts, prefix);
		else
			v = lwgeom_to_gml3(lwgeom, srs, precision, lwopts, prefix, gml_id);
	}

	if (!v)
		PG_RETURN_NULL();
	PG_RETURN_TEXT_P(v);
}


/**
 * Encode Feature in GeoJson
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGeoJson);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	int precision = OUT_DEFAULT_DECIMAL_DIGITS;
	int output_bbox = LW_FALSE;
	int output_long_crs = LW_FALSE;
	int output_short_crs = LW_FALSE;
	int output_guess_short_srid = LW_FALSE;
	const char *srs = NULL;
	int32_t srid;

	/* Get the geometry */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	geom = PG_GETARG_GSERIALIZED_P(0);
	srid = gserialized_get_srid(geom);

	/* Retrieve precision if any (default is max) */
	if ( PG_NARGS() > 1 && !PG_ARGISNULL(1) )
	{
		precision = PG_GETARG_INT32(1);
	}

	/* Retrieve output option
	 * 0 = without option
	 * 1 = bbox
	 * 2 = short crs
	 * 4 = long crs
	 * 8 = guess if CRS is needed (default)
	 */
	if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
	{
		int option = PG_GETARG_INT32(2);
		output_guess_short_srid = (option & 8) ? LW_TRUE : LW_FALSE;
		output_short_crs = (option & 2) ? LW_TRUE : LW_FALSE;
		output_long_crs = (option & 4) ? LW_TRUE : LW_FALSE;
		output_bbox = (option & 1) ? LW_TRUE : LW_FALSE;
	}
	else
		output_guess_short_srid = LW_TRUE;

	if (output_guess_short_srid && srid != WGS84_SRID && srid != SRID_UNKNOWN)
		output_short_crs = LW_TRUE;

	if (srid != SRID_UNKNOWN && (output_short_crs || output_long_crs))
	{
		srs = GetSRSCacheBySRID(fcinfo, srid, !output_long_crs);

		if (!srs)
		{
			elog(ERROR, "SRID %i unknown in spatial_ref_sys table", srid);
			PG_RETURN_NULL();
		}
	}

	lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_TEXT_P(lwgeom_to_geojson(lwgeom, srs, precision, output_bbox));
}


/**
 * Cast feature to JSON
 */
PG_FUNCTION_INFO_V1(geometry_to_json);
Datum geometry_to_json(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	lwvarlena_t *geojson = lwgeom_to_geojson(lwgeom, NULL, 15, 0);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_TEXT_P(geojson);
}

PG_FUNCTION_INFO_V1(geometry_to_jsonb);
Datum geometry_to_jsonb(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	lwvarlena_t *geojson = lwgeom_to_geojson(lwgeom, NULL, 15, 0);
	lwgeom_free(lwgeom);
	PG_RETURN_DATUM(DirectFunctionCall1(jsonb_in, PointerGetDatum(pstrdup(geojson->data))));
}


/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(LWGEOM_asSVG);
Datum LWGEOM_asSVG(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	int relative = 0;
	int precision = OUT_DEFAULT_DECIMAL_DIGITS;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = PG_GETARG_GSERIALIZED_P(0);

	/* check for relative path notation */
	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) )
		relative = PG_GETARG_INT32(1) ? 1:0;

	if ( PG_NARGS() > 2 && ! PG_ARGISNULL(2) )
	{
		precision = PG_GETARG_INT32(2);
	}

	lwgeom = lwgeom_from_gserialized(geom);
	PG_RETURN_TEXT_P(lwgeom_to_svg(lwgeom, precision, relative));
}

/**
 * Encode feature as X3D
 */
PG_FUNCTION_INFO_V1(LWGEOM_asX3D);
Datum LWGEOM_asX3D(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	int version;
	int option = 0;
	int precision = OUT_DEFAULT_DECIMAL_DIGITS;
	static const char* default_defid = "x3d:"; /* default defid */
	char *defidbuf;
	const char* defid = default_defid;
	text *defid_text;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if (  version != 3 )
	{
		elog(ERROR, "Only X3D version 3 are supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = PG_GETARG_GSERIALIZED_P(1);

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
	}

	/* retrieve option */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);


	/* retrieve defid */
	if (PG_NARGS() >4 && !PG_ARGISNULL(4))
	{
		defid_text = PG_GETARG_TEXT_P(4);
		if ( VARSIZE_ANY_EXHDR(defid_text) == 0 )
		{
			defid = "";
		}
		else
		{
			/* +2 is one for the ':' and one for term null */
			defidbuf = palloc(VARSIZE_ANY_EXHDR(defid_text)+2);
			memcpy(defidbuf, VARDATA(defid_text),
			       VARSIZE_ANY_EXHDR(defid_text));
			/* add colon and null terminate */
			defidbuf[VARSIZE_ANY_EXHDR(defid_text)] = ':';
			defidbuf[VARSIZE_ANY_EXHDR(defid_text)+1] = '\0';
			defid = defidbuf;
		}
	}

	lwgeom = lwgeom_from_gserialized(geom);

	if (option & LW_X3D_USE_GEOCOORDS) {
		if (lwgeom->srid != 4326)
		{
			PG_FREE_IF_COPY(geom, 0);
			/** TODO: we need to support UTM and other coordinate systems supported by X3D eventually
			http://www.web3d.org/documents/specifications/19775-1/V3.2/Part01/components/geodata.html#t-earthgeoids **/
			elog(ERROR, "Only SRID 4326 is supported for geocoordinates.");
			PG_RETURN_NULL();
		}
	}

	PG_RETURN_TEXT_P(lwgeom_to_x3d3(lwgeom, precision, option, defid));
}

/**
 * Encode feature as Encoded Polyline
 */
PG_FUNCTION_INFO_V1(LWGEOM_asEncodedPolyline);
Datum LWGEOM_asEncodedPolyline(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	int precision = 5;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = PG_GETARG_GSERIALIZED_P(0);
	if (gserialized_get_srid(geom) != 4326) {
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "Only SRID 4326 is supported.");
		PG_RETURN_NULL();
	}
	lwgeom = lwgeom_from_gserialized(geom);

	if (PG_NARGS() > 1 && !PG_ARGISNULL(1))
	{
		precision = PG_GETARG_INT32(1);
		if ( precision < 0 ) precision = 5;
	}

	PG_RETURN_TEXT_P(lwgeom_to_encoded_polyline(lwgeom, precision));
}
