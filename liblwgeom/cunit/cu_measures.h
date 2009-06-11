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

/***********************************************************************
** for Computational Geometry Suite
*/

/* Set-up / clean-up functions */
CU_pSuite register_measures_suite(void);
int init_measures_suite(void);
int clean_measures_suite(void);

/* Test functions */
void test_mindistance2d_recursive_tolerance(void);
