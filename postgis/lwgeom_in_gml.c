/**********************************************************************
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Oslandia
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/**
* @file GML input routines.
* Ability to parse GML geometry fragment and to return an LWGEOM
* or an error message.
*
* Implement ISO SQL/MM ST_GMLToSQL method
* Cf: ISO 13249-3 -> 5.1.50 (p 134) 
*
* GML versions supported:
*  - GML 2.1.2
*  - GML 3.1.1 Simple Features profile (1.0.0)
* Cf: <http://www.opengeospatial.org/standards/gml>
*
* Current known limitations:
*  - Support only GML SF profile geometries (i.e no PostGIS curves support)
*  - Don't care about GML namespace
*  - Don't handle Xlink or PointRef construction
*  - Don't reproject if mixed srsName in multi/aggregate geoms
*
* Written by Olivier Courtin - Oslandia
*
**********************************************************************/


#include "postgres.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"

#if HAVE_LIBXML2
#include <libxml/tree.h> 
#include <libxml/parser.h> 

#include "lwgeom_export.h" 	/* getSRSbySRID */


Datum geom_from_gml(PG_FUNCTION_ARGS);
static LWGEOM* parse_gml(xmlNodePtr xnode, bool *hasz);


/**
 * Ability to parse GML geometry fragment and to return an LWGEOM
 * or an error message.
 *
 * ISO SQL/MM define two error messages: 
*  Cf: ISO 13249-3 -> 5.1.50 (p 134) 
 *  - invalid GML representation
 *  - unknown spatial reference system 
 */
PG_FUNCTION_INFO_V1(geom_from_gml);
Datum geom_from_gml(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom, *geom2d;
	xmlDocPtr xmldoc;
	text *xml_input; 
	LWGEOM *lwgeom;
        int xml_size;
	uchar *srl;
	char *xml;
        size_t size=0;
	bool hasz=true;
	xmlNodePtr xmlroot=NULL;


	/* Get the GML stream */
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	xml_input = PG_GETARG_TEXT_P(0);

	xml_size = VARSIZE(xml_input) - VARHDRSZ; 	/* actual letters */
        xml = palloc(xml_size + 1); 			/* +1 for null */
	memcpy(xml, VARDATA(xml_input), xml_size);
	xml[xml_size] = 0; 				/* null term */

	/* Begin to Parse XML doc */
        xmlInitParser();
        xmldoc = xmlParseMemory(xml, xml_size);
        if (!xmldoc || (xmlroot = xmlDocGetRootElement(xmldoc)) == NULL) {
	        xmlFreeDoc(xmldoc);
	        xmlCleanupParser();
		lwerror("invalid GML representation");
	}

	lwgeom = parse_gml(xmlroot, &hasz);
	geom = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	xmlFreeDoc(xmldoc);
	xmlCleanupParser();

	/* GML geometry could be either 2 or 3D and can be nested mixed.
	 * Missing Z dimension is even tolerated inside some GML coords
	 *
	 * So we deal with 3D in all structure allocation, and flag hasz
	 * to false if we met once a missing Z dimension
	 * In this case, we force recursive 2D.
	 */
	if (!hasz) {
		srl = lwalloc(VARSIZE(geom));
        	lwgeom_force2d_recursive(SERIALIZED_FORM(geom), srl, &size);
        	geom2d = PG_LWGEOM_construct(srl, pglwgeom_getSRID(geom),
                                     lwgeom_hasBBOX(geom->type));
		lwfree(geom);
		geom = geom2d;
	}

	PG_RETURN_POINTER(geom);
}
			

/**
 * Parse a string supposed to be a double
 */
