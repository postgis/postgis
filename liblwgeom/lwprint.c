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
 * Copyright (C) 2010-2015 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ryu/ryu.h"

#define LATLON_SRID 4326
#define LATLON_MAX_TOKENS 32
#define LATLON_MAX_COMPONENTS 3

typedef enum
{
	LATLON_TOKEN_NUMBER,
	LATLON_TOKEN_DIR
} LATLON_TOKEN_TYPE;

typedef struct {
	LATLON_TOKEN_TYPE type;
	double value;
	int has_decimal;
	int has_negative_sign;
	int integer_digits;
	char dir;
} LATLON_TOKEN;

typedef struct {
	double value;
	int components;
	char dir;
} LATLON_COORD;

static int
lwlatlon_is_alpha(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static char
lwlatlon_upper(char c)
{
	if (c >= 'a' && c <= 'z')
		return c - ('a' - 'A');
	return c;
}

static int
lwlatlon_dir_axis(char dir)
{
	switch (lwlatlon_upper(dir))
	{
	case 'N':
	case 'S':
		return 1; /* latitude */
	case 'E':
	case 'W':
		return 2; /* longitude */
	default:
		return 0;
	}
}

static int
lwlatlon_dir_sign(char dir)
{
	switch (lwlatlon_upper(dir))
	{
	case 'S':
	case 'W':
		return -1;
	case 'N':
	case 'E':
		return 1;
	default:
		return 0;
	}
}

static int
lwlatlon_parse_number(const char *p,
		      const char **endptr,
		      double *value,
		      int *has_decimal,
		      int *has_negative_sign,
		      int *integer_digits)
{
	const char *s = p;
	double val = 0.0;
	int sign = 1;
	int ndigits = 0;
	int int_digits = 0;
	int saw_decimal = LW_FALSE;

	if (*s == '+' || *s == '-')
	{
		if (*s == '-')
			sign = -1;
		s++;
	}

	while (isdigit((unsigned char)*s))
	{
		val = val * 10.0 + (*s - '0');
		if (!isfinite(val))
			return LW_FALSE;
		ndigits++;
		int_digits++;
		s++;
	}

	if (*s == '.')
	{
		double numerator = 0.0;
		double denominator = 1.0;
		saw_decimal = LW_TRUE;
		s++;
		while (isdigit((unsigned char)*s))
		{
			numerator = numerator * 10.0 + (*s - '0');
			denominator *= 10.0;
			if (!isfinite(numerator) || !isfinite(denominator))
				return LW_FALSE;
			ndigits++;
			s++;
		}
		val += numerator / denominator;
	}

	if (ndigits == 0)
		return LW_FALSE;

	*value = sign * val;
	*has_decimal = saw_decimal;
	*has_negative_sign = (sign < 0);
	*integer_digits = int_digits;
	*endptr = s;
	return LW_TRUE;
}

static int
lwlatlon_parse_tokens(const char *input, LATLON_TOKEN *tokens, int *ntokens)
{
	const char *p = input;
	*ntokens = 0;

	while (*p)
	{
		if ((*p == '+' || *p == '-' || *p == '.' || isdigit((unsigned char)*p)) &&
		    ((*p != '+' && *p != '-') || isdigit((unsigned char)p[1]) || p[1] == '.') &&
		    (*p != '.' || isdigit((unsigned char)p[1])))
		{
			const char *endptr = NULL;
			double value;
			int has_decimal;
			int has_negative_sign;
			int integer_digits;

			if (*ntokens >= LATLON_MAX_TOKENS)
				return LW_FALSE;

			if (!lwlatlon_parse_number(
				p, &endptr, &value, &has_decimal, &has_negative_sign, &integer_digits))
				return LW_FALSE;

			tokens[*ntokens].type = LATLON_TOKEN_NUMBER;
			tokens[*ntokens].value = value;
			tokens[*ntokens].has_decimal = has_decimal;
			tokens[*ntokens].has_negative_sign = has_negative_sign;
			tokens[*ntokens].integer_digits = integer_digits;
			tokens[*ntokens].dir = '\0';
			(*ntokens)++;
			p = endptr;
			continue;
		}

		if (lwlatlon_dir_axis(*p) && !(p > input && lwlatlon_is_alpha((unsigned char)p[-1])) &&
		    !lwlatlon_is_alpha((unsigned char)p[1]))
		{
			if (*ntokens >= LATLON_MAX_TOKENS)
				return LW_FALSE;

			tokens[*ntokens].type = LATLON_TOKEN_DIR;
			tokens[*ntokens].value = 0.0;
			tokens[*ntokens].has_negative_sign = LW_FALSE;
			tokens[*ntokens].integer_digits = 0;
			tokens[*ntokens].dir = lwlatlon_upper(*p);
			(*ntokens)++;
		}

		p++;
	}

	return *ntokens > 0;
}

static int
lwlatlon_coord_from_components(const double values[LATLON_MAX_COMPONENTS],
			       const int has_decimal[LATLON_MAX_COMPONENTS],
			       const int has_negative_sign[LATLON_MAX_COMPONENTS],
			       const int integer_digits[LATLON_MAX_COMPONENTS],
			       int nvalues,
			       char dir,
			       int axis,
			       double *result)
{
	double degrees, minutes = 0.0, seconds = 0.0;
	double value;
	int sign = 1;
	int dirsign = lwlatlon_dir_sign(dir);
	double max_degrees = (axis == 1) ? 90.0 : 180.0;

	if (nvalues < 1 || nvalues > LATLON_MAX_COMPONENTS)
		return LW_FALSE;
	if (axis != 1 && axis != 2)
		return LW_FALSE;
	if (dir && lwlatlon_dir_axis(dir) != axis)
		return LW_FALSE;
	if (nvalues > 1 && has_decimal[0])
		return LW_FALSE;
	if (nvalues > 2 && has_decimal[1])
		return LW_FALSE;
	if (nvalues == 1 && !has_decimal[0] && integer_digits[0] > (axis == 1 ? 2 : 3))
		return LW_FALSE;

	degrees = values[0];
	if (has_negative_sign[0] || degrees < 0)
	{
		sign = -1;
		degrees = fabs(degrees);
	}
	if (nvalues > 1)
		minutes = values[1];
	if (nvalues > 2)
		seconds = values[2];

	if (minutes < 0.0 || minutes >= 60.0 || seconds < 0.0 || seconds >= 60.0)
		return LW_FALSE;
	if (degrees > max_degrees)
		return LW_FALSE;
	if (dirsign && sign < 0 && dirsign > 0)
		return LW_FALSE;

	value = degrees + minutes / 60.0 + seconds / 3600.0;
	if (dirsign)
		sign = dirsign;
	value *= sign;

	if (fabs(value) > max_degrees + 1e-12)
		return LW_FALSE;

	*result = value;
	return LW_TRUE;
}

static int
lwlatlon_coord_from_slice(const double *values,
			  const int *has_decimal,
			  const int *has_negative_sign,
			  const int *integer_digits,
			  int nvalues,
			  char dir,
			  LATLON_COORD *coord)
{
	double coord_values[LATLON_MAX_COMPONENTS] = {0.0, 0.0, 0.0};
	int coord_has_decimal[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int coord_has_negative_sign[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int coord_integer_digits[LATLON_MAX_COMPONENTS] = {0, 0, 0};
	int axis = lwlatlon_dir_axis(dir);
	int i;

	if (!axis || nvalues < 1 || nvalues > LATLON_MAX_COMPONENTS)
		return LW_FALSE;

	for (i = 0; i < nvalues; i++)
	{
		coord_values[i] = values[i];
		coord_has_decimal[i] = has_decimal[i];
		coord_has_negative_sign[i] = has_negative_sign[i];
		coord_integer_digits[i] = integer_digits[i];
	}

	if (!lwlatlon_coord_from_components(coord_values,
					    coord_has_decimal,
					    coord_has_negative_sign,
					    coord_integer_digits,
					    nvalues,
					    dir,
					    axis,
					    &coord->value))
		return LW_FALSE;

	coord->components = nvalues;
	coord->dir = dir;
	return LW_TRUE;
}

static int
lwlatlon_finalize_coord(const double values[LATLON_MAX_COMPONENTS],
			const int has_decimal[LATLON_MAX_COMPONENTS],
			const int has_negative_sign[LATLON_MAX_COMPONENTS],
			const int integer_digits[LATLON_MAX_COMPONENTS],
			int nvalues,
			char dir,
			LATLON_COORD *coords,
			int *ncoords)
{
	int axis = lwlatlon_dir_axis(dir);

	if (!axis || *ncoords >= 2)
		return LW_FALSE;
	if (!lwlatlon_coord_from_components(
		values, has_decimal, has_negative_sign, integer_digits, nvalues, dir, axis, &coords[*ncoords].value))
		return LW_FALSE;

	coords[*ncoords].components = nvalues;
	coords[*ncoords].dir = dir;
	(*ncoords)++;
	return LW_TRUE;
}

static int
lwlatlon_dir_has_numbers_after(const LATLON_TOKEN *tokens, int start, int ntokens)
{
	int i;

	for (i = start; i < ntokens; i++)
	{
		if (tokens[i].type == LATLON_TOKEN_NUMBER)
			return LW_TRUE;
		if (tokens[i].type == LATLON_TOKEN_DIR)
			return LW_FALSE;
	}

	return LW_FALSE;
}

static int
lwlatlon_try_finalize_mixed_dir_pair(const double values[LATLON_MAX_COMPONENTS],
				     const int has_decimal[LATLON_MAX_COMPONENTS],
				     const int has_negative_sign[LATLON_MAX_COMPONENTS],
				     const int integer_digits[LATLON_MAX_COMPONENTS],
				     int nvalues,
				     char prefix_dir,
				     char suffix_dir,
				     LATLON_COORD *coords,
				     int *ncoords)
{
	int first_components;
	LATLON_COORD first_coord;
	LATLON_COORD second_coord;

	if (*ncoords != 0)
		return LW_FALSE;

	for (first_components = 1; first_components < nvalues; first_components++)
	{
		int second_components = nvalues - first_components;

		if (!lwlatlon_coord_from_slice(values,
					       has_decimal,
					       has_negative_sign,
					       integer_digits,
					       first_components,
					       prefix_dir,
					       &first_coord))
			continue;
		if (!lwlatlon_coord_from_slice(values + first_components,
					       has_decimal + first_components,
					       has_negative_sign + first_components,
					       integer_digits + first_components,
					       second_components,
					       suffix_dir,
					       &second_coord))
			continue;
		if (first_coord.components != second_coord.components)
			continue;

		coords[(*ncoords)++] = first_coord;
		coords[(*ncoords)++] = second_coord;
		return LW_TRUE;
	}

	return LW_FALSE;
}

static int
lwlatlon_parse_axis_tokens(const LATLON_TOKEN *tokens, int ntokens, int axis, double *result, int *components)
{
	double values[LATLON_MAX_COMPONENTS] = {0.0, 0.0, 0.0};
	int has_decimal[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int has_negative_sign[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int integer_digits[LATLON_MAX_COMPONENTS] = {0, 0, 0};
	char dir = '\0';
	int nvalues = 0;
	int i;

	for (i = 0; i < ntokens; i++)
	{
		if (tokens[i].type == LATLON_TOKEN_NUMBER)
		{
			if (nvalues >= LATLON_MAX_COMPONENTS)
				return LW_FALSE;
			values[nvalues] = tokens[i].value;
			has_decimal[nvalues] = tokens[i].has_decimal;
			has_negative_sign[nvalues] = tokens[i].has_negative_sign;
			integer_digits[nvalues] = tokens[i].integer_digits;
			nvalues++;
		}
		else
		{
			if (dir || lwlatlon_dir_axis(tokens[i].dir) != axis)
				return LW_FALSE;
			dir = tokens[i].dir;
		}
	}

	if (!lwlatlon_coord_from_components(
		values, has_decimal, has_negative_sign, integer_digits, nvalues, dir, axis, result))
		return LW_FALSE;

	*components = nvalues;
	return LW_TRUE;
}

static int
lwlatlon_parse_no_dir_pair(const LATLON_TOKEN *tokens, int ntokens, double *lat, double *lon)
{
	double values[2 * LATLON_MAX_COMPONENTS];
	int has_decimal[2 * LATLON_MAX_COMPONENTS];
	int has_negative_sign[2 * LATLON_MAX_COMPONENTS];
	int integer_digits[2 * LATLON_MAX_COMPONENTS];
	double lat_values[LATLON_MAX_COMPONENTS] = {0.0, 0.0, 0.0};
	double lon_values[LATLON_MAX_COMPONENTS] = {0.0, 0.0, 0.0};
	int lat_has_decimal[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int lon_has_decimal[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int lat_has_negative_sign[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int lon_has_negative_sign[LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE};
	int lat_integer_digits[LATLON_MAX_COMPONENTS] = {0, 0, 0};
	int lon_integer_digits[LATLON_MAX_COMPONENTS] = {0, 0, 0};
	int nvalues = 0;
	int ncomponents;
	int i;

	for (i = 0; i < ntokens; i++)
	{
		if (tokens[i].type != LATLON_TOKEN_NUMBER)
			return LW_FALSE;
		if (nvalues >= 2 * LATLON_MAX_COMPONENTS)
			return LW_FALSE;
		values[nvalues] = tokens[i].value;
		has_decimal[nvalues] = tokens[i].has_decimal;
		has_negative_sign[nvalues] = tokens[i].has_negative_sign;
		integer_digits[nvalues] = tokens[i].integer_digits;
		nvalues++;
	}

	if (nvalues != 2 && nvalues != 4 && nvalues != 6)
		return LW_FALSE;

	ncomponents = nvalues / 2;
	for (i = 0; i < ncomponents; i++)
	{
		lat_values[i] = values[i];
		lat_has_decimal[i] = has_decimal[i];
		lat_has_negative_sign[i] = has_negative_sign[i];
		lat_integer_digits[i] = integer_digits[i];
		lon_values[i] = values[ncomponents + i];
		lon_has_decimal[i] = has_decimal[ncomponents + i];
		lon_has_negative_sign[i] = has_negative_sign[ncomponents + i];
		lon_integer_digits[i] = integer_digits[ncomponents + i];
	}

	return lwlatlon_coord_from_components(
		   lat_values, lat_has_decimal, lat_has_negative_sign, lat_integer_digits, ncomponents, '\0', 1, lat) &&
	       lwlatlon_coord_from_components(
		   lon_values, lon_has_decimal, lon_has_negative_sign, lon_integer_digits, ncomponents, '\0', 2, lon);
}

static int
lwlatlon_parse_dir_pair(const LATLON_TOKEN *tokens, int ntokens, double *lat, double *lon)
{
	LATLON_COORD coords[2];
	double values[2 * LATLON_MAX_COMPONENTS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	int has_decimal[2 * LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE};
	int has_negative_sign[2 * LATLON_MAX_COMPONENTS] = {LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE, LW_FALSE};
	int integer_digits[2 * LATLON_MAX_COMPONENTS] = {0, 0, 0, 0, 0, 0};
	char dir = '\0';
	int nvalues = 0;
	int ncoords = 0;
	int saw_lat = LW_FALSE;
	int saw_lon = LW_FALSE;
	int i;

	for (i = 0; i < ntokens; i++)
	{
		if (tokens[i].type == LATLON_TOKEN_NUMBER)
		{
			if (nvalues >= 2 * LATLON_MAX_COMPONENTS)
				return LW_FALSE;
			values[nvalues] = tokens[i].value;
			has_decimal[nvalues] = tokens[i].has_decimal;
			has_negative_sign[nvalues] = tokens[i].has_negative_sign;
			integer_digits[nvalues] = tokens[i].integer_digits;
			nvalues++;
			continue;
		}

		if (dir && nvalues > 0)
		{
			if (!lwlatlon_dir_has_numbers_after(tokens, i + 1, ntokens) &&
			    lwlatlon_dir_axis(dir) != lwlatlon_dir_axis(tokens[i].dir) &&
			    lwlatlon_try_finalize_mixed_dir_pair(values,
								 has_decimal,
								 has_negative_sign,
								 integer_digits,
								 nvalues,
								 dir,
								 tokens[i].dir,
								 coords,
								 &ncoords))
			{
				memset(values, 0, sizeof(values));
				memset(has_decimal, 0, sizeof(has_decimal));
				memset(has_negative_sign, 0, sizeof(has_negative_sign));
				memset(integer_digits, 0, sizeof(integer_digits));
				nvalues = 0;
				dir = '\0';
				continue;
			}

			if (!lwlatlon_finalize_coord(
				values, has_decimal, has_negative_sign, integer_digits, nvalues, dir, coords, &ncoords))
				return LW_FALSE;
			memset(values, 0, sizeof(values));
			memset(has_decimal, 0, sizeof(has_decimal));
			memset(has_negative_sign, 0, sizeof(has_negative_sign));
			memset(integer_digits, 0, sizeof(integer_digits));
			nvalues = 0;
			dir = tokens[i].dir;
		}
		else if (dir)
			return LW_FALSE;
		else if (nvalues > 0)
		{
			dir = tokens[i].dir;
			if (!lwlatlon_finalize_coord(
				values, has_decimal, has_negative_sign, integer_digits, nvalues, dir, coords, &ncoords))
				return LW_FALSE;
			memset(values, 0, sizeof(values));
			memset(has_decimal, 0, sizeof(has_decimal));
			memset(has_negative_sign, 0, sizeof(has_negative_sign));
			memset(integer_digits, 0, sizeof(integer_digits));
			nvalues = 0;
			dir = '\0';
		}
		else
			dir = tokens[i].dir;
	}

	if (dir || nvalues)
	{
		if (!dir || !nvalues)
			return LW_FALSE;
		if (!lwlatlon_finalize_coord(
			values, has_decimal, has_negative_sign, integer_digits, nvalues, dir, coords, &ncoords))
			return LW_FALSE;
	}

	if (ncoords != 2 || coords[0].components != coords[1].components)
		return LW_FALSE;

	for (i = 0; i < 2; i++)
	{
		switch (lwlatlon_dir_axis(coords[i].dir))
		{
		case 1:
			if (saw_lat)
				return LW_FALSE;
			*lat = coords[i].value;
			saw_lat = LW_TRUE;
			break;
		case 2:
			if (saw_lon)
				return LW_FALSE;
			*lon = coords[i].value;
			saw_lon = LW_TRUE;
			break;
		default:
			return LW_FALSE;
		}
	}

	return saw_lat && saw_lon;
}

LWPOINT *
lwpoint_from_latlon_parts(const char *lat, const char *lon)
{
	LATLON_TOKEN lat_tokens[LATLON_MAX_TOKENS];
	LATLON_TOKEN lon_tokens[LATLON_MAX_TOKENS];
	int nlat_tokens = 0;
	int nlon_tokens = 0;
	int lat_components = 0;
	int lon_components = 0;
	double lat_value = 0.0;
	double lon_value = 0.0;

	if (!lat || !lon || !lwlatlon_parse_tokens(lat, lat_tokens, &nlat_tokens) ||
	    !lwlatlon_parse_tokens(lon, lon_tokens, &nlon_tokens) ||
	    !lwlatlon_parse_axis_tokens(lat_tokens, nlat_tokens, 1, &lat_value, &lat_components) ||
	    !lwlatlon_parse_axis_tokens(lon_tokens, nlon_tokens, 2, &lon_value, &lon_components) ||
	    lat_components != lon_components)
		lwerror("Invalid latitude/longitude text");

	return lwpoint_make2d(LATLON_SRID, lon_value, lat_value);
}

LWPOINT *
lwpoint_from_latlon(const char *input)
{
	LATLON_TOKEN tokens[LATLON_MAX_TOKENS];
	int ntokens = 0;
	int has_dir = LW_FALSE;
	double lat = 0.0;
	double lon = 0.0;
	int i;

	if (!input || !lwlatlon_parse_tokens(input, tokens, &ntokens))
		lwerror("Invalid latitude/longitude text");

	for (i = 0; i < ntokens; i++)
	{
		if (tokens[i].type == LATLON_TOKEN_DIR)
		{
			has_dir = LW_TRUE;
			break;
		}
	}

	if (!(has_dir ? lwlatlon_parse_dir_pair(tokens, ntokens, &lat, &lon)
		      : lwlatlon_parse_no_dir_pair(tokens, ntokens, &lat, &lon)))
		lwerror("Invalid latitude/longitude text");

	return lwpoint_make2d(LATLON_SRID, lon, lat);
}

/* Ensures the given lat and lon are in the "normal" range:
 * -90 to +90 for lat, -180 to +180 for lon. */
static void lwprint_normalize_latlon(double *lat, double *lon)
{
	/* First remove all the truly excessive trips around the world via up or down. */
	while (*lat > 270)
	{
		*lat -= 360;
	}
	while (*lat < -270)
	{
		*lat += 360;
	}

	/* Now see if latitude is past the top or bottom of the world.
	 * Past 90  or -90 puts us on the other side of the earth,
	     * so wrap latitude and add 180 to longitude to reflect that. */
	if (*lat > 90)
	{
		*lat = 180 - *lat;
		*lon += 180;
	}
	if (*lat < -90)
	{
		*lat = -180 - *lat;
		*lon += 180;
	}
	/* Now make sure lon is in the normal range.  Wrapping longitude
	 * has no effect on latitude. */
	while (*lon > 180)
	{
		*lon -= 360;
	}
	while (*lon < -180)
	{
		*lon += 360;
	}
}

/* Converts a single double to DMS given the specified DMS format string.
 * Symbols are specified since N/S or E/W are the only differences when printing
 * lat vs. lon.  They are only used if the "C" (compass dir) token appears in the
 * format string.
 * NOTE: Format string and symbols are required to be in UTF-8. */
static char * lwdouble_to_dms(double val, const char *pos_dir_symbol, const char *neg_dir_symbol, const char * format)
{
	/* 3 numbers, 1 sign or compass dir, and 5 possible strings (degree signs, spaces, misc text, etc) between or around them.*/
#	define NUM_PIECES 9
#	define WORK_SIZE 1024
	char pieces[NUM_PIECES][WORK_SIZE];
	int current_piece = 0;
	int is_negative = 0;

	double degrees = 0.0;
	double minutes = 0.0;
	double seconds = 0.0;

	int compass_dir_piece = -1;

	int reading_deg = 0;
	int deg_digits = 0;
	int deg_has_decpoint = 0;
	int deg_dec_digits = 0;
	int deg_piece = -1;

	int reading_min = 0;
	int min_digits = 0;
	int min_has_decpoint = 0;
	int min_dec_digits = 0;
	int min_piece = -1;

	int reading_sec = 0;
	int sec_digits = 0;
	int sec_has_decpoint = 0;
	int sec_dec_digits = 0;
	int sec_piece = -1;

	int round_pow = 0;

	int format_length = ((NULL == format) ? 0 : strlen(format));

	char * result;

	int index, following_byte_index;
	int multibyte_char_width = 1;

	/* Initialize the working strs to blank.  We may not populate all of them, and
	 * this allows us to concat them all at the end without worrying about how many
	 * we actually needed. */
	for (index = 0; index < NUM_PIECES; index++)
	{
		pieces[index][0] = '\0';
	}

	/* If no format is provided, use a default. */
	if (0 == format_length)
	{
		/* C2B0 is UTF-8 for the degree symbol. */
		format = "D\xC2\xB0""M'S.SSS\"C";
		format_length = strlen(format);
	}
	else if (format_length > WORK_SIZE)
	{
		/* Sanity check, we don't want to overwrite an entire piece of work and no one should need a 1K-sized
		* format string anyway. */
		lwerror("Bad format, exceeds maximum length (%d).", WORK_SIZE);
	}

	for (index = 0; index < format_length; index++)
	{
		char next_char = format[index];
		switch (next_char)
		{
		case 'D':
			if (reading_deg)
			{
				/* If we're reading degrees, add another digit. */
				deg_has_decpoint ? deg_dec_digits++ : deg_digits++;
			}
			else
			{
				/* If we're not reading degrees, we are now. */
				current_piece++;
				deg_piece = current_piece;
				if (deg_digits > 0)
				{
					lwerror("Bad format, cannot include degrees (DD.DDD) more than once.");
				}
				reading_deg = 1;
				reading_min = 0;
				reading_sec = 0;
				deg_digits++;
			}
			break;
		case 'M':
			if (reading_min)
			{
				/* If we're reading minutes, add another digit. */
				min_has_decpoint ? min_dec_digits++ : min_digits++;
			}
			else
			{
				/* If we're not reading minutes, we are now. */
				current_piece++;
				min_piece = current_piece;
				if (min_digits > 0)
				{
					lwerror("Bad format, cannot include minutes (MM.MMM) more than once.");
				}
				reading_deg = 0;
				reading_min = 1;
				reading_sec = 0;
				min_digits++;
			}
			break;
		case 'S':
			if (reading_sec)
			{
				/* If we're reading seconds, add another digit. */
				sec_has_decpoint ? sec_dec_digits++ : sec_digits++;
			}
			else
			{
				/* If we're not reading seconds, we are now. */
				current_piece++;
				sec_piece = current_piece;
				if (sec_digits > 0)
				{
					lwerror("Bad format, cannot include seconds (SS.SSS) more than once.");
				}
				reading_deg = 0;
				reading_min = 0;
				reading_sec = 1;
				sec_digits++;
			}
			break;
		case 'C':
			/* We're done reading anything else we might have been reading. */
			if (reading_deg || reading_min || reading_sec)
			{
				/* We were reading something, that means this is the next piece. */
				reading_deg = 0;
				reading_min = 0;
				reading_sec = 0;
			}
			current_piece++;

			if (compass_dir_piece >= 0)
			{
				lwerror("Bad format, cannot include compass dir (C) more than once.");
			}
			/* The compass dir is a piece all by itself.  */
			compass_dir_piece = current_piece;
			current_piece++;
			break;
		case '.':
			/* If we're reading deg, min, or sec, we want a decimal point for it. */
			if (reading_deg)
			{
				deg_has_decpoint = 1;
			}
			else if (reading_min)
			{
				min_has_decpoint = 1;
			}
			else if (reading_sec)
			{
				sec_has_decpoint = 1;
			}
			else
			{
				/* Not reading anything, just pass through the '.' */
				strncat(pieces[current_piece], &next_char, 1);
			}
			break;
		default:
			/* Any other char is just passed through unchanged.  But it does mean we are done reading D, M, or S.*/
			if (reading_deg || reading_min || reading_sec)
			{
				/* We were reading something, that means this is the next piece. */
				current_piece++;
				reading_deg = 0;
				reading_min = 0;
				reading_sec = 0;
			}

			/* Check if this is a multi-byte UTF-8 character.  If so go ahead and read the rest of the bytes as well. */
			multibyte_char_width = 1;
			if (next_char & 0x80)
			{
				if ((next_char & 0xF8) == 0xF0)
				{
					multibyte_char_width += 3;
				}
				else if ((next_char & 0xF0) == 0xE0)
				{
					multibyte_char_width += 2;
				}
				else if ((next_char & 0xE0) == 0xC0)
				{
					multibyte_char_width += 1;
				}
				else
				{
					lwerror("Bad format, invalid high-order byte found first, format string may not be UTF-8.");
				}
			}
			if (multibyte_char_width > 1)
			{
				if (index + multibyte_char_width >= format_length)
				{
					lwerror("Bad format, UTF-8 character first byte found with insufficient following bytes, format string may not be UTF-8.");
				}
				for (following_byte_index = (index + 1); following_byte_index < (index + multibyte_char_width); following_byte_index++)
				{
					if ((format[following_byte_index] & 0xC0) != 0x80)
					{
						lwerror("Bad format, invalid byte found following leading byte of multibyte character, format string may not be UTF-8.");
					}
				}
			}
			/* Copy all the character's bytes into the current piece. */
			strncat(pieces[current_piece], &(format[index]), multibyte_char_width);
			/* Now increment index past the rest of those bytes. */
			index += multibyte_char_width - 1;
			break;
		}
		if (current_piece >= NUM_PIECES)
		{
			lwerror("Internal error, somehow needed more pieces than it should.");
		}
	}
	if (deg_piece < 0)
	{
		lwerror("Bad format, degrees (DD.DDD) must be included.");
	}

	/* Divvy the number up into D, DM, or DMS */
	if (val < 0)
	{
		val *= -1;
		is_negative = 1;
	}
	degrees = val;
	if (min_digits > 0)
	{
		/* Break degrees to integer and use fraction for minutes */
		minutes = modf(val, &degrees) * 60;
	}
	if (sec_digits > 0)
	{
		if (0 == min_digits)
		{
			lwerror("Bad format, cannot include seconds (SS.SSS) without including minutes (MM.MMM).");
		}
		seconds = modf(minutes, &minutes) * 60;
		if (sec_piece >= 0)
		{
			/* See if the formatted seconds round up to 60. If so, increment minutes and reset seconds. */
			round_pow = pow(10, sec_dec_digits);
			if (lround(seconds * round_pow) >= 60 * round_pow)
			{
				minutes += 1;
				seconds = 0;
				/* See if the formatted minutes round up to 60. If so, increment degrees and reset seconds. */
				if (lround(minutes * round_pow) >= 60 * round_pow)
				{
					degrees += 1;
					minutes = 0;
				}
			}
		}
	}

	/* Handle the compass direction.  If not using compass dir, display degrees as a positive/negative number. */
	if (compass_dir_piece >= 0)
	{
		strcpy(pieces[compass_dir_piece], is_negative ? neg_dir_symbol : pos_dir_symbol);
	}
	else if (is_negative)
	{
		degrees *= -1;
	}

	/* Format the degrees into their string piece. */
	if (deg_digits + deg_dec_digits + 2 > WORK_SIZE)
	{
		lwerror("Bad format, degrees (DD.DDD) number of digits was greater than our working limit.");
	}
	if(deg_piece >= 0)
	{
		snprintf(pieces[deg_piece], WORK_SIZE, "%*.*f", deg_digits, deg_dec_digits, degrees);
	}

	if (min_piece >= 0)
	{
		/* Format the minutes into their string piece. */
		if (min_digits + min_dec_digits + 2 > WORK_SIZE)
		{
			lwerror("Bad format, minutes (MM.MMM) number of digits was greater than our working limit.");
		}
		snprintf(pieces[min_piece], WORK_SIZE, "%*.*f", min_digits, min_dec_digits, minutes);
	}
	if (sec_piece >= 0)
	{
		/* Format the seconds into their string piece. */
		if (sec_digits + sec_dec_digits + 2 > WORK_SIZE)
		{
			lwerror("Bad format, seconds (SS.SSS) number of digits was greater than our working limit.");
		}
		snprintf(pieces[sec_piece], WORK_SIZE, "%*.*f", sec_digits, sec_dec_digits, seconds);
	}

	/* Allocate space for the result.  Leave plenty of room for excess digits, negative sign, etc.*/
	result = (char*)lwalloc(format_length + WORK_SIZE);
	memset(result, 0, format_length + WORK_SIZE);

	/* Append all the pieces together. There may be less than 9, but in that case the rest will be blank. */
	strcpy(result, pieces[0]);
	for (index = 1; index < NUM_PIECES; index++)
	{
		strcat(result, pieces[index]);
	}

	return result;
}

/* Print two doubles (lat and lon) in DMS form using the specified format.
 * First normalizes them so they will display as -90 to 90 and -180 to 180.
 * Format string may be null or 0-length, in which case a default format will be used.
 * NOTE: Format string is required to be in UTF-8.
 * NOTE2: returned string is lwalloc'ed, caller is responsible to lwfree it up
 */
static char * lwdoubles_to_latlon(double lat, double lon, const char * format)
{
	char * lat_text;
	char * lon_text;
	char * result;
	size_t sz;

	/* Normalize lat/lon to the normal (-90 to 90, -180 to 180) range. */
	lwprint_normalize_latlon(&lat, &lon);
	/* This is somewhat inefficient as the format is parsed twice. */
	lat_text = lwdouble_to_dms(lat, "N", "S", format);
	lon_text = lwdouble_to_dms(lon, "E", "W", format);

	/* lat + lon + a space between + the null terminator. */
	sz = strlen(lat_text) + strlen(lon_text) + 2;
	result = (char*)lwalloc(sz);
	snprintf(result, sz, "%s %s", lat_text, lon_text);
	lwfree(lat_text);
	lwfree(lon_text);
	return result;
}

/* Print the X (lon) and Y (lat) of the given point in DMS form using
 * the specified format.
 * First normalizes the values so they will display as -90 to 90 and -180 to 180.
 * Format string may be null or 0-length, in which case a default format will be used.
 * NOTE: Format string is required to be in UTF-8.
 * NOTE2: returned string is lwalloc'ed, caller is responsible to lwfree it up
 */
char* lwpoint_to_latlon(const LWPOINT * pt, const char *format)
{
	const POINT2D *p;
	if (NULL == pt)
	{
		lwerror("Cannot convert a null point into formatted text.");
	}
	if (lwgeom_is_empty((LWGEOM *)pt))
	{
		lwerror("Cannot convert an empty point into formatted text.");
	}
	p = getPoint2d_cp(pt->point, 0);
	return lwdoubles_to_latlon(p->y, p->x, format);
}

/*
 * Print an ordinate value using at most **maxdd** number of decimal digits
 * The actual number of printed decimal digits may be less than the
 * requested ones if out of significant digits.
 *
 * The function will write at most OUT_DOUBLE_BUFFER_SIZE bytes, including the
 * terminating NULL.
 * It returns the number of bytes written (excluding the final NULL)
 *
 */
int
lwprint_double(double d, int maxdd, char *buf)
{
	int length;
	double ad = fabs(d);
	int precision = FP_MAX(0, maxdd);

	if (ad <= OUT_MIN_DOUBLE || ad >= OUT_MAX_DOUBLE)
	{
		length = d2sexp_buffered_n(d, precision, buf);
	}
	else
	{
		length = d2sfixed_buffered_n(d, precision, buf);
	}
	buf[length] = '\0';

	return length;
}
