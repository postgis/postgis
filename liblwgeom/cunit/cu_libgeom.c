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

#include "cu_libgeom.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_libgeom_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("LibGeom Suite", init_libgeom_suite, clean_libgeom_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_typmod_macros()", test_typmod_macros)) ||
	    (NULL == CU_add_test(pSuite, "test_flags_macros()", test_flags_macros)) ||
	    (NULL == CU_add_test(pSuite, "test_serialized_srid()", test_serialized_srid)) ||
	    (NULL == CU_add_test(pSuite, "test_gserialized_from_lwgeom_size()", test_gserialized_from_lwgeom_size)) || 
	    (NULL == CU_add_test(pSuite, "test_gserialized_from_lwgeom()", test_gserialized_from_lwgeom)) || 
	    (NULL == CU_add_test(pSuite, "test_lwgeom_from_gserialized()", test_lwgeom_from_gserialized))  ||
	    (NULL == CU_add_test(pSuite, "test_geometry_type_from_string()", test_geometry_type_from_string)) || 
	    (NULL == CU_add_test(pSuite, "test_lwgeom_check_geodetic()", test_lwgeom_check_geodetic)) || 
	    (NULL == CU_add_test(pSuite, "test_lwgeom_count_vertices()", test_lwgeom_count_vertices))  || 
	    (NULL == CU_add_test(pSuite, "test_on_gser_lwgeom_count_vertices()", test_on_gser_lwgeom_count_vertices))  ||
	    (NULL == CU_add_test(pSuite, "test_gbox_from_spherical_coordinates()", test_gbox_from_spherical_coordinates)) ||
	    (NULL == CU_add_test(pSuite, "test_gserialized_get_gbox_geocentric()", test_gserialized_get_gbox_geocentric)) ||
	    (NULL == CU_add_test(pSuite, "test_gbox_calculation()", test_gbox_calculation)) 
	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_libgeom_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_libgeom_suite(void)
{
	return 0;
}

void test_typmod_macros(void)
{
	uint32 typmod = 0;
	int srid = 4326;
	int type = 6;
	int z = 1;
	int rv;
	
	typmod = TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);
	
	typmod = TYPMOD_SET_TYPE(typmod,type);
	rv = TYPMOD_GET_TYPE(typmod);
	CU_ASSERT_EQUAL(rv,type);
	
	typmod = TYPMOD_SET_Z(typmod);
	rv = TYPMOD_GET_Z(typmod);
	CU_ASSERT_EQUAL(rv,z);
	
	rv = TYPMOD_GET_M(typmod);
	CU_ASSERT_EQUAL(rv,0);
	
}

void test_flags_macros(void)
{
	uchar flags = 0;

	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	flags = FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	flags = FLAGS_SET_Z(flags, 0);
	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
	flags = FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_M(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));
	flags = FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_GEODETIC(flags));
	flags = FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));

	flags = 0;
	flags = gflags(1, 0, 1); /* z=1, m=0, geodetic=1 */

	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
}

void test_serialized_srid(void)
{
	GSERIALIZED s;
	uint32 srid = 4326;
	uint32 rv;

	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 0;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 1000000;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);
}

