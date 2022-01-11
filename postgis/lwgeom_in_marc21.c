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

/**********************************************************************
 * Ability to parse geographic data contained in MARC21/XML documents
 * to return an LWGEOM or an error message. It returns NULL if the
 * document does not contain any geographic data (datafield:034)
 * MARC21/XML version supported: 1.1
 *
 * Copyright (C) 2021 University of Münster (WWU), Germany
 * Author: Jim Jones <jim.jones@uni-muenster.de>
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
//#include <libxml/xpath.h>
#include <string.h>

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

//Datum geom_from_marc21(PG_FUNCTION_ARGS);
static LWGEOM* parse_marc21(xmlNodePtr xnode);

//#define MARC21_NS		((char *) "http://www.loc.gov/MARC21/slim")

/**
 * Ability to parse geographic data contained in MARC21/XML documents
 * to return an LWGEOM or an error message. It returns NULL if the
 * document does not contain any geographic data (MARC 034)
 */
PG_FUNCTION_INFO_V1(geom_from_marc21);
Datum geom_from_marc21(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	xmlDocPtr xmldoc;
	text *xml_input;
	int xml_size;
	char *xml;
	xmlNodePtr xmlroot=NULL;

	/* Get the MARC21/XML stream */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	xml_input = PG_GETARG_TEXT_P(0);
	xml = text_to_cstring(xml_input);
	xml_size = VARSIZE_ANY_EXHDR(xml_input);

	xmlInitParser();
	xmldoc = xmlReadMemory(xml, xml_size, NULL, NULL, XML_PARSE_SAX1);

	if (!xmldoc || (xmlroot = xmlDocGetRootElement(xmldoc)) == NULL)
	{
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
		lwpgerror("invalid MARC21/XML document.");
	}

	lwgeom = parse_marc21(xmlroot);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();

	if(lwgeom == NULL) {

		lwgeom_free(lwgeom);
		PG_RETURN_NULL();

	}

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(geom);
}

static int
is_literal_valid(char* literal){

	int j;
	int coord_start;

	if(strlen(literal)<3) return LW_FAILURE;

	coord_start = 0;

	if(literal[0]=='N' || literal[0]=='E' ||
			literal[0]=='S' || literal[0]=='W' ||
			literal[0]=='+' || literal[0]=='-' ) {

		if(strlen(literal)<4) return LW_FAILURE;

		coord_start = 1;
	}

	for (j=coord_start; j < strlen(literal); j++) {

		if(!isdigit(literal[j])){

			if(j<3) return LW_FAILURE;
			if(literal[j]!='.' && literal[j] !=',') return LW_FAILURE;

		}

	}

	return LW_SUCCESS;

}