static double parse_gml_double(char *d, bool space_before, bool space_after)
{
	char *p;
	int st;
	enum states {
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
	 * [-|\+]?[0-9]+(\.[0-9]+)?([Ee](\+|-)?[0-9]+)
	 *
	 * We could also meet spaces before or after
	 * this pattern upon parameters
	 */

	if (space_before) while (isspace(*d)) d++;
	for (st = INIT, p = d ; *p ; p++) {

		if (isdigit(*p)) {
				if (st == INIT || st == NEED_DIG) 	st = DIG;
			else if (st == NEED_DIG_DEC) 			st = DIG_DEC;
			else if (st == NEED_DIG_EXP || st == EXP) 	st = DIG_EXP;
			else if (st == DIG || st == DIG_DEC || st == DIG_EXP);
			else lwerror("invalid GML representation"); 
		} else if (*p == '.') {
			if      (st == DIG) 				st = NEED_DIG_DEC;
			else    lwerror("invalid GML representation"); 
		} else if (*p == '-' || *p == '+') {
			if      (st == INIT) 				st = NEED_DIG;
			else if (st == EXP) 				st = NEED_DIG_EXP;
			else    lwerror("invalid GML representation"); 
		} else if (*p == 'e' || *p == 'E') {
			if      (st == DIG || st == DIG_DEC) 		st = EXP;
			else    lwerror("invalid GML representation"); 
		} else if (isspace(*p)) {
			if (!space_after) lwerror("invalid GML representation");  
			if (st == DIG || st == DIG_DEC || st == DIG_EXP)st = END;
			else if (st == END);
			else    lwerror("invalid GML representation");
		} else  lwerror("invalid GML representation");
     	}	       

	if (st != DIG && st != DIG_DEC && st != DIG_EXP && st != END)
		lwerror("invalid GML representation");

	return atof(d);
}


/**
 * Parse gml:coordinates
 */
static POINTARRAY* parse_gml_coordinates(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *gml_coord, *gml_ts, *gml_cs, *gml_dec;
	char *coord, *p, *q;
	char cs, ts, dec;
	DYNPTARRAY *dpa;
	POINTARRAY *pa;
	int gml_dims;
	bool digit;
	POINT4D pt;
	uchar dims=0;

	/* We begin to retrieve coordinates string */
	gml_coord = xmlNodeGetContent(xnode);
	p = coord = (char *) gml_coord;

	/* Default GML coordinates pattern: 	x1,y1 x2,y2 
	 * 					x1,y1,z1 x2,y2,z2
	 * 				
	 * Cf GML 2.1.2 -> 4.3.1 (p18)
	 */

	/* Retrieve separator between coordinates tuples */
	gml_ts = xmlGetProp(xnode, (xmlChar *) "ts");
	if (gml_ts == NULL) ts = ' ';
	else {
		if (xmlStrlen(gml_ts) > 1 || isdigit(gml_ts[0]))
			lwerror("invalid GML representation");
		ts = gml_ts[0];
		xmlFree(gml_ts);
	}

	/* Retrieve separator between each coordinate */
	gml_cs = xmlGetProp(xnode, (xmlChar *) "cs");
	if (gml_cs == NULL) cs = ',';
	else {
		if (xmlStrlen(gml_cs) > 1 || isdigit(gml_cs[0]))
			lwerror("invalid GML representation");
		cs = gml_cs[0];
		xmlFree(gml_cs);
	}

	/* Retrieve decimal separator */
	gml_dec = xmlGetProp(xnode, (xmlChar *) "decimal");
	if (gml_dec == NULL) dec = '.';
	else {
		if (xmlStrlen(gml_dec) > 1 || isdigit(gml_dec[0]))
			lwerror("invalid GML representation");
		dec = gml_dec[0];
		xmlFree(gml_dec);
	}

	if (cs == ts || cs == dec || ts == dec)
		lwerror("invalid GML representation");

	/* Now we create PointArray from coordinates values */
	TYPE_SETZM(dims, 1, 0);
	dpa = dynptarray_create(1, dims);

	while(isspace(*p)) p++;		/* Eat extra whitespaces if any */
	for (q = p, gml_dims=0, digit = false ; *p ; p++) {

		if (isdigit(*p)) digit = true;	/* One state parser */

		/* Coordinate Separator */
		if (*p == cs) {
			*p = '\0';
			gml_dims++;

			if (*(p+1) == '\0') lwerror("invalid GML representation");

			if 	(gml_dims == 1) pt.x = parse_gml_double(q, 0, 1);
			else if (gml_dims == 2) pt.y = parse_gml_double(q, 0, 1);

			q = p+1;

		/* Tuple Separator (or end string) */
		} else if (digit && (*p == ts || *(p+1) == '\0')) {
			if (*p == ts) *p = '\0';
			gml_dims++;

			if (gml_dims < 2 || gml_dims > 3)
				lwerror("invalid GML representation");

			if (gml_dims == 3)
				pt.z = parse_gml_double(q, 0, 1);
			else {
				pt.y = parse_gml_double(q, 0, 1);
				*hasz = false;
			}

			dynptarray_addPoint4d(dpa, &pt, 0);
			digit = false;

			q = p+1;
			gml_dims = 0;

		/* Need to put standard decimal separator to atof handle */
		} else if (*p == dec && dec != '.') *p = '.';
	}

	xmlFree(gml_coord);
	pa = ptarray_clone(dpa->pa);
	lwfree(dpa);

	return pa;
}


/**
 * Parse gml:coord
 */
static POINTARRAY* parse_gml_coord(xmlNodePtr xnode, bool *hasz)
{
	xmlNodePtr xyz;
	DYNPTARRAY *dpa;
	POINTARRAY *pa;
	bool x,y,z;
	xmlChar *c;
	POINT4D p;
	uchar dims=0;

	TYPE_SETZM(dims, 1, 0);
	dpa = dynptarray_create(1, dims);

	x = y = z = false;
	for (xyz = xnode->children ; xyz != NULL ; xyz = xyz->next) {

		if (xyz->type != XML_ELEMENT_NODE) continue;

		if (!strcmp((char *) xyz->name, "X")) {
			if (x) lwerror("invalid GML representation");
			c = xmlNodeGetContent(xyz);
			p.x = parse_gml_double((char *) c, 1, 1);
			x = true;
			xmlFree(c);
		} else  if (!strcmp((char *) xyz->name, "Y")) {
			if (y) lwerror("invalid GML representation");
			c = xmlNodeGetContent(xyz);
			p.y = parse_gml_double((char *) c, 1, 1);
			y = true;
			xmlFree(c);
		} else if (!strcmp((char *) xyz->name, "Z")) {
			if (z) lwerror("invalid GML representation");
			c = xmlNodeGetContent(xyz);
			p.z = parse_gml_double((char *) c, 1, 1);
			z = true;
			xmlFree(c);
		}
	}
	/* Check dimension consistancy */
	if (!x || !y) lwerror("invalid GML representation");
	if (!z) *hasz = false;

	dynptarray_addPoint4d(dpa, &p, 0);
	x = y = z = false;

	pa = ptarray_clone(dpa->pa);
	lwfree(dpa);

	return pa;
}


/**
 * Parse gml:pos
 */
static POINTARRAY* parse_gml_pos(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *dimension, *gmlpos;
	xmlNodePtr posnode;
	int dim, gml_dim;
	DYNPTARRAY *dpa;
	POINTARRAY *pa;
	char *pos, *p;
	bool digit;
	POINT4D pt;
	uchar dims=0;

	TYPE_SETZM(dims, 1, 0);
	dpa = dynptarray_create(1, dims);

	for (posnode = xnode ; posnode != NULL ; posnode = posnode->next) {

		/* We only care about pos element */
		if (posnode->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) posnode->name, "pos")) continue;

		dimension = xmlGetProp(xnode, (xmlChar *) "dimension");
		if (dimension == NULL) dim = 2;	/* We assume that we are in 2D */
		else {
			dim = atoi((char *) dimension);
			xmlFree(dimension);
			if (dim < 2 || dim > 3)
				lwerror("invalid GML representation");
		}
		if (dim == 2) *hasz = false;
	
		/* We retrieve gml:pos string */
		gmlpos = xmlNodeGetContent(posnode);
		pos = (char *) gmlpos;
		while(isspace(*pos)) pos++;	/* Eat extra whitespaces if any */

		/* gml:pos pattern: 	x1 y1 
	 	* 			x1 y1 z1
	 	*/
		for (p=pos, gml_dim=0, digit=false ; *pos ; pos++) {

			if (isdigit(*pos)) digit = true;

			if (digit && (*pos == ' ' || *(pos+1) == '\0')) {

				if (*pos == ' ') *pos = '\0';
				gml_dim++;
				if 	(gml_dim == 1) pt.x = parse_gml_double(p, 0, 1);
				else if (gml_dim == 2) pt.y = parse_gml_double(p, 0, 1);
				else if (gml_dim == 3) pt.z = parse_gml_double(p, 0, 1);
			
				p = pos+1;
				digit = false;
			}
		}
		xmlFree(gmlpos);

		/* Test again coherent dimensions on each coord */
		if (gml_dim == 2) *hasz = false;
		if (gml_dim < 2 || gml_dim > 3 || gml_dim != dim)
			lwerror("invalid GML representation");

		dynptarray_addPoint4d(dpa, &pt, 0);
	}

	pa = ptarray_clone(dpa->pa);
	lwfree(dpa);

	return pa;
}


