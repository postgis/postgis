/*****************************************************************************
 *
 * gserialized_spgist_2d.c
 *	  SP-GiST implementation of 4-dimensional quad-tree over box2df
 *
 * This module provides SP-GiST implementation for boxes using quad tree
 * analogy in 4-dimensional space.  SP-GiST doesn't allow indexing of
 * overlapping objects.  We are making objects never-overlapping in
 * 4D space.  This technique has some benefits compared to traditional
 * R-Tree which is implemented as GiST.  The performance tests reveal
 * that this technique especially beneficial with too much overlapping
 * objects, so called "spaghetti data".
 *
 * Unlike the original quad-tree, we are splitting the tree into 16
 * quadrants in 4D space.  It is easier to imagine it as splitting space
 * two times into 2:
 *
 *				|	   |
 *				|	   |
 *				| -----+-----
 *				|	   |
 *				|	   |
 * -------------+-------------
 *				|
 *				|
 *				|
 *				|
 *				|
 *			  FRONT
 *
 * We are using box data type as the prefix, but we are treating them
 * as points in 4-dimensional space, because  boxes are not enough
 * to represent the quadrant boundaries in 4D space.  They however are
 * sufficient to point out the additional boundaries of the next
 * quadrant.
 *
 * We are using traversal values provided by SP-GiST to calculate and
 * to store the bounds of the quadrants, while traversing into the tree.
 * Traversal value has all the boundaries in the 4D space, and is is
 * capable of transferring the required boundaries to the following
 * traversal values.  In conclusion, three things are necessary
 * to calculate the next traversal value:
 *
 *	(1) the traversal value of the parent
 *	(2) the quadrant of the current node
 *	(3) the prefix of the current node
 *
 * If we visualize them on our simplified drawing (see the drawing above);
 * transferred boundaries of (1) would be the outer axis, relevant part
 * of (2) would be the up range_y part of the other axis, and (3) would be
 * the inner axis.
 *
 * For example, consider the case of overlapping.  When recursion
 * descends deeper and deeper down the tree, all quadrants in
 * the current node will be checked for overlapping.  The boundaries
 * will be re-calculated for all quadrants.  Overlap check answers
 * the question: can any box from this quadrant overlap with the given
 * box?  If yes, then this quadrant will be walked.  If no, then this
 * quadrant will be skipped.
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

#include <postgres.h>
#include <utils/builtins.h>    /* For float manipulation */
#include "access/spgist.h"     /* For SP-GiST */
#include "catalog/pg_type_d.h" /* For VOIDOID */

#include "../postgis_config.h"

#include "liblwgeom.h"        /* For standard geometry types. */
#include "lwgeom_pg.h"        /* For debugging macros. */
#include <gserialized_gist.h> /* For utility functions. */
#include <lwgeom_pg.h>        /* For debugging macros. */
#include <math.h>

/*
** SP-GiST 2D index function prototypes
*/

Datum gserialized_spgist_config_2d(PG_FUNCTION_ARGS);
Datum gserialized_spgist_choose_2d(PG_FUNCTION_ARGS);
Datum gserialized_spgist_picksplit_2d(PG_FUNCTION_ARGS);
Datum gserialized_spgist_inner_consistent_2d(PG_FUNCTION_ARGS);
Datum gserialized_spgist_leaf_consistent_2d(PG_FUNCTION_ARGS);
Datum gserialized_spgist_compress_2d(PG_FUNCTION_ARGS);

/*
 * Comparator for qsort
 *
 * We don't need to use the floating point macros in here, because this
 * is only going to be used in a place to effect the performance
 * of the index, not the correctness.
 */
static int
compareDoubles(const void *a, const void *b)
{
	double x = *(double *)a;
	double y = *(double *)b;

	if (x == y)
		return 0;
	return (x > y) ? 1 : -1;
}

typedef struct
{
	BOX2DF left;
	BOX2DF right;
} RectBox;

/*
 * Calculate the quadrant
 *
 * The quadrant is 4 bit unsigned integer with 4 least bits in use.
 * This function accepts 2 BOX2DF as input.  All 4 bits are set by comparing
 * a corner of the box. This makes 16 quadrants in total.
 */
