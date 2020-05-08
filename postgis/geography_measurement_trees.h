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
 * ^copyright^
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeodetic_tree.h"
#include "lwgeom_cache.h"

int geography_dwithin_cache(FunctionCallInfo fcinfo,
			    SHARED_GSERIALIZED *g1,
			    SHARED_GSERIALIZED *g2,
			    const SPHEROID *s,
			    double tolerance,
			    int *dwithin);
int geography_distance_cache(FunctionCallInfo fcinfo,
			     SHARED_GSERIALIZED *g1,
			     SHARED_GSERIALIZED *g2,
			     const SPHEROID *s,
			     double *distance);
int geography_tree_distance(const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double tolerance, double* distance);
