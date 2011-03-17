/**********************************************************************
 * $Id: gserialized_gist_2d.c 6519 2010-12-28 00:54:19Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/*
** R-Tree Bibliography
**
** [1] A. Guttman. R-tree: a dynamic index structure for spatial searching.
**     Proceedings of the ACM SIGMOD Conference, pp 47-57, June 1984.
** [2] C.-H. Ang and T. C. Tan. New linear node splitting algorithm for
**     R-Trees. Advances in Spatial Databases - 5th International Symposium,
**     1997
** [3] N. Beckmann, H.-P. Kriegel, R. Schneider, B. Seeger. The R*tree: an
**     efficient and robust access method for points and rectangles.
**     Proceedings of the ACM SIGMOD Conference. June 1990.
*/

#include "postgres.h"
#include "access/gist.h"    /* For GiST */
#include "access/itup.h"
#include "access/skey.h"

#include "../postgis_config.h"

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "gserialized_gist.h"	     /* For utility functions. */

/*
** When is a node split not so good? If more than 90% of the entries
** end up in one of the children.
*/
#define LIMIT_RATIO 0.1

/*
** For debugging
*/
#if POSTGIS_DEBUG_LEVEL > 0
static int g2d_counter_leaf = 0;
static int g2d_counter_internal = 0;
#endif

/*
** GiST 2D key stubs 
*/
Datum box2df_out(PG_FUNCTION_ARGS);
Datum box2df_in(PG_FUNCTION_ARGS);

/*
** GiST 2D index function prototypes
*/
Datum gserialized_gist_consistent_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_compress_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_decompress_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_penalty_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_picksplit_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_union_2d(PG_FUNCTION_ARGS);
Datum gserialized_gist_same_2d(PG_FUNCTION_ARGS);

/*
** GiST 2D operator prototypes
*/
Datum gserialized_same_2d(PG_FUNCTION_ARGS);
Datum gserialized_within_2d(PG_FUNCTION_ARGS);
Datum gserialized_contains_2d(PG_FUNCTION_ARGS);
Datum gserialized_overlaps_2d(PG_FUNCTION_ARGS);
Datum gserialized_left_2d(PG_FUNCTION_ARGS);
Datum gserialized_right_2d(PG_FUNCTION_ARGS);
Datum gserialized_above_2d(PG_FUNCTION_ARGS);
Datum gserialized_below_2d(PG_FUNCTION_ARGS);
Datum gserialized_overleft_2d(PG_FUNCTION_ARGS);
Datum gserialized_overright_2d(PG_FUNCTION_ARGS);
Datum gserialized_overabove_2d(PG_FUNCTION_ARGS);
Datum gserialized_overbelow_2d(PG_FUNCTION_ARGS);

/*
** true/false test function type
*/
typedef bool (*box2df_predicate)(BOX2DF *a, BOX2DF *b);


#if POSTGIS_DEBUG_LEVEL > 0
static char* box2df_to_string(const BOX2DF *a)
{
	char *rv = NULL;

	if ( a == NULL )
		return pstrdup("<NULLPTR>");

	rv = palloc(128);
	sprintf(rv, "BOX2DF(%.12g %.12g, %.12g %.12g)", a->xmin, a->ymin, a->xmax, a->ymax);
	return rv;
}
#endif


/* Allocate a new copy of BOX2DF */
static BOX2DF* box2df_copy(BOX2DF *b)
{
	BOX2DF *c = (BOX2DF*)palloc(sizeof(BOX2DF));
	memcpy((void*)c, (void*)b, sizeof(BOX2DF));
	POSTGIS_DEBUGF(5, "copied box2df (%p) to box2df (%p)", b, c);
	return c;
}



/* Enlarge b_union to contain b_new. If b_new contains more
   dimensions than b_union, expand b_union to contain those dimensions. */
static void box2df_merge(BOX2DF *b_union, BOX2DF *b_new)
{

	POSTGIS_DEBUGF(5, "merging %s with %s", box2df_to_string(b_union), box2df_to_string(b_new));
	/* Adjust minimums */
	b_union->xmin = Min(b_union->xmin, b_new->xmin);
	b_union->ymin = Min(b_union->ymin, b_new->ymin);
	/* Adjust maximums */
	b_union->xmax = Max(b_union->xmax, b_new->xmax);
	b_union->ymax = Max(b_union->ymax, b_new->ymax);

	POSTGIS_DEBUGF(5, "merge complete %s", box2df_to_string(b_union));
	return;
}



