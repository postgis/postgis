/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_out_kml.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_out_kml_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("KML Out Suite", init_out_kml_suite, clean_out_kml_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_precision()", out_kml_test_precision)) ||
	    (NULL == CU_add_test(pSuite, "test_dims()", out_kml_test_dims)) ||
	    (NULL == CU_add_test(pSuite, "test_geoms()", out_kml_test_geoms))
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
int init_out_kml_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_out_kml_suite(void)
{
	return 0;
}

static void do_kml_test(char * in, char * out, int precision)
{
	LWGEOM *g;
	char * h;

	g = lwgeom_from_ewkt(in, PARSER_CHECK_NONE);
	h = lwgeom_to_kml2(lwgeom_serialize(g), precision);

	if (strcmp(h, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n", in, h, out);

	CU_ASSERT_STRING_EQUAL(h, out);

	lwgeom_free(g);
	lwfree(h);
}


static void do_kml_unsupported(char * in, char * out)
{
	LWGEOM *g;
	char *h;

	g = lwgeom_from_ewkt(in, PARSER_CHECK_NONE);
	h = lwgeom_to_kml2(lwgeom_serialize(g), 0);

	if (strcmp(cu_error_msg, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n",
		        in, cu_error_msg, out);

	CU_ASSERT_STRING_EQUAL(out, cu_error_msg);
	cu_error_msg_reset();

	lwfree(h);
	lwgeom_free(g);
}


void out_kml_test_precision(void)
{
	/* 0 precision, i.e a round */
	do_kml_test(
	    "POINT(1.1111111111111 1.1111111111111)",
	    "<Point><coordinates>1,1</coordinates></Point>",
	    0);

	/* 3 digits precision */
	do_kml_test(
	    "POINT(1.1111111111111 1.1111111111111)",
	    "<Point><coordinates>1.111,1.111</coordinates></Point>",
	    3);

	/* huge digits precision, limit is in fact 15 */
	do_kml_test(
	    "POINT(1.2345678901234 1.2345678901234)",
	    "<Point><coordinates>1.23456789,1.23456789</coordinates></Point>",
	    9);

	/* huge data */
	do_kml_test(
	    "POINT(1E300 -1E300)",
	    "<Point><coordinates>1e+300,-1e+300</coordinates></Point>",
	    0);
}


void out_kml_test_dims(void)
{
	/* 3D */
	do_kml_test(
	    "POINT(0 1 2)",
	    "<Point><coordinates>0,1,2</coordinates></Point>",
	    0);

	/* 3DM */
	do_kml_test(
	    "POINTM(0 1 2)",
	    "<Point><coordinates>0,1</coordinates></Point>",
	    0);

	/* 4D */
	do_kml_test(
	    "POINT(0 1 2 3)",
	    "<Point><coordinates>0,1,2</coordinates></Point>",
	    0);
}


void out_kml_test_geoms(void)
{
	/* Linestring */
	do_kml_test(
	    "LINESTRING(0 1,2 3,4 5)",
	    "<LineString><coordinates>0,1 2,3 4,5</coordinates></LineString>",
	    0);

	/* Polygon */
	do_kml_test(
	    "POLYGON((0 1,2 3,4 5,0 1))",
	    "<Polygon><outerBoundaryIs><LinearRing><coordinates>0,1 2,3 4,5 0,1</coordinates></LinearRing></outerBoundaryIs></Polygon>",
	    0);

	/* Polygon - with internal ring */
	do_kml_test(
	    "POLYGON((0 1,2 3,4 5,0 1),(6 7,8 9,10 11,6 7))",
	    "<Polygon><outerBoundaryIs><LinearRing><coordinates>0,1 2,3 4,5 0,1</coordinates></LinearRing></outerBoundaryIs><innerBoundaryIs><LinearRing><coordinates>6,7 8,9 10,11 6,7</coordinates></LinearRing></innerBoundaryIs></Polygon>",
	    0);

	/* MultiPoint */
	do_kml_test(
	    "MULTIPOINT(0 1,2 3)",
	    "<MultiGeometry><Point><coordinates>0,1</coordinates></Point><Point><coordinates>2,3</coordinates></Point></MultiGeometry>",
	    0);

	/* MultiLine */
	do_kml_test(
	    "MULTILINESTRING((0 1,2 3,4 5),(6 7,8 9,10 11))",
	    "<MultiGeometry><LineString><coordinates>0,1 2,3 4,5</coordinates></LineString><LineString><coordinates>6,7 8,9 10,11</coordinates></LineString></MultiGeometry>",
	    0);

	/* MultiPolygon */
	do_kml_test(
	    "MULTIPOLYGON(((0 1,2 3,4 5,0 1)),((6 7,8 9,10 11,6 7)))",
	    "<MultiGeometry><Polygon><outerBoundaryIs><LinearRing><coordinates>0,1 2,3 4,5 0,1</coordinates></LinearRing></outerBoundaryIs></Polygon><Polygon><outerBoundaryIs><LinearRing><coordinates>6,7 8,9 10,11 6,7</coordinates></LinearRing></outerBoundaryIs></Polygon></MultiGeometry>",
	    0);

	/* GeometryCollection */
	do_kml_unsupported(
	    "GEOMETRYCOLLECTION(POINT(0 1))",
	    "lwgeom_to_kml2: 'GeometryCollection' geometry type not supported");

	/* CircularString */
	do_kml_unsupported(
	    "CIRCULARSTRING(-2 0,0 2,2 0,0 2,2 4)",
	    "lwgeom_to_kml2: 'CircularString' geometry type not supported");

	/* CompoundString */
	do_kml_unsupported(
	    "COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,0 1))",
	    "lwgeom_to_kml2: 'CompoundString' geometry type not supported");

	/* CurvePolygon */
	do_kml_unsupported(
	    "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))",
	    "lwgeom_to_kml2: 'CurvePolygon' geometry type not supported");

	/* MultiCurve */
	do_kml_unsupported(
	    "MULTICURVE((5 5,3 5,3 3,0 3),CIRCULARSTRING(0 0,2 1,2 2))",
	    "lwgeom_to_kml2: 'MultiCurve' geometry type not supported");

	/* GML2 - MultiSurface */
	do_kml_unsupported(
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
	    "lwgeom_to_kml2: 'MultiSurface' geometry type not supported");
}
