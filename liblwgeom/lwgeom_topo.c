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


#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"

#include <stdio.h>
#include <errno.h>

/*********************************************************************
 *
 * Backend iface
 *
 ********************************************************************/

LWT_BE_IFACE* lwt_CreateBackendIface(const LWT_BE_DATA *data)
{
  LWT_BE_IFACE *iface = lwalloc(sizeof(LWT_BE_IFACE));
  iface->data = data;
  iface->cb = NULL;
  return iface;
}

void lwt_BackendIfaceRegisterCallbacks(LWT_BE_IFACE *iface,
                                       const LWT_BE_CALLBACKS* cb)
{
  iface->cb = cb;
}

void lwt_FreeBackendIface(LWT_BE_IFACE* iface)
{
  lwfree(iface);
}

/*********************************************************************
 *
 * Backend wrappers
 *
 ********************************************************************/

#define CHECKCB(be, method) do { \
  if ( ! (be)->cb || ! (be)->cb->method ) \
  lwerror("Callback " # method " not registered by backend"); \
} while (0)

#define CB0(be, method) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data)

#define CB1(be, method, a1) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data, a1)

#define CBT0(to, method) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo)

#define CBT1(to, method, a1) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1)

#define CBT2(to, method, a1, a2) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2)

#define CBT3(to, method, a1, a2, a3) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3)

#define CBT4(to, method, a1, a2, a3, a4) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4)

#define CBT5(to, method, a1, a2, a3, a4, a5) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5)

#define CBT6(to, method, a1, a2, a3, a4, a5, a6) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5, a6)

const char *
lwt_be_lastErrorMessage(const LWT_BE_IFACE* be)
{
  CB0(be, lastErrorMessage);
}

LWT_BE_TOPOLOGY *
lwt_be_loadTopologyByName(LWT_BE_IFACE *be, const char *name)
{
  CB1(be, loadTopologyByName, name);
}

int
lwt_be_freeTopology(LWT_TOPOLOGY *topo)
{
  CBT0(topo, freeTopology);
}

LWT_ISO_NODE*
lwt_be_getNodeWithinDistance2D(LWT_TOPOLOGY* topo, LWPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getNodeWithinDistance2D, pt, dist, numelems, fields, limit);
}

int
lwt_be_insertNodes(LWT_TOPOLOGY* topo, LWT_ISO_NODE* node, int numelems)
{
  CBT2(topo, insertNodes, node, numelems);
}

LWT_ELEMID
lwt_be_getNextEdgeId(LWT_TOPOLOGY* topo)
{
  CBT0(topo, getNextEdgeId);
}

LWT_ISO_EDGE*
lwt_be_getEdgeById(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeById, ids, numelems, fields);
}

LWT_ISO_EDGE*
lwt_be_getEdgeWithinDistance2D(LWT_TOPOLOGY* topo, LWPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getEdgeWithinDistance2D, pt, dist, numelems, fields, limit);
}

int
lwt_be_insertEdges(LWT_TOPOLOGY* topo, LWT_ISO_EDGE* edge, int numelems)
{
  CBT2(topo, insertEdges, edge, numelems);
}

int
lwt_be_updateEdges(LWT_TOPOLOGY* topo,
  const LWT_ISO_EDGE* sel_edge, int sel_fields,
  const LWT_ISO_EDGE* upd_edge, int upd_fields,
  const LWT_ISO_EDGE* exc_edge, int exc_fields
)
{
  CBT6(topo, updateEdges, sel_edge, sel_fields,
                          upd_edge, upd_fields,
                          exc_edge, exc_fields);
}

int
lwt_be_deleteEdges(LWT_TOPOLOGY* topo,
  const LWT_ISO_EDGE* sel_edge, int sel_fields
)
{
  CBT2(topo, deleteEdges, sel_edge, sel_fields);
}

LWT_ELEMID
lwt_be_getFaceContainingPoint(LWT_TOPOLOGY* topo, LWPOINT* pt)
{
  CBT1(topo, getFaceContainingPoint, pt);
}


int
lwt_be_updateTopoGeomEdgeSplit(LWT_TOPOLOGY* topo, LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2)
{
  CBT3(topo, updateTopoGeomEdgeSplit, split_edge, new_edge1, new_edge2);
}

