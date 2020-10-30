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
** [4] A. Korotkov, "A new double sorting-based node splitting algorithm for R-tree",
**     http://syrcose.ispras.ru/2011/files/SYRCoSE2011_Proceedings.pdf#page=36
*/

#include "postgres.h"
#include "access/gist.h"    /* For GiST */
#include "access/itup.h"
#include "access/skey.h"

#include "../postgis_config.h"

#include "liblwgeom.h"         /* For standard geometry types. */
#include "liblwgeom_internal.h"
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "gserialized_gist.h"	     /* For utility functions. */

#include <float.h> /* For FLT_MAX */
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
Datum gserialized_gist_distance_2d(PG_FUNCTION_ARGS);

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
Datum gserialized_distance_box_2d(PG_FUNCTION_ARGS);
Datum gserialized_distance_centroid_2d(PG_FUNCTION_ARGS);
Datum gserialized_contains_box2df_geom_2d(PG_FUNCTION_ARGS);
Datum gserialized_contains_box2df_box2df_2d(PG_FUNCTION_ARGS);
Datum gserialized_within_box2df_geom_2d(PG_FUNCTION_ARGS);
Datum gserialized_within_box2df_box2df_2d(PG_FUNCTION_ARGS);
Datum gserialized_overlaps_box2df_geom_2d(PG_FUNCTION_ARGS);
Datum gserialized_overlaps_box2df_box2df_2d(PG_FUNCTION_ARGS);

/*
** true/false test function type
*/
typedef bool (*box2df_predicate)(const BOX2DF *a, const BOX2DF *b);


static char* box2df_to_string(const BOX2DF *a)
{
	static const int precision = 12;
	char tmp[13 + 4 * OUT_MAX_BYTES_DOUBLE] = {'B', 'O', 'X', '2', 'D', 'F', '(', 0};
	int len = 7;

	if (a == NULL)
		return pstrdup("<NULLPTR>");

	len += lwprint_double(a->xmin, precision, &tmp[len]);
	tmp[len++] = ' ';
	len += lwprint_double(a->ymin, precision, &tmp[len]);
	tmp[len++] = ',';
	tmp[len++] = ' ';
	len += lwprint_double(a->xmax, precision, &tmp[len]);
	tmp[len++] = ' ';
	len += lwprint_double(a->ymax, precision, &tmp[len]);
	tmp[len++] = ')';

	return pstrdup(tmp);
}

/* Allocate a new copy of BOX2DF */
BOX2DF* box2df_copy(BOX2DF *b)
{
	BOX2DF *c = (BOX2DF*)palloc(sizeof(BOX2DF));
	memcpy((void*)c, (void*)b, sizeof(BOX2DF));
	POSTGIS_DEBUGF(5, "copied box2df (%p) to box2df (%p)", b, c);
	return c;
}

inline bool box2df_is_empty(const BOX2DF *a)
{
	if (isnan(a->xmin))
		return true;
	else
		return false;
}

inline void box2df_set_empty(BOX2DF *a)
{
	a->xmin = a->xmax = a->ymin = a->ymax = NAN;
	return;
}

inline void box2df_set_finite(BOX2DF *a)
{
	if ( ! isfinite(a->xmax) )
		a->xmax = FLT_MAX;
	if ( ! isfinite(a->ymax) )
		a->ymax = FLT_MAX;
	if ( ! isfinite(a->ymin) )
		a->ymin = -1*FLT_MAX;
	if ( ! isfinite(a->xmin) )
		a->xmin = -1*FLT_MAX;
	return;
}

/* Enlarge b_union to contain b_new. */
void box2df_merge(BOX2DF *b_union, BOX2DF *b_new)
{

	POSTGIS_DEBUGF(5, "merging %s with %s", box2df_to_string(b_union), box2df_to_string(b_new));
	/* Adjust minimums */
	if (b_union->xmin > b_new->xmin || isnan(b_union->xmin))
		b_union->xmin = b_new->xmin;
	if (b_union->ymin > b_new->ymin || isnan(b_union->ymin))
		b_union->ymin = b_new->ymin;
	/* Adjust maximums */
	if (b_union->xmax < b_new->xmax || isnan(b_union->xmax))
		b_union->xmax = b_new->xmax;
	if (b_union->ymax < b_new->ymax || isnan(b_union->ymax))
		b_union->ymax = b_new->ymax;

	POSTGIS_DEBUGF(5, "merge complete %s", box2df_to_string(b_union));
	return;
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

int box2df_to_gbox_p(BOX2DF *a, GBOX *box)
{
	memset(box, 0, sizeof(GBOX));
	box->xmin = a->xmin;
	box->xmax = a->xmax;
	box->ymin = a->ymin;
	box->ymax = a->ymax;
	return LW_SUCCESS;
}

/***********************************************************************
** BOX2DF tests for 2D index operators.
*/

/* Ensure all minimums are below maximums. */
inline void box2df_validate(BOX2DF *b)
{
	float tmp;

	if ( box2df_is_empty(b) )
		return;

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

bool box2df_overlaps(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	if ( (a->xmin > b->xmax) || (b->xmin > a->xmax) ||
	     (a->ymin > b->ymax) || (b->ymin > a->ymax) )
	{
		return false;
	}

	return true;
}

bool box2df_contains(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b )
		return false;

	/* All things can contain EMPTY (except EMPTY) */
	if ( box2df_is_empty(b) && ! box2df_is_empty(a) )
		return true;

	if ( (a->xmin > b->xmin) || (a->xmax < b->xmax) ||
	     (a->ymin > b->ymin) || (a->ymax < b->ymax) )
	{
		return false;
	}

	return true;
}

static bool box2df_within(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b )
		return false;

	/* EMPTY is within all other things (except EMPTY) */
	if ( box2df_is_empty(a) && ! box2df_is_empty(b) )
		return true;

	if ( (a->xmin < b->xmin) || (a->xmax > b->xmax) ||
	     (a->ymin < b->ymin) || (a->ymax > b->ymax) )
	{
		return false;
	}

	return true;
}

bool box2df_equals(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a && !b )
		return true;
	else if ( !a || !b )
		return false;
	else if ( box2df_is_empty(a) && box2df_is_empty(b) )
		return true;
	else if ( box2df_is_empty(a) || box2df_is_empty(b) )
		return false;
	else if ((a->xmin == b->xmin) && (a->xmax == b->xmax) && (a->ymin == b->ymin) && (a->ymax == b->ymax))
		return true;
	else
		return false;
}

