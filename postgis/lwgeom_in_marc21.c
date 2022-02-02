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
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include <math.h>
#include "liblwgeom.h"

static LWGEOM* parse_marc21(xmlNodePtr xnode);

/**********************************************************************
 * Ability to parse geographic data contained in MARC21/XML records
 * to return an LWGEOM or an error message. It returns NULL if the
 * MARC21/XML record is valid but does not contain any geographic
 * data (datafield:034).
 *
 * MARC21/XML version supported: 1.1
 * MARC21/XML Cartographic Mathematical Data Definition:
 *    https://www.loc.gov/marc/bibliographic/bd034.html
 *
 * Copyright (C) 2021 University of Münster (WWU), Germany
 * Written by Jim Jones <jim.jones@uni-muenster.de>
 *
 **********************************************************************/

PG_FUNCTION_INFO_V1(geom_from_marc21);
Datum geom_from_marc21(PG_FUNCTION_ARGS) {
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	xmlDocPtr xmldoc;
	text *xml_input;
	int xml_size;
	char *xml;
	xmlNodePtr xmlroot = NULL;

	/*
	 * Get the MARC21/XML stream
	 */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();

	xml_input = PG_GETARG_TEXT_P(0);
	xml = text_to_cstring(xml_input);
	xml_size = VARSIZE_ANY_EXHDR(xml_input);

	xmlInitParser();
	xmldoc = xmlReadMemory(xml, xml_size, NULL, NULL, XML_PARSE_SAX1);

	if (!xmldoc || (xmlroot = xmlDocGetRootElement(xmldoc)) == NULL) {
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
		lwpgerror("invalid MARC21/XML document.");
	}

	lwgeom = parse_marc21(xmlroot);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();

	if (lwgeom == NULL) {

		lwgeom_free(lwgeom);
		PG_RETURN_NULL();

	}

	geom = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(geom);
}

static int is_literal_valid(char *literal) {

	int j;
	int num_dec_sep;
	int coord_start;
	int literal_length = strlen(literal);

	POSTGIS_DEBUGF(2, "is_literal_valid called (%s)", literal);

	if (literal_length < 3)	return LW_FALSE;

	coord_start = 0;
	num_dec_sep = 0;

	/**
	 * Shortest allowed literal:
	 *   containing hemisphere sign >  hemisphere sign + degree with 3 digits
	 *   no hemisphere sign >  degree with 3 digits
	 *
	 * The variable "coord_start" stores the position where the coordinates start (0 or 1),
	 * so that the script can identify where each literal element begins to validate its content.
	 */

	if (literal[0] == 'N' || literal[0] == 'E' || literal[0] == 'S'	|| literal[0] == 'W' || literal[0] == '+' || literal[0] == '-') {

		if (literal_length < 4) {
			POSTGIS_DEBUGF(3, "  invalid literal length (%d): \"%s\"", literal_length, literal);
			return LW_FALSE;
		}

		coord_start = 1;
	}

	for (j = coord_start; j < literal_length; j++) {

		if (!isdigit(literal[j])) {


			if (j < 3) {

				/**
				 * The first three characters represent the degrees and they must be numeric.
				 */

				POSTGIS_DEBUGF(3,"  invalid character '%c' at the degrees section: \"%s\"",	literal[j], literal);
				return LW_FALSE;

			}

			/**
			 * Decimal degrees are encoded with after the 3rd character with either a dot or a comma.
			 * Only one decimal separator is allowed.
			 */
			if (literal[j] == '.' || literal[j] == ',') {

				num_dec_sep++;

				if (num_dec_sep > 1) return LW_FALSE;

			} else {
				POSTGIS_DEBUGF(3, "  invalid character '%c' in %d: \"%s\"",	literal[j], j, literal);
				return LW_FALSE;

			}

		}

	}

	POSTGIS_DEBUGF(2, "=> is_literal_valid returns LW_TRUE for \"%s\"",	literal);
	return LW_TRUE;

}