static uint8
getQuadrant4D(BOX2DF *centroid, BOX2DF *inBox)
{
	uint8 quadrant = 0;

	if (inBox->xmin > centroid->xmin)
		quadrant |= 0x8;

	if (inBox->xmax > centroid->xmax)
		quadrant |= 0x4;

	if (inBox->ymin > centroid->ymin)
		quadrant |= 0x2;

	if (inBox->ymax > centroid->ymax)
		quadrant |= 0x1;

	return quadrant;
}

/*
 * Initialize the traversal value
 *
 * In the beginning, we don't have any restrictions.  We have to
 * initialize the struct to cover the whole 4D space.
 */
static RectBox *
initRectBox(void)
{
	RectBox *rect_box = (RectBox *)palloc(sizeof(RectBox));
	float infinity = FLT_MAX;

	rect_box->left.xmin = -infinity;
	rect_box->left.xmax = infinity;

	rect_box->left.ymin = -infinity;
	rect_box->left.ymax = infinity;

	rect_box->right.xmin = -infinity;
	rect_box->right.xmax = infinity;

	rect_box->right.ymin = -infinity;
	rect_box->right.ymax = infinity;

	return rect_box;
}

/*
 * Calculate the next traversal value
 *
 * All centroids are bounded by RectBox, but SP-GiST only keeps
 * boxes.  When we are traversing the tree, we must calculate RectBox,
 * using centroid and quadrant.
 */
static RectBox *
nextRectBox(RectBox *rect_box, BOX2DF *centroid, uint8 quadrant)
{
	RectBox *next_rect_box = (RectBox *)palloc(sizeof(RectBox));

	memcpy(next_rect_box, rect_box, sizeof(RectBox));

	if (quadrant & 0x8)
		next_rect_box->left.xmin = centroid->xmin;
	else
		next_rect_box->left.xmax = centroid->xmin;

	if (quadrant & 0x4)
		next_rect_box->right.xmin = centroid->xmax;
	else
		next_rect_box->right.xmax = centroid->xmax;

	if (quadrant & 0x2)
		next_rect_box->left.ymin = centroid->ymin;
	else
		next_rect_box->left.ymax = centroid->ymin;

	if (quadrant & 0x1)
		next_rect_box->right.ymin = centroid->ymax;
	else
		next_rect_box->right.ymax = centroid->ymax;

	return next_rect_box;
}

/* Can any cube from rect_box overlap with query? */
static bool
overlap4D(RectBox *rect_box, BOX2DF *query)
{
	return (rect_box->left.xmin <= query->xmax && rect_box->right.xmax >= query->xmin) &&
	       (rect_box->left.ymin <= query->ymax && rect_box->right.ymax >= query->ymin);
}

/* Can any cube from rect_box contain query? */
static bool
contain4D(RectBox *rect_box, BOX2DF *query)
{
	return (rect_box->right.xmax >= query->xmax && rect_box->left.xmin <= query->xmin) &&
	       (rect_box->right.ymax >= query->ymax && rect_box->left.ymin <= query->ymin);
}

/* Can any cube from rect_box be left of query? */
static bool
left4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->right.xmax <= query->xmin;
}

/* Can any cube from rect_box does not extend the right of query? */
static bool
overLeft4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->right.xmax <= query->xmax;
}

/* Can any cube from rect_box be right of query? */
static bool
right4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->left.xmin >= query->xmax;
}

/* Can any cube from rect_box does not extend the left of query? */
static bool
overRight4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->left.xmin >= query->xmin;
}

/* Can any cube from rect_box be below of query? */
static bool
below4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->right.ymax <= query->ymin;
}

/* Can any cube from rect_box does not extend above query? */
static bool
overBelow4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->right.ymax <= query->ymax;
}

/* Can any cube from rect_box be above of query? */
static bool
above4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->left.ymin >= query->ymax;
}

/* Can any cube from rect_box does not extend below of query? */
static bool
overAbove4D(RectBox *rect_box, BOX2DF *query)
{
	return rect_box->left.ymin >= query->ymin;
}

/*
 * SP-GiST config function
 */

PG_FUNCTION_INFO_V1(gserialized_spgist_config_2d);

