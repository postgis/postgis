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

#include "../postgis_config.h"

//#define POSTGIS_DEBUG_LEVEL 1
#include "lwgeom_log.h"

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"

#include "lwt_edgeend_star.h"
#include "lwt_edgeend.h"

#include <inttypes.h>

LWT_EDGEEND_STAR *
lwt_edgeEndStar_init( LWT_ELEMID nodeID )
{
  LWT_EDGEEND_STAR *star = lwalloc(sizeof(LWT_EDGEEND_STAR));
  star->numEdgeEnds = 0;
  star->edgeEndsCapacity = 0;
  star->edgeEnds = NULL;
  star->nodeID = nodeID;
  star->sorted = 0;

  return star;
}

void
lwt_edgeEndStar_release( LWT_EDGEEND_STAR *star )
{
  uint64_t i;
  for (i=0; i<star->edgeEndsCapacity; ++i)
  {
    lwfree( star->edgeEnds[i] );
  }
  if ( star->edgeEndsCapacity ) {
    lwfree( star->edgeEnds );
  }
  lwfree( star );
}

void
lwt_edgeEndStar_addEdge( LWT_EDGEEND_STAR *star, const LWT_ISO_EDGE *edge )
{
  int numEdgeEnds = 0;
  LWT_EDGEEND *edgeEnds[2];
  uint64_t newCapacity;

  if ( edge->start_node == star->nodeID )
  {
    LWT_EDGEEND *ee = lwt_edgeEnd_fromEdge( edge, 1 );
    if ( ! ee ) {
      lwerror("Could not construct outgoing EdgeEnd for edge %"
        LWTFMT_ELEMID, edge->edge_id);
      return;
    }
    edgeEnds[numEdgeEnds++] = ee;
  }
  if ( edge->end_node == star->nodeID )
  {
    LWT_EDGEEND *ee = lwt_edgeEnd_fromEdge( edge, 0 );
    if ( ! ee ) {
      lwerror("Could not construct outgoing incoming for edge %"
        LWTFMT_ELEMID, edge->edge_id);
      return;
    }
    edgeEnds[numEdgeEnds++] = ee;
  }

  if ( ! numEdgeEnds )
  {
    lwerror("Edge %" LWTFMT_ELEMID
      " doesn't start nor end on star node %" LWTFMT_ELEMID,
      edge->edge_id, star->nodeID);
    return;
  }

  LWDEBUGF(1, "Edge %" LWTFMT_ELEMID
    " got %" PRIu64 " ends incident to star node %" LWTFMT_ELEMID,
    edge->edge_id, numEdgeEnds, star->nodeID );

  newCapacity = star->numEdgeEnds + numEdgeEnds;
  LWDEBUGF(3, "Current star capacity:%d, required:%d",
    star->edgeEndsCapacity, newCapacity);
  if ( newCapacity > star->edgeEndsCapacity )
  {
    LWDEBUGF(3, "Reallocating edgeEnds from %p to new size %d", star->edgeEnds, newCapacity * sizeof(LWT_EDGEEND *));
    if ( star->edgeEnds ) {
      star->edgeEnds = lwrealloc(star->edgeEnds, newCapacity * sizeof(LWT_EDGEEND *));
    } else {
      star->edgeEnds = lwalloc(newCapacity * sizeof(LWT_EDGEEND *));
    }
    LWDEBUGF(3, "Reallocated edgeEnds are %p", star->edgeEnds);
    star->edgeEndsCapacity = newCapacity;
    LWDEBUGF(3, "New star capacity: %d", newCapacity);
  }

  for (int i=0; i<numEdgeEnds; ++i)
  {
    star->edgeEnds[ star->numEdgeEnds++ ] = edgeEnds[i];
  }
  star->sorted = 0;
}

