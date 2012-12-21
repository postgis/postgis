/**********************************************************************
 * $Id: shpcommon.c 5646 2010-05-27 13:19:12Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* This file contains functions that are shared between the loader and dumper */

#include <stdlib.h>
#include "shpcommon.h"


/**
 * Escape strings that are to be used as part of a PostgreSQL connection string. If no 
 * characters require escaping, simply return the input pointer. Otherwise return a 
 * new allocated string.
 */
char *
escape_connection_string(char *str)
{
	/*
	 * Escape apostrophes and backslashes:
	 *   ' -> \'
	 *   \ -> \\
	 *
	 * 1. find # of characters
	 * 2. make new string
	 */

	char *result;
	char *ptr, *optr;
	int toescape = 0;
	size_t size;

	ptr = str;

	/* Count how many characters we need to escape so we know the size of the string we need to return */
	while (*ptr)
	{
		if (*ptr == '\'' || *ptr == '\\')
			toescape++;

		ptr++;
	}

	/* If we don't have to escape anything, simply return the input pointer */
	if (toescape == 0)
		return str;

	size = ptr - str + toescape + 1;
	result = calloc(1, size);
	optr = result;
	ptr = str;

	while (*ptr)
	{
		if (*ptr == '\'' || *ptr == '\\')
			*optr++ = '\\';

		*optr++ = *ptr++;
	}

	*optr = '\0';

	return result;
}
