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


#ifndef LIBLWGEOM_TOPO_INTERNAL_H
#define LIBLWGEOM_TOPO_INTERNAL_H 1

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "liblwgeom_topo.h"

/************************************************************************
 *
 * Generic SQL handler
 *
 ************************************************************************/

struct LWT_BE_IFACE_T
{
  const LWT_BE_DATA *data;
  const LWT_BE_CALLBACKS *cb;
};

const char* lwt_be_lastErrorMessage(const LWT_BE_IFACE* be);

LWT_BE_TOPOLOGY * lwt_be_loadTopologyByName(LWT_BE_IFACE *be, const char *name);

int lwt_be_freeTopology(LWT_TOPOLOGY *topo);

LWT_ISO_NODE *lwt_be_getNodeWithinDistance2D(LWT_TOPOLOGY *topo,
					     LWPOINT *pt,
					     double dist,
					     uint64_t *numelems,
					     int fields,
					     int64_t limit);

LWT_ISO_NODE *lwt_be_getNodeById(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);

int lwt_be_ExistsCoincidentNode(LWT_TOPOLOGY* topo, LWPOINT* pt);
int lwt_be_insertNodes(LWT_TOPOLOGY *topo, LWT_ISO_NODE *node, uint64_t numelems);

int lwt_be_ExistsEdgeIntersectingPoint(LWT_TOPOLOGY* topo, LWPOINT* pt);

LWT_ELEMID lwt_be_getNextEdgeId(LWT_TOPOLOGY* topo);
LWT_ISO_EDGE *lwt_be_getEdgeById(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);
LWT_ISO_EDGE *lwt_be_getEdgeWithinDistance2D(LWT_TOPOLOGY *topo,
					     LWPOINT *pt,
					     double dist,
					     uint64_t *numelems,
					     int fields,
					     int64_t limit);
int lwt_be_insertEdges(LWT_TOPOLOGY *topo, LWT_ISO_EDGE *edge, uint64_t numelems);
int
lwt_be_updateEdges(LWT_TOPOLOGY* topo, const LWT_ISO_EDGE* sel_edge, int sel_fields, const LWT_ISO_EDGE* upd_edge, int upd_fields, const LWT_ISO_EDGE* exc_edge, int exc_fields);
int
lwt_be_deleteEdges(LWT_TOPOLOGY* topo, const LWT_ISO_EDGE* sel_edge, int sel_fields);

LWT_ELEMID lwt_be_getFaceContainingPoint(LWT_TOPOLOGY* topo, LWPOINT* pt);

int lwt_be_updateTopoGeomEdgeSplit(LWT_TOPOLOGY* topo, LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2);


/************************************************************************
 *
 * Internal objects
 *
 ************************************************************************/

struct LWT_TOPOLOGY_T
{
  const LWT_BE_IFACE *be_iface;
  LWT_BE_TOPOLOGY *be_topo;
  int32_t srid;
  double precision;
  int hasZ;
};

#endif /* LIBLWGEOM_TOPO_INTERNAL_H */
