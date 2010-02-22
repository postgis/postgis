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

#include "stringbuffer.h"

/* * * * * * * * * * * * *
 * raw string routines.  *
 * * * * * * * * * * * * */

/* just malloc out a string. */
static char *allocate_string(int len)
{
	char *s;
	s = malloc(sizeof(char) * len + 1); /* add for null termination. */
	s[len] = 0;
	return s;
}

/* extend a string. */
static char *extend_string(char *str, int cur_len, int ex_len)
{

	str = realloc(str, (cur_len * sizeof(char)) + (ex_len * sizeof(char)) + (1 * sizeof(char)));
	str[cur_len] = 0; /* make sure it's null terminated. */

	return str;
}

/* get a substring. */
static char *substring(char *begin, int len)
{
	char *new_string;

	new_string = allocate_string(len);
	memcpy(new_string, begin, len);
	new_string[len] = 0;

	return new_string;
}

/* FIXME: get rid of pesky strlen() */
/* used in aligning -- we try to get words up to end. */
static char *get_string_align(char *s, int end, int *len)
{
	char *cur_ptr;

	if (s == 0 || *s == 0) /* end of string or no string. */
		return NULL;

	/* if strlen is smaller than len go ahead and just return it. */
	if (strlen(s) < end)
	{
		*len = strlen(s);
		return strdup(s);
	}

	/* otherwise we need to hop to len */
	cur_ptr = &s[end - 1];

	/* now check to see if we have a whitespace behind cur_ptr
	 * if we do return at that point. */

	for (; cur_ptr != s; cur_ptr--)
	{
		if (*cur_ptr == ' ' || *cur_ptr == '\t')
		{
			/* copy here and return. */
			*len = (cur_ptr - s) + 1;
			return (substring(s, *len));
		}
	}

	/* keep walking till we find whitspace or end. */
	for (cur_ptr = &s[end - 1]; *cur_ptr != 0 && *cur_ptr != ' ' && *cur_ptr != '\t'; cur_ptr++);

	*len = (cur_ptr - s) + 1;
	return (substring(s, *len));
}

/* zap newlines by placing spaces in their place.
 * we use this before aligning. */
static void stringbuffer_zap_newline(stringbuffer_t *sb)
{

	stringbuffer_replace_c(sb, '\n', ' ');
	stringbuffer_replace_c(sb, '\r', ' ');
}

/* * * * * * * * * * * * * * *
 * stringbuffer routines.    *
 * * * * * * * * * * * * * * */

/* create a new stringbuffer */
stringbuffer_t *stringbuffer_create(void)
{
	stringbuffer_t *sb;

	sb = malloc(sizeof(stringbuffer_t));
	sb->len = 0;
	sb->capacity = 0;
	sb->buf = allocate_string(0);

	return sb;
}

/* destroy the stringbuffer */
void stringbuffer_destroy(stringbuffer_t *sb)
{
	free(sb->buf);
	free(sb);
}

/* clear a string. */
void stringbuffer_clear(stringbuffer_t *sb)
{
	sb->len = 0;
	sb->buf[0] = 0;
}

/* append character to stringbuffer */
void stringbuffer_append_c(stringbuffer_t *sb, char c)
{
	if (sb->capacity <= (sb->len))
	{
		sb->buf = extend_string(sb->buf, sb->len, STRINGBUFFER_CHUNKSIZE);
		sb->capacity += STRINGBUFFER_CHUNKSIZE;
	}

	sb->buf[sb->len] = c;
	sb->len++;
	sb->buf[sb->len] = 0;
}

/* append string to stringbuffer */
void stringbuffer_append(stringbuffer_t *sb, const char *s)
{
	int len = strlen(s);

	/* increase capacity if needed. */
	if (sb->capacity <= (len + sb->len))
	{

		/* if we're bigger than the chunksize then allocate len. */
		if (len > STRINGBUFFER_CHUNKSIZE)
		{

			sb->buf = extend_string(sb->buf, sb->capacity, len);
			sb->capacity += len;

		}
		else
		{

			/* otherwise allocate chunksize. */
			sb->buf = extend_string(sb->buf, sb->capacity, STRINGBUFFER_CHUNKSIZE);
			sb->capacity += STRINGBUFFER_CHUNKSIZE;
		}
	}

	/* copy new string into place: keep in mind we know all
	 * lengths so strcat() would be less effecient. */

	memcpy(&sb->buf[sb->len], s, len);

	sb->len += len;
	sb->buf[sb->len] = 0;

	return;
}

