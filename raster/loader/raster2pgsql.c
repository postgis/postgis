/*
 *
 * PostGIS raster loader
 *
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2011 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "raster2pgsql.h"
#include "gdal_vrt.h"
#include "ogr_srs_api.h"
#include <assert.h>
#include <stdarg.h>

#define xstr(s) str(s)
#define str(s) #s

static void
loader_rt_error_handler(const char *fmt, va_list ap) __attribute__ ((format (printf, 1, 0)));

static void
loader_rt_error_handler(const char *fmt, va_list ap) {
	static const char *label = "ERROR: ";
	char newfmt[1024] = {0};
	snprintf(newfmt, 1024, "%s%s\n", label, fmt);
	newfmt[1023] = '\0';
	vfprintf(stderr, newfmt, ap);
	va_end(ap);
}

static void
loader_rt_warning_handler(const char *fmt, va_list ap) __attribute__ ((format (printf, 1, 0)));

static void
loader_rt_warning_handler(const char *fmt, va_list ap) {
	static const char *label = "WARNING: ";
	char newfmt[1024] = {0};
	snprintf(newfmt, 1024, "%s%s\n", label, fmt);
	newfmt[1023] = '\0';
	vfprintf(stderr, newfmt, ap);
	va_end(ap);
}

static void
loader_rt_info_handler(const char *fmt, va_list ap) __attribute__ ((format (printf, 1, 0)));

static void
loader_rt_info_handler(const char *fmt, va_list ap) {
	static const char *label = "INFO: ";
	char newfmt[1024] = {0};
	snprintf(newfmt, 1024, "%s%s\n", label, fmt);
	newfmt[1023] = '\0';
	vfprintf(stderr, newfmt, ap);
	va_end(ap);
}

static void
rt_init_allocators(void) {
	rt_set_handlers(
		default_rt_allocator,
		default_rt_reallocator,
		default_rt_deallocator,
		loader_rt_error_handler,
		loader_rt_info_handler,
		loader_rt_warning_handler
	);
}

static char *
rtloader_alloc_sprintf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

static char *
rtloader_alloc_sprintf(const char *fmt, ...)
{
	int len;
	char *result;
	va_list ap;
	va_list ap2;

	va_start(ap, fmt);
	va_copy(ap2, ap);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (len < 0) {
		va_end(ap2);
		return NULL;
	}

	result = rtalloc((size_t)len + 1);
	if (result == NULL) {
		va_end(ap2);
		return NULL;
	}

	vsnprintf(result, (size_t)len + 1, fmt, ap2);
	va_end(ap2);
	return result;
}

static void
raster_destroy(rt_raster raster) {
	uint16_t i;
	uint16_t nbands = rt_raster_get_num_bands(raster);
	for (i = 0; i < nbands; i++) {
		rt_band band = rt_raster_get_band(raster, i);
		if (band == NULL) continue;

		if (!rt_band_is_offline(band) && !rt_band_get_ownsdata_flag(band)) {
			void* mem = rt_band_get_data(band);
			if (mem) rtdealloc(mem);
		}
		rt_band_destroy(band);
	}
	rt_raster_destroy(raster);
}

static int
array_range(int min, int max, int step, int **range, uint32_t *len) {
	int i = 0;
	int j = 0;

	step = abs(step);
	*len = (uint32_t) ((abs(max - min) + 1 + (step / 2)) / step);
	*range = rtalloc(sizeof(int) * *len);

	if (min < max) {
		for (i = min, j = 0; i <= max; i += step, j++)
			(*range)[j] = i;
	}
	else if (max < min) {
		if (step > 0) step *= -1;
		for (i = min, j = 0; i >= max; i += step, j++)
			(*range)[j] = i;
	}
	else if (min == max) {
		(*range)[0] = min;
	}
	else {
		*len = 0;
		*range = NULL;
		return 0;
	}

	return 1;
}

/* string replacement function taken from
 * http://ubuntuforums.org/showthread.php?s=aa6f015109fd7e4c7e30d2fd8b717497&t=141670&page=3
 */
