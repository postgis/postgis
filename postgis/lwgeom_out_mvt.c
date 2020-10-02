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
 * Copyright (C) 2016-2017 Björn Harrtell <bjorn@wololo.org>
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"
#include "mvt.h"

#ifdef HAVE_LIBPROTOBUF
#include "vector_tile.pb-c.h"
#endif  /* HAVE_LIBPROTOBUF */

/**
 * Process input parameters to mvt_geom and returned serialized geometry
 */
PG_FUNCTION_INFO_V1(ST_AsMVTGeom);
Datum ST_AsMVTGeom(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVTGeom: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	GBOX *bounds = NULL;
	int32_t extent = 0;
	int32_t buffer = 0;
	bool clip_geom = true;
	GSERIALIZED *geom_in, *geom_out;
	LWGEOM *lwgeom_in, *lwgeom_out;
	uint8_t type = 0;

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	if (PG_ARGISNULL(1))
	{
		elog(ERROR, "%s: Geometric bounds cannot be null", __func__);
		PG_RETURN_NULL();
	}
	bounds = (GBOX *)PG_GETARG_POINTER(1);
	if (bounds->xmax - bounds->xmin <= 0 || bounds->ymax - bounds->ymin <= 0)
	{
		elog(ERROR, "%s: Geometric bounds are too small", __func__);
		PG_RETURN_NULL();
	}

	extent = PG_ARGISNULL(2) ? 4096 : PG_GETARG_INT32(2);
	if (extent <= 0)
	{
		elog(ERROR, "%s: Extent must be greater than 0", __func__);
		PG_RETURN_NULL();
	}

	buffer = PG_ARGISNULL(3) ? 256 : PG_GETARG_INT32(3);
	clip_geom = PG_ARGISNULL(4) ? true : PG_GETARG_BOOL(4);

	geom_in = PG_GETARG_GSERIALIZED_P_COPY(0);
	type = gserialized_get_type(geom_in);

	/* If possible, peek into the bounding box before deserializing it to discard small geometries
	 * We don't check COLLECTIONTYPE since that might be a collection of points */
	if (type == LINETYPE || type == POLYGONTYPE || type == MULTILINETYPE || type == MULTIPOLYGONTYPE)
	{
		GBOX gserialized_box;
		/* We only apply the optimization if the bounding box is available */
		if (gserialized_fast_gbox_p(geom_in, &gserialized_box) == LW_SUCCESS)
		{
			/* Shortcut to drop geometries smaller than the resolution */
			double geom_width = gserialized_box.xmax - gserialized_box.xmin;
			double geom_height = gserialized_box.ymax - gserialized_box.ymin;

			/* We use half of the square height and width as limit: We use this
			 * and not area so it works properly with lines */
			double bounds_width = ((bounds->xmax - bounds->xmin) / extent) / 2.0;
			double bounds_height = ((bounds->ymax - bounds->ymin) / extent) / 2.0;
			if (geom_width < bounds_width && geom_height < bounds_height)
			{
				PG_RETURN_NULL();
			}
		}
	}

	lwgeom_in = lwgeom_from_gserialized(geom_in);

	lwgeom_out = mvt_geom(lwgeom_in, bounds, extent, buffer, clip_geom);
	if (lwgeom_out == NULL)
		PG_RETURN_NULL();

	geom_out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);
	PG_FREE_IF_COPY(geom_in, 0);
	PG_RETURN_POINTER(geom_out);
#endif
}

/**
 * Process input parameters and row data into state
 */
