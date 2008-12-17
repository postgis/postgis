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

/***********************************************************************
** for Computational Geometry Suite
*/

/* Set-up / clean-up functions */
CU_pSuite register_cg_suite(void);
int init_cg_suite(void);
int clean_cg_suite(void);

/* Test functions */
void test_lw_segment_side(void);
void test_lw_segment_intersects(void);
void test_lwline_crossing_short_lines(void);
void test_lwline_crossing_long_lines(void);