/* ---------------------------------------------------------------------------
  Name       : replace - Search & replace a substring by another one.
  Creation   : Thierry Husson, Sept 2010
  Parameters :
      str    : Big string where we search
      oldstr : Substring we are looking for
      newstr : Substring we want to replace with
      count  : Optional pointer to int (input / output value). NULL to ignore.
               Input:  Maximum replacements to be done. NULL or < 1 to do all.
               Output: Number of replacements done or -1 if not enough memory.
  Returns    : Pointer to the new string or NULL if error.
  Notes      :
     - Case sensitive - Otherwise, replace functions "strstr" by "strcasestr"
     - Always allocates memory for the result.
--------------------------------------------------------------------------- */
static char*
strreplace(
	const char *str,
	const char *oldstr, const char *newstr,
	int *count
) {
	const char *tmp = str;
	char *result;
	int found = 0;
	int length, reslen;
	int oldlen = strlen(oldstr);
	int newlen = strlen(newstr);
	int limit = (count != NULL && *count > 0) ? *count : -1;

	tmp = str;
	while ((tmp = strstr(tmp, oldstr)) != NULL && found != limit)
		found++, tmp += oldlen;

	length = (int)strlen(str) + found * (newlen - oldlen);
	if ((result = (char *) rtalloc(length + 1)) == NULL) {
		rterror(_("strreplace: Not enough memory"));
		found = -1;
	}
	else {
		tmp = str;
		limit = found; /* Countdown */
		reslen = 0; /* length of current result */

		/* Replace each old string found with new string  */
		while ((limit-- > 0) && (tmp = strstr(tmp, oldstr)) != NULL) {
			length = (tmp - str); /* Number of chars to keep intouched */
			strncpy(result + reslen, str, length); /* Original part keeped */
			strcpy(result + (reslen += length), newstr); /* Insert new string */

			reslen += newlen;
			tmp += oldlen;
			str = tmp;
		}
		strcpy(result + reslen, str); /* Copies last part and ending null char */
	}

	if (count != NULL) *count = found;
	return result;
}

static char *
strtolower(char * str) {
	int j;

	for (j = strlen(str) - 1; j >= 0; j--)
		str[j] = tolower(str[j]);

	return str;
}

/* split a string based on a delimiter */
static char**
strsplit(const char *str, const char *delimiter, uint32_t *n) {
	char *tmp = NULL;
	char **rtn = NULL;
	char *token = NULL;

	*n = 0;
	if (!str)
		return NULL;

	/* copy str to tmp as strtok will mangle the string */
	tmp = rtalloc(sizeof(char) * (strlen(str) + 1));
	if (NULL == tmp) {
		rterror(_("strsplit: Not enough memory"));
		return NULL;
	}
	strcpy(tmp, str);

	if (!strlen(tmp) || !delimiter || !strlen(delimiter)) {
		*n = 1;
		rtn = (char **) rtalloc(*n * sizeof(char *));
		if (NULL == rtn) {
			rterror(_("strsplit: Not enough memory"));
			return NULL;
		}
		rtn[0] = (char *) rtalloc(sizeof(char) * (strlen(tmp) + 1));
		if (NULL == rtn[0]) {
			rterror(_("strsplit: Not enough memory"));
			return NULL;
		}
		strcpy(rtn[0], tmp);
		rtdealloc(tmp);
		return rtn;
	}

	token = strtok(tmp, delimiter);
	while (token != NULL) {
		if (*n < 1) {
			rtn = (char **) rtalloc(sizeof(char *));
		}
		else {
			rtn = (char **) rtrealloc(rtn, (*n + 1) * sizeof(char *));
		}
		if (NULL == rtn) {
			rterror(_("strsplit: Not enough memory"));
			return NULL;
		}

		rtn[*n] = NULL;
		rtn[*n] = (char *) rtalloc(sizeof(char) * (strlen(token) + 1));
		if (NULL == rtn[*n]) {
			rterror(_("strsplit: Not enough memory"));
			return NULL;
		}

		strcpy(rtn[*n], token);
		*n = *n + 1;

		token = strtok(NULL, delimiter);
	}

	rtdealloc(tmp);
	return rtn;
}

static char*
trim(const char *input) {
	char *rtn;
	char *ptr;
	uint32_t offset = 0;
	size_t len = 0;

	if (!input)
		return NULL;
	else if (!*input)
		return (char *) input;

	/* trim left */
	while (isspace(*input))
		input++;

	/* trim right */
	ptr = ((char *) input) + strlen(input);
	while (isspace(*--ptr))
		offset++;

	len = strlen(input) - offset + 1;
	rtn = rtalloc(sizeof(char) * len);
	if (NULL == rtn) {
		rterror(_("trim: Not enough memory"));
		return NULL;
	}
	strncpy(rtn, input, len);

	return rtn;
}

static char*
chartrim(const char *input, char *remove) {
	char *rtn = NULL;
	char *ptr = NULL;
	uint32_t offset = 0;
	size_t len = 0;

	if (!input)
		return NULL;
	else if (!*input)
		return (char *) input;

	/* trim left */
	while (strchr(remove, *input) != NULL)
		input++;

	/* trim right */
	ptr = ((char *) input) + strlen(input);
	while (strchr(remove, *--ptr) != NULL)
		offset++;

	len = strlen(input) - offset + 1;
	rtn = rtalloc(sizeof(char) * len);
	if (NULL == rtn) {
		rterror(_("chartrim: Not enough memory"));
		return NULL;
	}
	strncpy(rtn, input, len);
	rtn[strlen(input) - offset] = '\0';

	return rtn;
}

static int
option_matches(const char *arg, const char *shortopt, const char *longopt)
{
	size_t longopt_len;

	if (CSEQUAL(arg, shortopt) || CSEQUAL(arg, longopt))
		return 1;

	longopt_len = strlen(longopt);
	return strncmp(arg, longopt, longopt_len) == 0 && arg[longopt_len] == '=';
}

