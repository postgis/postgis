/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 OpenGeo.org
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/

#include "shp2pgsql-core.h"


static void
usage()
{
	printf("RCSID: %s RELEASE: %s\n", RCSID, POSTGIS_VERSION);
	printf("USAGE: shp2pgsql [<options>] <shapefile> [<schema>.]<table>\n");
	printf("OPTIONS:\n");
	printf("  -s <srid>  Set the SRID field. Defaults to -1.\n");
	printf("  (-d|a|c|p) These are mutually exclusive options:\n");
	printf("      -d  Drops the table, then recreates it and populates\n");
	printf("          it with current shape file data.\n");
	printf("      -a  Appends shape file into current table, must be\n");
	printf("          exactly the same table schema.\n");
	printf("      -c  Creates a new table and populates it, this is the\n");
	printf("          default if you do not specify any options.\n");
	printf("      -p  Prepare mode, only creates the table.\n");
	printf("  -g <geocolumn> Specify the name of the geometry/geography column\n");
	printf("     (mostly useful in append mode).\n");
	printf("  -D  Use postgresql dump format (defaults to SQL insert statments.\n");
	printf("  -G  Use geography type (requires lon/lat data).\n");
	printf("  -k  Keep postgresql identifiers case.\n");
	printf("  -i  Use int4 type for all integer dbf fields.\n");
	printf("  -I  Create a spatial index on the geocolumn.\n");
	printf("  -S  Generate simple geometries instead of MULTI geometries.\n");
	printf("  -W <encoding> Specify the character encoding of Shape's\n");
	printf("     attribute column. (default : \"WINDOWS-1252\")\n");
	printf("  -N <policy> NULL geometries handling policy (insert*,skip,abort)\n");
	printf("  -n  Only import DBF file.\n");
	printf("  -?  Display this help screen.\n");
}


