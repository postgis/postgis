/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2022-2023 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2022 Martin Davis
 * Copyright (C) 2008 Kevin Neufeld
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * This program will generate a .png image for every .wkt file specified
 * in this directory's Makefile.  Every .wkt file may contain several
 * entries of geometries represented as WKT strings.  Every line in
 * a wkt file is stylized using a predetermined style (line thickness,
 * fill color, etc).
 * The styles are specified in the adjacent styles.conf file.
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
 * Usage:
 *  generator [-v] [-s <width>x<height>] <source_wktfile> [<output_pngfile>]
 *
 * -v - show generated Imagemagick commands
 * -s - output dimension, if omitted defaults to 200x200
 *
 * If <output_pngfile> is omitted the output image PNG file has the
 * same name as the source file
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* for rmdir */
#include <ctype.h>
#include <sys/wait.h> /* for WEXITSTATUS */
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "styles.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 for dot and sign */

bool optionVerbose = false;

// Some global styling variables
const char *imageSize = "200x200";

char tempdir_template[] = "generator-XXXXXX";
char *tmpdir = NULL;

typedef struct draw_context_t {
	LAYERSTYLE *style;
	const char *tmpdir;
	int drawNum; /* number of draw commands */
} GEOMETRY_DRAW_CONTEXT;

static void
initializeGeometryDrawContext(GEOMETRY_DRAW_CONTEXT *ctx) {
	ctx->style = NULL;
	ctx->tmpdir = NULL;
	ctx->drawNum = 0;
}

static void
checked_system(const char* cmd)
{
  int ret = system(cmd);
	if ( WEXITSTATUS(ret) != 0 ) {
		fprintf(stderr, "Failure return code (%d) from command: %s", WEXITSTATUS(ret), cmd);
		exit(EXIT_FAILURE);
	}
}

static void
cleanupTempDir(const char *dir)
{
	struct dirent *p;
	DIR *d = opendir(dir);
	char *buf;
	size_t maxlen;

	if ( NULL == d ) {
		perror( dir );
		exit(EXIT_FAILURE); /* or be tolerant ? */
	}

	maxlen = strlen(dir) + 64;
	buf = malloc(maxlen);

	while ( (p=readdir(d)) ) {
		if ( strcmp(p->d_name, ".") == 0 ) continue;
		if ( strcmp(p->d_name, ".." ) == 0) continue;
		snprintf(buf, maxlen-1, "%s/%s", dir, p->d_name);
		remove(buf);
	}

	closedir(d);
	rmdir(dir);
}

/**
 * Writes the coordinates of a POINTARRAY to a FILE* where ordinates are
 * separated by a comma and coordinates by a space so that the coordinate
 * pairs can be interpreted by ImageMagick's SVG draw command.
 *
 * @param output a file to write the POINTARRAY to
 * @param pa a reference to a POINTARRAY
 * @return the numbers of character written to *output
 */
static size_t
pointarrayToFile(FILE *output, POINTARRAY *pa)
{
	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y[OUT_DOUBLE_BUFFER_SIZE];
	unsigned int i, written = 0;

	for ( i=0; i < pa->npoints; i++ )
	{
		POINT2D pt;
		getPoint2d_p(pa, i, &pt);

		lwprint_double(pt.x, 10, x);
		lwprint_double(pt.y, 10, y);

		if ( i ) written += fprintf(output, " ");
		written += fprintf(output, "%s,%s", x, y);
	}

	return written;
}

/**
 * Draws a point in a POINTARRAY to a char* using ImageMagick SVG for styling.

 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 * @return the numbers of character written to *output
 */
static size_t
drawPointSymbol(char *output, POINTARRAY *pa, unsigned int index, int size, char* color)
{
	// short-circuit no-op
	if (size <= 0) return 0;

	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y1[OUT_DOUBLE_BUFFER_SIZE];
	char y2[OUT_DOUBLE_BUFFER_SIZE];
	char *ptr = output;

	POINT2D p;
	getPoint2d_p(pa, index, &p);

	lwprint_double(p.x, 10, x);
	lwprint_double(p.y, 10, y1);
	lwprint_double(p.y + size, 10, y2);

	ptr += sprintf(ptr, "-fill %s -strokewidth 0 ", color);
	ptr += sprintf(ptr, "-draw \"circle %s,%s %s,%s", x, y1, x, y2);
	ptr += sprintf(ptr, "'\" ");

	return (ptr - output);
}

