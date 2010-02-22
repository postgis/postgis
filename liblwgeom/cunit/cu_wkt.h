/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
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
void test_wkt_point(void);
void test_wkt_linestring(void);
void test_wkt_polygon(void);
void test_wkt_multipoint(void);
void test_wkt_multilinestring(void);
void test_wkt_multipolygon(void);
void test_wkt_collection(void);