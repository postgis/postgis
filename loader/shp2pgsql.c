/**********************************************************************
 * $Id$
 *
 * Author: Jeff Lounsbury, jeffloun@refractions.net
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * Using shapelib 1.2.8, this program reads in shape files and 
 * processes it's contents into a Insert statements which can be 
 * easily piped into a database frontend.
 * Specifically designed to insert type 'geometry' (a custom 
 * written PostgreSQL type) for the shape files and PostgreSQL 
 * standard types for all attributes of the entity. 
 **********************************************************************/

#include "shapefil.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "getopt.h"

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7

#define WKBZOFFSET 0x80000000
#define WKBMOFFSET 0x40000000

typedef struct {double x, y, z, m;} Point;

typedef struct Ring {
	Point *list;	//list of points
	struct Ring  *next;
	int n;	//number of points in list
	unsigned int linked; // number of "next" rings
} Ring;


/* globals */
int	dump_format = 0; //0=insert statements, 1 = dump
int	quoteidentifiers = 0;
int	forceint4 = 0;
char    opt;
char    *col_names;
char    *geom;
char	*pgtype;
int	istypeM;
int	pgdims;
unsigned int wkbtype;
char  	*shp_file;

DBFFieldType *types;	/* Fields type, width and precision */
SHPHandle  hSHPHandle;
DBFHandle  hDBFHandle;
int shpfiletype;
SHPObject  *obj;
int 	*widths;
int 	*precisions;
char    *table,*schema;
int     num_fields,num_records;
char    **field_names;
char 	*sr_id;

/* Prototypes */
int Insert_attributes(DBFHandle hDBFHandle, int row);
char *make_good_string(char *str);
char *protect_quotes_string(char *str);
int PIP( Point P, Point* V, int n );
void *safe_malloc(size_t size);
void create_table(void);
void usage(char *me, int exitcode);
void InsertPoint(void);
void InsertMultiPoint(void);
void InsertPolygon(void);
void InsertLineString(int id);
int parse_cmdline(int ARGC, char **ARGV);
void SetPgType(void);
char *dump_ring(Ring *ring);

// WKB
static char getEndianByte(void);
static void print_wkb_bytes(unsigned char* ptr, unsigned int cnt, size_t size);
static void print_wkb_byte(unsigned char val);
static void print_wkb_int(int val);
static void print_wkb_double(double val);

static char rcsid[] =
  "$Id$";

void *safe_malloc(size_t size)
{
	void *ret = malloc(size);
	if ( ! ret ) {
		fprintf(stderr, "Out of virtual memory\n");
		exit(1);
	}
	return ret;
}

#define malloc(x) safe_malloc(x)


char *
make_good_string(char *str)
{
	//find all the tabs and make them \<tab>s
	//
	// 1. find # of tabs
	// 2. make new string 
	//
	// we dont escape already escaped tabs


	char	*result;
	char	*str2;
	char	*start,*end;
	int	num_tabs = 0;

	(str2) = (str);

	while ((str2 = strchr(str2, '\t')) )
	{
		if ( (str2 == str) || (str2[-1] != '\\') ) //the previous char isnt a '\'
			num_tabs ++;
		str2++;
	}
	if (num_tabs == 0)
		return str;
	
	result =(char *) malloc ( strlen(str) + num_tabs+1);
	memset(result,0, strlen(str) + num_tabs+1 );
	start = str;

	while((end = strchr((start),'\t')))
	{
		if ( (end == str) || (end[-1] != '\\' ) ) //the previous char isnt a '\'
		{
			strncat(result, start, (end-start));	
			strcat(result,"\\\t");
			start = end +1;
		}
		else
		{
			strncat(result, start, (end-start));	
			strcat(result,"\t");	
			start = end  +1;
		}
	}
	strcat(result,start);
	return result;
	
}

