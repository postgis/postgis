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
 * Copyright (C) 2015-2017 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/


#ifndef LIBLWGEOM_TOPO_H
#define LIBLWGEOM_TOPO_H 1

#include "liblwgeom.h"

/* INT64 */
typedef int64_t LWT_INT64;

/** Identifier of topology element */
typedef LWT_INT64 LWT_ELEMID;

/*
 * ISO primitive elements
 */

/** NODE */
typedef struct
{
  LWT_ELEMID node_id;
  LWT_ELEMID containing_face; /* -1 if not isolated */
  LWPOINT *geom;
}
LWT_ISO_NODE;

void lwt_iso_node_release(LWT_ISO_NODE* node);

/** Node fields */
#define LWT_COL_NODE_NODE_ID         1<<0
#define LWT_COL_NODE_CONTAINING_FACE 1<<1
#define LWT_COL_NODE_GEOM            1<<2
#define LWT_COL_NODE_ALL            (1<<3)-1

/** EDGE */
typedef struct
{
  LWT_ELEMID edge_id;
  LWT_ELEMID start_node;
  LWT_ELEMID end_node;
  LWT_ELEMID face_left;
  LWT_ELEMID face_right;
  LWT_ELEMID next_left;
  LWT_ELEMID next_right;
  LWLINE *geom;
}
LWT_ISO_EDGE;

/** Edge fields */
#define LWT_COL_EDGE_EDGE_ID         1<<0
#define LWT_COL_EDGE_START_NODE      1<<1
#define LWT_COL_EDGE_END_NODE        1<<2
#define LWT_COL_EDGE_FACE_LEFT       1<<3
#define LWT_COL_EDGE_FACE_RIGHT      1<<4
#define LWT_COL_EDGE_NEXT_LEFT       1<<5
#define LWT_COL_EDGE_NEXT_RIGHT      1<<6
#define LWT_COL_EDGE_GEOM            1<<7
#define LWT_COL_EDGE_ALL            (1<<8)-1

/** FACE */
typedef struct
{
  LWT_ELEMID face_id;
  GBOX *mbr;
}
LWT_ISO_FACE;

/** Face fields */
#define LWT_COL_FACE_FACE_ID         1<<0
#define LWT_COL_FACE_MBR             1<<1
#define LWT_COL_FACE_ALL            (1<<2)-1

typedef enum LWT_SPATIALTYPE_T {
  LWT_PUNTAL = 0,
  LWT_LINEAL = 1,
  LWT_AREAL = 2,
  LWT_COLLECTION = 3
} LWT_SPATIALTYPE;

/*
 * Backend handling functions
 */

/* opaque pointers referencing native backend objects */

/**
 * Backend private data pointer
 *
 * Only the backend handler needs to know what it really is.
 * It will be passed to all registered callback functions.
 */
typedef struct LWT_BE_DATA_T LWT_BE_DATA;

/**
 * Backend interface handler
 *
 * Embeds all registered backend callbacks and private data pointer.
 * Will need to be passed (directly or indirectly) to al public facing
 * APIs of this library.
 */
typedef struct LWT_BE_IFACE_T LWT_BE_IFACE;

/**
 * Topology handler.
 *
 * Embeds backend interface handler.
 * Will need to be passed to all topology manipulation APIs
 * of this library.
 */
typedef struct LWT_BE_TOPOLOGY_T LWT_BE_TOPOLOGY;

/**
 * Structure containing base backend callbacks
 *
 * Used for registering into the backend iface
 */