PGDLLEXPORT Datum gserialized_spgist_config_2d(PG_FUNCTION_ARGS)
{
	spgConfigOut *cfg = (spgConfigOut *)PG_GETARG_POINTER(1);
	Oid boxoid = InvalidOid;
	/* We need to initialize the internal cache to access it later via postgis_oid() */
	postgis_initialize_cache(fcinfo);
	boxoid = postgis_oid(BOX2DFOID);

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

PG_FUNCTION_INFO_V1(gserialized_spgist_choose_2d);

PGDLLEXPORT Datum gserialized_spgist_choose_2d(PG_FUNCTION_ARGS)
{
	spgChooseIn *in = (spgChooseIn *)PG_GETARG_POINTER(0);
	spgChooseOut *out = (spgChooseOut *)PG_GETARG_POINTER(1);
	BOX2DF *centroid = (BOX2DF *)DatumGetPointer(in->prefixDatum), *box = (BOX2DF *)DatumGetPointer(in->leafDatum);

	out->resultType = spgMatchNode;
	out->result.matchNode.restDatum = PointerGetDatum(box);

	/* nodeN will be set by core, when allTheSame. */
	if (!in->allTheSame)
		out->result.matchNode.nodeN = getQuadrant4D(centroid, box);

	PG_RETURN_VOID();
}

/*
 * SP-GiST pick-split function
 *
 * It splits a list of boxes into quadrants by choosing a central 4D
 * point as the median of the coordinates of the boxes.
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_picksplit_2d);

PGDLLEXPORT Datum gserialized_spgist_picksplit_2d(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *)PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *)PG_GETARG_POINTER(1);
	BOX2DF *centroid;
	int median, i;
	double *lowXs = palloc(sizeof(double) * in->nTuples);
	double *highXs = palloc(sizeof(double) * in->nTuples);
	double *lowYs = palloc(sizeof(double) * in->nTuples);
	double *highYs = palloc(sizeof(double) * in->nTuples);

	/* Calculate median of all 4D coordinates */
	for (i = 0; i < in->nTuples; i++)
	{
		BOX2DF *box = (BOX2DF *)DatumGetPointer(in->datums[i]);

		lowXs[i] = (double)box->xmin;
		highXs[i] = (double)box->xmax;
		lowYs[i] = (double)box->ymin;
		highYs[i] = (double)box->ymax;
	}

	qsort(lowXs, in->nTuples, sizeof(double), compareDoubles);
	qsort(highXs, in->nTuples, sizeof(double), compareDoubles);
	qsort(lowYs, in->nTuples, sizeof(double), compareDoubles);
	qsort(highYs, in->nTuples, sizeof(double), compareDoubles);

	median = in->nTuples / 2;

	centroid = palloc(sizeof(BOX2DF));

	centroid->xmin = (float)lowXs[median];
	centroid->xmax = (float)highXs[median];
	centroid->ymin = (float)lowYs[median];
	centroid->ymax = (float)highYs[median];

	/* Fill the output */
	out->hasPrefix = true;
	out->prefixDatum = BoxPGetDatum(centroid);

	out->nNodes = 16;
	out->nodeLabels = NULL; /* We don't need node labels. */

	out->mapTuplesToNodes = palloc(sizeof(int) * in->nTuples);
	out->leafTupleDatums = palloc(sizeof(Datum) * in->nTuples);

	/*
	 * Assign ranges to corresponding nodes according to quadrants relative to the "centroid" range
	 */
	for (i = 0; i < in->nTuples; i++)
	{
		BOX2DF *box = (BOX2DF *)DatumGetPointer(in->datums[i]);
		uint8 quadrant = getQuadrant4D(centroid, box);

		out->leafTupleDatums[i] = PointerGetDatum(box);
		out->mapTuplesToNodes[i] = quadrant;
	}

	pfree(lowXs);
	pfree(highXs);
	pfree(lowYs);
	pfree(highYs);

	PG_RETURN_VOID();
}

