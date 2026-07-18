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
 * Copyright 2009 - 2010 Oslandia
 *
 **********************************************************************/


/**
* @file GML input routines.
* Ability to parse GML geometry fragment and to return an LWGEOM
* or an error message.
*
* Implement ISO SQL/MM ST_GMLToSQL method
* Cf: ISO 13249-3:2009 -> 5.1.50 (p 134)
*
* GML versions supported:
*  - Triangle, Tin and TriangulatedSurface,
*    and PolyhedralSurface elements
*  - GML 3.2.1 Namespace
*  - GML 3.1.1 Simple Features profile SF-2
*    (with backward compatibility to GML 3.1.0 and 3.0.0)
*  - GML 2.1.2
* Cf: <http://www.opengeospatial.org/standards/gml>
*
* NOTA: this code doesn't (yet ?) support SQL/MM curves
*
* Written by Olivier Courtin - Oslandia
*
**********************************************************************/

#include "postgres.h"
#include "executor/spi.h"
#include "utils/builtins.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_transform.h"


Datum geom_from_gml(PG_FUNCTION_ARGS);
static LWGEOM *lwgeom_from_gml(const char *wkt, int xml_size);
static LWGEOM* parse_gml(xmlNodePtr xnode, bool *hasz, int *root_srid);

typedef struct struct_gmlSrs
{
	int32_t srid;
	bool reverse_axis;
}
gmlSrs;

#define XLINK_NS	((char *) "http://www.w3.org/1999/xlink")
#define GML_NS		((char *) "http://www.opengis.net/gml")
#define GML32_NS	((char *) "http://www.opengis.net/gml/3.2")



static void gml_lwpgerror(char *msg, __attribute__((__unused__)) int error_code)
{
        POSTGIS_DEBUGF(3, "ST_GeomFromGML ERROR %i", error_code);
        lwpgerror("%s", msg);
}

/**
 * Ability to parse GML geometry fragment and to return an LWGEOM
 * or an error message.
 *
 * ISO SQL/MM define two error messages:
*  Cf: ISO 13249-3:2009 -> 5.1.50 (p 134)
 *  - invalid GML representation
 *  - unknown spatial reference system
 */
PG_FUNCTION_INFO_V1(geom_from_gml);
Datum geom_from_gml(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	text *xml_input;
	LWGEOM *lwgeom;
	char *xml;
	int root_srid=SRID_UNKNOWN;
	int xml_size;

	/* Get the GML stream */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	xml_input = PG_GETARG_TEXT_P(0);
	xml = text_to_cstring(xml_input);
	xml_size = VARSIZE_ANY_EXHDR(xml_input);

	/* Zero for undefined */
	root_srid = PG_GETARG_INT32(1);

	lwgeom = lwgeom_from_gml(xml, xml_size);
	if ( root_srid != SRID_UNKNOWN )
		lwgeom->srid = root_srid;

	geom = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(geom);
}


static inline bool
is_gml_element(xmlNodePtr xn, const char *gml_name)
{
	const char *colon_pos;
	const char *node_name;

	/* Not an element node, can't do anything */
	if (!xn || xn->type != XML_ELEMENT_NODE)
		return false;

	/* If there's a colon in the element name, */
	/* move past it before checking for equality with */
	/* the element name we are looking for */
	node_name = (const char*)xn->name;
	colon_pos = strchr(node_name, ':');
	if (colon_pos)
		node_name = colon_pos + 1;

	return strcmp(node_name, gml_name) == 0;
}


/**
 * Return false if current element namespace is not a GML one
 * Return true otherwise.
 */
static bool is_gml_namespace(xmlNodePtr xnode, bool is_strict)
{
	xmlNsPtr *ns, *p;

	ns = xmlGetNsList(xnode->doc, xnode);
	/*
	 * If no namespace is available we could return true anyway
	 * (because we work only on GML fragment, we don't want to
	 *  'oblige' to add namespace on the geometry root node)
	 */
	if (ns == NULL) { return !is_strict; }

	/*
	 * Handle namespaces:
	 *  - http://www.opengis.net/gml      (GML 3.1.1 and priors)
	 *  - http://www.opengis.net/gml/3.2  (GML 3.2.1)
	 */
	for (p=ns ; *p ; p++)
	{
                if ((*p)->href == NULL || (*p)->prefix == NULL ||
                     xnode->ns == NULL || xnode->ns->prefix == NULL) continue;

		if (!xmlStrcmp(xnode->ns->prefix, (*p)->prefix))
                {
			if (    !strcmp((char *) (*p)->href, GML_NS)
                             || !strcmp((char *) (*p)->href, GML32_NS))
			{
				xmlFree(ns);
                                return true;
			} else {
                                xmlFree(ns);
                                return false;
                        }
		}
	}

	xmlFree(ns);
	return !is_strict; /* Same reason here to not return false */
}


/**
 * Retrieve a GML property from a node or NULL otherwise
 * Respect namespaces if presents in the node element
 */
static xmlChar *gmlGetProp(xmlNodePtr xnode, const char *charProp)
{
	xmlChar *value;
	xmlChar *prop = (xmlChar*)charProp;

	if (!is_gml_namespace(xnode, true))
		return xmlGetProp(xnode, prop);
	/*
	 * Handle namespaces:
	 *  - http://www.opengis.net/gml      (GML 3.1.1 and priors)
	 *  - http://www.opengis.net/gml/3.2  (GML 3.2.1)
	 */
	value = xmlGetNsProp(xnode, prop, (xmlChar *) GML_NS);
	if (value == NULL) value = xmlGetNsProp(xnode, prop, (xmlChar *) GML32_NS);

	/* In last case try without explicit namespace */
	if (value == NULL) value = xmlGetNoNsProp(xnode, prop);

	return value;
}


/**
 * Return true if current node contains a simple XLink
 * Return false otherwise.
 */
static bool is_xlink(xmlNodePtr node)
{
	xmlChar *prop;

	prop = xmlGetNsProp(node, (xmlChar *)"type", (xmlChar *) XLINK_NS);
	if (prop == NULL) return false;
	if (strcmp((char *) prop, "simple"))
	{
		xmlFree(prop);
		return false;
	}

	prop = xmlGetNsProp(node, (xmlChar *)"href", (xmlChar *) XLINK_NS);
	if (prop == NULL) return false;
	if (prop[0] != '#')
	{
		xmlFree(prop);
		return false;
	}
	xmlFree(prop);

	return true;
}


/**
 * Return a xmlNodePtr on a node referenced by a XLink or NULL otherwise
 */
static xmlNodePtr get_xlink_node(xmlNodePtr xnode)
{
	char *id;
	xmlNsPtr *ns, *n;
	xmlXPathContext *ctx;
	xmlXPathObject *xpath;
	xmlNodePtr node, ret_node;
	xmlChar *href, *p, *node_id;

	href = xmlGetNsProp(xnode, (xmlChar *)"href", (xmlChar *) XLINK_NS);
	p = href;
	p++; /* ignore '#' first char */

	if (xnode->ns)
	{
		id = lwalloc((xmlStrlen(xnode->ns->prefix) * 2 + xmlStrlen(xnode->name) +
		              xmlStrlen(href) + sizeof("//:[@:id='']") + 1));
		/* XPath pattern look like: //gml:point[@gml:id='p1'] */
		sprintf(id, "//%s:%s[@%s:id='%s']",
		        (char *) xnode->ns->prefix,
		        (char *) xnode->name,
		        (char *) xnode->ns->prefix,
		        (char *) p);
	}
	else
	{
		id = lwalloc((xmlStrlen(xnode->name) +
		              xmlStrlen(href) + sizeof("//:[@:id='']") + 1));
		/* XPath pattern look like: //gml:point[@gml:id='p1'] */
		sprintf(id, "//%s[@id='%s']",
		        (char *) xnode->name,
		        (char *) p);
	}

	ctx = xmlXPathNewContext(xnode->doc);
	if (ctx == NULL)
	{
		xmlFree(href);
		lwfree(id);
		return NULL;
	}

	/* Handle namespaces */
	ns = xmlGetNsList(xnode->doc, xnode);
	for (n=ns ; *n; n++) xmlXPathRegisterNs(ctx, (*n)->prefix, (*n)->href);
	xmlFree(ns);

	/* Execute XPath expression */
	xpath = xmlXPathEvalExpression((xmlChar *) id, ctx);
	lwfree(id);
	if (xpath == NULL || xpath->nodesetval == NULL || xpath->nodesetval->nodeNr != 1)
	{
		xmlFree(href);
		xmlXPathFreeObject(xpath);
		xmlXPathFreeContext(ctx);
		return NULL;
	}
	ret_node = xpath->nodesetval->nodeTab[0];
	xmlXPathFreeObject(xpath);
	xmlXPathFreeContext(ctx);

	/* Protection against circular calls */
	for (node = xnode ; node != NULL ; node = node->parent)
	{
		if (node->type != XML_ELEMENT_NODE) continue;
		node_id = gmlGetProp(node, "id");
		if (node_id != NULL)
		{
			if (!xmlStrcmp(node_id, p))
				gml_lwpgerror("invalid GML representation", 2);
			xmlFree(node_id);
		}
	}

	xmlFree(href);
	return ret_node;
}



