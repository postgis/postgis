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
 **********************************************************************
 *
 *  Topology Polygonizer
 *
 **********************************************************************/

#include "../postgis_config.h"

/*#define POSTGIS_DEBUG_LEVEL 1*/
#include "lwgeom_log.h"

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"

#include "lwgeom_geos.h"

/* An array of pointers to EDGERING structures */
typedef struct LWT_ISO_EDGE_TABLE_T {
  LWT_ISO_EDGE *edges;
  int size;
} LWT_ISO_EDGE_TABLE;

static int
compare_iso_edges_by_id(const void *si1, const void *si2)
{
	int a = ((LWT_ISO_EDGE *)si1)->edge_id;
	int b = ((LWT_ISO_EDGE *)si2)->edge_id;
	if ( a < b )
		return -1;
	else if ( a > b )
		return 1;
	else
		return 0;
}

static LWT_ISO_EDGE *
_lwt_getIsoEdgeById(LWT_ISO_EDGE_TABLE *tab, LWT_ELEMID id)
{
  LWT_ISO_EDGE key;
  key.edge_id = id;

  void *match = bsearch( &key, tab->edges, tab->size,
                     sizeof(LWT_ISO_EDGE),
                     compare_iso_edges_by_id);
  return match;
}

typedef struct LWT_EDGERING_ELEM_T {
  /* externally owned */
  LWT_ISO_EDGE *edge;
  /* 0 if false, 1 if true */
  int left;
} LWT_EDGERING_ELEM;

/* A ring of edges */
typedef struct LWT_EDGERING_T {
  /* Signed edge identifiers
   * positive ones are walked in their direction, negative ones
   * in the opposite direction */
  LWT_EDGERING_ELEM **elems;
  /* Number of edges in the ring */
  int size;
  int capacity;
  /* Bounding box of the ring */
  GBOX *env;
  /* Bounding box of the ring in GEOS format (for STRTree) */
  GEOSGeometry *genv;
} LWT_EDGERING;

#define LWT_EDGERING_INIT(a) { \
  (a)->size = 0; \
  (a)->capacity = 1; \
  (a)->elems = lwalloc(sizeof(LWT_EDGERING_ELEM *) * (a)->capacity); \
  (a)->env = NULL; \
  (a)->genv = NULL; \
}

#define LWT_EDGERING_PUSH(a, r) { \
  if ( (a)->size + 1 > (a)->capacity ) { \
    (a)->capacity *= 2; \
    (a)->elems = lwrealloc((a)->elems, sizeof(LWT_EDGERING_ELEM *) * (a)->capacity); \
  } \
  /* lwdebug(1, "adding elem %d (%p) of edgering %p", (a)->size, (r), (a)); */ \
  (a)->elems[(a)->size++] = (r); \
}

#define LWT_EDGERING_CLEAN(a) { \
  int i; for (i=0; i<(a)->size; ++i) { \
    if ( (a)->elems[i] ) { \
      /* lwdebug(1, "freeing elem %d (%p) of edgering %p", i, (a)->elems[i], (a)); */ \
      lwfree((a)->elems[i]); \
    } \
  } \
  if ( (a)->elems ) { lwfree((a)->elems); (a)->elems = NULL; } \
  (a)->size = 0; \
  (a)->capacity = 0; \
  if ( (a)->env ) { lwfree((a)->env); (a)->env = NULL; } \
  if ( (a)->genv ) { GEOSGeom_destroy((a)->genv); (a)->genv = NULL; } \
}

/* An array of pointers to EDGERING structures */
typedef struct LWT_EDGERING_ARRAY_T {
  LWT_EDGERING **rings;
  int size;
  int capacity;
  GEOSSTRtree* tree;
} LWT_EDGERING_ARRAY;

#define LWT_EDGERING_ARRAY_INIT(a) { \
  (a)->size = 0; \
  (a)->capacity = 1; \
  (a)->rings = lwalloc(sizeof(LWT_EDGERING *) * (a)->capacity); \
  (a)->tree = NULL; \
}

