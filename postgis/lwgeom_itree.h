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
 * Copyright 2024 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

#pragma once

#include "liblwgeom.h"
#include "lwgeom_cache.h"
#include "intervaltree.h"

/**
* Checks for a cache hit against the provided geometry and returns
* a pre-built index structure (RTREE_POLY_CACHE) if one exists. Otherwise
* builds a new one and returns that.
*/
IntervalTree * GetIntervalTree(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1);

bool itree_pip_intersects(const IntervalTree *itree, const LWGEOM *lwpoints);
bool itree_pip_contains(const IntervalTree *itree, const LWGEOM *lwpoints);
bool itree_pip_covers(const IntervalTree *itree, const LWGEOM *lwpoints);