char	*protect_quotes_string(char *str){
	//find all quotes and make them \quotes
	//find all '\' and make them '\\'
	// 
	// 1. find # of characters
	// 2. make new string 

	char	*result;
	char	*str2;
	char	*start, *end1, *end2 = NULL;
	int	num_chars = 0;

	str2 =  str;

	while ((str2 = strchr((str2), '\'')) )
	{
		//if ( (str2 == str) || (str2[-1] != '\\') ) //the previous char isnt a '\'
		num_chars ++;
		str2++;
	}
	
	str2 = str;
	
	while ((str2 = strchr((str2), '\\')) )
	{
		num_chars ++;
		str2++;
	}
	if (num_chars == 0)
		return str;
	
	result =(char *) malloc ( strlen(str) + num_chars+1);
	memset(result,0, strlen(str) + num_chars+1 );
	start = str;

	while((end1 = strchr((start),'\''))||(end2 = strchr((start),'\\')))
	{
	   
	  if(end1){
	    strncat(result, start, (end1-start));
	    strcat(result, "\\\'");
	    start = end1+1;
	  }else{
	    strncat(result, start, (end2-start));
	    strcat(result, "\\\\");
	    start = end2+1;
	  }
	    
	  end1 = 0;
	}
	    
	strcat(result,start);
	return result;
}



