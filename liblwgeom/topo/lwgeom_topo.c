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
 * Copyright (C) 2015-2024 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/



#include "../postgis_config.h"

//#define POSTGIS_DEBUG_LEVEL 1
//#define POSTGIS_DEBUG_GEOMETRY_WKB 1
#include "lwgeom_log.h"

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"
#include "lwt_node_edges.h"
#include "lwt_edgeend_star.h"
#include "lwgeom_geos.h"

#include <inttypes.h>

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

LWT_ISO_NODE *
lwt_be_getNodeById(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  CBT3(topo, getNodeById, ids, numelems, fields);
}

LWT_ISO_NODE *
lwt_be_getNodeWithinDistance2D(LWT_TOPOLOGY *topo,
             const LWPOINT *pt,
             double dist,
             uint64_t *numelems,
             int fields,
             int64_t limit)
{
  CBT5(topo, getNodeWithinDistance2D, pt, dist, numelems, fields, limit);
}

static LWT_ISO_NODE *
lwt_be_getNodeWithinBox2D(const LWT_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, uint64_t limit)
{
  CBT4(topo, getNodeWithinBox2D, box, numelems, fields, limit);
}

LWT_ISO_EDGE *
lwt_be_getEdgeWithinBox2D(const LWT_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, uint64_t limit)
{
  CBT4(topo, getEdgeWithinBox2D, box, numelems, fields, limit);
}

LWT_ISO_FACE *
lwt_be_getFaceWithinBox2D(const LWT_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, uint64_t limit)
{
  CBT4(topo, getFaceWithinBox2D, box, numelems, fields, limit);
}

int
lwt_be_insertNodes(LWT_TOPOLOGY *topo, LWT_ISO_NODE *node, uint64_t numelems)
{
  CBT2(topo, insertNodes, node, numelems);
}

int
lwt_be_insertFaces(LWT_TOPOLOGY *topo, LWT_ISO_FACE *face, uint64_t numelems)
{
  CBT2(topo, insertFaces, face, numelems);
}

static int
lwt_be_deleteFacesById(const LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems)
{
  CBT2(topo, deleteFacesById, ids, numelems);
}

static int
lwt_be_deleteNodesById(const LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems)
{
  CBT2(topo, deleteNodesById, ids, numelems);
}

LWT_ELEMID
lwt_be_getNextEdgeId(LWT_TOPOLOGY* topo)
{
  CBT0(topo, getNextEdgeId);
}

LWT_ISO_EDGE *
lwt_be_getEdgeById(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  CBT3(topo, getEdgeById, ids, numelems, fields);
}

static LWT_ISO_FACE *
lwt_be_getFaceById(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  CBT3(topo, getFaceById, ids, numelems, fields);
}

LWT_ISO_EDGE *
lwt_be_getEdgeByNode(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields)
{
  CBT3(topo, getEdgeByNode, ids, numelems, fields);
}

static LWT_ISO_EDGE *
lwt_be_getEdgeByFace(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields, const GBOX *box)
{
  CBT4(topo, getEdgeByFace, ids, numelems, fields, box);
}

static LWT_ISO_NODE *
lwt_be_getNodeByFace(LWT_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields, const GBOX *box)
{
  CBT4(topo, getNodeByFace, ids, numelems, fields, box);
}

LWT_ISO_EDGE *
lwt_be_getEdgeWithinDistance2D(LWT_TOPOLOGY *topo,
             const LWPOINT *pt,
             double dist,
             uint64_t *numelems,
             int fields,
             int64_t limit)
{
  CBT5(topo, getEdgeWithinDistance2D, pt, dist, numelems, fields, limit);
}

int
lwt_be_insertEdges(LWT_TOPOLOGY *topo, LWT_ISO_EDGE *edge, uint64_t numelems)
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

static uint64_t
lwt_be_updateFacesById(LWT_TOPOLOGY* topo,
  const LWT_ISO_FACE* faces, uint64_t numfaces
)
{
  CBT2(topo, updateFacesById, faces, numfaces);
}

int
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

int
lwt_be_updateTopoGeomEdgeSplit(LWT_TOPOLOGY* topo, LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2)
{
  CBT3(topo, updateTopoGeomEdgeSplit, split_edge, new_edge1, new_edge2);
}

static int
lwt_be_updateTopoGeomFaceSplit(LWT_TOPOLOGY* topo, LWT_ELEMID split_face,
                               LWT_ELEMID new_face1, LWT_ELEMID new_face2)
{
  CBT3(topo, updateTopoGeomFaceSplit, split_face, new_face1, new_face2);
}

static int
lwt_be_checkTopoGeomRemEdge(LWT_TOPOLOGY* topo, LWT_ELEMID edge_id,
                            LWT_ELEMID face_left, LWT_ELEMID face_right)
{
  CBT3(topo, checkTopoGeomRemEdge, edge_id, face_left, face_right);
}

static int
lwt_be_checkTopoGeomRemIsoEdge(LWT_TOPOLOGY* topo, LWT_ELEMID edge_id)
{
  CBT1(topo, checkTopoGeomRemIsoEdge, edge_id);
}

static int
lwt_be_checkTopoGeomRemNode(LWT_TOPOLOGY* topo, LWT_ELEMID node_id,
                            LWT_ELEMID eid1, LWT_ELEMID eid2)
{
  CBT3(topo, checkTopoGeomRemNode, node_id, eid1, eid2);
}

static int
lwt_be_checkTopoGeomRemIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID node_id)
{
  CBT1(topo, checkTopoGeomRemIsoNode, node_id);
}

static int
lwt_be_updateTopoGeomFaceHeal(LWT_TOPOLOGY* topo,
                             LWT_ELEMID face1, LWT_ELEMID face2,
                             LWT_ELEMID newface)
{
  CBT3(topo, updateTopoGeomFaceHeal, face1, face2, newface);
}

static int
lwt_be_updateTopoGeomEdgeHeal(LWT_TOPOLOGY* topo,
                             LWT_ELEMID edge1, LWT_ELEMID edge2,
                             LWT_ELEMID newedge)
{
  CBT3(topo, updateTopoGeomEdgeHeal, edge1, edge2, newedge);
}

static LWT_ELEMID *
lwt_be_getRingEdges(LWT_TOPOLOGY *topo, LWT_ELEMID edge, uint64_t *numedges, uint64_t limit)
{
  CBT3(topo, getRingEdges, edge, numedges, limit);
}

LWT_ELEMID
lwt_be_ExistsCoincidentNode(LWT_TOPOLOGY* topo, const LWPOINT* pt)
{
  uint64_t exists = 0;
  lwt_be_getNodeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if (exists == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return 0;
  }
  return exists;
}

LWT_ELEMID
lwt_be_ExistsEdgeIntersectingPoint(LWT_TOPOLOGY* topo, const LWPOINT* pt)
{
  uint64_t exists = 0;
  lwt_be_getEdgeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if (exists == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return 0;
  }
  return exists;
}

static LWT_ISO_EDGE *
lwt_be_getClosestEdge(const LWT_TOPOLOGY *topo, const LWPOINT *pt, uint64_t *numelems, int fields)
{
  CBT3(topo, getClosestEdge, pt, numelems, fields);
}

static GBOX *
lwt_be_computeFaceMBR(const LWT_TOPOLOGY *topo, LWT_ELEMID face)
{
  CBT1(topo, computeFaceMBR, face);
}

/************************************************************************
 *
 * Utility functions
 *
 ************************************************************************/

static LWGEOM *
_lwt_toposnap(LWGEOM *src, LWGEOM *tgt, double tol)
{
  LWGEOM *tmp = src;
  LWGEOM *tmp2;
  int changed;
  int iterations = 0;

  int maxiterations = lwgeom_count_vertices(tgt);

  /* GEOS snapping can be unstable */
  /* See https://trac.osgeo.org/geos/ticket/760 */
  do {
    tmp2 = lwgeom_snap(tmp, tgt, tol);
    ++iterations;
    changed = ( lwgeom_count_vertices(tmp2) != lwgeom_count_vertices(tmp) );
    LWDEBUGF(2, "After iteration %d, geometry changed ? %d (%d vs %d vertices)", iterations, changed, lwgeom_count_vertices(tmp2), lwgeom_count_vertices(tmp));
    if ( tmp != src ) lwgeom_free(tmp);
    tmp = tmp2;
  } while ( changed && iterations <= maxiterations );

  LWDEBUGF(1, "It took %d/%d iterations to properly snap",
              iterations, maxiterations);

  return tmp;
}

void
_lwt_release_faces(LWT_ISO_FACE *faces, int num_faces)
{
  int i;
  for ( i=0; i<num_faces; ++i ) {
    if ( faces[i].mbr ) lwfree(faces[i].mbr);
  }
  lwfree(faces);
}

void
_lwt_release_edges(LWT_ISO_EDGE *edges, int num_edges)
{
  int i;
  for ( i=0; i<num_edges; ++i ) {
    if ( edges[i].geom ) lwline_free(edges[i].geom);
  }
  lwfree(edges);
}

