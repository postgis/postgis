/**********************************************************************
 * $Id: cu_out_twkb.c 12198 2014-01-29 17:49:35Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2014 Nicklas Avén
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

/*
** Global variable to hold TWKB strings
*/
uint8_t *s;
/*
** Global variable to hold length of TWKB-string
*/
size_t size;

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_twkb_out_suite(void)
{
	s = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_twkb_out_suite(void)
{
	if (s) free(s);
	s = NULL;
	return 0;
}


/*
** Creating an input TWKB from a wkt string 
*/
static void cu_twkb(char *wkt,int8_t  prec,  int64_t id)
{
	LWGEOM *g = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	if ( s ) free(s);
	s = (uint8_t*)lwgeom_to_twkb(g, TWKB_ID, &size,prec, id);
	lwgeom_free(g);
}


static void test_twkb_out_point(void)
{
	cu_twkb("POINT(0 0 0 0)",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"01810200000000");
//	printf("TWKB: %s\n",hexbytes_from_bytes(s,size));
	cu_twkb("SRID=4;POINTM(1 1 1)",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"016102020202");
//	printf("TWKB: %s\n",hexbytes_from_bytes(s,size));
}

static void test_twkb_out_linestring(void)
{
	cu_twkb("LINESTRING(0 0,1 1)",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0142020200000202");
//	printf("TWKB: %s\n",hexbytes_from_bytes(s,size));
	cu_twkb("LINESTRING(0 0 1,1 1 2,2 2 3)",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"01620203000002020202020202");

	cu_twkb("LINESTRING EMPTY",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0101024200");
}

static void test_twkb_out_polygon(void)
{
	cu_twkb("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0163020105000000000200020000000100010000");

	cu_twkb("SRID=14;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"01830201050000000200020002020000020001000201000002");

	cu_twkb("POLYGON EMPTY",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0101024300");

}

static void test_twkb_out_multipoint(void) 
{
	cu_twkb("SRID=4;MULTIPOINT(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"01640205000000000200020000000100010000");

	cu_twkb("MULTIPOINT(0 0 0, 0.26794919243112270647255365849413 1 3)",7,1);
	//printf("WKB: %s",s);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"71640202000000888BC70280DAC409808ECE1C");
//	printf("TWKB: %s\n",hexbytes_from_bytes(s,size));
}

static void test_twkb_out_multilinestring(void) {}

static void test_twkb_out_multipolygon(void)
{
	cu_twkb("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"016602020105000000000200020000000100010000020501010000060006000000050005000005020200000200020000000100010000");
}

static void test_twkb_out_collection(void)
{
	cu_twkb("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0167020263010500000000020002000000010001000061020202");

	cu_twkb("GEOMETRYCOLLECTION EMPTY",0,1);
	CU_ASSERT_STRING_EQUAL(hexbytes_from_bytes(s,size),"0101024700");
}


/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo twkb_out_tests[] =
{
	PG_TEST(test_twkb_out_point),
	PG_TEST(test_twkb_out_linestring),
	PG_TEST(test_twkb_out_polygon),
	PG_TEST(test_twkb_out_multipoint),
	PG_TEST(test_twkb_out_multilinestring),
	PG_TEST(test_twkb_out_multipolygon),
	PG_TEST(test_twkb_out_collection),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo twkb_out_suite = {"TWKB Out Suite",  init_twkb_out_suite,  clean_twkb_out_suite, twkb_out_tests};
