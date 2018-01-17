/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2014 Sandro Santilli <strk@kbt.io>
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

typedef struct
{
    int ldid;
    int cpg;
    char *desc;
    char *iconv;
    char *pg;
} code_page_entry;

static int num_code_pages = 60;

/* http://www.autopark.ru/ASBProgrammerGuide/DBFSTRUC.HTM */
/* http://resources.arcgis.com/fr/content/kbase?fa=articleShow&d=21106 */

static code_page_entry code_pages[] = {
    {0x01, 437, "U.S. MS-DOS", "CP437",""},
    {0x02, 850, "International MS-DOS", "CP850",""},
    {0x03, 1252, "Window ANSI", "WINDOWS-1252","WIN1252"},
    {0x08, 865, "Danish OEM", "CP865",""},
    {0x09, 437, "Dutch OEM", "CP437",""},
    {0x0A, 850, "Dutch OEM*", "CP850",""},
    {0x0B, 437, "Finnish OEM", "CP437",""},
    {0x0D, 437, "French OEM", "CP437",""},
    {0x0E, 850, "French OEM*", "CP850",""},
    {0x0F, 437, "German OEM", "CP437",""},
    {0x10, 850, "German OEM*", "CP850",""},
    {0x11, 437, "Italian OEM", "CP437",""},
    {0x12, 850, "Italian OEM*", "CP850",""},
    {0x13, 932, "Japanese Shift-JIS", "CP932","SJIS"},
    {0x14, 850, "Spanish OEM*", "CP850",""},
    {0x15, 437, "Swedish OEM", "CP437",""},
    {0x16, 850, "Swedish OEM*", "CP850",""},
    {0x17, 865, "Norwegian OEM", "CP865",""},
    {0x18, 437, "Spanish OEM", "CP865",""},
    {0x19, 437, "English OEM (Britain)", "CP437",""},
    {0x1A, 850, "English OEM (Britain)*", "CP850",""},
    {0x1B, 437, "English OEM (U.S.)", "CP437",""},
    {0x1C, 863, "French OEM (Canada)", "CP863",""},
    {0x1D, 850, "French OEM*", "CP850",""},
    {0x1F, 852, "Czech OEM", "CP852",""},
    {0x22, 852, "Hungarian OEM", "CP852",""},
    {0x23, 852, "Polish OEM", "CP852",""},
    {0x24, 860, "Portuguese OEM", "CP860",""},
    {0x25, 850, "Portuguese OEM*", "CP850",""},
    {0x26, 866, "Russian OEM", "WINDOWS-866","WIN866"},
    {0x37, 850, "English OEM (U.S.)*", "CP850",""},
    {0x40, 852, "Romanian OEM", "CP852",""},
    {0x4D, 936, "Chinese GBK (PRC)", "CP936",""},
    {0x4E, 949, "Korean (ANSI/OEM)", "CP949",""},
    {0x4F, 950, "Chinese Big 5 (Taiwan)", "CP950","BIG5"},
    {0x50, 874, "Thai (ANSI/OEM)", "WIN874",""},
    {0x57, 1252, "ANSI", "WINDOWS-1252",""},
    {0x58, 1252, "Western European ANSI", "WINDOWS-1252",""},
    {0x59, 1252, "Spanish ANSI", "WINDOWS-1252",""},
    {0x64, 852, "Eastern European MS-DOS", "CP852",""},
    {0x65, 866, "Russian MS-DOS", "CP866",""},
    {0x66, 865, "Nordic MS-DOS", "CP865",""},
    {0x67, 861, "Icelandic MS-DOS", "",""},
    {0x6A, 737, "Greek MS-DOS (437G)", "CP737",""},
    {0x6B, 857, "Turkish MS-DOS", "CP857",""},
    {0x6C, 863, "French-Canadian MS-DOS", "CP863",""},
    {0x78, 950, "Taiwan Big 5", "CP950",""},
    {0x79, 949, "Hangul (Wansung)", "CP949",""},
    {0x7A, 936, "PRC GBK", "CP936","GBK"},
    {0x7B, 932, "Japanese Shift-JIS", "CP932",""},
    {0x7C, 874, "Thai Windows/MS-DOS", "WINDOWS-874","WIN874"},
    {0x86, 737, "Greek OEM", "CP737",""},
    {0x87, 852, "Slovenian OEM", "CP852",""},
    {0x88, 857, "Turkish OEM", "CP857",""},
    {0xC8, 1250, "Eastern European Windows", "WINDOWS-1250","WIN1250"},
    {0xC9, 1251, "Russian Windows", "WINDOWS-1251","WIN1251"},
    {0xCA, 1254, "Turkish Windows", "WINDOWS-1254","WIN1254"},
    {0xCB, 1253, "Greek Windows", "WINDOWS-1253","WIN1253"},
    {0xCC, 1257, "Baltic Window", "WINDOWS-1257","WIN1257"},
    {0xFF, 65001, "UTF-8", "UTF-8","UTF8"}
};





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
	if (map != NULL){
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
}

const char *
colmap_dbf_by_pg(colmap *map, const char *pgname)
{
  int i;
  for (i=0; i<map->size; i++)
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
  for (i=0; i<map->size; i++)
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
*
* Return null on error (cannot allocate memory)
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
				int ret = asprintf(&codepage, "LDID/%d", code_pages[i].ldid);
				if ( ret == -1 ) return NULL; /* return null on error */
				return codepage;
			}
		}
	}

	/* OK, we give up, pretend it's UTF8 */
	return strdup("UTF-8");
}