static void
_lwt_release_nodes(LWT_ISO_NODE *nodes, int num_nodes)
{
  int i;
  for ( i=0; i<num_nodes; ++i ) {
    if ( nodes[i].geom ) lwpoint_free(nodes[i].geom);
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

/**
 * @param checkFace if non zero will check the given face
 *        for really containing the point or determine the
 *        face when given face is -1. Use 0 to simply use
 *        the given face value, no matter what (effectively
 *        allowing adding a non-isolated point when used
 *        with face=-1).
 */
static LWT_ELEMID
_lwt_AddIsoNode( LWT_TOPOLOGY* topo, LWT_ELEMID face,
                LWPOINT* pt, int skipISOChecks, int checkFace )
{
  LWT_ELEMID foundInFace = -1;

  if ( lwpoint_is_empty(pt) )
  {
    lwerror("Cannot add empty point as isolated node");
    return -1;
  }


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

  if ( checkFace && ( face == -1 || ! skipISOChecks ) )
  {
    foundInFace = lwt_GetFaceContainingPoint(topo, pt); /*x*/
    if ( foundInFace == -1 ) {
      PGTOPO_BE_ERROR();
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
    PGTOPO_BE_ERROR();
    return -1;
  }

  return node.node_id;
}

LWT_ELEMID
lwt_AddIsoNode( LWT_TOPOLOGY* topo, LWT_ELEMID face,
                LWPOINT* pt, int skipISOChecks )
{
  return _lwt_AddIsoNode( topo, face, pt, skipISOChecks, 1 );
}

/*
 * Check that an edge does not cross an existing node and
 * does not have non-boundary intersection with existing edge
 *
 * @param myself the id of an edge to skip, if any
 *               (for ChangeEdgeGeom). Can use 0 for none.
 *
 * Return -1 on cross or error, 0 if everything is fine.
 * Note that before returning -1, lwerror is invoked...
 */
static int
_lwt_CheckEdgeCrossing( LWT_TOPOLOGY* topo,
                        LWT_ELEMID start_node, LWT_ELEMID end_node,
                        const LWLINE *geom, LWT_ELEMID myself )
{
  uint64_t i, num_nodes, num_edges;
  LWT_ISO_EDGE *edges;
  LWT_ISO_NODE *nodes;
  const GBOX *edgebox;
  GEOSGeometry *edgegg;

  initGEOS(lwnotice, lwgeom_geos_error);

  edgegg = LWGEOM2GEOS(lwline_as_lwgeom(geom), 0);
  if (!edgegg)
  {
    lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
    return -1;
  }
  edgebox = lwgeom_get_bbox( lwline_as_lwgeom(geom) );

  /* loop over each node within the edge's gbox */
  nodes = lwt_be_getNodeWithinBox2D( topo, edgebox, &num_nodes,
                                            LWT_COL_NODE_ALL, 0 );
  LWDEBUGF(1, "lwt_be_getNodeWithinBox2D returned %llu nodes", num_nodes);
  if (num_nodes == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    const POINT2D *pt;
    LWT_ISO_NODE* node = &(nodes[i]);
    if ( node->node_id == start_node ) continue;
    if ( node->node_id == end_node ) continue;
    /* check if the edge contains this node (not on boundary) */
    /* ST_RelateMatch(rec.relate, 'T********') */
    pt = getPoint2d_cp(node->geom->point, 0);
    int contains = ptarray_contains_point_partial(geom->points, pt, LW_FALSE, NULL) == LW_BOUNDARY;
    if ( contains )
    {
      GEOSGeom_destroy(edgegg);
      _lwt_release_nodes(nodes, num_nodes);
      lwerror("SQL/MM Spatial exception - geometry crosses a node");
      return -1;
    }
  }
  if ( nodes ) _lwt_release_nodes(nodes, num_nodes);
               /* may be NULL if num_nodes == 0 */

  /* loop over each edge within the edge's gbox */
  edges = lwt_be_getEdgeWithinBox2D( topo, edgebox, &num_edges, LWT_COL_EDGE_ALL, 0 );
  LWDEBUGF(1, "lwt_be_getEdgeWithinBox2D returned %llu edges", num_edges);
  if (num_edges == UINT64_MAX)
  {
    GEOSGeom_destroy(edgegg);
    PGTOPO_BE_ERROR();
    return -1;
  }
  for ( i=0; i<num_edges; ++i )
  {
    LWT_ISO_EDGE* edge = &(edges[i]);
    LWT_ELEMID edge_id = edge->edge_id;
    GEOSGeometry *eegg;
    char *relate;
    int match;

    if ( edge_id == myself ) continue;

    if ( ! edge->geom ) {
      _lwt_release_edges(edges, num_edges);
      lwerror("Edge %" LWTFMT_ELEMID " has NULL geometry!", edge_id);
      return -1;
    }

    eegg = LWGEOM2GEOS( lwline_as_lwgeom(edge->geom), 0 );
    if ( ! eegg ) {
      GEOSGeom_destroy(edgegg);
      _lwt_release_edges(edges, num_edges);
      lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
      return -1;
    }

    LWDEBUGF(2, "Edge %" LWTFMT_ELEMID " converted to GEOS", edge_id);

    /* check if the edge has a non-boundary-boundary intersection with our edge */

    relate = GEOSRelateBoundaryNodeRule(eegg, edgegg, 2);
    GEOSGeom_destroy(eegg);
    if ( ! relate ) {
      GEOSGeom_destroy(edgegg);
      _lwt_release_edges(edges, num_edges);
      lwerror("GEOSRelateBoundaryNodeRule error: %s", lwgeom_geos_errmsg);
      return -1;
    }

    LWDEBUGF(2, "Edge %" LWTFMT_ELEMID " relate pattern is %s", edge_id, relate);

    match = GEOSRelatePatternMatch(relate, "FF*F*****");
    if ( match ) {
      /* error or no interior intersection */
      GEOSFree(relate);
      if ( match == 2 ) {
        _lwt_release_edges(edges, num_edges);
        GEOSGeom_destroy(edgegg);
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
        return -1;
      }
      else continue; /* no interior intersection */
    }

    match = GEOSRelatePatternMatch(relate, "1FFF*FFF2");
    if ( match ) {
      _lwt_release_edges(edges, num_edges);
      GEOSGeom_destroy(edgegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("SQL/MM Spatial exception - coincident edge %" LWTFMT_ELEMID,
                edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "1********");
    if ( match ) {
      _lwt_release_edges(edges, num_edges);
      GEOSGeom_destroy(edgegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("Spatial exception - geometry intersects edge %"
                LWTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "T********");
    if ( match ) {
      _lwt_release_edges(edges, num_edges);
      GEOSGeom_destroy(edgegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("SQL/MM Spatial exception - geometry crosses edge %"
                LWTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "*T*******");
    if ( match ) {
      _lwt_release_edges(edges, num_edges);
      GEOSGeom_destroy(edgegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("Spatial exception - geometry boundary touches interior of edge %"
                LWTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch(relate, "***T*****");
    if ( match ) {
      _lwt_release_edges(edges, num_edges);
      GEOSGeom_destroy(edgegg);
      GEOSFree(relate);
      if ( match == 2 ) {
        lwerror("GEOSRelatePatternMatch error: %s", lwgeom_geos_errmsg);
      } else {
        lwerror("Spatial exception - boundary of edge %" LWTFMT_ELEMID" touches interior of geometry", edge_id);
      }
      return -1;
    }

    LWDEBUGF(2, "Edge %" LWTFMT_ELEMID " analysis completed, it does no harm", edge_id);

    GEOSFree(relate);
  }
  LWDEBUGF(1, "No edge crossing detected among the %llu candidate edges", num_edges);
  if ( edges ) _lwt_release_edges(edges, num_edges);
              /* would be NULL if num_edges was 0 */

  GEOSGeom_destroy(edgegg);

  return 0;
}


LWT_ELEMID
lwt_AddIsoEdge( LWT_TOPOLOGY* topo, LWT_ELEMID startNode,
                LWT_ELEMID endNode, const LWLINE* geom )
{
  uint64_t num_nodes;
  uint64_t i;
  LWT_ISO_EDGE newedge;
  LWT_ISO_NODE *endpoints;
  LWT_ELEMID containing_face = -1;
  LWT_ELEMID node_ids[2];
  LWT_ISO_NODE updated_nodes[2];
  int skipISOChecks = 0;
  POINT2D p1, p2;

  /* NOT IN THE SPECS:
   * A closed edge is never isolated (as it forms a face)
   */
  if (startNode == endNode)
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
  node_ids[0] = startNode;
  node_ids[1] = endNode;
  endpoints = lwt_be_getNodeById( topo, node_ids, &num_nodes,
                                             LWT_COL_NODE_ALL );
  if (num_nodes == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  else if ( num_nodes < 2 )
  {
    if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);
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
        if ( ! P2D_SAME_STRICT(&p1, &p2) )
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
        if ( ! P2D_SAME_STRICT(&p1, &p2) )
        {
          _lwt_release_nodes(endpoints, num_nodes);
          lwerror("SQL/MM Spatial exception - "
                  "end node not geometry end point.");
          return -1;
        }
      }
    }
  }

  if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);

  if ( ! skipISOChecks )
  {
    if ( _lwt_CheckEdgeCrossing( topo, startNode, endNode, geom, 0 ) )
    {
      /* would have called lwerror already, leaking :( */
      return -1;
    }
  }

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = lwt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    PGTOPO_BE_ERROR();
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
    PGTOPO_BE_ERROR();
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
  updated_nodes[0].node_id = startNode;
  updated_nodes[0].containing_face = -1;
  updated_nodes[1].node_id = endNode;
  updated_nodes[1].containing_face = -1;
  ret = lwt_be_updateNodesById(topo, updated_nodes, 2,
                               LWT_COL_NODE_CONTAINING_FACE);
  if ( ret == -1 ) {
    PGTOPO_BE_ERROR();
    return -1;
  }

  return newedge.edge_id;
}

static LWCOLLECTION *
_lwt_EdgeSplit( LWT_TOPOLOGY* topo, LWT_ELEMID edge, LWPOINT* pt, int skipISOChecks, LWT_ISO_EDGE** oldedge )
{
  LWGEOM *split;
  LWCOLLECTION *split_col;
  uint64_t i;

  /* Get edge */
  i = 1;
  LWDEBUG(1, "calling lwt_be_getEdgeById");
  *oldedge = lwt_be_getEdgeById(topo, &edge, &i, LWT_COL_EDGE_ALL);
  LWDEBUGF(1, "lwt_be_getEdgeById returned %p", *oldedge);
  LWDEBUGG(2, lwline_as_lwgeom(oldedge[0]->geom), "Edge to be split");
  if ( ! *oldedge )
  {
    LWDEBUGF(1, "lwt_be_getEdgeById returned NULL and set i=%llu", i);
    if (i == UINT64_MAX)
    {
      PGTOPO_BE_ERROR();
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
              "but numelements output parameter has value %" PRIu64
              "(expected 0 or 1)", i);
      return NULL;
    }
  }


  /*
   *  - check if a coincident node already exists
   */
  if ( ! skipISOChecks )
  {
    LWDEBUG(1, "calling lwt_be_ExistsCoincidentNode");
    if ( lwt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      LWDEBUG(1, "lwt_be_ExistsCoincidentNode returned");
      _lwt_release_edges(*oldedge, 1);
      lwerror("SQL/MM Spatial exception - coincident node");
      return NULL;
    }
    LWDEBUG(1, "lwt_be_ExistsCoincidentNode returned");
  }

  /* Split edge */
  split = lwgeom_split((LWGEOM*)(*oldedge)->geom, (LWGEOM*)pt);
  if ( ! split )
  {
    _lwt_release_edges(*oldedge, 1);
    lwerror("could not split edge by point ?");
    return NULL;
  }
  split_col = lwgeom_as_lwcollection(split);
  if ( ! split_col ) {
    _lwt_release_edges(*oldedge, 1);
    lwgeom_free(split);
    lwerror("lwgeom_as_lwcollection returned NULL");
    return NULL;
  }
  if (split_col->ngeoms < 2) {
    LWDEBUGG(1, lwpoint_as_lwgeom(pt), "Unable to split edge by point");
    _lwt_release_edges(*oldedge, 1);
    lwgeom_free(split);
    lwerror("SQL/MM Spatial exception - point not on edge");
    return NULL;
  }

#if POSTGIS_DEBUG_LEVEL > 1
  {
  size_t sz;
  char *wkt = lwgeom_to_wkt((LWGEOM*)split_col, WKT_EXTENDED, 2, &sz);
  LWDEBUGF(1, "returning split col: %s", wkt);
  lwfree(wkt);
  }
#endif
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
  /* Make sure the SRID is set on the subgeom */
  ((LWGEOM*)oldedge_geom)->srid = split_col->srid;
  ((LWGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! lwt_be_insertNodes(topo, &node, 1) )
  {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if (node.node_id == -1) {
    /* should have been set by backend */
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Insert the new edge */
  newedge1.edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedge1.edge_id == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("first geometry in lwgeom_split output is not a line");
    return -1;
  }
  ret = lwt_be_insertEdges(topo, &newedge1, 1);
  if ( ret == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  } else if ( ret == 0 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update the old edge */
  updedge.geom = lwgeom_as_lwline(oldedge_geom);
  /* lwgeom_split of a line should only return lines ... */
  if ( ! updedge.geom ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  } else if ( ret == 0 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", oldedge->edge_id);
    return -1;
  } else if ( ret > 1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("More than a single edge found with id %" LWTFMT_ELEMID " !", oldedge->edge_id);
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = lwt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedge1.edge_id, -1);
  if ( ! ret ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  _lwt_release_edges(oldedge, 1);
  lwcollection_free(split_col);

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
  /* Make sure the SRID is set on the subgeom */
  ((LWGEOM*)oldedge_geom)->srid = split_col->srid;
  ((LWGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! lwt_be_insertNodes(topo, &node, 1) )
  {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if (node.node_id == -1) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    /* should have been set by backend */
    lwerror("Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Delete the old edge */
  seledge.edge_id = edge;
  ret = lwt_be_deleteEdges(topo, &seledge, LWT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* Get new edges identifiers */
  newedges[0].edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedges[0].edge_id == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }
  newedges[1].edge_id = lwt_be_getNextEdgeId(topo);
  if ( newedges[1].edge_id == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("second geometry in lwgeom_split output is not a line");
    return -1;
  }

  /* Insert both new edges */
  ret = lwt_be_insertEdges(topo, newedges, 2);
  if ( ret == -1 ) {
    _lwt_release_edges(oldedge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  } else if ( ret == 0 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    lwerror("Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update all next edge references pointing to old edge id */

  updedge.next_right = newedges[0].edge_id;
  seledge.next_right = edge;
  seledge.start_node = oldedge->start_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_RIGHT|LWT_COL_EDGE_START_NODE,
      &updedge, LWT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  updedge.next_right = -newedges[1].edge_id;
  seledge.next_right = -edge;
  seledge.start_node = oldedge->end_node;
  ret = lwt_be_updateEdges(topo,
      &seledge, LWT_COL_EDGE_NEXT_RIGHT|LWT_COL_EDGE_START_NODE,
      &updedge, LWT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
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
    _lwt_release_edges(oldedge, 1);
    lwcollection_release(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = lwt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedges[0].edge_id, newedges[1].edge_id);
  if ( ! ret ) {
    _lwt_release_edges(oldedge, 1);
    lwcollection_free(split_col);
    PGTOPO_BE_ERROR();
    return -1;
  }

  _lwt_release_edges(oldedge, 1);
  lwcollection_free(split_col);

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
 * Get first distinct vertex from endpoint
 * @param pa the pointarray to seek points in
 * @param ref the point we want to search a distinct one
 * @param from vertex index to start from (will really start from "from"+dir)
 * @param dir  1 to go forward
 *            -1 to go backward
 * @return 1 on success, 0 if edge is collapsed (no distinct points)
 */
static int
_lwt_FirstDistinctVertex2D(const POINTARRAY* pa, const POINT2D *ref, int from, int dir, POINT2D *op)
{
  int i, toofar, inc;
  POINT2D fp;

  if ( dir > 0 )
  {
    toofar = pa->npoints;
    inc = 1;
  }
  else
  {
    toofar = -1;
    inc = -1;
  }

  LWDEBUGF(2, "first point is index %d", from);
  fp = *ref; /* getPoint2d_p(pa, from, &fp); */
  for ( i = from+inc; i != toofar; i += inc )
  {
    LWDEBUGF(2, "testing point %d", i);
    getPoint2d_p(pa, i, op); /* pick next point */
    if ( P2D_SAME_STRICT(op,&fp) ) continue; /* equal to startpoint */
    /* this is a good one, neither same of start nor of end point */
    return 1; /* found */
  }

  /* no distinct vertices found */
  return 0;
}

/*
 * Return non-zero on failure (lwerror is invoked in that case)
 * Possible failures:
 *  -1 no two distinct vertices exist
 *  -2 azimuth computation failed
 */
static int
_lwt_InitEdgeEndByLine(edgeend *fee, edgeend *lee, LWLINE *edge,
                                            POINT2D *fp, POINT2D *lp)
{
  POINTARRAY *pa = edge->points;
  POINT2D pt;

  fee->nextCW = fee->nextCCW =
  lee->nextCW = lee->nextCCW = 0;
  fee->cwFace = fee->ccwFace =
  lee->cwFace = lee->ccwFace = -1;

  /* Compute azimuth of first edge end */
  LWDEBUG(1, "computing azimuth of first edge end");
  if ( ! _lwt_FirstDistinctVertex2D(pa, fp, 0, 1, &pt) )
  {
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(fp, &pt, &(fee->myaz)) ) {
    lwerror("error computing azimuth of first edgeend [%.15g %.15g,%.15g %.15g]",
            fp->x, fp->y, pt.x, pt.y);
    return -2;
  }
  LWDEBUGF(1, "azimuth of first edge end [%.15g %.15g,%.15g %.15g] is %g",
            fp->x, fp->y, pt.x, pt.y, fee->myaz);

  /* Compute azimuth of second edge end */
  LWDEBUG(1, "computing azimuth of second edge end");
  if ( ! _lwt_FirstDistinctVertex2D(pa, lp, pa->npoints-1, -1, &pt) )
  {
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(lp, &pt, &(lee->myaz)) ) {
    lwerror("error computing azimuth of last edgeend [%.15g %.15g,%.15g %.15g]",
            lp->x, lp->y, pt.x, pt.y);
    return -2;
  }
  LWDEBUGF(1, "azimuth of last edge end [%.15g %.15g,%.15g %.15g] is %g",
            lp->x, lp->y, pt.x, pt.y, lee->myaz);

  return 0;
}

/*
 * Find the first edges encountered going clockwise and counterclockwise
 * around a node, starting from the given azimuth, and take
 * note of the face on both sides.
 *
 * @param topo the topology to load edges from
 * @param node the identifier of the node to analyze
 * @param data input (myaz) / output (nextCW, nextCCW) parameter
 * @param other edgeend, if also incident to given node (closed edge).
 * @param myedge_id identifier of the edge id that data->myaz belongs to
 * @return number of incident edges found
 *
 */
static int
_lwt_FindAdjacentEdges( LWT_TOPOLOGY* topo, LWT_ELEMID node, edgeend *data,
                        edgeend *other, LWT_ELEMID myedge_id )
{
  LWT_ISO_EDGE *edges;
  uint64_t numedges = 1;
  uint64_t i;
  double minaz, maxaz;
  double az, azdif;

  data->nextCW = data->nextCCW = 0;
  data->cwFace = data->ccwFace = -1;

  if ( other ) {
    azdif = other->myaz - data->myaz;
    if ( azdif < 0 ) azdif += 2 * M_PI;
    minaz = maxaz = azdif;
    /* TODO: set nextCW/nextCCW/cwFace/ccwFace to other->something ? */
    LWDEBUGF(1, "Other edge end has cwFace=%" LWTFMT_ELEMID
                " and ccwFace=%" LWTFMT_ELEMID,
                other->cwFace, other->ccwFace);
  } else {
    minaz = maxaz = -1;
  }

  LWDEBUGF(1, "Looking for edges incident to node %" LWTFMT_ELEMID
              " and adjacent to azimuth %g", node, data->myaz);

  /* Get incident edges */
  edges = lwt_be_getEdgeByNode( topo, &node, &numedges, LWT_COL_EDGE_ALL );
  if (numedges == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return 0;
  }

  LWDEBUGF(1, "getEdgeByNode returned %llu edges, minaz=%g, maxaz=%g",
              numedges, minaz, maxaz);

  /* For each incident edge-end (1 or 2): */
  for ( i = 0; i < numedges; ++i )
  {
    LWT_ISO_EDGE *edge;
    LWGEOM *g;
    LWGEOM *cleangeom;
    POINT2D p1, p2;
    POINTARRAY *pa;

    edge = &(edges[i]);

    if ( edge->edge_id == myedge_id ) continue;

    g = lwline_as_lwgeom(edge->geom);
    /* NOTE: remove_repeated_points call could be replaced by
     * some other mean to pick two distinct points for endpoints */
    cleangeom = lwgeom_remove_repeated_points( g, 0 );
    pa = lwgeom_as_lwline(cleangeom)->points;

    if ( pa->npoints < 2 ) {{
      LWT_ELEMID id = edge->edge_id;
      _lwt_release_edges(edges, numedges);
      lwgeom_free(cleangeom);
      lwerror("corrupted topology: edge %" LWTFMT_ELEMID
              " does not have two distinct points", id);
      return -1;
    }}

    if ( edge->start_node == node ) {
      getPoint2d_p(pa, 0, &p1);
      if ( ! _lwt_FirstDistinctVertex2D(pa, &p1, 0, 1, &p2) )
      {
        lwerror("Edge %" LWTFMT_ELEMID " has no distinct vertices: [%.15g %.15g,%.15g %.15g]: ",
                edge->edge_id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }
      LWDEBUGF(1, "edge %" LWTFMT_ELEMID
                  " starts on node %" LWTFMT_ELEMID
                  ", edgeend is [%.15g %.15g,%.15g %.15g]",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {{
        LWT_ELEMID id = edge->edge_id;
        _lwt_release_edges(edges, numedges);
        lwgeom_free(cleangeom);
        lwerror("error computing azimuth of edge %"
          LWTFMT_ELEMID " first edgeend [%.15g %.15g,%.15g %.15g]",
          id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      LWDEBUGF(1, "azimuth of edge %" LWTFMT_ELEMID
                  ": %.15g (diff: %.15g)", edge->edge_id, az, azdif);

      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = edge->edge_id; /* outgoing */
        data->cwFace = edge->face_left;
        data->ccwFace = edge->face_right;
        LWDEBUGF(1, "new nextCW and nextCCW edge is %" LWTFMT_ELEMID
                    ", outgoing, "
                    "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                    " (face_right is new ccwFace, face_left is new cwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = edge->edge_id; /* outgoing */
          data->cwFace = edge->face_left;
          LWDEBUGF(1, "new nextCW edge is %" LWTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                      " (previous had minaz=%g, face_left is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = edge->edge_id; /* outgoing */
          data->ccwFace = edge->face_right;
          LWDEBUGF(1, "new nextCCW edge is %" LWTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                      " (previous had maxaz=%g, face_right is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    if ( edge->end_node == node ) {
      getPoint2d_p(pa, pa->npoints-1, &p1);
      if ( ! _lwt_FirstDistinctVertex2D(pa, &p1, pa->npoints-1, -1, &p2) )
      {
        lwerror("Edge %" LWTFMT_ELEMID " has no distinct vertices: [%.15g %.15g,%.15g %.15g]: ",
                edge->edge_id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }
      LWDEBUGF(1, "edge %" LWTFMT_ELEMID " ends on node %" LWTFMT_ELEMID
                  ", edgeend is [%.15g %.15g,%.15g %.15g]",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(&p1, &p2, &az) ) {{
        LWT_ELEMID id = edge->edge_id;
        _lwt_release_edges(edges, numedges);
        lwgeom_free(cleangeom);
        lwerror("error computing azimuth of edge %"
          LWTFMT_ELEMID " last edgeend [%.15g %.15g,%.15g %.15g]",
          id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      LWDEBUGF(1, "azimuth of edge %" LWTFMT_ELEMID
                  ": %.15g (diff: %.15g)", edge->edge_id, az, azdif);
      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = -edge->edge_id; /* incoming */
        data->cwFace = edge->face_right;
        data->ccwFace = edge->face_left;
        LWDEBUGF(1, "new nextCW and nextCCW edge is %" LWTFMT_ELEMID
                    ", incoming, "
                    "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                    " (face_right is new cwFace, face_left is new ccwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = -edge->edge_id; /* incoming */
          data->cwFace = edge->face_right;
          LWDEBUGF(1, "new nextCW edge is %" LWTFMT_ELEMID
                      ", incoming, "
                      "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                      " (previous had minaz=%g, face_right is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = -edge->edge_id; /* incoming */
          data->ccwFace = edge->face_left;
          LWDEBUGF(1, "new nextCCW edge is %" LWTFMT_ELEMID
                      ", outgoing, from start point, "
                      "with face_left %" LWTFMT_ELEMID " and face_right %" LWTFMT_ELEMID
                      " (previous had maxaz=%g, face_left is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    lwgeom_free(cleangeom);
  }
  if ( numedges ) _lwt_release_edges(edges, numedges);

  LWDEBUGF(1, "edges adjacent to azimuth %g"
              " (incident to node %" LWTFMT_ELEMID ")"
              ": CW:%" LWTFMT_ELEMID "(%g) CCW:%" LWTFMT_ELEMID "(%g)",
              data->myaz, node, data->nextCW, minaz,
              data->nextCCW, maxaz);

  if ( myedge_id < 1 && numedges && data->cwFace != data->ccwFace )
  {
    if ( data->cwFace != -1 && data->ccwFace != -1 ) {
      lwerror("Corrupted topology: adjacent edges %" LWTFMT_ELEMID " and %" LWTFMT_ELEMID
              " bind different face (%" LWTFMT_ELEMID " and %" LWTFMT_ELEMID ")",
              data->nextCW, data->nextCCW,
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
  uint32_t i;
  POINT2D fp, lp, tp;
  POINTARRAY *pa = edge->points;

  if ( pa->npoints < 2 ) return 0; /* empty or structurally collapsed */

  getPoint2d_p(pa, 0, &fp); /* save first point */
  getPoint2d_p(pa, pa->npoints-1, &lp); /* save last point */
  for (i=1; i<pa->npoints-1; ++i)
  {
    getPoint2d_p(pa, i, &tp); /* pick next point */
    if ( P2D_SAME_STRICT(&tp, &fp) ) continue; /* equal to startpoint */
    if ( P2D_SAME_STRICT(&tp, &lp) ) continue; /* equal to endpoint */
    /* this is a good one, neither same of start nor of end point */
    *ip = tp;
    return 1; /* found */
  }

  /* no distinct vertex found */

  /* interpolate if start point != end point */

  if ( P2D_SAME_STRICT(&fp, &lp) ) return 0; /* no distinct points in edge */

  ip->x = fp.x + ( (lp.x - fp.x) * 0.5 );
  ip->y = fp.y + ( (lp.y - fp.y) * 0.5 );

  return 1;
}

static LWPOLY *
_lwt_MakeRingShell(LWT_TOPOLOGY *topo, LWT_ELEMID *signed_edge_ids, uint64_t num_signed_edge_ids)
{
  LWT_ELEMID *edge_ids;
  uint64_t numedges, i, j;
  LWT_ISO_EDGE *ring_edges;

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
  lwfree( edge_ids );
  if (i == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return NULL;
  }
  else if ( i != numedges )
  {
    lwfree( signed_edge_ids );
    _lwt_release_edges(ring_edges, i);
    lwerror("Unexpected error: %" LWTFMT_ELEMID
      " edges found when expecting %" PRIu64, i, numedges);
    return NULL;
  }

  /* Should now build a polygon with those edges, in the order
   * given by GetRingEdges.
   */
  POINTARRAY *pa = NULL;
  for ( i=0; i<num_signed_edge_ids; ++i )
  {
    LWT_ELEMID eid = signed_edge_ids[i];
    LWDEBUGF(2, "Edge %llu in ring is edge %" LWTFMT_ELEMID, i, eid);
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
      _lwt_release_edges(ring_edges, numedges);
      lwerror("missing edge that was found in ring edges loop");
      return NULL;
    }

    if ( pa == NULL )
    {
      pa = ptarray_clone_deep(edge->geom->points);
      if ( eid < 0 ) ptarray_reverse_in_place(pa);
    }
    else
    {
      if ( eid < 0 )
      {
        epa = ptarray_clone_deep(edge->geom->points);
        ptarray_reverse_in_place(epa);
        ptarray_append_ptarray(pa, epa, 0);
        ptarray_free(epa);
      }
      else
      {
        /* avoid a clone here */
        ptarray_append_ptarray(pa, edge->geom->points, 0);
      }
    }
  }
  _lwt_release_edges(ring_edges, numedges);
  POINTARRAY **points = lwalloc(sizeof(POINTARRAY*));
  points[0] = pa;

  /* NOTE: the ring may very well have collapsed components,
   *       which would make it topologically invalid
   */
  LWPOLY* shell = lwpoly_construct(0, 0, 1, points);
  return shell;
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
  uint64_t numfaceedges, i, j;
  int newface_outside;
  uint64_t num_signed_edge_ids;
  LWT_ELEMID *signed_edge_ids;
  LWT_ISO_EDGE *edges;
  LWT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  LWT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;

  signed_edge_ids = lwt_be_getRingEdges(topo, sedge, &num_signed_edge_ids, 0);
  if (!signed_edge_ids)
  {
    //PGTOPO_BE_ERRORF("no ring edges for edge %" LWTFMT_ELEMID, sedge);
    PGTOPO_BE_ERROR();
    return -2;
  }
  LWDEBUGF(1, "getRingEdges returned %llu edges", num_signed_edge_ids);

  /* You can't get to the other side of an edge forming a ring */
  for (i=0; i<num_signed_edge_ids; ++i) {
    if ( signed_edge_ids[i] == -sedge ) {
      /* No split here */
      LWDEBUG(1, "not a ring");
      lwfree( signed_edge_ids );
      return 0;
    }
  }

  LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " split face %" LWTFMT_ELEMID " (mbr_only:%d)",
           sedge, face, mbr_only);

  /*
   * Construct a polygon using edges of the ring
   *
   * NOTE: this possibly includes dangling edges
   *
   */
  LWPOLY *shell = _lwt_MakeRingShell(topo, signed_edge_ids,
                                     num_signed_edge_ids);
  if ( ! shell ) {
    lwfree( signed_edge_ids );
    /* ring_edges should be NULL */
    lwerror("Could not create ring shell: %s", lwt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  const POINTARRAY *pa = shell->rings[0];
  if ( ! ptarray_is_closed_2d(pa) )
  {
    lwpoly_free(shell);
    lwfree( signed_edge_ids );
    lwerror("Corrupted topology: ring of edge %" LWTFMT_ELEMID
            " is geometrically not-closed", sedge);
    return -2;
  }

  int isccw = ptarray_isccw(pa);
  LWDEBUGF(1, "Ring of edge %" LWTFMT_ELEMID " is %sclockwise",
              sedge, isccw ? "counter" : "");
  const GBOX* shellbox = lwgeom_get_bbox(lwpoly_as_lwgeom(shell));

  if ( face == 0 )
  {
    /* Edge split the universe face */
    if ( ! isccw )
    {
      lwpoly_free(shell);
      lwfree( signed_edge_ids );
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
        lwfree( signed_edge_ids );
        lwpoly_free(shell); /* NOTE: owns shellbox above */
        PGTOPO_BE_ERROR();
        return -2;
      }
      if ( ret != 1 )
      {
        lwfree( signed_edge_ids );
        lwpoly_free(shell); /* NOTE: owns shellbox above */
        lwerror("Unexpected error: %d faces found when expecting 1", ret);
        return -2;
      }
    }}
    lwfree( signed_edge_ids );
    lwpoly_free(shell); /* NOTE: owns shellbox above */
    return -1; /* mbr only was requested */
  }

  LWT_ISO_FACE *oldface = NULL;
  LWT_ISO_FACE newface;
  newface.face_id = -1;
  if ( face != 0 && ! isccw)
  {{
    /* Face created an hole in an outer face */
    uint64_t nfaces = 1;
    oldface = lwt_be_getFaceById(topo, &face, &nfaces, LWT_COL_FACE_ALL);
    if (nfaces == UINT64_MAX)
    {
      lwfree( signed_edge_ids );
      lwpoly_free(shell); /* NOTE: owns shellbox */
      PGTOPO_BE_ERROR();
      return -2;
    }
    if ( nfaces != 1 )
    {
      lwfree( signed_edge_ids );
      lwpoly_free(shell); /* NOTE: owns shellbox */
      lwerror("Unexpected error: %" PRIu64 " faces found when expecting 1", nfaces);
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
    lwfree( signed_edge_ids );
    lwpoly_free(shell); /* NOTE: owns shellbox */
    PGTOPO_BE_ERROR();
    return -2;
  }
  if ( ret != 1 )
  {
    lwfree( signed_edge_ids );
    lwpoly_free(shell); /* NOTE: owns shellbox */
    lwerror("Unexpected error: %d faces inserted when expecting 1", ret);
    return -2;
  }
  if ( oldface ) {
    newface.mbr = NULL; /* it is a reference to oldface mbr... */
    _lwt_release_faces(oldface, 1);
  }

  /* Update side location of new face edges */

  /* We want the new face to be on the left, if possible */
  if ( face != 0 && ! isccw ) { /* ring is clockwise in a real face */
    /* face shrunk, must update all non-contained edges and nodes */
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
  edges = lwt_be_getEdgeByFace( topo, &face, &numfaceedges, fields, newface.mbr );
  if (numfaceedges == UINT64_MAX)
  {
    lwfree(signed_edge_ids);
    PGTOPO_BE_ERROR();
    return -2;
  }
  LWDEBUGF(1, "_lwt_AddFaceSplit: lwt_be_getEdgeByFace(%" LWTFMT_ELEMID ") returned %llu edges", face, numfaceedges);

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
      POINT2D ep;

      /* (2.1) skip edges whose ID is in the list of boundary edges */
      for ( j=0; j<num_signed_edge_ids; ++j )
      {
        int seid = signed_edge_ids[j]; /* signed ring edge id */
        if ( seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids, to speed next loop ? */
          LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " is a known forward edge of the new ring", e->edge_id);
          forward_edges[forward_edges_count].edge_id = e->edge_id;
          forward_edges[forward_edges_count++].face_left = newface.face_id;
          found++;
          if ( found == 2 ) break; /* both edge sides are found on the ring */
        }
        else if ( -seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids, to speed next loop ? */
          LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " is a known backward edge of the new ring", e->edge_id);
          backward_edges[backward_edges_count].edge_id = e->edge_id;
          backward_edges[backward_edges_count++].face_right = newface.face_id;
          found++;
          if ( found == 2 ) break; /* both edge sides are found on the ring */
        }
      }
      if ( found ) continue;
      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " is not a known edge of the new ring", e->edge_id);

      /* Check if the edge is now binding a different face */

      if ( ! getPoint2d_p(e->geom->points, 0, &ep) )
      {
        lwfree(signed_edge_ids);
        lwpoly_free(shell);
        lwfree(forward_edges); /* contents owned by edges */
        lwfree(backward_edges); /* contents owned by edges */
        _lwt_release_edges(edges, numfaceedges);
        lwerror("Edge %" LWTFMT_ELEMID " is empty", e->edge_id);
        return -2;
      }

      contains = gbox_contains_point2d(shellbox, &ep) == LW_TRUE ? LW_INSIDE : LW_OUTSIDE;
      contains = contains == LW_INSIDE ? ptarray_contains_point(pa, &ep) : contains;

      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " first point %s new ring",
          e->edge_id, (contains == LW_INSIDE ? "inside" :
           contains == LW_OUTSIDE ? "outside" : "on boundary of"));

      /* (2.2) skip edges (NOT, if newface_outside) contained in ring */
      if ( newface_outside )
      {
        if ( contains != LW_OUTSIDE )
        {
          LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " not outside of the new ring, not updating it",
                      e->edge_id);
          continue;
        }
      }
      else
      {
        if ( contains != LW_INSIDE )
        {
          LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " not inside the new ring, not updating it",
                      e->edge_id);
          continue;
        }
      }

      /* (2.3) push to forward_edges if left_face = oface */
      if ( e->face_left == face )
      {
        LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " has new face on the left side", e->edge_id);
        forward_edges[forward_edges_count].edge_id = e->edge_id;
        forward_edges[forward_edges_count++].face_left = newface.face_id;
      }

      /* (2.4) push to backward_edges if right_face = oface */
      if ( e->face_right == face )
      {
        LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " has new face on the right side", e->edge_id);
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
        lwfree( signed_edge_ids );
        PGTOPO_BE_ERROR();
        return -2;
      }
      if ( ret != forward_edges_count )
      {
        lwfree( signed_edge_ids );
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
        lwfree( signed_edge_ids );
        PGTOPO_BE_ERROR();
        return -2;
      }
      if ( ret != backward_edges_count )
      {
        lwfree( signed_edge_ids );
        lwerror("Unexpected error: %d edges updated when expecting %d",
                ret, backward_edges_count);
        return -2;
      }
    }

    lwfree(forward_edges);
    lwfree(backward_edges);

    _lwt_release_edges(edges, numfaceedges);
  }

  /* Update isolated nodes which are now in new face */
  uint64_t numisonodes = 1;
  fields = LWT_COL_NODE_NODE_ID | LWT_COL_NODE_GEOM;
  LWT_ISO_NODE *nodes = lwt_be_getNodeByFace(topo, &face,
                                             &numisonodes, fields, newface.mbr);
  if (numisonodes == UINT64_MAX)
  {
    lwfree(signed_edge_ids);
    PGTOPO_BE_ERROR();
    return -2;
  }
  if ( numisonodes ) {
    LWT_ISO_NODE *updated_nodes = lwalloc(sizeof(LWT_ISO_NODE)*numisonodes);
    int nodes_to_update = 0;
    for (i=0; i<numisonodes; ++i)
    {
      LWT_ISO_NODE *n = &(nodes[i]);
      const POINT2D *pt = getPoint2d_cp(n->geom->point, 0);
      int contains = ptarray_contains_point(pa, pt) == LW_INSIDE;
      LWDEBUGF(1, "Node %" LWTFMT_ELEMID " is %scontained in new ring, newface is %s",
                  n->node_id, contains ? "" : "not ",
                  newface_outside ? "outside" : "inside" );
      if ( newface_outside )
      {
        if ( contains )
        {
          LWDEBUGF(1, "Node %" LWTFMT_ELEMID " contained in an hole of the new face",
                      n->node_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          LWDEBUGF(1, "Node %" LWTFMT_ELEMID " not contained in the face shell",
                      n->node_id);
          continue;
        }
      }
      updated_nodes[nodes_to_update].node_id = n->node_id;
      updated_nodes[nodes_to_update++].containing_face =
                                       newface.face_id;
      LWDEBUGF(1, "Node %" LWTFMT_ELEMID " will be updated", n->node_id);
    }
    _lwt_release_nodes(nodes, numisonodes);
    if ( nodes_to_update )
    {
      int ret = lwt_be_updateNodesById(topo, updated_nodes,
                                       nodes_to_update,
                                       LWT_COL_NODE_CONTAINING_FACE);
      if ( ret == -1 ) {
        lwfree( signed_edge_ids );
        PGTOPO_BE_ERROR();
        return -2;
      }
    }
    lwfree(updated_nodes);
  }

  lwfree(signed_edge_ids);
  lwpoly_free(shell);

  return newface.face_id;
}

/**
 * @param modFace can be
 *    0 - have two new faces replace a split face
 *    1 - modify a split face, adding a new one
 *   -1 - do not check at all for face splitting
 *
 */
static LWT_ELEMID
_lwt_AddEdge( LWT_TOPOLOGY* topo,
              LWT_ELEMID start_node, LWT_ELEMID end_node,
              LWLINE *geom, int skipChecks, int modFace )
{
  LWT_ISO_EDGE newedge;
  LWGEOM *cleangeom;
  edgeend span; /* start point analysis */
  edgeend epan; /* end point analysis */
  POINT2D p1, pn, p2;
  POINTARRAY *pa;
  LWT_ELEMID node_ids[2];
  const LWPOINT *start_node_geom = NULL;
  const LWPOINT *end_node_geom = NULL;
  uint64_t num_nodes;
  LWT_ISO_NODE *endpoints;
  uint64_t i;
  LWT_ELEMID prev_left;
  LWT_ELEMID prev_right;
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
  /* TODO: should do the repeated points removal in 2D space */
  cleangeom = lwgeom_remove_repeated_points( lwline_as_lwgeom(geom), 0 );

  pa = lwgeom_as_lwline(cleangeom)->points;
  if ( pa->npoints < 2 ) {
    lwgeom_free(cleangeom);
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }

  /* Initialize endpoint info (some of that ) */
  span.cwFace = span.ccwFace =
  epan.cwFace = epan.ccwFace = -1;

  /* Compute azimuth of first edge end on start node */
  getPoint2d_p(pa, 0, &p1);
  if ( ! _lwt_FirstDistinctVertex2D(pa, &p1, 0, 1, &pn) )
  {
    lwgeom_free(cleangeom);
    lwerror("Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(&p1, &pn, &span.myaz) ) {
    lwgeom_free(cleangeom);
    lwerror("error computing azimuth of first edgeend [%.15g %.15g,%.15g %.15g]",
            p1.x, p1.y, pn.x, pn.y);
    return -1;
  }
  LWDEBUGF(1, "edge's start node is %g,%g", p1.x, p1.y);

  /* Compute azimuth of last edge end on end node */
  getPoint2d_p(pa, pa->npoints-1, &p2);
  if ( ! _lwt_FirstDistinctVertex2D(pa, &p2, pa->npoints-1, -1, &pn) )
  {
    lwgeom_free(cleangeom);
    /* This should never happen as we checked the edge while computing first edgend */
    lwerror("Invalid clean edge (no two distinct vertices exist) - should not happen");
    return -1;
  }
  lwgeom_free(cleangeom);
  if ( ! azimuth_pt_pt(&p2, &pn, &epan.myaz) ) {
    lwerror("error computing azimuth of last edgeend [%.15g %.15g,%.15g %.15g]",
            p2.x, p2.y, pn.x, pn.y);
    return -1;
  }
  LWDEBUGF(1, "edge's end node is %g,%g", p2.x, p2.y);

  /*
   * Check endpoints existence, match with Curve geometry
   * and get face information (if any)
   */

  if ( start_node != end_node ) {
    num_nodes = 2;
    node_ids[0] = start_node;
    node_ids[1] = end_node;
  } else {
    num_nodes = 1;
    node_ids[0] = start_node;
  }

  endpoints = lwt_be_getNodeById( topo, node_ids, &num_nodes, LWT_COL_NODE_ALL );
  if (num_nodes == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    LWT_ISO_NODE* node = &(endpoints[i]);
    if ( modFace != -1 && node->containing_face != -1 )
    {
      if ( newedge.face_left == -1 )
      {
        newedge.face_left = newedge.face_right = node->containing_face;
      }
      else if ( newedge.face_left != node->containing_face )
      {
        _lwt_release_nodes(endpoints, num_nodes);
        lwerror("SQL/MM Spatial exception - geometry crosses an edge"
                " (endnodes in faces %" LWTFMT_ELEMID " and %" LWTFMT_ELEMID ")",
                newedge.face_left, node->containing_face);
      }
    }

    LWDEBUGF(1, "Node %" LWTFMT_ELEMID ", with geom %p (looking for %"
             LWTFMT_ELEMID " and %" LWTFMT_ELEMID ")",
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
      if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);
      lwerror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = start_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! P2D_SAME_STRICT(&pn, &p1) )
      {
        if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);
        lwerror("SQL/MM Spatial exception"
                " - start node not geometry start point."
                //" - start node not geometry start point (%g,%g != %g,%g).", pn.x, pn.y, p1.x, p1.y
        );
        return -1;
      }
    }

    if ( ! end_node_geom )
    {
      if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);
      lwerror("SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = end_node_geom->point;
      getPoint2d_p(pa, 0, &pn);
      if ( ! P2D_SAME_STRICT(&pn, &p2) )
      {
        if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);
        lwerror("SQL/MM Spatial exception"
                " - end node not geometry end point."
                //" - end node not geometry end point (%g,%g != %g,%g).", pn.x, pn.y, p2.x, p2.y
        );
        return -1;
      }
    }

    if ( num_nodes ) _lwt_release_nodes(endpoints, num_nodes);

    if ( _lwt_CheckEdgeCrossing( topo, start_node, end_node, geom, 0 ) )
      return -1;

  } /* ! skipChecks */

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = lwt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* Find adjacent edges to each endpoint */
  int isclosed = start_node == end_node;
  int found;
  found = _lwt_FindAdjacentEdges( topo, start_node, &span,
                                  isclosed ? &epan : NULL, -1 );
  if ( found ) {
    span.was_isolated = 0;
    newedge.next_right = span.nextCW ? span.nextCW : -newedge.edge_id;
    prev_left = span.nextCCW ? -span.nextCCW : newedge.edge_id;
    LWDEBUGF(1, "New edge %" LWTFMT_ELEMID " is connected on start node, "
                "next_right is %" LWTFMT_ELEMID
                ", prev_left is %" LWTFMT_ELEMID,
                newedge.edge_id, newedge.next_right, prev_left);
    if ( modFace != -1 )
    {
      if ( newedge.face_right == -1 ) {
        newedge.face_right = span.cwFace;
      }
      if ( newedge.face_left == -1 ) {
        newedge.face_left = span.ccwFace;
      }
    }
  } else {
    span.was_isolated = 1;
    newedge.next_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    prev_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    LWDEBUGF(1, "New edge %" LWTFMT_ELEMID " is isolated on start node, "
                "next_right is %" LWTFMT_ELEMID
                ", prev_left is %" LWTFMT_ELEMID,
                newedge.edge_id, newedge.next_right, prev_left);
  }

  found = _lwt_FindAdjacentEdges( topo, end_node, &epan,
                                  isclosed ? &span : NULL, -1 );
  if ( found ) {
    epan.was_isolated = 0;
    newedge.next_left = epan.nextCW ? epan.nextCW : newedge.edge_id;
    prev_right = epan.nextCCW ? -epan.nextCCW : -newedge.edge_id;
    LWDEBUGF(1, "New edge %" LWTFMT_ELEMID " is connected on end node, "
                "next_left is %" LWTFMT_ELEMID
                ", prev_right is %" LWTFMT_ELEMID,
                newedge.edge_id, newedge.next_left, prev_right);
    if ( modFace != -1 )
    {
      if ( newedge.face_right == -1 ) {
        newedge.face_right = span.ccwFace;
      } else if ( newedge.face_right != epan.ccwFace ) {
        /* side-location conflict */
        lwerror("Side-location conflict: "
                "new edge starts in face"
                 " %" LWTFMT_ELEMID " and ends in face"
                 " %" LWTFMT_ELEMID,
                newedge.face_right, epan.ccwFace
        );
        return -1;
      }
      if ( newedge.face_left == -1 ) {
        newedge.face_left = span.cwFace;
      } else if ( newedge.face_left != epan.cwFace ) {
        /* side-location conflict */
        lwerror("Side-location conflict: "
                "new edge starts in face"
                 " %" LWTFMT_ELEMID " and ends in face"
                 " %" LWTFMT_ELEMID,
                newedge.face_left, epan.cwFace
        );
        return -1;
      }
    }
  } else {
    epan.was_isolated = 1;
    newedge.next_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    prev_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    LWDEBUGF(1, "New edge %" LWTFMT_ELEMID " is isolated on end node, "
                "next_left is %" LWTFMT_ELEMID
                ", prev_right is %" LWTFMT_ELEMID,
                newedge.edge_id, newedge.next_left, prev_right);
  }

  /*
   * If we don't have faces setup by now we must have encountered
   * a malformed topology (no containing_face on isolated nodes, no
   * left/right faces on adjacent edges or mismatching values)
   */
  if ( modFace > -1 )
  {
    if ( newedge.face_left != newedge.face_right )
    {
      lwerror("Left(%" LWTFMT_ELEMID ")/right(%" LWTFMT_ELEMID ")"
              " faces mismatch: invalid topology ?",
              newedge.face_left, newedge.face_right);
      return -1;
    }
    else if ( newedge.face_left == -1 )
    {
      lwerror("Could not derive edge face from linked primitives:"
              " invalid topology ?");
      return -1;
    }
  }

  /*
   * Insert the new edge, and update all linking
   */

  int ret = lwt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    PGTOPO_BE_ERROR();
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
      PGTOPO_BE_ERROR();
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
      PGTOPO_BE_ERROR();
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
      PGTOPO_BE_ERROR();
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
      PGTOPO_BE_ERROR();
      return -1;
    }
  }

  /* Check face splitting, if required */

  if ( modFace > -1 ) {

  if ( ! isclosed && ( epan.was_isolated || span.was_isolated ) )
  {
    LWDEBUG(1, "New edge is dangling, so it cannot split any face");
    return newedge.edge_id; /* no split */
  }

  int newface1 = -1;

  /* IDEA: avoid building edge ring if input is closed, which means we
   *       know in advance it splits a face */

  if ( ! modFace )
  {
    newface1 = _lwt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 0 );
    if ( newface1 == 0 ) {
      LWDEBUG(1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }
  }

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
      PGTOPO_BE_ERROR();
      return -1;
    }

    if ( ! modFace )
    {
      /* drop old face from the face table */
      ret = lwt_be_deleteFacesById(topo, &(newedge.face_left), 1);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      }
    }
  }

  } // end of face split checking

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

static LWGEOM *
_lwt_FaceByEdges(LWT_TOPOLOGY *topo, LWT_ISO_EDGE *edges, int numfaceedges)
{
  LWGEOM *outg;
  LWCOLLECTION *bounds;
  LWGEOM **geoms = lwalloc( sizeof(LWGEOM*) * numfaceedges );
  int i, validedges = 0;

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
    /* Face has no valid boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    if ( numfaceedges ) lwfree(geoms);
    LWDEBUG(1, "_lwt_FaceByEdges returning empty polygon");
    return lwpoly_as_lwgeom(
            lwpoly_construct_empty(topo->srid, topo->hasZ, 0)
           );
  }
  bounds = lwcollection_construct(MULTILINETYPE,
                                  topo->srid,
                                  NULL, /* gbox */
                                  validedges,
                                  geoms);
  outg = lwgeom_buildarea( lwcollection_as_lwgeom(bounds) );
  lwcollection_release(bounds);
  lwfree(geoms);
#if 0
  {
  size_t sz;
  char *wkt = lwgeom_to_wkt(outg, WKT_EXTENDED, 2, &sz);
  LWDEBUGF(1, "_lwt_FaceByEdges returning area: %s", wkt);
  lwfree(wkt);
  }
#endif
  return outg;
}

LWGEOM*
lwt_GetFaceGeometry(LWT_TOPOLOGY* topo, LWT_ELEMID faceid)
{
  uint64_t numfaceedges;
  LWT_ISO_EDGE *edges;
  LWT_ISO_FACE *face;
  LWPOLY *out;
  LWGEOM *outg;
  uint64_t i, edgeid;
  int fields;

  if (faceid == 0)
  {
    lwerror("SQL/MM Spatial exception - universal face has no geometry");
    return NULL;
  }

  /* Construct the face geometry */
  numfaceedges = 1;
  fields = LWT_COL_EDGE_GEOM |
           LWT_COL_EDGE_EDGE_ID |
           LWT_COL_EDGE_FACE_LEFT |
           LWT_COL_EDGE_FACE_RIGHT
           ;
  edges = lwt_be_getEdgeByFace( topo, &faceid, &numfaceedges, fields, NULL );
  if (numfaceedges == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return NULL;
  }
  LWDEBUGF(1, "lwt_GetFaceGeometry: lwt_be_getEdgeByFace returned %llu edges", numfaceedges);

  if ( numfaceedges == 0 )
  {
    i = 1;
    face = lwt_be_getFaceById(topo, &faceid, &i, LWT_COL_FACE_FACE_ID);
    if (i == UINT64_MAX)
    {
      PGTOPO_BE_ERROR();
      return NULL;
    }
    if ( i == 0 ) {
      lwerror("SQL/MM Spatial exception - non-existent face.");
      return NULL;
    }
    lwfree( face );
    if ( i > 1 ) {
      lwerror("Corrupted topology: multiple face records have face_id=%"
              LWTFMT_ELEMID, faceid);
      return NULL;
    }
    /* Face has no boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    lwnotice("Corrupted topology: face %"
        LWTFMT_ELEMID " has no associated edges.", faceid);
    out = lwpoly_construct_empty(topo->srid, topo->hasZ, 0);
    return lwpoly_as_lwgeom(out);
  }
  edgeid = edges[0].edge_id;

  outg = _lwt_FaceByEdges( topo, edges, numfaceedges );
  _lwt_release_edges(edges, numfaceedges);

  if ( ! outg )
  {
    /* Face did have edges but no polygon could be constructed
     * with that material, sounds like a corrupted topology..
     *
     * We'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
      lwnotice("Corrupted topology: face %"
        LWTFMT_ELEMID " could not be constructed only from edges "
        "knowing about it (like edge %" LWTFMT_ELEMID ").",
        faceid, edgeid);
      out = lwpoly_construct_empty(topo->srid, topo->hasZ, 0);
      return lwpoly_as_lwgeom(out);
  }

  return outg;
}

/* Find which edge from the "edges" set defines the next
 * portion of the given "ring".
 *
 * The edge might be either forward or backward.
 *
 * @param ring The ring to find definition of.
 *             It is assumed it does not contain duplicated vertices.
 * @param from offset of the ring point to start looking from
 * @param edges array of edges to search into
 * @param numedges number of edges in the edges array
 *
 * @return index of the edge defining the next ring portion or
 *               -1 if no edge was found to be part of the ring
 */
static int
_lwt_FindNextRingEdge(const POINTARRAY *ring, int from,
                      const LWT_ISO_EDGE *edges, int numedges)
{
  int i;
  POINT2D p1;

  /* Get starting ring point */
  getPoint2d_p(ring, from, &p1);

  LWDEBUGF(1, "Ring's 'from' point (%d) is %g,%g", from, p1.x, p1.y);

  /* find the edges defining the next portion of ring starting from
   * vertex "from" */
  for ( i=0; i<numedges; ++i )
  {
    const LWT_ISO_EDGE *isoe = &(edges[i]);
    LWLINE *edge = isoe->geom;
    POINTARRAY *epa = edge->points;
    POINT2D p2, pt;
    int match = 0;
    uint32_t j;

    /* Skip if the edge is a dangling one */
    if ( isoe->face_left == isoe->face_right )
    {
      LWDEBUGF(3, "_lwt_FindNextRingEdge: edge %" LWTFMT_ELEMID
                  " has same face (%" LWTFMT_ELEMID
                  ") on both sides, skipping",
                  isoe->edge_id, isoe->face_left);
      continue;
    }

    if (epa->npoints < 2)
    {
      LWDEBUGF(3, "_lwt_FindNextRingEdge: edge %" LWTFMT_ELEMID
                  " has only %"PRIu32" points",
                  isoe->edge_id, epa->npoints);
      continue;
    }

#if 0
    size_t sz;
    LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " is %s",
                isoe->edge_id,
                lwgeom_to_wkt(lwline_as_lwgeom(edge), WKT_EXTENDED, 2, &sz));
#endif

    /* ptarray_remove_repeated_points ? */

    getPoint2d_p(epa, 0, &p2);
    LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " 'first' point is %g,%g",
                isoe->edge_id, p2.x, p2.y);
    LWDEBUGF(1, "Rings's 'from' point is still %g,%g", p1.x, p1.y);
    if ( P2D_SAME_STRICT(&p1, &p2) )
    {
      LWDEBUG(1, "P2D_SAME_STRICT(p1,p2) returned true");
      LWDEBUGF(1, "First point of edge %" LWTFMT_ELEMID
                  " matches ring vertex %d", isoe->edge_id, from);
      /* first point matches, let's check next non-equal one */
      for ( j=1; j<epa->npoints; ++j )
      {
        getPoint2d_p(epa, j, &p2);
        LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " 'next' point %d is %g,%g",
                    isoe->edge_id, j, p2.x, p2.y);
        /* we won't check duplicated edge points */
        if ( P2D_SAME_STRICT(&p1, &p2) ) continue;
        /* we assume there are no duplicated points in ring */
        getPoint2d_p(ring, from+1, &pt);
        LWDEBUGF(1, "Ring's point %d is %g,%g",
                    from+1, pt.x, pt.y);
        match = P2D_SAME_STRICT(&pt, &p2);
        break; /* we want to check a single non-equal next vertex */
      }
#if POSTGIS_DEBUG_LEVEL > 0
      if ( match ) {
        LWDEBUGF(1, "Prev point of edge %" LWTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        LWDEBUGF(1, "Prev point of edge %" LWTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( ! match )
    {
      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " did not match as forward",
                 isoe->edge_id);
      getPoint2d_p(epa, epa->npoints-1, &p2);
      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " 'last' point is %g,%g",
                  isoe->edge_id, p2.x, p2.y);
      if ( P2D_SAME_STRICT(&p1, &p2) )
      {
        LWDEBUGF(1, "Last point of edge %" LWTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from);
        /* last point matches, let's check next non-equal one */
        for ( j=2; j<=epa->npoints; j++ )
        {
          getPoint2d_p(epa, epa->npoints - j, &p2);
          LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " 'prev' point %d is %g,%g",
                      isoe->edge_id, epa->npoints - j, p2.x, p2.y);
          /* we won't check duplicated edge points */
          if ( P2D_SAME_STRICT(&p1, &p2) ) continue;
          /* we assume there are no duplicated points in ring */
          getPoint2d_p(ring, from+1, &pt);
          LWDEBUGF(1, "Ring's point %d is %g,%g",
                      from+1, pt.x, pt.y);
          match = P2D_SAME_STRICT(&pt, &p2);
          break; /* we want to check a single non-equal next vertex */
        }
      }
#if POSTGIS_DEBUG_LEVEL > 0
      if ( match ) {
        LWDEBUGF(1, "Prev point of edge %" LWTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        LWDEBUGF(1, "Prev point of edge %" LWTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( match ) return i;

  }

  return -1;
}

/* Reverse values in array between "from" (inclusive)
 * and "to" (exclusive) indexes */
static void
_lwt_ReverseElemidArray(LWT_ELEMID *ary, int from, int to)
{
  LWT_ELEMID t;
  while (from < to)
  {
    t = ary[from];
    ary[from++] = ary[to];
    ary[to--] = t;
  }
}

/* Rotate values in array between "from" (inclusive)
 * and "to" (exclusive) indexes, so that "rotidx" is
 * the new value at "from" */
static void
_lwt_RotateElemidArray(LWT_ELEMID *ary, int from, int to, int rotidx)
{
  _lwt_ReverseElemidArray(ary, from, rotidx-1);
  _lwt_ReverseElemidArray(ary, rotidx, to-1);
  _lwt_ReverseElemidArray(ary, from, to-1);
}


int
lwt_GetFaceEdges(LWT_TOPOLOGY* topo, LWT_ELEMID face_id, LWT_ELEMID **out )
{
  LWGEOM *face;
  LWPOLY *facepoly;
  LWT_ISO_EDGE *edges;
  uint64_t numfaceedges;
  int fields;
  uint32_t i;
  int nseid = 0; /* number of signed edge ids */
  int prevseid;
  LWT_ELEMID *seid; /* signed edge ids */

  /* Get list of face edges */
  numfaceedges = 1;
  fields = LWT_COL_EDGE_EDGE_ID |
           LWT_COL_EDGE_GEOM |
           LWT_COL_EDGE_FACE_LEFT |
           LWT_COL_EDGE_FACE_RIGHT
           ;
  edges = lwt_be_getEdgeByFace( topo, &face_id, &numfaceedges, fields, NULL );
  if (numfaceedges == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( ! numfaceedges ) return 0; /* no edges in output */
  LWDEBUGF(1, "lwt_GetFaceEdges: lwt_be_getEdgeByFace returned %llu edges", numfaceedges);

  /* order edges by occurrence in face */

  face = _lwt_FaceByEdges(topo, edges, numfaceedges);
  if ( ! face )
  {
    /* _lwt_FaceByEdges should have already invoked lwerror in this case */
    _lwt_release_edges(edges, numfaceedges);
    lwerror("Corrupted topology: unable to build geometry of face %"
      LWTFMT_ELEMID " from its %"PRIu64" edges", face_id, numfaceedges);
    return -1;
  }

  if ( lwgeom_is_empty(face) )
  {
    /* no edges in output */
    _lwt_release_edges(edges, numfaceedges);
    lwgeom_free(face);
    return 0;
  }

  /* force_lhr, if the face is not the universe */
  /* _lwt_FaceByEdges seems to guaranteed RHR */
  /* lwgeom_force_clockwise(face); */
  if ( face_id ) lwgeom_reverse_in_place(face);

#if 0
  {
  size_t sz;
  char *wkt = lwgeom_to_wkt(face, WKT_EXTENDED, 6, &sz);
  LWDEBUGF(1, "Geometry of face %" LWTFMT_ELEMID " is: %s",
              face_id, wkt);
  lwfree(wkt);
  }
#endif

  facepoly = lwgeom_as_lwpoly(face);
  if ( ! facepoly )
  {
    _lwt_release_edges(edges, numfaceedges);
    lwgeom_free(face);
    lwerror("Geometry of face %" LWTFMT_ELEMID " is not a polygon", face_id);
    return -1;
  }

  nseid = prevseid = 0;
  seid = lwalloc( sizeof(LWT_ELEMID) * numfaceedges );

  /* for each ring of the face polygon... */
  for ( i=0; i<facepoly->nrings; ++i )
  {
    const POINTARRAY *ring = facepoly->rings[i];
    int32_t j = 0;
    LWT_ISO_EDGE *nextedge;
    LWLINE *nextline;

    LWDEBUGF(1, "Ring %d has %d points", i, ring->npoints);

    while ( j < (int32_t) ring->npoints-1 )
    {
      LWDEBUGF(1, "Looking for edge covering ring %d from vertex %d",
                  i, j);

      int edgeno = _lwt_FindNextRingEdge(ring, j, edges, numfaceedges);
      if ( edgeno == -1 )
      {
        /* should never happen */
        _lwt_release_edges(edges, numfaceedges);
        lwgeom_free(face);
        lwfree(seid);
        lwerror("No edge (among %" PRIu64 ") found to be defining geometry of face %"
                LWTFMT_ELEMID, numfaceedges, face_id);
        return -1;
      }

      nextedge = &(edges[edgeno]);
      nextline = nextedge->geom;

      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID
                  " covers ring %d from vertex %d to %d",
                  nextedge->edge_id, i, j, j + nextline->points->npoints - 1);

#if 0
      {
      size_t sz;
      char *wkt = lwgeom_to_wkt(lwline_as_lwgeom(nextline), WKT_EXTENDED, 6, &sz);
      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " is %s",
                  nextedge->edge_id, wkt);
      lwfree(wkt);
      }
#endif

      j += nextline->points->npoints - 1;

      /* Add next edge to the output array */
      seid[nseid++] = nextedge->face_left == face_id ?
                          nextedge->edge_id :
                         -nextedge->edge_id;

      /* avoid checking again on next time turn */
      nextedge->face_left = nextedge->face_right = -1;
    }

    /* now "scroll" the list of edges so that the one
     * with smaller absolute edge_id is first */
    /* Range is: [prevseid, nseid) -- [inclusive, exclusive) */
    if ( (nseid - prevseid) > 1 )
    {{
      LWT_ELEMID minid = 0;
      int minidx = 0;
      LWDEBUGF(1, "Looking for smallest id among the %d edges "
                  "composing ring %d", (nseid-prevseid), i);
      for ( j=prevseid; j<nseid; ++j )
      {
        LWT_ELEMID id = llabs(seid[j]);
        LWDEBUGF(1, "Abs id of edge in pos %d is %" LWTFMT_ELEMID, j, id);
        if ( ! minid || id < minid )
        {
          minid = id;
          minidx = j;
        }
      }
      LWDEBUGF(1, "Smallest id is %" LWTFMT_ELEMID
                  " at position %d", minid, minidx);
      if ( minidx != prevseid )
      {
        _lwt_RotateElemidArray(seid, prevseid, nseid, minidx);
      }
    }}

    prevseid = nseid;
  }

  lwgeom_free(face);
  _lwt_release_edges(edges, numfaceedges);

  *out = seid;
  return nseid;
}

int
lwt_ChangeEdgeGeom(LWT_TOPOLOGY* topo, LWT_ELEMID edge_id, LWLINE *geom)
{
  LWT_ISO_EDGE *oldedge;
  LWT_ISO_EDGE newedge;
  POINT2D p1, p2, pt;
  uint64_t i;
  int isclosed = 0;
  int leftRingIsCCW = -1;

  /* curve must be simple */
  if ( ! lwgeom_is_simple(lwline_as_lwgeom(geom)) )
  {
    lwerror("SQL/MM Spatial exception - curve not simple");
    return -1;
  }

  i = 1;
  oldedge = lwt_be_getEdgeById(topo, &edge_id, &i, LWT_COL_EDGE_ALL);
  if ( ! oldedge )
  {
    LWDEBUGF(1, "lwt_ChangeEdgeGeom: "
                "lwt_be_getEdgeById returned NULL and set i=%llu", i);
    if (i == UINT64_MAX)
    {
      PGTOPO_BE_ERROR();
      return -1;
    }
    else if ( i == 0 )
    {
      lwerror("SQL/MM Spatial exception - non-existent edge %"
              LWTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      lwerror("Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %" PRIu64 " "
              "(expected 0 or 1)", i);
      return -1;
    }
  }

  LWDEBUGF(1, "lwt_ChangeEdgeGeom: "
              "old edge has %d points, new edge has %d points",
              oldedge->geom->points->npoints, geom->points->npoints);

  /*
   * e) Check StartPoint consistency
   */
  getPoint2d_p(oldedge->geom->points, 0, &p1);
  getPoint2d_p(geom->points, 0, &pt);
  if ( ! P2D_SAME_STRICT(&p1, &pt) )
  {
    _lwt_release_edges(oldedge, 1);
    lwerror("SQL/MM Spatial exception - "
            "start node not geometry start point.");
    return -1;
  }

  /*
   * f) Check EndPoint consistency
   */
  if ( oldedge->geom->points->npoints < 2 )
  {
    _lwt_release_edges(oldedge, 1);
    lwerror("Corrupted topology: edge %" LWTFMT_ELEMID
            " has less than 2 vertices", oldedge->edge_id);
    return -1;
  }
  getPoint2d_p(oldedge->geom->points, oldedge->geom->points->npoints-1, &p2);
  if ( geom->points->npoints < 2 )
  {
    _lwt_release_edges(oldedge, 1);
    lwerror("Invalid edge: less than 2 vertices");
    return -1;
  }
  getPoint2d_p(geom->points, geom->points->npoints-1, &pt);
  if ( ! P2D_SAME_STRICT(&pt, &p2) )
  {
    _lwt_release_edges(oldedge, 1);
    lwerror("SQL/MM Spatial exception - "
            "end node not geometry end point.");
    return -1;
  }

  /* Not in the specs:
   * if the edge is closed, check we didn't change winding !
   *       (should be part of isomorphism checking)
   */
  if ( oldedge->start_node == oldedge->end_node )
  {
    isclosed = 1;
#if 1 /* TODO: this is actually bogus as a test */
    /* check for valid edge (distinct vertices must exist) */
    if ( ! _lwt_GetInteriorEdgePoint(geom, &pt) )
    {
      _lwt_release_edges(oldedge, 1);
      lwerror("Invalid edge (no two distinct vertices exist)");
      return -1;
    }
#endif

    if ( ptarray_isccw(oldedge->geom->points) !=
         ptarray_isccw(geom->points) )
    {
      _lwt_release_edges(oldedge, 1);
      lwerror("Edge twist at node POINT(%g %g)", p1.x, p1.y);
      return -1;
    }
  }

  if ( _lwt_CheckEdgeCrossing(topo, oldedge->start_node,
                                    oldedge->end_node, geom, edge_id ) )
  {
    /* would have called lwerror already, leaking :( */
    _lwt_release_edges(oldedge, 1);
    return -1;
  }

  LWDEBUG(1, "lwt_ChangeEdgeGeom: "
             "edge crossing check passed ");

  /*
   * Not in the specs:
   * Check topological isomorphism
   */

  /* Check that the "motion range" doesn't include any node */
  // 1. compute combined bbox of old and new edge
  GBOX mbox; /* motion box */
  lwgeom_add_bbox((LWGEOM*)oldedge->geom); /* just in case */
  lwgeom_add_bbox((LWGEOM*)geom); /* just in case */
  gbox_union(oldedge->geom->bbox, geom->bbox, &mbox);
  // 2. fetch all nodes in the combined box
  LWT_ISO_NODE *nodes;
  uint64_t numnodes;
  nodes = lwt_be_getNodeWithinBox2D(topo, &mbox, &numnodes,
                                          LWT_COL_NODE_ALL, 0);
  LWDEBUGF(1, "lwt_be_getNodeWithinBox2D returned %llu nodes", numnodes);
  if (numnodes == UINT64_MAX)
  {
    _lwt_release_edges(oldedge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  }
  // 3. if any node beside endnodes are found:
  if ( numnodes > ( 1 + isclosed ? 0 : 1 ) )
  {{
    //   3.2. bail out if any node is in one and not the other
    for (i=0; i<numnodes; ++i)
    {
      LWT_ISO_NODE *n = &(nodes[i]);
      if ( n->node_id == oldedge->start_node ) continue;
      if ( n->node_id == oldedge->end_node ) continue;
      const POINT2D *pt = getPoint2d_cp(n->geom->point, 0);
      int ocont = ptarray_contains_point_partial(oldedge->geom->points, pt, isclosed, NULL) == LW_INSIDE;
      int ncont = ptarray_contains_point_partial(geom->points, pt, isclosed, NULL) == LW_INSIDE;
      if (ocont != ncont)
      {
        size_t sz;
        char *wkt = lwgeom_to_wkt(lwpoint_as_lwgeom(n->geom), WKT_ISO, 15, &sz);
        _lwt_release_nodes(nodes, numnodes);
        lwerror("Edge motion collision at %s", wkt);
        lwfree(wkt); /* would not necessarely reach this point */
        return -1;
      }
    }
  }}
  if ( numnodes ) _lwt_release_nodes(nodes, numnodes);

  LWDEBUG(1, "nodes containment check passed");

  /*
   * Check edge adjacency before
   * TODO: can be optimized to gather azimuths of all edge ends once
   */

  edgeend span_pre, epan_pre;
  /* initialize span_pre.myaz and epan_pre.myaz with existing edge */
  int res = _lwt_InitEdgeEndByLine(&span_pre, &epan_pre, oldedge->geom, &p1, &p2);
  if (res)
    return -1; /* lwerror should have been raised */
  _lwt_FindAdjacentEdges( topo, oldedge->start_node, &span_pre,
                                  isclosed ? &epan_pre : NULL, edge_id );
  _lwt_FindAdjacentEdges( topo, oldedge->end_node, &epan_pre,
                                  isclosed ? &span_pre : NULL, edge_id );

  LWDEBUGF(1, "edges adjacent to old edge are %" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID " (first point), %" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID " (last point)",
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);

  /* If the same edge is both on CW and CCW direction on both start
   * and end points we need to verify winding of the left and right
   * rings to verify we didn't twist.
   * See https://trac.osgeo.org/postgis/ticket/5787
   *
   * NOTE: this could probably replace the "isclosed" test.
   *
   * NOTE: if either start or end node had different CW and CCW
   *       edges a twist would be caught in the previous check.
   */
  if ( ! isclosed &&
       oldedge->face_left != oldedge->face_right &&
       span_pre.nextCW == span_pre.nextCCW &&
       epan_pre.nextCW == epan_pre.nextCCW )
  {{
    uint64_t num_signed_edge_ids;
    LWT_ELEMID *signed_edge_ids;
    LWPOLY *shell;

    LWDEBUG(1, "Twist check before");
    signed_edge_ids = lwt_be_getRingEdges(topo, edge_id, &num_signed_edge_ids, 0);
    /* Get winding of left face ring */
    if (!signed_edge_ids)
    {
      //PGTOPO_BE_ERRORF("no ring edges for edge %" LWTFMT_ELEMID, sedge);
      PGTOPO_BE_ERROR();
      return -1;
    }
    LWDEBUGF(1, "getRingEdges returned %llu edges", num_signed_edge_ids);

    shell = _lwt_MakeRingShell(topo, signed_edge_ids, num_signed_edge_ids);
    if ( ! shell ) {
      lwfree( signed_edge_ids );
      /* ring_edges should be NULL */
      lwerror("Could not create ring shell: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    const POINTARRAY *pa = shell->rings[0];
    if ( ! ptarray_is_closed_2d(pa) )
    {
      lwpoly_free(shell);
      lwfree( signed_edge_ids );
      lwerror("Corrupted topology: ring of edge %" LWTFMT_ELEMID
              " is geometrically not-closed", edge_id);
      return -1;
    }

    leftRingIsCCW = ptarray_isccw(pa);
    lwpoly_free(shell);
    lwfree( signed_edge_ids );

    LWDEBUGF(1, "Ring of edge %" LWTFMT_ELEMID " is %sclockwise", edge_id, leftRingIsCCW ? "counter" : "");
  }}


  /* update edge geometry */
  newedge.edge_id = edge_id;
  newedge.geom = geom;
  res = lwt_be_updateEdgesById(topo, &newedge, 1, LWT_COL_EDGE_GEOM);
  if (res == -1)
  {
    _lwt_release_edges(oldedge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if (!res)
  {
    _lwt_release_edges(oldedge, 1);
    lwerror("Unexpected error: %" PRIu64 " edges updated when expecting 1", i);
    return -1;
  }

  /*
   * Check edge adjacency after
   */
  edgeend span_post, epan_post;
  /* initialize epan_post.myaz and epan_post.myaz */
  res = _lwt_InitEdgeEndByLine(&span_post, &epan_post, geom, &p1, &p2);
  if (res)
    return -1; /* lwerror should have been raised */
  _lwt_FindAdjacentEdges( topo, oldedge->start_node, &span_post,
                          isclosed ? &epan_post : NULL, edge_id );
  _lwt_FindAdjacentEdges( topo, oldedge->end_node, &epan_post,
                          isclosed ? &span_post : NULL, edge_id );

  LWDEBUGF(1, "edges adjacent to new edge are %" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID " (first point), %" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID " (last point)",
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);


  /* Bail out if next CW or CCW edge on start node changed */
  if ( span_pre.nextCW != span_post.nextCW ||
       span_pre.nextCCW != span_post.nextCCW )
  {{
    LWT_ELEMID nid = oldedge->start_node;
    _lwt_release_edges(oldedge, 1);
    lwerror("Edge changed disposition around start node %"
            LWTFMT_ELEMID, nid);
    return -1;
  }}

  /* Bail out if next CW or CCW edge on end node changed */
  if ( epan_pre.nextCW != epan_post.nextCW ||
       epan_pre.nextCCW != epan_post.nextCCW )
  {{
    LWT_ELEMID nid = oldedge->end_node;
    _lwt_release_edges(oldedge, 1);
    lwerror("Edge changed disposition around end node %"
            LWTFMT_ELEMID, nid);
    return -1;
  }}

  /* Check winding of left face ring did not change */
  if ( leftRingIsCCW != -1 )
  {{
    uint64_t num_signed_edge_ids;
    LWT_ELEMID *signed_edge_ids;
    LWPOLY *shell;
    int isCCW;

    LWDEBUG(1, "Twist check after");
    signed_edge_ids = lwt_be_getRingEdges(topo, edge_id, &num_signed_edge_ids, 0);
    /* Get winding of left face ring */
    if (!signed_edge_ids)
    {
      //PGTOPO_BE_ERRORF("no ring edges for edge %" LWTFMT_ELEMID, sedge);
      PGTOPO_BE_ERROR();
      return -1;
    }
    LWDEBUGF(1, "getRingEdges returned %llu edges", num_signed_edge_ids);

    shell = _lwt_MakeRingShell(topo, signed_edge_ids, num_signed_edge_ids);
    if ( ! shell ) {
      lwfree( signed_edge_ids );
      /* ring_edges should be NULL */
      lwerror("Could not create ring shell: %s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    const POINTARRAY *pa = shell->rings[0];
    if ( ! ptarray_is_closed_2d(pa) )
    {
      lwpoly_free(shell);
      lwfree( signed_edge_ids );
      lwerror("Corrupted topology: ring of edge %" LWTFMT_ELEMID
              " is geometrically not-closed", edge_id);
      return -1;
    }

    isCCW = ptarray_isccw(pa);
    lwpoly_free(shell);
    lwfree( signed_edge_ids );

    if ( isCCW != leftRingIsCCW )
    {
      _lwt_release_edges(oldedge, 1);
      lwerror("Edge ring changes winding");
      return -1;
    }
  }}

  /*
  -- Update faces MBR of left and right faces
  -- TODO: think about ways to optimize this part, like see if
  --       the old edge geometry participated in the definition
  --       of the current MBR (for shrinking) or the new edge MBR
  --       would be larger than the old face MBR...
  --
  */
  LWGEOM *oldgeom = lwline_as_lwgeom(oldedge->geom);
  LWGEOM *newgeom = lwline_as_lwgeom(geom);
  lwgeom_refresh_bbox(oldgeom); /* Ensure we use a fit mbr, see #5709 -- TODO: fix this at lower level */
  lwgeom_refresh_bbox(newgeom); /* Ensure we use a fit mbr, see #5709 -- TODO: fix this at lower level */
  const GBOX* oldbox = lwgeom_get_bbox(oldgeom);
  const GBOX* newbox = lwgeom_get_bbox(newgeom);
  if ( ! gbox_same(oldbox, newbox) )
  {
    GBOX* updatedBox;
    uint64_t facestoupdate = 0;
    LWT_ISO_FACE faces[2];
    if ( oldedge->face_left > 0 )
    {
      updatedBox = lwt_be_computeFaceMBR(topo, oldedge->face_left);
      if ( ! updatedBox )
      {
        lwerror("Corrupted topology: face %" LWTFMT_ELEMID
          ", left of edge %" LWTFMT_ELEMID ", has no bbox",
          oldedge->face_left, edge_id);
        return -1;
      }
      faces[facestoupdate].face_id = oldedge->face_left;
      /* ownership transferred to faces[] */
      faces[facestoupdate++].mbr = updatedBox;
    }
    if ( oldedge->face_right > 0
         /* no need to update twice the same face.. */
         && oldedge->face_right != oldedge->face_left )
    {
      updatedBox = lwt_be_computeFaceMBR(topo, oldedge->face_right);
      if ( ! updatedBox )
      {
        lwerror("Corrupted topology: face %"
          LWTFMT_ELEMID ", right of edge %" LWTFMT_ELEMID ", has no bbox",
          oldedge->face_right, edge_id);
        return -1;
      }
      faces[facestoupdate].face_id = oldedge->face_right;
      /* ownership transferred to faces[] */
      faces[facestoupdate++].mbr = updatedBox;
    }
    LWDEBUGF(1, "%llu faces to update", facestoupdate);
    if ( facestoupdate )
    {
      uint64_t updatedFaces = lwt_be_updateFacesById(topo, &(faces[0]), facestoupdate);
      if (updatedFaces != facestoupdate)
      {
        while ( facestoupdate-- ) lwfree(faces[facestoupdate].mbr);
        _lwt_release_edges(oldedge, 1);
        if (updatedFaces == UINT64_MAX)
          PGTOPO_BE_ERROR();
        else
          lwerror("Unexpected error: %" PRIu64 " faces updated when expecting 1", updatedFaces);
        return -1;
      }
    }
    while ( facestoupdate-- ) lwfree(faces[facestoupdate].mbr);
  }
  else
  {
    LWDEBUG(1, "BBOX of changed edge did not change");
  }

  LWDEBUG(1, "all done, cleaning up edges");

  _lwt_release_edges(oldedge, 1);
  return 0; /* success */
}

/* Only return CONTAINING_FACE in the node object */
static LWT_ISO_NODE *
_lwt_GetIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID nid)
{
  LWT_ISO_NODE *node;
  uint64_t n = 1;

  node = lwt_be_getNodeById( topo, &nid, &n, LWT_COL_NODE_CONTAINING_FACE );
  if (n == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return 0;
  }
  if ( n < 1 ) {
    lwerror("SQL/MM Spatial exception - non-existent node");
    return 0;
  }
  if ( node->containing_face == -1 )
  {
    lwfree(node);
    lwerror("SQL/MM Spatial exception - not isolated node");
    return 0;
  }

  return node;
}

int
lwt_MoveIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID nid, LWPOINT *pt)
{
  LWT_ISO_NODE *node;
  int ret;
  int newPointFace;

  node = _lwt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  if ( lwt_be_ExistsCoincidentNode(topo, pt) )
  {
    lwfree(node);
    lwerror("SQL/MM Spatial exception - coincident node");
    return -1;
  }

  if ( lwt_be_ExistsEdgeIntersectingPoint(topo, pt) )
  {
    lwfree(node);
    lwerror("SQL/MM Spatial exception - edge crosses node.");
    return -1;
  }

  /* Check that the new point is in the same containing face !
   * See https://trac.osgeo.org/postgis/ticket/3232 */
  newPointFace = lwt_GetFaceContainingPoint(topo, pt);
  if ( newPointFace == -1 ) {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( node->containing_face != newPointFace )
  {
    lwfree(node);
    lwerror("Cannot move isolated node across faces");
    return -1;
  }

  node->node_id = nid;
  node->geom = pt;
  ret = lwt_be_updateNodesById(topo, node, 1,
                               LWT_COL_NODE_GEOM);
  if ( ret == -1 ) {
    lwfree(node);
    PGTOPO_BE_ERROR();
    return -1;
  }

  lwfree(node);
  return 0;
}

int
lwt_RemoveIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID nid)
{
  LWT_ISO_NODE *node;
  int n = 1;

  node = _lwt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  n = lwt_be_deleteNodesById( topo, &nid, n );
  if ( n == -1 )
  {
    lwfree(node);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( n != 1 )
  {
    lwfree(node);
    lwerror("Unexpected error: %d nodes deleted when expecting 1", n);
    return -1;
  }

  if ( ! lwt_be_checkTopoGeomRemIsoNode(topo, nid) )
  {
    lwfree(node);
    lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  lwfree(node);
  return 0; /* success */
}

int
lwt_RemIsoEdge(LWT_TOPOLOGY* topo, LWT_ELEMID id)
{
  LWT_ISO_EDGE deledge;
  LWT_ISO_EDGE *edge;
  LWT_ELEMID nid[2];
  LWT_ISO_NODE upd_node[2];
  LWT_ELEMID containing_face;
  uint64_t n = 1;
  uint64_t i;

  edge = lwt_be_getEdgeById( topo, &id, &n, LWT_COL_EDGE_START_NODE|
                                            LWT_COL_EDGE_END_NODE |
                                            LWT_COL_EDGE_FACE_LEFT |
                                            LWT_COL_EDGE_FACE_RIGHT );
  if ( ! edge )
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( ! n )
  {
    lwerror("SQL/MM Spatial exception - non-existent edge");
    return -1;
  }
  if ( n > 1 )
  {
    lwfree(edge);
    lwerror("Corrupted topology: more than a single edge have id %"
            LWTFMT_ELEMID, id);
    return -1;
  }

  if ( edge[0].face_left != edge[0].face_right )
  {
    lwfree(edge);
    lwerror("SQL/MM Spatial exception - not isolated edge");
    return -1;
  }
  containing_face = edge[0].face_left;

  nid[0] = edge[0].start_node;
  nid[1] = edge[0].end_node;
  lwfree(edge);

  n = 2;
  edge = lwt_be_getEdgeByNode( topo, nid, &n, LWT_COL_EDGE_EDGE_ID );
  if ((n == UINT64_MAX) || (edge == NULL))
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for (i = 0; i < n; ++i)
  {
    if (edge[i].edge_id != id)
    {
      lwfree(edge);
      lwerror("SQL/MM Spatial exception - not isolated edge");
      return -1;
    }
  }
  lwfree(edge);

  deledge.edge_id = id;
  n = lwt_be_deleteEdges( topo, &deledge, LWT_COL_EDGE_EDGE_ID );
  if (n == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( n != 1 )
  {
    lwerror("Unexpected error: %" PRIu64 " edges deleted when expecting 1", n);
    return -1;
  }

  upd_node[0].node_id = nid[0];
  upd_node[0].containing_face = containing_face;
  n = 1;
  if ( nid[1] != nid[0] ) {
    upd_node[1].node_id = nid[1];
    upd_node[1].containing_face = containing_face;
    ++n;
  }
  n = lwt_be_updateNodesById(topo, upd_node, n,
                               LWT_COL_NODE_CONTAINING_FACE);
  if (n == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* Check that the edge can be safely removed
   * See https://trac.osgeo.org/postgis/ticket/3248
   */
  if ( ! lwt_be_checkTopoGeomRemIsoEdge(topo, id) )
  {
    lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return 0; /* success */
}

/* Used by _lwt_RemEdge to update edge face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_lwt_UpdateEdgeFaceRef( LWT_TOPOLOGY *topo, LWT_ELEMID of, LWT_ELEMID nf)
{
  LWT_ISO_EDGE sel_edge, upd_edge;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel_edge.face_left = of;
  upd_edge.face_left = nf;
  ret = lwt_be_updateEdges(topo, &sel_edge, LWT_COL_EDGE_FACE_LEFT,
                                 &upd_edge, LWT_COL_EDGE_FACE_LEFT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  /* Update face_right for all edges still referencing old face */
  sel_edge.face_right = of;
  upd_edge.face_right = nf;
  ret = lwt_be_updateEdges(topo, &sel_edge, LWT_COL_EDGE_FACE_RIGHT,
                                 &upd_edge, LWT_COL_EDGE_FACE_RIGHT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by _lwt_RemEdge to update node face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_lwt_UpdateNodeFaceRef( LWT_TOPOLOGY *topo, LWT_ELEMID of, LWT_ELEMID nf)
{
  LWT_ISO_NODE sel, upd;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel.containing_face = of;
  upd.containing_face = nf;
  ret = lwt_be_updateNodes(topo, &sel, LWT_COL_NODE_CONTAINING_FACE,
                                 &upd, LWT_COL_NODE_CONTAINING_FACE,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by lwt_RemEdgeModFace and lwt_RemEdgeNewFaces
 *
 * Returns -1 on error, identifier of the face that takes up the space
 * previously occupied by the removed edge if modFace is 1, identifier of
 * the created face (0 if none) if modFace is 0.
 */
static LWT_ELEMID
_lwt_RemEdge( LWT_TOPOLOGY* topo, LWT_ELEMID edge_id, int modFace )
{
  uint64_t i, nedges, nfaces, fields;
  LWT_ISO_EDGE *edge = NULL;
  LWT_ISO_EDGE *upd_edge = NULL;
  LWT_ISO_EDGE upd_edge_left[2];
  int nedge_left = 0;
  LWT_ISO_EDGE upd_edge_right[2];
  int nedge_right = 0;
  LWT_ISO_NODE upd_node[2];
  int nnode = 0;
  LWT_ISO_FACE *faces = NULL;
  LWT_ISO_FACE newface;
  LWT_ELEMID node_ids[2];
  LWT_ELEMID face_ids[2];
  int fnode_edges = 0; /* number of edges on the first node (excluded
                        * the one being removed ) */
  int lnode_edges = 0; /* number of edges on the last node (excluded
                        * the one being removed ) */


  newface.face_id = 0;

  i = 1;
  edge = lwt_be_getEdgeById(topo, &edge_id, &i, LWT_COL_EDGE_ALL);
  if (!edge)
  {
    LWDEBUGF(1, "lwt_be_getEdgeById returned NULL and set i=%llu", i);
    if (i == UINT64_MAX)
    {
      PGTOPO_BE_ERROR();
      return -1;
    }
    else if (i == 0)
    {
      lwerror("SQL/MM Spatial exception - non-existent edge %" LWTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      lwerror(
          "Backend coding error: getEdgeById callback returned NULL "
          "but numelements output parameter has value %" PRIu64 " "
          "(expected 0 or 1)",
          i);
      return -1;
    }
  }

  if ( ! lwt_be_checkTopoGeomRemEdge(topo, edge_id,
                                     edge->face_left, edge->face_right) )
  {
    lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  LWDEBUG(1, "Updating next_{right,left}_face of ring edges...");

  /* Update edge linking */

  nedges = 0;
  node_ids[nedges++] = edge->start_node;
  if ( edge->end_node != edge->start_node )
  {
    node_ids[nedges++] = edge->end_node;
  }
  fields = LWT_COL_EDGE_EDGE_ID | LWT_COL_EDGE_START_NODE |
           LWT_COL_EDGE_END_NODE | LWT_COL_EDGE_NEXT_LEFT |
           LWT_COL_EDGE_NEXT_RIGHT;
  upd_edge = lwt_be_getEdgeByNode( topo, &(node_ids[0]), &nedges, fields );
  if (nedges == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  nedge_left = nedge_right = 0;
  for ( i=0; i<nedges; ++i )
  {
    LWT_ISO_EDGE *e = &(upd_edge[i]);
    if ( e->edge_id == edge_id ) continue;
    if ( e->start_node == edge->start_node || e->end_node == edge->start_node )
    {
      ++fnode_edges;
    }
    if ( e->start_node == edge->end_node || e->end_node == edge->end_node )
    {
      ++lnode_edges;
    }
    if ( e->next_left == -edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_left == edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }

    if ( e->next_right == -edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_right == edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }
  }

  if ( nedge_left )
  {
    LWDEBUGF(1, "updating %d 'next_left' edges", nedge_left);
    /* update edges in upd_edge_left set next_left */
    int result = lwt_be_updateEdgesById(topo, &(upd_edge_left[0]), nedge_left, LWT_COL_EDGE_NEXT_LEFT);
    if (result == -1)
    {
      _lwt_release_edges(edge, 1);
      lwfree(upd_edge);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }
  if ( nedge_right )
  {
    LWDEBUGF(1, "updating %d 'next_right' edges", nedge_right);
    /* update edges in upd_edge_right set next_right */
    int result = lwt_be_updateEdgesById(topo, &(upd_edge_right[0]), nedge_right, LWT_COL_EDGE_NEXT_RIGHT);
    if (result == -1)
    {
      _lwt_release_edges(edge, 1);
      lwfree(upd_edge);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }
  LWDEBUGF(1, "releasing %llu updateable edges in %p", nedges, upd_edge);
  lwfree(upd_edge);

  /* Id of face that will take up all the space previously
   * taken by left and right faces of the edge */
  LWT_ELEMID floodface;

  /* Find floodface, and update its mbr if != 0 */
  if ( edge->face_left == edge->face_right )
  {
    floodface = edge->face_right;
  }
  else
  {
    /* Two faces healed */
    if ( edge->face_left == 0 || edge->face_right == 0 )
    {
      floodface = 0;
      LWDEBUG(1, "floodface is universe");
    }
    else
    {
      /* we choose right face as the face that will remain
       * to be symmetric with ST_AddEdgeModFace */
      floodface = edge->face_right;
      LWDEBUGF(1, "floodface is %" LWTFMT_ELEMID, floodface);
      /* update mbr of floodface as union of mbr of both faces */
      face_ids[0] = edge->face_left;
      face_ids[1] = edge->face_right;
      nfaces = 2;
      fields = LWT_COL_FACE_ALL;
      faces = lwt_be_getFaceById(topo, face_ids, &nfaces, fields);
      if (nfaces == UINT64_MAX)
      {
        PGTOPO_BE_ERROR();
        return -1;
      }
      GBOX *box1=NULL;
      GBOX *box2=NULL;
      for ( i=0; i<nfaces; ++i )
      {
        if ( faces[i].face_id == edge->face_left )
        {
          if ( ! box1 ) box1 = faces[i].mbr;
          else
          {
            i = edge->face_left;
            _lwt_release_edges(edge, 1);
            _lwt_release_faces(faces, nfaces);
            lwerror("corrupted topology: more than 1 face have face_id=%"
                    LWTFMT_ELEMID, i);
            return -1;
          }
        }
        else if ( faces[i].face_id == edge->face_right )
        {
          if ( ! box2 ) box2 = faces[i].mbr;
          else
          {
            i = edge->face_right;
            _lwt_release_edges(edge, 1);
            _lwt_release_faces(faces, nfaces);
            lwerror("corrupted topology: more than 1 face have face_id=%"
                    LWTFMT_ELEMID, i);
            return -1;
          }
        }
        else
        {
          i = faces[i].face_id;
          _lwt_release_edges(edge, 1);
          _lwt_release_faces(faces, nfaces);
          lwerror("Backend coding error: getFaceById returned face "
                  "with non-requested id %" LWTFMT_ELEMID, i);
          return -1;
        }
      }
      if ( ! box1 ) {
        i = edge->face_left;
        _lwt_release_edges(edge, 1);
        if ( nfaces ) _lwt_release_faces(faces, nfaces);
        lwerror("corrupted topology: no face have face_id=%"
                LWTFMT_ELEMID " (left face for edge %"
                LWTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      if ( ! box2 ) {
        i = edge->face_right;
        _lwt_release_edges(edge, 1);
        if ( nfaces ) _lwt_release_faces(faces, nfaces);
        lwerror("corrupted topology: no face have face_id=%"
                LWTFMT_ELEMID " (right face for edge %"
                LWTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      gbox_merge(box2, box1); /* box1 is now the union of the two */
      newface.mbr = box1;
      if ( modFace )
      {
        newface.face_id = floodface;
  int result = lwt_be_updateFacesById(topo, &newface, 1);
  _lwt_release_faces(faces, 2);
  if (result == -1)
  {
    _lwt_release_edges(edge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if (result != 1)
  {
    _lwt_release_edges(edge, 1);
    lwerror("Unexpected error: %" PRIu64 " faces updated when expecting 1", i);
    return -1;
  }
      }
      else
      {
        /* New face replaces the old two faces */
        newface.face_id = -1;
  int result = lwt_be_insertFaces(topo, &newface, 1);
  _lwt_release_faces(faces, 2);
  if (result == -1)
  {
    _lwt_release_edges(edge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if (result != 1)
  {
          _lwt_release_edges(edge, 1);
    lwerror("Unexpected error: %d faces inserted when expecting 1", result);
    return -1;
        }
        floodface = newface.face_id;
      }
    }

    /* Update face references for edges and nodes still referencing
     * the removed face(s) */

    if ( edge->face_left != floodface )
    {
      if ( -1 == _lwt_UpdateEdgeFaceRef(topo, edge->face_left, floodface) )
      {
        _lwt_release_edges(edge, 1);
        PGTOPO_BE_ERROR();
        return -1;
      }
      if ( -1 == _lwt_UpdateNodeFaceRef(topo, edge->face_left, floodface) )
      {
        _lwt_release_edges(edge, 1);
        PGTOPO_BE_ERROR();
        return -1;
      }
    }

    if ( edge->face_right != floodface )
    {
      if ( -1 == _lwt_UpdateEdgeFaceRef(topo, edge->face_right, floodface) )
      {
        _lwt_release_edges(edge, 1);
        PGTOPO_BE_ERROR();
        return -1;
      }
      if ( -1 == _lwt_UpdateNodeFaceRef(topo, edge->face_right, floodface) )
      {
        _lwt_release_edges(edge, 1);
        PGTOPO_BE_ERROR();
        return -1;
      }
    }

    /* Update topogeoms on heal */
    if ( ! lwt_be_updateTopoGeomFaceHeal(topo,
                                  edge->face_right, edge->face_left,
                                  floodface) )
    {
      _lwt_release_edges(edge, 1);
      lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  } /* two faces healed */

  /* Delete the edge */
  int result = lwt_be_deleteEdges(topo, edge, LWT_COL_EDGE_EDGE_ID);
  if (result == -1)
  {
    _lwt_release_edges(edge, 1);
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* If any of the edge nodes remained isolated, set
   * containing_face = floodface
   */
  if ( ! fnode_edges )
  {
    upd_node[nnode].node_id = edge->start_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( edge->end_node != edge->start_node && ! lnode_edges )
  {
    upd_node[nnode].node_id = edge->end_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( nnode )
  {
    int result = lwt_be_updateNodesById(topo, upd_node, nnode, LWT_COL_NODE_CONTAINING_FACE);
    if (result == -1)
    {
      _lwt_release_edges(edge, 1);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }

  if ( edge->face_left != edge->face_right )
  /* or there'd be no face to remove */
  {
    LWT_ELEMID ids[2];
    int nids = 0;
    if ( edge->face_right != floodface )
      ids[nids++] = edge->face_right;
    if ( edge->face_left != floodface )
      ids[nids++] = edge->face_left;
    int result = lwt_be_deleteFacesById(topo, ids, nids);
    if (result == -1)
    {
      _lwt_release_edges(edge, 1);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }

  _lwt_release_edges(edge, 1);
  return modFace ? floodface : newface.face_id;
}

LWT_ELEMID
lwt_RemEdgeModFace( LWT_TOPOLOGY* topo, LWT_ELEMID edge_id )
{
  return _lwt_RemEdge( topo, edge_id, 1 );
}

LWT_ELEMID
lwt_RemEdgeNewFace( LWT_TOPOLOGY* topo, LWT_ELEMID edge_id )
{
  return _lwt_RemEdge( topo, edge_id, 0 );
}

static LWT_ELEMID
_lwt_HealEdges( LWT_TOPOLOGY* topo, LWT_ELEMID eid1, LWT_ELEMID eid2,
                int modEdge )
{
  LWT_ELEMID ids[2];
  LWT_ELEMID commonnode = -1;
  int caseno = 0;
  LWT_ISO_EDGE *node_edges;
  uint64_t num_node_edges;
  LWT_ISO_EDGE *edges;
  LWT_ISO_EDGE *e1 = NULL;
  LWT_ISO_EDGE *e2 = NULL;
  LWT_ISO_EDGE newedge, updedge, seledge;
  uint64_t nedges, i;
  int e1freenode;
  int e2sign, e2freenode;
  POINTARRAY *pa;
  char buf[256];
  char *ptr;
  size_t bufleft = 256;

  ptr = buf;

  /* NOT IN THE SPECS: see if the same edge is given twice.. */
  if ( eid1 == eid2 )
  {
    lwerror("Cannot heal edge %" LWTFMT_ELEMID
            " with itself, try with another", eid1);
    return -1;
  }
  ids[0] = eid1;
  ids[1] = eid2;
  nedges = 2;
  edges = lwt_be_getEdgeById(topo, ids, &nedges, LWT_COL_EDGE_ALL);
  if ((nedges == UINT64_MAX) || (edges == NULL))
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for ( i=0; i<nedges; ++i )
  {
    if ( edges[i].edge_id == eid1 ) {
      if ( e1 ) {
        _lwt_release_edges(edges, nedges);
        lwerror("Corrupted topology: multiple edges have id %"
                LWTFMT_ELEMID, eid1);
        return -1;
      }
      e1 = &(edges[i]);
    }
    else if ( edges[i].edge_id == eid2 ) {
      if ( e2 ) {
        _lwt_release_edges(edges, nedges);
        lwerror("Corrupted topology: multiple edges have id %"
                LWTFMT_ELEMID, eid2);
        return -1;
      }
      e2 = &(edges[i]);
    }
  }
  if ( ! e1 )
  {
    _lwt_release_edges(edges, nedges);
    lwerror(
        "SQL/MM Spatial exception - non-existent edge %" LWTFMT_ELEMID,
        eid1);
    return -1;
  }
  if ( ! e2 )
  {
    _lwt_release_edges(edges, nedges);
    lwerror(
        "SQL/MM Spatial exception - non-existent edge %" LWTFMT_ELEMID,
        eid2);
    return -1;
  }

  /* NOT IN THE SPECS: See if any of the two edges are closed. */
  if ( e1->start_node == e1->end_node )
  {
    _lwt_release_edges(edges, nedges);
    lwerror("Edge %" LWTFMT_ELEMID " is closed, cannot heal to edge %"
            LWTFMT_ELEMID, eid1, eid2);
    return -1;
  }
  if ( e2->start_node == e2->end_node )
  {
    _lwt_release_edges(edges, nedges);
    lwerror("Edge %" LWTFMT_ELEMID " is closed, cannot heal to edge %"
            LWTFMT_ELEMID, eid2, eid1);
    return -1;
  }

  /* Find common node */

  if ( e1->end_node == e2->start_node )
  {
    commonnode = e1->end_node;
    caseno = 1;
  }
  else if ( e1->end_node == e2->end_node )
  {
    commonnode = e1->end_node;
    caseno = 2;
  }
  /* Check if any other edge is connected to the common node, if found */
  if ( commonnode != -1 )
  {
    num_node_edges = 1;
    node_edges = lwt_be_getEdgeByNode( topo, &commonnode,
                                       &num_node_edges, LWT_COL_EDGE_EDGE_ID );
    if (num_node_edges == UINT64_MAX)
    {
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }
    for (i=0; i<num_node_edges; ++i)
    {
      int r;
      if ( node_edges[i].edge_id == eid1 ) continue;
      if ( node_edges[i].edge_id == eid2 ) continue;
      commonnode = -1;
      /* append to string, for error message */
      if ( bufleft > 0 ) {
        r = snprintf(ptr, bufleft, "%s%" LWTFMT_ELEMID,
                     ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
        if ( r >= (int) bufleft )
        {
          bufleft = 0;
          buf[252] = '.';
          buf[253] = '.';
          buf[254] = '.';
          buf[255] = '\0';
        }
        else
        {
          bufleft -= r;
          ptr += r;
        }
      }
    }
    lwfree(node_edges);
  }

  if ( commonnode == -1 )
  {
    if ( e1->start_node == e2->start_node )
    {
      commonnode = e1->start_node;
      caseno = 3;
    }
    else if ( e1->start_node == e2->end_node )
    {
      commonnode = e1->start_node;
      caseno = 4;
    }
    /* Check if any other edge is connected to the common node, if found */
    if ( commonnode != -1 )
    {
      num_node_edges = 1;
      node_edges = lwt_be_getEdgeByNode( topo, &commonnode,
                                         &num_node_edges, LWT_COL_EDGE_EDGE_ID );
      if (num_node_edges == UINT64_MAX)
      {
        _lwt_release_edges(edges, nedges);
        PGTOPO_BE_ERROR();
        return -1;
      }
      for (i=0; i<num_node_edges; ++i)
      {
        int r;
        if ( node_edges[i].edge_id == eid1 ) continue;
        if ( node_edges[i].edge_id == eid2 ) continue;
        commonnode = -1;
        /* append to string, for error message */
        if ( bufleft > 0 ) {
          r = snprintf(ptr, bufleft, "%s%" LWTFMT_ELEMID,
                       ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
          if ( r >= (int) bufleft )
          {
            bufleft = 0;
            buf[252] = '.';
            buf[253] = '.';
            buf[254] = '.';
            buf[255] = '\0';
          }
          else
          {
            bufleft -= r;
            ptr += r;
          }
        }
      }
      if ( num_node_edges ) lwfree(node_edges);
    }
  }

  if ( commonnode == -1 )
  {
    _lwt_release_edges(edges, nedges);
    if ( ptr != buf )
    {
      lwerror("SQL/MM Spatial exception - other edges connected (%s)",
              buf);
    }
    else
    {
      lwerror("SQL/MM Spatial exception - non-connected edges");
    }
    return -1;
  }

  if ( ! lwt_be_checkTopoGeomRemNode(topo, commonnode,
                                     eid1, eid2 ) )
  {
    _lwt_release_edges(edges, nedges);
    lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Construct the geometry of the new edge */
  switch (caseno)
  {
    case 1: /* e1.end = e2.start */
      pa = ptarray_clone_deep(e1->geom->points);
      //pa = ptarray_merge(pa, e2->geom->points);
      ptarray_append_ptarray(pa, e2->geom->points, 0);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->end_node;
      newedge.next_left = e2->next_left;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = -1;
      e2sign = 1;
      break;
    case 2: /* e1.end = e2.end */
    {
      POINTARRAY *pa2;
      pa2 = ptarray_clone_deep(e2->geom->points);
      ptarray_reverse_in_place(pa2);
      pa = ptarray_clone_deep(e1->geom->points);
      //pa = ptarray_merge(e1->geom->points, pa);
      ptarray_append_ptarray(pa, pa2, 0);
      ptarray_free(pa2);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->start_node;
      newedge.next_left = e2->next_right;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = 1;
      e2sign = -1;
      break;
    }
    case 3: /* e1.start = e2.start */
      pa = ptarray_clone_deep(e2->geom->points);
      ptarray_reverse_in_place(pa);
      //pa = ptarray_merge(pa, e1->geom->points);
      ptarray_append_ptarray(pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->end_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_left;
      e1freenode = -1;
      e2freenode = -1;
      e2sign = -1;
      break;
    case 4: /* e1.start = e2.end */
      pa = ptarray_clone_deep(e2->geom->points);
      //pa = ptarray_merge(pa, e1->geom->points);
      ptarray_append_ptarray(pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->start_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_right;
      e1freenode = -1;
      e2freenode = 1;
      e2sign = 1;
      break;
    default:
      pa = NULL;
      e1freenode = 0;
      e2freenode = 0;
      e2sign = 0;
      _lwt_release_edges(edges, nedges);
      lwerror("Coding error: caseno=%d should never happen", caseno);
      return -1;
      break;
  }
  newedge.geom = lwline_construct(topo->srid, NULL, pa);

  if ( modEdge )
  {
    /* Update data of the first edge */
    newedge.edge_id = eid1;
    int result = lwt_be_updateEdgesById(topo,
          &newedge,
          1,
          LWT_COL_EDGE_NEXT_LEFT | LWT_COL_EDGE_NEXT_RIGHT | LWT_COL_EDGE_START_NODE |
              LWT_COL_EDGE_END_NODE | LWT_COL_EDGE_GEOM);
    if (result == -1)
    {
      lwline_free(newedge.geom);
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }
    else if (result != 1)
    {
      lwline_free(newedge.geom);
      if ( edges ) _lwt_release_edges(edges, nedges);
      lwerror("Unexpected error: %" PRIu64 " edges updated when expecting 1", i);
      return -1;
    }
  }
  else
  {
    /* Add new edge */
    newedge.edge_id = -1;
    newedge.face_left = e1->face_left;
    newedge.face_right = e1->face_right;
    int result = lwt_be_insertEdges(topo, &newedge, 1);
    if (result == -1)
    {
      lwline_free(newedge.geom);
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }
    else if (result == 0)
    {
      lwline_free(newedge.geom);
      _lwt_release_edges(edges, nedges);
      lwerror("Insertion of split edge failed (no reason)");
      return -1;
    }
  }
  lwline_free(newedge.geom);

  /*
  -- Update next_left_edge/next_right_edge for
  -- any edge having them still pointing at the edge being removed
  -- (eid2 only when modEdge, or both otherwise)
  --
  -- NOTE:
  -- e#freenode is 1 when edge# end node was the common node
  -- and -1 otherwise. This gives the sign of possibly found references
  -- to its "free" (non connected to other edge) endnode.
  -- e2sign is -1 if edge1 direction is opposite to edge2 direction,
  -- or 1 otherwise.
  --
  */

  /* update edges connected to e2's boundary from their end node */
  seledge.next_left = e2freenode * eid2;
  updedge.next_left = e2freenode * newedge.edge_id * e2sign;
  int result = lwt_be_updateEdges(topo, &seledge, LWT_COL_EDGE_NEXT_LEFT, &updedge, LWT_COL_EDGE_NEXT_LEFT, NULL, 0);
  if (result == -1)
  {
    _lwt_release_edges(edges, nedges);
    PGTOPO_BE_ERROR();
    return -1;
  }

  /* update edges connected to e2's boundary from their start node */
  seledge.next_right = e2freenode * eid2;
  updedge.next_right = e2freenode * newedge.edge_id * e2sign;
  result = lwt_be_updateEdges(topo, &seledge, LWT_COL_EDGE_NEXT_RIGHT, &updedge, LWT_COL_EDGE_NEXT_RIGHT, NULL, 0);
  if (result == -1)
  {
    _lwt_release_edges(edges, nedges);
    PGTOPO_BE_ERROR();
    return -1;
  }

  if ( ! modEdge )
  {
    /* update edges connected to e1's boundary from their end node */
    seledge.next_left = e1freenode * eid1;
    updedge.next_left = e1freenode * newedge.edge_id;
    result = lwt_be_updateEdges(topo, &seledge, LWT_COL_EDGE_NEXT_LEFT, &updedge, LWT_COL_EDGE_NEXT_LEFT, NULL, 0);
    if (result == -1)
    {
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }

    /* update edges connected to e1's boundary from their start node */
    seledge.next_right = e1freenode * eid1;
    updedge.next_right = e1freenode * newedge.edge_id;
    result = lwt_be_updateEdges(topo, &seledge, LWT_COL_EDGE_NEXT_RIGHT, &updedge, LWT_COL_EDGE_NEXT_RIGHT, NULL, 0);
    if (result == -1)
    {
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }

  /* delete the edges (only second on modEdge or both) */
  result = lwt_be_deleteEdges(topo, e2, LWT_COL_EDGE_EDGE_ID);
  if (result == -1)
  {
    _lwt_release_edges(edges, nedges);
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( ! modEdge ) {
    result = lwt_be_deleteEdges(topo, e1, LWT_COL_EDGE_EDGE_ID);
    if (result == -1)
    {
      _lwt_release_edges(edges, nedges);
      PGTOPO_BE_ERROR();
      return -1;
    }
  }

  _lwt_release_edges(edges, nedges);

  /* delete the common node */
  result = lwt_be_deleteNodesById( topo, &commonnode, 1 );
  if (result == -1)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }

  /*
  --
  -- NOT IN THE SPECS:
  -- Drop composition rows involving second
  -- edge, as the first edge took its space,
  -- and all affected TopoGeom have been previously checked
  -- for being composed by both edges.
  */
  if ( ! lwt_be_updateTopoGeomEdgeHeal(topo,
                                eid1, eid2, newedge.edge_id) )
  {
    lwerror("%s", lwt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return modEdge ? commonnode : newedge.edge_id;
}

LWT_ELEMID
lwt_ModEdgeHeal( LWT_TOPOLOGY* topo, LWT_ELEMID e1, LWT_ELEMID e2 )
{
  return _lwt_HealEdges( topo, e1, e2, 1 );
}

LWT_ELEMID
lwt_NewEdgeHeal( LWT_TOPOLOGY* topo, LWT_ELEMID e1, LWT_ELEMID e2 )
{
  return _lwt_HealEdges( topo, e1, e2, 0 );
}

LWT_ELEMID
lwt_GetNodeByPoint(LWT_TOPOLOGY *topo, LWPOINT *pt, double tol)
{
  LWT_ISO_NODE *elem;
  uint64_t num;
  int flds = LWT_COL_NODE_NODE_ID|LWT_COL_NODE_GEOM; /* geom not needed */
  LWT_ELEMID id = 0;
  POINT2D qp; /* query point */

  if ( ! getPoint2d_p(pt->point, 0, &qp) )
  {
    lwerror("Empty query point");
    return -1;
  }
  elem = lwt_be_getNodeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  else if ( num )
  {
    if ( num > 1 )
    {
      _lwt_release_nodes(elem, num);
      lwerror("Two or more nodes found");
      return -1;
    }
    id = elem[0].node_id;
    _lwt_release_nodes(elem, num);
  }

  return id;
}

LWT_ELEMID
lwt_GetEdgeByPoint(LWT_TOPOLOGY *topo, LWPOINT *pt, double tol)
{
  LWT_ISO_EDGE *elem;
  uint64_t num, i;
  int flds = LWT_COL_EDGE_EDGE_ID|LWT_COL_EDGE_GEOM; /* GEOM is not needed */
  LWT_ELEMID id = 0;
  LWGEOM *qp = lwpoint_as_lwgeom(pt); /* query point */

  if ( lwgeom_is_empty(qp) )
  {
    lwerror("Empty query point");
    return -1;
  }
  elem = lwt_be_getEdgeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for (i=0; i<num;++i)
  {
    LWT_ISO_EDGE *e = &(elem[i]);
#if 0
    LWGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      _lwt_release_edges(elem, num);
      lwnotice("Corrupted topology: edge %" LWTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* Should we check for intersection not being on an endpoint
     * as documented ? */
    geom = lwline_as_lwgeom(e->geom);
    dist = lwgeom_mindistance2d_tolerance(geom, qp, tol);
    if ( dist > tol ) continue;
#endif

    if ( id )
    {
      _lwt_release_edges(elem, num);
      lwerror("Two or more edges found");
      return -1;
    }
    else id = e->edge_id;
  }

  if ( num ) _lwt_release_edges(elem, num);

  return id;
}

LWT_ELEMID
lwt_GetFaceByPoint(LWT_TOPOLOGY *topo, const LWPOINT *pt, double tol)
{
  LWT_ELEMID id = 0;
  LWT_ISO_EDGE *elem;
  uint64_t num, i;
  int flds = LWT_COL_EDGE_EDGE_ID |
             LWT_COL_EDGE_GEOM |
             LWT_COL_EDGE_FACE_LEFT |
             LWT_COL_EDGE_FACE_RIGHT;
  LWGEOM *qp = lwpoint_as_lwgeom(pt);

  id = lwt_GetFaceContainingPoint(topo, pt);
  if ( id == -1 ) {
    PGTOPO_BE_ERROR();
    return -1;
  }

  if ( id > 0 )
  {
    return id;
  }

  if ( tol == 0 )
  {
    return id;
  }

  LWDEBUG(1, "No face properly contains query point,"
             " looking for edges");

  /* Not in a face, may be in universe or on edge, let's check
   * for distance */
  /* NOTE: we never pass a tolerance of 0 to avoid ever using
   * ST_Within, which doesn't include endpoints matches */
  elem = lwt_be_getEdgeWithinDistance2D(topo, pt, tol?tol:1e-5, &num, flds, 0);
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  for (i=0; i<num; ++i)
  {
    LWT_ISO_EDGE *e = &(elem[i]);
    LWT_ELEMID eface = 0;
    LWGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      _lwt_release_edges(elem, num);
      lwnotice("Corrupted topology: edge %" LWTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* don't consider dangling edges */
    if ( e->face_left == e->face_right )
    {
      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID
                  " is dangling, won't consider it", e->edge_id);
      continue;
    }

    geom = lwline_as_lwgeom(e->geom);
    dist = lwgeom_mindistance2d_tolerance(geom, qp, tol);

    LWDEBUGF(1, "Distance from edge %" LWTFMT_ELEMID
                " is %g (tol=%g)", e->edge_id, dist, tol);

    /* we won't consider edges too far */
    if ( dist > tol ) continue;
    if ( e->face_left == 0 ) {
      eface = e->face_right;
    }
    else if ( e->face_right == 0 ) {
      eface = e->face_left;
    }
    else {
      _lwt_release_edges(elem, num);
      lwerror("Two or more faces found");
      return -1;
    }

    if ( id && id != eface )
    {
      _lwt_release_edges(elem, num);
      lwerror("Two or more faces found"
#if 0 /* debugging */
              " (%" LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID ")", id, eface
#endif
             );
      return -1;
    }
    else id = eface;
  }
  if ( num ) _lwt_release_edges(elem, num);

  return id;
}

/* Return the smallest delta that can perturb
 * the given value */
static inline double
_lwt_minToleranceDouble( double d )
{
  double ret = 3.6 * pow(10,  - ( 15 - log10(d?d:1.0) ) );
  return ret;
}

/* Return the smallest delta that can perturb
 * the given point
static inline double
_lwt_minTolerancePoint2d( const POINT2D* p )
{
  double max = FP_ABS(p->x);
  if ( max < FP_ABS(p->y) ) max = FP_ABS(p->y);
  return _lwt_minToleranceDouble(max);
}
*/

/* Return the smallest delta that can perturb
 * the maximum absolute value of a geometry ordinate
 */
static double
_lwt_minTolerance( LWGEOM *g )
{
  const GBOX* gbox;
  double max;
  double ret;

  gbox = lwgeom_get_bbox(g);
  if ( ! gbox ) return 0; /* empty */
  max = FP_ABS(gbox->xmin);
  if ( max < FP_ABS(gbox->xmax) ) max = FP_ABS(gbox->xmax);
  if ( max < FP_ABS(gbox->ymin) ) max = FP_ABS(gbox->ymin);
  if ( max < FP_ABS(gbox->ymax) ) max = FP_ABS(gbox->ymax);

  ret = _lwt_minToleranceDouble(max);

  return ret;
}

#define _LWT_MINTOLERANCE( topo, geom ) ( \
  topo->precision ?  topo->precision : _lwt_minTolerance(geom) )

typedef struct scored_pointer_t {
  void *ptr;
  double score;
} scored_pointer;

static int
compare_scored_pointer(const void *si1, const void *si2)
{
  double a = ((scored_pointer *)si1)->score;
  double b = ((scored_pointer *)si2)->score;
  if ( a < b )
    return -1;
  else if ( a > b )
    return 1;
  else
    return 0;
}

/* Return identifier of an equal edge, 0 if none or -1 on error
 * (and lwerror gets called on error)
 *
 * If an equal edge is found, specify, in "forward" variable whether
 * the edge is also equal direction-wise
 *
 */
static LWT_ELEMID
_lwt_GetEqualEdge( LWT_TOPOLOGY *topo, LWLINE *edge, int *forward )
{
  LWT_ELEMID id;
  LWT_ISO_EDGE *edges;
  uint64_t num, i;
  const GBOX *qbox = lwgeom_get_bbox( lwline_as_lwgeom(edge) );
  GEOSGeometry *edgeg;
  const int flds = LWT_COL_EDGE_EDGE_ID|LWT_COL_EDGE_GEOM;

  edges = lwt_be_getEdgeWithinBox2D( topo, qbox, &num, flds, 0 );
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( num )
  {
    initGEOS(lwnotice, lwgeom_geos_error);

    edgeg = LWGEOM2GEOS( lwline_as_lwgeom(edge), 0 );
    if ( ! edgeg )
    {
      _lwt_release_edges(edges, num);
      lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
      return -1;
    }
    for (i=0; i<num; ++i)
    {
      LWT_ISO_EDGE *e = &(edges[i]);
      LWGEOM *g = lwline_as_lwgeom(e->geom);
      GEOSGeometry *gg;
      int equals;
      gg = LWGEOM2GEOS( g, 0 );
      if ( ! gg )
      {
        GEOSGeom_destroy(edgeg);
        _lwt_release_edges(edges, num);
        lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
        return -1;
      }
      equals = GEOSEquals(gg, edgeg);
      GEOSGeom_destroy(gg);
      if ( equals == 2 )
      {
        GEOSGeom_destroy(edgeg);
        _lwt_release_edges(edges, num);
        lwerror("GEOSEquals exception: %s", lwgeom_geos_errmsg);
        return -1;
      }
      if ( equals )
      {
        id = e->edge_id;
        /* Check if direction also matches */
        if ( forward )
        {
          /* If input line is closed, we use winding order */
          if ( lwline_is_closed(edge) )
          {
            if ( ptarray_isccw(edge->points) == ptarray_isccw(e->geom->points) )
            {
              *forward = 1;
            }
            else
            {
              *forward = 0;
            }
          }
          else
          {
            /* Input line is not closed, checking fist point is enough */
            if (
              memcmp(
                  getPoint_internal(edge->points, 0),
                  getPoint_internal(e->geom->points, 0),
                  sizeof(POINT2D)
              ) == 0
            )
            {
              *forward = 1;
            }
            else
            {
              *forward = 0;
            }
          }
        }
        GEOSGeom_destroy(edgeg);
        _lwt_release_edges(edges, num);
        return id;
      }
    }
    GEOSGeom_destroy(edgeg);
    _lwt_release_edges(edges, num);
  }

  return 0;
}


/**
 * Check the motion of a snapped edge, invoke lwerror if the movement
 * hits any other edge or node
 *
 * @param topo the Topology we are working on
 * @param splitC the result of the edge being split
 * @param edge the edge before the split/snap
 * @param existingEdge an edge on which one component of the split edge collapsed, or null if no collapse happened.
 * @param splitNodeEdges all edges attached to the split node
 *
 * TODO: check that newSplitEdgeLine retains its position in the edge end star (see ticket #5786)
 * TODO: check that the motion range does not contain any node
 *
 */
static int
_lwt_SnapEdge_checkMotion( LWT_TOPOLOGY* topo, const LWCOLLECTION *splitC, const LWT_ISO_EDGE *edge, LWT_ISO_EDGE *existingEdge, const LWT_NODE_EDGES *splitNodeEdges )
{
  // build the motion range shape: splitC->geoms[0] + splitC->geoms[1] - edge->geom
  POINTARRAY *motionRange = ptarray_clone_deep(lwgeom_as_lwline(splitC->geoms[0])->points);
  ptarray_append_ptarray(motionRange, lwgeom_as_lwline(splitC->geoms[1])->points, 0);
  POINTARRAY *reverseNewLine = ptarray_clone_deep(edge->geom->points);
  ptarray_reverse_in_place(reverseNewLine);
  ptarray_append_ptarray(motionRange, reverseNewLine, 0);
  ptarray_free(reverseNewLine);

  // motionBounds takes ownership of motionRange
  LWLINE *motionBounds = lwline_construct(topo->srid, NULL, motionRange);

  // motionPolyBare takes ownership of motionBounds
  LWGEOM *motionPolyBare = (LWGEOM *)lwpoly_from_lwlines(motionBounds, 0, NULL);
  LWGEOM *motionPoly = lwgeom_make_valid(motionPolyBare);
  lwgeom_free(motionPolyBare);

  LWDEBUGG(1, motionPoly, "Motion range");

  // check the Motion range doesn't cover any of
  // the edges incident to the split node other
  // than the existing edge
  GEOSGeometry *motionPolyG = NULL;
  for ( uint64_t t=0; t<splitNodeEdges->numEdges; t++ )
  {
    LWT_ISO_EDGE *e = &(splitNodeEdges->edges[t]);
    GEOSGeometry *eg;
    if ( e == existingEdge ) continue;
    if ( e == edge ) continue;
    if ( ! motionPolyG ) {
      motionPolyG = LWGEOM2GEOS( motionPoly, 0 );
      if ( ! motionPolyG )
      {
        lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
        return -1;
      }
    }
    eg = LWGEOM2GEOS( lwline_as_lwgeom(e->geom), 0 );
    if ( ! eg )
    {
      GEOSGeom_destroy(motionPolyG);
      lwgeom_free(motionPoly);
      lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
      return -1;
    }

    char *relate = GEOSRelate( motionPolyG, eg );
    GEOSGeom_destroy(eg);
    if ( ! relate )
    {
      GEOSGeom_destroy(motionPolyG);
      lwgeom_free(motionPoly);
      lwerror("Relate error: %s", lwgeom_geos_errmsg);
      return -1;
    }

    int match = GEOSRelatePatternMatch(relate, "FF*F*****");
    GEOSFree( relate );
    if (match == 2)
    {
      GEOSGeom_destroy(motionPolyG);
      lwgeom_free(motionPoly);
      lwerror("RelateMatch error: %s", lwgeom_geos_errmsg);
      return -1;
    }
    if ( ! match )
    {
      GEOSGeom_destroy(motionPolyG);
      lwgeom_free(motionPoly);
      lwerror("snapping edge %" LWTFMT_ELEMID
        " to new node moves it past edge %" LWTFMT_ELEMID,
        edge->edge_id, e->edge_id
      );
      return -1;
    }
  }
  if ( motionPolyG ) GEOSGeom_destroy(motionPolyG);

  lwgeom_free(motionPoly);

  return 0;
}

static int
_lwt_SnapEdgeToExistingNode(
  LWT_TOPOLOGY* topo,
  LWT_ISO_NODE *node,
  LWT_ISO_EDGE *edge, /* known to require an update from DB for the edge linking and face labelling - geometry is fine */
  double tol
)
{
  LWGEOM *prj = lwpoint_as_lwgeom(node->geom);
  const POINT2D *pt = getPoint2d_cp(lwgeom_as_lwpoint(prj)->point, 0);

  LWDEBUGF(1, "Experimental handling of within-tolerance edge %" LWTFMT_ELEMID, edge->edge_id);
  LWGEOM *snapE = lwgeom_snap(lwline_as_lwgeom(edge->geom), prj, tol);

  /* TODO: check if an endpoint moved */

  LWGEOM *splitE = lwgeom_split(snapE, prj);
  LWDEBUGG(1, splitE, "Split edge");

  LWCOLLECTION *splitC = lwgeom_as_lwcollection(splitE);
  if ( splitC->ngeoms != 2 )
  {
    lwerror("Split of edge resulted in %d components", splitC->ngeoms);
    return -1;
  }

  LWT_NODE_EDGES *firstNodeEdges = lwt_nodeEdges_loadFromDB(topo, edge->start_node, LWT_COL_EDGE_ALL );
  if ( ! firstNodeEdges ) {
    lwerror("No edges found in DB to be incident to split edge's first node");
    return -1;
  }
  LWT_NODE_EDGES *lastNodeEdges = lwt_nodeEdges_loadFromDB(topo, edge->end_node, LWT_COL_EDGE_ALL );
  if ( ! lastNodeEdges ) {
    lwerror("No edges found in DB to be incident to split edge's last node");
    return -1;
  }
  LWT_NODE_EDGES *splitNodeEdges = lwt_nodeEdges_loadFromDB(topo, node->node_id, LWT_COL_EDGE_ALL);
  if ( ! splitNodeEdges ) {
    lwerror("No edges found in DB to be incident to split node");
    return -1;
  }

  /* Update split edge reference as it was possibly changed by previous call */
  for ( uint64_t t=0; t<firstNodeEdges->numEdges; t++ )
  {
    if ( firstNodeEdges->edges[t].edge_id == edge->edge_id )
    {
      edge = &(firstNodeEdges->edges[t]);
      break;
    }
  }

  LWDEBUGF(1, "Split edge %"
      LWTFMT_ELEMID " start condition: next_right:%"
      LWTFMT_ELEMID ", next_left:%"
      LWTFMT_ELEMID ", face_right:%"
      LWTFMT_ELEMID ", face_left:%"
      LWTFMT_ELEMID ", start_node:%"
      LWTFMT_ELEMID ", end_node:%"
      LWTFMT_ELEMID,
      edge->edge_id,
      edge->next_right,
      edge->next_left,
      edge->face_right,
      edge->face_left,
      edge->start_node,
      edge->end_node
  );

  int ret;

  uint64_t n;
  LWT_ELEMID replacedBy[2];
  LWT_ISO_EDGE *existingEdges[2];
  //int replacedByForward[2];
  edgeend splitNodeEdgeEnds[2];
  for ( n=0; n<2; ++n)
  {
    int forward;
    /* TODO: directly get full existing edge rather than just id * */
    LWT_ELEMID existingEdgeId = _lwt_GetEqualEdge( topo, lwgeom_as_lwline(splitC->geoms[n]), &forward );
    if ( existingEdgeId == -1 )
    {
      /* probably too late, due to internal lwerror */
      return -1;
    }
    existingEdges[n] = NULL;
    if ( existingEdgeId == 0 )
    {
      LWDEBUGF(1, "Split component %llu is a new edge, computing edgeEndInfo", n);
      {
        // Compute nextCW and nextCCW of the split edge now
        // incident to the split point

        POINT2D op; /* other point */
        if ( n == 1 ) {
          ret = _lwt_FirstDistinctVertex2D(
            lwgeom_as_lwline(splitC->geoms[1])->points, //const POINTARRAY* pa,
            pt, //POINT2D *ref,
            0, // int from,
            1, // int dir,
            &op // POINT2D *op
          );
          if (!ret)
          {
            lwerror("No distinct vertices found on second part of split edge");
            return -1; /* lwerror should have been raised */
          }
        } else {
          const POINTARRAY *pa = lwgeom_as_lwline(splitC->geoms[0])->points;
          ret = _lwt_FirstDistinctVertex2D(
            pa, //const POINTARRAY* pa,
            pt, //POINT2D *ref,
            pa->npoints-1, // int from,
            -1, // int dir,
            &op // POINT2D *op
          );
          if (!ret)
          {
            lwerror("No distinct vertices found on first part of split edge");
            return -1; /* lwerror should have been raised */
          }
        }
        if ( ! azimuth_pt_pt(pt, &op, &(splitNodeEdgeEnds[n].myaz)) ) {
          lwerror("error computing azimuth of split endpoint [%.15g %.15g,%.15g %.15g]",
                  op.x, op.y,
                  pt->x, pt->y
          );
          return -1;
        }
        LWDEBUGF(1, "Azimuth of split component %llu edgeend [%.15g %.15g,%.15g %.15g] is %.15g",
          n, op.x, op.y, pt->x, pt->y, splitNodeEdgeEnds[n].myaz);
        ret = _lwt_FindAdjacentEdges( topo, node->node_id, &(splitNodeEdgeEnds[n]), NULL, -1 );
        if ( ! ret ) {
            lwerror("Unexpected backend return: _lwt_FindAdjacentEdges(%"
              LWTFMT_ELEMID ") found no edges when we previously split edge %"
              LWTFMT_ELEMID "d on it",
              node->node_id, edge->edge_id);
            return -1;
        }
        LWDEBUGF(1, "Component %llu of split edge %" LWTFMT_ELEMID
					" next CW is %" LWTFMT_ELEMID ", next CCW is %" LWTFMT_ELEMID,
          n, edge->edge_id, splitNodeEdgeEnds[n].nextCW, splitNodeEdgeEnds[n].nextCCW);
      }
    }
    else
    {
      LWDEBUGF(1, "Split component %llu matches existing edge %" LWTFMT_ELEMID
        " (%s)", n, existingEdgeId, forward ? "forward" : "backward" );
      /* Get existing edge */
      for ( uint64_t t=0; t<splitNodeEdges->numEdges; t++ )
      {
        if ( splitNodeEdges->edges[t].edge_id == existingEdgeId )
        {
          existingEdges[n] = &(splitNodeEdges->edges[t]);
          break;
        }
      }
      if (existingEdges[n] == NULL)
      {
        lwerror("could not find edge %" LWTFMT_ELEMID " in database, but _lwt_GetEqualEdges said it was there", existingEdgeId );
        return -1;
      }
    }

    replacedBy[n] = existingEdgeId;
  }


  LWT_ISO_EDGE updatedEdge;
  int updateFlags;
  LWT_ISO_EDGE selEdge;

  if ( ( replacedBy[0] != 0 && replacedBy[1] == 0 ) ||
       ( replacedBy[1] != 0 && replacedBy[0] == 0 ) )
  {
    /* One side of the snapped edge collapsed to an existing edge */

    LWT_ISO_EDGE *existingEdge;
    edgeend *splitNodeEdgeEndInfo;
    LWLINE *newSplitEdgeLine;
    int splitNodeNewEdgeOutgoing; // The new split edge goes out of the split node

    if ( replacedBy[1] == 0 )
    /* First part of edge is the existing edge */
    {
      existingEdge = existingEdges[0];
      newSplitEdgeLine = lwgeom_as_lwline(splitC->geoms[1]);
      splitNodeEdgeEndInfo = &(splitNodeEdgeEnds[1]);
      splitNodeNewEdgeOutgoing = 1;
    }
    else
    /* Second part of edge is the existing edge */
    {
      existingEdge = existingEdges[1];
      newSplitEdgeLine = lwgeom_as_lwline(splitC->geoms[0]);
      splitNodeEdgeEndInfo = &(splitNodeEdgeEnds[0]);
      splitNodeNewEdgeOutgoing = 0;
    }

    _lwt_SnapEdge_checkMotion( topo, splitC, edge, existingEdge, splitNodeEdges );

    LWDEBUGF(1, "Existing edge %"
        LWTFMT_ELEMID " (post-modEdgeSplit) next_right:%"
        LWTFMT_ELEMID ", next_left:%"
        LWTFMT_ELEMID ", face_right:%"
        LWTFMT_ELEMID ", face_left:%"
        LWTFMT_ELEMID,
        existingEdge->edge_id,
        existingEdge->next_right,
        existingEdge->next_left,
        existingEdge->face_right,
        existingEdge->face_left
    );

    /*****
     *
     * Update edge linking
     *
     */

    /*
     * Update next CW / CCW edges on split node
     * (fix linking on split node)
     */

    /* Update next CCW edge */
    {
      LWT_ELEMID nextEdge, sideFace;
      if ( splitNodeNewEdgeOutgoing ) {
        nextEdge = edge->edge_id;
        sideFace = edge->face_left;
      } else {
        nextEdge = -edge->edge_id;
        sideFace = edge->face_right;
      }

      LWDEBUGF(1, "Setting nextEdge/sideFace of CCW edge %" LWTFMT_ELEMID
        " to %" LWTFMT_ELEMID "/%" LWTFMT_ELEMID,
        splitNodeEdgeEndInfo->nextCCW,
        nextEdge, sideFace
      );

      if ( splitNodeEdgeEndInfo->nextCCW > 0 )
      {
        selEdge.edge_id = splitNodeEdgeEndInfo->nextCCW;
        updatedEdge.next_right = nextEdge;
        updatedEdge.face_right = sideFace;
        updateFlags = LWT_COL_EDGE_NEXT_RIGHT|
                      LWT_COL_EDGE_FACE_RIGHT;
        LWDEBUGF(1, "Will update next_right/face_right of outgoing CCW edge %" LWTFMT_ELEMID
          " to %" LWTFMT_ELEMID "/%" LWTFMT_ELEMID,
          selEdge.edge_id,
          nextEdge, sideFace
        );
      }
      else
      {
        selEdge.edge_id = -splitNodeEdgeEndInfo->nextCCW;
        updatedEdge.next_left = nextEdge;
        updatedEdge.face_left = sideFace;
        updateFlags = LWT_COL_EDGE_NEXT_LEFT|
                      LWT_COL_EDGE_FACE_LEFT;
        LWDEBUGF(1, "Will update next_left/face_left of incoming CCW edge %" LWTFMT_ELEMID
          " to %" LWTFMT_ELEMID "/%" LWTFMT_ELEMID,
          selEdge.edge_id,
          nextEdge, sideFace
        );
      }

      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge %" LWTFMT_ELEMID " disappeared during operations", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " ! (corrupted topology?)", selEdge.edge_id);
        return -1;
      }
    }

    /* Update next CW edge on split node */
    {
      LWT_ELEMID sideFace;
      if ( splitNodeNewEdgeOutgoing ) {
        sideFace = edge->face_right;
      } else {
        sideFace = edge->face_left;
      }

      LWDEBUGF(1, "Setting sideFace of CW edge %" LWTFMT_ELEMID
        " to %" LWTFMT_ELEMID,
        splitNodeEdgeEndInfo->nextCW, sideFace
      );

      if ( splitNodeEdgeEndInfo->nextCW > 0 )
      {
        selEdge.edge_id = splitNodeEdgeEndInfo->nextCW;
        updatedEdge.face_left = sideFace;
        updateFlags = LWT_COL_EDGE_FACE_LEFT;
        LWDEBUGF(1, "Updating left_face of edge %" LWTFMT_ELEMID
          ", outgoing next CW of new split edge on split node",
          selEdge.edge_id);
      }
      else
      {
        selEdge.edge_id = -splitNodeEdgeEndInfo->nextCW;
        updatedEdge.face_right = sideFace;
        updateFlags = LWT_COL_EDGE_FACE_RIGHT;
        LWDEBUGF(1, "Updating right_face of edge %" LWTFMT_ELEMID
          ", incoming next CW of new split edge on split node",
          selEdge.edge_id);
      }

      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge %" LWTFMT_ELEMID " disappeared during operations", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with edge_id %"
          LWTFMT_ELEMID " ! (corrupted topology?)", selEdge.edge_id);
        return -1;
      }
    }

    /*
     * Update links to now-disappeared edgeend
     * on the node originally in common with
     * the existing node
     */

    LWT_ISO_EDGE *nextCCWEdge = NULL;
    int nextCCWEdgeIsIncoming = 0;
    int collapsedEdgeIsIncoming;

    LWT_ELEMID commonNodeID;

    LWT_NODE_EDGES *commonNodeEdges;
    if ( replacedBy[0] ) {
      commonNodeEdges = firstNodeEdges;
      commonNodeID = edge->start_node;
      collapsedEdgeIsIncoming = 0;
    } else {
      commonNodeEdges = lastNodeEdges;
      commonNodeID = edge->end_node;
      collapsedEdgeIsIncoming = 1;
    }


    // Update next CCW edge
    {
      LWT_ELEMID signedCollapsedEdgeID = collapsedEdgeIsIncoming ?  -edge->edge_id: edge->edge_id;

      LWDEBUGF(1, "Looking for next CCW edge of split edge %"
          LWTFMT_ELEMID " on common node %" LWTFMT_ELEMID
          " having %llu attached edges",
          edge->edge_id,
          commonNodeID, commonNodeEdges->numEdges
      );

      /* Find next CCW edge */
      for ( uint64_t t=0; t<commonNodeEdges->numEdges; t++ )
      {
        LWT_ISO_EDGE *et = &(commonNodeEdges->edges[t]);

        LWDEBUGF(1, "common node edge %"
            LWTFMT_ELEMID " has next_left:%" LWTFMT_ELEMID
            " next_right:%" LWTFMT_ELEMID,
            et->edge_id,
            et->next_left,
            et->next_right
        );

        if ( et->start_node == commonNodeID )
        {
          if ( et->next_right == signedCollapsedEdgeID )
          {
            nextCCWEdge = et;
            nextCCWEdgeIsIncoming = 0;
            break;
          }
        }
        else if ( et->end_node == commonNodeID )
        {
          if ( et->next_left == signedCollapsedEdgeID )
          {
            nextCCWEdge = et;
            nextCCWEdgeIsIncoming = 1;
            break;
          }
        }
      }
      if ( ! nextCCWEdge )
      {
        lwerror("Could not find nextCCW edge on common node %" LWTFMT_ELEMID,
             commonNodeID
        );
        return -1;
      }

      LWDEBUGF(1, "Next CCW edge of split edge %"
          LWTFMT_ELEMID " on common node %" LWTFMT_ELEMID
          " is (%s) %" LWTFMT_ELEMID,
          edge->edge_id,
          commonNodeID,
          nextCCWEdgeIsIncoming ? "incoming" : "outgoing",
          nextCCWEdge->edge_id
      );

      if ( nextCCWEdge->edge_id == existingEdge->edge_id )
      {
        LWDEBUG(1, "Next CCW edge is existing/collapse edge, will not update here");
      }
      else
      {
        /*
         *  TODO: the nextCCW is only of interest IFF the collapse
         *        happened clockwise!
         *
         *  just update to the split edge id (positive or negative
         *  depending on whether it was outgoing or incoming in the
         *  common node)
         */
        LWT_ELEMID newNextEdge = existingEdge->start_node == commonNodeID ?
          existingEdge->edge_id :
          -existingEdge->edge_id;

        selEdge.edge_id = nextCCWEdge->edge_id;
        if ( nextCCWEdgeIsIncoming ) {
          updatedEdge.next_left = newNextEdge;
          updateFlags = LWT_COL_EDGE_NEXT_LEFT;
          LWDEBUGF(1, "Updating next_left of incoming next CCW edge %"
              LWTFMT_ELEMID
              " to %" LWTFMT_ELEMID,
              selEdge.edge_id,
              newNextEdge
          );
        } else {
          updatedEdge.next_right = newNextEdge;
          updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
          LWDEBUGF(1, "Updating next_right of outgoing next CCW edge %"
              LWTFMT_ELEMID
              " to %" LWTFMT_ELEMID,
              selEdge.edge_id,
              newNextEdge
          );
        }
        ret = lwt_be_updateEdges(topo,
          &selEdge, LWT_COL_EDGE_EDGE_ID,
          &updatedEdge, updateFlags,
          NULL, 0);
        if ( ret == -1 ) {
          PGTOPO_BE_ERROR();
          return -1;
        } else if ( ret == 0 ) {
          lwerror("Edge %" LWTFMT_ELEMID " disappeared during operations", selEdge.edge_id);
          return -1;
        } else if ( ret > 1 ) {
          lwerror("More than a single edge found with next_left %"
            LWTFMT_ELEMID " ! (corrupted topology?)", selEdge.next_left);
          return -1;
        }
      }
    }

    /*****
     *
     * Update the existing edge our split edge collapsed to
     *
     */

    selEdge.edge_id = existingEdge->edge_id;
    updateFlags = 0;
    if ( existingEdge->next_left == edge->edge_id )
    {
      updatedEdge.next_left = edge->next_right;
      updateFlags |= LWT_COL_EDGE_NEXT_LEFT;
      LWDEBUGF(1, "Will update next_left of existing edge %"
        LWTFMT_ELEMID " to %" LWTFMT_ELEMID,
        selEdge.edge_id,
        updatedEdge.next_left
      );
    }
    if ( existingEdge->next_right == edge->edge_id )
    {
      updatedEdge.next_right = edge->next_right;
      updateFlags |= LWT_COL_EDGE_NEXT_RIGHT;
      LWDEBUGF(1, "Will update next_right of existing edge %"
        LWTFMT_ELEMID " to %" LWTFMT_ELEMID,
        selEdge.edge_id,
        updatedEdge.next_right
      );
    }
    if ( existingEdge->next_left == -edge->edge_id )
    {
      updatedEdge.next_left = edge->next_left;
      updateFlags |= LWT_COL_EDGE_NEXT_LEFT;
      LWDEBUGF(1, "Will update next_left of existing edge %"
        LWTFMT_ELEMID " to %" LWTFMT_ELEMID,
        selEdge.edge_id,
        updatedEdge.next_left
      );
    }
    if ( existingEdge->next_right == -edge->edge_id )
    {
      updatedEdge.next_right = edge->next_left;
      updateFlags |= LWT_COL_EDGE_NEXT_RIGHT;
      LWDEBUGF(1, "Will update next_right of existing edge %"
        LWTFMT_ELEMID " to %" LWTFMT_ELEMID,
        selEdge.edge_id,
        updatedEdge.next_right
      );
    }
    if ( updateFlags )
    {
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    /*****
     *
     * Update the split edge
     *
     */

    selEdge.edge_id = edge->edge_id;
    updatedEdge.geom = newSplitEdgeLine;
    updateFlags = LWT_COL_EDGE_GEOM;
    if ( splitNodeNewEdgeOutgoing ) {
      updatedEdge.start_node = node->node_id;
      updatedEdge.next_right = splitNodeEdgeEndInfo->nextCW;
      updateFlags |= LWT_COL_EDGE_START_NODE;
      updateFlags |= LWT_COL_EDGE_NEXT_RIGHT;
      LWDEBUGF(1, "Updating split edge %" LWTFMT_ELEMID
        " start node to %" LWTFMT_ELEMID
        " and next_right to %" LWTFMT_ELEMID,
        selEdge.edge_id,
        updatedEdge.start_node,
        updatedEdge.next_right
      );
    } else {
      updatedEdge.end_node = node->node_id;
      updatedEdge.next_left = splitNodeEdgeEndInfo->nextCW;
      updateFlags |= LWT_COL_EDGE_END_NODE;
      updateFlags |= LWT_COL_EDGE_NEXT_LEFT;
      LWDEBUGF(1, "Updating split edge %" LWTFMT_ELEMID
        " end node to %" LWTFMT_ELEMID
        " and next_left to %" LWTFMT_ELEMID
        ,
        selEdge.edge_id,
        updatedEdge.end_node
        , updatedEdge.next_left
      );
    }

    ret = lwt_be_updateEdges(topo,
      &selEdge, LWT_COL_EDGE_EDGE_ID,
      &updatedEdge, updateFlags,
      NULL, 0);
    if ( ret == -1 ) {
      PGTOPO_BE_ERROR();
      return -1;
    } else if ( ret == 0 ) {
      lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
      return -1;
    } else if ( ret > 1 ) {
      lwerror("More than a single edge found with id %"
        LWTFMT_ELEMID " !", selEdge.edge_id);
      return -1;
    }

    /************
     *
     * Update split-edge collapsed side face MBR, if needed
     *
     */

    if ( edge->face_left != edge->face_right )
    {
      /* TODO: update a single face ? */
      LWT_ISO_FACE updface;
      updface.face_id = edge->face_right;
      if ( updface.face_id != 0 )
      {
        /* We only need to update the MBR if the shrunk face is
         * not the universe face */
        LWGEOM *fg = lwt_GetFaceGeometry(topo, updface.face_id);
        if ( ! fg )
        {
          lwerror("Could not get geometry of face %" LWTFMT_ELEMID, updface.face_id);
          return -1;
        }
        updface.mbr = (GBOX *)lwgeom_get_bbox(fg);
        if ( updface.mbr )
        {
          int ret = lwt_be_updateFacesById( topo, &updface, 1 );
          if ( ret == -1 )
          {
            PGTOPO_BE_ERROR();
            return -1;
          }
          lwgeom_free(fg);
        }
        else
        {
          lwerror("Programmatic error ? Geometry of face %" LWTFMT_ELEMID " is empty!", updface.face_id);
        }
      }
      updface.face_id = edge->face_left;
      if ( updface.face_id != 0 )
      {
        /* We only need to update the MBR if the shrunk face is
         * not the universe face */
        LWGEOM *fg = lwt_GetFaceGeometry(topo, updface.face_id);
        if ( ! fg )
        {
          lwerror("Could not get geometry of face %" LWTFMT_ELEMID, updface.face_id);
          return -1;
        }
        updface.mbr = (GBOX *)lwgeom_get_bbox(fg);
        if ( updface.mbr )
        {
          int ret = lwt_be_updateFacesById( topo, &updface, 1 );
          if ( ret == -1 )
          {
            PGTOPO_BE_ERROR();
            return -1;
          }
          lwgeom_free(fg);
        }
        else
        {
          lwerror("Programmatic error ? Geometry of face %" LWTFMT_ELEMID " is empty!", updface.face_id);
        }
      }
    }

    /************
     *
     * Update TopoGeometries composition
     *
     */

    LWDEBUGF(1, "Updating lineal TopoGeometry composition to add edge %" LWTFMT_ELEMID
      " to features composed by edge %" LWTFMT_ELEMID,
      existingEdge->edge_id, edge->edge_id
    );
    ret = lwt_be_updateTopoGeomEdgeSplit(topo, edge->edge_id, existingEdge->edge_id, -1);
    if ( ! ret ) {
      PGTOPO_BE_ERROR();
      return -1;
    }

    return 0;
  }
  else if ( replacedBy[0] == 0 && replacedBy[1] == 0 )
  {

    /* Neither sides of the snapped edge collapsed to an existing edge */

    /* New edge is the outgoing one, by design */
    LWT_ISO_EDGE newEdge;
    newEdge.edge_id = lwt_be_getNextEdgeId( topo );
    if ( newEdge.edge_id == -1 ) {
      PGTOPO_BE_ERROR();
      return -1;
    }
    newEdge.geom = lwgeom_as_lwline(splitC->geoms[1]);
    newEdge.end_node = edge->end_node;
    newEdge.start_node = node->node_id;
    newEdge.next_right = -updatedEdge.edge_id; // to be assigned later, if needed
    if ( edge->next_left == -edge->edge_id ) {
      newEdge.next_left = -newEdge.edge_id;
    } else {
      newEdge.next_left = edge->next_left; // unchanged
    }
    newEdge.face_left = edge->face_left; // to be changed later
    newEdge.face_right = edge->face_right; // to be changed later

    /* Updated edge is the incoming one, by design */
    updatedEdge.edge_id = edge->edge_id;
    updatedEdge.geom = lwgeom_as_lwline(splitC->geoms[0]);
    updatedEdge.end_node = node->node_id;
    updatedEdge.next_left = newEdge.edge_id; // to be re-assigned later, if needed


    LWT_EDGEEND_STAR *nodeStar = lwt_edgeEndStar_init( node->node_id );
    for ( uint64_t t=0; t<splitNodeEdges->numEdges; t++ )
    {
      lwt_edgeEndStar_addEdge( nodeStar, &(splitNodeEdges->edges[t]) );
    }
    lwt_edgeEndStar_addEdge( nodeStar, &updatedEdge );
    lwt_edgeEndStar_addEdge( nodeStar, &newEdge );

    _lwt_SnapEdge_checkMotion( topo, splitC, edge, NULL, splitNodeEdges );

    /* There cannot be anything in the middle of the two components,
     * so both sides will give the same nextCCW and same nextCW */

    lwt_EdgeEndStar_debugPrint( nodeStar );

    const LWT_EDGEEND *ee0 = lwt_edgeEndStar_getNextCW( nodeStar, &updatedEdge, 0); // incoming
    LWDEBUGF(1, "* NextCW of incoming snap-edge component is %s %" LWTFMT_ELEMID,
      ee0->outgoing ? "outgoing" : "incoming", ee0->edge->edge_id);
    if ( ee0->outgoing ) {
      updatedEdge.next_left = ee0->edge->edge_id;
    } else  {
      updatedEdge.next_left = -ee0->edge->edge_id;
    }
    int splitFaceOnLeft = 1;
    if ( ee0->edge->edge_id == newEdge.edge_id )
    {
      splitFaceOnLeft = 0;
    }

    const LWT_EDGEEND *ee1 = lwt_edgeEndStar_getNextCW( nodeStar, &newEdge, 1); // outgoing
    LWDEBUGF(1, "* NextCW of outgoing snap-edge component is %s %" LWTFMT_ELEMID,
      ee1->outgoing ? "outgoing" : "incoming", ee1->edge->edge_id);
    if ( ee1->outgoing ) {
      newEdge.next_right = ee1->edge->edge_id;
    } else  {
      newEdge.next_right = -ee1->edge->edge_id;
    }

    int ret = lwt_be_insertEdges(topo, &newEdge, 1);
    if (ret != 1)
    {
      PGTOPO_BE_ERROR();
      return -1;
    }

    /* Updating the first portion of the edge */
    selEdge.edge_id = edge->edge_id;
    ret = lwt_be_updateEdges(topo,
      &selEdge, LWT_COL_EDGE_EDGE_ID,
      &updatedEdge, LWT_COL_EDGE_NEXT_LEFT|LWT_COL_EDGE_GEOM|LWT_COL_EDGE_END_NODE,
      NULL, 0);
    if ( ret == -1 ) {
      PGTOPO_BE_ERROR();
      return -1;
    } else if ( ret == 0 ) {
      lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
      return -1;
    } else if ( ret > 1 ) {
      lwerror("More than a single edge found with id %"
        LWTFMT_ELEMID " !", selEdge.edge_id);
      return -1;
    }

    /* Update CCW edges if needed */
    ee0 = lwt_edgeEndStar_getNextCCW( nodeStar, &updatedEdge, 0); // incoming
    LWDEBUGF(1, "* NextCCW of incoming snap-edge component is %s %" LWTFMT_ELEMID,
      ee0->outgoing ? "outgoing" : "incoming", ee0->edge->edge_id);
    if ( ee0->edge->edge_id != newEdge.edge_id )
    {
      selEdge.edge_id = ee0->edge->edge_id;
      if ( ee0->outgoing ) {
        updatedEdge.next_right = -edge->edge_id;
        updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
      } else {
        updatedEdge.next_left = -edge->edge_id;
        updateFlags = LWT_COL_EDGE_NEXT_LEFT;
      }

      LWDEBUGF(1, "Updating edge %" LWTFMT_ELEMID ", %s nextCCW of incoming snap-edge",
        selEdge.edge_id, ee0->outgoing ? "outgoing" : "incoming");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    /* Update CCW edge of outgoing portion if needed */
    ee1 = lwt_edgeEndStar_getNextCCW( nodeStar, &newEdge, 1); // outgoing
    LWDEBUGF(1, "* NextCCW of outgoing snap-edge component is %s %" LWTFMT_ELEMID,
      ee1->outgoing ? "outgoing" : "incoming", ee1->edge->edge_id);
    if ( ee1->edge->edge_id != edge->edge_id )
    {
      selEdge.edge_id = ee1->edge->edge_id;
      if ( ee1->outgoing ) {
        updatedEdge.next_right = newEdge.edge_id;
        updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
      } else {
        updatedEdge.next_left = newEdge.edge_id;
        updateFlags = LWT_COL_EDGE_NEXT_LEFT;
      }

      LWDEBUGF(1, "Updating edge %" LWTFMT_ELEMID ", %s nextCCW of outgoing snap-edge",
        selEdge.edge_id, ee1->outgoing ? "outgoing" : "incoming");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    /* If any edge was connected to the end node of the split edge, we
     * need to update its link to us */
    updateFlags = 0;
    for ( uint64_t t=0; t<lastNodeEdges->numEdges; t++ )
    {
      const LWT_ISO_EDGE *in = &(lastNodeEdges->edges[t]);
      if ( in->next_right == -edge->edge_id )
      {
        if ( in->edge_id != edge->edge_id ) {
          selEdge.edge_id = in->edge_id;
          updatedEdge.next_right = -newEdge.edge_id;
          updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
        }
        break;
      }
      if ( in->next_left == -edge->edge_id )
      {
        if ( in->edge_id != edge->edge_id ) {
          selEdge.edge_id = in->edge_id;
          updatedEdge.next_left = -newEdge.edge_id;
          updateFlags = LWT_COL_EDGE_NEXT_LEFT;
        }
        break;
      }
    }
    if ( updateFlags != 0 )
    {
      LWDEBUGF(1, "Updating edge %" LWTFMT_ELEMID ", %s nextCCW of outgoing snap-edge",
        selEdge.edge_id,
        updateFlags & LWT_COL_EDGE_NEXT_LEFT ?  "incoming" : "outgoing");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    /* Handle face split */
    LWT_ELEMID newFaceId1 = 0;
    LWT_ELEMID newFaceId2 = 0;
    LWT_ELEMID oldFaceId;
    if ( splitFaceOnLeft ) {

      oldFaceId = edge->face_left;

      LWDEBUGF(1, "Adding face split for left side of new edge %" LWTFMT_ELEMID, newEdge.edge_id);
      newFaceId1 = _lwt_AddFaceSplit( topo, newEdge.edge_id, oldFaceId, 0 );
      LWDEBUGF(1, "_lwt_AddFaceSplit(%" LWTFMT_ELEMID ") returned %" LWTFMT_ELEMID,
                  newEdge.edge_id, newFaceId1);

      // TODO: retain oldFaceID rather than creating a new one, if a
      // new one was already created for the new edge ring !
      LWDEBUGF(1, "Adding face split for left side of updated edge %" LWTFMT_ELEMID, edge->edge_id);
      newFaceId2 = _lwt_AddFaceSplit( topo, edge->edge_id, oldFaceId, 0 );
      LWDEBUGF(1, "_lwt_AddFaceSplit(%" LWTFMT_ELEMID ") returned %" LWTFMT_ELEMID,
                  edge->edge_id, newFaceId2);


    } else {

      oldFaceId = edge->face_right;

      LWDEBUGF(1, "Adding face split for right side of new edge %" LWTFMT_ELEMID, newEdge.edge_id);
      newFaceId1 = _lwt_AddFaceSplit( topo, -newEdge.edge_id, oldFaceId, 0 );
      LWDEBUGF(1, "_lwt_AddFaceSplit(%" LWTFMT_ELEMID ") returned %" LWTFMT_ELEMID,
                  -newEdge.edge_id, newFaceId1);

      // TODO: retain oldFaceID rather than creating a new one, if a
      // new one was already created on the new edge ring !
      LWDEBUGF(1, "Adding face split for right side of updated edge %" LWTFMT_ELEMID, edge->edge_id);
      newFaceId2 = _lwt_AddFaceSplit( topo, -edge->edge_id, oldFaceId, 0 );
      LWDEBUGF(1, "_lwt_AddFaceSplit(%" LWTFMT_ELEMID ") returned %" LWTFMT_ELEMID,
                  -edge->edge_id, newFaceId2);

    }

    /************
     *
     * Update TopoGeometries composition
     *
     */

    ret = lwt_be_updateTopoGeomEdgeSplit(topo, edge->edge_id, newEdge.edge_id, -1);
    if ( ! ret ) {
      PGTOPO_BE_ERROR();
      return -1;
    }

    LWDEBUGF(1, "New faces %" LWTFMT_ELEMID " and %" LWTFMT_ELEMID
                "split from from old face %" LWTFMT_ELEMID,
                newFaceId1, newFaceId2, oldFaceId);
    if ( newFaceId1 || newFaceId2 )
    {
      ret = lwt_be_updateTopoGeomFaceSplit(
        topo, oldFaceId,
        newFaceId1 ? newFaceId1 : oldFaceId,
        newFaceId2 ? newFaceId2 : oldFaceId
      );
      if ( ! ret ) {
        PGTOPO_BE_ERROR();
        return -1;
      }

      if ( newFaceId1 && newFaceId2 )
      {
        LWDEBUGF(1, "Deleting fully replaced face %" LWTFMT_ELEMID, oldFaceId);
        int nids = 1;
        ret = lwt_be_deleteFacesById(topo, &oldFaceId, nids);
        if ( ret == -1 ) {
          PGTOPO_BE_ERROR();
          return -1;
        }
      }
    }

    return 0;
  }
  else if ( replacedBy[0] != 0 && replacedBy[1] != 0 )
  {
    // Edge was replaced with two existing edges

    // Sanity check
    if ( edge->face_right == edge->face_left )
    {
      lwerror("Unexpectedly, dangling edge %" LWTFMT_ELEMID " was replaced by "
        "edges %" LWTFMT_ELEMID " and %" LWTFMT_ELEMID
        " when snap to node %" LWTFMT_ELEMID,
        edge->edge_id, replacedBy[0], replacedBy[1], node->node_id);
      return -1;
    }

    LWT_ELEMID collapsedFace = 0;
    LWT_ELEMID replacingFace = 0;

    if ( FP_ABS(edge->next_right) == replacedBy[0] ) {
      /* Face on the right of the removed edge disappeared */
      // We need to update nextCW on first node and nextCCW on last node
      collapsedFace = edge->face_right;
      replacingFace = edge->face_left;
    } else {
      /* Face on the left of the removed edge disappeared */
      // We need to update nextCCW on first node and nextCW on last node
      collapsedFace = edge->face_left;
      replacingFace = edge->face_right;
    }

    //const LWT_ISO_EDGE *sp_nextCCW = 0;
    //const LWT_ISO_EDGE *ep_nextCCW = 0;

    // Update CCW edge on first node
    updateFlags = 0;
    for ( uint64_t t=0; t<firstNodeEdges->numEdges; t++ )
    {
      const LWT_ISO_EDGE *in = &(firstNodeEdges->edges[t]);
      if ( in->edge_id == edge->edge_id ) continue;
      if ( in->next_right == edge->edge_id )
      {
        selEdge.edge_id = in->edge_id;
        updatedEdge.next_right = edge->next_right;
        updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
        if ( in->face_right == collapsedFace ) {
          updatedEdge.face_right = replacingFace;
          updateFlags |= LWT_COL_EDGE_FACE_RIGHT;
        }
        //sp_nextCCW = in;
        break;
      }
      if ( in->next_left == edge->edge_id )
      {
        selEdge.edge_id = in->edge_id;
        updatedEdge.next_left = edge->next_right;
        updateFlags = LWT_COL_EDGE_NEXT_LEFT;
        if ( in->face_left == collapsedFace ) {
          updatedEdge.face_left = replacingFace;
          updateFlags |= LWT_COL_EDGE_FACE_LEFT;
        }
        //sp_nextCCW = in;
        break;
      }
    }
    if ( updateFlags != 0 )
    {
      LWDEBUGF(1, "Updating edge %" LWTFMT_ELEMID ", %s nextCCW of outgoing snap-edge",
        selEdge.edge_id,
        updateFlags & LWT_COL_EDGE_NEXT_LEFT ?  "incoming" : "outgoing");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    // Update CCW edge on last node
    updateFlags = 0;
    for ( uint64_t t=0; t<lastNodeEdges->numEdges; t++ )
    {
      const LWT_ISO_EDGE *in = &(lastNodeEdges->edges[t]);
      if ( in->edge_id == edge->edge_id ) continue;
      if ( in->next_right == -edge->edge_id )
      {
        selEdge.edge_id = in->edge_id;
        updatedEdge.next_right = edge->next_left;
        updateFlags = LWT_COL_EDGE_NEXT_RIGHT;
        if ( in->face_right == collapsedFace ) {
          updatedEdge.face_right = replacingFace;
          updateFlags |= LWT_COL_EDGE_FACE_RIGHT;
        }
        break;
      }
      if ( in->next_left == -edge->edge_id )
      {
        selEdge.edge_id = in->edge_id;
        updatedEdge.next_left = edge->next_left;
        updateFlags = LWT_COL_EDGE_NEXT_LEFT;
        if ( in->face_left == collapsedFace ) {
          updatedEdge.face_left = replacingFace;
          updateFlags |= LWT_COL_EDGE_FACE_LEFT;
        }
        break;
      }
    }
    if ( updateFlags != 0 )
    {
      LWDEBUGF(1, "Updating edge %" LWTFMT_ELEMID ", %s nextCCW of incoming snap-edge",
        selEdge.edge_id,
        updateFlags & LWT_COL_EDGE_NEXT_LEFT ?  "incoming" : "outgoing");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    if ( collapsedFace == edge->face_right )
    {
      // We need to update side face of nextCW on first node and nextCCW on last node
      // nextCCW on last node was done already, supposedly so we need to do nextCW
      // on first
      if ( edge->next_right < 0 ) {
        selEdge.edge_id = -edge->next_right;
        updatedEdge.face_right = replacingFace;
        updateFlags = LWT_COL_EDGE_FACE_RIGHT;
      } else {
        selEdge.edge_id = edge->next_right;
        updatedEdge.face_left = replacingFace;
        updateFlags = LWT_COL_EDGE_FACE_LEFT;
      }
      LWDEBUGF(1, "Updating %s face of edge %" LWTFMT_ELEMID ", %s nextCW of outgoing snap-edge",
        updateFlags & LWT_COL_EDGE_FACE_LEFT ?  "left" : "right",
        selEdge.edge_id,
        updateFlags & LWT_COL_EDGE_FACE_RIGHT ? "incoming" : "outgoing");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }
    else
    {
      // We need to update side face of nextCCW on first node and nextCW on last node
      // nextCCW on first node was done already, supposedly so we need to do nextCW
      // on last
      if ( edge->next_left < 0 ) {
        selEdge.edge_id = -edge->next_left;
        updatedEdge.face_right = replacingFace;
        updateFlags = LWT_COL_EDGE_FACE_RIGHT;
      } else {
        selEdge.edge_id = edge->next_left;
        updatedEdge.face_left = replacingFace;
        updateFlags = LWT_COL_EDGE_FACE_LEFT;
      }
      LWDEBUGF(1, "Updating %s face of edge %" LWTFMT_ELEMID ", %s nextCW of incoming snap-edge",
        updateFlags & LWT_COL_EDGE_FACE_LEFT ?  "left" : "right",
        selEdge.edge_id,
        updateFlags & LWT_COL_EDGE_FACE_RIGHT ? "incoming" : "outgoing");
      ret = lwt_be_updateEdges(topo,
        &selEdge, LWT_COL_EDGE_EDGE_ID,
        &updatedEdge, updateFlags,
        NULL, 0);
      if ( ret == -1 ) {
        PGTOPO_BE_ERROR();
        return -1;
      } else if ( ret == 0 ) {
        lwerror("Edge being split (%" LWTFMT_ELEMID ") disappeared during operations?", selEdge.edge_id);
        return -1;
      } else if ( ret > 1 ) {
        lwerror("More than a single edge found with id %"
          LWTFMT_ELEMID " !", selEdge.edge_id);
        return -1;
      }
    }

    /* Delete the old edge */
    selEdge.edge_id = edge->edge_id;
    LWDEBUGF(1, "Deleting edge %" LWTFMT_ELEMID, selEdge.edge_id);
    ret = lwt_be_deleteEdges(topo, &selEdge, LWT_COL_EDGE_EDGE_ID);
    if ( ret == -1 ) {
      PGTOPO_BE_ERROR();
      return -1;
    }

    /* Delete collapsed face */
    LWDEBUGF(1, "Deleting collapsed face %" LWTFMT_ELEMID, collapsedFace);
    int nids = 1;
    ret = lwt_be_deleteFacesById(topo, &collapsedFace, nids);
    if ( ret == -1 ) {
      PGTOPO_BE_ERROR();
      return -1;
    }

    /* Delete the collapsed face from composition of TopoGeometries */
    if ( ! lwt_be_updateTopoGeomFaceHeal( topo, collapsedFace, -1, -1) )
    {
      PGTOPO_BE_ERROR();
      return -1;
    }

    return 0;
  }

  //LWDEBUGF(1, "Not handling within-tolerance edge %" LWTFMT_ELEMID, edge->edge_id);
  lwerror("Don't know how to deal with edge %" LWTFMT_ELEMID
          " replaced by edges %" LWTFMT_ELEMID
          " and %" LWTFMT_ELEMID, edge->edge_id, replacedBy[0], replacedBy[1]);

  return -1;
}

/*
 *
 * Move the given point as close as possible to the
 * closest edge, snap all edges within tolerance to
 * it and split them accordingly.
 *
 * @param edges array of ISO edges with all attributes
 * @param num number of items in edges
 * @param point the point of the new node
 * @param moved if not-null will be set to 0 if the point was added
 *              w/out any snapping or 1 otherwise.
 *
 * @return id of the new node, 0 if no edge was found below tolerance or -1 on error
 *
 */
static LWT_ELEMID
_lwt_SplitAllEdgesToNewNode(LWT_TOPOLOGY* topo, LWT_ISO_EDGE *edges, uint64_t num, LWPOINT *point, double tol, int *moved)
{
  uint64_t i, j;
  scored_pointer *sorted = lwalloc(sizeof(scored_pointer)*num);
  LWT_ISO_EDGE *edges2 = NULL;
  LWT_ISO_NODE node;
  node.node_id = 0; /* unneeded, but hushes a compiler warning */
  LWGEOM *prj; /* incoming point projected to nearest node or edge */
  const POINT2D *pt = NULL; /* projected 2d point */
  LWGEOM *lwpoint = lwpoint_as_lwgeom(point);

  j = 0;
  for (i=0; i<num; ++i)
  {
    double dist;
    LWT_ISO_EDGE *e = edges+i;
    /* TODO: use square distance */
    dist = lwgeom_mindistance2d(lwline_as_lwgeom(edges[i].geom), lwpoint_as_lwgeom(point));
    LWDEBUGF(1, "Edge %" LWTFMT_ELEMID " distance: %.15g", e->edge_id, dist);
    if ( dist >= tol ) continue;
    sorted[j].ptr = e;
    sorted[j++].score = dist;
  }
  if ( ! j )
  {
    lwfree(sorted);
    return 0;
  }

  num = j;
  qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
  edges2 = lwalloc(sizeof(LWT_ISO_EDGE)*num);
  for (j=0, i=0; i<num; ++i)
  {
    if ( sorted[i].score == sorted[0].score )
    {
      edges2[j++] = *((LWT_ISO_EDGE*)sorted[i].ptr);
    }
    else
    {
      edges2[j++] = *((LWT_ISO_EDGE*)sorted[i].ptr);
      //lwline_free(((LWT_ISO_EDGE*)sorted[i].ptr)->geom);
    }
  }
  num = j;
  lwfree(sorted);
  edges = edges2;

  /* project point to closest edge */
  prj = lwgeom_closest_point(lwline_as_lwgeom(edges[0].geom), lwpoint);
  if ( moved ) *moved = lwgeom_same(prj, lwpoint) ? 0 : 1;
  if ( lwgeom_has_z(lwpoint) )
  {{
    /*
    -- This is a workaround for ClosestPoint lack of Z support:
    -- http://trac.osgeo.org/postgis/ticket/2033
    */
    LWGEOM *tmp;
    double z;
    POINT4D p4d;
    LWPOINT *prjpt;
    /* add Z to "prj" */
    tmp = lwgeom_force_3dz(prj, 0);
    prjpt = lwgeom_as_lwpoint(tmp);
    getPoint4d_p(point->point, 0, &p4d);
    z = p4d.z;
    getPoint4d_p(prjpt->point, 0, &p4d);
    p4d.z = z;
    ptarray_set_point4d(prjpt->point, 0, &p4d);
    lwgeom_free(prj);
    prj = tmp;
  }}

  LWDEBUGG(1, prj, "Projected point");
  node.geom = lwgeom_as_lwpoint(prj);
  node.containing_face = -1; /* means not-isolated */

  for (i=0; i<num; ++i)
  {
    /* The point is on or near one or more edges,
     * project it to closest edge and then split all
     * edges to it
     */

    LWT_ISO_EDGE *e = &(edges[i]);
    LWGEOM *g = lwline_as_lwgeom(e->geom);
    int contains;
    LWT_ELEMID edge_id = e->edge_id;

    if ( i )
    {{
      int ret = _lwt_SnapEdgeToExistingNode( topo, &node, e, tol );
      if ( ret != 0 ) {
        lwerror("Something went wrong with _lwt_SnapEdgeToExistingNode");
        return -1;
      }
      continue;
    }}

    LWDEBUGF(1, "Splitting closest edge %" LWTFMT_ELEMID, edge_id);

    pt = getPoint2d_cp(lwgeom_as_lwpoint(prj)->point, 0);
    contains = ptarray_contains_point_partial(e->geom->points, pt, 0, NULL) == LW_BOUNDARY;
    if ( ! contains )
    {{
      LWGEOM *snapedge;
      LWLINE *snapline;
      POINT4D p1, p2;

      LWDEBUGF(1, "Edge %" LWTFMT_ELEMID
                  " does not contain projected point to it",
                  edge_id);

#if 0
      /* In order to reduce the robustness issues, we'll pick
       * an edge that contains the projected point, if possible */
      if ( i+1 < num )
      {
        LWDEBUG(1, "But there's another to check");
        //lwgeom_free(prj);
        continue;
      }
#endif

      snapedge = _lwt_toposnap(g, prj, tol);
      snapline = lwgeom_as_lwline(snapedge);

      LWDEBUGF(1, "Edge snapped with tolerance %g", tol);

      /* TODO: check if snapping did anything ? */
#if POSTGIS_DEBUG_LEVEL > 0
      {
      size_t sz;
      char *wkt1 = lwgeom_to_wkt(g, WKT_EXTENDED, 15, &sz);
      char *wkt2 = lwgeom_to_wkt(snapedge, WKT_EXTENDED, 15, &sz);
      LWDEBUGF(1, "Edge %s snapped became %s", wkt1, wkt2);
      lwfree(wkt1);
      lwfree(wkt2);
      }
#endif


      /*
      -- Snapping currently snaps the first point below tolerance
      -- so may possibly move first point. See ticket #1631
      */
      getPoint4d_p(e->geom->points, 0, &p1);
      getPoint4d_p(snapline->points, 0, &p2);
      LWDEBUGF(1, "Edge first point is %g %g, "
                  "snapline first point is %g %g",
                  p1.x, p1.y, p2.x, p2.y);
      if ( p1.x != p2.x || p1.y != p2.y )
      {
        LWDEBUG(1, "Snapping moved first point, re-adding it");
        if ( LW_SUCCESS != ptarray_insert_point(snapline->points, &p1, 0) )
        {
          lwgeom_free(prj);
          lwgeom_free(snapedge);
          lwerror("GEOS exception on Contains: %s", lwgeom_geos_errmsg);
          return -1;
        }
#if POSTGIS_DEBUG_LEVEL > 0
        {
        size_t sz;
        char *wkt1 = lwgeom_to_wkt(g, WKT_EXTENDED, 15, &sz);
        LWDEBUGF(1, "Tweaked snapline became %s", wkt1);
        lwfree(wkt1);
        }
#endif
      }
#if POSTGIS_DEBUG_LEVEL > 0
      else {
        LWDEBUG(1, "Snapping did not move first point");
      }
#endif

      if ( -1 == lwt_ChangeEdgeGeom( topo, edge_id, snapline ) )
      {
        /* TODO: should have invoked lwerror already, leaking memory */
        lwgeom_free(prj);
        lwgeom_free(snapedge);
        lwerror("lwt_ChangeEdgeGeom failed");
        return -1;
      }
      lwgeom_free(snapedge);
    }}
#if POSTGIS_DEBUG_LEVEL > 0
    else
    {{
      size_t sz;
      char *wkt1 = lwgeom_to_wkt(g, WKT_EXTENDED, 15, &sz);
      char *wkt2 = lwgeom_to_wkt(prj, WKT_EXTENDED, 15, &sz);
      LWDEBUGF(1, "Edge %s contains projected point %s", wkt1, wkt2);
      lwfree(wkt1);
      lwfree(wkt2);
    }}
#endif

    /* TODO: pass 1 as last argument (skipChecks) ? */
    node.node_id = lwt_ModEdgeSplit( topo, edge_id, node.geom, 0 );
    if ( -1 == node.node_id )
    {
      /* TODO: should have invoked lwerror already, leaking memory */
      lwgeom_free(prj);
      lwerror("lwt_ModEdgeSplit failed");
      return -1;
    }

    /*
     * TODO: decimate the two new edges with the given tolerance ?
     *
     * the edge identifiers to decimate would be: edge_id and "id"
     * The problem here is that decimation of existing edges
     * may introduce intersections or topological inconsistencies,
     * for example:
     *
     *  - A node may end up falling on the other side of the edge
     *  - The decimated edge might intersect another existing edge
     *
     */

  }

  lwgeom_free(prj);

  if ( edges2 ) lwfree(edges2);
  return node.node_id;
}

/*
 * @param findFace if non-zero the code will determine which face
 *        contains the given point (unless it is known to be NOT
 *        isolated)
 * @param moved if not-null will be set to 0 if the point was added
 *              w/out any snapping or 1 otherwise.
 */
static LWT_ELEMID
_lwt_AddPoint(LWT_TOPOLOGY* topo, LWPOINT* point, double tol, int
              findFace, int *moved)
{
  uint64_t num, i;
  double mindist = FLT_MAX;
  LWT_ISO_NODE *nodes, *nodes2;
  LWT_ISO_EDGE *edges;
  LWGEOM *pt = lwpoint_as_lwgeom(point);
  scored_pointer *sorted;
  int flds;
  LWT_ELEMID id = 0;

  /* Get tolerance, if 0 was given */
  if (!tol)
    tol = _LWT_MINTOLERANCE(topo, pt);

  LWDEBUGG(1, pt, "Adding point");

  /*
  -- 1. Check if any existing node is closer than the given precision
  --    and if so pick the closest
  TODO: use WithinBox2D
  */
  flds = LWT_COL_NODE_NODE_ID | LWT_COL_NODE_GEOM;
  nodes = lwt_be_getNodeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  if ( num )
  {
    LWDEBUGF(1, "New point is within %.15g units of %llu nodes", tol, num);
    /* Order by distance if there are more than a single return */
    if ( num > 1 )
    {{
      sorted = lwalloc(sizeof(scored_pointer)*num);
      for (i=0; i<num; ++i)
      {
        sorted[i].ptr = nodes+i;
        sorted[i].score = lwgeom_mindistance2d(lwpoint_as_lwgeom(nodes[i].geom), pt);
        LWDEBUGF(1, "Node %" LWTFMT_ELEMID " distance: %.15g",
          ((LWT_ISO_NODE*)(sorted[i].ptr))->node_id, sorted[i].score);
      }
      qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
      nodes2 = lwalloc(sizeof(LWT_ISO_NODE)*num);
      for (i=0; i<num; ++i)
      {
        nodes2[i] = *((LWT_ISO_NODE*)sorted[i].ptr);
      }
      lwfree(sorted);
      lwfree(nodes);
      nodes = nodes2;
    }}

    for ( i=0; i<num; ++i )
    {
      LWT_ISO_NODE *n = &(nodes[i]);
      LWGEOM *g = lwpoint_as_lwgeom(n->geom);
      double dist = lwgeom_mindistance2d(g, pt);
      /* TODO: move this check in the previous sort scan ... */
      /* must be closer than tolerated, unless distance is zero */
      if ( dist && dist >= tol ) continue;
      if ( ! id || dist < mindist )
      {
        id = n->node_id;
        mindist = dist;
      }
    }
    if ( id )
    {
      /* found an existing node */
      if ( nodes ) _lwt_release_nodes(nodes, num);
      if ( moved ) *moved = mindist == 0 ? 0 : 1;
      return id;
    }
  }

  initGEOS(lwnotice, lwgeom_geos_error);

  /*
  -- 2. Check if any existing edge falls within tolerance
  --    and if so split it by a point projected on it
  TODO: use WithinBox2D
  */
  flds = LWT_COL_EDGE_ALL; /* EDGE_ID|LWT_COL_EDGE_GEOM;*/
  edges = lwt_be_getEdgeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if (num == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    return -1;
  }
  LWDEBUGF(1, "New point is within %.15g units of %llu edges", tol, num);
  if ( num )
  {
    id = _lwt_SplitAllEdgesToNewNode(topo, edges, num, lwgeom_as_lwpoint(pt), tol, moved);
    _lwt_release_edges(edges, num);
  }

  if ( id == 0 )
  {
    /* The point is isolated, add it as such */
    /* TODO: pass 1 as last argument (skipChecks) ? */
    id = _lwt_AddIsoNode(topo, -1, point, 0, findFace);
    if ( moved ) *moved = 0;
    if ( -1 == id )
    {
      /* should have invoked lwerror already, leaking memory */
      _lwt_release_edges(edges, num);
      lwerror("lwt_AddIsoNode failed");
      return -1;
    }
  }

  return id;
}

LWT_ELEMID
lwt_AddPoint(LWT_TOPOLOGY* topo, LWPOINT* point, double tol)
{
  return _lwt_AddPoint(topo, point, tol, 1, NULL);
}

/*
 * Add a pre-noded pre-split line edge. Used by lwt_AddLine
 * Return edge id, 0 if none added (empty edge), -1 on error
 *
 * @param handleFaceSplit if non-zero the code will check
 *        if the newly added edge would split a face and if so
 *        would create new faces accordingly. Otherwise it will
 *        set left_face and right_face to null (-1)
 *
 * @param forward output parameter, will be populated if
 *        a pre-existing edge was found in the topology,
 *        in which case a value of 1 means the incoming
 *        line will have the same direction of the edge,
 *        and 0 that the incomine line has opposite direction
 */
static LWT_ELEMID
_lwt_AddLineEdge( LWT_TOPOLOGY* topo, LWLINE* edge, double tol,
                  int handleFaceSplit, int *forward )
{
  LWCOLLECTION *col;
  LWPOINT *start_point, *end_point;
  LWGEOM *tmp = 0, *tmp2;
  LWT_ISO_NODE *node;
  LWT_ELEMID nid[2]; /* start_node, end_node */
  LWT_ELEMID id; /* edge id */
  POINT4D p4d;
  uint64_t nn, i;
  int moved=0, mm;

  LWDEBUGG(1, lwline_as_lwgeom(edge), "_lwtAddLineEdge");
  LWDEBUGF(1, "_lwtAddLineEdge with tolerance %g", tol);

  start_point = lwline_get_lwpoint(edge, 0);
  if ( ! start_point )
  {
    lwnotice("Empty component of noded line");
    return 0; /* must be empty */
  }
  nid[0] = _lwt_AddPoint( topo, start_point,
                          _lwt_minTolerance(lwpoint_as_lwgeom(start_point)),
                          handleFaceSplit, &mm );
  lwpoint_free(start_point); /* too late if lwt_AddPoint calls lwerror */
  if ( nid[0] == -1 ) return -1; /* lwerror should have been called */
  moved += mm;
  LWDEBUGF(1, "node for start point added or found to be %" LWTFMT_ELEMID " (moved ? %d)", nid[0], mm);


  end_point = lwline_get_lwpoint(edge, edge->points->npoints-1);
  if ( ! end_point )
  {
    lwerror("could not get last point of line "
            "after successfully getting first point !?");
    return -1;
  }
  nid[1] = _lwt_AddPoint( topo, end_point,
                          _lwt_minTolerance(lwpoint_as_lwgeom(end_point)),
                          handleFaceSplit, &mm );
  lwpoint_free(end_point); /* too late if lwt_AddPoint calls lwerror */
  if ( nid[1] == -1 ) return -1; /* lwerror should have been called */
  moved += mm;
  LWDEBUGF(1, "node for end point added or found to be %" LWTFMT_ELEMID " (moved ? %d)", nid[1], mm);

  /*
    -- Added endpoints may have drifted due to tolerance, so
    -- we need to re-snap the edge to the new nodes before adding it
  */
  if ( moved )
  {

    LWDEBUG(1, "One or both line endpoints moved by snap, updating line");

    nn = nid[0] == nid[1] ? 1 : 2;
    node = lwt_be_getNodeById( topo, nid, &nn,
                               LWT_COL_NODE_NODE_ID|LWT_COL_NODE_GEOM );
    if (nn == UINT64_MAX)
    {
      PGTOPO_BE_ERROR();
      return -1;
    }
    start_point = NULL; end_point = NULL;
    for (i=0; i<nn; ++i)
    {
      if ( node[i].node_id == nid[0] ) start_point = node[i].geom;
      if ( node[i].node_id == nid[1] ) end_point = node[i].geom;
    }
    if ( ! start_point  || ! end_point )
    {
      if ( nn ) _lwt_release_nodes(node, nn);
      lwerror("Could not find just-added nodes % " LWTFMT_ELEMID
              " and %" LWTFMT_ELEMID, nid[0], nid[1]);
      return -1;
    }

    /* snap */

    getPoint4d_p( start_point->point, 0, &p4d );
    lwline_setPoint4d(edge, 0, &p4d);

    getPoint4d_p( end_point->point, 0, &p4d );
    lwline_setPoint4d(edge, edge->points->npoints-1, &p4d);

    if ( nn ) _lwt_release_nodes(node, nn);

    LWDEBUGG(2, lwline_as_lwgeom(edge), "Snapped after drifted endpoints snap");

    /* make valid, after snap (to handle collapses) */
    tmp = lwgeom_make_valid(lwline_as_lwgeom(edge));

    LWDEBUGG(2, tmp, "Made-valid after snap to drifted endpoints");

    col = lwgeom_as_lwcollection(tmp);
    if ( col )
    {{

      LWCOLLECTION *colex = lwcollection_extract(col, LINETYPE);

      /* Check if the so-snapped edge collapsed (see #1650) */
      if ( colex->ngeoms == 0 )
      {
        lwcollection_free(colex);
        lwgeom_free(tmp);
        LWDEBUG(1, "Made-valid snapped edge collapsed");
        return 0;
      }

      tmp2 = lwgeom_clone_deep(colex->geoms[0]);
      lwgeom_free(tmp);
      tmp = tmp2;
      edge = lwgeom_as_lwline(tmp);
      lwcollection_free(colex);
      if ( ! edge )
      {
        /* should never happen */
        lwerror("lwcollection_extract(LINETYPE) returned a non-line?");
        return -1;
      }
    }}
    else
    {
      edge = lwgeom_as_lwline(tmp);
      if ( ! edge )
      {
        LWDEBUGF(1, "Made-valid snapped edge collapsed to %s",
                    lwtype_name(lwgeom_get_type(tmp)));
        lwgeom_free(tmp);
        return 0;
      }
    }

  }

  /* check if the so-snapped edge _now_ exists */
  id = _lwt_GetEqualEdge ( topo, edge, forward );
  LWDEBUGF(1, "_lwt_GetEqualEdge returned %" LWTFMT_ELEMID, id);
  if ( id == -1 )
  {
    if ( tmp ) lwgeom_free(tmp); /* probably too late, due to internal lwerror */
    return -1;
  }
  if ( id )
  {
    if ( tmp ) lwgeom_free(tmp); /* possibly takes "edge" down with it */
    return id;
  }

  /* No previously existing edge was found, we'll add one */

  /* Remove consecutive vertices below given tolerance
   * on edge addition */
  if ( tol )
  {{
    tmp2 = lwline_remove_repeated_points(edge, tol);
    LWDEBUGG(2, tmp2, "Repeated-point removed");
    edge = lwgeom_as_lwline(tmp2);
    if ( tmp ) lwgeom_free(tmp);
    tmp = tmp2;

    /* check if the so-decimated edge collapsed to single-point */
    if ( nid[0] == nid[1] && edge->points->npoints == 2 )
    {
      lwgeom_free(tmp);
      LWDEBUG(1, "Repeated-point removed edge collapsed");
      return 0;
    }

    /* check if the so-decimated edge _now_ exists */
    id = _lwt_GetEqualEdge ( topo, edge, forward );
    LWDEBUGF(1, "_lwt_GetEqualEdge returned %" LWTFMT_ELEMID, id);
    if ( id == -1 )
    {
      lwgeom_free(tmp); /* probably too late, due to internal lwerror */
      return -1;
    }
    if ( id )
    {
      lwgeom_free(tmp); /* takes "edge" down with it */
      return id;
    }
  }}


  /* TODO: skip checks ? */
  id = _lwt_AddEdge( topo, nid[0], nid[1], edge, 0, handleFaceSplit ?  1 : -1 );
  LWDEBUGF(1, "lwt_AddEdgeModFace returned %" LWTFMT_ELEMID, id);
  if ( id == -1 )
  {
    lwgeom_free(tmp); /* probably too late, due to internal lwerror */
    return -1;
  }
  lwgeom_free(tmp); /* possibly takes "edge" down with it */

  *forward = 1;
  return id;
}

/* Simulate split-loop as it was implemented in pl/pgsql version
 * of TopoGeo_addLinestring */
static LWGEOM *
_lwt_split_by_nodes(const LWGEOM *g, const LWGEOM *nodes)
{
  LWCOLLECTION *col = lwgeom_as_lwcollection(nodes);
  uint32_t i;
  LWGEOM *bg;

  bg = lwgeom_clone_deep(g);
  if ( ! col->ngeoms ) return bg;

  for (i=0; i<col->ngeoms; ++i)
  {
    LWGEOM *g2;
    g2 = lwgeom_split(bg, col->geoms[i]);
    lwgeom_free(bg);
    bg = g2;
  }
  bg->srid = nodes->srid;

  return bg;
}

static LWT_ELEMID*
_lwt_AddLine(LWT_TOPOLOGY* topo, LWLINE* line, double tol, int* nedges,
            int handleFaceSplit)
{
  LWGEOM *geomsbuf[1];
  LWGEOM **geoms;
  uint32_t ngeoms;
  LWGEOM *noded, *tmp;
  LWCOLLECTION *col;
  LWT_ELEMID *ids;
  LWT_ISO_EDGE *edges;
  LWT_ISO_NODE *nodes;
  uint64_t num, numedges = 0, numnodes = 0;
  uint64_t i;
  GBOX qbox;
  int forward;
  int input_was_closed = 0;
  POINT4D originalStartPoint;

  if ( lwline_is_empty(line) )
  {
    *nedges = 0;
    return NULL;
  }

  if ( lwline_is_closed(line) )
  {
    input_was_closed = 1;
    getPoint4d_p( line->points, 0, &originalStartPoint);
    LWDEBUGF(1, "Input line is closed, original point is %g,%g", originalStartPoint.x, originalStartPoint.y);
  }

  *nedges = -1; /* error condition, by default */

  /* Get tolerance, if 0 was given */
  if ( ! tol ) tol = _LWT_MINTOLERANCE( topo, (LWGEOM*)line );
  LWDEBUGF(1, "Working tolerance:%.15g", tol);
  LWDEBUGF(1, "Input line has srid=%d", line->srid);

  /* Remove consecutive vertices below given tolerance upfront */
  if ( tol )
  {{
    LWLINE *clean = lwgeom_as_lwline(lwline_remove_repeated_points(line, tol));
    tmp = lwline_as_lwgeom(clean); /* NOTE: might collapse to non-simple */
    LWDEBUGG(1, tmp, "Repeated-point removed");
  }} else tmp=(LWGEOM*)line;

  /* 1. Self-node */
  noded = lwgeom_node((LWGEOM*)tmp);
  if ( tmp != (LWGEOM*)line ) lwgeom_free(tmp);
  if ( ! noded ) return NULL; /* should have called lwerror already */
  LWDEBUGG(1, noded, "Noded");

  qbox = *lwgeom_get_bbox( lwline_as_lwgeom(line) );
  LWDEBUGF(1, "Line BOX is %.15g %.15g, %.15g %.15g", qbox.xmin, qbox.ymin,
                                          qbox.xmax, qbox.ymax);
  gbox_expand(&qbox, tol);
  LWDEBUGF(1, "BOX expanded by %g is %.15g %.15g, %.15g %.15g",
              tol, qbox.xmin, qbox.ymin, qbox.xmax, qbox.ymax);

  LWGEOM **nearby = 0;
  int nearbyindex = 0;
  int nearbycount = 0;

  /* 2.0. Find edges falling within tol distance */
  edges = lwt_be_getEdgeWithinBox2D( topo, &qbox, &numedges, LWT_COL_EDGE_ALL, 0 );
  if (numedges == UINT64_MAX)
  {
    lwgeom_free(noded);
    PGTOPO_BE_ERROR();
    return NULL;
  }
  LWDEBUGF(1, "Line has %u points, its bbox intersects %llu edges bboxes",
    line->points->npoints, numedges);
  if ( numedges )
  {{
    /* TODO: compute earlier for reuse later ? */
    GEOSGeometry *noded_g = LWGEOM2GEOS(noded, 0);
    /* collect those whose distance from us is < tol */
    nearbycount += numedges;
    nearby = lwalloc(numedges * sizeof(LWGEOM *));
    for (i=0; i<numedges; ++i)
    {{
      LW_ON_INTERRUPT(return NULL);
      LWT_ISO_EDGE *e = &(edges[i]);
      LWGEOM *g = lwline_as_lwgeom(e->geom);
      GEOSGeometry *edge_g = LWGEOM2GEOS(g, 0);
      LWDEBUGF(2, "Computing distance from edge %" LWTFMT_ELEMID " with %u points", i, e->geom->points->npoints);
      double dist;
      if ( 0 == GEOSDistanceIndexed(edge_g, noded_g, &dist) ) {
        GEOSGeom_destroy(edge_g);
        GEOSGeom_destroy(noded_g);
        lwgeom_free(noded);
        lwerror("GEOSDistanceIndexed error: %s", lwgeom_geos_errmsg);
        return NULL;
      }
      GEOSGeom_destroy(edge_g);
      if ( dist && dist >= tol ) continue;
      nearby[nearbyindex++] = g;
    }}
    LWDEBUGF(1, "Found %d edges closer than tolerance (%g)", nearbyindex, tol);
    GEOSGeom_destroy(noded_g);
  }}
  int nearbyedgecount = nearbyindex;

  /* 2.1. Find isolated nodes falling within tol distance
   *
   * TODO: add backend-interface support for only getting isolated nodes
   */
  nodes = lwt_be_getNodeWithinBox2D( topo, &qbox, &numnodes, LWT_COL_NODE_ALL, 0 );
  if (numnodes == UINT64_MAX)
  {
    lwgeom_free(noded);
    PGTOPO_BE_ERROR();
    return NULL;
  }
  LWDEBUGF(1, "Line bbox intersects %llu nodes bboxes", numnodes);
  if ( numnodes )
  {{
    /* collect those whose distance from us is < tol */
    nearbycount = nearbyedgecount + numnodes;
    nearby = nearby ?
        lwrealloc(nearby, nearbycount * sizeof(LWGEOM *))
        :
        lwalloc(nearbycount * sizeof(LWGEOM *))
        ;
    int nn = 0;
    for (i=0; i<numnodes; ++i)
    {
      LWT_ISO_NODE *n = &(nodes[i]);
      if ( n->containing_face == -1 ) continue; /* skip not-isolated nodes */
      LWGEOM *g = lwpoint_as_lwgeom(n->geom);
      double dist = lwgeom_mindistance2d(g, noded);
      /* must be closer than tolerated, unless distance is zero */
      if ( dist && dist >= tol )
      {
        LWDEBUGF(1, "Node %" LWTFMT_ELEMID " is %g units away, we tolerate only %g", n->node_id, dist, tol);
        continue;
      }
      nearby[nearbyindex++] = g;
      nn = nn + 1;
    }
    LWDEBUGF(1, "Found %d isolated nodes closer than tolerance (%g)", nn, tol);
  }}
  int nearbynodecount = nearbyindex - nearbyedgecount;
  nearbycount = nearbyindex;

  LWDEBUGF(1, "Number of nearby elements is %d", nearbycount);

  /* 2.2. Snap to nearby elements */
  if ( nearbycount )
  {{
    LWCOLLECTION *col;
    LWGEOM *elems;

    col = lwcollection_construct(COLLECTIONTYPE, topo->srid,
                                 NULL, nearbycount, nearby);
    elems = lwcollection_as_lwgeom(col);

    LWDEBUGG(1, elems, "Collected nearby elements");

    tmp = _lwt_toposnap(noded, elems, tol);
    lwgeom_free(noded);
    noded = tmp;
    LWDEBUGG(1, noded, "Elements-snapped");
    if ( input_was_closed )
    {{
      /* Recompute start point in case it moved */
      LWLINE *scrolled = lwgeom_as_lwline(noded);
      if (scrolled)
      {
        getPoint4d_p( scrolled->points, 0, &originalStartPoint);
        LWDEBUGF(1, "Closed input line start point after snap %g,%g", originalStartPoint.x, originalStartPoint.y);
      }
    }}

    /* will not release the geoms array */
    lwcollection_release(col);

    /*
    -- re-node to account for ST_Snap introduced self-intersections
    -- See http://trac.osgeo.org/postgis/ticket/1714
    -- TODO: consider running UnaryUnion once after all noding
    */
    tmp = lwgeom_unaryunion(noded);
    lwgeom_free(noded);
    noded = tmp;
    LWDEBUGG(1, noded, "Unary-unioned");
  }}

  /* 2.3. Node with nearby edges */
  if ( nearbyedgecount )
  {{
    LWCOLLECTION *col;
    LWGEOM *iedges; /* just an alias for col */
    LWGEOM *diff, *xset;

    LWDEBUGF(1, "Line intersects %d edges", nearbyedgecount);

    col = lwcollection_construct(COLLECTIONTYPE, topo->srid,
                                 NULL, nearbyedgecount, nearby);
    iedges = lwcollection_as_lwgeom(col);
    LWDEBUGG(1, iedges, "Collected edges");

    LWDEBUGF(1, "Diffing noded, with srid=%d "
                "and intersecting edges, with srid=%d",
                noded->srid, iedges->srid);
    diff = lwgeom_difference(noded, iedges);
    LWDEBUGG(1, diff, "Differenced");

    LWDEBUGF(1, "Intersecting noded, with srid=%d "
                "and intersecting edges, with srid=%d",
                noded->srid, iedges->srid);
    xset = lwgeom_intersection(noded, iedges);
    LWDEBUGG(1, xset, "Intersected");
    lwgeom_free(noded);

    /* We linemerge here because INTERSECTION, as of GEOS 3.8,
     * will result in shared segments being output as multiple
     * lines rather than a single line. Example:

          INTERSECTION(
            'LINESTRING(0 0, 5 0, 8 0, 10 0,12 0)',
            'LINESTRING(5 0, 8 0, 10 0)'
          )
          ==
          MULTILINESTRING((5 0,8 0),(8 0,10 0))

     * We will re-split in a subsequent step, by splitting
     * the final line with pre-existing nodes
     */
    LWDEBUG(1, "Linemerging intersection");
    tmp = lwgeom_linemerge(xset);
    LWDEBUGG(1, tmp, "Linemerged");
    lwgeom_free(xset);
    xset = tmp;

    /*
     * Here we union the (linemerged) intersection with
     * the difference (new lines)
     */
    LWDEBUG(1, "Unioning difference and (linemerged) intersection");
    noded = lwgeom_union(diff, xset);
    LWDEBUGG(1, noded, "Diff-Xset Unioned");
    lwgeom_free(xset);
    lwgeom_free(diff);

    /* will not release the geoms array */
    lwcollection_release(col);

    if ( input_was_closed )
    {{
      LWLINE *scrolled = lwgeom_as_lwline(noded);
      if (scrolled) {
        if ( lwline_is_closed(scrolled) ) {
          ptarray_scroll_in_place(scrolled->points, &originalStartPoint);
        }
        else {
          LWDEBUGG(1, lwline_as_lwgeom(scrolled), "Closed input became non closed");
        }
      }
      else {
        LWDEBUGG(1, noded, "Diff-Xset Unioned cannot be scrolled");
      }
    }}


  }}


  /* 2.4. Split by pre-existing nodes
   *
   * Pre-existing nodes are isolated nodes AND endpoints
   * of intersecting edges
   */
  if ( nearbyedgecount )
  {
    nearbycount += nearbyedgecount * 2; /* make space for endpoints */
    nearby = lwrealloc(nearby, nearbycount * sizeof(LWGEOM *));
    for (int i=0; i<nearbyedgecount; i++)
    {
      LWLINE *edge = lwgeom_as_lwline(nearby[i]);
      LWPOINT *startNode = lwline_get_lwpoint(edge, 0);
      LWPOINT *endNode = lwline_get_lwpoint(edge, edge->points->npoints-1);
      /* TODO: only add if within distance to noded AND if not duplicated */
      nearby[nearbyindex++] = lwpoint_as_lwgeom(startNode);
      nearbynodecount++;
      nearby[nearbyindex++] = lwpoint_as_lwgeom(endNode);
      nearbynodecount++;
    }
  }
  if ( nearbynodecount )
  {
    col = lwcollection_construct(MULTIPOINTTYPE, topo->srid,
                             NULL, nearbynodecount,
                             nearby + nearbyedgecount);
    LWGEOM *inodes = lwcollection_as_lwgeom(col);
    /* TODO: use lwgeom_split of lwgeom_union ... */
    tmp = _lwt_split_by_nodes(noded, inodes);
    lwgeom_free(noded);
    noded = tmp;
    LWDEBUGG(1, noded, "Node-split");
    /* will not release the geoms array */
    lwcollection_release(col);
  }


  LWDEBUG(1, "Freeing up nearby elements");

  /* TODO: free up endpoints of nearbyedges */
  if ( nearby ) lwfree(nearby);
  if ( nodes ) _lwt_release_nodes(nodes, numnodes);
  if ( edges ) _lwt_release_edges(edges, numedges);

  LWDEBUGG(2, noded, "Finally-noded");

  /* 3. For each (now-noded) segment, insert an edge */
  col = lwgeom_as_lwcollection(noded);
  if ( col )
  {
    LWDEBUG(1, "Noded line was a collection");
    geoms = col->geoms;
    ngeoms = col->ngeoms;
  }
  else
  {
    LWDEBUG(1, "Noded line was a single geom");
    geomsbuf[0] = noded;
    geoms = geomsbuf;
    ngeoms = 1;
  }

  LWDEBUGF(1, "Line was split into %d edges", ngeoms);

  /* TODO: refactor to first add all nodes (re-snapping edges if
   * needed) and then check all edges for existing already
   * ( so to save a DB scan for each edge to be added )
   */
  ids = lwalloc(sizeof(LWT_ELEMID)*ngeoms);
  num = 0;
  for ( i=0; i<ngeoms; ++i )
  {
    LWT_ELEMID id;
    LWGEOM *g = geoms[i];
    g->srid = noded->srid;

#if POSTGIS_DEBUG_LEVEL > 0
    {
      size_t sz;
      char *wkt1 = lwgeom_to_wkt(g, WKT_EXTENDED, 15, &sz);
      LWDEBUGF(1, "Component %llu of split line is: %s", i, wkt1);
      lwfree(wkt1);
    }
#endif

    id = _lwt_AddLineEdge( topo, lwgeom_as_lwline(g), tol, handleFaceSplit, &forward );
    LWDEBUGF(1, "_lwt_AddLineEdge returned %" LWTFMT_ELEMID, id);
    if ( id < 0 )
    {
      lwgeom_free(noded);
      lwfree(ids);
      return NULL;
    }
    if ( ! id )
    {
      LWDEBUGF(1, "Component %llu of split line collapsed", i);
      continue;
    }

    LWDEBUGF(1, "Component %llu of split line is %s edge %" LWTFMT_ELEMID,
                  i, forward ? "forward" : "backward", id);
    ids[num++] = forward ? id : -id; /* TODO: skip duplicates */
  }

  LWDEBUGG(2, noded, "Noded before free");
  lwgeom_free(noded);

  /* TODO: XXX remove duplicated ids if not done before */

  *nedges = num;
  return ids;
}

LWT_ELEMID*
lwt_AddLine(LWT_TOPOLOGY* topo, LWLINE* line, double tol, int* nedges)
{
  return _lwt_AddLine(topo, line, tol, nedges, 1);
}

LWT_ELEMID*
lwt_AddLineNoFace(LWT_TOPOLOGY* topo, LWLINE* line, double tol, int* nedges)
{
  return _lwt_AddLine(topo, line, tol, nedges, 0);
}

static void
lwt_LoadPoint(LWT_TOPOLOGY* topo, LWPOINT* point, double tol)
{
  _lwt_AddPoint(topo, point, tol, 1, NULL);
}

static void
lwt_LoadLine(LWT_TOPOLOGY* topo, LWLINE* line, double tol)
{
  LWT_ELEMID* ids;
  int nedges;

  /* TODO: avoid allocating edge ids */
  ids = lwt_AddLine(topo, line, tol, &nedges);
  if ( nedges > 0 ) lwfree(ids);
}

static void
lwt_LoadPolygon(LWT_TOPOLOGY* topo, const LWPOLY* poly, double tol)
{
  uint32_t i;

  /* Add each ring as an edge */
  for ( i=0; i<poly->nrings; ++i )
  {
    LWLINE *line;
    POINTARRAY *pa;

    /* TODO: avoid the clone here */
    pa = ptarray_clone(poly->rings[i]);
    line = lwline_construct(topo->srid, NULL, pa);
    lwt_LoadLine(topo, line, tol);
    lwline_free(line);
  }
}

LWT_ELEMID*
lwt_AddPolygon(LWT_TOPOLOGY* topo, LWPOLY* poly, double tol, int* nfaces)
{
  *nfaces = -1; /* error condition, by default */
  int num;
  LWT_ISO_FACE *faces;
  uint64_t nfacesinbox;
  uint64_t j;
  LWT_ELEMID *ids = NULL;
  GBOX qbox;
  const GEOSPreparedGeometry *ppoly;
  GEOSGeometry *polyg;

  /* Nothing to add, in an empty polygon */
  if ( lwpoly_is_empty(poly) )
  {
    *nfaces = 0;
    return NULL;
  }

  /* Get tolerance, if 0 was given */
  if ( ! tol ) tol = _LWT_MINTOLERANCE( topo, (LWGEOM*)poly );
  LWDEBUGF(1, "Working tolerance:%.15g", tol);

  lwt_LoadPolygon(topo, poly, tol);

  /*
  -- Find faces covered by input polygon
  -- NOTE: potential snapping changed polygon edges
  */
  qbox = *lwgeom_get_bbox( lwpoly_as_lwgeom(poly) );
  gbox_expand(&qbox, tol);
  faces = lwt_be_getFaceWithinBox2D( topo, &qbox, &nfacesinbox,
                                     LWT_COL_FACE_ALL, 0 );
  if (nfacesinbox == UINT64_MAX)
  {
    lwfree(ids);
    PGTOPO_BE_ERROR();
    return NULL;
  }

  num = 0;
  if ( nfacesinbox )
  {
    polyg = LWGEOM2GEOS(lwpoly_as_lwgeom(poly), 0);
    if ( ! polyg )
    {
      _lwt_release_faces(faces, nfacesinbox);
      lwerror("Could not convert poly geometry to GEOS: %s", lwgeom_geos_errmsg);
      return NULL;
    }
    ppoly = GEOSPrepare(polyg);
    ids = lwalloc(sizeof(LWT_ELEMID)*nfacesinbox);
    for ( j=0; j<nfacesinbox; ++j )
    {
      LWT_ISO_FACE *f = &(faces[j]);
      LWGEOM *fg;
      GEOSGeometry *fgg, *sp;
      int covers;

      /* check if a point on this face surface is covered by our polygon */
      fg = lwt_GetFaceGeometry( topo, f->face_id );
      if ( ! fg )
      {
        j = f->face_id; /* so we can destroy faces */
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        lwfree(ids);
        _lwt_release_faces(faces, nfacesinbox);
        lwerror("Could not get geometry of face %" LWTFMT_ELEMID, j);
        return NULL;
      }
      /* check if a point on this face's surface is covered by our polygon */
      fgg = LWGEOM2GEOS(fg, 0);
      lwgeom_free(fg);
      if ( ! fgg )
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _lwt_release_faces(faces, nfacesinbox);
        lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
        return NULL;
      }
      sp = GEOSPointOnSurface(fgg);
      GEOSGeom_destroy(fgg);
      if ( ! sp )
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _lwt_release_faces(faces, nfacesinbox);
        lwerror("Could not find point on face surface: %s", lwgeom_geos_errmsg);
        return NULL;
      }
      covers = GEOSPreparedCovers( ppoly, sp );
      GEOSGeom_destroy(sp);
      if (covers == 2)
      {
        GEOSPreparedGeom_destroy(ppoly);
        GEOSGeom_destroy(polyg);
        _lwt_release_faces(faces, nfacesinbox);
        lwerror("PreparedCovers error: %s", lwgeom_geos_errmsg);
        return NULL;
      }
      if ( ! covers )
      {
        continue; /* we're not composed by this face */
      }

      /* TODO: avoid duplicates ? */
      ids[num++] = f->face_id;
    }
    GEOSPreparedGeom_destroy(ppoly);
    GEOSGeom_destroy(polyg);
    _lwt_release_faces(faces, nfacesinbox);
  }

  /* possibly 0 if non face's surface point was found
   * to be covered by input polygon */
  *nfaces = num;

  return ids;
}

LWT_ELEMID
lwt_GetFaceContainingPoint(LWT_TOPOLOGY* topo, const LWPOINT* pt)
{
  LWT_ISO_EDGE* closestEdge;
  LWT_ISO_EDGE* edges;
  uint64_t numedges, i;
  const POINT2D *queryPoint;
  const POINT2D *closestPointOnEdge = NULL;
  uint32_t closestSegmentIndex;
  int closestSegmentSide;
  uint32_t closestPointVertex;
  const POINT2D *closestSegmentP0, *closestSegmentP1;
  LWT_ELEMID closestNode = 0;
  double dist;
  LWT_ELEMID containingFace = -1;

  closestEdge = lwt_be_getClosestEdge( topo, pt, &numedges,
    LWT_COL_EDGE_GEOM|
    LWT_COL_EDGE_EDGE_ID|
    LWT_COL_EDGE_START_NODE|
    LWT_COL_EDGE_END_NODE|
    LWT_COL_EDGE_FACE_LEFT|
    LWT_COL_EDGE_FACE_RIGHT
  );
  if (numedges == UINT64_MAX)
  {
    PGTOPO_BE_ERROR();
    /* cberror(topo->be_data, "Error from cb_getClosestEdge"); */
    return -1;
  }
  if (numedges == 0)
  {
    /* If there are no edges the point is in the universal face */
    return 0;
  }

  if ( closestEdge->face_left < 0 )
  {
    lwerror("Closest edge %" LWTFMT_ELEMID " has invalid face %" LWTFMT_ELEMID
      " on its left side", closestEdge->edge_id, closestEdge->face_left);
    _lwt_release_edges(closestEdge, 1);
    return -1;
  }

  if ( closestEdge->face_right < 0 )
  {
    lwerror("Closest edge %" LWTFMT_ELEMID " has invalid face %" LWTFMT_ELEMID
      " on its right side", closestEdge->edge_id, closestEdge->face_right);
    _lwt_release_edges(closestEdge, 1);
    return -1;
  }

  if ( closestEdge->geom->points->npoints < 2 )
  {
    lwerror("Corrupted topology: geometry of edge %" LWTFMT_ELEMID " is EMPTY",
      closestEdge->edge_id);
    _lwt_release_edges(closestEdge, 1);
    return -1;
  }

  LWDEBUGGF(2, lwline_as_lwgeom(closestEdge->geom), "Closest edge %" LWTFMT_ELEMID, closestEdge->edge_id);

  /* Find closest segment of edge to the point */
  queryPoint = getPoint2d_cp(pt->point, 0);
  closestSegmentIndex = ptarray_closest_segment_2d(closestEdge->geom->points, queryPoint, &dist);
  LWDEBUGF(1, "Closest segment on edge %" LWTFMT_ELEMID " is %d (dist %g)", closestEdge->edge_id, closestSegmentIndex, dist);
  closestSegmentP0 = getPoint2d_cp(closestEdge->geom->points, closestSegmentIndex);
  closestSegmentP1 = getPoint2d_cp(closestEdge->geom->points, closestSegmentIndex + 1);
  LWDEBUGF(1, "Closest segment on edge %" LWTFMT_ELEMID " is LINESTRING(%g %g, %g %g)",
    closestEdge->edge_id,
    closestSegmentP0->x,
    closestSegmentP0->y,
    closestSegmentP1->x,
    closestSegmentP1->y
  );

  /*
   * We use comp.graphics.algorithms Frequently Asked Questions method
   *
   * (1)           AC dot AB
   *           r = ----------
   *                ||AB||^2
   *  r has the following meaning:
   *  r=0 P = A
   *  r=1 P = B
   *  r<0 P is on the backward extension of AB
   *  r>1 P is on the forward extension of AB
   *  0<r<1 P is interior to AB
   *
   */
  const POINT2D *p = queryPoint;
  const POINT2D *A = closestSegmentP0;
  const POINT2D *B = closestSegmentP1;
  double r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );
  if ( r <= 0 )
  {
    closestPointOnEdge = A;
    closestPointVertex = closestSegmentIndex;
    if ( closestSegmentIndex == 0 )
    {
      closestNode = closestEdge->start_node;
    }
  }
  else if (r >= 1 )
  {
    closestPointOnEdge = B;
    closestPointVertex = closestSegmentIndex + 1;
    if ( closestSegmentIndex + 2 == closestEdge->geom->points->npoints )
    {
      closestNode = closestEdge->end_node;
    }
  }
  else
  {
    closestPointVertex = closestEdge->geom->points->npoints;
  }

  if ( closestNode != 0 )
  {
    LWDEBUGF(1, "Closest point is node %" LWTFMT_ELEMID, closestNode);
    if ( dist == 0 )
    {
      LWDEBUGF(1, "Query point is node %" LWTFMT_ELEMID, closestNode);
      /* Query point is the node
       *
       * If all edges incident to the node are
       * dangling, we can return their common
       * side face, otherwise the point will be
       * on multiple face boundaries
       */
      if ( closestEdge->face_left != closestEdge->face_right )
      {
        _lwt_release_edges(closestEdge, 1);
        lwerror("Two or more faces found");
        return -1;
      }
      containingFace = closestEdge->face_left;

      /* Check other incident edges */
      numedges = 1;
      edges = lwt_be_getEdgeByNode( topo, &closestNode, &numedges, LWT_COL_EDGE_FACE_LEFT|LWT_COL_EDGE_FACE_RIGHT );
      if (numedges == UINT64_MAX)
      {
        PGTOPO_BE_ERROR();
        /* cberror(topo->be_data, "Error from cb_getClosestEdge"); */
        _lwt_release_edges(closestEdge, 1);
        return -1;
      }
      for (i=0; i<numedges; ++i)
      {
        if ( edges[i].face_left != containingFace ||
             edges[i].face_right != containingFace )
        {
          _lwt_release_edges(edges, numedges);
          _lwt_release_edges(closestEdge, 1);
          lwerror("Two or more faces found");
          return -1;
        }
      }
      if (numedges < 1 )
      {
        lwerror("Unexpected backend return: getEdgeByNode(%" LWTFMT_ELEMID
          ") returns no edges when we previously found edge %"
          LWTFMT_ELEMID " ending on that node",
          closestNode, closestEdge->edge_id);
        _lwt_release_edges(edges, numedges);
        _lwt_release_edges(closestEdge, 1);
        return -1;
      }
      LWDEBUGF(1, "lwt_be_getEdgeByNode returned %" PRIu64 " edges", numedges);
      _lwt_release_edges(edges, numedges);
      _lwt_release_edges(closestEdge, 1);
      return containingFace;
    }

    /* Closest point is a node, but query point is NOT on the node */

    /* let's do azimuth computation */
    edgeend ee;
    if ( ! azimuth_pt_pt(closestPointOnEdge, queryPoint, &ee.myaz) ) {
      lwerror("error computing azimuth of query point [%.15g %.15g,%.15g %.15g]",
              closestPointOnEdge->x, closestPointOnEdge->y,
              queryPoint->x, queryPoint->y);
      _lwt_release_edges(closestEdge, 1);
      return -1;
    }

    LWDEBUGF(1, "Query point azimuth is %g", ee.myaz);

    int found = _lwt_FindAdjacentEdges( topo, closestNode, &ee, NULL, -1 );
    if ( ! found ) {
        lwerror("Unexpected backend return: _lwt_FindAdjacentEdges(%"
          LWTFMT_ELEMID
          ") found no edges when we previously found edge %"
          LWTFMT_ELEMID " ending on that node",
          closestNode, closestEdge->edge_id);
        _lwt_release_edges(closestEdge, 1);
        return -1;
    }

    _lwt_release_edges(closestEdge, 1);
    return ee.cwFace;

  }

  LWDEBUG(1, "Closest point is NOT a node");

  /* If this edge has the same face on the left and right sides
   * we found the face containing our query point */
  if ( closestEdge->face_left == closestEdge->face_right )
  {
    LWDEBUGF(1, "Closest point is on a edge with face %" LWTFMT_ELEMID
      " on both sides", closestEdge->face_left);

    containingFace = closestEdge->face_left;
    _lwt_release_edges(closestEdge, 1);
    return containingFace;
  }

  if ( dist == 0 )
  {
    /* We checked the dangling case above */
    _lwt_release_edges(closestEdge, 1);
    lwerror("Two or more faces found");
    return -1;
  }

  /* Find on which side of the segment the query point lays */
  if ( closestPointVertex != closestEdge->geom->points->npoints )
  {
    /* Closest point is a vertex of the closest segment */
    LWDEBUGF(1, "Closest point is vertex %d of closest segment", closestPointVertex);

    /*
     * We need to check if rotating clockwise the line
     * from previous vertex to closest vertex clockwise
     * around the closest vertex encounters
     * the line to query point first (which means it's on the left
     * of the closest edge) or the line to next vertex first (which
     * means the query point is on the right)
     */

    uint32_t prevVertexIndex = closestPointVertex > 0 ?
      closestPointVertex - 1u :
      closestEdge->geom->points->npoints - 2u; /* last vertex would be == first vertex, this being a closed edge */

    const POINT2D *prevVertex = getPoint2d_cp(
      closestEdge->geom->points,
      prevVertexIndex
    );

    LWDEBUGF(1, "Previous vertex %u is POINT(%.15g %.15g)",
      prevVertexIndex,
      prevVertex->x,
      prevVertex->y
    );

    uint32_t nextVertexIndex = closestPointVertex == closestEdge->geom->points->npoints - 1u ?
      1u : /* first point would be == last point, this being a closed edge */
      closestPointVertex + 1u;

    const POINT2D *nextVertex = getPoint2d_cp(
      closestEdge->geom->points,
      nextVertexIndex
    );

    LWDEBUGF(1, "Next vertex %u is POINT(%.15g %.15g)",
      nextVertexIndex,
      nextVertex->x,
      nextVertex->y
    );


    double azS0; /* azimuth from previous vertex to closestPointVertex */
    double azS1; /* azimuth from closestPointVertex to next vertex */
    double azSL; /* azimuth from closestPointVertex to query point */

    if ( ! azimuth_pt_pt(closestPointOnEdge, prevVertex, &azS0)) {
      lwerror("error computing azimuth of segment to closest point [%.15g %.15g,%.15g %.15g]",
              closestPointOnEdge->x, closestPointOnEdge->y,
              prevVertex->x, prevVertex->y);
      _lwt_release_edges(closestEdge, 1);
      return -1;
    }
    if ( ! azimuth_pt_pt(closestPointOnEdge, nextVertex, &azS1)) {
      lwerror("error computing azimuth of segment from closest point [%.15g %.15g,%.15g %.15g]",
              closestPointOnEdge->x, closestPointOnEdge->y,
              nextVertex->x, nextVertex->y);
      _lwt_release_edges(closestEdge, 1);
      return -1;
    }
    if ( ! azimuth_pt_pt(closestPointOnEdge, queryPoint, &azSL) ) {
      lwerror("error computing azimuth of queryPoint [%.15g %.15g,%.15g %.15g]",
              closestPointOnEdge->x, closestPointOnEdge->y,
              queryPoint->x, queryPoint->y);
      _lwt_release_edges(closestEdge, 1);
      return -1;
    }

    double angle_S0_S1 = azS1 - azS0;
    if ( angle_S0_S1 < 0 ) angle_S0_S1 += 2 * M_PI;

    double angle_S0_SL = azSL - azS0;
    if ( angle_S0_SL < 0 ) angle_S0_SL += 2 * M_PI;

    LWDEBUGF(1, "Azimuths from closest (vertex) point: P:%g, N:%g (+%g), Q:%g (+%g)",
      azS0,
      azS1, angle_S0_S1,
      azSL, angle_S0_SL
    );
    if ( angle_S0_SL < angle_S0_S1 )
    {
      /* line to query point was encountered first, is on the left */
      containingFace = closestEdge->face_left;
    }
    else
    {
      /* line to query point was NOT encountered first, is on the right */
      containingFace = closestEdge->face_right;
    }
  }
  else
  {
    /* Closest point is internal to closest segment, we can use
     * lw_segment_side */

    LWDEBUGF(1, "Closest point is internal to closest segment, calling lw_segment_side((%g,%g),(%g,%g),(%g,%g)",
      closestSegmentP0->x,
      closestSegmentP0->y,
      closestSegmentP1->x,
      closestSegmentP1->y,
      queryPoint->x,
      queryPoint->y
    );

    closestSegmentSide = lw_segment_side(closestSegmentP0, closestSegmentP1, queryPoint);
    LWDEBUGF(1, "Side of closest segment query point falls on: %d (%s)",
      closestSegmentSide, closestSegmentSide == -1 ? "left" : closestSegmentSide == 1 ? "right" : "collinear" );

    if ( closestSegmentSide == -1 ) /* left */
    {
      containingFace = closestEdge->face_left;
    }
    else if ( closestSegmentSide == 1 ) /* right */
    {
      containingFace = closestEdge->face_right;
    }
    else
    {
      lwerror("Unexpected collinearity reported from lw_segment_side");
      _lwt_release_edges(closestEdge, 1);
      return -1;
    }

  }

  _lwt_release_edges(closestEdge, 1);
  return containingFace;
}

void
_lwt_LoadGeometryRecursive(LWT_TOPOLOGY* topo, LWGEOM* geom, double tol)
{
  switch (geom->type)
  {
    case POINTTYPE:
      lwt_LoadPoint(topo, lwgeom_as_lwpoint(geom), tol);
      return;

    case LINETYPE:
      lwt_LoadLine(topo, lwgeom_as_lwline(geom), tol);
      return;

    case POLYGONTYPE:
      lwt_LoadPolygon(topo, lwgeom_as_lwpoly(geom), tol);
      return;

    case MULTILINETYPE:
    case MULTIPOLYGONTYPE:
    case MULTIPOINTTYPE:
    case COLLECTIONTYPE:
    {
      LWCOLLECTION *coll;
      uint32_t i;
      coll = (LWCOLLECTION *)geom;
      for (i=0; i<coll->ngeoms; i++)
        _lwt_LoadGeometryRecursive(topo, coll->geoms[i], tol);
      return;
    }

    default:
    {
      lwerror("%s: Unsupported geometry type: %s", __func__,
              lwtype_name(geom->type));
      return;
    }

  }
}

void
lwt_LoadGeometry(LWT_TOPOLOGY* topo, LWGEOM* geom, double tol)
{
  _lwt_LoadGeometryRecursive(topo, geom, tol);
}
