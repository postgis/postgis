/**********************************************************************
 * $Id:$ 
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "proj_api.h"

projPJ make_project(char *str1);
int transform_point(POINT4D *pt, projPJ srcdefn, projPJ dstdefn);
char* GetProj4StringSPI(int srid);
