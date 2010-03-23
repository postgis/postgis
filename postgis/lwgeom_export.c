/**********************************************************************
 * $Id:$
 *
 * PostGIS - Export functions for PostgreSQL/PostGIS
 * Copyright 2009 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/


/** @file
 *  Commons functions for all export functions
 */

#include "postgres.h"
#include "executor/spi.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"

Datum LWGEOM_asGML(PG_FUNCTION_ARGS);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS);
Datum LWGEOM_asSVG(PG_FUNCTION_ARGS);


/*
 * Retrieve an SRS from a given SRID
 * Require valid spatial_ref_sys table entry
 *
 * Could return SRS as short one (i.e EPSG:4326)
 * or as long one: (i.e urn:ogc:def:crs:EPSG:4326)
 */
char * getSRSbySRID(int SRID, bool short_crs)
{
	char query[256];
	char *srs, *srscopy;
	int size, err;

	if (SPI_OK_CONNECT != SPI_connect ())
	{
		elog(NOTICE, "getSRSbySRID: could not connect to SPI manager");
		SPI_finish();
		return NULL;
	}

	if (short_crs)
		sprintf(query, "SELECT auth_name||':'||auth_srid \
		        FROM spatial_ref_sys WHERE srid='%d'", SRID);
	else
		sprintf(query, "SELECT 'urn:ogc:def:crs:'||auth_name||':'||auth_srid \
		        FROM spatial_ref_sys WHERE srid='%d'", SRID);

	err = SPI_exec(query, 1);
	if ( err < 0 )
	{
		elog(NOTICE, "getSRSbySRID: error executing query %d", err);
		SPI_finish();
		return NULL;
	}

	/* no entry in spatial_ref_sys */
	if (SPI_processed <= 0)
	{
		SPI_finish();
		return NULL;
	}

	/* get result  */
	srs = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);

	/* NULL result */
	if ( ! srs )
	{
		SPI_finish();
		return NULL;
	}

	/* copy result to upper executor context */
	size = strlen(srs)+1;
	srscopy = SPI_palloc(size);
	memcpy(srscopy, srs, size);

	/* disconnect from SPI */
	SPI_finish();

	return srscopy;
}


/**
 * Encode feature in GML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGML);
Datum LWGEOM_asGML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *gml;
	text *result;
	int len;
	int version;
	char *srs;
	int SRID;
	int option = 0;
	int is_deegree = 0;
	int precision = OUT_MAX_DOUBLE_PRECISION;
	static const char* default_prefix = "gml:"; /* default prefix */
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

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

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

	SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
	if (SRID == -1)      srs = NULL;
	else if (option & 1) srs = getSRSbySRID(SRID, false);
	else                 srs = getSRSbySRID(SRID, true);

	if (option & 16) is_deegree = 1;

	if (version == 2)
		gml = lwgeom_to_gml2(SERIALIZED_FORM(geom), srs, precision, prefix);
	else
		gml = lwgeom_to_gml3(SERIALIZED_FORM(geom), srs, precision, is_deegree, prefix);

	PG_FREE_IF_COPY(geom, 1);

	len = strlen(gml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), gml, len-VARHDRSZ);

	lwfree(gml);

	PG_RETURN_POINTER(result);
}


/**
 * Encode feature in KML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *kml;
	text *result;
	int len;
	int version;
	int precision = OUT_MAX_DOUBLE_PRECISION;


	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 2)
	{
		elog(ERROR, "Only KML 2 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if ( PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > OUT_MAX_DOUBLE_PRECISION )
			precision = OUT_MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	kml = lwgeom_to_kml2(SERIALIZED_FORM(geom), precision);

	PG_FREE_IF_COPY(geom, 1);

	len = strlen(kml) + VARHDRSZ;

	result = palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), kml, len-VARHDRSZ);

	lwfree(kml);

	PG_RETURN_POINTER(result);
}


/**
 * Encode Feature in GeoJson
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGeoJson);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *geojson;
	text *result;
	int SRID;
	int len;
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

	/* Get the geometry */
	if (PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

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
		SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
		if ( SRID != -1 )
		{
			if (option & 2) srs = getSRSbySRID(SRID, true);
			if (option & 4) srs = getSRSbySRID(SRID, false);
			if (!srs)
			{
				elog(	ERROR, 
					"SRID %i unknown in spatial_ref_sys table",
					SRID);
				PG_RETURN_NULL();
			}
		}
	}

	if (option & 1) has_bbox = 1;

	geojson = lwgeom_to_geojson(SERIALIZED_FORM(geom), srs, precision, has_bbox);

	PG_FREE_IF_COPY(geom, 1);
	if (srs) pfree(srs);

	len = strlen(geojson) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), geojson, len-VARHDRSZ);

	lwfree(geojson);

	PG_RETURN_POINTER(result);
}


/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(LWGEOM_asSVG);
Datum LWGEOM_asSVG(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *svg;
	text *result;
	int len;
	int relative = 0;
	int precision=OUT_MAX_DOUBLE_PRECISION;

	if ( PG_ARGISNULL(0) ) PG_RETURN_NULL();

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

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

	svg = lwgeom_to_svg(SERIALIZED_FORM(geom), precision, relative);
	PG_FREE_IF_COPY(geom, 0);

	len = strlen(svg) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), svg, len-VARHDRSZ);

	lwfree(svg);

	PG_RETURN_POINTER(result);
}