static bool box2df_intersection(const BOX2DF *a, const BOX2DF *b, BOX2DF *n)
{
	POSTGIS_DEBUGF(5, "calculating intersection of %s with %s", box2df_to_string(a), box2df_to_string(b));

	if( a == NULL || b == NULL || n == NULL ) 
		return FALSE;
		
	n->xmax = Min(a->xmax, b->xmax);
	n->ymax = Min(a->ymax, b->ymax);
	n->xmin = Max(a->xmin, b->xmin);
	n->ymin = Max(a->ymin, b->ymin);

	POSTGIS_DEBUGF(5, "intersection is %s", box2df_to_string(n));

	if ( (n->xmax < n->xmin) || (n->ymax < n->ymin) )
		return FALSE;

	return TRUE;
}

static float box2df_size(const BOX2DF *a)
{
	float result;

	if ( a == NULL ) 
		return (float)0.0;
		
	if ( (a->xmax <= a->xmin) || (a->ymax <= a->ymin) )
	{
		result =  (float) 0.0;
	}
	else
	{
		result = (((double) a->xmax)-((double) a->xmin)) * (((double) a->ymax)-((double) a->ymin));
	}

	return result;
}

static float box2df_union_size(const BOX2DF *a, const BOX2DF *b)
{
	float result;

	POSTGIS_DEBUG(5,"entered function");

	if ( a == NULL && b == NULL )
	{
		elog(ERROR, "box2df_union_size received two null arguments");
		return 0.0;
	}
	
	if ( a == NULL )
		return box2df_size(b);

	if ( b == NULL )
		return box2df_size(a);

	result = ((double)Max(a->xmax,b->xmax) - (double)Min(a->xmin,b->xmin)) * 
 	         ((double)Max(a->ymax,b->ymax) - (double)Min(a->ymin,b->ymin));

	POSTGIS_DEBUGF(5, "union size of %s and %s is %.8g", box2df_to_string(a), box2df_to_string(b), result);

	return result;
}


/* Convert a double-based GBOX into a float-based BOX2DF,
   ensuring the float box is larger than the double box */
static inline int box2df_from_gbox_p(GBOX *box, BOX2DF *a)
{
	a->xmin = next_float_down(box->xmin);
	a->xmax = next_float_up(box->xmax);
	a->ymin = next_float_down(box->ymin);
	a->ymax = next_float_up(box->ymax);
	return LW_SUCCESS;
}

/***********************************************************************
** BOX3DF tests for 2D index operators.
*/

/* Ensure all minimums are below maximums. */
static inline void box2df_validate(BOX2DF *b)
{
	float tmp;
	POSTGIS_DEBUGF(5,"validating box2df (%s)", box2df_to_string(b));
	if ( b->xmax < b->xmin ) 
	{
		tmp = b->xmin;
		b->xmin = b->xmax;
		b->xmax = tmp;
	}
	if ( b->ymax < b->ymin ) 
	{
		tmp = b->ymin;
		b->ymin = b->ymax;
		b->ymax = tmp;
	}
	return;
}

static bool box2df_overlaps(BOX2DF *a, BOX2DF *b)
{
	if ( (a->xmin > b->xmax) || (b->xmin > a->xmax) ||
	     (a->ymin > b->ymax) || (b->ymin > a->ymax) )
	{
			return FALSE;
	}

	return TRUE;
}

static bool box2df_contains(BOX2DF *a, BOX2DF *b)
{
	if ( (a->xmin > b->xmin) || (a->xmax < b->xmax) ||
	     (a->ymin > b->ymin) || (a->ymax < b->ymax) )
	{
			return FALSE;
	}

	return TRUE;
}

static bool box2df_within(BOX2DF *a, BOX2DF *b)
{
	POSTGIS_DEBUG(5, "entered function");
	return box2df_contains(b,a);
}