/**
 * Parse gml:posList
 */
static POINTARRAY* parse_gml_poslist(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *dimension, *gmlposlist;
	char *poslist, *p;
	int dim, gml_dim;
	DYNPTARRAY *dpa;
	POINTARRAY *pa;
	POINT4D pt;
	bool digit;
	uchar dims=0;

	/* Retrieve gml:dimension attribute if any */
	dimension = xmlGetProp(xnode, (xmlChar *) "dimension");
	if (dimension == NULL) dim = 2;	/* We assume that we are in common 2D */
	else {
		dim = atoi((char *) dimension);
		xmlFree(dimension);
		if (dim < 2 || dim > 3) lwerror("invalid GML representation");
	}
	if (dim == 2) *hasz = false;
	
	/* Retrieve gml:posList string */
	gmlposlist = xmlNodeGetContent(xnode);
	poslist = (char *) gmlposlist;

	TYPE_SETZM(dims, 1, 0);
	dpa = dynptarray_create(1, dims);

	/* gml:posList pattern: 	x1 y1 x2 y2
	 * 				x1 y1 z1 x2 y2 z2
	 */
	while(isspace(*poslist)) poslist++;	/* Eat extra whitespaces if any */
	for (p=poslist, gml_dim=0, digit=false ; *poslist ; poslist++) {

		if (isdigit(*poslist)) digit = true;

		if (digit && (*poslist == ' ' || *(poslist+1) == '\0')) {

			if (*poslist == ' ') *poslist = '\0';

			gml_dim++;
			if 	(gml_dim == 1) pt.x = parse_gml_double(p, 0, 1);
			else if (gml_dim == 2) pt.y = parse_gml_double(p, 0, 1);
			else if (gml_dim == 3) pt.z = parse_gml_double(p, 0, 1);

			if (gml_dim == dim) {
				dynptarray_addPoint4d(dpa, &pt, 0);
				gml_dim = 0;
			} else if (*(poslist+1) == '\0')
				lwerror("invalid GML representation");

			p = poslist+1;
			digit = false;
		}
	}

	xmlFree(gmlposlist);
	pa = ptarray_clone(dpa->pa);
	lwfree(dpa);

	return pa;
}


