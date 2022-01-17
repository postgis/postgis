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

#include "liblwgeom_internal.h"
#include "stringbuffer.h"
#include "lwgeom_log.h"

static int gbox_to_marc21_sb(const GBOX box, int precision, stringbuffer_t *sb);
//static int is_geometry_supported(const LWGEOM *geom);

lwvarlena_t *
lwgeom_to_marc21(const LWGEOM *geom, int precision)
{

	LWDEBUGF(2,"lwgeom_to_marc21 called: %s with precision %d ",lwtype_name(lwgeom_get_type(geom)),precision);

	if(lwgeom_is_empty(geom) )	return NULL;

	char* ns = "http://www.loc.gov/MARC21/slim";
	stringbuffer_t *sb;
	GBOX box;

	LWDEBUG(3,"creating stringbuffer");
	sb = stringbuffer_create();

	LWDEBUGF(3,"opening MARC21/XML record: %s",lwtype_name(lwgeom_get_type(geom)));

	if (stringbuffer_aprintf(sb, "<record xmlns=\"%s\">",ns) < 0 ) return NULL;

	if(lwgeom_is_collection(geom)){

		LWDEBUGF(3,"  collection detected: %s",lwtype_name(lwgeom_get_type(geom)));

		int i;
		LWCOLLECTION * coll = (LWCOLLECTION *)geom;

		//if(!is_geometry_supported(geom)) lwerror("collection type not supported: %s",lwtype_name(lwgeom_get_type(geom)));

		for (i=0; i<coll->ngeoms; i++) {

			//if(!is_geometry_supported(coll->geoms[i])) lwerror("collection contains an unsupported geometry: %s",lwtype_name(lwgeom_get_type(coll->geoms[i])));

			if (lwgeom_calculate_gbox(coll->geoms[i], &box) == LW_FAILURE) {

				stringbuffer_destroy(sb);
				lwerror("failed to calculate bbox for a geometry in the collection: %s",lwtype_name(lwgeom_get_type(coll->geoms[i])));

			}

			if(gbox_to_marc21_sb(box, precision, sb)==LW_FAILURE) {

				stringbuffer_destroy(sb);
				lwerror("failed to create MARC21/XML for a geometry in the collection: %s",lwtype_name(lwgeom_get_type(coll->geoms[i])));

			}

		}


	} else {

		//if(!is_geometry_supported(geom)) lwerror("geometry type not supported: %s",lwtype_name(lwgeom_get_type(geom)));

		LWDEBUGF(3,"  calculating gbox: %s",lwtype_name(lwgeom_get_type(geom)));
		if (lwgeom_calculate_gbox(geom, &box) == LW_FAILURE) {

			stringbuffer_destroy(sb);
			lwerror("failed to calculate bbox for %s",lwtype_name(lwgeom_get_type(geom)));


		}

		LWDEBUGF(3,"  creating MARC21/XML datafield: %s",lwtype_name(lwgeom_get_type(geom)));
		if(gbox_to_marc21_sb(box, precision, sb)==LW_FAILURE){

			stringbuffer_destroy(sb);
			lwerror("failed to create MARC21/XML for %s",lwtype_name(lwgeom_get_type(geom)));

		}


	}

	LWDEBUG(3,"  closing MARC21/XML record");
	if ( stringbuffer_aprintf(sb, "</record>") < 0 ) return LW_FAILURE;

	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}

/**
static int
is_geometry_supported(const LWGEOM *geom){

	if(lwgeom_get_type(geom)!=MULTIPOINTTYPE &&
	   lwgeom_get_type(geom)!=MULTIPOLYGONTYPE &&
	   lwgeom_get_type(geom)!=MULTILINETYPE &&
	   lwgeom_get_type(geom)!=COLLECTIONTYPE &&

	   lwgeom_get_type(geom)!=LINETYPE &&
	   lwgeom_get_type(geom)!=POINTTYPE &&
	   lwgeom_get_type(geom)!=POLYGONTYPE)	{

		return LW_FALSE;

	}

	return LW_TRUE;

}

static char*
format_marc21_literal(double coordinate,int precision)
{

	LWDEBUGF(2,"format_marc21_literal called: %.*f",precision, coordinate);
	LWDEBUGF(5,"format_marc21_literal char *res = malloc(%d);",sizeof(coordinate)*2);
	//char *res = malloc(sizeof(coordinate)*2);
	char *res = malloc(precision+4);
	//char res[sizeof(coordinate)];
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

	LWDEBUGF(2,"=> format_marc21_literal returns: %s",res);
	return res;

}
**/

static int
corner_to_subfield_sb(stringbuffer_t *sb, double coordinate,int precision, char subfield){

	char *res = malloc(precision+4);
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

	if ( stringbuffer_aprintf(sb, "<subfield code=\"%c\">%s</subfield>",subfield,res) < 0 ) return LW_FAILURE;

	//free(////res)
	free(res);
	return LW_SUCCESS;
}

static int
gbox_to_marc21_sb(const GBOX box, int precision, stringbuffer_t *sb)
{

	LWDEBUG(2,"gbox_to_marc21_sb called");

	if (stringbuffer_aprintf(sb, "<datafield tag=\"034\" ind1=\"1\" ind2=\" \">") < 0 ) return LW_FAILURE;
	if(!corner_to_subfield_sb(sb, box.xmin, precision, 'd')) return LW_FAILURE;
	if(!corner_to_subfield_sb(sb, box.xmax, precision, 'e')) return LW_FAILURE;
	if(!corner_to_subfield_sb(sb, box.ymax, precision, 'f')) return LW_FAILURE;
	if(!corner_to_subfield_sb(sb, box.ymin, precision, 'g')) return LW_FAILURE;
	if (stringbuffer_aprintf(sb, "</datafield>") < 0 ) return LW_FAILURE;

	//if ( stringbuffer_aprintf(sb, "<subfield code=\"d\">%s</subfield>", format_marc21_literal(box.xmin,precision)) < 0 ) return LW_FAILURE;
	//if ( stringbuffer_aprintf(sb, "<subfield code=\"e\">%s</subfield>", format_marc21_literal(box.xmax,precision)) < 0 ) return LW_FAILURE;
	//if ( stringbuffer_aprintf(sb, "<subfield code=\"f\">%s</subfield>", format_marc21_literal(box.ymax,precision)) < 0 ) return LW_FAILURE;
	//if ( stringbuffer_aprintf(sb, "<subfield code=\"g\">%s</subfield>", format_marc21_literal(box.ymin,precision)) < 0 ) return LW_FAILURE;


	LWDEBUG(2,"=> gbox_to_marc21_sb returns LW_SUCCESS");

	return LW_SUCCESS;
}
