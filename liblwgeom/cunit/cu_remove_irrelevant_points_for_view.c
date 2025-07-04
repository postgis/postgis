/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2024 Sam Peters <gluser1357@gmx.de>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "lwgeom_remove_irrelevant_points_for_view.h"

// test case for lwgeom_remove_irrelevant_points_for_view()
static void test_lwgeom_remove_irrelevant_points_for_view(void)
{
	LWGEOM *geom;
	char *wkt, *in_wkt;
	GBOX *bbox;

	bbox = gbox_new(0);
	bbox->xmin = 12;
	bbox->xmax = 18;
	bbox->ymin = 12;
	bbox->ymax = 18;

	geom = lwgeom_from_wkt("POLYGON((0 30, 15 30, 30 30, 30 0, 0 0, 0 30))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);

	lwgeom_remove_irrelevant_points_for_view(geom, bbox, true);
	wkt = lwgeom_to_ewkt(geom);
	in_wkt = "POLYGON((15 30,30 0,0 0,15 30))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);

	lwfree(wkt);
	lwgeom_free(geom);
	lwfree(bbox);
}

void remove_irrelevant_points_for_view_suite_setup(void);
void
remove_irrelevant_points_for_view_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("remove_irrelevant_points_for_view", NULL, NULL);
	PG_ADD_TEST(suite, test_lwgeom_remove_irrelevant_points_for_view);
}
