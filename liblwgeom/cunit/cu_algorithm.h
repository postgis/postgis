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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "lwalgorithm.h"
#include "cu_tester.h"

/***********************************************************************
** for Computational Geometry Suite
*/

/* Test functions */
void test_lw_segment_side(void);
void test_lw_segment_intersects(void);
void test_lwline_crossing_short_lines(void);
void test_lwline_crossing_long_lines(void);
void test_lwpoint_set_ordinate(void);
void test_lwpoint_get_ordinate(void);
void test_lwpoint_interpolate(void);
void test_lwline_clip(void);
void test_lwline_clip_big(void);
void test_lwmline_clip(void);
void test_geohash_precision(void);
void test_geohash_point(void);
void test_geohash(void);