/**
 * Use Proj to reproject a given POINTARRAY
 */

static POINTARRAY *
gml_reproject_pa(POINTARRAY *pa, int32_t epsg_in, int32_t epsg_out)
{
	LWPROJ *lwp;
	char text_in[16];
	char text_out[16];

	if (epsg_in == SRID_UNKNOWN)
		return pa; /* nothing to do */

	if (epsg_out == SRID_UNKNOWN)
	{
		gml_lwpgerror("invalid GML representation", 3);
		return NULL;
	}

	snprintf(text_in, 16, "EPSG:%d", epsg_in);
	snprintf(text_out, 16, "EPSG:%d", epsg_out);


	lwp = lwproj_from_str(text_in, text_out);
	if (!lwp)
	{
		gml_lwpgerror("Could not create LWPROJ*", 57);
		return NULL;
	}

	if (ptarray_transform(pa, lwp) == LW_FAILURE)
	{
		elog(ERROR, "gml_reproject_pa: reprojection failed");
		return NULL;
	}
	proj_destroy(lwp->pj);
	pfree(lwp);

	return pa;
}

static LWGEOM *
gml_reproject_lwgeom(LWGEOM *geom, int32_t epsg_in, int32_t epsg_out)
{
	LWPROJ *lwp;
	char text_in[16];
	char text_out[16];

	if (epsg_in == SRID_UNKNOWN)
		return geom;

	if (epsg_out == SRID_UNKNOWN)
	{
		gml_lwpgerror("invalid GML representation", 3);
		return NULL;
	}

	snprintf(text_in, 16, "EPSG:%d", epsg_in);
	snprintf(text_out, 16, "EPSG:%d", epsg_out);

	lwp = lwproj_from_str(text_in, text_out);
	if (!lwp)
	{
		gml_lwpgerror("Could not create LWPROJ*", 57);
		return NULL;
	}

	if (lwgeom_transform(geom, lwp) == LW_FAILURE)
	{
		elog(ERROR, "gml_reproject_lwgeom: reprojection failed");
		return NULL;
	}
	proj_destroy(lwp->pj);
	pfree(lwp);

	return geom;
}

/**
 * Return 1 if the SRS definition from the authority has a GIS friendly order,
 * that is easting,northing. This is typically true for most projected CRS
 * (but not all!), and this is false for EPSG geographic CRS.
 */
static int
gml_is_srs_axis_order_gis_friendly(int32_t srid)
{
	char *srtext;
	char query[256];
	int is_axis_order_gis_friendly, err;

	if (SPI_OK_CONNECT != SPI_connect ())
		lwpgerror("gml_is_srs_axis_order_gis_friendly: could not connect to SPI manager");

	sprintf(query, "SELECT srtext \
                        FROM spatial_ref_sys WHERE srid='%d'", srid);

	err = SPI_exec(query, 1);
	if (err < 0) lwpgerror("gml_is_srs_axis_order_gis_friendly: error executing query %d", err);

	/* No entry in spatial_ref_sys */
	if (SPI_processed <= 0)
	{
		SPI_finish();
		return -1;
	}

	srtext = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);

	is_axis_order_gis_friendly = 1;
	if (srtext && srtext[0] != '\0')
	{
		char* ptr;
		char* srtext_horizontal = (char*) malloc(strlen(srtext) + 1);

		if (!srtext_horizontal)
		{
			SPI_finish();
			lwpgerror("gml_is_srs_axis_order_gis_friendly: could not allocate memory");
		}
		strcpy(srtext_horizontal, srtext);

		/* Remove the VERT_CS part if we are in a COMPD_CS */
		ptr = strstr(srtext_horizontal, ",VERT_CS[");
		if (ptr)
			*ptr = '\0';

		if( strstr(srtext_horizontal, "AXIS[") == NULL &&
		    strstr(srtext_horizontal, "GEOCCS[") == NULL )
		{
			/* If there is no axis definition, then due to how GDAL < 3
			* generated the WKT, this means that the axis order is not
			* GIS friendly */
			is_axis_order_gis_friendly = 0;
		}
		else if( strstr(srtext_horizontal,
		           "AXIS[\"Latitude\",NORTH],AXIS[\"Longitude\",EAST]") != NULL )
		{
			is_axis_order_gis_friendly = 0;
		}
		else if( strstr(srtext_horizontal,
		           "AXIS[\"Northing\",NORTH],AXIS[\"Easting\",EAST]") != NULL )
		{
			is_axis_order_gis_friendly = 0;
		}
		else if( strstr(srtext_horizontal,
		           "AXIS[\"geodetic latitude (Lat)\",north,ORDER[1]") != NULL )
		{
			is_axis_order_gis_friendly = 0;
		}

		free(srtext_horizontal);
	}
	SPI_finish();

	return is_axis_order_gis_friendly;
}


/**
 * Parse gml srsName attribute
 */
static void parse_gml_srs(xmlNodePtr xnode, gmlSrs *srs)
{
	char *p;
	int is_axis_order_gis_friendly;
	xmlNodePtr node;
	xmlChar *srsname;
	bool honours_authority_axis_order = false;
	char sep = ':';

	node = xnode;
	srsname = gmlGetProp(node, "srsName");
	/*printf("srsname %s\n",srsname);*/
	if (!srsname)
	{
		if (node->parent == NULL)
		{
			srs->srid = SRID_UNKNOWN;
			srs->reverse_axis = false;
			return;
		}
		parse_gml_srs(node->parent, srs);
	}
	else
	{
		/* Severals	srsName formats are available...
		 *  cf WFS 1.1.0 -> 9.2 (p36)
		 *  cf ISO 19142:2009 -> 7.9.2.4.4 (p34)
		 *  cf RFC 5165 <http://tools.ietf.org/html/rfc5165>
		 *  cf CITE WFS-1.1 (GetFeature-tc17.2)
		 */

		/* SRS pattern like:   	EPSG:4326
		  			urn:EPSG:geographicCRS:4326
		  		  	urn:ogc:def:crs:EPSG:4326
		 			urn:ogc:def:crs:EPSG::4326
		  			urn:ogc:def:crs:EPSG:6.6:4326
		   			urn:x-ogc:def:crs:EPSG:6.6:4326
					http://www.opengis.net/gml/srs/epsg.xml#4326
					http://www.epsg.org/6.11.2/4326
		*/

		if (!strncmp((char *) srsname, "EPSG:", 5))
		{
			sep = ':';
			honours_authority_axis_order = false;
		}
		else if (!strncmp((char *) srsname, "urn:ogc:def:crs:EPSG:", 21)
		         || !strncmp((char *) srsname, "urn:x-ogc:def:crs:EPSG:", 23)
		         || !strncmp((char *) srsname, "urn:EPSG:geographicCRS:", 23))
		{
			sep = ':';
			honours_authority_axis_order = true;
		}
		else if (!strncmp((char *) srsname,
		                  "http://www.opengis.net/gml/srs/epsg.xml#", 40))
		{
			sep = '#';
			honours_authority_axis_order = false;
		}
		else gml_lwpgerror("unknown spatial reference system", 4);

		/* retrieve the last ':' or '#' char */
		for (p = (char *) srsname ; *p ; p++);
		for (--p ; *p != sep ; p--)
			if (!isdigit(*p)) gml_lwpgerror("unknown spatial reference system", 5);

		srs->srid = atoi(++p);

		/* Check into spatial_ref_sys that this SRID really exist */
		is_axis_order_gis_friendly = gml_is_srs_axis_order_gis_friendly(srs->srid);
		if (srs->srid == SRID_UNKNOWN || is_axis_order_gis_friendly == -1)
			gml_lwpgerror("unknown spatial reference system", 6);

		/* Reverse axis order if the srsName is meant to honour the axis
		   order defined by the authority and if that axis order is not
		   the GIS friendly one. */
		srs->reverse_axis = !is_axis_order_gis_friendly && honours_authority_axis_order;

		xmlFree(srsname);
		return;
	}
}


