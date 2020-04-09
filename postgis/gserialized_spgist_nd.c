/*****************************************************************************
 *
 * gserialized_spgist_nd.c
 *	  SP-GiST implementation of 2N-dimensional oct-tree over GIDX boxes
 *
 * This module provides SP-GiST implementation for N-dimensional boxes using
 * oct-tree analogy in 2N-dimensional space.  SP-GiST doesn't allow indexing
 * of overlapping objects.  We make N-dimensional objects never-overlapping
 * in 2N-dimensional space.  This technique has some benefits compared to
 * traditional R-Tree which is implemented as GiST.  The performance tests
 * reveal that this technique especially beneficial with too much overlapping
 * objects, so called "spaghetti data".
 *
 * Unlike the original oct-tree, we are splitting the tree 2N times in
 * N-dimensional space. For example, if N = 3 then it is easier to imagine
 * it as splitting space three times into 3:
 *
 *				|	   |						|	   |
 *				|	   |						|	   |
 *				| -----+-----					| -----+-----
 *				|	   |						|	   |
 *				|	   |						|	   |
 * -------------+------------- -+- -------------+-------------
 *				|								|
 *				|								|
 *				|								|
 *				|								|
 *				|								|
 *			  FRONT							  BACK
 *
 * We are using GIDX data type as the prefix, but we are treating them
 * as points in 2N-dimensional space, because GIDX boxes are not enough
 * to represent the octant boundaries in ND space.  They however are
 * sufficient to point out the additional boundaries of the next
 * octant.
 *
 * We are using traversal values provided by SP-GiST to calculate and
 * to store the bounds of the octants, while traversing into the tree.
 * Traversal value has all the boundaries in the ND space, and is
 * capable of transferring the required boundaries to the following
 * traversal values.  In conclusion, three things are necessary
 * to calculate the next traversal value:
 *
 *	(1) the traversal value of the parent
 *	(2) the octant of the current node
 *	(3) the prefix of the current node
 *
 * If we visualize them on our simplified drawing (see the drawing above);
 * transferred boundaries of (1) would be the outer axis, relevant part
 * of (2) would be the up range_y part of the other axis, and (3) would be
 * the inner axis.
 *
 * For example, consider the case of overlapping.  When recursion
 * descends deeper and deeper down the tree, all octants in
 * the current node will be checked for overlapping.  The boundaries
 * will be re-calculated for all octants.  Overlap check answers
 * the question: can any box from this octant overlap with the given
 * box?  If yes, then this octant will be walked.  If no, then this
 * octant will be skipped.
 *
 * This method provides restrictions for minimum and maximum values of
 * every dimension of every corner of the box on every level of the tree
 * except the root.  For the root node, we are setting the boundaries
 * that we don't yet have as infinity.
 *
 * Portions Copyright (c) 2018, Esteban Zimanyi, Arthur Lesuisse,
 * 		Université Libre de Bruxelles
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *****************************************************************************/

#include "gserialized_spgist_3d.h"
#include "lwgeom_pg.h"
#include "gserialized_gist.h"

/* Maximum number of children nodes given that GIDX_MAX_DIM = 4 */
#define GIDX_MAX_NODES 256

/*
 * ND SP-GiST prototypes
 */

Datum gserialized_spgist_config_nd(PG_FUNCTION_ARGS);
Datum gserialized_spgist_choose_nd(PG_FUNCTION_ARGS);
Datum gserialized_spgist_picksplit_nd(PG_FUNCTION_ARGS);
Datum gserialized_spgist_inner_consistent_nd(PG_FUNCTION_ARGS);
Datum gserialized_spgist_leaf_consistent_nd(PG_FUNCTION_ARGS);
Datum gserialized_spgist_compress_nd(PG_FUNCTION_ARGS);

/* Structure storing the n-dimensional bounding box */

typedef struct
{
	GIDX *left;
	GIDX *right;
} CubeGIDX;

/*
 * Comparator for qsort
 *
 * We don't need to use the floating point macros in here, because this
 * is only going to be used in a place to effect the performance
 * of the index, not the correctness.
 */
static int
compareFloats(const void *a, const void *b)
{
	float x = *(float *)a;
	float y = *(float *)b;

	if (x == y)
		return 0;
	return (x > y) ? 1 : -1;
}

/*
 * Calculate the octant
 *
 * The octant is a 16-bit unsigned integer with 8 bits in use. The function
 * accepts 2 GIDX as input. The 8 bits are set by comparing a corner of the
 * box. This makes 256 octants in total.
 */