typedef struct LWT_BE_CALLBACKS_T {

  /**
   * Read last error message from backend
   *
   * @return NULL-terminated error string
   */
  const char* (*lastErrorMessage) (const LWT_BE_DATA* be);

  /**
   * Create a new topology in the backend
   *
   * @param name the topology name
   * @param srid the topology SRID
   * @param precision the topology precision/tolerance
   * @param hasZ non-zero if topology primitives should have a Z ordinate
   * @return a topology handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
  LWT_BE_TOPOLOGY *(*createTopology)(const LWT_BE_DATA *be, const char *name, int32_t srid, double precision, int hasZ);

  /**
   * Load a topology from the backend
   *
   * @param name the topology name
   * @return a topology handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
  LWT_BE_TOPOLOGY* (*loadTopologyByName) (
    const LWT_BE_DATA* be,
    const char* name
  );

  /**
   * Release memory associated to a backend topology
   *
   * @param topo the backend topology handler
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
  int (*freeTopology) (LWT_BE_TOPOLOGY* topo);

  /**
   * Get nodes by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of node in output array.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_NODE_* macros
   *
   * @return an array of nodes
   *         or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   *
   */
  LWT_ISO_NODE *(*getNodeById)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);

  /**
   * Get nodes within distance by point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  LWT_ISO_NODE *(*getNodeWithinDistance2D)(const LWT_BE_TOPOLOGY *topo,
					   const LWPOINT *pt,
					   double dist,
					   uint64_t *numelems,
					   int fields,
					   int64_t limit);

  /**
   * Insert nodes
   *
   * Insert node primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param nodes the nodes to insert. Those with a node_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the nodes array
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
  int (*insertNodes)(const LWT_BE_TOPOLOGY *topo, LWT_ISO_NODE *nodes, uint64_t numelems);

  /**
   * Get edge by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of edge identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_EDGE_* macros
   *
   * @return an array of edges or NULL in the following cases:
   *         - none found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  LWT_ISO_EDGE *(*getEdgeById)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);

  /**
   * Get edges within distance by point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_EDGE_* macros
   * @param limit max number of edges to return, 0 for no limit, -1
   *              to only check for existence if a matching row.
   *
   * @return an array of edges or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  LWT_ISO_EDGE *(*getEdgeWithinDistance2D)(const LWT_BE_TOPOLOGY *topo,
					   const LWPOINT *pt,
					   double dist,
					   uint64_t *numelems,
					   int fields,
					   int64_t limit);

  /**
   * Get next available edge identifier
   *
   * Identifiers returned by this function should not be considered
   * available anymore.
   *
   * @param topo the topology to act upon
   *
   * @return next available edge identifier or -1 on error
   */
  LWT_ELEMID (*getNextEdgeId) (
      const LWT_BE_TOPOLOGY* topo
  );

  /**
   * Insert edges
   *
   * Insert edge primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param edges the edges to insert. Those with a edge_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the edges array
   *
   * @return number of inserted edges, or -1 (@see lastErrorMessage)
   */
  int (*insertEdges)(const LWT_BE_TOPOLOGY *topo, LWT_ISO_EDGE *edges, uint64_t numelems);

  /**
   * Update edges selected by fields match/mismatch
   *
   * @param topo the topology to act upon
   * @param sel_edge an LWT_ISO_EDGE object with selecting fields set.
   * @param sel_fields fields used to select edges to be updated,
   *                   see LWT_COL_EDGE_* macros
   * @param upd_edge an LWT_ISO_EDGE object with updated fields set.
   * @param upd_fields fields to be updated for the selected edges,
   *                   see LWT_COL_EDGE_* macros
   * @param exc_edge an LWT_ISO_EDGE object with exclusion fields set,
   *                 can be NULL if no exlusion condition exists.
   * @param exc_fields fields used for excluding edges from the update,
   *                   see LWT_COL_EDGE_* macros
   *
   * @return number of edges being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateEdges) (
      const LWT_BE_TOPOLOGY* topo,
      const LWT_ISO_EDGE* sel_edge, int sel_fields,
      const LWT_ISO_EDGE* upd_edge, int upd_fields,
      const LWT_ISO_EDGE* exc_edge, int exc_fields
  );

  /**
   * Get faces by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of edge identifiers
   *                 in the input array, gets number of node in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_FACE_* macros
   *
   * @return an array of faces or NULL in the following cases:
   *         - none found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  LWT_ISO_FACE *(*getFaceById)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);

  /**
   * Get face containing point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   *
   * @return a face identifier, -1 if no face contains the point
   *         (could be in universe face or on an edge)
   *         or -2 on error (@see lastErrorMessage)
   */
  LWT_ELEMID (*getFaceContainingPoint) (
      const LWT_BE_TOPOLOGY* topo,
      const LWPOINT* pt
  );

  /**
   * Update TopoGeometry objects after an edge split event
   *
   * @param topo the topology to act upon
   * @param split_edge identifier of the edge that was split.
   * @param new_edge1 identifier of the first new edge that was created
   *        as a result of edge splitting.
   * @param new_edge2 identifier of the second new edge that was created
   *        as a result of edge splitting, or -1 if the old edge was
   *        modified rather than replaced.
   *
	 * @return 1 on success, 0 on error
   *
   * @note on splitting an edge, the new edges both have the
   *       same direction as the original one. If a second new edge was
   *       created, its start node will be equal to the first new edge
   *       end node.
   */
  int (*updateTopoGeomEdgeSplit) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID split_edge, LWT_ELEMID new_edge1, LWT_ELEMID new_edge2
  );

  /**
   * Delete edges
   *
   * @param topo the topology to act upon
   * @param sel_edge an LWT_ISO_EDGE object with selecting fields set.
   * @param sel_fields fields used to select edges to be deleted,
   *                   see LWT_COL_EDGE_* macros
   *
   * @return number of edges being deleted or -1 on error
   *         (@see lastErroMessage)
   */
  int (*deleteEdges) (
      const LWT_BE_TOPOLOGY* topo,
      const LWT_ISO_EDGE* sel_edge, int sel_fields
  );

  /**
   * Get edges whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existence if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  LWT_ISO_NODE *(
      *getNodeWithinBox2D)(const LWT_BE_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, int limit);

  /**
   * Get edges whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box, to be considered infinite if NULL
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_EDGE_* macros
   * @param limit max number of edges to return, 0 for no limit, -1
   *              to only check for existence if a matching row.
   *
   * @return an array of edges or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  LWT_ISO_EDGE *(
      *getEdgeWithinBox2D)(const LWT_BE_TOPOLOGY *topo, const GBOX *box, uint64_t *numelems, int fields, int limit);

  /**
   * Get edges that start or end on any of the given node identifiers
   *
   * @param topo the topology to act upon
   * @param ids an array of node identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_EDGE_* macros
   *
   * @return an array of edges that are incident to a node
   *         or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   */
  LWT_ISO_EDGE *(*getEdgeByNode)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t *numelems, int fields);

  /**
   * Update nodes selected by fields match/mismatch
   *
   * @param topo the topology to act upon
   * @param sel_node an LWT_ISO_NODE object with selecting fields set.
   * @param sel_fields fields used to select nodes to be updated,
   *                   see LWT_COL_NODE_* macros
   * @param upd_node an LWT_ISO_NODE object with updated fields set.
   * @param upd_fields fields to be updated for the selected nodes,
   *                   see LWT_COL_NODE_* macros
   * @param exc_node an LWT_ISO_NODE object with exclusion fields set,
   *                 can be NULL if no exclusion condition exists.
   * @param exc_fields fields used for excluding nodes from the update,
   *                   see LWT_COL_NODE_* macros
   *
   * @return number of nodes being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateNodes) (
      const LWT_BE_TOPOLOGY* topo,
      const LWT_ISO_NODE* sel_node, int sel_fields,
      const LWT_ISO_NODE* upd_node, int upd_fields,
      const LWT_ISO_NODE* exc_node, int exc_fields
  );

  /**
   * Update TopoGeometry objects after a face split event
   *
   * @param topo the topology to act upon
   * @param split_face identifier of the face that was split.
   * @param new_face1 identifier of the first new face that was created
   *        as a result of face splitting.
   * @param new_face2 identifier of the second new face that was created
   *        as a result of face splitting, or -1 if the old face was
   *        modified rather than replaced.
   *
	 * @return 1 on success, 0 on error (@see lastErroMessage)
   *
   */
  int (*updateTopoGeomFaceSplit) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID split_face, LWT_ELEMID new_face1, LWT_ELEMID new_face2
  );

  /**
   * Insert faces
   *
   * Insert face primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param faces the faces to insert. Those with a node_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the faces array
   *
   * @return number of inserted faces, or -1 (@see lastErrorMessage)
   */
  int (*insertFaces)(const LWT_BE_TOPOLOGY *topo, LWT_ISO_FACE *faces, uint64_t numelems);

  /**
   * Update faces by id
   *
   * @param topo the topology to act upon
   * @param faces an array of LWT_ISO_FACE object with selecting id
   *              and setting mbr.
   * @param numfaces number of faces in the "faces" array
   *
   * @return number of faces being updated or UINT64_MAX on error
   *         (@see lastErroMessage)
   */
  uint64_t (*updateFacesById) (
      const LWT_BE_TOPOLOGY* topo,
      const LWT_ISO_FACE* faces, uint64_t numfaces
  );

  /*
   * Get the ordered list edge visited by a side walk
   *
   * The walk starts from the side of an edge and stops when
   * either the max number of visited edges OR the starting
   * position is reached. The output list never includes a
   * duplicated signed edge identifier.
   *
   * It is expected that the walk uses the "next_left" and "next_right"
   * attributes of ISO edges to perform the walk (rather than recomputing
   * the turns at each node).
   *
   * @param topo the topology to operate on
   * @param edge walk start position and direction:
   *             abs value identifies the edge, sign expresses
   *             side (left if positive, right if negative)
   *             and direction (forward if positive, backward if negative).
   * @param numedges output parameter, gets the number of edges visited
   *
   * @param limit max edges to return (to avoid an infinite loop in case
   *              of a corrupted topology). 0 is for unlimited.
   *              The function is expected to error out if the limit is hit.
   *
   * @return an array of signed edge identifiers (positive edges being
   *         walked in their direction, negative ones in opposite) or
   *         NULL on error (@see lastErrorMessage)
   */
  LWT_ELEMID *(*getRingEdges)(const LWT_BE_TOPOLOGY *topo, LWT_ELEMID edge, uint64_t *numedges, int limit);

  /**
   * Update edges by id
   *
   * @param topo the topology to act upon
   * @param edges an array of LWT_ISO_EDGE object with selecting id
   *              and updating fields.
   * @param numedges number of edges in the "edges" array
   * @param upd_fields fields to be updated for the selected edges,
   *                   see LWT_COL_EDGE_* macros
   *
   * @return number of edges being updated or -1 on error
   *         (@see lastErrorMessage)
   */
  int (*updateEdgesById)(const LWT_BE_TOPOLOGY *topo, const LWT_ISO_EDGE *edges, uint64_t numedges, int upd_fields);

  /**
   * \brief
   * Get edges that have any of the given faces on the left or right side
   * and optionally whose bounding box overlaps the given one.
   *
   * @param topo the topology to act upon
   * @param ids an array of face identifiers
   * @param numelems input/output parameter, pass number of face identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_EDGE_* macros
   * @param box optional bounding box to further restrict matches, use
   *            NULL for no further restriction.
   *
   * @return an array of edges identifiers or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  LWT_ISO_EDGE *(*getEdgeByFace)(const LWT_BE_TOPOLOGY *topo,
				 const LWT_ELEMID *ids,
				 uint64_t *numelems,
				 int fields,
				 const GBOX *box);

  /**
   * Get isolated nodes contained in any of the given faces
   *
   * @param topo the topology to act upon
   * @param faces an array of face identifiers
   * @param numelems input/output parameter, pass number of face
   *                 identifiers in the input array, gets number of
   *                 nodes in output array if the return is not null,
   *                 otherwise see @return section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_NODE_* macros
   * @param box optional bounding box to further restrict matches, use
   *            NULL for no further restriction.
   *
   * @return an array of nodes or NULL in the following cases:
   *         - no nod found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1, @see lastErrorMessage)
   */
  LWT_ISO_NODE *(*getNodeByFace)(const LWT_BE_TOPOLOGY *topo,
				 const LWT_ELEMID *faces,
				 uint64_t *numelems,
				 int fields,
				 const GBOX *box);

  /**
   * Update nodes by id
   *
   * @param topo the topology to act upon
   * @param nodes an array of LWT_ISO_EDGE objects with selecting id
   *              and updating fields.
   * @param numnodes number of nodes in the "nodes" array
   * @param upd_fields fields to be updated for the selected edges,
   *                   see LWT_COL_NODE_* macros
   *
   * @return number of nodes being updated or -1 on error
   *         (@see lastErrorMessage)
   */
  int (*updateNodesById)(const LWT_BE_TOPOLOGY *topo, const LWT_ISO_NODE *nodes, uint64_t numnodes, int upd_fields);

  /**
   * Delete faces by id
   *
   * @param topo the topology to act upon
   * @param ids an array of face identifiers
   * @param numelems number of face identifiers in the ids array
   *
   * @return number of faces being deleted or -1 on error
   *         (@see lastErrorMessage)
   */
  int (*deleteFacesById)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems);

  /**
   * Get topology SRID
   * @return 0 for unknown
   */
  int (*topoGetSRID) (
      const LWT_BE_TOPOLOGY* topo
  );

  /**
   * Get topology precision
   */
  double (*topoGetPrecision) (
      const LWT_BE_TOPOLOGY* topo
  );

  /**
   * Get topology Z flag
   * @return 1 if topology elements do have Z value, 0 otherwise
   */
  int (*topoHasZ) (
      const LWT_BE_TOPOLOGY* topo
  );

  /**
   * Delete nodes by id
   *
   * @param topo the topology to act upon
   * @param ids an array of node identifiers
   * @param numelems number of node identifiers in the ids array
   *
   * @return number of nodes being deleted or -1 on error
   *         (@see lastErrorMessage)
   */
  int (*deleteNodesById)(const LWT_BE_TOPOLOGY *topo, const LWT_ELEMID *ids, uint64_t numelems);

  /**
   * Check TopoGeometry objects before an edge removal event
   *
   * @param topo the topology to act upon
   * @param rem_edge identifier of the edge that's been removed
   * @param face_left identifier of the face on the edge's left side
   * @param face_right identifier of the face on the edge's right side
   *
   * @return 1 to allow, 0 to forbid the operation
   *         (reporting reason via lastErrorMessage)
   *
   */
  int (*checkTopoGeomRemEdge) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID rem_edge,
      LWT_ELEMID face_left,
      LWT_ELEMID face_right
  );

  /**
   * Update TopoGeometry objects after healing two faces
   *
   * @param topo the topology to act upon
   * @param face1 identifier of the first face
   * @param face2 identifier of the second face
   * @param newface identifier of the new face
   *
   * @note that newface may or may not be equal to face1 or face2,
   *       while face1 should never be the same as face2.
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   *
   */
  int (*updateTopoGeomFaceHeal) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID face1, LWT_ELEMID face2, LWT_ELEMID newface
  );

  /**
   * Check TopoGeometry objects before a node removal event
   *
   * @param topo the topology to act upon
   * @param rem_node identifier of the node that's been removed
   * @param e1 identifier of the first connected edge
   * @param e2 identifier of the second connected edge
   *
   * The operation should be forbidden if any TopoGeometry object
   * exists which contains only one of the two healed edges.
   *
   * The operation should also be forbidden if the removed node
   * takes part in the definition of a TopoGeometry, although
   * this wasn't the case yet as of PostGIS version 2.1.8:
   * https://trac.osgeo.org/postgis/ticket/3239
   *
   * @return 1 to allow, 0 to forbid the operation
   *         (reporting reason via lastErrorMessage)
   *
   */
  int (*checkTopoGeomRemNode) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID rem_node,
      LWT_ELEMID e1,
      LWT_ELEMID e2
  );

  /**
   * Update TopoGeometry objects after healing two edges
   *
   * @param topo the topology to act upon
   * @param edge1 identifier of the first edge
   * @param edge2 identifier of the second edge
   * @param newedge identifier of the new edge, taking the space
   *                previously occupied by both original edges
   *
   * @note that newedge may or may not be equal to edge1 or edge2,
   *       while edge1 should never be the same as edge2.
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   *
   */
  int (*updateTopoGeomEdgeHeal) (
      const LWT_BE_TOPOLOGY* topo,
      LWT_ELEMID edge1, LWT_ELEMID edge2, LWT_ELEMID newedge
  );

  /**
   * Get faces whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWT_COL_FACE_* macros
   * @param limit max number of faces to return, 0 for no limit, -1
   *              to only check for existence if a matching row.
   *
   * @return an array of faces or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  LWT_ISO_FACE *(*getFaceWithinBox2D)(const LWT_BE_TOPOLOGY *topo,
				      const GBOX *box,
				      uint64_t *numelems,
				      int fields,
				      int limit);

} LWT_BE_CALLBACKS;


/**
 * Create a new backend interface
 *
 * Ownership to caller delete with lwt_FreeBackendIface
 *
 * @param data Backend data, passed as first parameter to all callback functions
 */
