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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include "libpq-fe.h"
#include "shapefil.h"
#include "getopt.h"
#include "compat.h"

#ifdef __CYGWIN__
#include <sys/param.h>       
#endif

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7
#define	BBOXONLYTYPE	99

/*
 * Verbosity:
 *   set to 1 to see record fetching progress
 *   set to 2 to see also shapefile creation progress
 */
#define VERBOSE 1

/* Define this to use HEX encoding instead of bytea encoding */
#define HEXWKB 1

typedef unsigned long int uint32;
typedef unsigned char byte;

/* Global data */
PGconn *conn;
int rowbuflen;
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
SHPObject * (*shape_creator)(byte *, int);
int big_endian = 0;
int pgis_major_version;

/* Prototypes */
int getMaxFieldSize(PGconn *conn, char *schema, char *table, char *fname);
int parse_commandline(int ARGC, char **ARGV);
void usage(int exitstatus);
char *getTableOID(char *schema, char *table);
int addRecord(PGresult *res, int residx, int row);
int initShapefile(char *shp_file, PGresult *res);
int initialize(void);
int getGeometryOID(PGconn *conn);
int getGeometryType(char *schema, char *table, char *geo_col_name);
int getGeometryMaxDims(char *schema, char *table, char *geo_col_name);
char *shapetypename(int num);
int parse_points(char *str, int num_points, double *x,double *y,double *z);
int num_points(char *str);
int num_lines(char *str);
char *scan_to_same_level(char *str);
int points_per_sublist( char *str, int *npoints, long max_lists);
int reverse_points(int num_points, double *x, double *y, double *z, double *m);
int is_clockwise(int num_points,double *x,double *y,double *z);
int is_bigendian(void);
SHPObject * shape_creator_wrapper_WKB(byte *str, int idx);
int get_postgis_major_version(void);
static void parse_table(char *spec);
static int create_usrquerytable(void);

/* WKB functions */
SHPObject * create_polygon2D_WKB(byte *wkb);
SHPObject * create_polygon3D_WKB(byte *wkb);
SHPObject * create_polygon4D_WKB(byte *wkb);
SHPObject * create_multipoint2D_WKB(byte *wkb);
SHPObject * create_multipoint3D_WKB(byte *wkb);
SHPObject * create_multipoint4D_WKB(byte *wkb);
SHPObject * create_point2D_WKB(byte *wkb);
SHPObject * create_point3D_WKB(byte *wkb);
SHPObject * create_point4D_WKB(byte *wkb);
SHPObject * create_multiline2D_WKB (byte *wkb);
SHPObject * create_multiline3D_WKB (byte *wkb);
SHPObject * create_multiline4D_WKB (byte *wkb);
SHPObject * create_line2D_WKB(byte *wkb);
SHPObject * create_line3D_WKB(byte *wkb);
SHPObject * create_line4D_WKB(byte *wkb);
SHPObject * create_multipolygon2D_WKB(byte *wkb);
SHPObject * create_multipolygon3D_WKB(byte *wkb);
SHPObject * create_multipolygon4D_WKB(byte *wkb);
byte getbyte(byte *c);
void skipbyte(byte **c);
byte popbyte(byte **c);
uint32 popint(byte **c);
uint32 getint(byte *c);
void skipint(byte **c);
double popdouble(byte **c);
void skipdouble(byte **c);
void dump_wkb(byte *wkb);
byte * HexDecode(byte *hex);

#define WKBZOFFSET 0x80000000
#define WKBMOFFSET 0x40000000
#define ZMFLAG(x) (((x)&((WKBZOFFSET)+(WKBMOFFSET)))>>30)


static void exit_nicely(PGconn *conn){
	PQfinish(conn);
	exit(1);
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
	shape_creator = NULL;
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
#ifdef DEBUG
	FILE *debug;
#endif

	if ( getenv("ROWBUFLEN") ) rowbuflen=atoi(getenv("ROWBUFLEN"));

	if ( ARGC == 1 ) {
		usage(0);
	}

	if ( ! parse_commandline(ARGC, ARGV) ) {
                printf("\n**ERROR** invalid option or command parameters\n\n");
		usage(2);
	}

	/* Use table name as shapefile name */
        if(shp_file == NULL) shp_file = table;

	/* Make a connection to the specified database, and exit on failure */
	conn = PQconnectdb("");
	if (PQstatus(conn) == CONNECTION_BAD) {
		printf( "%s", PQerrorMessage(conn));
		exit_nicely(conn);
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
	if ( ! initialize() ) exit_nicely(conn);
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
		exit_nicely(conn);
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
#if VERBOSE > 2
	printf( "MAINSCAN: %s\n", main_scan_query);
#endif
	free(main_scan_query);
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		printf( "MainScanQuery: %s", PQerrorMessage(conn));
		exit_nicely(conn);
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
#if VERBOSE 
		fprintf(stdout, "X"); fflush(stdout);
#endif
		res = PQexec(conn, fetchquery);
		if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
			printf( "RecordFetch: %s",
				PQerrorMessage(conn));
			exit_nicely(conn);
		}

		/* No more rows, break the loop */
		if ( ! PQntuples(res) ) {
			PQclear(res);
			break;
		}

		for(i=0; i<PQntuples(res); i++)
		{
			/* Add record in all output files */
			if ( ! addRecord(res, i, row) ) exit_nicely(conn);
			row++; 
		}

#if VERBOSE > 2
		printf("End of result, clearing..."); fflush(stdout);
#endif
		PQclear(res);
#if VERBOSE > 2
		printf("Done.\n");
#endif
	}
	printf(" [%d rows].\n", row);

	DBFClose(dbf);
	if (shp) SHPClose(shp);
	exit_nicely(conn);


#ifdef DEBUG
	fclose(debug);
#endif	 /* DEBUG */

	return 0;
}



