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

#include "lwgeom_log.h"
#include "liblwgeom_internal.h"
#include "stringbuffer.h"

static char* format_marc21_literal(double coordinate,int precision);
static int gbox_to_marc21_sb(const GBOX box, int precision, stringbuffer_t *sb);

lwvarlena_t *
lwgeom_to_marc21(const LWGEOM *geom, int precision)
{

	//lwdebug(1,"lwgeom_to_marc21 started: geom %d with %d ",geom->type,precision);

	//POSTGIS_DEBUGF(2,"lwgeom_to_marc21 called: %s with %d ",lwtype_name(lwgeom_get_type(geom)),precision);
	//LWDEBUGF(2," xxxxxxxxxxxxxxxxx lwgeom_to_marc21 called: %s with %d ",lwtype_name(lwgeom_get_type(geom)),precision);

	if( lwgeom_is_empty(geom) )	return NULL;

	char* ns = "http://www.loc.gov/MARC21/slim";
	stringbuffer_t *sb;
	GBOX box;

	sb = stringbuffer_create();
	//lwdebug(1,"lwgeom_to_marc21 -> stringbuffer created");

	/**
	 * Opens the MARX21/XML record
	 */

	if (stringbuffer_aprintf(sb, "<record xmlns=\"%s\">",ns) < 0 ) return NULL;

	if(lwgeom_is_collection(geom)){

		//lwdebug(1,"lwgeom_to_marc21 -> geom is collection (%d)",geom->type);

		int i;
		LWCOLLECTION * coll = (LWCOLLECTION *)geom;

		for (i=0; i<coll->ngeoms; i++) {

			if (lwgeom_calculate_gbox(coll->geoms[i], &box) == LW_FAILURE) {

				stringbuffer_destroy(sb);
				lwerror("failed to calculate bbox for a geometry in the collection: %s",lwtype_name(lwgeom_get_type(coll->geoms[i])));

			}

			if(gbox_to_marc21_sb(box, precision, sb)==LW_FAILURE) {
				stringbuffer_destroy(sb);
				//lwerror("failed to create MARC21/XML for a geometry in the collection: %s",lwgeom_get_type(coll->geoms[i]));
			}

		}


	} else {

		//lwdebug(1,"lwgeom_to_marc21 -> geom is a single feature (%d)",geom->type);

		if (lwgeom_calculate_gbox(geom, &box) == LW_FAILURE) {

			stringbuffer_destroy(sb);
			lwerror("failed to calculate bbox for %s",lwtype_name(lwgeom_get_type(geom)));


		}

		//lwdebug(1,"lwgeom_to_marc21 -> bbox calculated ");

		if(gbox_to_marc21_sb(box, precision, sb)==LW_FAILURE){

			stringbuffer_destroy(sb);
			lwerror("failed to create MARC21/XML for %s",lwtype_name(lwgeom_get_type(geom)));

		}


	}

	/**
	 * Closes the MARX21/XML record
	 */
	if ( stringbuffer_aprintf(sb, "</record>") < 0 ) return LW_FAILURE;

	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}

static char*
format_marc21_literal(double coordinate,int precision)
{
	char *r;
	char *res = malloc(sizeof(coordinate)*2);
	double ds;
	modf(coordinate, &ds);

	sprintf(res, "%.*f",precision,coordinate);

	if(ds<100){

		sprintf(res, "0%.*f",precision,coordinate);

		if(ds>=0 && ds<10) sprintf(res, "00%.*f",precision,coordinate);
		if(ds<=-0 && ds>-10) sprintf(res, "-00%.*f",precision,coordinate*-1);
		if(ds<=-10 && ds>-100) sprintf(res, "-0%.*f",precision,coordinate*-1);
		if(ds<=-100) sprintf(res, "%.*f",precision,coordinate);
	}

	//lwdebug(1,"format_marc21_literal got coordinate (%.*f) and returned (%s)", precision, coordinate, res);

	//TODO: free(res)

	//strncpy(r,&res[0],strlen(res));
	//free(res);

	return res;
	//return r;

}

static int
gbox_to_marc21_sb(const GBOX box, int precision, stringbuffer_t *sb)
{

	if ( stringbuffer_aprintf(sb, "<datafield tag=\"034\" ind1=\"1\" ind2=\" \">") < 0 ) return LW_FAILURE;
	if ( stringbuffer_aprintf(sb, "<subfield code=\"d\">%s</subfield>", format_marc21_literal(box.xmin,precision)) < 0 ) return LW_FAILURE;
	if ( stringbuffer_aprintf(sb, "<subfield code=\"e\">%s</subfield>", format_marc21_literal(box.xmax,precision)) < 0 ) return LW_FAILURE;
	if ( stringbuffer_aprintf(sb, "<subfield code=\"f\">%s</subfield>", format_marc21_literal(box.ymax,precision)) < 0 ) return LW_FAILURE;
	if ( stringbuffer_aprintf(sb, "<subfield code=\"g\">%s</subfield>", format_marc21_literal(box.ymin,precision)) < 0 ) return LW_FAILURE;
	if ( stringbuffer_aprintf(sb, "</datafield>") < 0 ) return LW_FAILURE;

	//lwdebug(1,"gbox_to_marc21_sb: bbox values formatted");

	return LW_SUCCESS;
}