/* WARNING: use of 'j' is intentional, not to clash with
 * 'i' used in LWT_EDGERING_CLEAN */
#define LWT_EDGERING_ARRAY_CLEAN(a) { \
  int j; for (j=0; j<(a)->size; ++j) { \
    LWT_EDGERING_CLEAN((a)->rings[j]); \
  } \
  if ( (a)->capacity ) lwfree((a)->rings); \
  if ( (a)->tree ) { \
    GEOSSTRtree_destroy( (a)->tree ); \
    (a)->tree = NULL; \
  } \
}

#define LWT_EDGERING_ARRAY_PUSH(a, r) { \
  if ( (a)->size + 1 > (a)->capacity ) { \
    (a)->capacity *= 2; \
    (a)->rings = lwrealloc((a)->rings, sizeof(LWT_EDGERING *) * (a)->capacity); \
  } \
  (a)->rings[(a)->size++] = (r); \
}

typedef struct LWT_EDGERING_POINT_ITERATOR_T {
  LWT_EDGERING *ring;
  LWT_EDGERING_ELEM *curelem;
  int curelemidx;
  int curidx;
} LWT_EDGERING_POINT_ITERATOR;

static int
_lwt_EdgeRingIterator_next(LWT_EDGERING_POINT_ITERATOR *it, POINT2D *pt)
{
  LWT_EDGERING_ELEM *el = it->curelem;
  POINTARRAY *pa;

  if ( ! el ) return 0; /* finished */

  pa = el->edge->geom->points;

  int tonext = 0;
  LWDEBUGF(3, "iterator fetching idx %d from pa of %d points", it->curidx, pa->npoints);
  getPoint2d_p(pa, it->curidx, pt);
  if ( el->left ) {
    it->curidx++;
    if ( it->curidx >= (int) pa->npoints ) tonext = 1;
  } else {
    it->curidx--;
    if ( it->curidx < 0 ) tonext = 1;
  }

  if ( tonext )
  {
    LWDEBUG(3, "iterator moving to next element");
    it->curelemidx++;
    if ( it->curelemidx < it->ring->size )
    {
      el = it->curelem = it->ring->elems[it->curelemidx];
      it->curidx = el->left ? 0 : el->edge->geom->points->npoints - 1;
    }
    else
    {
      it->curelem = NULL;
    }
  }

  return 1;
}

/* Release return with lwfree */
static LWT_EDGERING_POINT_ITERATOR *
_lwt_EdgeRingIterator_begin(LWT_EDGERING *er)
{
  LWT_EDGERING_POINT_ITERATOR *ret = lwalloc(sizeof(LWT_EDGERING_POINT_ITERATOR));
  ret->ring = er;
  if ( er->size ) ret->curelem = er->elems[0];
  else ret->curelem = NULL;
  ret->curelemidx = 0;
  ret->curidx = (ret->curelem == NULL || ret->curelem->left) ? 0 : ret->curelem->edge->geom->points->npoints - 1;
  return ret;
}

/* Identifier for a placeholder face that will be
 * used to mark hole rings */
#define LWT_HOLES_FACE_PLACEHOLDER INT32_MIN

static int
_lwt_FetchNextUnvisitedEdge(__attribute__((__unused__)) LWT_TOPOLOGY *topo, LWT_ISO_EDGE_TABLE *etab, int from)
{
  while (
    from < etab->size &&
    etab->edges[from].face_left != -1 &&
    etab->edges[from].face_right != -1
  ) from++;
  return from < etab->size ? from : -1;
}

static LWT_ISO_EDGE *
_lwt_FetchAllEdges(LWT_TOPOLOGY *topo, int *numedges)
{
  LWT_ISO_EDGE *edge;
  int fields = LWT_COL_EDGE_ALL;
  uint64_t nelems = 1;

  edge = lwt_be_getEdgeWithinBox2D( topo, NULL, &nelems, fields, 0);
  *numedges = nelems;
  if (nelems == UINT64_MAX)
  {
	  PGTOPO_BE_ERROR();
	  return NULL;
  }
  return edge;
}