SHPObject *
shape_creator_wrapper_WKB(byte *str, int idx)
{
	byte *ptr = str;
	uint32 type;
	int ndims;
	int wkb_big_endian;

	// skip byte order
	//skipbyte(&ptr);
	wkb_big_endian = ! popbyte(&ptr);
	if ( wkb_big_endian != big_endian )
	{
		printf( "Wrong WKB endiannes, dunno how to flip\n");
		exit(1);
	}

	// get type
	type = getint(ptr);

	ndims=2;
	if ( type&WKBZOFFSET ) ndims++;
	if ( type&WKBMOFFSET )
	{

		ndims++;
	}

	type &= ~WKBZOFFSET;
	type &= ~WKBMOFFSET;

	switch(type)
	{
		case MULTILINETYPE:
			if ( ndims == 2 )
				return create_multiline2D_WKB(str);
			else if ( ndims == 3 )
				return create_multiline3D_WKB(str);
			else if ( ndims == 4 )
				return create_multiline4D_WKB(str);
				
		case LINETYPE:
			if ( ndims == 2 )
				return create_line2D_WKB(str);
			else if ( ndims == 3 )
				return create_line3D_WKB(str);
			else if ( ndims == 4 )
				return create_line4D_WKB(str);

		case POLYGONTYPE:
			if ( ndims == 2 )
				return create_polygon2D_WKB(str);
			else if ( ndims == 3 )
				return create_polygon3D_WKB(str);
			else if ( ndims == 4 )
				return create_polygon4D_WKB(str);

		case MULTIPOLYGONTYPE:
			if ( ndims == 2 )
				return create_multipolygon2D_WKB(str);
			else if ( ndims == 3 )
				return create_multipolygon3D_WKB(str);
			else if ( ndims == 4 )
				return create_multipolygon4D_WKB(str);

		case POINTTYPE:
			if ( ndims == 2 )
				return create_point2D_WKB(str);
			else if ( ndims == 3 )
				return create_point3D_WKB(str);
			else if ( ndims == 4 )
				return create_point4D_WKB(str);

		case MULTIPOINTTYPE:
			if ( ndims == 2 )
				return create_multipoint2D_WKB(str);
			else if ( ndims == 3 )
				return create_multipoint3D_WKB(str);
			else if ( ndims == 4 )
				return create_multipoint4D_WKB(str);

		default:
			printf( "Unknown WKB type (%8.8lx) - (%s:%d)\n",
				type, __FILE__, __LINE__);
			return NULL;
	}
}

//reads points into x,y,z co-ord arrays
int parse_points(char *str, int num_points, double *x,double *y,double *z){
	int	keep_going;
	int	num_found= 0;
	char	*end_of_double;

	if ( (str == NULL) || (str[0] == 0) ){
		return 0;  //either null string or empty string
	}
	
	//look ahead for the "("
	str = strchr(str,'(') ;
	
	if ( (str == NULL) || (str[1] == 0) ){  // str[0] = '(';
		return 0;  //either didnt find "(" or its at the end of the string
	}
	str++;  //move forward one char
	keep_going = 1;
	while (keep_going == 1){
		
		//attempt to get the point
		//scanf is slow, so we use strtod()

		x[num_found] = (double)strtod(str,&end_of_double);
		if (end_of_double == str){
			return 0; //error occured (nothing parsed)
		}
		str = end_of_double;
		y[num_found] = strtod(str,&end_of_double);
		if (end_of_double == str){
			return 0; //error occured (nothing parsed)
		}
		str = end_of_double;
		z[num_found] = strtod(str,&end_of_double); //will be zero if error occured
		str = end_of_double;
		num_found++;

		str=strpbrk(str,",)");  // look for a "," or ")"
		if (str != NULL && str[0] == ','){
			str++;
		}
		keep_going = (str != NULL) && (str[0] != ')');		
	}
	return num_found; 
}





//returns how many points are in the first list in str
//
//  1. scan ahead looking for "("
//  2. find "," until hit a ")"
//  3. return number of points found
//	
// NOTE: doesnt actually parse the points, so if the 
//       str contains an invalid geometry, this could give
// 	   back the wrong answer.
//
// "(1 2 3, 4 5 6),(7 8, 9 10, 11 12 13)" => 2 (2nd list is not included)
int	num_points(char *str){
	int		keep_going;
	int		points_found = 1; //no "," if only one point (and last point)
						

	if ( (str == NULL) || (str[0] == 0) )
	{
		return 0;  //either null string or empty string
	}

	//look ahead for the "("

	str = strchr(str,'(') ;
	
	if ( (str == NULL) || (str[1] == 0) )  // str[0] = '(';
	{
		return 0;  //either didnt find "(" or its at the end of the string
	}

	keep_going = 1;
	while (keep_going) 
	{
		str=strpbrk(str,",)");  // look for a "," or ")"
		keep_going = (str != NULL);
		if (keep_going)  // found a , or )
		{
			if (str[0] == ')')
			{
				//finished
				return points_found;
			}
			else	//str[0] = ","
			{
				points_found++;
				str++; //move 1 char forward	
			}
		}
	}
	return points_found; // technically it should return an error.
}

//number of sublist in a string.

// Find the number of lines in a Multiline
// OR
// The number of rings in a Polygon
// OR
// The number of polygons in a multipolygon

// ( (..),(..),(..) )  -> 3
// ( ( (..),(..) ), ( (..) )) -> 2
// ( ) -> 0
// scan through the list, for every "(", depth (nesting) increases by 1
//				  for every ")", depth (nesting) decreases by 1
// if find a "(" at depth 1, then there is a sub list
//
// example: 
//      "(((..),(..)),((..)))"
//depth  12333223332112333210
//        +           +         increase here

int num_lines(char *str){
	int	current_depth = 0;
	int	numb_lists = 0;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()"); //look for "(" or ")"
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
					numb_lists ++;
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return numb_lists ;
			}
			str++;
		}
	}
	return numb_lists ; // probably should give an error
}



//simple scan-forward to find the next "(" at the same level
//  ( (), (),(), ),(...
//                 + return this location
char *scan_to_same_level(char *str){

	//scan forward in string looking for at "(" at the same level
	// as the one its already pointing at

	int	current_depth = 0;
	int  first_one=1;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()");
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				if (!(first_one))
				{
					if (current_depth == 0)
						return str;
				}
				else
					first_one = 0;  //ignore the first opening "("
				current_depth++;
			}
			if (str[0] == ')')
			{
				current_depth--;
			}

			str++;
		}
	}
	return str ; // probably should give an error
}






// Find out how many points are in each sublist, put the result in the array npoints[]
//  (for at most max_list sublists)
//
//  ( (L1),(L2),(L3) )  --> npoints[0] = points in L1,
//				    npoints[1] = points in L2,
//				    npoints[2] = points in L3
//
// We find these by, again, scanning through str looking for "(" and ")"
// to determine the current depth.  We dont actually parse the points.

