/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeodetic.h"
#include "stringbuffer.h"
#include "lwgeom_transform.h"


Datum transform(PG_FUNCTION_ARGS);
Datum transform_geom(PG_FUNCTION_ARGS);
Datum transform_pipeline_geom(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);
Datum postgis_proj_compiled_version(PG_FUNCTION_ARGS);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS);

/**
 * transform( GEOMETRY, INT (output srid) )
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{
	GSERIALIZED* geom;
	GSERIALIZED* result=NULL;
	LWGEOM* lwgeom;
	LWPROJ *pj;
	int32 srid_to, srid_from;

	srid_to = PG_GETARG_INT32(1);
	if (srid_to == SRID_UNKNOWN)
	{
		elog(ERROR, "ST_Transform: %d is an invalid target SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	srid_from = gserialized_get_srid(geom);

	if ( srid_from == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_Transform: Input geometry has unknown (%d) SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Input SRID and output SRID are equal, noop */
	if ( srid_from == srid_to )
		PG_RETURN_POINTER(geom);

	postgis_initialize_cache();
	if ( lwproj_lookup(srid_from, srid_to, &pj) == LW_FAILURE )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_Transform: Failure reading projections from spatial_ref_sys.");
		PG_RETURN_NULL();
	}

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom = lwgeom_from_gserialized(geom);
	lwgeom_transform(lwgeom, pj);
	lwgeom->srid = srid_to;

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( lwgeom->bbox )
	{
		lwgeom_refresh_bbox(lwgeom);
	}

	result = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result); /* new geometry */
}

/**
 * Transform_geom( GEOMETRY, TEXT (input proj4), TEXT (output proj4),
 *	INT (output srid)
 *
 * tmpPts - if there is a nadgrid error (-38), we re-try the transform
 * on a copy of points.  The transformed points
 * are in an indeterminate state after the -38 error is thrown.
 */
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser, *gser_result=NULL;
	LWGEOM *geom;
	char *input_srs, *output_srs;
	int32 result_srid;
	int rv;

	/* Take a copy, since we will be altering the coordinates */
	gser = PG_GETARG_GSERIALIZED_P_COPY(0);

	/* Convert from text to cstring for libproj */
	input_srs = text_to_cstring(PG_GETARG_TEXT_P(1));
	output_srs = text_to_cstring(PG_GETARG_TEXT_P(2));
	result_srid = PG_GETARG_INT32(3);

	/* now we have a geometry, and input/output PJ structs. */
	geom = lwgeom_from_gserialized(gser);
	rv = lwgeom_transform_from_str(geom, input_srs, output_srs);
	lwfree(input_srs);
	lwfree(output_srs);

	if (rv == LW_FAILURE)
	{
		elog(ERROR, "coordinate transformation failed");
		PG_RETURN_NULL();
	}

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	geom->srid = result_srid;
	if (geom->bbox)
		lwgeom_refresh_bbox(geom);

	gser_result = geometry_serialize(geom);
	lwgeom_free(geom);
	PG_FREE_IF_COPY(gser, 0);

	PG_RETURN_POINTER(gser_result); /* new geometry */
}


/**
 * transform_pipeline_geom( GEOMETRY, TEXT (input proj pipeline),
 *	BOOL (is forward), INT (output srid)
 */
PG_FUNCTION_INFO_V1(transform_pipeline_geom);
Datum transform_pipeline_geom(PG_FUNCTION_ARGS)
{
	GSERIALIZED *gser, *gser_result=NULL;
	LWGEOM *geom;
	char *input_pipeline;
	bool is_forward;
	int32 result_srid;
	int rv;

	/* Take a copy, since we will be altering the coordinates */
	gser = PG_GETARG_GSERIALIZED_P_COPY(0);

	/* Convert from text to cstring for libproj */
	input_pipeline = text_to_cstring(PG_GETARG_TEXT_P(1));
	is_forward = PG_GETARG_BOOL(2);
	result_srid = PG_GETARG_INT32(3);

	geom = lwgeom_from_gserialized(gser);
	rv = lwgeom_transform_pipeline(geom, input_pipeline, is_forward);
	lwfree(input_pipeline);

	if (rv == LW_FAILURE)
	{
		elog(ERROR, "coordinate transformation failed");
		PG_RETURN_NULL();
	}

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	geom->srid = result_srid;
	if (geom->bbox)
		lwgeom_refresh_bbox(geom);

	gser_result = geometry_serialize(geom);
	lwgeom_free(geom);
	PG_FREE_IF_COPY(gser, 0);

	PG_RETURN_POINTER(gser_result); /* new geometry */
}


PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	stringbuffer_t sb;

	PJ_INFO pji = proj_info();
	stringbuffer_init(&sb);
	stringbuffer_append(&sb, pji.version);

#if POSTGIS_PROJ_VERSION >= 70100

	stringbuffer_aprintf(&sb,
		" NETWORK_ENABLED=%s",
		proj_context_is_network_enabled(NULL) ? "ON" : "OFF");

	if (proj_context_get_url_endpoint(NULL))
		stringbuffer_aprintf(&sb, " URL_ENDPOINT=%s", proj_context_get_url_endpoint(NULL));

	if (proj_context_get_user_writable_directory(NULL, 0))
		stringbuffer_aprintf(&sb, " USER_WRITABLE_DIRECTORY=%s", proj_context_get_user_writable_directory(NULL, 0));

	if (proj_context_get_database_path(NULL))
		stringbuffer_aprintf(&sb, " DATABASE_PATH=%s", proj_context_get_database_path(NULL));

#endif

	PG_RETURN_POINTER(cstring_to_text(stringbuffer_getstring(&sb)));
}

PG_FUNCTION_INFO_V1(postgis_proj_compiled_version);
Datum postgis_proj_compiled_version(PG_FUNCTION_ARGS)
{
  static char ver[64];
  text *result;
  sprintf(
    ver,
    "%d.%d.%d",
    (POSTGIS_PROJ_VERSION/10000),
    ((POSTGIS_PROJ_VERSION%10000)/100),
    ((POSTGIS_PROJ_VERSION)%100)
  );

  result = cstring_to_text(ver);
  PG_RETURN_POINTER(result);
}

/**
 * Encode feature in KML
 */
PG_FUNCTION_INFO_V1(LWGEOM_asKML);
Datum LWGEOM_asKML(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom;
	lwvarlena_t *kml;
	const char *default_prefix = ""; /* default prefix */
	char *prefixbuf;
	const char *prefix = default_prefix;
	int32_t srid_from;
	const int32_t srid_to = 4326;

	/* Get the geometry */
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P_COPY(0);
	int precision = PG_GETARG_INT32(1);
	text *prefix_text = PG_GETARG_TEXT_P(2);
	srid_from = gserialized_get_srid(geom);

	if ( srid_from == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(geom, 0);
		elog(ERROR, "ST_AsKML: Input geometry has unknown (%d) SRID", SRID_UNKNOWN);
		PG_RETURN_NULL();
	}

	/* Condition precision */
	if (precision < 0)
		precision = 0;

	if (VARSIZE_ANY_EXHDR(prefix_text) > 0)
	{
		/* +2 is one for the ':' and one for term null */
		prefixbuf = lwalloc(VARSIZE_ANY_EXHDR(prefix_text)+2);
		memcpy(prefixbuf, VARDATA(prefix_text),
		       VARSIZE_ANY_EXHDR(prefix_text));
		/* add colon and null terminate */
		prefixbuf[VARSIZE_ANY_EXHDR(prefix_text)] = ':';
		prefixbuf[VARSIZE_ANY_EXHDR(prefix_text)+1] = '\0';
		prefix = prefixbuf;
	}

	lwgeom = lwgeom_from_gserialized(geom);

	if (srid_from != srid_to)
	{
		LWPROJ *pj;
		if (lwproj_lookup(srid_from, srid_to, &pj) == LW_FAILURE)
		{
			PG_FREE_IF_COPY(geom, 0);
			elog(ERROR, "ST_AsKML: Failure reading projections from spatial_ref_sys.");
			PG_RETURN_NULL();
		}
		lwgeom_transform(lwgeom, pj);
	}

	kml = lwgeom_to_kml2(lwgeom, precision, prefix);
	if (kml)
		PG_RETURN_TEXT_P(kml);
	PG_RETURN_NULL();
}

/********************************************************************************
 * PROJ database reading functions
 */

struct srs_entry {
	text* auth_name;
	text* auth_code;
	double sort;
};

struct srs_data {
	struct srs_entry* entries;
	uint32_t num_entries;
	uint32_t capacity;
	uint32_t current_entry;
};

static int
srs_entry_cmp (const void *a, const void *b)
{
	const struct srs_entry *entry_a = (const struct srs_entry*)(a);
	const struct srs_entry *entry_b = (const struct srs_entry*)(b);
	if      (entry_a->sort < entry_b->sort) return -1;
	else if (entry_a->sort > entry_b->sort) return 1;
	else return 0;
}

