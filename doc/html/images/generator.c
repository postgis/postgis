/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2025 Darafei Praliaskouski <me@komzpa.net>
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
 * In order to generate a png file, GraphicsMagick or ImageMagick must be
 * installed in the user's path as system calls are invoked to "gm convert",
 * "magick convert", or the classic "convert" binary. In this manner, WKT
 * files are converted into SVG syntax and rasterized as png using their
 * command-line interfaces.
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
 * -v - show generated GraphicsMagick commands
 * -s - output dimension, if omitted defaults to 200x200
 *
 * If <output_pngfile> is omitted the output image PNG file has the
 * same name as the source file
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <sys/wait.h> /* for WEXITSTATUS */
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#ifndef X_OK
#define X_OK 0
#endif
#define access _access
#endif

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "stringbuffer.h"
#include "styles.h"

typedef struct generator_options {
	bool verbose;
	const char *image_size;
} generator_options;

/**
 * Emit the command-line synopsis expected by documentation maintainers.
 *
 * The helper keeps the main routine compact and ensures both -h and --help
 * share the exact same prose, which simplifies future updates to the option
 * set.
 */
static void
print_usage(FILE *stream, const char *progname)
{
	fprintf(stream,
		"Usage: %s [-v] [-s <width>x<height>] <source_wktfile> [<output_pngfile>]\n"
		"\n"
		"Options:\n"
		"  -v             Emit the assembled GraphicsMagick/ImageMagick command.\n"
		"  -s <WxH>       Override the output canvas size (default: 200x200).\n"
		"  -h, --help     Display this help text and exit.\n"
		"\n"
		"If <output_pngfile> is omitted the generator derives it from the source\n"
		"WKT filename.\n",
		progname);
}

typedef struct generator_job {
	generator_options options;
	const char *converter_cli;
	LAYERSTYLE *styles;
	stringbuffer_t command;
} generator_job;

static void
generator_job_init(generator_job *job, const generator_options *options)
{
	job->options = *options;
	job->converter_cli = NULL;
	job->styles = NULL;
	stringbuffer_init(&job->command);
}

static void
generator_job_reset(generator_job *job)
{
	if (job->styles)
	{
		freeStyles(&job->styles);
		job->styles = NULL;
	}

	stringbuffer_release(&job->command);
}

typedef struct draw_context_t {
	LAYERSTYLE *style;
} GEOMETRY_DRAW_CONTEXT;

static GEOMETRY_DRAW_CONTEXT
geometry_draw_context_init(void)
{
	GEOMETRY_DRAW_CONTEXT ctx;
	ctx.style = NULL;
	return ctx;
}

/**
 * Execute an external command and abort on failure.
 *
 * Several raster utilities also rely on libc's \c system() to orchestrate
 * external programs, so keeping this helper self-contained makes it trivial to
 * promote into a shared header if we ever need the same GraphicsMagick
 * pipeline while testing rasters.
 */
static void
checked_system(const char *cmd)
{
	int ret = system(cmd);

	if (ret == -1)
	{
		perror("system");
		fprintf(stderr, "Unable to execute command: %s\n", cmd);
		exit(EXIT_FAILURE);
	}

	if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0)
	{
		fprintf(stderr, "Failure return code (%d) from command: %s\n", WEXITSTATUS(ret), cmd);
		exit(EXIT_FAILURE);
	}
}

/*
 * Locate executables in PATH using liblwgeom's stringbuffer helpers so the
 * probing logic can be promoted to the raster tooling if it ever needs the
 * same GraphicsMagick/ImageMagick detection.
 */
