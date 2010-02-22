/*
 *
 * Copyright 2002 Thamer Alharbash
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * 3. The names of the authors may not be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Sringbuffer object:
 *
 * (*) allows printfing into a string,
 * (*) fast string manipulation by keeping track of string length.
 * (*) alignment of string against columns.
 *
 * Internally stringbuffer does not count the terminating null as the length.
 * Therefore the raw string routines will always assume +1 when given length.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGBUFFER_CHUNKSIZE 16

typedef struct
{
	size_t len;
	size_t capacity;
	char *buf;
}
stringbuffer_t;

extern stringbuffer_t *stringbuffer_create(void);
extern void stringbuffer_destroy(stringbuffer_t *sb);
extern void stringbuffer_clear(stringbuffer_t *sb);
extern void stringbuffer_append_c(stringbuffer_t *sb, char c);
extern void stringbuffer_append(stringbuffer_t *sb, const char *s);
extern stringbuffer_t *stringbuffer_trim_whitespace(stringbuffer_t *sb);
extern char *stringbuffer_get_last_occurance(stringbuffer_t *sb, char c);
extern void stringbuffer_trim_newline(stringbuffer_t *sb);
extern const char *stringbuffer_getstring(stringbuffer_t *sb);
extern void stringbuffer_set(stringbuffer_t *dest, const char *s);
extern void stringbuffer_copy(stringbuffer_t *dest, stringbuffer_t *src);
extern void stringbuffer_replace_c(stringbuffer_t *sb, char c, char replace);
extern void stringbuffer_replace(stringbuffer_t *sb, const char *string, const char *replace);
extern void stringbuffer_align(stringbuffer_t *sb, int begin, int end);
extern void stringbuffer_avprintf_align(stringbuffer_t *sb, int start, int end, const char *fmt, va_list ap);
extern void stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...);
extern void stringbuffer_aprintf_align(stringbuffer_t *sb, int start, int end, const char *fmt, ...);
extern void stringbuffer_avprintf(stringbuffer_t *sb, const char *fmt, va_list ap);
extern int stringbuffer_marknewlines(stringbuffer_t *sb);
extern const char *stringbuffer_getnextline(stringbuffer_t *sb, const char *cptr);
extern int stringbuffer_getlen(stringbuffer_t *sb);
extern void stringbuffer_vasbappend(stringbuffer_t *sb, char *fmt, ... );
