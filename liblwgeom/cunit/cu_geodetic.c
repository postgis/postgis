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
		(NULL == CU_add_test(pSuite, "test_gbox_from_spherical_coordinates()", test_gbox_from_spherical_coordinates))  ||
		(NULL == CU_add_test(pSuite, "test_edge_intersection()", test_edge_intersection)) || 
		(NULL == CU_add_test(pSuite, "test_gserialized_get_gbox_geocentric()", test_gserialized_get_gbox_geocentric))  ||
		(NULL == CU_add_test(pSuite, "test_clairaut()", test_clairaut)) 

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

void test_gbox_from_spherical_coordinates(void)
{
    const double gtolerance = 0.000001;
    const int loops = 0;
//    const int loops = 0;
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
    
    for( i = 0; i < loops; i++ )
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

        if( 
            ( fabs( gbox->xmin - gbox_slow->xmin ) > gtolerance ) ||
            ( fabs( gbox->xmax - gbox_slow->xmax ) > gtolerance ) ||
            ( fabs( gbox->ymin - gbox_slow->ymin ) > gtolerance ) ||
            ( fabs( gbox->ymax - gbox_slow->ymax ) > gtolerance ) ||
            ( fabs( gbox->zmin - gbox_slow->zmin ) > gtolerance ) ||
            ( fabs( gbox->zmax - gbox_slow->zmax ) > gtolerance ) )
        {
            printf("\n-------\n");
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

void test_clairaut(void)
{
  
    GEOGRAPHIC_POINT gs, ge;
    POINT3D vs, ve;
    GEOGRAPHIC_POINT g_out, v_out;  

    gs.lat = deg2rad(60.0);
    gs.lon = deg2rad(-45.0);
    ge.lat = deg2rad(60.0);
    ge.lon = deg2rad(135.0);

    geog2cart(gs, &vs);
    geog2cart(ge, &ve);
    
    clairaut_cartesian(vs, ve, LW_TRUE, &v_out);
    clairaut_geographic(gs, ge, LW_TRUE, &g_out);    

	CU_ASSERT_DOUBLE_EQUAL(v_out.lat, g_out.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out.lon, g_out.lon, 0.000001);

    clairaut_cartesian(vs, ve, LW_FALSE, &v_out);
    clairaut_geographic(gs, ge, LW_FALSE, &g_out);    

	CU_ASSERT_DOUBLE_EQUAL(v_out.lat, g_out.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out.lon, g_out.lon, 0.000001);

    gs.lat = 1.3021240033804449;
    ge.lat = 1.3021240033804449;
    gs.lon = -1.3387392931438733;
    ge.lon = 1.80285336044592;

    geog2cart(gs, &vs);
    geog2cart(ge, &ve);

    clairaut_cartesian(vs, ve, LW_TRUE, &v_out);
    clairaut_geographic(gs, ge, LW_TRUE, &g_out);    

	CU_ASSERT_DOUBLE_EQUAL(v_out.lat, g_out.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out.lon, g_out.lon, 0.000001);

    clairaut_cartesian(vs, ve, LW_FALSE, &v_out);
    clairaut_geographic(gs, ge, LW_FALSE, &g_out);    

	CU_ASSERT_DOUBLE_EQUAL(v_out.lat, g_out.lat, 0.000001);
	CU_ASSERT_DOUBLE_EQUAL(v_out.lon, g_out.lon, 0.000001);
}
    

#include "cu_geodetic_data.h"

void test_gserialized_get_gbox_geocentric(void)
{
	LWGEOM *lwg;
	GSERIALIZED *g;
	GBOX *gbox, *gbox_good, *gbox_good_calc;
	int i;
	
	for ( i = 0; i < gbox_data_length; i++ )
	{
//		if( i != 25 && i != 35 ) continue; /* skip our one bad case */
//		printf("\n------------\n");
//		printf("%s\n", gbox_data[2*i]);
//		printf("%s\n", gbox_data[2*i+1]);
		lwg = lwgeom_from_ewkt(gbox_data[2*i], PARSER_CHECK_NONE);
		gbox_good = gbox_from_string(gbox_data[2*i+1]);
		g = gserialized_from_lwgeom(lwg, 1, 0);
		g->flags = FLAGS_SET_GEODETIC(g->flags, 1);
		lwgeom_free(lwg);
		gbox_geocentric_slow = LW_FALSE;
		gbox = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_TRUE;
		gbox_good_calc = gserialized_calculate_gbox_geocentric(g);
		gbox_geocentric_slow = LW_FALSE;
//		printf("\nCALC: %s\n", gbox_to_string(gbox));
//		printf("GOOD: %s\n", gbox_to_string(gbox_good));
//		printf("GOOD CALC: %s\n", gbox_to_string(gbox_good_calc));
//		printf("line %d: diff %.9g\n", i, fabs(gbox->xmin - gbox_good->xmin)+fabs(gbox->ymin - gbox_good->ymin)+fabs(gbox->zmin - gbox_good->zmin));
		CU_ASSERT_DOUBLE_EQUAL(gbox->xmin, gbox_good_calc->xmin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->ymin, gbox_good_calc->ymin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->zmin, gbox_good_calc->zmin, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL_FATAL(gbox->xmax, gbox_good_calc->xmax, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->ymax, gbox_good_calc->ymax, 0.000001);
		CU_ASSERT_DOUBLE_EQUAL(gbox->zmax, gbox_good_calc->zmax, 0.000001);
		lwfree(g);
		lwfree(gbox);
		lwfree(gbox_good);
		lwfree(gbox_good_calc);
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


void test_edge_intersection(void)
{
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT g;
	int rv;
	
	e1.start.lon = -1.0;
	e1.start.lat = 0.0;
	e1.end.lon = 1.0;
	e1.end.lat = 0.0;
	
	e2.start.lon = 0.0;
	e2.start.lat = -1.0;
	e2.end.lon = 0.0;
	e2.end.lat = 1.0;

	edge_deg2rad(&e1);
	edge_deg2rad(&e2);
	
	rv = edge_intersection(e1, e2, &g);
	point_rad2deg(&g);
//	printf("g = (%.9g %.9g)\n", g.lon, g.lat);
//	printf("rv = %d\n", rv);
	CU_ASSERT_DOUBLE_EQUAL(g.lat, 0.0, 0.00001);
	CU_ASSERT_DOUBLE_EQUAL(g.lon, 0.0, 0.00001);
	CU_ASSERT_EQUAL(rv, LW_TRUE);
	
	e2.end.lat = -2.0;
	rv = edge_intersection(e1, e2, &g);
	CU_ASSERT_EQUAL(rv, LW_FALSE);
	
}





