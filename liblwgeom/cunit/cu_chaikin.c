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

static void do_test_chaikin(char *geom_txt,char *expected, int n_iterations, int preserve_end_points)
{
	LWGEOM *geom_in, *geom_out;
	char *out_txt;
	geom_in = lwgeom_from_wkt(geom_txt, LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_chaikin(geom_in, n_iterations, preserve_end_points);
	out_txt = lwgeom_to_wkt(geom_out, WKT_EXTENDED, 3, NULL);
	if(strcmp(expected, out_txt))
		printf("%s is not equal to %s\n", expected, out_txt);
	CU_ASSERT_STRING_EQUAL(expected, out_txt)
	lwfree(out_txt);
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
	return;
}

static void do_test_chaikin_lines(void)
{
	/*Simpliest test*/
	do_test_chaikin("LINESTRING(0 0,8 8,16 0)","LINESTRING(0 0,6 6,10 6,16 0)", 1, 1);
	/*2 iterations*/
	do_test_chaikin("LINESTRING(0 0,8 8,16 0)","LINESTRING(0 0,4.5 4.5,7 6,9 6,11.5 4.5,16 0)", 2, 1);
	/*check so it really ignores preserve_end_points set to off for linestrings*/
	do_test_chaikin("LINESTRING(0 0,8 8,16 0)","LINESTRING(0 0,4.5 4.5,7 6,9 6,11.5 4.5,16 0)", 2, 0);
	return;
}

static void do_test_chaikin_polygons(void)
{
	/*Simpliest test*/
	do_test_chaikin("POLYGON((0 0,8 8,16 0,0 0))","POLYGON((0 0,6 6,10 6,14 2,12 0,0 0))", 1, 1);
	/*2 iterations*/
	do_test_chaikin("POLYGON((0 0,8 8,16 0,0 0))","POLYGON((0 0,4.5 4.5,7 6,9 6,11 5,13 3,13.5 1.5,12.5 0.5,9 0,0 0))", 2, 1);
	/*2 iterations without preserving end points*/
	do_test_chaikin("POLYGON((0 0,8 8,16 0,0 0))","POLYGON((3 3,5 5,7 6,9 6,11 5,13 3,13.5 1.5,12.5 0.5,10 0,6 0,3.5 0.5,2.5 1.5,3 3))", 2, 0);
	return;
}


void chaikin_suite_setup(void);
void chaikin_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("chaikin",NULL,NULL);
	PG_ADD_TEST(suite, do_test_chaikin_lines);
	PG_ADD_TEST(suite, do_test_chaikin_polygons);
}