static double
parse_geo_literal(char* literal){

	char dgr[3];
	char* min;
	char* sec;
	char* dec;
	char* csl;

	int literal_length;

	char hemisphere_sign = literal[0];
	int start_literal = 0;
	double result = 0.0;

	POSTGIS_DEBUGF(2,"parse_geo_literal called (%s)",literal);

	literal_length = strlen(literal);

	if(isdigit(hemisphere_sign)) start_literal=1;



	/**
	 * degrees/minutes/seconds: hdddmmss (hemisphere-degrees-minutes-seconds)
	 */

	strncpy(dgr,&literal[1-start_literal],3);

	if(strchr(literal,'.')==NULL && strchr(literal,',')==NULL) {

		//level 3
		POSTGIS_DEBUGF(2,"  lat/lon literal detected > %s",literal);
		POSTGIS_DEBUGF(2,"  degrees: %s",dgr);

		//level 5
		POSTGIS_DEBUGF(5,"  min = malloc(%d)",literal_length-5);
		min = malloc(literal_length-5);

		if(literal_length>(4-start_literal)){

			strncpy(min,&literal[(start_literal+4)],2);
			min[2]='\0';
		}

		//level 3
		POSTGIS_DEBUGF(2,"  lat/lon minutes: %s",min);

		POSTGIS_DEBUGF(5,"  sec = malloc(%d)",literal_length-5);
		sec = malloc(literal_length-5);

		if(literal_length > (7-start_literal)){

			strncpy(sec,&literal[6-start_literal],literal_length-(6-start_literal));
			sec[literal_length-(6-start_literal)]='\0';
		}

		//level 3
		POSTGIS_DEBUGF(2,"  lat/lon seconds: %s",sec);
		result = atof(dgr);
		if(min) result = result + atof(min)/60;
		if(sec) result = result + atof(sec)/3600;
		if(hemisphere_sign=='S' || hemisphere_sign=='W' || hemisphere_sign=='-') result = result*-1;

		free(min);
		POSTGIS_DEBUG(5,"  free(min)");
		free(sec);
		POSTGIS_DEBUG(5,"  free(sec)");

	} else {


		//level 3
		POSTGIS_DEBUG(2,"  decimal literal detected");

		if(strchr(literal,',')){

			/**
			 * Changes the literal decimal sign from comma to period to avoid problems with atof.
			 * In MARC21/XML coordinates, the decimal sign may be either a period or a comma.
			 **/
			csl = malloc(literal_length);

			strncpy(csl,&literal[0],literal_length-strlen(strchr(literal,',')));
			csl[literal_length-strlen(strchr(literal,','))]='\0';

			strcat(csl,".");
			strcat(csl,strchr(literal,',')+1);

			strncpy(literal,&csl[0],literal_length);
			free(csl);

			//level 3
			POSTGIS_DEBUGF(2,"  new literal value (replaced comma): %s",literal);

		}

		POSTGIS_DEBUGF(2,"  degrees: %s",dgr);

		if (literal[start_literal+4]=='.'){

			/**
			 * decimal degrees: hddd.dddddd (hemisphere-degrees.decimal degrees)
			 * Ex. E079.533265
			 *     +079.533265
			 *     ||  |_ indicates decimal degrees
			 *     ||_ degrees
			 *     |_ start_literal (hemisphere)
			 */
			POSTGIS_DEBUGF(5,"  dec = malloc(%d)",literal_length-start_literal);
			dec = malloc(literal_length-start_literal);

			strncpy(dec,&literal[1-start_literal],literal_length-start_literal);

			result = atof(dec);
			POSTGIS_DEBUGF(2,"  decimal degrees: %s",dec);
			POSTGIS_DEBUG(5,"  free(dec)");
			free(dec);

		} else if (literal[start_literal+6]=='.'){

			/**
			 * decimal minutes: hdddmm.mmmm (hemisphere-degrees-minutes.decimal minutes)
			 * Ex. E07932.5332
			 *     ||  | |_ indicates decimal minutes
			 *     ||  |_ minutes
			 *     ||_ degrees
			 *     |_ start_literal (hemisphere)
			 */
			POSTGIS_DEBUGF(5,"  min = malloc(%d)",literal_length-(4-start_literal));
			min = malloc(literal_length-(4-start_literal));
			strncpy(min,&literal[4-start_literal],literal_length-(4-start_literal));

			result = atof(dgr)+(atof(min)/100);
			//level 3
			POSTGIS_DEBUGF(2,"  decimal minutes: %s",min);
			POSTGIS_DEBUG(5,"  free(min)");
			free(min);

		} else if (literal[start_literal+8]=='.'){

			/**
			 * decimal seconds: hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
			 * Ex. E0793235.575
			 *     ||  | | |_indicates decimal seconds
			 *     ||  | |_seconds
			 *     ||  |_minutes
			 *     ||_degrees
			 *     |_start_literal (hemisphere)
			 */

			POSTGIS_DEBUGF(5,"  min = malloc(%d)",literal_length-(4-start_literal));
			min = malloc(literal_length);
			strncpy(min,&literal[4-start_literal],2);
			//min[sizeof(min)] = '\0';

			POSTGIS_DEBUGF(5,"  sec = malloc(%d)",literal_length-(6-start_literal));
			sec = malloc(literal_length);
			strncpy(sec,&literal[6-start_literal],literal_length-(6-start_literal));
			//sec[literal_length-(6-start_literal)] = '\0';

			result =  atof(dgr)+(atof(min)/100)+(atof(sec)/10000);
			//level 3
			POSTGIS_DEBUGF(2,"  minutes: %s",min);
			POSTGIS_DEBUGF(2,"  decimal seconds: %s",sec);

			POSTGIS_DEBUG(5,"  free(min)");
			free(min);
			POSTGIS_DEBUG(5,"  free(dec)");
			free(sec);


		}

	}

	//level 2
	POSTGIS_DEBUGF(2,"=> parse_geo_literal returned: %f (in decimal degrees)",result);
	return result;
}