void test_gserialized_from_lwgeom_size(void)
{
	LWGEOM *g;
	size_t size = 0;

	g = lwgeom_from_ewkt("POINT(0 0)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 32 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POINT(0 0 0)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 40 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("MULTIPOINT(0 0 0, 1 1 1)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 80 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("LINESTRING(0 0, 1 1)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 48 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("MULTILINESTRING((0 0, 1 1),(0 0, 1 1))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 96 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 104 );
	lwgeom_free(g);
	
	g = lwgeom_from_ewkt("POLYGON((-1 -1, -1 2, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g, 0);
	CU_ASSERT_EQUAL( size, 184 );
	lwgeom_free(g);
	
}

void test_gserialized_from_lwgeom(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	uint32 type;
	double *inspect; /* To poke right into the blob. */
	
	geom = lwgeom_from_ewkt("POINT(0 0.2)", PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	type = gserialized_get_type(g);
	CU_ASSERT_EQUAL( type, POINTTYPE );
	inspect = (double*)g;
	CU_ASSERT_EQUAL(inspect[3], 0.2);
	lwgeom_free(geom);
	lwfree(g);
	
	geom = lwgeom_from_ewkt("POLYGON((-1 -1, -1 2.5, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	type = gserialized_get_type(g);
	CU_ASSERT_EQUAL( type, POLYGONTYPE );
	inspect = (double*)g;
	CU_ASSERT_EQUAL(inspect[9], 2.5);
	lwgeom_free(geom);
	lwfree(g);

	geom = lwgeom_from_ewkt("MULTILINESTRING((0 0, 1 1),(0 0.1, 1 1))", PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	type = gserialized_get_type(g);
	CU_ASSERT_EQUAL( type, MULTILINETYPE );
	inspect = (double*)g;
	CU_ASSERT_EQUAL(inspect[12], 0.1);
	lwgeom_free(geom);
	lwfree(g);
	
}


void test_lwgeom_from_gserialized(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *in_ewkt;
	char *out_ewkt;
	int i = 0;
	
	char ewkt[][512] = { 
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
		
	for( i = 0; i < 13; i++ )
	{
		in_ewkt = ewkt[i];
		geom = lwgeom_from_ewkt(in_ewkt, PARSER_CHECK_NONE);
		g = gserialized_from_lwgeom(geom, 0, 0);
		lwgeom_free(geom);
		geom = lwgeom_from_gserialized(g);
		out_ewkt = lwgeom_to_ewkt(geom, PARSER_CHECK_NONE);
		//printf("\n in = %s\nout = %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwgeom_release(geom);
		lwfree(g);
		lwfree(out_ewkt);
	}

}

void test_geometry_type_from_string(void) 
{
	int rv;
	int type = 0, z = 0, m = 0;
	char *str;
	
	str = "  POINTZ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, POINTTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 0);

	str = "LINESTRINGM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, LINETYPE);
	CU_ASSERT_EQUAL(z, 0);
	CU_ASSERT_EQUAL(m, 1);

	str = "MULTIPOLYGONZM";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, MULTIPOLYGONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);
	
	str = "  GEOMETRYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, COLLECTIONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);

	str = "  GEOMERYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_FAILURE);

}

/* 
* Build LWGEOM on top of *aligned* structure so we can use the read-only 
* point access methods on them. 
*/
static LWGEOM* lwgeom_over_gserialized(char *wkt)
{
	LWGEOM *lwg;
	GSERIALIZED *g;
	
	lwg = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(lwg, 1, 0);
	lwgeom_free(lwg);
	return lwgeom_from_gserialized(g);
}

void test_lwgeom_check_geodetic(void)
{
	LWGEOM *geom;
	int i = 0;
	
	char ewkt[][512] = { 
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"POINT(0 220.2)",
		"LINESTRING(-1 -1,-1231 2.5,2 2,2 -1)",
		"SRID=1;MULTILINESTRING((-1 -131,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -133,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,211 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1111 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
	};
		
	for( i = 0; i < 6; i++ )
	{
		geom = lwgeom_over_gserialized(ewkt[i]);
		CU_ASSERT_EQUAL(lwgeom_check_geodetic(geom), G_TRUE);
		lwgeom_free(geom);
	}

	for( i = 6; i < 12; i++ )
	{
		//char *out_ewkt;
		geom = lwgeom_over_gserialized(ewkt[i]);
		CU_ASSERT_EQUAL(lwgeom_check_geodetic(geom), G_FALSE);
		//out_ewkt = lwgeom_to_ewkt(geom, PARSER_CHECK_NONE);
		//printf("%s\n", out_ewkt);
		lwgeom_free(geom);
	}

}

void test_lwgeom_count_vertices(void)
{
	LWGEOM *geom;

	geom = lwgeom_from_ewkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1)", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),4);
	lwgeom_free(geom);

	geom = lwgeom_from_ewkt("SRID=1;MULTILINESTRING((-1 -131,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),16);
	lwgeom_free(geom);

	geom = lwgeom_from_ewkt("SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,211 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),30);
	lwgeom_free(geom);

}


void test_on_gser_lwgeom_count_vertices(void)
{
	LWGEOM *lwgeom;
	GSERIALIZED *g_ser1;
	size_t ret_size;

	lwgeom = lwgeom_from_ewkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1,1 1,2 2,4 5)", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	g_ser1 = gserialized_from_lwgeom(lwgeom, 1, &ret_size);
    g_ser1->flags = FLAGS_SET_GEODETIC(g_ser1->flags, 1);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_release(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1); 

	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_release(lwgeom);

	lwfree(g_ser1);
	
}


void test_gbox_from_spherical_coordinates(void)
{
	double ll[64];
	GBOX *box = gbox_new(gflags(0, 0, 1));
	int rv;
	POINTARRAY *pa;

	ll[0] = 0.0;
	ll[1] = 0.0;
	ll[2] = 45.0;
	ll[3] = 45.0;

	pa = pointArray_construct((uchar*)ll, 0, 0, 2);
	rv = ptarray_calculate_gbox_geodetic(pa, box);
	//printf("%s\n", gbox_to_string(box, gflags(0, 0, 1)));
	CU_ASSERT_DOUBLE_EQUAL(box->xmin, 0.707106781187, 0.001);
	lwfree(pa);
	
	ll[0] = 45.0;
	ll[1] = 45.0;
	ll[2] = 45.5;
	ll[3] = 45.5;
	ll[4] = 46.0;
	ll[5] = 46.0;

	pa = pointArray_construct((uchar*)ll, 0, 0, 3);
	rv = ptarray_calculate_gbox_geodetic(pa, box);
	//printf("%s\n", gbox_to_string(box, gflags(0, 0, 1)));
	CU_ASSERT_DOUBLE_EQUAL(box->xmin, 0.694658370459, 0.001);
	lwfree(pa);


	ll[0] = 45.0;
	ll[1] = 45.0;

	pa = pointArray_construct((uchar*)ll, 0, 0, 1);
	rv = ptarray_calculate_gbox_geodetic(pa, box);
	//printf("%s\n", gbox_to_string(box, gflags(0, 0, 1)));
	CU_ASSERT_DOUBLE_EQUAL(box->xmin, 0.707106781187, 0.001);
	lwfree(pa);

	lwfree(box);
	
}

void test_gserialized_get_gbox_geocentric(void)
{
	LWGEOM *lwg;
	GSERIALIZED *g;
	GBOX *gbox;
	int i;

	char wkt[][512] = { 
		"POINT(45 45)",
		"MULTILINESTRING((45 45, 45 45),(45 45, 45 45))",
		"MULTILINESTRING((45 45, 45 45),(45 45, 45.5 45.5),(45 45, 46 46))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((45 45, 45 45),(45 45, 45.5 45.5),(45 45, 46 46)))",
		"MULTIPOLYGON(((45 45, 45 45, 45 45, 45 45),(45 45, 45 45, 45 45, 45 45)))",
		"LINESTRING(45 45, 45.1 45.1)",
		"MULTIPOINT(45 45, 45.05 45.05, 45.1 45.1)"
	};
    
    double xmin[] = {
		0.707106781187,
		0.707106781187,
		0.694658370459,
		0.694658370459,
		0.707106781187,
		0.705871570679,
		0.705871570679
	};
	
    for ( i = 0; i < 7; i++ )
    {
		//printf("%s\n", wkt[i]);
		lwg = lwgeom_from_ewkt(wkt[i], PARSER_CHECK_NONE);
		g = gserialized_from_lwgeom(lwg, 1, 0);
		g->flags = FLAGS_SET_GEODETIC(g->flags, 1);
		lwgeom_free(lwg);
		gbox = gserialized_calculate_gbox_geocentric(g);
		//printf("%s\n", gbox_to_string(box, g->flags));
		CU_ASSERT_DOUBLE_EQUAL(gbox->xmin, xmin[i], 0.00000001);
		lwfree(g);
		lwfree(gbox);
	}

}

void test_gbox_calculation(void)
{

	LWGEOM *geom;
	int i = 0;
	GBOX *gbox = gbox_new(gflags(0,0,0));
	BOX3D *box3d;
	
	char ewkt[][512] = { 
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"POINT(0 220.2)",
		"LINESTRING(-1 -1,-1231 2.5,2 2,2 -1)",
		"SRID=1;MULTILINESTRING((-1 -131,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -133,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,211 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1111 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
	};
		
	for( i = 0; i < 6; i++ )
	{
		geom = lwgeom_over_gserialized(ewkt[i]);
		lwgeom_calculate_gbox(geom, gbox);
		box3d = lwgeom_compute_box3d(geom);
		//printf("%g %g\n", gbox->xmin, box3d->xmin);
		CU_ASSERT_EQUAL(gbox->xmin, box3d->xmin);
		CU_ASSERT_EQUAL(gbox->xmax, box3d->xmax);
		CU_ASSERT_EQUAL(gbox->ymin, box3d->ymin);
		CU_ASSERT_EQUAL(gbox->ymax, box3d->ymax);
		lwfree(box3d);
	}
	
	lwfree(gbox);


}