/* Update the side face of given ring edges
 *
 * Edge identifiers are signed, those with negative identifier
 * need to be updated their right_face, those with positive
 * identifier need to be updated their left_face.
 *
 * @param face identifier of the face bound by the ring
 * @return 0 on success, -1 on error
 */
static int
_lwt_UpdateEdgeRingSideFace(LWT_TOPOLOGY *topo, LWT_EDGERING *ring,
                            LWT_ELEMID face)
{
  LWT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  LWT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;
  int i, ret;

  /* Make a list of forward_edges and backward_edges */

  forward_edges = lwalloc(sizeof(LWT_ISO_EDGE) * ring->size);
  forward_edges_count = 0;
  backward_edges = lwalloc(sizeof(LWT_ISO_EDGE) * ring->size);
  backward_edges_count = 0;

  for ( i=0; i<ring->size; ++i )
  {
    LWT_EDGERING_ELEM *elem = ring->elems[i];
    LWT_ISO_EDGE *edge = elem->edge;
    LWT_ELEMID id = edge->edge_id;
    if ( elem->left )
    {
      LWDEBUGF(3, "Forward edge %d is %d", forward_edges_count, id);
      forward_edges[forward_edges_count].edge_id = id;
      forward_edges[forward_edges_count++].face_left = face;
      edge->face_left = face;
    }
    else
    {
      LWDEBUGF(3, "Backward edge %d is %d", forward_edges_count, id);
      backward_edges[backward_edges_count].edge_id = id;
      backward_edges[backward_edges_count++].face_right = face;
      edge->face_right = face;
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
      lwfree( forward_edges );
      lwfree( backward_edges );
      PGTOPO_BE_ERROR();
      return -1;
    }
    if ( ret != forward_edges_count )
    {
      lwfree( forward_edges );
      lwfree( backward_edges );
      lwerror("Unexpected error: %d edges updated when expecting %d (forward)",
              ret, forward_edges_count);
      return -1;
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
      lwfree( forward_edges );
      lwfree( backward_edges );
      PGTOPO_BE_ERROR();
      return -1;
    }
    if ( ret != backward_edges_count )
    {
      lwfree( forward_edges );
      lwfree( backward_edges );
      lwerror("Unexpected error: %d edges updated when expecting %d (backward)",
              ret, backward_edges_count);
      return -1;
    }
  }

  lwfree( forward_edges );
  lwfree( backward_edges );

  return 0;
}

/*
 * @param side 1 for left side, -1 for right side
 */
static LWT_EDGERING *
_lwt_BuildEdgeRing(__attribute__((__unused__)) LWT_TOPOLOGY *topo, LWT_ISO_EDGE_TABLE *edges,
                   LWT_ISO_EDGE *edge, int side)
{
  LWT_EDGERING *ring;
  LWT_EDGERING_ELEM *elem;
  LWT_ISO_EDGE *cur;
  int curside;

  ring = lwalloc(sizeof(LWT_EDGERING));
  LWT_EDGERING_INIT(ring);

  cur = edge;
  curside = side;

  LWDEBUGF(2, "Building rings for edge %d (side %d)", cur->edge_id, side);

  do {
    LWT_ELEMID next;

    elem = lwalloc(sizeof(LWT_EDGERING_ELEM));
    elem->edge = cur;
    elem->left = ( curside == 1 );

    /* Mark edge as "visited" */
    if ( elem->left ) cur->face_left = LWT_HOLES_FACE_PLACEHOLDER;
    else cur->face_right = LWT_HOLES_FACE_PLACEHOLDER;

    LWT_EDGERING_PUSH(ring, elem);
    next = elem->left ? cur->next_left : cur->next_right;

    LWDEBUGF(3, " next edge is %d", next);

    if ( next > 0 ) curside = 1;
    else { curside = -1; next = -next; }
    cur = _lwt_getIsoEdgeById(edges, next);
    if ( ! cur )
    {
      lwerror("Could not find edge with id %" LWTFMT_ELEMID, next);
      break;
    }
  } while (cur != edge || curside != side);

  LWDEBUGF(1, "Ring for edge %d has %d elems", edge->edge_id*side, ring->size);

  return ring;
}

