/*****************************************************************************
 *
 * Box3DSpgist.c
 *	  SP-GiST implementation of 6-dimensional oct-tree over 3D boxes
 *
 * This module provides SP-GiST implementation for boxes using oct tree
 * analogy in 4-dimensional space.  SP-GiST doesn't allow indexing of
 * overlapping objects.  We are making 3D objects never-overlapping in
 * 6D space.  This technique has some benefits compared to traditional
 * R-Tree which is implemented as GiST.  The performance tests reveal
 * that this technique especially beneficial with too much overlapping
 * objects, so called "spaghetti data".
 *
 * Unlike the original oct-tree, we are splitting the tree into 64
 * octants in 6D space.  It is easier to imagine it as splitting space
 * three times into 3:
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
 * We are using box3D data type as the prefix, but we are treating them
 * as points in 6-dimensional space, because 3D boxes are not enough
 * to represent the octant boundaries in 6D space.  They however are
 * sufficient to point out the additional boundaries of the next
 * octant.
 *
 * We are using traversal values provided by SP-GiST to calculate and
 * to store the bounds of the octants, while traversing into the tree.
 * Traversal value has all the boundaries in the 6D space, and is is
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

#include "spgist_box3d.h"
#include "lwgeom_pg.h"

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
	double		x = *(double *) a;
	double		y = *(double *) b;

	if (x == y)
		return 0;
	return (x > y) ? 1 : -1;
}

typedef struct
{
	BOX3D	left;
	BOX3D	right;
} CubeBox3D;

/*
 * Calculate the octant
 *
 * The octant is 8 bit unsigned integer with 6 least bits in use.
 * This function accepts 2 BOX3D as input. All 6 bits are set by comparing
 * a corner of the box. This makes 64 octants in total.
 */
static uint8
getOctant(BOX3D *centroid, BOX3D *inBox)
{
	uint8		octant = 0;

	if (inBox->xmin > centroid->xmin)
		octant |= 0x20;

	if (inBox->xmax > centroid->xmax)
		octant |= 0x10;

	if (inBox->ymin > centroid->ymin)
		octant |= 0x08;

	if (inBox->ymax > centroid->ymax)
		octant |= 0x04;

	if (inBox->zmin > centroid->zmin)
		octant |= 0x02;

	if (inBox->zmax > centroid->zmax)
		octant |= 0x01;

	return octant;
}

/*
 * Initialize the traversal value
 *
 * In the beginning, we don't have any restrictions.  We have to
 * initialize the struct to cover the whole 6D space.
 */
static CubeBox3D *
initCubeBox(void)
{
	CubeBox3D  *cube_box = (CubeBox3D *) palloc(sizeof(CubeBox3D));
	double		infinity = get_float8_infinity();

	cube_box->left.xmin = -infinity;
	cube_box->left.xmax = infinity;

	cube_box->left.ymin = -infinity;
	cube_box->left.ymax = infinity;

	cube_box->left.zmin = -infinity;
	cube_box->left.zmax = infinity;

	cube_box->right.xmin = -infinity;
	cube_box->right.xmax = infinity;

	cube_box->right.ymin = -infinity;
	cube_box->right.ymax = infinity;

	cube_box->right.zmin = -infinity;
	cube_box->right.zmax = infinity;

	return cube_box;
}

/*
 * Calculate the next traversal value
 *
 * All centroids are bounded by CubeBox3D, but SP-GiST only keeps
 * boxes.  When we are traversing the tree, we must calculate CubeBox3D,
 * using centroid and octant.
 */
static CubeBox3D *
nextCubeBox3D(CubeBox3D *cube_box, BOX3D *centroid, uint8 octant)
{
	CubeBox3D    *next_cube_box = (CubeBox3D *) palloc(sizeof(CubeBox3D));

	memcpy(next_cube_box, cube_box, sizeof(CubeBox3D));

	if (octant & 0x20)
		next_cube_box->left.xmin = centroid->xmin;
	else
		next_cube_box->left.xmax = centroid->xmin;

	if (octant & 0x10)
		next_cube_box->right.xmin = centroid->xmax;
	else
		next_cube_box->right.xmax = centroid->xmax;

	if (octant & 0x08)
		next_cube_box->left.ymin = centroid->ymin;
	else
		next_cube_box->left.ymax = centroid->ymin;

	if (octant & 0x04)
		next_cube_box->right.ymin = centroid->ymax;
	else
		next_cube_box->right.ymax = centroid->ymax;

	if (octant & 0x02)
		next_cube_box->left.zmin = centroid->zmin;
	else
		next_cube_box->left.zmax = centroid->zmin;

	if (octant & 0x01)
		next_cube_box->right.zmin = centroid->zmax;
	else
		next_cube_box->right.zmax = centroid->zmax;

	return next_cube_box;
}

