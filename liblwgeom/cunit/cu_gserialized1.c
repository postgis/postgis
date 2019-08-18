/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
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
#include "gserialized1.c" /* for gserialized_peek_gbox_p */
#include "cu_tester.h"

static void test_typmod_macros(void)
{
	int32_t typmod = 0;
	int32_t srid = 4326;
	int type = 6;
	int z = 1;
	int rv;

	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

	srid = -5005;
	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

        srid = 999999;
        TYPMOD_SET_SRID(typmod,srid);
        rv = TYPMOD_GET_SRID(typmod);
        CU_ASSERT_EQUAL(rv, srid);

        srid = -999999;
        TYPMOD_SET_SRID(typmod,srid);
        rv = TYPMOD_GET_SRID(typmod);
        CU_ASSERT_EQUAL(rv, srid);

	srid = SRID_UNKNOWN;
	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 0;
	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 1;
	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

	TYPMOD_SET_TYPE(typmod,type);
	rv = TYPMOD_GET_TYPE(typmod);
	CU_ASSERT_EQUAL(rv,type);

	TYPMOD_SET_Z(typmod);
	rv = TYPMOD_GET_Z(typmod);
	CU_ASSERT_EQUAL(rv,z);

	rv = TYPMOD_GET_M(typmod);
	CU_ASSERT_EQUAL(rv,0);

}

