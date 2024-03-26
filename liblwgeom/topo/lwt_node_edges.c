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

#include "lwt_node_edges.h"

void
lwt_nodeEdges_release( LWT_NODE_EDGES *star )
{
  if ( star->numEdges ) {
    if ( star->fields & LWT_COL_EDGE_GEOM )
      _lwt_release_edges(star->edges, star->numEdges);
    else
      lwfree(star->edges);
  }
  lwfree(star);
}

/*
 * @return NULL on error, calling lwerror, otherwise an object to
 *         be released with lwt_nodeEdges_release()
 */
LWT_NODE_EDGES *
lwt_nodeEdges_loadFromDB( LWT_TOPOLOGY *topo, LWT_ELEMID node_id, int fields )
{
  LWT_NODE_EDGES *star = lwalloc(sizeof(LWT_NODE_EDGES));
  star->numEdges = 1;
  /* Get incident edges */
  star->edges = lwt_be_getEdgeByNode( topo, &node_id, &(star->numEdges), fields );
  if (star->numEdges == UINT64_MAX)
  {
	  PGTOPO_BE_ERROR();
    lwfree(star);
	  return NULL;
  }
  star->fields = fields;
  return star;
}
