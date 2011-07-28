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
 * Note: the coordinates of the supplied geometries should be within and scaled
 * to the x-y range of [0,200], otherwise the rendered geometries may be
 * rendered outside of the generated image's extents, or may be rendered too
 * small to be recognizable as anything other than a single point.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "liblwgeom.h"
#include "styles.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 for dot and sign */

// Some global styling variables
char *imageSize = "200x200";


int getStyleName(char **styleName, char* line);

/**
 * Set up liblwgeom to run in stand-alone mode using the
 * usual system memory handling functions.
 */
void lwgeom_init_allocators(void)
{
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
pointarrayToString(char *output, POINTARRAY *pa)
{
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	int i;
	char *ptr = output;

	for ( i=0; i < pa->npoints; i++ )
	{
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
drawPoint(char *output, LWPOINT *lwp, LAYERSTYLE *styles)
{
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y1[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y2[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char *ptr = output;
	POINTARRAY *pa = lwp->point;
	POINT2D p;
	getPoint2d_p(pa, 0, &p);

	LWDEBUGF(4, "%s", "drawPoint called");
	LWDEBUGF( 4, "point = %s", lwgeom_to_ewkt((LWGEOM*)lwp,0) );

	sprintf(x, "%f", p.x);
	trim_trailing_zeros(x);
	sprintf(y1, "%f", p.y);
	trim_trailing_zeros(y1);
	sprintf(y2, "%f", p.y + styles->pointSize);
	trim_trailing_zeros(y2);

	ptr += sprintf(ptr, "-fill %s -strokewidth 0 ", styles->pointColor);
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
drawLineString(char *output, LWLINE *lwl, LAYERSTYLE *style)
{
	char *ptr = output;

	LWDEBUGF(4, "%s", "drawLineString called");
	LWDEBUGF( 4, "line = %s", lwgeom_to_ewkt((LWGEOM*)lwl,0) );

	ptr += sprintf(ptr, "-fill none -stroke %s -strokewidth %d ", style->lineColor, style->lineWidth);
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
drawPolygon(char *output, LWPOLY *lwp, LAYERSTYLE *style)
{
	char *ptr = output;
	int i;

	LWDEBUGF(4, "%s", "drawPolygon called");
	LWDEBUGF( 4, "poly = %s", lwgeom_to_ewkt((LWGEOM*)lwp,0) );

	ptr += sprintf(ptr, "-fill %s -stroke %s -strokewidth %d ", style->polygonFillColor, style->polygonStrokeColor, style->polygonStrokeWidth );
	ptr += sprintf(ptr, "-draw \"path '");
	for (i=0; i<lwp->nrings; i++)
	{
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
drawGeometry(char *output, LWGEOM *lwgeom, LAYERSTYLE *styles )
{
	char *ptr = output;
	int i;
	int type = lwgeom->type;

	switch (type)
	{
	case POINTTYPE:
		ptr += drawPoint(ptr, (LWPOINT*)lwgeom, styles );
		break;
	case LINETYPE:
		ptr += drawLineString(ptr, (LWLINE*)lwgeom, styles );
		break;
	case POLYGONTYPE:
		ptr += drawPolygon(ptr, (LWPOLY*)lwgeom, styles );
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		for (i=0; i<((LWCOLLECTION*)lwgeom)->ngeoms; i++)
		{
			ptr += drawGeometry( ptr, lwcollection_getsubgeom ((LWCOLLECTION*)lwgeom, i), styles );
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
addDropShadow(int layerNumber)
{
	// TODO: change to properly sized string
	char str[512];
	sprintf(
	    str,
	    "convert tmp%d.png -gravity center \\( +clone -background navy -shadow 100x3+4+4 \\) +swap -background none -flatten tmp%d.png",
	    layerNumber, layerNumber);
	LWDEBUGF(4, "%s", str);
	system(str);
}

/**
 * Invokes a system call to ImageMagick's "convert" command that adds a
 * highlight to the current layer image.
 *
 * @param layerNumber the current working layer number.
 */
static void
addHighlight(int layerNumber)
{
	// TODO: change to properly sized string
	char str[512];
	sprintf(
	    str,
	    "convert tmp%d.png \\( +clone -channel A -separate +channel -negate -background black -virtual-pixel background -blur 0x3 -shade 120x55 -contrast-stretch 0%% +sigmoidal-contrast 7x50%% -fill grey50 -colorize 10%% +clone +swap -compose overlay -composite \\) -compose In -composite tmp%d.png",
	    layerNumber, layerNumber);
	LWDEBUGF(4, "%s", str);
	system(str);
}

/**
 * Invokes a system call to ImageMagick's "convert" command that reduces
 * the overall filesize
 *
 * @param filename the current working image.
 */
static void
optimizeImage(char* filename)
{
	char *str;
	str = malloc( (18 + (2*strlen(filename)) + 1) * sizeof(char) );
	sprintf(str, "convert %s -depth 8 %s", filename, filename);
	LWDEBUGF(4, "%s", str);
	system(str);
	free(str);
}

/**
 * Flattens all the temporary processing png files into a single image
 */
static void
flattenLayers(char* filename)
{
	char *str;
	str = malloc( (48 + strlen(filename) + 1) * sizeof(char) );
	sprintf(str, "convert tmp[0-9].png -background white -flatten %s", filename);

	LWDEBUGF(4, "%s", str);
	system(str);
	// TODO: only remove the tmp files if they exist.
	remove("tmp0.png");
	remove("tmp1.png");
	remove("tmp2.png");
	remove("tmp3.png");
	remove("tmp4.png");
	remove("tmp5.png");
	free(str);
}


// TODO: comments
int
getStyleName(char **styleName, char* line)
{
	char *ptr = strrchr(line, ';');
	if (ptr == NULL)
	{
		*styleName = malloc( 8 );
		strncpy(*styleName, "Default", 7);
		(*styleName)[7] = '\0';
		return 1;
	}
	else
	{
		*styleName = malloc( ptr - line + 1);
		strncpy(*styleName, line, ptr - line);
		(*styleName)[ptr - line] = '\0';
		LWDEBUGF( 4, "%s", *styleName );
		return 0;
	}
}


/**
 * Main Application.  Currently, drawing styles are hardcoded in this method.
 * Future work may entail reading the styles from a .properties file.
 */
int main( int argc, const char* argv[] )
{
	FILE *pfile;
	LWGEOM *lwgeom;
	char line [2048];
	char *filename;
	int layerCount;
	int styleNumber;
	LAYERSTYLE *styles;

	getStyles(&styles);

	if ( argc != 2 )
	{
		lwerror("You must specifiy a wkt filename to convert.\n");
		return -1;
	}

	if ( (pfile = fopen(argv[1], "r")) == NULL)
	{
		perror ( argv[1] );
		return -1;
	}

	filename = malloc( strlen(argv[1])+11 );
	strncpy( filename, "../images/", 11 );
	strncat( filename, argv[1], strlen(argv[1])-3 );
	strncat( filename, "png", 3 );

	printf( "generating %s\n", filename );

	layerCount = 0;
	while ( fgets ( line, sizeof line, pfile ) != NULL && !isspace(*line) )
	{

		char output[32768];
		char *ptr = output;
		char *styleName;
		int useDefaultStyle;

		ptr += sprintf( ptr, "convert -size %s xc:none ", imageSize );

		useDefaultStyle = getStyleName(&styleName, line);
		LWDEBUGF( 4, "%s", styleName );

		if (useDefaultStyle)
		{
			printf("   Warning: using Default style for layer %d\n", layerCount);
			lwgeom = lwgeom_from_ewkt( line, PARSER_CHECK_NONE );
		}
		else
			lwgeom = lwgeom_from_ewkt( line+strlen(styleName)+1, PARSER_CHECK_NONE );
		LWDEBUGF( 4, "geom = %s", lwgeom_to_ewkt((LWGEOM*)lwgeom,0) );

		styleNumber = layerCount % length(styles);
		ptr += drawGeometry( ptr, lwgeom, getStyle(styles, styleName) );

		ptr += sprintf( ptr, "-flip tmp%d.png", layerCount );

		lwfree( lwgeom );

		LWDEBUGF( 4, "%s", output );
		system(output);

		addHighlight( layerCount );
		addDropShadow( layerCount );
		layerCount++;
		free(styleName);
	}

	flattenLayers(filename);
	optimizeImage(filename);

	fclose(pfile);
	free(filename);
	freeStyles(&styles);
	return 0;
}
