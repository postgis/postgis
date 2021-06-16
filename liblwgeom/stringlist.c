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
#include "stringlist.h"

static size_t
stringlist_capacity_in_bytes(size_t capacity)
{
	return capacity * sizeof(char*);
}

static void
stringlist_init_with_size(stringlist_t *s, size_t size)
{
	s->capacity = size;
	s->length = 0;
	s->data = lwalloc(stringlist_capacity_in_bytes(s->capacity));
	memset(s->data, 0, stringlist_capacity_in_bytes(s->capacity));
}

void
stringlist_init(stringlist_t *s)
{
	stringlist_init_with_size(s, STRINGLIST_STARTSIZE);
}

void
stringlist_release(stringlist_t *s)
{
	size_t i;
	if (!s || !s->data) return;
	for (i = 0; i < s->length; i++)
		if (s->data[i]) lwfree(s->data[i]);
	lwfree(s->data);
	memset(s, 0, sizeof(stringlist_t));
}


stringlist_t *
stringlist_create_with_size(size_t size)
{
	stringlist_t *s = lwalloc(sizeof(stringlist_t));
	memset(s, 0, sizeof(stringlist_t));
	stringlist_init(s);
	return s;
}

stringlist_t *
stringlist_create(void)
{
	return stringlist_create_with_size(STRINGLIST_STARTSIZE);
}

void
stringlist_destroy(stringlist_t *s)
{
	stringlist_release(s);
	lwfree(s);
}

static int
stringlist_cmp(const void *a, const void *b)
{
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
}

static void
stringlist_add_string_internal(stringlist_t *s, const char* string, int dosort)
{
	if (!string) return;
	if (s->capacity == 0)
	{
		stringlist_init(s);
	}
	if (s->length == s->capacity)
	{
		s->capacity *= 2;
		s->data = lwrealloc(s->data, stringlist_capacity_in_bytes(s->capacity));
	};
	s->data[s->length++] = lwstrdup(string);
	if (dosort)
		stringlist_sort(s);
	return;
}

void
stringlist_add_string(stringlist_t *s, const char* string)
{
	stringlist_add_string_internal(s, string, 1);
}

void
stringlist_add_string_nosort(stringlist_t *s, const char* string)
{
	stringlist_add_string_internal(s, string, 0);
}

void
stringlist_sort(stringlist_t *s)
{
	qsort(s->data, s->length, sizeof(char*), stringlist_cmp);
}

const char *
stringlist_find(stringlist_t *s, const char *key)
{
	char ** rslt = bsearch(&key, s->data, s->length, sizeof(char*), stringlist_cmp);
	if (! rslt) return NULL;
	return *rslt;
}

size_t
stringlist_length(stringlist_t *s)
{
	return s->length;
}

const char *
stringlist_get(stringlist_t *s, size_t i)
{
	if (i < s->length)
		return s->data[i];
	return NULL;
}
