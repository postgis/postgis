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
 * Copyright 2002 Thamer Alharbash
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGBUFFER_STARTSIZE 128

typedef struct
{
	size_t capacity;
	char *str_end;
	char *str_start;
}
stringbuffer_t;

extern stringbuffer_t *stringbuffer_create_with_size(size_t size);
extern stringbuffer_t *stringbuffer_create(void);
extern void stringbuffer_init(stringbuffer_t *s);
extern void stringbuffer_release(stringbuffer_t *s);
extern void stringbuffer_destroy(stringbuffer_t *sb);
extern void stringbuffer_clear(stringbuffer_t *sb);
void stringbuffer_set(stringbuffer_t *sb, const char *s);
void stringbuffer_copy(stringbuffer_t *sb, stringbuffer_t *src);
extern void stringbuffer_append(stringbuffer_t *sb, const char *s);
extern int stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...);
extern const char *stringbuffer_getstring(stringbuffer_t *sb);
extern char *stringbuffer_getstringcopy(stringbuffer_t *sb);
extern int stringbuffer_getlength(stringbuffer_t *sb);
extern char stringbuffer_lastchar(stringbuffer_t *s);
extern int stringbuffer_trim_trailing_white(stringbuffer_t *s);
extern int stringbuffer_trim_trailing_zeroes(stringbuffer_t *s);

#endif /* _STRINGBUFFER_H */