static bool box2df_equals(BOX2DF *a, BOX2DF *b)
{
	if ( (a->xmin != b->xmin) || (a->xmax != b->xmax) ||
	     (a->ymin != b->ymin) || (a->ymax != b->ymax) )
	{
		return FALSE;
	}

	return TRUE;
}

static bool box2df_overleft(BOX2DF *a, BOX2DF *b)
{
	/* a.xmax <= b.xmax */
	return a->xmax <= b->xmax;
}

static bool box2df_left(BOX2DF *a, BOX2DF *b)
{
	/* a.xmax < b.xmin */
	return a->xmax < b->xmin;
}

static bool box2df_right(BOX2DF *a, BOX2DF *b)
{
	/* a.xmin > b.xmax */
	return a->xmin > b->xmax;
}

static bool box2df_overright(BOX2DF *a, BOX2DF *b)
{
	/* a.xmin >= b.xmin */
	return a->xmin >= b->xmin;
}

static bool box2df_overbelow(BOX2DF *a, BOX2DF *b)
{
	/* a.ymax <= b.ymax */
	return a->ymax <= b->ymax;
}

static bool box2df_below(BOX2DF *a, BOX2DF *b)
{
	/* a.ymax < b.ymin */
	return a->ymax < b->ymin;
}

static bool box2df_above(BOX2DF *a, BOX2DF *b)
{
	/* a.ymin > b.ymax */
	return a->ymin > b->ymax;
}

static bool box2df_overabove(BOX2DF *a, BOX2DF *b)
{
	/* a.ymin >= b.ymin */
	return a->ymin >= b->ymin;
}


/**
* Peak into a #GSERIALIZED datum to find the bounding box. If the
* box is there, copy it out and return it. If not, calculate the box from the
* full object and return the box based on that. If no box is available,
* return #LW_FAILURE, otherwise #LW_SUCCESS.
*/
static int 
gserialized_datum_get_box2df_p(Datum gsdatum, BOX2DF *box2df)
{
	GSERIALIZED *gpart;
	uchar flags;
	int result = LW_SUCCESS;

	POSTGIS_DEBUG(4, "entered function");

	/*
	** The most info we need is the 8 bytes of serialized header plus the 
	** of floats necessary to hold the bounding box.
	*/
	gpart = (GSERIALIZED*)PG_DETOAST_DATUM_SLICE(gsdatum, 0, 8 + sizeof(BOX2DF));
	flags = gpart->flags;

	POSTGIS_DEBUGF(4, "got flags %d", gpart->flags);

	/* Do we even have a serialized bounding box? */
	if ( FLAGS_GET_BBOX(flags) )
	{
		/* Yes! Copy it out into the box! */
		POSTGIS_DEBUG(4, "copying box out of serialization");
		memcpy(box2df, gpart->data, sizeof(BOX2DF));
		result = LW_SUCCESS;
	}
	else
	{
		/* No, we need to calculate it from the full object. */
		GBOX gbox;
		GSERIALIZED *g = (GSERIALIZED*)PG_DETOAST_DATUM(gsdatum);
		LWGEOM *lwgeom = lwgeom_from_gserialized(g);
		if ( lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE )
		{
			POSTGIS_DEBUG(4, "could not calculate bbox, returning failure");
			lwgeom_free(lwgeom);
			return LW_FAILURE;
		}
		lwgeom_free(lwgeom);
		result = box2df_from_gbox_p(&gbox, box2df);
	}
	
	if ( result == LW_SUCCESS )
	{
		POSTGIS_DEBUGF(4, "got box2df %s", box2df_to_string(box2df));
	}

	return result;
}


/**
* Support function. Based on two datums return true if
* they satisfy the predicate and false otherwise.
*/
static int 
gserialized_datum_predicate_2d(Datum gs1, Datum gs2, box2df_predicate predicate)
{
	BOX2DF b1, b2;
	
	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for each arguement (ie, not empty geometry)
	   and overlap boxes to return true. */
	if ( (gserialized_datum_get_box2df_p(gs1, &b1) == LW_SUCCESS) &&
	     (gserialized_datum_get_box2df_p(gs2, &b2) == LW_SUCCESS) &&
	      predicate(&b1, &b2) )
	{
		POSTGIS_DEBUGF(3, "got boxes %s and %s", box2df_to_string(&b1), box2df_to_string(&b2));
		return LW_TRUE;
	}
	return LW_FALSE;
}




