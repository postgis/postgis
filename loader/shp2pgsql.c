/**********************************************************************
 * $Id$
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
 *
 * Original Author: Jeff Lounsbury, jeffloun@refractions.net
 *
 * Maintainer: Sandro Santilli, strk@refractions.net
 *
 **********************************************************************/

#include "shapefil.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
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

//#define DEBUG 1

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
int	createindex = 0;
char    opt = ' ';
char    *col_names = NULL;
char	*pgtype;
int	istypeM = 0;
int	pgdims;
unsigned int wkbtype;
char  	*shp_file = NULL;
int	hwgeom = 0; // old (hwgeom) mode
#ifdef USE_ICONV
char	*encoding=NULL;
#endif

DBFFieldType *types;	/* Fields type, width and precision */
SHPHandle  hSHPHandle;
DBFHandle  hDBFHandle;
int shpfiletype;
SHPObject  *obj=NULL;
int 	*widths;
int 	*precisions;
char    *table=NULL,*schema=NULL,*geom=NULL;
int     num_fields,num_records,num_entities;
char    **field_names;
char 	*sr_id = NULL;

/* Prototypes */
int Insert_attributes(DBFHandle hDBFHandle, int row);
char *make_good_string(char *str);
char *protect_quotes_string(char *str);
int PIP( Point P, Point* V, int n );
void *safe_malloc(size_t size);
void CreateTable(void);
void CreateIndex(void);
void usage(char *me, int exitcode);
void InsertPoint(void);
void InsertPointWKT(void);
void InsertMultiPoint(void);
void InsertPolygon(void);
void InsertPolygonWKT(void);
void InsertLineString(int id);
void InsertLineStringWKT(int id);
int ParseCmdline(int ARGC, char **ARGV);
void SetPgType(void);
char *dump_ring(Ring *ring);
#ifdef USE_ICONV
char *utf8(const char *fromcode, char *inputbuf);
#endif
int FindPolygons(SHPObject *obj, Ring ***Out);
void ReleasePolygons(Ring **polys, int npolys);
void DropTable(char *schema, char *table, char *geom);
void GetFieldsSpec(void);
void LoadData(void);
void UpdateSequence(void);
void OpenShape(void);
void LowerCase(char *s);
void Cleanup(void);


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

	char *result;
	char *ptr, *optr;
	int toescape = 0;
	size_t size;
#ifdef USE_ICONV
	char *utf8str=NULL;

	if ( encoding )
	{
		utf8str=utf8(encoding, str);
		if ( ! utf8str ) exit(1);
		str = utf8str; 
	}
#endif

	ptr = str;

	while (*ptr) {
		if ( *ptr == '\t' || *ptr == '\\' ) toescape++;
		ptr++;
	}

	if (toescape == 0) return str;

	size = ptr-str+toescape+1;

	result = calloc(1, size);

	optr=result;
	ptr=str;
	while (*ptr) {
		if ( *ptr == '\t' || *ptr == '\\' ) *optr++='\\';
		*optr++=*ptr++;
	}
	*optr='\0';

#ifdef USE_ICONV
	if ( encoding ) free(str);
#endif

	return result;
	
}

char *
protect_quotes_string(char *str)
{
	//find all quotes and make them \quotes
	//find all '\' and make them '\\'
	// 
	// 1. find # of characters
	// 2. make new string 

	char	*result;
	char	*ptr, *optr;
	int	toescape = 0;
	size_t size;
#ifdef USE_ICONV
	char *utf8str=NULL;

	if ( encoding )
	{
		utf8str=utf8(encoding, str);
		if ( ! utf8str ) exit(1);
		str = utf8str; 
	}
#endif

	ptr = str;

	while (*ptr) {
		if ( *ptr == '\'' || *ptr == '\\' ) toescape++;
		ptr++;
	}

	if (toescape == 0) return str;
	
	size = ptr-str+toescape+1;

	result = calloc(1, size);

	optr=result;
	ptr=str;
	while (*ptr) {
		if ( *ptr == '\'' || *ptr == '\\' ) *optr++='\\';
		*optr++=*ptr++;
	}
	*optr='\0';

#ifdef USE_ICONV
	if ( encoding ) free(str);
#endif

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
   char *escval;

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
	       if ( val[strlen(val)-1] == '.' ) val[strlen(val)-1] = '\0';
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
				escval = make_good_string(val);
				printf("%s", escval);
				printf("\t");
			} else {
				escval = protect_quotes_string(val);
				printf("'%s'", escval);
				printf(",");
			}
			if ( val != escval ) free(escval);
		 }
	}
	return 1;
}