/* Can any cube from cube_box overlap with query? */
static bool
overlap6D(CubeBox3D *cube_box, BOX3D *query)
{
	return (FPle(cube_box->left.xmin, query->xmax) &&
			FPge(cube_box->right.xmax, query->xmin) &&
			FPle(cube_box->left.ymin, query->ymax) &&
			FPge(cube_box->right.ymax, query->ymin) &&
			FPle(cube_box->left.zmin, query->zmax) &&
			FPge(cube_box->right.zmax, query->zmin));
}

/* Can any cube from cube_box contain query? */
static bool
contain6D(CubeBox3D *cube_box, BOX3D *query)
{
	return (FPge(cube_box->right.xmax, query->xmax) &&
			FPle(cube_box->left.xmin, query->xmin) &&
			FPge(cube_box->right.ymax, query->ymax) &&
			FPle(cube_box->left.ymin, query->ymin) &&
			FPge(cube_box->right.zmax, query->zmax) &&
			FPle(cube_box->left.zmin, query->zmin));
}

/* Can any cube from cube_box be left of query? */
static bool
left6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.xmax, query->xmin);
}

/* Can any cube from cube_box does not extend the right of query? */
static bool
overLeft6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.xmax, query->xmax);
}

/* Can any cube from cube_box be right of query? */
static bool
right6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.xmin, query->xmax);
}

/* Can any cube from cube_box does not extend the left of query? */
static bool
overRight6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.xmin, query->xmin);
}

/* Can any cube from cube_box be below of query? */
static bool
below6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.ymax, query->ymin);
}

/* Can any cube from cube_box does not extend above query? */
static bool
overBelow6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.ymax, query->ymax);
}

/* Can any cube from cube_box be above of query? */
static bool
above6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.ymin, query->ymax);
}

/* Can any cube from cube_box does not extend below of query? */
static bool
overAbove6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.ymin, query->ymin);
}

/* Can any cube from cube_box be before of query? */
static bool
before6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.zmax, query->zmin);
}

/* Can any cube from cube_box does not extend the after of query? */
static bool
overBefore6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPle(cube_box->right.zmax, query->zmax);
}

/* Can any cube from cube_box be after of query? */
static bool
after6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.zmin, query->zmax);
}

/* Can any cube from cube_box does not extend the before of query? */
static bool
overAfter6D(CubeBox3D *cube_box, BOX3D *query)
{
	return FPge(cube_box->left.zmin, query->zmin);
}

/*
 * SP-GiST config function
 */

PG_FUNCTION_INFO_V1(spgist_box3D_octree_config);

PGDLLEXPORT Datum
spgist_box3D_octree_config(PG_FUNCTION_ARGS)
{
	spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);

	cfg->prefixType = TypenameGetTypid("box3d");
	cfg->labelType = VOIDOID;	/* We don't need node labels. */
	cfg->canReturnData = true;
	cfg->longValuesOK = false;

	PG_RETURN_VOID();
}

/*
 * SP-GiST choose function
 */

PG_FUNCTION_INFO_V1(spgist_box3D_octree_choose);

PGDLLEXPORT Datum
spgist_box3D_octree_choose(PG_FUNCTION_ARGS)
{
	spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
	spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);
	BOX3D	   *centroid = DatumGetBox3DP(in->prefixDatum),
			   *box = DatumGetBox3DP(in->datum);

	out->resultType = spgMatchNode;
	out->result.matchNode.restDatum = Box3DPGetDatum(box);

	/* nodeN will be set by core, when allTheSame. */
	if (!in->allTheSame)
		out->result.matchNode.nodeN = getOctant(centroid, box);

	PG_RETURN_VOID();
}

/*
 * SP-GiST pick-split function
 *
 * It splits a list of boxes into octants by choosing a central 6D
 * point as the median of the coordinates of the boxes.
 */
PG_FUNCTION_INFO_V1(spgist_box3D_octree_picksplit);