bool box2df_overleft(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.xmax <= b.xmax */
	return a->xmax <= b->xmax;
}

bool box2df_left(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.xmax < b.xmin */
	return a->xmax < b->xmin;
}

bool box2df_right(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.xmin > b.xmax */
	return a->xmin > b->xmax;
}

bool box2df_overright(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.xmin >= b.xmin */
	return a->xmin >= b->xmin;
}

bool box2df_overbelow(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.ymax <= b.ymax */
	return a->ymax <= b->ymax;
}

bool box2df_below(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.ymax < b.ymin */
	return a->ymax < b->ymin;
}

bool box2df_above(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.ymin > b.ymax */
	return a->ymin > b->ymax;
}

bool box2df_overabove(const BOX2DF *a, const BOX2DF *b)
{
	if ( !a || !b || box2df_is_empty(a) || box2df_is_empty(b) )
		return false;

	/* a.ymin >= b.ymin */
	return a->ymin >= b->ymin;
}

/**
* Calculate the centroid->centroid distance between the boxes.
*/
static double box2df_distance_leaf_centroid(const BOX2DF *a, const BOX2DF *b)
{
    /* The centroid->centroid distance between the boxes */
    double a_x = (a->xmax + a->xmin) / 2.0;
    double a_y = (a->ymax + a->ymin) / 2.0;
    double b_x = (b->xmax + b->xmin) / 2.0;
    double b_y = (b->ymax + b->ymin) / 2.0;

    /* This "distance" is only used for comparisons, */
    return sqrt((a_x - b_x) * (a_x - b_x) + (a_y - b_y) * (a_y - b_y));
}

/* Quick distance function */
static inline double pt_distance(double ax, double ay, double bx, double by)
{
	return sqrt((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
}

/**
* Calculate the box->box distance.
*/
static double box2df_distance(const BOX2DF *a, const BOX2DF *b)
{
	/* Check for overlap */
	if ( box2df_overlaps(a, b) )
		return 0.0;

	if ( box2df_left(a, b) )
	{
		if ( box2df_above(a, b) )
			return pt_distance(a->xmax, a->ymin, b->xmin, b->ymax);
		if ( box2df_below(a, b) )
			return pt_distance(a->xmax, a->ymax, b->xmin, b->ymin);
		else
			return (double)b->xmin - (double)a->xmax;
	}
	if ( box2df_right(a, b) )
	{
		if ( box2df_above(a, b) )
			return pt_distance(a->xmin, a->ymin, b->xmax, b->ymax);
		if ( box2df_below(a, b) )
			return pt_distance(a->xmin, a->ymax, b->xmax, b->ymin);
		else
			return (double)a->xmin - (double)b->xmax;
	}
	if ( box2df_above(a, b) )
	{
		if ( box2df_left(a, b) )
			return pt_distance(a->xmax, a->ymin, b->xmin, b->ymax);
		if ( box2df_right(a, b) )
			return pt_distance(a->xmin, a->ymin, b->xmax, b->ymax);
		else
			return (double)a->ymin - (double)b->ymax;
	}
	if ( box2df_below(a, b) )
	{
		if ( box2df_left(a, b) )
			return pt_distance(a->xmax, a->ymax, b->xmin, b->ymin);
		if ( box2df_right(a, b) )
			return pt_distance(a->xmin, a->ymax, b->xmax, b->ymin);
		else
			return (double)b->ymin - (double)a->ymax;
	}

	return FLT_MAX;
}

/**
 * Peak into a #GSERIALIZED datum to find its bounding box and some other metadata. If the box is there, copy it out and
 * return it. If not, calculate the box from the full object and return the box based on that. If no box is available,
 * return #LW_FAILURE, otherwise #LW_SUCCESS.
 */
int
gserialized_datum_get_internals_p(Datum gsdatum, GBOX *gbox, lwflags_t *flags, uint8_t *type, int32_t *srid)
{
	int result = LW_SUCCESS;
	GSERIALIZED *gpart = NULL;
	int need_detoast = PG_GSERIALIZED_DATUM_NEEDS_DETOAST((struct varlena *)gsdatum);
	if (need_detoast)
	{
		gpart = (GSERIALIZED *)PG_DETOAST_DATUM_SLICE(gsdatum, 0, gserialized_max_header_size());
	}
	else
	{
		gpart = (GSERIALIZED *)gsdatum;
	}

	if (!gserialized_has_bbox(gpart) && need_detoast && LWSIZE_GET(gpart->size) >= gserialized_max_header_size())
	{
		/* The headers don't contain a bbox and the object is larger than what we retrieved, so
		 * we now detoast it completely */
		POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
		gpart = (GSERIALIZED *)PG_DETOAST_DATUM(gsdatum);
	}

	result = gserialized_get_gbox_p(gpart, gbox);
	*flags = gserialized_get_lwflags(gpart);
	*srid = gserialized_get_srid(gpart);
	*type = gserialized_get_type(gpart);

	POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
	return result;
}

/**
 * Given a #GSERIALIZED datum, as quickly as possible (peaking into the top
 * of the memory) return the gbox extents. Does not deserialize the geometry,
 * but <em>WARNING</em> returns a slightly larger bounding box than actually
 * encompasses the objects. For geography objects returns geocentric bounding
 * box, for geometry objects returns cartesian bounding box.
 */
int
gserialized_datum_get_gbox_p(Datum gsdatum, GBOX *gbox)
{
	uint8_t type;
	int32_t srid;
	lwflags_t flags;

	return gserialized_datum_get_internals_p(gsdatum, gbox, &flags, &type, &srid);
}

/* Note that this duplicates code from gserialized_datum_get_internals_p. It does so because
 * accessing only the BOX2DF (as floats) is faster and this is a hot path function in indexes
 */
int
gserialized_datum_get_box2df_p(Datum gsdatum, BOX2DF *box2df)
{
	int result = LW_SUCCESS;
	GSERIALIZED *gpart = NULL;
	int need_detoast = PG_GSERIALIZED_DATUM_NEEDS_DETOAST((struct varlena *)gsdatum);
	if (need_detoast)
	{
		gpart = (GSERIALIZED *)PG_DETOAST_DATUM_SLICE(gsdatum, 0, gserialized_max_header_size());
	}
	else
	{
		gpart = (GSERIALIZED *)gsdatum;
	}

	if (gserialized_has_bbox(gpart))
	{
		size_t box_ndims;
		const float *f = gserialized_get_float_box_p(gpart, &box_ndims);
		memcpy(box2df, f, sizeof(BOX2DF));
		result = LW_SUCCESS;
	}
	else
	{
		GBOX gbox = {0};
		if (need_detoast && LWSIZE_GET(gpart->size) >= gserialized_max_header_size())
		{
			/* The headers don't contain a bbox and the object is larger than what we retrieved, so
			 * we now detoast it completely and recheck */
			POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
			gpart = (GSERIALIZED *)PG_DETOAST_DATUM(gsdatum);
		}
		result = gserialized_get_gbox_p(gpart, &gbox);
		if (result == LW_SUCCESS)
		{
			result = box2df_from_gbox_p(&gbox, box2df);
		}
	}

	POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
	return result;
}

/**
* Support function. Based on two datums return true if
* they satisfy the predicate and false otherwise.
*/
static int
gserialized_datum_predicate_2d(Datum gs1, Datum gs2, box2df_predicate predicate)
{
	BOX2DF b1, b2, *br1=NULL, *br2=NULL;
	POSTGIS_DEBUG(3, "entered function");

	if (gserialized_datum_get_box2df_p(gs1, &b1) == LW_SUCCESS) br1 = &b1;
	if (gserialized_datum_get_box2df_p(gs2, &b2) == LW_SUCCESS) br2 = &b2;

	if ( predicate(br1, br2) )
	{
		POSTGIS_DEBUGF(3, "got boxes %s and %s", br1 ? box2df_to_string(&b1) : "(null)", br2 ? box2df_to_string(&b2) : "(null)");
		return LW_TRUE;
	}
	return LW_FALSE;
}

static int
gserialized_datum_predicate_box2df_geom_2d(const BOX2DF *br1, Datum gs2, box2df_predicate predicate)
{
	BOX2DF b2, *br2=NULL;
	POSTGIS_DEBUG(3, "entered function");

	if (gserialized_datum_get_box2df_p(gs2, &b2) == LW_SUCCESS) br2 = &b2;

	if ( predicate(br1, br2) )
	{
		POSTGIS_DEBUGF(3, "got boxes %s", br2 ? box2df_to_string(&b2) : "(null)");
		return LW_TRUE;
	}
	return LW_FALSE;
}

/***********************************************************************
* BRIN 2-D Index Operator Functions
*/

PG_FUNCTION_INFO_V1(gserialized_contains_box2df_geom_2d);
Datum gserialized_contains_box2df_geom_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
        if ( gserialized_datum_predicate_box2df_geom_2d((BOX2DF*)PG_GETARG_POINTER(0), PG_GETARG_DATUM(1), box2df_contains) == LW_TRUE )
                PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_contains_box2df_box2df_2d);
Datum gserialized_contains_box2df_box2df_2d(PG_FUNCTION_ARGS)
{
	if ( box2df_contains((BOX2DF *)PG_GETARG_POINTER(0), (BOX2DF *)PG_GETARG_POINTER(1)))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_within_box2df_geom_2d);
Datum gserialized_within_box2df_geom_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
        if ( gserialized_datum_predicate_box2df_geom_2d((BOX2DF*)PG_GETARG_POINTER(0), PG_GETARG_DATUM(1), box2df_within) == LW_TRUE )
                PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_within_box2df_box2df_2d);