LWT_BE_IFACE* lwt_CreateBackendIface(const LWT_BE_DATA* data);

/**
 * Register backend callbacks into the opaque iface handler
 *
 * @param iface the backend interface handler (see lwt_CreateBackendIface)
 * @param cb a pointer to the callbacks structure; ownership left to caller.
 */
void lwt_BackendIfaceRegisterCallbacks(LWT_BE_IFACE* iface, const LWT_BE_CALLBACKS* cb);

/** Release memory associated with an LWT_BE_IFACE */
void lwt_FreeBackendIface(LWT_BE_IFACE* iface);

/********************************************************************
 *
 * End of BE interface
 *
 *******************************************************************/

/**
 * Topology errors type
 */
typedef enum LWT_TOPOERR_TYPE_T {
  LWT_TOPOERR_EDGE_CROSSES_NODE,
  LWT_TOPOERR_EDGE_INVALID,
  LWT_TOPOERR_EDGE_NOT_SIMPLE,
  LWT_TOPOERR_EDGE_CROSSES_EDGE,
  LWT_TOPOERR_EDGE_STARTNODE_MISMATCH,
  LWT_TOPOERR_EDGE_ENDNODE_MISMATCH,
  LWT_TOPOERR_FACE_WITHOUT_EDGES,
  LWT_TOPOERR_FACE_HAS_NO_RINGS,
  LWT_TOPOERR_FACE_OVERLAPS_FACE,
  LWT_TOPOERR_FACE_WITHIN_FACE
} LWT_TOPOERR_TYPE;