/**
 * Parse a string supposed to be a double
 */
static double parse_gml_double(char *d, bool space_before, bool space_after)
{
	char *p;
	int st;
	enum states
	{
		INIT     	= 0,
		NEED_DIG  	= 1,
		DIG	  	= 2,
		NEED_DIG_DEC 	= 3,
		DIG_DEC 	= 4,
		EXP	 	= 5,
		NEED_DIG_EXP 	= 6,
		DIG_EXP 	= 7,
		END 		= 8
	};

	/*
	 * Double pattern
	 * [-|\+]?[0-9]+(\.)?([0-9]+)?([Ee](\+|-)?[0-9]+)?
	 * We could also meet spaces before and/or after
	 * this pattern upon parameters
	 */

	if (space_before) while (isspace(*d)) d++;
	for (st = INIT, p = d ; *p ; p++)
	{

		if (isdigit(*p))
		{
			if (st == INIT || st == NEED_DIG) 		st = DIG;
			else if (st == NEED_DIG_DEC) 			st = DIG_DEC;
			else if (st == NEED_DIG_EXP || st == EXP) 	st = DIG_EXP;
			else if (st == DIG || st == DIG_DEC || st == DIG_EXP);
			else gml_lwpgerror("invalid GML representation", 7);
		}
		else if (*p == '.')
		{
			if      (st == DIG) 				st = NEED_DIG_DEC;
			else    gml_lwpgerror("invalid GML representation", 8);
		}
		else if (*p == '-' || *p == '+')
		{
			if      (st == INIT) 				st = NEED_DIG;
			else if (st == EXP) 				st = NEED_DIG_EXP;
			else    gml_lwpgerror("invalid GML representation", 9);
		}
		else if (*p == 'e' || *p == 'E')
		{
			if      (st == DIG || st == DIG_DEC) 		st = EXP;
			else    gml_lwpgerror("invalid GML representation", 10);
		}
		else if (isspace(*p))
		{
			if (!space_after) gml_lwpgerror("invalid GML representation", 11);
			if (st == DIG || st == DIG_DEC || st == DIG_EXP)st = END;
			else if (st == NEED_DIG_DEC)			st = END;
			else if (st == END);
			else    gml_lwpgerror("invalid GML representation", 12);
		}
		else  gml_lwpgerror("invalid GML representation", 13);
	}

	if (st != DIG && st != NEED_DIG_DEC && st != DIG_DEC && st != DIG_EXP && st != END)
		gml_lwpgerror("invalid GML representation", 14);

	return atof(d);
}


/**
 * Parse gml:coordinates
 */
static POINTARRAY* parse_gml_coordinates(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *gml_coord, *gml_ts, *gml_cs, *gml_dec;
	char cs, ts, dec;
	POINTARRAY *dpa;
	int gml_dims;
	char *p, *q;
	bool digit;
	POINT4D pt = {0};

	/* We begin to retrieve coordinates string */
	gml_coord = xmlNodeGetContent(xnode);
	p = (char *) gml_coord;

	/* Default GML coordinates pattern: 	x1,y1 x2,y2
	 * 					x1,y1,z1 x2,y2,z2
	 *
	 * Cf GML 2.1.2 -> 4.3.1 (p18)
	 */

	/* Retrieve separator between coordinates tuples */
	gml_ts = gmlGetProp(xnode, "ts");
	if (gml_ts == NULL) ts = ' ';
	else
	{
		if (xmlStrlen(gml_ts) > 1 || isdigit(gml_ts[0]))
			gml_lwpgerror("invalid GML representation", 15);
		ts = gml_ts[0];
		xmlFree(gml_ts);
	}

	/* Retrieve separator between each coordinate */
	gml_cs = gmlGetProp(xnode, "cs");
	if (gml_cs == NULL) cs = ',';
	else
	{
		if (xmlStrlen(gml_cs) > 1 || isdigit(gml_cs[0]))
			gml_lwpgerror("invalid GML representation", 16);
		cs = gml_cs[0];
		xmlFree(gml_cs);
	}

	/* Retrieve decimal separator */
	gml_dec = gmlGetProp(xnode, "decimal");
	if (gml_dec == NULL) dec = '.';
	else
	{
		if (xmlStrlen(gml_dec) > 1 || isdigit(gml_dec[0]))
			gml_lwpgerror("invalid GML representation", 17);
		dec = gml_dec[0];
		xmlFree(gml_dec);
	}

	if (cs == ts || cs == dec || ts == dec)
		gml_lwpgerror("invalid GML representation", 18);

	/* HasZ, !HasM, 1 Point */
	dpa = ptarray_construct_empty(1, 0, 1);

	while (isspace(*p)) p++;		/* Eat extra whitespaces if any */
	for (q = p, gml_dims=0, digit = false ; *p ; p++)
	{

		if (isdigit(*p)) digit = true;	/* One state parser */

		/* Coordinate Separator */
		if (*p == cs)
		{
			*p = '\0';
			gml_dims++;

			if (*(p+1) == '\0') gml_lwpgerror("invalid GML representation", 19);

			if 	(gml_dims == 1) pt.x = parse_gml_double(q, false, true);
			else if (gml_dims == 2) pt.y = parse_gml_double(q, false, true);

			q = p+1;

			/* Tuple Separator (or end string) */
		}
		else if (digit && (*p == ts || *(p+1) == '\0'))
		{
			if (*p == ts) *p = '\0';
			gml_dims++;

			if (gml_dims < 2 || gml_dims > 3)
				gml_lwpgerror("invalid GML representation", 20);

			if (gml_dims == 3)
				pt.z = parse_gml_double(q, false, true);
			else
			{
				pt.y = parse_gml_double(q, false, true);
				*hasz = false;
			}

			ptarray_append_point(dpa, &pt, LW_TRUE);
			digit = false;

			q = p+1;
			gml_dims = 0;

			/* Need to put standard decimal separator to atof handle */
		}
		else if (*p == dec && dec != '.') *p = '.';
	}

	xmlFree(gml_coord);

	return dpa; /* ptarray_clone_deep(dpa); */
}


/**
 * Parse gml:coord
 */
static POINTARRAY* parse_gml_coord(xmlNodePtr xnode, bool *hasz)
{
	xmlNodePtr xyz;
	POINTARRAY *dpa;
	bool x,y,z;
	xmlChar *c;
	POINT4D p = {0};

	/* HasZ?, !HasM, 1 Point */
	dpa = ptarray_construct_empty(1, 0, 1);

	x = y = z = false;
	for (xyz = xnode->children ; xyz != NULL ; xyz = xyz->next)
	{
		if (xyz->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xyz, false)) continue;

		if (is_gml_element(xyz, "X"))
		{
			if (x) gml_lwpgerror("invalid GML representation", 21);
			c = xmlNodeGetContent(xyz);
			p.x = parse_gml_double((char *) c, true, true);
			x = true;
			xmlFree(c);
		}
		else  if (is_gml_element(xyz, "Y"))
		{
			if (y) gml_lwpgerror("invalid GML representation", 22);
			c = xmlNodeGetContent(xyz);
			p.y = parse_gml_double((char *) c, true, true);
			y = true;
			xmlFree(c);
		}
		else if (is_gml_element(xyz, "Z"))
		{
			if (z) gml_lwpgerror("invalid GML representation", 23);
			c = xmlNodeGetContent(xyz);
			p.z = parse_gml_double((char *) c, true, true);
			z = true;
			xmlFree(c);
		}
	}
	/* Check dimension consistency */
	if (!x || !y) gml_lwpgerror("invalid GML representation", 24);
	if (!z) *hasz = false;

	ptarray_append_point(dpa, &p, LW_FALSE);

	return dpa; /* ptarray_clone_deep(dpa); */
}


