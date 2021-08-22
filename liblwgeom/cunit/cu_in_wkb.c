/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2010 Paul Ramsey <pramsey@cleverelephant.ca>
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
** Global variable to hold WKB strings
*/
static char *hex_a;
static char *hex_b;

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkb_in_suite(void)
{
	hex_a = NULL;
	hex_b = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkb_in_suite(void)
{
	if (hex_a) free(hex_a);
	if (hex_b) free(hex_b);
	hex_a = NULL;
	hex_b = NULL;
	return 0;
}

static void cu_wkb_malformed_in(char *hex)
{
	LWGEOM *g = lwgeom_from_hexwkb(hex, LW_PARSER_CHECK_ALL);
	if (g) {
		char *outhex = lwgeom_to_hexwkb_buffer(g, 0);
		printf("cu_wkb_malformed_in input: %s\n", hex);
		printf("cu_wkb_malformed_in output: %s\n", outhex);
		lwfree(outhex);
	}
	CU_ASSERT( g == NULL );
}

static void cu_wkb_in(char *wkt)
{
	LWGEOM_PARSER_RESULT pr;
	LWGEOM *g_a, *g_b;
	lwvarlena_t *wkb_a, *wkb_b;
	/* int i; char *hex; */

	if ( hex_a ) free(hex_a);
	if ( hex_b ) free(hex_b);

	/* Turn WKT into geom */
	lwgeom_parse_wkt(&pr, wkt, LW_PARSER_CHECK_NONE);
	if ( pr.errcode )
	{
		printf("ERROR: %s\n", pr.message);
		printf("POSITION: %d\n", pr.errlocation);
		exit(0);
	}

	/* Get the geom */
	g_a = pr.geom;

	/* Turn geom into WKB */
	wkb_a = lwgeom_to_wkb_varlena(g_a, WKB_NDR | WKB_EXTENDED);

	/* Turn WKB back into geom  */
	g_b = lwgeom_from_wkb((uint8_t *)wkb_a->data, LWSIZE_GET(wkb_a->size) - LWVARHDRSZ, LW_PARSER_CHECK_NONE);

	/* Turn geom to WKB again */
	wkb_b = lwgeom_to_wkb_varlena(g_b, WKB_NDR | WKB_EXTENDED);

	/* Turn geoms into WKB for comparisons */
	hex_a = hexbytes_from_bytes((uint8_t *)wkb_a->data, LWSIZE_GET(wkb_a->size) - LWVARHDRSZ);
	hex_b = hexbytes_from_bytes((uint8_t *)wkb_b->data, LWSIZE_GET(wkb_b->size) - LWVARHDRSZ);

	/* Clean up */
	lwfree(wkb_a);
	lwfree(wkb_b);
	lwgeom_parser_result_free(&pr);
	lwgeom_free(g_b);
}

static void test_wkb_in_point(void)
{
	cu_wkb_in("POINT(0 0 0 0)");
//	printf("old: %s\nnew: %s\n",hex_a, hex_b);
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=4;POINTM(1 1 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POINT EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=4326;POINT EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POINT Z EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POINT M EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POINT ZM EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

}

static void test_wkb_in_linestring(void)
{
	cu_wkb_in("LINESTRING(0 0,1 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("LINESTRING(0 0 1,1 1 2,2 2 3)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_polygon(void)
{
	cu_wkb_in("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=14;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=4;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POLYGON EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multipoint(void)
{
	cu_wkb_in("SRID=4;MULTIPOINT(0 0 0,0 1 0,1 1 0,1 0 0,0 0 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("MULTIPOINT(0 0 0, 0.26794919243112270647255365849413 1 3)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multilinestring(void) {}

static void test_wkb_in_multipolygon(void)
{
	cu_wkb_in("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
	//printf("old: %s\nnew: %s\n",hex_a, hex_b);
}

static void test_wkb_in_collection(void)
{
	cu_wkb_in("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("GEOMETRYCOLLECTION EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=14;GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))),POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1),LINESTRING(0 0 0, 1 1 1))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

}

static void test_wkb_in_circularstring(void)
{
	cu_wkb_in("CIRCULARSTRING(0 -2,-2 0,0 2,2 0,0 -2)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("CIRCULARSTRING(-5 0 0 4, 0 5 1 3, 5 0 2 2, 10 -5 3 1, 15 0 4 0)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=43;CIRCULARSTRING(-5 0 0 4, 0 5 1 3, 5 0 2 2, 10 -5 3 1, 15 0 4 0)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_compoundcurve(void)
{
	cu_wkb_in("COMPOUNDCURVE(CIRCULARSTRING(0 0 0, 0.26794919243112270647255365849413 1 3, 0.5857864376269049511983112757903 1.4142135623730950488016887242097 1),(0.5857864376269049511983112757903 1.4142135623730950488016887242097 1,2 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_curvpolygon(void)
{
	cu_wkb_in("CURVEPOLYGON(CIRCULARSTRING(-2 0 0 0,-1 -1 1 2,0 0 2 4,1 -1 3 6,2 0 4 8,0 2 2 4,-2 0 0 0),(-1 0 1 2,0 0.5 2 4,1 0 3 6,0 1 3 4,-1 0 1 2))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multicurve(void) {}

static void test_wkb_in_multisurface(void) {}

static void test_wkb_in_malformed(void)
{
	/* OSSFUXX */
	cu_wkb_malformed_in("0000000008200000002020202020202020");

	/* See http://trac.osgeo.org/postgis/ticket/1445 */
	cu_wkb_malformed_in("01060000400200000001040000400100000001010000400000000000000000000000000000000000000000000000000101000040000000000000F03F000000000000F03F000000000000F03F");
	cu_wkb_malformed_in("01050000400200000001040000400100000001010000400000000000000000000000000000000000000000000000000101000040000000000000F03F000000000000F03F000000000000F03F");
	cu_wkb_malformed_in("01040000400200000001040000400100000001010000400000000000000000000000000000000000000000000000000101000040000000000000F03F000000000000F03F000000000000F03F");
	cu_wkb_malformed_in("01030000400200000001040000400100000001010000400000000000000000000000000000000000000000000000000101000040000000000000F03F000000000000F03F000000000000F03F");

	/* See http://trac.osgeo.org/postgis/ticket/168 */
	cu_wkb_malformed_in("01060000C00100000001030000C00100000003000000E3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFFE3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFFE3D9107E234F5041A3DB66BC97A30F4122ACEF440DAF9440FFFFFFFFFFFFEFFF");

	/* Oracle "WKB" */
	cu_wkb_malformed_in("00000f424200000007000000020000000240b92ab16861e92940d6c3e0f5be5d9e40ba1a50623d0bfa40d6ef6f2b020c4a000f42410000000340ba1a50623d0bfa40d6ef6f2b020c4a40ba1ebb14cce52440d6f048fed27bec40ba22f374aba38740d6f1323d6c7219000000020000000240ba22f374aba38740d6f1323d6c721940ba5725e34330d740d6fd1dc28f5c29000f42410000002740ba5725e34330d740d6fd1dc28f5c2940ba5b26dbc90e1940d6fe04a63c03cf40ba5f1ced80a17b40d6feee8726d04e40ba64fab16b244a40d700545391e30840ba6aba5e24788540d701c1db1e9f2740ba705be892932540d703384e2d19da40ba75d91676640a40d704b74bc6a7f040ba7b3120b0397b40d7063fe6e24e3d40ba805df3a57eaa40d707d1a9fbe76d40ba80dc43d054f140d707f92b656dd940ba815a1c9b413a40d70820c497742540ba86048c041b5440d7098df370651840ba8a8666559f6f40d70b03332f017540ba929a863f35bf40d70ddd2c513fcd40ba9a0c08205ff240d710d24dd2f1aa40ba9e1c9f51a16040d7129a3226dd1140baa1f53f6c269a40d71469cabc515540baa59752e7123040d7163fbd0bd2f440baa905e34330d740d7181bd70a3d7140baac425b1082b440d719fcc009c75240baaf50623d0bfa40d71be27ef9db2340bab157404ac14d40d71d3785fe998c40bab34a3d5fdcdf40d71e8e666234a840bab66a43c834e340d721170a535ffd40bab94e977c88e840d723a4188f42ff40babb5fc577e64340d7259a5666c23440babd53f7be121f40d727926e978d5040babf2cd708467540d7298a75344b5340bac0ef1a8ef77f40d72b83c6a7ef9e40bac29c6faca73a40d72d7c9a4a17e740bac4399988d2a240d72f76459d990340bac5c8b018b9dc40d7316efed2743140bac74e146a1a5040d733683126e97940bac8cbda53946e40d735601daf2bd440baca4666559f6f40d737583126e97940bacab55b9b068340d737ec8d0113b440bacb249b951c5c40d73880e55c0fcb40bacdc490bfe33240d73ac7bdc376b240bad210623d0bfa40d73ce46a7ac81d000000020000000240bad210623d0bfa40d73ce46a7ac81d40badf86a7ded6bb40d742325e353f7d000f42410000000540badf86a7ded6bb40d742325e353f7d40bae9151fb3633540d74738abbc084240bae8f89363f57340d74cc8f5be5d9e40bae7c450ce91b940d74ec51a03a44340bae7d374aba38740d750c70a3d70a4000000020000000340bae7d374aba38740d750c70a3d70a440bafb1c28e4fb9840d75bedb228dc9840bb24d3b634dad340d76c4126e54717");

}

static void
test_wkb_fuzz(void)
{
	/* OSS-FUZZ https://trac.osgeo.org/postgis/ticket/4534 */
	uint8_t wkb[36] = {000, 000, 000, 000, 015, 000, 000, 000, 003, 000, 200, 000, 000, 010, 000, 000, 000, 000,
			   000, 000, 000, 000, 010, 000, 000, 000, 000, 000, 000, 000, 000, 010, 000, 000, 000, 000};
	LWGEOM *g = lwgeom_from_wkb(wkb, 36, LW_PARSER_CHECK_NONE);
	lwgeom_free(g);

	/* OSS-FUZZ https://trac.osgeo.org/postgis/ticket/4536 */
	uint8_t wkb2[319] = {
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 012, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 051, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000, 115, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 000, 000, 000, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 002,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 207, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000, 000, 000, 000, 000,
	    000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001,
	    001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001};
	g = lwgeom_from_wkb(wkb2, 319, LW_PARSER_CHECK_NONE);
	lwgeom_free(g);

	/* OSS-FUZZ: https://trac.osgeo.org/postgis/ticket/4535 */
	uint8_t wkb3[9] = {0x01, 0x03, 0x00, 0x00, 0x10, 0x8d, 0x55, 0xf3, 0xff};
	g = lwgeom_from_wkb(wkb3, 9, LW_PARSER_CHECK_NONE);
	lwgeom_free(g);

	/* OSS-FUZZ: https://trac.osgeo.org/postgis/ticket/4544 */
	uint8_t wkb4[22] = {0x01, 0x0f, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
			    0x00, 0x00, 0x11, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00};
	g = lwgeom_from_wkb(wkb4, 22, LW_PARSER_CHECK_NONE);
	lwgeom_free(g);

	/* OSS-FUZZ: https://trac.osgeo.org/postgis/ticket/4621 */
	uint32_t big_size = 20000000;
	uint8_t *wkb5 = lwalloc(big_size);
	memset(wkb5, 0x01, big_size);
	g = lwgeom_from_wkb(wkb5, big_size, LW_PARSER_CHECK_NONE);
	lwgeom_free(g);
	lwfree(wkb5);
}

/*
** Used by test harness to register the tests in this file.
*/
void wkb_in_suite_setup(void);
void wkb_in_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("wkb_input", init_wkb_in_suite, clean_wkb_in_suite);
	PG_ADD_TEST(suite, test_wkb_in_point);
	PG_ADD_TEST(suite, test_wkb_in_linestring);
	PG_ADD_TEST(suite, test_wkb_in_polygon);
	PG_ADD_TEST(suite, test_wkb_in_multipoint);
	PG_ADD_TEST(suite, test_wkb_in_multilinestring);
	PG_ADD_TEST(suite, test_wkb_in_multipolygon);
	PG_ADD_TEST(suite, test_wkb_in_collection);
	PG_ADD_TEST(suite, test_wkb_in_circularstring);
	PG_ADD_TEST(suite, test_wkb_in_compoundcurve);
	PG_ADD_TEST(suite, test_wkb_in_curvpolygon);
	PG_ADD_TEST(suite, test_wkb_in_multicurve);
	PG_ADD_TEST(suite, test_wkb_in_multisurface);
	PG_ADD_TEST(suite, test_wkb_in_malformed);
	PG_ADD_TEST(suite, test_wkb_fuzz);
}
