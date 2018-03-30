/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2018 Nicklas Avén <nicklas.aven@jordogskog.no>
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

static void do_test_filterm(char *geom_txt,char *expected, double min, double max)
{
	LWGEOM *geom_in, *geom_out;
	char *out_txt;
	geom_in = lwgeom_from_wkt(geom_txt, LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_filter_m(geom_in,min, max, 1);
	out_txt = lwgeom_to_wkt(geom_out, WKT_EXTENDED, 3, NULL);
	if(strcmp(expected, out_txt))
		printf("%s is not equal to %s\n", expected, out_txt);
	CU_ASSERT_STRING_EQUAL(expected, out_txt)
	lwfree(out_txt);
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
	return;
}

static void do_test_filterm_single_geometries(void)
{
	/*Point*/
	do_test_filterm("POINT(0 0 0 0)","POINT(0 0 0 0)", 0, 11);
	do_test_filterm("POINT(0 0 0 0)","POINT EMPTY", 1, 11);
	/*Line*/
	do_test_filterm("LINESTRING(0 0 0 0,1 1 0 2,2 2 0 5,3 1 0 3)","LINESTRING(1 1 0 2,3 1 0 3)",2,3);
	do_test_filterm("LINESTRING(0 0 0 0,1 1 0 2,2 2 0 5,3 1 0 3)","LINESTRING EMPTY",10, 11);
	/*Polygon*/
	do_test_filterm("POLYGON((0 0 0 0,1 1 0 2,5 5 0 5,3 1 0 3,0 0 0 0))","POLYGON((0 0 0 0,1 1 0 2,3 1 0 3,0 0 0 0))",0,4);
	do_test_filterm("POLYGON((0 0 0 0,1 1 0 2,5 5 0 5,3 1 0 3,0 0 0 0))","POLYGON EMPTY",10, 11);
	return;
}

static void do_test_filterm_collections(void)
{
	do_test_filterm("GEOMETRYCOLLECTION(POINT(1 1 1 1), LINESTRING(1 1 1 1, 1 2 1 4, 2 2 1 3), POLYGON((0 0 0 4,1 1 0 2,5 5 0 5,3 1 0 3,0 0 0 4)))","GEOMETRYCOLLECTION(POINT(1 1 1 1),LINESTRING(1 1 1 1,1 2 1 4,2 2 1 3),POLYGON((0 0 0 4,1 1 0 2,3 1 0 3,0 0 0 4)))",0,4);
	do_test_filterm("GEOMETRYCOLLECTION(POINT(1 1 1 1), LINESTRING(1 1 1 1, 1 2 1 4, 2 2 1 3), POLYGON((0 0 0 4,1 1 0 2,5 5 0 5,3 1 0 3,0 0 0 4)))","GEOMETRYCOLLECTION(LINESTRING(1 2 1 4,2 2 1 3),POLYGON((0 0 0 4,1 1 0 2,3 1 0 3,0 0 0 4)))",2,4);
	return;
}

void filterm_suite_setup(void);
void filterm_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("filterm",NULL,NULL);
	PG_ADD_TEST(suite, do_test_filterm_single_geometries);
	PG_ADD_TEST(suite, do_test_filterm_collections);
}