/**
 * Parse gml:pos
 */
static POINTARRAY* parse_gml_pos(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *dimension, *gmlpos;
	int dim, gml_dim;
	POINTARRAY *dpa;
	char *pos, *p;
	bool digit;
	POINT4D pt = {0, 0, 0, 0};

	/* HasZ, !HasM, 1 Point */
	dpa = ptarray_construct_empty(1, 0, 1);

    dimension = gmlGetProp(xnode, "srsDimension");
    if (dimension == NULL) /* in GML 3.0.0 it was dimension */
        dimension = gmlGetProp(xnode, "dimension");
    if (dimension == NULL) dim = 2;	/* We assume that we are in 2D */
    else
    {
        dim = atoi((char *) dimension);
        xmlFree(dimension);
        if (dim < 2 || dim > 3)
            gml_lwpgerror("invalid GML representation", 25);
    }
    if (dim == 2) *hasz = false;

    /* We retrieve gml:pos string */
    gmlpos = xmlNodeGetContent(xnode);
    pos = (char *) gmlpos;
    while (isspace(*pos)) pos++;	/* Eat extra whitespaces if any */

    /* gml:pos pattern: 	x1 y1
        * 			x1 y1 z1
        */
    for (p=pos, gml_dim=0, digit=false ; *pos ; pos++)
    {
        if (isdigit(*pos)) digit = true;
        if (digit && (*pos == ' ' || *(pos+1) == '\0'))
        {
            if (*pos == ' ') *pos = '\0';
            gml_dim++;
            if 	(gml_dim == 1)
                pt.x = parse_gml_double(p, true, true);
            else if (gml_dim == 2)
                pt.y = parse_gml_double(p, true, true);
            else if (gml_dim == 3)
                pt.z = parse_gml_double(p, true, true);

            p = pos+1;
            digit = false;
        }
    }
    xmlFree(gmlpos);

    /* Test again coherent dimensions on each coord */
    if (gml_dim == 2) *hasz = false;
    if (gml_dim < 2 || gml_dim > 3 || gml_dim != dim)
        gml_lwpgerror("invalid GML representation", 26);

    ptarray_append_point(dpa, &pt, LW_FALSE);

	return dpa; /* ptarray_clone_deep(dpa); */
}


/**
 * Parse gml:posList
 */
static POINTARRAY* parse_gml_poslist(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *dimension, *gmlposlist;
	char *poslist, *p;
	int dim, gml_dim;
	POINTARRAY *dpa;
	POINT4D pt = {0, 0, 0, 0};
	bool digit;

	/* Retrieve gml:srsDimension attribute if any */
	dimension = gmlGetProp(xnode, "srsDimension");
	if (dimension == NULL) /* in GML 3.0.0 it was dimension */
		dimension = gmlGetProp(xnode, "dimension");
	if (dimension == NULL) dim = 2;	/* We assume that we are in common 2D */
	else
	{
		dim = atoi((char *) dimension);
		xmlFree(dimension);
		if (dim < 2 || dim > 3) gml_lwpgerror("invalid GML representation", 27);
	}
	if (dim == 2) *hasz = false;

	/* Retrieve gml:posList string */
	gmlposlist = xmlNodeGetContent(xnode);
	poslist = (char *) gmlposlist;

	/* HasZ?, !HasM, 1 point */
	dpa = ptarray_construct_empty(1, 0, 1);

	/* gml:posList pattern: 	x1 y1 x2 y2
	 * 				x1 y1 z1 x2 y2 z2
	 */
	while (isspace(*poslist)) poslist++;	/* Eat extra whitespaces if any */
	for (p=poslist, gml_dim=0, digit=false ; *poslist ; poslist++)
	{
		if (isdigit(*poslist)) digit = true;
		if (digit && (*poslist == ' ' || *(poslist+1) == '\0'))
		{
			if (*poslist == ' ') *poslist = '\0';

			gml_dim++;
			if 	(gml_dim == 1) pt.x = parse_gml_double(p, true, true);
			else if (gml_dim == 2) pt.y = parse_gml_double(p, true, true);
			else if (gml_dim == 3) pt.z = parse_gml_double(p, true, true);

			if (gml_dim == dim)
			{
				/* Add to ptarray, allowing dupes */
				ptarray_append_point(dpa, &pt, LW_TRUE);
				pt.x = pt.y = pt.z = pt.m = 0.0;
				gml_dim = 0;
			}
			else if (*(poslist+1) == '\0')
				gml_lwpgerror("invalid GML representation", 28);

			p = poslist+1;
			digit = false;
		}
	}

	xmlFree(gmlposlist);

	return dpa; /* ptarray_clone_deep(dpa); */
}


/**
 * Parse data coordinates
 *
 * There's several ways to encode data coordinates, who could be mixed
 * inside a single geometry:
 *  - gml:pos element
 *  - gml:posList element
 *  - gml:pointProperty
 *  - gml:pointRep 					(deprecated in 3.1.0)
 *  - gml:coordinate element with tuples string inside 	(deprecated in 3.1.0)
 *  - gml:coord elements with X,Y(,Z) nested elements 	(deprecated in 3.0.0)
 */
