/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 *
 * PostGIS to Shapefile converter
 *
 * Original Author: Jeff Lounsbury, jeffloun@refractions.net
 *
 * Maintainer: Sandro Santilli, strk@refractions.net
 *
 **********************************************************************/

static char rcsid[] =
  "$Id$";

#include "../postgis_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
/* Solaris9 does not provide stdint.h */
/* #include <stdint.h> */
#include <inttypes.h>
#include "libpq-fe.h"
#include "shapefil.h"
#include "getopt.h"
#include <sys/types.h> // for getpid()
#include <unistd.h> // for getpid()

#ifdef __CYGWIN__
#include <sys/param.h>       
#endif

#include "../liblwgeom/liblwgeom.h"

/*
 * Verbosity:
 *   set to 1 to see record fetching progress
 *   set to 2 to see also shapefile creation progress
 *   set to 3 for debugging
 */
#define VERBOSE 1

/* Maximum DBF field width (according to ARCGIS) */
#define MAX_DBF_FIELD_SIZE 255

typedef unsigned char byte;

/* Global data */
PGconn *conn;
char conn_string[2048];
int rowbuflen;
char temptablename[256];
char *geo_col_name, *table, *shp_file, *schema, *usrquery;
int type_ary[256];
char *main_scan_query;
DBFHandle dbf;
SHPHandle shp;
int geotype;
int outshptype;
char outtype;
int dswitchprovided;
int includegid;
int unescapedattrs;
int binary;
int keep_fieldname_case;
int big_endian = 0;
int pgis_major_version;

/* Prototypes */
int getMaxFieldSize(PGconn *conn, char *schema, char *table, char *fname);
int parse_commandline(int ARGC, char **ARGV);
void usage(char* me, int exitstatus, FILE* out);
char *getTableOID(char *schema, char *table);
int addRecord(PGresult *res, int residx, int row);
int initShapefile(char *shp_file, PGresult *res);
int initialize(void);
int getGeometryOID(PGconn *conn);
int getGeometryType(char *schema, char *table, char *geo_col_name);
int getGeometryMaxDims(char *schema, char *table, char *geo_col_name);
char *shapetypename(int num);
int reverse_points(int num_points, double *x, double *y, double *z, double *m);
int is_clockwise(int num_points,double *x,double *y,double *z);
int is_bigendian(void);
SHPObject * shape_creator_wrapper_WKB(char *hexwkb, int idx);
SHPObject * create_point(LWPOINT *lwpoint);
SHPObject * create_multipoint(LWMPOINT *lwmultipoint);
SHPObject * create_polygon(LWPOLY *lwpolygon);
SHPObject * create_multipolygon(LWMPOLY *lwmultipolygon);
SHPObject * create_linestring(LWLINE *lwlinestring);
SHPObject * create_multilinestring(LWMLINE *lwmultilinestring);
int get_postgis_major_version(void);
static void parse_table(char *spec);
static int create_usrquerytable(void);
static const char *nullDBFValue(char fieldType);
int projFileCreate(const char * pszFilename, char *schema, char *table, char *geo_col_name);
/* 
 * Make appropriate formatting of a DBF value based on type.
 * Might return untouched input or pointer to static private 
 * buffer: use return value right away.
 */
static const char * goodDBFValue(const char *in, char fieldType);

/* Binary to hexewkb conversion function */
char *convert_bytes_to_hex(uchar *ewkb, size_t size);


/* liblwgeom allocator callback - install the defaults (malloc/free/stdout/stderr) */
void lwgeom_init_allocators()
{
	lwgeom_install_default_allocators();
}

static void exit_nicely(PGconn *conn, int code)
{
	PQfinish(conn);
	exit(code);
}

int
main(int ARGC, char **ARGV)
{
	char *query=NULL;
	int row;
	PGresult *res;
	char fetchquery[256];

	dbf=NULL;
	shp=NULL;
	geotype=-1;
	table = NULL;
	schema = NULL;
	usrquery = NULL;
	geo_col_name = NULL;
	shp_file = NULL;
	main_scan_query = NULL;
	rowbuflen=100;
	outtype = 's';
	dswitchprovided = 0;
	includegid=0;
	unescapedattrs=0;
	binary = 0;
	keep_fieldname_case = 0;
	conn_string[0] = '\0';
#ifdef DEBUG
	FILE *debug;
#endif

	if ( getenv("ROWBUFLEN") ) rowbuflen=atoi(getenv("ROWBUFLEN"));

	/*
	 * Make sure dates are returned in ISO
	 * style (YYYY-MM-DD).
	 * This is to allow goodDBFValue() function 
	 * to successfully extract YYYYMMDD format
	 * expected in shapefile's dbf file.
	 */
	putenv("PGDATESTYLE=ISO");

	if ( ! parse_commandline(ARGC, ARGV) ) {
                printf("\n**ERROR** invalid option or command parameters\n\n");
		usage(ARGV[0], 2, stderr);
	}

	/* Use table name as shapefile name */
        if(shp_file == NULL) shp_file = table;

	/* Make a connection to the specified database, and exit on failure */
	conn = PQconnectdb(conn_string);
	if (PQstatus(conn) == CONNECTION_BAD) {
		printf( "%s", PQerrorMessage(conn));
		exit_nicely(conn, 1);
	}

	/* Create temporary table for user query */
	if ( usrquery ) {
		if ( ! create_usrquerytable() ) {
			exit(2);
		}
	}

#ifdef DEBUG
	debug = fopen("/tmp/trace.out", "w");
	PQtrace(conn, debug);
#endif	 /* DEBUG */


	/* Initialize shapefile and database infos */
	fprintf(stdout, "Initializing... "); fflush(stdout);
	if ( ! initialize() ) exit_nicely(conn, 1);
	fprintf(stdout, "Done (postgis major version: %d).\n",
		pgis_major_version); 

	if ( pgis_major_version > 0 && dswitchprovided )
	{
		printf("WARNING: -d switch is useless when dumping from postgis-1.0.0+\n");
	}


	printf("Output shape: %s\n", shapetypename(outshptype));


	/*
	 * Begin the transaction
	 * (a cursor can only be defined inside a transaction block)
	 */
	res=PQexec(conn, "BEGIN");
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		printf( "%s", PQerrorMessage(conn));
		exit_nicely(conn, 1);
	}
	PQclear(res);

	/*
	 * Declare a cursor for the main scan query
	 * as set by the initializer function.
	 */
	query = (char *)malloc(strlen(main_scan_query)+256);
	if ( binary ) {
		sprintf(query, "DECLARE cur BINARY CURSOR FOR %s",
				main_scan_query);
	} else {
		sprintf(query, "DECLARE cur CURSOR FOR %s", main_scan_query);
	}

	LWDEBUGF(4, "QUERY: %s\n", query);

	free(main_scan_query);
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		printf( "MainScanQuery: %s", PQerrorMessage(conn));
		exit_nicely(conn, 1);
	}
	PQclear(res);

	/* Set the fetch query */
	sprintf(fetchquery, "FETCH %d FROM cur", rowbuflen);

	fprintf(stdout, "Dumping: "); fflush(stdout);

	/*
	 * Main scan
	 */
	row=0;
	while(1)
	{
		int i;

		/* Fetch next record buffer from cursor */
		fprintf(stdout, "X"); fflush(stdout);

		LWDEBUGF(4, "Fetch query: %s", fetchquery);

		res = PQexec(conn, fetchquery);
		if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
			printf( "RecordFetch: %s",
				PQerrorMessage(conn));
			exit_nicely(conn, 1);
		}

		/* No more rows, break the loop */
		if ( ! PQntuples(res) ) {
			PQclear(res);
			break;
		}

		for(i=0; i<PQntuples(res); i++)
		{
			/* Add record in all output files */
			if ( ! addRecord(res, i, row) ) exit_nicely(conn, 1);
			row++; 
		}

		LWDEBUG(4, "End of result, clearing...");

		PQclear(res);

		LWDEBUG(4, "Done.\n");
	}
	printf(" [%d rows].\n", row);

	if (dbf) DBFClose(dbf);
	if (shp) SHPClose(shp);


#ifdef DEBUG
	fclose(debug);
#endif	 /* DEBUG */

	exit_nicely(conn, 0);
	return 0;
}