/**
 * Parse data coordinates
 *
 * There's four ways to encode coordinates, who could be mixed inside a 
 * single geometrie:
 *  - gml:pos element
 *  - gml:posList element 
 *  - gml:coord elements with X,Y(,Z) nested elements (deprecated in 3.0.0)
 *  - gml:coordinate element with tuples string inside (deprecated in 3.1.0)
 *
 * Cf: GML 2.1.2 -> 4.3.2 (p17)
 * Cf: GML 3.1.1 -> 9.1.3.5 (p45)
 *
 *  NOTA: we don't support pointRep references !
 */
static POINTARRAY* parse_gml_data(xmlNodePtr xnode, bool *hasz)
{
	xmlNodePtr node;
	POINTARRAY *pa, *tmp_pa;

	pa = NULL;

	for (node = xnode ; node != NULL ; node = node->next) {

		if (node->type != XML_ELEMENT_NODE) continue;
		if (!strcmp((char *) node->name, "pos")) {
	  		tmp_pa = parse_gml_pos(node, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		} else if (!strcmp((char *) node->name, "posList")) {
	  		tmp_pa = parse_gml_poslist(node, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		} else if (!strcmp((char *) node->name, "coordinates")) {
	  		tmp_pa = parse_gml_coordinates(node, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);

		} else if (!strcmp((char *) node->name, "coord")) {
	  		tmp_pa = parse_gml_coord(node, hasz);
			if (pa == NULL) pa = tmp_pa;
			else pa = ptarray_merge(pa, tmp_pa);
		}
	}

	if (pa == NULL) lwerror("invalid GML representation");

	return pa;
}


/**
 * Parse gml srsName attribute
 */ 
static int parse_gml_srs(xmlNodePtr xnode)
{
	xmlChar *srs;
	char *p;
	int srid=-1;

	srs = xmlGetProp(xnode, (xmlChar *) "srsName");
	if (!srs) return -1;	/* Default srid if no srsName attribute */

	/* Severals srsName formats are available...
	 *  cf WFS 1.1.0 -> 9.2 (p36)
	 *  cf ISO 19142 -> 7.9.2.4.4 (p34)
	 *  cf RFC 5165 <http://tools.ietf.org/html/rfc5165>
	 */

	/* SRS pattern like:   	EPSG:4326 					
	  			urn:EPSG:geographicCRS:4326			
	  		  	urn:ogc:def:crs:EPSG:4326 
	 			urn:ogc:def:crs:EPSG::4326 
	  			urn:ogc:def:crs:EPSG:6.6:4326
	   			urn:x-ogc:def:crs:EPSG:6.6:4326	*/

	if (	   !strncmp((char *) srs, "EPSG:", 5)
		|| !strncmp((char *) srs, "urn:ogc:def:crs:EPSG:", 21)
		|| !strncmp((char *) srs, "urn:x-ogc:def:crs:EPSG:", 23)
		|| !strncmp((char *) srs, "urn:EPSG:geographicCRS:", 23)) {

		/* retrieve the last ':' char */
		for (p = (char *) srs ; *p ; p++);
		for (--p ; *p != ':' ; p--)
			if (!isdigit(*p)) lwerror("unknown spatial reference system");

		srid = atoi(++p);

	/* SRS pattern like:	http://www.opengis.net/gml/srs/epsg.xml#4326 	*/
	}  else if (!strncmp((char *) srs, "http://www.opengis.net/gml/srs/epsg.xml#", 40)) {
		p = strchr((char *) srs, '#');
		srid = atoi(++p);
	}

	/* Check into spatial_ref_sys that this SRID really exist */
	if (srid == -1 || getSRSbySRID(srid, true) == NULL) 
		lwerror("unknown spatial reference system");

	xmlFree(srs);
	return srid;
}


/**
 * Parse GML point (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_point(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	POINTARRAY *pa;

	srid = parse_gml_srs(xnode);

	pa = parse_gml_data(xnode->children, hasz);
	if (pa->npoints != 1)
		lwerror("invalid GML representation");

	return (LWGEOM *) lwpoint_construct(srid, NULL, pa);
}


/**
 * Parse GML lineString (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_line(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	POINTARRAY *pa;

	srid = parse_gml_srs(xnode);
	pa = parse_gml_data(xnode->children, hasz);
	if (pa->npoints < 2)
		lwerror("invalid GML representation");

	return (LWGEOM *) lwline_construct(srid, NULL, pa);
}


/**
 * Parse GML Curve (3.x.x)
 */
static LWGEOM* parse_gml_curve(xmlNodePtr xnode, bool *hasz)
{
	int srid, lss, last, i;
	xmlChar *interpolation;
	POINTARRAY **ppa;
	POINTARRAY *pa;
	xmlNodePtr xa;
	bool found=false;
	unsigned int npoints=0;

	srid = parse_gml_srs(xnode);

	/* Looking for gml:segments */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!strcmp((char *) xa->name, "segments")) {
			found = true;
			break;
		}
	}
	if (!found) lwerror("invalid GML representation");

	ppa = (POINTARRAY**) lwalloc(sizeof(POINTARRAY*));

	/* Processing each gml:LineStringSegment */
	for (xa = xa->children, lss=0; xa != NULL ; xa = xa->next) {

		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "LineStringSegment")) continue;

		/* GML SF is resticted to linear interpolation  */
		interpolation = xmlGetProp(xa, (xmlChar *) "interpolation");
		if (interpolation != NULL) {
			if (strcmp((char *) interpolation, "linear"))
				lwerror("invalid GML representation");
			xmlFree(interpolation);
		}

		if (lss > 0) ppa = (POINTARRAY**) lwrealloc((POINTARRAY *) ppa,
				sizeof(POINTARRAY*) * (lss + 1));

		ppa[lss] = parse_gml_data(xa->children, hasz);
		npoints += ppa[lss]->npoints;
		if (ppa[lss]->npoints < 2)
			lwerror("invalid GML representation");
		lss++;
	}
	if (lss == 0) lwerror("invalid GML representation");

	/* Most common case, a single segment */
	if (lss == 1) return (LWGEOM *) lwline_construct(srid, NULL, ppa[0]);


	/* 
	 * "The curve segments are connected to one another, with the end point 
	 *  of each segment except the last being the start point of the next 
	 *  segment"  from  ISO 19107 -> 6.3.16.1 (p43)
	 *
	 * So we must aggregate all the segments into a single one and avoid
	 * to copy the redundants points
	 */
	pa = ptarray_construct(1, 0, npoints - (lss - 1));
	for (last = npoints = i = 0; i < lss ; i++) {

		if (i + 1 == lss) last = 1;
		/* Check if segments are not disjoints */
		if (i > 0 && memcmp(	getPoint_internal(pa, npoints),
					getPoint_internal(ppa[i], 0),
					*hasz?sizeof(POINT3D):sizeof(POINT2D)))
			lwerror("invalid GML representation");

		/* Aggregate stuff */
		memcpy(	getPoint_internal(pa, npoints),
			getPoint_internal(ppa[i], 0),
			pointArray_ptsize(ppa[i]) * (ppa[i]->npoints + last));

		npoints += ppa[i]->npoints - 1;
		lwfree(ppa[i]);
	}	
	lwfree(ppa);

	return (LWGEOM *) lwline_construct(srid, NULL, pa);
}