int points_per_sublist( char *str, int *npoints, long max_lists){
	//scan through, noting depth and ","s

	int	current_depth = 0;
	int	current_list =-1 ;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"(),");  //find "(" or ")" or ","
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
				{
					current_list ++;
					if (current_list >=max_lists)
						return 1;			// too many sub lists found
					npoints[current_list] = 1;
				}
				// might want to return an error if depth>2
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return 1 ;
			}
			if (str[0] == ',')
			{
				if (current_depth==2)
				{
					npoints[current_list] ++;	
				}
			}

			str++;
		}
	}
	return 1 ; // probably should give an error
}

SHPObject *
create_multiline3D_WKB (byte *wkb)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *zm=NULL;
	int nparts=0, *part_index=NULL, totpoints=0, nlines=0;
	int li;
	int zmflag;

	// skip byteOrder
	skipbyte(&wkb);

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all lines in multiline
	 */
	nlines=popint(&wkb); // num_wkbLineStrings
#if VERBOSE > 2
	printf("Multiline with %d lines\n", nlines);
#endif

	part_index = (int *)malloc(sizeof(int)*(nlines));
	for (li=0; li<nlines; li++)
	{
		int npoints, pn;

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		npoints = popint(&wkb);

#if VERBOSE > 2
		printf("Line %d has %d points\n", li, npoints);
#endif

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));
		zm = realloc(zm, sizeof(double)*(totpoints+npoints));

		/* wkb now points at first point */
		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			zm[totpoints+pn] = popdouble(&wkb);
		}

		part_index[li] = totpoints;
		totpoints += npoints;
	}

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, nparts,
			part_index, NULL, totpoints,
			x, y, NULL, zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, nparts,
			part_index, NULL, totpoints,
			x, y, zm, NULL);
	}

	free(part_index); free(x); free(y); free(zm);

	return obj;
}

SHPObject *
create_multiline4D_WKB (byte *wkb)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *z=NULL, *m=NULL;
	int nparts=0, *part_index=NULL, totpoints=0, nlines=0;
	int li;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all lines in multiline
	 */
	nlines=popint(&wkb); // num_wkbLineStrings
#if VERBOSE > 2
	printf("Multiline with %d lines\n", nlines);
#endif

	part_index = (int *)malloc(sizeof(int)*(nlines));
	for (li=0; li<nlines; li++)
	{
		int npoints, pn;

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		npoints = popint(&wkb);

#if VERBOSE > 2
		printf("Line %d has %d points\n", li, npoints);
#endif

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));
		z = realloc(z, sizeof(double)*(totpoints+npoints));
		m = realloc(m, sizeof(double)*(totpoints+npoints));

		/* wkb now points at first point */
		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			z[totpoints+pn] = popdouble(&wkb);
			m[totpoints+pn] = popdouble(&wkb);
		}

		part_index[li] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(outshptype, -1, nparts,
		part_index, NULL, totpoints,
		x, y, z, m);

	free(part_index); free(x); free(y); free(z); free(m);

	return obj;
}

SHPObject *
create_multiline2D_WKB (byte *wkb)
{
	double *x=NULL, *y=NULL;
	int nparts=0, *part_index=NULL, totpoints=0, nlines=0;
	int li;
	SHPObject *obj;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all lines in multiline
	 */
	nlines=popint(&wkb); // num_wkbLineStrings
#if VERBOSE > 2
	printf("Multiline with %d lines\n", nlines);
#endif

	part_index = (int *)malloc(sizeof(int)*(nlines));
	for (li=0; li<nlines; li++)
	{
		int npoints, pn;

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		npoints = popint(&wkb);

#if VERBOSE > 2
		printf("Line %d has %d points\n", li, npoints);
#endif

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));

		/* wkb now points at first point */
		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
		}

		part_index[li] = totpoints;
		totpoints += npoints;
	}


	obj = SHPCreateObject(outshptype, -1, nparts,
		part_index, NULL, totpoints,
		x, y, NULL, NULL);

	free(part_index); free(x); free(y);

	return obj;
}

SHPObject *
create_line4D_WKB (byte *wkb)
{
	double *x=NULL, *y=NULL, *z=NULL, *m=NULL;
	uint32 npoints=0, pn;
	SHPObject *obj;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

#if VERBOSE > 2
	printf("Line has %lu points\n", npoints);
#endif

	x = malloc(sizeof(double)*(npoints));
	y = malloc(sizeof(double)*(npoints));
	z = malloc(sizeof(double)*(npoints));
	m = malloc(sizeof(double)*(npoints));

	/* wkb now points at first point */
	for (pn=0; pn<npoints; pn++)
	{
		x[pn] = popdouble(&wkb);
		y[pn] = popdouble(&wkb);
		z[pn] = popdouble(&wkb);
		m[pn] = popdouble(&wkb);
	}

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
		npoints, x, y, z, m);
	free(x); free(y); free(z); free(m);

	return obj;
}

SHPObject *
create_line3D_WKB (byte *wkb)
{
	double *x=NULL, *y=NULL, *zm=NULL;
	uint32 npoints=0, pn;
	SHPObject *obj;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

#if VERBOSE > 2
	printf("Line has %lu points\n", npoints);
#endif

	x = malloc(sizeof(double)*(npoints));
	y = malloc(sizeof(double)*(npoints));
	zm = malloc(sizeof(double)*(npoints));

	/* wkb now points at first point */
	for (pn=0; pn<npoints; pn++)
	{
		x[pn] = popdouble(&wkb);
		y[pn] = popdouble(&wkb);
		zm[pn] = popdouble(&wkb);
	}

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			npoints, x, y, NULL, zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			npoints, x, y, zm, NULL);
	}

	free(x); free(y); free(zm);

	return obj;
}

SHPObject *
create_line2D_WKB (byte *wkb)
{
	double *x=NULL, *y=NULL, *z=NULL;
	uint32 npoints=0, pn;
	SHPObject *obj;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

#if VERBOSE > 2
	printf("Line has %lu points\n", npoints);
#endif

	x = malloc(sizeof(double)*(npoints));
	y = malloc(sizeof(double)*(npoints));

	/* wkb now points at first point */
	for (pn=0; pn<npoints; pn++)
	{
		x[pn] = popdouble(&wkb);
		y[pn] = popdouble(&wkb);
	}

	obj = SHPCreateSimpleObject(outshptype, npoints, x, y, z);
	free(x); free(y); 

	return obj;
}

SHPObject *
create_point4D_WKB(byte *wkb)
{
	SHPObject *obj;
	double x, y, z, m;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	x = popdouble(&wkb);
	y = popdouble(&wkb);
	z = popdouble(&wkb);
	m = popdouble(&wkb);

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
		1, &x, &y, &z, &m);

	return obj;
}