SHPObject *
create_point(LWPOINT *lwpoint)
{
	SHPObject *obj;
	POINT4D p4d;

	double *xpts, *ypts, *zpts, *mpts;

	/* Allocate storage for points */
	xpts = malloc(sizeof(double));
	ypts = malloc(sizeof(double));
	zpts = malloc(sizeof(double));
	mpts = malloc(sizeof(double));

	/* Grab the point: note getPoint4d will correctly handle
	the case where the POINTs don't contain Z or M coordinates */
	p4d = getPoint4d(lwpoint->point, 0);

	xpts[0] = p4d.x;
	ypts[0] = p4d.y;
	zpts[0] = p4d.z;
	mpts[0] = p4d.m;

	LWDEBUGF(4, "Point: %g %g %g %g", xpts[0], ypts[0], zpts[0], mpts[0]);

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL, 1, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);

	return obj;
}

SHPObject *
create_multipoint(LWMPOINT *lwmultipoint)
{
	SHPObject *obj;
	POINT4D p4d;
	int i;

	double *xpts, *ypts, *zpts, *mpts;

	/* Allocate storage for points */
	xpts = malloc(sizeof(double) * lwmultipoint->ngeoms);
	ypts = malloc(sizeof(double) * lwmultipoint->ngeoms);
	zpts = malloc(sizeof(double) * lwmultipoint->ngeoms);
	mpts = malloc(sizeof(double) * lwmultipoint->ngeoms);

	/* Grab the points: note getPoint4d will correctly handle
	the case where the POINTs don't contain Z or M coordinates */
	for (i = 0; i < lwmultipoint->ngeoms; i++)
	{
		p4d = getPoint4d(lwmultipoint->geoms[i]->point, 0);

		xpts[i] = p4d.x;
		ypts[i] = p4d.y;
		zpts[i] = p4d.z;
		mpts[i] = p4d.m;

		LWDEBUGF(4, "MultiPoint %d - Point: %g %g %g %g", i, xpts[i], ypts[i], zpts[i], mpts[i]);
	}

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL, lwmultipoint->ngeoms, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);

	return obj;
}

SHPObject *
create_polygon(LWPOLY *lwpolygon)
{
	SHPObject *obj;
	POINT4D p4d;
	int i, j;

	double *xpts, *ypts, *zpts, *mpts;

	int *shpparts, shppointtotal = 0, shppoint = 0;

	/* Allocate storage for ring pointers */
	shpparts = malloc(sizeof(int) * lwpolygon->nrings);

	/* First count through all the points in each ring so we now how much memory is required */
	for (i = 0; i < lwpolygon->nrings; i++)
		shppointtotal += lwpolygon->rings[i]->npoints;

	/* Allocate storage for points */
	xpts = malloc(sizeof(double) * shppointtotal);
	ypts = malloc(sizeof(double) * shppointtotal);
	zpts = malloc(sizeof(double) * shppointtotal);
	mpts = malloc(sizeof(double) * shppointtotal);

	LWDEBUGF(4, "Total number of points: %d", shppointtotal);

	/* Iterate through each ring setting up shpparts to point to the beginning of each ring */
	for (i = 0; i < lwpolygon->nrings; i++)
	{
		/* For each ring, store the integer coordinate offset for the start of each ring */
		shpparts[i] = shppoint;

		for (j = 0; j < lwpolygon->rings[i]->npoints; j++)
		{
			p4d = getPoint4d(lwpolygon->rings[i], j);
	
			xpts[shppoint] = p4d.x;
			ypts[shppoint] = p4d.y;
			zpts[shppoint] = p4d.z;
			mpts[shppoint] = p4d.m;
	
			LWDEBUGF(4, "Polygon Ring %d - Point: %g %g %g %g", i, xpts[shppoint], ypts[shppoint], zpts[shppoint], mpts[shppoint]);
			
			/* Increment the point counter */
			shppoint++;
		}

		/*
		 * First ring should be clockwise,
		 * other rings should be counter-clockwise
		 */
		if ( i == 0 ) {
			if ( ! is_clockwise(lwpolygon->rings[i]->npoints, 
				&xpts[shpparts[i]], &ypts[shpparts[i]], NULL) )
			{
				LWDEBUG(4, "Outer ring not clockwise, forcing clockwise\n");

				reverse_points(lwpolygon->rings[i]->npoints, 
					&xpts[shpparts[i]], &ypts[shpparts[i]], 
					&zpts[shpparts[i]], &mpts[shpparts[i]]);
			}
		} 
		else 
		{
			if ( is_clockwise(lwpolygon->rings[i]->npoints,
				&xpts[shpparts[i]], &ypts[shpparts[i]], NULL) )
			{
				LWDEBUGF(4, "Inner ring %d not counter-clockwise, forcing counter-clockwise\n", i);

				reverse_points(lwpolygon->rings[i]->npoints,
					&xpts[shpparts[i]], &ypts[shpparts[i]], 
					&zpts[shpparts[i]], &mpts[shpparts[i]]);
			}
		}
	}

	obj = SHPCreateObject(outshptype, -1, lwpolygon->nrings, shpparts, NULL, shppointtotal, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);
	free(shpparts);

	return obj;
}

SHPObject *
create_multipolygon(LWMPOLY *lwmultipolygon)
{
	SHPObject *obj;
	POINT4D p4d;
	int i, j, k;

	double *xpts, *ypts, *zpts, *mpts;

	int *shpparts, shppointtotal = 0, shppoint = 0, shpringtotal = 0, shpring = 0;

	/* NOTE: Multipolygons are stored in shapefiles as Polygon* shapes with multiple outer rings */

	/* First count through each ring of each polygon so we now know much memory is required */
	for (i = 0; i < lwmultipolygon->ngeoms; i++)
	{
		for (j = 0; j < lwmultipolygon->geoms[i]->nrings; j++)
		{
			shpringtotal++;
			shppointtotal += lwmultipolygon->geoms[i]->rings[j]->npoints;
		}
	}

	/* Allocate storage for ring pointers */
	shpparts = malloc(sizeof(int) * shpringtotal);

	/* Allocate storage for points */
	xpts = malloc(sizeof(double) * shppointtotal);
	ypts = malloc(sizeof(double) * shppointtotal);
	zpts = malloc(sizeof(double) * shppointtotal);
	mpts = malloc(sizeof(double) * shppointtotal);

	LWDEBUGF(4, "Total number of rings: %d   Total number of points: %d", shpringtotal, shppointtotal);

	/* Iterate through each ring of each polygon in turn */
	for (i = 0; i < lwmultipolygon->ngeoms; i++)
	{
		for (j = 0; j < lwmultipolygon->geoms[i]->nrings; j++)
		{
			/* For each ring, store the integer coordinate offset for the start of each ring */
			shpparts[shpring] = shppoint;
	
			LWDEBUGF(4, "Ring offset: %d", shpring);

			for (k = 0; k < lwmultipolygon->geoms[i]->rings[j]->npoints; k++)
			{
				p4d = getPoint4d(lwmultipolygon->geoms[i]->rings[j], k);
		
				xpts[shppoint] = p4d.x;
				ypts[shppoint] = p4d.y;
				zpts[shppoint] = p4d.z;
				mpts[shppoint] = p4d.m;
		
				LWDEBUGF(4, "MultiPolygon %d Polygon Ring %d - Point: %g %g %g %g", i, j, xpts[shppoint], ypts[shppoint], zpts[shppoint], mpts[shppoint]);
				
				/* Increment the point counter */
				shppoint++;
			}

			/*
			* First ring should be clockwise,
			* other rings should be counter-clockwise
			*/
			if ( j == 0 ) {
				if ( ! is_clockwise(lwmultipolygon->geoms[i]->rings[j]->npoints, 
					&xpts[shpparts[shpring]], &ypts[shpparts[shpring]], NULL) )
				{
					LWDEBUG(4, "Outer ring not clockwise, forcing clockwise\n");
	
					reverse_points(lwmultipolygon->geoms[i]->rings[j]->npoints, 
						&xpts[shpparts[shpring]], &ypts[shpparts[shpring]], 
						&zpts[shpparts[shpring]], &mpts[shpparts[shpring]]);
				}
			} 
			else 
			{
				if ( is_clockwise(lwmultipolygon->geoms[i]->rings[j]->npoints,
					&xpts[shpparts[shpring]], &ypts[shpparts[shpring]], NULL) )
				{
					LWDEBUGF(4, "Inner ring %d not counter-clockwise, forcing counter-clockwise\n", i);
	
					reverse_points(lwmultipolygon->geoms[i]->rings[j]->npoints,
						&xpts[shpparts[shpring]], &ypts[shpparts[shpring]], 
						&zpts[shpparts[shpring]], &mpts[shpparts[shpring]]);
				}
			}

			/* Increment the ring counter */
			shpring++;
		}
	}

	obj = SHPCreateObject(outshptype, -1, shpringtotal, shpparts, NULL, shppointtotal, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);
	free(shpparts);

	return obj;
}

