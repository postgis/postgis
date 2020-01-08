/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2008 Paul Ramsey
 * Copyright (C) 2020 Kristian Thy <thy@42.dk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"

static void
test_force_2d(void)
{
	LWGEOM *geom;
	LWGEOM *geom2d;
	char *wkt_out;

	geom = lwgeom_from_wkt("CIRCULARSTRINGM(-5 0 4,0 5 3,5 0 2,10 -5 1,15 0 0)", LW_PARSER_CHECK_NONE);
	geom2d = lwgeom_force_2d(geom);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("CIRCULARSTRING(-5 0,0 5,5 0,10 -5,15 0)", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt(
	    "GEOMETRYCOLLECTION(POINT(0 0 0),LINESTRING(1 1 1,2 2 2),POLYGON((0 0 1,0 1 1,1 1 1,1 0 1,0 0 1)),CURVEPOLYGON(CIRCULARSTRING(0 0 0,1 1 1,2 2 2,1 1 1,0 0 0)))",
	    LW_PARSER_CHECK_NONE);
	geom2d = lwgeom_force_2d(geom);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL(
	    "GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(1 1,2 2),POLYGON((0 0,0 1,1 1,1 0,0 0)),CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 2,1 1,0 0)))",
	    wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);
}

static void
test_force_3dm(void)
{
	LWGEOM *geom;
	LWGEOM *geom3dm;
	char *wkt_out;

	geom = lwgeom_from_wkt("CIRCULARSTRING(-5 0 4,0 5 3,5 0 2,10 -5 1,15 0 0)", LW_PARSER_CHECK_NONE);
	geom3dm = lwgeom_force_3dm(geom, 1);
	wkt_out = lwgeom_to_ewkt(geom3dm);
	CU_ASSERT_STRING_EQUAL("CIRCULARSTRINGM(-5 0 1,0 5 1,5 0 1,10 -5 1,15 0 1)", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom3dm);
	lwfree(wkt_out);
}

static void
test_force_3dz(void)
{
	LWGEOM *geom;
	LWGEOM *geom3dz;
	char *wkt_out;

	geom = lwgeom_from_wkt("CIRCULARSTRING(-5 0,0 5,5 0,10 -5,15 0)", LW_PARSER_CHECK_NONE);
	geom3dz = lwgeom_force_3dz(geom, -99);
	wkt_out = lwgeom_to_ewkt(geom3dz);
	CU_ASSERT_STRING_EQUAL("CIRCULARSTRING(-5 0 -99,0 5 -99,5 0 -99,10 -5 -99,15 0 -99)", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom3dz);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt("CIRCULARSTRING(-5 0,0 5,5 0,10 -5,15 0)", LW_PARSER_CHECK_NONE);
	geom3dz = lwgeom_force_3dz(geom, 0.0);
	wkt_out = lwgeom_to_ewkt(geom3dz);
	CU_ASSERT_STRING_EQUAL("CIRCULARSTRING(-5 0 0,0 5 0,5 0 0,10 -5 0,15 0 0)", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom3dz);
	lwfree(wkt_out);
}

static void
test_force_4d(void)
{
	LWGEOM *geom;
	LWGEOM *geom4d;
	char *wkt_out;

	geom = lwgeom_from_wkt("POINT(1 2)", LW_PARSER_CHECK_NONE);
	geom4d = lwgeom_force_4d(geom, 3, 4);
	wkt_out = lwgeom_to_ewkt(geom4d);
	CU_ASSERT_STRING_EQUAL("POINT(1 2 3 4)", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom4d);
	lwfree(wkt_out);
}

/*
** Used by the test harness to register the tests in this file.
*/
void force_dims_suite_setup(void);
void
force_dims_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("force_dims", NULL, NULL);
	PG_ADD_TEST(suite, test_force_2d);
	PG_ADD_TEST(suite, test_force_3dm);
	PG_ADD_TEST(suite, test_force_3dz);
	PG_ADD_TEST(suite, test_force_4d);
}
