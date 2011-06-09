/**********************************************************************
 * $Id:$
 *
 * PostGIS - Export functions for PostgreSQL/PostGIS
 * Copyright 2009-2011 Olivier Courtin <olivier.courtin@oslandia.com>
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
Datum LWGEOM_asX3D(PG_FUNCTION_ARGS);

/*
 * Retrieve an SRS from a given SRID
 * Require valid spatial_ref_sys table entry
 *
 * Could return SRS as short one (i.e EPSG:4326)
 * or as long one: (i.e urn:ogc:def:crs:EPSG::4326)
 */
char * getSRSbySRID(int srid, bool short_crs)
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
		        FROM spatial_ref_sys WHERE srid='%d'", srid);
	else
		sprintf(query, "SELECT 'urn:ogc:def:crs:'||auth_name||'::'||auth_srid \
		        FROM spatial_ref_sys WHERE srid='%d'", srid);

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
	LWGEOM *lwgeom;
	char *gml = NULL;
	text *result;
	int version;
	char *srs;
	int srid;
	int option = 0;
	int lwopts = LW_GML_IS_DIMS;
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

	srid = pglwgeom_get_srid(geom);
	if (srid == SRID_UNKNOWN)      srs = NULL;
	else if (option & 1) srs = getSRSbySRID(srid, false);
	else                 srs = getSRSbySRID(srid, true);

	if (option & 2)  lwopts &= ~LW_GML_IS_DIMS; 
	if (option & 4)  lwopts |= LW_GML_SHORTLINE;
	if (option & 16) lwopts |= LW_GML_IS_DEGREE;
        if (option & 32) lwopts |= LW_GML_EXTENT;

	lwgeom = pglwgeom_deserialize(geom);

        if (version == 2 && lwopts & LW_GML_EXTENT)
		gml = lwgeom_extent_to_gml2(lwgeom, srs, precision, prefix);
	else if (version == 2)
		gml = lwgeom_to_gml2(lwgeom, srs, precision, prefix);
        else if (version == 3 && lwopts & LW_GML_EXTENT)
		gml = lwgeom_extent_to_gml3(lwgeom, srs, precision, lwopts, prefix);
	else if (version == 3) 
		gml = lwgeom_to_gml3(lwgeom, srs, precision, lwopts, prefix);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 1);

	result = cstring2text(gml);
	lwfree(gml);

	PG_RETURN_TEXT_P(result);
}


/**
 * Encode feature in KML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	char *kml;
	text *result;
	int version;
	int precision = OUT_MAX_DOUBLE_PRECISION;
	static const char* default_prefix = ""; /* default prefix */
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
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

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

	lwgeom = pglwgeom_deserialize(geom);
	kml = lwgeom_to_kml2(lwgeom, precision, prefix);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 1);
	
	if( ! kml ) 
		PG_RETURN_NULL();	

	result = cstring2text(kml);
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
	LWGEOM *lwgeom;
	char *geojson;
	text *result;
	int srid;
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
		srid = pglwgeom_get_srid(geom);
		if ( srid != SRID_UNKNOWN )
		{
			if (option & 2) srs = getSRSbySRID(srid, true);
			if (option & 4) srs = getSRSbySRID(srid, false);
			if (!srs)
			{
				elog(	ERROR,
				      "SRID %i unknown in spatial_ref_sys table",
				      srid);
				PG_RETURN_NULL();
			}
		}
	}

	if (option & 1) has_bbox = 1;

	lwgeom = pglwgeom_deserialize(geom);
	geojson = lwgeom_to_geojson(lwgeom, srs, precision, has_bbox);
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(geom, 1);
	if (srs) pfree(srs);

	result = cstring2text(geojson);

	lwfree(geojson);

	PG_RETURN_TEXT_P(result);
}


/**
 * SVG features
 */
PG_FUNCTION_INFO_V1(LWGEOM_asSVG);
Datum LWGEOM_asSVG(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	char *svg;
	text *result;
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

	lwgeom = pglwgeom_deserialize(geom);
	svg = lwgeom_to_svg(lwgeom, precision, relative);
	result = cstring2text(svg);
	lwgeom_free(lwgeom);
	pfree(svg);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_TEXT_P(result);
}

/**
 * Encode feature as X3D
 */
PG_FUNCTION_INFO_V1(LWGEOM_asX3D);
Datum LWGEOM_asX3D(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	char *x3d;
	text *result;
	int version;
	char *srs;
	int srid;
	int option = 0;
	int is_deegree = 0;
	int is_dims = 1;
	int precision = OUT_MAX_DOUBLE_PRECISION;
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

	/* retrieve defid */
	if (PG_NARGS() >4 && !PG_ARGISNULL(4))
	{
		defid_text = PG_GETARG_TEXT_P(4);
		if ( VARSIZE(defid_text)-VARHDRSZ == 0 )
		{
			defid = "";
		}
		else
		{
			/* +2 is one for the ':' and one for term null */
			defidbuf = palloc(VARSIZE(defid_text)-VARHDRSZ+2);
			memcpy(defidbuf, VARDATA(defid_text),
			       VARSIZE(defid_text)-VARHDRSZ);
			/* add colon and null terminate */
			defidbuf[VARSIZE(defid_text)-VARHDRSZ] = ':';
			defidbuf[VARSIZE(defid_text)-VARHDRSZ+1] = '\0';
			defid = defidbuf;
		}
	}

	srid = pglwgeom_get_srid(geom);
	if (srid == SRID_UNKNOWN)      srs = NULL;
	else if (option & 1) srs = getSRSbySRID(srid, false);
	else                 srs = getSRSbySRID(srid, true);

	if (option & 2)  is_dims = 0;
	if (option & 16) is_deegree = 1;

	lwgeom = pglwgeom_deserialize(geom);


	x3d = lwgeom_to_x3d3(lwgeom, srs, precision,option, defid);

	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 1);

	result = cstring2text(x3d);
	lwfree(x3d);

	PG_RETURN_TEXT_P(result);
}
