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
#include <unistd.h>

/* Test functions */
void test_ShpLoaderCreate(void);
void test_ShpLoaderDestroy(void);
void test_ShpLoaderGetSQLHeader_drop_prepare(void);
void test_ShpLoaderGetSQLHeader_if_not_exists_table_modifier(void);
void test_ShpLoaderGetSQLFooter_if_not_exists_index_modifier(void);
void test_ShpLoaderFIDColumnSQL(void);
void test_ShpLoaderRejectsInvalidFIDColumn(void);
void test_ShpLoaderOpenShapeRejectsZip(void);
void test_ShpLoaderTopoGeometryDefaults(void);
void test_ShpLoaderTopoGeometrySQLHeader(void);
void test_ShpLoaderTopoGeometryIfNotExistsSQLHeader(void);
void test_ShpLoaderTopoGeometrySQLDrop(void);
void test_ShpLoaderTopoGeometrySQLAppend(void);
void test_ShpLoaderTopoGeometrySQLAppendRow(void);
void test_ShpLoaderTopoGeometryLineFeatureType(void);
void test_ShpLoaderTopoGeometryMOnlyReject(void);
void test_ShpLoaderTopoGeometryRejectsDBFOnlyFallback(void);
void test_ShpLoaderTopoGeometrySQLFooter(void);

SHPLOADERCONFIG *loader_config;
SHPLOADERSTATE *loader_state;

static void free_loader_config(SHPLOADERCONFIG *config);
static char *loader_fixture_base_path(const char *name, const char *required_suffix);
static char *loader_fixture_path(const char *name);
static SHPLOADERCONFIG *make_topogeometry_config(void);
static SHPLOADERSTATE *make_topogeometry_state(SHPLOADERCONFIG *config);

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
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderOpenShapeRejectsZip()", test_ShpLoaderOpenShapeRejectsZip)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderTopoGeometryDefaults()", test_ShpLoaderTopoGeometryDefaults)) ||
	    (NULL ==
	     CU_add_test(pSuite, "test_ShpLoaderTopoGeometrySQLHeader()", test_ShpLoaderTopoGeometrySQLHeader)) ||
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderTopoGeometryIfNotExistsSQLHeader()",
				 test_ShpLoaderTopoGeometryIfNotExistsSQLHeader)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderTopoGeometrySQLDrop()", test_ShpLoaderTopoGeometrySQLDrop)) ||
	    (NULL ==
	     CU_add_test(pSuite, "test_ShpLoaderTopoGeometrySQLAppend()", test_ShpLoaderTopoGeometrySQLAppend)) ||
	    (NULL ==
	     CU_add_test(pSuite, "test_ShpLoaderTopoGeometrySQLAppendRow()", test_ShpLoaderTopoGeometrySQLAppendRow)) ||
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderTopoGeometryLineFeatureType()",
				 test_ShpLoaderTopoGeometryLineFeatureType)) ||
	    (NULL ==
	     CU_add_test(pSuite, "test_ShpLoaderTopoGeometryMOnlyReject()", test_ShpLoaderTopoGeometryMOnlyReject)) ||
	    (NULL == CU_add_test(pSuite,
				 "test_ShpLoaderTopoGeometryRejectsDBFOnlyFallback()",
				 test_ShpLoaderTopoGeometryRejectsDBFOnlyFallback)) ||
	    (NULL == CU_add_test(pSuite, "test_ShpLoaderTopoGeometrySQLFooter()", test_ShpLoaderTopoGeometrySQLFooter)))
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

static void
free_loader_config(SHPLOADERCONFIG *config)
{
	if (!config)
		return;

	free(config->topology);
	free(config->schema);
	free(config->table);
	free(config->geo_col);
	free(config->shp_file);
	free(config->encoding);
	free(config);
}

static char *
loader_fixture_base_path(const char *name, const char *required_suffix)
{
	static const char *prefixes[] = {"regress/loader/", "../../regress/loader/"};
	size_t i;

	for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++)
	{
		char *path;
		char *required_path;
		size_t suffix_len = strlen(required_suffix);
		int len = snprintf(NULL, 0, "%s%s", prefixes[i], name);
		if (len < 0)
			continue;

		path = malloc((size_t)len + 1);
		required_path = malloc((size_t)len + suffix_len + 1);
		if (!path || !required_path)
		{
			free(path);
			free(required_path);
			return NULL;
		}

		snprintf(path, (size_t)len + 1, "%s%s", prefixes[i], name);
		snprintf(required_path, (size_t)len + suffix_len + 1, "%s%s", path, required_suffix);
		if (access(required_path, R_OK) == 0)
		{
			free(required_path);
			return path;
		}
		free(path);
		free(required_path);
	}

	return NULL;
}

