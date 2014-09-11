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

#include "../../postgis_config.h"

static void test_lwgeom_make_valid(void)
{
#if POSTGIS_GEOS_VERSION >= 33
	LWGEOM *gin, *gout;
	char *wkt;

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
	 *       to the exception:
	 * See http://trac.osgeo.org/postgis/ticket/1735
	 */

	lwgeom_free(gout);
	lwgeom_free(gin);

  /* Test for http://trac.osgeo.org/postgis/ticket/2307 */

  gin = lwgeom_from_hexwkb("0106000020E6100000010000000103000000010000000A0000004B7DA956B99844C0DB0790FE8B4D1DC010BA74A9AF9444C049AFFC5B8C4D1DC03FC6CC690D9844C0DD67E5628C4D1DC07117B56B0D9844C0C80ABA67C45E1DC0839166ABAF9444C0387D4568C45E1DC010BA74A9AF9444C049AFFC5B8C4D1DC040C3CD74169444C0362EC0608C4D1DC07C1A3B77169444C0DC3ADB40B2641DC03AAE5F68B99844C0242948DEB1641DC04B7DA956B99844C0DB0790FE8B4D1DC0", LW_PARSER_CHECK_NONE);
	CU_ASSERT(gin != NULL);

	gout = lwgeom_make_valid(gin);
	CU_ASSERT(gout != NULL);
	lwgeom_free(gin);

	/* We're really only interested in avoiding memory problems.
	 * Convertion to ewkt ensures full scan of coordinates thus
	 * triggering the error, if any
	 */
	wkt = lwgeom_to_ewkt(gout);
	lwgeom_free(gout);
	lwfree(wkt);

	/* Test multipoint */

	/* Test for trac.osgeo.org/postgis/ticket/1953 */
	gin = lwgeom_from_wkt(
		"POLYGON((1 1, 2 1, 2 2, 1 2))",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(gin != NULL);

	gout = lwgeom_make_valid(gin);

	wkt = lwgeom_to_wkt(gout, WKT_ISO, 8, NULL);
	CU_ASSERT(!strcmp(wkt, "POLYGON((1 1,2 1,2 2,1 2,1 1))"));
	lwfree(wkt);

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
