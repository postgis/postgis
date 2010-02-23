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


#include "liblwgeom.h"

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

void stringbuffer_destroy(stringbuffer_t *sb)
{
	lwfree(sb->str);
	lwfree(sb);
}

void stringbuffer_clear(stringbuffer_t *sb)
{
	sb->str[0] = '\0';
	sb->length = 0;
	memset(sb->buffer,0,STRINGBUFFER_WORKSIZE);
}

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

void stringbuffer_append(stringbuffer_t *sb, const char *s)
{
	int slen = strlen(s); /* Length of string to append */
	int slen0 = slen + 1; /* Length including null terminator */

	stringbuffer_makeroom(sb, slen0);

	memcpy(sb->str + sb->length, s, slen0);

	sb->length += slen;
	sb->str[sb->length] = '\0';

}

const char *stringbuffer_getstring(stringbuffer_t *sb)
{
	return sb->str;
}

void stringbuffer_set(stringbuffer_t *sb, const char *s)
{
	stringbuffer_clear(sb); 
	stringbuffer_append(sb, s);
}

void stringbuffer_copy(stringbuffer_t *sb, stringbuffer_t *src)
{
	stringbuffer_set(sb, stringbuffer_getstring(src));
}

static void stringbuffer_avprintf(stringbuffer_t *sb, const char *fmt, va_list ap)
{
	int len = 0; /* Length of the output */
	int len0 = 0; /* Length of the output with null terminator */
	va_list ap2;
	va_list ap3;
	
	va_copy(ap2, ap);
	va_copy(ap3, ap);
	
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

void stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	stringbuffer_avprintf(sb, fmt, ap);
	va_end(ap);
}

void stringbuffer_vasbappend(stringbuffer_t *sb, const char *fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	stringbuffer_avprintf(sb, fmt, ap);
	va_end(ap);
}