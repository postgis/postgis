/**********************************************************************
 * $Id: geography_inout.c 4535 2009-09-28 18:16:21Z colivier $
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

#include "libgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */

Datum geography_distance_sphere(PG_FUNCTION_ARGS);


/*
** geography_in(cstring) returns *GSERIALIZED
*/
PG_FUNCTION_INFO_V1(geography_distance_sphere);
Datum geography_distance_sphere(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GBOX gbox1;
	GBOX gbox2;
	GIDX *gidx1 = gidx_new(3);
	GIDX *gidx2 = gidx_new(3);
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	double tolerance;
	double distance;
	
	/* We need the bounding boxes in case of polygon calculations,
	   which requires them to generate a stab-line to test point-in-polygon. */
	geography_datum_gidx(PG_GETARG_DATUM(0), gidx1);
	geography_datum_gidx(PG_GETARG_DATUM(0), gidx2);
	gbox_from_gidx(gidx1, &gbox1);
	gbox_from_gidx(gidx2, &gbox2);
	pfree(gidx1);
	pfree(gidx2);
	
	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	g2 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);
	
	/* Read our tolerance value. */
	tolerance = PG_GETARG_FLOAT8(2);

	/* Calculate the distance */
	distance = lwgeom_distance_sphere(lwgeom1, lwgeom2, &gbox1, &gbox2, tolerance);

	/* Something went wrong... should already be eloged */
	if( distance < 0.0 )
	{
		elog(ERROR, "Error in geography_distance_sphere calculation.");
		PG_RETURN_NULL();
	}

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);
	
	PG_RETURN_FLOAT8(distance);

}

