/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2021 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2021 Aliaksandr Kalinik <kalenik.aliaksandr@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

#define BOUNDARY_TEST(wkt_in, wkt_exp) \
	do \
	{ \
		LWGEOM *gin, *gout, *gexp; \
		cu_error_msg_reset(); \
		gin = lwgeom_from_wkt(wkt_in, LW_PARSER_CHECK_NONE); \
		CU_ASSERT_PTR_NOT_NULL_FATAL(gin); \
		gexp = lwgeom_from_wkt(wkt_exp, LW_PARSER_CHECK_NONE); \
		CU_ASSERT_PTR_NOT_NULL_FATAL(gexp); \
		gout = lwgeom_boundary(gin); \
		CU_ASSERT_PTR_NOT_NULL_FATAL(gout); \
		ASSERT_NORMALIZED_GEOM_SAME(gout, gexp); \
		lwgeom_free(gout); \
		lwgeom_free(gexp); \
		lwgeom_free(gin); \
	} while (0)

static void
boundary_point(void)
{
	BOUNDARY_TEST(
		"POINT EMPTY",
		"POINT EMPTY"
	);

	BOUNDARY_TEST(
		"POINT(0 0)",
		"POINT EMPTY"
	);

	BOUNDARY_TEST(
		"MULTIPOINT EMPTY",
		"MULTIPOINT EMPTY" /* debatable return type */
	);

	BOUNDARY_TEST(
		"MULTIPOINT(0 0,10 0)",
		"MULTIPOINT EMPTY" /* debatable return type */
	);
}

static void
boundary_line(void)
{
	BOUNDARY_TEST(
		"LINESTRING EMPTY",
		"MULTIPOINT EMPTY"
	);

	BOUNDARY_TEST(
		"CIRCULARSTRING EMPTY",
		"MULTIPOINT EMPTY"
	);

	BOUNDARY_TEST(
		"LINESTRING(0 0, 5 5, 10 0, 0 0)",
		"MULTIPOINT EMPTY"
	);

	BOUNDARY_TEST(
		"CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0)",
		"MULTIPOINT EMPTY"
	);

	BOUNDARY_TEST(
		"LINESTRING(0 0, 5 0, 10 0)",
		"MULTIPOINT(0 0, 10 0)"
	);

	BOUNDARY_TEST(
		"MULTILINESTRING((0 0, 5 0, 10 0),(20 5, 20 6, 20 10))",
		"MULTIPOINT(0 0, 10 0, 20 5, 20 10)"
	);

	BOUNDARY_TEST(
		"MULTICURVE((1 1 1,0 0 0.5, -1 1 1),(1 1 0.5,0 0 0.5, -1 1 0.5, 1 1 0.5))",
		"MULTIPOINT(1 1 1,-1 1 1)"
	);

}

static void
boundary_polygon(void)
{
	BOUNDARY_TEST(
		"POLYGON EMPTY",
		"MULTILINESTRING EMPTY"
	);

	BOUNDARY_TEST(
		"POLYGON((0 0, 5 5, 10 0, 0 0))",
		"LINESTRING(0 0, 5 5, 10 0, 0 0)"
	);

	BOUNDARY_TEST(
		"POLYGON((0 0, 5 5, 10 0, 0 0),(3 1,7 1,5 2,3 1))",
		"MULTILINESTRING((0 0, 5 5, 10 0, 0 0),(3 1,7 1,5 2,3 1))"
	);

	/* See https://trac.osgeo.org/postgis/ticket/4961 */
	BOUNDARY_TEST(
		"MULTIPOLYGON(((0 0, 5 5, 10 0, 0 0),(3 1,7 1,5 2,3 1)),((20 0,20 5,25 5,20 0)))",
		"MULTILINESTRING((0 0, 5 5, 10 0, 0 0),(3 1,7 1,5 2,3 1),(20 0,20 5,25 5,20 0))"
	);

	BOUNDARY_TEST(
		"CURVEPOLYGON(CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0))",
		"MULTICURVE(CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0))"
	);

	BOUNDARY_TEST(
		"CURVEPOLYGON(CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0),(1 1, 3 3, 3 1, 1 1))",
		"MULTICURVE(CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0),(1 1, 3 3, 3 1, 1 1))"
	);

}

static void
boundary_triangle(void)
{
	BOUNDARY_TEST(
		"TRIANGLE EMPTY",
		"LINESTRING EMPTY"
	);

	BOUNDARY_TEST(
		"TRIANGLE((1 1, 0 0, -1 1, 1 1))",
		"LINESTRING(1 1, 0 0, -1 1, 1 1)"
	);
}

static void
boundary_tin(void)
{
	BOUNDARY_TEST(
		"TIN EMPTY",
		"GEOMETRYCOLLECTION EMPTY"
	);

	BOUNDARY_TEST(
		"TIN(((0 0,0 -1,-1 1,0 0)))",
		"LINESTRING(0 0,0 -1,-1 1,0 0)"
	);

	BOUNDARY_TEST(
		"TIN(((0 0,0 -1,-1 1,0 0)),((0 0,1 0,0 -1,0 0)))",
		"MULTILINESTRING((0 0,0 -1,-1 1,0 0),(0 0,1 0,0 -1,0 0))"
	);
}

static void
boundary_collection(void)
{
	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION EMPTY",
		"GEOMETRYCOLLECTION EMPTY"
	);

	/* See https://trac.osgeo.org/postgis/ticket/4956 */
	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION(POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2)),POLYGON((2 2,2 4,4 4,4 2,2 2)))",
		"MULTILINESTRING((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2),(2 2,2 4,4 4,4 2,2 2))"
	);

	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION(TIN(((0 0,0 -1,-1 1,0 0)),((0 0,1 0,0 -1,0 0))),TIN(((10 10,10 20,20 20,10 10))))",
		"MULTILINESTRING((0 0,0 -1,-1 1,0 0),(0 0,1 0,0 -1,0 0),(10 10,10 20,20 20,10 10))"
	);

	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION(TRIANGLE((1 1, 0 0, -1 1, 1 1)))",
		"LINESTRING(1 1, 0 0, -1 1, 1 1)"
	);

	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(TRIANGLE((0 0,0 -1,-1 1,0 0)),TRIANGLE((0 0,1 0,0 -1,0 0))),MULTILINESTRING((0 0, 5 0, 10 0),(20 5, 20 6, 20 10)))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((0 0,0 -1,1 0,0 0),(0 0,-1 1,0 -1,0 0)),MULTIPOINT(20 10,20 5,10 0,0 0))"
	);

	BOUNDARY_TEST(
		"GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)),POINT(1 1),MULTIPOINT(2 2,3 3))",
		"GEOMETRYCOLLECTION EMPTY"
	);
}

void boundary_suite_setup(void);
void
boundary_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("boundary", NULL, NULL);
	PG_ADD_TEST(suite, boundary_point);
	PG_ADD_TEST(suite, boundary_line);
	PG_ADD_TEST(suite, boundary_polygon);
	PG_ADD_TEST(suite, boundary_triangle);
	PG_ADD_TEST(suite, boundary_tin);
	PG_ADD_TEST(suite, boundary_collection);
}
