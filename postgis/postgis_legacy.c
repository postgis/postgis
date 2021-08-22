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
 * Copyright (C) 2018-2020 Regina Obe <lr@pcorp.us>
 *
 **********************************************************************/
/******************************************************************************
 * This file is to hold functions we no longer use,
 * but we need to keep because they were used at one time behind SQL API functions.
 * This is to ease pg_upgrade upgrades
 *
 * All functions in this file should throw an error telling the user to upgrade
 * the install
 *
 *****************************************************************************/

#include "postgres.h"
#include "utils/builtins.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"

#define POSTGIS_DEPRECATE(version, funcname) \
	Datum funcname(PG_FUNCTION_ARGS); \
	PG_FUNCTION_INFO_V1(funcname); \
	Datum funcname(PG_FUNCTION_ARGS) \
	{ \
		ereport(ERROR, (\
			errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
			errmsg("A stored procedure tried to use deprecated C function '%s'", \
			       __func__), \
			errdetail("Library function '%s' was deprecated in PostGIS %s", \
			          __func__, version), \
			errhint("Consider running: SELECT postgis_extensions_upgrade()") \
		)); \
		PG_RETURN_POINTER(NULL); \
	}

POSTGIS_DEPRECATE("2.2.0", LWGEOMFromWKB);
POSTGIS_DEPRECATE("2.5.0", pgis_abs_in);
POSTGIS_DEPRECATE("2.5.0", pgis_abs_out);
POSTGIS_DEPRECATE("3.0.0", area);
POSTGIS_DEPRECATE("3.0.0", LWGEOM_area_polygon);
POSTGIS_DEPRECATE("3.0.0", distance);
POSTGIS_DEPRECATE("3.0.0", LWGEOM_mindistance2d);
POSTGIS_DEPRECATE("3.0.0", geomunion);
POSTGIS_DEPRECATE("3.0.0", geos_geomunion);
POSTGIS_DEPRECATE("3.0.0", intersection);
POSTGIS_DEPRECATE("3.0.0", geos_intersection);
POSTGIS_DEPRECATE("3.0.0", difference);
POSTGIS_DEPRECATE("3.0.0", geos_difference);
POSTGIS_DEPRECATE("3.0.0", geos_intersects);
POSTGIS_DEPRECATE("3.0.0", sfcgal_intersects);
POSTGIS_DEPRECATE("3.0.0", intersects3d);
POSTGIS_DEPRECATE("3.0.0", intersects3d_dwithin);
POSTGIS_DEPRECATE("3.0.0", sfcgal_intersects3d);
POSTGIS_DEPRECATE("3.0.0", distance3d);
POSTGIS_DEPRECATE("3.0.0", sfcgal_distance3d);
POSTGIS_DEPRECATE("3.0.0", LWGEOM_mindistance3d);
POSTGIS_DEPRECATE("3.0.0", intersects);
POSTGIS_DEPRECATE("3.0.0", pgis_geometry_accum_finalfn);
POSTGIS_DEPRECATE("3.0.0", postgis_autocache_bbox);
POSTGIS_DEPRECATE("2.0.0", postgis_uses_stats);

/**Start: SFCGAL functions moved to postgis_sfcgal lib, to ease pg_upgrade **/
/**: TODO these can be taken out in a later release once PostGIS 3.0 is EOLd **/
POSTGIS_DEPRECATE("3.1.0", sfcgal_from_ewkt);
POSTGIS_DEPRECATE("3.1.0", sfcgal_area3D);
POSTGIS_DEPRECATE("3.1.0", sfcgal_intersection3D);
POSTGIS_DEPRECATE("3.1.0", sfcgal_difference3D);
POSTGIS_DEPRECATE("3.1.0", sfcgal_union3D);
POSTGIS_DEPRECATE("3.1.0", sfcgal_volume);
POSTGIS_DEPRECATE("3.1.0", sfcgal_extrude);
POSTGIS_DEPRECATE("3.1.0", sfcgal_straight_skeleton);
POSTGIS_DEPRECATE("3.1.0", sfcgal_approximate_medial_axis);
POSTGIS_DEPRECATE("3.1.0", sfcgal_is_planar);
POSTGIS_DEPRECATE("3.1.0", sfcgal_orientation);
POSTGIS_DEPRECATE("3.1.0", sfcgal_force_lhr);
POSTGIS_DEPRECATE("3.1.0", ST_ConstrainedDelaunayTriangles);
POSTGIS_DEPRECATE("3.1.0", sfcgal_tesselate);
POSTGIS_DEPRECATE("3.1.0", sfcgal_minkowski_sum);
POSTGIS_DEPRECATE("3.1.0", sfcgal_make_solid);
POSTGIS_DEPRECATE("3.1.0", sfcgal_is_solid);
POSTGIS_DEPRECATE("3.1.0", postgis_sfcgal_noop);
/**End: SFCGAL functions moved to postgis_sfcgal lib, to ease pg_upgrade **/

POSTGIS_DEPRECATE("3.1.0", LWGEOM_locate_between_m);
POSTGIS_DEPRECATE("3.1.0", postgis_svn_version);
