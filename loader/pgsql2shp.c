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
 * $Log$
 * Revision 1.46  2004/04/21 07:38:34  strk
 * Memory allocated for main_scan_query was not enough when using binary cursor. Fixed
 *
 * Revision 1.45  2004/03/29 10:20:48  strk
 * Fixed a bug in WKB parsing for Multipoints.
 * Fixed a bug in -d handling for WKB.
 * Added point->multipoint fluffing capabilities.
 *
 * Revision 1.44  2004/03/10 18:46:07  strk
 * Fixed a bug reducing the output shapes from Multipolygon tables.
 *
 * Revision 1.43  2004/03/06 17:43:06  strk
 * Added RCSID string in usage output
 *
 * Revision 1.42  2004/02/09 18:49:23  strk
 * byte endiannes detected empirically
 *
 * Revision 1.41  2004/02/06 08:26:02  strk
 * updated wkb reading funx to reflect changes made by pramsey in postgis_inout.c to be nicer with solaris
 *
 * Revision 1.40  2003/12/27 13:30:23  strk
 * Added schema specification support
 *
 * Revision 1.39  2003/12/19 18:55:46  strk
 * substituted setenv() calls with putenv() for Solaris support
 *
 * Revision 1.38  2003/12/04 19:11:56  strk
 * code cleanup (removed useless and leaking malloc calls)
 *
 * Revision 1.37  2003/11/26 18:54:22  strk
 * fixed bug in HexDecoder, made WKB parsing the default
 *
 * Revision 1.36  2003/11/26 18:14:11  strk
 * binary cursor implemented
 *
 * Revision 1.35  2003/11/26 17:21:00  strk
 * Made HEXWKB parsing settable at compile time
 *
 * Revision 1.34  2003/11/26 16:40:41  strk
 * Handled NULLS in wkb parsing, reduced functions args
 *
 * Revision 1.33  2003/11/26 15:45:53  strk
 * wkb support for all geom types
 *
 * Revision 1.32  2003/11/26 14:31:20  strk
 * WKB start to work
 *
 * Revision 1.31  2003/11/25 17:28:03  strk
 * hardly trying to get WKB parsing work
 *
 * Revision 1.30  2003/11/24 17:36:28  strk
 * Removed useless BYTE_ORDER checks
 *
 * Revision 1.29  2003/11/21 23:51:14  pramsey
 * Added Cygwin endian definition include to fix windows compile.
 *
 * Revision 1.28  2003/11/20 18:01:26  strk
 * patch from m.spring@gmx.de
 *
 * Revision 1.27  2003/11/20 15:27:20  strk
 * Removed some useless strdups.
 * Removed pgtype 22 (int2vector) from the list of integer DBF field types.
 * Added pgtype 1700 (numeric) in DBF doubles list.
 *
 * Revision 1.26  2003/11/18 14:58:47  strk
 * default row buffer lenght set to 100
 *
 * Revision 1.25  2003/11/18 14:39:26  strk
 * Some more structuring. Initialization routine moved out of main loop.
 * Preparing dumper for WKB parsing.
 *
 * Revision 1.24  2003/11/16 00:27:46  strk
 * Huge code re-organization. More structured code, more errors handled,
 * cursor based iteration, less code lines.
 *
 * Revision 1.23  2003/11/14 22:04:51  strk
 * Used environment vars to pass libpq connection options (less error prone,
 * easier to read). Printed clearer error message on query error.
 *
 * Revision 1.22  2003/09/10 22:44:56  jeffloun
 * got rid of warning...
 *
 * Revision 1.21  2003/09/10 22:40:11  jeffloun
 * changed it to make the field names in the dbf file capital letters
 *
 * Revision 1.20  2003/09/10 21:36:04  jeffloun
 * fixed a bug in is_clockwise...
 *
 * Revision 1.19  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 * Revision 1.18  2003/02/04 21:39:20  pramsey
 * Added CVS substitution strings for logging.
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

/* Define this to try WKB parsing */
#define USE_WKB
/* Define this to use HEX encoding instead of bytea encoding */
#define HEXWKB

typedef unsigned long int uint32;
typedef unsigned char byte;

/* Global data */
PGconn *conn;
int rowbuflen;
char *geo_col_name, *table, *shp_file, *schema;
int type_ary[256];
char *main_scan_query;
DBFHandle dbf;
SHPHandle shp;
int geotype;
int outshptype;
int is3d;
int binary;
#ifdef USE_WKB
SHPObject * (*shape_creator)(byte *, int);
#else
SHPObject * (*shape_creator)(char *, int, SHPHandle, int);
#endif
int big_endian = 0;

/* Prototypes */
int getMaxFieldSize(PGconn *conn, char *table, char *fname);
int parse_commandline(int ARGC, char **ARGV);
void usage(int exitstatus);
char *getTableOID(char *table);
int addRecord(PGresult *res, int residx, int row);
int initShapefile(char *shp_file, PGresult *res);
int initialize(void);
int getGeometryOID(PGconn *conn);
int getGeometryType(char *schema, char *table, char *geo_col_name);
SHPObject *create_lines(char *str,int shape_id, SHPHandle shp,int dims);
SHPObject *create_multilines(char *str,int shape_id, SHPHandle shp,int dims);
SHPObject *create_points(char *str,int shape_id, SHPHandle shp,int dims);
SHPObject *create_multipoints(char *str,int shape_id, SHPHandle shp,int dims);
SHPObject *create_polygons(char *str,int shape_id, SHPHandle shp,int dims);
SHPObject *create_multipolygons(char *str,int shape_id, SHPHandle shp,int dims);
int parse_points(char *str, int num_points, double *x,double *y,double *z);
int num_points(char *str);
int num_lines(char *str);
char *scan_to_same_level(char *str);
int points_per_sublist( char *str, int *npoints, long max_lists);
int reverse_points(int num_points,double *x,double *y,double *z);
int is_clockwise(int num_points,double *x,double *y,double *z);
int is_bigendian(void);
SHPObject * shape_creator_wrapper(char *str, int idx, SHPHandle shp, int is3d);
SHPObject * shape_creator_wrapper_WKB(byte *str, int idx);