static char *
option_value(int argc, char **argv, int *argit, const char *longopt)
{
	const size_t longopt_len = strlen(longopt);
	char *arg = argv[*argit];

	if (strncmp(arg, longopt, longopt_len) == 0 && arg[longopt_len] == '=')
		return arg + longopt_len + 1;

	if (*argit < argc - 1)
		return argv[++(*argit)];

	return NULL;
}

static void
usage() {
	printf(_("RELEASE: %s GDAL_VERSION=%d (%s)\n"), POSTGIS_LIB_VERSION, POSTGIS_GDAL_VERSION, xstr(POSTGIS_REVISION));
	printf(
	    _("USAGE: raster2pgsql [<options>] <raster>[ <raster>[ ...]] [[<schema>.]<table>]\n"
	      "  Multiple rasters can also be specified using wildcards (*,?).\n"
	      "\n"
	      "OPTIONS:\n"
	      "  Long options with values also accept --option=value.\n"));
	printf(_("  -s, --srid [<from>:]<srid> Set the SRID field. Defaults to %d.\n"
		 "     Optionally reprojects from given SRID (cannot be used with -Y).\n"
		 "     Raster's metadata will be checked to determine an appropriate SRID.\n"
		 "     Metadata lookup is also used when %d is provided as from or target.\n"),
	       SRID_UNKNOWN,
	       SRID_UNKNOWN);
	printf(
	    _("  -b, --band <band> Index (1-based) of band to extract from raster. For more\n"
	      "      than one band index, separate with comma (,). Ranges can be\n"
	      "      defined by separating with dash (-). If unspecified, all bands\n"
	      "      of raster will be extracted.\n"));
	printf(
	    _("  -t, --tile-size <tile size> Cut raster into tiles to be inserted one per\n"
	      "      table row. <tile size> is expressed as WIDTHxHEIGHT.\n"
	      "      <tile size> can also be \"auto\" to allow the loader to compute\n"
	      "      an appropriate tile size using the first raster and applied to\n"
	      "      all rasters.\n"));
	printf(
	    _("  -P, --pad Pad right-most and bottom-most tiles to guarantee that all tiles\n"
	      "     have the same width and height.\n"));
	printf(
	    _("  -R, --register Register the raster as an out-of-db (filesystem) raster. Provided\n"
	      "      raster should have absolute path to the file\n"));
	printf(
	    _(" (-d|a|c|p) These are mutually exclusive options:\n"
	      "     -d  Drops the table, then recreates it and populates\n"
	      "         it with current raster data.\n"
	      "     -a  Appends raster into current table, must be\n"
	      "         exactly the same table schema.\n"
	      "     -c  Creates a new table and populates it, this is the\n"
	      "         default if you do not specify any options.\n"
	      "     -p  Prepare mode, only creates the table.\n"));
	printf(
	    _("  --if-not-exists  Use IF NOT EXISTS for table creation in -c and -p\n"
	      "     modes. With -I/--create-index, also use IF NOT EXISTS for index\n"
	      "     creation.\n"
	      "     Append mode requires an explicit creation action.\n"));
	printf(
	    _("  --drop-table  Drop the target table before other actions.\n"
	      "      With no mode specified, the default create/load actions still apply.\n"
	      "  --create-table  Create the target table.\n"
	      "  --load-data  Load raster data into the target table.\n"
	      "  --create-index  Create a GIST spatial index on the raster column\n"
	      "      at the end of this raster2pgsql run. With repeated -a append\n"
	      "      runs, create the index on the final run or after loading; add\n"
	      "      --if-not-exists to make reruns tolerate an existing index.\n"));
	printf(_("  -f, --raster-column <column> Specify the name of the raster column\n"));
	printf(_("  -F, --filename Add a column with the filename of the raster.\n"));
	printf(_("  -n, --filename-column <column> Specify the name of the filename column. Implies -F.\n"));
	printf(
	    _("  -l, --overview-factor <overview factor> Create overview of the raster. For more than\n"
	      "      one factor, separate with comma(,). Overview table name follows\n"
	      "      the pattern o_<overview factor>_<table>. Created overview is\n"
	      "      stored in the database and is not affected by -R.\n"));
	printf(_("  -q, --quote Wrap PostgreSQL identifiers in quotes.\n"));
	printf(_("  -I  Alias for --create-index.\n"));
	printf(
	    _("  --add-constraints  Set the standard set of constraints on the\n"
	      "      raster column after the rasters are loaded. Some constraints may\n"
	      "      fail if one or more rasters violate the constraint.\n"
	      "  --vacuum  Run VACUUM on the table of the raster column.\n"
	      "  --analyze  Run ANALYZE on the table of the raster column.\n"
	      "  --no-transaction  Execute statements without a transaction.\n"));
	printf(_(
		"  -M  Run VACUUM ANALYZE on the table of the raster column. Most\n"
		"      useful when appending raster to existing table with -a.\n"
	));
	printf(
	    _("  -C  Alias for --add-constraints.\n"
	      "  -x, --no-extent Disable setting the max extent constraint. Only applied if\n"
	      "      -C/--add-constraints is also used.\n"
	      "  -r, --regular-blocking Set the constraints (spatially unique and coverage tile) for\n"
	      "      regular blocking. Only applied if -C/--add-constraints is also used.\n"));
	printf(
	    _("  -T, --tablespace <tablespace> Specify the tablespace for the new table.\n"
	      "      Note that indices (including the primary key) will still use\n"
	      "      the default tablespace unless the -X flag is also used.\n"));
	printf(
	    _("  -X, --index-tablespace <tablespace> Specify the tablespace for the table's new index.\n"
	      "      This applies to the primary key and the spatial index if\n"
	      "      the -I flag is used.\n"));
	printf(_("  -N, --nodata <nodata> NODATA value to use on bands without a NODATA value.\n"));
	printf(
	    _("  -k, --skip-nodata-check Keep empty tiles by skipping NODATA value checks for each raster band. \n"));
	printf(
	    _("  -E, --endian <endian> Control endianness of generated binary output of\n"
	      "      raster. Use 0 for XDR and 1 for NDR (default). Only NDR\n"
	      "      is supported at this time.\n"));
	printf(
	    _("  -V, --wkb-version <version> Specify version of output WKB format. Default\n"
	      "      is 0. Only 0 is supported at this time.\n"));
	printf(_("  -e, --no-transaction Execute each statement individually, do not use a transaction.\n"));
	printf(
	    _("  -Y, --copy [<max_rows_per_copy>] Use COPY statements instead of INSERT statements. \n"
	      "    Optionally specify <max_rows_per_copy>; default 50 when not specified. \n"));

	printf(_("  -G, --gdal-formats Print the supported GDAL raster formats.\n"));
	printf(_("  -?, --help Display this help screen.\n"));
}

