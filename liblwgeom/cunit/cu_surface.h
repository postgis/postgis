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

#include "liblwgeom_internal.h"
#include "libtgeom.h"
#include "cu_tester.h"

/* Test functions */
void triangle_parse(void);
void tin_parse(void);
void polyhedralsurface_parse(void);
void tin_tgeom(void);
void psurface_tgeom(void);
void surface_dimension(void);
void surface_perimeter(void);
