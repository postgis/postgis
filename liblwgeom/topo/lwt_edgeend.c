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

#include "lwt_edgeend.h"

void lwt_edgeEnd_release( LWT_EDGEEND *end )
{
  lwfree(end);
}

static int
_lwt_DistinctVertexes2D(const POINTARRAY* pa, int from, int dir, POINT2D *p0, POINT2D *p1)
{
  int i, toofar, inc;

  if ( dir > 0 ) {
    toofar = pa->npoints;
    inc = 1;
  } else {
    toofar = -1;
    inc = -1;
  }

  LWDEBUGF(1, "first point is index %d", from);
  getPoint2d_p(pa, from, p0);
  for ( i = from+inc; i != toofar; i += inc )
  {
    LWDEBUGF(1, "testing point %d", i);
    getPoint2d_p(pa, i, p1); /* pick next point */
    if ( P2D_SAME_STRICT(p0, p1) ) continue; /* equal to startpoint */
    /* this is a good one, neither same of start nor of end point */
    return 1; /* found */
  }

  /* no distinct vertices found */
  return 0;
}

LWT_EDGEEND *
lwt_edgeEnd_fromEdge( const LWT_ISO_EDGE *edge, int outgoing )
{
  LWT_EDGEEND *ee = lwalloc(sizeof(LWT_EDGEEND));
  ee->edge = edge;
  ee->outgoing = outgoing;
  const POINTARRAY *pa = edge->geom->points;

  int ret = _lwt_DistinctVertexes2D(
    edge->geom->points,
    outgoing ? 0 : pa->npoints-1, // from,
    outgoing ? 1 : -1, // dir,
    &(ee->p0),
    &(ee->p1)
  );
  if (!ret)
  {
    lwerror("No distinct vertices found in edge %" LWTFMT_ELEMID, edge->edge_id);
    return NULL;
  }

  if ( ! azimuth_pt_pt(&(ee->p0), &(ee->p1), &(ee->azimuth)) ) {
    lwerror("error computing azimuth of endpoint [%.15g %.15g,%.15g %.15g]",
            ee->p0.x, ee->p0.y,
            ee->p1.x, ee->p1.y
    );
    return NULL;
  }

  LWDEBUGF(1, "Azimuth of segment [%.15g %.15g,%.15g %.15g] = %.15g",
            ee->p0.x, ee->p0.y,
            ee->p1.x, ee->p1.y,
            ee->azimuth
  );

  return ee;
}