PGDLLEXPORT Datum
spgist_box3D_octree_picksplit(PG_FUNCTION_ARGS)
{
	spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
	spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
	BOX3D	   *centroid;
	int			median,
				i;
	double	   *lowXs = palloc(sizeof(double) * in->nTuples);
	double	   *highXs = palloc(sizeof(double) * in->nTuples);
	double	   *lowYs = palloc(sizeof(double) * in->nTuples);
	double	   *highYs = palloc(sizeof(double) * in->nTuples);
	double	   *lowZs = palloc(sizeof(double) * in->nTuples);
	double	   *highZs = palloc(sizeof(double) * in->nTuples);

	BOX3D		   *box = DatumGetBox3DP(in->datums[0]);
	int32_t 		srid = box->srid;

	/* Calculate median of all 6D coordinates */
	for (i = 0; i < in->nTuples; i++)
	{
		BOX3D		   *box = DatumGetBox3DP(in->datums[i]);

		lowXs[i] = box->xmin;
		highXs[i] = box->xmax;
		lowYs[i] = box->ymin;
		highYs[i] = box->ymax;
		lowZs[i] = box->zmin;
		highZs[i] = box->zmax;
	}

	qsort(lowXs, in->nTuples, sizeof(double), compareDoubles);
	qsort(highXs, in->nTuples, sizeof(double), compareDoubles);
	qsort(lowYs, in->nTuples, sizeof(double), compareDoubles);
	qsort(highYs, in->nTuples, sizeof(double), compareDoubles);
	qsort(lowZs, in->nTuples, sizeof(double), compareDoubles);
	qsort(highZs, in->nTuples, sizeof(double), compareDoubles);

	median = in->nTuples / 2;

	centroid = palloc(sizeof(BOX3D));

	centroid->xmin = lowXs[median];
	centroid->xmax = highXs[median];
	centroid->ymin = lowYs[median];
	centroid->ymax = highYs[median];
	centroid->zmin = lowZs[median];
	centroid->zmax = highZs[median];
	centroid->srid = srid;

	/* Fill the output */
	out->hasPrefix = true;
	out->prefixDatum = Box3DPGetDatum(centroid);

	out->nNodes = 64;
	out->nodeLabels = NULL;		/* We don't need node labels. */

	out->mapTuplesToNodes = palloc(sizeof(int) * in->nTuples);
	out->leafTupleDatums = palloc(sizeof(Datum) * in->nTuples);

	/*
	 * Assign ranges to corresponding nodes according to octants relative to
	 * the "centroid" range
	 */
	for (i = 0; i < in->nTuples; i++)
	{
		BOX3D	   *box = DatumGetBox3DP(in->datums[i]);
		uint8		octant = getOctant(centroid, box);

		out->leafTupleDatums[i] = Box3DPGetDatum(box);
		out->mapTuplesToNodes[i] = octant;
	}

	pfree(lowXs);
	pfree(highXs);
	pfree(lowYs);
	pfree(highYs);
	pfree(lowZs);
	pfree(highZs);

	PG_RETURN_VOID();
}

/*
 * SP-GiST inner consistent function
 */
PG_FUNCTION_INFO_V1(spgist_box3D_octree_inner_consistent);