static bool
command_exists(const char *cmd)
{
	const char *path_env;
	const char *cursor;

	if (cmd == NULL || *cmd == '\0')
		return false;

#ifdef _WIN32
	if (strchr(cmd, ':') || strchr(cmd, '\\'))
	{
		return access(cmd, X_OK) == 0;
	}
#else
	if (strchr(cmd, '/'))
	{
		return access(cmd, X_OK) == 0;
	}
#endif

	path_env = getenv("PATH");
	if (!path_env || !*path_env)
		return false;

	cursor = path_env;
	while (*cursor)
	{
		const char *sep;
		size_t dir_len;
		stringbuffer_t candidate;

		sep = strchr(cursor,
#ifdef _WIN32
			     ';'
#else
			     ':'
#endif
		);
		dir_len = sep ? (size_t)(sep - cursor) : strlen(cursor);

		stringbuffer_init(&candidate);
		if (dir_len == 0)
		{
			stringbuffer_append(&candidate, ".");
		}
		else
		{
			stringbuffer_append_len(&candidate, cursor, dir_len);
		}

		if (stringbuffer_getlength(&candidate) > 0)
		{
			char last = stringbuffer_lastchar(&candidate);
#ifdef _WIN32
			if (last != '/' && last != '\\')
				stringbuffer_append_char(&candidate, '\\');
#else
			if (last != '/')
				stringbuffer_append_char(&candidate, '/');
#endif
		}

		stringbuffer_append(&candidate, cmd);

		if (access(stringbuffer_getstring(&candidate), X_OK) == 0)
		{
			stringbuffer_release(&candidate);
			return true;
		}

#ifdef _WIN32
		stringbuffer_append(&candidate, ".exe");
		if (access(stringbuffer_getstring(&candidate), X_OK) == 0)
		{
			stringbuffer_release(&candidate);
			return true;
		}
#endif

		stringbuffer_release(&candidate);

		if (!sep)
			break;
		cursor = sep + 1;
	}

	return false;
}

/*
 * Prefer GraphicsMagick but gracefully fall back to ImageMagick 7 ("magick")
 * or the legacy "convert" binary so older documentation builds keep working.
 */
static const char *
select_converter_cli(void)
{
	const char *override = getenv("POSTGIS_DOC_CONVERTER");
	if (override && *override)
		return override;

	if (command_exists("gm"))
		return "gm convert";

	if (command_exists("magick"))
		return "magick convert";

	if (command_exists("convert"))
		return "convert";

	return NULL;
}

static char *
derive_styles_path(const char *source_path)
{
	const char *styles_basename = "styles.conf";
	const char *slash = strrchr(source_path, '/');
	char *resolved;

	if (!slash)
		return lwstrdup(styles_basename);

	{
		size_t dir_len = (size_t)(slash - source_path);
		size_t basename_len = strlen(styles_basename);
		size_t total = dir_len + 1 + basename_len + 1;

		resolved = lwalloc(total);
		if (!resolved)
			return NULL;

		memcpy(resolved, source_path, dir_len);
		resolved[dir_len] = '/';
		memcpy(resolved + dir_len + 1, styles_basename, basename_len + 1);
	}

	return resolved;
}

static char *
derive_output_path(const char *source_path, const char *override_path)
{
	size_t len;
	char *result;

	if (override_path && *override_path)
		return lwstrdup(override_path);

	len = strlen(source_path);
	result = lwstrdup(source_path);
	if (!result)
		return NULL;

	if (len >= 3)
		memcpy(result + len - 3, "png", 3);

	return result;
}

/**
 * Append a coordinate pair as "x,y" so the resulting buffer can be reused by
 * both the documentation generator and raster command builders that already
 * rely on liblwgeom's stringbuffer helpers.
 */
static void
append_coord_pair(stringbuffer_t *sb, double x, double y)
{
	stringbuffer_append_double(sb, x, 10);
	stringbuffer_append_char(sb, ',');
	stringbuffer_append_double(sb, y, 10);
}

static void
pointarrayToBuffer(stringbuffer_t *output, POINTARRAY *pa)
{
	unsigned int i;

	for (i = 0; i < pa->npoints; i++)
	{
		POINT2D pt;
		getPoint2d_p(pa, i, &pt);

		if (i)
			stringbuffer_append_char(output, ' ');
		append_coord_pair(output, pt.x, pt.y);
	}
}