/**
 * Parse GML Polygon (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_polygon(xmlNodePtr xnode, bool *hasz)
{
	int srid, ring;
	xmlNodePtr xa, xb;
	POINTARRAY **ppa = NULL;

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* Polygon/outerBoundaryIs -> GML 2.1.2 */
		/* Polygon/exterior        -> GML 3.x.x */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if  (strcmp((char *) xa->name, "outerBoundaryIs") &&
		     strcmp((char *) xa->name, "exterior")) continue;
	       
		for (xb = xa->children ; xb != NULL ; xb = xb->next) {

			if (xb->type != XML_ELEMENT_NODE) continue;
			if (strcmp((char *) xb->name, "LinearRing")) continue;
			
			ppa = (POINTARRAY**) lwalloc(sizeof(POINTARRAY*));
			ppa[0] = parse_gml_data(xb->children, hasz);

			if (ppa[0]->npoints < 4
				|| (!*hasz && !ptarray_isclosed2d(ppa[0]))
				||  (*hasz && !ptarray_isclosed3d(ppa[0])))
				lwerror("invalid GML representation");
		}
	}

	for (ring=1, xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* Polygon/innerBoundaryIs -> GML 2.1.2 */
		/* Polygon/interior        -> GML 3.x.x */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if  (strcmp((char *) xa->name, "innerBoundaryIs") && 
		     strcmp((char *) xa->name, "interior")) continue;
		
		for (xb = xa->children ; xb != NULL ; xb = xb->next) {

			if (xb->type != XML_ELEMENT_NODE) continue;
			if (strcmp((char *) xb->name, "LinearRing")) continue;

			ppa = (POINTARRAY**) lwrealloc((POINTARRAY *) ppa,
				sizeof(POINTARRAY*) * (ring + 1));
			ppa[ring] = parse_gml_data(xb->children, hasz);

			if (ppa[ring]->npoints < 4
				|| (!*hasz && !ptarray_isclosed2d(ppa[ring]))
				||  (*hasz && !ptarray_isclosed3d(ppa[ring])))
				lwerror("invalid GML representation");
			ring++;
		}
	}
			
	/* Exterior Ring is mandatory */
	if (ppa == NULL || ppa[0] == NULL) lwerror("invalid GML representation");

	srid = parse_gml_srs(xnode);
	return (LWGEOM*) lwpoly_construct(srid, NULL, ring, ppa);
}