static double parse_geo_literal(char *literal) {

	/**
	 * Coordinate formats supported by MARC21/XML:
	 *
	 *  -> hdddmmss (hemisphere-degrees-minutes-seconds)
	 *  -> hddd.dddddd (hemisphere-degrees.decimal degrees)
	 *  -> +-ddd.dddddd (hemisphere[+/-]-degrees.decimal degrees)
	 *     (“+” for N and E, “-“ for S and W; the plus sign is optional)
	 *  -> hdddmm.mmmm (hemisphere-degrees-minutes.decimal minutes):
	 *  -> hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
	 *
	 */

	//char dgr[3];
	char *dgr;
	char *min;
	char *sec;
	char *dec;
	char *csl;

	int literal_length;

	char hemisphere_sign = literal[0];
	int start_literal = 0;
	double result = 0.0;

	POSTGIS_DEBUGF(2, "parse_geo_literal called (%s)", literal);
	POSTGIS_DEBUGF(2, "  start character: %c", hemisphere_sign);

	literal_length = strlen(literal);

	if (!isdigit(hemisphere_sign)) {

		start_literal = 1;
	}

	POSTGIS_DEBUGF(2, "    var start_literal=%d", start_literal);

	//memset(dgr, '\0', sizeof(dgr));
	dgr = malloc(literal_length);
	memcpy(dgr, &literal[start_literal], 3);
	dgr[3]='\0';

	if (strchr(literal, '.') == NULL && strchr(literal, ',') == NULL) {

		/**
		 * degrees/minutes/seconds: hdddmmss (hemisphere-degrees-minutes-seconds)
		 * Ex. +0793235
		 *     E0793235
		 *     ||  | |_seconds
		 *     ||  |_minutes
		 *     ||_degrees
		 *     |_start_literal (hemisphere)
		 */

		POSTGIS_DEBUG(2, "  lat/lon literal detected");
		POSTGIS_DEBUGF(2, "  degrees: %s", dgr);

		POSTGIS_DEBUGF(5, "  min = malloc(%d)", literal_length-5);
		min = malloc(literal_length - 5);

		if (literal_length > (start_literal + 3)) {

			memcpy(min, &literal[start_literal + 3], 2);
			min[2] = '\0';
		}

		POSTGIS_DEBUGF(2, "  lat/lon minutes: %s", min);
		POSTGIS_DEBUGF(5, "  sec = malloc(%d)", literal_length-5);
		sec = malloc(literal_length - 5);

		if (literal_length >= (start_literal + 5)) {

			memcpy(sec, &literal[start_literal + 5], 2);
			sec[2] = '\0';
		}

		POSTGIS_DEBUGF(2, "  lat/lon seconds: %s", sec);
		result = atof(dgr);
		if (min)
			result = result + atof(min) / 60;
		if (sec)
			result = result + atof(sec) / 3600;

		free(min);
		POSTGIS_DEBUG(5, "  free(min)");
		free(sec);
		POSTGIS_DEBUG(5, "  free(sec)");

	} else {

		POSTGIS_DEBUG(2, "  decimal literal detected");

		if (strchr(literal, ',')) {

			/**
			 * Changes the literal decimal sign from comma to period to avoid problems with atof.
			 * -> In MARC21/XML coordinates, the decimal sign may be either a period or a comma.
			 **/
			csl = malloc(literal_length);

			memcpy(csl, &literal[0],literal_length - strlen(strchr(literal, ',')));
			csl[literal_length - strlen(strchr(literal, ','))] = '\0';

			strcat(csl, ".");
			strcat(csl, strchr(literal, ',') + 1);

			memcpy(literal, &csl[0], literal_length);
			free(csl);

			POSTGIS_DEBUGF(3, "  new literal value (replaced comma): %s",literal);

		}

		POSTGIS_DEBUGF(2, "  degrees: %s", dgr);

		if (literal[start_literal + 3] == '.') {

			/**
			 * decimal degrees: hddd.dddddd (hemisphere-degrees.decimal degrees)
			 * Ex. E079.533265
			 *     +079.533265
			 *     ||  |_ indicates decimal degrees
			 *     ||_ degrees
			 *     |_ start_literal (hemisphere)
			 */
			POSTGIS_DEBUGF(5, "  dec = malloc(%d)",literal_length-start_literal);
			dec = malloc(literal_length - start_literal);

			memcpy(dec, &literal[start_literal],literal_length - start_literal);
			dec[literal_length - start_literal] = '\0';

			result = atof(dec);
			POSTGIS_DEBUGF(2, "  decimal degrees: %s", dec);
			POSTGIS_DEBUG(5, "  free(dec)");
			free(dec);

		} else if (literal[start_literal + 5] == '.') {

			/**
			 * decimal minutes: hdddmm.mmmm (hemisphere-degrees-minutes.decimal minutes)
			 * Ex. E07932.5332
			 *     ||  | |_ indicates decimal minutes
			 *     ||  |_ minutes
			 *     ||_ degrees
			 *     |_ start_literal (hemisphere)
			 */
			POSTGIS_DEBUGF(5, "  min = malloc(%d)", literal_length);

			min = malloc(literal_length);
			memcpy(min, &literal[start_literal + 3],literal_length - (start_literal + 3));
			min[literal_length - (start_literal + 3)] = '\0';
			POSTGIS_DEBUGF(2, "  decimal minutes: %s", min);

			result = atof(dgr) + (atof(min) / 60);

			POSTGIS_DEBUG(5, "  free(min)");
			free(min);

		} else if (literal[start_literal + 7] == '.') {

			/**
			 * decimal seconds: hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
			 * Ex. E0793235.575
			 *     ||  | | |_indicates decimal seconds
			 *     ||  | |_seconds
			 *     ||  |_minutes
			 *     ||_degrees
			 *     |_start_literal (hemisphere)
			 */
			POSTGIS_DEBUGF(5, "  min = malloc(%d)", literal_length);
			min = malloc(literal_length);
			memcpy(min, &literal[start_literal + 3], 2);
			min[2] = '\0';

			POSTGIS_DEBUGF(5, "  sec = malloc(%d)", literal_length);
			sec = malloc(literal_length);
			memcpy(sec, &literal[start_literal + 5],literal_length - (start_literal + 5));
			sec[literal_length - (start_literal + 5)] = '\0';

			result = atof(dgr) + (atof(min) / 60) + (atof(sec) / 3600);
			POSTGIS_DEBUGF(2, "  minutes: %s", min);
			POSTGIS_DEBUGF(2, "  decimal seconds: %s", sec);

			POSTGIS_DEBUG(5, "  free(min)");
			free(min);
			POSTGIS_DEBUG(5, "  free(dec)");
			free(sec);

		}

	}

	/**
	 * “+” for N and E (the plus sign is optional)
	 * “-“ for S and W
	 */

	free(dgr);

	if (hemisphere_sign == 'S' || hemisphere_sign == 'W' || hemisphere_sign == '-') {

		POSTGIS_DEBUGF(2, "  switching sign due to start character: '%c'", hemisphere_sign);
		result = -result;

	}

	POSTGIS_DEBUGF(2, "=> parse_geo_literal returns: %.*f (in decimal degrees)", literal_length-(3+start_literal), result);
	return result;
}