static double
_lwt_EdgeRingSignedArea(LWT_EDGERING_POINT_ITERATOR *it)
{
	POINT2D P1;
	POINT2D P2;
	POINT2D P3;
	double sum = 0.0;
	double x0, x, y1, y2;

  if ( ! _lwt_EdgeRingIterator_next(it, &P1) ) return 0.0;
  if ( ! _lwt_EdgeRingIterator_next(it, &P2) ) return 0.0;

  LWDEBUG(2, "_lwt_EdgeRingSignedArea");

  x0 = P1.x;
  while ( _lwt_EdgeRingIterator_next(it, &P3)  )
  {
    x = P2.x - x0;
    y1 = P3.y;
    y2 = P1.y;
    sum += x * (y2-y1);

    /* Move forwards! */
    P1 = P2;
    P2 = P3;
  }

	return sum / 2.0;
}


/* Return 1 for true, 0 for false */
static int
_lwt_EdgeRingIsCCW(LWT_EDGERING *ring)
{
  double sa;

  LWDEBUGF(2, "_lwt_EdgeRingIsCCW, ring has %d elems", ring->size);
  LWT_EDGERING_POINT_ITERATOR *it = _lwt_EdgeRingIterator_begin(ring);
  sa = _lwt_EdgeRingSignedArea(it);
  LWDEBUGF(2, "_lwt_EdgeRingIsCCW, signed area is %g", sa);
  lwfree(it);
  if ( sa >= 0 ) return 0;
  else return 1;
}

static int
_lwt_EdgeRingCrossingCount(const POINT2D *p, LWT_EDGERING_POINT_ITERATOR *it)
{
	int cn = 0;    /* the crossing number counter */
	POINT2D v1, v2;
#ifndef RELAX
  POINT2D v0;
#endif

  if ( ! _lwt_EdgeRingIterator_next(it, &v1) ) return cn;
  v0 = v1;
	while ( _lwt_EdgeRingIterator_next(it, &v2) )
	{
		double vt;

		/* edge from vertex i to vertex i+1 */
		if
		(
		    /* an upward crossing */
		    ((v1.y <= p->y) && (v2.y > p->y))
		    /* a downward crossing */
		    || ((v1.y > p->y) && (v2.y <= p->y))
		)
		{

			vt = (double)(p->y - v1.y) / (v2.y - v1.y);

			/* P->x <intersect */
			if (p->x < v1.x + vt * (v2.x - v1.x))
			{
				/* a valid crossing of y=p->y right of p->x */
				++cn;
			}
		}
		v1 = v2;
	}

	LWDEBUGF(3, "_lwt_EdgeRingCrossingCount returning %d", cn);

#ifndef RELAX
  if ( memcmp(&v1, &v0, sizeof(POINT2D)) )
  {
    lwerror("_lwt_EdgeRingCrossingCount: V[n] != V[0] (%g %g != %g %g)",
      v1.x, v1.y, v0.x, v0.y);
    return -1;
  }
#endif

  return cn;
}

/* Return 1 for true, 0 for false */
static int
_lwt_EdgeRingContainsPoint(LWT_EDGERING *ring, POINT2D *p)
{
  int cn = 0;

  LWT_EDGERING_POINT_ITERATOR *it = _lwt_EdgeRingIterator_begin(ring);
  cn = _lwt_EdgeRingCrossingCount(p, it);
  lwfree(it);
	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}