SHPObject *
create_linestring(LWLINE *lwlinestring)
{
	SHPObject *obj;
	POINT4D p4d;
	int i;

	double *xpts, *ypts, *zpts, *mpts;

	/* Allocate storage for points */
	xpts = malloc(sizeof(double) * lwlinestring->points->npoints);
	ypts = malloc(sizeof(double) * lwlinestring->points->npoints);
	zpts = malloc(sizeof(double) * lwlinestring->points->npoints);
	mpts = malloc(sizeof(double) * lwlinestring->points->npoints);

	/* Grab the points: note getPoint4d will correctly handle
	the case where the POINTs don't contain Z or M coordinates */
	for (i = 0; i < lwlinestring->points->npoints; i++)
	{
		p4d = getPoint4d(lwlinestring->points, i);

		xpts[i] = p4d.x;
		ypts[i] = p4d.y;
		zpts[i] = p4d.z;
		mpts[i] = p4d.m;

		LWDEBUGF(4, "Linestring - Point: %g %g %g %g", i, xpts[i], ypts[i], zpts[i], mpts[i]);
	}

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL, lwlinestring->points->npoints, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);

	return obj;
}

SHPObject *
create_multilinestring(LWMLINE *lwmultilinestring)
{
	SHPObject *obj;
	POINT4D p4d;
	int i, j;

	double *xpts, *ypts, *zpts, *mpts;

	int *shpparts, shppointtotal = 0, shppoint = 0;

	/* Allocate storage for ring pointers */
	shpparts = malloc(sizeof(int) * lwmultilinestring->ngeoms);

	/* First count through all the points in each linestring so we now how much memory is required */
	for (i = 0; i < lwmultilinestring->ngeoms; i++)
		shppointtotal += lwmultilinestring->geoms[i]->points->npoints;

	LWDEBUGF(3, "Total number of points: %d", shppointtotal);

	/* Allocate storage for points */
	xpts = malloc(sizeof(double) * shppointtotal);
	ypts = malloc(sizeof(double) * shppointtotal);
	zpts = malloc(sizeof(double) * shppointtotal);
	mpts = malloc(sizeof(double) * shppointtotal);

	/* Iterate through each linestring setting up shpparts to point to the beginning of each line */
	for (i = 0; i < lwmultilinestring->ngeoms; i++)
	{
		/* For each linestring, store the integer coordinate offset for the start of each line */
		shpparts[i] = shppoint;

		for (j = 0; j < lwmultilinestring->geoms[i]->points->npoints; j++)
		{
			p4d = getPoint4d(lwmultilinestring->geoms[i]->points, j);
	
			xpts[shppoint] = p4d.x;
			ypts[shppoint] = p4d.y;
			zpts[shppoint] = p4d.z;
			mpts[shppoint] = p4d.m;

			LWDEBUGF(4, "Linestring %d - Point: %g %g %g %g", i, xpts[shppoint], ypts[shppoint], zpts[shppoint], mpts[shppoint]);

			/* Increment the point counter */
			shppoint++;
		}
	}

	obj = SHPCreateObject(outshptype, -1, lwmultilinestring->ngeoms, shpparts, NULL, shppoint, xpts, ypts, zpts, mpts);

	free(xpts); free(ypts); free(zpts); free(mpts);

	return obj;
}

SHPObject *
shape_creator_wrapper_WKB(char *hexwkb, int idx)
{
	int result;
	LWGEOM_PARSER_RESULT lwg_parser_result;
	LWGEOM *lwgeom;


	/* Use the liblwgeom parser to convert from EWKB format to serialized LWGEOM */
	result = serialized_lwgeom_from_hexwkb(&lwg_parser_result, hexwkb, PARSER_CHECK_ALL);
	if (result)
	{
		printf("ERROR: %s\n", lwg_parser_result.message);
		exit(1);
	}

	/* Deserialize the LWGEOM */
	lwgeom = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

	/* Call the relevant method depending upon the geometry type */
	LWDEBUGF(4, "geomtype: %d\n", lwgeom_getType(lwgeom->type));

	switch(lwgeom_getType(lwgeom->type))
	{
		case POINTTYPE:
			return create_point(lwgeom_as_lwpoint(lwgeom));
			break;

		case MULTIPOINTTYPE:
			return create_multipoint(lwgeom_as_lwmpoint(lwgeom));
			break;

		case POLYGONTYPE:
			return create_polygon(lwgeom_as_lwpoly(lwgeom));
			break;

		case MULTIPOLYGONTYPE:
			return create_multipolygon(lwgeom_as_lwmpoly(lwgeom));
			break;

		case LINETYPE:
			return create_linestring(lwgeom_as_lwline(lwgeom));
			break;

		case MULTILINETYPE:
			return create_multilinestring(lwgeom_as_lwmline(lwgeom));
			break;

		default:
			printf( "Unknown WKB type (%8.8x) - (%s:%d)\n",
				lwgeom_getType(lwgeom->type), __FILE__, __LINE__);
			return NULL;
	}

	/* Free both the original and geometries */
	lwfree(lwg_parser_result.serialized_lwgeom);
	lwfree(lwgeom);
}


/*Reverse the clockwise-ness of the point list... */
int
reverse_points(int num_points, double *x, double *y, double *z, double *m)
{
	
	int i,j;
	double temp;
	j = num_points -1;
	for(i=0; i <num_points; i++){
		if(j <= i){
			break;
		}
		temp = x[j];
		x[j] = x[i];
		x[i] = temp;

		temp = y[j];
		y[j] = y[i];
		y[i] = temp;

		if ( z )
		{
			temp = z[j];
			z[j] = z[i];
			z[i] = temp;
		}

		if ( m )
		{
			temp = m[j];
			m[j] = m[i];
			m[i] = temp;
		}

		j--;
	}
	return 1;
}

/* Return 1 if the points are in clockwise order */
int
is_clockwise(int num_points, double *x, double *y, double *z)
{
	int i;
	double x_change,y_change,area;
	double *x_new, *y_new; /* the points, translated to the origin
	                        * for safer accuracy */

	x_new = (double *)malloc(sizeof(double) * num_points);	
	y_new = (double *)malloc(sizeof(double) * num_points);	
	area=0.0;
	x_change = x[0];
	y_change = y[0];

	for(i=0; i < num_points ; i++)
	{
		x_new[i] = x[i] - x_change;
		y_new[i] = y[i] - y_change;
	}

	for(i=0; i < num_points - 1; i++)
	{
		/* calculate the area	 */
		area += (x[i] * y[i+1]) - (y[i] * x[i+1]);
	}
	if(area > 0 ){
		free(x_new); free(y_new);
		return 0; /*counter-clockwise */
	}else{
		free(x_new); free(y_new);
		return 1; /*clockwise */
	}
}

/*
 * Returns OID integer on success
 * Returns -1 on error.
 */
int
getGeometryOID(PGconn *conn)
{
	PGresult *res1;
	char *temp_int;
	int OID;

	res1=PQexec(conn, "select OID from pg_type where typname = 'geometry'");	
	if ( ! res1 || PQresultStatus(res1) != PGRES_TUPLES_OK )
	{
		printf( "OIDQuery: %s", PQerrorMessage(conn));
		return -1;
	}

	if(PQntuples(res1) <= 0 )
	{
		printf( "Geometry type unknown "
				"(have you enabled postgis?)\n");
		return -1;
	}

	temp_int = (char *)PQgetvalue(res1, 0, 0);
	OID = atoi(temp_int);
	PQclear(res1);
	return OID;
}




/* 
 * Passed result is a 1 row result.
 * Return 1 on success.
 * Return 0 on failure.
 */
