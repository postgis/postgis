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
 **********************************************************************
 *
 * Topology extension for liblwgeom.
 * Initially funded by Tuscany Region (Italy) - SITA (CIG: 60351023B8)
 *
 **********************************************************************/


#include "../postgis_config.h"

/*#define POSTGIS_DEBUG_LEVEL 1*/
#include "lwgeom_log.h"

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"
#include "lwgeom_geos.h"

#include <stdio.h>
#include <inttypes.h> /* for PRId64 */
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

static int
lwt_be_topoGetSRID(LWT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetSRID);
}

static double
lwt_be_topoGetPrecision(LWT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetPrecision);
}

static int
lwt_be_topoHasZ(LWT_TOPOLOGY *topo)
{
  CBT0(topo, topoHasZ);
}

int
lwt_be_freeTopology(LWT_TOPOLOGY *topo)
{
  CBT0(topo, freeTopology);
}

LWT_ISO_NODE*
lwt_be_getNodeById(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getNodeById, ids, numelems, fields);
}

LWT_ISO_NODE*
lwt_be_getNodeWithinDistance2D(LWT_TOPOLOGY* topo, LWPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getNodeWithinDistance2D, pt, dist, numelems, fields, limit);
}

LWT_ISO_NODE*
lwt_be_getNodeWithinBox2D( const LWT_TOPOLOGY* topo,
                           const GBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getNodeWithinBox2D, box, numelems, fields, limit);
}

LWT_ISO_EDGE*
lwt_be_getEdgeWithinBox2D( const LWT_TOPOLOGY* topo,
                           const GBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getEdgeWithinBox2D, box, numelems, fields, limit);
}

int
lwt_be_insertNodes(LWT_TOPOLOGY* topo, LWT_ISO_NODE* node, int numelems)
{
  CBT2(topo, insertNodes, node, numelems);
}

static int
lwt_be_insertFaces(LWT_TOPOLOGY* topo, LWT_ISO_FACE* face, int numelems)
{
  CBT2(topo, insertFaces, face, numelems);
}

static int
lwt_be_deleteFacesById(const LWT_TOPOLOGY* topo, const LWT_ELEMID* ids, int numelems)
{
  CBT2(topo, deleteFacesById, ids, numelems);
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

static LWT_ISO_FACE*
lwt_be_getFaceById(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getFaceById, ids, numelems, fields);
}

static LWT_ISO_EDGE*
lwt_be_getEdgeByNode(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeByNode, ids, numelems, fields);
}

static LWT_ISO_EDGE*
lwt_be_getEdgeByFace(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeByFace, ids, numelems, fields);
}

static LWT_ISO_NODE*
lwt_be_getNodeByFace(LWT_TOPOLOGY* topo, const LWT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getNodeByFace, ids, numelems, fields);
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

static int
lwt_be_updateNodes(LWT_TOPOLOGY* topo,
  const LWT_ISO_NODE* sel_node, int sel_fields,
  const LWT_ISO_NODE* upd_node, int upd_fields,
  const LWT_ISO_NODE* exc_node, int exc_fields
)
{
  CBT6(topo, updateNodes, sel_node, sel_fields,
                          upd_node, upd_fields,
                          exc_node, exc_fields);
}

static int
lwt_be_updateFacesById(LWT_TOPOLOGY* topo,
  const LWT_ISO_FACE* faces, int numfaces
)
{
  CBT2(topo, updateFacesById, faces, numfaces);
}

static int
lwt_be_updateEdgesById(LWT_TOPOLOGY* topo,
  const LWT_ISO_EDGE* edges, int numedges, int upd_fields
)
{
  CBT3(topo, updateEdgesById, edges, numedges, upd_fields);
}