// main()     
//see description at the top of this file

int
main (int ARGC, char **ARGV)
{

	/*
	 * Parse command line
	 */
	if ( ! ParseCmdline(ARGC, ARGV) ) usage(ARGV[0], 2);

	/*
	 * Open shapefile and initialize globals
	 */
	OpenShape();

	/*
	 * Compute output geometry type
	 */
	SetPgType();

	fprintf(stderr, "Shapefile type: %s\n", SHPTypeName(shpfiletype));
	fprintf(stderr, "Postgis type: %s[%d]\n", pgtype, pgdims);

#ifdef USE_ICONV
	if ( encoding )
	{
		printf("SET CLIENT_ENCODING TO UTF8;\n");
	}
#endif // defined USE_ICONV

	/*
	 * Drop table if requested
	 */
	if(opt == 'd') DropTable(schema, table, geom);

	/*
	 * Get col names and types for table creation
	 * and data load.
	 */
	GetFieldsSpec();

	printf("BEGIN;\n");

	/*
	 * If not in 'append' mode create the spatial table
	 */
	if(opt != 'a') CreateTable();

	/*
	 * Create GiST index if requested
	 */
	if(createindex) CreateIndex();

	/*
	 * Generate INSERT or COPY lines
	 */
	if(opt != 'p') LoadData();

	printf("END;\n"); // End the last transaction


	return 0; 
}//end main()

void
LowerCase(char *s)
{
	int j;
	for (j=0; j<strlen(s); j++) s[j] = tolower(s[j]);
}

void
Cleanup(void)
{
	if ( col_names ) free(col_names);
}

void
OpenShape(void)
{
	int j;
	SHPObject *obj=NULL;

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
	SHPGetInfo(hSHPHandle, &num_entities, &shpfiletype, NULL, NULL);

	/* Check we have at least a not-null geometry */
	for (j=0; j<num_entities; j++)
	{
		obj = SHPReadObject(hSHPHandle,j);
		if ( obj && obj->nVertices > 0 ) {
			SHPDestroyObject(obj);
			break;
		}
		SHPDestroyObject(obj);
		obj=NULL;
	}

	if ( obj == NULL) 
	{
		fprintf(stderr, "Shapefile contains %d NULL object(s)\n",
			num_entities);
		exit(-1);
	}

}

/*
 * Not used
 */
void
UpdateSequence(void)
{
	if ( schema )
	{
		if(num_entities>1){
			printf("SELECT setval('\"%s\".\"%s_gid_seq\"', %i, true);\n", schema, table, num_entities-1);
		}
	}
	else
	{
		if(num_entities>1){
			printf("SELECT setval('\"%s_gid_seq\"', %i, true);\n", table, num_entities-1);
		}
	}
}