static uint16_t
getOctant(GIDX *centroid, GIDX *inBox)
{
	uint16_t octant = 0, dim = 0x01;
	int ndims, i;

	ndims = Min(GIDX_NDIMS(centroid), GIDX_NDIMS(inBox));

	for (i = 0; i < ndims; i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(centroid, i) != FLT_MAX && GIDX_GET_MAX(inBox, i) != FLT_MAX)
		{
			if (GIDX_GET_MAX(inBox, i) > GIDX_GET_MAX(centroid, i))
				octant |= dim;
			dim = dim << 1;
			if (GIDX_GET_MIN(inBox, i) > GIDX_GET_MIN(centroid, i))
				octant |= dim;
			dim = dim << 1;
		}
	}

	return octant;
}

/*
 * Initialize the traversal value
 *
 * In the beginning, we don't have any restrictions.  We have to
 * initialize the struct to cover the whole ND space.
 */
static CubeGIDX *
initCubeBox(int ndims)
{
	CubeGIDX *cube_box = (CubeGIDX *)palloc(sizeof(CubeGIDX));
	GIDX *left = (GIDX *)palloc(GIDX_SIZE(ndims));
	GIDX *right = (GIDX *)palloc(GIDX_SIZE(ndims));
	int i;

	SET_VARSIZE(left, GIDX_SIZE(ndims));
	SET_VARSIZE(right, GIDX_SIZE(ndims));
	cube_box->left = left;
	cube_box->right = right;

	for (i = 0; i < ndims; i++)
	{
		GIDX_SET_MIN(cube_box->left, i, -1 * FLT_MAX);
		GIDX_SET_MAX(cube_box->left, i, FLT_MAX);
		GIDX_SET_MIN(cube_box->right, i, -1 * FLT_MAX);
		GIDX_SET_MAX(cube_box->right, i, FLT_MAX);
	}

	return cube_box;
}

/*
 * Calculate the next traversal value
 *
 * All centroids are bounded by CubeGIDX, but SP-GiST only keeps
 * boxes.  When we are traversing the tree, we must calculate CubeGIDX,
 * using centroid and octant.
 */
static CubeGIDX *
nextCubeBox(CubeGIDX *cube_box, GIDX *centroid, uint16_t octant)
{
	int ndims = GIDX_NDIMS(centroid), i;
	CubeGIDX *next_cube_box = (CubeGIDX *)palloc(sizeof(CubeGIDX));
	GIDX *left = (GIDX *)palloc(GIDX_SIZE(ndims));
	GIDX *right = (GIDX *)palloc(GIDX_SIZE(ndims));
	uint16_t dim = 0x01;

	memcpy(left, cube_box->left, VARSIZE(cube_box->left));
	memcpy(right, cube_box->right, VARSIZE(cube_box->right));
	next_cube_box->left = left;
	next_cube_box->right = right;

	for (i = 0; i < ndims; i++)
	{
		if (GIDX_GET_MAX(cube_box->left, i) != FLT_MAX && GIDX_GET_MAX(centroid, i) != FLT_MAX)
		{
			if (octant & dim)
				GIDX_GET_MIN(next_cube_box->right, i) = GIDX_GET_MAX(centroid, i);
			else
				GIDX_GET_MAX(next_cube_box->right, i) = GIDX_GET_MAX(centroid, i);

			dim = dim << 1;

			if (octant & dim)
				GIDX_GET_MIN(next_cube_box->left, i) = GIDX_GET_MIN(centroid, i);
			else
				GIDX_GET_MAX(next_cube_box->left, i) = GIDX_GET_MIN(centroid, i);

			dim = dim << 1;
		}
	}

	return next_cube_box;
}

/* Can any cube from cube_box overlap with query? */
static bool
overlapND(CubeGIDX *cube_box, GIDX *query)
{
	int i, ndims;
	bool result = true;

	ndims = Min(GIDX_NDIMS(cube_box->left), GIDX_NDIMS(query));

	for (i = 0; i < ndims; i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(cube_box->left, i) != FLT_MAX && GIDX_GET_MAX(query, i) != FLT_MAX)
			result &= (GIDX_GET_MIN(cube_box->left, i) <= GIDX_GET_MAX(query, i)) &&
				  (GIDX_GET_MAX(cube_box->right, i) >= GIDX_GET_MIN(query, i));
	}
	return result;
}