int
main (int argc, char **argv)
{
	SHPLOADERCONFIG *config;
	SHPLOADERSTATE *state;
	char *header, *footer, *record;
	char c;
	int ret, i;


	/* If no options are specified, display usage */
	if (argc == 1)
	{
		usage();
		exit(0);
	}

	/* Parse command line options and set configuration */
	config = malloc(sizeof(SHPLOADERCONFIG));
	set_config_defaults(config);

	while ((c = pgis_getopt(argc, argv, "kcdapGDs:Sg:iW:wIN:n")) != EOF)
	{
		switch (c)
		{
		case 'c':
		case 'd':
		case 'a':
		case 'p':
			config->opt = c;
			break;

		case 'D':
			config->dump_format = 1;
			break;

		case 'G':
			config->geography = 1;
			break;

		case 'S':
			config->simple_geometries = 1;
			break;

		case 's':
			if (optarg)
			{
				sscanf(optarg, "%d", &(config->sr_id));
			}
			else
			{
				/* With -s, user must specify SRID */
				usage();
				exit(0);
			}
			break;

		case 'g':
			config->geom = optarg;
			break;

		case 'k':
			config->quoteidentifiers = 1;
			break;

		case 'i':
			config->forceint4 = 1;
			break;

		case 'I':
			config->createindex = 1;
			break;

		case 'w':
			config->hwgeom = 1;
			break;

		case 'n':
			config->readshape = 0;
			break;

		case 'W':
			config->encoding = optarg;
			break;

		case 'N':
			switch (optarg[0])
			{
			case 'a':
				config->null_policy = POLICY_NULL_ABORT;
				break;
			case 'i':
				config->null_policy = POLICY_NULL_INSERT;
				break;
			case 's':
				config->null_policy = POLICY_NULL_SKIP;
				break;
			default:
				fprintf(stderr, "Unsupported NULL geometry handling policy.\nValid policies: insert, skip, abort\n");
				exit(1);
			}
			break;

		case '?':
			usage();
			exit(0);

		default:
			usage();
			exit(0);
		}
	}

	/* Determine the shapefile name from the next argument, if no shape file, exit. */
	if (optind < argc)
	{
		config->shp_file = argv[optind];
		optind++;
	}
	else
	{
		usage();
		exit(0);
	}

	/* Determine the table and schema names from the next argument */
	if (optind < argc)
	{
		char *ptr;

		ptr = strchr(argv[optind], '.');

		/* Schema qualified table name */
		if (ptr)
		{
			config->schema = malloc(strlen(argv[optind]) + 1);
			snprintf(config->schema, ptr - argv[optind] + 1, "%s", argv[optind]);

			config->table = malloc(strlen(argv[optind]));
			snprintf(config->table, strlen(argv[optind]) - strlen(config->schema), "%s", ptr + 1);
		}
		else
		{
			config->table = malloc(strlen(argv[optind]) + 1);
			strcpy(config->table, argv[optind]);
		}
	}

	/* If the table parameter is not provided, use the shape file name as a proxy value.
	   Strip out the .shp and the leading path information first. */
	if ( config->shp_file && config->table == NULL)
	{
		char *shp_file = strdup(config->shp_file);
		char *ptr;
		for ( ptr = shp_file + strlen(shp_file); ptr >= shp_file; ptr-- )
		{
			if ( *ptr == '.' )
			{
				*ptr = '\0';
			}
			if ( *ptr == '/' || *ptr == '\\' )
			{
				ptr++;
				break;
			}
		}
		config->table = strdup(ptr);
		free(shp_file);
	}


	/* Transform table name to lower case if no quoting specified */
	if (!config->quoteidentifiers)
	{
		if ( config->table )
			strtolower(config->table);
		if ( config->schema )
			strtolower(config->schema);
	}

	/* Make the geocolumn name consistent with the load type (geometry or geography) */
	if ( config->geography )
	{
		if (config->geom) free(config->geom);
		config->geom = strdup(GEOGRAPHY_DEFAULT);
	}


	/* Create the shapefile state object */
	state = ShpLoaderCreate(config);

	/* Open the shapefile */
	ret = ShpLoaderOpenShape(state);
	if (ret != SHPLOADEROK)
	{
		fprintf(stderr, "%s\n", state->message);

		if (ret == SHPLOADERERR)
			exit(1);
	}

	/* If reading the whole shapefile, display its type */
	if (state->config->readshape)
	{
		fprintf(stderr, "Shapefile type: %s\n", SHPTypeName(state->shpfiletype));
		fprintf(stderr, "Postgis type: %s[%d]\n", state->pgtype, state->pgdims);
	}

	/* Print the header to stdout */
	ret = ShpLoaderGetSQLHeader(state, &header);
	if (ret != SHPLOADEROK)
	{
		fprintf(stderr, "%s\n", state->message);

		if (ret == SHPLOADERERR)
			exit(1);
	}

	printf("%s", header);
	free(header);

	/* If we are not in "prepare" mode, go ahead and write out the data. */
	if ( state->config->opt != 'p' )
	{

		/* If in COPY mode, output the COPY statement */
		if (state->config->dump_format)
		{
			ret = ShpLoaderGetSQLCopyStatement(state, &header);
			if (ret != SHPLOADEROK)
			{
				fprintf(stderr, "%s\n", state->message);

				if (ret == SHPLOADERERR)
					exit(1);
			}

			printf("%s", header);
			free(header);
		}

		/* Main loop: iterate through all of the records and send them to stdout */
		for (i = 0; i < ShpLoaderGetRecordCount(state); i++)
		{
			ret = ShpLoaderGenerateSQLRowStatement(state, i, &record);

			switch (ret)
			{
			case SHPLOADEROK:
				/* Simply display the geometry */
				printf("%s\n", record);
				free(record);
				break;

			case SHPLOADERERR:
				/* Display the error message then stop */
				fprintf(stderr, "%s\n", state->message);
				exit(1);
				break;

			case SHPLOADERWARN:
				/* Display the warning, but continue */
				fprintf(stderr, "%s\n", state->message);
				printf("%s\n", record);
				free(record);
				break;

			case SHPLOADERRECDELETED:
				/* Record is marked as deleted - ignore */
				break;

			case SHPLOADERRECISNULL:
				/* Record is NULL and should be ignored according to NULL policy */
				break;
			}
		}

		/* If in COPY mode, terminate the COPY statement */
		if (state->config->dump_format)
			printf("\\.\n");

	}

	/* Print the footer to stdout */
	ret = ShpLoaderGetSQLFooter(state, &footer);
	if (ret != SHPLOADEROK)
	{
		fprintf(stderr, "%s\n", state->message);

		if (ret == SHPLOADERERR)
			exit(1);
	}

	printf("%s", footer);
	free(footer);


	/* Free the state object */
	ShpLoaderDestroy(state);

	/* Free configuration variables */
	if (config->schema)
		free(config->schema);
	if (config->table)
		free(config->table);
	free(config);

	return 0;
}