/* WKB functions */
SHPObject * create_polygon3D_WKB(byte *wkb, int shape_id);
SHPObject * create_polygon2D_WKB(byte *wkb, int shape_id);
SHPObject * create_multipoint2D_WKB(byte *wkb, int shape_id);
SHPObject * create_multipoint3D_WKB(byte *wkb, int shape_id);
SHPObject * create_point2D_WKB(byte *wkb, int shape_id);
SHPObject * create_point3D_WKB(byte *wkb, int shape_id);
SHPObject * create_multiline3D_WKB (byte *wkb, int shpid);
SHPObject * create_multiline2D_WKB (byte *wkb, int shpid);
SHPObject * create_line2D_WKB(byte *wkb, int shape_id);
SHPObject * create_line3D_WKB(byte *wkb, int shape_id);
SHPObject * create_multipolygon2D_WKB(byte *wkb, int shpid);
SHPObject * create_multipolygon3D_WKB(byte *wkb, int shpid);
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
#define WKB3DOFFSET 0x80000000


static void exit_nicely(PGconn *conn){
	PQfinish(conn);
	exit(1);
}

//main
//USAGE: pgsql2shp [<options>] <database> <table>
//OPTIONS:
//  -d Set the dump file to 3 dimensions, if this option is not used
//     all dumping will be 2d only.
//  -f <filename>  Use this option to specify the name of the file
//     to create.
//  -h <host>  Allows you to specify connection to a database on a
//     machine other than the localhost.
//  -p <port>  Allows you to specify a database port other than 5432.
//  -P <password>  Connect to the database with the specified password.
//  -u <user>  Connect to the database as the specified user.
//  -g <geometry_column> Specify the geometry column to be exported.

int main(int ARGC, char **ARGV){
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
	geo_col_name = NULL;
	shp_file = NULL;
	main_scan_query = NULL;
	rowbuflen=100;
	is3d = 0;
	binary = 0;
#ifdef DEBUG
	FILE *debug;
#endif

	if ( getenv("ROWBUFLEN") ) rowbuflen=atoi(getenv("ROWBUFLEN"));

	if ( ! parse_commandline(ARGC, ARGV) ) {
                printf("\n**ERROR** invalid option or command parameters\n\n");
		usage(2);
		exit_nicely(conn);
	}


	/* Use table name as shapefile name */
        if(shp_file == NULL) shp_file = table;

	/* Make a connection to the specified database, and exit on failure */
	conn = PQconnectdb("");
	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
		exit_nicely(conn);
	}

#ifdef DEBUG
	debug = fopen("/tmp/trace.out", "w");
	PQtrace(conn, debug);
#endif	 /* DEBUG */


	/* Initialize shapefile and database infos */
	fprintf(stdout, "Initializing... "); fflush(stdout);
	if ( ! initialize() ) exit_nicely(conn);
	fprintf(stdout, "Done.\n"); 

	/*
	 * Begin the transaction
	 * (a cursor can only be defined inside a transaction block)
	 */
	res=PQexec(conn, "BEGIN");
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
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
//fprintf(stderr, "MAINSCAN: %s\n", main_scan_query);
	res = PQexec(conn, query);	
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_COMMAND_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
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
			fprintf(stderr, "%s", PQerrorMessage(conn));
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
	int is3d;

	// skip byte order
	skipbyte(&ptr);

	// get type
	type = getint(ptr);

	is3d = type&WKB3DOFFSET;

	switch(type)
	{
		case MULTILINETYPE:
			if ( is3d )
				return create_multiline3D_WKB(str, idx);
			else
				return create_multiline2D_WKB(str, idx);
				
		case LINETYPE:
			if ( is3d )
				return create_line3D_WKB(str, idx);
			else
				return create_line2D_WKB(str, idx);
		case POLYGONTYPE:
			if ( is3d )
				return create_polygon3D_WKB(str, idx);
			else
				return create_polygon2D_WKB(str, idx);
		case MULTIPOLYGONTYPE:
			if ( is3d )
				return create_multipolygon3D_WKB(str, idx);
			else
				return create_multipolygon2D_WKB(str, idx);

		case POINTTYPE:
			if ( is3d )
				return create_point3D_WKB(str, idx);
			else
				return create_point2D_WKB(str, idx);

		case MULTIPOINTTYPE:
			if ( is3d )
				return create_multipoint3D_WKB(str, idx);
			else
				return create_multipoint2D_WKB(str, idx);

		default:
			fprintf(stderr, "Unknown WKB type (%lu) - (%s:%d)\n",
				type, __FILE__, __LINE__);
			return NULL;
	}
}