/* Can any cube from cube_box contain query? */
static bool
containND(CubeGIDX *cube_box, GIDX *query)
{
	int i, ndims;
	bool result = true;

	ndims = Min(GIDX_NDIMS(cube_box->left), GIDX_NDIMS(query));

	for (i = 0; i < ndims; i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(cube_box->left, i) != FLT_MAX && GIDX_GET_MAX(query, i) != FLT_MAX)
			result &= (GIDX_GET_MAX(cube_box->right, i) >= GIDX_GET_MAX(query, i)) &&
				  (GIDX_GET_MIN(cube_box->left, i) <= GIDX_GET_MIN(query, i));
	}
	return result;
}

/*
 * SP-GiST config function
 */

PG_FUNCTION_INFO_V1(gserialized_spgist_config_nd);

PGDLLEXPORT Datum gserialized_spgist_config_nd(PG_FUNCTION_ARGS)
{
	spgConfigOut *cfg = (spgConfigOut *)PG_GETARG_POINTER(1);
	Oid boxoid = InvalidOid;
	postgis_initialize_cache(fcinfo);
	boxoid = postgis_oid(GIDXOID);

	cfg->prefixType = boxoid;
	cfg->labelType = VOIDOID; /* We don't need node labels. */
	cfg->leafType = boxoid;
	cfg->canReturnData = false;
	cfg->longValuesOK = false;

	PG_RETURN_VOID();
}

/*
 * SP-GiST choose function
 */

PG_FUNCTION_INFO_V1(gserialized_spgist_choose_nd);

PGDLLEXPORT Datum gserialized_spgist_choose_nd(PG_FUNCTION_ARGS)
{
	spgChooseIn *in = (spgChooseIn *)PG_GETARG_POINTER(0);
	spgChooseOut *out = (spgChooseOut *)PG_GETARG_POINTER(1);
	GIDX *centroid = (GIDX *)DatumGetPointer(in->prefixDatum), *box = (GIDX *)DatumGetPointer(in->leafDatum);

	out->resultType = spgMatchNode;
	out->result.matchNode.restDatum = PointerGetDatum(box);

	/* nodeN will be set by core, when allTheSame. */
	if (!in->allTheSame)
		out->result.matchNode.nodeN = getOctant(centroid, box);

	PG_RETURN_VOID();
}

/*
 * SP-GiST pick-split function
 *
 * It splits a list of boxes into octants by choosing a central ND
 * point as the median of the coordinates of the boxes.
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_picksplit_nd);

PGDLLEXPORT Datum gserialized_spgist_picksplit_nd(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *)PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *)PG_GETARG_POINTER(1);
	GIDX *box, *centroid;
	float *lowXs, *highXs;
	int ndims, maxdims = -1, count[GIDX_MAX_DIM], median, dim, tuple;

	for (dim = 0; dim < GIDX_MAX_DIM; dim++)
		count[dim] = 0;

	lowXs = palloc(sizeof(float) * in->nTuples * GIDX_MAX_DIM),
	highXs = palloc(sizeof(float) * in->nTuples * GIDX_MAX_DIM);

	/* Calculate maxdims median of all ND coordinates */
	for (tuple = 0; tuple < in->nTuples; tuple++)
	{
		box = (GIDX *)DatumGetPointer(in->datums[tuple]);
		ndims = GIDX_NDIMS(box);
		if (maxdims < ndims)
			maxdims = ndims;
		for (dim = 0; dim < ndims; dim++)
		{
			/* If the missing dimension was not padded with -+FLT_MAX */
			if (GIDX_GET_MAX(box, dim) != FLT_MAX)
			{
				lowXs[dim * in->nTuples + count[dim]] = GIDX_GET_MIN(box, dim);
				highXs[dim * in->nTuples + count[dim]] = GIDX_GET_MAX(box, dim);
				count[dim]++;
			}
		}
	}

	for (dim = 0; dim < maxdims; dim++)
	{
		qsort(&lowXs[dim * in->nTuples], count[dim], sizeof(float), compareFloats);
		qsort(&highXs[dim * in->nTuples], count[dim], sizeof(float), compareFloats);
	}

	centroid = (GIDX *)palloc(GIDX_SIZE(maxdims));
	SET_VARSIZE(centroid, GIDX_SIZE(maxdims));

	for (dim = 0; dim < maxdims; dim++)
	{
		median = count[dim] / 2;
		GIDX_SET_MIN(centroid, dim, lowXs[dim * in->nTuples + median]);
		GIDX_SET_MAX(centroid, dim, highXs[dim * in->nTuples + median]);
	}

	/* Fill the output */
	out->hasPrefix = true;
	out->prefixDatum = PointerGetDatum(gidx_copy(centroid));

	out->nNodes = 0x01 << (2 * maxdims);
	out->nodeLabels = NULL; /* We don't need node labels. */

	out->mapTuplesToNodes = palloc(sizeof(int) * in->nTuples);
	out->leafTupleDatums = palloc(sizeof(Datum) * in->nTuples);

	/*
	 * Assign ranges to corresponding nodes according to octants relative to
	 * the "centroid" range
	 */
	for (tuple = 0; tuple < in->nTuples; tuple++)
	{
		GIDX *box = (GIDX *)DatumGetPointer(in->datums[tuple]);
		uint16_t octant = getOctant(centroid, box);

		out->leafTupleDatums[tuple] = PointerGetDatum(box);
		out->mapTuplesToNodes[tuple] = octant;
	}

	pfree(lowXs);
	pfree(highXs);

	PG_RETURN_VOID();
}

