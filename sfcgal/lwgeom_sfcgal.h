/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2012-2013 Oslandia <infos@oslandia.com>
 *
 **********************************************************************/

#include "../liblwgeom/lwgeom_sfcgal.h"

/* Conversion from GSERIALIZED* to SFCGAL::Geometry */
sfcgal_geometry_t *POSTGIS2SFCGALGeometry(GSERIALIZED *pglwgeom);

/* Conversion from GSERIALIZED* to SFCGAL::PreparedGeometry */
sfcgal_prepared_geometry_t *POSTGIS2SFCGALPreparedGeometry(GSERIALIZED *pglwgeom);

/* Conversion from SFCGAL::Geometry to GSERIALIZED */
GSERIALIZED *SFCGALGeometry2POSTGIS(const sfcgal_geometry_t *geom, int force3D, int32_t SRID);

/* Conversion from SFCGAL::PreparedGeometry to GSERIALIZED */
GSERIALIZED *SFCGALPreparedGeometry2POSTGIS(const sfcgal_prepared_geometry_t *geom, int force3D);

/* Initialize sfcgal with PostGIS error handlers */
void sfcgal_postgis_init(void);
