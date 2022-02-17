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

PG_FUNCTION_INFO_V1(ST_GeomFromMARC21);
Datum ST_GeomFromMARC21(PG_FUNCTION_ARGS) {
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	xmlDocPtr xmldoc;
	text *xml_input;
	int xml_size;
	char *xml;
	xmlNodePtr xmlroot = NULL;

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

		//lwgeom_free(lwgeom);
		PG_RETURN_NULL();

	}

	geom = geometry_serialize(lwgeom);

	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(geom);
}

static int is_literal_valid(const char *literal) {

	int num_dec_sep;
	int coord_start;
	int literal_length;

	if(literal == NULL) return LW_FALSE;

	literal_length = strlen(literal);

	POSTGIS_DEBUGF(2, "is_literal_valid called (%s)", literal);

	if (literal_length < 3)	return LW_FALSE;

	coord_start = 0;
	num_dec_sep = 0;

	/**
	 * Shortest allowed literal:
	 *   containing cardinal direction >  cardinal direction + degree with 3 digits
	 *   no cardinal direction >  degree with 3 digits
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

	for (int j = coord_start; j < literal_length; j++) {

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
	 * Coordinate formats supported (from the MARC21/XML documentation):
	 *
	 *  -> hdddmmss (hemisphere-degrees-minutes-seconds)
	 *  -> hddd.dddddd (hemisphere-degrees.decimal degrees)
	 *  -> +-ddd.dddddd (hemisphere[+/-]-degrees.decimal degrees)
	 *     (“+” for N and E, “-“ for S and W; the plus sign is optional)
	 *  -> hdddmm.mmmm (hemisphere-degrees-minutes.decimal minutes):
	 *  -> hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
	 *
	 */

	char *dgr;
	char *min;
	char *sec;
	size_t literal_length;

	char start_character = literal[0];
	int start_literal = 0;
	double result = 0.0;

	const size_t numdigits_degrees = 3;
	const size_t numdigits_minutes = 2;
	const size_t numdigits_seconds = 2;

	POSTGIS_DEBUGF(2, "parse_geo_literal called (%s)", literal);
	POSTGIS_DEBUGF(2, "  start character: %c", start_character);

	literal_length = strlen(literal);

	if (!isdigit(start_character)) start_literal = 1;

	POSTGIS_DEBUGF(2, "    start_literal=%d", start_literal);

	dgr = palloc(sizeof(char)*numdigits_degrees+1);
	snprintf(dgr, numdigits_degrees+1, "%s", &literal[start_literal]);

	if (strchr(literal, '.') == NULL && strchr(literal, ',') == NULL) {

		/**
		 * degrees/minutes/seconds: hdddmmss (hemisphere-degrees-minutes-seconds)
		 * Ex.  0793235
		 *     +0793235
		 *     E0793235
		 *     ||  | |-> seconds (2 digits; left padded with 0)
		 *     ||  |-> minutes (2 digits; left padded with 0)
		 *     ||-> degrees (3 digits; left padded with 0)
		 *     |-> start_literal (1 digit; optional)
		 */

		POSTGIS_DEBUG(2, "  lat/lon integer coordinates detected");
		POSTGIS_DEBUGF(2, "    parsed degrees (lon/lat): %s", dgr);

		/* literal contain at least degrees.
		 * minutes and seconds are optional */
		result = atof(dgr);

		/* checks if the literal contains minutes */
		if (literal_length > (start_literal + numdigits_degrees)) {

			min = palloc(sizeof(char)*numdigits_minutes+1);
			snprintf(min, numdigits_minutes+1, "%s", &literal[start_literal+numdigits_degrees]);
			POSTGIS_DEBUGF(2, "    parsed minutes (lon/lat): %s", min);
			result = result + atof(min) / 60;
			pfree(min);

			/* checks if the literal contains seconds */
			if (literal_length >= (start_literal + numdigits_degrees + numdigits_minutes)) {

				sec = palloc(sizeof(char)*numdigits_seconds+1);
				snprintf(sec, numdigits_seconds+1, "%s", &literal[start_literal+numdigits_degrees+numdigits_minutes]);
				POSTGIS_DEBUGF(2, "    parsed seconds (lon/lat): %s", sec);

				result = result + atof(sec) / 3600;
				pfree(sec);

			}


		}


	} else {

		POSTGIS_DEBUG(2, "  decimal coordinates detected");

		if (strchr(literal, ',')) {

			/* changes the literal decimal sign from comma to period to avoid problems with atof.
			 * from the docs "In MARC21/XML coordinates, the decimal sign may be either a period or a comma." */

			literal[literal_length-strlen(strchr(literal, ','))]='.';
			POSTGIS_DEBUGF(2, "  decimal separator changed to '.': %s",literal);

		}

		/* checks if the literal is encoded in decimal degrees */
		if (literal[start_literal + numdigits_degrees] == '.') {

			/**
			 * decimal degrees: hddd.dddddd (hemisphere-degrees.decimal degrees)
			 * Ex. E079.533265
			 *     +079.533265
			 *     ||  |-> indicates decimal degrees
			 *     ||-> degrees (3 digits; left padded with 0)
			 *     |-> start_literal (1 digit; optional)
			 */

			char *dec = palloc(sizeof(char)*literal_length+1);
			snprintf(dec, literal_length+1, "%s", &literal[start_literal]);
			result = atof(dec);

			POSTGIS_DEBUGF(2, "    parsed decimal degrees: %s", dec);
			pfree(dec);

		/* checks if the literal is encoded in decimal minutes */
		} else if (literal[start_literal + numdigits_degrees + numdigits_minutes] == '.') {

			/**
			 * decimal minutes: hdddmm.mmmm (cardinal direction|degrees|minutes.decimal minutes)
			 * Ex. E07932.5332
			 *     ||  | |-> indicates decimal minutes
			 *     ||  |-> minutes (2 digits; left padded with 0)
			 *     ||-> degrees (3 digits; left padded with 0)
			 *     |-> start_literal (1 digit; optional)
			 */

			size_t len_decimal_minutes = literal_length - (start_literal + numdigits_degrees);

			min = palloc(sizeof(char)*len_decimal_minutes+1);
			snprintf(min, len_decimal_minutes+1, "%s", &literal[start_literal + numdigits_degrees]);

			POSTGIS_DEBUGF(2, "    parsed degrees: %s", dgr);
			POSTGIS_DEBUGF(2, "    parsed decimal minutes: %s", min);

			result = atof(dgr) + (atof(min) / 60);

			pfree(min);

		/* checks if the literal is encoded in decimal seconds */
		} else if (literal[start_literal + numdigits_degrees + numdigits_minutes + numdigits_seconds] == '.') {

			/**
			 * decimal seconds: hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
			 * Ex. E0793235.575
			 *     ||  | | |-> indicates decimal seconds
			 *     ||  | |-> seconds (2 digits; left padded with 0)
			 *     ||  |-> minutes (2 digits; left padded with 0)
			 *     ||-> degrees (3 digits; left padded with 0)
			 *     |-> start_literal (1 digit; optional)
			 */

			size_t len_decimal_seconds = literal_length - (start_literal + numdigits_degrees + numdigits_minutes);

			min = palloc(sizeof(char)*numdigits_minutes+1);
			snprintf(min, numdigits_minutes+1, "%s", &literal[start_literal + numdigits_degrees]);

			sec = palloc(sizeof(char)*len_decimal_seconds+1);
			snprintf(sec, len_decimal_seconds+1, "%s", &literal[start_literal + numdigits_degrees + numdigits_minutes]);

			result = atof(dgr) + (atof(min) / 60) + (atof(sec) / 3600);

			POSTGIS_DEBUGF(2, "    parsed degrees: %s", dgr);
			POSTGIS_DEBUGF(2, "    parsed minutes: %s", min);
			POSTGIS_DEBUGF(2, "    parsed decimal seconds: %s", sec);
			pfree(min);
			pfree(sec);

		}

	}

	/**
	 * “+” for N and E (the plus sign is optional)
	 * “-“ for S and W
	 */

	pfree(dgr);

	if (start_character == 'S' || start_character == 'W' || start_character == '-') {

		POSTGIS_DEBUGF(2, "  switching sign due to start character: '%c'", start_character);
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
