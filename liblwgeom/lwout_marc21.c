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
 * Written by Jim Jones <jim.jones@uni-muenster.de>*
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "stringbuffer.h"
#include "lwgeom_log.h"

static int gbox_to_marc21_sb(const GBOX box, const char* format, stringbuffer_t *sb);
static int is_format_valid(const char* format);

lwvarlena_t*
lwgeom_to_marc21(const LWGEOM *geom, const char* format) {

	LWDEBUGF(2, "lwgeom_to_marc21 called: %s with format %s ", lwtype_name(lwgeom_get_type(geom)), format);

	if (lwgeom_is_empty(geom))	return NULL;
	if (is_format_valid(format)==LW_FALSE) lwerror("invalid output format: \"%s\"", format);

	char *ns = "http://www.loc.gov/MARC21/slim";
	stringbuffer_t *sb;
	GBOX box;

	LWDEBUG(3, "creating stringbuffer");
	sb = stringbuffer_create();

	LWDEBUGF(3, "opening MARC21/XML record: %s", lwtype_name(lwgeom_get_type(geom)));

	if (stringbuffer_aprintf(sb, "<record xmlns=\"%s\">", ns) < 0)
		return NULL;

	if (lwgeom_is_collection(geom)) {

		LWDEBUGF(3, "  collection detected: %s", lwtype_name(lwgeom_get_type(geom)));

		LWCOLLECTION *coll = (LWCOLLECTION*) geom;

		for (uint32_t i = 0; i < coll->ngeoms; i++) {

			if (lwgeom_calculate_gbox(coll->geoms[i], &box) == LW_FAILURE) {

				stringbuffer_destroy(sb);
				lwerror("failed to calculate bbox for a geometry in the collection: %s", lwtype_name(lwgeom_get_type(coll->geoms[i])));

			}

			if (gbox_to_marc21_sb(box, format, sb) == LW_FAILURE) {

				stringbuffer_destroy(sb);
				lwerror("failed to create MARC21/XML for a geometry in the collection: %s",	lwtype_name(lwgeom_get_type(coll->geoms[i])));

			}

		}

	} else {

		LWDEBUGF(3, "  calculating gbox: %s", lwtype_name(lwgeom_get_type(geom)));

		if (lwgeom_calculate_gbox(geom, &box) == LW_FAILURE) {

			stringbuffer_destroy(sb);
			lwerror("failed to calculate bbox for %s", lwtype_name(lwgeom_get_type(geom)));

		}

		LWDEBUGF(3, "  creating MARC21/XML datafield: %s", lwtype_name(lwgeom_get_type(geom)));

		if (gbox_to_marc21_sb(box, format, sb) == LW_FAILURE) {

			stringbuffer_destroy(sb);
			lwerror("failed to create MARC21/XML for %s", lwtype_name(lwgeom_get_type(geom)));

		}

	}

	LWDEBUG(3, "  closing MARC21/XML record");
	if (stringbuffer_aprintf(sb, "</record>") < 0) return LW_FAILURE;

	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}


static int is_format_valid(const char* format){

	char *dec_part = strchr(format, '.');
	if(!dec_part) dec_part = strchr(format, ',');

	if(!dec_part) {

		if (strcmp(format, "hdddmmss") && strcmp(format, "dddmmss")) {

			return LW_FALSE;

		}

	} else {

		if(strlen(dec_part)<2)	return LW_FALSE;

		char *int_part = malloc(strlen(format) - strlen(dec_part));
		memcpy(int_part, &format[0], strlen(format) - strlen(dec_part));
		int_part[strlen(format) - strlen(dec_part)]='\0';

		if (strcmp(int_part,"hddd") && strcmp(int_part,"ddd") &&
			strcmp(int_part,"hdddmm") && strcmp(int_part,"dddmm") &&
			strcmp(int_part,"hdddmmss") && strcmp(int_part,"dddmmss")) {

			free(int_part);
			return LW_FALSE;

		}

		for (size_t i = 1; i < strlen(dec_part); i++) {

			if(dec_part[i]!=int_part[strlen(int_part)-1]) {

				free(int_part);
				return LW_FALSE;
			}
		}

		free(int_part);

	}

	return LW_TRUE;

}

