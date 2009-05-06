/**********************************************************************
 * $Id: generator.c 3967 2009-05-04 16:48:11Z kneufeld $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Kevin Neufeld
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * This program will generate a .png image for every .wkt file specified 
 * in this directory's Makefile.  Every .wkt file may contain several 
 * entries of geometries represented as WKT strings.  Every line in 
 * a wkt file is stylized using a predetermined style (line thinkness, 
 * fill color, etc) currently hard coded in this programs main function.
 *
 * In order to generate a png file, ImageMagicK must be installed in the
 * user's path as system calls are invoked to "convert".  In this manner,
 * WKT files are converted into SVG syntax and rasterized as png.  (PostGIS's
 * internal SVG methods could not be used dues to syntax issues with ImageMagick)
 *
 * The goal of this application is to dynamically generate all the spatial
 * pictures used in PostGIS's documentation pages.
 *
 * Note: the coordinates of the supplied geometries should be within the x-y range 
 * of 200, otherwise they will appear outside of the generated image.
 * 
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "lwalgorithm.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 for dot and sign */

// Some global styling variables
char *imageSize = "200x200";

typedef struct {	
	int	  pointSize;
	char *pointColor;
	
	int   lineWidth;
	char *lineColor;
	
	char *polygonFillColor;
	char *polygonStrokeColor;
	int   polygonStrokeWidth;
} LAYERSTYLE;


/**
 * Set up liblwgeom to run in stand-alone mode using the 
 * usual system memory handling functions.
 */
void lwgeom_init_allocators(void) {
		/* liblwgeom callback - install default handlers */
		lwgeom_install_default_allocators();
}

/**
 * Writes the coordinates of a POINTARRAY to a char* where ordinates are 
 * separated by a comma and coordinates by a space so that the coordinate
 * pairs can be interpreted by ImageMagick's SVG draw command.
 *
 * @param output a reference to write the POINTARRAY to
 * @param pa a reference to a POINTARRAY
 * @return the numbers of character written to *output
 */
static size_t 
pointarrayToString(char *output, POINTARRAY *pa) {
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	int i;
	char *ptr = output;
	
	for ( i=0; i < pa->npoints; i++ ) {
		POINT2D pt;
		getPoint2d_p(pa, i, &pt);
		sprintf(x, "%f", pt.x);
		trim_trailing_zeros(x);
		sprintf(y, "%f", pt.y);
		trim_trailing_zeros(y);
		if ( i ) ptr += sprintf(ptr, " ");
		ptr += sprintf(ptr, "%s,%s", x, y);
	}
	
	return (ptr - output);
}

/**
 * Serializes a LWPOINT to a char*.  This is a helper function that partially
 * writes the appropriate draw and fill commands used to generate an SVG image
 * using ImageMagick's "convert" command.
 
 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 * @return the numbers of character written to *output
 */