SHPObject *
shape_creator_wrapper(char *str, int idx, SHPHandle shp, int is3d)
{
	char *ptr;

	ptr = strchr(str,';');
	ptr++;
	if ( (ptr == NULL) || (*ptr == 0) ){
		fprintf(stderr, "Malformed WKT\n");
		return NULL;  
	}

	if ( ! strncmp(ptr, "MULTILIN", 8) )
		return create_multilines(str, idx, shp, is3d);
	if ( ! strncmp(ptr, "MULTIPOL", 8) )
		return create_multipolygons(str, idx, shp, is3d);
	if ( ! strncmp(ptr, "MULTIPOI", 8) )
		return create_multipoints(str, idx, shp, is3d);
	if ( ! strncmp(ptr, "LINESTRI", 8) )
		return create_lines(str, idx, shp, is3d);
	if ( ! strncmp(ptr, "POLYGON", 7) )
		return create_polygons(str, idx, shp, is3d);
	if ( ! strncmp(ptr, "POINT", 5) )
		return create_points(str, idx, shp, is3d);

	fprintf(stderr, "Unknown geometry type provided (%s)\n", ptr);
	return NULL;

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
create_multilines(char *str,int shape_id, SHPHandle shp,int dims)
{
	int		lines,i,j,max_points,index;
	int		*points;
	int		*part_index;
	int 		notnull;
	
	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;
	SHPObject *obj;

	notnull=1;
	lines = num_lines(str);
	
	points = (int *)malloc(sizeof(int) * lines);

	if(points_per_sublist(str, points, lines) ==0){
		printf("error - points_per_sublist failed");
	}
	max_points = 0;
	for(j=0;j<lines;j++){
		max_points += points[j];
	}
		
	part_index = (int *)malloc(sizeof(int) * lines);
	totx = (double *)malloc(sizeof(double) * max_points);
	toty = (double *)malloc(sizeof(double) * max_points);
	totz = (double *)malloc(sizeof(double) * max_points);
	
	index=0;

	if(lines == 0 ){
		notnull = 0;
	}
	for(i=0;i<lines;i++){
		str = strchr(str,'(') ;

		
		if(str[0] =='(' && str[1] == '('){
			str++;
		}
		
		x = (double *)malloc(sizeof(double) * points[i]);
		y = (double *)malloc(sizeof(double) * points[i]);
		z = (double *)malloc(sizeof(double) * points[i]);

		notnull = parse_points(str,points[i],x,y,z);
		str = scan_to_same_level(str);
		part_index[i] = index;
		for(j=0;j<points[i];j++){
			totx[index] = x[j];
			toty[index] = y[j];
			totz[index] = z[j];
			index++;
		}
		free(x);
		free(y);
		free(z);
	}

	
	if(dims == 0){
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_ARC,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
		}
	}else{
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_ARCZ,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,lines,part_index,NULL,max_points,totx,toty,totz,NULL);
		}
	}
	free(part_index); free(points);
	free(totx); free(toty); free(totz);

	return obj;
}

SHPObject *
create_multiline3D_WKB (byte *wkb, int shape_id)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *z=NULL;
	int nparts=0, *part_index=NULL, totpoints=0, nlines=0;
	int li;

	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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

		/* wkb now points at first point */
		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			z[totpoints+pn] = popdouble(&wkb);
		}

		part_index[li] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(SHPT_ARC, shape_id, nparts,
		part_index, NULL, totpoints,
		x, y, z, NULL);

	free(part_index); free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_multiline2D_WKB (byte *wkb, int shape_id)
{
	double *x=NULL, *y=NULL;
	int nparts=0, *part_index=NULL, totpoints=0, nlines=0;
	int li;
	SHPObject *obj;
	
	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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


	obj = SHPCreateObject(SHPT_ARC, shape_id, nparts,
		part_index, NULL, totpoints,
		x, y, NULL, NULL);

	free(part_index); free(x); free(y);

	return obj;
}

SHPObject *
create_lines(char *str,int shape_id, SHPHandle shp,int dims)
{
	int		points;
	int		*part_index;
	int 		notnull;
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;

	notnull =1;
	part_index = (int *)malloc(sizeof(int));	//we know lines only have 1 part so make the array of size 1
	part_index[0] = 0;

	points = num_points(str);
	x = (double *)malloc(sizeof(double) * points);
	y = (double *)malloc(sizeof(double) * points);
	z = (double *)malloc(sizeof(double) * points);

	notnull = parse_points(str,points,x,y,z);

	if(dims == 0){
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_ARC,shape_id,1,part_index,NULL,points,x,y,z,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,1,part_index,NULL,points,x,y,z,NULL);
		}
	}else{
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_ARCZ,shape_id,1,part_index,NULL,points,x,y,z,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,1,part_index,NULL,points,x,y,z,NULL);
		}
	}
	free(part_index);
	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_line3D_WKB (byte *wkb, int shape_id)
{
	double *x=NULL, *y=NULL, *z=NULL;
	uint32 npoints=0, pn;
	SHPObject *obj;
	
	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

	npoints = popint(&wkb);

#if VERBOSE > 2
	printf("Line has %lu points\n", npoints);
#endif

	x = malloc(sizeof(double)*(npoints));
	y = malloc(sizeof(double)*(npoints));
	z = malloc(sizeof(double)*(npoints));

	/* wkb now points at first point */
	for (pn=0; pn<npoints; pn++)
	{
		x[pn] = popdouble(&wkb);
		y[pn] = popdouble(&wkb);
		z[pn] = popdouble(&wkb);
	}

	obj = SHPCreateSimpleObject(SHPT_ARCZ, npoints, x, y, z);
	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_line2D_WKB (byte *wkb, int shape_id)
{
	double *x=NULL, *y=NULL, *z=NULL;
	uint32 npoints=0, pn;
	SHPObject *obj;
	
	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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

	obj = SHPCreateSimpleObject(SHPT_ARC, npoints, x, y, z);
	free(x); free(y); 

	return obj;
}


SHPObject *
create_point3D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	double x, y, z;

	// skip byteOrder and wkbType
	skipbyte(&wkb); skipint(&wkb);

	x = popdouble(&wkb);
	y = popdouble(&wkb);
	z = popdouble(&wkb);

	if ( geotype == POINTTYPE )
	{
		obj = SHPCreateSimpleObject(SHPT_POINTZ,1,&x,&y,&z);
	}
	else if ( geotype == MULTIPOINTTYPE )
	{
		//fprintf(stderr, "create_point3D_WKB: fluffing to MULTIPOINT\n");
		obj = SHPCreateSimpleObject(SHPT_MULTIPOINTZ, 1, &x, &y, &z);
	}
	else
	{
		fprintf(stderr, "ERROR: create_point2D_WKB called with wrong geometry type (%d)\n", geotype);
		return NULL;
	}

	return obj;
}