static int corner_to_subfield_sb(stringbuffer_t *sb, double decimal_degrees, const char*format, char subfield) {

	LWDEBUGF(2,"corner_to_subfield_sb called with coordinates: %f and format: %s",decimal_degrees,format);

	char cardinal_direction;

	int degrees = (int) decimal_degrees;
	double minutes = fabs((decimal_degrees-degrees)*60);
	double seconds = fabs((minutes-(int)minutes) *60);

	char decimal_separator;

	int has_cardinal_direction = 0;
	int num_decimals = 0;
	char* res = malloc(strlen(format)+1);

	if((int)(seconds + 0.5)>=60) {

		seconds = seconds-60;
		minutes = minutes +1;

	}


	if(strchr(format, '.')) {
		num_decimals = strlen(strchr(format, '.'))-1;
		decimal_separator = '.';
	}

	if(strchr(format, ',')) {
		num_decimals = strlen(strchr(format, ','))-1;
		decimal_separator = ',';
	}

	if(format[0]=='h'){

		has_cardinal_direction = 1;

		if(subfield=='d'||subfield=='e'){

			if(decimal_degrees>0){

				cardinal_direction='E';

			} else {

				cardinal_direction='W';
				degrees=abs(degrees);
				decimal_degrees= fabs(decimal_degrees);

			}
		}

		if(subfield=='f'||subfield=='g'){

			if(decimal_degrees>0){

				cardinal_direction='N';

			} else {

				cardinal_direction='S';
				degrees=abs(degrees);
				decimal_degrees= fabs(decimal_degrees);

			}
		}


	}

	if(format[3+has_cardinal_direction]=='.' || format[3+has_cardinal_direction]==',' )	{

		/**
		 * decimal degrees
		 */

		int pad_degrees = (int)strlen(format);

		if(decimal_degrees <0 && decimal_degrees>-100) pad_degrees=strlen(format)+1;

		if(has_cardinal_direction) pad_degrees=pad_degrees-1;

		sprintf(res,"%0*.*f",pad_degrees,num_decimals,decimal_degrees);


	} else if(format[5+has_cardinal_direction]=='.' || format[5+has_cardinal_direction]==',' )	{

		/**
		 * decimal minutes
		 */
		int pad_minutes = 0;

		if(minutes<10) pad_minutes = (int)strlen(format)-has_cardinal_direction-3;

		sprintf(res,"%.3d%0*.*f",degrees,pad_minutes,num_decimals,fabs(minutes));

	}

	else if(format[7+has_cardinal_direction]=='.' || format[7+has_cardinal_direction]==',') {

		/*
		 * decimal seconds
		 */
		int pad_seconds = 0;

		if(seconds<10) pad_seconds = (int) strlen(format)-has_cardinal_direction-5;

		sprintf(res,"%.3d%.2d%0*.*f",degrees,(int)minutes,pad_seconds,num_decimals,fabs(seconds));

	} else {

		/**
		 * degrees/minutes/seconds (dddmmss)
		 */

		sprintf(res,"%.3d%.2d%.2d",degrees,(int)minutes,(int)(seconds + 0.5));

	}


	if(decimal_separator==','){

		res[strlen(res)-num_decimals-1] = ',';

	}

	if(has_cardinal_direction){

		if (stringbuffer_aprintf(sb, "<subfield code=\"%c\">%c%s</subfield>", subfield, cardinal_direction, res) < 0) return LW_FAILURE;

	} else {

		if (stringbuffer_aprintf(sb, "<subfield code=\"%c\">%s</subfield>",	subfield, res) < 0)	return LW_FAILURE;

	}


	free(res);
	return LW_SUCCESS;



}

static int gbox_to_marc21_sb(const GBOX box, const char* format, stringbuffer_t *sb) {

	LWDEBUG(2, "gbox_to_marc21_sb called");

	if (stringbuffer_aprintf(sb, "<datafield tag=\"034\" ind1=\"1\" ind2=\" \">") < 0)	return LW_FAILURE;
	if (stringbuffer_aprintf(sb, "<subfield code=\"a\">a</subfield>") < 0) return LW_FAILURE;
	if (!corner_to_subfield_sb(sb, box.xmin, format, 'd')) return LW_FAILURE;
	if (!corner_to_subfield_sb(sb, box.xmax, format, 'e')) return LW_FAILURE;
	if (!corner_to_subfield_sb(sb, box.ymax, format, 'f')) return LW_FAILURE;
	if (!corner_to_subfield_sb(sb, box.ymin, format, 'g')) return LW_FAILURE;
	if (stringbuffer_aprintf(sb, "</datafield>") < 0) return LW_FAILURE;

	LWDEBUG(2, "=> gbox_to_marc21_sb returns LW_SUCCESS");

	return LW_SUCCESS;
}