int
addRecord(PGresult *res, int residx, int row)
{
	int j;
	int nFields = PQnfields(res);
	int flds = 0; /* number of dbf field */
	char *hexewkb = NULL;
	unsigned char *hexewkb_binary = NULL;
	size_t hexewkb_len;

	for (j=0; j<nFields; j++)
	{
		const char *val;
		SHPObject *obj;

		/* Default (not geometry) attribute */
		if (type_ary[j] != 9)
		{
			/*
			 * Transform NULL numbers to '0'
			 * This is because the shapelib
			 * won't easly take care of setting
			 * nulls unless paying the acquisition
			 * of a bug in long integer values
			 */
			if ( PQgetisnull(res, residx, j) )
			{
				val = nullDBFValue(type_ary[j]);
			}
			else
			{
				val = PQgetvalue(res, residx, j);
				val = goodDBFValue(val, type_ary[j]);
			}

			LWDEBUG(3, "s");

			if(!DBFWriteAttributeDirectly(dbf, row, flds, val))
			{
				printf("error(string) - Record could not be "
						"created\n");
				return 0;
			}
			flds++;
			continue;
		}
		
		/* If we arrived here it is a geometry attribute */

		/* Handle NULL shapes */
		if ( PQgetisnull(res, residx, j) ) {
			obj=SHPCreateSimpleObject(SHPT_NULL,0,NULL,NULL,NULL);
			if ( SHPWriteObject(shp,-1,obj) == -1)
			{
				printf(
					"Error writing null shape %d\n", row);
				SHPDestroyObject(obj);
				return 0;
			}
			SHPDestroyObject(obj);
			continue;
		}

		/* Get the value from the result set */
		val = PQgetvalue(res, residx, j);

		if ( ! binary )
		{
			if ( pgis_major_version > 0 )
			{
				LWDEBUG(4, "PostGIS >= 1.0, non-binary cursor");
			
				/* Input is bytea encoded text field, so it must be unescaped and
				then converted to hexewkb string */
				hexewkb_binary = PQunescapeBytea((byte *)val, &hexewkb_len);
				hexewkb = convert_bytes_to_hex(hexewkb_binary, hexewkb_len);
			}
			else
			{
				LWDEBUG(4, "PostGIS < 1.0, non-binary cursor");

				/* Input is already hexewkb string, so we can just
				copy directly to hexewkb */
				hexewkb_len = PQgetlength(res, residx, j);
				hexewkb = malloc(hexewkb_len + 1);
				strncpy(hexewkb, val, hexewkb_len + 1);
			}
		}
		else /* binary */
		{
			LWDEBUG(4, "PostGIS (any version) using binary cursor");

			/* Input is binary field - must convert to hexewkb string */
			hexewkb_len = PQgetlength(res, residx, j);
			hexewkb = convert_bytes_to_hex((unsigned char *)val, hexewkb_len);
		}

		LWDEBUGF(4, "HexEWKB - length: %d  value: %s", strlen(hexewkb), hexewkb);
		
		obj = shape_creator_wrapper_WKB(hexewkb, row);
		if ( ! obj )
		{
			printf( "Error creating shape for record %d "
					"(geotype is %d)\n", row, geotype);
			return 0;
		}
		if ( SHPWriteObject(shp,-1,obj) == -1)
		{
			printf( "Error writing shape %d\n", row);
			SHPDestroyObject(obj);
			return 0;
		}
		SHPDestroyObject(obj);

		/* Free the hexewkb (and temporary bytea unescaped string if used) */
		if (hexewkb) free(hexewkb);
		if (hexewkb_binary) PQfreemem(hexewkb_binary);
	}

	LWDEBUGF(4, "Finished adding record %d\n", row);

	return 1;
}

/* 
 * Return allocate memory. Free after use.
 */
char *
getTableOID(char *schema, char *table)
{
	PGresult *res3;
	char *query;
	char *ret;
	size_t size;

	size = strlen(table)+256;
	if ( schema ) size += strlen(schema)+1;

	query = (char *)malloc(size);

	if ( schema )
	{
		sprintf(query, "SELECT oid FROM pg_class c, pg_namespace n WHERE c.relnamespace n.oid AND n.nspname = '%s' AND c.relname = '%s'", schema, table);
	} else {
		sprintf(query, "SELECT oid FROM pg_class WHERE relname = '%s'", table);
	}

	res3 = PQexec(conn, query);
	free(query);
	if ( ! res3 || PQresultStatus(res3) != PGRES_TUPLES_OK ) {
		printf( "TableOID: %s", PQerrorMessage(conn));
		exit_nicely(conn, 1);
	}
	if(PQntuples(res3) == 1 ){
		ret = strdup(PQgetvalue(res3, 0, 0));
	}else if(PQntuples(res3) == 0 ){
		printf( "Cannot find relation OID (does table exist?).\n");
		PQclear(res3);
		return NULL;
	}else{
		ret = strdup(PQgetvalue(res3, 0, 0));
		printf( "Warning: Multiple relations detected, the program will only dump the first relation.\n");
	}	
	PQclear(res3);
	return ret;
}

/*
 * Return geometry type as defined at top file.
 * Return -1 on error.
 * Return  0 on unknown or unsupported geometry type.
 * Set outtype to 'm' or 'z' depending on input type.
 */
int
getGeometryType(char *schema, char *table, char *geo_col_name)
{
	char query[1024];
	PGresult *res;
	char *geo_str; /* the geometry type string */
	int multitype=0;
	int basetype=0;
	int foundmulti=0;
	int foundsingle=0;
	int i;

	/**************************************************
	 * Get what kind of Geometry type is in the table
	 **************************************************/

	if ( schema )
	{
		sprintf(query, "SELECT DISTINCT geometrytype(\"%s\") "
			"FROM \"%s\".\"%s\" WHERE NOT geometrytype(\"%s\") "
			"IS NULL", geo_col_name, schema, table, geo_col_name);
	}
	else
	{
		sprintf(query, "SELECT DISTINCT geometrytype(\"%s\") "
			"FROM \"%s\" WHERE NOT geometrytype(\"%s\") IS NULL",
			geo_col_name, table, geo_col_name);
	}

#if VERBOSE > 2
	printf( "%s\n",query);
#endif
	res = PQexec(conn, query);	
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "GeometryType: %s", PQerrorMessage(conn));
		return -1;
	}

	if (PQntuples(res) == 0)
	{
		printf("ERROR: Cannot determine geometry type (empty table).\n");
		PQclear(res);
		return -1;
	}

	/* This will iterate max 2 times */
	for (i=0; i<PQntuples(res); i++)
	{
		geo_str = PQgetvalue(res, i, 0);
		if ( ! strncmp(geo_str, "MULTI", 5) )
		{
			foundmulti=1;
			geo_str+=5;
		}
		else
		{
			foundsingle=1;
		}

		if ( ! strncmp(geo_str, "LINESTRI", 8) )
		{
			if ( basetype && basetype != LINETYPE )
			{
				printf( "ERROR: uncompatible mixed geometry types in table\n");
				PQclear(res);
				return -1;
			}
			basetype = LINETYPE;
			multitype = MULTILINETYPE;
		}
		else if ( ! strncmp(geo_str, "POLYGON", 7) )
		{
			if ( basetype && basetype != POLYGONTYPE )
			{
				printf( "ERROR: uncompatible mixed geometries in table\n");
				PQclear(res);
				return -1;
			}
			basetype = POLYGONTYPE;
			multitype = MULTIPOLYGONTYPE;
		}
		else if ( ! strncmp(geo_str, "POINT", 5) )
		{
			if ( basetype && basetype != POINTTYPE )
			{
				printf( "ERROR: uncompatible mixed geometries in table\n");
				PQclear(res);
				return -1;
			}
			basetype = POINTTYPE;
			multitype = MULTIPOINTTYPE;
		}
		else
		{
			printf( "type '%s' is not Supported at this time.\n",
				geo_str);
			printf( "The DBF file will be created but not the shx "
				"or shp files.\n");
			PQclear(res);
			return 0;
		}

	}

	PQclear(res);

	/**************************************************
	 * Get Geometry dimensions (2d/3dm/3dz/4d)
	 **************************************************/
	if ( pgis_major_version > 0 )
		if ( -1 == getGeometryMaxDims(schema, table, geo_col_name) )
			return -1;

	if ( foundmulti )
		return multitype;
	else
		return basetype;

}

