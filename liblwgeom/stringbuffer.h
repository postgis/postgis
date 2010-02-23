/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2002 Thamer Alharbash
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGBUFFER_STARTSIZE 32
#define STRINGBUFFER_WORKSIZE 128

typedef struct
{
	size_t length;
	size_t capacity;
	char buffer[STRINGBUFFER_WORKSIZE];
	char *str;
}
stringbuffer_t;

extern stringbuffer_t *stringbuffer_create(void);
extern void stringbuffer_destroy(stringbuffer_t *sb);
extern void stringbuffer_clear(stringbuffer_t *sb);
void stringbuffer_set(stringbuffer_t *sb, const char *s);
void stringbuffer_copy(stringbuffer_t *sb, stringbuffer_t *src);
extern void stringbuffer_append(stringbuffer_t *sb, const char *s);
extern void stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...);
extern const char *stringbuffer_getstring(stringbuffer_t *sb);
extern void stringbuffer_vasbappend(stringbuffer_t *sb, const char *fmt, ... );

