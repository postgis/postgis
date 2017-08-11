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
 * Copyright (C) 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 * Copyright (C) 2010 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2009-2011 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "utils/geo_decls.h"

#include "../postgis_config.h"
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

PG_FUNCTION_INFO_V1(lwgeom_lt);
Datum lwgeom_lt(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
	bool empty1, empty2;

	POSTGIS_DEBUG(2, "lwgeom_lt called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	POSTGIS_DEBUG(3, "lwgeom_lt passed getSRID test");

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	POSTGIS_DEBUG(3, "lwgeom_lt getbox2d_p passed");

	if  ( empty1 != empty2 )
	{
		PG_RETURN_BOOL(FALSE);
	}

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
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
	bool empty1, empty2;

	POSTGIS_DEBUG(2, "lwgeom_le called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( empty1 != empty2 )
	{
		PG_RETURN_BOOL(FALSE);
	}

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
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
  bool empty1, empty2;
	bool result;

	POSTGIS_DEBUG(2, "lwgeom_eq called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	gbox_init(&box1);
	gbox_init(&box2);

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( empty1 != empty2 )
	{
    result = FALSE;
	}
  else if  ( ! (FPeq(box1.xmin, box2.xmin) && FPeq(box1.ymin, box2.ymin) &&
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
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
	bool empty1, empty2;

	POSTGIS_DEBUG(2, "lwgeom_ge called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( empty1 != empty2 )
	{
		PG_RETURN_BOOL(FALSE);
	}

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
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
	bool empty1, empty2;

	POSTGIS_DEBUG(2, "lwgeom_gt called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( empty1 != empty2 )
	{
		PG_RETURN_BOOL(FALSE);
	}

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
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GBOX box1;
	GBOX box2;
	bool empty1, empty2;

	POSTGIS_DEBUG(2, "lwgeom_cmp called");

	error_if_srid_mismatch(gserialized_get_srid(geom1), gserialized_get_srid(geom2));

	empty1 = ( gserialized_get_gbox_p(geom1, &box1) == LW_FAILURE );
	empty2 = ( gserialized_get_gbox_p(geom2, &box2) == LW_FAILURE );

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	if  ( empty1 || empty2 )
	{
		if  ( empty1 && empty2 )
		{
			PG_RETURN_INT32(1);
		}
		else if ( empty1 )
		{
			PG_RETURN_INT32(-1);
		}
		else
		{
			PG_RETURN_INT32(1);
		}
	}

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