/*
 * Set global outtype variable to:
 * 	'm' for 3dm input
 * 	'z' for 3dz or 4d input
 * 	's' for 2d
 * Return -1 on error, 0 on success.
 * Call only on postgis >= 1.0.0
 */
int
getGeometryMaxDims(char *schema, char *table, char *geo_col_name)
{
	char query[1024];
	PGresult *res;
	int maxzmflag;

	if ( schema )
	{
		sprintf(query, "SELECT max(zmflag(\"%s\")) "
			"FROM \"%s\".\"%s\"", 
			 geo_col_name, schema, table);
	}
	else
	{
		sprintf(query, "SELECT max(zmflag(\"%s\")) "
			"FROM \"%s\"",
			geo_col_name, table);
	}

#if VERBOSE > 2
	printf("%s\n",query);
#endif
	res = PQexec(conn, query);	
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "ZMflagQuery: %s", PQerrorMessage(conn));
		PQclear(res);
		return -1;
	}

	if (PQntuples(res) == 0)
	{
		printf("ERROR: Cannot determine geometry dimensions (empty table).\n");
		PQclear(res);
		return -1;
	}

	maxzmflag = atoi(PQgetvalue(res, 0, 0));
	PQclear(res);

	switch (maxzmflag)
	{
		case 0:
			outtype = 's';
			break;
		case 1:
			outtype = 'm';
			break;
		default:
			outtype = 'z';
			break;
	}


	return 0;
}

void
usage(char* me, int status, FILE* out)
{
        fprintf(out,"RCSID: %s RELEASE: %s\n", rcsid, POSTGIS_VERSION);
	fprintf(out,"USAGE: %s [<options>] <database> [<schema>.]<table>\n", me);
	fprintf(out,"       %s [<options>] <database> <query>\n", me);
	fprintf(out,"\n");
       	fprintf(out,"OPTIONS:\n");
       	fprintf(out,"  -f <filename>  Use this option to specify the name of the file\n");
       	fprintf(out,"     to create.\n");
       	fprintf(out,"  -h <host>  Allows you to specify connection to a database on a\n");
	fprintf(out,"     machine other than the default.\n");
       	fprintf(out,"  -p <port>  Allows you to specify a database port other than the default.\n");
       	fprintf(out,"  -P <password>  Connect to the database with the specified password.\n");
       	fprintf(out,"  -u <user>  Connect to the database as the specified user.\n");
	fprintf(out,"  -g <geometry_column> Specify the geometry column to be exported.\n");
	fprintf(out,"  -b Use a binary cursor.\n");
	fprintf(out,"  -r Raw mode. Do not assume table has been created by \n");
	fprintf(out,"     the loader. This would not unescape attribute names\n");
	fprintf(out,"     and will not skip the 'gid' attribute.\n");
	fprintf(out,"  -k Keep postgresql identifiers case.\n");
        fprintf(out,"  -? Display this help screen.\n");
       	fprintf(out,"\n");
       	exit (status);
}

/* Parse command line parameters */
int
parse_commandline(int ARGC, char **ARGV)
{
	int c, curindex;
	char buf[2048];
        
        if ( ARGC == 1 ) {
                usage(ARGV[0], 0, stdout);
        }

	memset(buf, 0, 2048); /* just in case... */

	/* Parse command line */
        while ((c = pgis_getopt(ARGC, ARGV, "bf:h:du:p:P:g:rk")) != EOF){
		switch (c) {
			case 'b':
				binary = 1;
				break;
			case 'f':
				shp_file = optarg;
				break;
			case 'h':
				snprintf(buf + strlen(buf), 255, "host=%s ", optarg);	
				break;
			case 'd':
				dswitchprovided = 1;
				outtype = 'z';
				break;		  
			case 'r':
				includegid = 1;
				unescapedattrs = 1;
				break;		  
			case 'u':
				snprintf(buf + strlen(buf), 255, "user=%s ", optarg);
				break;
			case 'p':
				snprintf(buf + strlen(buf), 255, "port=%s ", optarg);	
				break;
			case 'P':
				snprintf(buf + strlen(buf), 255, "password=%s ", optarg);
				break;
			case 'g':
				geo_col_name = optarg;
				break;
			case 'k':
				keep_fieldname_case = 1;
				break;
			case '?':
                                usage(ARGV[0], 0, stdout);
			default:
				return 0;
		}
        }

        curindex=0;
        for (; optind<ARGC; optind++){
                if (curindex == 0) {
		    	snprintf(buf + strlen(buf), 255, "dbname=%s", ARGV[optind]);
                }else if(curindex == 1){
			parse_table(ARGV[optind]);

                }
                curindex++;
        }
        if (curindex < 2) return 0;
	
	/* Copy the buffer result to the connection string */
	strncpy(conn_string, buf, 2048);	
	
	return 1;
}

int
get_postgis_major_version(void)
{
	PGresult *res;
	char *version;
	int ver;
	char query[] = "SELECT postgis_version()";
	res = PQexec(conn, query);

	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "Can't detect postgis version:\n");
		printf( "PostgisVersionQuery: %s",
			PQerrorMessage(conn));
		exit(1);
	}

	version = PQgetvalue(res, 0, 0);
	ver = atoi(version);
	PQclear(res);
	return ver;
}

/*
 * Initialize shapefile files, main scan query,
 * type array.
 */