PG_FUNCTION_INFO_V1(pgis_asmvt_transfn);
Datum pgis_asmvt_transfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVT: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext, old_context;
	mvt_agg_context *ctx;

	/* We need to initialize the internal cache to access it later via postgis_oid() */
	postgis_initialize_cache(fcinfo);

	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	if (PG_ARGISNULL(0)) {
		old_context = MemoryContextSwitchTo(aggcontext);
		ctx = palloc(sizeof(*ctx));
		ctx->name = "default";
		if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
			ctx->name = text_to_cstring(PG_GETARG_TEXT_P(2));
		ctx->extent = 4096;
		if (PG_NARGS() > 3 && !PG_ARGISNULL(3))
			ctx->extent = PG_GETARG_INT32(3);
		ctx->geom_name = NULL;
		if (PG_NARGS() > 4 && !PG_ARGISNULL(4))
			ctx->geom_name = text_to_cstring(PG_GETARG_TEXT_P(4));
		if (PG_NARGS() > 5 && !PG_ARGISNULL(5))
			ctx->id_name = text_to_cstring(PG_GETARG_TEXT_P(5));
		else
			ctx->id_name = NULL;

		ctx->trans_context = AllocSetContextCreate(aggcontext, "MVT transfn", ALLOCSET_DEFAULT_SIZES);

		MemoryContextSwitchTo(ctx->trans_context);
		mvt_agg_init_context(ctx);
		MemoryContextSwitchTo(old_context);
	} else {
		ctx = (mvt_agg_context *) PG_GETARG_POINTER(0);
	}

	if (!type_is_rowtype(get_fn_expr_argtype(fcinfo->flinfo, 1)))
		elog(ERROR, "%s: parameter row cannot be other than a rowtype", __func__);
	ctx->row = PG_GETARG_HEAPTUPLEHEADER(1);

	old_context = MemoryContextSwitchTo(ctx->trans_context);
	mvt_agg_transfn(ctx);
	MemoryContextSwitchTo(old_context);

	PG_FREE_IF_COPY(ctx->row, 1);
	PG_RETURN_POINTER(ctx);
#endif
}

/**
 * Encode final state to Mapbox Vector Tile
 */
PG_FUNCTION_INFO_V1(pgis_asmvt_finalfn);
Datum pgis_asmvt_finalfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVT: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	mvt_agg_context *ctx;
	bytea *buf;
	elog(DEBUG2, "%s called", __func__);
	if (!AggCheckCallContext(fcinfo, NULL))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	if (PG_ARGISNULL(0))
	{
		bytea *emptybuf = palloc(VARHDRSZ);
		SET_VARSIZE(emptybuf, VARHDRSZ);
		PG_RETURN_BYTEA_P(emptybuf);
	}

	ctx = (mvt_agg_context *) PG_GETARG_POINTER(0);
	buf = mvt_agg_finalfn(ctx);
	PG_RETURN_BYTEA_P(buf);
#endif
}

PG_FUNCTION_INFO_V1(pgis_asmvt_serialfn);
Datum pgis_asmvt_serialfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVT: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	mvt_agg_context *ctx;
	bytea *result;
	elog(DEBUG2, "%s called", __func__);
	if (!AggCheckCallContext(fcinfo, NULL))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	if (PG_ARGISNULL(0))
	{
		bytea *emptybuf = palloc(VARHDRSZ);
		SET_VARSIZE(emptybuf, VARHDRSZ);
		PG_RETURN_BYTEA_P(emptybuf);
	}

	ctx = (mvt_agg_context *) PG_GETARG_POINTER(0);
	result = mvt_ctx_serialize(ctx);
	if (ctx->trans_context)
		MemoryContextDelete(ctx->trans_context);
	ctx->trans_context = NULL;
	PG_RETURN_BYTEA_P(result);
#endif
}


PG_FUNCTION_INFO_V1(pgis_asmvt_deserialfn);
Datum pgis_asmvt_deserialfn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVT: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext, oldcontext;
	mvt_agg_context *ctx;
	elog(DEBUG2, "%s called", __func__);
	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	oldcontext = MemoryContextSwitchTo(aggcontext);
	ctx = mvt_ctx_deserialize(PG_GETARG_BYTEA_P(0));
	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_POINTER(ctx);
#endif
}

PG_FUNCTION_INFO_V1(pgis_asmvt_combinefn);
Datum pgis_asmvt_combinefn(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	elog(ERROR, "ST_AsMVT: Compiled without protobuf-c support");
	PG_RETURN_NULL();
#else
	MemoryContext aggcontext, oldcontext;
	mvt_agg_context *ctx, *ctx1, *ctx2;
	elog(DEBUG2, "%s called", __func__);
	if (!AggCheckCallContext(fcinfo, &aggcontext))
		elog(ERROR, "%s called in non-aggregate context", __func__);

	ctx1 = (mvt_agg_context*)PG_GETARG_POINTER(0);
	ctx2 = (mvt_agg_context*)PG_GETARG_POINTER(1);
	oldcontext = MemoryContextSwitchTo(aggcontext);
	ctx = mvt_ctx_combine(ctx1, ctx2);
	MemoryContextSwitchTo(oldcontext);
	PG_RETURN_POINTER(ctx);
#endif
}

