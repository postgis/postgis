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

#ifndef _LWT_EDGEEND_STAR_H
#define _LWT_EDGEEND_STAR_H 1

#include "liblwgeom_internal.h"
#include "liblwgeom_topo_internal.h"
#include "lwt_edgeend.h"


/* Collection of edge ends around a node */
typedef struct LWT_EDGEEND_STAR_t {
  uint64_t numEdgeEnds;
  uint64_t edgeEndsCapacity;
  LWT_EDGEEND **edgeEnds; /* owned by us */
  LWT_ELEMID nodeID; /* identifier of the node this star belongs to */
  int sorted; /* 0 when the edge ends are not yet sorted by azimuth */
} LWT_EDGEEND_STAR;

/*
 * Initialize an edge end star for a given nodeId
 *
 * Release it with lwt_edgeEndStar_release
 */
LWT_EDGEEND_STAR *lwt_edgeEndStar_init( LWT_ELEMID nodeID );

/*
 * Release memory owned by an edge end star
 */
void lwt_edgeEndStar_release( LWT_EDGEEND_STAR *star );

/* Add an edge to the star
 *
 * It may result in one or two EdgeEnds being added, depending on
 * whether or not both endnodes match the star nodeId.
 *
 */
void lwt_edgeEndStar_addEdge( LWT_EDGEEND_STAR *star, const LWT_ISO_EDGE *edge );

void lwt_EdgeEndStar_debugPrint( const LWT_EDGEEND_STAR *star );

/* Get the next clockwise edgeEnd */
const LWT_EDGEEND *lwt_edgeEndStar_getNextCW( LWT_EDGEEND_STAR *star, LWT_ISO_EDGE *edge, int outgoing );

/* Get the next counterclockwise edgeEnd */
const LWT_EDGEEND *lwt_edgeEndStar_getNextCCW( LWT_EDGEEND_STAR *star, LWT_ISO_EDGE *edge, int outgoing );

#endif /* !defined _LWT_EDGEEND_STAR_H  */
