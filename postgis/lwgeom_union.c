#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "postgres.h"
#include "fmgr.h"
#include "utils/varlena.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_log.h"
#include "lwgeom_pg.h"
#include "lwgeom_union.h"

#define GetAggContext(aggcontext) \
	if (!AggCheckCallContext(fcinfo, aggcontext)) \
		elog(ERROR, "%s called in non-aggregate context", __func__)

#define CheckAggContext() GetAggContext(NULL)


Datum pgis_geometry_union_parallel_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_parallel_combinefn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_parallel_serialfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_parallel_deserialfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_parallel_finalfn(PG_FUNCTION_ARGS);

static UnionState* state_create(void);
static void state_append(UnionState *state, const GSERIALIZED *gser);
static bytea* state_serialize(const UnionState *state);
static UnionState* state_deserialize(const bytea* serialized);
static void state_combine(UnionState *state1, UnionState *state2);

static LWGEOM* gserialized_list_union(List* list, float8 gridSize);


PG_FUNCTION_INFO_V1(pgis_geometry_union_parallel_transfn);
Datum pgis_geometry_union_parallel_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext, old;
	UnionState *state;
	Datum argType;
	GSERIALIZED *gser = NULL;

	/* Check argument type */
	argType = get_fn_expr_argtype(fcinfo->flinfo, 1);
	if (argType == InvalidOid)
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("%s: could not determine input data type", __func__)));

	/* Get memory context */
	GetAggContext(&aggcontext);

	/* Get state */
	if (PG_ARGISNULL(0)) {
		old = MemoryContextSwitchTo(aggcontext);
		state = state_create();
		MemoryContextSwitchTo(old);
	}
	else
	{
		state = (UnionState*) PG_GETARG_POINTER(0);
	}

	/* Get value */
	if (!PG_ARGISNULL(1))
		gser = PG_GETARG_GSERIALIZED_P(1);

	/* Get grid size */
	if (PG_NARGS() > 2 && !PG_ARGISNULL(2))
	{
		double gridSize = PG_GETARG_FLOAT8(2);
		if (gridSize > 0)
			state->gridSize = gridSize;
	}

	/* Copy serialized geometry into state */
	if (gser) {
		old = MemoryContextSwitchTo(aggcontext);
		state_append(state, gser);
		MemoryContextSwitchTo(old);
	}

	PG_RETURN_POINTER(state);
}


PG_FUNCTION_INFO_V1(pgis_geometry_union_parallel_combinefn);
Datum pgis_geometry_union_parallel_combinefn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext, old;
	UnionState *state1 = NULL;
	UnionState *state2 = NULL;

	if (!PG_ARGISNULL(0))
		state1 = (UnionState*) PG_GETARG_POINTER(0);
	if (!PG_ARGISNULL(1))
		state2 = (UnionState*) PG_GETARG_POINTER(1);

	GetAggContext(&aggcontext);

	if (state1 && state2)
	{
		old = MemoryContextSwitchTo(aggcontext);
		state_combine(state1, state2);
		lwfree(state2);
		MemoryContextSwitchTo(old);
	}
	else if (state2)
	{
		state1 = state2;
	}

	if (!state1)
		PG_RETURN_NULL();
	PG_RETURN_POINTER(state1);
}


PG_FUNCTION_INFO_V1(pgis_geometry_union_parallel_serialfn);
Datum pgis_geometry_union_parallel_serialfn(PG_FUNCTION_ARGS)
{
	UnionState *state;

	CheckAggContext();

	state = (UnionState*) PG_GETARG_POINTER(0);
	PG_RETURN_BYTEA_P(state_serialize(state));
}


PG_FUNCTION_INFO_V1(pgis_geometry_union_parallel_deserialfn);
Datum pgis_geometry_union_parallel_deserialfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext, old;
	UnionState *state;
	bytea *serialized;

	GetAggContext(&aggcontext);

	serialized = PG_GETARG_BYTEA_P(0);

	old = MemoryContextSwitchTo(aggcontext);
	state = state_deserialize(serialized);
	MemoryContextSwitchTo(old);

	PG_RETURN_POINTER(state);
}


PG_FUNCTION_INFO_V1(pgis_geometry_union_parallel_finalfn);
Datum pgis_geometry_union_parallel_finalfn(PG_FUNCTION_ARGS)
{
	UnionState *state;
	LWGEOM *geom = NULL;

	CheckAggContext();

	state = (UnionState*)PG_GETARG_POINTER(0);
	geom = gserialized_list_union(state->list, state->gridSize);
	if (!geom)
		PG_RETURN_NULL();
	PG_RETURN_POINTER(geometry_serialize(geom));
}


