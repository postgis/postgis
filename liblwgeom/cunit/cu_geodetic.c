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

#include "cu_geodetic.h"

#define RANDOM_TEST 0

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_geodetic_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("Geodetic Suite", init_geodetic_suite, clean_geodetic_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_signum()", test_signum))  ||
#if RANDOM_TEST
	    (NULL == CU_add_test(pSuite, "test_gbox_from_spherical_coordinates()", test_gbox_from_spherical_coordinates))  ||
#endif
	    (NULL == CU_add_test(pSuite, "test_gserialized_get_gbox_geocentric()", test_gserialized_get_gbox_geocentric))  ||
	    (NULL == CU_add_test(pSuite, "test_clairaut()", test_clairaut))  || 
	    (NULL == CU_add_test(pSuite, "test_edge_intersection()", test_edge_intersection)) 

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
int init_geodetic_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_geodetic_suite(void)
{
	return 0;
}


void test_signum(void)
{
	CU_ASSERT_EQUAL(signum(-5.0),-1);
	CU_ASSERT_EQUAL(signum(5.0),1);
}

#if RANDOM_TEST

void test_gbox_from_spherical_coordinates(void)
{
	const double gtolerance = 0.000001;
	const int loops = 5;
	int i;
	double ll[64];
	GBOX *gbox;
	GBOX *gbox_slow;
	int rndlat;
	int rndlon;

	POINTARRAY *pa;
	LWLINE *lwline;
	GSERIALIZED *g;

	ll[0] = -3.083333333333333333333333333333333;
	ll[1] = 9.83333333333333333333333333333333;
	ll[2] = 15.5;
	ll[3] = -5.25;

	pa = pointArray_construct((uchar*)ll, 0, 0, 2);
	lwline = lwline_construct(-1, 0, pa);

	srandomdev();

	for ( i = 0; i < loops; i++ )
	{
		rndlat = (int)(90.0 - 180.0 * (double)random() / pow(2.0, 31.0));
		rndlon = (int)(180.0 - 360.0 * (double)random() / pow(2.0, 31.0));
		ll[0] = (double)rndlon;
		ll[1] = (double)rndlat;

		rndlat = (int)(90.0 - 180.0 * (double)random() / pow(2.0, 31.0));
		rndlon = (int)(180.0 - 360.0 * (double)random() / pow(2.0, 31.0));
		ll[2] = (double)rndlon;
		ll[3] = (double)rndlat;

		g = gserialized_from_lwgeom((LWGEOM*)lwline, 1, 0);
		g->flags = FLAGS_SET_GEODETIC(g->flags, 1);
		gbox_geocentric_slow = LW_FALSE;
		gbox = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_TRUE;
		gbox_slow = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_FALSE;
		lwfree(g);

		if (
		    ( fabs( gbox->xmin - gbox_slow->xmin ) > gtolerance ) ||
		    ( fabs( gbox->xmax - gbox_slow->xmax ) > gtolerance ) ||
		    ( fabs( gbox->ymin - gbox_slow->ymin ) > gtolerance ) ||
		    ( fabs( gbox->ymax - gbox_slow->ymax ) > gtolerance ) ||
		    ( fabs( gbox->zmin - gbox_slow->zmin ) > gtolerance ) ||
		    ( fabs( gbox->zmax - gbox_slow->zmax ) > gtolerance ) )
		{
			printf("\n-------\n");
			printf("If you are seeing this, cut and paste it, it is a randomly generated test case!\n");
			printf("LOOP: %d\n", i);
			printf("SEGMENT (Lon Lat): (%.9g %.9g) (%.9g %.9g)\n", ll[0], ll[1], ll[2], ll[3]);
			printf("CALC: %s\n", gbox_to_string(gbox));
			printf("SLOW: %s\n", gbox_to_string(gbox_slow));
			printf("-------\n\n");
			CU_FAIL_FATAL(Slow (GOOD) and fast (CALC) box calculations returned different values!!);
		}

		lwfree(gbox);
		lwfree(gbox_slow);
	}

	lwfree(lwline);
	lwfree(pa);

}

#endif /* RANDOM_TEST */

void test_clairaut(void)
{

	GEOGRAPHIC_POINT gs, ge;
	POINT3D vs, ve;
	GEOGRAPHIC_POINT g_out_top, g_out_bottom, v_out_top, v_out_bottom;

	gs.lat = deg2rad(60.0);
	gs.lon = deg2rad(-45.0);
	ge.lat = deg2rad(60.0);
	ge.lon = deg2rad(135.0);

	geog2cart(gs, &vs);
	geog2cart(ge, &ve);

	clairaut_cartesian(vs, ve, &v_out_top, &v_out_bottom);
	clairaut_geographic(gs, ge, &g_out_top, &g_out_bottom);

	CU_ASSERT_DOUBLE_EQUAL(v_out_top.lat, g_out_top.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_top.lon, g_out_top.lon, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_bottom.lat, g_out_bottom.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_bottom.lon, g_out_bottom.lon, 0.000001);

	gs.lat = 1.3021240033804449;
	ge.lat = 1.3021240033804449;
	gs.lon = -1.3387392931438733;
	ge.lon = 1.80285336044592;

	geog2cart(gs, &vs);
	geog2cart(ge, &ve);

	clairaut_cartesian(vs, ve, &v_out_top, &v_out_bottom);
	clairaut_geographic(gs, ge, &g_out_top, &g_out_bottom);

	CU_ASSERT_DOUBLE_EQUAL(v_out_top.lat, g_out_top.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_top.lon, g_out_top.lon, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_bottom.lat, g_out_bottom.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out_bottom.lon, g_out_bottom.lon, 0.000001);
}


#include "cu_geodetic_data.h"

void test_gserialized_get_gbox_geocentric(void)
{
	LWGEOM *lwg;
	GSERIALIZED *g;
	GBOX *gbox, *gbox_slow;
	int i;

	for ( i = 0; i < gbox_data_length; i++ )
	{
#if 0
//		if ( i != 0 ) continue; /* skip our bad case */
		printf("\n\n------------\n");
		printf("%s\n", gbox_data[i]);
#endif
		lwg = lwgeom_from_ewkt(gbox_data[i], PARSER_CHECK_NONE);
		g = gserialized_from_lwgeom(lwg, 1, 0);
		g->flags = FLAGS_SET_GEODETIC(g->flags, 1);
		lwgeom_free(lwg);
		gbox_geocentric_slow = LW_FALSE;
		gbox = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_TRUE;
		gbox_slow = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_FALSE;
#if 0
		printf("\nCALC: %s\n", gbox_to_string(gbox));
		printf("GOOD: %s\n", gbox_to_string(gbox_slow));
		printf("line %d: diff %.9g\n", i, fabs(gbox->xmin - gbox_slow->xmin)+fabs(gbox->ymin - gbox_slow->ymin)+fabs(gbox->zmin - gbox_slow->zmin));
		printf("------------\n");
#endif
		CU_ASSERT_DOUBLE_EQUAL(gbox->xmin, gbox_slow->xmin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->ymin, gbox_slow->ymin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->zmin, gbox_slow->zmin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->xmax, gbox_slow->xmax, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->ymax, gbox_slow->ymax, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->zmax, gbox_slow->zmax, 0.000001);
		lwfree(g);
		lwfree(gbox);
		lwfree(gbox_slow);
	}

}

/*
* Build LWGEOM on top of *aligned* structure so we can use the read-only
* point access methods on them.
static LWGEOM* lwgeom_over_gserialized(char *wkt)
{
	LWGEOM *lwg;
	GSERIALIZED *g;

	lwg = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(lwg, 1, 0);
	lwgeom_free(lwg);
	return lwgeom_from_gserialized(g);
}
*/

static void edge_set(double lon1, double lat1, double lon2, double lat2, GEOGRAPHIC_EDGE *e)
{
	e->start.lon = lon1;
	e->start.lat = lat1;
	e->end.lon = lon2;
	e->end.lat = lat2;
	edge_deg2rad(e);
}

void test_edge_intersection(void)
{
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT g;
	int rv;

	edge_set(-1.0, 0.0, 1.0, 0.0, &e1);
	edge_set(0.0, -1.0, 0.0, 1.0, &e2);

	/* Intersection at (0 0) */
	edge_set(-1.0, 0.0, 1.0, 0.0, &e1);
	edge_set(0.0, -1.0, 0.0, 1.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 0.0, 0.00001);
	CU_ASSERT_DOUBLE_EQUAL(g.lon, 0.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE);

	/*  No intersection at (0 0) */
	edge_set(0.0, -1.0, 0.0, -2.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	CU_ASSERT_EQUAL(rv, LW_FALSE);

	/*  End touches middle of segment at (0 0) */
	edge_set(0.0, -1.0, 0.0, 0.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
#if 0
	printf("\n");
	printf("LINESTRING(%.8g %.8g, %.8g %.8g)\n", e1.start.lon,  e1.start.lat, e1.end.lon,  e1.end.lat);
	printf("LINESTRING(%.8g %.8g, %.8g %.8g)\n", e2.start.lon,  e2.start.lat, e2.end.lon,  e2.end.lat);
	printf("g = (%.9g %.9g)\n", g.lon, g.lat);
	printf("rv = %d\n", rv);
#endif
	CU_ASSERT_DOUBLE_EQUAL(g.lon, 0.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE);

	/*  End touches end of segment at (0 0) */
	edge_set(0.0, 0.0, 1.0, 0.0, &e1);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
#if 0
	printf("\n");
	printf("LINESTRING(%.8g %.8g, %.8g %.8g)\n", e1.start.lon,  e1.start.lat, e1.end.lon,  e1.end.lat);
	printf("LINESTRING(%.8g %.8g, %.8g %.8g)\n", e2.start.lon,  e2.start.lat, e2.end.lon,  e2.end.lat);
	printf("g = (%.9g %.9g)\n", g.lon, g.lat);
	printf("rv = %d\n", rv);
#endif
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 0.0, 0.00001);
	CU_ASSERT_DOUBLE_EQUAL(g.lon, 0.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE);

	/* Intersection at (180 0) */
	edge_set(-179.0, -1.0, 179.0, 1.0, &e1);
	edge_set(-179.0, 1.0, 179.0, -1.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 0.0, 0.00001);
	CU_ASSERT_DOUBLE_EQUAL(fabs(g.lon), 180.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 

	/* Intersection at (180 0) */
	edge_set(-170.0, 0.0, 170.0, 0.0, &e1);
	edge_set(180.0, -10.0, 180.0, 10.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 0.0, 0.00001);
	CU_ASSERT_DOUBLE_EQUAL(fabs(g.lon), 180.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 
	
	/* Intersection at north pole */
	edge_set(-180.0, 80.0, 0.0, 80.0, &e1);
	edge_set(90.0, 80.0, -90.0, 80.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 90.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 

	/* End touches arc at north pole */
	edge_set(-180.0, 80.0, 0.0, 80.0, &e1);
	edge_set(90.0, 80.0, -90.0, 90.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 90.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 	

	/* End touches end at north pole */
	edge_set(-180.0, 80.0, 0.0, 90.0, &e1);
	edge_set(90.0, 80.0, -90.0, 90.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 90.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 	

	/* Equal edges return true */
	edge_set(45.0, 10.0, 50.0, 20.0, &e1);
	edge_set(45.0, 10.0, 50.0, 20.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 	

	/* Parallel edges (same great circle, different end points) return true  */
	edge_set(40.0, 0.0, 70.0, 0.0, &e1);
	edge_set(60.0, 0.0, 50.0, 0.0, &e2);
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
	CU_ASSERT_EQUAL(rv, LW_TRUE); 	

}





