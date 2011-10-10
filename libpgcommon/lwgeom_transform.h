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

char* GetProj4StringSPI(int srid);
void SetPROJ4LibPath(void) ;


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


/**
 * Builtin SRID values
 * @{
 */

/**  Start of the reserved offset */
#define SRID_RESERVE_OFFSET  999000

/**  World Mercator, equivalent to EPSG:3395 */
#define SRID_WORLD_MERCATOR  999000

/**  Start of UTM North zone, equivalent to EPSG:32601 */
#define SRID_NORTH_UTM_START 999001

/**  End of UTM North zone, equivalent to EPSG:32660 */
#define SRID_NORTH_UTM_END   999060

/** Lambert Azimuthal Equal Area North Pole, equivalent to EPSG:3574 */
#define SRID_NORTH_LAMBERT   999061

/** PolarSteregraphic North, equivalent to EPSG:3995 */
#define SRID_NORTH_STEREO    999062

/**  Start of UTM South zone, equivalent to EPSG:32701 */
#define SRID_SOUTH_UTM_START 999101

/**  Start of UTM South zone, equivalent to EPSG:32760 */
#define SRID_SOUTH_UTM_END   999160

/** Lambert Azimuthal Equal Area South Pole, equivalent to EPSG:3409 */
#define SRID_SOUTH_LAMBERT   999161

/** PolarSteregraphic South, equivalent to EPSG:3031 */
#define SRID_SOUTH_STEREO    999162

/** @} */