/*
 * SP-GiST inner consistent function
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_inner_consistent_2d);

PGDLLEXPORT Datum gserialized_spgist_inner_consistent_2d(PG_FUNCTION_ARGS)
{
	spgInnerConsistentIn *in = (spgInnerConsistentIn *)PG_GETARG_POINTER(0);
	spgInnerConsistentOut *out = (spgInnerConsistentOut *)PG_GETARG_POINTER(1);
	int i;
	MemoryContext old_ctx;
	RectBox *rect_box;
	uint8 quadrant;
	BOX2DF *centroid;

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
	 * We are saving the traversal value or initialize it an unbounded one, if
	 * we have just begun to walk the tree.
	 */
	if (in->traversalValue)
		rect_box = in->traversalValue;
	else
		rect_box = initRectBox();

	centroid = (BOX2DF *)DatumGetPointer(in->prefixDatum);

	/* Allocate enough memory for nodes */
	out->nNodes = 0;
	out->nodeNumbers = (int *)palloc(sizeof(int) * in->nNodes);
	out->traversalValues = (void **)palloc(sizeof(void *) * in->nNodes);

	/*
	 * We switch memory context, because we want to allocate memory for new
	 * traversal values (next_rect_box) and pass these pieces of memory to
	 * further call of this function.
	 */
	old_ctx = MemoryContextSwitchTo(in->traversalMemoryContext);

	for (quadrant = 0; quadrant < in->nNodes; quadrant++)
	{
		RectBox *next_rect_box = nextRectBox(rect_box, centroid, quadrant);
		bool flag = true;

		for (i = 0; i < in->nkeys; i++)
		{
			StrategyNumber strategy = in->scankeys[i].sk_strategy;

			Datum query = in->scankeys[i].sk_argument;
			BOX2DF query_gbox_index;

			/* Quick sanity check on query argument. */
			if (DatumGetPointer(query) == NULL)
			{
				POSTGIS_DEBUG(4, "[SPGIST] null query pointer (!?!), returning false");
				PG_RETURN_BOOL(false); /* NULL query! This is screwy! */
			}

			if (gserialized_datum_get_box2df_p(query, &query_gbox_index) == LW_FAILURE)
			{
				POSTGIS_DEBUG(4, "[SPGIST] null query_gbox_index!");
				PG_RETURN_BOOL(false);
			}

			switch (strategy)
			{
			case RTOverlapStrategyNumber:
			case RTContainedByStrategyNumber:
			case RTOldContainedByStrategyNumber:
				flag = overlap4D(next_rect_box, &query_gbox_index);
				break;

			case RTContainsStrategyNumber:
			case RTSameStrategyNumber:
				flag = contain4D(next_rect_box, &query_gbox_index);
				break;

			case RTLeftStrategyNumber:
				flag = !overRight4D(next_rect_box, &query_gbox_index);
				break;

			case RTOverLeftStrategyNumber:
				flag = !right4D(next_rect_box, &query_gbox_index);
				break;

			case RTRightStrategyNumber:
				flag = !overLeft4D(next_rect_box, &query_gbox_index);
				break;

			case RTOverRightStrategyNumber:
				flag = !left4D(next_rect_box, &query_gbox_index);
				break;

			case RTAboveStrategyNumber:
				flag = !overBelow4D(next_rect_box, &query_gbox_index);
				break;

			case RTOverAboveStrategyNumber:
				flag = !below4D(next_rect_box, &query_gbox_index);
				break;

			case RTBelowStrategyNumber:
				flag = !overAbove4D(next_rect_box, &query_gbox_index);
				break;

			case RTOverBelowStrategyNumber:
				flag = !above4D(next_rect_box, &query_gbox_index);
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
			out->traversalValues[out->nNodes] = next_rect_box;
			out->nodeNumbers[out->nNodes] = quadrant;
			out->nNodes++;
		}
		else
		{
			/*
			 * If this node is not selected, we don't need to keep the next
			 * traversal value in the memory context.
			 */
			pfree(next_rect_box);
		}
	}

	/* Switch after */
	MemoryContextSwitchTo(old_ctx);

	PG_RETURN_VOID();
}

/*
 * SP-GiST inner consistent function
 */
PG_FUNCTION_INFO_V1(gserialized_spgist_leaf_consistent_2d);