static Datum
srs_tuple_from_entry(const struct srs_entry* entry, TupleDesc tuple_desc)
{
	HeapTuple tuple;
	Datum tuple_data[7] = {0, 0, 0, 0, 0, 0, 0};
	bool  tuple_null[7] = {true, true, true, true, true, true, true};
	PJ_CONTEXT * ctx = NULL;
	const char * const empty_options[2] = {NULL};
	const char * const wkt_options[2] = {"MULTILINE=NO", NULL};
	const char * srtext;
	const char * proj4text;
	const char * srname;
	double w_lon, s_lat, e_lon, n_lat;
	int ok;

	PJ *obj = proj_create_from_database(ctx,
	            text_to_cstring(entry->auth_name),
	            text_to_cstring(entry->auth_code),
	            PJ_CATEGORY_CRS, 0, empty_options);

	if (!obj)
		return (Datum) 0;

	srtext = proj_as_wkt(ctx, obj, PJ_WKT1_GDAL, wkt_options);
	proj4text = proj_as_proj_string(ctx, obj, PJ_PROJ_4, empty_options);
	srname = proj_get_name(obj);
	ok = proj_get_area_of_use(ctx, obj, &w_lon, &s_lat, &e_lon, &n_lat, NULL);

	if (entry->auth_name) {
		tuple_data[0] = PointerGetDatum(entry->auth_name);
		tuple_null[0] = false;
	}

	if (entry->auth_code) {
		tuple_data[1] = PointerGetDatum(entry->auth_code);
		tuple_null[1] = false;
	}

	if (srname) {
		tuple_data[2] = PointerGetDatum(cstring_to_text(srname));
		tuple_null[2] = false;
	}

	if (srtext) {
		tuple_data[3] = PointerGetDatum(cstring_to_text(srtext));
		tuple_null[3] = false;
	}

	if (proj4text) {
		tuple_data[4] = PointerGetDatum(cstring_to_text(proj4text));
		tuple_null[4] = false;
	}

	if (ok) {
		LWPOINT *p_sw = lwpoint_make2d(4326, w_lon, s_lat);
		LWPOINT *p_ne = lwpoint_make2d(4326, e_lon, n_lat);
		GSERIALIZED *g_sw = geometry_serialize((LWGEOM*)p_sw);
		GSERIALIZED *g_ne = geometry_serialize((LWGEOM*)p_ne);
		tuple_data[5] = PointerGetDatum(g_sw);
		tuple_null[5] = false;
		tuple_data[6] = PointerGetDatum(g_ne);
		tuple_null[6] = false;
	}

	tuple = heap_form_tuple(tuple_desc, tuple_data, tuple_null);
	proj_destroy(obj);

	return HeapTupleGetDatum(tuple);
}

static struct srs_data *
srs_state_init()
{
	struct srs_data *state = lwalloc0(sizeof(*state));
	state->capacity = 8192;
	state->num_entries = 0;
	state->entries = lwalloc0(state->capacity * sizeof(*(state->entries)));
	return state;
}

static void
srs_state_memcheck(struct srs_data *state)
{
	if (state->num_entries == state->capacity)
	{
		state->capacity *= 2;
		state->entries = lwrealloc(state->entries, state->capacity * sizeof(*(state->entries)));
	}
	return;
}

static void
srs_state_codes(const char* auth_name, struct srs_data *state)
{
	/*
	* Only a subset of supported proj types actually
	* show up in spatial_ref_sys
	*/
	#define ntypes 3
	PJ_TYPE types[ntypes] = {PJ_TYPE_PROJECTED_CRS, PJ_TYPE_GEOGRAPHIC_CRS, PJ_TYPE_COMPOUND_CRS};
	uint32_t j;

	for (j = 0; j < ntypes; j++)
	{
		PJ_CONTEXT *ctx = NULL;
		int allow_deprecated = 0;
		PJ_TYPE type = types[j];
		PROJ_STRING_LIST codes_ptr = proj_get_codes_from_database(ctx, auth_name, type, allow_deprecated);
		PROJ_STRING_LIST codes = codes_ptr;
		const char *code;
		while(codes && *codes)
		{
			/* Read current code and move forward one entry */
			code = *codes++;
			/* Ensure there is space in the entry list */
			srs_state_memcheck(state);

			/* Write the entry into the entry list and increment */
			state->entries[state->num_entries].auth_name = cstring_to_text(auth_name);
			state->entries[state->num_entries].auth_code = cstring_to_text(code);
			state->num_entries++;
		}
		/* Clean up system allocated memory */
		proj_string_list_destroy(codes_ptr);
	}
}

