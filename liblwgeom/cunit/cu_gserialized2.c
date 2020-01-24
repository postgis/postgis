/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2019 Paul Ramsey <pramsey@cleverelephant.ca>
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
#include "gserialized2.c" /* for gserialized_peek_gbox_p */
#include "cu_tester.h"


static void test_g2flags_macros(void)
{
	uint8_t flags = 0;

	CU_ASSERT_EQUAL(0, G2FLAGS_GET_Z(flags));
	G2FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_Z(flags));
	G2FLAGS_SET_Z(flags, 0);
	CU_ASSERT_EQUAL(0, G2FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, G2FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, G2FLAGS_GET_M(flags));
	G2FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_M(flags));

	CU_ASSERT_EQUAL(0, G2FLAGS_GET_BBOX(flags));
	G2FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, G2FLAGS_GET_GEODETIC(flags));
	G2FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_GEODETIC(flags));

	flags = g2flags(1, 0, 1); /* z=1, m=0, geodetic=1 */

	CU_ASSERT_EQUAL(1, G2FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, G2FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(2, G2FLAGS_GET_ZM(flags));

	flags = g2flags(1, 1, 1); /* z=1, m=1, geodetic=1 */

	CU_ASSERT_EQUAL(1, G2FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(3, G2FLAGS_GET_ZM(flags));

	flags = g2flags(0, 1, 0); /* z=0, m=1, geodetic=0 */

	CU_ASSERT_EQUAL(0, G2FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(0, G2FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_ZM(flags));

	flags = 0;
	G2FLAGS_SET_VERSION(flags, 0);
	CU_ASSERT_EQUAL(0, G2FLAGS_GET_VERSION(flags));
	G2FLAGS_SET_VERSION(flags, 1);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_VERSION(flags));
}

static void test_gserialized2_extended_flags(void)
{
	LWGEOM *lwg1, *lwg2;
	GSERIALIZED *g;
	size_t size = 0;

	/* No extended flags, so 32 bytes for a point */
	lwg1 = lwgeom_from_wkt("POINT(100 100)", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(lwg1);
	CU_ASSERT_EQUAL(size, 32);

	/* Add extended flag, we now expect 8 extra bytes */
	FLAGS_SET_SOLID(lwg1->flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_SOLID(lwg1->flags));
	size = gserialized2_from_lwgeom_size(lwg1);
	CU_ASSERT_EQUAL(size, 40);

	/* We expect that the extended bit will be set on the serialization */
	g = gserialized2_from_lwgeom(lwg1, &size);
	CU_ASSERT_EQUAL(1, G2FLAGS_GET_EXTENDED(g->gflags));
	/* And that the serialization in fact hits 40 bytes */
	CU_ASSERT_EQUAL(size, 40);
	lwgeom_free(lwg1);
	lwg2 = lwgeom_from_gserialized2(g);
	lwfree(g);
	/* And that when deserialized again, the solid flag is still there */
	CU_ASSERT_EQUAL(1, FLAGS_GET_SOLID(lwg2->flags));
	lwgeom_free(lwg2);

}

static void test_gserialized2_srid(void)
{
	GSERIALIZED s;
	int32_t srid, rv;

	srid = 4326;
	gserialized2_set_srid(&s, srid);
	rv = gserialized2_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = -3005;
	gserialized2_set_srid(&s, srid);
	rv = gserialized2_get_srid(&s);
	//printf("srid=%d rv=%d\n",srid,rv);
	CU_ASSERT_EQUAL(rv, SRID_UNKNOWN);

	srid = SRID_UNKNOWN;
	gserialized2_set_srid(&s, srid);
	rv = gserialized2_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = SRID_UNKNOWN;
	gserialized2_set_srid(&s, srid);
	rv = gserialized2_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 100000;
	gserialized2_set_srid(&s, srid);
	rv = gserialized2_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);
}

static void test_gserialized2_from_lwgeom_size(void)
{
	LWGEOM *g;
	size_t size = 0;

	g = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 32 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POINT(0 0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 40 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTIPOINT(0 0 0, 1 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 80 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("LINESTRING(0 0, 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 48 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTILINESTRING((0 0, 1 1),(0 0, 1 1))", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 96 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 104 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((-1 -1, -1 2, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized2_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 184 );
	lwgeom_free(g);

}


static void test_lwgeom_from_gserialized2(void)
{
	LWGEOM *geom1, *geom2;
	GSERIALIZED *g1, *g2;
	char *in_ewkt, *out_ewkt;
	size_t i = 0;

	char *ewkt[] =
	{
		"POINT EMPTY",
		"POINT(0 0.2)",
		"LINESTRING EMPTY",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"MULTIPOINT EMPTY",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"MULTIPOINT(0.9 0.9,0.9 0.9,EMPTY,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING EMPTY",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"POLYGON EMPTY",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON EMPTY",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
		"SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;MULTIPOLYGON EMPTY",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"SRID=4326;GEOMETRYCOLLECTION EMPTY",
		"SRID=4326;GEOMETRYCOLLECTION(POINT EMPTY,MULTIPOLYGON EMPTY)",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 0.2),POINT EMPTY,POINT(0 0.2))",
		"MULTICURVE((5 5 1 3,3 5 2 2,3 3 3 1,0 3 1 1),CIRCULARSTRING(0 0 0 0,0.26794 1 3 -2,0.5857864 1.414213 1 2))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING EMPTY))",
		"POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 1,1 0 1,1 1 1,0 1 1,0 0 1)),((0 0 0,0 0 1,0 1 1,0 1 0,0 0 0)),((1 0 0,1 1 0,1 1 1,1 0 1,1 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((0 1 0,0 1 1,1 1 1,1 1 0,0 1 0)))",
	};

	for ( i = 0; i < (sizeof ewkt/sizeof(char*)); i++ )
	{
		size_t size1, size2;

		in_ewkt = ewkt[i];
		geom1 = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		lwgeom_add_bbox(geom1);
		if (geom1->bbox) gbox_float_round(geom1->bbox);
		size1 = gserialized2_from_lwgeom_size(geom1);
		g1 = gserialized2_from_lwgeom(geom1, &size2);
		CU_ASSERT_EQUAL(size1, size2);

		geom2 = lwgeom_from_gserialized2(g1);
		out_ewkt = lwgeom_to_ewkt(geom2);

		/* printf("\n in = %s\nout = %s\n", in_ewkt, out_ewkt); */
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		g2 = gserialized2_from_lwgeom(geom2, &size2);
		CU_ASSERT_EQUAL(g1->gflags, g2->gflags);

		/* either both or none of the bboxes are null */
		CU_ASSERT(geom1->bbox != NULL || geom2->bbox == NULL);

		/* either both are null or they are the same */
		CU_ASSERT(geom1->bbox == NULL || gbox_same(geom1->bbox, geom2->bbox));

		lwfree(out_ewkt);
		lwfree(g1);
		lwfree(g2);
		lwgeom_free(geom1);
		lwgeom_free(geom2);

		/* SOLID / BOX INTERACTION */
		geom1 = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		lwgeom_add_bbox(geom1);
		FLAGS_SET_SOLID(geom1->flags, 1);
		size1 = gserialized2_from_lwgeom_size(geom1);
		g1 = gserialized2_from_lwgeom(geom1, &size2);
		CU_ASSERT_EQUAL(size1, size2);
		CU_ASSERT_EQUAL(LWSIZE_GET(g1->size), size2);
		geom2 = lwgeom_from_gserialized2(g1);
		CU_ASSERT_EQUAL(geom1->flags, geom2->flags);
		g2 = gserialized2_from_lwgeom(geom2, &size2);
		CU_ASSERT_EQUAL(LWSIZE_GET(g2->size), size2);
		CU_ASSERT_EQUAL(g1->gflags, g2->gflags);
		CU_ASSERT_EQUAL(FLAGS_GET_SOLID(geom2->flags), 1);
		lwfree(g1);
		lwfree(g2);
		lwgeom_free(geom1);
		lwgeom_free(geom2);
	}

}


static void test_gserialized2_is_empty(void)
{
	int i = 0;
	struct gserialized_empty_cases {
		const char* wkt;
		int isempty;
	};

	struct gserialized_empty_cases cases[] = {
		{ "POINT EMPTY", 1 },
		{ "POINT(1 1)", 0 },
		{ "LINESTRING EMPTY", 1 },
		{ "MULTILINESTRING EMPTY", 1 },
		{ "MULTILINESTRING(EMPTY)", 1 },
		{ "MULTILINESTRING(EMPTY,EMPTY)", 1 },
		{ "MULTILINESTRING(EMPTY,(0 0,1 1))", 0 },
		{ "MULTILINESTRING((0 0,1 1),EMPTY)", 0 },
		{ "MULTILINESTRING(EMPTY,(0 0,1 1),EMPTY)", 0 },
		{ "MULTILINESTRING(EMPTY,EMPTY,EMPTY)", 1 },
		{ "GEOMETRYCOLLECTION(POINT EMPTY,MULTILINESTRING(EMPTY,EMPTY,EMPTY))", 1 },
		{ "GEOMETRYCOLLECTION(POINT EMPTY,MULTILINESTRING(EMPTY),POINT(1 1))", 0 },
		{ "GEOMETRYCOLLECTION(POINT EMPTY,MULTILINESTRING(EMPTY, (0 0)),POINT EMPTY)", 0 },
		{ "GEOMETRYCOLLECTION(POLYGON EMPTY,POINT EMPTY,MULTILINESTRING(EMPTY,EMPTY),POINT EMPTY)", 1 },
		{ "GEOMETRYCOLLECTION(POLYGON EMPTY,GEOMETRYCOLLECTION(POINT EMPTY),MULTILINESTRING(EMPTY,EMPTY),POINT EMPTY)", 1 },
		{ NULL, 0 }
	};

	while( cases[i].wkt )
	{
		// i = 11;
		LWGEOM *lw = lwgeom_from_wkt(cases[i].wkt, LW_PARSER_CHECK_NONE);
		GSERIALIZED *g = gserialized2_from_lwgeom(lw, 0);
		int ie = gserialized2_is_empty(g);
		// printf("%s: we say %d, they say %d\n", cases[i].wkt, cases[i].isempty, ie);
		CU_ASSERT_EQUAL(ie, cases[i].isempty);
		lwgeom_free(lw);
		lwfree(g);
		i++;
	}
}



static void test_on_gser2_lwgeom_count_vertices(void)
{
	LWGEOM *lwgeom;
	GSERIALIZED *g_ser1;
	size_t ret_size;

	lwgeom = lwgeom_from_wkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1,1 1,2 2,4 5)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	g_ser1 = gserialized2_from_lwgeom(lwgeom, &ret_size);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized2(g_ser1);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized2(g_ser1);

	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_free(lwgeom);

	lwfree(g_ser1);

}


static void test_gserialized2_peek_gbox_p_no_box_when_empty(void)
{
	uint32_t i;

	char *ewkt[] =
	{
		"POINT EMPTY",
		"LINESTRING EMPTY",
		"MULTIPOINT EMPTY",
		"MULTIPOINT (EMPTY)",
		"MULTILINESTRING EMPTY",
		"MULTILINESTRING (EMPTY)"
	};

	for ( i = 0; i < (sizeof ewkt/sizeof(char*)); i++ )
	{
		LWGEOM* geom = lwgeom_from_wkt(ewkt[i], LW_PARSER_CHECK_NONE);
		GBOX box;
		gbox_init(&box);

		GSERIALIZED* gser = gserialized2_from_lwgeom(geom, NULL);

		CU_ASSERT_FALSE(gserialized2_has_bbox(gser));

		CU_ASSERT_EQUAL(LW_FAILURE, gserialized2_peek_gbox_p(gser, &box));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static void test_gserialized2_peek_gbox_p_gets_correct_box(void)
{
	uint32_t i;

	char *ewkt[] =
	{
		"POINT (2.2945672355 48.85822923236)",
		"POINTZ (2.2945672355 48.85822923236 15)",
		"POINTM (2.2945672355 48.85822923236 12)",
		"POINT ZM (2.2945672355 48.85822923236 12 2)",
		"MULTIPOINT ((-76.45402132523 44.225406213532))",
		"MULTIPOINT Z ((-76.45402132523 44.225406213532 112))",
		"MULTIPOINT ZM ((-76.45402132523 44.225406213532 112 44))",
		"LINESTRING (2.2945672355 48.85822923236, -76.45402132523 44.225406213532)",
		"LINESTRING Z (2.2945672355 48.85822923236 6, -76.45402132523 44.225406213532 8)",
		"LINESTRING ZM (2.2945672355 48.85822923236 3 2, -76.45402132523 44.225406213532 9 4)",
		"MULTILINESTRING ((2.2945672355 48.85822923236, -76.45402132523 44.225406213532))",
		"MULTILINESTRING Z ((2.2945672355 48.85822923236 4, -76.45402132523 44.225406213532 3))"
	};

	for ( i = 0; i < (sizeof ewkt/sizeof(char*)); i++ )
	{
		LWGEOM* geom = lwgeom_from_wkt(ewkt[i], LW_PARSER_CHECK_NONE);
		GBOX box_from_peek;
		GBOX box_from_lwgeom;
		gbox_init(&box_from_peek);
		gbox_init(&box_from_lwgeom);

		GSERIALIZED* gser = gserialized2_from_lwgeom(geom, NULL);

		CU_ASSERT_FALSE(gserialized2_has_bbox(gser));

		lwgeom_calculate_gbox(geom, &box_from_lwgeom);
		gserialized2_peek_gbox_p(gser, &box_from_peek);

		gbox_float_round(&box_from_lwgeom);

		CU_ASSERT_TRUE(gbox_same(&box_from_peek, &box_from_lwgeom));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static void test_gserialized2_peek_gbox_p_fails_for_unsupported_cases(void)
{
	uint32_t i;

	char *ewkt[] =
	{
		"MULTIPOINT ((-76.45402132523 44.225406213532), (-72 33))",
		"LINESTRING (2.2945672355 48.85822923236, -76.45402132523 44.225406213532, -72 33)",
		"MULTILINESTRING ((2.2945672355 48.85822923236, -76.45402132523 44.225406213532, -72 33))",
		"MULTILINESTRING ((2.2945672355 48.85822923236, -76.45402132523 44.225406213532), (-72 33, -71 32))"
	};

	for ( i = 0; i < (sizeof ewkt/sizeof(char*)); i++ )
	{
		LWGEOM* geom = lwgeom_from_wkt(ewkt[i], LW_PARSER_CHECK_NONE);
		GBOX box;
		gbox_init(&box);
		lwgeom_drop_bbox(geom);

		/* Construct a GSERIALIZED* that doesn't have a box, so that we can test the
		 * actual logic of the peek function */
		size_t expected_size = gserialized2_from_lwgeom_size(geom);
		GSERIALIZED* gser = lwalloc(expected_size);
		uint8_t* ptr = (uint8_t*) gser;

		ptr += 8; // Skip header
		gserialized2_from_lwgeom_any(geom, ptr);
		gser->gflags = lwflags_get_g2flags(geom->flags);

		CU_ASSERT_FALSE(gserialized2_has_bbox(gser));
		CU_ASSERT_EQUAL(LW_FAILURE, gserialized2_peek_gbox_p(gser, &box));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static int
peek2_point_helper(char *geometry, POINT4D *p)
{
	cu_error_msg_reset();
	p->x = p->y = p->z = p->m = 0;
	LWGEOM *geom = lwgeom_from_wkt(geometry, LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	GSERIALIZED *g = gserialized2_from_lwgeom(geom, NULL);
	CU_ASSERT(g != NULL);

	int ret = gserialized2_peek_first_point(g, p);
	lwfree(g);
	lwgeom_free(geom);

	return ret;
}

static void
test_gserialized2_peek_first_point(void)
{
	POINT4D p = {0};

	CU_ASSERT(peek2_point_helper("POINT(1 2)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 1);
	CU_ASSERT_EQUAL(p.y, 2);

	CU_ASSERT(peek2_point_helper("POINTZ(10 20 30)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 10);
	CU_ASSERT_EQUAL(p.y, 20);
	CU_ASSERT_EQUAL(p.z, 30);

	CU_ASSERT(peek2_point_helper("POINTM(100 200 300)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 100);
	CU_ASSERT_EQUAL(p.y, 200);
	CU_ASSERT_EQUAL(p.m, 300);

	CU_ASSERT(peek2_point_helper("POINTZM(1000 2000 3000 4000)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 1000);
	CU_ASSERT_EQUAL(p.y, 2000);
	CU_ASSERT_EQUAL(p.z, 3000);
	CU_ASSERT_EQUAL(p.m, 4000);

	CU_ASSERT(peek2_point_helper("MULTIPOINT((0 0), (1 1))", &p) == LW_FAILURE);
	CU_ASSERT(peek2_point_helper("LINESTRING(0 0, 1 1)", &p) == LW_FAILURE);
	CU_ASSERT(peek2_point_helper("MULTILINESTRING((0 0, 1 1), (0 0, 1 1))", &p) == LW_FAILURE);
	CU_ASSERT(peek2_point_helper("POLYGON((0 0, 1 1, 1 0, 0 0))", &p) == LW_FAILURE);
}

/*
** Used by test harness to register the tests in this file.
*/
void gserialized2_suite_setup(void);
void gserialized2_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("serialization/deserialization v2", NULL, NULL);
	PG_ADD_TEST(suite, test_g2flags_macros);
	PG_ADD_TEST(suite, test_gserialized2_srid);
	PG_ADD_TEST(suite, test_gserialized2_from_lwgeom_size);
	PG_ADD_TEST(suite, test_lwgeom_from_gserialized2);
	PG_ADD_TEST(suite, test_on_gser2_lwgeom_count_vertices);
	PG_ADD_TEST(suite, test_gserialized2_is_empty);
	PG_ADD_TEST(suite, test_gserialized2_peek_gbox_p_no_box_when_empty);
	PG_ADD_TEST(suite, test_gserialized2_peek_gbox_p_gets_correct_box);
	PG_ADD_TEST(suite, test_gserialized2_peek_gbox_p_fails_for_unsupported_cases);
	PG_ADD_TEST(suite, test_gserialized2_extended_flags);
	PG_ADD_TEST(suite, test_gserialized2_peek_first_point);
}