SHPObject *
create_point3D_WKB(byte *wkb)
{
	SHPObject *obj;
	double x, y, zm;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	x = popdouble(&wkb);
	y = popdouble(&wkb);
	zm = popdouble(&wkb);

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			1, &x, &y, NULL, &zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			1, &x, &y, &zm, NULL);
	}

	return obj;
}

SHPObject *
create_point2D_WKB(byte *wkb)
{
	SHPObject *obj;
	double x, y;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	x = popdouble(&wkb);
	y = popdouble(&wkb);

	obj = SHPCreateSimpleObject(outshptype, 1, &x, &y, NULL);

	return obj;
}

SHPObject *
create_multipoint4D_WKB(byte *wkb)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *z=NULL, *m=NULL;
	int npoints;
	int pn;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

	x = (double *)malloc(sizeof(double)*npoints);
	y = (double *)malloc(sizeof(double)*npoints);
	z = (double *)malloc(sizeof(double)*npoints);
	m = (double *)malloc(sizeof(double)*npoints);

	for (pn=0; pn<npoints; pn++)
	{
		skipbyte(&wkb); // byteOrder
		skipint(&wkb);  // wkbType
		x[pn]=popdouble(&wkb);
		y[pn]=popdouble(&wkb);
		z[pn]=popdouble(&wkb);
		m[pn]=popdouble(&wkb);
	}

	obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
		npoints, x, y, z, m);

	free(x); free(y); free(z); free(m);

	return obj;
}

SHPObject *
create_multipoint3D_WKB(byte *wkb)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *zm=NULL;
	uint32 npoints;
	uint32 pn;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

	x = (double *)malloc(sizeof(double)*npoints);
	y = (double *)malloc(sizeof(double)*npoints);
	zm = (double *)malloc(sizeof(double)*npoints);

	for (pn=0; pn<npoints; pn++)
	{
		skipbyte(&wkb); // byteOrder
		skipint(&wkb);  // wkbType
		x[pn]=popdouble(&wkb);
		y[pn]=popdouble(&wkb);
		zm[pn]=popdouble(&wkb);
	}

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			npoints, x, y, NULL, zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, 0, NULL, NULL,
			npoints, x, y, zm, NULL);
	}

	free(x); free(y); free(zm);

	return obj;
}

SHPObject *
create_multipoint2D_WKB(byte *wkb)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL;
	uint32 npoints;
	uint32 pn;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	npoints = popint(&wkb);

	x = (double *)malloc(sizeof(double)*npoints);
	y = (double *)malloc(sizeof(double)*npoints);

	for (pn=0; pn<npoints; pn++)
	{
		skipbyte(&wkb); // byteOrder
		skipint(&wkb);  // wkbType
		x[pn]=popdouble(&wkb);
		y[pn]=popdouble(&wkb);

	}

	obj = SHPCreateSimpleObject(outshptype,npoints,x,y,NULL);
	free(x); free(y); 

	return obj;
}

SHPObject *
create_polygon2D_WKB(byte *wkb)
{
	SHPObject *obj;
	int ri, nrings, totpoints=0, *part_index=NULL;
	double *x=NULL, *y=NULL, *z=NULL;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all rings
	 */
	nrings = popint(&wkb);
#if VERBOSE > 2
	printf("Polygon with %d rings\n", nrings);
#endif
	part_index = (int *)malloc(sizeof(int)*nrings);
	for (ri=0; ri<nrings; ri++)
	{
		int pn;
		int npoints = popint(&wkb);

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));

		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
		}

		/*
		 * First ring should be clockwise,
		 * other rings should be counter-clockwise
		 */
		if ( !ri ) {
			if ( ! is_clockwise(npoints, x+totpoints,
						y+totpoints, NULL) )
			{
#if VERBOSE > 2
				printf("Forcing CW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, NULL, NULL);
			}
		} else {
			if ( is_clockwise(npoints, x+totpoints,
						y+totpoints, NULL) )
			{
#if VERBOSE > 2
				printf("Forcing CCW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, NULL, NULL);
			}
		}

		part_index[ri] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(outshptype, -1, nrings,
		part_index, NULL, totpoints,
		x, y, z, NULL);

	free(part_index);
	free(x); free(y); 

	return obj;
}

SHPObject *
create_polygon4D_WKB(byte *wkb)
{
	SHPObject *obj;
	int ri, nrings, totpoints=0, *part_index=NULL;
	double *x=NULL, *y=NULL, *z=NULL, *m=NULL;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all rings
	 */
	nrings = popint(&wkb);
#if VERBOSE > 2
	printf("Polygon with %d rings\n", nrings);
#endif
	part_index = (int *)malloc(sizeof(int)*nrings);
	for (ri=0; ri<nrings; ri++)
	{
		int pn;
		int npoints = popint(&wkb);

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));
		z = realloc(z, sizeof(double)*(totpoints+npoints));
		m = realloc(m, sizeof(double)*(totpoints+npoints));

		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			z[totpoints+pn] = popdouble(&wkb);
			m[totpoints+pn] = popdouble(&wkb);
		}

		/*
		 * First ring should be clockwise,
		 * other rings should be counter-clockwise
		 */
		if ( !ri ) {
			if ( ! is_clockwise(npoints, x+totpoints,
						y+totpoints, z+totpoints) ) {
#if VERBOSE > 2
				printf("Forcing CW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, z+totpoints, m+totpoints);
			}
		} else {
			if ( is_clockwise(npoints, x+totpoints,
						y+totpoints, z+totpoints) ) {
#if VERBOSE > 2
				printf("Forcing CCW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, z+totpoints, m+totpoints);
			}
		}

		part_index[ri] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(outshptype, -1, nrings,
		part_index, NULL, totpoints,
		x, y, z, m);

	free(part_index);
	free(x); free(y); free(z); free(m);

	return obj;
}

SHPObject *
create_polygon3D_WKB(byte *wkb)
{
	SHPObject *obj;
	int ri, nrings, totpoints=0, *part_index=NULL;
	double *x=NULL, *y=NULL, *zm=NULL, *z=NULL;
	int zmflag;
	
	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all rings
	 */
	nrings = popint(&wkb);
#if VERBOSE > 2
	printf("Polygon with %d rings\n", nrings);
#endif
	part_index = (int *)malloc(sizeof(int)*nrings);
	for (ri=0; ri<nrings; ri++)
	{
		int pn;
		int npoints = popint(&wkb);

		x = realloc(x, sizeof(double)*(totpoints+npoints));
		y = realloc(y, sizeof(double)*(totpoints+npoints));
		zm = realloc(zm, sizeof(double)*(totpoints+npoints));

		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			zm[totpoints+pn] = popdouble(&wkb);
		}

		/*
		 * First ring should be clockwise,
		 * other rings should be counter-clockwise
		 */

		// Set z to NULL if TYPEM
		if ( zmflag == 1 ) z = NULL;
		else z = zm+totpoints;

		if ( !ri ) {
			if ( ! is_clockwise(npoints, x+totpoints,
						y+totpoints, z) ) {
#if VERBOSE > 2
				printf("Forcing CW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, zm+totpoints, NULL);
			}
		} else {
			if ( is_clockwise(npoints, x+totpoints,
						y+totpoints, z) ) {
#if VERBOSE > 2
				printf("Forcing CCW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, zm+totpoints, NULL);
			}
		}

		part_index[ri] = totpoints;
		totpoints += npoints;
	}

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, nrings,
			part_index, NULL, totpoints,
			x, y, NULL, zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, nrings,
			part_index, NULL, totpoints,
			x, y, zm, NULL);
	}

	free(part_index);
	free(x); free(y); free(zm);

	return obj;
}