/***********************************************************************
* GiST 2-D Index Operator Functions
*/

PG_FUNCTION_INFO_V1(gserialized_same_2d);
Datum gserialized_same_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_equals) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_within_2d);
Datum gserialized_within_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_within) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_contains_2d);
Datum gserialized_contains_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_contains) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_overlaps_2d);
Datum gserialized_overlaps_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overlaps) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_left_2d);
Datum gserialized_left_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_left) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_right_2d);
Datum gserialized_right_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_right) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_above_2d);
Datum gserialized_above_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_above) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_below_2d);
Datum gserialized_below_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_below) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_overleft_2d);
Datum gserialized_overleft_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overleft) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_overright_2d);
Datum gserialized_overright_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overright) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_overabove_2d);
Datum gserialized_overabove_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overabove) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(gserialized_overbelow_2d);
Datum gserialized_overbelow_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overbelow) == LW_TRUE )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}


/***********************************************************************
* GiST Index  Support Functions
*/

/*
** GiST support function. Given a geography, return a "compressed"
** version. In this case, we convert the geography into a geocentric
** bounding box. If the geography already has the box embedded in it
** we pull that out and hand it back.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_compress_2d);
Datum gserialized_gist_compress_2d(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry_in = (GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *entry_out = NULL;
	BOX2DF bbox_out;
	int result = LW_SUCCESS;

	POSTGIS_DEBUG(4, "[GIST] 'compress' function called");

	/*
	** Not a leaf key? There's nothing to do.
	** Return the input unchanged.
	*/
	if ( ! entry_in->leafkey )
	{
		POSTGIS_DEBUG(4, "[GIST] non-leafkey entry, returning input unaltered");
		PG_RETURN_POINTER(entry_in);
	}

	POSTGIS_DEBUG(4, "[GIST] processing leafkey input");
	entry_out = palloc(sizeof(GISTENTRY));

	/*
	** Null key? Make a copy of the input entry and
	** return.
	*/
	if ( DatumGetPointer(entry_in->key) == NULL )
	{
		POSTGIS_DEBUG(4, "[GIST] leafkey is null");
		gistentryinit(*entry_out, (Datum) 0, entry_in->rel,
		              entry_in->page, entry_in->offset, FALSE);
		POSTGIS_DEBUG(4, "[GIST] returning copy of input");
		PG_RETURN_POINTER(entry_out);
	}

	/* Extract our index key from the GiST entry. */
	result = gserialized_datum_get_box2df_p(entry_in->key, &bbox_out);

	/* Is the bounding box valid (non-empty, non-infinite)? If not, return input uncompressed. */
	if ( result == LW_FAILURE )
	{
		POSTGIS_DEBUG(4, "[GIST] empty geometry!");
		PG_RETURN_POINTER(entry_in);
	}

	POSTGIS_DEBUGF(4, "[GIST] got entry_in->key: %s", box2df_to_string(&bbox_out));

	/* Check all the dimensions for finite values */
	if ( ! finite(bbox_out.xmax) || ! finite(bbox_out.xmin) ||
	     ! finite(bbox_out.ymax) || ! finite(bbox_out.ymin) )
	{
		POSTGIS_DEBUG(4, "[GIST] infinite geometry!");
		PG_RETURN_POINTER(entry_in);
	}

	/* Enure bounding box has minimums below maximums. */
	box2df_validate(&bbox_out);

	/* Prepare GISTENTRY for return. */
	gistentryinit(*entry_out, PointerGetDatum(box2df_copy(&bbox_out)),
	              entry_in->rel, entry_in->page, entry_in->offset, FALSE);

	/* Return GISTENTRY. */
	POSTGIS_DEBUG(4, "[GIST] 'compress' function complete");
	PG_RETURN_POINTER(entry_out);
}


/*
** GiST support function.
** Decompress an entry. Unused for geography, so we return.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_decompress_2d);
Datum gserialized_gist_decompress_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(5, "[GIST] 'decompress' function called");
	/* We don't decompress. Just return the input. */
	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}



