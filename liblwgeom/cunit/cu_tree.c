/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
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
#include "lwgeodetic.h";
#include "lwgeodetic_tree.h";
#include "cu_tester.h"


static void test_tree_circ_create(void)
{
	LWLINE *g;
	CIRC_NODE *c;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(0 88,0 89,0 90,180 89,180 88)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
//	circ_tree_print(c, 0);
	CU_ASSERT(c->num_nodes == 2);
	circ_tree_free(c);
	lwline_free(g);
}


static void test_tree_circ_pip(void)
{
	LWLINE *g;
	CIRC_NODE *c;
	POINT2D pt, pt_outside;
	int rv, on_boundary;
	
	pt.x = 0.0;
	pt.y = 0.0;
	pt_outside.x = -2.0;
	pt_outside.y = 0.0;
	
	/* Point in square */
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,1 -1,1 1,-1 1,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 1);

	/* Point on other side of square */
	pt.x = 2.0;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 0);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);

	/* Point in square, stab passing through vertex */
	pt.x = 0.0;
	pt.y = 0.0;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 1,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	//circ_tree_print(c, 0);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 1);

	/* Point on other side of square, stab passing through vertex */
	pt.x = 2.0;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 0);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);

	/* Point outside "w" thing, stab passing through vertexes and grazing pointy thing */
	pt.x = 2.0;
	pt.y = 0.0;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	//circ_tree_print(c, 0);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	//printf("rv %d\n", rv);
	CU_ASSERT_EQUAL(rv, 0);

	/* Point inside "w" thing, stab passing through vertexes and grazing pointy thing */
	pt.x = 0.8;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	//printf("rv %d\n", rv);
	CU_ASSERT_EQUAL(rv, 1);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);

}

static void test_tree_circ_pip2(void)
{
	LWGEOM* g;
	LWPOLY* p;
	LWPOINT* lwpt;
	int rv_classic, rv_tree, on_boundary;
	POINT2D pt, pt_outside;
	GBOX gbox;
	CIRC_NODE *c;

	g = lwgeom_from_wkt("POLYGON((0 0,0 1,1 1,1 0,0 0))", LW_PARSER_CHECK_NONE);
	p = lwgeom_as_lwpoly(g);

	pt.x = 0.2;
	pt.y = 0.1;
	lwgeom_calculate_gbox_geodetic(g, &gbox);
	gbox_pt_outside(&gbox, &pt_outside);
	c = circ_tree_new(p->rings[0]);
	//circ_tree_print(c, 0);
	rv_classic = lwpoly_covers_point2d(p, &pt);
	rv_tree = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv_tree, rv_classic);
	circ_tree_free(c);
	lwgeom_free(g);
	
	g = lwgeom_from_hexwkb("0103000020E6100000010000004700000000000000621119C000000040C70C4B40000000E0CC6C18C0000000A026FF4A4000000040438519C000000000E8F44A40000000000F5318C00000004024C84A4000000060F9E518C0000000A027AD4A40000000805E0D18C0000000C0F5784A4000000040539718C000000080815E4A40000000C026FE19C0000000C0502D4A4000000060127019C000000040EA164A40000000003BFD1BC0000000609E234A4000000080D9011CC000000060B9114A4000000040C8501EC0000000C0D50C4A40000000C05F2C20C000000040C9E749400000006008D820C000000080D6F0494000000080139F20C000000060F3DE4940000000A0B16421C0000000C059C94940000000808FA223C0000000C007B949400000000041E722C000000080C3DC4940000000808F4224C0000000405DCE494000000060752923C000000040A9EF4940000000005CAD24C0000000C036E4494000000040F88624C00000008078FE494000000060558523C00000006025134A40000000403AED24C00000000011174A40000000A05E7D23C0000000E0A41F4A4000000040F0AD23C0000000809A304A4000000040A64E23C000000040C9474A40000000C0FCA221C0000000C030554A40000000805EDD23C0000000E010474A4000000040BFF822C00000008078664A4000000080C98F22C000000040E2914A40000000C036E021C00000002024924A4000000080D9E121C000000000D0A14A4000000040533723C000000040B99D4A40000000204B1E23C0000000C0CCB04A4000000000625A24C0000000A071B44A40000000004A5F24C0000000806FC64A40000000E0DD6523C00000006088CC4A400000004012D023C0000000001AE14A40000000806E1F23C0000000400BEE4A40000000E0A2E123C0000000C017EF4A4000000060449423C00000004003F94A40000000C0DC0624C0000000A0ED1B4B40000000A0803F24C0000000005D0C4B4000000040753924C000000080701D4B400000002021F320C00000000001234B4000000000C65221C000000080792D4B40000000406D6020C0000000001A514B4000000040BF9821C0000000A00E594B400000000031B520C0000000C0726B4B400000002019EA20C000000020977F4B400000000002ED1FC0000000E0B49B4B400000000084CC1EC0000000602D8C4B4000000020BB2A1FC000000060239B4B4000000040AE871EC0000000A0FDA14B400000008077771EC0000000C0E5864B40000000C0AABA1EC000000000B7794B40000000C03EC91DC0000000E020874B4000000000A4301EC0000000C0C49B4B4000000000B5811DC0000000A0A3B04B400000004095BD1BC000000020869E4B400000004091021DC00000004009894B40000000409D361EC000000080A2614B40000000809FB41FC0000000A0AB594B40000000C046021FC0000000C0164C4B40000000C0EC5020C0000000E05A384B4000000040DF3C1EC0000000803F104B4000000000B4221DC0000000C0CD0F4B40000000C0261E1CC00000006067354B4000000080E17A1AC000000080C3044B4000000000621119C000000040C70C4B40", LW_PARSER_CHECK_NONE);
	p = lwgeom_as_lwpoly(g);
	lwpt = (LWPOINT*)lwgeom_from_hexwkb("0101000020E610000057B89C28FEB320C09C8102CB3B2B4A40", LW_PARSER_CHECK_NONE);
	lwpoint_getPoint2d_p(lwpt, &pt);
	lwgeom_calculate_gbox_geodetic(g, &gbox);
	gbox_pt_outside(&gbox, &pt_outside);
	printf("OUTSIDE POINT(%g %g)\n", pt_outside.x, pt_outside.y);
	c = circ_tree_new(p->rings[0]);
	rv_classic = lwpoly_covers_point2d(p, &pt);
	rv_tree = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv_tree, rv_classic);
	circ_tree_free(c);
	lwpoint_free(lwpt);
	lwgeom_free(g);
}


static void test_tree_circ_distance(void)
{
	LWLINE *line;
	LWPOINT *point;
	CIRC_NODE *cline, *cpoint;
	SPHEROID s;
	double distance_tree, distance_geom;
	double threshold = 0.0;
	
	spheroid_init(&s, 1.0, 1.0);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(-2 0)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, &s, threshold);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_geom, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(2 2)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, &s, threshold);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_geom, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(1 1)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, &s, threshold);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_tree, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(1 0.5)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, &s, threshold);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
//	printf("distance_tree %g\n", distance_tree);
//	printf("distance_geom %g\n", distance_geom);
//	circ_tree_print(cline, 0);
//	circ_tree_print(cpoint, 0);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_tree, distance_geom, 0.0001);
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo tree_tests[] =
{
	PG_TEST(test_tree_circ_create),
	PG_TEST(test_tree_circ_pip),
	PG_TEST(test_tree_circ_pip2),
	PG_TEST(test_tree_circ_distance),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo tree_suite = {"Internal Spatial Trees",  NULL,  NULL, tree_tests};

