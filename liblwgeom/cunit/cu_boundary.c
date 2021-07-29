/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2021 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

#define BOUNDARY_TEST(wkt_in, wkt_exp) do {\
	LWGEOM *gin, *gout, *gexp; \
	cu_error_msg_reset(); \
	gin = lwgeom_from_wkt( wkt_in, LW_PARSER_CHECK_NONE);  \
	CU_ASSERT(gin != NULL);  \
	gexp = lwgeom_from_wkt( wkt_exp, LW_PARSER_CHECK_NONE);  \
	CU_ASSERT(gexp != NULL);  \
	gout = lwgeom_boundary(gin);  \
	CU_ASSERT(gout != NULL);  \
	ASSERT_NORMALIZED_GEOM_SAME(gout, gexp);  \
	lwgeom_free(gout);  \
	lwgeom_free(gexp);  \
	lwgeom_free(gin);  \
} while (0)

static void boundary_point(void)
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

static void boundary_line(void)
{
	BOUNDARY_TEST(
		"LINESTRING EMPTY",
		"MULTIPOINT EMPTY"
	);

}

static void boundary_collection(void)
{
	/* See https://trac.osgeo.org/postgis/ticket/4956 */
	BOUNDARY_TEST(
"GEOMETRYCOLLECTION(POLYGON((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2)),POLYGON((2 2,2 4,4 4,4 2,2 2)))",
"MULTILINESTRING((0 0,0 10,10 10,10 0,0 0),(2 2,4 2,4 4,2 4,2 2),(2 2,2 4,4 4,4 2,2 2))"
	);
}


void boundary_suite_setup(void);
void boundary_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("boundary", NULL, NULL);
	PG_ADD_TEST(suite,boundary_point);
	PG_ADD_TEST(suite,boundary_line);
#if 0 /* write these */
	PG_ADD_TEST(suite,boundary_polygon);
	PG_ADD_TEST(suite,boundary_tin);
#endif
	//PG_ADD_TEST(suite,boundary_collection);
}
