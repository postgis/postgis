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
 *
 **********************************************************************/


#include "postgres.h"
#include "access/hash.h"

#include "../postgis_config.h"

#include "liblwgeom.h"         /* For standard geometry types. */
#include "liblwgeom_internal.h"         /* For FP comparators. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "gserialized_gist.h"
#include "geography.h"	     /* For utility functions. */

Datum geography_lt(PG_FUNCTION_ARGS);
Datum geography_le(PG_FUNCTION_ARGS);
Datum geography_eq(PG_FUNCTION_ARGS);
Datum geography_ge(PG_FUNCTION_ARGS);
Datum geography_gt(PG_FUNCTION_ARGS);
Datum geography_cmp(PG_FUNCTION_ARGS);


/*
** Utility function to return the center point of a
** geocentric bounding box. We don't divide by two
** because we're only using the values for comparison.
*/
static void geography_gidx_center(const GIDX *gidx, POINT3D *p)
{
	p->x = GIDX_GET_MIN(gidx, 0) + GIDX_GET_MAX(gidx, 0);
	p->y = GIDX_GET_MIN(gidx, 1) + GIDX_GET_MAX(gidx, 1);
	p->z = GIDX_GET_MIN(gidx, 2) + GIDX_GET_MAX(gidx, 2);
}

static int geography_cmp_internal(const GSERIALIZED *g1, const GSERIALIZED *g2)
{
	int cmp, have_box1, have_box2;
	int g1_is_empty = gserialized_is_empty(g1);
	int g2_is_empty = gserialized_is_empty(g2);
	size_t sz1 = SIZE_GET(g1->size);
	size_t sz2 = SIZE_GET(g2->size);
	size_t sz = sz1 < sz2 ? sz1 : sz2;

	/* Put aside some stack memory and use it for GIDX pointers. */
	char gboxmem1[GIDX_MAX_SIZE];
	char gboxmem2[GIDX_MAX_SIZE];
	GIDX *gbox1 = (GIDX*)gboxmem1;
	GIDX *gbox2 = (GIDX*)gboxmem2;
	POINT3D p1, p2;

	/* Empty == Empty */
	if (g1_is_empty && g2_is_empty)
    {
        /* POINT EMPTY == POINT EMPTY */
        /* POINT EMPTY < LINESTRING EMPTY */
        uint32_t t1 = gserialized_get_type(g1);
        uint32_t t2 = gserialized_get_type(g2);
		return t1 == t2 ? 0 : (t1 < t2 ? -1 : 1);
    }
	
	/* Empty < Non-empty */
	if (g1_is_empty)
		return -1;
	
	/* Non-empty > Empty */
	if (g2_is_empty)
		return 1;

	/* Return equality for perfect equality only */
	cmp = memcmp(g1, g2, sz);
	if ( sz1 == sz2 && cmp == 0 )
		return 0;
	
	/* Must be able to build box for each argument */
	/* (ie, not empty geometry) */
	have_box1 = gserialized_get_gidx_p(g1, gbox1);
	have_box2 = gserialized_get_gidx_p(g2, gbox2);
	if ( ! (have_box1 && have_box2) )
	{
		lwerror("%s [%d] unable to calculate boxes of geographies", __FILE__, __LINE__);
	}

	/* Order geographies more or less left-to-right */
	/* using the centroids, and preferring the X axis */
	geography_gidx_center(gbox1, &p1);
	geography_gidx_center(gbox2, &p2);

	if  ( ! FP_EQUALS(p1.x, p2.x) )
		return p1.x < p2.x ? -1 : 1;

	if  ( ! FP_EQUALS(p1.y, p2.y) )
		return p1.y < p2.y ? -1 : 1;

	if  ( ! FP_EQUALS(p1.z, p2.z) )
		return p1.z < p2.z ? -1 : 1;

	/* Geographies that are the "same in centroid" */
	/* but didn't pass the exact equality test */
	/* First order smallest-first */
	if (sz1 != sz2)
		return sz1 < sz2 ? -1 : 1;
	/* Then just order on the memcmp return */
	else
		return cmp;
}

/*
** BTree support function. Based on two geographies return true if
** they are "less than" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_lt);
Datum geography_lt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = geography_cmp_internal(g1, g2);
	if (cmp < 0)
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);	
}

/*
** BTree support function. Based on two geographies return true if
** they are "less than or equal" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_le);
Datum geography_le(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = geography_cmp_internal(g1, g2);
	if (cmp <= 0)
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);	
}

/*
** BTree support function. Based on two geographies return true if
** they are "greater than" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_gt);
Datum geography_gt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = geography_cmp_internal(g1, g2);
	if (cmp > 0)
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);	
}

/*
** BTree support function. Based on two geographies return true if
** they are "greater than or equal" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_ge);
Datum geography_ge(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = geography_cmp_internal(g1, g2);
	if (cmp >= 0)
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);	
}

/*
** BTree support function. Based on two geographies return true if
** they are "equal" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_eq);
Datum geography_eq(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	int cmp = geography_cmp_internal(g1, g2);
	if (cmp == 0)
		PG_RETURN_BOOL(TRUE);
	else
		PG_RETURN_BOOL(FALSE);	
}

/*
** BTree support function. Based on two geographies return true if
** they are "equal" and false otherwise.
*/
PG_FUNCTION_INFO_V1(geography_cmp);
Datum geography_cmp(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	PG_RETURN_INT32(geography_cmp_internal(g1, g2));
}


#if 0
/*
** Calculate a hash code based on the geometry data alone
*/
static uint32 geography_hash(GSERIALIZED *g)
{
	return DatumGetUInt32(hash_any((void*)g, VARSIZE(g)));
}
/*
** BTree support function. Based on two geographies return true if
** they are "equal" and false otherwise. This version uses a hash
** function to try and shoot for a more exact equality test.
*/
PG_FUNCTION_INFO_V1(geography_eq);
Datum geography_eq(PG_FUNCTION_ARGS)
{
	/* Perfect equals test based on hash */
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);

	uint32 h1 = geography_hash(g1);
	uint32 h2 = geography_hash(g2);

	PG_FREE_IF_COPY(g1,0);
	PG_FREE_IF_COPY(g2,0);

	if ( h1 == h2 )
		PG_RETURN_BOOL(TRUE);

	PG_RETURN_BOOL(FALSE);
}
#endif


