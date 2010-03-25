/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2002 Thamer Alharbash
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 **********************************************************************/


#include "liblwgeom.h"

/**
* Allocate a new stringbuffer_t. Use stringbuffer_destroy to free.
*/
stringbuffer_t* stringbuffer_create(void)
{
	stringbuffer_t *s;

	s = lwalloc(sizeof(stringbuffer_t));
	s->str_start = lwalloc(sizeof(char) * STRINGBUFFER_STARTSIZE);
	s->str_end = s->str_start;
	s->capacity = STRINGBUFFER_STARTSIZE;
	memset(s->str_start,0,STRINGBUFFER_STARTSIZE);
	return s;
}

/**
* Free the stringbuffer_t and all memory managed within it.
*/
void stringbuffer_destroy(stringbuffer_t *s)
{
	if( s->str_start ) lwfree(s->str_start);
	if( s ) lwfree(s);
}

/** 
* Reset the stringbuffer_t. Useful for starting a fresh string
* without the expense of freeing and re-allocating a new
* stringbuffer_t.
*/
void stringbuffer_clear(stringbuffer_t *s)
{
	s->str_start[0] = '\0';
	s->str_end = s->str_start;
}

/**
* If necessary, expand the stringbuffer_t internal buffer to accomodate the 
* specified additional size.
*/
static void stringbuffer_makeroom(stringbuffer_t *s, size_t size_to_add)
{
	size_t current_size = (s->str_end - s->str_start);
	size_t capacity = s->capacity; 
	size_t required_size = current_size + size_to_add;

	while(capacity < required_size)
		capacity *= 2;

	if( capacity > s->capacity )
	{
		s->str_start = lwrealloc(s->str_start, sizeof(char) * capacity);
		s->capacity = capacity;
		s->str_end = s->str_start + current_size;
	}
}

/**
* Append the specified string to the stringbuffer_t.
*/
void stringbuffer_append(stringbuffer_t *s, const char *a)
{
	int alen = strlen(a); /* Length of string to append */
	int alen0 = alen + 1; /* Length including null terminator */

	stringbuffer_makeroom(s, alen0);
	memcpy(s->str_end, a, alen0);
	s->str_end += alen;
}

/**
* Returns a reference to the internal string being managed by
* the stringbuffer. The current string will be null-terminated
* within the internal string.
*/
const char* stringbuffer_getstring(stringbuffer_t *s)
{
	return s->str_start;
}

/**
* Returns a newly allocated string large enough to contain the 
* current state of the string. Caller is responsible for
* freeing the return value.
*/
char* stringbuffer_getstringcopy(stringbuffer_t *s)
{
	size_t size = (s->str_end - s->str_start) + 1;
	char *str = lwalloc(size);
	memcpy(str, s->str_start, size);
	str[size - 1] = '\0';
	return str;
}

/**
* Returns the length of the current string, not including the 
* null terminator (same behavior as strlen()).
*/
int stringbuffer_getlength(stringbuffer_t *s)
{
	return (s->str_end - s->str_start);
}

/**
* Clear the stringbuffer_t and re-start it with the specified string.
*/
void stringbuffer_set(stringbuffer_t *s, const char *str)
{
	stringbuffer_clear(s); 
	stringbuffer_append(s, str);
}

/** 
* Copy the contents of src into dst.
*/
void stringbuffer_copy(stringbuffer_t *dst, stringbuffer_t *src)
{
	stringbuffer_set(dst, stringbuffer_getstring(src));
}

/**
* Appends a formatted string to the current string buffer, 
* using the format and argument list provided. Returns -1 on error,
* check errno for reasons, documented in the printf man page.
*/
static int stringbuffer_avprintf(stringbuffer_t *s, const char *fmt, va_list ap)
{
	int maxlen = (s->capacity - (s->str_end - s->str_start));
	int len = 0; /* Length of the output */
	va_list ap2;
	
	/* Make a copy of the variadic arguments, in case we need to print twice */
	va_copy(ap2, ap);
	
	/* Print to our buffer */
	len = vsnprintf(s->str_end, maxlen, fmt, ap);

	/* Propogate any printing errors upwards (check errno for info) */
	if( len < 0 ) 
		return -1;

	/* If we had enough space, return the number of characters printed */
	if( len < maxlen ) 
	{
		s->str_end += len;
		return len;
	}

	/* We didn't have enough space! Expand the buffer. */
	stringbuffer_makeroom(s, len + 1);
	maxlen = (s->capacity - (s->str_end - s->str_start));
	
	/* Try to print a second time */
	len = vsnprintf(s->str_end, maxlen, fmt, ap2);

	/* Printing error or too long still? Error! */
	if( len < 0 || len >= maxlen ) 
		return -1;

	/* Move end pointer forward and return. */
	s->str_end += len;
	return len;
}

/**
* Appends a formatted string to the current string buffer, 
* using the format and argument list provided.
* Returns -1 on error, check errno for reasons, 
* as documented in the printf man page.
*/
int stringbuffer_aprintf(stringbuffer_t *s, const char *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = stringbuffer_avprintf(s, fmt, ap);
	va_end(ap);
	return r;
}

/**
* Synonym for stringbuffer_aprintf
* TODO: Remove this.
*/
int stringbuffer_vasbappend(stringbuffer_t *s, const char *fmt, ... )
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = stringbuffer_avprintf(s, fmt, ap);
	va_end(ap);
	return r;
}