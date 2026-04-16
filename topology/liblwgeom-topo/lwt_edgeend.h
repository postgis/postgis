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

#ifndef _LWT_EDGEEND_H
#define _LWT_EDGEEND_H 1

#include "liblwgeom.h"
#include "liblwgeom_topo.h"

/* Single edge end */
typedef struct LWT_EDGEEND_t {
  const LWT_ISO_EDGE *edge; /* Edge this edgeend comes from (parent edge) */
  POINT2D p0; /* point incident to the node */
  POINT2D p1; /* point away from the node */
  int outgoing; /* 1 if p0 is the start_node of the parent edge, 0 if it is the end_node of it */
  double azimuth; /* azimuth of the edge end (p0 to p1) */
} LWT_EDGEEND;

/*
 * @param edge an edge, needs have a geometry
 * @param outgoing 1 if p0 is the start_node of the parent edge, 0 if it is the end_node of it
 */
LWT_EDGEEND *lwt_edgeEnd_fromEdge( const LWT_ISO_EDGE *edge, int outgoing );

/*
 * Release memory owned by an edgeend
 */
void lwt_edgeEnd_release( LWT_EDGEEND *end );

#endif /* !defined _LWT_EDGEEND_H  */
