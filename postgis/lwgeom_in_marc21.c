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

/**
 * Ability to parse geographic data contained in MARC21/XML documents
 * to return an LWGEOM or an error message. It returns NULL if the
 * document does not contain any geographic data (datafield:034)
 *
 * MARC21/XML version supported: 1.1
 * Cf: <https://www.loc.gov/standards/marcxml/>
 *
 * Copyright 2021 University of Münster, Germany (WWU)
 * Written by Jim Jones <jim.jones@uni-muenster.de>
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/builtins.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <string.h>

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

//#include <regex.h>


Datum geom_from_marc21(PG_FUNCTION_ARGS);
static LWGEOM* parse_marc21_xpath(xmlNodePtr xnode);
static LWGEOM* parse_marc21(xmlNodePtr xnode);

#define MARC21_NS		((char *) "http://www.loc.gov/MARC21/slim")


/**
 * Ability to parse geographic data contained in MARC21/XML documents
 * to return an LWGEOM or an error message. It returns NULL if the
 * document does not contain any geographic data (datafield:034)
 */
PG_FUNCTION_INFO_V1(geom_from_marc21);
Datum geom_from_marc21(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom, *hlwgeom;
	xmlDocPtr xmldoc;
	text *xml_input;
	int xml_size;
	char *xml;
	//bool hasz=true;
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

static xmlNodeSetPtr* parse_xml_node(xmlNodePtr xnode, char* xpath_expr){

	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	xmlChar *xpath = (xmlChar*) xpath_expr;
	context = xmlXPathNewContext(xnode);

	xmlXPathRegisterNs(context,  BAD_CAST "mx", BAD_CAST "http://www.loc.gov/MARC21/slim");

	if (context == NULL) {
		lwdebug(1,"Error in xmlXPathNewContext\n");
		return NULL;
	}

	result = xmlXPathEvalExpression(xpath, context);

	xmlXPathFreeContext(context);

	if(result) {

		return result->nodesetval;

	} else {

		if (result == NULL) {
			lwdebug(1,"Error in xmlXPathEvalExpression\n");
		}

		if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
			lwdebug(1,"Datafield empty. \n");
		}

		xmlXPathFreeObject (result);
		return NULL;

	}
}


int is_literal_valid(char* literal){

	if(strlen(literal)<3) return LW_FAILURE;

	int coord_start = 0;

	if(literal[0]=='N' || literal[0]=='E' ||
	   literal[0]=='S' || literal[0]=='W' ||
	   literal[0]=='+' || literal[0]=='-' ) {

		if(strlen(literal)<4) return LW_FAILURE;

		coord_start = 1;
	}

	int j;

	for (j=coord_start; j < strlen(literal); j++) {

		if(!isdigit(literal[j])){

			if(j<3) return LW_FAILURE;
			if(literal[j]!='.' && literal[j] !=',') return LW_FAILURE;

		}

	}

	return LW_SUCCESS;

}

//static int is_literal_valid_regex(char* literal){
//
//
//
//	regex_t regex;
//	int reti;
//
//
//	//TEST VALIDATE STRING BY PARSING INSTEAD OF REGEX! TOO SLOW.
//
//	/**
//	 * regex validation reads:
//	 *
//	 *   - starts with either a digit (0-9), a plus sign (+), a minus sign (-),
//	 *     or one of the following characters (hemispheres): ESWN
//	 *   - contains exact three digits after the starting character (degree)
//	 *   - after the degree it may contain further digits or/and a period/comma(./,)
//	 */
//	reti = regcomp(&regex, "^[0-9ESWN+-]{1}[0-9]{3}([\.\,]{,1}[0-9])?*$", REG_EXTENDED);
//
//	if (reti) {
//		lwpgerror(1, "Could not compile regex\n");
//		return 1;
//	}
//
//	reti = regexec(&regex, literal, 0, NULL, 0);
//
//	regfree(&regex);
//	return reti;
//}

char *substr(char *string, int position, int length)
{
	char *p;
	int c;

	p = palloc(length+1);

	if (p == NULL)	{
		lwpgerror("Unable to allocate memory");
	}

	for (c = 0; c < length; c++) {
		*(p+c) = *(string+position-1);
		string++;
	}

	*(p+c) = '\0';

	return p;

}