static int
lwt_edgeEnd_compare(const void *i1, const void *i2)
{
	const LWT_EDGEEND *ee1 = *(const LWT_EDGEEND **)i1;
	const LWT_EDGEEND *ee2 = *(const LWT_EDGEEND **)i2;
  int ret;
	if ( ee1->azimuth < ee2->azimuth )
		ret = -1;
	else if ( ee1->azimuth > ee2->azimuth )
		ret = 1;
	else
		ret = 0;

  LWDEBUGF(4, "qsort comparator for %s edge %d with azimuth %g and %s edge %d with azimuth %g returning %d",
      ee1->outgoing ? "outgoing" : "incoming", ee1->edge->edge_id, ee1->azimuth,
      ee2->outgoing ? "outgoing" : "incoming", ee2->edge->edge_id, ee2->azimuth,
      ret
  );

  return ret;
}

static void
lwt_edgeEndStar_ensureSorted( LWT_EDGEEND_STAR *star )
{
  if ( star->sorted ) return; // nothing to do
  qsort( star->edgeEnds, star->numEdgeEnds, sizeof(LWT_EDGEEND *), lwt_edgeEnd_compare);
  star->sorted = 1;
}

void
lwt_EdgeEndStar_debugPrint( const LWT_EDGEEND_STAR *star )
{
  lwdebug(1, "Star around node %" LWTFMT_ELEMID " has %" PRIu64 " edgeEnds, %s",
    star->nodeID, star->numEdgeEnds, star->sorted ? "sorted" : "unsorted" );
  for ( uint64_t i=0; i<star->numEdgeEnds; ++i )
  {
    LWT_EDGEEND *ee = star->edgeEnds[i];
    lwdebug(1, " EdgeEnd %" PRIu64 " is %s edge %" LWTFMT_ELEMID ", azimuth=%g",
      i, ee->outgoing ? "outgoing" : "incoming",
      ee->edge->edge_id, ee->azimuth
    );
  }
}

const LWT_EDGEEND *
lwt_edgeEndStar_getNextCW( LWT_EDGEEND_STAR *star, LWT_ISO_EDGE *edge, int outgoing )
{
  lwt_edgeEndStar_ensureSorted( star );

  uint64_t i=0;
  LWT_EDGEEND *thisEdgeEnd = NULL;
  for ( i=0; i<star->numEdgeEnds; ++i )
  {
    LWT_EDGEEND *ee = star->edgeEnds[i];
    if ( ee->edge == edge && ee->outgoing == outgoing ) {
      thisEdgeEnd = ee;
      break;
    }
  }
  if ( ! thisEdgeEnd ) {
    lwerror("Could not find %s edge %" LWTFMT_ELEMID " in the star",
      outgoing ?  "outgoing" : "incoming", edge->edge_id);
    return NULL;
  }
  LWT_EDGEEND *nextEdgeEnd = i < star->numEdgeEnds-1 ? star->edgeEnds[i+1] : star->edgeEnds[0];
  return nextEdgeEnd;
}

const LWT_EDGEEND *
lwt_edgeEndStar_getNextCCW( LWT_EDGEEND_STAR *star, LWT_ISO_EDGE *edge, int outgoing )
{
  lwt_edgeEndStar_ensureSorted( star );

  uint64_t i=0;
  LWT_EDGEEND *thisEdgeEnd = NULL;
  for ( i=0; i<star->numEdgeEnds; ++i )
  {
    LWT_EDGEEND *ee = star->edgeEnds[i];
    if ( ee->edge == edge && ee->outgoing == outgoing ) {
      thisEdgeEnd = ee;
      break;
    }
  }
  if ( ! thisEdgeEnd ) {
    lwerror("Could not find %s edge %" LWTFMT_ELEMID " in the star",
      outgoing ?  "outgoing" : "incoming", edge->edge_id);
    return NULL;
  }
  LWT_EDGEEND *nextEdgeEnd = i > 0 ? star->edgeEnds[i-1] : star->edgeEnds[star->numEdgeEnds-1];
  return nextEdgeEnd;
}