PGDLLEXPORT Datum
spgist_box3D_octree_inner_consistent(PG_FUNCTION_ARGS)
{
	spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
	spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
	int			i;
	MemoryContext old_ctx;
	CubeBox3D   *cube_box;
	uint8		octant;
	BOX3D	   *centroid;
	int		   *nodeNumbers;
	void	  **traversalValues;

	if (in->allTheSame)
	{
		/* Report that all nodes should be visited */
		out->nNodes = in->nNodes;
		out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
		for (i = 0; i < in->nNodes; i++)
			out->nodeNumbers[i] = i;

		PG_RETURN_VOID();
	}

	/*
	 * We are saving the traversal value or initialize it an unbounded one, if
	 * we have just begun to walk the tree.
	 */
	if (in->traversalValue)
		cube_box = in->traversalValue;
	else
		cube_box = initCubeBox();

	centroid = DatumGetBox3DP(in->prefixDatum);

	/* Allocate enough memory for nodes */
	out->nNodes = 0;
	// out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
	// out->traversalValues = (void **) palloc(sizeof(void *) * in->nNodes);
	nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
	traversalValues = (void **) palloc(sizeof(void *) * in->nNodes);

	/*
	 * We switch memory context, because we want to allocate memory for new
	 * traversal values (next_cube_box) and pass these pieces of memory to
	 * further call of this function.
	 */
	old_ctx = MemoryContextSwitchTo(in->traversalMemoryContext);

	for (octant = 0; octant < in->nNodes; octant++)
	{
		CubeBox3D  *next_cube_box = nextCubeBox3D(cube_box, centroid, octant);
		bool		flag = true;

		for (i = 0; i < in->nkeys; i++)
		{
			StrategyNumber strategy = in->scankeys[i].sk_strategy;

			switch (strategy)
			{
				case RTOverlapStrategyNumber:
				case RTContainedByStrategyNumber:
					flag = overlap6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTContainsStrategyNumber:
				case RTSameStrategyNumber:
					flag = contain6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTLeftStrategyNumber:
					flag = !overRight6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverLeftStrategyNumber:
					flag = !right6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTRightStrategyNumber:
					flag = !overLeft6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverRightStrategyNumber:
					flag = !left6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTAboveStrategyNumber:
					flag = !overBelow6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverAboveStrategyNumber:
					flag = !below6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTBelowStrategyNumber:
					flag = !overAbove6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverBelowStrategyNumber:
					flag = !above6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTAfterStrategyNumber:
					flag = !overBefore6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverAfterStrategyNumber:
					flag = !before6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTBeforeStrategyNumber:
					flag = !overAfter6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
					break;

				case RTOverBeforeStrategyNumber:
					flag = !after6D(next_cube_box, DatumGetBox3DP(in->scankeys[i].sk_argument));
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
			nodeNumbers[out->nNodes] = octant;
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
	out->nodeNumbers = (int *) palloc(sizeof(int) * out->nNodes);
	out->traversalValues = (void **) palloc(sizeof(void *) * out->nNodes);
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
PG_FUNCTION_INFO_V1(spgist_box3D_octree_leaf_consistent);

PGDLLEXPORT Datum
spgist_box3D_octree_leaf_consistent(PG_FUNCTION_ARGS)
{
	spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
	spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
	BOX3D		*leaf = DatumGetBox3DP(in->leafDatum);
	bool		flag = true;
	int			i;

	/* All tests are exact. */
	out->recheck = false;

	/* leafDatum is what it is... */
	out->leafValue = in->leafDatum;

	/* Perform the required comparison(s) */
	for (i = 0; i < in->nkeys; i++)
	{
		StrategyNumber strategy = in->scankeys[i].sk_strategy;
		BOX3D		*query = DatumGetBox3DP(in->scankeys[i].sk_argument);

		switch (strategy)
		{
			case RTOverlapStrategyNumber:
				flag = BOX3D_overlaps_internal(leaf, query);
				break;

			case RTContainsStrategyNumber:
				flag = BOX3D_contains_internal(leaf, query);
				break;

			case RTContainedByStrategyNumber:
				flag = BOX3D_contained_internal(leaf, query);
				break;

			case RTSameStrategyNumber:
				flag = BOX3D_same_internal(leaf, query);
				break;

			case RTLeftStrategyNumber:
				flag = BOX3D_left_internal(leaf, query);
				break;

			case RTOverLeftStrategyNumber:
				flag = BOX3D_overleft_internal(leaf, query);
				break;

			case RTRightStrategyNumber:
				flag = BOX3D_right_internal(leaf, query);
				break;

			case RTOverRightStrategyNumber:
				flag = BOX3D_overright_internal(leaf, query);
				break;

			case RTAboveStrategyNumber:
				flag = BOX3D_above_internal(leaf, query);
				break;

			case RTOverAboveStrategyNumber:
				flag = BOX3D_overabove_internal(leaf, query);
				break;

			case RTBelowStrategyNumber:
				flag = BOX3D_below_internal(leaf, query);
				break;

			case RTOverBelowStrategyNumber:
				flag = BOX3D_overbelow_internal(leaf, query);
				break;

			case RTAfterStrategyNumber:
				flag = BOX3D_after_internal(leaf, query);
				break;

			case RTOverAfterStrategyNumber:
				flag = BOX3D_overafter_internal(leaf, query);
				break;

			case RTBeforeStrategyNumber:
				flag = BOX3D_before_internal(leaf, query);
				break;

			case RTOverBeforeStrategyNumber:
				flag = BOX3D_overbefore_internal(leaf, query);
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

/*****************************************************************************/
