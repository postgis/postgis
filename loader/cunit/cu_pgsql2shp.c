/**********************************************************************
 * $Id: cu_pgsql2shp.c 5674 2010-06-03 02:04:15Z mleslie $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 LISAsoft Pty Ltd
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_pgsql2shp.h"
#include "cu_tester.h"
#include "../pgsql2shp-core.h"

/* Test functions */
void test_ShpDumperCreate(void);
void test_ShpDumperDestroy(void);

SHPDUMPERCONFIG *config;
SHPDUMPERSTATE *state;

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_pgsql2shp_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("Shapefile Loader File pgsql2shp Test", init_pgsql2shp_suite, clean_pgsql2shp_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_ShpDumperCreate()", test_ShpDumperCreate)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpDumperDestroy()", test_ShpDumperDestroy))
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
int init_pgsql2shp_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_pgsql2shp_suite(void)
{
	return 0;
}

void test_ShpDumperCreate(void)
{	
	config = (SHPDUMPERCONFIG*)calloc(1, sizeof(SHPDUMPERCONFIG));
	state = ShpDumperCreate(config);
	CU_ASSERT_PTR_NOT_NULL(state);
	CU_ASSERT_EQUAL(state->outtype, 's');
}

void test_ShpDumperDestroy(void)
{
	ShpDumperDestroy(state);
}