PGDLLEXPORT Datum gserialized_spgist_leaf_consistent_2d(PG_FUNCTION_ARGS)
{
	spgLeafConsistentIn *in = (spgLeafConsistentIn *)PG_GETARG_POINTER(0);
	spgLeafConsistentOut *out = (spgLeafConsistentOut *)PG_GETARG_POINTER(1);
	BOX2DF *key = (BOX2DF *)DatumGetPointer(in->leafDatum);
	bool flag = true;
	int i;

	/* Quick sanity check on entry key. */
	if (DatumGetPointer(key) == NULL)
	{
		POSTGIS_DEBUG(4, "[SPGIST] null index entry, returning false");
		PG_RETURN_BOOL(false); /* NULL entry! */
	}

	/* All tests are exact. */
	out->recheck = false;

	/* leafDatum is what it is... */
	out->leafValue = in->leafDatum;

	/* Perform the required comparison(s) */
	for (i = 0; i < in->nkeys; i++)
	{
		StrategyNumber strategy = in->scankeys[i].sk_strategy;
		Datum query = in->scankeys[i].sk_argument;
		BOX2DF query_gbox_index;

		/* Quick sanity check on query argument. */
		if (DatumGetPointer(query) == NULL)
		{
			POSTGIS_DEBUG(4, "[SPGIST] null query pointer (!?!), returning false");
			PG_RETURN_BOOL(false); /* NULL query! This is screwy! */
		}

		if (gserialized_datum_get_box2df_p(query, &query_gbox_index) == LW_FAILURE)
		{
			POSTGIS_DEBUG(4, "[SPGIST] null query_gbox_index!");
			PG_RETURN_BOOL(false);
		}

		switch (strategy)
		{
		case RTOverlapStrategyNumber:
			flag = box2df_overlaps(key, &query_gbox_index);
			break;

		case RTContainsStrategyNumber:
		case RTOldContainsStrategyNumber:
			flag = box2df_contains(key, &query_gbox_index);
			break;

		case RTContainedByStrategyNumber:
		case RTOldContainedByStrategyNumber:
			flag = box2df_contains(&query_gbox_index, key);
			break;

		case RTSameStrategyNumber:
			flag = box2df_equals(key, &query_gbox_index);
			break;

		case RTLeftStrategyNumber:
			flag = box2df_left(key, &query_gbox_index);
			break;

		case RTOverLeftStrategyNumber:
			flag = box2df_overleft(key, &query_gbox_index);
			break;

		case RTRightStrategyNumber:
			flag = box2df_right(key, &query_gbox_index);
			break;

		case RTOverRightStrategyNumber:
			flag = box2df_overright(key, &query_gbox_index);
			break;

		case RTAboveStrategyNumber:
			flag = box2df_above(key, &query_gbox_index);
			break;

		case RTOverAboveStrategyNumber:
			flag = box2df_overabove(key, &query_gbox_index);
			break;

		case RTBelowStrategyNumber:
			flag = box2df_below(key, &query_gbox_index);
			break;

		case RTOverBelowStrategyNumber:
			flag = box2df_overbelow(key, &query_gbox_index);
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

PG_FUNCTION_INFO_V1(gserialized_spgist_compress_2d);

PGDLLEXPORT Datum gserialized_spgist_compress_2d(PG_FUNCTION_ARGS)
{
	Datum gsdatum = PG_GETARG_DATUM(0);
	BOX2DF *bbox_out = palloc(sizeof(BOX2DF));
	int result = LW_SUCCESS;

	POSTGIS_DEBUG(4, "[SPGIST] 'compress' function called");

	/* Extract the index key from the argument. */
	result = gserialized_datum_get_box2df_p(gsdatum, bbox_out);

	/* Is the bounding box valid (non-empty, non-infinite)? If not, return input uncompressed. */
	if (result == LW_FAILURE)
	{
		box2df_set_empty(bbox_out);

		POSTGIS_DEBUG(4, "[SPGIST] empty geometry!");
		PG_RETURN_POINTER(bbox_out);
	}

	/* Check all the dimensions for finite values */
	if ((!isfinite(bbox_out->xmax) || !isfinite(bbox_out->xmin)) ||
	    (!isfinite(bbox_out->ymax) || !isfinite(bbox_out->ymin)))
	{
		box2df_set_finite(bbox_out);

		POSTGIS_DEBUG(4, "[SPGIST] infinite geometry!");
		PG_RETURN_POINTER(bbox_out);
	}

	/* Ensure bounding box has minimums below maximums. */
	box2df_validate(bbox_out);

	/* Return BOX2DF. */
	POSTGIS_DEBUG(4, "[SPGIST] 'compress' function complete");
	PG_RETURN_POINTER(bbox_out);
}

/*****************************************************************************/
