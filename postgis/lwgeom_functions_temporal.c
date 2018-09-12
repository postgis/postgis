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
 * Copyright (C) 2015 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

/*
 * Return the measure at which interpolated points on the two
 * input lines are at the smallest distance.
 */
Datum ST_ClosestPointOfApproach(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClosestPointOfApproach);
Datum ST_ClosestPointOfApproach(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs0 = PG_GETARG_GSERIALIZED_P(0);
  GSERIALIZED *gs1 = PG_GETARG_GSERIALIZED_P(1);
  /* All checks already performed by liblwgeom, not worth checking again */
  LWGEOM *g0 = lwgeom_from_gserialized(gs0);
  LWGEOM *g1 = lwgeom_from_gserialized(gs1);
  double m = lwgeom_tcpa(g0, g1, NULL);
	lwgeom_free(g0);
	lwgeom_free(g1);
  PG_FREE_IF_COPY(gs0, 0);
  PG_FREE_IF_COPY(gs1, 1);
	if ( m < 0 ) PG_RETURN_NULL();
	PG_RETURN_FLOAT8(m);
}

/*
 * Does the object correctly model a trajectory ?
 * Must be a LINESTRINGM with growing measures
 */
Datum ST_IsValidTrajectory(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_IsValidTrajectory);
Datum ST_IsValidTrajectory(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs0 = PG_GETARG_GSERIALIZED_P(0);
  /* All checks already performed by liblwgeom, not worth checking again */
  LWGEOM *g0 = lwgeom_from_gserialized(gs0);
  int ret = lwgeom_is_trajectory(g0);
  lwgeom_free(g0);
  PG_RETURN_BOOL(ret == LW_TRUE);
}

/*
 * Return the distance between two trajectories at their
 * closest point of approach.
 */
Datum ST_DistanceCPA(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_DistanceCPA);
Datum ST_DistanceCPA(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs0 = PG_GETARG_GSERIALIZED_P(0);
  GSERIALIZED *gs1 = PG_GETARG_GSERIALIZED_P(1);
  /* All checks already performed by liblwgeom, not worth checking again */
  LWGEOM *g0 = lwgeom_from_gserialized(gs0);
  LWGEOM *g1 = lwgeom_from_gserialized(gs1);
  double mindist;
  double m = lwgeom_tcpa(g0, g1, &mindist);
  lwgeom_free(g0);
  lwgeom_free(g1);
  PG_FREE_IF_COPY(gs0, 0);
  PG_FREE_IF_COPY(gs1, 1);
	if ( m < 0 ) PG_RETURN_NULL();
  PG_RETURN_FLOAT8(mindist);
}

/*
 * Return true if the distance between two trajectories at their
 * closest point of approach is within the given max.
 */
Datum ST_CPAWithin(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CPAWithin);
Datum ST_CPAWithin(PG_FUNCTION_ARGS)
{
  GSERIALIZED *gs0 = PG_GETARG_GSERIALIZED_P(0);
  GSERIALIZED *gs1 = PG_GETARG_GSERIALIZED_P(1);
  double maxdist = PG_GETARG_FLOAT8(2);
  /* All checks already performed by liblwgeom, not worth checking again */
  LWGEOM *g0 = lwgeom_from_gserialized(gs0);
  LWGEOM *g1 = lwgeom_from_gserialized(gs1);
  int ret = lwgeom_cpa_within(g0, g1, maxdist);
  lwgeom_free(g0);
  lwgeom_free(g1);
  PG_FREE_IF_COPY(gs0, 0);
  PG_FREE_IF_COPY(gs1, 1);
	PG_RETURN_BOOL( ret == LW_TRUE );
}