/*
 * SP-GiST inner consistent function
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_inner_consistent_nd);

PGDLLEXPORT Datum gserialized_spgist_inner_consistent_nd(PG_FUNCTION_ARGS)
{
	spgInnerConsistentIn *in = (spgInnerConsistentIn *)PG_GETARG_POINTER(0);
	spgInnerConsistentOut *out = (spgInnerConsistentOut *)PG_GETARG_POINTER(1);
	MemoryContext old_ctx;
	CubeGIDX *cube_box;
	int *nodeNumbers, i, j;
	void **traversalValues;
	char gidxmem[GIDX_MAX_SIZE];
	GIDX *centroid, *query_gbox_index = (GIDX *)gidxmem;

	POSTGIS_DEBUG(4, "[SPGIST] 'inner consistent' function called");

	if (in->allTheSame)
	{
		/* Report that all nodes should be visited */
		out->nNodes = in->nNodes;
		out->nodeNumbers = (int *)palloc(sizeof(int) * in->nNodes);
		for (i = 0; i < in->nNodes; i++)
			out->nodeNumbers[i] = i;

		PG_RETURN_VOID();
	}

	/*
	 * We switch memory context, because we want to allocate memory for new
	 * traversal values (next_cube_box) and pass these pieces of memory to
	 * further call of this function.
	 */
	old_ctx = MemoryContextSwitchTo(in->traversalMemoryContext);

	centroid = (GIDX *)DatumGetPointer(in->prefixDatum);

	/*
	 * We are saving the traversal value or initialize it an unbounded one, if
	 * we have just begun to walk the tree.
	 */
	if (in->traversalValue)
		cube_box = in->traversalValue;
	else
		cube_box = initCubeBox(GIDX_NDIMS(centroid));

	/* Allocate enough memory for nodes */
	out->nNodes = 0;
	nodeNumbers = (int *)palloc(sizeof(int) * in->nNodes);
	traversalValues = (void **)palloc(sizeof(void *) * in->nNodes);

	for (i = 0; i < in->nNodes; i++)
	{
		CubeGIDX *next_cube_box = nextCubeBox(cube_box, centroid, (uint16_t)i);
		bool flag = true;

		for (j = 0; j < in->nkeys; j++)
		{
			StrategyNumber strategy = in->scankeys[j].sk_strategy;
			Datum query = in->scankeys[j].sk_argument;

			/* Quick sanity check on query argument. */
			if (DatumGetPointer(query) == NULL)
			{
				POSTGIS_DEBUG(4, "[SPGIST] null query pointer (!?!)");
				flag = false;
				break;
			}

			/* Null box should never make this far. */
			if (gserialized_datum_get_gidx_p(query, query_gbox_index) == LW_FAILURE)
			{
				POSTGIS_DEBUG(4, "[SPGIST] null query_gbox_index!");
				flag = false;
				break;
			}

			switch (strategy)
			{
			case SPGOverlapStrategyNumber:
			case SPGContainedByStrategyNumber:
				flag = overlapND(next_cube_box, query_gbox_index);
				break;

			case SPGContainsStrategyNumber:
			case SPGSameStrategyNumber:
				flag = containND(next_cube_box, query_gbox_index);
				break;

			default:
				elog(ERROR, "unrecognized strategy: %d", strategy);
			}

			/* If any check is failed, we have found our answer. */
			if (!flag)
				break;
		}

		if (flag)
		{
			traversalValues[out->nNodes] = next_cube_box;
			nodeNumbers[out->nNodes] = i;
			out->nNodes++;
		}
		else
		{
			/*
			 * If this node is not selected, we don't need to keep the next
			 * traversal value in the memory context.
			 */
			pfree(next_cube_box);
		}
	}

	/* Pass to the next level only the values that need to be traversed */
	out->nodeNumbers = (int *)palloc(sizeof(int) * out->nNodes);
	out->traversalValues = (void **)palloc(sizeof(void *) * out->nNodes);
	for (i = 0; i < out->nNodes; i++)
	{
		out->nodeNumbers[i] = nodeNumbers[i];
		out->traversalValues[i] = traversalValues[i];
	}
	pfree(nodeNumbers);
	pfree(traversalValues);

	/* Switch after */
	MemoryContextSwitchTo(old_ctx);

	PG_RETURN_VOID();
}