SHPObject *
create_point2D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	double x, y;

	// skip byteOrder and wkbType
	skipbyte(&wkb); skipint(&wkb);

	x = popdouble(&wkb);
	y = popdouble(&wkb);

	obj = SHPCreateSimpleObject(outshptype, 1, &x, &y, NULL);

	return obj;
}

SHPObject *
create_points(char *str, int shape_id, SHPHandle shp, int is3d)
{
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;
	int notnull;
	int outtype;

	notnull = 1;
	
	x = (double *)malloc(sizeof(double));
	y = (double *)malloc(sizeof(double));
	z = (double *)malloc(sizeof(double));

	notnull = parse_points(str,1,x,y,z);

	if ( ! notnull )
	{
		outtype = SHPT_NULL;
	}
	else if ( geotype == POINTTYPE )
	{
		outtype = is3d ? SHPT_POINTZ : SHPT_POINT;
	}
	else if ( geotype == MULTIPOINTTYPE )
	{
		//fprintf(stderr, "create_points: fluffing to MULTIPOINT\n");
		outtype = is3d ? SHPT_MULTIPOINTZ : SHPT_MULTIPOINT;
	}
	else
	{
		fprintf(stderr, "create_points called with wrong geometry type (%d)\n", geotype);
		return NULL;
	}
	
	obj = SHPCreateSimpleObject(outtype,1,x,y,z);
	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_multipoints(char *str,int shape_id, SHPHandle shp,int dims)
{
	int points;	
	
	double	*x,
			*y,
			*z;
	SHPObject	*obj;
	int notnull;
	notnull=1;	

	points = num_points(str);
	x = (double *)malloc(sizeof(double)*points);
	y = (double *)malloc(sizeof(double)*points);
	z = (double *)malloc(sizeof(double)*points);

	notnull = parse_points(str,points,x,y,z);

	if(dims == 0){
		if(notnull > 0){
			obj = SHPCreateSimpleObject(SHPT_MULTIPOINT ,points,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL ,points,x,y,z);
		}
	}else{
		if(notnull > 0){
			obj = SHPCreateSimpleObject(SHPT_MULTIPOINTZ ,points,x,y,z);
		}else{
			obj = SHPCreateSimpleObject(SHPT_NULL ,points,x,y,z);
		}
	}

	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_multipoint3D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL, *z=NULL;
	int npoints;
	int pn;

	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

	npoints = popint(&wkb);

	x = (double *)malloc(sizeof(double)*npoints);
	y = (double *)malloc(sizeof(double)*npoints);
	z = (double *)malloc(sizeof(double)*npoints);

	for (pn=0; pn<npoints; pn++)
	{
		x[pn]=popdouble(&wkb);
		y[pn]=popdouble(&wkb);
		z[pn]=popdouble(&wkb);
	}

	obj = SHPCreateSimpleObject(SHPT_MULTIPOINTZ,npoints,x,y,z);
	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_multipoint2D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	double *x=NULL, *y=NULL;
	uint32 npoints;
	uint32 pn;

	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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
create_polygons(char *str,int shape_id, SHPHandle shp,int dims)
{
	int		rings,i,j,max_points,index;
	int		*points;
	int		*part_index;
	int notnull;
	
	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;
	SHPObject *obj;

	notnull = 1;

	rings = num_lines(str); //the number of rings in the polygon
	points = (int *)malloc(sizeof(int) * rings);

	if(points_per_sublist(str, points, rings) ==0){
		printf("error - points_per_sublist failed");
	}
	max_points = 0;
	for(j=0;j<rings;j++){
		max_points += points[j];
	}
		
	part_index = (int *)malloc(sizeof(int) * rings);
	totx = (double *)malloc(sizeof(double) * max_points);
	toty = (double *)malloc(sizeof(double) * max_points);
	totz = (double *)malloc(sizeof(double) * max_points);
	
	index=0;
	if (rings == 0){
		notnull = 0;
	}
	for(i=0;i<rings;i++){
		str = strchr(str,'(') ;

		
		if(str[0] =='(' && str[1] == '('){
			str++;
		}
		
		x = (double *)malloc(sizeof(double) * points[i]);
		y = (double *)malloc(sizeof(double) * points[i]);
		z = (double *)malloc(sizeof(double) * points[i]);

		notnull = parse_points(str,points[i],x,y,z);

		str = scan_to_same_level(str);
		part_index[i] = index;

		//if this the first ring it should be clockwise, other rings are holes and should be counter-clockwise
		if(i ==0){
			if(is_clockwise(points[i],x,y,z) == 0){
				reverse_points(points[i],x,y,z);
			}
		}else{
			if(is_clockwise(points[i],x,y,z) == 1){
				reverse_points(points[i],x,y,z);
			}
		}

		for(j=0;j<points[i];j++){
			totx[index] = x[j];
			toty[index] = y[j];
			totz[index] = z[j];
			index++;
		}
//		free(x);
//		free(y);
//		free(z);
	}

	if(dims == 0){
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_POLYGON,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
		}
	}else{
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_POLYGONZ,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,rings,part_index,NULL,max_points,totx,toty,totz,NULL);
		}
	}
	free(part_index);
	free(points);
	free(totx);
	free(toty);
	free(totz);

	return obj;
}