/** Topology error */
typedef struct LWT_TOPOERR_T {
  /** Type of error */
  LWT_TOPOERR_TYPE err;
  /** Identifier of first affected element */
  LWT_ELEMID elem1;
  /** Identifier of second affected element (0 if inapplicable) */
  LWT_ELEMID elem2;
} LWT_TOPOERR;

/*
 * Topology functions
 */

/** Opaque topology structure
 *
 * Embeds backend interface and topology
 */
typedef struct LWT_TOPOLOGY_T LWT_TOPOLOGY;


/*******************************************************************
 *
 * Non-ISO signatures here
 *
 *******************************************************************/

/**
 * Initializes a new topology
 *
 * @param iface the backend interface handler (see lwt_CreateBackendIface)
 * @param name name of the new topology
 * @param srid the topology SRID
 * @param prec the topology precision/tolerance
 * @param hasz non-zero if topology primitives should have a Z ordinate
 *
 * @return the handler of the topology, or NULL on error
 *         (liblwgeom error handler will be invoked with error message)
 */
LWT_TOPOLOGY *lwt_CreateTopology(LWT_BE_IFACE *iface, const char *name, int32_t srid, double prec, int hasz);

/**
 * Loads an existing topology by name from the database
 *
 * @param iface the backend interface handler (see lwt_CreateBackendIface)
 * @param name name of the topology to load
 *
 * @return the handler of the topology, or NULL on error
 *         (liblwgeom error handler will be invoked with error message)
 */