/**
 * Parse GML Surface (3.x.x)
 */
static LWGEOM* parse_gml_surface(xmlNodePtr xnode, bool *hasz)
{
	xmlChar *interpolation;
	xmlNodePtr xa, xb, xc;
	int patch, ring, srid;
	POINTARRAY **ppa;
	bool found=false;

	/* Looking for gml:patches */
	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		if (xa->type != XML_ELEMENT_NODE) continue;
		if (!strcmp((char *) xa->name, "patches")) {
			found = true;
			break;
		}
	}
	if (!found) lwerror("invalid GML representation");

	/* Processing gml:PolygonPatch */
	for (patch=0, xa = xa->children ; xa != NULL ; xa = xa->next) {

		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "PolygonPatch")) continue;
		patch++;

		/* SQL/MM define ST_CurvePolygon as a single patch only, 
		   cf ISO 13249-3 -> 4.2.9 (p27) */
		if (patch > 1) lwerror("invalid GML representation");

		/* GML SF is resticted to planar interpolation  */
		interpolation = xmlGetProp(xa, (xmlChar *) "interpolation");
		if (interpolation != NULL) {
			if (strcmp((char *) interpolation, "planar"))
				lwerror("invalid GML representation");
			xmlFree(interpolation);
		}

		/* PolygonPatch/exterior */
		for (xb = xa->children ; xb != NULL ; xb = xb->next) {
			if (strcmp((char *) xb->name, "exterior")) continue;
	       
			/* PolygonPatch/exterior/LinearRing */
			for (xc = xb->children ; xc != NULL ; xc = xc->next) {

				if (xc->type != XML_ELEMENT_NODE) continue;
				if (strcmp((char *) xc->name, "LinearRing")) continue;
			
				ppa = (POINTARRAY**) lwalloc(sizeof(POINTARRAY*));
				ppa[0] = parse_gml_data(xc->children, hasz);

				if (ppa[0]->npoints < 4
					|| (!*hasz && !ptarray_isclosed2d(ppa[0]))
					||  (*hasz && !ptarray_isclosed3d(ppa[0])))
					lwerror("invalid GML representation");
			}
		}

		/* PolygonPatch/interior */
		for (ring=1, xb = xa->children ; xb != NULL ; xb = xb->next) {

			if (xb->type != XML_ELEMENT_NODE) continue;
			if (strcmp((char *) xb->name, "interior")) continue;
			if (xb->children == NULL) break;	/* FIXME Why this is needed ??? */

			/* PolygonPatch/interior/LinearRing */
			for (xc = xb->children ; xc != NULL ; xc = xc->next) {

				if (xc->type != XML_ELEMENT_NODE) continue;
				if (strcmp((char *) xc->name, "LinearRing")) continue;

				ppa = (POINTARRAY**) lwrealloc((POINTARRAY *) ppa,
					sizeof(POINTARRAY*) * (ring + 1));
				ppa[ring] = parse_gml_data(xc->children, hasz);

				if (ppa[ring]->npoints < 4
					|| (!*hasz && !ptarray_isclosed2d(ppa[ring]))
					|| ( *hasz && !ptarray_isclosed3d(ppa[ring])))
					lwerror("invalid GML representation");
			}
			ring++;
		}
	}

	/* Exterior Ring is mandatory */
	if (ppa == NULL || ppa[0] == NULL) lwerror("invalid GML representation");

	srid = parse_gml_srs(xnode);
	return (LWGEOM*) lwpoly_construct(srid, NULL, ring, ppa);
}


