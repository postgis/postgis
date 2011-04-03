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
void test_ShpDumperGeoMapRead(void);
void test_ShpDumperFieldnameLimit(void);

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
	    (NULL == CU_add_test(pSuite, "test_ShpDumperGeoMapRead()", test_ShpDumperGeoMapRead)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpDumperFieldnameLimit()", test_ShpDumperFieldnameLimit))
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
	SHPDUMPERCONFIG *config;
	SHPDUMPERSTATE *state;

	config = (SHPDUMPERCONFIG*)calloc(1, sizeof(SHPDUMPERCONFIG));
	state = ShpDumperCreate(config);
	CU_ASSERT_PTR_NOT_NULL(state);
	CU_ASSERT_EQUAL(state->outtype, 's');
	free(state);
	free(config);
}

void ShpDumperGeoMapRead(char* filename, SHPDUMPERSTATE *state);

void test_ShpDumperGeoMapRead(void)
{
	SHPDUMPERSTATE *state;

	state = malloc(sizeof(SHPDUMPERSTATE));
	ShpDumperGeoMapRead("map.txt", state);
	CU_ASSERT_PTR_NOT_NULL(state->geo_map);
	CU_ASSERT_EQUAL(state->geo_map_size, 3);
	CU_ASSERT_STRING_EQUAL(state->geo_map[0], "0123456789toolong0");
	CU_ASSERT_STRING_EQUAL(state->geo_map[0] + strlen(state->geo_map[0]) + 1, "0123456780");
	CU_ASSERT_STRING_EQUAL(state->geo_map[1], "0123456789toolong1");
	CU_ASSERT_STRING_EQUAL(state->geo_map[1] + strlen(state->geo_map[1]) + 1, "0123456781");
	CU_ASSERT_STRING_EQUAL(state->geo_map[2], "0123456789toolong2");
	CU_ASSERT_STRING_EQUAL(state->geo_map[2] + strlen(state->geo_map[2]) + 1, "0123456782");
	free(*state->geo_map);
	free(state->geo_map);
	free(state);
}

char* ShpDumperFieldnameLimit(char* ptr, SHPDUMPERSTATE *state);

void test_ShpDumperFieldnameLimit(void)
{
	SHPDUMPERSTATE *state;

	state = malloc(sizeof(SHPDUMPERSTATE));
	ShpDumperGeoMapRead("map.txt", state);
	{
		char* from = "UNTOUCHED";
		char* to = ShpDumperFieldnameLimit(from, state);
		CU_ASSERT_STRING_EQUAL(from, to);
		free(to);
	}
	{
		char* from = "0123456789toolong1";
		char* to = ShpDumperFieldnameLimit(from, state);
		CU_ASSERT_STRING_EQUAL(to, "0123456781");
		free(to);
	}
	free(*state->geo_map);
	free(state->geo_map);
	free(state);
}