SHPObject *
create_polygon2D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	int ri, nrings, totpoints=0, *part_index=NULL;
	double *x=NULL, *y=NULL, *z=NULL;
	
	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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
						y+totpoints, NULL);
			}
		} else {
			if ( is_clockwise(npoints, x+totpoints,
						y+totpoints, NULL) )
			{
#if VERBOSE > 2
				printf("Forcing CCW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, NULL);
			}
		}

		part_index[ri] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(outshptype, shape_id, nrings,
		part_index, NULL, totpoints,
		x, y, z, NULL);

	free(part_index);
	free(x); free(y); 

	return obj;
}

SHPObject *
create_polygon3D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	int ri, nrings, totpoints=0, *part_index=NULL;
	double *x=NULL, *y=NULL, *z=NULL;
	
	// skip byteOrder and type
	skipbyte(&wkb); skipint(&wkb);

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

		for (pn=0; pn<npoints; pn++)
		{
			x[totpoints+pn] = popdouble(&wkb);
			y[totpoints+pn] = popdouble(&wkb);
			z[totpoints+pn] = popdouble(&wkb);
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
						y+totpoints, z+totpoints);
			}
		} else {
			if ( is_clockwise(npoints, x+totpoints,
						y+totpoints, z+totpoints) ) {
#if VERBOSE > 2
				printf("Forcing CCW\n");
#endif
				reverse_points(npoints, x+totpoints,
						y+totpoints, z+totpoints);
			}
		}

		part_index[ri] = totpoints;
		totpoints += npoints;
	}

	obj = SHPCreateObject(SHPT_POLYGONZ, shape_id, nrings,
		part_index, NULL, totpoints,
		x, y, z, NULL);

	free(part_index);
	free(x); free(y); free(z);

	return obj;
}

