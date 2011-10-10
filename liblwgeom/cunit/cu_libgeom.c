/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
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
#include "cu_tester.h"

static void test_typmod_macros(void)
{
	int32_t typmod = 0;
	int srid = 4326;
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

	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	FLAGS_SET_Z(flags, 0);
	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
	FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_M(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));
	FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_GEODETIC(flags));
	FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));

	flags = 0;
	flags = gflags(1, 0, 1); /* z=1, m=0, geodetic=1 */

	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
}

static void test_serialized_srid(void)
{
	GSERIALIZED s;
	int32_t srid, rv;

	srid = 4326;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = -3005;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	//printf("srid=%d rv=%d\n",srid,rv);
	CU_ASSERT_EQUAL(rv, SRID_UNKNOWN);

	srid = SRID_UNKNOWN;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = SRID_UNKNOWN;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 100000;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);
}

static void test_gserialized_from_lwgeom_size(void)
{
	LWGEOM *g;
	size_t size = 0;

	g = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 32 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POINT(0 0 0)", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 40 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTIPOINT(0 0 0, 1 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 80 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("LINESTRING(0 0, 1 1)", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 48 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("MULTILINESTRING((0 0, 1 1),(0 0, 1 1))", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 96 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 104 );
	lwgeom_free(g);

	g = lwgeom_from_wkt("POLYGON((-1 -1, -1 2, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 184 );
	lwgeom_free(g);

}

static void test_gbox_serialized_size(void)
{
	uint8_t flags = gflags(0, 0, 0);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),16);
	FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),16);
	FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);
	FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),32);
	FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);

}




static void test_lwgeom_from_gserialized(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *in_ewkt;
	char *out_ewkt;
	int i = 0;

	char ewkt[][512] =
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
	};

	for ( i = 0; i < 13; i++ )
	{
		in_ewkt = ewkt[i];
		geom = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		g = gserialized_from_lwgeom(geom, 0, 0);
		lwgeom_free(geom);
		geom = lwgeom_from_gserialized(g);
		out_ewkt = lwgeom_to_ewkt(geom);
		//printf("\n in = %s\nout = %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwgeom_free(geom);
		lwfree(g);
		lwfree(out_ewkt);
	}

}

