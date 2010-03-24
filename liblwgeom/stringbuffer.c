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
stringbuffer_t *stringbuffer_create(void)
{
	stringbuffer_t *sb;

	sb = lwalloc(sizeof(stringbuffer_t));
	sb->length = 0;
	sb->capacity = STRINGBUFFER_STARTSIZE;
	sb->str = lwalloc(sizeof(char) * STRINGBUFFER_STARTSIZE);
	memset(sb->str,0,STRINGBUFFER_STARTSIZE);
	memset(sb->buffer,0,STRINGBUFFER_WORKSIZE);
	return sb;
}

/**
* Free the stringbuffer_t and all memory managed within it.
*/
void stringbuffer_destroy(stringbuffer_t *sb)
{
	if( sb->str ) lwfree(sb->str);
	if( sb ) lwfree(sb);
}

/** 
* Reset the stringbuffer_t. Useful for starting a fresh string
* without the expense of freeing and re-allocating a new
* stringbuffer_t.
*/
void stringbuffer_clear(stringbuffer_t *sb)
{
	sb->str[0] = '\0';
	sb->length = 0;
	memset(sb->buffer,0,STRINGBUFFER_WORKSIZE);
}

/**
* If necessary, expand the stringbuffer_t internal buffer to accomodate the 
* specified additional size.
*/
static void stringbuffer_makeroom(stringbuffer_t *sb, size_t length_to_add)
{
	size_t reqd_capacity = sb->capacity; 

	while(reqd_capacity < (length_to_add + sb->length))
		reqd_capacity *= 2;

	if( reqd_capacity > sb->capacity )
	{
		sb->str = lwrealloc(sb->str, sizeof(char) * reqd_capacity);
		sb->capacity = reqd_capacity;
	}
}

/**
* Append the specified string to the stringbuffer_t.
*/
void stringbuffer_append(stringbuffer_t *sb, const char *s)
{
	int slen = strlen(s); /* Length of string to append */
	int slen0 = slen + 1; /* Length including null terminator */

	stringbuffer_makeroom(sb, slen0);

	memcpy(sb->str + sb->length, s, slen0);

	sb->length += slen;
	sb->str[sb->length] = '\0';

}

/**
* Returns a reference to the internal string being managed by
* the stringbuffer. The current string will be null-terminated
* within the internal string.
*/
const char *stringbuffer_getstring(stringbuffer_t *sb)
{
	return sb->str;
}

/**
* Returns a newly allocated string large enough to contain the 
* current state of the string. Caller is responsible for
* freeing the return value.
*/
char *stringbuffer_getstringcopy(stringbuffer_t *sb)
{
	char *rv;
	size_t size;
	if( sb->length <= 0 )
		return NULL;
	size = sb->length + 1;
	rv = lwalloc(size);
	memcpy(rv, sb->str, size);
	rv[sb->length] = '\0';
	return rv;
}

/**
* Returns the length of the current string, not including the 
* null terminator (same behavior as strlen()).
*/
int stringbuffer_getlength(stringbuffer_t *sb)
{
	return sb->length;
}

/**
* Clear the stringbuffer_t and re-start it with the specified string.
*/
void stringbuffer_set(stringbuffer_t *sb, const char *s)
{
	stringbuffer_clear(sb); 
	stringbuffer_append(sb, s);
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
* using the format and argument list provided.
*/
static void stringbuffer_avprintf(stringbuffer_t *sb, const char *fmt, va_list ap)
{
	int len = 0; /* Length of the output */
	int len0 = 0; /* Length of the output with null terminator */
	va_list ap2;
	
	va_copy(ap2, ap);
	
	/* Print to our static buffer */
	len = vsnprintf(sb->buffer, STRINGBUFFER_WORKSIZE, fmt, ap);
	len0 = len + 1;
	
	stringbuffer_makeroom(sb, len0);

	/* This output doesn't fit in our static buffer, allocate a dynamic one */
	if( len0 > STRINGBUFFER_WORKSIZE )
	{
		char *tmpstr = lwalloc(sizeof(char) * len0);
		vsnprintf(tmpstr, len0, fmt, ap2);
		memcpy(sb->str + sb->length, tmpstr, len0);
		lwfree(tmpstr);
	}
	/* Copy the result out of the static buffer */
	else
	{
		memcpy(sb->str + sb->length, sb->buffer, len0);
	}
	sb->length += len;
	sb->str[sb->length] = '\0';
}

/**
* Appends a formatted string to the current string buffer, 
* using the format and argument list provided.
*/
void stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	stringbuffer_avprintf(sb, fmt, ap);
	va_end(ap);
}

/**
* Synonym for stringbuffer_aprintf
* TODO: Remove this.
*/
void stringbuffer_vasbappend(stringbuffer_t *sb, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	stringbuffer_avprintf(sb, fmt, ap);
	va_end(ap);
}