/* remove whitespace (including tabs) */
stringbuffer_t *stringbuffer_trim_whitespace(stringbuffer_t *sb)
{
	char *newbuf;
	int new_len;
	int i, j;

	if (sb->len == 0) /* empty string. */
		return sb;

	/* find beginning of string after tabs and whitespaces. */
	for (i = 0; i < sb->len && (sb->buf[i] == ' ' || sb->buf[i] == '\t'); i++);

	if (sb->buf[i] != '\0')
	{

		/* we do have whitespace in the beginning so find the end. */
		for (j = (sb->len -1); (sb->buf[j] == ' ' || sb->buf[j] == '\t'); j--);

		/* increment j since it's on the non whitespace character. */
		j++;

		/* create a new string. */
		new_len = j - i;
		newbuf = allocate_string(new_len);

		/* copy in. */
		memcpy(newbuf, &sb->buf[i], (j - i) * sizeof(char));
		newbuf[new_len] = 0;

		/* free up old. */
		free(sb->buf);

		/* set new. */
		sb->buf = newbuf;
		sb->len = new_len;
		sb->capacity = new_len;


	}
	else
	{

		/* zap beginning of string. since its all whitespace. */
		sb->buf[0] = 0;
		sb->len = 0;
	}

	return sb;
}

/* get the last occurance of a specific character. useful for slicing. */
char *stringbuffer_get_last_occurance(stringbuffer_t *sb, char c)
{
	char *ptr, *ptrend = NULL;
	int i;

	ptr = sb->buf;
	for (i = 0; i < sb->len; i++)
	{

		if (ptr[i] == c)
			ptrend = &ptr[i];
	}

	return ptrend;
}

/* remove the last newline character. */
void stringbuffer_trim_newline(stringbuffer_t *sb)
{
	char *ptr;

	ptr = stringbuffer_get_last_occurance(sb, '\n');
	if (ptr != NULL)
		*ptr = 0;


	ptr = stringbuffer_get_last_occurance(sb, '\r');
	if (ptr != NULL)
		*ptr = 0;


	sb->len = strlen(sb->buf);

	return;
}

/* return the C string from the buffer. */
const char *stringbuffer_getstring(stringbuffer_t *sb)
{
	return sb->buf;
}

/* set a string into a stringbuffer */
void stringbuffer_set(stringbuffer_t *dest, const char *s)
{
	stringbuffer_clear(dest); /* zap */
	stringbuffer_append(dest, s);
}

/* copy a stringbuffer into another stringbuffer */
void stringbuffer_copy(stringbuffer_t *dest, stringbuffer_t *src)
{
	stringbuffer_set(dest, stringbuffer_getstring(src));
}

/* replace in stringbuffer occurances of c with replace */
void stringbuffer_replace_c(stringbuffer_t *sb, char c, char replace)
{
	int i;

	for (i = 0; i < sb->len; i++)
	{
		if (sb->buf[i] == c)
			sb->buf[i] = replace;
	}

	return;
}

/* replace in stringbuffer occurances of c with replace */
void stringbuffer_replace(stringbuffer_t *sb, const char *string, const char *replace)
{
	char *ptr;
	int i;
	int str_len = strlen(string);
	stringbuffer_t *sb_replace;

	if (string[0] == 0)
		return; /* nothing to replace. */

	sb_replace = stringbuffer_create();
	ptr = sb->buf;

	for (i = 0; i < sb->len; i++)
	{

		if ((sb->len - i) < str_len)
		{

			/* copy in. */
			stringbuffer_copy(sb, sb_replace);

			/* append what's left. */
			stringbuffer_append(sb, &ptr[i]);

			/* free up. */
			stringbuffer_destroy(sb_replace);

			/* we're done. */
			return;
		}

		if (ptr[i] == string[0])
		{

			/* we know that we have at least enough to complete string. */
			if (!memcmp(&ptr[i], string, str_len))
			{

				/* we have a match, replace. */
				stringbuffer_append(sb_replace, replace);
				i += (str_len - 1);
				continue;
			}
		}

		stringbuffer_append_c(sb_replace, ptr[i]);
	}

	/* we're done: we should only get here if the last string to
	   be replaced ended the string itself. */

	/* copy in. */
	stringbuffer_copy(sb, sb_replace);

	/* free up. */
	stringbuffer_destroy(sb_replace);

	/* we're done. */
	return;

}