static POINTARRAY* parse_gml_data(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	POINTARRAY *pa = 0, *tmp_pa = 0;
	xmlNodePtr xa, xb;
	gmlSrs srs;
	bool found;

	pa = NULL;

	for (xa = xnode ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (xa->name == NULL) continue;

		if (is_gml_element(xa, "pos"))
		{
			tmp_pa = parse_gml_pos(xa, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		}
		else if (is_gml_element(xa, "posList"))
		{
			tmp_pa = parse_gml_poslist(xa, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		}
		else if (is_gml_element(xa, "coordinates"))
		{
			tmp_pa = parse_gml_coordinates(xa, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		}
		else if (is_gml_element(xa, "coord"))
		{
			tmp_pa = parse_gml_coord(xa, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		}
		else if (is_gml_element(xa, "pointRep") ||
		         is_gml_element(xa, "pointProperty"))
		{
			found = false;
			for (xb = xa->children ; xb != NULL ; xb = xb->next)
			{
				if (xb->type != XML_ELEMENT_NODE) continue;
				if (!is_gml_namespace(xb, false)) continue;
				if (is_gml_element(xb, "Point"))
				{
					found = true;
					break;
				}
			}
			if (!found || xb == NULL)
				gml_lwpgerror("invalid GML representation", 29);

			if (is_xlink(xb)) xb = get_xlink_node(xb);
			if (xb == NULL || xb->children == NULL)
				gml_lwpgerror("invalid GML representation", 30);

			tmp_pa = parse_gml_data(xb->children, hasz, root_srid);
			if (tmp_pa->npoints != 1)
				gml_lwpgerror("invalid GML representation", 31);

			parse_gml_srs(xb, &srs);
			if (srs.reverse_axis) tmp_pa = ptarray_flip_coordinates(tmp_pa);
			if (*root_srid == SRID_UNKNOWN) *root_srid = srs.srid;
			else if (srs.srid != *root_srid)
				gml_reproject_pa(tmp_pa, srs.srid, *root_srid);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);
		}
	}

	if (pa == NULL) gml_lwpgerror("invalid GML representation", 32);

	return pa;
}


/**
 * Parse GML point (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_point(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	LWGEOM *geom;
	POINTARRAY *pa;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	if (xnode->children == NULL)
		return lwpoint_as_lwgeom(lwpoint_construct_empty(*root_srid, 0, 0));

	pa = parse_gml_data(xnode->children, hasz, root_srid);
	if (pa->npoints != 1) gml_lwpgerror("invalid GML representation", 34);

	parse_gml_srs(xnode, &srs);
	if (srs.reverse_axis) pa = ptarray_flip_coordinates(pa);
	if (!*root_srid)
	{
		*root_srid = srs.srid;
		geom = (LWGEOM *) lwpoint_construct(*root_srid, NULL, pa);
	}
	else
	{
		if (srs.srid != *root_srid)
			gml_reproject_pa(pa, srs.srid, *root_srid);
		geom = (LWGEOM *) lwpoint_construct(SRID_UNKNOWN, NULL, pa);
	}

	return geom;
}


/**
 * Parse GML lineString (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_line(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	LWGEOM *geom;
	POINTARRAY *pa;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	if (xnode->children == NULL)
		return lwline_as_lwgeom(lwline_construct_empty(*root_srid, 0, 0));

	pa = parse_gml_data(xnode->children, hasz, root_srid);
	if (pa->npoints < 2) gml_lwpgerror("invalid GML representation", 36);

	parse_gml_srs(xnode, &srs);
	if (srs.reverse_axis) pa = ptarray_flip_coordinates(pa);
	if (!*root_srid)
	{
		*root_srid = srs.srid;
		geom = (LWGEOM *) lwline_construct(*root_srid, NULL, pa);
	}
	else
	{
		if (srs.srid != *root_srid)
			gml_reproject_pa(pa, srs.srid, *root_srid);
		geom = (LWGEOM *) lwline_construct(SRID_UNKNOWN, NULL, pa);
	}

	return geom;
}


/**
 * Parse GML Curve (3.1.1)
 */
static LWGEOM *
parse_gml_curve_segment(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	POINTARRAY *pa;
	xmlChar *interpolation = NULL;

	if (is_gml_element(xnode, "LineStringSegment"))
	{
		interpolation = gmlGetProp(xnode, "interpolation");
		if (interpolation != NULL)
		{
			if (strcmp((char *)interpolation, "linear"))
				gml_lwpgerror("invalid GML representation", 38);
			xmlFree(interpolation);
		}

		pa = parse_gml_data(xnode->children, hasz, root_srid);
		if (pa->npoints < 2)
			gml_lwpgerror("invalid GML representation", 39);
		return lwline_as_lwgeom(lwline_construct(SRID_UNKNOWN, NULL, pa));
	}

	if (is_gml_element(xnode, "ArcString"))
	{
		interpolation = gmlGetProp(xnode, "interpolation");
		if (interpolation != NULL)
		{
			if (strcmp((char *)interpolation, "circularArc3Points"))
				gml_lwpgerror("invalid GML representation", 38);
			xmlFree(interpolation);
		}

		pa = parse_gml_data(xnode->children, hasz, root_srid);
		if (pa->npoints < 3 || pa->npoints % 2 == 0)
			gml_lwpgerror("invalid GML representation", 39);
		return lwcircstring_as_lwgeom(lwcircstring_construct(SRID_UNKNOWN, NULL, pa));
	}

	return NULL;
}

static int
gml_curve_endpoint(const LWGEOM *geom, POINT4D *pt)
{
	if (!geom || lwgeom_is_empty(geom))
		return LW_FAILURE;

	if (geom->type == COMPOUNDTYPE)
	{
		const LWCOMPOUND *compound = (const LWCOMPOUND *)geom;
		return gml_curve_endpoint(compound->geoms[compound->ngeoms - 1], pt);
	}

	if (geom->type == LINETYPE || geom->type == CIRCSTRINGTYPE)
	{
		const LWLINE *line = (const LWLINE *)geom;
		if (!line->points || line->points->npoints < 1)
			return LW_FAILURE;
		return getPoint4d_p(line->points, line->points->npoints - 1, pt) ? LW_SUCCESS : LW_FAILURE;
	}

	return LW_FAILURE;
}

static int
gml_curve_points_equal(const POINT4D *a, const POINT4D *b, bool hasz)
{
	if (a->x != b->x || a->y != b->y)
		return LW_FALSE;

	if (hasz && a->z != b->z)
		return LW_FALSE;

	return LW_TRUE;
}

static void
gml_compound_add_curve_component(LWCOMPOUND *compound, LWGEOM *segment)
{
	if (segment->type == COMPOUNDTYPE)
	{
		LWCOMPOUND *nested = (LWCOMPOUND *)segment;

		for (uint32_t i = 0; i < nested->ngeoms; i++)
			gml_compound_add_curve_component(compound, nested->geoms[i]);

		nested->ngeoms = 0;
		lwgeom_free(segment);
		return;
	}

	if (compound->ngeoms > 0)
	{
		POINT4D first, last;
		bool hasz = lwgeom_has_z(lwcompound_as_lwgeom(compound));

		if (lwgeom_startpoint(segment, &first) == LW_FAILURE ||
		    gml_curve_endpoint(compound->geoms[compound->ngeoms - 1], &last) == LW_FAILURE ||
		    !gml_curve_points_equal(&first, &last, hasz))
			gml_lwpgerror("invalid GML representation", 41);
	}

	if (lwcompound_add_lwgeom(compound, segment) == LW_FAILURE)
		gml_lwpgerror("invalid GML representation", 41);
}

static LWGEOM *
gml_curve_segments_to_lwgeom(LWGEOM **segments, size_t nsegments, bool *hasz, int *root_srid)
{
	bool has_curve = false;

	if (nsegments == 0)
		gml_lwpgerror("invalid GML representation", 40);

	for (size_t i = 0; i < nsegments; i++)
	{
		if (segments[i]->type != LINETYPE)
		{
			has_curve = true;
			break;
		}
	}

	if (nsegments == 1)
		return segments[0];

	if (!has_curve)
	{
		LWLINE *line = (LWLINE *)segments[0];
		POINTARRAY *pa = ptarray_clone_deep(line->points);
		size_t point_size = *hasz ? sizeof(POINT3D) : sizeof(POINT2D);

		for (size_t i = 1; i < nsegments; i++)
		{
			LWLINE *next = (LWLINE *)segments[i];
			if (memcmp(
				getPoint_internal(pa, pa->npoints - 1), getPoint_internal(next->points, 0), point_size))
				gml_lwpgerror("invalid GML representation", 41);
			if (ptarray_append_ptarray(pa, next->points, 0.0) == LW_FAILURE)
				gml_lwpgerror("invalid GML representation", 41);
			lwgeom_free(segments[i]);
		}
		lwgeom_free(segments[0]);

		return lwline_as_lwgeom(lwline_construct(*root_srid, NULL, pa));
	}
	else
	{
		LWCOMPOUND *compound = lwcompound_construct_empty(*root_srid, *hasz, 0);
		for (size_t i = 0; i < nsegments; i++)
			gml_compound_add_curve_component(compound, segments[i]);

		return lwcompound_as_lwgeom(compound);
	}
}

static LWGEOM* parse_gml_curve(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	xmlNodePtr xa;
	size_t lss;
	bool found=false;
	gmlSrs srs;
	LWGEOM *geom=NULL;
	LWGEOM **segments = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	/* Looking for gml:segments */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "segments"))
		{
			found = true;
			break;
		}
	}
	if (!found) gml_lwpgerror("invalid GML representation", 37);

	segments = (LWGEOM **)lwalloc(sizeof(LWGEOM *));

	/* Processing each gml:LineStringSegment or gml:ArcString */
	for (xa = xa->children, lss=0; xa != NULL ; xa = xa->next)
	{
		LWGEOM *segment;

		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;

		segment = parse_gml_curve_segment(xa, hasz, root_srid);
		if (segment == NULL)
			continue;

		if (lss > 0)
			segments = lwrealloc(segments, sizeof(LWGEOM *) * (lss + 1));
		segments[lss] = segment;
		lss++;
	}
	if (lss == 0) gml_lwpgerror("invalid GML representation", 40);

	geom = gml_curve_segments_to_lwgeom(segments, lss, hasz, root_srid);
	lwfree(segments);

	parse_gml_srs(xnode, &srs);
	if (srs.reverse_axis)
		lwgeom_swap_ordinates(geom, LWORD_X, LWORD_Y);
	if (srs.srid != *root_srid && *root_srid != SRID_UNKNOWN)
		gml_reproject_lwgeom(geom, srs.srid, *root_srid);
	geom->srid = *root_srid;

	return geom;
}


