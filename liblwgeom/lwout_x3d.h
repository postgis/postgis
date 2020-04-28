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
 * Copyright 2011-2017 Arrival 3D, Regina Obe
 *
 **********************************************************************/

/**
* @file X3D output routines.
*
**********************************************************************/
#include <string.h>
#include "liblwgeom_internal.h"
#include "stringbuffer.h"

/** defid is the id of the coordinate can be used to hold other elements DEF='abc' transform='' etc. **/
static int lwgeom_to_x3d3_sb(const LWGEOM *geom, int precision, int opts, const char *defid, stringbuffer_t *sb);

static int asx3d3_point_sb(const LWPOINT *point, int precision, int opts, const char *defid, stringbuffer_t *sb);
static int asx3d3_line_sb(const LWLINE *line, int precision, int opts, const char *defid, stringbuffer_t *sb);

static int
asx3d3_triangle_sb(const LWTRIANGLE *triangle, int precision, int opts, const char *defid, stringbuffer_t *sb);

static int asx3d3_multi_sb(const LWCOLLECTION *col, int precision, int opts, const char *defid, stringbuffer_t *sb);
static int asx3d3_psurface_sb(const LWPSURFACE *psur, int precision, int opts, const char *defid, stringbuffer_t *sb);
static int asx3d3_tin_sb(const LWTIN *tin, int precision, int opts, const char *defid, stringbuffer_t *sb);

static int
asx3d3_collection_sb(const LWCOLLECTION *col, int precision, int opts, const char *defid, stringbuffer_t *sb);
static int ptarray_to_x3d3_sb(POINTARRAY *pa, int precision, int opts, int is_closed, stringbuffer_t *sb );