static GBOX *
_lwt_EdgeRingGetBbox(LWT_EDGERING *ring)
{
  int i;

  if ( ! ring->env )
  {
    LWDEBUGF(2, "Computing GBOX for ring %p", ring);
    for (i=0; i<ring->size; ++i)
    {
      LWT_EDGERING_ELEM *elem = ring->elems[i];
      LWLINE *g = elem->edge->geom;
      const GBOX *newbox = lwgeom_get_bbox(lwline_as_lwgeom(g));
      if ( ! i ) ring->env = gbox_clone( newbox );
      else gbox_merge( newbox, ring->env );
    }
  }

  return ring->env;
}

static LWT_ELEMID
_lwt_EdgeRingGetFace(LWT_EDGERING *ring)
{
  LWT_EDGERING_ELEM *el = ring->elems[0];
  return el->left ? el->edge->face_left : el->edge->face_right;
}


/*
 * Register a face on an edge side
 *
 * Create and register face to shell (CCW) walks,
 * register arbitrary negative face_id to CW rings.
 *
 * Push CCW rings to shells, CW rings to holes.
 *
 * The ownership of the "geom" and "ids" members of the
 * LWT_EDGERING pushed to the given LWT_EDGERING_ARRAYS
 * are transferred to caller.
 *
 * @param side 1 for left side, -1 for right side
 *
 * @param holes an array where holes will be pushed
 *
 * @param shells an array where shells will be pushed
 *
 * @param registered id of registered face. It will be a negative number
 *  for holes or isolated edge strips (still registered in the face
 *  table, but only temporary).
 *
 * @return 0 on success, -1 on error.
 *
 */
static int
_lwt_RegisterFaceOnEdgeSide(LWT_TOPOLOGY *topo, LWT_ISO_EDGE *edge,
                            int side, LWT_ISO_EDGE_TABLE *edges,
                            LWT_EDGERING_ARRAY *holes,
                            LWT_EDGERING_ARRAY *shells,
                            LWT_ELEMID *registered)
{
  const LWT_BE_IFACE *iface = topo->be_iface;
  /* this is arbitrary, could be taken as parameter */
  static const int placeholder_faceid = LWT_HOLES_FACE_PLACEHOLDER;
  LWT_EDGERING *ring;

  /* Get edge ring */
  ring = _lwt_BuildEdgeRing(topo, edges, edge, side);

	LWDEBUG(2, "Ring built, calling EdgeRingIsCCW");

  /* Compute winding (CW or CCW?) */
  int isccw = _lwt_EdgeRingIsCCW(ring);

  if ( isccw )
  {
    /* Create new face */
    LWT_ISO_FACE newface;

    LWDEBUGF(1, "Ring of edge %d is a shell (shell %d)", edge->edge_id * side, shells->size);

    newface.mbr = _lwt_EdgeRingGetBbox(ring);

    newface.face_id = -1;
    /* Insert the new face */
    int ret = lwt_be_insertFaces( topo, &newface, 1 );
    newface.mbr = NULL;
    if ( ret == -1 )
    {
      PGTOPO_BE_ERROR();
      return -1;
    }
    if ( ret != 1 )
    {
      lwerror("Unexpected error: %d faces inserted when expecting 1", ret);
      return -1;
    }
    /* return new face_id */
    *registered = newface.face_id;
    LWT_EDGERING_ARRAY_PUSH(shells, ring);

    /* update ring edges set new face_id on resp. side to *registered */
    ret = _lwt_UpdateEdgeRingSideFace(topo, ring, *registered);
    if ( ret )
    {
        lwerror("Errors updating edgering side face: %s",
                lwt_be_lastErrorMessage(iface));
        return -1;
    }

  }
  else /* cw, so is an hole */
  {
    LWDEBUGF(1, "Ring of edge %d is a hole (hole %d)", edge->edge_id * side, holes->size);
    *registered = placeholder_faceid;
    LWT_EDGERING_ARRAY_PUSH(holes, ring);
  }

  return 0;
}

static void
_lwt_AccumulateCanditates(void* item, void* userdata)
{
  LWT_EDGERING_ARRAY *candidates = userdata;
  LWT_EDGERING *sring = item;
  LWT_EDGERING_ARRAY_PUSH(candidates, sring);
}

