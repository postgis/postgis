/***********************************************************
 *
 * $Id$
 *
 * Comparision function for use in Binary Tree searches
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/geo_decls.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

Datum lwgeom_lt(PG_FUNCTION_ARGS);
Datum lwgeom_le(PG_FUNCTION_ARGS);
Datum lwgeom_eq(PG_FUNCTION_ARGS);
Datum lwgeom_ge(PG_FUNCTION_ARGS);
Datum lwgeom_gt(PG_FUNCTION_ARGS);
Datum lwgeom_cmp(PG_FUNCTION_ARGS);


#define BTREE_SRID_MISMATCH_SEVERITY ERROR

PG_FUNCTION_INFO_V1(lwgeom_lt);
Datum lwgeom_lt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;

	POSTGIS_DEBUG(2, "lwgeom_lt called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUG(3, "lwgeom_lt passed getSRID test");

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	POSTGIS_DEBUG(3, "lwgeom_lt getbox2d_p passed");

	if  ( ! FPeq(box1.xmin , box2.xmin) )
	{
		if  (box1.xmin < box2.xmin)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) )
	{
		if  (box1.ymin < box2.ymin)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) )
	{
		if  (box1.xmax < box2.xmax)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if  (box1.ymax < box2.ymax)
			PG_RETURN_BOOL(TRUE);
	}

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(lwgeom_le);
Datum lwgeom_le(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;

	POSTGIS_DEBUG(2, "lwgeom_le called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( ! FPeq(box1.xmin , box2.xmin) )
	{
		if  (box1.xmin < box2.xmin)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) )
	{
		if  (box1.ymin < box2.ymin)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) )
	{
		if  (box1.xmax < box2.xmax)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if  (box1.ymax < box2.ymax)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(lwgeom_eq);
Datum lwgeom_eq(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;
	bool result;

	POSTGIS_DEBUG(2, "lwgeom_eq called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);
	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( ! (FPeq(box1.xmin, box2.xmin) && FPeq(box1.ymin, box2.ymin) &&
	         FPeq(box1.xmax, box2.xmax) && FPeq(box1.ymax, box2.ymax)) )
	{
		result = FALSE;
	}
	else
	{
		result = TRUE;
	}

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(lwgeom_ge);
Datum lwgeom_ge(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;

	POSTGIS_DEBUG(2, "lwgeom_ge called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( ! FPeq(box1.xmin , box2.xmin) )
	{
		if  (box1.xmin > box2.xmin)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) )
	{
		if  (box1.ymin > box2.ymin)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) )
	{
		if  (box1.xmax > box2.xmax)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if  (box1.ymax > box2.ymax)
		{
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(lwgeom_gt);
Datum lwgeom_gt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;

	POSTGIS_DEBUG(2, "lwgeom_gt called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( ! FPeq(box1.xmin , box2.xmin) )
	{
		if  (box1.xmin > box2.xmin)
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) )
	{
		if  (box1.ymin > box2.ymin)
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) )
	{
		if  (box1.xmax > box2.xmax)
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if  (box1.ymax > box2.ymax)
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(lwgeom_cmp);
Datum lwgeom_cmp(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GSERIALIZED *geom2 = (GSERIALIZED *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GBOX box1;
	GBOX box2;

	POSTGIS_DEBUG(2, "lwgeom_cmp called");

	if (pglwgeom_get_srid(geom1) != pglwgeom_get_srid(geom2))
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
		     "Operation on two GEOMETRIES with different SRIDs\n");
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	pglwgeom_getbox2d_p(geom1, &box1);
	pglwgeom_getbox2d_p(geom2, &box2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( ! FPeq(box1.xmin , box2.xmin) )
	{
		if  (box1.xmin < box2.xmin)
		{
			PG_RETURN_INT32(-1);
		}
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) )
	{
		if  (box1.ymin < box2.ymin)
		{
			PG_RETURN_INT32(-1);
		}
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) )
	{
		if  (box1.xmax < box2.xmax)
		{
			PG_RETURN_INT32(-1);
		}
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if  (box1.ymax < box2.ymax)
		{
			PG_RETURN_INT32(-1);
		}
		PG_RETURN_INT32(1);
	}

	PG_RETURN_INT32(0);
}

