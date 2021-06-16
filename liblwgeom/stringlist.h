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


#ifndef _STRINGLIST_H
#define _STRINGLIST_H 1

#include "liblwgeom_internal.h"


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGLIST_STARTSIZE 8

typedef struct
{
	size_t capacity;
	size_t length;
	char **data;
}
stringlist_t;


extern stringlist_t * stringlist_create_with_size(size_t size);
extern stringlist_t * stringlist_create(void);
extern void stringlist_destroy(stringlist_t *s);

extern void stringlist_init(stringlist_t *s);
extern void stringlist_release(stringlist_t *s);

extern void stringlist_add_string(stringlist_t *s, const char* string);
extern void stringlist_add_string_nosort(stringlist_t *s, const char* string);
extern void stringlist_sort(stringlist_t *s);
extern const char * stringlist_find(stringlist_t *s, const char *key);
extern size_t stringlist_length(stringlist_t *s);
extern const char * stringlist_get(stringlist_t *s, size_t i);


#endif