/* wrappers of backend wrappers... */

int
lwt_be_ExistsCoincidentNode(LWT_TOPOLOGY* topo, LWPOINT* pt)
{
  int exists = 0;
  lwt_be_getNodeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

int
lwt_be_ExistsEdgeIntersectingPoint(LWT_TOPOLOGY* topo, LWPOINT* pt)
{
  int exists = 0;
  lwt_be_getEdgeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

/************************************************************************
 *
 * API implementation
 *
 ************************************************************************/

LWT_TOPOLOGY *
lwt_LoadTopology( LWT_BE_IFACE *iface, const char *name )
{
  LWT_BE_TOPOLOGY* be_topo;
  LWT_TOPOLOGY* topo;

  be_topo = lwt_be_loadTopologyByName(iface, name);
  if ( ! be_topo ) {
    //lwerror("Could not load topology from backend: %s",
    lwerror("%s", lwt_be_lastErrorMessage(iface));
    return NULL;
  }
  topo = lwalloc(sizeof(LWT_TOPOLOGY));
  topo->be_iface = iface;
  topo->be_topo = be_topo;
  topo->name = NULL; /* don't want to think about it now.. */
  topo->table_prefix = NULL; /* don't want to think about it now */

  return topo;
}

void
lwt_FreeTopology( LWT_TOPOLOGY* topo )
{
  if ( ! lwt_be_freeTopology(topo) ) {
    lwnotice("Could not release backend topology memory: %s",
            lwt_be_lastErrorMessage(topo->be_iface));
  }
  lwfree(topo);
}


LWT_ELEMID
lwt_AddIsoNode( LWT_TOPOLOGY* topo, LWT_ELEMID face,
                LWPOINT* pt, int skipISOChecks )
{
  LWT_ELEMID foundInFace = -1;

  if ( ! skipISOChecks )
  {
    if ( lwt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      lwerror("SQL/MM Spatial exception - coincident node");
      return -1;
    }
    if ( lwt_be_ExistsEdgeIntersectingPoint(topo, pt) ) /*x*/
    {
      lwerror("SQL/MM Spatial exception - edge crosses node.");
      return -1;
    }
  }

  if ( face == -1 || ! skipISOChecks )
  {
    foundInFace = lwt_be_getFaceContainingPoint(topo, pt); /*x*/
    if ( foundInFace == -2 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( foundInFace == -1 ) foundInFace = 0;
  }

  if ( face == -1 ) {
    face = foundInFace;
  }
  else if ( ! skipISOChecks && foundInFace != face ) {
#if 0
    lwerror("SQL/MM Spatial exception - within face %d (not %d)",
            foundInFace, face);
#else
    lwerror("SQL/MM Spatial exception - not within face");
#endif
    return -1;
  }

  LWT_ISO_NODE node;
  node.node_id = -1;
  node.containing_face = face;
  node.geom = pt;
  if ( ! lwt_be_insertNodes(topo, &node, 1) )
  {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return node.node_id;
}

static LWCOLLECTION *
_lwt_EdgeSplit( LWT_TOPOLOGY* topo, LWT_ELEMID edge, LWPOINT* pt, int skipISOChecks, LWT_ISO_EDGE** oldedge )
{
  LWGEOM *split;
  LWCOLLECTION *split_col;
  int i;

  /* Get edge */
  i = 1;
  LWDEBUG(1, "lwt_NewEdgesSplit: calling lwt_be_getEdgeById");
  *oldedge = lwt_be_getEdgeById(topo, &edge, &i, LWT_COL_EDGE_ALL);
  LWDEBUGF(1, "lwt_NewEdgesSplit: lwt_be_getEdgeById returned %p", *oldedge);
  if ( ! *oldedge )
  {
    LWDEBUGF(1, "lwt_NewEdgesSplit: "
                "lwt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    else if ( i == 0 )
    {
      lwerror("SQL/MM Spatial exception - non-existent edge");
      return NULL;
    }
    else
    {
      lwerror("Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
    }
  }


  /*
   *  - check if a coincident node already exists
   */
  if ( ! skipISOChecks )
  {
    LWDEBUG(1, "lwt_NewEdgesSplit: calling lwt_be_ExistsCoincidentNode");
    if ( lwt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      LWDEBUG(1, "lwt_NewEdgesSplit: lwt_be_ExistsCoincidentNode returned");
      lwerror("SQL/MM Spatial exception - coincident node");
      return NULL;
    }
    LWDEBUG(1, "lwt_NewEdgesSplit: lwt_be_ExistsCoincidentNode returned");
  }

  /* Split edge */
  split = lwgeom_split((LWGEOM*)(*oldedge)->geom, (LWGEOM*)pt);
  if ( ! split )
  {
    lwerror("could not split edge by point ?");
    return NULL;
  }
  split_col = lwgeom_as_lwcollection(split);
  if ( ! split_col ) {
    lwgeom_release(split);
    lwerror("lwgeom_as_lwcollection returned NULL");
    return NULL;
  }
  if (split_col->ngeoms < 2) {
    lwgeom_release(split);
    lwerror("SQL/MM Spatial exception - point not on edge");
    return NULL;
  }
  return split_col;
}

LWT_ELEMID
lwt_ModEdgeSplit( LWT_TOPOLOGY* topo, LWT_ELEMID edge,
                  LWPOINT* pt, int skipISOChecks )
{
  LWT_ISO_NODE node;
  LWT_ISO_EDGE* oldedge = NULL;
  LWCOLLECTION *split_col;
  const LWGEOM *oldedge_geom;
  const LWGEOM *newedge_geom;
  LWT_ISO_EDGE newedge1;
  LWT_ISO_EDGE seledge, updedge, excedge;
  int ret;

  split_col = _lwt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! lwt_be_insertNodes(topo, &node, 1) )
  {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    /* should have been set by backend */
    lwerror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Insert the new edge */
  newedge1.edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedge1.edge_id == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedge1.start_node = node.node_id;
  newedge1.end_node = oldedge->end_node;
  newedge1.face_left = oldedge->face_left;
  newedge1.face_right = oldedge->face_right;
  newedge1.next_left = oldedge->next_left == -oldedge->edge_id ?
      -newedge1.edge_id : oldedge->next_left;
  newedge1.next_right = -oldedge->edge_id;
  newedge1.geom = lwgeom_as_lwline(newedge_geom);
  /* lwgeom_split of a line should only return lines ... */
  if ( ! newedge1.geom ) {
    lwcollection_release(split_col);
    lwerror("first geometry in lwgeom_split output is not a line");
    return -1;
  }
  ret = lwt_be_insertEdges(topo, &newedge1, 1);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    lwcollection_release(split_col);
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update the old edge */
  updedge.geom = lwgeom_as_lwline(oldedge_geom);
  /* lwgeom_split of a line should only return lines ... */
  if ( ! updedge.geom ) {
    lwcollection_release(split_col);
    lwerror("second geometry in lwgeom_split output is not a line");
    return -1;
  }
  updedge.next_left = newedge1.edge_id;
  updedge.end_node = node.node_id;
  ret = lwt_be_updateEdges(topo,
      oldedge, LWT_COL_EDGE_EDGE_ID,
      &updedge, LWT_COL_EDGE_GEOM|LWT_COL_EDGE_NEXT_LEFT|LWT_COL_EDGE_END_NODE,
      NULL, 0);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    lwerror("Edge being splitted (%d) disappeared during operations?", oldedge->edge_id);
    return -1;
  } else if ( ret > 1 ) {
    lwerror("More than a single edge found with id %d !", oldedge->edge_id);
    return -1;
  }

  /* Update all next edge references to match new layout (ST_ModEdgeSplit) */

  updedge.next_right = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_right = -oldedge->edge_id;
  seledge.start_node = oldedge->end_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_RIGHT|LWT_COL_EDGE_START_NODE,
      &updedge, LWT_COL_EDGE_NEXT_RIGHT,
      &excedge, LWT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_left = -oldedge->edge_id;
  seledge.end_node = oldedge->end_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_LEFT|LWT_COL_EDGE_END_NODE,
      &updedge, LWT_COL_EDGE_NEXT_LEFT,
      &excedge, LWT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = lwt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedge1.edge_id, -1);
  if ( ! ret ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  lwcollection_release(split_col);

  /* return new node id */
  return node.node_id;
}

LWT_ELEMID
lwt_NewEdgesSplit( LWT_TOPOLOGY* topo, LWT_ELEMID edge,
                   LWPOINT* pt, int skipISOChecks )
{
  LWT_ISO_NODE node;
  LWT_ISO_EDGE* oldedge = NULL;
  LWCOLLECTION *split_col;
  const LWGEOM *oldedge_geom;
  const LWGEOM *newedge_geom;
  LWT_ISO_EDGE newedges[2];
  //LWT_ISO_EDGE newedge1, newedge2;
  LWT_ISO_EDGE seledge, updedge;
  int ret;

  split_col = _lwt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! lwt_be_insertNodes(topo, &node, 1) )
  {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    /* should have been set by backend */
    lwerror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Delete the old edge */
  seledge.edge_id = edge;
  ret = lwt_be_deleteEdges(topo, &seledge, LWT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Get new edges identifiers */
  newedges[0].edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedges[0].edge_id == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedges[1].edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedges[1].edge_id == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Define the first new edge (to new node) */
  newedges[0].start_node = oldedge->start_node;
  newedges[0].end_node = node.node_id;
  newedges[0].face_left = oldedge->face_left;
  newedges[0].face_right = oldedge->face_right;
  newedges[0].next_left = newedges[1].edge_id;
  if ( oldedge->next_right == edge )
    newedges[0].next_right = newedges[0].edge_id;
  else if ( oldedge->next_right == -edge )
    newedges[0].next_right = -newedges[1].edge_id;
  else
    newedges[0].next_right = oldedge->next_right;
  newedges[0].geom = lwgeom_as_lwline(oldedge_geom);
  /* lwgeom_split of a line should only return lines ... */
  if ( ! newedges[0].geom ) {
    lwcollection_release(split_col);
    lwerror("first geometry in lwgeom_split output is not a line");
    return -1;
  }

  /* Define the second new edge (from new node) */
  newedges[1].start_node = node.node_id;
  newedges[1].end_node = oldedge->end_node;
  newedges[1].face_left = oldedge->face_left;
  newedges[1].face_right = oldedge->face_right;
  newedges[1].next_right = -newedges[0].edge_id;
  if ( oldedge->next_left == -edge )
    newedges[1].next_left = -newedges[1].edge_id;
  else if ( oldedge->next_left == edge )
    newedges[1].next_left = newedges[0].edge_id;
  else
    newedges[1].next_left = oldedge->next_left;
  newedges[1].geom = lwgeom_as_lwline(newedge_geom);
  /* lwgeom_split of a line should only return lines ... */
  if ( ! newedges[1].geom ) {
    lwcollection_release(split_col);
    lwerror("second geometry in lwgeom_split output is not a line");
    return -1;
  }

  /* Insert both new edges */
  ret = lwt_be_insertEdges(topo, newedges, 2);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    lwcollection_release(split_col);
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update all next edge references pointing to old edge id */

  updedge.next_right = newedges[1].edge_id;
  seledge.next_right = edge;
  seledge.start_node = oldedge->start_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_RIGHT|LWT_COL_EDGE_START_NODE,
      &updedge, LWT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_right = -newedges[0].edge_id;
  seledge.next_right = -edge;
  seledge.start_node = oldedge->end_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_RIGHT|LWT_COL_EDGE_START_NODE,
      &updedge, LWT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = newedges[0].edge_id;
  seledge.next_left = edge;
  seledge.end_node = oldedge->start_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_LEFT|LWT_COL_EDGE_END_NODE,
      &updedge, LWT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedges[1].edge_id;
  seledge.next_left = -edge;
  seledge.end_node = oldedge->end_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_LEFT|LWT_COL_EDGE_END_NODE,
      &updedge, LWT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition -- TODO */
  ret = lwt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedges[0].edge_id, newedges[1].edge_id);
  if ( ! ret ) {
    lwcollection_release(split_col);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  lwcollection_release(split_col);

  /* return new node id */
  return node.node_id;
}
