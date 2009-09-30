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

#include "lwgeodetic.h"
#include "cu_tester.h"

/***********************************************************************
** For new geography library lwgeodetic.h
*/


/* Test functions */
void test_signum(void);
void test_gbox_from_spherical_coordinates(void);
void test_gserialized_get_gbox_geocentric(void);
void test_clairaut(void);
void test_gbox_calculation(void);
void test_edge_intersection(void);
void test_edge_distance_to_point(void);