static double parse_geo_literal(char* literal){

	double coords;
	char hemisphere_sign = literal[0];
	int start_literal = 0;

	if(isdigit(hemisphere_sign)) start_literal=1;

	/**
	 * degrees/minutes/seconds: hdddmmss (hemisphere-degrees-minutes-seconds)
	 */
	double degrees = atof(substr(literal,(2-start_literal),3));

	if(strchr(literal,'.')==NULL && strchr(literal,',')==NULL) {


		double minutes = 0;

		if(strlen(literal)>(4-start_literal)){
			minutes = atof(substr(literal,(5-start_literal),2));
		}

		double seconds = 0;

		if(strlen(literal)>(7-start_literal)){
			seconds = atof(substr(literal,(7-start_literal),strlen(literal)-(6-start_literal)));
		}

		coords = degrees + minutes/60 + seconds/3600;

		if(hemisphere_sign=='S' || hemisphere_sign=='W' || hemisphere_sign=='-' ) {
			coords = coords *-1;
		}


	} else {

		/**
		 * Changes the literal decimal sign from comma to period to avoid problems with atof.
		 * In MARC21/XML coordinates, the decimal sign may be either a period or a comma.
		 **/
		if(strchr(literal,',')){

			char* frmtliteral = substr(literal, 1, strlen(literal)-strlen(strchr(literal,',')));
			strcat(frmtliteral,".");
			strcat(frmtliteral,strchr(literal,',')+1);
			literal = frmtliteral;

		}

		/**
		 * decimal degrees: hddd.dddddd (hemisphere-degrees.decimal degrees)
		 */
		if (strcmp(substr(literal,5-start_literal,1),".") ==0 ){

			coords = atof(substr(literal,(2-start_literal),strlen(literal)-start_literal));

			/**
			 * decimal minutes: hdddmm.mmmm (hemisphere-degrees-minutes.decimal minutes)
			 */
		} else if (strcmp(substr(literal,7-start_literal,1),".") ==0 ){

			double decimal = atof(substr(literal,(5-start_literal),strlen(literal)-(5-start_literal)))/100;
			coords = degrees+decimal;

			/**
			 * decimal seconds: hdddmmss.sss (hemisphere-degrees-minutes-seconds.decimal seconds)
			 */
		} else if (strcmp(substr(literal,9-start_literal,1),".") ==0 ){

			double decimal = atof(substr(literal,(5-start_literal),strlen(literal)-(5-start_literal)))/100;
			coords = degrees+(decimal/100);

		}

	}

	return coords;
}


static LWGEOM* parse_marc21(xmlNodePtr xnode) {


	int ngeoms = 0;
	uint8_t result_type;
	xmlNodePtr datafield;
	LWGEOM *result;
	LWGEOM **lwgeoms = (LWGEOM **) lwalloc(sizeof(LWGEOM *));

	for (datafield = xnode->children ; datafield != NULL ; datafield = datafield->next) {

		if (datafield->type != XML_ELEMENT_NODE) continue;

		if (strcmp((char *) datafield->name, "datafield")!=0 || strcmp((char *) xmlGetProp(datafield,"tag"), "034")!=0)  continue;

		xmlNodePtr subfield;
		char* lw = NULL;
		char* le = NULL;
		char* ln = NULL;
		char* ls = NULL;

		for (subfield = datafield->children ; subfield != NULL ; subfield = subfield->next){

			if (subfield->type != XML_ELEMENT_NODE) continue;
			if (strcmp((char *)subfield->name, "subfield")!=0) continue;

			char* code = xmlGetProp(subfield,"code");

			if ((strcmp(code, "d")!=0 && strcmp(code, "e")!=0 && strcmp(code, "f")!=0 && strcmp(code, "g"))!=0) continue;

			char* literal = xmlNodeGetContent(subfield);

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

			uint8_t geometry_type;

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

	if(!ngeoms) {

		return NULL;

	} else if (ngeoms == 1){

		return lwgeoms[0];

	} else if (ngeoms > 1){

		result = (LWGEOM *)lwcollection_construct_empty(result_type, SRID_UNKNOWN, 0, 0);

		int i;

		for (i=0; i < ngeoms; i++) {

			result = (LWGEOM*)lwcollection_add_lwgeom((LWCOLLECTION*)result, lwgeoms[i]);

		}

		return result;

	}

}




