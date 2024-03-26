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
 * Copyright (C) 2024 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"
#include "lwgeom_geos.h"

typedef struct LWT_NODE_EDGES_t {
  uint64_t numEdges;
  int fields; /* bitwise AND of fields present in edges, see LWT_COL_EDGE_* macros */
  LWT_ISO_EDGE *edges;
} LWT_NODE_EDGES;

/*
 * @param fields fields to load for each edge, see LWT_COL_EDGE_* macros
 *
 * @return NULL on error, calling lwerror, otherwise an object to
 *         be released with lwt_nodeEdges_release()
 */
LWT_NODE_EDGES *lwt_nodeEdges_loadFromDB( LWT_TOPOLOGY *topo, LWT_ELEMID node_id, int fields );

void lwt_nodeEdges_release( LWT_NODE_EDGES *star );

