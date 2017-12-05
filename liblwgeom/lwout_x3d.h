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

/** defid is the id of the coordinate can be used to hold other elements DEF='abc' transform='' etc. **/
static size_t asx3d3_point_size(const LWPOINT *point, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_point(const LWPOINT *point, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_line_size(const LWLINE *line, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_line(const LWLINE *line, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_poly_size(const LWPOLY *poly, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_triangle_size(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_triangle(const LWTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_multi_size(const LWCOLLECTION *col, char *srs, int precisioSn, int opts, const char *defid);
static char *asx3d3_multi(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_psurface(const LWPSURFACE *psur, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_tin(const LWTIN *tin, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_collection_size(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char *asx3d3_collection(const LWCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static size_t pointArray_toX3D3(POINTARRAY *pa, char *buf, int precision, int opts, int is_closed);

static size_t pointArray_X3Dsize(POINTARRAY *pa, int precision);