static LWGEOM*
parse_marc21(xmlNodePtr xnode) {

	int ngeoms;
	int i;
	xmlNodePtr datafield;
	xmlNodePtr subfield;
	LWGEOM *result;
	LWGEOM **lwgeoms = (LWGEOM**) lwalloc(sizeof(LWGEOM*));
	uint8_t geometry_type;
	uint8_t result_type;
	char *code;
	char *literal;

	POSTGIS_DEBUGF(2, "parse_marc21 called: root '<%s>'", xnode->name);

	/**
	 * MARC21/XML documents must have <record> as top level element.
	 * https://www.loc.gov/standards/marcxml/xml/spy/spy.html
	 */

	if (xmlStrcmp(xnode->name, (xmlChar*) "record")) lwpgerror("invalid MARC21/XML document. Root element <record> expected but <%s> found.",xnode->name);

	result_type = 0;
	ngeoms = 0;

	for (datafield = xnode->children; datafield != NULL; datafield = datafield->next) {

		char *lw = NULL;
		char *le = NULL;
		char *ln = NULL;
		char *ls = NULL;

		if (datafield->type != XML_ELEMENT_NODE) continue;

		if (xmlStrcmp(datafield->name, (xmlChar*) "datafield") != 0	|| xmlStrcmp(xmlGetProp(datafield, (xmlChar*) "tag"),(xmlChar*) "034") != 0) continue;

		POSTGIS_DEBUG(3, "  datafield found");

		for (subfield = datafield->children; subfield != NULL; subfield = subfield->next) {

			if (subfield->type != XML_ELEMENT_NODE)	continue;
			if (xmlStrcmp(subfield->name, (xmlChar*) "subfield") != 0) continue;

			code = (char*) xmlGetProp(subfield, (xmlChar*) "code");

			if ((strcmp(code, "d") != 0 && strcmp(code, "e") != 0 && strcmp(code, "f") != 0 && strcmp(code, "g")) != 0)	continue;

			literal = (char*) xmlNodeGetContent(subfield);

			POSTGIS_DEBUGF(3, "    subfield code '%s': %s", code, literal);

			if (is_literal_valid(literal) == LW_TRUE) {

				if (strcmp(code, "d") == 0) lw = literal;
				else if (strcmp(code, "e") == 0) le = literal;
				else if (strcmp(code, "f") == 0) ln = literal;
				else if (strcmp(code, "g") == 0) ls = literal;

			} else {

				xmlFreeNode(subfield);
				lwpgerror("parse error - invalid literal at 034$%s: \"%s\"", code, literal);

			}

		}

		xmlFreeNode(subfield);

		if (lw && le && ln && ls) {

			double w = parse_geo_literal(lw);
			double e = parse_geo_literal(le);
			double n = parse_geo_literal(ln);
			double s = parse_geo_literal(ls);
			geometry_type = 0;

			if (ngeoms > 0)	lwgeoms = (LWGEOM**) lwrealloc(lwgeoms,	sizeof(LWGEOM*) * (ngeoms + 1));

			if (fabs(w - e) < 0.0000001f && fabs(n - s) < 0.0000001f) {

				/**
				 *  If the coordinates are given in terms of a center point rather than outside limits,
				 *  the longitude and latitude which form the central axis are recorded twice (in subfields
				 *  $d and $e and in $f and $g, respectively).
				 */

				lwgeoms[ngeoms] = (LWGEOM*) lwpoint_make2d(SRID_UNKNOWN, w, s);
				geometry_type = MULTIPOINTTYPE;

			} else {

				lwgeoms[ngeoms] = (LWGEOM*) lwpoly_construct_envelope(SRID_UNKNOWN, w, n, e, s);
				geometry_type = MULTIPOLYGONTYPE;

			}

			if (ngeoms && result_type != geometry_type) {
				result_type = COLLECTIONTYPE;
			} else {
				result_type = geometry_type;
			}

			ngeoms++;

		} else {

			if (lw || le || ln || ls) {

				lwpgerror("parse error - the Coded Cartographic Mathematical Data (datafield:034) in the given MARC21/XML is incomplete. Coordinates for subfields \"$d\",\"$e\",\"$f\" and \"$g\" are expected.");
			}

		}

	}

	POSTGIS_DEBUG(5, "  xmlFreeNode(datafield)");
	xmlFreeNode(datafield);

	if (ngeoms == 1) {

		POSTGIS_DEBUGF(2, "=> parse_marc21 returns single geometry: %s",lwtype_name(lwgeom_get_type(lwgeoms[0])));
		lwgeom_force_clockwise(lwgeoms[0]);
		return lwgeoms[0];

	} else if (ngeoms > 1) {

		result = (LWGEOM*) lwcollection_construct_empty(result_type,SRID_UNKNOWN, 0, 0);

		for (i = 0; i < ngeoms; i++) {

			POSTGIS_DEBUGF(3, "  adding geometry to result set: %s",lwtype_name(lwgeom_get_type(lwgeoms[i])));
			lwgeom_force_clockwise(lwgeoms[i]);
			result = (LWGEOM*) lwcollection_add_lwgeom((LWCOLLECTION*) result,lwgeoms[i]);

		}

		POSTGIS_DEBUGF(2, "=> parse_marc21 returns a collection: %s", lwtype_name(lwgeom_get_type(result)));
		return result;

	}

	/**
	 * MARC21/XML Document has a <record> but no <datafield:034>
	 */
	POSTGIS_DEBUG(2, "=> parse_marc21 returns NULL");
	return NULL;

}