LWT_TOPOLOGY *lwt_LoadTopology(LWT_BE_IFACE *iface, const char *name);

/**
 * Drop a topology and all its associated objects from the database
 *
 * @param topo the topology to drop
 */
void lwt_DropTopology(LWT_TOPOLOGY* topo);

/** Release memory associated with an LWT_TOPOLOGY
 *
 * @param topo the topology to release (it's not removed from db)
 */
void lwt_FreeTopology(LWT_TOPOLOGY* topo);

/**
 * Retrieve the id of a node at a point location
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for a node
 * @return a node identifier if one is found, 0 if none is found, -1
 *         on error (multiple nodes within distance).
 *         The liblwgeom error handler will be invoked in case of error.
 */
LWT_ELEMID lwt_GetNodeByPoint(LWT_TOPOLOGY *topo, LWPOINT *pt, double tol);

/**
 * Find the edge-id of an edge that intersects a given point
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for an
 *            intersecting edge
 * @return an edge identifier if one is found, 0 if none is found, -1
 *         on error (multiple edges within distance).
 *         The liblwgeom error handler will be invoked in case of error.
 */
LWT_ELEMID lwt_GetEdgeByPoint(LWT_TOPOLOGY *topo, LWPOINT *pt, double tol);

/**
 * Find the face-id of a face containing a given point
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for a
 *            containing face
 * @return a face identifier if one is found (0 if universe), -1
 *         on error (multiple faces within distance or point on node
 *         or edge).
 *         The liblwgeom error handler will be invoked in case of error.
 */