SHPObject *
create_multipolygon2D_WKB(byte *wkb)
{
	SHPObject *obj;
	uint32 nrings, nparts;
	uint32 npolys;
	uint32 totpoints=0;
	int *part_index=NULL;
	uint32 pi;
	double *x=NULL, *y=NULL;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));

	/*
	 * Scan all polygons in multipolygon
	 */
	nparts = 0;
	npolys = popint(&wkb);  // num_wkbPolygons
#if VERBOSE > 2
	printf("Multipolygon with %lu polygons\n", npolys);
#endif

	/*
	 * Now wkb points to a WKBPolygon structure
	 */
	for (pi=0; pi<npolys; pi++)
	{
		uint32 ri; // ring index

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		/*
		 * Find total number of points and
		 * fill part index
		 */

		nrings = popint(&wkb);
		part_index = (int *)realloc(part_index,
				sizeof(int)*(nparts+nrings));

#if VERBOSE > 2
		printf("Polygon %lu has %lu rings\n", pi, nrings);
#endif

		// wkb now points at first ring
		for (ri=0; ri<nrings; ri++)
		{
			uint32 pn; // point number
			uint32 npoints;

			npoints = popint(&wkb);

#if VERBOSE > 2
			printf("Ring %lu has %lu points\n", ri, npoints);
#endif

			x = realloc(x, sizeof(double)*(totpoints+npoints));
			y = realloc(y, sizeof(double)*(totpoints+npoints));

			/* wkb now points at first point */
			for (pn=0; pn<npoints; pn++)
			{
				x[totpoints+pn] = popdouble(&wkb);
				y[totpoints+pn] = popdouble(&wkb);
#if VERBOSE > 3
	printf("Point%lu (%f,%f)\n", pn, x[totpoints+pn], y[totpoints+pn]);
#endif
			}

			/*
			 * First ring should be clockwise,
			 * other rings should be counter-clockwise
			 */
			if ( !ri ) {
				if (!is_clockwise(npoints, x+totpoints,
							y+totpoints, NULL))
				{
#if VERBOSE > 2
					printf("Forcing CW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, NULL, NULL);
				}
			} else {
				if (is_clockwise(npoints, x+totpoints,
							y+totpoints, NULL))
				{
#if VERBOSE > 2
					printf("Forcing CCW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, NULL, NULL);
				}
			}

			part_index[nparts+ri] = totpoints;
			totpoints += npoints;
		}
#if VERBOSE > 2
		printf("End of rings\n");
#endif
		nparts += nrings;
	}

#if VERBOSE > 2
	printf("End of polygons\n");
#endif

	obj = SHPCreateObject(outshptype, -1, nparts,
		part_index, NULL, totpoints,
		x, y, NULL, NULL);

#if VERBOSE > 2
	printf("Object created\n");
#endif

	free(part_index);
	free(x); free(y); 

	return obj;
}

SHPObject *
create_multipolygon3D_WKB(byte *wkb)
{
	SHPObject *obj;
	int nrings, nparts;
	uint32 npolys;
	int totpoints=0;
	int *part_index=NULL;
	int pi;
	double *x=NULL, *y=NULL, *z=NULL, *zm=NULL;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));
	
	/*
	 * Scan all polygons in multipolygon
	 */
	nparts = 0;
	npolys = popint(&wkb);  // num_wkbPolygons
#if VERBOSE > 2
	printf("Multipolygon with %lu polygons\n", npolys);