/**
 * Draws a point in a POINTARRAY to a char* using ImageMagick SVG for styling.

 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 * @return the numbers of character written to *output
 */
static size_t
drawLineArrow(char *output, POINTARRAY *pa, int size, int strokeWidth, char* color)
{
	// short-circuit no-op
	if (size <= 0) return 0;
	if (pa->npoints <= 1) return 0;

	char s0x[OUT_DOUBLE_BUFFER_SIZE];
	char s0y[OUT_DOUBLE_BUFFER_SIZE];
	char s1x[OUT_DOUBLE_BUFFER_SIZE];
	char s1y[OUT_DOUBLE_BUFFER_SIZE];
	char s2x[OUT_DOUBLE_BUFFER_SIZE];
	char s2y[OUT_DOUBLE_BUFFER_SIZE];

	POINT2D pn;
	getPoint2d_p(pa, pa->npoints-1, &pn);
	POINT2D pn1;
	getPoint2d_p(pa, pa->npoints-2, &pn1);

	double dx = pn1.x - pn.x;
	double dy = pn1.y - pn.y;
	double len = sqrt(dx*dx + dy*dy);
	//-- abort if final line segment has length 0
	if (len <= 0) return 0;

	double offx = -0.5 * size * dy/len;
	double offy =  0.5 * size * dx/len;


	double p1x = pn.x + size * dx / len + offx;
	double p1y = pn.y + size * dy / len + offy;
	double p2x = pn.x + size * dx / len - offx;
	double p2y = pn.y + size * dy / len - offy;

	lwprint_double(pn.x, 10, s0x);
	lwprint_double(pn.y, 10, s0y);
	lwprint_double(p1x,  10, s1x);
	lwprint_double(p1y,  10, s1y);
	lwprint_double(p2x,  10, s2x);
	lwprint_double(p2y,  10, s2y);

	char *ptr = output;
	ptr += sprintf(ptr, "-fill %s -strokewidth %d ", color, 2);
	ptr += sprintf(ptr, "-draw \"path 'M %s,%s %s,%s %s,%s %s,%s'\" ", s0x, s0y, s1x, s1y, s2x, s2y, s0x, s0y);

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
drawPoint(char *output, LWPOINT *lwp, GEOMETRY_DRAW_CONTEXT *ctx)
{
	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y1[OUT_DOUBLE_BUFFER_SIZE];
	char y2[OUT_DOUBLE_BUFFER_SIZE];
	char *ptr = output;
	LAYERSTYLE *styles = ctx->style;
	POINTARRAY *pa = lwp->point;
	POINT2D p;
	getPoint2d_p(pa, 0, &p);

	LWDEBUGF(4, "%s", "drawPoint called");
	LWDEBUGF( 4, "point = %s", lwgeom_to_ewkt((LWGEOM*)lwp) );

	lwprint_double(p.x, 10, x);
	lwprint_double(p.y, 10, y1);
	lwprint_double(p.y + styles->pointSize, 10, y2);

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
drawLineString(char *output, LWLINE *lwl, GEOMETRY_DRAW_CONTEXT *ctx)
{
	char *ptr = output;
	LAYERSTYLE *style = ctx->style;
	char *drawFname;
	FILE *drawFile;

	LWDEBUGF(4, "%s", "drawLineString called");
	LWDEBUGF( 4, "line = %s", lwgeom_to_ewkt((LWGEOM*)lwl) );

	ptr += sprintf(ptr, "-fill none -stroke %s -strokewidth %d ", style->lineColor, style->lineWidth);

	ptr += sprintf(ptr, "-draw '@");
	drawFname = ptr; /* hack to save allocating a new string just for the filename */
	ptr += sprintf(ptr, "%s/draw%d", ctx->tmpdir, ctx->drawNum++);
	drawFile = fopen(drawFname, "w");
	if ( NULL == drawFile ) {
		perror( drawFname );
		exit(EXIT_FAILURE); /* or be tolerant ? */
	}
	ptr += sprintf(ptr, "' "); /* from now on drawFname is invalid */

	fprintf(drawFile, "stroke-linecap round stroke-linejoin round path 'M ");
	pointarrayToFile(drawFile, lwl->points );
	fprintf(drawFile, "'");

	fclose(drawFile);

	ptr += drawPointSymbol(ptr, lwl->points, 0, style->lineStartSize, style->lineColor);
	ptr += drawPointSymbol(ptr, lwl->points, lwl->points->npoints-1, style->lineEndSize, style->lineColor);
	ptr += drawLineArrow(ptr, lwl->points, style->lineArrowSize, style->lineWidth, style->lineColor);

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
drawPolygon(char *output, LWPOLY *lwp, GEOMETRY_DRAW_CONTEXT *ctx)
{
	char *ptr = output;
	unsigned int i;
	LAYERSTYLE *style = ctx->style;
	char *drawFname;
	FILE *drawFile;

	LWDEBUGF(4, "%s", "drawPolygon called");
	LWDEBUGF( 4, "poly = %s", lwgeom_to_ewkt((LWGEOM*)lwp) );

	ptr += sprintf(ptr, "-fill %s -stroke %s -strokewidth %d ", style->polygonFillColor, style->polygonStrokeColor, style->polygonStrokeWidth );

	ptr += sprintf(ptr, "-draw '@");
	drawFname = ptr; /* hack to save allocating a new string just for the filename */
	ptr += sprintf(ptr, "%s/draw%d", ctx->tmpdir, ctx->drawNum++);
	drawFile = fopen(drawFname, "w");
	if ( NULL == drawFile ) {
		perror( drawFname );
		exit(EXIT_FAILURE); /* or be tolerant ? */
	}
	ptr += sprintf(ptr, "' "); /* from now on drawFname is invalid */

	fprintf(drawFile, "path '");
	for (i=0; i<lwp->nrings; i++)
	{
		fprintf(drawFile, "M ");
		pointarrayToFile(drawFile, lwp->rings[i] );
		fprintf(drawFile, " ");
	}
	fprintf(drawFile, "'");

	fclose(drawFile);

	return (ptr - output);
}

/**
 * Serializes a LWGEOM to a char*.  This is a helper function that partially
 * writes the appropriate draw, stroke, and fill commands used to generate an
 * SVG image using ImageMagick's "convert" command.

 * @param output a char reference to write the LWGEOM to
 * @param lwgeom a reference to a LWGEOM
 * @param ctx drawing context
 * @return the numbers of character written to *output
 */
static size_t
drawGeometry(char *output, const LWGEOM *lwgeom, GEOMETRY_DRAW_CONTEXT *ctx)
{
	char *ptr = output;
	unsigned int i;
	int type = lwgeom->type;

	switch (type)
	{
	case POINTTYPE:
		ptr += drawPoint(ptr, (LWPOINT*)lwgeom, ctx );
		break;
	case LINETYPE:
		ptr += drawLineString(ptr, (LWLINE*)lwgeom, ctx );
		break;
	case POLYGONTYPE:
		ptr += drawPolygon(ptr, (LWPOLY*)lwgeom, ctx );
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		for (i=0; i<((LWCOLLECTION*)lwgeom)->ngeoms; i++)
		{
			ptr += drawGeometry( ptr, lwcollection_getsubgeom ((LWCOLLECTION*)lwgeom, i), ctx );
		}
		break;
	}

	return (ptr - output);
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
	if (optionVerbose) {
		puts(str);
	}
	checked_system(str);
	free(str);
}

/**
 * Flattens all the temporary processing png files into a single image
 */
static void
flattenLayers(char* filename)
{
	char *str = malloc( (48 + strlen(filename) + strlen(tmpdir) + 2) * sizeof(char) );
	sprintf(str, "convert %s/tmp*.png -background white -flatten %s", tmpdir, filename);
	if (optionVerbose) {
		puts(str);
	}

	LWDEBUGF(4, "%s", str);
	checked_system(str);
	free(str);
}


// TODO: comments
static int
getStyleName(char **styleName, const char* line)
{
	char *ptr = strrchr(line, ';');
	if (ptr == NULL)
	{
		*styleName = strdup("Default");
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

int parseOptions(int argc, const char* argv[] )
{
	if (argc <= 1) return 1;

	int argPos = 1;
	while (argPos < argc && strncmp(argv[argPos], "-", 1) == 0) {
		if (strncmp(argv[argPos], "-v", 2) == 0) {
			optionVerbose = true;
		}
		if (strncmp(argv[argPos], "-s", 2) == 0) {
			if ( ++argPos >= argc ) return 1;
			imageSize = argv[argPos];
		}
		argPos++;
	}
	return argPos;
}

/**
 * Main Application.
 */
int main( int argc, const char* argv[] )
{
	FILE *pfile;
	LWGEOM *lwgeom;
	char line [65536];
	char *filename;
	int layerCount;
	LAYERSTYLE *styles;
	char *stylefile_path;
	const char *image_src;
	char *ptr;
	const char *stylefilename = "styles.conf";

	int filePos = parseOptions(argc, argv);
	if ( filePos >= argc || strlen(argv[filePos]) < 3)
	{
		lwerror("Usage: %s [-v] [-s <width>x<height>] <source_wktfile> [<output_pngfile>]", argv[0]);
		return -1;
	}

	image_src = argv[filePos];

	if ( (pfile = fopen(image_src, "r")) == NULL)
	{
		perror ( image_src );
		return -1;
	}

	/* Get style */
	ptr = rindex( image_src, '/' );
	if ( ptr ) /* source image file has a slash */
	{
		size_t dirname_len = (ptr - image_src);
		stylefile_path = malloc( strlen(stylefilename) + dirname_len + 2);
		/* copy the directory name */
		memcpy(stylefile_path, image_src, dirname_len);
		sprintf(stylefile_path + dirname_len, "/%s", stylefilename);
	}
	else /* source image file has no slash, use CWD */
	{
		stylefile_path = strdup(stylefilename);
	}
	printf("reading styles from %s\n", stylefile_path);
	getStyles(stylefile_path, &styles);
	free(stylefile_path);

	if ( argc - filePos >= 2 )
	{
		filename = strdup(argv[filePos + 1]);
	}
	else
	{
		filename = strdup(image_src);
		sprintf(filename + strlen(image_src) - 3, "png" );
	}

	tmpdir = mkdtemp(tempdir_template);
	if ( NULL == tmpdir ) {
		perror ( image_src );
		exit(EXIT_FAILURE);
	}

	printf( "generating %s\n", filename );

	layerCount = 0;
	while ( fgets ( line, sizeof line, pfile ) != NULL && !isspace(*line) )
	{
		GEOMETRY_DRAW_CONTEXT ctx;
		char output[32768];
		char *ptr = output;
		char *styleName;
		int useDefaultStyle;

		initializeGeometryDrawContext(&ctx);
		ctx.tmpdir = tmpdir;

		ptr += sprintf( ptr, "convert -size %s xc:none ", imageSize );

		useDefaultStyle = getStyleName(&styleName, line);
		LWDEBUGF( 4, "%s", styleName );

		if (useDefaultStyle)
		{
			printf("   Warning: using Default style for layer %d\n", layerCount);
			lwgeom = lwgeom_from_wkt( line, LW_PARSER_CHECK_NONE );
		}
		else
			lwgeom = lwgeom_from_wkt( line+strlen(styleName)+1, LW_PARSER_CHECK_NONE );

		LWDEBUGF( 4, "geom = %s", lwgeom_to_ewkt((LWGEOM*)lwgeom) );

		ctx.style = getStyle(styles, styleName);
		if ( ! ctx.style ) {
		  lwerror("Could not find style named %s", styleName);
			free(styleName);
		  return -1;
		}
		free(styleName);

		ptr += drawGeometry( ptr, lwgeom, &ctx );

		ptr += sprintf( ptr, "-flip %s/tmp%d.png", tmpdir, layerCount );

		lwgeom_free( lwgeom );

		LWDEBUGF( 4, "%s", output );
		if (optionVerbose) {
			puts(output);
		}
		checked_system(output);

		layerCount++;
	}

	flattenLayers(filename);
	optimizeImage(filename);

	fclose(pfile);
	free(filename);
	freeStyles(&styles);

	cleanupTempDir(tmpdir);

	return 0;
}
