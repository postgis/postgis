/**********************************************************************
 * $Id: lwgeom_transform.h -1M 2011-08-11 11:15:57Z (local) $ 
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
char* GetProj4StringSPI(int srid);

/**
 * Opaque type to use in the projection cache API.
 */
typedef void *Proj4Cache ;

void SetPROJ4LibPath(void);
Proj4Cache GetPROJ4Cache(FunctionCallInfoData *fcinfo) ;
bool IsInPROJ4Cache(Proj4Cache cache, int srid) ;
void AddToPROJ4Cache(Proj4Cache cache, int srid, int other_srid);
void DeleteFromPROJ4Cache(Proj4Cache cache, int srid) ;
projPJ GetProjectionFromPROJ4Cache(Proj4Cache cache, int srid);