#endif

	/*
	 * Now wkb points to a WKBPolygon structure
	 */
	for (pi=0; pi<npolys; pi++)
	{
		int ri; // ring index

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		/*
		 * Find total number of points and
		 * fill part index
		 */

		nrings = popint(&wkb);
		part_index = (int *)realloc(part_index,
				sizeof(int)*(nparts+nrings));

#if VERBOSE > 2
		printf("Polygon %d has %d rings\n", pi, nrings);
#endif

		// wkb now points at first ring
		for (ri=0; ri<nrings; ri++)
		{
			int pn; // point number
			int npoints;

			npoints = popint(&wkb);

#if VERBOSE > 2
			printf("Ring %d has %d points\n", ri, npoints);
#endif

			x = realloc(x, sizeof(double)*(totpoints+npoints));
			y = realloc(y, sizeof(double)*(totpoints+npoints));
			zm = realloc(zm, sizeof(double)*(totpoints+npoints));

			/* wkb now points at first point */
			for (pn=0; pn<npoints; pn++)
			{
				x[totpoints+pn] = popdouble(&wkb);
				y[totpoints+pn] = popdouble(&wkb);
				zm[totpoints+pn] = popdouble(&wkb);
#if VERBOSE > 3
	printf("Point%d (%f,%f)\n", pn, x[totpoints+pn], y[totpoints+pn]);
#endif
			}

			/*
			 * First ring should be clockwise,
			 * other rings should be counter-clockwise
			 */

			// Set z to NULL if TYPEM
			if ( zmflag == 1 ) z = NULL;
			else z = zm+totpoints;

			if ( !ri ) {
				if (!is_clockwise(npoints, x+totpoints,
							y+totpoints, z))
				{
#if VERBOSE > 2
					printf("Forcing CW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, zm+totpoints, NULL);
				}
			} else {
				if (is_clockwise(npoints, x+totpoints,
							y+totpoints, z))
				{
#if VERBOSE > 2
					printf("Forcing CCW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, zm+totpoints, NULL);
				}
			}

			part_index[nparts+ri] = totpoints;
			totpoints += npoints;
		}
#if VERBOSE > 2
		printf("End of rings\n");
#endif
		nparts += nrings;
	}

#if VERBOSE > 2
	printf("End of polygons\n");
#endif

	if ( zmflag == 1 ) {
		obj = SHPCreateObject(outshptype, -1, nparts,
			part_index, NULL, totpoints,
			x, y, NULL, zm);
	} else {
		obj = SHPCreateObject(outshptype, -1, nparts,
			part_index, NULL, totpoints,
			x, y, zm, NULL);
	}

#if VERBOSE > 2
	printf("Object created\n");
#endif

	free(part_index);
	free(x); free(y); free(zm);

	return obj;
}

SHPObject *
create_multipolygon4D_WKB(byte *wkb)
{
	SHPObject *obj;
	int nrings, nparts;
	uint32 npolys;
	int totpoints=0;
	int *part_index=NULL;
	int pi;
	double *x=NULL, *y=NULL, *z=NULL, *m=NULL;
	int zmflag;

	// skip byteOrder 
	skipbyte(&wkb); 

	// extract zmflag from type
	zmflag = ZMFLAG(popint(&wkb));
	
	/*
	 * Scan all polygons in multipolygon
	 */
	nparts = 0;
	npolys = popint(&wkb);  // num_wkbPolygons
#if VERBOSE > 2
	printf("Multipolygon with %lu polygons\n", npolys);
#endif

	/*
	 * Now wkb points to a WKBPolygon structure
	 */
	for (pi=0; pi<npolys; pi++)
	{
		int ri; // ring index

		// skip byteOrder and wkbType
		skipbyte(&wkb); skipint(&wkb);

		/*
		 * Find total number of points and
		 * fill part index
		 */

		nrings = popint(&wkb);
		part_index = (int *)realloc(part_index,
				sizeof(int)*(nparts+nrings));

#if VERBOSE > 2
		printf("Polygon %d has %d rings\n", pi, nrings);
#endif

		// wkb now points at first ring
		for (ri=0; ri<nrings; ri++)
		{
			int pn; // point number
			int npoints;

			npoints = popint(&wkb);

#if VERBOSE > 2
			printf("Ring %d has %d points\n", ri, npoints);
#endif

			x = realloc(x, sizeof(double)*(totpoints+npoints));
			y = realloc(y, sizeof(double)*(totpoints+npoints));
			z = realloc(z, sizeof(double)*(totpoints+npoints));
			m = realloc(m, sizeof(double)*(totpoints+npoints));

			/* wkb now points at first point */
			for (pn=0; pn<npoints; pn++)
			{
				x[totpoints+pn] = popdouble(&wkb);
				y[totpoints+pn] = popdouble(&wkb);
				z[totpoints+pn] = popdouble(&wkb);
				m[totpoints+pn] = popdouble(&wkb);
#if VERBOSE > 3
	printf("Point%d (%f,%f)\n", pn, x[totpoints+pn], y[totpoints+pn]);
#endif
			}

			/*
			 * First ring should be clockwise,
			 * other rings should be counter-clockwise
			 */
			if ( !ri ) {
				if (!is_clockwise(npoints, x+totpoints,
							y+totpoints, z+totpoints))
				{
#if VERBOSE > 2
					printf("Forcing CW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, z+totpoints, m+totpoints);
				}
			} else {
				if (is_clockwise(npoints, x+totpoints,
							y+totpoints, z+totpoints))
				{
#if VERBOSE > 2
					printf("Forcing CCW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, z+totpoints, m+totpoints);
				}
			}

			part_index[nparts+ri] = totpoints;
			totpoints += npoints;
		}
#if VERBOSE > 2
		printf("End of rings\n");
#endif
		nparts += nrings;
	}

#if VERBOSE > 2
	printf("End of polygons\n");
#endif

	obj = SHPCreateObject(outshptype, -1, nparts,
		part_index, NULL, totpoints,
		x, y, z, m);

#if VERBOSE > 2
	printf("Object created\n");
#endif

	free(part_index);
	free(x); free(y); free(z); free(m);

	return obj;
}

//Reverse the clockwise-ness of the point list...
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

//return 1 if the points are in clockwise order
int is_clockwise(int num_points, double *x, double *y, double *z)
{
	int i;
	double x_change,y_change,area;
	double *x_new, *y_new; //the points, translated to the origin for safer accuracy

	x_new = (double *)malloc(sizeof(double) * num_points);	
	y_new = (double *)malloc(sizeof(double) * num_points);	
	area=0.0;
	x_change = x[0];
	y_change = y[0];

	for(i=0; i < num_points ; i++){
		x_new[i] = x[i] - x_change;
		y_new[i] = y[i] - y_change;
	}

	for(i=0; i < num_points - 1; i++){
		area += (x[i] * y[i+1]) - (y[i] * x[i+1]); //calculate the area	
	}
	if(area > 0 ){
		free(x_new); free(y_new);
		return 0; //counter-clockwise
	}else{
		free(x_new); free(y_new);
		return 1; //clockwise
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
	char *val;
	char *v;
	int junk;

	for (j=0; j<nFields; j++)
	{
		SHPObject *obj;

		/* Default (not geometry) attribute */
		if (type_ary[j] != 9)
		{
			/*
			 * Transform NULL numbers to '0'
			 * This is because the shapelibe
			 * won't easly take care of setting
			 * nulls unless paying the acquisition
			 * of a bug in long integer values
			 */
			if ( PQgetisnull(res, residx, j) &&
				( type_ary[j] == 1 || type_ary[j] == 2 ) )
			{
				val = "0";
			}
			else
			{
				val = PQgetvalue(res, residx, j);
			}
#if VERBOSE > 1
fprintf(stdout, "s"); fflush(stdout);
#endif
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

		// Handle NULL shapes
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

		if ( ! binary )
		{
			v = PQgetvalue(res, residx, j);
#ifndef HEXWKB
			val = PQunescapeBytea(v, &junk);
#else
			if ( pgis_major_version > 0 )
			{
				val = PQunescapeBytea(v, &junk);
			}
			else
			{
				val = HexDecode(v);
			}
#endif // HEXWKB
#if VERBOSE > 2
		dump_wkb(val);
#endif // VERBOSE > 2
		}
		else // binary
		{
			val = (char *)PQgetvalue(res, residx, j);
		}

#if VERBOSE > 1
		fprintf(stdout, "g"); fflush(stdout);
#endif

		obj = shape_creator_wrapper_WKB(val, row);
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

		if ( ! binary ) free(val);
	}

#if VERBOSE > 2
	printf("Finished adding record %d\n", row);
#endif

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
		exit_nicely(conn);
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
	char *geo_str; // the geometry type string
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
usage(status)
{
	printf("RCSID: %s\n", rcsid);
	printf("USAGE: pgsql2shp [<options>] <database> [<schema>.]<table>\n");
	printf("       pgsql2shp [<options>] <database> <query>\n");
	printf("\n");
       	printf("OPTIONS:\n");
       	printf("  -f <filename>  Use this option to specify the name of the file\n");
       	printf("     to create.\n");
       	printf("  -h <host>  Allows you to specify connection to a database on a\n");
	printf("     machine other than the default.\n");
       	printf("  -p <port>  Allows you to specify a database port other than the default.\n");
       	printf("  -P <password>  Connect to the database with the specified password.\n");
       	printf("  -u <user>  Connect to the database as the specified user.\n");
	printf("  -g <geometry_column> Specify the geometry column to be exported.\n");
	printf("  -b Use a binary cursor.\n");
	printf("  -r Raw mode. Do not assume table has been created by \n");
	printf("     the loader. This would not unescape attribute names\n");
	printf("     and will not skip the 'gid' attribute.");
       	printf("\n");
       	exit (status);
}

/* Parse command line parameters */
int
parse_commandline(int ARGC, char **ARGV)
{
	int c, curindex;
	char buf[1024];

	buf[1023] = '\0'; // just in case...

	/* Parse command line */
        while ((c = getopt(ARGC, ARGV, "bf:h:du:p:P:g:r")) != EOF){
               switch (c) {
               case 'b':
                    binary = 1;
                    break;
               case 'f':
                    shp_file = optarg;
                    break;
               case 'h':
        	    //setenv("PGHOST", optarg, 1);
		    snprintf(buf, 255, "PGHOST=%s", optarg);
		    putenv(strdup(buf));
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
		    //setenv("PGUSER", optarg, 1);
		    snprintf(buf, 255, "PGUSER=%s", optarg);
		    putenv(strdup(buf));
                    break;
               case 'p':
		    //setenv("PGPORT", optarg, 1);
		    snprintf(buf, 255, "PGPORT=%s", optarg);
		    putenv(strdup(buf));
                    break;
	       case 'P':
		    //setenv("PGPASSWORD", optarg, 1);
		    snprintf(buf, 255, "PGPASSWORD=%s", optarg);
		    putenv(strdup(buf));
		    break;
	       case 'g':
		    geo_col_name = optarg;
		    break;
               case '?':
                    return 0;
               default:
		    return 0;
               }
        }

        curindex=0;
        for (; optind<ARGC; optind++){
                if (curindex == 0) {
			//setenv("PGDATABASE", ARGV[optind], 1);
		    	snprintf(buf, 255, "PGDATABASE=%s", ARGV[optind]);
		    	putenv(strdup(buf));
                }else if(curindex == 1){
			parse_table(ARGV[optind]);

                }
                curindex++;
        }
        if (curindex < 2) return 0;
	return 1;
}

int
get_postgis_major_version()
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

	res = PQexec(conn, query);
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
initialize()
{
	PGresult *res;
	char *query;
	int i;
	char buf[256];
	int tmpint;
	int geo_oid; // geometry oid
	int geom_fld = -1;
	char *mainscan_flds[256];
	int mainscan_nflds=0;
	int size;
	int gidfound=0;

	/* Detect postgis version */
	pgis_major_version = get_postgis_major_version();

	/* Detect host endiannes */
	big_endian = is_bigendian();

	/* Query user attributes name, type and size */

	size = strlen(table);
	if ( schema ) size += strlen(schema);
	size += 256;

	query = (char *)malloc(size);
	if ( ! query ) return 0; // out of virtual memory

	if ( schema )
	{
		sprintf(query, "SELECT a.attname, a.atttypid, a.attlen FROM "
			"pg_attribute a, pg_class c, pg_namespace n WHERE "
			"n.nspname = '%s' AND a.attrelid = c.oid AND "
			"n.oid = c.relnamespace AND "
			"a.atttypid != 0 AND "
			"a.attnum > 0 AND c.relname = '%s'", schema, table);
	}
	else
	{
		sprintf(query, "SELECT a.attname, a.atttypid, a.attlen FROM "
			"pg_attribute a, pg_class c WHERE "
			"a.attrelid = c.oid and a.attnum > 0 AND "
			"a.atttypid != 0 AND "
			"c.relname = '%s'", table);
	}


	/* Exec query */
#if VERBOSE > 2
	printf( "Attribute query: %s\n", query);
#endif
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
		int type, size;
		char *fname; // pgsql attribute name
		char *ptr;
		char field_name[32]; // dbf version of field name

		fname = PQgetvalue(res, i, 0);
		type = atoi(PQgetvalue(res, i, 1));
		size = atoi(PQgetvalue(res, i, 2));

//printf( "A: %s, T: %d, S: %d\n", fname, type, size);
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
			else if (!strcmp(geo_col_name,fname))
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


		if(strlen(ptr) <32) strcpy(field_name, ptr);
		else
		{
			/*
			 * TODO: you find an appropriate name if
			 * running in RAW mode
			 */
			printf("dbf attribute name %s is too long, must be "
				"less than 32 characters.\n", ptr);
			return 0;
		}


		/* make UPPERCASE */
		for(j=0; j < strlen(field_name); j++)
			field_name[j] = toupper(field_name[j]);

		/* 
		 * make sure the fields all have unique names,
		 * 10-digit limit on dbf names...
		 */
		for(j=0;j<i;j++)
		{
			if(strncmp(field_name, PQgetvalue(res, j, 0),10) == 0)
			{
		printf("\nWarning: Field '%s' has first 10 characters which "
			"duplicate a previous field name's.\n"
			"Renaming it to be: '",field_name);
				strncpy(field_name,field_name,9);
				field_name[9] = 0;
				sprintf(field_name,"%s%d",field_name,i);
				printf("%s'\n\n",field_name);
			}
		}

		/*
		 * Find appropriate type of dbf attributes
		 */

		/* integer type */
		if(type == 20 || type == 21 || type == 23)
		{
			if(DBFAddField(dbf, field_name,FTInteger,16,0) == -1)
			{
				printf( "error - Field could not "
					"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=1;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}
		
		/* double type */
		if(type == 700 || type == 701 || type == 1700 )
		{
			if(DBFAddField(dbf, field_name,FTDouble,32,10) == -1)
			{
				printf( "error - Field could not "
						"be created.\n");
				return 0;
			}
			type_ary[mainscan_nflds]=2;
			mainscan_flds[mainscan_nflds++] = fname;
			continue;
		}

		/*
		 * date field, which we store as a string so we need
		 * more width in the column
		 */
		if(type == 1082)
		{
			size = 10;
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
		 * For variable-sized fields we'll use max size in table
		 * as dbf field size
		 */
		else if(size == -1)
		{
			size = getMaxFieldSize(conn, schema, table, fname);
			if ( size == -1 ) return 0;
			if ( ! size ) size = 32; // might 0 be a good size ?
		}
//printf( "FIELD_NAME: %s, SIZE: %d\n", field_name, size);
		
		/* generic type (use string representation) */
		if(DBFAddField(dbf, field_name,FTString,size,0) == -1)
		{
			printf( "error - Field could not "
					"be created.\n");
			return 0;
		}
		type_ary[mainscan_nflds]=3;
		mainscan_flds[mainscan_nflds++] = fname;
	}

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
				shape_creator = NULL;
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


#ifdef HEXWKB
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'XDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'XDR')",
						mainscan_flds[i]);
				}
#else
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'XDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'XDR')::bytea", mainscan_flds[i]);
				}
#endif

			}
			else // little_endian
			{

#ifdef HEXWKB
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'NDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'NDR')",
						mainscan_flds[i]);
				}
#else // ndef HEXWKB
				if ( pgis_major_version > 0 )
				{
					sprintf(buf, "asEWKB(setSRID(\"%s\", -1), 'NDR')", mainscan_flds[i]);
				}
				else
				{
					sprintf(buf, "asbinary(\"%s\", 'NDR')::bytea",
						mainscan_flds[i]);
				}
#endif // def HEXWKB

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

	// Order by 'gid' (if found)
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

	//( this is ugly: don't forget counting the length 
	// when changing the fixed query strings )

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

/*
 * Input is a NULL-terminated string.
 * Output is a binary string.
 */
byte *
HexDecode(byte *hex)
{
	byte *ret, *retptr, *hexptr;
	byte byt;
	int len;
	
	len = strlen(hex)/2;
	ret = (byte *)malloc(len);
	if ( ! ret )  {
		printf( "Out of virtual memory\n");
		exit(1);
	}

	//printf("Decoding %d bytes", len); fflush(stdout);
	hexptr = hex; retptr = ret;

	// for postgis > 0.9.x skip SRID=#; if found
	if ( pgis_major_version > 0 )
	{
		if ( hexptr[0] == 'S' )
		{
			hexptr = strchr(hexptr, ';');
			hexptr++;
		}
	}

	while (*hexptr)
	{
		/*
		 * All these checks on WKB correctness
		 * can be avoided, are only here because
		 * I keep getting segfaults whereas
		 * bytea unescaping works fine...
		 */

		//printf("%c", *hexptr);
		if ( *hexptr < 58 && *hexptr > 47 )
			byt = (((*hexptr)-48)<<4);
		else if ( *hexptr > 64 && *hexptr < 71 )
			byt = (((*hexptr)-55)<<4);
		else {
			printf( "Malformed WKB\n");
			exit(1);
		}
		hexptr++;

		//printf("%c", *hexptr);
		if ( *hexptr < 58 && *hexptr > 47 )
			byt |= ((*hexptr)-48);
		else if ( *hexptr > 64 && *hexptr < 71 )
			byt |= ((*hexptr)-55);
		else {
			printf("Malformed WKB\n");
			exit(1);
		}
		hexptr++;

		//printf("(%d)", byt);

		*retptr = (byte)byt;
		retptr++;
	}
	//printf(" Done.\n");

	return ret;
}

int is_bigendian()
{
	int test = 1;

	if ( (((char *)(&test))[0]) == 1)
	{
		return 0; //NDR (little_endian)
	}
	else
	{
		return 1; //XDR (big_endian)
	}
}

/*********************************************************************
 *
 * The following functions might go in a wkb lib
 *
 * Right now they work on a binary representation, they 
 * might work on an exadecimal representation of it as returned
 * by text cursors by postgis.
 *
 *********************************************************************/

void
dump_wkb(byte *wkb)
{
	int byteOrder;
	int type;

	printf("\n-----\n");
	byteOrder = popbyte(&wkb);
	if ( byteOrder == 0 ) printf("ByteOrder: XDR\n");
	else if ( byteOrder == 1 ) printf("ByteOrder: NDR\n");
	else printf("ByteOrder: unknown (%d)\n", byteOrder);

	type = popint(&wkb); 
	if ( type&WKBZOFFSET ) printf ("Has Z!\n");
	if ( type&WKBMOFFSET ) printf ("Has M!\n");
	type &= ~WKBZOFFSET; // strip Z flag
	type &= ~WKBMOFFSET; // strip M flag
	printf ("Type: %x\n", type);

	printf("-----\n");
}


void skipbyte(byte **c) {
	*c+=1;
}

byte getbyte(byte *c) {
	return *((char*)c);
}

byte popbyte(byte **c) {
	return *((*c)++);
}

uint32 popint(byte **c) {
	uint32 i;
	memcpy(&i, *c, 4);
	*c+=4;
	return i;
}

uint32 getint(byte *c) {
	uint32 i;
	memcpy(&i, c, 4);
	return i;
}

void skipint(byte **c) {
	*c+=4;
}

double popdouble(byte **c) {
	double d;
	memcpy(&d, *c, 8);
	*c+=8;
	return d;
}

void skipdouble(byte **c) {
	*c+=8;
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

	// Spec is a query
	if ( strstr(spec, "SELECT ") || strstr(spec, "select ") )
	{
		usrquery = spec;
		table = "__pgsql2shp_tmp_table";
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
create_usrquerytable()
{
	char *query;
	PGresult *res;

	query = malloc(sizeof(table)+sizeof(usrquery)+256);
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

/**********************************************************************
 * $Log$
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
