/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2010 LISAsoft Pty Ltd
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_shp2pgsql.h"
#include "cu_tester.h"
#include "../shp2pgsql-core.h"

#include <string.h>

/* Test functions */
void test_ShpLoaderCreate(void);
void test_ShpLoaderDestroy(void);
void test_ShpLoaderFIDColumnSQL(void);

SHPLOADERCONFIG *loader_config;
SHPLOADERSTATE *loader_state;

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_shp2pgsql_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("Shapefile Loader File shp2pgsql Test", init_shp2pgsql_suite, clean_shp2pgsql_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if ((NULL == CU_add_test(pSuite, "test_ShpLoaderCreate()", test_ShpLoaderCreate)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderDestroy()", test_ShpLoaderDestroy)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderFIDColumnSQL()", test_ShpLoaderFIDColumnSQL)))
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** The suite initialization function.
** Create any reused objects.
*/
int init_shp2pgsql_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_shp2pgsql_suite(void)
{
	return 0;
}

void test_ShpLoaderCreate(void)
{
	loader_config = (SHPLOADERCONFIG*)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_state = ShpLoaderCreate(loader_config);
	CU_ASSERT_PTR_NOT_NULL(loader_state);
	CU_ASSERT_STRING_EQUAL(loader_state->config->encoding, ENCODING_DEFAULT);
}

void test_ShpLoaderDestroy(void)
{
	ShpLoaderDestroy(loader_state);
}

void
test_ShpLoaderFIDColumnSQL(void)
{
	SHPLOADERCONFIG config;
	SHPLOADERSTATE *state;
	char *header = NULL;

	set_loader_config_defaults(&config);
	config.table = "loadedshp";
	config.readshape = 0;

	state = ShpLoaderCreate(&config);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE \"loadedshp\" (\"gid\" serial"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "ADD PRIMARY KEY (\"gid\")"));
	free(header);
	ShpLoaderDestroy(state);

	config.fid_col = "fid";
	state = ShpLoaderCreate(&config);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE \"loadedshp\" (\"fid\" serial"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "ADD PRIMARY KEY (\"fid\")"));
	free(header);
	ShpLoaderDestroy(state);

	free(config.encoding);
}
