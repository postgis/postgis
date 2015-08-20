/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
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

LWT_ISO_NODE* lwt_be_getNodeWithinDistance2D(LWT_TOPOLOGY* topo, LWPOINT* pt, double dist, int* numelems, int fields, int limit);

LWT_ISO_NODE* lwt_be_getNodeById(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids, int* numelems, int fields);

int lwt_be_ExistsCoincidentNode(LWT_TOPOLOGY* topo, LWPOINT* pt);
int lwt_be_insertNodes(LWT_TOPOLOGY* topo, LWT_ISO_NODE* node, int numelems);

int lwt_be_ExistsEdgeIntersectingPoint(LWT_TOPOLOGY* topo, LWPOINT* pt);

LWT_ELEMID lwt_be_getNextEdgeId(LWT_TOPOLOGY* topo);
LWT_ISO_EDGE* lwt_be_getEdgeById(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                               int* numelems, int fields);
LWT_ISO_EDGE* lwt_be_getEdgeWithinDistance2D(LWT_TOPOLOGY* topo, LWPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit);
int
lwt_be_insertEdges(LWT_TOPOLOGY* topo, LWT_ISO_EDGE* edge, int numelems);
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
  int srid;
  double precision;
  int hasZ;
};

#endif /* LIBLWGEOM_TOPO_INTERNAL_H */
