/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* For internationalization */
#ifdef USE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#define PACKAGE "shp2pgsql"
#else
#define _(String) String
#endif

/* Standard headers */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iconv.h>

#include "shapefil.h"
#include "shpcommon.h"
#include "getopt.h"


#include "../liblwgeom/liblwgeom.h"

#define RCSID "$Id$"


/* Loader policy constants */
#define POLICY_NULL_ABORT 	0x0
#define POLICY_NULL_INSERT 	0x1
#define POLICY_NULL_SKIP	0x2


/* Error message handling */
#define SHPLOADERMSGLEN		1024

/* Error status codes */
#define SHPLOADEROK		-1
#define SHPLOADERERR		0
#define SHPLOADERWARN		1

/* Record status codes */
#define SHPLOADERRECDELETED	2
#define SHPLOADERRECISNULL	3

/*
 * Shapefile (dbf) field name are at most 10chars + 1 NULL.
 * Postgresql field names are at most 63 bytes + 1 NULL.
 */
#define MAXFIELDNAMELEN 64
#define MAXVALUELEN 1024

/*
 * Default geometry column name
 */
#define GEOMETRY_DEFAULT "geom"
#define GEOGRAPHY_DEFAULT "geog"

/*
 * Default character encoding
 */
#define ENCODING_DEFAULT "UTF-8"

/*
 * Structure to hold the loader configuration options 
 */
typedef struct shp_loader_config
{
	/* load mode: c = create, d = delete, a = append, p = prepare */
	char opt;

	/* table to load into */
	char *table;

	/* schema to load into */
	char *schema;

	/* geometry column name to use */
	char *geom; 

	/* the shape file (without the .shp extension) */
	char *shp_file;

	/* 0 = SQL inserts, 1 = dump */
	int dump_format;

	/* 0 = MULTIPOLYGON/MULTILINESTRING, 1 = force to POLYGON/LINESTRING */
	int simple_geometries;
	
	/* 0 = geometry, 1 = geography */
	int geography;

	/* 0 = columnname, 1 = "columnName" */
	int quoteidentifiers;

	/* 0 = allow int8 fields, 1 = no int8 fields */
	int forceint4;

	/* 0 = no index, 1 = create index after load */
	int createindex;

	/* 0 = load DBF file only, 1 = load everything */
	int readshape;

	/* iconv encoding name */
	char *encoding;

	/* tablespace name for the table */
	char *tablespace;

	/* tablespace name for the indexes */
	char *idxtablespace;

	/* how to handle nulls */
	int null_policy;

	/* SRID specified */
	int sr_id;

	/* 0 = new style (PostGIS 1.x) geometries, 1 = old style (PostGIS 0.9.x) geometries */
	int hwgeom;

} SHPLOADERCONFIG;


/*
 * Structure to holder the current loader state 
 */
typedef struct shp_loader_state
{
	/* Configuration for this state */
	SHPLOADERCONFIG *config;

	/* Shapefile handle */
	SHPHandle hSHPHandle;
	
	/* Shapefile type */
	int shpfiletype;

	/* Data file handle */
	DBFHandle hDBFHandle;

	/* Number of rows in the shapefile */
	int num_entities;

	/* Number of fields in the shapefile */
	int num_fields;

	/* Number of rows in the DBF file */
	int num_records;

	/* Pointer to an array of field names */
	char **field_names;

	/* Field type */
	DBFFieldType *types;

	/* Arrays of field widths and precisions */
	int *widths;
	int *precisions;

	/* String containing colume name list in the form "(col1, col2, col3 ... , colN)" */
	char *col_names;

	/* String containing the PostGIS geometry type, e.g. POINT, POLYGON etc. */
	char *pgtype;

	/* PostGIS geometry type (numeric version) */
	uint32 wkbtype;

	/* Number of dimensions to output */
	int pgdims;

	/* 0 = simple geometry, 1 = multi geometry */
	int istypeM;

	/* Last (error) message */
	char message[SHPLOADERMSGLEN];

} SHPLOADERSTATE;


/* Externally accessible functions */
void strtolower(char *s);
void vasbappend(stringbuffer_t *sb, char *fmt, ... );
void set_config_defaults(SHPLOADERCONFIG *config);

SHPLOADERSTATE *ShpLoaderCreate(SHPLOADERCONFIG *config);
int ShpLoaderOpenShape(SHPLOADERSTATE *state);
int ShpLoaderGetSQLHeader(SHPLOADERSTATE *state, char **strheader);
int ShpLoaderGetSQLCopyStatement(SHPLOADERSTATE *state, char **strheader);
int ShpLoaderGetRecordCount(SHPLOADERSTATE *state);
int ShpLoaderGenerateSQLRowStatement(SHPLOADERSTATE *state, int item, char **strrecord);
int ShpLoaderGetSQLFooter(SHPLOADERSTATE *state, char **strfooter);
void ShpLoaderDestroy(SHPLOADERSTATE *state);
