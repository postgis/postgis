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


#ifndef _OPTIONLIST_H
#define _OPTIONLIST_H 1

#include "liblwgeom_internal.h"

#define OPTION_LIST_SIZE 128

/**
* option_list is a null-terminated list of strings, where every odd
* string is a key and every even string is a value. To avoid lots of
* memory allocations, we use the input string as the backing store
* and fill in the olist with pointers to the start of the key and
* value strings, with keys in the even slots and values in the
* odd slots. The input needs to be writeable because we write a
* '\0' into the string at the break between token separators.
*
* // array of char*
* char *olist[OPTION_LIST_SIZE];
* // zero out all the pointers so our list ends up null-terminated
* memset(olist, 0, sizeof(olist));
* char input[OPTION_LIST_SIZE];
* strcpy(input, "key1=value1 key2=value2"); // writeable
* option_list_parse(input, olist);
* char* key = "key2";
* const char* value = option_list_search(olist, key);
* printf("value of '%s' is '%s'\n", key, value);
*
*/
extern void option_list_parse(char* input, char **olist);

/**
* Returns null if the key cannot be found. Only
* use fully lowercase keys, because we lowercase
* keys when we parse the olist
*/
extern const char* option_list_search(char** olist, const char* key);

/**
* Returns the total number of keys and values in the list
*/
extern size_t option_list_length(char **olist);


extern void option_list_gdal_parse(char* input, char** olist);


#endif