static char *
loader_fixture_path(const char *name)
{
	return loader_fixture_base_path(name, ".shp");
}

static SHPLOADERCONFIG *
make_topogeometry_config(void)
{
	SHPLOADERCONFIG *config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	if (!config)
		return NULL;

	set_loader_config_defaults(config);
	config->topology = strdup("city_topo");
	config->schema = strdup("public");
	config->table = strdup("blocks");
	config->geo_col = strdup("topogeom");
	if (!config->encoding || !config->topology || !config->schema || !config->table || !config->geo_col)
	{
		free_loader_config(config);
		return NULL;
	}

	return config;
}

static SHPLOADERSTATE *
make_topogeometry_state(SHPLOADERCONFIG *config)
{
	SHPLOADERSTATE *state;
	if (!config)
		return NULL;

	state = ShpLoaderCreate(config);
	if (!state)
		return NULL;

	state->pgtype = "POLYGON";
	state->pgdims = 2;
	state->col_names = strdup("topogeom");
	if (!state->col_names)
	{
		ShpLoaderDestroy(state);
		return NULL;
	}

	return state;
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

void
test_ShpLoaderTopoGeometryDefaults(void)
{
	SHPLOADERCONFIG *config = (SHPLOADERCONFIG *)calloc(1, sizeof(SHPLOADERCONFIG));
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	set_loader_config_defaults(config);
	CU_ASSERT_FATAL(config->encoding != NULL);
	config->topology = strdup("city_topo");
	CU_ASSERT_FATAL(config->topology != NULL);
	state = ShpLoaderCreate(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_STRING_EQUAL(state->geo_col, TOPOGEOMETRY_DEFAULT);

	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometrySQLHeader(void)
{
	char *header = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	state = make_topogeometry_state(config);
	CU_ASSERT_FATAL(state != NULL);
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE \"public\".\"blocks\" (\"gid\" serial);"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "SELECT topology.AddTopoGeometryColumn('city_topo', '\"public\".\"blocks\"'::regclass, 'topogeom', NULL, 'POLYGON');"));
	CU_ASSERT_PTR_NULL(strstr(header, "AddGeometryColumn"));
	CU_ASSERT_PTR_NULL(strstr(header, "geometry(POLYGON"));

	free(header);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometryIfNotExistsSQLHeader(void)
{
	char *header = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->actions.if_not_exists = 1;
	state = make_topogeometry_state(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "CREATE TABLE IF NOT EXISTS \"public\".\"blocks\""));
	CU_ASSERT_PTR_NOT_NULL(
	    strstr(header, "IF topology.FindLayer('\"public\".\"blocks\"'::regclass, 'topogeom'::name) IS NULL THEN"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "PERFORM topology.AddTopoGeometryColumn('city_topo', '\"public\".\"blocks\"'::regclass, 'topogeom', NULL, 'POLYGON');"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "CREATE OR REPLACE FUNCTION pg_temp.shp2pgsql_topogeometry_layer_id(toponame name, tab regclass, col name)"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "PERFORM pg_temp.shp2pgsql_topogeometry_layer_id('city_topo'::name, '\"public\".\"blocks\"'::regclass, 'topogeom'::name);"));

	free(header);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometrySQLDrop(void)
{
	char *header = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->actions.mode = 'd';
	state = make_topogeometry_state(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "target_table regclass := to_regclass('\"public\".\"blocks\"');"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "schema_name = target_schema AND table_name = target_name"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "PERFORM topology.DropTopoGeometryColumn(target_schema::varchar, target_name::varchar, lyr.feature_column);"));
	CU_ASSERT_PTR_NULL(strstr(header, "AND feature_column = 'topogeom'"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "DROP TABLE IF EXISTS \"public\".\"blocks\";"));

	free(header);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometrySQLAppend(void)
{
	char *header = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->actions.mode = 'a';
	state = make_topogeometry_state(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "CREATE OR REPLACE FUNCTION pg_temp.shp2pgsql_topogeometry_layer_id(toponame name, tab regclass, col name)"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "topoid := topology.GetTopologyID(toponame::varchar);"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "lyr := topology.FindLayer(tab, col);"));
	CU_ASSERT_PTR_NOT_NULL(
	    strstr(header, "RAISE EXCEPTION 'TopoGeometry column %.% is not registered in topology.layer'"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "IF lyr.topology_id != topoid THEN"));
	CU_ASSERT_PTR_NOT_NULL(strstr(header, "RETURN lyr.layer_id;"));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "PERFORM pg_temp.shp2pgsql_topogeometry_layer_id('city_topo'::name, '\"public\".\"blocks\"'::regclass, 'topogeom'::name);"));
	CU_ASSERT_PTR_NULL(strstr(header, "CREATE TABLE"));
	CU_ASSERT_PTR_NULL(strstr(header, "AddTopoGeometryColumn"));

	free(header);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometrySQLAppendRow(void)
{
	char *record = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->actions.mode = 'a';
	config->shp_file = loader_fixture_path("ReprojectPts");
	CU_ASSERT_FATAL(config->shp_file != NULL);
	state = ShpLoaderCreate(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderOpenShape(state), SHPLOADEROK);
	CU_ASSERT_EQUAL(ShpLoaderGenerateSQLRowStatement(state, 0, &record), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(record, "topology.toTopoGeom("));
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    record,
	    "pg_temp.shp2pgsql_topogeometry_layer_id('city_topo'::name, '\"public\".\"blocks\"'::regclass, 'topogeom'::name)"));
	CU_ASSERT_PTR_NULL(strstr(record, "layer_id(topology.FindLayer"));

	free(record);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometryLineFeatureType(void)
{
	char *header = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->shp_file = loader_fixture_path("Arc");
	CU_ASSERT_FATAL(config->shp_file != NULL);
	state = ShpLoaderCreate(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderOpenShape(state), SHPLOADEROK);
	CU_ASSERT_STRING_EQUAL(state->pgtype, "MULTILINE");
	CU_ASSERT_EQUAL(ShpLoaderGetSQLHeader(state, &header), SHPLOADEROK);
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    header,
	    "SELECT topology.AddTopoGeometryColumn('city_topo', '\"public\".\"blocks\"'::regclass, 'topogeom', NULL, 'MULTILINE');"));

	free(header);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometryMOnlyReject(void)
{
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->force_output = FORCE_OUTPUT_3DM;
	config->shp_file = loader_fixture_path("PointM");
	CU_ASSERT_FATAL(config->shp_file != NULL);
	state = ShpLoaderCreate(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderOpenShape(state), SHPLOADERERR);
	CU_ASSERT_PTR_NOT_NULL(strstr(
	    state->message,
	    "Topology loading does not support M-only geometries; use -t 2D or -t 3DZ to drop the measure dimension."));

	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometryRejectsDBFOnlyFallback(void)
{
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	config->shp_file = loader_fixture_base_path("CharNoWidth", ".dbf");
	CU_ASSERT_FATAL(config->shp_file != NULL);
	state = ShpLoaderCreate(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderOpenShape(state), SHPLOADERERR);
	CU_ASSERT_PTR_NOT_NULL(strstr(state->message, "topology loading requires readable shape"));
	CU_ASSERT_EQUAL(config->readshape, 1);

	ShpLoaderDestroy(state);
	free_loader_config(config);
}

void
test_ShpLoaderTopoGeometrySQLFooter(void)
{
	char *footer = NULL;
	SHPLOADERCONFIG *config = make_topogeometry_config();
	SHPLOADERSTATE *state;

	CU_ASSERT_FATAL(config != NULL);
	state = make_topogeometry_state(config);
	CU_ASSERT_FATAL(state != NULL);

	CU_ASSERT_EQUAL(ShpLoaderGetSQLFooter(state, &footer), SHPLOADEROK);
	CU_ASSERT_PTR_NULL(strstr(footer, "USING GIST"));
	CU_ASSERT_PTR_NOT_NULL(strstr(footer, "ANALYZE \"public\".\"blocks\";"));

	free(footer);
	ShpLoaderDestroy(state);
	free_loader_config(config);
}
