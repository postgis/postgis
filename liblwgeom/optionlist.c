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
 * Copyright 2021 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "optionlist.h"

#include <ctype.h>  // tolower
#include <string.h> // strtok

static void
option_list_string_to_lower(char* key)
{
	if (!key) return;
	while (*key) {
		*key = tolower(*key);
		key++;
	}
	return;
}

// static void
// option_list_string_to_upper(char* key)
// {
// 	if (!key) return;
// 	while (*key) {
// 		*key = toupper(*key);
// 		key++;
// 	}
// 	return;
// }

const char*
option_list_search(char** olist, const char* key)
{
	size_t i = 0;
	if (!olist) return NULL;
	if (!key) return NULL;
	while (olist[i]) {
		// Even entries are keys
		if (!(i % 2)) {
			// Does this key match ours?
			if (strcmp(olist[i], key) == 0) {
				return olist[i+1];
			}
		}
		i++;
	}
	return NULL;
}

size_t
option_list_length(char** olist)
{
	size_t i = 0;
	char **iter = olist;
	if (!olist) return 0;
	while(*iter) {
		i++;
		iter++;
	}
	return i;
}

void
option_list_parse(char* input, char** olist)
{
	const char *toksep = " ";
	const char kvsep = '=';
	char *key, *val;
	if (!input) return;
	size_t i = 0, sz;

	/* strtok nulls out the space between each token */
	for (key = strtok(input, toksep); key; key = strtok(NULL, toksep)) {
		if (i >= OPTION_LIST_SIZE) return;
		olist[i] = key;
		i += 2;
	}

	sz = i;
	/* keys are every second entry in the olist */
	for (i = 0; i < sz; i += 2) {
		if (i >= OPTION_LIST_SIZE) return;
		key = olist[i];
		/* find the key/value separator */
		val = strchr(key, kvsep);
		if (!val) {
			lwerror("Option string entry '%s' lacks separator '%c'", key, kvsep);
		}
		/* null out the separator */
		*val = '\0';
		/* point value entry to just after separator */
		olist[i+1] = ++val;
		/* all keys forced to lower case */
		option_list_string_to_lower(key);
	}
}

void
option_list_gdal_parse(char* input, char** olist)
{
	const char *toksep = " ";
	const char kvsep = '=';
	const char q2 = '"';
	const char q1 = '\'';
	const char notspace = 0x1F;

	char *key, *val;
	int in_str = 0;
	size_t i = 0, sz, input_sz;
	char *ptr = input;

	if (!input)
		lwerror("Option string is null");
	input_sz = strlen(input);

	/* Temporarily hide quoted spaces */
	while(*ptr) {
		if (*ptr == q2 || *ptr == q1)
			in_str = !in_str;
		else if (in_str && *ptr == ' ')
			*ptr = notspace;

		ptr++;
	}

	/* Tokenize on spaces */
	for (key = strtok(input, toksep); key; key = strtok(NULL, toksep)) {
		if (i >= OPTION_LIST_SIZE) return;
		olist[i++] = key;
	}

	/* Check that these are GDAL KEY=VALUE options */
	sz = i;
	for (i = 0; i < sz; ++i) {
		if (i >= OPTION_LIST_SIZE) return;
		key = olist[i];
		/* find the key/value separator */
		val = strchr(key, kvsep);
		if (!val) {
			lwerror("Option string entry '%s' lacks separator '%c'", key, kvsep);
			return;
		}
	}

	/* Unhide quoted space */
	for (i = 0; i <= input_sz; ++i) {
		if (input[i] == notspace)
			input[i] = ' ';
	}
}

