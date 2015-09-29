/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2014 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2010 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* This file contains functions that are shared between the loader and dumper */

#include <stdio.h>
#include <string.h>
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

void
colmap_init(colmap *map)
{
  map->size = 0;
  map->pgfieldnames = NULL;
  map->dbffieldnames = NULL;
}

void
colmap_clean(colmap *map)
{
  int i;
  if (map->size)
  {
    for (i = 0; i < map->size; i++)
    {
      if (map->pgfieldnames[i]) free(map->pgfieldnames[i]);
      if (map->dbffieldnames[i]) free(map->dbffieldnames[i]);
    }
    free(map->pgfieldnames);
    free(map->dbffieldnames);
  }
}

const char *
colmap_dbf_by_pg(colmap *map, const char *pgname)
{
  int i;
  for (i=0; i<map->size; ++i)
  {
    if (!strcasecmp(map->pgfieldnames[i], pgname))
    {
      return map->dbffieldnames[i];
    }
  }
  return NULL;
}

const char *
colmap_pg_by_dbf(colmap *map, const char *dbfname)
{
  int i;
  for (i=0; i<map->size; ++i)
  {
    if (!strcasecmp(map->dbffieldnames[i], dbfname))
    {
      return map->pgfieldnames[i];
    }
  }
  return NULL;
}

int
colmap_read(const char *filename, colmap *map, char *errbuf, size_t errbuflen)
{
  FILE *fptr;
  char linebuffer[1024];
  char *tmpstr;
  int curmapsize, fieldnamesize;
  
  /* Read column map file and load the colmap_dbffieldnames
   * and colmap_pgfieldnames arrays */
  fptr = fopen(filename, "r");
  if (!fptr)
  {
    /* Return an error */
    snprintf(errbuf, errbuflen, _("ERROR: Unable to open column map file %s"),
                     filename);
    return 0;
  }
  
  /* First count how many columns we have... */
  while (fgets(linebuffer, 1024, fptr) != NULL) ++map->size;
  
  /* Now we know the final size, allocate the arrays and load the data */
  fseek(fptr, 0, SEEK_SET);
  map->pgfieldnames = (char **)malloc(sizeof(char *) * map->size);
  map->dbffieldnames = (char **)malloc(sizeof(char *) * map->size);
  
  /* Read in a line at a time... */
  curmapsize = 0;
  while (fgets(linebuffer, 1024, fptr) != NULL)
  {
    /* Split into two separate strings: pgfieldname and dbffieldname */
    /* First locate end of first column (pgfieldname) */
    fieldnamesize = strcspn(linebuffer, "\t\n "); 
    tmpstr = linebuffer;

    /* Allocate memory and copy the string ensuring it is terminated */
    map->pgfieldnames[curmapsize] = malloc(fieldnamesize + 1);
    strncpy(map->pgfieldnames[curmapsize], tmpstr, fieldnamesize);
    map->pgfieldnames[curmapsize][fieldnamesize] = '\0';

    /* Now swallow up any whitespace */
    tmpstr = linebuffer + fieldnamesize;
    tmpstr += strspn(tmpstr, "\t\n "); 

    /* Finally locate end of second column (dbffieldname) */
    fieldnamesize = strcspn(tmpstr, "\t\n "); 

    /* Allocate memory and copy the string ensuring it is terminated */
    map->dbffieldnames[curmapsize] = malloc(fieldnamesize + 1);
    strncpy(map->dbffieldnames[curmapsize], tmpstr, fieldnamesize);
    map->dbffieldnames[curmapsize][fieldnamesize] = '\0';
    
    /* Error out if the dbffieldname is > 10 chars */
    if (strlen(map->dbffieldnames[curmapsize]) > 10)
    {
      snprintf(errbuf, errbuflen, _("ERROR: column map file specifies a DBF field name \"%s\" which is longer than 10 characters"), map->dbffieldnames[curmapsize]);
      return 0;
    }
    
    ++curmapsize;
  }

  fclose(fptr);

  /* Done; return success */
  return 1;
}

/*
* Code page info will come out of dbfopen as either a bare codepage number
* (e.g. 1256) or as "LDID/1234" from the DBF hreader. We want to look up 
* the equivalent iconv encoding string so we can use iconv to transcode
* the data into UTF8
*/
char *
codepage2encoding(const char *cpg)
{
    int cpglen;
    int is_ldid = 0;
    int num, i;
    
    /* Do nothing on nothing. */
    if ( ! cpg ) return NULL;
    
    /* Is this an LDID string? */
    /* If so, note it and move past the "LDID/" tag */
    cpglen = strlen(cpg);
    if ( strstr(cpg, "LDID/") )
    {
        if ( cpglen > 5 )
        {
            cpg += 5;
            is_ldid = 1;
        }
        else
        {
            return NULL;
        }
    }
    
    /* Read the number */
    num = atoi(cpg);
    
    /* Can we find this number in our lookup table? */
    for ( i = is_ldid ; i < num_code_pages; i++ )
    {
        if ( is_ldid )
        {
            if ( code_pages[i].ldid == num )
                return strdup(code_pages[i].iconv);
        }
        else
        {
            if ( code_pages[i].cpg == num )
                return strdup(code_pages[i].iconv);
        }
    }
    
    /* Didn't find a matching entry */
    return NULL;
    
}

/*
* In the case where data is coming out of the database in some wierd encoding
* we want to look up the appropriate code page entry to feed to DBFCreateEx
*/
char *
encoding2codepage(const char *encoding)
{
	int i;
	for ( i = 0; i < num_code_pages; i++ )
	{
		if ( strcasecmp(encoding, code_pages[i].pg) == 0 )
		{
			if ( code_pages[i].ldid == 0xFF )
			{
				return strdup("UTF-8");
			}
			else
			{
				char *codepage = NULL;
				asprintf(&codepage, "LDID/%d", code_pages[i].ldid);
				return codepage;
			}
		}
	}

	/* OK, we give up, pretend it's UTF8 */
	return strdup("UTF-8");
}