static void test_flags_macros(void)
{
	uint8_t flags = 0;

	CU_ASSERT_EQUAL(0, G1FLAGS_GET_Z(flags));
	G1FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_Z(flags));
	G1FLAGS_SET_Z(flags, 0);
	CU_ASSERT_EQUAL(0, G1FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, G1FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, G1FLAGS_GET_M(flags));
	G1FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_M(flags));

	CU_ASSERT_EQUAL(0, G1FLAGS_GET_BBOX(flags));
	G1FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, G1FLAGS_GET_GEODETIC(flags));
	G1FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_GEODETIC(flags));

	flags = g1flags(1, 0, 1); /* z=1, m=0, geodetic=1 */

	CU_ASSERT_EQUAL(1, G1FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, G1FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(2, G1FLAGS_GET_ZM(flags));

	flags = g1flags(1, 1, 1); /* z=1, m=1, geodetic=1 */

	CU_ASSERT_EQUAL(1, G1FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(3, G1FLAGS_GET_ZM(flags));

	flags = g1flags(0, 1, 0); /* z=0, m=1, geodetic=0 */

	CU_ASSERT_EQUAL(0, G1FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(0, G1FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_M(flags));
	CU_ASSERT_EQUAL(1, G1FLAGS_GET_ZM(flags));
}

static void test_serialized1_srid(void)
{
	GSERIALIZED s;
	int32_t srid, rv;

	srid = 4326;
	gserialized1_set_srid(&s, srid);
	rv = gserialized1_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = -3005;
	gserialized1_set_srid(&s, srid);
	rv = gserialized1_get_srid(&s);
	//printf("srid=%d rv=%d\n",srid,rv);
	CU_ASSERT_EQUAL(rv, SRID_UNKNOWN);

	srid = SRID_UNKNOWN;
	gserialized1_set_srid(&s, srid);
	rv = gserialized1_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = SRID_UNKNOWN;
	gserialized1_set_srid(&s, srid);
	rv = gserialized1_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 100000;
	gserialized1_set_srid(&s, srid);
	rv = gserialized1_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);
}

static void test_gserialized1_from_lwgeom_size(void)
{
	LWGEOM *g;
	size_t size = 0;

	g = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 32 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POINT(0 0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 40 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTIPOINT(0 0 0, 1 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 80 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("LINESTRING(0 0, 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 48 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTILINESTRING((0 0, 1 1),(0 0, 1 1))", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 96 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 104 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((-1 -1, -1 2, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized1_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 184 );
	lwgeom_free(g);

}

static void test_lwgeom_calculate_gbox(void)
{
	LWGEOM *g;
	GBOX b;

	g = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	lwgeom_calculate_gbox_cartesian(g, &b);
	CU_ASSERT_DOUBLE_EQUAL(b.xmin, 0.0, 0.0000001);
	lwgeom_free(g);

	/* Inf = 0x7FF0000000000000 */
	/* POINT(0 0) = 00 00000001 0000000000000000 0000000000000000 */
	/* POINT(0 Inf) = 00 00000001 0000000000000000 7FF0000000000000 */
	g = lwgeom_from_hexwkb("000000000100000000000000007FF0000000000000", LW_PARSER_CHECK_NONE);
	lwgeom_calculate_gbox_cartesian(g, &b);
	CU_ASSERT_DOUBLE_EQUAL(b.xmin, 0.0, 0.0000001);
	CU_ASSERT(isinf(b.ymax));
	lwgeom_free(g);

	/* LINESTRING(0 0, 0 Inf) = 00 00000002 00000002 0000000000000000 7FF0000000000000 0000000000000000 0000000000000000 */
	/* Inf should show up in bbox */
	g = lwgeom_from_hexwkb("00000000020000000200000000000000007FF000000000000000000000000000000000000000000000", LW_PARSER_CHECK_NONE);
	lwgeom_calculate_gbox_cartesian(g, &b);
	CU_ASSERT_DOUBLE_EQUAL(b.xmin, 0.0, 0.0000001);
	CU_ASSERT(isinf(b.ymax));
	lwgeom_free(g);

	/* Geometry with NaN 0101000020E8640000000000000000F8FF000000000000F8FF */
	/* NaN should show up in bbox for "SRID=4326;POINT(0 NaN)" */
	g = lwgeom_from_hexwkb("0101000020E86400000000000000000000000000000000F8FF", LW_PARSER_CHECK_NONE);
	lwgeom_calculate_gbox_cartesian(g, &b);
	CU_ASSERT(isnan(b.ymax));
	lwgeom_free(g);

}

static void test_lwgeom_from_gserialized(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *in_ewkt;
	char *out_ewkt;
	size_t i = 0;

	char *ewkt[] =
	{
		"POINT EMPTY",
		"POINT(0 0.2)",
		"LINESTRING EMPTY",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"MULTIPOINT EMPTY",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING EMPTY",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
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
		"MULTICURVE((5 5 1 3,3 5 2 2,3 3 3 1,0 3 1 1),CIRCULARSTRING(0 0 0 0,0.26794 1 3 -2,0.5857864 1.414213 1 2))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING EMPTY))",
	};

	for ( i = 0; i < (sizeof ewkt/sizeof(char*)); i++ )
	{
		LWGEOM* geom2;
		size_t sz1, sz2;

		in_ewkt = ewkt[i];
		geom = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		lwgeom_add_bbox(geom);
		if ( geom->bbox ) gbox_float_round(geom->bbox);
		sz1 = gserialized1_from_lwgeom_size(geom);
		g = gserialized1_from_lwgeom(geom, &sz2);
		CU_ASSERT_EQUAL(sz1, sz2);

		geom2 = lwgeom_from_gserialized(g);
		out_ewkt = lwgeom_to_ewkt(geom2);

		/* printf("\n in = %s\nout = %s\n", in_ewkt, out_ewkt); */
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);

		/* either both or none of the bboxes are null */
		CU_ASSERT( (geom->bbox != NULL) || (geom2->bbox == NULL) );

		/* either both are null or they are the same */
		CU_ASSERT(geom->bbox == NULL || gbox_same(geom->bbox, geom2->bbox));

		lwgeom_free(geom);
		lwgeom_free(geom2);
		lwfree(g);
		lwfree(out_ewkt);
	}

}


static void test_gserialized1_is_empty(void)
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
		GSERIALIZED *g = gserialized1_from_lwgeom(lw, 0);
		int ie = gserialized1_is_empty(g);
		// printf("%s: we say %d, they say %d\n", cases[i].wkt, cases[i].isempty, ie);
		CU_ASSERT_EQUAL(ie, cases[i].isempty);
		lwgeom_free(lw);
		lwfree(g);
		i++;
	}
}


static void test_geometry_type_from_string(void)
{
	int rv;
	uint8_t type = 0;
	int z = 0, m = 0;
	char *str;

	str = "  POINTZ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
	CU_ASSERT_EQUAL(type, POINTTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 0);

	str = "LINESTRINGM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
	CU_ASSERT_EQUAL(type, LINETYPE);
	CU_ASSERT_EQUAL(z, 0);
	CU_ASSERT_EQUAL(m, 1);

	str = "MULTIPOLYGONZM";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
	CU_ASSERT_EQUAL(type, MULTIPOLYGONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);

	str = "  GEOMETRYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
	CU_ASSERT_EQUAL(type, COLLECTIONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);

	str = "  GEOMERYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, LW_FAILURE);

}

static void test_lwgeom_count_vertices(void)
{
	LWGEOM *geom;

	geom = lwgeom_from_wkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),4);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("SRID=1;MULTILINESTRING((-1 -131,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),16);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,211 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),30);
	lwgeom_free(geom);

}

static void test_on_gser_lwgeom_count_vertices(void)
{
	LWGEOM *lwgeom;
	GSERIALIZED *g_ser1;
	size_t ret_size;

	lwgeom = lwgeom_from_wkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1,1 1,2 2,4 5)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	g_ser1 = gserialized1_from_lwgeom(lwgeom, &ret_size);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1);

	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_free(lwgeom);

	lwfree(g_ser1);

}