static void
calc_tile_size(uint32_t dimX, uint32_t dimY, int *tileX, int *tileY)
{
	uint32_t min_tile_size = 30;
	uint32_t max_tile_size = 300;

	for (uint8_t current_dimension = 0; current_dimension <= 1; current_dimension++)
	{
		uint32_t img_size = (current_dimension == 0) ? dimX : dimY;
		uint32_t best_gap = max_tile_size;
		uint32_t best_size = img_size;

		if (img_size > max_tile_size)
		{
			for (uint32_t tile_size = max_tile_size; tile_size >= min_tile_size; tile_size--)
			{
				uint32_t gap = img_size % tile_size;
				if (gap < best_gap)
				{
					best_gap = gap;
					best_size = tile_size;
				}
			}
		}

		if (current_dimension == 0)
			*tileX = best_size;
		else
			*tileY = best_size;
	}
}

static void
init_rastinfo(RASTERINFO *info) {
	info->srid = SRID_UNKNOWN;
	info->srs = NULL;
	memset(info->dim, 0, sizeof(uint32_t) * 2);
	info->nband_count = 0;
	info->nband = NULL;
	info->gdalbandtype = NULL;
	info->bandtype = NULL;
	info->hasnodata = NULL;
	info->nodataval = NULL;
	memset(info->gt, 0, sizeof(double) * 6);
	memset(info->tile_size, 0, sizeof(int) * 2);
}

static void
rtdealloc_rastinfo(RASTERINFO *info) {
	if (info->srs != NULL)
		rtdealloc(info->srs);
	if (info->nband_count > 0 && info->nband != NULL)
		rtdealloc(info->nband);
	if (info->gdalbandtype != NULL)
		rtdealloc(info->gdalbandtype);
	if (info->bandtype != NULL)
		rtdealloc(info->bandtype);
	if (info->hasnodata != NULL)
		rtdealloc(info->hasnodata);
	if (info->nodataval != NULL)
		rtdealloc(info->nodataval);
}

