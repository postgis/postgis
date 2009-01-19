/**********************************************************************
 * $Id: shp2pgsql-gui.c 3450 2008-12-18 20:42:09Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 OpenGeo.org
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/

#include <unistd.h>
#include "shp2pgsql-core.h"

static void
pcli_usage(char *me, int exitcode, FILE* out)
{
    fprintf(out, "RCSID: %s RELEASE: %s\n", RCSID, POSTGIS_VERSION);
	fprintf(out, "USAGE: %s [<options>] <shapefile> [<schema>.]<table>\n", me);
	fprintf(out, "OPTIONS:\n");
	fprintf(out, "  -s <srid>  Set the SRID field. If not specified it defaults to -1.\n");
	fprintf(out, "  (-d|a|c|p) These are mutually exclusive options:\n");
	fprintf(out, "      -d  Drops the table, then recreates it and populates\n");
	fprintf(out, "          it with current shape file data.\n");
	fprintf(out, "      -a  Appends shape file into current table, must be\n");
	fprintf(out, "          exactly the same table schema.\n");
	fprintf(out, "      -c  Creates a new table and populates it, this is the\n");
	fprintf(out, "          default if you do not specify any options.\n");
 	fprintf(out, "      -p  Prepare mode, only creates the table.\n");
	fprintf(out, "  -g <geometry_column> Specify the name of the geometry column\n");
	fprintf(out, "     (mostly useful in append mode).\n");
	fprintf(out, "  -D  Use postgresql dump format (defaults to sql insert statments.\n");
	fprintf(out, "  -k  Keep postgresql identifiers case.\n");
	fprintf(out, "  -i  Use int4 type for all integer dbf fields.\n");
	fprintf(out, "  -I  Create a GiST index on the geometry column.\n");
	fprintf(out, "  -S  Generate simple geometries instead of MULTI geometries.\n");
#ifdef HAVE_ICONV
	fprintf(out, "  -W <encoding> Specify the character encoding of Shape's\n");
	fprintf(out, "     attribute column. (default : \"ASCII\")\n");
#endif
	fprintf(out, "  -N <policy> Specify NULL geometries handling policy (insert,skip,abort)\n");
	fprintf(out, "  -n  Only import DBF file.\n");
    fprintf(out, "  -? Display this help screen\n");
	exit (exitcode);
}


static int
pcli_cmdline(int ARGC, char **ARGV)
{
	int c;
	int curindex=0;
	char  *ptr;
	extern char *optarg;
	extern int optind;

	if ( ARGC == 1 ) {
		pcli_usage(ARGV[0], 0, stdout);
	}

	while ((c = getopt(ARGC, ARGV, "kcdapDs:Sg:iW:wIN:n")) != EOF){
		switch (c) {
			case 'c':
				if (opt == ' ')
					 opt ='c';
				else
					 return 0;
				break;
			case 'd':
				if (opt == ' ')
					 opt ='d';
				else
					 return 0;
				break;
			case 'a':
				if (opt == ' ')
					 opt ='a';
				else
					 return 0;
				break;
			case 'p':
				if (opt == ' ')
					 opt ='p';
				else
					 return 0;
				break;
			case 'D':
				dump_format =1;
				break;
			case 'S':
				simple_geometries =1;
				break;
			case 's':
				if( optarg ) 
					(void)sscanf(optarg, "%d", &sr_id);
				else 
					pcli_usage(ARGV[0], 0, stdout); 
				break;
			case 'g':
				geom = optarg;
				break;
			case 'k':
				quoteidentifiers = 1;
				break;
			case 'i':
				forceint4 = 1;
				break;
			case 'I':
				createindex = 1;
				break;
			case 'n':
				readshape = 0;
				break;
			case 'W':
#ifdef HAVE_ICONV
				encoding = optarg;
#else
				fprintf(stderr, "WARNING: the -W switch will have no effect. UTF8 disabled at compile time\n");
#endif
				break;
			case 'N':
				switch (optarg[0])
				{	
					case 'a':
						null_policy = abort_on_null;
						break;
					case 'i':
						null_policy = insert_null;
						break;
					case 's':
						null_policy = skip_null;
						break;
					default:
						fprintf(stderr, "Unsupported NULL geometry handling policy.\nValid policies: insert, skip, abort\n");
						exit(1);
				}
				break;
			case '?':
				pcli_usage(ARGV[0], 0, stdout); 
			default:              
				return 0;
		}
	}

	if ( !sr_id ) sr_id = -1;

	if ( !geom ) geom = "the_geom";

	if ( opt==' ' ) opt = 'c';

	for (; optind < ARGC; optind++){
		if(curindex ==0){
			shp_file = ARGV[optind];
		}else if(curindex == 1){
			table = ARGV[optind];
			if ( (ptr=strchr(table, '.')) )
			{
				*ptr = '\0';
				schema = table;
				table = ptr+1;
			}
		}
		curindex++;
	}
	
	/*
	 * Third argument (if present) is supported for compatibility
	 * with old shp2pgsql versions taking also database name.
	 */
	if(curindex < 2 || curindex > 3){
		return 0;
	}

	/* 
	 * Transform table name to lower case unless asked
	 * to keep original case (we'll quote it later on)
	 */
	if ( ! quoteidentifiers )
	{
		LowerCase(table);
		if ( schema ) LowerCase(schema);
	}

	return 1;
}

int
main (int ARGC, char **ARGV)
{

	int rv = 0;

	/* Emit output to stdout/stderr, not a GUI */
	gui_mode = 0;

	/* Parse command line options and set globals */
	if ( ! pcli_cmdline(ARGC, ARGV) ) pcli_usage(ARGV[0], 2, stderr);

	/* Set record number to beginning of file, and translation stage to first one */
	cur_entity = -1;
	translation_stage = 1;
	
	rv = translation_start();
	if( ! rv ) return 1;
	while( translation_stage == 2 ) 
	{
		rv = translation_middle();
		if( ! rv ) return 1;
	}
	rv = translation_end();
	if( ! rv ) return 1;

	return 0;

}


/**********************************************************************
 * $Log$
 *
 **********************************************************************/