/**
 * Parse gml:MultiPoint (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_mpoint(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, MULTIPOINTTYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* MultiPoint/pointMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "pointMember")) continue;
		if (xa->children != NULL)
			geom = lwmpoint_add((LWMPOINT *)geom, -1,
				parse_gml(xa->children, hasz));
	}

	return geom;
}


/**
 * Parse gml:MultiLineString (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_mline(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, MULTILINETYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* MultiLineString/lineMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "lineStringMember")) continue;
		if (xa->children != NULL)
			geom = lwmline_add((LWMLINE *)geom, -1,
				parse_gml(xa->children, hasz));
	}

	return geom;
}


/**
 * Parse GML MultiCurve (3.x.x)
 */
static LWGEOM* parse_gml_mcurve(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, MULTILINETYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* MultiCurve/curveMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "curveMember")) continue;
		if (xa->children != NULL)
			geom = lwmline_add((LWMLINE *)geom, -1,
				parse_gml(xa->children, hasz));
	}

	return geom;
}


/**
 * Parse GML MultiPolygon (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_mpoly(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, MULTIPOLYGONTYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* MultiPolygon/polygonMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "polygonMember")) continue;
		if (xa->children != NULL)
			geom = lwmpoly_add((LWMPOLY *)geom, -1,
				parse_gml(xa->children, hasz));
	}

	return geom;
}


/**
 * Parse GML MultiSurface (3.x.x)
 */