static void test_geometry_type_from_string(void)
{
	int rv;
	int type = 0, z = 0, m = 0;
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
	g_ser1 = gserialized_from_lwgeom(lwgeom, 1, &ret_size);
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
	CU_ASSERT_EQUAL(TYPE_GETTYPE(col->type), MULTIPOINTTYPE);

	/* How to properly release 'col' ? 
	 * See http://http://trac.osgeo.org/postgis/ticket/1102
	 */
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

static void do_lwgeom_flip_coordinates(char *in, char *out)
{
	LWGEOM *g,*h;
	char * t;

	g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	h = lwgeom_flip_coordinates(g);

	t = lwgeom_to_wkt(g, WKT_EXTENDED, 8, NULL);
	if (t == NULL) fprintf(stderr, "In:%s", in);
	if (strcmp(t, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n", in, t, out);

	CU_ASSERT_STRING_EQUAL(t, out)

	lwgeom_free(g);
	lwfree(t);
}

static void test_lwgeom_flip_coordinates(void)
{
	/*
	     * 2D geometries types
	     */
	do_lwgeom_flip_coordinates(
	    "POINT(1 2)",
	    "POINT(2 1)"
	);

	do_lwgeom_flip_coordinates(
	    "LINESTRING(1 2,3 4)",
	    "LINESTRING(2 1,4 3)"
	);

	do_lwgeom_flip_coordinates(
	    "POLYGON((1 2,3 4,5 6,1 2))",
	    "POLYGON((2 1,4 3,6 5,2 1))"
	);

	do_lwgeom_flip_coordinates(
	    "POLYGON((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8))",
	    "POLYGON((2 1,4 3,6 5,2 1),(8 7,10 9,12 11,8 7))"
	);

	do_lwgeom_flip_coordinates(
	    "MULTIPOINT(1 2,3 4)",
	    "MULTIPOINT(2 1,4 3)"
	);

	do_lwgeom_flip_coordinates(
	    "MULTILINESTRING((1 2,3 4),(5 6,7 8))",
	    "MULTILINESTRING((2 1,4 3),(6 5,8 7))"
	);

	do_lwgeom_flip_coordinates(
	    "MULTIPOLYGON(((1 2,3 4,5 6,7 8)),((9 10,11 12,13 14,10 9)))",
	    "MULTIPOLYGON(((2 1,4 3,6 5,8 7)),((10 9,12 11,14 13,9 10)))"
	);

	do_lwgeom_flip_coordinates(
	    "GEOMETRYCOLLECTION EMPTY",
	    "GEOMETRYCOLLECTION EMPTY"
	);

	do_lwgeom_flip_coordinates(
	    "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))",
	    "GEOMETRYCOLLECTION(POINT(2 1),LINESTRING(4 3,6 5))"
	);

	do_lwgeom_flip_coordinates(
	    "GEOMETRYCOLLECTION(POINT(1 2),GEOMETRYCOLLECTION(LINESTRING(3 4,5 6)))",
	    "GEOMETRYCOLLECTION(POINT(2 1),GEOMETRYCOLLECTION(LINESTRING(4 3,6 5)))"
	);

	do_lwgeom_flip_coordinates(
	    "CIRCULARSTRING(-2 0,0 2,2 0,0 2,2 4)",
	    "CIRCULARSTRING(0 -2,2 0,0 2,2 0,4 2)"
	);

	do_lwgeom_flip_coordinates(
	    "COMPOUNDCURVE(CIRCULARSTRING(0 1,1 1,1 0),(1 0,0 1))",
	    "COMPOUNDCURVE(CIRCULARSTRING(1 0,1 1,0 1),(0 1,1 0))"
	);

	do_lwgeom_flip_coordinates(
	    "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))",
	    "CURVEPOLYGON(CIRCULARSTRING(0 -2,-1 -1,0 0,-1 1,0 2,2 0,0 -2),(0 -1,0.5 0,0 1,1 0,0 -1))"
	);

	do_lwgeom_flip_coordinates(
	    "MULTICURVE((5 5,3 5,3 3,0 3),CIRCULARSTRING(0 0,2 1,2 3))",
	    "MULTICURVE((5 5,5 3,3 3,3 0),CIRCULARSTRING(0 0,1 2,3 2))"
	);

	do_lwgeom_flip_coordinates(
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(0 -2,-1 -1,0 0,-1 1,0 2,2 0,0 -2),(0 -1,0.5 0,0 1,1 0,0 -1)),((8 7,10 10,14 6,11 4,8 7)))"
	);


	/*
	     * Ndims
	     */

	do_lwgeom_flip_coordinates(
	    "POINT(1 2 3)",
	    "POINT(2 1 3)"
	);

	do_lwgeom_flip_coordinates(
	    "POINTM(1 2 3)",
	    "POINTM(2 1 3)"
	);

	do_lwgeom_flip_coordinates(
	    "POINT(1 2 3 4)",
	    "POINT(2 1 3 4)"
	);


	/*
	     * Srid
	     */

	do_lwgeom_flip_coordinates(
	    "SRID=4326;POINT(1 2)",
	    "SRID=4326;POINT(2 1)"
	);

	do_lwgeom_flip_coordinates(
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
}

/*
 * This is a test for memory leaks, can't really test
 * w/out checking with a leak detector (ie: valgrind)
 *
 * See http://trac.osgeo.org/postgis/ticket/1102
 */
static void test_lwgeom_clone(void)
{
	int i;

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
			fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwfree(out_ewkt);
		lwgeom_free(cloned);
		lwgeom_free(geom);
	}


}


/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo libgeom_tests[] =
{
	PG_TEST(test_typmod_macros),
	PG_TEST(test_flags_macros),
	PG_TEST(test_serialized_srid),
	PG_TEST(test_gserialized_from_lwgeom_size),
	PG_TEST(test_gbox_serialized_size),
	PG_TEST(test_lwgeom_from_gserialized),
	PG_TEST(test_lwgeom_count_vertices),
	PG_TEST(test_on_gser_lwgeom_count_vertices),
	PG_TEST(test_geometry_type_from_string),
	PG_TEST(test_lwcollection_extract),
	PG_TEST(test_lwgeom_free),
	PG_TEST(test_lwgeom_flip_coordinates),
	PG_TEST(test_f2d),
	PG_TEST(test_lwgeom_clone),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo libgeom_suite = {"LibGeom Suite",  NULL,  NULL, libgeom_tests};

