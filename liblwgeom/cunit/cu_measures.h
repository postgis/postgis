/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom.h"
#include "cu_tester.h"
#include "measures.h"
#include "lwtree.h"

/***********************************************************************
** for Computational Geometry Suite
*/



/* Test functions */
void test_mindistance2d_tolerance(void);
void test_rect_tree_contains_point(void);
void test_rect_tree_intersects_tree(void);

