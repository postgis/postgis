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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2017-2019 Darafei Praliaskouski <me@komzpa.net>
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
#include "access/gist.h" /* For GiST */
#include "access/itup.h"
#include "access/skey.h"

#include "../postgis_config.h"

#include "liblwgeom.h"        /* For standard geometry types. */
#include "lwgeom_pg.h"        /* For debugging macros. */
#include "gserialized_gist.h" /* For utility functions. */
#include "geography.h"

#include <assert.h>
#include <math.h>

/*
** When is a node split not so good? If more than 90% of the entries
** end up in one of the children.
*/
#define LIMIT_RATIO 0.1

/*
** For debugging
*/
#if POSTGIS_DEBUG_LEVEL > 0
static int geog_counter_leaf = 0;
static int geog_counter_internal = 0;
#endif

/*
** ND Index key type stub prototypes
*/
Datum gidx_out(PG_FUNCTION_ARGS);
Datum gidx_in(PG_FUNCTION_ARGS);

/*
** ND GiST prototypes
*/
Datum gserialized_gist_consistent(PG_FUNCTION_ARGS);
Datum gserialized_gist_compress(PG_FUNCTION_ARGS);
Datum gserialized_gist_decompress(PG_FUNCTION_ARGS);
Datum gserialized_gist_penalty(PG_FUNCTION_ARGS);
Datum gserialized_gist_picksplit(PG_FUNCTION_ARGS);
Datum gserialized_gist_union(PG_FUNCTION_ARGS);
Datum gserialized_gist_same(PG_FUNCTION_ARGS);
Datum gserialized_gist_distance(PG_FUNCTION_ARGS);
Datum gserialized_gist_geog_distance(PG_FUNCTION_ARGS);

/*
** ND Operator prototypes
*/
Datum gserialized_overlaps(PG_FUNCTION_ARGS);
Datum gserialized_gidx_geom_overlaps(PG_FUNCTION_ARGS);
Datum gserialized_gidx_gidx_overlaps(PG_FUNCTION_ARGS);
Datum gserialized_contains(PG_FUNCTION_ARGS);
Datum gserialized_gidx_geom_contains(PG_FUNCTION_ARGS);
Datum gserialized_gidx_gidx_contains(PG_FUNCTION_ARGS);
Datum gserialized_within(PG_FUNCTION_ARGS);
Datum gserialized_gidx_geom_within(PG_FUNCTION_ARGS);
Datum gserialized_gidx_gidx_within(PG_FUNCTION_ARGS);
Datum gserialized_same(PG_FUNCTION_ARGS);
Datum gserialized_gidx_geom_same(PG_FUNCTION_ARGS);
Datum gserialized_gidx_gidx_same(PG_FUNCTION_ARGS);
Datum gserialized_distance_nd(PG_FUNCTION_ARGS);

/*
** GIDX true/false test function type
*/
typedef bool (*gidx_predicate)(GIDX *a, GIDX *b);

/* Allocate a new copy of GIDX */
GIDX *
gidx_copy(GIDX *b)
{
	GIDX *c = (GIDX *)palloc(VARSIZE(b));
	POSTGIS_DEBUGF(5, "copied gidx (%p) to gidx (%p)", b, c);
	memcpy((void *)c, (void *)b, VARSIZE(b));
	return c;
}

/* Ensure all minimums are below maximums. */
void
gidx_validate(GIDX *b)
{
	uint32_t i;
	Assert(b);
	POSTGIS_DEBUGF(5, "validating gidx (%s)", gidx_to_string(b));
	for (i = 0; i < GIDX_NDIMS(b); i++)
	{
		if (GIDX_GET_MIN(b, i) > GIDX_GET_MAX(b, i))
		{
			float tmp;
			tmp = GIDX_GET_MIN(b, i);
			GIDX_SET_MIN(b, i, GIDX_GET_MAX(b, i));
			GIDX_SET_MAX(b, i, tmp);
		}
	}
	return;
}

/* An "unknown" GIDX is used to represent the bounds of an EMPTY
   geometry or other-wise unindexable geometry (like one with NaN
   or Inf bounds) */
inline bool
gidx_is_unknown(const GIDX *a)
{
	size_t size = VARSIZE_ANY_EXHDR(a);
	/* "unknown" gidx objects have a too-small size of one float */
	if (size <= 0.0)
		return true;
	return false;
}

void
gidx_set_unknown(GIDX *a)
{
	SET_VARSIZE(a, VARHDRSZ);
}

/* Enlarge b_union to contain b_new. */
void
gidx_merge(GIDX **b_union, GIDX *b_new)
{
	int i, dims_union, dims_new;
	Assert(b_union);
	Assert(*b_union);
	Assert(b_new);

	/* Can't merge an unknown into any thing */
	/* Q: Unknown is 0 dimensions. Should we reset result to unknown instead? (ticket #4232) */
	if (gidx_is_unknown(b_new))
		return;

	/* Merge of unknown and known is known */
	/* Q: Unknown is 0 dimensions. Should we never modify unknown instead? (ticket #4232) */
	if (gidx_is_unknown(*b_union))
	{
		pfree(*b_union);
		*b_union = gidx_copy(b_new);
		return;
	}

	dims_union = GIDX_NDIMS(*b_union);
	dims_new = GIDX_NDIMS(b_new);

	/* Shrink unshared dimensions.
	 * Unset dimension is essentially [-FLT_MAX, FLT_MAX], so we can either trim it or reset to that range.*/
	if (dims_new < dims_union)
	{
		*b_union = (GIDX *)repalloc(*b_union, GIDX_SIZE(dims_new));
		SET_VARSIZE(*b_union, VARSIZE(b_new));
		dims_union = dims_new;
	}

	for (i = 0; i < dims_union; i++)
	{
		/* Adjust minimums */
		GIDX_SET_MIN(*b_union, i, Min(GIDX_GET_MIN(*b_union, i), GIDX_GET_MIN(b_new, i)));
		/* Adjust maximums */
		GIDX_SET_MAX(*b_union, i, Max(GIDX_GET_MAX(*b_union, i), GIDX_GET_MAX(b_new, i)));
	}
}

/* Calculate the volume (in n-d units) of the GIDX */
static float
gidx_volume(GIDX *a)
{
	float result;
	uint32_t i;
	if (!a || gidx_is_unknown(a))
		return 0.0;
	result = GIDX_GET_MAX(a, 0) - GIDX_GET_MIN(a, 0);
	for (i = 1; i < GIDX_NDIMS(a); i++)
		result *= (GIDX_GET_MAX(a, i) - GIDX_GET_MIN(a, i));
	POSTGIS_DEBUGF(5, "calculated volume of %s as %.8g", gidx_to_string(a), result);
	return result;
}

/* Calculate the edge of the GIDX */
static float
gidx_edge(GIDX *a)
{
	float result;
	uint32_t i;
	if (!a || gidx_is_unknown(a))
		return 0.0;
	result = GIDX_GET_MAX(a, 0) - GIDX_GET_MIN(a, 0);
	for (i = 1; i < GIDX_NDIMS(a); i++)
		result += (GIDX_GET_MAX(a, i) - GIDX_GET_MIN(a, i));
	POSTGIS_DEBUGF(5, "calculated edge of %s as %.8g", gidx_to_string(a), result);
	return result;
}

/* Ensure the first argument has the higher dimensionality. */
static void
gidx_dimensionality_check(GIDX **a, GIDX **b)
{
	if (GIDX_NDIMS(*a) < GIDX_NDIMS(*b))
	{
		GIDX *tmp = *b;
		*b = *a;
		*a = tmp;
	}
}