/**
 * Draws a point in a POINTARRAY to a char* using GraphicsMagick SVG for styling.

 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawPointSymbol(stringbuffer_t *output, POINTARRAY *pa, unsigned int index, int size, const char *color)
{
	// short-circuit no-op
	if (size <= 0)
		return;

	POINT2D p;
	getPoint2d_p(pa, index, &p);

	stringbuffer_aprintf(output, "-fill %s -strokewidth 0 ", color);
	stringbuffer_append(output, "-draw \"circle ");
	append_coord_pair(output, p.x, p.y);
	stringbuffer_append_char(output, ' ');
	append_coord_pair(output, p.x, p.y + size);
	stringbuffer_append(output, "\" ");
}

/**
 * Draws a point in a POINTARRAY to a char* using GraphicsMagick SVG for styling.

 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawLineArrow(stringbuffer_t *output, POINTARRAY *pa, int size, int strokeWidth, const char *color)
{
	// short-circuit no-op
	if (size <= 0)
		return;
	if (pa->npoints <= 1)
		return;

	POINT2D pn;
	getPoint2d_p(pa, pa->npoints - 1, &pn);
	POINT2D pn1;
	getPoint2d_p(pa, pa->npoints - 2, &pn1);

	double dx = pn1.x - pn.x;
	double dy = pn1.y - pn.y;
	double len = sqrt(dx * dx + dy * dy);
	//-- abort if final line segment has length 0
	if (len <= 0)
		return;

	double offx = -0.5 * size * dy / len;
	double offy = 0.5 * size * dx / len;

	double p1x = pn.x + size * dx / len + offx;
	double p1y = pn.y + size * dy / len + offy;
	double p2x = pn.x + size * dx / len - offx;
	double p2y = pn.y + size * dy / len - offy;

	stringbuffer_aprintf(output, "-fill %s -strokewidth %d ", color, 2);
	stringbuffer_append(output, "-draw \"path 'M ");
	append_coord_pair(output, pn.x, pn.y);
	stringbuffer_append_char(output, ' ');
	append_coord_pair(output, p1x, p1y);
	stringbuffer_append_char(output, ' ');
	append_coord_pair(output, p2x, p2y);
	stringbuffer_append_char(output, ' ');
	append_coord_pair(output, pn.x, pn.y);
	stringbuffer_append(output, "'\" ");
}

/**
 * Serializes a LWPOINT to a char*.  This is a helper function that partially
 * writes the appropriate draw and fill commands used to generate an SVG image
 * using GraphicsMagick's "gm convert" command.

 * @param output a char reference to write the LWPOINT to
 * @param lwp a reference to a LWPOINT
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawPoint(stringbuffer_t *output, LWPOINT *lwp, GEOMETRY_DRAW_CONTEXT *ctx)
{
	LAYERSTYLE *styles = ctx->style;
	POINTARRAY *pa = lwp->point;
	POINT2D p;
	getPoint2d_p(pa, 0, &p);

	LWDEBUGF(4, "%s", "drawPoint called");
	LWDEBUGF(4, "point = %s", lwgeom_to_ewkt((LWGEOM *)lwp));

	stringbuffer_aprintf(output, "-fill %s -strokewidth 0 ", styles->pointColor);
	stringbuffer_append(output, "-draw \"circle ");
	append_coord_pair(output, p.x, p.y);
	stringbuffer_append_char(output, ' ');
	append_coord_pair(output, p.x, p.y + styles->pointSize);
	stringbuffer_append(output, "\" ");
}

/**
 * Serializes a LWLINE to a char*.  This is a helper function that partially
 * writes the appropriate draw and stroke commands used to generate an SVG image
 * using GraphicsMagick's "gm convert" command.

 * @param output a char reference to write the LWLINE to
 * @param lwl a reference to a LWLINE
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawLineString(stringbuffer_t *output, LWLINE *lwl, GEOMETRY_DRAW_CONTEXT *ctx)
{
	LAYERSTYLE *style = ctx->style;
	LWDEBUGF(4, "%s", "drawLineString called");
	LWDEBUGF(4, "line = %s", lwgeom_to_ewkt((LWGEOM *)lwl));

	stringbuffer_aprintf(output, "-fill none -stroke %s -strokewidth %d ", style->lineColor, style->lineWidth);

	stringbuffer_t path;
	stringbuffer_init(&path);
	stringbuffer_append(&path, "stroke-linecap round stroke-linejoin round path 'M ");
	pointarrayToBuffer(&path, lwl->points);
	stringbuffer_append(&path, "'");

	stringbuffer_append(output, "-draw \"");
	stringbuffer_append(output, stringbuffer_getstring(&path));
	stringbuffer_append(output, "\" ");

	stringbuffer_release(&path);

	drawPointSymbol(output, lwl->points, 0, style->lineStartSize, style->lineColor);
	drawPointSymbol(output, lwl->points, lwl->points->npoints - 1, style->lineEndSize, style->lineColor);
	drawLineArrow(output, lwl->points, style->lineArrowSize, style->lineWidth, style->lineColor);
}

/**
 * Serializes a LWPOLY to a char*.  This is a helper function that partially
 * writes the appropriate draw and fill commands used to generate an SVG image
 * using GraphicsMagick's "gm convert" command.

 * @param output a char reference to write the LWPOLY to
 * @param lwp a reference to a LWPOLY
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawPolygon(stringbuffer_t *output, LWPOLY *lwp, GEOMETRY_DRAW_CONTEXT *ctx)
{
	unsigned int i;
	LAYERSTYLE *style = ctx->style;

	LWDEBUGF(4, "%s", "drawPolygon called");
	LWDEBUGF(4, "poly = %s", lwgeom_to_ewkt((LWGEOM *)lwp));

	stringbuffer_aprintf(output,
			     "-fill %s -stroke %s -strokewidth %d ",
			     style->polygonFillColor,
			     style->polygonStrokeColor,
			     style->polygonStrokeWidth);

	stringbuffer_t path;
	stringbuffer_init(&path);
	stringbuffer_append(&path, "path '");
	for (i = 0; i < lwp->nrings; i++)
	{
		stringbuffer_append(&path, "M ");
		pointarrayToBuffer(&path, lwp->rings[i]);
		stringbuffer_append_char(&path, ' ');
	}
	stringbuffer_append(&path, "'");

	stringbuffer_append(output, "-draw \"");
	stringbuffer_append(output, stringbuffer_getstring(&path));
	stringbuffer_append(output, "\" ");

	stringbuffer_release(&path);
}

/**
 * Serializes a LWGEOM to a char*.  This is a helper function that partially
 * writes the appropriate draw, stroke, and fill commands used to generate an
 * SVG image using GraphicsMagick's "gm convert" command.

 * @param output a char reference to write the LWGEOM to
 * @param lwgeom a reference to a LWGEOM
 * @param ctx drawing context
 *
 * The drawing commands are appended directly to the supplied buffer.
 */