static LWGEOM* parse_gml_msurface(xmlNodePtr xnode, bool *hasz)
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, MULTIPOLYGONTYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		/* MultiSurface/surfaceMember */
		if (xa->type != XML_ELEMENT_NODE) continue;
		if (strcmp((char *) xa->name, "surfaceMember")) continue;
		if (xa->children != NULL)
			geom = lwmpoly_add((LWMPOLY *)geom, -1,
				parse_gml(xa->children, hasz));
	}

	return geom;
}


/**
 * Parse GML MultiGeometry (2.1.2, 3.x.x)
 */
static LWGEOM* parse_gml_coll(xmlNodePtr xnode, bool *hasz) 
{
	int srid;
	xmlNodePtr xa;
	LWGEOM *geom = NULL;

	srid = parse_gml_srs(xnode);

       	geom = (LWGEOM *)lwcollection_construct_empty(srid, 1, 0);
       	geom->type = lwgeom_makeType(1, 0, srid != -1, COLLECTIONTYPE);

	for (xa = xnode->children ; xa != NULL ; xa = xa->next) {

		if (xa->type != XML_ELEMENT_NODE) continue;

		if (	   !strcmp((char *) xa->name, "pointMember")
		 	|| !strcmp((char *) xa->name, "lineMember") 
		 	|| !strcmp((char *) xa->name, "curveMember") 
		 	|| !strcmp((char *) xa->name, "polygonMember") 
		 	|| !strcmp((char *) xa->name, "surfaceMember")
		 	|| !strcmp((char *) xa->name, "geometryMember")) {

			if (xa->children == NULL) break;
			geom = lwcollection_add((LWCOLLECTION *)geom, -1,
				 parse_gml(xa->children, hasz));

		} else if (  !strcmp((char *) xa->name, "MultiPoint")
			  || !strcmp((char *) xa->name, "MultiLineString")
			  || !strcmp((char *) xa->name, "MultiCurve")
			  || !strcmp((char *) xa->name, "MultiPolygon")
			  || !strcmp((char *) xa->name, "MultiSurface") 
			  || !strcmp((char *) xa->name, "MultiGeometry")) {

			if (xa->children == NULL) break;
			geom = lwcollection_add((LWCOLLECTION *)geom, -1,
				parse_gml(xa, hasz));
		}
	}

	return geom;
}


/**
 * Parse GML 
 */
static LWGEOM* parse_gml(xmlNodePtr xnode, bool *hasz) 
{
	xmlNodePtr xa = xnode;

	while (xa != NULL && xa->type != XML_ELEMENT_NODE) xa = xa->next;

	if (!strcmp((char *) xa->name, "Point"))
		return parse_gml_point(xa, hasz);

	if (!strcmp((char *) xa->name, "LineString"))
		return parse_gml_line(xa, hasz);

	if (!strcmp((char *) xa->name, "Curve"))
		return parse_gml_curve(xa, hasz);

	if (!strcmp((char *) xa->name, "Polygon"))
		return parse_gml_polygon(xa, hasz);

	if (!strcmp((char *) xa->name, "Surface"))
		return parse_gml_surface(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiPoint"))
		return parse_gml_mpoint(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiLineString"))
		return parse_gml_mline(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiCurve"))
		return parse_gml_mcurve(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiPolygon"))
		return parse_gml_mpoly(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiSurface"))
		return parse_gml_msurface(xa, hasz);

	if (!strcmp((char *) xa->name, "MultiGeometry"))
		return parse_gml_coll(xa, hasz);
	
	lwerror("invalid GML representation");
	return NULL; /* Never reach */
}

#endif /* if HAVE_LIBXML2 */