/* Calculate the volume of the union of the boxes. Avoids creating an intermediate box. */
static float
gidx_union_volume(GIDX *a, GIDX *b)
{
	float result;
	int i;
	int ndims_a, ndims_b;

	POSTGIS_DEBUG(5, "entered function");

	if (!a && !b)
	{
		elog(ERROR, "gidx_union_volume received two null arguments");
		return 0.0;
	}

	if (!a || gidx_is_unknown(a))
		return gidx_volume(b);

	if (!b || gidx_is_unknown(b))
		return gidx_volume(a);

	if (gidx_is_unknown(a) && gidx_is_unknown(b))
		return 0.0;

	/* Ensure 'a' has the most dimensions. */
	gidx_dimensionality_check(&a, &b);

	ndims_a = GIDX_NDIMS(a);
	ndims_b = GIDX_NDIMS(b);

	/* Initialize with maximal length of first dimension. */
	result = Max(GIDX_GET_MAX(a, 0), GIDX_GET_MAX(b, 0)) - Min(GIDX_GET_MIN(a, 0), GIDX_GET_MIN(b, 0));

	/* Multiply by maximal length of remaining dimensions. */
	for (i = 1; i < ndims_b; i++)
		result *= (Max(GIDX_GET_MAX(a, i), GIDX_GET_MAX(b, i)) - Min(GIDX_GET_MIN(a, i), GIDX_GET_MIN(b, i)));

	/* Add in dimensions of higher dimensional box. */
	for (i = ndims_b; i < ndims_a; i++)
		result *= (GIDX_GET_MAX(a, i) - GIDX_GET_MIN(a, i));

	POSTGIS_DEBUGF(5, "volume( %s union %s ) = %.12g", gidx_to_string(a), gidx_to_string(b), result);

	return result;
}

/* Calculate the edge of the union of the boxes. Avoids creating an intermediate box. */
static float
gidx_union_edge(GIDX *a, GIDX *b)
{
	float result;
	int i;
	int ndims_a, ndims_b;

	POSTGIS_DEBUG(5, "entered function");

	if (!a && !b)
	{
		elog(ERROR, "gidx_union_edge received two null arguments");
		return 0.0;
	}

	if (!a || gidx_is_unknown(a))
		return gidx_volume(b);

	if (!b || gidx_is_unknown(b))
		return gidx_volume(a);

	if (gidx_is_unknown(a) && gidx_is_unknown(b))
		return 0.0;

	/* Ensure 'a' has the most dimensions. */
	gidx_dimensionality_check(&a, &b);

	ndims_a = GIDX_NDIMS(a);
	ndims_b = GIDX_NDIMS(b);

	/* Initialize with maximal length of first dimension. */
	result = Max(GIDX_GET_MAX(a, 0), GIDX_GET_MAX(b, 0)) - Min(GIDX_GET_MIN(a, 0), GIDX_GET_MIN(b, 0));

	/* Add maximal length of remaining dimensions. */
	for (i = 1; i < ndims_b; i++)
		result += (Max(GIDX_GET_MAX(a, i), GIDX_GET_MAX(b, i)) - Min(GIDX_GET_MIN(a, i), GIDX_GET_MIN(b, i)));

	/* Add in dimensions of higher dimensional box. */
	for (i = ndims_b; i < ndims_a; i++)
		result += (GIDX_GET_MAX(a, i) - GIDX_GET_MIN(a, i));

	POSTGIS_DEBUGF(5, "edge( %s union %s ) = %.12g", gidx_to_string(a), gidx_to_string(b), result);

	return result;
}

/* Calculate the volume of the intersection of the boxes. */
static float
gidx_inter_volume(GIDX *a, GIDX *b)
{
	uint32_t i;
	float result;

	POSTGIS_DEBUG(5, "entered function");

	if (!a || !b)
	{
		elog(ERROR, "gidx_inter_volume received a null argument");
		return 0.0;
	}

	if (gidx_is_unknown(a) || gidx_is_unknown(b))
		return 0.0;

	/* Ensure 'a' has the most dimensions. */
	gidx_dimensionality_check(&a, &b);

	/* Initialize with minimal length of first dimension. */
	result = Min(GIDX_GET_MAX(a, 0), GIDX_GET_MAX(b, 0)) - Max(GIDX_GET_MIN(a, 0), GIDX_GET_MIN(b, 0));

	/* If they are disjoint (max < min) then return zero. */
	if (result < 0.0)
		return 0.0;

	/* Continue for remaining dimensions. */
	for (i = 1; i < GIDX_NDIMS(b); i++)
	{
		float width = Min(GIDX_GET_MAX(a, i), GIDX_GET_MAX(b, i)) - Max(GIDX_GET_MIN(a, i), GIDX_GET_MIN(b, i));
		if (width < 0.0)
			return 0.0;
		/* Multiply by minimal length of remaining dimensions. */
		result *= width;
	}
	POSTGIS_DEBUGF(5, "volume( %s intersection %s ) = %.12g", gidx_to_string(a), gidx_to_string(b), result);
	return result;
}

/*
** Overlapping GIDX box test.
**
** Box(A) Overlaps Box(B) IFF for every dimension d:
**   min(A,d) <= max(B,d) && max(A,d) => min(B,d)
**
** Any missing dimension is assumed by convention to
** overlap whatever finite range available on the
** other operand. See
** http://lists.osgeo.org/pipermail/postgis-devel/2015-February/024757.html
**
** Empty boxes never overlap.
*/
bool
gidx_overlaps(GIDX *a, GIDX *b)
{
	int i, dims_a, dims_b;

	POSTGIS_DEBUG(5, "entered function");

	if (!a || !b)
		return false;

	if (gidx_is_unknown(a) || gidx_is_unknown(b))
		return false;

	dims_a = GIDX_NDIMS(a);
	dims_b = GIDX_NDIMS(b);

	/* For all shared dimensions min(a) > max(b) and min(b) > max(a)
	   Unshared dimensions do not matter */
	for (i = 0; i < Min(dims_a, dims_b); i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(a, i) != FLT_MAX && GIDX_GET_MAX(b, i) != FLT_MAX)
		{
			if (GIDX_GET_MIN(a, i) > GIDX_GET_MAX(b, i))
				return false;
			if (GIDX_GET_MIN(b, i) > GIDX_GET_MAX(a, i))
				return false;
		}
	}

	return true;
}

/*
** Containment GIDX test.
**
** Box(A) CONTAINS Box(B) IFF (pt(A)LL < pt(B)LL) && (pt(A)UR > pt(B)UR)
*/
bool
gidx_contains(GIDX *a, GIDX *b)
{
	uint32_t i, dims_a, dims_b;

	if (!a || !b)
		return false;

	if (gidx_is_unknown(a) || gidx_is_unknown(b))
		return false;

	dims_a = GIDX_NDIMS(a);
	dims_b = GIDX_NDIMS(b);

	/* For all shared dimensions min(a) > min(b) and max(a) < max(b)
	   Unshared dimensions do not matter */
	for (i = 0; i < Min(dims_a, dims_b); i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(a, i) != FLT_MAX && GIDX_GET_MAX(b, i) != FLT_MAX)
		{
			if (GIDX_GET_MIN(a, i) > GIDX_GET_MIN(b, i))
				return false;
			if (GIDX_GET_MAX(a, i) < GIDX_GET_MAX(b, i))
				return false;
		}
	}

	return true;
}