/*
** GiST support function. Called from gserialized_gist_consistent below.
*/
static inline bool gserialized_gist_consistent_leaf_2d(BOX2DF *key, BOX2DF *query, StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(4, "[GIST] leaf consistent, strategy [%d], count[%i]",
	               strategy, geog_counter_leaf++);

	switch (strategy)
	{
	case RTOverlapStrategyNumber:
		retval = (bool) box2df_overlaps(key, query);
		break;
	case RTSameStrategyNumber:
		retval = (bool) box2df_equals(key, query);
		break;
	case RTContainsStrategyNumber:
	case RTOldContainsStrategyNumber:
		retval = (bool) box2df_contains(key, query);
		break;
	case RTContainedByStrategyNumber:
	case RTOldContainedByStrategyNumber:
		retval = (bool) box2df_contains(query, key);
		break;
	default:
		retval = FALSE;
	}

	return (retval);
}

/*
** GiST support function. Called from gserialized_gist_consistent below.
*/
static inline bool gserialized_gist_consistent_internal_2d(BOX2DF *key, BOX2DF *query, StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(4, "[GIST] internal consistent, strategy [%d], count[%i], query[%s], key[%s]",
	               strategy, geog_counter_internal++, box2df_to_string(query), box2df_to_string(key) );

	switch (strategy)
	{
	case RTOverlapStrategyNumber:
		retval = (bool) box2df_overlaps(key, query);
		break;
	case RTSameStrategyNumber:
	case RTContainsStrategyNumber:
	case RTOldContainsStrategyNumber:
		retval = (bool) box2df_contains(key, query);
		break;
	case RTContainedByStrategyNumber:
	case RTOldContainedByStrategyNumber:
		retval = (bool) box2df_overlaps(key, query);
		break;
	default:
		retval = FALSE;
	}

	return (retval);
}

/*
** GiST support function. Take in a query and an entry and see what the
** relationship is, based on the query strategy.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_consistent_2d);
Datum gserialized_gist_consistent_2d(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
	bool result;
	BOX2DF query_gbox_index;

#if POSTGIS_PGSQL_VERSION >= 84
	/* PostgreSQL 8.4 and later require the RECHECK flag to be set here,
	   rather than being supplied as part of the operator class definition */
	bool *recheck = (bool *) PG_GETARG_POINTER(4);

	/* We set recheck to false to avoid repeatedly pulling every "possibly matched" geometry
	   out during index scans. For cases when the geometries are large, rechecking
	   can make things twice as slow. */
	*recheck = false;
#endif

	POSTGIS_DEBUG(4, "[GIST] 'consistent' function called");

	/* Quick sanity check on query argument. */
	if ( DatumGetPointer(PG_GETARG_DATUM(1)) == NULL )
	{
		POSTGIS_DEBUG(4, "[GIST] null query pointer (!?!), returning false");
		PG_RETURN_BOOL(FALSE); /* NULL query! This is screwy! */
	}

	/* Quick sanity check on entry key. */
	if ( DatumGetPointer(entry->key) == NULL )
	{
		POSTGIS_DEBUG(4, "[GIST] null index entry, returning false");
		PG_RETURN_BOOL(FALSE); /* NULL entry! */
	}

	/* Null box should never make this far. */
	if ( gserialized_datum_get_box2df_p(PG_GETARG_DATUM(1), &query_gbox_index) == LW_FAILURE )
	{
		POSTGIS_DEBUG(4, "[GIST] null query_gbox_index!");
		PG_RETURN_BOOL(FALSE);
	}

	/* Treat leaf node tests different from internal nodes */
	if (GIST_LEAF(entry))
	{
		result = gserialized_gist_consistent_leaf_2d(
		             (BOX2DF*)DatumGetPointer(entry->key),
		             &query_gbox_index, strategy);
	}
	else
	{
		result = gserialized_gist_consistent_internal_2d(
		             (BOX2DF*)DatumGetPointer(entry->key),
		             &query_gbox_index, strategy);
	}

	PG_RETURN_BOOL(result);
}