LWT_ELEMID lwt_GetFaceByPoint(LWT_TOPOLOGY *topo, LWPOINT *pt, double tol);


/*******************************************************************
 *
 * Topology population (non-ISO)
 *
 *******************************************************************/

/**
 * Adds a point to the topology
 *
 * The given point will snap to existing nodes or edges within given
 * tolerance. An existing edge may be split by the point.
 *
 * @param topo the topology to operate on
 * @param point the point to add
 * @param tol snap tolerance, the topology tolerance will be used if 0
 *
 * @return identifier of added (or pre-existing) node or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 */
LWT_ELEMID lwt_AddPoint(LWT_TOPOLOGY* topo, LWPOINT* point, double tol);

/**
 * Adds a linestring to the topology
 *
 * The given line will snap to existing nodes or edges within given
 * tolerance. Existing edges or faces may be split by the line.
 *
 * @param topo the topology to operate on
 * @param line the line to add
 * @param tol snap tolerance, the topology tolerance will be used if 0
 * @param nedges output parameter, will be set to number of edges the
 *               line was split into, or -1 on error
 *               (liblwgeom error handler will be invoked with error message)
 *
 * @return an array of <nedges> edge identifiers that sewed togheter
 *         will build up the input linestring (after snapping). Caller
 *         will need to free the array using lwfree(), if not null.
 */
LWT_ELEMID* lwt_AddLine(LWT_TOPOLOGY* topo, LWLINE* line, double tol,
                        int* nedges);

/**
 * Adds a linestring to the topology without determining generated faces
 *
 * The given line will snap to existing nodes or edges within given
 * tolerance. Existing edges or faces may be split by the line.
 *
 * Side faces for the new edges will not be determined and no new
 * faces will be created, effectively leaving the topology in an
 * invalid state (WARNING!)
 *
 * @param topo the topology to operate on
 * @param line the line to add
 * @param tol snap tolerance, the topology tolerance will be used if 0
 * @param nedges output parameter, will be set to number of edges the
 *               line was split into, or -1 on error
 *               (liblwgeom error handler will be invoked with error message)
 *
 * @return an array of <nedges> edge identifiers that sewed togheter
 *         will build up the input linestring (after snapping). Caller
 *         will need to free the array using lwfree(), if not null.
 */