static int
copy_rastinfo(RASTERINFO *dst, RASTERINFO *src) {
	if (src->srs != NULL) {
		dst->srs = rtalloc(sizeof(char) * (strlen(src->srs) + 1));
		if (dst->srs == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		strcpy(dst->srs, src->srs);
	}
	memcpy(dst->dim, src->dim, sizeof(uint32_t) * 2);
	dst->nband_count = src->nband_count;
	if (src->nband_count && src->nband != NULL) {
		dst->nband = rtalloc(sizeof(int) * src->nband_count);
		if (dst->nband == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		memcpy(dst->nband, src->nband, sizeof(int) * src->nband_count);
	}
	if (src->gdalbandtype != NULL) {
		dst->gdalbandtype = rtalloc(sizeof(GDALDataType) * src->nband_count);
		if (dst->gdalbandtype == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		memcpy(dst->gdalbandtype, src->gdalbandtype, sizeof(GDALDataType) * src->nband_count);
	}
	if (src->bandtype != NULL) {
		dst->bandtype = rtalloc(sizeof(rt_pixtype) * src->nband_count);
		if (dst->bandtype == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		memcpy(dst->bandtype, src->bandtype, sizeof(rt_pixtype) * src->nband_count);
	}
	if (src->hasnodata != NULL) {
		dst->hasnodata = rtalloc(sizeof(int) * src->nband_count);
		if (dst->hasnodata == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		memcpy(dst->hasnodata, src->hasnodata, sizeof(int) * src->nband_count);
	}
	if (src->nodataval != NULL) {
		dst->nodataval = rtalloc(sizeof(double) * src->nband_count);
		if (dst->nodataval == NULL) {
			rterror(_("copy_rastinfo: Not enough memory"));
			return 0;
		}
		memcpy(dst->nodataval, src->nodataval, sizeof(double) * src->nband_count);
	}
	memcpy(dst->gt, src->gt, sizeof(double) * 6);
	memcpy(dst->tile_size, src->tile_size, sizeof(int) * 2);

	return 1;
}

static void
diff_rastinfo(RASTERINFO *x, RASTERINFO *ref) {
	static uint8_t msg[6] = {0};
	uint32_t i = 0;

	/* # of bands */
	if (
		!msg[0] &&
		x->nband_count != ref->nband_count
	) {
		rtwarn(_("Different number of bands found in the set of rasters being converted to PostGIS raster"));
		msg[0]++;
	}

	/* pixel types */
	if (!msg[1]) {
		for (i = 0; i < ref->nband_count; i++) {
			if (x->bandtype[i] != ref->bandtype[i]) {
				rtwarn(_("Different pixel types found for band %d in the set of rasters being converted to PostGIS raster"), ref->nband[i]);
				msg[1]++;
			}
		}
	}

	/* hasnodata */
	if (!msg[2]) {
		for (i = 0; i < ref->nband_count; i++) {
			if (x->hasnodata[i] != ref->hasnodata[i]) {
				rtwarn(_("Different hasnodata flags found for band %d in the set of rasters being converted to PostGIS raster"), ref->nband[i]);
				msg[2]++;
			}
		}
	}

	/* nodataval */
	if (!msg[3]) {
		for (i = 0; i < ref->nband_count; i++) {
			if (!x->hasnodata[i] && !ref->hasnodata[i]) continue;
			if (x->hasnodata[i] != ref->hasnodata[i]) {
				rtwarn(_("Different NODATA values found for band %d in the set of rasters being converted to PostGIS raster"), ref->nband[i]);
				msg[3]++;
			}
		}
	}

	/* alignment */
	if (!msg[4]) {
		rt_raster rx = NULL;
		rt_raster rref = NULL;
		int err;
		int aligned;

		if (
			(rx = rt_raster_new(1, 1)) == NULL ||
			(rref = rt_raster_new(1, 1)) == NULL
		) {
			rterror(_("diff_rastinfo: Could not allocate memory for raster alignment test"));
			if (rx != NULL) rt_raster_destroy(rx);
			if (rref != NULL) rt_raster_destroy(rref);
			return;
		}

		rt_raster_set_geotransform_matrix(rx, x->gt);
		rt_raster_set_geotransform_matrix(rref, ref->gt);

		err = rt_raster_same_alignment(rx, rref, &aligned, NULL);
		rt_raster_destroy(rx);
		rt_raster_destroy(rref);
		if (err != ES_NONE) {
			rterror(_("diff_rastinfo: Could not run raster alignment test"));
			return;
		}

		if (!aligned) {
			rtwarn(_("Raster with different alignment found in the set of rasters being converted to PostGIS raster"));
			msg[4]++;
		}
	}

	/* tile size */
	if (!msg[5]) {
		for (i = 0; i < 2; i++) {
			if (x->tile_size[i] != ref->tile_size[i]) {
				rtwarn(_("Different tile sizes found in the set of rasters being converted to PostGIS raster"));
				msg[5]++;
				break;
			}
		}
	}
}

static void
init_config(RTLOADERCFG *config) {
	config->rt_file_count = 0;
	config->rt_file = NULL;
	config->rt_filename = NULL;
	config->schema = NULL;
	config->table = NULL;
	config->raster_column = NULL;
	config->file_column = 0;
	config->file_column_name = NULL;
	config->overview_count = 0;
	config->overview = NULL;
	config->overview_table = NULL;
	config->quoteident = 0;
	config->srid = config->out_srid = SRID_UNKNOWN;
	config->nband = NULL;
	config->nband_count = 0;
	memset(config->tile_size, 0, sizeof(int) * 2);
	config->pad_tile = 0;
	config->outdb = 0;
	config->opt = 'c';
	memset(&config->actions, 0, sizeof(config->actions));
	config->actions.mode = 'c';
	config->actions.create_table = LOADER_CREATE_ALWAYS;
	config->actions.load_data = 1;
	memset(&config->plan, 0, sizeof(config->plan));
	config->max_extent = 1;
	config->regular_blocking = 0;
	config->tablespace = NULL;
	config->idx_tablespace = NULL;
	config->hasnodata = 0;
	config->nodataval = 0;
	config->skip_nodataval_check = 0;
	config->endian = 1;
	config->version = 0;
	config->transaction = 1;
	config->copy_statements = 0;
	config->max_tiles_per_copy = 50;
}

static void rtdealloc_config(RTLOADERCFG *config);

static void
exit_config_error(RTLOADERCFG *config)
{
	rtdealloc_config(config);
	exit(1);
}

static int
apply_action_presets(RTLOADERCFG *config)
{
	LoaderActionOptions *actions = &config->actions;

	memset(&config->plan, 0, sizeof(config->plan));

	switch (actions->mode)
	{
	case 'd':
		config->plan.drop_table = 1;
		config->plan.create_table = LOADER_CREATE_ALWAYS;
		config->plan.load_data = 1;
		break;
	case 'a':
		config->plan.load_data = 1;
		break;
	case 'c':
		config->plan.create_table = LOADER_CREATE_ALWAYS;
		config->plan.load_data = 1;
		break;
	case 'p':
		config->plan.create_table = LOADER_CREATE_ALWAYS;
		break;
	default:
		rterror(_("Unknown loader operation: -%c"), actions->mode);
		return 0;
	}

	if (actions->drop_table)
		config->plan.drop_table = 1;
	if (actions->create_table_set)
		config->plan.create_table = actions->create_table;
	if (actions->load_data_set)
		config->plan.load_data = actions->load_data;
	if (actions->create_index_set)
		config->plan.create_index = actions->create_index;
	if (actions->add_constraints)
		config->plan.add_constraints = 1;
	config->plan.vacuum = actions->vacuum;
	config->plan.analyze = actions->analyze;

	if (config->plan.drop_table && config->plan.load_data && config->plan.create_table == LOADER_CREATE_NONE)
	{
		rterror(_("--drop-table with load data requires a table creation action"));
		return 0;
	}

	if (actions->if_not_exists)
	{
		if (config->plan.create_table == LOADER_CREATE_NONE && config->plan.create_index == LOADER_CREATE_NONE)
		{
			rterror(_("--if-not-exists requires a table or index creation action"));
			return 0;
		}

		if (config->plan.create_table == LOADER_CREATE_ALWAYS)
			config->plan.create_table = LOADER_CREATE_IF_NOT_EXISTS;
		if (config->plan.create_index == LOADER_CREATE_ALWAYS)
			config->plan.create_index = LOADER_CREATE_IF_NOT_EXISTS;
	}

	return 1;
}

static void
rtdealloc_config(RTLOADERCFG *config) {
	int i = 0;
	if (config->rt_file_count) {
		for (i = config->rt_file_count - 1; i >= 0; i--) {
			rtdealloc(config->rt_file[i]);
			if (config->rt_filename)
				rtdealloc(config->rt_filename[i]);
		}
		rtdealloc(config->rt_file);
		if (config->rt_filename)
			rtdealloc(config->rt_filename);
	}
	if (config->schema != NULL)
		rtdealloc(config->schema);
	if (config->table != NULL)
		rtdealloc(config->table);
	if (config->raster_column != NULL)
		rtdealloc(config->raster_column);
	if (config->file_column_name != NULL)
		rtdealloc(config->file_column_name);
	if (config->overview_count > 0) {
		if (config->overview != NULL)
			rtdealloc(config->overview);
		if (config->overview_table != NULL) {
			for (i = config->overview_count - 1; i >= 0; i--)
				rtdealloc(config->overview_table[i]);
			rtdealloc(config->overview_table);
		}
	}
	if (config->nband_count > 0 && config->nband != NULL)
		rtdealloc(config->nband);
	if (config->tablespace != NULL)
		rtdealloc(config->tablespace);
	if (config->idx_tablespace != NULL)
		rtdealloc(config->idx_tablespace);

	rtdealloc(config);
}

static void
init_stringbuffer(STRINGBUFFER *buffer) {
	buffer->line = NULL;
	buffer->length = 0;
}

static void
rtdealloc_stringbuffer(STRINGBUFFER *buffer, int freebuffer) {
	if (buffer->length) {
		uint32_t i = 0;
		for (i = 0; i < buffer->length; i++) {
			if (buffer->line[i] != NULL)
				rtdealloc(buffer->line[i]);
		}
		rtdealloc(buffer->line);
	}
	buffer->line = NULL;
	buffer->length = 0;

	if (freebuffer)
		rtdealloc(buffer);
}

static void
dump_stringbuffer(STRINGBUFFER *buffer) {
	uint32_t i = 0;

	for (i = 0; i < buffer->length; i++) {
		printf("%s\n", buffer->line[i]);
	}
}

static void
flush_stringbuffer(STRINGBUFFER *buffer) {
	dump_stringbuffer(buffer);
	rtdealloc_stringbuffer(buffer, 0);
}

/* Takes ownership of the passed string */
static int
append_stringbuffer(STRINGBUFFER *buffer, const char *str) {
	buffer->length++;

	buffer->line = rtrealloc(buffer->line, sizeof(char *) * buffer->length);
	if (buffer->line == NULL) {
		rterror(_("append_stringbuffer: Could not allocate memory for appending string to buffer"));
		return 0;
	}

	buffer->line[buffer->length - 1] = (char *) str;

	return 1;
}

static int
append_sql_to_buffer(STRINGBUFFER *buffer, const char *str) {
	if (buffer->length > 9)
		flush_stringbuffer(buffer);

	return append_stringbuffer(buffer, str);
}

static int
copy_from(const char *schema, const char *table, const char *column,
          const char *filename, const char *file_column_name,
          STRINGBUFFER *buffer)
{
	char *sql = NULL;

	assert(table != NULL);
	assert(column != NULL);

	sql = rtloader_alloc_sprintf("COPY %s%s (%s%s%s) FROM stdin;",
		(schema != NULL ? schema : ""),
		table,
		column,
		(filename != NULL ? "," : ""),
		(filename != NULL ? file_column_name : "")
	);
	if (sql == NULL) {
		rterror(_("copy_from: Could not allocate memory for COPY statement"));
		return 0;
	}

	append_sql_to_buffer(buffer, sql);
	sql = NULL;

	return 1;
}

static int
copy_from_end(STRINGBUFFER *buffer)
{
	/* end of data */
	append_sql_to_buffer(buffer, strdup("\\."));

	return 1;
}

static int
insert_records(
	const char *schema, const char *table, const char *column,
	const char *filename, const char *file_column_name,
	int copy_statements, int out_srid,
	STRINGBUFFER *tileset, STRINGBUFFER *buffer
) {
	char *fn = NULL;
	char *sql = NULL;
	uint32_t x = 0;

	assert(table != NULL);
	assert(column != NULL);

	/* COPY statements */
	if (copy_statements) {

    if (!copy_from(
      schema, table, column,
      (file_column_name ? filename : NULL), file_column_name,
      buffer
    )) {
      rterror(_("insert_records: Could not add COPY statement to string buffer"));
      return 0;
    }


		/* escape tabs in filename */
		if (filename != NULL)
			fn = strreplace(filename, "\t", "\\t", NULL);

		/* rows */
		for (x = 0; x < tileset->length; x++) {
			sql = rtloader_alloc_sprintf("%s%s%s",
				tileset->line[x],
				(filename != NULL ? "\t" : ""),
				(filename != NULL ? fn : "")
			);
			if (sql == NULL) {
				rterror(_("insert_records: Could not allocate memory for COPY statement"));
				return 0;
			}

			append_sql_to_buffer(buffer, sql);
			sql = NULL;
		}

    if (!copy_from_end(buffer)) {
      rterror(_("process_rasters: Could not add COPY end statement to string buffer"));
      return 0;
    }

	}
	/* INSERT statements */
	else {
		/* escape single-quotes in filename */
		if (filename != NULL)
			fn = strreplace(filename, "'", "''", NULL);

		for (x = 0; x < tileset->length; x++) {
			if (out_srid != SRID_UNKNOWN && filename != NULL) {
				sql = rtloader_alloc_sprintf(
					"INSERT INTO %s%s (%s,%s) VALUES (ST_Transform('%s'::raster, %d),'%s');",
					(schema != NULL ? schema : ""), table, column,
					file_column_name, tileset->line[x], out_srid, fn);
			}
			else if (out_srid != SRID_UNKNOWN) {
				sql = rtloader_alloc_sprintf(
					"INSERT INTO %s%s (%s) VALUES (ST_Transform('%s'::raster, %d));",
					(schema != NULL ? schema : ""), table, column,
					tileset->line[x], out_srid);
			}
			else if (filename != NULL) {
				sql = rtloader_alloc_sprintf(
					"INSERT INTO %s%s (%s,%s) VALUES ('%s'::raster,'%s');",
					(schema != NULL ? schema : ""), table, column,
					file_column_name, tileset->line[x], fn);
			}
			else {
				sql = rtloader_alloc_sprintf(
					"INSERT INTO %s%s (%s) VALUES ('%s'::raster);",
					(schema != NULL ? schema : ""), table, column,
					tileset->line[x]);
			}
			if (sql == NULL) {
				rterror(_("insert_records: Could not allocate memory for INSERT statement"));
				return 0;
			}

			append_sql_to_buffer(buffer, sql);
			sql = NULL;
		}
	}

	if (fn != NULL) rtdealloc(fn);
	return 1;
}

static int
drop_table(const char *schema, const char *table, STRINGBUFFER *buffer) {
	char *sql = NULL;

	sql = rtloader_alloc_sprintf("DROP TABLE IF EXISTS %s%s;",
		(schema != NULL ? schema : ""),
		table
	);
	if (sql == NULL) {
		rterror(_("drop_table: Could not allocate memory for DROP TABLE statement"));
		return 0;
	}

	append_sql_to_buffer(buffer, sql);

	return 1;
}

static int
create_table(const char *schema,
	     const char *table,
	     const char *column,
	     const int file_column,
	     const char *file_column_name,
	     const char *tablespace,
	     const char *idx_tablespace,
	     int if_not_exists,
	     STRINGBUFFER *buffer)
{
	char *sql = NULL;

	assert(table != NULL);
	assert(column != NULL);

	sql = rtloader_alloc_sprintf(
		"CREATE TABLE %s%s%s (\"rid\" serial PRIMARY KEY%s%s,%s raster%s%s%s)%s%s;",
		(if_not_exists ? "IF NOT EXISTS " : ""),
		(schema != NULL ? schema : ""),
		table,
		(idx_tablespace != NULL ? " USING INDEX TABLESPACE " : ""),
		(idx_tablespace != NULL ? idx_tablespace : ""),
		column,
		(file_column ? "," : ""),
		(file_column ? file_column_name : ""),
		(file_column ? " text" : ""),
		(tablespace != NULL ? " TABLESPACE " : ""),
		(tablespace != NULL ? tablespace : ""));
	if (sql == NULL) {
		rterror(_("create_table: Could not allocate memory for CREATE TABLE statement"));
		return 0;
	}

	append_sql_to_buffer(buffer, sql);

	return 1;
}

static int
create_index(const char *schema,
	     const char *table,
	     const char *column,
	     const char *tablespace,
	     int if_not_exists,
	     STRINGBUFFER *buffer)
{
	char *sql = NULL;
	size_t len = 0;
	char *_table = NULL;
	char *_column = NULL;

	assert(table != NULL);
	assert(column != NULL);

	_table = chartrim(table, "\"");
	_column = chartrim(column, "\"");

	/* create index */
	len = strlen("CREATE INDEX IF NOT EXISTS \"__gist\" ON  USING gist (st_convexhull());") + 1;
	if (schema != NULL)
		len += strlen(schema);
	len += strlen(_table);
	len += strlen(_column);
	len += strlen(table);
	len += strlen(column);
	if (tablespace != NULL)
		len += strlen(" TABLESPACE ") + strlen(tablespace);

	sql = rtalloc(sizeof(char) * len);
	if (sql == NULL) {
		rterror(_("create_index: Could not allocate memory for CREATE INDEX statement"));
		rtdealloc(_table);
		rtdealloc(_column);
		return 0;
	}
	if (if_not_exists)
	{
		snprintf(sql,
			 len,
			 "CREATE INDEX IF NOT EXISTS \"%s_%s_gist\" ON %s%s USING gist (st_convexhull(%s))%s%s;",
			 _table,
			 _column,
			 (schema != NULL ? schema : ""),
			 table,
			 column,
			 (tablespace != NULL ? " TABLESPACE " : ""),
			 (tablespace != NULL ? tablespace : ""));
	}
	else
	{
		snprintf(sql,
			 len,
			 "CREATE INDEX ON %s%s USING gist (st_convexhull(%s))%s%s;",
			 (schema != NULL ? schema : ""),
			 table,
			 column,
			 (tablespace != NULL ? " TABLESPACE " : ""),
			 (tablespace != NULL ? tablespace : ""));
	}
	rtdealloc(_table);
	rtdealloc(_column);

	append_sql_to_buffer(buffer, sql);

	return 1;
}

static int
analyze_table(
	const char *schema, const char *table,
	STRINGBUFFER *buffer
) {
	char *sql = NULL;

	assert(table != NULL);

	sql = rtloader_alloc_sprintf("ANALYZE %s%s;",
		(schema != NULL ? schema : ""),
		table
	);
	if (sql == NULL) {
		rterror(_("analyze_table: Could not allocate memory for ANALYZE TABLE statement"));
		return 0;
	}

	append_sql_to_buffer(buffer, sql);

	return 1;
}

static int
vacuum_table(const char *schema, const char *table, int analyze, STRINGBUFFER *buffer)
{
	char *sql = NULL;

	assert(table != NULL);

	sql = rtloader_alloc_sprintf("VACUUM%s %s%s;",
		(analyze ? " ANALYZE" : ""),
		(schema != NULL ? schema : ""),
		table
	);
	if (sql == NULL) {
		rterror(_("vacuum_table: Could not allocate memory for VACUUM statement"));
		return 0;
	}

	append_sql_to_buffer(buffer, sql);

	return 1;
}

static int
add_raster_constraints(
	const char *schema, const char *table, const char *column,
	int regular_blocking, int max_extent,
	STRINGBUFFER *buffer
) {
	char *sql = NULL;

	char *_tmp = NULL;
	char *_schema = NULL;
	char *_table = NULL;
	char *_column = NULL;

	assert(table != NULL);
	assert(column != NULL);

	/* schema */
	if (schema != NULL) {
		_tmp = chartrim(schema, ".");
		_schema = chartrim(_tmp, "\"");
		rtdealloc(_tmp);
		_tmp = strreplace(_schema, "'", "''", NULL);
		rtdealloc(_schema);
		_schema = _tmp;
	}

	/* table */
	_tmp = chartrim(table, "\"");
	_table = strreplace(_tmp, "'", "''", NULL);
	rtdealloc(_tmp);

	/* column */
	_tmp = chartrim(column, "\"");
	_column = strreplace(_tmp, "'", "''", NULL);
	rtdealloc(_tmp);

	sql = rtloader_alloc_sprintf(
		"CALL AddRasterConstraints('%s','%s','%s',TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,%s,TRUE,TRUE,TRUE,TRUE,%s,TRUE);",