static void
srs_find_planar(const char *auth_name, const LWGEOM *bounds, struct srs_data *state)
{
	int32_t srid_from = lwgeom_get_srid(bounds);
	const int32_t srid_to = 4326;
	GBOX gbox = *(lwgeom_get_bbox(bounds));
	PJ_TYPE types[1] = {PJ_TYPE_PROJECTED_CRS};
	PROJ_CRS_INFO **crs_list_ptr, **crs_list;
	int crs_count;
	PJ_CONTEXT *ctx = NULL;

	PROJ_CRS_LIST_PARAMETERS *params = proj_get_crs_list_parameters_create();
	params->types = types;
	params->typesCount = 1;
	params->crs_area_of_use_contains_bbox = true;
	params->bbox_valid = true;
	params->allow_deprecated = false;

#if POSTGIS_PROJ_VERSION >= 80100
	params->celestial_body_name = "Earth";
#endif

	if (srid_from != srid_to)
	{
		LWPROJ *pj;
		if (lwproj_lookup(srid_from, srid_to, &pj) == LW_FAILURE)
			elog(ERROR, "%s: Lookup of SRID %u => %u transform failed",
			    __func__, srid_from, srid_to);

		box3d_transform(&gbox, pj);
	}

	params->west_lon_degree = gbox.xmin;
	params->south_lat_degree = gbox.ymin;
	params->east_lon_degree = gbox.xmax;
	params->north_lat_degree = gbox.ymax;

	crs_list = crs_list_ptr = proj_get_crs_info_list_from_database(
	                            ctx, auth_name, params, &crs_count);

	/* TODO, throw out really huge / dumb areas? */

	while (crs_list && *crs_list)
	{
		/* Read current crs and move forward one entry */
		PROJ_CRS_INFO *crs = *crs_list++;

		/* Read the corners of the CRS area of use */
		double area;
		double height = crs->north_lat_degree - crs->south_lat_degree;
		double width = crs->east_lon_degree - crs->west_lon_degree;
		if (width < 0.0)
			width = 360 - (crs->west_lon_degree - crs->east_lon_degree);
		area = width * height;

		/* Ensure there is space in the entry list */
		srs_state_memcheck(state);

		/* Write the entry into the entry list and increment */
		state->entries[state->num_entries].auth_name = cstring_to_text(crs->auth_name);
		state->entries[state->num_entries].auth_code = cstring_to_text(crs->code);
		state->entries[state->num_entries].sort = area;
		state->num_entries++;
	}

	/* Put the list of entries into order of area size, smallest to largest */
	qsort(state->entries, state->num_entries, sizeof(struct srs_data), srs_entry_cmp);

	proj_crs_info_list_destroy(crs_list_ptr);
	proj_get_crs_list_parameters_destroy(params);
}

/**
 * Search for srtext and proj4text given auth_name and auth_srid,
 * returns TABLE(auth_name text, auth_srid text, srtext text, proj4text text)
 */
Datum postgis_srs_entry(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_srs_entry);
Datum postgis_srs_entry(PG_FUNCTION_ARGS)
{
	Datum result;
	struct srs_entry entry;
	text* auth_name = PG_GETARG_TEXT_P(0);
	text* auth_code = PG_GETARG_TEXT_P(1);
	TupleDesc tuple_desc;

	if (get_call_result_type(fcinfo, 0, &tuple_desc) != TYPEFUNC_COMPOSITE)
	{
		ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			errmsg("%s called with incompatible return type", __func__)));
	}
	BlessTupleDesc(tuple_desc);

	entry.auth_name = auth_name;
	entry.auth_code = auth_code;
	result = srs_tuple_from_entry(&entry, tuple_desc);

	if (result)
		PG_RETURN_DATUM(srs_tuple_from_entry(&entry, tuple_desc));
	else
		PG_RETURN_NULL();
}