Datum gserialized_within_box2df_box2df_2d(PG_FUNCTION_ARGS)
{
        if ( box2df_within((BOX2DF *)PG_GETARG_POINTER(0), (BOX2DF *)PG_GETARG_POINTER(1)))
                PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overlaps_box2df_geom_2d);
Datum gserialized_overlaps_box2df_geom_2d(PG_FUNCTION_ARGS)
{
        POSTGIS_DEBUG(3, "entered function");
        if ( gserialized_datum_predicate_box2df_geom_2d((BOX2DF*)PG_GETARG_POINTER(0), PG_GETARG_DATUM(1), box2df_overlaps) == LW_TRUE )
                PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overlaps_box2df_box2df_2d);
Datum gserialized_overlaps_box2df_box2df_2d(PG_FUNCTION_ARGS)
{
        if ( box2df_overlaps((BOX2DF *)PG_GETARG_POINTER(0), (BOX2DF *)PG_GETARG_POINTER(1)))
                PG_RETURN_BOOL(true);

        PG_RETURN_BOOL(false);
}

/***********************************************************************
* GiST 2-D Index Operator Functions
*/

PG_FUNCTION_INFO_V1(gserialized_distance_centroid_2d);
Datum gserialized_distance_centroid_2d(PG_FUNCTION_ARGS)
{
	BOX2DF b1, b2;
	Datum gs1 = PG_GETARG_DATUM(0);
	Datum gs2 = PG_GETARG_DATUM(1);

	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for each argument (ie, not empty geometry). */
	if ( (gserialized_datum_get_box2df_p(gs1, &b1) == LW_SUCCESS) &&
	     (gserialized_datum_get_box2df_p(gs2, &b2) == LW_SUCCESS) )
	{
		double distance = box2df_distance_leaf_centroid(&b1, &b2);
		POSTGIS_DEBUGF(3, "got boxes %s and %s", box2df_to_string(&b1), box2df_to_string(&b2));
		PG_RETURN_FLOAT8(distance);
	}
	PG_RETURN_FLOAT8(FLT_MAX);
}

PG_FUNCTION_INFO_V1(gserialized_distance_box_2d);
Datum gserialized_distance_box_2d(PG_FUNCTION_ARGS)
{
	BOX2DF b1, b2;
	Datum gs1 = PG_GETARG_DATUM(0);
	Datum gs2 = PG_GETARG_DATUM(1);

	POSTGIS_DEBUG(3, "entered function");

	/* Must be able to build box for each argument (ie, not empty geometry). */
	if ( (gserialized_datum_get_box2df_p(gs1, &b1) == LW_SUCCESS) &&
	     (gserialized_datum_get_box2df_p(gs2, &b2) == LW_SUCCESS) )
	{
		double distance = box2df_distance(&b1, &b2);
		POSTGIS_DEBUGF(3, "got boxes %s and %s", box2df_to_string(&b1), box2df_to_string(&b2));
		PG_RETURN_FLOAT8(distance);
	}
	PG_RETURN_FLOAT8(FLT_MAX);
}