static LWGEOM*
parse_marc21(xmlNodePtr xnode) {

	int ngeoms;
	int i;
	xmlNodePtr datafield;
	xmlNodePtr subfield;
	LWGEOM *result;
	LWGEOM **lwgeoms = (LWGEOM **) lwalloc(sizeof(LWGEOM *));
	uint8_t geometry_type;
	uint8_t result_type;
	char* code;
	char* literal;


	result_type = 0;
	ngeoms = 0;

	for (datafield = xnode->children ; datafield != NULL ; datafield = datafield->next) {

		char* lw = NULL;
		char* le = NULL;
		char* ln = NULL;
		char* ls = NULL;

		if (datafield->type != XML_ELEMENT_NODE) continue;

		if (strcmp((char *) datafield->name, "datafield")!=0 || strcmp((char *) xmlGetProp(datafield,(xmlChar *)"tag"), "034")!=0)  continue;



		for (subfield = datafield->children ; subfield != NULL ; subfield = subfield->next){

			if (subfield->type != XML_ELEMENT_NODE) continue;
			if (strcmp((char *)subfield->name, "subfield")!=0) continue;

			code = (char *) xmlGetProp(subfield,(xmlChar *)"code");

			if ((strcmp(code, "d")!=0 && strcmp(code, "e")!=0 && strcmp(code, "f")!=0 && strcmp(code, "g"))!=0) continue;

			literal = (char *) xmlNodeGetContent(subfield);

			if(is_literal_valid(literal)==LW_SUCCESS) {

				if(strcmp(code,"d")==0)	lw = literal;
				else if(strcmp(code,"e")==0) le = literal;
				else if(strcmp(code,"f")==0) ln = literal;
				else if(strcmp(code,"g")==0) ls = literal;

			} else {

				xmlFreeNode(subfield);
				lwpgerror("parse error - invalid literal at 034$%s: %s",code,literal);
				return NULL;
			}

		}

		xmlFreeNode(subfield);

		if(lw && le && ln && ls){

			double w = parse_geo_literal(lw);
			double n = parse_geo_literal(ln);
			double e = parse_geo_literal(le);
			double s = parse_geo_literal(ls);
			geometry_type = 0;

			if(ngeoms>0) lwgeoms = (LWGEOM**) lwrealloc(lwgeoms,sizeof(LWGEOM*) * (ngeoms+1));

			if(fabs(w-e)<0.0000001f && fabs(n-s)<0.0000001f ){

				lwgeoms[ngeoms] = (LWGEOM *) lwpoint_make2d(SRID_UNKNOWN, w, s);
				geometry_type = MULTIPOINTTYPE;

			} else {

				lwgeoms[ngeoms] = (LWGEOM *) lwpoly_construct_envelope(SRID_UNKNOWN, w,n,e,s);
				geometry_type = MULTIPOLYGONTYPE;

			}

			if(ngeoms && result_type != geometry_type){
				result_type = COLLECTIONTYPE;
			} else {
				result_type = geometry_type;
			}

			ngeoms++;


		} else {

			if(lw || le || ln || ls) {

				lwpgerror("parse error - the Coded Cartographic Mathematical Data (034) on the given MARC21/XML is invalid:\n - 034$d: %s\n - 034$e: %s \n - 034$f: %s \n - 034$g: %s",lw,le,ln,ls);
			}

		}


	}

	xmlFreeNode(datafield);

	if (ngeoms == 1){

		return lwgeoms[0];

	} else if (ngeoms > 1){

		result = (LWGEOM *)lwcollection_construct_empty(result_type, SRID_UNKNOWN, 0, 0);

		for (i=0; i < ngeoms; i++) {

			result = (LWGEOM*)lwcollection_add_lwgeom((LWCOLLECTION*)result, lwgeoms[i]);

		}

		return result;

	}

	return NULL;

}




