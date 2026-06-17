/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"

#include "liblwgeom_internal.h"
#include "topo/liblwgeom_topo.h"
#include "cu_tester.h"

static LWGEOM *
lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if (LW_FAILURE == lwgeom_parse_wkt(&r, (char *)str, LW_PARSER_CHECK_NONE))
		return NULL;
	return r.geom;
}

static void
test_lwt_IsTopoRingCCW_large_finite_coordinates(void)
{
	LWGEOM *geom = lwgeom_from_text("POLYGON((0 1e308,1e308 0,0 0,0 1e308))");
	LWPOLY *poly;

	CU_ASSERT_PTR_NOT_NULL(geom);
	if (!geom)
		return;

	poly = lwgeom_as_lwpoly(geom);
	CU_ASSERT_PTR_NOT_NULL(poly);
	if (!poly)
	{
		lwgeom_free(geom);
		return;
	}

	CU_ASSERT_EQUAL(lwt_IsTopoRingCCW(poly->rings[0]), LW_FALSE);

	lwgeom_free(geom);
}

void
topo_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("topology", NULL, NULL);
	PG_ADD_TEST(suite, test_lwt_IsTopoRingCCW_large_finite_coordinates);
}