/*
** GiST support function. Calculate the "penalty" cost of adding this entry into an existing entry.
** Calculate the change in volume of the old entry once the new entry is added.
** TODO: Re-evaluate this in light of R*Tree penalty approaches.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_penalty_2d);
Datum gserialized_gist_penalty_2d(PG_FUNCTION_ARGS)
{
	GISTENTRY *origentry = (GISTENTRY*) PG_GETARG_POINTER(0);
	GISTENTRY *newentry = (GISTENTRY*) PG_GETARG_POINTER(1);
	float *result = (float*) PG_GETARG_POINTER(2);
	BOX2DF *gbox_index_orig, *gbox_index_new;
	float size_union, size_orig;

	POSTGIS_DEBUG(4, "[GIST] 'penalty' function called");

	gbox_index_orig = (BOX2DF*)DatumGetPointer(origentry->key);
	gbox_index_new = (BOX2DF*)DatumGetPointer(newentry->key);

	/* Drop out if we're dealing with null inputs. Shouldn't happen. */
	if ( (gbox_index_orig == NULL) && (gbox_index_new == NULL) )
	{
		POSTGIS_DEBUG(4, "[GIST] both inputs NULL! returning penalty of zero");
		*result = 0.0;
		PG_RETURN_FLOAT8(*result);
	}

	/* Calculate the size difference of the boxes. */
	size_union = box2df_union_size(gbox_index_orig, gbox_index_new);
	size_orig = box2df_size(gbox_index_orig);
	*result = size_union - size_orig;

	POSTGIS_DEBUGF(4, "[GIST] 'penalty', union size (%.12f), original size (%.12f), penalty (%.12f)", size_union, size_orig, *result);

	PG_RETURN_POINTER(result);
}

/*
** GiST support function. Merge all the boxes in a page into one master box.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_union_2d);
Datum gserialized_gist_union_2d(PG_FUNCTION_ARGS)
{
	GistEntryVector	*entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	int *sizep = (int *) PG_GETARG_POINTER(1); /* Size of the return value */
	int	numranges, i;
	BOX2DF *box_cur, *box_union;

	POSTGIS_DEBUG(4, "[GIST] 'union' function called");

	numranges = entryvec->n;

	box_cur = (BOX2DF*) DatumGetPointer(entryvec->vector[0].key);

	box_union = box2df_copy(box_cur);

	for ( i = 1; i < numranges; i++ )
	{
		box_cur = (BOX2DF*) DatumGetPointer(entryvec->vector[i].key);
		box2df_merge(box_union, box_cur);
	}

	*sizep = sizeof(BOX2DF);

	POSTGIS_DEBUGF(4, "[GIST] 'union', numranges(%i), pageunion %s", numranges, box2df_to_string(box_union));

	PG_RETURN_POINTER(box_union);

}

