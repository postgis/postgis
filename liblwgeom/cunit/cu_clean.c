/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2012 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

static void test_lwgeom_make_valid(void)
{
#if POSTGIS_GEOS_VERSION >= 33
	LWGEOM *gin, *gout;
	char *ewkt;

	/* Because i don't trust that much prior tests...  ;) */
	cu_error_msg_reset();

	gin = lwgeom_from_wkt(
"MULTIPOLYGON(((1725063 4819121, 1725104 4819067, 1725060 4819087, 1725064.14183882 4819094.70208557,1725064.13656044 4819094.70235069,1725064.14210359 4819094.70227252,1725064.14210362 4819094.70227252,1725064.13656043 4819094.70235069,1725055. 4819094, 1725055 4819094, 1725055 4819094, 1725063 4819121)))",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(gin != NULL);

	gout = lwgeom_make_valid(gin);

	/* We're really only interested in avoiding a crash in here.
	 * See http://trac.osgeo.org/postgis/ticket/1738
	 * TODO: enhance the test if we find a workaround 
	 *       to the excepion:
	 * See http://trac.osgeo.org/postgis/ticket/1735
	 */

	lwgeom_free(gout);
	lwgeom_free(gin);

	/* Test collection */

	gin = lwgeom_from_wkt(
"GEOMETRYCOLLECTION(LINESTRING(0 0, 0 0), POLYGON((0 0, 10 10, 10 0, 0 10, 0 0)), LINESTRING(10 0, 10 10))",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(gin != NULL);

	gout = lwgeom_make_valid(gin);
	CU_ASSERT(gout != NULL);

	ewkt = lwgeom_to_ewkt(gout);
	/* printf("c = %s\n", ewkt); */
	CU_ASSERT_STRING_EQUAL(ewkt,
"GEOMETRYCOLLECTION(POINT(0 0),MULTIPOLYGON(((5 5,0 0,0 10,5 5)),((5 5,10 10,10 0,5 5))),LINESTRING(10 0,10 10))");
	lwfree(ewkt);

	lwgeom_free(gout);
	lwgeom_free(gin);

	/* Test multipoint */

	gin = lwgeom_from_wkt(
"MULTIPOINT(0 0,1 1,2 2)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(gin != NULL);

	gout = lwgeom_make_valid(gin);
	CU_ASSERT(gout != NULL);

	ewkt = lwgeom_to_ewkt(gout);
	/* printf("c = %s\n", ewkt); */
	CU_ASSERT_STRING_EQUAL(ewkt,
"MULTIPOINT(0 0,1 1,2 2)");
	lwfree(ewkt);

	lwgeom_free(gout);
	lwgeom_free(gin);

#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

/* TODO: add more tests ! */


/*
** Used by test harness to register the tests in this file.
*/
static CU_TestInfo clean_tests[] =
{
	PG_TEST(test_lwgeom_make_valid),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo clean_suite = {"clean",  NULL,  NULL, clean_tests};