/*
** Equality GIDX test.
**
** Box(A) EQUALS Box(B) IFF (pt(A)LL == pt(B)LL) && (pt(A)UR == pt(B)UR)
*/
bool
gidx_equals(GIDX *a, GIDX *b)
{
	uint32_t i, dims_a, dims_b;

	if (!a && !b)
		return true;
	if (!a || !b)
		return false;

	if (gidx_is_unknown(a) && gidx_is_unknown(b))
		return true;

	if (gidx_is_unknown(a) || gidx_is_unknown(b))
		return false;

	dims_a = GIDX_NDIMS(a);
	dims_b = GIDX_NDIMS(b);

	/* For all shared dimensions min(a) == min(b), max(a) == max(b)
	   Unshared dimensions do not matter */
	for (i = 0; i < Min(dims_a, dims_b); i++)
	{
		/* If the missing dimension was not padded with -+FLT_MAX */
		if (GIDX_GET_MAX(a, i) != FLT_MAX && GIDX_GET_MAX(b, i) != FLT_MAX)
		{
			if (GIDX_GET_MIN(a, i) != GIDX_GET_MIN(b, i))
				return false;
			if (GIDX_GET_MAX(a, i) != GIDX_GET_MAX(b, i))
				return false;
		}
	}
	return true;
}

/**
 * Support function. Based on two datums return true if
 * they satisfy the predicate and false otherwise.
 */
static int
gserialized_datum_predicate(Datum gs1, Datum gs2, gidx_predicate predicate)
{
	/* Put aside some stack memory and use it for GIDX pointers. */
	char boxmem1[GIDX_MAX_SIZE];
	char boxmem2[GIDX_MAX_SIZE];
	GIDX *gidx1 = (GIDX *)boxmem1;
	GIDX *gidx2 = (GIDX *)boxmem2;

	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for each arguement (ie, not empty geometry)
	   and predicate function to return true. */
	if ((gserialized_datum_get_gidx_p(gs1, gidx1) == LW_SUCCESS) &&
	    (gserialized_datum_get_gidx_p(gs2, gidx2) == LW_SUCCESS) && predicate(gidx1, gidx2))
	{
		POSTGIS_DEBUGF(3, "got boxes %s and %s", gidx_to_string(gidx1), gidx_to_string(gidx2));
		return LW_TRUE;
	}
	return LW_FALSE;
}

static int
gserialized_datum_predicate_gidx_geom(GIDX *gidx1, Datum gs2, gidx_predicate predicate)
{
	/* Put aside some stack memory and use it for GIDX pointers. */
	char boxmem2[GIDX_MAX_SIZE];
	GIDX *gidx2 = (GIDX *)boxmem2;

	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for gs2 arguement (ie, not empty geometry)
	   and predicate function to return true. */
	if ((gserialized_datum_get_gidx_p(gs2, gidx2) == LW_SUCCESS) && predicate(gidx1, gidx2))
	{
		POSTGIS_DEBUGF(3, "got boxes %s and %s", gidx_to_string(gidx1), gidx_to_string(gidx2));
		return LW_TRUE;
	}
	return LW_FALSE;
}

static int
gserialized_datum_predicate_geom_gidx(Datum gs1, GIDX *gidx2, gidx_predicate predicate)
{
	/* Put aside some stack memory and use it for GIDX pointers. */
	char boxmem2[GIDX_MAX_SIZE];
	GIDX *gidx1 = (GIDX *)boxmem2;

	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for gs2 arguement (ie, not empty geometry)
	   and predicate function to return true. */
	if ((gserialized_datum_get_gidx_p(gs1, gidx1) == LW_SUCCESS) && predicate(gidx1, gidx2))
	{
		POSTGIS_DEBUGF(3, "got boxes %s and %s", gidx_to_string(gidx1), gidx_to_string(gidx2));
		return LW_TRUE;
	}
	return LW_FALSE;
}

/**
 * Calculate the box->box distance.
 */
static double
gidx_distance(const GIDX *a, const GIDX *b, int m_is_time)
{
	int ndims, i;
	double sum = 0;

	/* Base computation on least available dimensions */
	ndims = Min(GIDX_NDIMS(b), GIDX_NDIMS(a));
	for (i = 0; i < ndims; ++i)
	{
		double d;
		double amin = GIDX_GET_MIN(a, i);
		double amax = GIDX_GET_MAX(a, i);
		double bmin = GIDX_GET_MIN(b, i);
		double bmax = GIDX_GET_MAX(b, i);
		POSTGIS_DEBUGF(3, "A %g - %g", amin, amax);
		POSTGIS_DEBUGF(3, "B %g - %g", bmin, bmax);

		if ((amin <= bmax && amax >= bmin))
		{
			/* overlaps */
			d = 0;
		}
		else if (i == 4 && m_is_time)
		{
			return FLT_MAX;
		}
		else if (bmax < amin)
		{
			/* is "left" */
			d = amin - bmax;
		}
		else
		{
			/* is "right" */
			assert(bmin > amax);
			d = bmin - amax;
		}
		if (!isfinite(d))
		{
			/* Can happen if coordinates are corrupted/NaN */
			continue;
		}
		sum += d * d;
		POSTGIS_DEBUGF(3, "dist %g, squared %g, grows sum to %g", d, d * d, sum);
	}
	return sqrt(sum);
}

/**
 * Return a #GSERIALIZED with an expanded bounding box.
 */
GSERIALIZED *
gserialized_expand(GSERIALIZED *g, double distance)
{
	GBOX gbox;
	gbox_init(&gbox);

	/* Get our bounding box out of the geography, return right away if
	   input is an EMPTY geometry. */
	if (gserialized_get_gbox_p(g, &gbox) == LW_FAILURE)
		return g;

	gbox_expand(&gbox, distance);

	return gserialized_set_gbox(g, &gbox);
}

/***********************************************************************
 * GiST N-D Index Operator Functions
 */

/*
 * do "real" n-d distance
 */
PG_FUNCTION_INFO_V1(gserialized_distance_nd);
Datum gserialized_distance_nd(PG_FUNCTION_ARGS)
{
	/* Feature-to-feature distance */
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lw1 = lwgeom_from_gserialized(geom1);
	LWGEOM *lw2 = lwgeom_from_gserialized(geom2);
	LWGEOM *closest;
	double distance;

	/* Find an exact shortest line w/ the dimensions we support */
	if (lwgeom_has_z(lw1) && lwgeom_has_z(lw2))
	{
		closest = lwgeom_closest_line_3d(lw1, lw2);
		distance = lwgeom_length(closest);
	}
	else
	{
		closest = lwgeom_closest_line(lw1, lw2);
		distance = lwgeom_length_2d(closest);
	}

	/* Can only add the M term if both objects have M */
	if (lwgeom_has_m(lw1) && lwgeom_has_m(lw2))
	{
		double m1 = 0, m2 = 0;
		int usebox = false;
		/* Un-sqrt the distance so we can add extra terms */
		distance = distance * distance;

		if (lwgeom_get_type(lw1) == POINTTYPE)
		{
			POINT4D p;
			lwpoint_getPoint4d_p((LWPOINT *)lw1, &p);
			m1 = p.m;
		}
		else if (lwgeom_get_type(lw1) == LINETYPE)
		{
			LWPOINT *lwp1 = lwline_get_lwpoint(lwgeom_as_lwline(closest), 0);
			m1 = lwgeom_interpolate_point(lw1, lwp1);
			lwpoint_free(lwp1);
		}
		else
			usebox = true;

		if (lwgeom_get_type(lw2) == POINTTYPE)
		{
			POINT4D p;
			lwpoint_getPoint4d_p((LWPOINT *)lw2, &p);
			m2 = p.m;
		}
		else if (lwgeom_get_type(lw2) == LINETYPE)
		{
			LWPOINT *lwp2 = lwline_get_lwpoint(lwgeom_as_lwline(closest), 1);
			m2 = lwgeom_interpolate_point(lw2, lwp2);
			lwpoint_free(lwp2);
		}
		else
			usebox = true;

		if (usebox)
		{
			GBOX b1, b2;
			if (gserialized_get_gbox_p(geom1, &b1) && gserialized_get_gbox_p(geom2, &b2))
			{
				double d;
				/* Disjoint left */
				if (b1.mmin > b2.mmax)
					d = b1.mmin - b2.mmax;
				/* Disjoint right */
				else if (b2.mmin > b1.mmax)
					d = b2.mmin - b1.mmax;
				/* Not Disjoint */
				else
					d = 0;
				distance += d * d;
			}
		}
		else
			distance += (m2 - m1) * (m2 - m1);

		distance = sqrt(distance);
	}

	lwgeom_free(closest);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_FLOAT8(distance);
}