/*
** GiST support function. Test equality of keys.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_same_2d);
Datum gserialized_gist_same_2d(PG_FUNCTION_ARGS)
{
	BOX2DF *b1 = (BOX2DF*)PG_GETARG_POINTER(0);
	BOX2DF *b2 = (BOX2DF*)PG_GETARG_POINTER(1);
	bool *result = (bool*)PG_GETARG_POINTER(2);

	POSTGIS_DEBUG(4, "[GIST] 'same' function called");

	*result = box2df_equals(b1, b2);

	PG_RETURN_POINTER(result);
}

typedef struct
{
	BOX2DF *key;
	int pos;
}
KBsort;

static int
compare_KB(const void* a, const void* b)
{
	BOX2DF *abox = ((KBsort*)a)->key;
	BOX2DF *bbox = ((KBsort*)b)->key;
	float sa = (abox->xmax - abox->xmin) * (abox->ymax - abox->ymin);
	float sb = (bbox->xmax - bbox->xmin) * (bbox->ymax - bbox->ymin);

	if ( sa==sb ) return 0;
	return ( sa>sb ) ? 1 : -1;
}

/**
** The GiST PickSplit method
** New linear algorithm, see 'New Linear Node Splitting Algorithm for R-tree',
** C.H.Ang and T.C.Tan
*/
PG_FUNCTION_INFO_V1(gserialized_gist_picksplit_2d);
Datum gserialized_gist_picksplit_2d(PG_FUNCTION_ARGS)
{
	GistEntryVector	*entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);

	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i;
	OffsetNumber *listL, *listR, *listB, *listT;
	BOX2DF *unionL, *unionR, *unionB, *unionT;
	int posL, posR, posB, posT;
	BOX2DF pageunion;
	BOX2DF *cur;
	char direction = ' ';
	bool allisequal = true;
	OffsetNumber maxoff;
	int nbytes;

	POSTGIS_DEBUG(3, "[GIST] 'picksplit' entered");

	posL = posR = posB = posT = 0;

	maxoff = entryvec->n - 1;
	cur = (BOX2DF*) DatumGetPointer(entryvec->vector[FirstOffsetNumber].key);

	memcpy((void *) &pageunion, (void *) cur, sizeof(BOX2DF));

	/* find MBR */
	for (i = OffsetNumberNext(FirstOffsetNumber); i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = (BOX2DF *) DatumGetPointer(entryvec->vector[i].key);

		if ( allisequal == true &&  (
		            pageunion.xmax != cur->xmax ||
		            pageunion.ymax != cur->ymax ||
		            pageunion.xmin != cur->xmin ||
		            pageunion.ymin != cur->ymin
		        ) )
			allisequal = false;

		if (pageunion.xmax < cur->xmax)
			pageunion.xmax = cur->xmax;
		if (pageunion.xmin > cur->xmin)
			pageunion.xmin = cur->xmin;
		if (pageunion.ymax < cur->ymax)
			pageunion.ymax = cur->ymax;
		if (pageunion.ymin > cur->ymin)
			pageunion.ymin = cur->ymin;
	}

	POSTGIS_DEBUGF(4, "pageunion is %s", box2df_to_string(&pageunion));

	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	listL = (OffsetNumber *) palloc(nbytes);
	listR = (OffsetNumber *) palloc(nbytes);
	unionL = (BOX2DF *) palloc(sizeof(BOX2DF));
	unionR = (BOX2DF *) palloc(sizeof(BOX2DF));

	if (allisequal)
	{
		POSTGIS_DEBUG(4, " AllIsEqual!");

		cur = (BOX2DF*) DatumGetPointer(entryvec->vector[OffsetNumberNext(FirstOffsetNumber)].key);


		if (memcmp((void *) cur, (void *) &pageunion, sizeof(BOX2DF)) == 0)
		{
			v->spl_left = listL;
			v->spl_right = listR;
			v->spl_nleft = v->spl_nright = 0;
			memcpy((void *) unionL, (void *) &pageunion, sizeof(BOX2DF));
			memcpy((void *) unionR, (void *) &pageunion, sizeof(BOX2DF));

			for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
			{
				if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
				{
					v->spl_left[v->spl_nleft] = i;
					v->spl_nleft++;
				}
				else
				{
					v->spl_right[v->spl_nright] = i;
					v->spl_nright++;
				}
			}
			v->spl_ldatum = PointerGetDatum(unionL);
			v->spl_rdatum = PointerGetDatum(unionR);

			PG_RETURN_POINTER(v);
		}
	}

	listB = (OffsetNumber *) palloc(nbytes);
	listT = (OffsetNumber *) palloc(nbytes);
	unionB = (BOX2DF *) palloc(sizeof(BOX2DF));
	unionT = (BOX2DF *) palloc(sizeof(BOX2DF));