static void test_lwcollection_extract(void)
{

	LWGEOM *geom;
	LWCOLLECTION *col;

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(POINT(0 0))", LW_PARSER_CHECK_NONE);

	col = lwcollection_extract((LWCOLLECTION*)geom, 1);
	CU_ASSERT_EQUAL(col->type, MULTIPOINTTYPE);
	lwcollection_free(col);

	col = lwcollection_extract((LWCOLLECTION*)geom, 2);
	CU_ASSERT_EQUAL(col->type, MULTILINETYPE);
	lwcollection_free(col);

	col = lwcollection_extract((LWCOLLECTION*)geom, 3);
	CU_ASSERT_EQUAL(col->type, MULTIPOLYGONTYPE);
	lwcollection_free(col);

	lwgeom_free(geom);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION EMPTY", LW_PARSER_CHECK_NONE);

	col = lwcollection_extract((LWCOLLECTION*)geom, 1);
	CU_ASSERT_EQUAL(col->type, MULTIPOINTTYPE);
	lwcollection_free(col);

	col = lwcollection_extract((LWCOLLECTION*)geom, 2);
	CU_ASSERT_EQUAL(col->type, MULTILINETYPE);
	lwcollection_free(col);

	col = lwcollection_extract((LWCOLLECTION*)geom, 3);
	CU_ASSERT_EQUAL(col->type, MULTIPOLYGONTYPE);
	lwcollection_free(col);

	lwgeom_free(geom);
}

