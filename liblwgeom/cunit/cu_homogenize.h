/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "libgeom.h"
#include "cu_tester.h"

/**********************************************************************/


/* Test functions */
void test_coll_point(void);
void test_coll_line(void);
void test_coll_poly(void);
void test_coll_coll(void);
void test_geom(void);
