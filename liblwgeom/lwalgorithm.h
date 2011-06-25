/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <math.h>

enum CG_SEGMENT_INTERSECTION_TYPE {
    SEG_ERROR = -1,
    SEG_NO_INTERSECTION = 0,
    SEG_COLINEAR = 1,
    SEG_CROSS_LEFT = 2,
    SEG_CROSS_RIGHT = 3,
    SEG_TOUCH_LEFT = 4,
    SEG_TOUCH_RIGHT = 5
};

double lw_segment_side(const POINT2D *p1, const POINT2D *p2, const POINT2D *q);
int lw_segment_intersects(const POINT2D *p1, const POINT2D *p2, const POINT2D *q1, const POINT2D *q2);
int lw_segment_envelope_intersects(const POINT2D *p1, const POINT2D *p2, const POINT2D *q1, const POINT2D *q2);


enum CG_LINE_CROSS_TYPE {
    LINE_NO_CROSS = 0,
    LINE_CROSS_LEFT = -1,
    LINE_CROSS_RIGHT = 1,
    LINE_MULTICROSS_END_LEFT = -2,
    LINE_MULTICROSS_END_RIGHT = 2,
    LINE_MULTICROSS_END_SAME_FIRST_LEFT = -3,
    LINE_MULTICROSS_END_SAME_FIRST_RIGHT = 3
};

int lwline_crossing_direction(LWLINE *l1, LWLINE *l2);

double lwpoint_get_ordinate(const POINT4D *p, int ordinate);
void lwpoint_set_ordinate(POINT4D *p, int ordinate, double value);
int lwpoint_interpolate(const POINT4D *p1, const POINT4D *p2, POINT4D *p, int ndims, int ordinate, double interpolation_value);
LWCOLLECTION *lwline_clip_to_ordinate_range(LWLINE *line, int ordinate, double from, double to);
LWCOLLECTION *lwmline_clip_to_ordinate_range(LWMLINE *mline, int ordinate, double from, double to);

int lwgeom_geohash_precision(GBOX bbox, GBOX *bounds);
char *lwgeom_geohash(const LWGEOM *lwgeom, int precision);
char *geohash_point(double longitude, double latitude, int precision);