static void test_lwgeom_free(void)
{
	LWGEOM *geom;

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(geom->type, COLLECTIONTYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_wkt("POLYGON EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(geom->type, POLYGONTYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_wkt("LINESTRING EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(geom->type, LINETYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_wkt("POINT EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(geom->type, POINTTYPE);
	lwgeom_free(geom);

}

static void do_lwgeom_swap_ordinates(char *in, char *out)
{
	LWGEOM *g;
	char * t;
	double xmax, ymax;
	int testbox;

	g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwgeom_add_bbox(g);

	testbox = (g->bbox != NULL);
	if ( testbox )
	{
		xmax = g->bbox->xmax;
		ymax = g->bbox->ymax;
	}

    lwgeom_swap_ordinates(g,LWORD_X,LWORD_Y);

	if ( testbox )
	{
		CU_ASSERT_DOUBLE_EQUAL(g->bbox->xmax, ymax, 0.00001);
		CU_ASSERT_DOUBLE_EQUAL(g->bbox->ymax, xmax, 0.00001);
	}

	t = lwgeom_to_wkt(g, WKT_EXTENDED, 8, NULL);
	if (t == NULL) fprintf(stderr, "In:%s", in);
	if (strcmp(t, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n", in, t, out);

	CU_ASSERT_STRING_EQUAL(t, out)

	lwgeom_free(g);
	lwfree(t);
}

static void test_lwgeom_swap_ordinates(void)
{
	/*
	     * 2D geometries types
	     */
	do_lwgeom_swap_ordinates(
	    "POINT(1 2)",
	    "POINT(2 1)"
	);

	do_lwgeom_swap_ordinates(
	    "LINESTRING(1 2,3 4)",
	    "LINESTRING(2 1,4 3)"
	);

	do_lwgeom_swap_ordinates(
	    "POLYGON((1 2,3 4,5 6,1 2))",
	    "POLYGON((2 1,4 3,6 5,2 1))"
	);

	do_lwgeom_swap_ordinates(
	    "POLYGON((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8))",
	    "POLYGON((2 1,4 3,6 5,2 1),(8 7,10 9,12 11,8 7))"
	);

	do_lwgeom_swap_ordinates(
	    "MULTIPOINT(1 2,3 4)",
	    "MULTIPOINT(2 1,4 3)"
	);

	do_lwgeom_swap_ordinates(
	    "MULTILINESTRING((1 2,3 4),(5 6,7 8))",
	    "MULTILINESTRING((2 1,4 3),(6 5,8 7))"
	);

	do_lwgeom_swap_ordinates(
	    "MULTIPOLYGON(((1 2,3 4,5 6,7 8)),((9 10,11 12,13 14,10 9)))",
	    "MULTIPOLYGON(((2 1,4 3,6 5,8 7)),((10 9,12 11,14 13,9 10)))"
	);

	do_lwgeom_swap_ordinates(
	    "GEOMETRYCOLLECTION EMPTY",
	    "GEOMETRYCOLLECTION EMPTY"
	);

	do_lwgeom_swap_ordinates(
	    "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))",
	    "GEOMETRYCOLLECTION(POINT(2 1),LINESTRING(4 3,6 5))"
	);

	do_lwgeom_swap_ordinates(
	    "GEOMETRYCOLLECTION(POINT(1 2),GEOMETRYCOLLECTION(LINESTRING(3 4,5 6)))",
	    "GEOMETRYCOLLECTION(POINT(2 1),GEOMETRYCOLLECTION(LINESTRING(4 3,6 5)))"
	);

	do_lwgeom_swap_ordinates(
	    "CIRCULARSTRING(-2 0,0 2,2 0,0 2,2 4)",
	    "CIRCULARSTRING(0 -2,2 0,0 2,2 0,4 2)"
	);

	do_lwgeom_swap_ordinates(
	    "COMPOUNDCURVE(CIRCULARSTRING(0 1,1 1,1 0),(1 0,0 1))",
	    "COMPOUNDCURVE(CIRCULARSTRING(1 0,1 1,0 1),(0 1,1 0))"
	);

	do_lwgeom_swap_ordinates(
	    "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))",
	    "CURVEPOLYGON(CIRCULARSTRING(0 -2,-1 -1,0 0,-1 1,0 2,2 0,0 -2),(0 -1,0.5 0,0 1,1 0,0 -1))"
	);

	do_lwgeom_swap_ordinates(
	    "MULTICURVE((5 5,3 5,3 3,0 3),CIRCULARSTRING(0 0,2 1,2 3))",
	    "MULTICURVE((5 5,5 3,3 3,3 0),CIRCULARSTRING(0 0,1 2,3 2))"
	);

	do_lwgeom_swap_ordinates(
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(0 -2,-1 -1,0 0,-1 1,0 2,2 0,0 -2),(0 -1,0.5 0,0 1,1 0,0 -1)),((8 7,10 10,14 6,11 4,8 7)))"
	);


	/*
	     * Ndims
	     */

	do_lwgeom_swap_ordinates(
	    "POINT(1 2 3)",
	    "POINT(2 1 3)"
	);

	do_lwgeom_swap_ordinates(
	    "POINTM(1 2 3)",
	    "POINTM(2 1 3)"
	);

	do_lwgeom_swap_ordinates(
	    "POINT(1 2 3 4)",
	    "POINT(2 1 3 4)"
	);


	/*
	* Srid
	*/

	do_lwgeom_swap_ordinates(
	    "SRID=4326;POINT(1 2)",
	    "SRID=4326;POINT(2 1)"
	);

	do_lwgeom_swap_ordinates(
	    "SRID=0;POINT(1 2)",
	    "POINT(2 1)"
	);
}

static void test_f2d(void)
{
	double d = 1000000.123456789123456789;
	float f;
	double e;

	f = next_float_down(d);
	d = next_float_down(f);
	CU_ASSERT_DOUBLE_EQUAL(f,d, 0.0000001);

	e = (double)f;
	CU_ASSERT_DOUBLE_EQUAL(f,e, 0.0000001);

	f = next_float_down(d);
	d = next_float_down(f);
	CU_ASSERT_DOUBLE_EQUAL(f,d, 0.0000001);

	f = next_float_up(d);
	d = next_float_up(f);
	CU_ASSERT_DOUBLE_EQUAL(f,d, 0.0000001);

	f = next_float_up(d);
	d = next_float_up(f);
	CU_ASSERT_DOUBLE_EQUAL(f,d, 0.0000001);

	d = DBL_MAX;
	f = next_float_up(d);
	d = next_float_up(f);
	CU_ASSERT_DOUBLE_EQUAL(f, d, 0.0000001);

	d = DBL_MAX;
	f = next_float_down(d);
	d = next_float_down(f);
	CU_ASSERT_DOUBLE_EQUAL(f, d, 0.0000001);

	d = -DBL_MAX;
	f = next_float_up(d);
	d = next_float_up(f);
	CU_ASSERT_DOUBLE_EQUAL(f, d, 0.0000001);

	d = -DBL_MAX;
	f = next_float_down(d);
	d = next_float_down(f);
	CU_ASSERT_DOUBLE_EQUAL(f, d, 0.0000001);
}

/*
 * This is a test for memory leaks, can't really test
 * w/out checking with a leak detector (ie: valgrind)
 *
 * See http://trac.osgeo.org/postgis/ticket/1102
 */
static void test_lwgeom_clone(void)
{
	size_t i;

	char *ewkt[] =
	{
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
		"SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"MULTICURVE((5 5 1 3,3 5 2 2,3 3 3 1,0 3 1 1),CIRCULARSTRING(0 0 0 0,0.26794 1 3 -2,0.5857864 1.414213 1 2))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
		"TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))"
	};


	for ( i = 0; i < (sizeof ewkt/sizeof(char *)); i++ )
	{
		LWGEOM *geom, *cloned;
		char *in_ewkt;
		char *out_ewkt;

		in_ewkt = ewkt[i];
		geom = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		cloned = lwgeom_clone(geom);
		out_ewkt = lwgeom_to_ewkt(cloned);
		if (strcmp(in_ewkt, out_ewkt))
			fprintf(stderr, "\nExp:  %s\nObt:  %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwfree(out_ewkt);
		lwgeom_free(cloned);
		lwgeom_free(geom);
	}


}

/*
 * Test lwgeom_force_clockwise
 */
static void test_lwgeom_force_clockwise(void)
{
	LWGEOM *geom;
	LWGEOM *geom2;
	char *in_ewkt, *out_ewkt;

	/* counterclockwise, must be reversed */
	geom = lwgeom_from_wkt("POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FALSE(lwgeom_is_clockwise(geom));
	lwgeom_force_clockwise(geom);
	in_ewkt = "POLYGON((0 0,0 10,10 10,10 0,0 0))";
	out_ewkt = lwgeom_to_ewkt(geom);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);

	/* clockwise, fine as is */
	geom = lwgeom_from_wkt("POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_TRUE(lwgeom_is_clockwise(geom));
	lwgeom_force_clockwise(geom);
	in_ewkt = "POLYGON((0 0,0 10,10 10,10 0,0 0))";
	out_ewkt = lwgeom_to_ewkt(geom);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);

	/* counterclockwise shell (must be reversed), mixed-wise holes */
	geom = lwgeom_from_wkt("POLYGON((0 0,10 0,10 10,0 10,0 0),(2 2,2 4,4 2,2 2),(6 2,8 2,8 4,6 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FALSE(lwgeom_is_clockwise(geom));
	lwgeom_force_clockwise(geom);
	in_ewkt = "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,2 4,2 2),(6 2,8 2,8 4,6 2))";
	out_ewkt = lwgeom_to_ewkt(geom);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:  %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);

	/* clockwise shell (fine), mixed-wise holes */
	geom = lwgeom_from_wkt("POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,2 4,2 2),(6 2,8 4,8 2,6 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FALSE(lwgeom_is_clockwise(geom));
	lwgeom_force_clockwise(geom);
	in_ewkt = "POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,2 4,2 2),(6 2,8 2,8 4,6 2))";
	out_ewkt = lwgeom_to_ewkt(geom);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:  %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);

	/* clockwise narrow ring, fine as-is */
	/* NOTE: this is a narrow ring, see ticket #1302 */
	in_ewkt  = "0103000000010000000500000000917E9BA468294100917E9B8AEA2841C976BE1FA4682941C976BE9F8AEA2841B39ABE1FA46829415ACCC29F8AEA284137894120A4682941C976BE9F8AEA284100917E9BA468294100917E9B8AEA2841";
	geom = lwgeom_from_hexwkb(in_ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_hexwkb(in_ewkt, LW_PARSER_CHECK_NONE);
	lwgeom_force_clockwise(geom2);

	/** use same check instead of strcmp to account
	  for difference in endianness **/
	CU_ASSERT( lwgeom_same(geom, geom2) );
	lwgeom_free(geom);
	lwgeom_free(geom2);
}

/*
 * Test lwgeom_is_empty
 */
static void test_lwgeom_is_empty(void)
{
	LWGEOM *geom;

	geom = lwgeom_from_wkt("POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( !lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POINT EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("LINESTRING EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POLYGON EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOINT EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTILINESTRING EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOLYGON EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT EMPTY, LINESTRING EMPTY, POLYGON EMPTY, MULTIPOINT EMPTY, MULTILINESTRING EMPTY, MULTIPOLYGON EMPTY, GEOMETRYCOLLECTION(MULTIPOLYGON EMPTY))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_is_empty(geom) );
	lwgeom_free(geom);

}

/*
 * Test lwgeom_same
 */
static void test_lwgeom_same(void)
{
	LWGEOM *geom, *geom2;

	geom = lwgeom_from_wkt("POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("LINESTRING(0 0, 2 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTILINESTRING((0 0, 2 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOINT((0 0),(4 5))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POINT EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("LINESTRING EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_add_bbox(geom);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POLYGON EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOINT EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTILINESTRING EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("MULTIPOLYGON EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT EMPTY, LINESTRING EMPTY, POLYGON EMPTY, MULTIPOINT EMPTY, MULTILINESTRING EMPTY, MULTIPOLYGON EMPTY, GEOMETRYCOLLECTION(MULTIPOLYGON EMPTY))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( lwgeom_same(geom, geom) );
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY, POINT EMPTY, LINESTRING EMPTY, POLYGON EMPTY, MULTIPOINT EMPTY, MULTILINESTRING EMPTY, MULTIPOLYGON EMPTY, GEOMETRYCOLLECTION(MULTIPOLYGON EMPTY))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( ! lwgeom_same(geom, geom2) );
	lwgeom_free(geom);
	lwgeom_free(geom2);

	geom = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("MULTIPOINT((0 0))", LW_PARSER_CHECK_NONE);
	CU_ASSERT( ! lwgeom_same(geom, geom2) );
	lwgeom_free(geom);
	lwgeom_free(geom2);

	geom = lwgeom_from_wkt("POINT EMPTY", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POINT Z EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT( ! lwgeom_same(geom, geom2) );
	lwgeom_free(geom);
	lwgeom_free(geom2);

}

/*
 * Test lwgeom_force_curve
 */
static void test_lwgeom_as_curve(void)
{
	LWGEOM *geom;
	LWGEOM *geom2;
	char *in_ewkt, *out_ewkt;

	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_as_curve(geom);
	in_ewkt = "COMPOUNDCURVE((0 0,10 0))";
	out_ewkt = lwgeom_to_ewkt(geom2);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);
	lwgeom_free(geom2);

	geom = lwgeom_from_wkt("MULTILINESTRING((0 0, 10 0))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_as_curve(geom);
	in_ewkt = "MULTICURVE((0 0,10 0))";
	out_ewkt = lwgeom_to_ewkt(geom2);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);
	lwgeom_free(geom2);

	geom = lwgeom_from_wkt("POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_as_curve(geom);
	in_ewkt = "CURVEPOLYGON((0 0,10 0,10 10,0 10,0 0))";
	out_ewkt = lwgeom_to_ewkt(geom2);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);
	lwgeom_free(geom2);

	geom = lwgeom_from_wkt("MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_as_curve(geom);
	in_ewkt = "MULTISURFACE(((0 0,10 0,10 10,0 10,0 0)))";
	out_ewkt = lwgeom_to_ewkt(geom2);
	if (strcmp(in_ewkt, out_ewkt))
		fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
	CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(geom);
	lwgeom_free(geom2);

}

static void test_lwline_from_lwmpoint(void)
{
	LWLINE *line;
	LWMPOINT *mpoint;

	mpoint = (LWMPOINT*)lwgeom_from_wkt("MULTIPOINT(0 0, 0 1, 1 1, 1 2, 2 2)", LW_PARSER_CHECK_NONE);
	line = lwline_from_lwmpoint(SRID_DEFAULT, mpoint);
	CU_ASSERT_EQUAL(line->points->npoints, mpoint->ngeoms);
	CU_ASSERT_DOUBLE_EQUAL(lwline_length_2d(line), 4.0, 0.000001);

	lwline_free(line);
	lwmpoint_free(mpoint);
}

/*
 * Test lwgeom_scale
 */
static void test_lwgeom_scale(void)
{
	LWGEOM *geom;
	POINT4D factor;
	char *out_ewkt;
	GBOX *box;

	geom = lwgeom_from_wkt("SRID=4326;GEOMETRYCOLLECTION(POINT(0 1 2 3),POLYGON((-1 -1 0 1,-1 2.5 0 1,2 2 0 1,2 -1 0 1,-1 -1 0 1),(0 0 1 2,0 1 1 2,1 1 1 2,1 0 2 3,0 0 1 2)),LINESTRING(0 0 0 0, 1 2 3 4))", LW_PARSER_CHECK_NONE);
	factor.x = 2; factor.y = 3; factor.z = 4; factor.m = 5;
	lwgeom_scale(geom, &factor);
	out_ewkt = lwgeom_to_ewkt(geom);
	ASSERT_STRING_EQUAL(out_ewkt, "SRID=4326;GEOMETRYCOLLECTION(POINT(0 3 8 15),POLYGON((-2 -3 0 5,-2 7.5 0 5,4 6 0 5,4 -3 0 5,-2 -3 0 5),(0 0 4 10,0 3 4 10,2 3 4 10,2 0 8 15,0 0 4 10)),LINESTRING(0 0 0 0,2 6 12 20))");
	lwgeom_free(geom);
	lwfree(out_ewkt);

	geom = lwgeom_from_wkt("POINT(1 1 1 1)", LW_PARSER_CHECK_NONE);
	lwgeom_add_bbox(geom);
	factor.x = 2; factor.y = 3; factor.z = 4; factor.m = 5;
	lwgeom_scale(geom, &factor);
	box = geom->bbox;
	ASSERT_DOUBLE_EQUAL(box->xmin, 2);
	ASSERT_DOUBLE_EQUAL(box->xmax, 2);
	ASSERT_DOUBLE_EQUAL(box->ymin, 3);
	ASSERT_DOUBLE_EQUAL(box->ymax, 3);
	ASSERT_DOUBLE_EQUAL(box->zmin, 4);
	ASSERT_DOUBLE_EQUAL(box->zmax, 4);
	ASSERT_DOUBLE_EQUAL(box->mmin, 5);
	ASSERT_DOUBLE_EQUAL(box->mmax, 5);
	lwgeom_free(geom);
}

static void test_gbox_same_2d(void)
{
	LWGEOM* g1 = lwgeom_from_wkt("LINESTRING(0 0, 1 1)", LW_PARSER_CHECK_NONE);
    LWGEOM* g2 = lwgeom_from_wkt("LINESTRING(0 0, 0 1, 1 1)", LW_PARSER_CHECK_NONE);
    LWGEOM* g3 = lwgeom_from_wkt("LINESTRING(0 0, 1 1.000000000001)", LW_PARSER_CHECK_NONE);

    lwgeom_add_bbox(g1);
    lwgeom_add_bbox(g2);
    lwgeom_add_bbox(g3);

    CU_ASSERT_TRUE(gbox_same_2d(g1->bbox, g2->bbox));
    CU_ASSERT_FALSE(gbox_same_2d(g1->bbox, g3->bbox));

    /* Serializing a GBOX with precise coordinates renders the boxes not strictly equal,
     * but still equal according to gbox_same_2d_float.
     */
    GSERIALIZED* s3 = gserialized1_from_lwgeom(g3, NULL);
    GBOX s3box;
    gserialized1_read_gbox_p(s3, &s3box);

    CU_ASSERT_FALSE(gbox_same_2d(g3->bbox, &s3box));
    CU_ASSERT_TRUE(gbox_same_2d_float(g3->bbox, &s3box));

    /* The serialized box equals itself by either the exact or closest-float compares */
    CU_ASSERT_TRUE(gbox_same_2d(&s3box, &s3box));
    CU_ASSERT_TRUE(gbox_same_2d_float(&s3box, &s3box));

    lwgeom_free(g1);
    lwgeom_free(g2);
    lwgeom_free(g3);
    lwfree(s3);
}

static void test_gserialized1_peek_gbox_p_no_box_when_empty(void)
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

		GSERIALIZED* gser = gserialized1_from_lwgeom(geom, NULL);

		CU_ASSERT_FALSE(gserialized1_has_bbox(gser));

		CU_ASSERT_EQUAL(LW_FAILURE, gserialized1_peek_gbox_p(gser, &box));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static void test_gserialized1_peek_gbox_p_gets_correct_box(void)
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

		GSERIALIZED* gser = gserialized1_from_lwgeom(geom, NULL);

		CU_ASSERT_FALSE(gserialized1_has_bbox(gser));

		lwgeom_calculate_gbox(geom, &box_from_lwgeom);
		gserialized1_peek_gbox_p(gser, &box_from_peek);

		gbox_float_round(&box_from_lwgeom);

		CU_ASSERT_TRUE(gbox_same(&box_from_peek, &box_from_lwgeom));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static void test_gserialized1_peek_gbox_p_fails_for_unsupported_cases(void)
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
		size_t expected_size = gserialized1_from_lwgeom_size(geom);
		GSERIALIZED* gser = lwalloc(expected_size);
		uint8_t* ptr = (uint8_t*) gser;

		ptr += 8; // Skip header
		gserialized1_from_lwgeom_any(geom, ptr);
		gser->gflags = lwflags_get_g1flags(geom->flags);

		CU_ASSERT_FALSE(gserialized1_has_bbox(gser));
		CU_ASSERT_EQUAL(LW_FAILURE, gserialized1_peek_gbox_p(gser, &box));

		lwgeom_free(geom);
		lwfree(gser);
	}
}

static void test_signum_macro(void)
{
	CU_ASSERT_EQUAL(SIGNUM(-5.0),-1);
	CU_ASSERT_EQUAL(SIGNUM( 5.0), 1);
	CU_ASSERT_EQUAL(SIGNUM( 0.0), 0);
	CU_ASSERT_EQUAL(SIGNUM(10) * 5, 5);
	CU_ASSERT_EQUAL(SIGNUM(-10) * 5, -5);
}

static int
peek1_point_helper(char *geometry, POINT4D *p)
{
	cu_error_msg_reset();
	p->x = p->y = p->z = p->m = 0;
	LWGEOM *geom = lwgeom_from_wkt(geometry, LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	GSERIALIZED *g = gserialized1_from_lwgeom(geom, NULL);
	CU_ASSERT(g != NULL);

	int ret = gserialized1_peek_first_point(g, p);
	lwfree(g);
	lwgeom_free(geom);

	return ret;
}

static void
test_gserialized1_peek_first_point(void)
{
	POINT4D p = {0};

	CU_ASSERT(peek1_point_helper("POINT(1 2)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 1);
	CU_ASSERT_EQUAL(p.y, 2);

	CU_ASSERT(peek1_point_helper("POINTZ(10 20 30)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 10);
	CU_ASSERT_EQUAL(p.y, 20);
	CU_ASSERT_EQUAL(p.z, 30);

	CU_ASSERT(peek1_point_helper("POINTM(100 200 300)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 100);
	CU_ASSERT_EQUAL(p.y, 200);
	CU_ASSERT_EQUAL(p.m, 300);

	CU_ASSERT(peek1_point_helper("POINTZM(1000 2000 3000 4000)", &p) == LW_SUCCESS);
	CU_ASSERT_EQUAL(p.x, 1000);
	CU_ASSERT_EQUAL(p.y, 2000);
	CU_ASSERT_EQUAL(p.z, 3000);
	CU_ASSERT_EQUAL(p.m, 4000);

	CU_ASSERT(peek1_point_helper("MULTIPOINT((0 0), (1 1))", &p) == LW_FAILURE);
	CU_ASSERT(peek1_point_helper("LINESTRING(0 0, 1 1)", &p) == LW_FAILURE);
	CU_ASSERT(peek1_point_helper("MULTILINESTRING((0 0, 1 1), (0 0, 1 1))", &p) == LW_FAILURE);
	CU_ASSERT(peek1_point_helper("POLYGON((0 0, 1 1, 1 0, 0 0))", &p) == LW_FAILURE);
}

/*
** Used by test harness to register the tests in this file.
*/
void gserialized1_suite_setup(void);
void gserialized1_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("serialization/deserialization v1", NULL, NULL);
	PG_ADD_TEST(suite, test_typmod_macros);
	PG_ADD_TEST(suite, test_flags_macros);
	PG_ADD_TEST(suite, test_serialized1_srid);
	PG_ADD_TEST(suite, test_gserialized1_from_lwgeom_size);
	PG_ADD_TEST(suite, test_lwgeom_from_gserialized);
	PG_ADD_TEST(suite, test_lwgeom_count_vertices);
	PG_ADD_TEST(suite, test_on_gser_lwgeom_count_vertices);
	PG_ADD_TEST(suite, test_geometry_type_from_string);
	PG_ADD_TEST(suite, test_lwcollection_extract);
	PG_ADD_TEST(suite, test_lwgeom_free);
	PG_ADD_TEST(suite, test_lwgeom_swap_ordinates);
	PG_ADD_TEST(suite, test_f2d);
	PG_ADD_TEST(suite, test_lwgeom_clone);
	PG_ADD_TEST(suite, test_lwgeom_force_clockwise);
	PG_ADD_TEST(suite, test_lwgeom_calculate_gbox);
	PG_ADD_TEST(suite, test_lwgeom_is_empty);
	PG_ADD_TEST(suite, test_lwgeom_same);
	PG_ADD_TEST(suite, test_lwline_from_lwmpoint);
	PG_ADD_TEST(suite, test_lwgeom_as_curve);
	PG_ADD_TEST(suite, test_lwgeom_scale);
	PG_ADD_TEST(suite, test_gserialized1_is_empty);
	PG_ADD_TEST(suite, test_gserialized1_peek_gbox_p_no_box_when_empty);
	PG_ADD_TEST(suite, test_gserialized1_peek_gbox_p_gets_correct_box);
	PG_ADD_TEST(suite, test_gserialized1_peek_gbox_p_fails_for_unsupported_cases);
	PG_ADD_TEST(suite, test_gbox_same_2d);
	PG_ADD_TEST(suite, test_signum_macro);
	PG_ADD_TEST(suite, test_gserialized1_peek_first_point);
}
