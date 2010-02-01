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