static LWT_ELEMID
_lwt_FindFaceContainingRing(LWT_TOPOLOGY* topo, LWT_EDGERING *ring,
                            LWT_EDGERING_ARRAY *shells)
{
  LWT_ELEMID foundInFace = -1;
  int i;
  const GBOX *minenv = NULL;
  POINT2D pt;
  const GBOX *testbox;
  GEOSGeometry *ghole;

  getPoint2d_p( ring->elems[0]->edge->geom->points, 0, &pt );

  testbox = _lwt_EdgeRingGetBbox(ring);

  /* Create a GEOS Point from a vertex of the hole ring */
  {
    LWPOINT *point = lwpoint_make2d(topo->srid, pt.x, pt.y);
    ghole = LWGEOM2GEOS( lwpoint_as_lwgeom(point), 1 );
    lwpoint_free(point);
    if ( ! ghole ) {
      lwerror("Could not convert edge geometry to GEOS: %s", lwgeom_geos_errmsg);
      return -1;
    }
  }

  /* Build STRtree of shell envelopes */
  if ( ! shells->tree )
  {
    static const int STRTREE_NODE_CAPACITY = 10;
    LWDEBUG(1, "Building STRtree");
	  shells->tree = GEOSSTRtree_create(STRTREE_NODE_CAPACITY);
    if (shells->tree == NULL)
    {
      lwerror("Could not create GEOS STRTree: %s", lwgeom_geos_errmsg);
      return -1;
    }
    for (i=0; i<shells->size; ++i)
    {
      LWT_EDGERING *sring = shells->rings[i];
      const GBOX* shellbox = _lwt_EdgeRingGetBbox(sring);
      LWDEBUGF(2, "GBOX of shell %p for edge %d is %g %g,%g %g",
        sring, sring->elems[0]->edge->edge_id, shellbox->xmin,
        shellbox->ymin, shellbox->xmax, shellbox->ymax);
      POINTARRAY *pa = ptarray_construct(0, 0, 2);
      POINT4D pt;
      LWLINE *diag;
      pt.x = shellbox->xmin;
      pt.y = shellbox->ymin;
      ptarray_set_point4d(pa, 0, &pt);
      pt.x = shellbox->xmax;
      pt.y = shellbox->ymax;
      ptarray_set_point4d(pa, 1, &pt);
      diag = lwline_construct(topo->srid, NULL, pa);
      /* Record just envelope in ggeom */
      /* making valid, probably not needed */
      sring->genv = LWGEOM2GEOS( lwline_as_lwgeom(diag), 1 );
      lwline_free(diag);
      GEOSSTRtree_insert(shells->tree, sring->genv, sring);
    }
    LWDEBUG(1, "STRtree build completed");
  }

  LWT_EDGERING_ARRAY candidates;
  LWT_EDGERING_ARRAY_INIT(&candidates);
	GEOSSTRtree_query(shells->tree, ghole, &_lwt_AccumulateCanditates, &candidates);
  LWDEBUGF(1, "Found %d candidate shells containing first point of ring's originating edge %d",
          candidates.size, ring->elems[0]->edge->edge_id * ( ring->elems[0]->left ? 1 : -1 ) );

  /* TODO: sort candidates by bounding box size */

  for (i=0; i<candidates.size; ++i)
  {
    LWT_EDGERING *sring = candidates.rings[i];
    const GBOX* shellbox = _lwt_EdgeRingGetBbox(sring);
    int contains = 0;

    if ( sring->elems[0]->edge->edge_id == ring->elems[0]->edge->edge_id )
    {
      LWDEBUGF(1, "Shell %d is on other side of ring",
               _lwt_EdgeRingGetFace(sring));
      continue;
    }

    /* The hole envelope cannot equal the shell envelope */
    if ( gbox_same(shellbox, testbox) )
    {
      LWDEBUGF(1, "Bbox of shell %d equals that of hole ring",
               _lwt_EdgeRingGetFace(sring));
      continue;
    }

    /* Skip if ring box is not in shell box */
    if ( ! gbox_contains_2d(shellbox, testbox) )
    {
      LWDEBUGF(1, "Bbox of shell %d does not contain bbox of ring point",
               _lwt_EdgeRingGetFace(sring));
      continue;
    }

    /* Skip test if a containing shell was already found
     * and this shell's bbox is not contained in the other */
    if ( minenv && ! gbox_contains_2d(minenv, shellbox) )
    {
      LWDEBUGF(2, "Bbox of shell %d (face %d) not contained by bbox "
                  "of last shell found to contain the point",
                  i, _lwt_EdgeRingGetFace(sring));
      continue;
    }

    contains = _lwt_EdgeRingContainsPoint(sring, &pt);
    if ( contains )
    {
      /* Continue until all shells are tested, as we want to
       * use the one with the smallest bounding box */
      /* IDEA: sort shells by bbox size, stopping on first match */
      LWDEBUGF(1, "Shell %d contains hole of edge %d",
               _lwt_EdgeRingGetFace(sring),
               ring->elems[0]->edge->edge_id);
      minenv = shellbox;
      foundInFace = _lwt_EdgeRingGetFace(sring);
    }
  }
  if ( foundInFace == -1 ) foundInFace = 0;

  candidates.size = 0; /* Avoid destroying the actual shell rings */
  LWT_EDGERING_ARRAY_CLEAN(&candidates);

  GEOSGeom_destroy(ghole);

  return foundInFace;
}