PG_FUNCTION_INFO_V1(gserialized_same_2d);
Datum gserialized_same_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_equals) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_within_2d);
Datum gserialized_within_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_within) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_contains_2d);
Datum gserialized_contains_2d(PG_FUNCTION_ARGS)
{
	POSTGIS_DEBUG(3, "entered function");
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_contains) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overlaps_2d);
Datum gserialized_overlaps_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overlaps) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_left_2d);
Datum gserialized_left_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_left) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_right_2d);
Datum gserialized_right_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_right) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_above_2d);
Datum gserialized_above_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_above) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_below_2d);
Datum gserialized_below_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_below) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overleft_2d);
Datum gserialized_overleft_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overleft) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overright_2d);
Datum gserialized_overright_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overright) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overabove_2d);
Datum gserialized_overabove_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overabove) == LW_TRUE )
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(gserialized_overbelow_2d);
Datum gserialized_overbelow_2d(PG_FUNCTION_ARGS)
{
	if ( gserialized_datum_predicate_2d(PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), box2df_overbelow) == LW_TRUE )
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
		              entry_in->page, entry_in->offset, false);
		POSTGIS_DEBUG(4, "[GIST] returning copy of input");
		PG_RETURN_POINTER(entry_out);
	}

	/* Extract our index key from the GiST entry. */
	result = gserialized_datum_get_box2df_p(entry_in->key, &bbox_out);

	/* Is the bounding box valid (non-empty, non-infinite)? If not, return input uncompressed. */
	if ( result == LW_FAILURE )
	{
		box2df_set_empty(&bbox_out);
		gistentryinit(*entry_out, PointerGetDatum(box2df_copy(&bbox_out)),
		              entry_in->rel, entry_in->page, entry_in->offset, false);

		POSTGIS_DEBUG(4, "[GIST] empty geometry!");
		PG_RETURN_POINTER(entry_out);
	}

	POSTGIS_DEBUGF(4, "[GIST] got entry_in->key: %s", box2df_to_string(&bbox_out));

	/* Check all the dimensions for finite values */
	if ( ! isfinite(bbox_out.xmax) || ! isfinite(bbox_out.xmin) ||
	     ! isfinite(bbox_out.ymax) || ! isfinite(bbox_out.ymin) )
	{
		box2df_set_finite(&bbox_out);
		gistentryinit(*entry_out, PointerGetDatum(box2df_copy(&bbox_out)),
		              entry_in->rel, entry_in->page, entry_in->offset, false);

		POSTGIS_DEBUG(4, "[GIST] infinite geometry!");
		PG_RETURN_POINTER(entry_out);
	}

	/* Enure bounding box has minimums below maximums. */
	box2df_validate(&bbox_out);

	/* Prepare GISTENTRY for return. */
	gistentryinit(*entry_out, PointerGetDatum(box2df_copy(&bbox_out)),
	              entry_in->rel, entry_in->page, entry_in->offset, false);

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
	               strategy, g2d_counter_leaf++);

	switch (strategy)
	{

	/* Basic overlaps */
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
		retval = (bool) box2df_within(key, query);
		break;

	/* To one side */
	case RTAboveStrategyNumber:
		retval = (bool) box2df_above(key, query);
		break;
	case RTBelowStrategyNumber:
		retval = (bool) box2df_below(key, query);
		break;
	case RTRightStrategyNumber:
		retval = (bool) box2df_right(key, query);
		break;
	case RTLeftStrategyNumber:
		retval = (bool) box2df_left(key, query);
		break;

	/* Overlapping to one side */
	case RTOverAboveStrategyNumber:
		retval = (bool) box2df_overabove(key, query);
		break;
	case RTOverBelowStrategyNumber:
		retval = (bool) box2df_overbelow(key, query);
		break;
	case RTOverRightStrategyNumber:
		retval = (bool) box2df_overright(key, query);
		break;
	case RTOverLeftStrategyNumber:
		retval = (bool) box2df_overleft(key, query);
		break;

	default:
		retval = false;
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
	               strategy, g2d_counter_internal++, box2df_to_string(query), box2df_to_string(key) );

	switch (strategy)
	{

	/* Basic overlaps */
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

	/* To one side */
	case RTAboveStrategyNumber:
		retval = (bool)(!box2df_overbelow(key, query));
		break;
	case RTBelowStrategyNumber:
		retval = (bool)(!box2df_overabove(key, query));
		break;
	case RTRightStrategyNumber:
		retval = (bool)(!box2df_overleft(key, query));
		break;
	case RTLeftStrategyNumber:
		retval = (bool)(!box2df_overright(key, query));
		break;

	/* Overlapping to one side */
	case RTOverAboveStrategyNumber:
		retval = (bool)(!box2df_below(key, query));
		break;
	case RTOverBelowStrategyNumber:
		retval = (bool)(!box2df_above(key, query));
		break;
	case RTOverRightStrategyNumber:
		retval = (bool)(!box2df_left(key, query));
		break;
	case RTOverLeftStrategyNumber:
		retval = (bool)(!box2df_right(key, query));
		break;

	default:
		retval = false;
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

	/* PostgreSQL 8.4 and later require the RECHECK flag to be set here,
	   rather than being supplied as part of the operator class definition */
	bool *recheck = (bool *) PG_GETARG_POINTER(4);

	/* We set recheck to false to avoid repeatedly pulling every "possibly matched" geometry
	   out during index scans. For cases when the geometries are large, rechecking
	   can make things twice as slow. */
	*recheck = false;

	POSTGIS_DEBUG(4, "[GIST] 'consistent' function called");

	/* Quick sanity check on query argument. */
	if ( DatumGetPointer(PG_GETARG_DATUM(1)) == NULL )
	{
		POSTGIS_DEBUG(4, "[GIST] null query pointer (!?!), returning false");
		PG_RETURN_BOOL(false); /* NULL query! This is screwy! */
	}

	/* Quick sanity check on entry key. */
	if ( DatumGetPointer(entry->key) == NULL )
	{
		POSTGIS_DEBUG(4, "[GIST] null index entry, returning false");
		PG_RETURN_BOOL(false); /* NULL entry! */
	}

	/* Null box should never make this far. */
	if ( gserialized_datum_get_box2df_p(PG_GETARG_DATUM(1), &query_gbox_index) == LW_FAILURE )
	{
		POSTGIS_DEBUG(4, "[GIST] null query_gbox_index!");
		PG_RETURN_BOOL(false);
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
** GiST support function. Take in a query and an entry and return the "distance"
** between them.
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
** Strategy 13 = true distance tests <->
** Strategy 14 = box-based distance tests <#>
*/
PG_FUNCTION_INFO_V1(gserialized_gist_distance_2d);
Datum gserialized_gist_distance_2d(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
	BOX2DF query_box;
	BOX2DF *entry_box;
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
	double distance;
	bool *recheck = (bool *) PG_GETARG_POINTER(4);

	POSTGIS_DEBUG(4, "[GIST] 'distance' function called");

	/* We are using '13' as the gist true-distance <-> strategy number
	*  and '14' as the gist distance-between-boxes <#> strategy number */
	if ( strategy != 13 && strategy != 14 ) {
		elog(ERROR, "unrecognized strategy number: %d", strategy);
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* Null box should never make this far. */
	if ( gserialized_datum_get_box2df_p(PG_GETARG_DATUM(1), &query_box) == LW_FAILURE )
	{
		POSTGIS_DEBUG(4, "[GIST] null query_gbox_index!");
		PG_RETURN_FLOAT8(FLT_MAX);
	}

	/* Get the entry box */
	entry_box = (BOX2DF*)DatumGetPointer(entry->key);

	/* Box-style distance test */
	if ( strategy == 14 ) /* operator <#> */
	{
		distance = box2df_distance(entry_box, &query_box);
	}
	/* True distance test (formerly centroid distance) */
	else if ( strategy == 13 ) /* operator <-> */
	{
		/* In all cases, since we only have keys (boxes) we'll return */
		/* the minimum possible distance, which is the box2df_distance */
		/* and let the recheck sort things out in the case of leaves */
		distance = box2df_distance(entry_box, &query_box);

		if (GIST_LEAF(entry))
			*recheck = true;
	}
	else
	{
		elog(ERROR, "%s: reached unreachable code", __func__);
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(distance);
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

static inline float
box2df_penalty(const BOX2DF *b1, const BOX2DF *b2)
{
	float b1xmin = b1->xmin, b1xmax = b1->xmax;
	float b1ymin = b1->ymin, b1ymax = b1->ymax;
	float b2xmin = b2->xmin, b2xmax = b2->xmax;
	float b2ymin = b2->ymin, b2ymax = b2->ymax;

	float box_union_xmin = Min(b1xmin, b2xmin), box_union_xmax = Max(b1xmax, b2xmax);
	float box_union_ymin = Min(b1ymin, b2ymin), box_union_ymax = Max(b1ymax, b2ymax);

	float b1dx = b1xmax - b1xmin, b1dy = b1ymax - b1ymin;
	float box_union_dx = box_union_xmax - box_union_xmin, box_union_dy = box_union_ymax - box_union_ymin;

	float box_union_area = box_union_dx * box_union_dy, box1area = b1dx * b1dy;
	float box_union_edge = box_union_dx + box_union_dy, box1edge = b1dx + b1dy;

	float area_extension = box_union_area - box1area;
	float edge_extension = box_union_edge - box1edge;

	/* REALM 1: Area extension is nonzero, return it */
	if (area_extension > FLT_EPSILON)
		return pack_float(area_extension, 1);
	/* REALM 0: Area extension is zero, return nonzero edge extension */
	else if (edge_extension > FLT_EPSILON)
		return pack_float(edge_extension, 0);

	return 0;
}

/*
** GiST support function. Calculate the "penalty" cost of adding this entry into an existing entry.
** Calculate the change in volume of the old entry once the new entry is added.
*/
PG_FUNCTION_INFO_V1(gserialized_gist_penalty_2d);
Datum gserialized_gist_penalty_2d(PG_FUNCTION_ARGS)
{
	GISTENTRY *origentry = (GISTENTRY*) PG_GETARG_POINTER(0);
	GISTENTRY *newentry = (GISTENTRY*) PG_GETARG_POINTER(1);
	float *result = (float*) PG_GETARG_POINTER(2);
	BOX2DF *b1, *b2;

	b1 = (BOX2DF *)DatumGetPointer(origentry->key);
	b2 = (BOX2DF *)DatumGetPointer(newentry->key);

	/* Penalty value of 0 has special code path in Postgres's gistchoose.
	 * It is used as an early exit condition in subtree loop, allowing faster tree descend.
	 * For multi-column index, it lets next column break the tie, possibly more confidently.
	 */
	*result = 0;

	if (b1 && b2 && !box2df_is_empty(b1) && !box2df_is_empty(b2))
		*result = box2df_penalty(b1, b2);

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

/*
 * Adjust BOX2DF b boundaries with insertion of addon.
 */
static void
adjustBox(BOX2DF *b, BOX2DF *addon)
{
	if (b->xmax < addon->xmax || isnan(b->xmax))
		b->xmax = addon->xmax;
	if (b->xmin > addon->xmin || isnan(b->xmin))
		b->xmin = addon->xmin;
	if (b->ymax < addon->ymax || isnan(b->ymax))
		b->ymax = addon->ymax;
	if (b->ymin > addon->ymin || isnan(b->ymin))
		b->ymin = addon->ymin;
}

/*
 * Trivial split: half of entries will be placed on one page
 * and another half - to another
 */
static void
fallbackSplit(GistEntryVector *entryvec, GIST_SPLITVEC *v)
{
	OffsetNumber i,
				maxoff;
	BOX2DF	   *unionL = NULL,
			   *unionR = NULL;
	int			nbytes;

	maxoff = entryvec->n - 1;

	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	v->spl_left = (OffsetNumber *) palloc(nbytes);
	v->spl_right = (OffsetNumber *) palloc(nbytes);
	v->spl_nleft = v->spl_nright = 0;

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		BOX2DF	   *cur = (BOX2DF *) DatumGetPointer(entryvec->vector[i].key);

		if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
		{
			v->spl_left[v->spl_nleft] = i;
			if (unionL == NULL)
			{
				unionL = (BOX2DF *) palloc(sizeof(BOX2DF));
				*unionL = *cur;
			}
			else
				adjustBox(unionL, cur);

			v->spl_nleft++;
		}
		else
		{
			v->spl_right[v->spl_nright] = i;
			if (unionR == NULL)
			{
				unionR = (BOX2DF *) palloc(sizeof(BOX2DF));
				*unionR = *cur;
			}
			else
				adjustBox(unionR, cur);

			v->spl_nright++;
		}
	}

	if (v->spl_ldatum_exists)
		adjustBox(unionL, (BOX2DF *) DatumGetPointer(v->spl_ldatum));
	v->spl_ldatum = BoxPGetDatum(unionL);

	if (v->spl_rdatum_exists)
		adjustBox(unionR, (BOX2DF *) DatumGetPointer(v->spl_rdatum));
	v->spl_rdatum = BoxPGetDatum(unionR);

	v->spl_ldatum_exists = v->spl_rdatum_exists = false;
}

/*
 * Represents information about an entry that can be placed to either group
 * without affecting overlap over selected axis ("common entry").
 */
typedef struct
{
	/* Index of entry in the initial array */
	int			index;
	/* Delta between penalties of entry insertion into different groups */
	float		delta;
} CommonEntry;

/*
 * Context for g_box_consider_split. Contains information about currently
 * selected split and some general information.
 */
typedef struct
{
	int			entriesCount;	/* total number of entries being split */
	BOX2DF		boundingBox;	/* minimum bounding box across all entries */

	/* Information about currently selected split follows */

	bool		first;			/* true if no split was selected yet */

	float		leftUpper;		/* upper bound of left interval */
	float		rightLower;		/* lower bound of right interval */

	float4		ratio;
	float4		overlap;
	int			dim;			/* axis of this split */
	float		range;			/* width of general MBR projection to the
								 * selected axis */
} ConsiderSplitContext;

/*
 * Interval represents projection of box to axis.
 */
typedef struct
{
	float		lower,
				upper;
} SplitInterval;

/*
 * Interval comparison function by lower bound of the interval;
 */
static int
interval_cmp_lower(const void *i1, const void *i2)
{
	float		lower1 = ((const SplitInterval *) i1)->lower,
				lower2 = ((const SplitInterval *) i2)->lower;

	if (isnan(lower1))
	{
		if (isnan(lower2))
			return 0;
		else
			return 1;
	}
	else if (isnan(lower2))
	{
		return -1;
	}
	else
	{
		if (lower1 < lower2)
			return -1;
		else if (lower1 > lower2)
			return 1;
		else
			return 0;
	}
}


/*
 * Interval comparison function by upper bound of the interval;
 */
static int
interval_cmp_upper(const void *i1, const void *i2)
{
	float		upper1 = ((const SplitInterval *) i1)->upper,
				upper2 = ((const SplitInterval *) i2)->upper;

	if (isnan(upper1))
	{
		if (isnan(upper2))
			return 0;
		else
			return -1;
	}
	else if (isnan(upper2))
	{
		return 1;
	}
	else
	{
		if (upper1 < upper2)
			return -1;
		else if (upper1 > upper2)
			return 1;
		else
			return 0;
	}
}

/*
 * Replace negative value with zero.
 */
static inline float
non_negative(float val)
{
	if (val >= 0.0f)
		return val;
	else
		return 0.0f;
}

/*
 * Consider replacement of currently selected split with the better one.
 */
static inline void
g_box_consider_split(ConsiderSplitContext *context, int dimNum,
					 float rightLower, int minLeftCount,
					 float leftUpper, int maxLeftCount)
{
	int			leftCount,
				rightCount;
	float4		ratio,
				overlap;
	float		range;

	POSTGIS_DEBUGF(5, "consider split: dimNum = %d, rightLower = %f, "
		"minLeftCount = %d, leftUpper = %f, maxLeftCount = %d ",
		dimNum, rightLower, minLeftCount, leftUpper, maxLeftCount);

	/*
	 * Calculate entries distribution ratio assuming most uniform distribution
	 * of common entries.
	 */
	if (minLeftCount >= (context->entriesCount + 1) / 2)
	{
		leftCount = minLeftCount;
	}
	else
	{
		if (maxLeftCount <= context->entriesCount / 2)
			leftCount = maxLeftCount;
		else
			leftCount = context->entriesCount / 2;
	}
	rightCount = context->entriesCount - leftCount;

	/*
	 * Ratio of split - quotient between size of lesser group and total
	 * entries count.
	 */
	ratio = ((float4) Min(leftCount, rightCount)) /
		((float4) context->entriesCount);

	if (ratio > LIMIT_RATIO)
	{
		bool		selectthis = false;

		/*
		 * The ratio is acceptable, so compare current split with previously
		 * selected one. Between splits of one dimension we search for minimal
		 * overlap (allowing negative values) and minimal ration (between same
		 * overlaps. We switch dimension if find less overlap (non-negative)
		 * or less range with same overlap.
		 */
		if (dimNum == 0)
			range = context->boundingBox.xmax - context->boundingBox.xmin;
		else
			range = context->boundingBox.ymax - context->boundingBox.ymin;

		overlap = (leftUpper - rightLower) / range;

		/* If there is no previous selection, select this */
		if (context->first)
			selectthis = true;
		else if (context->dim == dimNum)
		{
			/*
			 * Within the same dimension, choose the new split if it has a
			 * smaller overlap, or same overlap but better ratio.
			 */
			if (overlap < context->overlap ||
				(overlap == context->overlap && ratio > context->ratio))
				selectthis = true;
		}
		else
		{
			/*
			 * Across dimensions, choose the new split if it has a smaller
			 * *non-negative* overlap, or same *non-negative* overlap but
			 * bigger range. This condition differs from the one described in
			 * the article. On the datasets where leaf MBRs don't overlap
			 * themselves, non-overlapping splits (i.e. splits which have zero
			 * *non-negative* overlap) are frequently possible. In this case
			 * splits tends to be along one dimension, because most distant
			 * non-overlapping splits (i.e. having lowest negative overlap)
			 * appears to be in the same dimension as in the previous split.
			 * Therefore MBRs appear to be very prolonged along another
			 * dimension, which leads to bad search performance. Using range
			 * as the second split criteria makes MBRs more quadratic. Using
			 * *non-negative* overlap instead of overlap as the first split
			 * criteria gives to range criteria a chance to matter, because
			 * non-overlapping splits are equivalent in this criteria.
			 */
			if (non_negative(overlap) < non_negative(context->overlap) ||
				(range > context->range &&
				 non_negative(overlap) <= non_negative(context->overlap)))
				selectthis = true;
		}

		if (selectthis)
		{
			/* save information about selected split */
			context->first = false;
			context->ratio = ratio;
			context->range = range;
			context->overlap = overlap;
			context->rightLower = rightLower;
			context->leftUpper = leftUpper;
			context->dim = dimNum;
			POSTGIS_DEBUG(5, "split selected");
		}
	}
}

/*
 * Compare common entries by their deltas.
 */
static int
common_entry_cmp(const void *i1, const void *i2)
{
	float		delta1 = ((const CommonEntry *) i1)->delta,
				delta2 = ((const CommonEntry *) i2)->delta;

	if (delta1 < delta2)
		return -1;
	else if (delta1 > delta2)
		return 1;
	else
		return 0;
}

/*
 * --------------------------------------------------------------------------
 * Double sorting split algorithm. This is used for both boxes and points.
 *
 * The algorithm finds split of boxes by considering splits along each axis.
 * Each entry is first projected as an interval on the X-axis, and different
 * ways to split the intervals into two groups are considered, trying to
 * minimize the overlap of the groups. Then the same is repeated for the
 * Y-axis, and the overall best split is chosen. The quality of a split is
 * determined by overlap along that axis and some other criteria (see
 * g_box_consider_split).
 *
 * After that, all the entries are divided into three groups:
 *
 * 1) Entries which should be placed to the left group
 * 2) Entries which should be placed to the right group
 * 3) "Common entries" which can be placed to any of groups without affecting
 *	  of overlap along selected axis.
 *
 * The common entries are distributed by minimizing penalty.
 *
 * For details see:
 * "A new double sorting-based node splitting algorithm for R-tree", A. Korotkov
 * http://syrcose.ispras.ru/2011/files/SYRCoSE2011_Proceedings.pdf#page=36
 * --------------------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(gserialized_gist_picksplit_2d);
Datum gserialized_gist_picksplit_2d(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i,
				maxoff;
	ConsiderSplitContext context;
	BOX2DF	   *box,
			   *leftBox,
			   *rightBox;
	int			dim,
				commonEntriesCount;
	SplitInterval *intervalsLower,
			   *intervalsUpper;
	CommonEntry *commonEntries;
	int			nentries;

	POSTGIS_DEBUG(3, "[GIST] 'picksplit' entered");

	memset(&context, 0, sizeof(ConsiderSplitContext));

	maxoff = entryvec->n - 1;
	nentries = context.entriesCount = maxoff - FirstOffsetNumber + 1;

	/* Allocate arrays for intervals along axes */
	intervalsLower = (SplitInterval *) palloc(nentries * sizeof(SplitInterval));
	intervalsUpper = (SplitInterval *) palloc(nentries * sizeof(SplitInterval));

	/*
	 * Calculate the overall minimum bounding box over all the entries.
	 */
	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		box = (BOX2DF *) DatumGetPointer(entryvec->vector[i].key);
		if (i == FirstOffsetNumber)
			context.boundingBox = *box;
		else
			adjustBox(&context.boundingBox, box);
	}

	POSTGIS_DEBUGF(4, "boundingBox is %s", box2df_to_string(
														&context.boundingBox));

	/*
	 * Iterate over axes for optimal split searching.
	 */
	context.first = true;		/* nothing selected yet */
	for (dim = 0; dim < 2; dim++)
	{
		float		leftUpper,
					rightLower;
		int			i1,
					i2;

		/* Project each entry as an interval on the selected axis. */
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
		{
			box = (BOX2DF *) DatumGetPointer(entryvec->vector[i].key);
			if (dim == 0)
			{
				intervalsLower[i - FirstOffsetNumber].lower = box->xmin;
				intervalsLower[i - FirstOffsetNumber].upper = box->xmax;
			}
			else
			{
				intervalsLower[i - FirstOffsetNumber].lower = box->ymin;
				intervalsLower[i - FirstOffsetNumber].upper = box->ymax;
			}
		}

		/*
		 * Make two arrays of intervals: one sorted by lower bound and another
		 * sorted by upper bound.
		 */
		memcpy(intervalsUpper, intervalsLower,
			   sizeof(SplitInterval) * nentries);
		qsort(intervalsLower, nentries, sizeof(SplitInterval),
			  interval_cmp_lower);
		qsort(intervalsUpper, nentries, sizeof(SplitInterval),
			  interval_cmp_upper);

		/*----
		 * The goal is to form a left and right interval, so that every entry
		 * interval is contained by either left or right interval (or both).
		 *
		 * For example, with the intervals (0,1), (1,3), (2,3), (2,4):
		 *
		 * 0 1 2 3 4
		 * +-+
		 *	 +---+
		 *	   +-+
		 *	   +---+
		 *
		 * The left and right intervals are of the form (0,a) and (b,4).
		 * We first consider splits where b is the lower bound of an entry.
		 * We iterate through all entries, and for each b, calculate the
		 * smallest possible a. Then we consider splits where a is the
		 * upper bound of an entry, and for each a, calculate the greatest
		 * possible b.
		 *
		 * In the above example, the first loop would consider splits:
		 * b=0: (0,1)-(0,4)
		 * b=1: (0,1)-(1,4)
		 * b=2: (0,3)-(2,4)
		 *
		 * And the second loop:
		 * a=1: (0,1)-(1,4)
		 * a=3: (0,3)-(2,4)
		 * a=4: (0,4)-(2,4)
		 */

		/*
		 * Iterate over lower bound of right group, finding smallest possible
		 * upper bound of left group.
		 */
		i1 = 0;
		i2 = 0;
		rightLower = intervalsLower[i1].lower;
		leftUpper = intervalsUpper[i2].lower;
		while (true)
		{
			/*
			 * Find next lower bound of right group.
			 */
			while (i1 < nentries && (rightLower == intervalsLower[i1].lower ||
					isnan(intervalsLower[i1].lower)))
			{
				leftUpper = Max(leftUpper, intervalsLower[i1].upper);
				i1++;
			}
			if (i1 >= nentries)
				break;
			rightLower = intervalsLower[i1].lower;

			/*
			 * Find count of intervals which anyway should be placed to the
			 * left group.
			 */
			while (i2 < nentries && intervalsUpper[i2].upper <= leftUpper)
				i2++;

			/*
			 * Consider found split.
			 */
			g_box_consider_split(&context, dim, rightLower, i1, leftUpper, i2);
		}

		/*
		 * Iterate over upper bound of left group finding greatest possible
		 * lower bound of right group.
		 */
		i1 = nentries - 1;
		i2 = nentries - 1;
		rightLower = intervalsLower[i1].upper;
		leftUpper = intervalsUpper[i2].upper;
		while (true)
		{
			/*
			 * Find next upper bound of left group.
			 */
			while (i2 >= 0 && (leftUpper == intervalsUpper[i2].upper ||
					isnan(intervalsUpper[i2].upper)))
			{
				rightLower = Min(rightLower, intervalsUpper[i2].lower);
				i2--;
			}
			if (i2 < 0)
				break;
			leftUpper = intervalsUpper[i2].upper;

			/*
			 * Find count of intervals which anyway should be placed to the
			 * right group.
			 */
			while (i1 >= 0 && intervalsLower[i1].lower >= rightLower)
				i1--;

			/*
			 * Consider found split.
			 */
			g_box_consider_split(&context, dim,
								 rightLower, i1 + 1, leftUpper, i2 + 1);
		}
	}

	/*
	 * If we failed to find any acceptable splits, use trivial split.
	 */
	if (context.first)
	{
		POSTGIS_DEBUG(4, "no acceptable splits,  trivial split");
		fallbackSplit(entryvec, v);
		PG_RETURN_POINTER(v);
	}

	/*
	 * Ok, we have now selected the split across one axis.
	 *
	 * While considering the splits, we already determined that there will be
	 * enough entries in both groups to reach the desired ratio, but we did
	 * not memorize which entries go to which group. So determine that now.
	 */

	POSTGIS_DEBUGF(4, "split direction: %d", context.dim);

	/* Allocate vectors for results */
	v->spl_left = (OffsetNumber *) palloc(nentries * sizeof(OffsetNumber));
	v->spl_right = (OffsetNumber *) palloc(nentries * sizeof(OffsetNumber));
	v->spl_nleft = 0;
	v->spl_nright = 0;

	/* Allocate bounding boxes of left and right groups */
	leftBox = palloc0(sizeof(BOX2DF));
	rightBox = palloc0(sizeof(BOX2DF));

	/*
	 * Allocate an array for "common entries" - entries which can be placed to
	 * either group without affecting overlap along selected axis.
	 */
	commonEntriesCount = 0;
	commonEntries = (CommonEntry *) palloc(nentries * sizeof(CommonEntry));

	/* Helper macros to place an entry in the left or right group */
#define PLACE_LEFT(box, off)					\
	do {										\
		if (v->spl_nleft > 0)					\
			adjustBox(leftBox, box);			\
		else									\
			*leftBox = *(box);					\
		v->spl_left[v->spl_nleft++] = off;		\
	} while(0)

#define PLACE_RIGHT(box, off)					\
	do {										\
		if (v->spl_nright > 0)					\
			adjustBox(rightBox, box);			\
		else									\
			*rightBox = *(box);					\
		v->spl_right[v->spl_nright++] = off;	\
	} while(0)

	/*
	 * Distribute entries which can be distributed unambiguously, and collect
	 * common entries.
	 */
	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		float		lower,
					upper;

		/*
		 * Get upper and lower bounds along selected axis.
		 */
		box = (BOX2DF *) DatumGetPointer(entryvec->vector[i].key);
		if (context.dim == 0)
		{
			lower = box->xmin;
			upper = box->xmax;
		}
		else
		{
			lower = box->ymin;
			upper = box->ymax;
		}

		if (upper <= context.leftUpper || isnan(upper))
		{
			/* Fits to the left group */
			if (lower >= context.rightLower || isnan(lower))
			{
				/* Fits also to the right group, so "common entry" */
				commonEntries[commonEntriesCount++].index = i;
			}
			else
			{
				/* Doesn't fit to the right group, so join to the left group */
				PLACE_LEFT(box, i);
			}
		}
		else
		{
			/*
			 * Each entry should fit on either left or right group. Since this
			 * entry didn't fit on the left group, it better fit in the right
			 * group.
			 */
			Assert(lower >= context.rightLower);

			/* Doesn't fit to the left group, so join to the right group */
			PLACE_RIGHT(box, i);
		}
	}

	POSTGIS_DEBUGF(4, "leftBox is %s", box2df_to_string(leftBox));
	POSTGIS_DEBUGF(4, "rightBox is %s", box2df_to_string(rightBox));

	/*
	 * Distribute "common entries", if any.
	 */
	if (commonEntriesCount > 0)
	{
		/*
		 * Calculate minimum number of entries that must be placed in both
		 * groups, to reach LIMIT_RATIO.
		 */
		int			m = ceil(LIMIT_RATIO * (double) nentries);

		/*
		 * Calculate delta between penalties of join "common entries" to
		 * different groups.
		 */
		for (i = 0; i < commonEntriesCount; i++)
		{
			box = (BOX2DF *) DatumGetPointer(entryvec->vector[
												commonEntries[i].index].key);
			commonEntries[i].delta = Abs(box2df_penalty(leftBox, box) - box2df_penalty(rightBox, box));
		}

		/*
		 * Sort "common entries" by calculated deltas in order to distribute
		 * the most ambiguous entries first.
		 */
		qsort(commonEntries, commonEntriesCount, sizeof(CommonEntry), common_entry_cmp);

		/*
		 * Distribute "common entries" between groups.
		 */
		for (i = 0; i < commonEntriesCount; i++)
		{
			box = (BOX2DF *) DatumGetPointer(entryvec->vector[
												commonEntries[i].index].key);

			/*
			 * Check if we have to place this entry in either group to achieve
			 * LIMIT_RATIO.
			 */
			if (v->spl_nleft + (commonEntriesCount - i) <= m)
				PLACE_LEFT(box, commonEntries[i].index);
			else if (v->spl_nright + (commonEntriesCount - i) <= m)
				PLACE_RIGHT(box, commonEntries[i].index);
			else
			{
				/* Otherwise select the group by minimal penalty */
				if (box2df_penalty(leftBox, box) < box2df_penalty(rightBox, box))
					PLACE_LEFT(box, commonEntries[i].index);
				else
					PLACE_RIGHT(box, commonEntries[i].index);
			}
		}
	}
	v->spl_ldatum = PointerGetDatum(leftBox);
	v->spl_rdatum = PointerGetDatum(rightBox);

	POSTGIS_DEBUG(4, "[GIST] 'picksplit' completed");

	PG_RETURN_POINTER(v);
}

/*
** The BOX2DF key must be defined as a PostgreSQL type, even though it is only
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
  BOX2DF *box = (BOX2DF *) PG_GETARG_POINTER(0);
  char *result = box2df_to_string(box);
  PG_RETURN_CSTRING(result);
}