UnionState* state_create(void)
{
	UnionState *state = lwalloc(sizeof(UnionState));
	state->gridSize = -1.0;
	state->list = NIL;
	state->size = 0;
	return state;
}


void state_append(UnionState *state, const GSERIALIZED *gser)
{
	GSERIALIZED *gser_copy;

	assert(gser);
	gser_copy = lwalloc(VARSIZE(gser));
	memcpy(gser_copy, gser, VARSIZE(gser));

	state->list = lappend(state->list, gser_copy);
	state->size += VARSIZE(gser);
}


bytea* state_serialize(const UnionState *state)
{
	int32 size = VARHDRSZ + sizeof(state->gridSize) + state->size;
	bytea *serialized = lwalloc(size);
	uint8 *data;
	ListCell *cell;

	SET_VARSIZE(serialized, size);
	data = (uint8*)VARDATA(serialized);

	/* grid size */
	memcpy(data, &state->gridSize, sizeof(state->gridSize));
	data += sizeof(state->gridSize);

	/* items */
	foreach (cell, state->list)
	{
		const GSERIALIZED *gser = (const GSERIALIZED*)lfirst(cell);
		assert(gser);
		memcpy(data, gser, VARSIZE(gser));
		data += VARSIZE(gser);
	}

	return serialized;
}


UnionState* state_deserialize(const bytea* serialized)
{
	UnionState *state = state_create();
	const uint8 *data = (const uint8*)VARDATA(serialized);
	const uint8 *data_end = (const uint8*)serialized + VARSIZE(serialized);

	/* grid size */
	memcpy(&state->gridSize, data, sizeof(state->gridSize));
	data += sizeof(state->gridSize);

	/* items */
	while (data < data_end)
	{
		const GSERIALIZED* gser = (const GSERIALIZED*)data;
		state_append(state, gser);
		data += VARSIZE(gser);
	}

	return state;
}


void state_combine(UnionState *state1, UnionState *state2)
{
	List *list1;
	List *list2;

	assert(state1 && state2);

	list1 = state1->list;
	list2 = state2->list;

	if (list1 != NIL && list2 != NIL)
	{
		state1->list = list_concat(list1, list2);
		/** Took out to prevent crash https://trac.osgeo.org/postgis/ticket/5371 **/
		//list_free(list2);
		state1->size += state2->size;
	}
	else if (list2 != NIL)
	{
		state1->gridSize = state2->gridSize;
		state1->list = list2;
		state1->size = state2->size;
	}
	state2->list = NIL;
}


LWGEOM* gserialized_list_union(List* list, float8 gridSize)
{
	int ngeoms = 0;
	LWGEOM **geoms;
	int32_t srid = SRID_UNKNOWN;
	bool first = true;
	int empty_type = 0;
	int has_z = LW_FALSE;
	ListCell *cell;

	if (list_length(list) == 0)
		return NULL;

	geoms = lwalloc(list_length(list) * sizeof(LWGEOM*));
	foreach (cell, list)
	{
		GSERIALIZED *gser;
		LWGEOM *geom;

		gser = (GSERIALIZED*)lfirst(cell);
		assert(gser);
		geom = lwgeom_from_gserialized(gser);

		if (!lwgeom_is_empty(geom))
		{
			geoms[ngeoms++] = geom; /* no cloning */
			if (first)
			{
				srid = lwgeom_get_srid(geom);
				has_z = lwgeom_has_z(geom);
				first = false;
			}
		}
		else
		{
			int type = lwgeom_get_type(geom);
			if (type > empty_type)
				empty_type = type;
			if (srid == SRID_UNKNOWN)
				srid = lwgeom_get_srid(geom);
		}
	}

	if (ngeoms > 0)
	{
		/*
		 * Create a collection and pass it into cascaded union
		 */
		LWCOLLECTION *col = lwcollection_construct(COLLECTIONTYPE, srid, NULL, ngeoms, geoms);
		LWGEOM *result = lwgeom_unaryunion_prec(lwcollection_as_lwgeom(col), gridSize);
		if (!result)
			lwcollection_free(col);
		return result;
	}

	/*
	 * Only empty geometries in the list,
	 * create geometry with largest type number or return NULL
	 */
	return (empty_type > 0)
		? lwgeom_construct_empty(empty_type, srid, has_z, 0)
		: NULL;
}
