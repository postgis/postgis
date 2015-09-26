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

#ifndef SHPCOMMON_H
#define SHPCOMMON_H

/* For internationalization */
#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define PACKAGE "shp2pgsql"
#else
#define _(String) String
#endif



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



typedef struct shp_connection_state
{
	/* PgSQL username to log in with */
	char *username;

	/* PgSQL password to log in with */
	char *password;

	/* PgSQL database to connect to */
	char *database;

	/* PgSQL port to connect to */
	char *port;

	/* PgSQL server to connect to */
	char *host;

} SHPCONNECTIONCONFIG;

/* External shared functions */
char *escape_connection_string(char *str);

/* Column map between pgsql and dbf */
typedef struct colmap_t {
	
	/* Column map pgfieldnames */
	char **pgfieldnames;
	
	/* Column map dbffieldnames */
	char **dbffieldnames;
	
	/* Number of entries within column map */
	int size;

} colmap;

/**
 * Read the content of filename into a symbol map 
 *
 * The content of the file is lines of two names separated by
 * white space and no trailing or leading space:
 *
 *    COLUMNNAME DBFFIELD1
 *    AVERYLONGCOLUMNNAME DBFFIELD2
 *
 *    etc.
 *
 * It is the reponsibility of the caller to reclaim the allocated space
 * as follows:
 *
 * free(map->colmap_pgfieldnames[]) to free the column names
 * free(map->colmap_dbffieldnames[]) to free the dbf field names
 *
 * TODO: provide a clean_colmap()
 *
 * @param filename name of the mapping file
 *
 * @param map container of colmap where the malloc'd
 *            symbol map will be stored.
 *
 * @param errbuf buffer to write error messages to
 *
 * @param errbuflen length of buffer to write error messages to
 *
 * @return 1 on success, 0 on error (and errbuf would be filled)
 */
int colmap_read(const char *fname, colmap *map, char *ebuf, size_t ebuflen);

void colmap_init(colmap *map);

void colmap_clean(colmap *map);

const char *colmap_dbf_by_pg(colmap *map, const char *pgname);

const char *colmap_pg_by_dbf(colmap *map, const char *dbfname);

char *codepage2encoding(const char *cpg);
char *encoding2codepage(const char *encoding);

#endif
