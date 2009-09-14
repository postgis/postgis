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

#include "libgeom.h"

/***********************************************************************
** For new geometry library libgeom.h
*/

/* Set-up / clean-up functions */
CU_pSuite register_libgeom_suite(void);
int init_libgeom_suite(void);
int clean_libgeom_suite(void);

/* Test functions */
void test_typmod_macros(void);
void test_flags_macros(void);
void test_serialized_srid(void);
void test_gserialized_from_lwgeom_size(void);
void test_gserialized_from_lwgeom(void);
void test_lwgeom_from_gserialized(void);
void test_geometry_type_from_string(void);
void test_lwgeom_check_geodetic(void);
void test_lwgeom_count_vertices(void);
void test_on_gser_lwgeom_count_vertices(void);
void test_gbox_from_spherical_coordinates(void);
void test_gserialized_get_gbox_geocentric(void);
void test_gbox_calculation(void);