/**
 * Parse GML LinearRing (3.1.1)
 */
static LWGEOM* parse_gml_linearring(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	LWGEOM *geom;
	POINTARRAY **ppa = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);
	parse_gml_srs(xnode, &srs);

	ppa = (POINTARRAY**) lwalloc(sizeof(POINTARRAY*));
	ppa[0] = parse_gml_data(xnode->children, hasz, root_srid);

	if (ppa[0]->npoints < 4
            || (!*hasz && !ptarray_is_closed_2d(ppa[0]))
            ||  (*hasz && !ptarray_is_closed_3d(ppa[0])))
	    gml_lwpgerror("invalid GML representation", 42);

	if (srs.reverse_axis)
		ppa[0] = ptarray_flip_coordinates(ppa[0]);

	if (srs.srid != *root_srid && *root_srid != SRID_UNKNOWN)
		gml_reproject_pa(ppa[0], srs.srid, *root_srid);

	geom = (LWGEOM *) lwpoly_construct(*root_srid, NULL, 1, ppa);

	return geom;
}


/**
 * Parse GML Polygon (2.1.2, 3.1.1)
 */
static void
gml_validate_ring_geom(LWGEOM *ring, bool hasz, int error_code)
{
	POINT4D first, last;
	uint32_t vertices_needed = ring->type == LINETYPE ? 4 : 3;

	if (lwgeom_count_vertices(ring) < vertices_needed || lwgeom_startpoint(ring, &first) == LW_FAILURE ||
	    gml_curve_endpoint(ring, &last) == LW_FAILURE || !gml_curve_points_equal(&first, &last, hasz))
		gml_lwpgerror("invalid GML representation", error_code);
}

static LWGEOM *
parse_gml_linearring_as_lwgeom(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	POINTARRAY *pa = parse_gml_data(xnode->children, hasz, root_srid);
	LWGEOM *ring = lwline_as_lwgeom(lwline_construct(SRID_UNKNOWN, NULL, pa));

	parse_gml_srs(xnode, &srs);
	if (srs.reverse_axis)
		lwgeom_swap_ordinates(ring, LWORD_X, LWORD_Y);
	if (srs.srid != *root_srid && *root_srid != SRID_UNKNOWN)
		gml_reproject_lwgeom(ring, srs.srid, *root_srid);
	ring->srid = *root_srid;

	gml_validate_ring_geom(ring, *hasz, 43);

	return ring;
}

static LWGEOM *
parse_gml_ring(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	xmlNodePtr xa, xb;
	LWGEOM **segments = NULL;
	size_t nsegments = 0;
	LWGEOM *ring;

	if (is_xlink(xnode))
		xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	segments = (LWGEOM **)lwalloc(sizeof(LWGEOM *));

	for (xa = xnode->children; xa != NULL; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE)
			continue;
		if (!is_gml_namespace(xa, false))
			continue;

		if (is_gml_element(xa, "curveMembers"))
		{
			for (xb = xa->children; xb != NULL; xb = xb->next)
			{
				if (xb->type != XML_ELEMENT_NODE)
					continue;
				if (!is_gml_namespace(xb, false))
					continue;

				if (nsegments > 0)
					segments = lwrealloc(segments, sizeof(LWGEOM *) * (nsegments + 1));
				segments[nsegments++] = parse_gml(xb, hasz, root_srid);
			}
		}
		else if (is_gml_element(xa, "curveMember"))
		{
			if (xa->children != NULL)
			{
				if (nsegments > 0)
					segments = lwrealloc(segments, sizeof(LWGEOM *) * (nsegments + 1));
				segments[nsegments++] = parse_gml(xa->children, hasz, root_srid);
			}
		}
	}

	if (nsegments == 0)
		gml_lwpgerror("invalid GML representation", 43);

	ring = gml_curve_segments_to_lwgeom(segments, nsegments, hasz, root_srid);
	lwfree(segments);

	gml_validate_ring_geom(ring, *hasz, 43);

	return ring;
}

static LWGEOM* parse_gml_polygon(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	int ring;
	LWGEOM *geom;
	xmlNodePtr xa, xb;
	LWGEOM **rings = NULL;
	bool has_curve_ring = false;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	if (xnode->children == NULL)
		return lwpoly_as_lwgeom(lwpoly_construct_empty(*root_srid, 0, 0));

	parse_gml_srs(xnode, &srs);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* Polygon/outerBoundaryIs -> GML 2.1.2 */
		/* Polygon/exterior        -> GML 3.1.1 */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!(is_gml_element(xa, "outerBoundaryIs") ||
		      is_gml_element(xa, "exterior")))
			continue;

		for (xb = xa->children ; xb != NULL ; xb = xb->next)
		{
			if (xb->type != XML_ELEMENT_NODE) continue;
			if (!is_gml_namespace(xb, false)) continue;
			if (!(is_gml_element(xb, "LinearRing") || is_gml_element(xb, "Ring")))
				continue;

			rings = (LWGEOM **)lwalloc(sizeof(LWGEOM *));
			rings[0] = is_gml_element(xb, "Ring") ? parse_gml_ring(xb, hasz, root_srid)
							      : parse_gml_linearring_as_lwgeom(xb, hasz, root_srid);
			if (rings[0]->type != LINETYPE)
				has_curve_ring = true;
		}
	}

	/* Found an <exterior> or <outerBoundaryIs> but no rings?!? We're outa here! */
	if (!rings)
		gml_lwpgerror("invalid GML representation", 43);

	for (ring=1, xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* Polygon/innerBoundaryIs -> GML 2.1.2 */
		/* Polygon/interior        -> GML 3.1.1 */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!(is_gml_element(xa, "innerBoundaryIs") ||
		      is_gml_element(xa, "interior")))
			continue;

		for (xb = xa->children ; xb != NULL ; xb = xb->next)
		{
			if (xb->type != XML_ELEMENT_NODE) continue;
			if (!is_gml_namespace(xb, false)) continue;
			if (!(is_gml_element(xb, "LinearRing") || is_gml_element(xb, "Ring")))
				continue;

			rings = lwrealloc(rings, sizeof(LWGEOM *) * (ring + 1));
			rings[ring] = is_gml_element(xb, "Ring") ? parse_gml_ring(xb, hasz, root_srid)
								 : parse_gml_linearring_as_lwgeom(xb, hasz, root_srid);
			if (rings[ring]->type != LINETYPE)
				has_curve_ring = true;
			ring++;
		}
	}

	/* Exterior Ring is mandatory */
	if (rings == NULL || rings[0] == NULL)
		gml_lwpgerror("invalid GML representation", 44);

	if (has_curve_ring)
	{
		LWCURVEPOLY *curvepoly =
		    lwcurvepoly_construct_empty(*root_srid, FLAGS_GET_Z(rings[0]->flags), FLAGS_GET_M(rings[0]->flags));

		for (int i = 0; i < ring; i++)
		{
			if (lwcurvepoly_add_ring(curvepoly, rings[i]) == LW_FAILURE)
				gml_lwpgerror("invalid GML representation", 43);
		}
		lwfree(rings);
		geom = lwcurvepoly_as_lwgeom(curvepoly);
	}
	else
	{
		POINTARRAY **ppa = (POINTARRAY **)lwalloc(sizeof(POINTARRAY *) * ring);

		for (int i = 0; i < ring; i++)
		{
			LWLINE *line = (LWLINE *)rings[i];
			ppa[i] = ptarray_clone_deep(line->points);
			lwgeom_free(rings[i]);
		}
		lwfree(rings);
		geom = (LWGEOM *)lwpoly_construct(*root_srid, NULL, ring, ppa);
	}

	return geom;
}


/**
 * Parse GML Triangle (3.1.1)
 */
