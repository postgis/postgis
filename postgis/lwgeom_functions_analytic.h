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
 * Copyright 2001-2011 Refractions Research Inc.
 *
 **********************************************************************/


#include "lwgeom_rtree.h"

/*
** Public prototypes for analytic functions.
*/

int point_in_polygon_rtree(RTREE_NODE **root, int ringCount, LWPOINT *point);
int point_in_multipolygon_rtree(RTREE_NODE **root, int polyCount, int *ringCounts, LWPOINT *point);
int point_in_polygon(LWPOLY *polygon, LWPOINT *point);
int point_in_multipolygon(LWMPOLY *mpolygon, LWPOINT *pont);