/*
 * SP-GiST inner consistent function
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_leaf_consistent_nd);

PGDLLEXPORT Datum gserialized_spgist_leaf_consistent_nd(PG_FUNCTION_ARGS)
{
	spgLeafConsistentIn *in = (spgLeafConsistentIn *)PG_GETARG_POINTER(0);
	spgLeafConsistentOut *out = (spgLeafConsistentOut *)PG_GETARG_POINTER(1);
	bool flag = true;
	int i;
	char gidxmem[GIDX_MAX_SIZE];
	GIDX *leaf = (GIDX *)DatumGetPointer(in->leafDatum), *query_gbox_index = (GIDX *)gidxmem;

	POSTGIS_DEBUG(4, "[SPGIST] 'leaf consistent' function called");

	/* All tests are exact. */
	out->recheck = false;

	/* leafDatum is what it is... */
	out->leafValue = in->leafDatum;

	/* Perform the required comparison(s) */
	for (i = 0; i < in->nkeys; i++)
	{
		StrategyNumber strategy = in->scankeys[i].sk_strategy;
		Datum query = in->scankeys[i].sk_argument;

		/* Quick sanity check on query argument. */
		if (DatumGetPointer(query) == NULL)
		{
			POSTGIS_DEBUG(4, "[SPGIST] null query pointer (!?!)");
			flag = false;
		}

		/* Null box should never make this far. */
		if (gserialized_datum_get_gidx_p(query, query_gbox_index) == LW_FAILURE)
		{
			POSTGIS_DEBUG(4, "[SPGIST] null query_gbox_index!");
			flag = false;
		}

		switch (strategy)
		{
		case SPGOverlapStrategyNumber:
			flag = gidx_overlaps(leaf, query_gbox_index);
			break;

		case SPGContainsStrategyNumber:
			flag = gidx_contains(leaf, query_gbox_index);
			break;

		case SPGContainedByStrategyNumber:
			flag = gidx_contains(query_gbox_index, leaf);
			break;

		case SPGSameStrategyNumber:
			flag = gidx_equals(leaf, query_gbox_index);
			break;

		default:
			elog(ERROR, "unrecognized strategy: %d", strategy);
		}

		/* If any check is failed, we have found our answer. */
		if (!flag)
			break;
	}

	PG_RETURN_BOOL(flag);
}

PG_FUNCTION_INFO_V1(gserialized_spgist_compress_nd);

PGDLLEXPORT Datum gserialized_spgist_compress_nd(PG_FUNCTION_ARGS)
{
	char gidxmem[GIDX_MAX_SIZE];
	GIDX *result = (GIDX *)gidxmem;
	long unsigned int i;

	POSTGIS_DEBUG(4, "[SPGIST] 'compress' function called");

	/* Input entry is null? Return NULL. */
	if (PG_ARGISNULL(0))
	{
		POSTGIS_DEBUG(4, "[SPGIST] null entry (!?!)");
		PG_RETURN_NULL();
	}

	/* Is the bounding box valid (non-empty, non-infinite) ?
	 * If not, return NULL. */
	if (gserialized_datum_get_gidx_p(PG_GETARG_DATUM(0), result) == LW_FAILURE)
	{
		POSTGIS_DEBUG(4, "[SPGIST] empty geometry!");
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(4, "[SPGIST] got GIDX: %s", gidx_to_string(result));

	/* Check all the dimensions for finite values.
	 * If not, use the "unknown" GIDX as a key */
	for (i = 0; i < GIDX_NDIMS(result); i++)
	{
		if (!isfinite(GIDX_GET_MAX(result, i)) || !isfinite(GIDX_GET_MIN(result, i)))
		{
			gidx_set_unknown(result);
			PG_RETURN_POINTER(result);
		}
	}

	/* Ensure bounding box has minimums below maximums. */
	gidx_validate(result);

	/* Return GIDX. */
	PG_RETURN_POINTER(gidx_copy(result));
}

/*****************************************************************************/