void
CreateTable(void)
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
		printf("CREATE TABLE \"%s\".\"%s\" (gid serial PRIMARY KEY",
			schema, table);
	}
	else
	{
		printf("CREATE TABLE \"%s\" (gid serial PRIMARY KEY", table);
	}

	for(j=0;j<num_fields;j++)
	{
		type = types[j];
		field_width = widths[j];
		field_precision = precisions[j];

		printf(",\n\"%s\" ", field_names[j]);

		if(hDBFHandle->pachFieldType[j] == 'D' ) /* Date field */
		{
			printf ("varchar(8)");//date data-type is not supported in API so check for it explicity before the api call.
		}
			
		else
		{

			if(type == FTString)
			{
				// use DBF attribute size as maximum width
				printf ("varchar(%d)", field_width);
			}
			else if(type == FTInteger)
			{
				if ( forceint4 )
				{
					printf ("int4");
				}
				else if  ( field_width <= 6 )
				{
					printf ("int2");
				}
				else if  ( field_width <= 11 )
				{
					printf ("int4");
				}
				else if  ( field_width <= 20 )
				{
					printf ("int8");
				}
				else 
				{
					printf("numeric(%d,0)",
						field_width);
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
CreateIndex(void)
{
	/* 
	 * Create gist index
	 */
	if ( schema )
	{
		printf("CREATE INDEX \"%s_%s_gist\" ON \"%s\".\"%s\" using gist (\"%s\" gist_geometry_ops);\n", table, geom, schema, table, geom);
	}
	else
	{
		printf("CREATE INDEX \"%s_%s_gist\" ON \"%s\" using gist (\"%s\" gist_geometry_ops);\n", table, geom, table, geom);
	}
}

void
LoadData(void)
{
	int j, trans=0;

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


	/**************************************************************
	 * 
	 *   MAIN SHAPE OBJECTS SCAN
	 * 
	 **************************************************************/
	for (j=0; j<num_entities; j++)
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
				if ( hwgeom ) InsertPolygonWKT();
				else InsertPolygon();
				break;

			case SHPT_POINT:
			case SHPT_POINTM:
			case SHPT_POINTZ:
				if ( hwgeom ) InsertPointWKT();
				else InsertPoint();
				break;

			case SHPT_MULTIPOINT:
			case SHPT_MULTIPOINTM:
			case SHPT_MULTIPOINTZ:
				if ( hwgeom ) InsertPointWKT();
				else InsertMultiPoint();
				break;

			case SHPT_ARC:
			case SHPT_ARCM:
			case SHPT_ARCZ:
				if ( hwgeom ) InsertLineStringWKT(j);
				else InsertLineString(j);
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
	fprintf(stderr, "  (-d|a|c|p) These are mutually exclusive options:\n");
	fprintf(stderr, "      -d  Drops the table , then recreates it and populates\n");
	fprintf(stderr, "          it with current shape file data.\n");
	fprintf(stderr, "      -a  Appends shape file into current table, must be\n");
	fprintf(stderr, "          exactly the same table schema.\n");
	fprintf(stderr, "      -c  Creates a new table and populates it, this is the\n");
	fprintf(stderr, "          default if you do not specify any options.\n");
 	fprintf(stderr, "      -p  Prepare mode, only creates the table\n");
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
	fprintf(stderr, "\n");
	fprintf(stderr, "  -I  Create a GiST index on the geometry column.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -w  Use wkt format (for postgis-0.x support - drops M - drifts coordinates).\n");
#ifdef USE_ICONV
	fprintf(stderr, "\n");
	fprintf(stderr, "  -W <ENCODING_NAME> Specify the character encoding of Shape's\n");
	fprintf(stderr, "     attribute column. (default : \"ASCII\")\n");
#endif
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

		print_wkb_int(ve-vs);
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

void
InsertLineStringWKT(int id)
{
	int pi; // part index

	/* Invalid (MULTI)Linestring */
	if ( obj->nVertices < 2 )
	{
		fprintf(stderr,
			"MULTILINESTRING %d as %d vertices, set to NULL\n",
			id, obj->nVertices);
		if (dump_format) printf("\\N\n");
		else printf("NULL);\n");

		return;
	}

	if (dump_format) printf("SRID=%s;MULTILINESTRING(",sr_id);
	else printf("GeometryFromText('MULTILINESTRING (");

	for (pi=0; pi<obj->nParts; pi++)
	{
		int vi; // vertex index
		int vs; // start vertex
		int ve; // end vertex

		if (pi) printf(",");
		printf("(");

		// Set start and end vertexes
		if ( pi==obj->nParts-1 ) ve = obj->nVertices;
		else ve = obj->panPartStart[pi+1];
		vs = obj->panPartStart[pi];

		for ( vi=vs; vi<ve; vi++)
		{
			if ( vi ) printf(",");
			printf("%.15g %.15g",
				obj->padfX[vi],
				obj->padfY[vi]);
			if ( wkbtype & WKBZOFFSET )
				printf(" %.15g", obj->padfZ[vi]);
		}

		printf(")");
				
	}

	if (dump_format) printf(")\n");
	else printf(")',%s) );\n",sr_id);
}

int
FindPolygons(SHPObject *obj, Ring ***Out)
{
	Ring **Outer;    // Pointers to Outer rings
	int out_index=0; // Count of Outer rings
	Ring **Inner;    // Pointers to Inner rings
	int in_index=0;  // Count of Inner rings
	int pi; // part index

#ifdef DEBUG
	static int call = -1;
	call++;

	fprintf(stderr, "FindPolygons[%d]: allocated space for %d rings\n",
		call, obj->nParts);
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
	fprintf(stderr, "FindPolygons[%d]: found %d Outer, %d Inners\n",
		call, out_index, in_index);
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

			in = PIP(pt, Outer[i]->list, Outer[i]->n);
			if( in || PIP(pt2, Outer[i]->list, Outer[i]->n) )
			{
				outer = Outer[i];
				break;
			}
			//fprintf(stderr, "!PIP %s\nOUTE %s\n", dump_ring(inner), dump_ring(Outer[i]));
		}

		if ( outer )
		{
			outer->linked++;
			while(outer->next) outer = outer->next;
			outer->next = inner;
		}
		else
		{
			// The ring wasn't within any outer rings,
			// assume it is a new outer ring.
#ifdef DEBUG
			fprintf(stderr,
				"FindPolygons[%d]: hole %d is orphan\n",
				call, pi);
#endif
			Outer[out_index] = inner;
			out_index++;
		}
	}

	*Out = Outer;
	free(Inner);

	return out_index;
}

void
ReleasePolygons(Ring **polys, int npolys)
{
	int pi;
	// Release all memory
	for(pi=0; pi<npolys; pi++)
	{
		Ring *Poly, *temp;
		Poly = polys[pi];
		while(Poly != NULL){
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
	}
}

//This function basically deals with the polygon case.
//it sorts the polys in order of outer,inner,inner, so that inners
//always come after outers they are within 
void
InsertPolygon(void)
{
	unsigned int subtype = POLYGONTYPE | (wkbtype&WKBZOFFSET) | 
		(wkbtype&WKBMOFFSET);
	Ring **Outer;
	int out_index;
	int pi; // part index

	out_index = FindPolygons(obj, &Outer);

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
	ReleasePolygons(Outer, out_index);
	free(Outer);
}

void
InsertPolygonWKT(void)
{
	Ring **Outer;    // Pointers to Outer rings
	int out_index=0; // Count of Outer rings
	int pi; // part index

#ifdef DEBUG
	static int call = -1;
	call++;

	fprintf(stderr, "InsertPolygon[%d]: allocated space for %d rings\n",
		call, obj->nParts);
#endif

	out_index = FindPolygons(obj, &Outer);

	if (dump_format) printf("SRID=%s;MULTIPOLYGON(",sr_id );
	else printf("GeometryFromText('MULTIPOLYGON(");

	// Write the coordinates
	for(pi=0; pi<out_index; pi++)
	{
		Ring *poly;

		poly = Outer[pi];

		if ( pi ) printf(",");
		printf("(");

		while(poly)
		{
			int vi; // vertex index

			printf("(");
			for(vi=0; vi<poly->n; vi++)
			{
				if ( vi ) printf(",");
				printf("%.15g %.15g",
					poly->list[vi].x,
					poly->list[vi].y);
				if ( wkbtype & WKBZOFFSET )
					printf(" %.15g", poly->list[vi].z);
			}
			printf(")");

			poly = poly->next;
			if ( poly ) printf(",");
		}

		printf(")");

	}

	if (dump_format) printf(")\n");
	else printf(")',%s) );\n",sr_id);

	// Release all memory
	ReleasePolygons(Outer, out_index);
	free(Outer);
}

void
InsertPoint(void)
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
InsertPointWKT(void)
{
	unsigned int u;
	if (dump_format) printf("SRID=%s;%s(", sr_id, pgtype);
	else printf("GeometryFromText('%s(", pgtype);

	for (u=0;u<obj->nVertices; u++){
		if (u>0) printf(",");
		printf("%.15g %.15g",obj->padfX[u],obj->padfY[u]);
		if ( wkbtype & WKBZOFFSET ) printf(" %.15g", obj->padfZ[u]);
	}
	if (dump_format) printf(")\n");
	else printf(")',%s) );\n",sr_id);

}

void
InsertMultiPoint(void)
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
ParseCmdline(int ARGC, char **ARGV)
{
	int c;
	int curindex=0;
	char  *ptr;
	extern char *optarg;
	extern int optind;

	while ((c = getopt(ARGC, ARGV, "kcdapDs:g:iW:wI")) != EOF){
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
               case 'I':
                    createindex = 1;
                    break;
               case 'w':
                    hwgeom = 1;
                    break;
#ifdef USE_ICONV
		case 'W':
                    encoding = optarg;
                    break;
#endif
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


void
SetPgType(void)
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
			wkbtype = POINTTYPE | WKBMOFFSET;
			if ( ! hwgeom ) {
				pgtype = "POINTM";
				pgdims = 3;
				istypeM = 1;
			} else {
				pgtype = "POINT";
				pgdims = 2;
			}
			break;
		case SHPT_ARCM: // PolyLineM
			wkbtype = MULTILINETYPE | WKBMOFFSET;
			if ( ! hwgeom ) {
				pgtype = "MULTILINESTRINGM";
				pgdims = 3;
				istypeM = 1;
			} else {
				pgtype = "MULTILINESTRING";
				pgdims = 2;
			}
			break;
		case SHPT_POLYGONM: // PolygonM
			wkbtype = MULTIPOLYGONTYPE | WKBMOFFSET;
			if ( ! hwgeom ) {
				pgtype = "MULTIPOLYGONM";
				pgdims = 3;
				istypeM = 1;
			} else {
				pgtype = "MULTIPOLYGON";
				pgdims = 2;
			}
			break;
		case SHPT_MULTIPOINTM: // MultiPointM
			wkbtype = MULTIPOINTTYPE | WKBMOFFSET;
			if ( ! hwgeom ) {
				pgtype = "MULTIPOINTM";
				pgdims = 3;
				istypeM = 1;
			} else {
				pgtype = "MULTIPOINT";
				pgdims = 2;
			}
			break;
		case SHPT_POINTZ: // PointZ
			wkbtype = POINTTYPE | WKBMOFFSET | WKBZOFFSET;
			pgtype = "POINT";
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_ARCZ: // PolyLineZ
			pgtype = "MULTILINESTRING";
			wkbtype = MULTILINETYPE | WKBZOFFSET | WKBMOFFSET;
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_POLYGONZ: // MultiPolygonZ
			pgtype = "MULTIPOLYGON";
			wkbtype = MULTIPOLYGONTYPE | WKBZOFFSET | WKBMOFFSET;
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_MULTIPOINTZ: // MultiPointZ
			pgtype = "MULTIPOINT";
			wkbtype = MULTIPOINTTYPE | WKBZOFFSET | WKBMOFFSET;
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
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
	char *buf = malloc(256*ring->n);
	int i;

	buf[0] = '\0';
	for (i=0; i<ring->n; i++)
	{
		if (i) strcat(buf, ",");
		sprintf(buf+strlen(buf), "%g %g",
			ring->list[i].x,
			ring->list[i].y);
	}
	return buf;
}

//--------------- WKB handling 

static char outchr[]={"0123456789ABCDEF"};

static int endian_check_int = 1; // dont modify this!!!

static char
getEndianByte(void)
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

void
DropTable(char *schema, char *table, char *geom)
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

void
GetFieldsSpec(void)
{
/*
 * Shapefile (dbf) field name are at most 10chars + 1 NULL.
 * Postgresql field names are at most 63 bytes + 1 NULL.
 */
#define MAXFIELDNAMELEN 64
	int field_precision, field_width;
	int j, z;
	char  name[MAXFIELDNAMELEN];
	char  name2[MAXFIELDNAMELEN];
	DBFFieldType type = -1;
#ifdef USE_ICONV
	char *utf8str;
#endif

	num_fields = DBFGetFieldCount( hDBFHandle );
	num_records = DBFGetRecordCount(hDBFHandle);
	field_names = malloc(num_fields*sizeof(char*));
	types = (DBFFieldType *)malloc(num_fields*sizeof(int));
	widths = malloc(num_fields*sizeof(int));
	precisions = malloc(num_fields*sizeof(int));
	col_names = malloc((num_fields+2) * sizeof(char) * MAXFIELDNAMELEN);
	strcpy(col_names, "(" );

	//fprintf(stderr, "Number of fields from DBF: %d\n", num_fields);
	for(j=0;j<num_fields;j++)
	{
		type = DBFGetFieldInfo(hDBFHandle, j, name, &field_width, &field_precision); 

//fprintf(stderr, "Field %d (%s) width/decimals: %d/%d\n", j, name, field_width, field_precision);
		types[j] = type;
		widths[j] = field_width;
		precisions[j] = field_precision;

#ifdef USE_ICONV
		if ( encoding )
		{
			utf8str = utf8(encoding, name);
			if ( ! utf8str ) exit(1);
			strcpy(name, utf8str);
			free(utf8str);
		}
#endif


		/*
		 * Make field names lowercase unless asked to
		 * keep identifiers case.
		 */
		if ( ! quoteidentifiers ) LowerCase(name);

		/*
		 * Escape names starting with the
		 * escape char (_), those named 'gid'
		 * or after pgsql reserved attribute names
		 */
		if( name[0]=='_' ||
			! strcmp(name,"gid") ||
			! strcmp(name, "tableoid") ||
			! strcmp(name, "cmax") ||
			! strcmp(name, "xmax") ||
			! strcmp(name, "cmin") ||
			! strcmp(name, "primary") ||
			! strcmp(name, "oid") ||
			! strcmp(name, "ctid") )
		{
			strcpy(name2+2, name);
			name2[0] = '_';
			name2[1] = '_';
			strcpy(name, name2);
		}

		/* Avoid duplicating field names */
		for(z=0; z < j ; z++){
			if(strcmp(field_names[z],name)==0){
				strcat(name,"__");
				sprintf(name+strlen(name),"%i",j);
				break;
			}
		}	

		field_names[j] = malloc (strlen(name)+1);
		strcpy(field_names[j], name);

		/*sprintf(col_names, "%s\"%s\",", col_names, name);*/
		strcat(col_names, "\"");
		strcat(col_names, name);
		strcat(col_names, "\",");
	}
	/*sprintf(col_names, "%s\"%s\")", col_names, geom);*/
	strcat(col_names, geom);
	strcat(col_names, ")");
}

#ifdef USE_ICONV

#include <iconv.h>

char *
utf8 (const char *fromcode, char *inputbuf)
{
	iconv_t cd;
	char *outputptr;
	char *outputbuf;
	size_t outbytesleft;
	size_t inbytesleft;

	inbytesleft = strlen (inputbuf);

	cd = iconv_open ("UTF-8", fromcode);
	if (cd == (iconv_t) - 1)
	{
		fprintf(stderr, "utf8: iconv_open: %s\n", strerror (errno));
		return NULL;
	}

	outbytesleft = inbytesleft*3+1; // UTF8 string can be 3 times larger
					// then local string
	outputbuf = (char *) malloc (outbytesleft);
	if (!outputbuf)
	{
		fprintf(stderr, "utf8: malloc: %s\n", strerror (errno));
		return NULL;
	}
	bzero (outputbuf, outbytesleft);
	outputptr = outputbuf;

	if (-1==iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft))
	{
		fprintf(stderr, "utf8: %s", strerror (errno));
		return NULL;
	}

	iconv_close (cd);

	return outputbuf;
}

#endif // defined USE_ICONV

/**********************************************************************
 * $Log$
 * Revision 1.88.2.11  2005/10/24 11:31:27  strk
 * Backported stricter STRING and INTEGER attributes handling.
 *
 * Revision 1.99  2005/10/03 18:08:55  strk
 * Stricter string attributes lenght handling. DBF header will be used
 * to set varchar maxlenght, (var)char typmod will be used to set DBF header
 * len.
 *
 * Revision 1.88.2.10  2005/10/21 11:34:25  strk
 * Applied patch by Lars Roessiger handling numerical values with a trailing decima
 * l dot
 *
 * Revision 1.88.2.9  2005/10/13 13:40:15  strk
 * Fixed return code from shp2pgsql
 *
 * Revision 1.88.2.8  2005/09/30 08:59:21  strk
 * Fixed release of stack memory occurring when shp2pgsql is compiled with USE_ICONV defined, an attribute value needs to be escaped and no -W is used
 *
 * Revision 1.88.2.7  2005/08/29 22:36:14  strk
 * Removed premature object destruction in InsertLineString{WKT,} causing segfault
 *
 * Revision 1.88.2.6  2005/08/29 11:48:42  strk
 * Fixed sprintf() calls to avoid overlapping memory,
 * reworked not-null objects existance check to reduce startup costs.
 *
 * Revision 1.88.2.5  2005/07/27 02:47:06  strk
 * Support for multibyte field names in loader
 *
 * Revision 1.88.2.4  2005/07/27 02:34:29  strk
 * Minor cleanups in loader
 *
 * Revision 1.88.2.3  2005/07/27 02:05:20  strk
 * Fixed handling of POINT types as WKT (-w) in loader
 *
 * Revision 1.88.2.2  2005/07/01 14:22:40  strk
 * backported -I switch
 *
 * Revision 1.88.2.1  2005/04/21 09:07:41  strk
 * Applied patch from Ron Mayer fixing a segfault in string escaper funx
 *
 * Revision 1.88  2005/04/14 12:58:59  strk
 * Applied patch by Gino Lucrezi fixing bug in string escaping code.
 *
 * Revision 1.87  2005/04/06 14:16:43  strk
 * Removed manual update of gid field.
 *
 * Revision 1.86  2005/04/06 14:02:08  mschaber
 * added -p option (prepare mode) that spits out the table schema without
 * inserting any data.
 *
 * Revision 1.85  2005/04/06 10:46:10  strk
 * Bugfix in -w (hwgeom) handling of ZM shapefiles.
 * Big reorganizzation of code to easy maintainance.
 *
 * Revision 1.84  2005/04/04 20:51:26  strk
 * Added -w flag to output old (WKT/HWGEOM) sql.
 *
 * Revision 1.83  2005/03/15 12:24:40  strk
 * hole-in-ring detector made more readable
 *
 * Revision 1.82  2005/03/14 22:02:31  strk
 * Fixed holes handling.
 *
 * Revision 1.81  2005/03/08 11:06:33  strk
 * modernized old-style parameter declarations
 *
 * Revision 1.80  2005/03/04 14:48:22  strk
 * Applied patch from Jonne Savolainen fixing multilines handling
 *
 * Revision 1.79  2005/01/31 22:15:22  strk
 * Added maintainer notice, to reduce Jeff-strk mail bounces
 *
 * Revision 1.78  2005/01/17 09:21:13  strk
 * Added one more bytes for terminating NULL in utf8 encoder
 *
 * Revision 1.77  2005/01/16 16:50:01  strk
 * String escaping algorithm made simpler and more robust.
 * Removed escaped strings leaking.
 * Fixed UTF8 encoder to allocate enough space for 3bytes chars strings.
 *
 * Revision 1.76  2005/01/12 17:03:20  strk
 * Added optional UTF8 output support as suggested by IIDA Tetsushi
 *
 * Revision 1.75  2004/11/15 10:51:35  strk
 * Fixed a bug in PIP invocation, added some debugging lines.
 *
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