static size_t
drawPoint(char *output, LWPOINT *lwp, LAYERSTYLE style) {
	LWDEBUGF( 4, "%s", "enter drawPoint" );
	
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y1[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y2[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char *ptr = output;
	POINTARRAY *pa = lwp->point;
	POINT2D p;
	getPoint2d_p(pa, 0, &p);
	
	sprintf(x, "%f", p.x);
	trim_trailing_zeros(x);
	sprintf(y1, "%f", p.y);
	trim_trailing_zeros(y1);
	sprintf(y2, "%f", p.y + style.pointSize);
	trim_trailing_zeros(y2);
		
	ptr += sprintf(ptr, "-fill %s -strokewidth 5 ", style.pointColor);
	ptr += sprintf(ptr, "-draw \"circle %s,%s %s,%s", x, y1, x, y2);
	ptr += sprintf(ptr, "'\" ");
	
	return (ptr - output);
}

/**
 * Serializes a LWLINE to a char*.  This is a helper function that partially
 * writes the appropriate draw and stroke commands used to generate an SVG image
 * using ImageMagick's "convert" command.
 
 * @param output a char reference to write the LWLINE to
 * @param lwl a reference to a LWLINE
 * @return the numbers of character written to *output
 */
static size_t
drawLineString(char *output, LWLINE *lwl, LAYERSTYLE style) {
	LWDEBUGF( 4, "%s", "enter drawLineString" );
	char *ptr = output;

	ptr += sprintf(ptr, "-fill none -stroke %s -strokewidth %d ", style.lineColor, style.lineWidth);
	ptr += sprintf(ptr, "-draw \"stroke-linecap round stroke-linejoin round path 'M ");
	ptr += pointarrayToString(ptr, lwl->points );
	ptr += sprintf(ptr, "'\" ");
	
	return (ptr - output);
}

/**
 * Serializes a LWPOLY to a char*.  This is a helper function that partially
 * writes the appropriate draw and fill commands used to generate an SVG image
 * using ImageMagick's "convert" command.
 
 * @param output a char reference to write the LWPOLY to
 * @param lwp a reference to a LWPOLY
 * @return the numbers of character written to *output
 */
static size_t
drawPolygon(char *output, LWPOLY *lwp, LAYERSTYLE style) {
	LWDEBUGF( 4, "%s", "enter drawPolygon" );
	
	char *ptr = output;
	int i;

	ptr += sprintf(ptr, "-fill %s -stroke %s -strokewidth %d ", style.polygonFillColor, style.polygonStrokeColor, style.polygonStrokeWidth );
	ptr += sprintf(ptr, "-draw \"path '");
	for (i=0; i<lwp->nrings; i++) {
		ptr += sprintf(ptr, "M ");
		ptr += pointarrayToString(ptr, lwp->rings[i] );
		ptr += sprintf(ptr, " ");
	}
	ptr += sprintf(ptr, "'\" ");
	
	return (ptr - output);
}

/**
 * Serializes a LWGEOM to a char*.  This is a helper function that partially
 * writes the appropriate draw, stroke, and fill commands used to generate an 
 * SVG image using ImageMagick's "convert" command.
 
 * @param output a char reference to write the LWGEOM to
 * @param lwgeom a reference to a LWGEOM
 * @return the numbers of character written to *output
 */
static size_t
drawGeometry(char *output, LWGEOM *lwgeom, LAYERSTYLE style ) {
	LWDEBUGF( 4, "%s", "enter drawGeometry" );
	char *ptr = output;
	int i;
	int type = lwgeom_getType(lwgeom->type);	
				
	LWDEBUGF( 4, "switching on %d", type );
	switch(type) {
		case POINTTYPE:
			ptr += drawPoint(ptr, (LWPOINT*)lwgeom, style );
			break;
		case LINETYPE:
			ptr += drawLineString(ptr, (LWLINE*)lwgeom, style );
			break;
		case POLYGONTYPE:
			ptr += drawPolygon(ptr, (LWPOLY*)lwgeom, style );
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			for (i=0; i<((LWCOLLECTION*)lwgeom)->ngeoms; i++) {
				ptr += drawGeometry( ptr, lwcollection_getsubgeom ((LWCOLLECTION*)lwgeom, i), style );
			}
			break;
	 }
	 
	return (ptr - output);
}

/**
 * Invokes a system call to ImageMagick's "convert" command that adds a drop 
 * shadow to the current layer image.
 * 
 * @param layerNumber the current working layer number.
 */
static void
addDropShadow(int layerNumber) {
	char str[129];
	sprintf(
		str, 
		"convert tmp%d.png -gravity center \\( +clone -background navy -shadow 100x3+4+4 \\) +swap -background none -flatten tmp%d.png", 
		layerNumber, layerNumber);
	system(str);
}

/**
 * Invokes a system call to ImageMagick's "convert" command that adds a  
 * highlight to the current layer image.
 * 
 * @param layerNumber the current working layer number.
 */
static void
addHighlight(int layerNumber) {
	char str[129];
	sprintf(
		str, 
		"convert tmp%d.png  -fx A +matte -blur 1x1  -shade 120x45  -normalize tmp%d.png  -compose Overlay -composite tmp%d.png  -matte  -compose Dst_In  -composite tmp%d.png", 
		layerNumber, layerNumber, layerNumber, layerNumber);
	system(str);
}

/**
 * Flattens all the temporary processing png files into a single image
 */
static void
flattenLayers(char* filename) {
	char *str;
	str = malloc( (48 + strlen(filename) + 1) * sizeof(char) );
	sprintf(str, "convert tmp[0-9].png -background none -flatten %s", filename);
	system(str);
	system("rm -f tmp[0-9].png");
	free(str);
}


/**
 * Main Application.  Currently, drawing styles are hardcoded in this method.
 * Future work may entail reading the styles from a .properties file.
 */
int main( int argc, const char* argv[] ) {
	FILE *pfile;
	LWGEOM *lwgeom;
	char line [2048];
	char *filename;
	int layerCount;
	int styleNumber;
	LAYERSTYLE styles[3];
	
	styles[0].pointSize = 6;
	styles[0].pointColor = "Blue";
	styles[0].lineWidth = 7;
	styles[0].lineColor = "Blue";
	styles[0].polygonFillColor = "Blue";
	styles[0].polygonStrokeColor = "Blue";
	styles[0].polygonStrokeWidth = 1;
	
	styles[1].pointSize = 6;
	styles[1].pointColor = "Green";
	styles[1].lineWidth = 7;
	styles[1].lineColor = "Green";
	styles[1].polygonFillColor = "Green";
	styles[1].polygonStrokeColor = "Green";
	styles[1].polygonStrokeWidth = 1;
	
	styles[2].pointSize = 6;
	styles[2].pointColor = "Red";
	styles[2].lineWidth = 7;
	styles[2].lineColor = "Red";
	styles[2].polygonFillColor = "Red";
	styles[2].polygonStrokeColor = "Red";
	styles[2].polygonStrokeWidth = 1;
	
		
	if ( argc != 2 ) {
		printf("You must specifiy a wkt filename to convert.\n");
		return -1;
	}
	
	if( (pfile = fopen(argv[1], "r")) == NULL){
		perror ( argv[1] );
		return -1;
	}
		
	filename = malloc( (strlen(argv[1])+8) * sizeof(char) );
	strcpy( filename, "../images/" );
	strncat( filename, argv[1], strlen(argv[1])-3 );
	strcat( filename, "png" );
	
	printf( "generating %s\n", filename );
	
	layerCount = 0;
	while ( fgets ( line, sizeof line, pfile ) != NULL ) {		
		
		char output [2048];
		char *ptr = output;
		ptr += sprintf( ptr, "convert -size %s xc:none ", imageSize );
	
		lwgeom = lwgeom_from_ewkt( line, PARSER_CHECK_NONE );
		LWDEBUGF( 4, "geom = %s", lwgeom_to_ewkt((LWGEOM*)lwgeom,0) );
		
		styleNumber = layerCount % 3;
		LWDEBUGF( 4, "using style %d", styleNumber );
		ptr += drawGeometry( ptr, lwgeom, styles[styleNumber] );
		LWDEBUGF( 4, "%s", "after drawGeometry" );
		
		ptr += sprintf( ptr, "-flip tmp%d.png", layerCount );
		 
		lwfree( lwgeom );	
		
		LWDEBUGF( 4, "%s", output );
		system(output);
		
		addHighlight( layerCount );
		addDropShadow( layerCount );
		layerCount++;
	}
	
	LWDEBUGF(4, "%s", filename);
	flattenLayers(filename);
	
	fclose(pfile);
	free(filename);
	return 0;
}