static void
drawGeometry(stringbuffer_t *output, const LWGEOM *lwgeom, GEOMETRY_DRAW_CONTEXT *ctx)
{
	unsigned int i;
	int type = lwgeom->type;

	switch (type)
	{
	case POINTTYPE:
		drawPoint(output, (LWPOINT *)lwgeom, ctx);
		break;
	case LINETYPE:
		drawLineString(output, (LWLINE *)lwgeom, ctx);
		break;
	case POLYGONTYPE:
		drawPolygon(output, (LWPOLY *)lwgeom, ctx);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		for (i = 0; i < ((LWCOLLECTION *)lwgeom)->ngeoms; i++)
		{
			drawGeometry(output, lwcollection_getsubgeom((LWCOLLECTION *)lwgeom, i), ctx);
		}
		break;
	}
}

/**
 * Extract an optional "style;WKT" prefix and return both pieces.  When the
 * prefix is absent the caller falls back to the "Default" block in
 * styles.conf while keeping the full line as WKT input.
 */
static void
parse_layer_line(const char *line, char **style_name, const char **wkt_literal, bool *uses_default)
{
	const char *separator = strrchr(line, ';');

	if (separator == NULL)
	{
		*style_name = lwstrdup("Default");
		if (!*style_name)
		{
			lwerror("Out of memory while reading style name");
			*wkt_literal = NULL;
			*uses_default = true;
			return;
		}
		*wkt_literal = line;
		*uses_default = true;
		return;
	}

	{
		size_t style_len = (size_t)(separator - line);
		*style_name = lwalloc(style_len + 1);
		if (!*style_name)
		{
			lwerror("Out of memory while reading style name");
			*style_name = NULL;
			*wkt_literal = NULL;
			*uses_default = true;
			return;
		}

		memcpy(*style_name, line, style_len);
		(*style_name)[style_len] = '\0';
	}

	*wkt_literal = separator + 1;
	*uses_default = false;
}

/**
 * Stream all non-empty layers from @a source into @a job's command buffer.
 * The helper mirrors the raster tooling, which prefers to marshal draw
 * commands in memory before forking external utilities.
 */
static int
append_layers(generator_job *job, FILE *source)
{
	char line[65536];
	int layer_index = 0;

	while (fgets(line, sizeof line, source) != NULL)
	{
		if (isspace((unsigned char)line[0]))
			break;

		GEOMETRY_DRAW_CONTEXT ctx = geometry_draw_context_init();
		char *style_name = NULL;
		const char *wkt_literal = NULL;
		bool uses_default_style = false;
		LWGEOM *lwgeom;

		parse_layer_line(line, &style_name, &wkt_literal, &uses_default_style);

		if (!style_name || !wkt_literal)
		{
			lwfree(style_name);
			return -1;
		}

		if (uses_default_style)
			printf("   Warning: using Default style for layer %d\n", layer_index);

		lwgeom = lwgeom_from_wkt(wkt_literal, LW_PARSER_CHECK_NONE);
		if (!lwgeom)
		{
			lwerror("Could not parse geometry for layer %d", layer_index);
			lwfree(style_name);
			return -1;
		}

		LWDEBUGF(4, "geom = %s", lwgeom_to_ewkt(lwgeom));

		ctx.style = getStyle(job->styles, style_name);
		if (!ctx.style)
		{
			lwgeom_free(lwgeom);
			lwerror("Could not find style named %s", style_name);
			lwfree(style_name);
			return -1;
		}

		drawGeometry(&job->command, lwgeom, &ctx);

		lwgeom_free(lwgeom);
		lwfree(style_name);
		layer_index++;
	}

	return layer_index;
}

