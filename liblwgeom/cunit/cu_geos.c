/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
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

#include "lwgeom_geos.h"
#include "cu_tester.h"

#include "liblwgeom_internal.h"

static void test_geos_noop(void)
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
	};


	for ( i = 0; i < (sizeof ewkt/sizeof(char *)); i++ )
	{
		LWGEOM *geom_in, *geom_out;
		char *in_ewkt;
		char *out_ewkt;

		in_ewkt = ewkt[i];
		geom_in = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		geom_out = lwgeom_geos_noop(geom_in);
		if ( ! geom_out ) {
			fprintf(stderr, "\nNull return from lwgeom_geos_noop with wkt:   %s\n", in_ewkt);
			lwgeom_free(geom_in);
			continue;
		}
		out_ewkt = lwgeom_to_ewkt(geom_out);
		if (strcmp(in_ewkt, out_ewkt))
			fprintf(stderr, "\nExp:   %s\nObt:  %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwfree(out_ewkt);
		lwgeom_free(geom_out);
		lwgeom_free(geom_in);
	}


}

static void test_geos_linemerge(void)
{
	char *ewkt;
	char *out_ewkt;
	LWGEOM *geom1;
	LWGEOM *geom2;

	ewkt = "MULTILINESTRING((0 0, 0 100),(0 -5, 0 0))";
	geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_linemerge(geom1);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom2);
	ASSERT_STRING_EQUAL(out_ewkt, "LINESTRING(0 -5,0 0,0 100)");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	ewkt = "MULTILINESTRING EMPTY";
	geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_linemerge(geom1);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom2);
	ASSERT_STRING_EQUAL(out_ewkt, "GEOMETRYCOLLECTION EMPTY");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
}


static void test_geos_subdivide(void)
{
#if POSTGIS_GEOS_VERSION < 35
	// printf("%d\n", POSTGIS_GEOS_VERSION);
	return;
#else
	char *ewkt = "MULTILINESTRING((0 0, 0 100))";
	char *out_ewkt;
	LWGEOM *geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	LWGEOM *geom2 = lwgeom_segmentize2d(geom1, 1.0);
	
	LWCOLLECTION *geom3 = lwgeom_subdivide(geom2, 80);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom3);
	// printf("\n--------\n%s\n--------\n", out_ewkt);
	CU_ASSERT_EQUAL(2, geom3->ngeoms);
	lwfree(out_ewkt);
	lwcollection_free(geom3);

	geom3 = lwgeom_subdivide(geom2, 20);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom3);
	// printf("\n--------\n%s\n--------\n", out_ewkt);
	CU_ASSERT_EQUAL(8, geom3->ngeoms);
	lwfree(out_ewkt);
	lwcollection_free(geom3);

	lwgeom_free(geom2);
	lwgeom_free(geom1);
#endif
}

static void test_geos_precision(void)
{
#if POSTGIS_GEOS_VERSION < 36
	return;
#else
	LWGEOM* g1 = lwgeom_from_wkt("MULTIPOINT (0 0, 1 1)", LW_PARSER_CHECK_NONE);
	LWGEOM* g2 = lwgeom_from_wkt("MULTIPOINT (1 1, 2 2)", LW_PARSER_CHECK_NONE);

	double tol = 1e2;
	LWGEOM* result = lwgeom_intersection(g1, g2, &tol);

	lwgeom_free(g1);
	lwgeom_free(g2);
	lwgeom_free(result);
#endif
}

/*
** Used by test harness to register the tests in this file.
*/
void geos_suite_setup(void);
void geos_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("GEOS", NULL, NULL);
	PG_ADD_TEST(suite, test_geos_noop);
	PG_ADD_TEST(suite, test_geos_subdivide);
	PG_ADD_TEST(suite, test_geos_linemerge);
	PG_ADD_TEST(suite, test_geos_precision);
}