static LWGEOM* parse_gml_triangle(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	LWGEOM *geom;
	xmlNodePtr xa, xb;
	POINTARRAY *pa = NULL;
	xmlChar *interpolation=NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	if (xnode->children == NULL)
		return lwtriangle_as_lwgeom(lwtriangle_construct_empty(*root_srid, 0, 0));

	/* GML SF is restricted to planar interpolation
	       NOTA: I know Triangle is not part of SF, but
	       we have to be consistent with other surfaces */
	interpolation = gmlGetProp(xnode, "interpolation");
	if (interpolation != NULL)
	{
		if (strcmp((char *) interpolation, "planar"))
			gml_lwpgerror("invalid GML representation", 45);
		xmlFree(interpolation);
	}

	parse_gml_srs(xnode, &srs);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* Triangle/exterior */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "exterior")) continue;

		for (xb = xa->children ; xb != NULL ; xb = xb->next)
		{
			/* Triangle/exterior/LinearRing */
			if (xb->type != XML_ELEMENT_NODE) continue;
			if (!is_gml_namespace(xb, false)) continue;
			if (!is_gml_element(xb, "LinearRing")) continue;

			pa = (POINTARRAY*) lwalloc(sizeof(POINTARRAY));
			pa = parse_gml_data(xb->children, hasz, root_srid);

			if (pa->npoints != 4
			        || (!*hasz && !ptarray_is_closed_2d(pa))
			        ||  (*hasz && !ptarray_is_closed_3d(pa)))
				gml_lwpgerror("invalid GML representation", 46);

			if (srs.reverse_axis) pa = ptarray_flip_coordinates(pa);
		}
	}

	/* Exterior Ring is mandatory */
	if (pa == NULL) gml_lwpgerror("invalid GML representation", 47);

	if (srs.srid != *root_srid && *root_srid != SRID_UNKNOWN)
		gml_reproject_pa(pa, srs.srid, *root_srid);

	geom = (LWGEOM *) lwtriangle_construct(*root_srid, NULL, pa);

	return geom;
}


/**
 * Parse GML PolygonPatch (3.1.1)
 */
static LWGEOM* parse_gml_patch(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	xmlChar *interpolation = NULL;
	LWGEOM *geom;

	/* PolygonPatch */
	if (!is_gml_element(xnode, "PolygonPatch"))
		gml_lwpgerror("invalid GML representation", 48);

	/* GML SF is restricted to planar interpolation  */
	interpolation = gmlGetProp(xnode, "interpolation");
	if (interpolation != NULL)
	{
		if (strcmp((char *) interpolation, "planar"))
			gml_lwpgerror("invalid GML representation", 48);
		xmlFree(interpolation);
	}

	geom = parse_gml_polygon(xnode, hasz, root_srid);
	if (lwgeom_is_empty(geom))
		gml_lwpgerror("invalid GML representation", 50);

	return geom;
}


/**
 * Parse GML Surface (3.1.1)
 */
static LWGEOM* parse_gml_surface(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	xmlNodePtr xa;
	int patch;
	LWGEOM *geom=NULL;
	bool found=false;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	/* Looking for gml:patches */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "patches"))
		{
			found = true;
			break;
		}
	}
	if (!found) gml_lwpgerror("invalid GML representation", 51);

	/* Processing gml:PolygonPatch */
	for (patch=0, xa = xa->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "PolygonPatch")) continue;
		patch++;

		/* SQL/MM define ST_CurvePolygon as a single patch only,
		   cf ISO 13249-3:2009 -> 4.2.9 (p27) */
		if (patch > 1) gml_lwpgerror("invalid GML representation", 52);

		geom = parse_gml_patch(xa, hasz, root_srid);
	}

	if (!patch) gml_lwpgerror("invalid GML representation", 53);

	return geom;
}


/**
 * Parse GML Tin (and TriangulatedSurface) (3.1.1)
 *
 * TODO handle also Tin attributes:
 * - stopLines
 * - breakLines
 * - maxLength
 * - position
 */
static LWGEOM* parse_gml_tin(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa;
	LWGEOM *geom=NULL;
	bool found=false;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(TINTYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	/* Looking for gml:patches or gml:trianglePatches */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "patches") ||
		    is_gml_element(xa, "trianglePatches"))
		{
			found = true;
			break;
		}
	}
	if (!found) return geom; /* empty one */

	/* Processing each gml:Triangle */
	for (xa = xa->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "Triangle")) continue;

		if (xa->children != NULL)
			geom = (LWGEOM*) lwtin_add_lwtriangle((LWTIN *) geom,
			       (LWTRIANGLE *) parse_gml_triangle(xa, hasz, root_srid));
	}

	return geom;
}


/**
 * Parse gml:MultiPoint (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_mpoint(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa, xb;
	LWGEOM *geom = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(MULTIPOINTTYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* MultiPoint/pointMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "pointMembers"))
		{
			for (xb = xa->children ; xb != NULL ; xb = xb->next)
			{
				geom = (LWGEOM*)lwmpoint_add_lwpoint(
				    (LWMPOINT*)geom,
				    (LWPOINT*)parse_gml(xb, hasz, root_srid));
			}
		}
		else if (is_gml_element(xa, "pointMember"))
		{
			if (xa->children != NULL)
				geom = (LWGEOM*)lwmpoint_add_lwpoint((LWMPOINT*)geom,
			                                         (LWPOINT*)parse_gml(xa->children, hasz, root_srid));
		}
	}

	return geom;
}


/**
 * Parse gml:MultiLineString (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_mline(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(MULTILINETYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* MultiLineString/lineStringMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "lineStringMember")) continue;
		if (xa->children != NULL)
			geom = (LWGEOM*)lwmline_add_lwline((LWMLINE*)geom,
			                                   (LWLINE*)parse_gml(xa->children, hasz, root_srid));
	}

	return geom;
}


/**
 * Parse GML MultiCurve (3.1.1)
 */