int
initialize(void)
{
	PGresult *res;
	char *query;
	int i;
	char buf[256];
	int tmpint;
	int geo_oid; /* geometry oid */
	int geom_fld = -1;
	char *mainscan_flds[256];
	int mainscan_nflds=0;
	int size;
	int gidfound=0;
	char *dbf_flds[256];
	int dbf_nfields=0;
	int projsuccess;

	/* Detect postgis version */
	pgis_major_version = get_postgis_major_version();

	/* Detect host endiannes */
	big_endian = is_bigendian();

	/* Query user attributes name, type and size */

	size = strlen(table);
	if ( schema ) size += strlen(schema);
	size += 256;

	query = (char *)malloc(size);
	if ( ! query ) return 0; /* out of virtual memory */

	if ( schema )
	{
		sprintf(query, "SELECT a.attname, a.atttypid, a.attlen, "
			"a.atttypmod FROM "
			"pg_attribute a, pg_class c, pg_namespace n WHERE "
			"n.nspname = '%s' AND a.attrelid = c.oid AND "
			"n.oid = c.relnamespace AND "
			"a.atttypid != 0 AND "
			"a.attnum > 0 AND c.relname = '%s'", schema, table);
	}
	else
	{
		sprintf(query, "SELECT a.attname, a.atttypid, a.attlen, "
			"a.atttypmod FROM "
			"pg_attribute a, pg_class c WHERE "
			"a.attrelid = c.oid and a.attnum > 0 AND "
			"a.atttypid != 0 AND "
			"c.relname = '%s'", table);
	}


	/* Exec query */
	LWDEBUGF(4, "Attribute query: %s\n", query);

	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "Querying for attributes: %s",
			PQerrorMessage(conn));
		return 0;
	}
	if (! PQntuples(res)) {
		printf( "Table %s does not exist\n", table);
		PQclear(res);
		return 0;
	}

	/* Create the dbf file */
	dbf = DBFCreate(shp_file);
	if ( ! dbf ) {
		printf( "Could not create dbf file\n");
		return 0;
	}
	
	/* Get geometry oid */
	geo_oid = getGeometryOID(conn);
	if ( geo_oid == -1 )
	{
		PQclear(res);
		return 0;
	}

	/*
	 * Scan the result setting fields to be returned in mainscan
	 * query, filling the type_ary, and creating .dbf and .shp files.
	 */
	for (i=0; i<PQntuples(res); i++)
	{
		int j;
		int type, size, mod;
		char *fname; /* pgsql attribute name */
		char *ptr;
		char field_name[32]; /* dbf version of field name */

		fname = PQgetvalue(res, i, 0);
		type = atoi(PQgetvalue(res, i, 1));
		size = atoi(PQgetvalue(res, i, 2));
		mod = atoi(PQgetvalue(res, i, 3));

		/*
		 * This is a geometry column
		 */
		if(type == geo_oid)
		{
			/* We've already found our geometry column */
			if ( geom_fld != -1 ) continue;

			/*
			 * A geometry attribute name has not been
			 * provided: we'll use this one (the first).
			 */
			if ( ! geo_col_name )
			{
				geom_fld = mainscan_nflds;
				type_ary[mainscan_nflds]=9; 
				geo_col_name = fname;
				mainscan_flds[mainscan_nflds++] = fname;
			}

			/*
			 * This is exactly the geometry privided
			 * by the user.
			 */
			else if (!strcmp(geo_col_name, fname))
			{
				geom_fld = mainscan_nflds;
				type_ary[mainscan_nflds]=9; 
				mainscan_flds[mainscan_nflds++] = fname;
			}

			continue;
		}


		/* 
		 * Everything else (non geometries) will be
		 * a DBF attribute.
		 */

		/* Skip gid (if not asked to do otherwise */
		if ( ! strcmp(fname, "gid") )
		{
			gidfound = 1;
			if ( ! includegid ) continue;
		}

		/* Unescape field name */
		ptr = fname;
		if ( ! unescapedattrs )
		{
			if (*ptr=='_') ptr+=2;
		}
		/*
		 * This needs special handling since both xmin and _xmin
		 * becomes __xmin when escaped
		 */

		/* Limit dbf field name to 10-digits */
		strncpy(field_name, ptr, 10);
		field_name[10] = 0;

		/* 
		 * make sure the fields all have unique names,
		 */
		tmpint=1;
		for(j=0; j<dbf_nfields; j++)
		{
			if(!strncasecmp(field_name, dbf_flds[j], 10))
			{
				sprintf(field_name,"%.7s_%.2d",
					ptr,
					tmpint++);
				j=-1; continue;
			}
		}

		/* make UPPERCASE if keep_fieldname_case = 0 */
		if (keep_fieldname_case == 0)
			for(j=0; j<strlen(field_name); j++)
				field_name[j] = toupper(field_name[j]);

		if ( strcasecmp(fname, field_name) ) {
			fprintf(stderr, "Warning, field %s renamed to %s\n",
				fname, field_name);
		}

		/*fprintf(stderr, "DBFfield: %s\n", field_name); */
		dbf_flds[dbf_nfields++] = strdup(field_name);

		/*
		 * Find appropriate type of dbf attributes
		 */

		/* int2 type */
		if ( type == 21 )
		{
			/* 
			 * Longest text representation for
			 * an int2 type (16bit) is 6 bytes
			 * (-32768)
			 */
			if ( DBFAddField(dbf, field_name, FTInteger,
				6, 0) == -1 )
			{
				printf( "error - Field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTInteger;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/* int4 type */
		if ( type == 23 )
		{
			/* 
			 * Longest text representation for
			 * an int4 type (32bit) is 11 bytes
			 * (-2147483648)
			 */
			if ( DBFAddField(dbf, field_name, FTInteger,
				11, 0) == -1 )
			{
				printf( "Error - Ingeter field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTInteger;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/* int8 type */
		if ( type == 20 )
		{
			/* 
			 * Longest text representation for
			 * an int8 type (64bit) is 20 bytes
			 * (-9223372036854775808)
			 */
			if ( DBFAddField(dbf, field_name, FTInteger,
				20, 0) == -1 )
			{
				printf( "Error - Integer field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTInteger;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}
		
		/*
		 * double or numeric types:
  		 *    700: float4
		 *    701: float8
		 *   1700: numeric
		 *
		 *
		 * TODO: stricter handling of sizes
		 */
		if(type == 700 || type == 701 || type == 1700 )
		{
			if(DBFAddField(dbf, field_name,FTDouble,32,10) == -1)
			{
				printf( "Error - Double field could not "
						"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTDouble;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/*
		 * Boolean field, we use FTLogical
		 */
		if ( type == 16 )
		{
			if ( DBFAddField(dbf, field_name, FTLogical,
				2, 0) == -1 )
			{
				printf( "Error - Boolean field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTLogical;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/*
		 * Date field
		 */
		if(type == 1082)
		{
			if ( DBFAddField(dbf, field_name, FTDate,
				8, 0) == -1 )
			{
				printf( "Error - Date field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=FTDate;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/*
		 * timestamp field, which we store as a string so we need
		 * more width in the column
		 */
		else if(type == 1114)
		{
			size = 19;
		}

		/*
		 * For variable-sized fields we'll use either 
		 * maximum allowed size (atttypmod) or max actual
		 * attribute value in table.
		 */
		else if(size == -1)
		{
			/*
			 * 1042 is bpchar,  1043 is varchar
			 * mod is maximum allowed size, including
			 * header which contains *real* size.
			 */
			if ( (type == 1042 || type == 1043) && mod != -1 )
			{
				size = mod-4; /* 4 is header size */
			}
			else
			{
				size = getMaxFieldSize(conn, schema,
					table, fname);
				if ( size == -1 ) return 0;
				if ( ! size ) size = 32;
				/* might 0 be a good size ? */
			}
		}

		if ( size > MAX_DBF_FIELD_SIZE )
		{
			fprintf(stderr, "Warning: values of field '%s' "
				"exceeding maximum dbf field width (%d) "
				"will be truncated.\n",
				fname, MAX_DBF_FIELD_SIZE);
			size = MAX_DBF_FIELD_SIZE;
		}

		LWDEBUGF(3, "FIELD_NAME: %s, SIZE: %d\n", field_name, size);

		/* generic type (use string representation) */
		if(DBFAddField(dbf, field_name, FTString, size, 0) == -1)
		{
			fprintf(stderr, "Error - String field could not "
					"be created.\n");
			return 0;
		}
		type_ary[mainscan_nflds]=FTString;
		mainscan_flds[mainscan_nflds++] = fname;
	}

	/* Release dbf field memory */
	for (i=0; i<dbf_nfields; i++) free(dbf_flds[i]);

	/*
	 * If no geometry has been found
	 * we have finished with initialization
	 */
	if ( geom_fld == -1 )
	{
		if ( geo_col_name )
		{
			printf( "%s: no such attribute in table %s\n",
					geo_col_name, table);
			return 0;
		}
		printf( "No geometry column found.\n");
		printf( "The DBF file will be created "
				"but not the shx or shp files.\n");
	}
	else
	{
		/*
		 * We now create the appropriate shape (shp) file.
		 * And set the shape creator function.
		 */
		 /**Create the prj file if we can **/
		projsuccess = projFileCreate(shp_file, schema, table, geo_col_name);
		geotype = getGeometryType(schema, table, geo_col_name);
		if ( geotype == -1 ) return 0;

		switch (geotype)
		{
			case MULTILINETYPE:
			case LINETYPE:
				if (outtype == 'z') outshptype=SHPT_ARCZ;
				else if (outtype == 'm') outshptype=SHPT_ARCM;
				else outshptype=SHPT_ARC;
				break;
				
			case POLYGONTYPE:
			case MULTIPOLYGONTYPE:
				if (outtype == 'z') outshptype=SHPT_POLYGONZ;
				else if (outtype == 'm') outshptype=SHPT_POLYGONM;
				else outshptype=SHPT_POLYGON;
				break;

			case POINTTYPE:
				if (outtype == 'z') outshptype=SHPT_POINTZ;
				else if (outtype == 'm') outshptype=SHPT_POINTM;
				else outshptype=SHPT_POINT;
				break;

			case MULTIPOINTTYPE:
				if (outtype == 'z') outshptype=SHPT_MULTIPOINTZ;
				else if (outtype == 'm') outshptype=SHPT_MULTIPOINTM;
				else outshptype=SHPT_MULTIPOINT;
				break;

			default:
				shp = NULL;
				printf( "You've found a bug! (%s:%d)\n",
					__FILE__, __LINE__);
				return 0;

		}
		shp = SHPCreate(shp_file, outshptype);
	}

	/* 
	 * Ok. Now we should have an array of allocate strings
	 * representing the fields we'd like to be returned by
	 * main scan query.
	 */

	tmpint = strlen(table)+2;
	for (i=0; i<mainscan_nflds; i++)
		tmpint += strlen(mainscan_flds[i])+32;
	main_scan_query = (char *)malloc(tmpint+256);

	sprintf(main_scan_query, "SELECT ");
	for (i=0; i<mainscan_nflds; i++)
	{
		if ( i ) {
			sprintf(buf, ",");
			strcat(main_scan_query, buf);
		}

		/* this is the geometry */
		if ( type_ary[i] == 9 )
		{
			if ( big_endian ) 
			{
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'XDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'XDR')",
						mainscan_flds[i]);
				}
			}
			else /* little_endian */
			{
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'NDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'NDR')",
						mainscan_flds[i]);
				}
			}
		}
		else
		{
			if ( binary )
				sprintf(buf, "\"%s\"::text", mainscan_flds[i]);
			else
				sprintf(buf, "\"%s\"", mainscan_flds[i]);
		}

		strcat(main_scan_query, buf);
	}

	if ( schema )
	{
		sprintf(buf, " FROM \"%s\".\"%s\"", schema, table);
	}
	else
	{
		sprintf(buf, " FROM \"%s\"", table);
	}

	strcat(main_scan_query, buf);

	/* Order by 'gid' (if found) */
	if ( gidfound )
	{
		sprintf(buf, " ORDER BY \"gid\"");
		strcat(main_scan_query, buf);
	}

	PQclear(res);

	return 1;
}

/* 
 * Return the maximum octet_length from given table.
 * Return -1 on error.
 */
int
getMaxFieldSize(PGconn *conn, char *schema, char *table, char *fname)
{
	int size;
	char *query;
	PGresult *res;

	/*( this is ugly: don't forget counting the length  */
	/* when changing the fixed query strings ) */

	if ( schema )
	{
		query = (char *)malloc(strlen(fname)+strlen(table)+
			strlen(schema)+40); 
		sprintf(query,
			"select max(octet_length(\"%s\")) from \"%s\".\"%s\"",
			fname, schema, table);
	}
	else
	{
		query = (char *)malloc(strlen(fname)+strlen(table)+40); 
		sprintf(query,
			"select max(octet_length(\"%s\")) from \"%s\"",
			fname, table);
	}
#if VERBOSE > 2
	printf( "maxFieldLenQuery: %s\n", query);
#endif
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "Querying for maximum field length: %s",
			PQerrorMessage(conn));
		return -1;
	}

	if(PQntuples(res) <= 0 )
	{
		PQclear(res);
		return -1;
	}
	size = atoi(PQgetvalue(res, 0, 0));
	PQclear(res);
	return size;
}

int
is_bigendian(void)
{
	int test = 1;

	if ( (((char *)(&test))[0]) == 1)
	{
		return 0; /*NDR (little_endian) */
	}
	else
	{
		return 1; /*XDR (big_endian) */
	}
}

char *
shapetypename(int num)
{
	switch(num)
	{
		case SHPT_NULL:
			return "Null Shape";
		case SHPT_POINT:
			return "Point";
		case SHPT_ARC:
			return "PolyLine";
		case SHPT_POLYGON:
			return "Polygon";
		case SHPT_MULTIPOINT:
			return "MultiPoint";
		case SHPT_POINTZ:
			return "PointZ";
		case SHPT_ARCZ:
			return "PolyLineZ";
		case SHPT_POLYGONZ:
			return "PolygonZ";
		case SHPT_MULTIPOINTZ:
			return "MultiPointZ";
		case SHPT_POINTM:
			return "PointM";
		case SHPT_ARCM:
			return "PolyLineM";
		case SHPT_POLYGONM:
			return "PolygonM";
		case SHPT_MULTIPOINTM:
			return "MultiPointM";
		case SHPT_MULTIPATCH:
			return "MultiPatch";
		default:
			return "Unknown";
	}
}

/*
 * Either get a table (and optionally a schema)
 * or a query.
 * A query starts with a "select" or "SELECT" string.
 */
static void
parse_table(char *spec)
{
	char *ptr;

	/* Spec is a query */
	if ( strstr(spec, "SELECT ") || strstr(spec, "select ") )
	{
		usrquery = spec;

		/*
		 * encode pid in table name to reduce 
		 * clashes probability (see bug#115)
		 */
		sprintf(temptablename,
			"__pgsql2shp%lu_tmp_table",
			(long)getpid());
		table = temptablename; 
	}
	else
	{
        	table = spec;
		if ( (ptr=strchr(table, '.')) )
		{
			*ptr = '\0';
			schema = table;
			table = ptr+1;
		}
	}
}

static int
create_usrquerytable(void)
{
	char *query;
	PGresult *res;

	query = malloc(strlen(table)+strlen(usrquery)+32);
	sprintf(query, "CREATE TEMP TABLE \"%s\" AS %s", table, usrquery);

        printf("Preparing table for user query... ");
	fflush(stdout);
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		printf( "Failed: %s\n",
			PQerrorMessage(conn));
		return 0;
	}
	PQclear(res);
	printf("Done.\n");
	return 1;
}

/* This is taken and adapted from dbfopen.c of shapelib */
static const char *
nullDBFValue(char fieldType)
{
	switch(fieldType)
	{
		case FTInteger:
		case FTDouble:
			/* NULL numeric fields have value "****************" */
			return "****************";

		case FTDate:
			/* NULL date fields have value "00000000" */
			return "00000000";

		case FTLogical:
			/* NULL boolean fields have value "?" */ 
			return "?";

		default:
			/* empty string fields are considered NULL */
			return "";
	}
}

/* 
 * Make appropriate formatting of a DBF value based on type.
 * Might return untouched input or pointer to static private 
 * buffer: use return value right away.
 */
static const char *
goodDBFValue(const char *in, char fieldType)
{
	/*
	 * We only work on FTLogical and FTDate.
	 * FTLogical is 1 byte, FTDate is 8 byte (YYYYMMDD)
	 * We allocate space for 9 bytes to take
	 * terminating null into account
	 */
	static char buf[9];

	switch (fieldType)
	{
		case FTLogical:
			buf[0] = toupper(in[0]);
			buf[1]='\0';
			return buf;
		case FTDate:
			buf[0]=in[0]; /* Y */
			buf[1]=in[1]; /* Y */
			buf[2]=in[2]; /* Y */
			buf[3]=in[3]; /* Y */
			buf[4]=in[5]; /* M */
			buf[5]=in[6]; /* M */
			buf[6]=in[8]; /* D */
			buf[7]=in[9]; /* D */
			buf[8]='\0';
			return buf;
		default:
			return in;
	}
}

char *convert_bytes_to_hex(uchar *ewkb, size_t size)
{
	int i;
	char *hexewkb;

	/* Convert the byte stream to a hex string using liblwgeom's deparse_hex function */
	hexewkb = malloc(size * 2 + 1);
	for (i=0; i<size; ++i) deparse_hex(ewkb[i], &hexewkb[i * 2]);
	hexewkb[size * 2] = '\0';

	return hexewkb;
}

int projFileCreate(const char * pszFilename, char *schema, char *table, char *geo_col_name)
{
    FILE	*fp;
    char	*pszFullname, *pszBasename;
    int		i, result;
	char *srtext;
	char *query;
	char *esc_schema;
	char *esc_table;
	char *esc_geo_col_name;
	int error;
	PGresult *res;
	int size;
	
	/***********
	*** I'm multiplying by 2 instead of 3 because I am too lazy to figure out how many characters to add
	*** after escaping if any **/
	size = 1000;
	if ( schema ) {
		size += 3 * strlen(schema);
	}
	size += 1000;
	esc_table = (char *) malloc(3 * strlen(table) + 1);
	esc_geo_col_name = (char *) malloc(3 * strlen(geo_col_name) + 1);
	PQescapeStringConn(conn, esc_table, table, strlen(table), &error);
	PQescapeStringConn(conn, esc_geo_col_name, geo_col_name, strlen(geo_col_name), &error);

	/** make our address space large enough to hold query with table/schema **/
	query = (char *) malloc(size);
	if ( ! query ) return 0; /* out of virtual memory */
	
	/**************************************************
	 * Get what kind of spatial ref is the selected geometry field
	 * We first check the geometry_columns table for a match and then if no match do a distinct against the table
	 * NOTE: COALESCE does a short-circuit check returning the faster query result and skipping the second if first returns something
	 *	Escaping quotes in the schema and table in query may not be necessary except to prevent malicious attacks 
	 *	or should someone be crazy enough to have quotes or other weird character in their table, column or schema names 
	 **************************************************/
	if ( schema )
	{
		esc_schema = (char *) malloc(2 * strlen(schema) + 1);
		PQescapeStringConn(conn, esc_schema, schema, strlen(schema), &error);
		sprintf(query, "SELECT COALESCE((SELECT sr.srtext "
				" FROM  geometry_columns As gc INNER JOIN spatial_ref_sys sr ON sr.srid = gc.srid "
				" WHERE gc.f_table_schema = '%s' AND gc.f_table_name = '%s' AND gc.f_geometry_column = '%s' LIMIT 1),  " 
				" (SELECT CASE WHEN COUNT(DISTINCT sr.srid) > 1 THEN 'm' ELSE MAX(sr.srtext) END As srtext "
			" FROM \"%s\".\"%s\" As g INNER JOIN spatial_ref_sys sr ON sr.srid = ST_SRID(g.\"%s\")) , ' ') As srtext ", 
				esc_schema, esc_table,esc_geo_col_name, schema, table, geo_col_name);
		free(esc_schema);
	}
	else
	{
		sprintf(query, "SELECT COALESCE((SELECT sr.srtext "
				" FROM  geometry_columns As gc INNER JOIN spatial_ref_sys sr ON sr.srid = gc.srid "
				" WHERE gc.f_table_name = '%s' AND gc.f_geometry_column = '%s' AND pg_table_is_visible((gc.f_table_schema || '.' || gc.f_table_name)::regclass) LIMIT 1),  "
				" (SELECT CASE WHEN COUNT(DISTINCT sr.srid) > 1 THEN 'm' ELSE MAX(sr.srtext) END as srtext "
			" FROM \"%s\" As g INNER JOIN spatial_ref_sys sr ON sr.srid = ST_SRID(g.\"%s\")), ' ') As srtext ", 
				esc_table, esc_geo_col_name, table, geo_col_name);
	}

	LWDEBUGF(3,"%s\n",query);
	free(esc_table);
	free(esc_geo_col_name);

	res = PQexec(conn, query);
	
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		printf( "Error: %s", PQerrorMessage(conn));
		return 0;
	}

	for (i=0; i < PQntuples(res); i++)
	{
		srtext = PQgetvalue(res, i, 0);
		if (strcmp(srtext,"m") == 0){
			printf("ERROR: Mixed set of spatial references.\n");	
			PQclear(res);
			return 0;
		}
		else {
			if (srtext[0] == ' '){
				printf("ERROR: Cannot determine spatial reference (empty table or unknown spatial ref).\n");
				PQclear(res);
				return 0;
			}
			else {	
				/* -------------------------------------------------------------------- */
				/*	Compute the base (layer) name.  If there is any extension	*/
				/*	on the passed in filename we will strip it off.			*/
				/* -------------------------------------------------------------------- */
					pszBasename = (char *) malloc(strlen(pszFilename)+5);
					strcpy( pszBasename, pszFilename );
					for( i = strlen(pszBasename)-1; 
					 i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
						   && pszBasename[i] != '\\';
					 i-- ) {}
				
					if( pszBasename[i] == '.' )
						pszBasename[i] = '\0';
				
					pszFullname = (char *) malloc(strlen(pszBasename) + 5);
					sprintf( pszFullname, "%s.prj", pszBasename );
					free( pszBasename );
					
				
				/* -------------------------------------------------------------------- */
				/*      Create the file.                                                */
				/* -------------------------------------------------------------------- */
				fp = fopen( pszFullname, "wb" );
				if( fp == NULL ){
					return 0;
				}
				result = fputs (srtext,fp);
				LWDEBUGF(3, "\n result %d proj SRText is %s .\n", result, srtext);	
				fclose( fp );
				free( pszFullname );
			}
		}
	}
	PQclear(res);
	free(query);
	return 1;
}

/**********************************************************************
 * $Log$
 * Revision 1.85  2006/06/16 14:12:16  strk
 *         - BUGFIX in pgsql2shp successful return code.
 *         - BUGFIX in shp2pgsql handling of MultiLine WKT.
 *
 * Revision 1.84  2006/04/18 14:09:28  strk
 * Limited text field size to 255 (bug #84)  [will eventually provide a switch to support wider fields ]
 *
 * Revision 1.83  2006/02/03 20:53:36  strk
 * Swapped stdint.h (unavailable on Solaris9) with inttypes.h
 *
 * Revision 1.82  2006/01/16 10:42:57  strk
 * Added support for Bool and Date DBF<=>PGIS mapping
 *
 * Revision 1.81  2006/01/09 16:40:16  strk
 * ISO C90 comments, signedness mismatch fixes
 *
 * Revision 1.80  2005/10/24 11:30:59  strk
 *
 * Fixed a bug in string attributes handling truncating values of maximum
 * allowed length, curtesy of Lars Roessiger.
 * Reworked integer attributes handling to be stricter in dbf->sql mapping
 * and to allow for big int8 values in sql->dbf conversion
 *
 * Revision 1.79  2005/10/03 18:08:55  strk
 * Stricter string attributes lenght handling. DBF header will be used
 * to set varchar maxlenght, (var)char typmod will be used to set DBF header
 * len.
 *
 * Revision 1.78  2005/07/22 19:15:28  strk
 * Fixed bug in {get,pop}{int,double} for 64bit archs
 *
 * Revision 1.77  2005/07/12 16:19:35  strk
 * Fixed bug in user query handling, reported by Andrew Seales
 *
 * Revision 1.76  2005/05/16 17:22:43  strk
 * Fixed DBF field names handling as for clashes avoiding.
 * pgsql field renames are warned.
 *
 * Revision 1.75  2005/05/13 14:06:24  strk
 * Applied patch from Obe, Regina to keep identifiers case.
 *
 * Revision 1.74  2005/03/25 18:43:07  strk
 * Fixed PQunescapeBytearea argument (might give problems on 64bit archs)
 *
 * Revision 1.73  2005/03/08 11:06:33  strk
 * modernized old-style parameter declarations
 *
 * Revision 1.72  2005/03/04 14:54:03  strk
 * Fixed bug in multiline handling.
 *
 * Revision 1.71  2005/01/31 22:15:22  strk
 * Added maintainer notice, to reduce Jeff-strk mail bounces
 *
 * Revision 1.70  2004/12/22 10:29:09  strk
 * Drop useless SRID from geometry when downloading EWKB format.
 *
 * Revision 1.69  2004/12/15 08:46:47  strk
 * Fixed memory leaks depending on input size.
 *
 * Revision 1.68  2004/11/18 18:14:19  strk
 * Added a copy of the PQunescapeBytea function found in libpq of PG>=73
 *
 * Revision 1.67  2004/10/17 12:16:47  strk
 * fixed prototype for user query table
 *
 * Revision 1.66  2004/10/17 12:15:10  strk
 * Bug fixed in multipoint4D creation
 *
 * Revision 1.65  2004/10/15 08:26:03  strk
 * Fixed handling of mixed dimensioned geometries in source table.
 *
 * Revision 1.64  2004/10/14 09:59:51  strk
 * Added support for user query (replacing schema.table)
 *
 * Revision 1.63  2004/10/11 14:34:40  strk
 * Added endiannes specification for postgis-1.0.0+
 *
 * Revision 1.62  2004/10/07 21:51:05  strk
 * Fixed a bug in 4d handling
 *
 * Revision 1.61  2004/10/07 17:15:28  strk
 * Fixed TYPEM handling.
 *
 * Revision 1.60  2004/10/07 06:54:24  strk
 * cleanups
 *
 * Revision 1.59  2004/10/06 17:04:38  strk
 * ZM handling. Log trimmed.
 *
 * Revision 1.58  2004/09/23 16:14:19  strk
 * Added -m / -z switches to control output type: XYM,XYMZ.
 *
 * Revision 1.57  2004/09/20 17:11:44  strk
 * Added -d -d availability notice in help string.
 * Added user notice about output shape type.
 *
 * Revision 1.56  2004/09/20 16:33:05  strk
 * Added 4d geometries support.
 * Changelog section moved at bottom file.
 *
 * Revision 1.55  2004/09/20 14:14:43  strk
 * Fixed a bug in popbyte. Trapped WKB endiannes errors.
 *
 * Revision 1.54  2004/09/20 13:49:27  strk
 * Postgis-1.x support (LWGEOM) added.
 * postgis version detected at runtime.
 * Endiannes unchecked ... TODO.
 *
 **********************************************************************/