static int
lwt_be_updateNodesById(LWT_TOPOLOGY* topo,
  const LWT_ISO_NODE* nodes, int numnodes, int upd_fields
)
{
  CBT3(topo, updateNodesById, nodes, numnodes, upd_fields);
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

static int
lwt_be_updateTopoGeomFaceSplit(LWT_TOPOLOGY* topo, LWT_ELEMID split_face, LWT_ELEMID new_face1, LWT_ELEMID new_face2)
{
  CBT3(topo, updateTopoGeomFaceSplit, split_face, new_face1, new_face2);
}

static LWT_ELEMID*
lwt_be_getRingEdges( LWT_TOPOLOGY* topo,
                     LWT_ELEMID edge, int *numedges, int limit )
{
  CBT3(topo, getRingEdges, edge, numedges, limit);
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
 * Utility functions
 *
 ************************************************************************/

static void
_lwt_release_edges(LWT_ISO_EDGE *edges, int num_edges)
{
  int i;
  for ( i=0; i<num_edges; ++i ) {
    lwline_release(edges[i].geom);
  }
  lwfree(edges);
}

static void
_lwt_release_nodes(LWT_ISO_NODE *nodes, int num_nodes)
{
  int i;
  for ( i=0; i<num_nodes; ++i ) {
    lwpoint_release(nodes[i].geom);
  }
  lwfree(nodes);
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
  topo->srid = lwt_be_topoGetSRID(topo);
  topo->hasZ = lwt_be_topoHasZ(topo);
  topo->precision = lwt_be_topoGetPrecision(topo);

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

/* Check that an edge does not cross an existing node or edge
 *
 * Return -1 on cross or error, 0 if everything is fine.
 * Note that before returning -1, lwerror is invoked...
 */
static int
_lwt_CheckEdgeCrossing( LWT_TOPOLOGY* topo,
                        LWT_ELEMID start_node, LWT_ELEMID end_node,
                        const LWLINE *geom )
{
  int i, num_nodes, num_edges;
  LWT_ISO_EDGE *edges;
  LWT_ISO_NODE *nodes;
  const GBOX *edgebox;
  GEOSGeometry *edgegg;
  const GEOSPreparedGeometry* prepared_edge;

  initGEOS(lwnotice, lwgeom_geos_error);

  edgegg = LWGEOM2GEOS( lwline_as_lwgeom(geom), 0);
  if ( ! edgegg ) {
    lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
    return -1;
  }
  prepared_edge = GEOSPrepare( edgegg );
  if ( ! prepared_edge ) {
    lwerror("Could not prepare edge geometry: %s", lwgeom_geos_errmsg);
    return -1;
  }
  edgebox = lwgeom_get_bbox( lwline_as_lwgeom(geom) );

  /* loop over each node within the edge's gbox */
  nodes = lwt_be_getNodeWithinBox2D( topo, edgebox, &num_nodes,
                                            LWT_COL_NODE_ALL, 0 );
  lwnotice("lwt_be_getNodeWithinBox2D returned %d nodes", num_nodes);
  if ( num_nodes == -1 ) {
    GEOSPreparedGeom_destroy(prepared_edge);
    GEOSGeom_destroy(edgegg);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    LWT_ISO_NODE* node = &(nodes[i]);
    GEOSGeometry *nodegg;
    int contains;
    if ( node->node_id == start_node ) continue;
    if ( node->node_id == end_node ) continue;
    /* check if the edge contains this node (not on boundary) */
    nodegg = LWGEOM2GEOS( lwpoint_as_lwgeom(node->geom) , 0);
    /* ST_RelateMatch(rec.relate, 'T********') */
    contains = GEOSPreparedContains( prepared_edge, nodegg );
    GEOSGeom_destroy(nodegg);
    if (contains == 2)
    {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      lwfree(nodes);
      lwerror("GEOS exception on PreparedContains: %s", lwgeom_geos_errmsg);
      return -1;
    }
    if ( contains )
    {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      lwfree(nodes);
      lwerror("SQL/MM Spatial exception - geometry crosses a node");
      return -1;
    }
  }
  if ( nodes ) lwfree(nodes); /* may be NULL if num_nodes == 0 */

  /* loop over each edge within the edge's gbox */
  edges = lwt_be_getEdgeWithinBox2D( topo, edgebox, &num_edges, LWT_COL_EDGE_ALL, 0 );
  LWDEBUGF(1, "lwt_be_getEdgeWithinBox2D returned %d edges", num_edges);
  if ( num_edges == -1 ) {
    GEOSPreparedGeom_destroy(prepared_edge);
    GEOSGeom_destroy(edgegg);
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_edges; ++i )
  {
    LWT_ISO_EDGE* edge = &(edges[i]);
    GEOSGeometry *eegg;
    char *relate;
    int match;

    if ( ! edge->geom ) {
      lwerror("Edge %d has NULL geometry!", edge->edge_id);
      return -1;
    }

    eegg = LWGEOM2GEOS( lwline_as_lwgeom(edge->geom), 0 );
    if ( ! eegg ) {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
      return -1;
    }

    LWDEBUGF(2, "Edge %d converted to GEOS", edge->edge_id);

    /* check if the edge crosses our edge (not boundary-boundary) */

    relate = GEOSRelateBoundaryNodeRule(eegg, edgegg, 2);
    if ( ! relate ) {
      GEOSGeom_destroy(eegg);
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      lwerror("GEOSRelateBoundaryNodeRule error: %s", lwgeom_geos_errmsg);
      return -1;
    }

    LWDEBUGF(2, "Edge %d relate pattern is %s", edge->edge_id, relate);

    match = GEOSRelatePatternMatch(relate, "F********");
    if ( match ) {
      if ( match == 2 ) {
        GEOSPreparedGeom_destroy(prepared_edge);
        GEOSGeom_destroy(edgegg);
        GEOSGeom_destroy(eegg);
        GEOSFree(relate);
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
        return -1;
      }
      else continue; /* no interior intersection */
    }

    match = GEOSRelatePatternMatch(relate, "1FFF*FFF2");
    if ( match ) {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("SQL/MM Spatial exception - coincident edge %" PRId64,
                edge->edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "1********");
    if ( match ) {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("Spatial exception - geometry intersects edge %" PRId64,
                edge->edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "T********");
    if ( match ) {
      GEOSPreparedGeom_destroy(prepared_edge);
      GEOSGeom_destroy(edgegg);
      GEOSGeom_destroy(eegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("SQL/MM Spatial exception - geometry crosses edge %"
                PRId64, edge->edge_id);
      }
      return -1;
    }

    LWDEBUGF(2, "Edge %d analisys completed, it does no harm", edge->edge_id);

    GEOSFree(relate);
    GEOSGeom_destroy(eegg);
  }
  if ( edges ) lwfree(edges); /* would be NULL if num_edges was 0 */

  GEOSPreparedGeom_destroy(prepared_edge);
  GEOSGeom_destroy(edgegg);

  return 0;
}


LWT_ELEMID
lwt_AddIsoEdge( LWT_TOPOLOGY* topo, LWT_ELEMID startNode,
                LWT_ELEMID endNode, const LWLINE* geom )
{
  int num_nodes;
  int i;
  LWT_ISO_EDGE newedge;
  LWT_ISO_NODE *endpoints;
  LWT_ELEMID containing_face = -1;
  LWT_ELEMID *node_ids;
  int skipISOChecks = 0;
  POINT2D p1, p2;

  /* NOT IN THE SPECS:
   * A closed edge is never isolated (as it forms a face)
   */
  if ( startNode == endNode )
  {
    lwerror("Closed edges would not be isolated, try lwt_AddEdgeNewFaces");
    return -1;
  }

  if ( ! skipISOChecks )
  {
    /* Acurve must be simple */
    if ( ! lwgeom_is_simple(lwline_as_lwgeom(geom)) )
    {
      lwerror("SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  /*
   * Check for:
   *    existence of nodes
   *    nodes faces match
   * Extract:
   *    nodes face id
   *    nodes geoms
   */
  num_nodes = 2;
  node_ids = lwalloc(sizeof(LWT_ELEMID) * num_nodes);
  node_ids[0] = startNode;
  node_ids[1] = endNode;
  endpoints = lwt_be_getNodeById( topo, node_ids, &num_nodes,
                                             LWT_COL_NODE_ALL );
  if ( ! endpoints ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num_nodes < 2 )
  {
    _lwt_release_nodes(endpoints, num_nodes);
    lwerror("SQL/MM Spatial exception - non-existent node");
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    const LWT_ISO_NODE *n = &(endpoints[i]);
    if ( n->containing_face == -1 )
    {
      _lwt_release_nodes(endpoints, num_nodes);
      lwerror("SQL/MM Spatial exception - not isolated node");
      return -1;
    }
    if ( containing_face == -1 ) containing_face = n->containing_face;
    else if ( containing_face != n->containing_face )
    {
      _lwt_release_nodes(endpoints, num_nodes);
      lwerror("SQL/MM Spatial exception - nodes in different faces");
      return -1;
    }

    if ( ! skipISOChecks )
    {
      if ( n->node_id == startNode )
      {
        /* l) Check that start point of acurve match start node geoms. */
        getPoint2d_p(geom->points, 0, &p1);
        getPoint2d_p(n->geom->point, 0, &p2);
        if ( ! p2d_same(&p1, &p2) )
        {
          _lwt_release_nodes(endpoints, num_nodes);
          lwerror("SQL/MM Spatial exception - "
                  "start node not geometry start point.");
          return -1;
        }
      }
      else
      {
        /* m) Check that end point of acurve match end node geoms. */
        getPoint2d_p(geom->points, geom->points->npoints-1, &p1);
        getPoint2d_p(n->geom->point, 0, &p2);
        if ( ! p2d_same(&p1, &p2) )
        {
          _lwt_release_nodes(endpoints, num_nodes);
          lwerror("SQL/MM Spatial exception - "
                  "end node not geometry end point.");
          return -1;
        }
      }
    }
  }

  if ( ! skipISOChecks )
  {
    if ( _lwt_CheckEdgeCrossing( topo, startNode, endNode, geom ) )
      return -1;
  }

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = lwt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* TODO: this should likely be an exception instead ! */
  if ( containing_face == -1 ) containing_face = 0;

  newedge.start_node = startNode;
  newedge.end_node = endNode;
  newedge.face_left = newedge.face_right = containing_face;
  newedge.next_left = -newedge.edge_id;
  newedge.next_right = newedge.edge_id;
  newedge.geom = (LWLINE *)geom; /* const cast.. */

  int ret = lwt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /*
   * Update Node containing_face values
   *
   * the nodes anode and anothernode are no more isolated
   * because now there is an edge connecting them
   */ 
  LWT_ISO_NODE *updated_nodes = lwalloc(sizeof(LWT_ISO_NODE) * 2);
  updated_nodes[0].node_id = startNode;
  updated_nodes[0].containing_face = -1;
  updated_nodes[1].node_id = endNode;
  updated_nodes[1].containing_face = -1;
  ret = lwt_be_updateNodesById(topo, updated_nodes, 2,
                               LWT_COL_NODE_CONTAINING_FACE);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return newedge.edge_id;
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

  /* Update TopoGeometries composition */
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

/* Data structure used by AddEdgeX functions */
typedef struct edgeend_t {
  /* Signed identifier of next clockwise edge (+outgoing,-incoming) */
  LWT_ELEMID nextCW;
  /* Identifier of face between myaz and next CW edge */
  LWT_ELEMID cwFace;
  /* Signed identifier of next counterclockwise edge (+outgoing,-incoming) */
  LWT_ELEMID nextCCW;
  /* Identifier of face between myaz and next CCW edge */
  LWT_ELEMID ccwFace;
  int was_isolated;
  double myaz; /* azimuth of edgeend geometry */
} edgeend;

/* 
 * Find the first edge encountered going counterclockwise
 * around a node, starting from the given azimuth, and take
 * note of the face on the given azimut's side.
 *
 * @param topo the topology to act upon
 * @param node the identifier of the node to analyze
 * @param data input (myaz) / output (nextCW, nextCCW) parameter
 * @param other edgeend, if also incident to given node (closed edge).
 * @return number of incident edges found
 *
 */
static int
_lwt_FindAdjacentEdges( LWT_TOPOLOGY* topo, LWT_ELEMID node, edgeend *data,
                        edgeend *other )
{
  LWT_ISO_EDGE *edges;
  int numedges = 1;
  int i;
  double minaz, maxaz;
  double az, azdif;

  data->nextCW = data->nextCCW = 0;
  data->cwFace = data->ccwFace = -1;

  if ( other ) {
    azdif = other->myaz - data->myaz;
    if ( azdif < 0 ) azdif += 2 * M_PI;
    minaz = maxaz = azdif;
    /* TODO: set nextCW/nextCCW/cwFace/ccwFace to other->something ? */
    LWDEBUGF(1, "Other edge end has cwFace=%d and ccwFace=%d",
                other->cwFace, other->ccwFace);
  } else {
    minaz = maxaz = -1;
  }

  LWDEBUGF(1, "Looking for edges incident to node %" PRId64
              " and adjacent to azimuth %g", node, data->myaz);

  /* Get incident edges */
  edges = lwt_be_getEdgeByNode( topo, &node, &numedges, LWT_COL_EDGE_ALL );
  if ( numedges == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }

  LWDEBUGF(1, "getEdgeByNode returned %d edges, minaz=%g, maxaz=%g",
              numedges, minaz, maxaz);

  /* For each incident edge-end (1 or 2): */
  for ( i = 0; i < numedges; ++i )
  {
    LWT_ISO_EDGE *edge = &(edges[i]);
    LWGEOM *g = lwline_as_lwgeom(edge->geom);
    /* NOTE: remove_repeated_points call could be replaced by
     * some other mean to pick two distinct points for endpoints */
    LWGEOM *cleangeom = lwgeom_remove_repeated_points( g, 0 );
    POINT2D p1, p2;
    POINTARRAY *pa = lwgeom_as_lwline(cleangeom)->points;

    if ( pa->npoints < 2 ) {
      lwgeom_free(cleangeom);
      lwerror("corrupted topology: edge %" PRId64
              " does not have two distinct points", edge->edge_id);
      return -1;
    }

    if ( edge->start_node == node ) {
      getPoint2d_p(pa, 0, &p1);
      getPoint2d_p(pa, 1, &p2);
      LWDEBUGF(1, "edge %" PRId64
                  " starts on node %" PRId64
                  ", edgeend is %g,%g-%g,%g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {
        lwgeom_free(cleangeom);
        lwerror("error computing azimuth of edge %d first edgeend [%g,%g-%g,%g]",
                edge->edge_id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }
      azdif = az - data->myaz;
      LWDEBUGF(1, "azimuth of edge %" PRId64
                  ": %g (diff: %g)", edge->edge_id, az, azdif);

      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = edge->edge_id; /* outgoing */
        data->cwFace = edge->face_left;
        data->ccwFace = edge->face_right;
        LWDEBUGF(1, "new nextCW and nextCCW edge is %" PRId64
                    ", outgoing, "
                    "with face_left %" PRId64 " and face_right %" PRId64
                    " (face_right is new ccwFace, face_left is new cwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = edge->edge_id; /* outgoing */
          data->cwFace = edge->face_left;
          LWDEBUGF(1, "new nextCW edge is %" PRId64
                      ", outgoing, "
                      "with face_left %" PRId64 " and face_right %" PRId64
                      " (previous had minaz=%g, face_left is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = edge->edge_id; /* outgoing */
          data->ccwFace = edge->face_right;
          LWDEBUGF(1, "new nextCCW edge is %" PRId64
                      ", outgoing, "
                      "with face_left %" PRId64 " and face_right %" PRId64
                      " (previous had maxaz=%g, face_right is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    if ( edge->end_node == node ) {
      getPoint2d_p(pa, pa->npoints-1, &p1);
      getPoint2d_p(pa, pa->npoints-2, &p2);
      LWDEBUGF(1, "edge %" PRId64 " ends on node %" PRId64
                  ", edgeend is %g,%g-%g,%g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {
        lwgeom_free(cleangeom);
        lwerror("error computing azimuth of edge %d last edgeend [%g,%g-%g,%g]",
                edge->edge_id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }
      azdif = az - data->myaz;
      LWDEBUGF(1, "azimuth of edge %" PRId64
                  ": %g (diff: %g)", edge->edge_id, az, azdif);
      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = -edge->edge_id; /* incoming */
        data->cwFace = edge->face_right;
        data->ccwFace = edge->face_left;
        LWDEBUGF(1, "new nextCW and nextCCW edge is %" PRId64
                    ", incoming, "
                    "with face_left %" PRId64 " and face_right %" PRId64
                    " (face_right is new cwFace, face_left is new ccwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = -edge->edge_id; /* incoming */
          data->cwFace = edge->face_right;
          LWDEBUGF(1, "new nextCW edge is %" PRId64
                      ", incoming, "
                      "with face_left %" PRId64 " and face_right %" PRId64
                      " (previous had minaz=%g, face_right is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = -edge->edge_id; /* incoming */
          data->ccwFace = edge->face_left;
          LWDEBUGF(1, "new nextCCW edge is %" PRId64
                      ", outgoing, from start point, "
                      "with face_left %" PRId64 " and face_right %" PRId64
                      " (previous had maxaz=%g, face_left is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    lwgeom_free(cleangeom);
    lwline_free(edge->geom);
  }
  if ( edges ) lwfree(edges); /* there might be none */

  LWDEBUGF(1, "edges adjacent to azimuth %g"
              " (incident to node %" PRId64 ")"
              ": CW:%" PRId64 "(%g) CCW:%" PRId64 "(%g)",
              data->myaz, node, data->nextCW, minaz,
              data->nextCCW, maxaz);

  if ( numedges && data->cwFace != data->ccwFace )
  {
    if ( data->cwFace != -1 && data->ccwFace != -1 ) {
      lwerror("Corrupted topology: adjacent edges %" PRId64 " and %" PRId64
              " bind different face (%" PRId64 " and %" PRId64 ")",
              numedges, data->nextCW, data->nextCCW,
              data->cwFace, data->ccwFace);
      return -1;
    }
  }

  /* Return number of incident edges found */
  return numedges;
}

/*
 * Get a point internal to the line and write it into the "ip"
 * parameter
 *
 * return 0 on failure (line is empty or collapsed), 1 otherwise
 */
static int
_lwt_GetInteriorEdgePoint(const LWLINE* edge, POINT2D* ip)
{
  int i;
  POINT2D fp, lp, tp;
  POINTARRAY *pa = edge->points;

  if ( pa->npoints < 2 ) return 0; /* empty or structurally collapsed */

  getPoint2d_p(pa, 0, &fp); /* save first point */
  getPoint2d_p(pa, pa->npoints-1, &lp); /* save last point */
  for (i=1; i<pa->npoints-1; ++i)
  {
    getPoint2d_p(pa, i, &tp); /* pick next point */
    if ( p2d_same(&tp, &fp) ) continue; /* equal to startpoint */
    if ( p2d_same(&tp, &lp) ) continue; /* equal to endpoint */
    /* this is a good one, neither same of start nor of end point */
    *ip = tp;
    return 1; /* found */
  }

  /* no distinct vertex found */

  /* interpolate if start point != end point */

  if ( p2d_same(&fp, &lp) ) return 0; /* no distinct points in edge */
 
  ip->x = fp.x + ( (lp.x - fp.x) * 0.5 );
  ip->y = fp.y + ( (lp.y - fp.y) * 0.5 );

  return 1;
}

/*
 * Add a split face by walking on the edge side.
 *
 * @param topo the topology to act upon
 * @param sedge edge id and walking side and direction
 *              (forward,left:positive backward,right:negative)
 * @param face the face in which the edge identifier is known to be
 * @param mbr_only do not create a new face but update MBR of the current
 *
 * @return:
 *    -1: if mbr_only was requested
 *     0: if the edge does not form a ring
 *    -1: if it is impossible to create a face on the requested side
 *        ( new face on the side is the universe )
 *    -2: error
 *   >0 : id of newly added face
 */
static LWT_ELEMID
_lwt_AddFaceSplit( LWT_TOPOLOGY* topo,
                   LWT_ELEMID sedge, LWT_ELEMID face,
                   int mbr_only )
{
  int numedges, numfaceedges, i, j;
  int newface_outside;
  int num_signed_edge_ids;
  LWT_ELEMID *signed_edge_ids;
  LWT_ELEMID *edge_ids;
  LWT_ISO_EDGE *edges;
  LWT_ISO_EDGE *ring_edges;
  LWT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  LWT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;

  signed_edge_ids = lwt_be_getRingEdges(topo, sedge,
                                        &num_signed_edge_ids, 0);
  if ( ! signed_edge_ids ) {
    lwerror("Backend error (no ring edges for edge %" PRId64 "): %s",
            sedge, lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  LWDEBUGF(1, "getRingEdges returned %d edges", num_signed_edge_ids);

  /* You can't get to the other side of an edge forming a ring */
  for (i=0; i<num_signed_edge_ids; ++i) {
    if ( signed_edge_ids[i] == -sedge ) {
      /* No split here */
      LWDEBUG(1, "not a ring");
      lwfree( signed_edge_ids );
      return 0;
    }
  }

  LWDEBUGF(1, "Edge %" PRId64 " splitted face %" PRId64 " (mbr_only:%d)",
           sedge, face, mbr_only);

  /* Construct a polygon using edges of the ring */
  numedges = 0;
  edge_ids = lwalloc(sizeof(LWT_ELEMID)*num_signed_edge_ids);
  for (i=0; i<num_signed_edge_ids; ++i) {
    int absid = llabs(signed_edge_ids[i]);
    int found = 0;
    /* Do not add the same edge twice */
    for (j=0; j<numedges; ++j) {
      if ( edge_ids[j] == absid ) {
        found = 1;
        break;
      }
    }
    if ( ! found ) edge_ids[numedges++] = absid;
  }
  i = numedges;
  ring_edges = lwt_be_getEdgeById(topo, edge_ids, &i,
                                  LWT_COL_EDGE_EDGE_ID|LWT_COL_EDGE_GEOM);
  if ( i == -1 )
  {
    lwfree( signed_edge_ids );
    /* ring_edges should be NULL */
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  else if ( i != numedges )
  {
    lwfree( signed_edge_ids );
    _lwt_release_edges(ring_edges, numedges);
    lwerror("Unexpected error: %d edges found when expecting %d", i, numedges);
    return -2;
  }

  /* Should now build a polygon with those edges, in the order
   * given by GetRingEdges.
   */
  POINTARRAY *pa = NULL;
  for ( i=0; i<num_signed_edge_ids; ++i )
  {
    LWT_ELEMID eid = signed_edge_ids[i];
    LWDEBUGF(1, "Edge %d in ring of edge %" PRId64 " is edge %" PRId64,
                i, sedge, eid);
    LWT_ISO_EDGE *edge = NULL;
    POINTARRAY *epa;
    for ( j=0; j<numedges; ++j )
    {
      if ( ring_edges[j].edge_id == llabs(eid) )
      {
        edge = &(ring_edges[j]);
        break;
      }
    }
    if ( edge == NULL )
    {
      lwfree( signed_edge_ids );
      _lwt_release_edges(ring_edges, numedges);
      lwerror("missing edge that was found in ring edges loop");
      return -2;
    }
    epa = ptarray_clone_deep(edge->geom->points);
    if ( eid < 0 ) ptarray_reverse(epa);

    pa = pa ? ptarray_merge(pa, epa) : epa;
  }
  POINTARRAY **points = lwalloc(sizeof(POINTARRAY*));
  points[0] = pa;
  /* NOTE: the ring may very well have collapsed components,
   *       which would make it topologically invalid
   */
  LWPOLY* shell = lwpoly_construct(0, 0, 1, points);

  int isccw = ptarray_isccw(pa);
  LWDEBUGF(1, "Ring of edge %" PRId64 " is %sclockwise",
              sedge, isccw ? "counter" : "");
  const GBOX* shellbox = lwgeom_get_bbox(lwpoly_as_lwgeom(shell));

  if ( face == 0 )
  {
    /* Edge splitted the universe face */
    if ( ! isccw )
    {
      /* Face on the left side of this ring is the universe face.
       * Next call (for the other side) should create the split face
       */
      LWDEBUG(1, "The left face of this clockwise ring is the universe, "
                 "won't create a new face there");
      return -1;
    }
  }

  if ( mbr_only && face != 0 )
  {
    if ( isccw )
    {{
      LWT_ISO_FACE updface;
      updface.face_id = face;
      updface.mbr = (GBOX *)shellbox; /* const cast, we won't free it, later */
      int ret = lwt_be_updateFacesById( topo, &updface, 1 );
      if ( ret == -1 )
      {
        lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != 1 )
      {
        lwerror("Unexpected error: %d faces found when expecting 1", ret);
        return -2;
      }
    }}
    return -1; /* mbr only was requested */
  }

  LWT_ISO_FACE *oldface = NULL;
  LWT_ISO_FACE newface;
  newface.face_id = -1;
  if ( face != 0 && ! isccw)
  {{
    /* Face created an hole in an outer face */
    int nfaces = 1;
    oldface = lwt_be_getFaceById(topo, &face, &nfaces, LWT_COL_FACE_ALL);
    if ( nfaces == -1 )
    {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -2;
    }
    if ( nfaces != 1 )
    {
      lwerror("Unexpected error: %d faces found when expecting 1", nfaces);
      return -2;
    }
    newface.mbr = oldface->mbr;
  }}
  else
  {
    newface.mbr = (GBOX *)shellbox; /* const cast, we won't free it, later */
  }

  /* Insert the new face */
  int ret = lwt_be_insertFaces( topo, &newface, 1 );
  if ( ret == -1 )
  {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( ret != 1 )
  {
    lwerror("Unexpected error: %d faces inserted when expecting 1", ret);
    return -2;
  }
  if ( oldface ) {
    lwfree(oldface); /* NOTE: oldface.mbr is owned by shell */
  }

  /* Update side location of new face edges */

  /* We want the new face to be on the left, if possible */
  if ( face != 0 && ! isccw ) { /* ring is clockwise in a real face */
    /* face shrinked, must update all non-contained edges and nodes */
    LWDEBUG(1, "New face is on the outside of the ring, updating rings in former shell");
    newface_outside = 1;
    /* newface is outside */
  } else {
    LWDEBUG(1, "New face is on the inside of the ring, updating forward edges in new ring");
    newface_outside = 0;
    /* newface is inside */
  }

  /* Update edges bounding the old face */
  /* (1) fetch all edges where left_face or right_face is = oldface */
  int fields = LWT_COL_EDGE_EDGE_ID |
               LWT_COL_EDGE_FACE_LEFT |
               LWT_COL_EDGE_FACE_RIGHT |
               LWT_COL_EDGE_GEOM
               ;
  numfaceedges = 1;
  edges = lwt_be_getEdgeByFace( topo, &face, &numfaceedges, fields );
  if ( numfaceedges == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  LWDEBUGF(1, "lwt_be_getEdgeByFace returned %d edges", numfaceedges);
  GEOSGeometry *shellgg = 0;
  const GEOSPreparedGeometry* prepshell = 0;
  shellgg = LWGEOM2GEOS( lwpoly_as_lwgeom(shell), 0);
  if ( ! shellgg ) {
    lwpoly_free(shell);
    lwfree(signed_edge_ids);
    _lwt_release_edges(ring_edges, numedges);
    _lwt_release_edges(edges, numfaceedges);
    lwerror("Could not convert shell geometry to GEOS: %s", lwgeom_geos_errmsg);
    return -2;
  }
  prepshell = GEOSPrepare( shellgg );
  if ( ! prepshell ) {
    GEOSGeom_destroy(shellgg);
    lwpoly_free(shell);
    lwfree(signed_edge_ids);
    _lwt_release_edges(ring_edges, numedges);
    _lwt_release_edges(edges, numfaceedges);
    lwerror("Could not prepare shell geometry: %s", lwgeom_geos_errmsg);
    return -2;
  }

  if ( numfaceedges )
  {
    forward_edges = lwalloc(sizeof(LWT_ISO_EDGE)*numfaceedges);
    forward_edges_count = 0;
    backward_edges = lwalloc(sizeof(LWT_ISO_EDGE)*numfaceedges);
    backward_edges_count = 0;

    /* (2) loop over the results and: */
    for ( i=0; i<numfaceedges; ++i )
    {
      LWT_ISO_EDGE *e = &(edges[i]);
      int found = 0;
      int contains;
      GEOSGeometry *egg;
      LWPOINT *epgeom;
      POINT2D ep;

      /* (2.1) skip edges whose ID is in the list of boundary edges ? */
      for ( j=0; j<num_signed_edge_ids; ++j )
      {
        int seid = signed_edge_ids[j];
        if ( seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          LWDEBUGF(1, "Edge %d is a forward edge of the new ring", e->edge_id);
          forward_edges[forward_edges_count].edge_id = e->edge_id;
          forward_edges[forward_edges_count++].face_left = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
        else if ( -seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          LWDEBUGF(1, "Edge %d is a backward edge of the new ring", e->edge_id);
          backward_edges[backward_edges_count].edge_id = e->edge_id;
          backward_edges[backward_edges_count++].face_right = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
      }
      if ( found ) continue;

      /* We need to check only a single point
       * (to avoid collapsed elements of the shell polygon
       * giving false positive).
       * The point but must not be an endpoint.
       */
      if ( ! _lwt_GetInteriorEdgePoint(e->geom, &ep) )
      {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwfree(forward_edges); /* contents owned by ring_edges */
        lwfree(backward_edges); /* contents owned by ring_edges */
        _lwt_release_edges(ring_edges, numedges);
        _lwt_release_edges(edges, numfaceedges);
        lwerror("Could not find interior point for edge %d: %s",
                e->edge_id, lwgeom_geos_errmsg);
        return -2;
      }

      epgeom = lwpoint_make2d(0, ep.x, ep.y);
      egg = LWGEOM2GEOS( lwpoint_as_lwgeom(epgeom) , 0);
      lwpoint_release(epgeom);
      if ( ! egg ) {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwfree(forward_edges); /* contents owned by ring_edges */
        lwfree(backward_edges); /* contents owned by ring_edges */
        _lwt_release_edges(ring_edges, numedges);
        _lwt_release_edges(edges, numfaceedges);
        lwerror("Could not convert edge geometry to GEOS: %s",
                lwgeom_geos_errmsg);
        return -2;
      }
      /* IDEA: can be optimized by computing this on our side rather
       *       than on GEOS (saves conversion of big edges) */
      /* IDEA: check that bounding box shortcut is taken, or use
       *       shellbox to do it here */
      contains = GEOSPreparedContains( prepshell, egg );
      GEOSGeom_destroy(egg);
      if ( contains == 2 )
      {
        GEOSPreparedGeom_destroy(prepshell);
        GEOSGeom_destroy(shellgg);
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwfree(forward_edges); /* contents owned by ring_edges */
        lwfree(backward_edges); /* contents owned by ring_edges */
        _lwt_release_edges(ring_edges, numedges);
        _lwt_release_edges(edges, numfaceedges);
        lwerror("GEOS exception on PreparedContains: %s", lwgeom_geos_errmsg);
        return -2;
      }
      LWDEBUGF(1, "Edge %d %scontained in new ring", e->edge_id,
                  (contains?"":"not "));

      /* (2.2) skip edges (NOT, if newface_outside) contained in shell */
      if ( newface_outside )
      {
        if ( contains )
        {
          LWDEBUGF(1, "Edge %d contained in an hole of the new face",
                      e->edge_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          LWDEBUGF(1, "Edge %d not contained in the face shell",
                      e->edge_id);
          continue;
        }
      }

      /* (2.3) push to forward_edges if left_face = oface */
      if ( e->face_left == face )
      {
        LWDEBUGF(1, "Edge %d has new face on the left side", e->edge_id);
        forward_edges[forward_edges_count].edge_id = e->edge_id;
        forward_edges[forward_edges_count++].face_left = newface.face_id;
      }

      /* (2.4) push to backward_edges if right_face = oface */
      if ( e->face_right == face )
      {
        LWDEBUGF(1, "Edge %d has new face on the right side", e->edge_id);
        backward_edges[backward_edges_count].edge_id = e->edge_id;
        backward_edges[backward_edges_count++].face_right = newface.face_id;
      }
    }

    /* Update forward edges */
    if ( forward_edges_count )
    {
      ret = lwt_be_updateEdgesById(topo, forward_edges,
                                   forward_edges_count,
                                   LWT_COL_EDGE_FACE_LEFT);
      if ( ret == -1 )
      {
        lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != forward_edges_count )
      {
        lwerror("Unexpected error: %d edges updated when expecting %d",
                ret, forward_edges_count);
        return -2;
      }
    }

    /* Update backward edges */
    if ( backward_edges_count )
    {
      ret = lwt_be_updateEdgesById(topo, backward_edges,
                                   backward_edges_count,
                                   LWT_COL_EDGE_FACE_RIGHT);
      if ( ret == -1 )
      {
        lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != backward_edges_count )
      {
        lwerror("Unexpected error: %d edges updated when expecting %d",
                ret, backward_edges_count);
        return -2;
      }
    }

    lwfree(forward_edges);
    lwfree(backward_edges);

  }

  _lwt_release_edges(ring_edges, numedges);
  _lwt_release_edges(edges, numfaceedges);

  /* Update isolated nodes which are now in new face */
  int numisonodes = 1;
  fields = LWT_COL_NODE_NODE_ID | LWT_COL_NODE_GEOM;
  LWT_ISO_NODE *nodes = lwt_be_getNodeByFace(topo, &face,
                                             &numisonodes, fields);
  if ( numisonodes == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( numisonodes ) {
    LWT_ISO_NODE *updated_nodes = lwalloc(sizeof(LWT_ISO_NODE)*numisonodes);
    int nodes_to_update = 0;
    for (i=0; i<numisonodes; ++i)
    {
      LWT_ISO_NODE *n = &(nodes[i]);
      GEOSGeometry *ngg;
      ngg = LWGEOM2GEOS( lwpoint_as_lwgeom(n->geom), 0 );
      int contains;
      if ( ! ngg ) {
        if ( prepshell ) GEOSPreparedGeom_destroy(prepshell);
        if ( shellgg ) GEOSGeom_destroy(shellgg);
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwerror("Could not convert node geometry to GEOS: %s",
                lwgeom_geos_errmsg);
        return -2;
      }
      contains = GEOSPreparedContains( prepshell, ngg );
      GEOSGeom_destroy(ngg);
      if ( contains == 2 )
      {
        if ( prepshell ) GEOSPreparedGeom_destroy(prepshell);
        if ( shellgg ) GEOSGeom_destroy(shellgg);
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwerror("GEOS exception on PreparedContains: %s", lwgeom_geos_errmsg);
        return -2;
      }
      LWDEBUGF(1, "Node %d is %scontained in new ring, newface is %s",
                  n->node_id, contains ? "" : "not ",
                  newface_outside ? "outside" : "inside" );
      if ( newface_outside )
      {
        if ( contains )
        {
          LWDEBUGF(1, "Node %d contained in an hole of the new face",
                      n->node_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          LWDEBUGF(1, "Node %d not contained in the face shell",
                      n->node_id);
          continue;
        }
      }
      lwpoint_release(n->geom);
      updated_nodes[nodes_to_update].node_id = n->node_id;
      updated_nodes[nodes_to_update++].containing_face =
                                       newface.face_id;
      LWDEBUGF(1, "Node %d will be updated", n->node_id);
    }
    lwfree(nodes);
    if ( nodes_to_update )
    {
      int ret = lwt_be_updateNodesById(topo, updated_nodes,
                                       nodes_to_update,
                                       LWT_COL_NODE_CONTAINING_FACE);
      if ( ret == -1 ) {
        lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
    }
    lwfree(updated_nodes);
  }

  GEOSPreparedGeom_destroy(prepshell);
  GEOSGeom_destroy(shellgg);
  lwfree(signed_edge_ids);
  lwpoly_free(shell);

  return newface.face_id;
}

static LWT_ELEMID
_lwt_AddEdge( LWT_TOPOLOGY* topo,
              LWT_ELEMID start_node, LWT_ELEMID end_node,
              LWLINE *geom, int skipChecks, int modFace )
{
  LWT_ISO_EDGE newedge;
  LWGEOM *cleangeom;
  edgeend span; /* start point analisys */
  edgeend epan; /* end point analisys */
  POINT2D p1, pn, p2;
  POINTARRAY *pa;
  LWT_ELEMID *node_ids;
  const LWPOINT *start_node_geom = NULL;
  const LWPOINT *end_node_geom = NULL;
  int num_nodes;
  LWT_ISO_NODE *endpoints;
  int i;
  int prev_left;
  int prev_right;
  LWT_ISO_EDGE seledge;
  LWT_ISO_EDGE updedge;

  if ( ! skipChecks )
  {
    /* curve must be simple */
    if ( ! lwgeom_is_simple(lwline_as_lwgeom(geom)) )
    {
      lwerror("SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  newedge.start_node = start_node;
  newedge.end_node = end_node;
  newedge.geom = geom;
  newedge.face_left = -1;
  newedge.face_right = -1;
  cleangeom = lwgeom_remove_repeated_points( lwline_as_lwgeom(geom), 0 );

  pa = lwgeom_as_lwline(cleangeom)->points;
  if ( pa->npoints < 2 ) {
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }

  /* Initialize endpoint info (some of that ) */
  span.cwFace = span.ccwFace =
  epan.cwFace = epan.ccwFace = -1;

  /* Compute azimut of first edge end on start node */
  getPoint2d_p(pa, 0, &p1);
  getPoint2d_p(pa, 1, &pn);
  if ( p2d_same(&p1, &pn) ) {
    /* Can still happen, for 2-point lines */
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(&p1, &pn, &span.myaz) ) {
    lwerror("error computing azimuth of first edgeend [%g,%g-%g,%g]",
            p1.x, p1.y, pn.x, pn.y);
    return -1;
  }
  lwnotice("edge's start node is %g,%g", p1.x, p1.y);

  /* Compute azimuth of last edge end on end node */
  getPoint2d_p(pa, pa->npoints-1, &p2);
  getPoint2d_p(pa, pa->npoints-2, &pn);
  if ( ! azimuth_pt_pt(&p2, &pn, &epan.myaz) ) {
    lwerror("error computing azimuth of last edgeend [%g,%g-%g,%g]",
            p2.x, p2.y, pn.x, pn.y);
    return -1;
  }
  lwnotice("edge's end node is %g,%g", p2.x, p2.y);

  /*
   * Check endpoints existance, match with Curve geometry
   * and get face information (if any)
   */

  if ( start_node != end_node ) {
    num_nodes = 2;
    node_ids = lwalloc(sizeof(LWT_ELEMID)*num_nodes);
    node_ids[0] = start_node;
    node_ids[1] = end_node;
  } else {
    num_nodes = 1;
    node_ids = lwalloc(sizeof(LWT_ELEMID)*num_nodes);
    node_ids[0] = start_node;
  }

  endpoints = lwt_be_getNodeById( topo, node_ids, &num_nodes, LWT_COL_NODE_ALL );
  if ( ! endpoints ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    LWT_ISO_NODE* node = &(endpoints[i]);
    if ( node->containing_face != -1 )
    {
      if ( newedge.face_left == -1 )
      {
        newedge.face_left = newedge.face_right = node->containing_face;
      }
      else if ( newedge.face_left != node->containing_face )
      {
        lwerror("SQL/MM Spatial exception - geometry crosses an edge"
                " (endnodes in faces %" PRId64 " and %" PRId64 ")",
                newedge.face_left, node->containing_face);
      }
    }

    lwnotice("Node %d, with geom %p (looking for %d and %d)",
             node->node_id, node->geom, start_node, end_node);
    if ( node->node_id == start_node ) {
      start_node_geom = node->geom;
    } 
    if ( node->node_id == end_node ) {
      end_node_geom = node->geom;
    } 
  }

  if ( ! skipChecks )
  {
    if ( ! start_node_geom )
    {
      lwerror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = start_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! p2d_same(&pn, &p1) )
      {
        lwerror("SQL/MM Spatial exception"
                " - start node not geometry start point."
                //" - start node not geometry start point (%g,%g != %g,%g).", pn.x, pn.y, p1.x, p1.y
        );
        return -1;
      }
    }

    if ( ! end_node_geom )
    {
      lwerror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = end_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! p2d_same(&pn, &p2) )
      {
        lwerror("SQL/MM Spatial exception"
                " - end node not geometry end point."
                //" - end node not geometry end point (%g,%g != %g,%g).", pn.x, pn.y, p2.x, p2.y
        );
        return -1;
      }
    }

    if ( _lwt_CheckEdgeCrossing( topo, start_node, end_node, geom ) )
      return -1;

  } /* ! skipChecks */

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = lwt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Find adjacent edges to each endpoint */
  int isclosed = start_node == end_node;
  int found;
  found = _lwt_FindAdjacentEdges( topo, start_node, &span,
                                  isclosed ? &epan : NULL );
  if ( found ) {
    span.was_isolated = 0;
    newedge.next_right = span.nextCW ? span.nextCW : -newedge.edge_id;
    prev_left = span.nextCCW ? -span.nextCCW : newedge.edge_id;
    LWDEBUGF(1, "New edge %d is connected on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.cwFace;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.ccwFace;
    }
  } else {
    span.was_isolated = 1;
    newedge.next_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    prev_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    LWDEBUGF(1, "New edge %d is isolated on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
  }

  found = _lwt_FindAdjacentEdges( topo, end_node, &epan,
                                  isclosed ? &span : NULL );
  if ( found ) {
    epan.was_isolated = 0;
    newedge.next_left = epan.nextCW ? epan.nextCW : newedge.edge_id;
    prev_right = epan.nextCCW ? -epan.nextCCW : -newedge.edge_id;
    LWDEBUGF(1, "New edge %d is connected on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.ccwFace;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.cwFace;
    }
  } else {
    epan.was_isolated = 1;
    newedge.next_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    prev_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    LWDEBUGF(1, "New edge %d is isolated on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
  }

  /*
   * If we don't have faces setup by now we must have encountered
   * a malformed topology (no containing_face on isolated nodes, no
   * left/right faces on adjacent edges or mismatching values)
   */
  if ( newedge.face_left != newedge.face_right )
  {
    lwerror("Left(%" PRId64 ")/right(%" PRId64 ")"
            "faces mismatch: invalid topology ?",
            newedge.face_left, newedge.face_right);
    return -1;
  }
  else if ( newedge.face_left == -1 )
  {
    lwerror("Could not derive edge face from linked primitives:"
            " invalid topology ?");
    return -1;
  }

  /*
   * Insert the new edge, and update all linking
   */

  int ret = lwt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  int updfields;

  /* Link prev_left to us
   * (if it's not us already) */
  if ( llabs(prev_left) != newedge.edge_id )
  {
    if ( prev_left > 0 )
    {
      /* its next_left_edge is us */
      updfields = LWT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = newedge.edge_id;
      seledge.edge_id = prev_left;
    }
    else
    {
      /* its next_right_edge is us */
      updfields = LWT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = newedge.edge_id;
      seledge.edge_id = -prev_left;
    }

    ret = lwt_be_updateEdges(topo,
        &seledge, LWT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* Link prev_right to us 
   * (if it's not us already) */
  if ( llabs(prev_right) != newedge.edge_id )
  {
    if ( prev_right > 0 )
    {
      /* its next_left_edge is -us */
      updfields = LWT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = -newedge.edge_id;
      seledge.edge_id = prev_right;
    }
    else
    {
      /* its next_right_edge is -us */
      updfields = LWT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = -newedge.edge_id;
      seledge.edge_id = -prev_right;
    }

    ret = lwt_be_updateEdges(topo,
        &seledge, LWT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* NOT IN THE SPECS...
   * set containing_face = null for start_node and end_node
   * if they where isolated
   *
   */
  LWT_ISO_NODE updnode, selnode;
  updnode.containing_face = -1;
  if ( span.was_isolated )
  {
    selnode.node_id = start_node;
    ret = lwt_be_updateNodes(topo,
        &selnode, LWT_COL_NODE_NODE_ID,
        &updnode, LWT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  if ( epan.was_isolated )
  {
    selnode.node_id = end_node;
    ret = lwt_be_updateNodes(topo,
        &selnode, LWT_COL_NODE_NODE_ID,
        &updnode, LWT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  int newface1 = -1;

  if ( ! modFace )
  {
    newface1 = _lwt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 0 );
    if ( newface1 == 0 ) {
      LWDEBUG(1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }
  }

  /* Check face splitting */
  int newface = _lwt_AddFaceSplit( topo, newedge.edge_id,
                                   newedge.face_left, 0 );
  if ( modFace )
  {
    if ( newface == 0 ) {
      LWDEBUG(1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }

    if ( newface < 0 )
    {
      /* face on the left is the universe face */
      /* must be forming a maximal ring in universal face */
      newface = _lwt_AddFaceSplit( topo, -newedge.edge_id,
                                   newedge.face_left, 0 );
      if ( newface < 0 ) return newedge.edge_id; /* no split */
    }
    else
    {
      _lwt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 1 );
    }
  }

  /*
   * Update topogeometries, if needed
   */
  if ( newedge.face_left != 0 )
  {
    ret = lwt_be_updateTopoGeomFaceSplit(topo, newedge.face_left,
                                         newface, newface1);
    if ( ret == 0 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    if ( ! modFace )
    {
      /* drop old face from the face table */
      ret = lwt_be_deleteFacesById(topo, &(newedge.face_left), 1);
      if ( ret == -1 ) {
        lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }
  }

  return newedge.edge_id;
}

LWT_ELEMID
lwt_AddEdgeModFace( LWT_TOPOLOGY* topo,
                    LWT_ELEMID start_node, LWT_ELEMID end_node,
                    LWLINE *geom, int skipChecks )
{
  return _lwt_AddEdge( topo, start_node, end_node, geom, skipChecks, 1 );
}

LWT_ELEMID
lwt_AddEdgeNewFaces( LWT_TOPOLOGY* topo,
                    LWT_ELEMID start_node, LWT_ELEMID end_node,
                    LWLINE *geom, int skipChecks )
{
  return _lwt_AddEdge( topo, start_node, end_node, geom, skipChecks, 0 );
}

LWGEOM*
lwt_GetFaceGeometry(LWT_TOPOLOGY* topo, LWT_ELEMID faceid)
{
  int numfaceedges;
  LWT_ISO_EDGE *edges;
  LWT_ISO_FACE *face;
  LWPOLY *out;
  LWGEOM *outg;
  int i;
  int fields;

  if ( faceid == 0 )
  {
    lwerror("SQL/MM Spatial exception - universal face has no geometry");
    return NULL;
  }

  /* Construct the face geometry */
  numfaceedges = 1;
  fields = LWT_COL_EDGE_GEOM |
           LWT_COL_EDGE_FACE_LEFT |
           LWT_COL_EDGE_FACE_RIGHT
           ;
  edges = lwt_be_getEdgeByFace( topo, &faceid, &numfaceedges, fields );
  if ( numfaceedges == -1 ) {
    lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }

  if ( numfaceedges == 0 )
  {
    i = 1;
    face = lwt_be_getFaceById(topo, &faceid, &i, LWT_COL_FACE_FACE_ID);
    if ( i == -1 ) {
      lwerror("Backend error: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    if ( i == 0 ) {
      lwerror("SQL/MM Spatial exception - non-existent face.");
      return NULL;
    }
    lwfree( face );
    if ( i > 1 ) {
      lwerror("Corrupted topology: multiple face records have face_id=%"
              PRId64, faceid);
      return NULL;
    }
    /* Face has no boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    out = lwpoly_construct_empty(topo->srid, topo->hasZ, 0);
    return lwpoly_as_lwgeom(out);
  }

  /* Linemerge polygon boundaries */

  LWGEOM **geoms = lwalloc( sizeof(LWGEOM*) * numfaceedges );
  int validedges = 0;
  for ( i=0; i<numfaceedges; ++i )
  {
    /* NOTE: skipping edges with same face on both sides, although
     *       correct, results in a failure to build faces from
     *       invalid topologies as expected by legacy tests.
     * TODO: update legacy tests expectances/unleash this skipping ?
     */
    /* if ( edges[i].face_left == edges[i].face_right ) continue; */
    geoms[validedges++] = lwline_as_lwgeom(edges[i].geom);
  }
  if ( ! validedges )
  {
    lwfree(geoms);
    /* Face has no valid boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    out = lwpoly_construct_empty(topo->srid, topo->hasZ, 0);
    return lwpoly_as_lwgeom(out);
  }
  LWCOLLECTION *bounds = lwcollection_construct(MULTILINETYPE,
                                                topo->srid,
                                                NULL, /* gbox */
                                                validedges,
                                                geoms);
#if 0 /* debugging (a leaking one) */
  lwnotice("Collected face bounds: %s", lwgeom_to_wkt((LWGEOM*)bounds,
                          WKT_ISO, 2, NULL));
#endif

  outg = lwgeom_buildarea( lwcollection_as_lwgeom(bounds) );

  _lwt_release_edges(edges, numfaceedges);
  lwcollection_release(bounds);

  return outg;
}

int
lwt_GetFaceEdges(LWT_TOPOLOGY* topo, LWT_ELEMID face, LWT_ELEMID **edges)
{
  /* TODO: Construct the face geometry */
  /* TODO: Then for each ring of the face polygon... */
  lwerror("lwt_GetFaceEdges not implemented yet");
  return -1;
}