LWT_ELEMID* lwt_AddLineNoFace(LWT_TOPOLOGY* topo, LWLINE* line, double tol,
                        int* nedges);

/*
 * Determine and register all topology faces:
 *
 *  - Determines which faces are generated by existing
 *    edges.
 *  - Creates face records with correct mbr
 *  - Update edge left/right face attributes
 *
 *  Precondition:
 *     - the topology edges are correctly linked
 *     - there are no faces registered in the topology
 *
 *  Postconditions:
 *     - all left/right face attributes of edges
 *       reference faces with correct mbr.
 *
 *  Notes:
 *     - does not attempt to assign isolated nodes to their
 *       containing faces
 *     - does not remove existing face records
 *     - loads in memory all the topology edges
 *
 * @param topo the topology to operate on
 *
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error
 *         message)
 */
int lwt_Polygonize(LWT_TOPOLOGY* topo);

/**
 * Adds a polygon to the topology
 *
 * The boundary of the given polygon will snap to existing nodes or
 * edges within given tolerance.
 * Existing edges or faces may be split by the boundary of the polygon.
 *
 * @param topo the topology to operate on
 * @param poly the polygon to add
 * @param tol snap tolerance, the topology tolerance will be used if 0
 * @param nfaces output parameter, will be set to number of faces the
 *               polygon was split into, or -1 on error
 *               (liblwgeom error handler will be invoked with error message)
 *
 * @return an array of <nfaces> face identifiers that sewed togheter
 *         will build up the input polygon (after snapping). Caller
 *         will need to free the array using lwfree(), if not null.
 */
LWT_ELEMID* lwt_AddPolygon(LWT_TOPOLOGY* topo, LWPOLY* poly, double tol,
                        int* nfaces);

/*******************************************************************
 *
 * ISO signatures here
 *
 *******************************************************************/

/**
 * Populate an empty topology with data from a simple geometry
 *
 * For ST_CreateTopoGeo
 *
 * @param topo the topology to operate on
 * @param geom the geometry to import
 *
 */
void lwt_CreateTopoGeo(LWT_TOPOLOGY* topo, LWGEOM *geom);

/**
 * Add an isolated node
 *
 * For ST_AddIsoNode
 *
 * @param topo the topology to operate on
 * @param face the identifier of containing face or -1 for "unknown"
 * @param pt the node position
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident nodes, crossing edges,
 *                    actual face containment)
 *
 * @return ID of the newly added node, or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_AddIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID face,
                          LWPOINT* pt, int skipChecks);

/**
 * Move an isolated node
 *
 * For ST_MoveIsoNode
 *
 * @param topo the topology to operate on
 * @param node the identifier of the nod to be moved
 * @param pt the new node position
 * @return 0 on success, -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
int lwt_MoveIsoNode(LWT_TOPOLOGY* topo,
                    LWT_ELEMID node, LWPOINT* pt);

/**
 * Remove an isolated node
 *
 * For ST_RemoveIsoNode
 *
 * @param topo the topology to operate on
 * @param node the identifier of the node to be moved
 * @return 0 on success, -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
int lwt_RemoveIsoNode(LWT_TOPOLOGY* topo, LWT_ELEMID node);

/**
 * Remove an isolated edge
 *
 * For ST_RemIsoEdge
 *
 * @param topo the topology to operate on
 * @param edge the identifier of the edge to be moved
 * @return 0 on success, -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
int lwt_RemIsoEdge(LWT_TOPOLOGY* topo, LWT_ELEMID edge);

/**
 * Add an isolated edge connecting two existing isolated nodes
 *
 * For ST_AddIsoEdge
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @return ID of the newly added edge, or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_AddIsoEdge(LWT_TOPOLOGY* topo,
                          LWT_ELEMID startNode, LWT_ELEMID endNode,
                          const LWLINE *geom);

/**
 * Add a new edge possibly splitting a face (modifying it)
 *
 * For ST_AddEdgeModFace
 *
 * If the new edge splits a face, the face is shrunk and a new one
 * is created. Unless the face being split is the Universal Face, the
 * new face will be on the right side of the newly added edge.
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @param skipChecks if non-zero skips consistency checks
 *                   (curve being simple and valid, start/end nodes
 *                    consistency actual face containment)
 *
 * @return ID of the newly added edge or null on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_AddEdgeModFace(LWT_TOPOLOGY* topo,
                              LWT_ELEMID start_node, LWT_ELEMID end_node,
                              LWLINE *geom, int skipChecks);

/**
 * Add a new edge possibly splitting a face (replacing with two new faces)
 *
 * For ST_AddEdgeNewFaces
 *
 * If the new edge splits a face, the face is replaced by two new faces.
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @param skipChecks if non-zero skips consistency checks
 *                   (curve being simple and valid, start/end nodes
 *                    consistency actual face containment)
 * @return ID of the newly added edge
 *
 */