/* align a stringbuffer on begin and end columns. */
void stringbuffer_align(stringbuffer_t *sb, int begin, int end)
{
	char *ptr, *word_string;
	stringbuffer_t *aligned_string;
	int len, i;

	stringbuffer_zap_newline(sb);

	aligned_string = stringbuffer_create();
	ptr = sb->buf;

	while (1)
	{

		word_string = get_string_align(ptr, end, &len);

		if (word_string == NULL)
			break;

		ptr += len;

		for (i = 0; i < begin; i++)
			stringbuffer_append(aligned_string, " ");

		stringbuffer_append(aligned_string, word_string);
		stringbuffer_append(aligned_string, "\n");
		free(word_string);

	}

	stringbuffer_copy(sb, aligned_string);
	stringbuffer_destroy(aligned_string);

	return;
}

/* stringbuffer_*printf* these all use snprintf/snvprintf internally */

/* append vprintf with alignment. */
void stringbuffer_avprintf_align(stringbuffer_t *sb, int start, int end, const char *fmt, va_list ap)
{
	stringbuffer_t *tmp_sb;
	char *str;
	int total, len;

	/* our first malloc is bogus. */
	len = 1;
	str = malloc(sizeof(char) * len);
	total = vsnprintf(str, len, fmt, ap);

	/* total is the real length needed. */
	free(str);
	len = total + 1;
	str = malloc(sizeof(char) * len);
	vsnprintf(str, len, fmt, ap);

	/* now align if we want to align. */
	if (start != 0 && end != 0)
	{

		tmp_sb = stringbuffer_create();

		stringbuffer_append(tmp_sb, str);
		stringbuffer_align(tmp_sb, start, end);
		stringbuffer_append(sb, stringbuffer_getstring(tmp_sb));

		stringbuffer_destroy(tmp_sb);

	}
	else
	{
		stringbuffer_append(sb, str);
	}

	free(str);

	return;
}

/* append printf. */
void stringbuffer_aprintf(stringbuffer_t *sb, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	stringbuffer_avprintf(sb, fmt, ap);

	va_end(ap);
}

/* append printf with alignment. */
void stringbuffer_aprintf_align(stringbuffer_t *sb, int start, int end, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	stringbuffer_avprintf_align(sb, start, end, fmt, ap);

	va_end(ap);
}

/* append vprintf. */
void stringbuffer_avprintf(stringbuffer_t *sb, const char *fmt, va_list ap)
{
	stringbuffer_avprintf_align(sb, 0, 0, fmt, ap);
}

/* newline marking and sweeping. this wrecks the string inside
 * the stringbuffer. */

/* mark newlines to walk through.
 * our sentinel is the null terminator.
 * two null terminations marks the end of the string.
 * we're guaranteed it is a unique sentinel since
 * we never accept null terminators from outside sources
 * and never build our own strings (obviously!) with
 * null terminators inside of them.
 */
int stringbuffer_marknewlines(stringbuffer_t *sb)
{
	char *c;
	int newline_count = 0;

	/* first append one null termination to the end
	 * to act as a proper terminator. */
	stringbuffer_append_c(sb, 0);


	c = sb->buf;
	while (1)
	{

		if (*c == '\n')
		{
			newline_count++;
			*c = 0;
		}

		c++;
		if (*c == 0)
			break;
	}

	return newline_count; /* return our line count. */
}

/* called _after_ newlines are marked. */
const char *stringbuffer_getnextline(stringbuffer_t *sb, const char *cptr)
{
	const char *ptr;

	if (cptr == NULL)
	{

		/* get first line. */
		cptr = sb->buf;

	}
	else
	{

		for (ptr = cptr; *ptr != 0; ptr++);
		if (*ptr == 0 && *(ptr + 1) == 0)
		{
			return NULL;

		}
		else
		{
			cptr = ptr + 1;

		}
	}

	return cptr;
}

int stringbuffer_getlen(stringbuffer_t *sb)
{
	return sb->len;
}