static LWGEOM* parse_gml_mcurve(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa, xb;
	LWGEOM **geoms = NULL;
	uint32_t ngeoms = 0;
	bool has_curve = false;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	if (xnode->children == NULL)
		return (LWGEOM *)lwcollection_construct_empty(MULTILINETYPE, *root_srid, 1, 0);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{

		/* MultiCurve/curveMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "curveMembers"))
		{
			for (xb = xa->children ; xb != NULL ; xb = xb->next)
			{
				LWGEOM *member;
				if (xb->type != XML_ELEMENT_NODE)
					continue;
				if (!is_gml_namespace(xb, false))
					continue;

				member = parse_gml(xb, hasz, root_srid);
				geoms = ngeoms == 0 ? (LWGEOM **)lwalloc(sizeof(LWGEOM *))
						    : (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * (ngeoms + 1));
				geoms[ngeoms++] = member;
				if (member->type != LINETYPE)
					has_curve = true;
			}
		}
		else if (is_gml_element(xa, "curveMember"))
		{
			if (xa->children != NULL)
			{
				LWGEOM *member = parse_gml(xa->children, hasz, root_srid);
				geoms = ngeoms == 0 ? (LWGEOM **)lwalloc(sizeof(LWGEOM *))
						    : (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * (ngeoms + 1));
				geoms[ngeoms++] = member;
				if (member->type != LINETYPE)
					has_curve = true;
			}
		}
	}

	if (ngeoms == 0)
		return (LWGEOM *)lwcollection_construct_empty(MULTILINETYPE, *root_srid, 1, 0);
	else
	{
		LWCOLLECTION *col = lwcollection_construct_empty(has_curve ? MULTICURVETYPE : MULTILINETYPE,
								 *root_srid,
								 FLAGS_GET_Z(geoms[0]->flags),
								 FLAGS_GET_M(geoms[0]->flags));
		for (uint32_t i = 0; i < ngeoms; i++)
			lwcollection_add_lwgeom(col, geoms[i]);
		lwfree(geoms);
		return (LWGEOM *)col;
	}
}


/**
 * Parse GML MultiPolygon (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_mpoly(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(MULTIPOLYGONTYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* MultiPolygon/polygonMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "polygonMember")) continue;
		if (xa->children != NULL)
			geom = (LWGEOM*)lwmpoly_add_lwpoly((LWMPOLY*)geom,
			                                   (LWPOLY*)parse_gml(xa->children, hasz, root_srid));
	}

	return geom;
}


/**
 * Parse GML MultiSurface (3.1.1)
 */
static LWGEOM* parse_gml_msurface(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa, xb;
	LWGEOM **geoms = NULL;
	uint32_t ngeoms = 0;
	bool has_curve = false;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	if (xnode->children == NULL)
		return (LWGEOM *)lwcollection_construct_empty(MULTIPOLYGONTYPE, *root_srid, 1, 0);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		/* MultiSurface/surfaceMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "surfaceMembers"))
		{
			for (xb = xa->children ; xb != NULL ; xb = xb->next)
			{
				LWGEOM *member;
				if (xb->type != XML_ELEMENT_NODE)
					continue;
				if (!is_gml_namespace(xb, false))
					continue;

				member = parse_gml(xb, hasz, root_srid);
				geoms = ngeoms == 0 ? (LWGEOM **)lwalloc(sizeof(LWGEOM *))
						    : (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * (ngeoms + 1));
				geoms[ngeoms++] = member;
				if (member->type != POLYGONTYPE)
					has_curve = true;
			}
		}
		else if (is_gml_element(xa, "Polygon") || is_gml_element(xa, "CurvePolygon"))
		{
			LWGEOM *member = parse_gml(xa, hasz, root_srid);
			geoms = ngeoms == 0 ? (LWGEOM **)lwalloc(sizeof(LWGEOM *))
					    : (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * (ngeoms + 1));
			geoms[ngeoms++] = member;
			if (member->type != POLYGONTYPE)
				has_curve = true;
		}
		else if (is_gml_element(xa, "surfaceMember"))
		{
			if (xa->children != NULL)
			{
				LWGEOM *member = parse_gml(xa->children, hasz, root_srid);
				geoms = ngeoms == 0 ? (LWGEOM **)lwalloc(sizeof(LWGEOM *))
						    : (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * (ngeoms + 1));
				geoms[ngeoms++] = member;
				if (member->type != POLYGONTYPE)
					has_curve = true;
			}
		}
	}

	if (ngeoms == 0)
		return (LWGEOM *)lwcollection_construct_empty(MULTIPOLYGONTYPE, *root_srid, 1, 0);
	else
	{
		LWCOLLECTION *col = lwcollection_construct_empty(has_curve ? MULTISURFACETYPE : MULTIPOLYGONTYPE,
								 *root_srid,
								 FLAGS_GET_Z(geoms[0]->flags),
								 FLAGS_GET_M(geoms[0]->flags));
		for (uint32_t i = 0; i < ngeoms; i++)
			lwcollection_add_lwgeom(col, geoms[i]);
		lwfree(geoms);
		return (LWGEOM *)col;
	}
}


/**
 * Parse GML PolyhedralSurface (3.1.1)
 * Nota: It's not part of SF-2
 */
static LWGEOM* parse_gml_psurface(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa;
	bool found = false;
	LWGEOM *geom = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(POLYHEDRALSURFACETYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	/* Looking for gml:polygonPatches */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (is_gml_element(xa, "polygonPatches"))
		{
			found = true;
			break;
		}
	}
	if (!found) return geom;

	for (xa = xa->children ; xa != NULL ; xa = xa->next)
	{
		/* PolyhedralSurface/polygonPatches/PolygonPatch */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;
		if (!is_gml_element(xa, "PolygonPatch")) continue;

		geom = (LWGEOM*)lwpsurface_add_lwpoly((LWPSURFACE*)geom,
		                                      (LWPOLY*)parse_gml_patch(xa, hasz, root_srid));
	}

	return geom;
}


/**
 * Parse GML MultiGeometry (2.1.2, 3.1.1)
 */
static LWGEOM* parse_gml_coll(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	gmlSrs srs;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	if (is_xlink(xnode)) xnode = get_xlink_node(xnode);
	if (xnode == NULL)
		gml_lwpgerror("invalid GML representation", 30);

	parse_gml_srs(xnode, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
		*root_srid = srs.srid;

	geom = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, *root_srid, 1, 0);

	if (xnode->children == NULL)
		return geom;

	for (xa = xnode->children ; xa != NULL ; xa = xa->next)
	{
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!is_gml_namespace(xa, false)) continue;

		/*
		 * In GML 2.1.2 pointMember, lineStringMember and
		 * polygonMember are parts of geometryMember
		 * substitution group
		 */
		if (	   is_gml_element(xa, "pointMember")
		        || is_gml_element(xa, "lineStringMember")
		        || is_gml_element(xa, "polygonMember")
		        || is_gml_element(xa, "geometryMember"))
		{
			if (xa->children == NULL) break;
			geom = (LWGEOM*)lwcollection_add_lwgeom((LWCOLLECTION *)geom,
			                                        parse_gml(xa->children, hasz, root_srid));
		}
	}

	return geom;
}

/**
 * Read GML
 */
static LWGEOM *
lwgeom_from_gml(const char *xml, int xml_size)
{
	xmlDocPtr xmldoc;
	xmlNodePtr xmlroot=NULL;
	LWGEOM *lwgeom = NULL;
	bool hasz=true;
	int root_srid=SRID_UNKNOWN;

	/* Begin to Parse XML doc */
	xmlInitParser();

	xmldoc = xmlReadMemory(xml, xml_size, NULL, NULL, 0);
	if (!xmldoc)
	{
		xmlCleanupParser();
		gml_lwpgerror("invalid GML representation", 1);
		return NULL;
	}

	xmlroot = xmlDocGetRootElement(xmldoc);
	if (!xmlroot)
	{
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
		gml_lwpgerror("invalid GML representation", 1);
		return NULL;
	}

	lwgeom = parse_gml(xmlroot, &hasz, &root_srid);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();
	/* shouldn't we be releasing xmldoc too here ? */

	if ( root_srid != SRID_UNKNOWN )
		lwgeom->srid = root_srid;

	/* GML geometries could be either 2 or 3D and can be nested mixed.
	 * Missing Z dimension is even tolerated inside some GML coords
	 *
	 * So we deal with 3D in all structures allocation, and flag hasz
	 * to false if we met once a missing Z dimension
	 * In this case, we force recursive 2D.
	 */
	if (!hasz)
	{
		LWGEOM *tmp = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = tmp;
	}

	return lwgeom;
}


/**
 * Parse GML
 */
static LWGEOM* parse_gml(xmlNodePtr xnode, bool *hasz, int *root_srid)
{
	xmlNodePtr xa = xnode;
	gmlSrs srs;

	/* Scroll forward to the root node */
	while (xa != NULL &&
	      (xa->type != XML_ELEMENT_NODE || !is_gml_namespace(xa, false)))
	{
		xa = xa->next;
	}

	if (xa == NULL) gml_lwpgerror("invalid GML representation", 55);

	parse_gml_srs(xa, &srs);
	if (*root_srid == SRID_UNKNOWN && srs.srid != SRID_UNKNOWN)
	{
		*root_srid = srs.srid;
	}

	if (is_gml_element(xa, "Point"))
		return parse_gml_point(xa, hasz, root_srid);

	if (is_gml_element(xa, "LineString"))
		return parse_gml_line(xa, hasz, root_srid);

	if (is_gml_element(xa, "Curve"))
		return parse_gml_curve(xa, hasz, root_srid);

	if (is_gml_element(xa, "LinearRing"))
		return parse_gml_linearring(xa, hasz, root_srid);

	if (is_gml_element(xa, "Polygon") || is_gml_element(xa, "CurvePolygon"))
		return parse_gml_polygon(xa, hasz, root_srid);

	if (is_gml_element(xa, "Triangle"))
		return parse_gml_triangle(xa, hasz, root_srid);

	if (is_gml_element(xa, "Surface"))
		return parse_gml_surface(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiPoint"))
		return parse_gml_mpoint(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiLineString"))
		return parse_gml_mline(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiCurve"))
		return parse_gml_mcurve(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiPolygon"))
		return parse_gml_mpoly(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiSurface"))
		return parse_gml_msurface(xa, hasz, root_srid);

	if (is_gml_element(xa, "PolyhedralSurface"))
		return parse_gml_psurface(xa, hasz, root_srid);

	if (is_gml_element(xa, "Tin") ||
	    is_gml_element(xa, "TriangulatedSurface"))
		return parse_gml_tin(xa, hasz, root_srid);

	if (is_gml_element(xa, "MultiGeometry"))
		return parse_gml_coll(xa, hasz, root_srid);

	gml_lwpgerror("invalid GML representation", 56);
	return NULL; /* Never reach */
}