#define ADDLIST( list, unionD, pos, num ) do { \
	if ( pos ) { \
		if ( unionD->xmax < cur->xmax )    unionD->xmax	= cur->xmax; \
		if ( unionD->xmin	> cur->xmin  ) unionD->xmin	= cur->xmin; \
		if ( unionD->ymax < cur->ymax )    unionD->ymax	= cur->ymax; \
		if ( unionD->ymin	> cur->ymin  ) unionD->ymin	= cur->ymin; \
	} else { \
			memcpy( (void*)unionD, (void*) cur, sizeof( BOX2DF ) );  \
	} \
	list[pos] = num; \
	(pos)++; \
} while(0)

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = (BOX2DF*) DatumGetPointer(entryvec->vector[i].key);

		if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
			ADDLIST(listL, unionL, posL,i);
		else
			ADDLIST(listR, unionR, posR,i);
		if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
			ADDLIST(listB, unionB, posB,i);
		else
			ADDLIST(listT, unionT, posT,i);
	}

	POSTGIS_DEBUGF(4, "unionL is %s", box2df_to_string(unionL));
	POSTGIS_DEBUGF(4, "unionR is %s", box2df_to_string(unionR));
	POSTGIS_DEBUGF(4, "unionT is %s", box2df_to_string(unionT));
	POSTGIS_DEBUGF(4, "unionB is %s", box2df_to_string(unionB));

	/* bad disposition, sort by ascending and resplit */
	if ( (posR==0 || posL==0) && (posT==0 || posB==0) )
	{
		KBsort *arr = (KBsort*)palloc( sizeof(KBsort) * maxoff );
		posL = posR = posB = posT = 0;
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
		{
			arr[i-1].key = (BOX2DF*) DatumGetPointer(entryvec->vector[i].key);
			arr[i-1].pos = i;
		}
		qsort( arr, maxoff, sizeof(KBsort), compare_KB );
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
		{
			cur = arr[i-1].key;
			if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
				ADDLIST(listL, unionL, posL,arr[i-1].pos);
			else if ( cur->xmin - pageunion.xmin == pageunion.xmax - cur->xmax )
			{
				if ( posL>posR )
					ADDLIST(listR, unionR, posR,arr[i-1].pos);
				else
					ADDLIST(listL, unionL, posL,arr[i-1].pos);
			}
			else
				ADDLIST(listR, unionR, posR,arr[i-1].pos);

			if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
				ADDLIST(listB, unionB, posB,arr[i-1].pos);
			else if ( cur->ymin - pageunion.ymin == pageunion.ymax - cur->ymax )
			{
				if ( posB>posT )
					ADDLIST(listT, unionT, posT,arr[i-1].pos);
				else
					ADDLIST(listB, unionB, posB,arr[i-1].pos);
			}
			else
				ADDLIST(listT, unionT, posT,arr[i-1].pos);
		}
		pfree(arr);
	}

	/* which split more optimal? */
	if (Max(posL, posR) < Max(posB, posT))
		direction = 'x';
	else if (Max(posL, posR) > Max(posB, posT))
		direction = 'y';
	else
	{
		float sizeLR, sizeBT;
		BOX2DF interLR, interBT;
		
		if ( box2df_intersection(unionL, unionR, &interLR) == FALSE )
			sizeLR = 0.0;
		else
			sizeLR = box2df_size(&interLR);

		if ( box2df_intersection(unionB, unionT, &interBT) == FALSE )
			sizeBT = 0.0;
		else
			sizeBT = box2df_size(&interBT);
		
		if (sizeLR < sizeBT)
			direction = 'x';
		else
			direction = 'y';
	}

	POSTGIS_DEBUGF(4, "split direction '%c'", direction);

	if (direction == 'x')
	{
		pfree(unionB);
		pfree(listB);
		pfree(unionT);
		pfree(listT);

		v->spl_left = listL;
		v->spl_right = listR;
		v->spl_nleft = posL;
		v->spl_nright = posR;
		v->spl_ldatum = PointerGetDatum(unionL);
		v->spl_rdatum = PointerGetDatum(unionR);
	}
	else
	{
		pfree(unionR);
		pfree(listR);
		pfree(unionL);
		pfree(listL);

		v->spl_left = listB;
		v->spl_right = listT;
		v->spl_nleft = posB;
		v->spl_nright = posT;
		v->spl_ldatum = PointerGetDatum(unionB);
		v->spl_rdatum = PointerGetDatum(unionT);
	}
	
	POSTGIS_DEBUG(4, "[GIST] 'picksplit' completed");
	
	PG_RETURN_POINTER(v);
}


/*
** The BOX32DF key must be defined as a PostgreSQL type, even though it is only
** ever used internally. These no-op stubs are used to bind the type.
*/
PG_FUNCTION_INFO_V1(box2df_in);
Datum box2df_in(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function box2df_in not implemented")));
	PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(box2df_out);
Datum box2df_out(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function box2df_out not implemented")));
	PG_RETURN_POINTER(NULL);
}
