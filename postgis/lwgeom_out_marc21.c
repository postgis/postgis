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
 * Copyright (C) 2021 University of Münster (WWU), Germany
 * Author: Jim Jones <jim.jones@uni-muenster.de>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_transform.h"

/**********************************************************************
 * Ability to serialise MARC21/XML Records from geometries. The coordinates
 * are returned in decimal degrees.
 *
 * MARC21/XML version supported: 1.1
 * MARC21/XML Cartographic Mathematical Data Definition:
 *    https://www.loc.gov/marc/bibliographic/bd034.html
 *
 * Copyright (C) 2021 University of Münster (WWU), Germany
 * Author: Jim Jones <jim.jones@uni-muenster.de>
 *
 **********************************************************************/

PG_FUNCTION_INFO_V1(LWGEOM_asMARC21);
Datum LWGEOM_asMARC21(PG_FUNCTION_ARGS)
{
	lwvarlena_t *marc21;
	int32_t srid;
	LWPROJ *lwproj;
	LWGEOM *lwgeom;
	GSERIALIZED *gs = PG_GETARG_GSERIALIZED_P_COPY(0);
	int precision = PG_GETARG_INT32(1);
	srid = gserialized_get_srid(gs);

	if ( srid == SRID_UNKNOWN )
	{
		PG_FREE_IF_COPY(gs, 0);
		lwerror("ST_AsMARC21: Input geometry has unknown (%d) SRID", srid);
		PG_RETURN_NULL();

	}

	if ( GetLWPROJ(srid, srid, &lwproj) == LW_FAILURE) {
		
		PG_FREE_IF_COPY(gs, 0);
		lwerror("ST_AsMARC21: Failure reading projections from spatial_ref_sys.");
		PG_RETURN_NULL();
		
	}

	if (!lwproj->source_is_latlong){

		PG_FREE_IF_COPY(gs, 0);
		lwerror("ST_AsMARC21: Unsupported SRID (%d). Only lon/lat coordinate systems are supported in MARC21/XML Documents.",srid);		
		PG_RETURN_NULL();
	}

//	if (precision > 15){
//
//		PG_FREE_IF_COPY(gs, 0);
//		lwerror("ST_AsMARC21: The given precision exceeds the maximum allowed (15): %d",precision);
//		PG_RETURN_NULL();
//	}


	if (precision < 0) precision = 0;

	lwgeom = lwgeom_from_gserialized(gs);
	marc21 = lwgeom_to_marc21(lwgeom, precision);

	if (marc21)	PG_RETURN_TEXT_P(marc21);

	PG_RETURN_NULL();
}