/*
 * @return -1 on error (and report error),
 *          1 if faces beside the universal one exist
 *          0 otherwise
 */
static int
_lwt_CheckFacesExist(LWT_TOPOLOGY *topo)
{
  LWT_ISO_FACE *faces;
  int fields = LWT_COL_FACE_FACE_ID;
  uint64_t nelems = 1;
  GBOX qbox;

  qbox.xmin = qbox.ymin = -DBL_MAX;
  qbox.xmax = qbox.ymax = DBL_MAX;
  faces = lwt_be_getFaceWithinBox2D( topo, &qbox, &nelems, fields, 1);
  if (nelems == UINT64_MAX)
  {
	  PGTOPO_BE_ERROR();
	  return -1;
  }
  if ( faces )
  {
    /* we do not call _lwt_release_faces because we are not asking
     * for the MBR so there are no nested objects to release */
    lwfree(faces);
  }
  return nelems;
}

int
lwt_Polygonize(LWT_TOPOLOGY* topo)
{
  /*
     Fetch all edges
     Sort edges by edge_id
     Mark all edges' left and right face as -1
     Iteratively:
       Fetch next edge with left or right face == -1
       For each side with face == -1:
         Find ring on its side
         If ring is CCW:
            create a new face, assign to the ring edges' appropriate side
         If ring is CW (face needs to be same of external):
            assign a negative face_id the ring edges' appropriate side
     Now for each edge with a negative face_id on the side:
       Find containing face (mbr cache and all)
       Update with id of containing face
   */

  const LWT_BE_IFACE *iface = topo->be_iface;
  LWT_ISO_EDGE *edge;
  int numfaces = -1;
  LWT_ISO_EDGE_TABLE edgetable;
  LWT_EDGERING_ARRAY holes, shells;
  int i;
  int err = 0;

  LWT_EDGERING_ARRAY_INIT(&holes);
  LWT_EDGERING_ARRAY_INIT(&shells);

  initGEOS(lwnotice, lwgeom_geos_error);

  /*
   Check if Topology already contains some Face
   (ignoring the Universal Face)
  */
  numfaces = _lwt_CheckFacesExist(topo);
  if ( numfaces != 0 ) {
    if ( numfaces > 0 ) {
      /* Faces exist */
      lwerror("Polygonize: face table is not empty.");
    }
    /* Backend error, message should have been printed already */
    return -1;
  }


  edgetable.edges = _lwt_FetchAllEdges(topo, &(edgetable.size));
  if ( ! edgetable.edges ) {
    if (edgetable.size == 0) {
      /* not an error: no Edges */
      return 0;
    }
    /* error should have been printed already */
    return -1;
  }

  /* Sort edges by ID (to allow btree searches) */
  qsort(edgetable.edges, edgetable.size, sizeof(LWT_ISO_EDGE), compare_iso_edges_by_id);

  /* Mark all edges as unvisited */
  for (i=0; i<edgetable.size; ++i)
    edgetable.edges[i].face_left = edgetable.edges[i].face_right = -1;

  i = 0;
  while (1)
  {
    i = _lwt_FetchNextUnvisitedEdge(topo, &edgetable, i);
    if ( i < 0 ) break; /* end of unvisited */
    edge = &(edgetable.edges[i]);

    LWT_ELEMID newface = -1;

    LWDEBUGF(1, "Next face-missing edge has id:%d, face_left:%d, face_right:%d",
               edge->edge_id, edge->face_left, edge->face_right);
    if ( edge->face_left == -1 )
    {
      err = _lwt_RegisterFaceOnEdgeSide(topo, edge, 1, &edgetable,
                                        &holes, &shells, &newface);
      if ( err ) break;
      LWDEBUGF(1, "New face on the left of edge %d is %d",
                 edge->edge_id, newface);
      edge->face_left = newface;
    }
    if ( edge->face_right == -1 )
    {
      err = _lwt_RegisterFaceOnEdgeSide(topo, edge, -1, &edgetable,
                                        &holes, &shells, &newface);
      if ( err ) break;
      LWDEBUGF(1, "New face on the right of edge %d is %d",
                 edge->edge_id, newface);
      edge->face_right = newface;
    }
  }

  if ( err )
  {
      _lwt_release_edges(edgetable.edges, edgetable.size);
      LWT_EDGERING_ARRAY_CLEAN( &holes );
      LWT_EDGERING_ARRAY_CLEAN( &shells );
      lwerror("Errors fetching or registering face-missing edges: %s",
              lwt_be_lastErrorMessage(iface));
      return -1;
  }

  LWDEBUGF(1, "Found %d holes and %d shells", holes.size, shells.size);

  /* TODO: sort holes by pt.x, sort shells by bbox.xmin */

  /* Assign shells to holes */
  for (i=0; i<holes.size; ++i)
  {
    LWT_ELEMID containing_face;
    LWT_EDGERING *ring = holes.rings[i];

    containing_face = _lwt_FindFaceContainingRing(topo, ring, &shells);
    LWDEBUGF(1, "Ring %d contained by face %" LWTFMT_ELEMID, i, containing_face);
    if ( containing_face == -1 )
    {
      _lwt_release_edges(edgetable.edges, edgetable.size);
      LWT_EDGERING_ARRAY_CLEAN( &holes );
      LWT_EDGERING_ARRAY_CLEAN( &shells );
      lwerror("Errors finding face containing ring: %s",
              lwt_be_lastErrorMessage(iface));
      return -1;
    }
    int ret = _lwt_UpdateEdgeRingSideFace(topo, holes.rings[i], containing_face);
    if ( ret )
    {
      _lwt_release_edges(edgetable.edges, edgetable.size);
      LWT_EDGERING_ARRAY_CLEAN( &holes );
      LWT_EDGERING_ARRAY_CLEAN( &shells );
      lwerror("Errors updating edgering side face: %s",
              lwt_be_lastErrorMessage(iface));
      return -1;
    }
  }

  LWDEBUG(1, "All holes assigned, cleaning up");

  _lwt_release_edges(edgetable.edges, edgetable.size);

  /* delete all shell and hole EDGERINGS */
  LWT_EDGERING_ARRAY_CLEAN( &holes );
  LWT_EDGERING_ARRAY_CLEAN( &shells );

  return 0;
}