Datum postgis_srs_entry_all(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_srs_entry_all);
Datum postgis_srs_entry_all(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	struct srs_data *state;
	Datum result;

	/*
	* On the first call, fill in the state with all
	* of the auth_name/auth_srid pairings in the
	* proj database. Then the per-call routine is just
	* one isolated call per pair.
	*/
	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/*
		* Could read all authorities from database, but includes
		* authorities (IGN, OGC) that use non-integral values in
		* auth_srid. So hand-coded list for now.
		*/
		state = srs_state_init();
		srs_state_codes("EPSG", state);
		srs_state_codes("ESRI", state);
		srs_state_codes("IAU_2015", state);

		/*
		* Read the TupleDesc from the FunctionCallInfo. The SQL definition
		* of the function must have the right number of fields and types
		* to match up to this C code.
		*/
		if (get_call_result_type(fcinfo, 0, &funcctx->tuple_desc) != TYPEFUNC_COMPOSITE)
		{
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			    errmsg("%s called with incompatible return type", __func__)));
		}

		BlessTupleDesc(funcctx->tuple_desc);
		funcctx->user_fctx = state;
		MemoryContextSwitchTo(oldcontext);
	}

	/* Stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	state = funcctx->user_fctx;

	/* Exit when we've read all entries */
	if (!state->num_entries || state->current_entry == state->num_entries)
	{
		SRF_RETURN_DONE(funcctx);
	}

	/* Lookup the srtext/proj4text for this entry */
	result = srs_tuple_from_entry(
	           state->entries + state->current_entry++,
	           funcctx->tuple_desc);

	if (result)
		SRF_RETURN_NEXT(funcctx, result);

	/* Stop if lookup fails drastically */
	SRF_RETURN_DONE(funcctx);
}


Datum postgis_srs_codes(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_srs_codes);
Datum postgis_srs_codes(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	struct srs_data *state;
	Datum result;
	text* auth_name = PG_GETARG_TEXT_P(0);
	text* auth_code;

	/*
	* On the first call, fill in the state with all
	* of the auth_name/auth_srid pairings in the
	* proj database. Then the per-call routine is just
	* one isolated call per pair.
	*/
	if (SRF_IS_FIRSTCALL())
	{
		/*
		* Only a subset of supported proj types actually
		* show up in spatial_ref_sys
		*/
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
		state = srs_state_init();
		srs_state_codes(text_to_cstring(auth_name), state);
		funcctx->user_fctx = state;
		MemoryContextSwitchTo(oldcontext);
	}

	/* Stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	state = funcctx->user_fctx;

	/* Exit when we've read all entries */
	if (!state->num_entries || state->current_entry == state->num_entries)
	{
		SRF_RETURN_DONE(funcctx);
	}

	/* Read the code for this entry */
	auth_code = state->entries[state->current_entry++].auth_code;
	result = PointerGetDatum(auth_code);

	/* We are returning setof(text) */
	if (result)
		SRF_RETURN_NEXT(funcctx, result);

	/* Stop if lookup fails drastically */
	SRF_RETURN_DONE(funcctx);
	SRF_RETURN_DONE(funcctx);
}


/**
 * Search for projections given extent and (optional) auth_name
 * returns TABLE(auth_name, auth_srid, srtext, proj4text, point_sw, point_ne)
 */
Datum postgis_srs_search(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_srs_search);
Datum postgis_srs_search(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	struct srs_data *state;
	Datum result;

	/*
	* On the first call, fill in the state with all
	* of the auth_name/auth_srid pairings in the
	* proj database. Then the per-call routine is just
	* one isolated call per pair.
	*/
	if (SRF_IS_FIRSTCALL())
	{
		GSERIALIZED *gbounds = PG_GETARG_GSERIALIZED_P(0);
		LWGEOM *bounds = lwgeom_from_gserialized(gbounds);
		text *auth_name = PG_GETARG_TEXT_P(1);

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/*
		* Could read all authorities from database, but includes
		* authorities (IGN, OGC) that use non-integral values in
		* auth_srid. So hand-coded list for now.
		*/
		state = srs_state_init();

		/* Run the Proj query */
		srs_find_planar(text_to_cstring(auth_name), bounds, state);

		/*
		* Read the TupleDesc from the FunctionCallInfo. The SQL definition
		* of the function must have the right number of fields and types
		* to match up to this C code.
		*/
		if (get_call_result_type(fcinfo, 0, &funcctx->tuple_desc) != TYPEFUNC_COMPOSITE)
		{
			ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			    errmsg("%s called with incompatible return type", __func__)));
		}

		BlessTupleDesc(funcctx->tuple_desc);
		funcctx->user_fctx = state;
		MemoryContextSwitchTo(oldcontext);
	}

	/* Stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	state = funcctx->user_fctx;

	/* Exit when we've read all entries */
	if (!state->num_entries ||
	    state->current_entry == state->num_entries)
	{
		SRF_RETURN_DONE(funcctx);
	}

	/* Lookup the srtext/proj4text for this entry */
	result = srs_tuple_from_entry(
	           state->entries + state->current_entry++,
	           funcctx->tuple_desc);

	if (result)
		SRF_RETURN_NEXT(funcctx, result);

	/* Stop if lookup fails drastically */
	SRF_RETURN_DONE(funcctx);
}