SHPObject *
create_multipolygons(char *str,int shape_id, SHPHandle shp,int dims)
{
	int		polys,rings,i,j,k,max_points;
	int		index,indexk,index2part,tot_rings,final_max_points;
	int		*points;
	int		*part_index;
	int		*final_part_index;
	int 		notnull;
	
	char	*temp;
	char	*temp_addr;

	double	*x;
	double	*y;
	double	*z;

	double	*totx;
	double	*toty;
	double	*totz;

	double	*finalx;
	double	*finaly;
	double	*finalz;

	SHPObject *obj;
	
	points=0;
	notnull = 1;

	polys = num_lines(str); //the number of rings in the polygon
	final_max_points =0;

	temp_addr = (char *)malloc(strlen(str) +1 );
	temp = temp_addr; //keep original pointer to free the mem later
	strcpy(temp,str);
	tot_rings=0;
	index2part=0;
	indexk=0;
	

	for(i=0;i<polys;i++){
		temp = strstr(temp,"((") ;
		if(temp[0] =='(' && temp[1] == '(' && temp[2] =='('){
			temp++;
		}
		rings = num_lines(temp);
		points = (int *)malloc(sizeof(int) * rings);
		points_per_sublist(temp, points, rings);
		tot_rings += rings;
		for(j=0;j<rings;j++){
			final_max_points += points[j];
		}
		
		temp+= 2;
	}
	free(points);
	temp= temp_addr;
	
	final_part_index = (int *)malloc(sizeof(int) * tot_rings);	
	finalx = (double *)malloc(sizeof(double) * final_max_points); 
	finaly = (double *)malloc(sizeof(double) * final_max_points); 
	finalz = (double *)malloc(sizeof(double) * final_max_points); 

	if(polys == 0){
		notnull = 0;
	}

	for(k=0;k<polys ; k++){ //for each polygon  

		str = strstr(str,"((") ;
		if(strlen(str) >2 && str[0] =='(' && str[1] == '(' && str[2] =='('){
			str++;
		}

		rings = num_lines(str);
		points = (int *)malloc(sizeof(int) * rings);

		if(points_per_sublist(str, points, rings) ==0){
			printf("error - points_per_sublist failed");
		}
		max_points = 0;
		for(j=0;j<rings;j++){
			max_points += points[j];
		}
		
		part_index = (int *)malloc(sizeof(int) * rings);
		totx = (double *)malloc(sizeof(double) * max_points);
		toty = (double *)malloc(sizeof(double) * max_points);
		totz = (double *)malloc(sizeof(double) * max_points);
	
		index=0;
		for(i=0;i<rings;i++){
			str = strchr(str,'(') ;
			if(str[0] =='(' && str[1] == '('){
				str++;
			}
			
			x = (double *)malloc(sizeof(double) * points[i]);
			y = (double *)malloc(sizeof(double) * points[i]);
			z = (double *)malloc(sizeof(double) * points[i]);

			notnull = parse_points(str,points[i],x,y,z);
			str = scan_to_same_level(str);
			part_index[i] = index;

			//if this the first ring it should be clockwise, other rings are holes and should be counter-clockwise
	                if(i ==0){
        	                if(is_clockwise(points[i],x,y,z) == 0){
                	                reverse_points(points[i],x,y,z);
                        	}
	                }else{
        	                if(is_clockwise(points[i],x,y,z) == 1){
                	                reverse_points(points[i],x,y,z);
	                        }
        	        }

			for(j=0;j<points[i];j++){
				totx[index] = x[j];
				toty[index] = y[j];
				totz[index] = z[j];
				index++;
			}
			free(x);
			free(y);
			free(z);
		}
		for(j=0;j<i;j++){
			final_part_index[index2part] = part_index[j]+indexk ;
			index2part++;
		}

		for(j=0;j<index;j++){
			finalx[indexk] = totx[j];
			finaly[indexk] = toty[j];
			finalz[indexk] = totz[j];
			indexk++;
		}
		
		free(points);
		free(part_index);
		free(totx);
		free(toty);
		free(totz);
		str -= 1;
	}//end for (k < polys... loop


	if(dims == 0){
		if(notnull > 0){
			obj = SHPCreateObject(SHPT_POLYGON,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
		}
	}else{
		if(notnull > 0){		
			obj = SHPCreateObject(SHPT_POLYGONZ,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
		}else{
			obj = SHPCreateObject(SHPT_NULL,shape_id,tot_rings,final_part_index,NULL,final_max_points,finalx,finaly,finalz,NULL);
		}
	}
	
	free(final_part_index);
	free(finalx);
	free(finaly);
	free(finalz);

	return obj;
}

SHPObject *
create_multipolygon2D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	uint32 nrings, nparts;
	uint32 npolys;
	uint32 totpoints=0;
	int *part_index=NULL;
	uint32 pi;
	double *x=NULL, *y=NULL;

	// skip byteOrder and type
	//printf("byteOrder is %d\n", popbyte(&wkb));
	//printf("Type is %d", popint(&wkb)); 
	skipbyte(&wkb); skipint(&wkb);

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
							y+totpoints, NULL);
				}
			} else {
				if (is_clockwise(npoints, x+totpoints,
							y+totpoints, NULL))
				{
#if VERBOSE > 2
					printf("Forcing CCW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, NULL);
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

	obj = SHPCreateObject(outshptype, shape_id, nparts,
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
create_multipolygon3D_WKB(byte *wkb, int shape_id)
{
	SHPObject *obj;
	int nrings, nparts;
	uint32 npolys;
	int totpoints=0;
	int *part_index=NULL;
	int pi;
	double *x=NULL, *y=NULL, *z=NULL;

	// skip byteOrder and type
	//printf("byteOrder is %d\n", popbyte(&wkb));
	//printf("Type is %d", popint(&wkb)); 
	skipbyte(&wkb); skipint(&wkb);
	
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

			/* wkb now points at first point */
			for (pn=0; pn<npoints; pn++)
			{
				x[totpoints+pn] = popdouble(&wkb);
				y[totpoints+pn] = popdouble(&wkb);
				z[totpoints+pn] = popdouble(&wkb);
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
							y+totpoints, NULL))
				{
#if VERBOSE > 2
					printf("Forcing CW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, NULL);
				}
			} else {
				if (is_clockwise(npoints, x+totpoints,
							y+totpoints, NULL))
				{
#if VERBOSE > 2
					printf("Forcing CCW\n");
#endif
					reverse_points(npoints, x+totpoints,
							y+totpoints, NULL);
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

	obj = SHPCreateObject(SHPT_POLYGON, shape_id, nparts,
		part_index, NULL, totpoints,
		x, y, z, NULL);

#if VERBOSE > 2
	printf("Object created\n");
#endif

	free(part_index);
	free(x); free(y); free(z);

	return obj;
}

//Reverse the clockwise-ness of the point list...
int reverse_points(int num_points,double *x,double *y,double *z){
	
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

		j--;
	}
	return 1;
}

//return 1 if the points are in clockwise order
int is_clockwise(int  num_points,double *x,double *y,double *z){
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
		return 0; //counter-clockwise
	}else{
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
		fprintf(stderr, "OID query error: %s", PQerrorMessage(conn));
		return -1;
	}

	if(PQntuples(res1) <= 0 )
	{
		fprintf(stderr, "Geometry type unknown "
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

	for (j=0; j<nFields; j++)
	{
		SHPObject *obj;

		/* Integer attribute */
		if (type_ary[j] == 1)
		{
			int temp;
			if ( PQgetisnull(res, residx, j) ) {
				temp = 0;
			} else {
				val = (char *)PQgetvalue(res, residx, j);
				temp = atoi(val);
			}
#if VERBOSE > 1
fprintf(stdout, "i"); fflush(stdout);
#endif
			if (!DBFWriteIntegerAttribute(dbf, row, flds, temp))
			{
				fprintf(stderr, "error(int) - Record could not be created\n");
				return 0;
			}
			flds++;
			continue;
		}
		
		/* Double attribute */
		if (type_ary[j] == 2)
		{
			double temp;
			if ( PQgetisnull(res, residx, j) ) {
				temp = 0;
			} else {
				val = PQgetvalue(res, residx, j);
				temp = atof(val);
			}
#if VERBOSE > 1
fprintf(stdout, "d"); fflush(stdout);
#endif
			if (!DBFWriteDoubleAttribute(dbf, row, flds, temp))
			{
				fprintf(stderr, "error(double) - Record could "
						"not be created\n");
				return 0;
			}
			flds++;
			continue;
		}

		/* Default (not geometry) attribute */
		if (type_ary[j] != 9)
		{
			if ( PQgetisnull(res, residx, j) ) {
				val = "";
			} else {
				val = PQgetvalue(res, residx, j);
			}
#if VERBOSE > 1
fprintf(stdout, "s"); fflush(stdout);
#endif
			if(!DBFWriteStringAttribute(dbf, row, flds, val))
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
				fprintf(stderr,
					"Error writing null shape %d\n", row);
				SHPDestroyObject(obj);
				return 0;
			}
			SHPDestroyObject(obj);
			continue;
		}

#ifdef USE_WKB
		if ( ! binary )
		{
#ifndef HEXWKB
			{
				int junk;
				char *v = PQgetvalue(res, residx, j);
				val = PQunescapeBytea(v, &junk);
				//printf("Unescaped %d bytes\n", junk);
			}

#else
			char *v = PQgetvalue(res, residx, j);
			val = HexDecode(v);
#endif // HEXWKB
#if VERBOSE > 2
		dump_wkb(val);
#endif // VERBOSE > 2
		}
		else // binary
		{
			val = (char *)PQgetvalue(res, residx, j);
		}
#else // ndef USE_WKB
		val = PQgetvalue(res, residx, j);
#endif // USE_WKB

#if VERBOSE > 1
		fprintf(stdout, "g"); fflush(stdout);
#endif

#ifdef USE_WKB
		obj = shape_creator_wrapper_WKB(val, row);
		//obj = shape_creator(val, row);
#else
		obj = shape_creator_wrapper(val, row, shp, is3d);
		//obj = shape_creator(val, row, shp, is3d);
#endif
		if ( ! obj )
		{
			fprintf(stderr, "Error creating shape for record %d "
					"(geotype is %d)\n", row, geotype);
			return 0;
		}
		if ( SHPWriteObject(shp,-1,obj) == -1)
		{
			fprintf(stderr, "Error writing shape %d\n", row);
			SHPDestroyObject(obj);
			return 0;
		}
		SHPDestroyObject(obj);

#ifdef USE_WKB
		if ( ! binary ) free(val);
#endif
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
getTableOID(char *table)
{
	PGresult *res3;
	char *query;
	char *ret;

	query = (char *)malloc(strlen(table)+256);
	sprintf(query, "SELECT oid FROM pg_class WHERE relname = '%s'", table);
	res3 = PQexec(conn, query);
	free(query);
	if ( ! res3 || PQresultStatus(res3) != PGRES_TUPLES_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
		exit_nicely(conn);
	}
	if(PQntuples(res3) == 1 ){
		ret = strdup(PQgetvalue(res3, 0, 0));
	}else if(PQntuples(res3) == 0 ){
		fprintf(stderr, "ERROR: Cannot determine relation OID.\n");
		return NULL;
	}else{
		ret = strdup(PQgetvalue(res3, 0, 0));
		fprintf(stderr, "Warning: Multiple relations detected, the program will only dump the first relation.\n");
	}	
	return ret;
}

/*
 * Return geometry type as defined at top file.
 * Return -1 on error
 * Return  0 on unknown or unsupported geometry type
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

	//get what kind of Geometry type is in the table
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

	//printf("\n\n-->%s\n\n",query);
	res = PQexec(conn, query);	
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
		return -1;
	}

	if (PQntuples(res) == 0)
	{
		printf("ERROR: Cannot determine geometry type (empty table).\n");
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
				fprintf(stderr, "ERROR: uncompatible mixed geometries in table\n");
				return -1;
			}
			basetype = LINETYPE;
			multitype = MULTILINETYPE;
		}
		else if ( ! strncmp(geo_str, "POLYGON", 7) )
		{
			if ( basetype && basetype != POLYGONTYPE )
			{
				fprintf(stderr, "ERROR: uncompatible mixed geometries in table\n");
				return -1;
			}
			basetype = POLYGONTYPE;
			multitype = MULTIPOLYGONTYPE;
		}
		else if ( ! strncmp(geo_str, "POINT", 5) )
		{
			if ( basetype && basetype != POINTTYPE )
			{
				fprintf(stderr, "ERROR: uncompatible mixed geometries in table\n");
				return -1;
			}
			basetype = POINTTYPE;
			multitype = MULTIPOINTTYPE;
		}
		else
		{
			fprintf(stderr, "type '%s' is not Supported at this time.\n",
				geo_str);
			fprintf(stderr, "The DBF file will be created but not the shx "
				"or shp files.\n");
			return 0;
		}
	}

	if ( foundmulti )
		return multitype;
	else
		return basetype;

}

void
usage(status)
{
	printf("RCSID: %s\n", rcsid);
	printf("USAGE: pgsql2shp [<options>] <database> [<schema>.]<table>\n");
	printf("\n");
       	printf("OPTIONS:\n");
       	printf("  -d Set the dump file to 3 dimensions, if this option is not used\n");
       	printf("     all dumping will be 2d only.\n");
       	printf("  -f <filename>  Use this option to specify the name of the file\n");
       	printf("     to create.\n");
       	printf("  -h <host>  Allows you to specify connection to a database on a\n");
	printf("     machine other than the default.\n");
       	printf("  -p <port>  Allows you to specify a database port other than the default.\n");
       	printf("  -P <password>  Connect to the database with the specified password.\n");
       	printf("  -u <user>  Connect to the database as the specified user.\n");
	printf("  -g <geometry_column> Specify the geometry column to be exported.\n");
	printf("  -b Use a binary cursor.\n");
       	printf("\n");
       	exit (status);
}

/* Parse command line parameters */
int parse_commandline(int ARGC, char **ARGV)
{
	int c, curindex;
	char buf[256], *ptr;

	buf[255] = '\0'; // just in case...

	/* Parse command line */
        while ((c = getopt(ARGC, ARGV, "bf:h:du:p:P:g:")) != EOF){
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
	            is3d = 1;
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
        for ( ; optind < ARGC; optind++){
                if(curindex ==0){
			//setenv("PGDATABASE", ARGV[optind], 1);
		    	snprintf(buf, 255, "PGDATABASE=%s", ARGV[optind]);
		    	putenv(strdup(buf));
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
        if(curindex != 2) return 0;
	return 1;
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
	//fprintf(stderr, "Attribute query:\n%s\n", query);
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
		return 0;
	}
	if (! PQntuples(res)) {
		fprintf(stderr, "Table %s does not exist\n", table);
		PQclear(res);
		return 0;
	}

	/* Create the dbf file */
	dbf = DBFCreate(shp_file);
	if ( ! dbf ) {
		fprintf(stderr, "Could not create dbf file\n");
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
		char field_name[32]; // dbf version of field name

		fname = PQgetvalue(res, i, 0);
		type = atoi(PQgetvalue(res, i, 1));
		size = atoi(PQgetvalue(res, i, 2));

//fprintf(stderr, "A: %s, T: %d, S: %d\n", fname, type, size);
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

		if(strlen(fname) <32) strcpy(field_name, fname);
		else {
			printf("field name %s is too long, must be "
				"less than 32 characters.\n", fname);
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
				fprintf(stderr, "error - Field could not "
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
				fprintf(stderr, "error - Field could not "
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
		 * For variable-sized fields we'll use max size in table
		 * as dbf field size
		 */
		else if(size == -1)
		{
			size = getMaxFieldSize(conn, table, fname);
			if ( size == -1 ) return 0;
			if ( ! size ) size = 32; // might 0 be a good size ?
		}
//fprintf(stderr, "FIELD_NAME: %s, SIZE: %d\n", field_name, size);
		
		/* generic type (use string representation) */
		if(DBFAddField(dbf, field_name,FTString,size,0) == -1)
		{
			fprintf(stderr, "error - Field could not "
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
			fprintf(stderr, "%s: no such attribute in table %s\n",
					geo_col_name, table);
			return 0;
		}
		fprintf(stderr, "No geometry column found.\n");
		fprintf(stderr, "The DBF file will be created "
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
				if ( is3d ) outshptype=SHPT_ARCZ;
				else outshptype=SHPT_ARC;
				break;
				
			case POLYGONTYPE:
			case MULTIPOLYGONTYPE:
				if ( is3d ) outshptype=SHPT_POLYGONZ;
				else outshptype=SHPT_POLYGON;
				break;

			case POINTTYPE:
				if ( is3d ) outshptype=SHPT_POINTZ;
				else outshptype=SHPT_POINT;
				break;

			case MULTIPOINTTYPE:
				if ( is3d ) outshptype=SHPT_MULTIPOINTZ;
				else outshptype=SHPT_MULTIPOINT;
				break;

			
			default:
				shp = NULL;
				shape_creator = NULL;
				fprintf(stderr, "You've found a bug! (%s:%d)\n",
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
#ifdef USE_WKB

			if ( big_endian ) 
			{

#ifdef HEXWKB
				sprintf(buf, "asbinary(\"%s\", 'XDR')",
					mainscan_flds[i]);
#else
				sprintf(buf, "asbinary(\"%s\", 'XDR')::bytea",
					mainscan_flds[i]);
#endif

			}
			else // little_endian
			{

#ifdef HEXWKB
				sprintf(buf, "asbinary(\"%s\", 'NDR')",
					mainscan_flds[i]);
#else // ndef HEXWKB
				sprintf(buf, "asbinary(\"%s\", 'NDR')::bytea",
					mainscan_flds[i]);
#endif // def HEXWKB

			}
#else // ndef USE_WKB
			if ( binary ) sprintf(buf, "\"%s\"::text",
				mainscan_flds[i]);
			else sprintf(buf, "\"%s\"", mainscan_flds[i]);
#endif // def USE_WKB
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

	PQclear(res);

	return 1;
}

/* 
 * Return the maximum octet_length from given table.
 * Return -1 on error.
 */
int
getMaxFieldSize(PGconn *conn, char *table, char *fname)
{
	int size;
	char *query;
	PGresult *res;

	//( this is ugly: don't forget counting the length 
	// when changing the fixed query strings )
	query = (char *)malloc(strlen(fname)+strlen(table)+40); 
	sprintf(query, "select max(octet_length(\"%s\")) from \"%s\"",
		fname, table);
	res = PQexec(conn, query);
	free(query);
	if ( ! res || PQresultStatus(res) != PGRES_TUPLES_OK ) {
		fprintf(stderr, "%s", PQerrorMessage(conn));
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
		fprintf(stderr, "Out of virtual memory\n");
		exit(1);
	}


	//printf("Decoding %d bytes", len); fflush(stdout);
	hexptr = hex; retptr = ret;
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
			fprintf(stderr, "Malformed WKB\n");
			exit(1);
		}
		hexptr++;

		//printf("%c", *hexptr);
		if ( *hexptr < 58 && *hexptr > 47 )
			byt |= ((*hexptr)-48);
		else if ( *hexptr > 64 && *hexptr < 71 )
			byt |= ((*hexptr)-55);
		else {
			fprintf(stderr, "Malformed WKB\n");
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
	if ( type&WKB3DOFFSET ) printf ("Is 3D\n");
	type &= ~WKB3DOFFSET; // strip 3d flag
	printf ("Type: %x\n", type);

	printf("-----\n");
}


void skipbyte(byte **c) {
	*c+=1;
}

byte getbyte(byte *c) {
	return *((char*)c);
}

// #define popbyte(x) *x++
byte popbyte(byte **c) {
	return *((byte*)*c++);
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

