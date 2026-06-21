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
void test_ShpLoaderGetSQLHeader_drop_prepare(void);
void test_ShpLoaderGetSQLHeader_if_not_exists_table_modifier(void);
void test_ShpLoaderGetSQLFooter_if_not_exists_index_modifier(void);
void test_ShpLoaderFIDColumnSQL(void);
void test_ShpLoaderRejectsInvalidFIDColumn(void);
void test_ShpLoaderOpenShapeRejectsZip(void);

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
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderGetSQLHeader_drop_prepare()",
				 test_ShpLoaderGetSQLHeader_drop_prepare)) ||
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderGetSQLHeader_if_not_exists_table_modifier()",
				 test_ShpLoaderGetSQLHeader_if_not_exists_table_modifier)) ||
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderGetSQLFooter_if_not_exists_index_modifier()",
				 test_ShpLoaderGetSQLFooter_if_not_exists_index_modifier)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderFIDColumnSQL()", test_ShpLoaderFIDColumnSQL)) ||
	    (NULL ==
	     CU_add_test(pSuite, "test_ShpLoaderRejectsInvalidFIDColumn()", test_ShpLoaderRejectsInvalidFIDColumn)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderOpenShapeRejectsZip()", test_ShpLoaderOpenShapeRejectsZip)))
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
test_ShpLoaderGetSQLHeader_drop_prepare(void)
{
	char *header = NULL;
	const char *drop;
	const char *create;

	loader_config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_config->actions.mode = 'p';
	loader_config->actions.drop_table = 1;
	/* This header-only test does not open a shapefile, so avoid geometry metadata. */
	loader_config->readshape = 0;
	loader_config->table = "loadedshp";
	loader_config->geo_col = "the_geom";

	loader_state = ShpLoaderCreate(loader_config);
	loader_state->pgtype = "POINT";
	loader_state->pgdims = 2;
	loader_state->to_srid = 0;
	loader_state->num_fields = 0;

	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(loader_state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(header);

	drop = strstr(header, "DROP TABLE IF EXISTS \"loadedshp\";");
	create = strstr(header, "CREATE TABLE \"loadedshp\"");
	CU_ASSERT_PTR_NOT_NULL(drop);
	CU_ASSERT_PTR_NOT_NULL(create);
	CU_ASSERT_PTR_NULL(strstr(header, "DropGeometryColumn"));
	CU_ASSERT(drop < create);

	free(header);
	ShpLoaderDestroy(loader_state);
}

void
test_ShpLoaderGetSQLHeader_if_not_exists_table_modifier(void)
{
	char *header = NULL;

	loader_config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_config->actions.mode = 'p';
	loader_config->actions.create_table = LOADER_CREATE_IF_NOT_EXISTS;
	loader_config->actions.create_table_set = 1;
	loader_config->table = "loadedshp";
	loader_config->geo_col = "the_geom";

	loader_state = ShpLoaderCreate(loader_config);
	loader_state->pgtype = "POINT";
	loader_state->pgdims = 2;
	loader_state->to_srid = 0;
	loader_state->num_fields = 0;

	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(loader_state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(header);
	CU_ASSERT_PTR_NOT_NULL(strstr(header,
				      "CREATE TABLE IF NOT EXISTS \"loadedshp\" "
				      "(\"gid\" serial PRIMARY KEY,\n"
				      "\"the_geom\" geometry(POINT,0));"));
	CU_ASSERT_PTR_NULL(strstr(header, "AddGeometryColumn"));
	CU_ASSERT_PTR_NULL(strstr(header, "ALTER TABLE"));

	free(header);
	ShpLoaderDestroy(loader_state);
}

void
test_ShpLoaderGetSQLFooter_if_not_exists_index_modifier(void)
{
	char *footer = NULL;

	loader_config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_config->table = "loadedshp";
	loader_config->geo_col = "the_geom";
	loader_config->actions.create_index = LOADER_CREATE_IF_NOT_EXISTS;
	loader_config->actions.create_index_set = 1;

	loader_state = ShpLoaderCreate(loader_config);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLFooter(loader_state, &footer), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(footer);
	CU_ASSERT_PTR_NOT_NULL(strstr(footer, "CREATE INDEX IF NOT EXISTS \"loadedshp_the_geom_gist\""));

	free(footer);
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
	CU_ASSERT_EQUAL_FATAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(header);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE \"loadedshp\" (\"gid\" serial"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "ADD PRIMARY KEY (\"gid\")"));
	free(header);
	header = NULL;
	ShpLoaderDestroy(state);

	config.fid_col = "fid";
	state = ShpLoaderCreate(&config);
	CU_ASSERT_EQUAL_FATAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL_FATAL(header);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE \"loadedshp\" (\"fid\" serial"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "ADD PRIMARY KEY (\"fid\")"));
	free(header);
	ShpLoaderDestroy(state);

	free(config.encoding);
}

void
test_ShpLoaderRejectsInvalidFIDColumn(void)
{
	SHPLOADERCONFIG config;
	SHPLOADERSTATE *state;
	char *header = NULL;

	set_loader_config_defaults(&config);
	config.table = "loadedshp";
	config.readshape = 0;
	config.fid_col = "bad\"fid";

	state = ShpLoaderCreate(&config);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADERERR);
	CU_ASSERT_PTR_NULL(header);
	CU_ASSERT_PTR_NOT_NULL(strstr(state->message, "Invalid feature id column name"));
	ShpLoaderDestroy(state);

	config.fid_col = "ctid";
	state = ShpLoaderCreate(&config);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADERERR);
	CU_ASSERT_PTR_NULL(header);
	CU_ASSERT_PTR_NOT_NULL(strstr(state->message, "Invalid feature id column name"));
	ShpLoaderDestroy(state);

	free(config.encoding);
}

void
test_ShpLoaderOpenShapeRejectsZip(void)
{
	int ret;

	loader_config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_config->shp_file = "fixtures/parcel.ZIP";
	loader_state = ShpLoaderCreate(loader_config);
	ret = ShpLoaderOpenShape(loader_state);
	CU_ASSERT_EQUAL(ret, SHPLOADERERR);
	CU_ASSERT_PTR_NOT_NULL(strstr(loader_state->message, "does not read .zip archives"));
	ShpLoaderDestroy(loader_state);

	loader_config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(loader_config);
	loader_config->shp_file = "fixtures.zip/parcel";
	loader_config->readshape = 0;
	loader_state = ShpLoaderCreate(loader_config);
	ret = ShpLoaderOpenShape(loader_state);
	CU_ASSERT_EQUAL(ret, SHPLOADERERR);
	CU_ASSERT_PTR_NULL(strstr(loader_state->message, "does not read .zip archives"));
	ShpLoaderDestroy(loader_state);
}
