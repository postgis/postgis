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

#include "cu_measures.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_measures_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("PostGIS Measures Suite", init_measures_suite, clean_measures_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_mindistance2d_recursive_tolerance()", test_mindistance2d_recursive_tolerance)) 
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
int init_measures_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_measures_suite(void)
{
	return 0;
}

void test_mindistance2d_recursive_tolerance(void)
{
	LWGEOM_PARSER_RESULT gp1;
	LWGEOM_PARSER_RESULT gp2;
	double distance;
	int result1, result2;

	/*
	** Simple case.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "MULTIPOINT(0 1.5,0 2,0 2.5)", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("\ndistance #1 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 1.5);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(POINT(3 4))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #2 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #3 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4))))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #4 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection Collection Multipoint.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #5 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection vs Geometry Collection
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(POINT(0 0))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(POINT(3 4))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #6 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection Collection vs Geometry Collection Collection
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #7 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection Collection Multipoint vs Geometry Collection Collection Multipoint
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(0 0)))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_recursive_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #8 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

}