/**
 * Parse command-line switches shared with the historical ImageMagick driver.
 * The generator continues to accept the same options so existing Makefile
 * rules and scripts do not need updates.
 */
static int
parse_options(int argc, const char *argv[], generator_options *opts, bool *show_help)
{
	int arg_pos = 1;

	opts->verbose = false;
	opts->image_size = "200x200";
	*show_help = false;

	if (argc <= 1)
		return -1;

	while (arg_pos < argc && argv[arg_pos][0] == '-')
	{
		if (strcmp(argv[arg_pos], "-h") == 0 || strcmp(argv[arg_pos], "--help") == 0)
		{
			*show_help = true;
			return argc;
		}

		if (strncmp(argv[arg_pos], "-v", 2) == 0)
		{
			opts->verbose = true;
			arg_pos++;
			continue;
		}

		if (strncmp(argv[arg_pos], "-s", 2) == 0)
		{
			if (++arg_pos >= argc)
				return -1;
			opts->image_size = argv[arg_pos];
			arg_pos++;
			continue;
		}

		return -1;
	}

	return arg_pos;
}

static int
generator_render(const generator_options *options, const char *source_path, const char *target_override)
{
	generator_job job;
	FILE *source = NULL;
	char *styles_path = NULL;
	char *target_path = NULL;
	int rc = -1;
	const char *converter_cli;

	generator_job_init(&job, options);

	converter_cli = select_converter_cli();
	if (!converter_cli)
	{
		lwerror(
		    "Could not find GraphicsMagick or ImageMagick executables (gm, magick, convert). Set POSTGIS_DOC_CONVERTER to the desired command.");
		goto cleanup;
	}
	job.converter_cli = converter_cli;

	source = fopen(source_path, "r");
	if (!source)
	{
		perror(source_path);
		goto cleanup;
	}

	styles_path = derive_styles_path(source_path);
	if (!styles_path)
	{
		lwerror("Out of memory while resolving styles");
		goto cleanup;
	}

	printf("reading styles from %s\n", styles_path);
	getStyles(styles_path, &job.styles);

	target_path = derive_output_path(source_path, target_override);
	if (!target_path)
	{
		lwerror("Out of memory while preparing output filename");
		goto cleanup;
	}

	printf("generating %s\n", target_path);

	stringbuffer_aprintf(&job.command, "%s -size %s xc:none ", job.converter_cli, job.options.image_size);

	if (append_layers(&job, source) < 0)
		goto cleanup;

	stringbuffer_append(&job.command, "-flip -depth 8 ");
	stringbuffer_append(&job.command, target_path);

	if (job.options.verbose)
		puts(stringbuffer_getstring(&job.command));

	checked_system(stringbuffer_getstring(&job.command));

	rc = 0;

cleanup:
	if (source)
		fclose(source);
	if (styles_path)
		lwfree(styles_path);
	if (target_path)
		lwfree(target_path);
	generator_job_reset(&job);
	return rc;
}

/**
 * Main Application.
 */
int
main(int argc, const char *argv[])
{
	generator_options options;
	const char *image_src;
	const char *target_override = NULL;
	bool show_help;

	int filePos = parse_options(argc, argv, &options, &show_help);
	if (show_help)
	{
		print_usage(stdout, argv[0]);
		return 0;
	}

	if (filePos < 0 || filePos >= argc || strlen(argv[filePos]) < 3)
	{
		lwerror("Usage: %s [-v] [-s <width>x<height>] <source_wktfile> [<output_pngfile>]", argv[0]);
		return -1;
	}

	image_src = argv[filePos];

	if (argc - filePos >= 2)
		target_override = argv[filePos + 1];

	if (generator_render(&options, image_src, target_override) != 0)
		return -1;

	return 0;
}