LWT_ELEMID lwt_AddEdgeNewFaces(LWT_TOPOLOGY* topo,
                              LWT_ELEMID start_node, LWT_ELEMID end_node,
                              LWLINE *geom, int skipChecks);

/**
 * Remove an edge, possibly merging two faces (replacing both with a new one)
 *
 * For ST_RemEdgeNewFace
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be removed
 * @return the id of newly created face, 0 if no new face was created
 *         or -1 on error
 *
 */
LWT_ELEMID lwt_RemEdgeNewFace(LWT_TOPOLOGY* topo, LWT_ELEMID edge);

/**
 * Remove an edge, possibly merging two faces (replacing one with the other)
 *
 * For ST_RemEdgeModFace
 *
 * Preferentially keep the face on the right, to be symmetric with
 * lwt_AddEdgeModFace.
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be removed
 * @return the id of the face that takes the space previously occupied
 *         by the removed edge, or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_RemEdgeModFace(LWT_TOPOLOGY* topo, LWT_ELEMID edge);

/**
 * Changes the shape of an edge without affecting the topology structure.
 *
 * For ST_ChangeEdgeGeom
 *
 * @param topo the topology to operate on
 * @param curve the edge geometry
 * @return 0 on success, -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
int lwt_ChangeEdgeGeom(LWT_TOPOLOGY* topo, LWT_ELEMID edge, LWLINE* curve);

/**
 * Split an edge by a node, modifying the original edge and adding a new one.
 *
 * For ST_ModEdgeSplit
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be split
 * @param pt geometry of the new node
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident node, point not on edge...)
 * @return the id of newly created node, or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_ModEdgeSplit(LWT_TOPOLOGY* topo, LWT_ELEMID edge, LWPOINT* pt, int skipChecks);

/**
 * Split an edge by a node, replacing it with two new edges
 *
 * For ST_NewEdgesSplit
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be split
 * @param pt geometry of the new node
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident node, point not on edge...)
 * @return the id of newly created node
 *
 */
LWT_ELEMID lwt_NewEdgesSplit(LWT_TOPOLOGY* topo, LWT_ELEMID edge, LWPOINT* pt, int skipChecks);

/**
 * Merge two edges, modifying the first and deleting the second
 *
 * For ST_ModEdgeHeal
 *
 * @param topo the topology to operate on
 * @param e1 identifier of first edge
 * @param e2 identifier of second edge
 * @return the id of the removed node or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_ModEdgeHeal(LWT_TOPOLOGY* topo, LWT_ELEMID e1, LWT_ELEMID e2);

/**
 * Merge two edges, replacing both with a new one
 *
 * For ST_NewEdgeHeal
 *
 * @param topo the topology to operate on
 * @param e1 identifier of first edge
 * @param e2 identifier of second edge
 * @return the id of the new edge or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
LWT_ELEMID lwt_NewEdgeHeal(LWT_TOPOLOGY* topo, LWT_ELEMID e1, LWT_ELEMID e2);

/**
 * Return the list of directed edges bounding a face
 *
 * For ST_GetFaceEdges
 *
 * @param topo the topology to operate on
 * @param face identifier of the face
 * @param edges will be set to an array of signed edge identifiers, will
 *              need to be released with lwfree
 * @return the number of edges in the edges array, or -1 on error
 *         (liblwgeom error handler will be invoked with error message)
 *
 */
int lwt_GetFaceEdges(LWT_TOPOLOGY* topo, LWT_ELEMID face, LWT_ELEMID **edges);

/**
 * Return the geometry of a face
 *
 * For ST_GetFaceGeometry
 *
 * @param topo the topology to operate on
 * @param face identifier of the face
 * @return a polygon geometry representing the face, ownership to caller,
 *         to be released with lwgeom_release, or NULL on error
 *         (liblwgeom error handler will be invoked with error message)
 */
LWGEOM* lwt_GetFaceGeometry(LWT_TOPOLOGY* topo, LWT_ELEMID face);

#endif /* LIBLWGEOM_TOPO_H */