/*
** '~~' and operator function. Based on two serialized return true if
** the first is contained by the second.
*/
PG_FUNCTION_INFO_V1(gserialized_within);
Datum gserialized_within(PG_FUNCTION_ARGS)
{
	if (gserialized_datum_predicate(PG_GETARG_DATUM(1), PG_GETARG_DATUM(0), gidx_contains))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '~~' and operator function. Based on a GIDX and a serialized return true if
** the first is contained by the second.
*/
PG_FUNCTION_INFO_V1(gserialized_gidx_geom_within);
Datum gserialized_gidx_geom_within(PG_FUNCTION_ARGS)
{
	GIDX *gidx = (GIDX *)PG_GETARG_POINTER(0);

	if (gserialized_datum_predicate_geom_gidx(PG_GETARG_DATUM(1), gidx, gidx_contains))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '~~' and operator function. Based on two GIDX return true if
** the first is contained by the second.
*/
PG_FUNCTION_INFO_V1(gserialized_gidx_gidx_within);
Datum gserialized_gidx_gidx_within(PG_FUNCTION_ARGS)
{
	if (gidx_contains((GIDX *)PG_GETARG_POINTER(1), (GIDX *)PG_GETARG_POINTER(0)))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '@@' and operator function. Based on two serialized return true if
** the first contains the second.
*/
PG_FUNCTION_INFO_V1(gserialized_contains);
Datum gserialized_contains(PG_FUNCTION_ARGS)
{
	if (gserialized_datum_predicate(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), gidx_contains))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '@@' and operator function. Based on a GIDX and a serialized return true if
** the first contains the second.
*/
PG_FUNCTION_INFO_V1(gserialized_gidx_geom_contains);
Datum gserialized_gidx_geom_contains(PG_FUNCTION_ARGS)
{
	GIDX *gidx = (GIDX *)PG_GETARG_POINTER(0);

	if (gserialized_datum_predicate_gidx_geom(gidx, PG_GETARG_DATUM(1), gidx_contains))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '@@' and operator function. Based on two GIDX return true if
** the first contains the second.
*/
PG_FUNCTION_INFO_V1(gserialized_gidx_gidx_contains);
Datum gserialized_gidx_gidx_contains(PG_FUNCTION_ARGS)
{
	if (gidx_contains((GIDX *)PG_GETARG_POINTER(0), (GIDX *)PG_GETARG_POINTER(1)))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '~=' and operator function. Based on two serialized return true if
** the first equals the second.
*/
PG_FUNCTION_INFO_V1(gserialized_same);
Datum gserialized_same(PG_FUNCTION_ARGS)
{
	if (gserialized_datum_predicate(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), gidx_equals))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_gidx_geom_same);
Datum gserialized_gidx_geom_same(PG_FUNCTION_ARGS)
{
	GIDX *gidx = (GIDX *)PG_GETARG_POINTER(0);

	if (gserialized_datum_predicate_gidx_geom(gidx, PG_GETARG_DATUM(1), gidx_equals))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '~=' and operator function. Based on two GIDX return true if
** the first equals the second.
*/
PG_FUNCTION_INFO_V1(gserialized_gidx_gidx_same);
Datum gserialized_gidx_gidx_same(PG_FUNCTION_ARGS)
{
	if (gidx_equals((GIDX *)PG_GETARG_POINTER(0), (GIDX *)PG_GETARG_POINTER(1)))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
** '&&&' operator function. Based on two serialized return true if
** they overlap and false otherwise.
*/
PG_FUNCTION_INFO_V1(gserialized_overlaps);
Datum gserialized_overlaps(PG_FUNCTION_ARGS)
{
	if (gserialized_datum_predicate(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), gidx_overlaps))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

/*
 * This is the cross-operator for the geographies
 */
PG_FUNCTION_INFO_V1(gserialized_gidx_geog_overlaps);
Datum gserialized_gidx_geog_overlaps(PG_FUNCTION_ARGS)
{
	GIDX *gidx = (GIDX *)PG_GETARG_POINTER(0);

	if (gserialized_datum_predicate_gidx_geom(gidx, PG_GETARG_DATUM(1), gidx_overlaps))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_gidx_geom_overlaps);
Datum gserialized_gidx_geom_overlaps(PG_FUNCTION_ARGS)
{
	GIDX *gidx = (GIDX *)PG_GETARG_POINTER(0);

	if (gserialized_datum_predicate_gidx_geom(gidx, PG_GETARG_DATUM(1), gidx_overlaps))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_gidx_gidx_overlaps);
Datum gserialized_gidx_gidx_overlaps(PG_FUNCTION_ARGS)
{
	if (gidx_overlaps((GIDX *)PG_GETARG_POINTER(0), (GIDX *)PG_GETARG_POINTER(1)))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
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
PG_FUNCTION_INFO_V1(gserialized_gist_compress);
Datum gserialized_gist_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry_in = (GISTENTRY *)PG_GETARG_POINTER(0);
	GISTENTRY *entry_out = NULL;
	char gidxmem[GIDX_MAX_SIZE];
	GIDX *bbox_out = (GIDX *)gidxmem;
	int result = LW_SUCCESS;
	uint32_t i;

	POSTGIS_DEBUG(4, "[GIST] 'compress' function called");

	/*
	** Not a leaf key? There's nothing to do.
	** Return the input unchanged.
	*/
	if (!entry_in->leafkey)
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
	if (!DatumGetPointer(entry_in->key))
	{
		POSTGIS_DEBUG(4, "[GIST] leafkey is null");
		gistentryinit(*entry_out, (Datum)0, entry_in->rel, entry_in->page, entry_in->offset, false);
		POSTGIS_DEBUG(4, "[GIST] returning copy of input");
		PG_RETURN_POINTER(entry_out);
	}

	/* Extract our index key from the GiST entry. */
	result = gserialized_datum_get_gidx_p(entry_in->key, bbox_out);

	/* Is the bounding box valid (non-empty, non-infinite) ?
	 * If not, use the "unknown" GIDX. */
	if (result == LW_FAILURE)
	{
		POSTGIS_DEBUG(4, "[GIST] empty geometry!");
		gidx_set_unknown(bbox_out);
		gistentryinit(*entry_out,
			      PointerGetDatum(gidx_copy(bbox_out)),
			      entry_in->rel,
			      entry_in->page,
			      entry_in->offset,
			      false);
		PG_RETURN_POINTER(entry_out);
	}

	POSTGIS_DEBUGF(4, "[GIST] got entry_in->key: %s", gidx_to_string(bbox_out));

	/* ORIGINAL VERSION */
	/* Check all the dimensions for finite values.
	 * If not, use the "unknown" GIDX as a key */
	for (i = 0; i < GIDX_NDIMS(bbox_out); i++)
	{
		if (!isfinite(GIDX_GET_MAX(bbox_out, i)) || !isfinite(GIDX_GET_MIN(bbox_out, i)))
		{
			gidx_set_unknown(bbox_out);
			gistentryinit(*entry_out,
				      PointerGetDatum(gidx_copy(bbox_out)),
				      entry_in->rel,
				      entry_in->page,
				      entry_in->offset,
				      false);
			PG_RETURN_POINTER(entry_out);
		}
	}

	/* Ensure bounding box has minimums below maximums. */
	gidx_validate(bbox_out);

	/* Prepare GISTENTRY for return. */
	gistentryinit(
	    *entry_out, PointerGetDatum(gidx_copy(bbox_out)), entry_in->rel, entry_in->page, entry_in->offset, false);

	/* Return GISTENTRY. */
	POSTGIS_DEBUG(4, "[GIST] 'compress' function complete");
	PG_RETURN_POINTER(entry_out);
}

/*
** GiST support function.
** Decompress an entry. Unused for geography, so we return.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_decompress);
Datum gserialized_gist_decompress(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(5, "[GIST] 'decompress' function called");
	/* We don't decompress. Just return the input. */
	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}

/*
** GiST support function. Called from gserialized_gist_consistent below.
*/
static inline bool
gserialized_gist_consistent_leaf(GIDX *key, GIDX *query, StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(4, "[GIST] leaf consistent, strategy [%d], count[%i]", strategy, geog_counter_leaf++);

	switch (strategy)
	{
	case RTOverlapStrategyNumber:
		retval = (bool)gidx_overlaps(key, query);
		break;
	case RTSameStrategyNumber:
		retval = (bool)gidx_equals(key, query);
		break;
	case RTContainsStrategyNumber:
	case RTOldContainsStrategyNumber:
		retval = (bool)gidx_contains(key, query);
		break;
	case RTContainedByStrategyNumber:
	case RTOldContainedByStrategyNumber:
		retval = (bool)gidx_contains(query, key);
		break;
	default:
		retval = false;
	}

	return retval;
}

/*
** GiST support function. Called from gserialized_gist_consistent below.
*/
static inline bool
gserialized_gist_consistent_internal(GIDX *key, GIDX *query, StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(4,
		       "[GIST] internal consistent, strategy [%d], count[%i], query[%s], key[%s]",
		       strategy,
		       geog_counter_internal++,
		       gidx_to_string(query),
		       gidx_to_string(key));

	switch (strategy)
	{
	case RTOverlapStrategyNumber:
	case RTContainedByStrategyNumber:
	case RTOldContainedByStrategyNumber:
		retval = (bool)gidx_overlaps(key, query);
		break;
	case RTSameStrategyNumber:
	case RTContainsStrategyNumber:
	case RTOldContainsStrategyNumber:
		retval = (bool)gidx_contains(key, query);
		break;
	default:
		retval = false;
	}

	return retval;
}

/*
** GiST support function. Take in a query and an entry and see what the
** relationship is, based on the query strategy.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_consistent);
Datum gserialized_gist_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY *)PG_GETARG_POINTER(0);
	StrategyNumber strategy = (StrategyNumber)PG_GETARG_UINT16(2);
	bool result;
	char gidxmem[GIDX_MAX_SIZE];
	GIDX *query_gbox_index = (GIDX *)gidxmem;

	/* PostgreSQL 8.4 and later require the RECHECK flag to be set here,
	   rather than being supplied as part of the operator class definition */
	bool *recheck = (bool *)PG_GETARG_POINTER(4);

	/* We set recheck to false to avoid repeatedly pulling every "possibly matched" geometry
	   out during index scans. For cases when the geometries are large, rechecking
	   can make things twice as slow. */
	*recheck = false;

	POSTGIS_DEBUG(4, "[GIST] 'consistent' function called");

	/* Quick sanity check on query argument. */
	if (!DatumGetPointer(PG_GETARG_DATUM(1)))
	{
		POSTGIS_DEBUG(4, "[GIST] null query pointer (!?!), returning false");
		PG_RETURN_BOOL(false); /* NULL query! This is screwy! */
	}

	/* Quick sanity check on entry key. */
	if (!DatumGetPointer(entry->key))
	{
		POSTGIS_DEBUG(4, "[GIST] null index entry, returning false");
		PG_RETURN_BOOL(false); /* NULL entry! */
	}

	/* Null box should never make this far. */
	if (gserialized_datum_get_gidx_p(PG_GETARG_DATUM(1), query_gbox_index) == LW_FAILURE)
	{
		POSTGIS_DEBUG(4, "[GIST] null query_gbox_index!");
		PG_RETURN_BOOL(false);
	}

	/* Treat leaf node tests different from internal nodes */
	if (GIST_LEAF(entry))
	{
		result =
		    gserialized_gist_consistent_leaf((GIDX *)DatumGetPointer(entry->key), query_gbox_index, strategy);
	}
	else
	{
		result = gserialized_gist_consistent_internal(
		    (GIDX *)DatumGetPointer(entry->key), query_gbox_index, strategy);
	}

	PG_RETURN_BOOL(result);
}

/*
** Function to pack floats of different realms.
** This function serves to pack bit flags inside float type.
** Result value represent can be from two different "realms".
** Every value from realm 1 is greater than any value from realm 0.
** Values from the same realm loose one bit of precision.
**
** This technique is possible due to floating point numbers specification
** according to IEEE 754: exponent bits are highest
** (excluding sign bits, but here penalty is always positive). If float a is
** greater than float b, integer A with same bit representation as a is greater
** than integer B with same bits as b.
*/
static inline float
pack_float(const float value, const uint8_t realm)
{
	union {
		float f;
		struct {
			unsigned value : 31, sign : 1;
		} vbits;
		struct {
			unsigned value : 30, realm : 1, sign : 1;
		} rbits;
	} a;

	a.f = value;
	a.rbits.value = a.vbits.value >> 1;
	a.rbits.realm = realm;

	return a.f;
}

/*
** GiST support function. Calculate the "penalty" cost of adding this entry into an existing entry.
** Calculate the change in volume of the old entry once the new entry is added.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_penalty);
Datum gserialized_gist_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY *origentry = (GISTENTRY *)PG_GETARG_POINTER(0);
	GISTENTRY *newentry = (GISTENTRY *)PG_GETARG_POINTER(1);
	float *result = (float *)PG_GETARG_POINTER(2);
	GIDX *gbox_index_orig, *gbox_index_new;

	gbox_index_orig = (GIDX *)DatumGetPointer(origentry->key);
	gbox_index_new = (GIDX *)DatumGetPointer(newentry->key);

	/* Penalty value of 0 has special code path in Postgres's gistchoose.
	 * It is used as an early exit condition in subtree loop, allowing faster tree descend.
	 * For multi-column index, it lets next column break the tie, possibly more confidently.
	 */
	*result = 0.0;

	/* Drop out if we're dealing with null inputs. Shouldn't happen. */
	if (gbox_index_orig && gbox_index_new)
	{
		/* Calculate the size difference of the boxes (volume difference in this case). */
		float size_union = gidx_union_volume(gbox_index_orig, gbox_index_new);
		float size_orig = gidx_volume(gbox_index_orig);
		float volume_extension = size_union - size_orig;

		/* REALM 1: Area extension is nonzero, return it */
		if (volume_extension > FLT_EPSILON)
			*result = pack_float(volume_extension, 1);
		else
		{
			/* REALM 0: Area extension is zero, return nonzero edge extension */
			float edge_union = gidx_union_edge(gbox_index_orig, gbox_index_new);
			float edge_orig = gidx_edge(gbox_index_orig);
			float edge_extension = edge_union - edge_orig;
			if (edge_extension > FLT_EPSILON)
				*result = pack_float(edge_extension, 0);
		}
	}
	PG_RETURN_POINTER(result);
}

/*
** GiST support function. Merge all the boxes in a page into one master box.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_union);
Datum gserialized_gist_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *)PG_GETARG_POINTER(0);
	int *sizep = (int *)PG_GETARG_POINTER(1); /* Size of the return value */
	int numranges, i;
	GIDX *box_cur, *box_union;

	POSTGIS_DEBUG(4, "[GIST] 'union' function called");

	numranges = entryvec->n;

	box_cur = (GIDX *)DatumGetPointer(entryvec->vector[0].key);

	box_union = gidx_copy(box_cur);

	for (i = 1; i < numranges; i++)
	{
		box_cur = (GIDX *)DatumGetPointer(entryvec->vector[i].key);
		gidx_merge(&box_union, box_cur);
	}

	*sizep = VARSIZE(box_union);

	POSTGIS_DEBUGF(4, "[GIST] union called, numranges(%i), pageunion %s", numranges, gidx_to_string(box_union));

	PG_RETURN_POINTER(box_union);
}

/*
** GiST support function. Test equality of keys.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_same);
Datum gserialized_gist_same(PG_FUNCTION_ARGS)
{
	GIDX *b1 = (GIDX *)PG_GETARG_POINTER(0);
	GIDX *b2 = (GIDX *)PG_GETARG_POINTER(1);
	bool *result = (bool *)PG_GETARG_POINTER(2);

	POSTGIS_DEBUG(4, "[GIST] 'same' function called");

	*result = gidx_equals(b1, b2);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(gserialized_gist_geog_distance);
Datum gserialized_gist_geog_distance(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY *)PG_GETARG_POINTER(0);
	Datum query_datum = PG_GETARG_DATUM(1);
	StrategyNumber strategy = (StrategyNumber)PG_GETARG_UINT16(2);
	bool *recheck = (bool *)PG_GETARG_POINTER(4);
	char query_box_mem[GIDX_MAX_SIZE];
	GIDX *query_box = (GIDX *)query_box_mem;
	GIDX *entry_box;
	double distance;

	POSTGIS_DEBUGF(3, "[GIST] '%s' function called", __func__);

	/* We are using '13' as the gist geography distance <-> strategy number */
	if (strategy != 13)
	{
		elog(ERROR, "unrecognized strategy number: %d", strategy);
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* Null box should never make this far. */
	if (gserialized_datum_get_gidx_p(query_datum, query_box) == LW_FAILURE)
	{
		POSTGIS_DEBUG(2, "[GIST] null query_gbox_index!");
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* When we hit leaf nodes, it's time to turn on recheck */
	if (GIST_LEAF(entry))
		*recheck = true;

	/* Get the entry box */
	entry_box = (GIDX *)DatumGetPointer(entry->key);

	/* Return distances from key-based tests should always be */
	/* the minimum possible distance, box-to-box */
	/* We scale up to "world units" so that the box-to-box distances */
	/* compare reasonably with the over-the-spheroid distances that */
	/* the recheck process will turn up */
	distance = WGS84_RADIUS * gidx_distance(entry_box, query_box, 0);
	POSTGIS_DEBUGF(2, "[GIST] '%s' got distance %g", __func__, distance);

	PG_RETURN_FLOAT8(distance);
}

/*
** GiST support function.
** Take in a query and an entry and return the "distance" between them.
**
** Given an index entry p and a query value q, this function determines the
** index entry's "distance" from the query value. This function must be
** supplied if the operator class contains any ordering operators. A query
** using the ordering operator will be implemented by returning index entries
** with the smallest "distance" values first, so the results must be consistent
** with the operator's semantics. For a leaf index entry the result just
** represents the distance to the index entry; for an internal tree node, the
** result must be the smallest distance that any child entry could have.
**
** Strategy 13 is centroid-based distance tests <<->>
** Strategy 20 is cpa based distance tests |=|
*/
PG_FUNCTION_INFO_V1(gserialized_gist_distance);
Datum gserialized_gist_distance(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY *)PG_GETARG_POINTER(0);
	StrategyNumber strategy = (StrategyNumber)PG_GETARG_UINT16(2);
	char query_box_mem[GIDX_MAX_SIZE];
	GIDX *query_box = (GIDX *)query_box_mem;
	GIDX *entry_box;
	bool *recheck = (bool *)PG_GETARG_POINTER(4);

	double distance;

	POSTGIS_DEBUG(4, "[GIST] 'distance' function called");

	/* Strategy 13 is <<->> */
	/* Strategy 20 is |=| */
	if (strategy != 13 && strategy != 20)
	{
		elog(ERROR, "unrecognized strategy number: %d", strategy);
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* Null box should never make this far. */
	if (gserialized_datum_get_gidx_p(PG_GETARG_DATUM(1), query_box) == LW_FAILURE)
	{
		POSTGIS_DEBUG(4, "[GIST] null query_gbox_index!");
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* Get the entry box */
	entry_box = (GIDX *)DatumGetPointer(entry->key);

	/* Strategy 20 is |=| */
	distance = gidx_distance(entry_box, query_box, strategy == 20);

	/* Treat leaf node tests different from internal nodes */
	if (GIST_LEAF(entry))
		*recheck = true;

	PG_RETURN_FLOAT8(distance);
}

/*
** Utility function to add entries to the axis partition lists in the
** picksplit function.
*/
static void
gserialized_gist_picksplit_addlist(OffsetNumber *list, GIDX **box_union, GIDX *box_current, int *pos, int num)
{
	if (*pos)
		gidx_merge(box_union, box_current);
	else
	{
		pfree(*box_union);
		*box_union = gidx_copy(box_current);
	}

	list[*pos] = num;
	(*pos)++;
}

/*
** Utility function check whether the number of entries two halves of the
** space constitute a "bad ratio" (poor balance).
*/
static int
gserialized_gist_picksplit_badratio(int x, int y)
{
	POSTGIS_DEBUGF(4, "[GIST] checking split ratio (%d, %d)", x, y);
	if ((y == 0) || (((float)x / (float)y) < LIMIT_RATIO) || (x == 0) || (((float)y / (float)x) < LIMIT_RATIO))
		return true;

	return false;
}

static bool
gserialized_gist_picksplit_badratios(int *pos, int dims)
{
	int i;
	for (i = 0; i < dims; i++)
	{
		if (gserialized_gist_picksplit_badratio(pos[2 * i], pos[2 * i + 1]) == false)
			return false;
	}
	return true;
}

/*
** Where the picksplit algorithm cannot find any basis for splitting one way
** or another, we simply split the overflowing node in half.
*/
static void
gserialized_gist_picksplit_fallback(GistEntryVector *entryvec, GIST_SPLITVEC *v)
{
	OffsetNumber i, maxoff;
	GIDX *unionL = NULL;
	GIDX *unionR = NULL;
	int nbytes;

	POSTGIS_DEBUG(4, "[GIST] in fallback picksplit function");

	maxoff = entryvec->n - 1;

	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	v->spl_left = (OffsetNumber *)palloc(nbytes);
	v->spl_right = (OffsetNumber *)palloc(nbytes);
	v->spl_nleft = v->spl_nright = 0;

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		GIDX *cur = (GIDX *)DatumGetPointer(entryvec->vector[i].key);

		if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
		{
			v->spl_left[v->spl_nleft] = i;
			if (!unionL)
				unionL = gidx_copy(cur);
			else
				gidx_merge(&unionL, cur);
			v->spl_nleft++;
		}
		else
		{
			v->spl_right[v->spl_nright] = i;
			if (!unionR)
				unionR = gidx_copy(cur);
			else
				gidx_merge(&unionR, cur);
			v->spl_nright++;
		}
	}

	if (v->spl_ldatum_exists)
		gidx_merge(&unionL, (GIDX *)DatumGetPointer(v->spl_ldatum));

	v->spl_ldatum = PointerGetDatum(unionL);

	if (v->spl_rdatum_exists)
		gidx_merge(&unionR, (GIDX *)DatumGetPointer(v->spl_rdatum));

	v->spl_rdatum = PointerGetDatum(unionR);
	v->spl_ldatum_exists = v->spl_rdatum_exists = false;
}

static void
gserialized_gist_picksplit_constructsplit(GIST_SPLITVEC *v,
					  OffsetNumber *list1,
					  int nlist1,
					  GIDX **union1,
					  OffsetNumber *list2,
					  int nlist2,
					  GIDX **union2)
{
	bool firstToLeft = true;

	POSTGIS_DEBUG(4, "[GIST] picksplit in constructsplit function");

	if (v->spl_ldatum_exists || v->spl_rdatum_exists)
	{
		if (v->spl_ldatum_exists && v->spl_rdatum_exists)
		{
			GIDX *LRl = gidx_copy(*union1);
			GIDX *LRr = gidx_copy(*union2);
			GIDX *RLl = gidx_copy(*union2);
			GIDX *RLr = gidx_copy(*union1);
			double sizeLR, sizeRL;

			gidx_merge(&LRl, (GIDX *)DatumGetPointer(v->spl_ldatum));
			gidx_merge(&LRr, (GIDX *)DatumGetPointer(v->spl_rdatum));
			gidx_merge(&RLl, (GIDX *)DatumGetPointer(v->spl_ldatum));
			gidx_merge(&RLr, (GIDX *)DatumGetPointer(v->spl_rdatum));

			sizeLR = gidx_inter_volume(LRl, LRr);
			sizeRL = gidx_inter_volume(RLl, RLr);

			POSTGIS_DEBUGF(4, "[GIST] sizeLR / sizeRL == %.12g / %.12g", sizeLR, sizeRL);

			if (sizeLR > sizeRL)
				firstToLeft = false;
		}
		else
		{
			float p1, p2;
			GISTENTRY oldUnion, addon;

			gistentryinit(oldUnion,
				      (v->spl_ldatum_exists) ? v->spl_ldatum : v->spl_rdatum,
				      NULL,
				      NULL,
				      InvalidOffsetNumber,
				      false);

			gistentryinit(addon, PointerGetDatum(*union1), NULL, NULL, InvalidOffsetNumber, false);
			DirectFunctionCall3(gserialized_gist_penalty,
					    PointerGetDatum(&oldUnion),
					    PointerGetDatum(&addon),
					    PointerGetDatum(&p1));
			gistentryinit(addon, PointerGetDatum(*union2), NULL, NULL, InvalidOffsetNumber, false);
			DirectFunctionCall3(gserialized_gist_penalty,
					    PointerGetDatum(&oldUnion),
					    PointerGetDatum(&addon),
					    PointerGetDatum(&p2));

			POSTGIS_DEBUGF(4, "[GIST] p1 / p2 == %.12g / %.12g", p1, p2);

			if ((v->spl_ldatum_exists && p1 > p2) || (v->spl_rdatum_exists && p1 < p2))
				firstToLeft = false;
		}
	}

	POSTGIS_DEBUGF(4, "[GIST] firstToLeft == %d", firstToLeft);

	if (firstToLeft)
	{
		v->spl_left = list1;
		v->spl_right = list2;
		v->spl_nleft = nlist1;
		v->spl_nright = nlist2;
		if (v->spl_ldatum_exists)
			gidx_merge(union1, (GIDX *)DatumGetPointer(v->spl_ldatum));
		v->spl_ldatum = PointerGetDatum(*union1);
		if (v->spl_rdatum_exists)
			gidx_merge(union2, (GIDX *)DatumGetPointer(v->spl_rdatum));
		v->spl_rdatum = PointerGetDatum(*union2);
	}
	else
	{
		v->spl_left = list2;
		v->spl_right = list1;
		v->spl_nleft = nlist2;
		v->spl_nright = nlist1;
		if (v->spl_ldatum_exists)
			gidx_merge(union2, (GIDX *)DatumGetPointer(v->spl_ldatum));
		v->spl_ldatum = PointerGetDatum(*union2);
		if (v->spl_rdatum_exists)
			gidx_merge(union1, (GIDX *)DatumGetPointer(v->spl_rdatum));
		v->spl_rdatum = PointerGetDatum(*union1);
	}

	v->spl_ldatum_exists = v->spl_rdatum_exists = false;
}

#define BELOW(d) (2 * (d))
#define ABOVE(d) ((2 * (d)) + 1)

/*
** GiST support function. Split an overflowing node into two new nodes.
** Uses linear algorithm from Ang & Tan [2], dividing node extent into
** four quadrants, and splitting along the axis that most evenly distributes
** entries between the new nodes.
** TODO: Re-evaluate this in light of R*Tree picksplit approaches.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_picksplit);
Datum gserialized_gist_picksplit(PG_FUNCTION_ARGS)
{

	GistEntryVector *entryvec = (GistEntryVector *)PG_GETARG_POINTER(0);

	GIST_SPLITVEC *v = (GIST_SPLITVEC *)PG_GETARG_POINTER(1);
	OffsetNumber i;
	/* One union box for each half of the space. */
	GIDX **box_union;
	/* One offset number list for each half of the space. */
	OffsetNumber **list;
	/* One position index for each half of the space. */
	int *pos;
	GIDX *box_pageunion;
	GIDX *box_current;
	int direction = -1;
	bool all_entries_equal = true;
	OffsetNumber max_offset;
	int nbytes, ndims_pageunion, d;
	int posmin = entryvec->n;

	POSTGIS_DEBUG(4, "[GIST] 'picksplit' function called");

	/*
	** First calculate the bounding box and maximum number of dimensions in this page.
	*/

	max_offset = entryvec->n - 1;
	box_current = (GIDX *)DatumGetPointer(entryvec->vector[FirstOffsetNumber].key);
	box_pageunion = gidx_copy(box_current);

	/* Calculate the containing box (box_pageunion) for the whole page we are going to split. */
	for (i = OffsetNumberNext(FirstOffsetNumber); i <= max_offset; i = OffsetNumberNext(i))
	{
		box_current = (GIDX *)DatumGetPointer(entryvec->vector[i].key);

		if (all_entries_equal && !gidx_equals(box_pageunion, box_current))
			all_entries_equal = false;

		gidx_merge(&box_pageunion, box_current);
	}

	POSTGIS_DEBUGF(3, "[GIST] box_pageunion: %s", gidx_to_string(box_pageunion));

	/* Every box in the page is the same! So, we split and just put half the boxes in each child. */
	if (all_entries_equal)
	{
		POSTGIS_DEBUG(4, "[GIST] picksplit finds all entries equal!");
		gserialized_gist_picksplit_fallback(entryvec, v);
		PG_RETURN_POINTER(v);
	}

	/* Initialize memory structures. */
	nbytes = (max_offset + 2) * sizeof(OffsetNumber);
	ndims_pageunion = GIDX_NDIMS(box_pageunion);
	POSTGIS_DEBUGF(4, "[GIST] ndims_pageunion == %d", ndims_pageunion);
	pos = palloc(2 * ndims_pageunion * sizeof(int));
	list = palloc(2 * ndims_pageunion * sizeof(OffsetNumber *));
	box_union = palloc(2 * ndims_pageunion * sizeof(GIDX *));
	for (d = 0; d < ndims_pageunion; d++)
	{
		list[BELOW(d)] = (OffsetNumber *)palloc(nbytes);
		list[ABOVE(d)] = (OffsetNumber *)palloc(nbytes);
		box_union[BELOW(d)] = gidx_new(ndims_pageunion);
		box_union[ABOVE(d)] = gidx_new(ndims_pageunion);
		pos[BELOW(d)] = 0;
		pos[ABOVE(d)] = 0;
	}

	/*
	** Assign each entry in the node to the volume partitions it belongs to,
	** such as "above the x/y plane, left of the y/z plane, below the x/z plane".
	** Each entry thereby ends up in three of the six partitions.
	*/
	POSTGIS_DEBUG(4, "[GIST] 'picksplit' calculating best split axis");
	for (i = FirstOffsetNumber; i <= max_offset; i = OffsetNumberNext(i))
	{
		box_current = (GIDX *)DatumGetPointer(entryvec->vector[i].key);

		for (d = 0; d < ndims_pageunion; d++)
		{
			if (GIDX_GET_MIN(box_current, d) - GIDX_GET_MIN(box_pageunion, d) <
			    GIDX_GET_MAX(box_pageunion, d) - GIDX_GET_MAX(box_current, d))
				gserialized_gist_picksplit_addlist(
				    list[BELOW(d)], &(box_union[BELOW(d)]), box_current, &(pos[BELOW(d)]), i);
			else
				gserialized_gist_picksplit_addlist(
				    list[ABOVE(d)], &(box_union[ABOVE(d)]), box_current, &(pos[ABOVE(d)]), i);
		}
	}

	/*
	** "Bad disposition", too many entries fell into one octant of the space, so no matter which
	** plane we choose to split on, we're going to end up with a mostly full node. Where the
	** data is pretty homogeneous (lots of duplicates) entries that are equidistant from the
	** sides of the page union box can occasionally all end up in one place, leading
	** to this condition.
	*/
	if (gserialized_gist_picksplit_badratios(pos, ndims_pageunion))
	{
		/*
		** Instead we split on center points and see if we do better.
		** First calculate the average center point for each axis.
		*/
		double *avgCenter = palloc(ndims_pageunion * sizeof(double));

		for (d = 0; d < ndims_pageunion; d++)
			avgCenter[d] = 0.0;

		POSTGIS_DEBUG(4, "[GIST] picksplit can't find good split axis, trying center point method");

		for (i = FirstOffsetNumber; i <= max_offset; i = OffsetNumberNext(i))
		{
			box_current = (GIDX *)DatumGetPointer(entryvec->vector[i].key);
			for (d = 0; d < ndims_pageunion; d++)
				avgCenter[d] += (GIDX_GET_MAX(box_current, d) + GIDX_GET_MIN(box_current, d)) / 2.0;
		}
		for (d = 0; d < ndims_pageunion; d++)
		{
			avgCenter[d] /= max_offset;
			pos[BELOW(d)] = pos[ABOVE(d)] = 0; /* Re-initialize our counters. */
			POSTGIS_DEBUGF(4, "[GIST] picksplit average center point[%d] = %.12g", d, avgCenter[d]);
		}

		/* For each of our entries... */
		for (i = FirstOffsetNumber; i <= max_offset; i = OffsetNumberNext(i))
		{
			double center;
			box_current = (GIDX *)DatumGetPointer(entryvec->vector[i].key);

			for (d = 0; d < ndims_pageunion; d++)
			{
				center = (GIDX_GET_MIN(box_current, d) + GIDX_GET_MAX(box_current, d)) / 2.0;
				if (center < avgCenter[d])
					gserialized_gist_picksplit_addlist(
					    list[BELOW(d)], &(box_union[BELOW(d)]), box_current, &(pos[BELOW(d)]), i);
				else if (FPeq(center, avgCenter[d]))
					if (pos[BELOW(d)] > pos[ABOVE(d)])
						gserialized_gist_picksplit_addlist(list[ABOVE(d)],
										   &(box_union[ABOVE(d)]),
										   box_current,
										   &(pos[ABOVE(d)]),
										   i);
					else
						gserialized_gist_picksplit_addlist(list[BELOW(d)],
										   &(box_union[BELOW(d)]),
										   box_current,
										   &(pos[BELOW(d)]),
										   i);
				else
					gserialized_gist_picksplit_addlist(
					    list[ABOVE(d)], &(box_union[ABOVE(d)]), box_current, &(pos[ABOVE(d)]), i);
			}
		}

		/* Do we have a good disposition now? If not, screw it, just cut the node in half. */
		if (gserialized_gist_picksplit_badratios(pos, ndims_pageunion))
		{
			POSTGIS_DEBUG(4,
				      "[GIST] picksplit still cannot find a good split! just cutting the node in half");
			gserialized_gist_picksplit_fallback(entryvec, v);
			PG_RETURN_POINTER(v);
		}
	}

	/*
	** Now, what splitting plane gives us the most even ratio of
	** entries in our child pages? Since each split region has been apportioned entries
	** against the same number of total entries, the axis that has the smallest maximum
	** number of entries in its regions is the most evenly distributed.
	** TODO: what if the distributions are equal in two or more axes?
	*/
	for (d = 0; d < ndims_pageunion; d++)
	{
		int posd = Max(pos[ABOVE(d)], pos[BELOW(d)]);
		if (posd < posmin)
		{
			direction = d;
			posmin = posd;
		}
	}
	if (direction == -1 || posmin == entryvec->n)
		elog(ERROR, "Error in building split, unable to determine split direction.");

	POSTGIS_DEBUGF(3, "[GIST] 'picksplit' splitting on axis %d", direction);

	gserialized_gist_picksplit_constructsplit(v,
						  list[BELOW(direction)],
						  pos[BELOW(direction)],
						  &(box_union[BELOW(direction)]),
						  list[ABOVE(direction)],
						  pos[ABOVE(direction)],
						  &(box_union[ABOVE(direction)]));

	POSTGIS_DEBUGF(4, "[GIST] spl_ldatum: %s", gidx_to_string((GIDX *)v->spl_ldatum));
	POSTGIS_DEBUGF(4, "[GIST] spl_rdatum: %s", gidx_to_string((GIDX *)v->spl_rdatum));

	POSTGIS_DEBUGF(
	    4,
	    "[GIST] axis %d: parent range (%.12g, %.12g) left range (%.12g, %.12g), right range (%.12g, %.12g)",
	    direction,
	    GIDX_GET_MIN(box_pageunion, direction),
	    GIDX_GET_MAX(box_pageunion, direction),
	    GIDX_GET_MIN((GIDX *)v->spl_ldatum, direction),
	    GIDX_GET_MAX((GIDX *)v->spl_ldatum, direction),
	    GIDX_GET_MIN((GIDX *)v->spl_rdatum, direction),
	    GIDX_GET_MAX((GIDX *)v->spl_rdatum, direction));

	PG_RETURN_POINTER(v);
}

/*
** The GIDX key must be defined as a PostgreSQL type, even though it is only
** ever used internally. These no-op stubs are used to bind the type.
*/
PG_FUNCTION_INFO_V1(gidx_in);
Datum gidx_in(PG_FUNCTION_ARGS)
{
	ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("function gidx_in not implemented")));
	PG_RETURN_POINTER(NULL);
}

PG_FUNCTION_INFO_V1(gidx_out);
Datum gidx_out(PG_FUNCTION_ARGS)
{
	GIDX *box = (GIDX *)PG_GETARG_POINTER(0);
	char *result = gidx_to_string(box);
	PG_RETURN_CSTRING(result);
}