// PIP(): crossing number test for a point in a polygon
//      input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      returns: 0 = outside, 1 = inside
int
PIP( Point P, Point* V, int n )
{
	int cn = 0;    // the crossing number counter
	int i;

    // loop through all edges of the polygon
    for (i=0; i<n-1; i++) {    // edge from V[i] to V[i+1]
       if (((V[i].y <= P.y) && (V[i+1].y > P.y))    // an upward crossing
        || ((V[i].y > P.y) && (V[i+1].y <= P.y))) { // a downward crossing
            double vt = (float)(P.y - V[i].y) / (V[i+1].y - V[i].y);
            if (P.x < V[i].x + vt * (V[i+1].x - V[i].x)) // P.x < intersect
                ++cn;   // a valid crossing of y=P.y right of P.x
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if odd (in)

}


//Insert the attributes from the correct row of dbf file

int
Insert_attributes(DBFHandle hDBFHandle, int row)
{
   int i,num_fields;
   char val[1024];

   num_fields = DBFGetFieldCount( hDBFHandle );
   for( i = 0; i < num_fields; i++ )
   {
      if(DBFIsAttributeNULL( hDBFHandle, row, i))
      {
         if(dump_format)
         {
            printf("\\N");
            printf("\t");
         }
         else
         {
            printf("NULL");
            printf(",");
         }
      }

      else /* Attribute NOT NULL */
      {
         switch (types[i]) 
         {
            case FTInteger:
            case FTDouble:
               if ( -1 == snprintf(val, 1024, "%s",
                     DBFReadStringAttribute(hDBFHandle, row, i)) )
               {
                  fprintf(stderr, "Warning: field %d name trucated\n", i);
                  val[1023] = '\0';
               }
	       // pg_atoi() does not do this
	       if ( val[0] == '\0' ) { val[0] = '0'; val[1] = '\0'; }
               break;
            case FTString:
            case FTLogical:
               if ( -1 == snprintf(val, 1024, "%s",
                     DBFReadStringAttribute(hDBFHandle, row, i)) )
               {
                  fprintf(stderr, "Warning: field %d name trucated\n", i);
                  val[1023] = '\0';
               }
               break;
            default:
               fprintf(stderr,
                  "Error: field %d has invalid or unknown field type (%d)\n",
                  i, types[i]);
               exit(1);
         }
 
			if (dump_format) {
				printf("%s",make_good_string(val));
				printf("\t");
			} else {
				printf("'%s'",protect_quotes_string(val));
				printf(",");
			}
		 }
	}
	return 1;
}




// main()     
//see description at the top of this file

int
main (int ARGC, char **ARGV)
{
	int trans,field_precision, field_width;
	int num_entities, phnshapetype;
	double padminbound[8], padmaxbound[8];
	int j,z;
	char  name[64];
	char  name2[64];
	DBFFieldType type = -1;
	extern char *optarg;
	extern int optind;

	istypeM = 0;

	opt = ' ';
	j=0;
	sr_id = shp_file = table = schema = geom = NULL;
	obj=NULL;

	if ( ! parse_cmdline(ARGC, ARGV) ) usage(ARGV[0], 2);

	/* 
	 * Transform table name to lower case unless asked
	 * to keep original case (we'll quote it later on)
	 */
	if ( ! quoteidentifiers )
	{
		for (j=0; j<strlen(table); j++) table[j] = tolower(table[j]);
		if ( schema )
		{
			for (j=0; j<strlen(schema); j++)
				schema[j] = tolower(schema[j]);
		}
	}


	//Open the shp and dbf files
	hSHPHandle = SHPOpen( shp_file, "rb" );
	if (hSHPHandle == NULL) {
		fprintf(stderr, "%s: shape (.shp) or index files (.shx) can not be opened.\n", shp_file);
		exit(-1);
	}
	hDBFHandle = DBFOpen( shp_file, "rb" );
	if (hSHPHandle == NULL || hDBFHandle == NULL){
		fprintf(stderr, "%s: dbf file (.dbf) can not be opened.\n",
			shp_file);
		exit(-1);
	}

	SHPGetInfo(hSHPHandle, NULL, &shpfiletype, NULL, NULL);
	SetPgType();

	fprintf(stderr, "Shapefile type: %s\n", SHPTypeName(shpfiletype));
	fprintf(stderr, "Postgis type: %s[%d]\n", pgtype, pgdims);

	if(opt == 'd')
	{
		//---------------Drop the table--------------------------
		// TODO: if the table has more then one geometry column
		// the DROP TABLE call will leave spurious records in
		// geometry_columns. 
		//
		// If the geometry column in the table being dropped
		// does not match 'the_geom' or the name specified with
		// -g an error is returned by DropGeometryColumn. 
		//
		// The table to be dropped might not exist.
		//
		if ( schema )
		{
			printf("SELECT DropGeometryColumn('%s','%s','%s');\n",
				schema, table, geom);
			printf("DROP TABLE \"%s\".\"%s\";\n", schema, table);
		}
		else
		{
			printf("SELECT DropGeometryColumn('','%s','%s');\n",
				table, geom);
			printf("DROP TABLE \"%s\";\n", table);
		}
	}


	/*
	 * Get col names and types to fully specify them in inserts
	 */
	num_fields = DBFGetFieldCount( hDBFHandle );
	num_records = DBFGetRecordCount(hDBFHandle);
	field_names = malloc(num_fields*sizeof(char*));
	types = (DBFFieldType *)malloc(num_fields*sizeof(int));
	widths = malloc(num_fields*sizeof(int));
	precisions = malloc(num_fields*sizeof(int));
	col_names = malloc((num_fields+2) * sizeof(char) * 32);
	if(opt != 'a'){
		strcpy(col_names, "(gid," );
	}else{
		strcpy(col_names, "(" );
	}

	//fprintf(stderr, "Number of fields from DBF: %d\n", num_fields);
	for(j=0;j<num_fields;j++)
	{
		type = DBFGetFieldInfo(hDBFHandle, j, name, &field_width, &field_precision); 

//fprintf(stderr, "Field %d (%s) width/decimals: %d/%d\n", j, name, field_width, field_precision);
		types[j] = type;
		widths[j] = field_width;
		precisions[j] = field_precision;

		/*
		 * Make field names lowercase unless asked to
		 * keep identifiers case.
		 */
		if ( ! quoteidentifiers ) {
			for(z=0; z<strlen(name) ;z++)
				name[z] = tolower(name[z]);
		}

		/*
		 * Escape names starting with the
		 * escape char (_)
		 */
		if( name[0]=='_' )
		{
			strcpy(name2+2, name);
			name2[0] = '_';
			name2[1] = '_';
			strcpy(name, name2);
		}

		/*
		 * Escape attributes named 'gid'
		 * and pgsql reserved attribute names
		 */
		else if(
			! strcmp(name,"gid") ||
			! strcmp(name, "tableoid") ||
			! strcmp(name, "cmax") ||
			! strcmp(name, "xmax") ||
			! strcmp(name, "cmin") ||
			! strcmp(name, "primary") ||
			! strcmp(name, "oid") ||
			! strcmp(name, "ctid")
		) {
			strcpy(name2+2, name);
			name2[0] = '_';
			name2[1] = '_';
			strcpy(name, name2);
		}

		/* Avoid duplicating field names */
		for(z=0; z < j ; z++){
			if(strcmp(field_names[z],name)==0){
				strcat(name,"__");
				sprintf(name,"%s%i",name,j);
				break;
			}
		}	


		field_names[j] = malloc ( strlen(name)+3 );
		strcpy(field_names[j], name);

		sprintf(col_names, "%s\"%s\",", col_names, name);
	}
	sprintf(col_names, "%s\"%s\")", col_names, geom);


	SHPGetInfo( hSHPHandle, &num_entities, &phnshapetype, &padminbound[0], &padmaxbound[0]);
	j=num_entities;
	while( ( obj == NULL || obj->nVertices == 0 ) && j--)
	{
		obj = SHPReadObject(hSHPHandle,j);
	}
	if ( obj == NULL) 
	{
		fprintf(stderr, "Shapefile contains %d NULL object(s)\n", num_entities);
		exit(-1);
	}

	trans=0;

	printf("BEGIN;\n");

	//if opt is 'a' do nothing, go straight to making inserts
	if(opt == 'c' || opt == 'd') create_table();

	if (dump_format){
		if ( schema )
		{
			printf("COPY \"%s\".\"%s\" %s FROM stdin;\n",
				schema, table, col_names);
		}
		else
		{
			printf("COPY \"%s\" %s FROM stdin;\n",
				table, col_names);
		}
	}

	obj = SHPReadObject(hSHPHandle,0);
	

/**************************************************************
 * 
 *   MAIN SHAPE OBJECTS SCAN
 * 
 **************************************************************/
	for (j=0;j<num_entities; j++)
	{
		//wrap a transaction block around each 250 inserts...
		if ( ! dump_format )
		{
			if (trans == 250) {
				trans=0;
				printf("END;\n");
				printf("BEGIN;\n");
			}
		}
		trans++;
		// transaction stuff done

		//open the next object
		obj = SHPReadObject(hSHPHandle,j);

		if (!dump_format)
		{
			if ( schema )
			{
				printf("INSERT INTO \"%s\".\"%s\" %s VALUES (",
                                        schema, table, col_names);
			}
			else
			{
		  		printf("INSERT INTO \"%s\" %s VALUES (",
					table, col_names);
			}
			if(opt != 'a')
			{
				printf("'%d',", j);
			}
		}
		else
		{
			if(opt != 'a')
			{
				printf("%d\t", j);
			}
		}
		Insert_attributes(hDBFHandle,j);

		// ---------- NULL SHAPE -----------------
		if (obj->nVertices == 0)
		{
			if (dump_format) printf("\\N\n");
			else printf("NULL);\n");
			SHPDestroyObject(obj);	
			continue;
		}

		switch (obj->nSHPType)
		{
			case SHPT_POLYGON:
			case SHPT_POLYGONM:
			case SHPT_POLYGONZ:
				InsertPolygon();
				break;

			case SHPT_POINT:
			case SHPT_POINTM:
			case SHPT_POINTZ:
				InsertPoint();
				break;

			case SHPT_MULTIPOINT:
			case SHPT_MULTIPOINTM:
			case SHPT_MULTIPOINTZ:
				InsertMultiPoint();
				break;

			case SHPT_ARC:
			case SHPT_ARCM:
			case SHPT_ARCZ:
				InsertLineString(j);
				break;

			default:
				printf ("\n\n**** Type is NOT SUPPORTED, type id = %d ****\n\n",
					obj->nSHPType);
				break;

		}
		
		SHPDestroyObject(obj);	

	} // END of MAIN SHAPE OBJECT LOOP


	if ((dump_format) ) {
		printf("\\.\n");

	} 

	free(col_names);
	if(opt != 'a')
	{
		if ( schema )
		{
			printf("\nALTER TABLE ONLY \"%s\".\"%s\" ADD CONSTRAINT \"%s_pkey\" PRIMARY KEY (gid);\n",schema,table,table);
			if(j > 1)
			{
				printf("SELECT setval ('\"%s\".\"%s_gid_seq\"', %i, true);\n", schema, table, j-1);
			}
		}
		else
		{
			printf("\nALTER TABLE ONLY \"%s\" ADD CONSTRAINT \"%s_pkey\" PRIMARY KEY (gid);\n",table,table);
			if(j > 1){
				printf("SELECT setval ('\"%s_gid_seq\"', %i, true);\n", table, j-1);
			}
		}
	}

	printf("END;\n"); //End the last transaction

	return(1);
}//end main()

void
create_table()
{
	int j;
	int field_precision, field_width;
	DBFFieldType type = -1;

	/* 
	 * Create a table for inserting the shapes into with appropriate
	 * columns and types
	 */

	if ( schema )
	{
		printf("CREATE TABLE \"%s\".\"%s\" (gid serial", schema, table);
	}
	else
	{
		printf("CREATE TABLE \"%s\" (gid serial", table);
	}

	for(j=0;j<num_fields;j++)
	{
		type = types[j];
		field_width = widths[j];
		field_precision = precisions[j];

		printf(", \"%s\" ", field_names[j]);

		if(hDBFHandle->pachFieldType[j] == 'D' ) /* Date field */
		{
			printf ("varchar(8)");//date data-type is not supported in API so check for it explicity before the api call.
		}
			
		else
		{

			if(type == FTString)
			{
				printf ("varchar");
			}
			else if(type == FTInteger)
			{
				if ( forceint4 || field_width <= 9 )
				{
					printf ("int4");
				}
				else if( field_width > 18 )
				{
					printf("numeric(%d,0)",
						field_width);
				}
				else 
				{
					printf ("int8");
				}
			}
			else if(type == FTDouble)
			{
				if( field_width > 18 )
				{
					printf ("numeric");
				}
				else
				{
					printf ("float8");
				}
			}
			else if(type == FTLogical)
			{
				printf ("boolean");
			}
			else
			{
				printf ("Invalid type in DBF file");
			}
		}	
	}
	printf (");\n");

	//create the geometry column with an addgeometry call to dave's function
	if ( schema )
	{
		printf("SELECT AddGeometryColumn('%s','%s','%s','%s',",
			schema, table, geom, sr_id);
	}
	else
	{
		printf("SELECT AddGeometryColumn('','%s','%s','%s',",
			table, geom, sr_id);
	}

	printf("'%s',%d);\n", pgtype, pgdims);
}

void
usage(char *me, int exitcode)
{
	fprintf(stderr, "RCSID: %s\n", rcsid);
	fprintf(stderr, "USAGE: %s [<options>] <shapefile> [<schema>.]<table>\n", me);
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -s <srid>  Set the SRID field. If not specified it defaults to -1.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  (-d|a|c) These are mutually exclusive options:\n");
	fprintf(stderr, "      -d  Drops the table , then recreates it and populates\n");
	fprintf(stderr, "          it with current shape file data.\n");
	fprintf(stderr, "      -a  Appends shape file into current table, must be\n");
	fprintf(stderr, "          exactly the same table schema.\n");
	fprintf(stderr, "      -c  Creates a new table and populates it, this is the\n");
	fprintf(stderr, "          default if you do not specify any options.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -g <geometry_column> Specify the name of the geometry column\n");
	fprintf(stderr, "     (mostly useful in append mode).\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -D  Use postgresql dump format (defaults to sql insert\n");
	fprintf(stderr, "      statments.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -k  Keep postgresql identifiers case.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -i  Use int4 type for all integer dbf fields.\n");
	exit (exitcode);
}

void
InsertLineString(int id)
{
	int pi; // part index
	unsigned int subtype = LINETYPE | (wkbtype&WKBZOFFSET) | 
		(wkbtype&WKBMOFFSET);

	/* Invalid (MULTI)Linestring */
	if ( obj->nVertices < 2 )
	{
		fprintf(stderr,
			"MULTILINESTRING %d as %d vertices, set to NULL\n",
			id, obj->nVertices);
		if (dump_format) printf("\\N\n");
		else printf("NULL);\n");

		SHPDestroyObject(obj);
		return;
	}

	if (!dump_format) printf("'");
	if ( sr_id && sr_id != "-1" ) printf("SRID=%s;", sr_id);

	print_wkb_byte(getEndianByte());
	print_wkb_int(wkbtype);
	print_wkb_int(obj->nParts);

	for (pi=0; pi<obj->nParts; pi++)
	{
		int vi; // vertex index
		int vs; // start vertex
		int ve; // end vertex

		print_wkb_byte(getEndianByte());
		print_wkb_int(subtype);

		// Set start and end vertexes
		if ( pi==obj->nParts-1 ) ve = obj->nVertices;
		else ve = obj->panPartStart[pi+1];
		vs = obj->panPartStart[pi];

		print_wkb_int(obj->nVertices);
		for ( vi=vs; vi<ve; vi++)
		{
			print_wkb_double(obj->padfX[vi]);
			print_wkb_double(obj->padfY[vi]);
			if ( wkbtype & WKBZOFFSET )
				print_wkb_double(obj->padfZ[vi]);
			if ( wkbtype & WKBMOFFSET )
				print_wkb_double(obj->padfM[vi]);
		}
	}

	if (dump_format) printf("\n");
	else printf("');\n");
}

//This function basically deals with the polygon case.
//it sorts the polys in order of outer,inner,inner, so that inners
//always come after outers they are within 
void
InsertPolygon()
{
	Ring **Outer;    // Pointers to Outer rings
	int out_index=0; // Count of Outer rings
	Ring **Inner;    // Pointers to Inner rings
	int in_index=0;  // Count of Inner rings
	int pi; // part index
	unsigned int subtype = POLYGONTYPE | (wkbtype&WKBZOFFSET) | 
		(wkbtype&WKBMOFFSET);

#ifdef DEBUG
	fprintf(stderr, "InsertPolygon: allocated space for %d rings\n",
		obj->nParts);
#endif

	// Allocate initial memory
	Outer = (Ring**)malloc(sizeof(Ring*)*obj->nParts);
	Inner = (Ring**)malloc(sizeof(Ring*)*obj->nParts);

	// Iterate over rings dividing in Outers and Inners
	for (pi=0; pi<obj->nParts; pi++)
	{
		int vi; // vertex index
		int vs; // start index
		int ve; // end index
		int nv; // number of vertex
		double area = 0.0;
		Ring *ring;

		// Set start and end vertexes
		if ( pi==obj->nParts-1 ) ve = obj->nVertices;
		else ve = obj->panPartStart[pi+1];
		vs = obj->panPartStart[pi];

		// Compute number of vertexes
		nv = ve-vs;

		// Allocate memory for a ring
		ring = (Ring*)malloc(sizeof(Ring));
		ring->list = (Point*)malloc(sizeof(Point)*nv);
		ring->n = nv;
		ring->next = NULL;
		ring->linked = 0;

		// Iterate over ring vertexes
		for ( vi=vs; vi<ve; vi++)
		{
			int vn = vi+1; // next vertex for area
			if ( vn==ve ) vn = vs;

			ring->list[vi-vs].x = obj->padfX[vi];
			ring->list[vi-vs].y = obj->padfY[vi];
			ring->list[vi-vs].z = obj->padfZ[vi];
			ring->list[vi-vs].m = obj->padfM[vi];

			area += (obj->padfX[vi] * obj->padfY[vn]) -
				(obj->padfY[vi] * obj->padfX[vn]); 
		}

		// Close the ring with first vertex 
		//ring->list[vi].x = obj->padfX[vs];
		//ring->list[vi].y = obj->padfY[vs];
		//ring->list[vi].z = obj->padfZ[vs];
		//ring->list[vi].m = obj->padfM[vs];

		// Clockwise (or single-part). It's an Outer Ring !
		if(area < 0.0 || obj->nParts ==1) {
			Outer[out_index] = ring;
			out_index++;
		}

		// Counterclockwise. It's an Inner Ring !
		else {
			Inner[in_index] = ring;
			in_index++;
		}
	}

#ifdef DEBUG
	fprintf(stderr, "InsertPolygon: found %d Outer, %d Inners\n",
		out_index, in_index);
#endif

	// Put the inner rings into the list of the outer rings
	// of which they are within
	for(pi=0; pi<in_index; pi++)
	{
		Point pt,pt2;
		int i;
		Ring *inner=Inner[pi], *outer=NULL;

		pt.x = inner->list[0].x;
		pt.y = inner->list[0].y;

		pt2.x = inner->list[1].x;
		pt2.y = inner->list[1].y;

		for(i=0; i<out_index; i++)
		{
			int in;

			in = PIP(pt, Outer[i]->list, Outer[i]->n-1);
			if( in || PIP(pt2,Outer[i]->list, Outer[i]->n-1) )
			{
				outer = Outer[i];
				break;
			}
			//fprintf(stderr, "!PIP %s\nOUTE %s\n", dump_ring(Inner[pi]), dump_ring(Outer[i]));
		}

		if ( outer )
		{
			outer->linked++;
			while(outer->next) outer = outer->next;
			outer->next = inner;
			break;
		}

		// The ring wasn't within any outer rings,
		// assume it is a new outer ring.
		Outer[out_index] = inner;
		out_index++;
	}

	if (!dump_format) printf("'");
	if ( sr_id && sr_id != "-1" ) printf("SRID=%s;", sr_id);

	print_wkb_byte(getEndianByte());
	print_wkb_int(wkbtype);
	print_wkb_int(out_index); // npolys

	// Write the coordinates
	for(pi=0; pi<out_index; pi++)
	{
		Ring *poly;

		poly = Outer[pi];

		print_wkb_byte(getEndianByte());
		print_wkb_int(subtype);
		print_wkb_int(poly->linked+1); // nrings

		while(poly)
		{
			int vi; // vertex index

			print_wkb_int(poly->n); // npoints

			for(vi=0; vi<poly->n; vi++)
			{
				print_wkb_double(poly->list[vi].x);
				print_wkb_double(poly->list[vi].y);
				if ( wkbtype & WKBZOFFSET )
					print_wkb_double(poly->list[vi].z);
				if ( wkbtype & WKBMOFFSET )
					print_wkb_double(poly->list[vi].m);
			}

			poly = poly->next;
		}

	}

	if (dump_format) printf("\n");
	else printf("');\n");

	// Release all memory
	for(pi=0; pi<out_index; pi++)
	{
		Ring *Poly, *temp;
		Poly = Outer[pi];
		while(Poly != NULL){
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
	}
	free(Outer);
	free(Inner);
}

void
InsertPoint()
{
	if (!dump_format) printf("'");
	if ( sr_id && sr_id != "-1" ) printf("SRID=%s;", sr_id);

	print_wkb_byte(getEndianByte());
	print_wkb_int(wkbtype);
	print_wkb_double(obj->padfX[0]);
	print_wkb_double(obj->padfY[0]);
	if ( wkbtype & WKBZOFFSET ) print_wkb_double(obj->padfZ[0]);
	if ( wkbtype & WKBMOFFSET ) print_wkb_double(obj->padfM[0]);

	if (dump_format) printf("\n");
	else printf("');\n");
}

void
InsertMultiPoint()
{
	unsigned int u;
	unsigned int subtype = POINTTYPE | (wkbtype&WKBZOFFSET) | 
		(wkbtype&WKBMOFFSET);

	if (!dump_format) printf("'");
	if ( sr_id && sr_id != "-1" ) printf("SRID=%s;", sr_id);

	print_wkb_byte(getEndianByte());
	print_wkb_int(wkbtype);
	print_wkb_int(obj->nVertices);
	
	for (u=0;u<obj->nVertices; u++)
	{
		print_wkb_byte(getEndianByte());
		print_wkb_int(subtype);
		print_wkb_double(obj->padfX[u]);
		print_wkb_double(obj->padfY[u]);
		if ( wkbtype & WKBZOFFSET ) print_wkb_double(obj->padfZ[u]);
		if ( wkbtype & WKBMOFFSET ) print_wkb_double(obj->padfM[u]);
	}

	if (dump_format) printf("\n");
	else printf("');\n");
}

int
parse_cmdline(int ARGC, char **ARGV)
{
	int c;
	int curindex=0;
	char  *ptr;

	while ((c = getopt(ARGC, ARGV, "kcdaDs:g:i")) != EOF){
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
	       case 'D':
		    dump_format =1;
                    break;
               case 's':
                    sr_id = optarg;
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
               case '?':
               default:              
		return 0;
               }
	}

	if ( !sr_id ) sr_id = "-1";

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

	return 1;
}


void
SetPgType()
{
	switch(shpfiletype)
	{
		case SHPT_POINT: // Point
			pgtype = "POINT";
			wkbtype = POINTTYPE;
			pgdims = 2;
			break;
		case SHPT_ARC: // PolyLine
			pgtype = "MULTILINESTRING";
			wkbtype = MULTILINETYPE ;
			pgdims = 2;
			break;
		case SHPT_POLYGON: // Polygon
			pgtype = "MULTIPOLYGON";
			wkbtype = MULTIPOLYGONTYPE;
			pgdims = 2;
			break;
		case SHPT_MULTIPOINT: // MultiPoint
			pgtype = "MULTIPOINT";
			wkbtype = MULTIPOINTTYPE;
			pgdims = 2;
			break;
		case SHPT_POINTM: // PointM
			pgtype = "POINTM";
			wkbtype = POINTTYPE | WKBMOFFSET;
			pgdims = 3;
			istypeM = 1;
			break;
		case SHPT_ARCM: // PolyLineM
			pgtype = "MULTILINESTRINGM";
			wkbtype = MULTILINETYPE | WKBMOFFSET;
			pgdims = 3;
			istypeM = 1;
			break;
		case SHPT_POLYGONM: // PolygonM
			pgtype = "MULTIPOLYGONM";
			wkbtype = MULTIPOLYGONTYPE | WKBMOFFSET;
			pgdims = 3;
			istypeM = 1;
			break;
		case SHPT_MULTIPOINTM: // MultiPointM
			pgtype = "MULTIPOINTM";
			wkbtype = MULTIPOINTTYPE | WKBMOFFSET;
			pgdims = 3;
			istypeM = 1;
			break;
		case SHPT_POINTZ: // PointZ
			pgtype = "POINT";
			wkbtype = POINTTYPE | WKBMOFFSET | WKBZOFFSET;
			pgdims = 4;
			break;
		case SHPT_ARCZ: // PolyLineZ
			pgtype = "MULTILINESTRING";
			wkbtype = MULTILINETYPE | WKBZOFFSET | WKBMOFFSET;
			pgdims = 4;
			break;
		case SHPT_POLYGONZ: // MultiPolygonZ
			pgtype = "MULTIPOLYGON";
			wkbtype = MULTIPOLYGONTYPE | WKBZOFFSET | WKBMOFFSET;
			pgdims = 4;
			break;
		case SHPT_MULTIPOINTZ: // MultiPointZ
			pgtype = "MULTIPOINT";
			wkbtype = MULTIPOINTTYPE | WKBZOFFSET | WKBMOFFSET;
			pgdims = 4;
			break;
		default:
			pgtype = "GEOMETRY";
			wkbtype = COLLECTIONTYPE | WKBZOFFSET | WKBMOFFSET;
			pgdims = 4;
			fprintf(stderr, "Unknown geometry type: %d\n",
				shpfiletype);
			break;
	}
}

char *
dump_ring(Ring *ring)
{
	char *buf = malloc(256);
	int i;
	for (i=0; i<ring->n; i++)
	{
		if (i) sprintf(buf, "%s,", buf);
		sprintf(buf, "%s%g %g",
			buf,
			ring->list[i].x,
			ring->list[i].y);
	}
	return buf;
}

//--------------- WKB handling 

static char outchr[]={"0123456789ABCDEF"};

static int endian_check_int = 1; // dont modify this!!!

static char getEndianByte()
{
	// 0 = big endian, 1 = little endian
	if ( *((char *) &endian_check_int) ) return 1;
	else return 0;
}

static void
print_wkb_double(double val)
{
	print_wkb_bytes((unsigned char *)&val, 1, 8);
}

static void
print_wkb_byte(unsigned char val)
{
	print_wkb_bytes((unsigned char *)&val, 1, 1);
}

static void
print_wkb_int(int val)
{
	print_wkb_bytes((unsigned char *)&val, 1, 4);
}

static void
print_wkb_bytes(unsigned char *ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; // byte count
	static char buf[256];
	char *bufp;

	if ( size*cnt*2 > 256 )
	{
		fprintf(stderr,
			"You found a bug! wkb_bytes does not allocate enough bytes");
		exit(4);
	}

	bufp = buf;
	while(cnt--){
		for(bc=0; bc<size; bc++)
		{
			*bufp++ = outchr[ptr[bc]>>4];
			*bufp++ = outchr[ptr[bc]&0x0F];
		}
	}
	*bufp = '\0';
	//fprintf(stderr, "\nwkbbytes:%s\n", buf);
	printf("%s", buf);
}

/**********************************************************************
 * $Log$
 * Revision 1.74  2004/10/17 13:25:44  strk
 * removed USE_WKB partially-used define
 *
 * Revision 1.73  2004/10/17 13:24:44  strk
 * HEXWKB polygon
 *
 * Revision 1.72  2004/10/17 12:59:12  strk
 * HEXWKB multiline output
 *
 * Revision 1.71  2004/10/17 12:26:02  strk
 * Point and MultiPoint loaded using HEXWKB.
 *
 * Revision 1.70  2004/10/15 22:01:35  strk
 * Initial WKB functionalities
 *
 * Revision 1.69  2004/10/07 21:52:28  strk
 * Lots of rewriting/cleanup. TypeM/TypeZ supports.
 *
 * Revision 1.68  2004/10/07 06:54:24  strk
 * cleanups
 *
 * Revision 1.67  2004/10/06 10:11:16  strk
 * Other separator fixes
 *
 * Revision 1.66  2004/10/06 09:40:27  strk
 * Handled 0-DBF-attributes corner case.
 *
 * Revision 1.65  2004/09/20 17:13:31  strk
 * changed comments to better show shape type handling
 *
 * Revision 1.64  2004/08/20 08:14:37  strk
 * Whole output wrapped in transaction blocks.
 * Drops are out of transaction, and multiple transactions are used
 * for INSERT mode.
 *
 **********************************************************************/
