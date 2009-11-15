/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 */

 /** *********************************************************************
 * \file Using shapelib 1.2.8, this program reads in shape files and
 * 	processes it's contents into a Insert statements which can be
 * 	easily piped into a database frontend.
 * 	Specifically designed to insert type 'geometry' (a custom
 * 	written PostgreSQL type) for the shape files and PostgreSQL
 * 	standard types for all attributes of the entity.
 *
 * 	Original Author: Jeff Lounsbury, jeffloun@refractions.net
 *
 * 	Maintainer: Sandro Santilli, strk@refractions.net
 *
 **********************************************************************/

#include "../postgis_config.h"
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
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include "../liblwgeom/liblwgeom.h"


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
	Point *list;		/* list of points */
	struct Ring  *next;
	int n;			/* number of points in list */
	unsigned int linked; 	/* number of "next" rings */
} Ring;

/** @brief Values for null_policy global */
enum {
	insert_null,
	skip_null,
	abort_on_null
};

/* globals */
int	dump_format = 0; /* 0=insert statements, 1 = dump */
int     simple_geometries = 0; /* 0 = MULTILINESTRING/MULTIPOLYGON, 1 = LINESTRING/POLYGON */
int	quoteidentifiers = 0;
int	forceint4 = 0;
int	createindex = 0;
int readshape = 1;
char    opt = ' ';
char    *col_names = NULL;
char	*pgtype;
int	istypeM = 0;
int	pgdims;
unsigned int wkbtype;
char  	*shp_file = NULL;
int	hwgeom = 0; /* old (hwgeom) mode */
#ifdef HAVE_ICONV
char	*encoding=NULL;
#endif
int null_policy = insert_null;

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
int 	sr_id = 0;

/* Prototypes */
int Insert_attributes(DBFHandle hDBFHandle, int row);
char *escape_copy_string(char *str);
char *escape_insert_string(char *str);
int PIP( Point P, Point* V, int n );
void *safe_malloc(size_t size);
void CreateTable(void);
void CreateIndex(void);
void usage(char *me, int exitcode, FILE* out);
void InsertPoint(void);
void InsertMultiPoint(void);
void InsertPolygon(void);
void InsertLineString(void);
void OutputGeometry(char *geometry);
int ParseCmdline(int ARGC, char **ARGV);
void SetPgType(void);
char *dump_ring(Ring *ring);
#ifdef HAVE_ICONV
char *utf8(const char *fromcode, char *inputbuf);
#endif
int FindPolygons(SHPObject *obj, Ring ***Out);
void ReleasePolygons(Ring **polys, int npolys);
void DropTable(char *schema, char *table, char *geom);
void GetFieldsSpec(void);
void LoadData(void);
void OpenShape(void);
void LowerCase(char *s);
void Cleanup(void);

static char rcsid[] =
  "$Id$";


/* liblwgeom allocator callback - install the defaults (malloc/free/stdout/stderr) */
void lwgeom_init_allocators()
{
	lwgeom_install_default_allocators();
}


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


/**
 * @brief Escape input string suitable for COPY
 */

char *
escape_copy_string(char *str)
{
	/*
	 * Escape the following characters by adding a preceding backslash
	 *      tab, backslash, cr, lf
	 *
	 * 1. find # of escaped characters
	 * 2. make new string
	 *
	 */

	char *result;
	char *ptr, *optr;
	int toescape = 0;
	size_t size;
#ifdef HAVE_ICONV
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
		if ( *ptr == '\t' || *ptr == '\\' ||
			*ptr == '\n' || *ptr == '\r' )
				toescape++;
		ptr++;
	}

	if (toescape == 0) return str;

	size = ptr-str+toescape+1;

	result = calloc(1, size);

	optr=result;
	ptr=str;
	while (*ptr) {
		if ( *ptr == '\t' || *ptr == '\\' ||
			*ptr == '\n' || *ptr == '\r' )
				*optr++='\\';
				*optr++=*ptr++;
	}
	*optr='\0';

#ifdef HAVE_ICONV
	if ( encoding ) free(str);
#endif

	return result;

}

char *
escape_insert_string(char *str)
{
	/*
	 * Escape single quotes by adding a preceding single quote
	 *
	 * 1. find # of characters
	 * 2. make new string
	 */

	char	*result;
	char	*ptr, *optr;
	int	toescape = 0;
	size_t size;
#ifdef HAVE_ICONV
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
		if ( *ptr == '\'' ) toescape++;
		ptr++;
	}

	if (toescape == 0) return str;

	size = ptr-str+toescape+1;

	result = calloc(1, size);

	optr=result;
	ptr=str;
	while (*ptr) {
				if ( *ptr == '\'') *optr++='\'';
		*optr++=*ptr++;
	}
	*optr='\0';

#ifdef HAVE_ICONV
	if ( encoding ) free(str);
#endif

	return result;
}



/**
 * @brief PIP(): crossing number test for a point in a polygon
 *      input:   P = a point,
 *               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
 * @return   0 = outside, 1 = inside
 */
int
PIP( Point P, Point* V, int n )
{
	int cn = 0;    /* the crossing number counter */
	int i;

	/* loop through all edges of the polygon */
	for (i=0; i<n-1; i++) {    /* edge from V[i] to V[i+1] */
	   if (((V[i].y <= P.y) && (V[i+1].y > P.y))    /* an upward crossing */
		|| ((V[i].y > P.y) && (V[i+1].y <= P.y))) { /* a downward crossing */
			double vt = (float)(P.y - V[i].y) / (V[i+1].y - V[i].y);
			if (P.x < V[i].x + vt * (V[i+1].x - V[i].x)) /* P.x < intersect */
				++cn;   /* a valid crossing of y=P.y right of P.x */
		}
	}
	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */

}


/**
 * @brief Insert the attributes from the correct row of dbf file
 */

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
			//printf("\t");
		}
		else
		{
			printf("NULL");
			//printf(",");
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
				  fprintf(stderr, "Warning: field %d name truncated\n", i);
				  val[1023] = '\0';
			   }
		   /* pg_atoi() does not do this */
		   if ( val[0] == '\0' ) { val[0] = '0'; val[1] = '\0'; }
		   if ( val[strlen(val)-1] == '.' ) val[strlen(val)-1] = '\0';
			   break;

			case FTString:
			case FTLogical:
		case FTDate:
			   if ( -1 == snprintf(val, 1024, "%s",
					 DBFReadStringAttribute(hDBFHandle, row, i)) )
			   {
				  fprintf(stderr, "Warning: field %d name truncated\n", i);
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
				escval = escape_copy_string(val);
				printf("%s", escval);
				//printf("\t");
			} else {
				escval = escape_insert_string(val);
				printf("'%s'", escval);
				//printf(",");
			}
			if ( val != escval ) free(escval);
		 }
		//only put in delimeter if not last field or a shape will follow
		if(readshape == 1 || i < (num_fields - 1))
		{
			if (dump_format){
			   printf("\t");
			}
			else {
				printf(",");
			}
		}
	}
	return 1;
}




/**
 * main()
 * @brief see description at the top of this file
 */
int
main (int ARGC, char **ARGV)
{

	/*
	 * Parse command line
	 */
	if ( ! ParseCmdline(ARGC, ARGV) ) usage(ARGV[0], 2, stderr);

	/*
	 * Open shapefile and initialize globals
	 */
	OpenShape();

	if (readshape == 1){
		/*
		 * Compute output geometry type
		 */

		SetPgType();

		fprintf(stderr, "Shapefile type: %s\n", SHPTypeName(shpfiletype));
		fprintf(stderr, "Postgis type: %s[%d]\n", pgtype, pgdims);
	}

#ifdef HAVE_ICONV
	if ( encoding )
	{
		printf("SET CLIENT_ENCODING TO UTF8;\n");
	}
#endif /* defined HAVE_ICONV */

	/*
	 * Use SQL-standard string escaping rather than PostgreSQL standard
	 */
	printf("SET STANDARD_CONFORMING_STRINGS TO ON;\n");

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
	 * Generate INSERT or COPY lines
	 */
	if(opt != 'p') LoadData();

	/*
	 * Create GiST index if requested
	 */
	if(createindex) CreateIndex();

	printf("END;\n"); /* End the last transaction */


	return 0;
}/*end main() */

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

	if (readshape == 1)
	{
		hSHPHandle = SHPOpen( shp_file, "rb" );
		if (hSHPHandle == NULL)
		{
			fprintf(stderr, "%s: shape (.shp) or index files (.shx) can not be opened, will just import attribute data.\n", shp_file);
			readshape = 0;
		}
	}

	hDBFHandle = DBFOpen( shp_file, "rb" );
	if ((hSHPHandle == NULL && readshape == 1) || hDBFHandle == NULL){
		fprintf(stderr, "%s: dbf file (.dbf) can not be opened.\n",shp_file);
		exit(-1);
	}

	if (readshape == 1)
	{
		SHPGetInfo(hSHPHandle, &num_entities, &shpfiletype, NULL, NULL);

		if ( null_policy == abort_on_null )
		{
			for (j=0; j<num_entities; j++)
			{
				obj = SHPReadObject(hSHPHandle,j);
				if ( ! obj )
				{
					fprintf(stderr, "Error reading shape object %d\n", j);
					exit(1);
				}
				if ( obj->nVertices == 0 )
				{
					fprintf(stderr, "Empty geometries found, aborted.\n");
					exit(1);
				}
				SHPDestroyObject(obj);
			}
		}
	}
	else {
		num_entities = DBFGetRecordCount(hDBFHandle);
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

		if(type == FTString)
		{
			/* use DBF attribute size as maximum width */
			printf ("varchar(%d)", field_width);
		}
		else if (type == FTDate)
		{
			printf ("date");
		}
		else if (type == FTInteger)
		{
			if ( forceint4 )
			{
				printf ("int4");
			}
			else if  ( field_width < 5 )
			{
				printf ("int2");
			}
			else if  ( field_width < 10 )
			{
				printf ("int4");
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
	printf (");\n");

	/* Create the geometry column with an addgeometry call */
	if ( schema && readshape == 1 )
	{
		printf("SELECT AddGeometryColumn('%s','%s','%s','%d',",
			schema, table, geom, sr_id);
	}
	else if (readshape == 1)
	{
		printf("SELECT AddGeometryColumn('','%s','%s','%d',",
			table, geom, sr_id);
	}
	if (pgtype)
	{ //pgtype will only be set if we are loading geometries
		printf("'%s',%d);\n", pgtype, pgdims);
	}
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
		/*wrap a transaction block around each 250 inserts... */
		if ( ! dump_format )
		{
			if (trans == 250) {
				trans=0;
				printf("END;\n");
				printf("BEGIN;\n");
			}
		}
		trans++;
		/* transaction stuff done */

		/* skip the record if it has been deleted */
		if(readshape != 1 && DBFReadDeleted(hDBFHandle, j)) {
			continue;
		}

		/* open the next object */
		if (readshape == 1)
		{
			obj = SHPReadObject(hSHPHandle,j);
			if ( ! obj )
			{
				fprintf(stderr, "Error reading shape object %d\n", j);
				exit(1);
			}

			if ( null_policy == skip_null && obj->nVertices == 0 )
			{
				SHPDestroyObject(obj);
				continue;
			}
		}

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

		if (readshape == 1)
		{
			/* ---------- NULL SHAPE ----------------- */
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
				case SHPT_MULTIPOINT:
				case SHPT_MULTIPOINTM:
				case SHPT_MULTIPOINTZ:
					InsertPoint();
					break;

				case SHPT_ARC:
				case SHPT_ARCM:
				case SHPT_ARCZ:
					InsertLineString();
					break;

				default:
					printf ("\n\n**** Type is NOT SUPPORTED, type id = %d ****\n\n",
						obj->nSHPType);
					break;

			}

			SHPDestroyObject(obj);
		}
		else
			if (dump_format){ /*close for dbf only dump format */
				printf("\n");
			}
			else { /*close for dbf only sql insert format */
				printf(");\n");
			}

	} /* END of MAIN SHAPE OBJECT LOOP */



	if ((dump_format) ) {
		printf("\\.\n");

	}
}

void
usage(char *me, int exitcode, FILE* out)
{
	fprintf(out, "RCSID: %s RELEASE: %s\n", rcsid, POSTGIS_VERSION);
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
	fprintf(out, "  -D  Use postgresql dump format (defaults to sql insert statments).\n");
	fprintf(out, "  -k  Keep postgresql identifiers case.\n");
	fprintf(out, "  -i  Use int4 type for all integer dbf fields.\n");
	fprintf(out, "  -I  Create a GiST index on the geometry column.\n");
	fprintf(out, "  -S  Generate simple geometries instead of MULTI geometries.\n");
	fprintf(out, "  -w  Use wkt format (for postgis-0.x support - drops M - drifts coordinates).\n");
#ifdef HAVE_ICONV
	fprintf(out, "  -W <encoding> Specify the character encoding of Shape's\n");
	fprintf(out, "     attribute column. (default : \"ASCII\")\n");
#endif
	fprintf(out, "  -N <policy> Specify NULL geometries handling policy (insert,skip,abort).\n");
	fprintf(out, "     Default: insert.\n");
	fprintf(out, "  -n  Only import DBF file.\n");
	fprintf(out, "  -? Display this help screen\n");
	exit (exitcode);
}

void
InsertLineString()
{
	LWCOLLECTION *lwcollection;

	LWGEOM **lwmultilinestrings;
	uchar *serialized_lwgeom;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;

	DYNPTARRAY **dpas;
	POINT4D point4d;

	int dims = 0, hasz = 0, hasm = 0;
	int result;
	int u, v, start_vertex, end_vertex;

	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (wkbtype & WKBZOFFSET) hasz = 1;
	if (!hwgeom)
		if (wkbtype & WKBMOFFSET) hasm = 1;
	TYPE_SETZM(dims, hasz, hasm);

	if (simple_geometries == 1 && obj->nParts > 1)
	{
		fprintf(stderr, "We have a Multilinestring with %d parts, can't use -S switch!\n", obj->nParts);
		exit(1);
	}

	/* Allocate memory for our array of LWLINEs and our dynptarrays */
	lwmultilinestrings = malloc(sizeof(LWPOINT *) * obj->nParts);
	dpas = malloc(sizeof(DYNPTARRAY *) * obj->nParts);

	/* We need an array of pointers to each of our sub-geometries */
	for (u = 0; u < obj->nParts; u++)
	{
		/* Create a dynptarray containing the line points */
		dpas[u] = dynptarray_create(obj->nParts, dims);

		/* Set the start/end vertices depending upon whether this is
		a MULTILINESTRING or not */
		if ( u == obj->nParts-1 )
			end_vertex = obj->nVertices;
		else
			end_vertex = obj->panPartStart[u + 1];

		start_vertex = obj->panPartStart[u];

		for (v = start_vertex; v < end_vertex; v++)
		{
			/* Generate the point */
			point4d.x = obj->padfX[v];
			point4d.y = obj->padfY[v];

			if (wkbtype & WKBZOFFSET)
				point4d.z = obj->padfZ[v];
			if (wkbtype & WKBMOFFSET)
				point4d.m = obj->padfM[v];

			dynptarray_addPoint4d(dpas[u], &point4d, 0);
		}

		/* Generate the LWLINE */
		lwmultilinestrings[u] = lwline_as_lwgeom(lwline_construct(sr_id, NULL, dpas[u]->pa));
	}

	/* If using MULTILINESTRINGs then generate the serialized collection, otherwise just a single LINESTRING */
	if (simple_geometries == 0)
	{
		lwcollection = lwcollection_construct(MULTILINETYPE, sr_id, NULL, obj->nParts, lwmultilinestrings);
		serialized_lwgeom = lwgeom_serialize(lwcollection_as_lwgeom(lwcollection));
	}
	else
	{
		serialized_lwgeom = lwgeom_serialize(lwmultilinestrings[0]);
	}

	if (!hwgeom)
		result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);
	else
		result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE);

	if (result)
	{
		fprintf(stderr, "ERROR: %s\n", lwg_unparser_result.message);
		exit(1);
	}

	OutputGeometry(lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
		lwfree(lwg_unparser_result.wkoutput);
		lwfree(serialized_lwgeom);

	for (u = 0; u < obj->nParts; u++)
	{
			lwline_free(lwgeom_as_lwline(lwmultilinestrings[u]));
			lwfree(dpas[u]);
	}

	lwfree(dpas);
	lwfree(lwmultilinestrings);
}

int
FindPolygons(SHPObject *obj, Ring ***Out)
{
	Ring **Outer;    /* Pointers to Outer rings */
	int out_index=0; /* Count of Outer rings */
	Ring **Inner;    /* Pointers to Inner rings */
	int in_index=0;  /* Count of Inner rings */
	int pi; /* part index */

#if POSTGIS_DEBUG_LEVEL > 0
	static int call = -1;
	call++;
#endif

	LWDEBUGF(4, "FindPolygons[%d]: allocated space for %d rings\n", call, obj->nParts);

	/* Allocate initial memory */
	Outer = (Ring**)malloc(sizeof(Ring*)*obj->nParts);
	Inner = (Ring**)malloc(sizeof(Ring*)*obj->nParts);

	/* Iterate over rings dividing in Outers and Inners */
	for (pi=0; pi<obj->nParts; pi++)
	{
		int vi; /* vertex index */
		int vs; /* start index */
		int ve; /* end index */
		int nv; /* number of vertex */
		double area = 0.0;
		Ring *ring;

		/* Set start and end vertexes */
		if ( pi==obj->nParts-1 ) ve = obj->nVertices;
		else ve = obj->panPartStart[pi+1];
		vs = obj->panPartStart[pi];

		/* Compute number of vertexes */
		nv = ve-vs;

		/* Allocate memory for a ring */
		ring = (Ring*)malloc(sizeof(Ring));
		ring->list = (Point*)malloc(sizeof(Point)*nv);
		ring->n = nv;
		ring->next = NULL;
		ring->linked = 0;

		/* Iterate over ring vertexes */
		for ( vi=vs; vi<ve; vi++)
		{
			int vn = vi+1; /* next vertex for area */
			if ( vn==ve ) vn = vs;

			ring->list[vi-vs].x = obj->padfX[vi];
			ring->list[vi-vs].y = obj->padfY[vi];
			ring->list[vi-vs].z = obj->padfZ[vi];
			ring->list[vi-vs].m = obj->padfM[vi];

			area += (obj->padfX[vi] * obj->padfY[vn]) -
				(obj->padfY[vi] * obj->padfX[vn]);
		}

		/* Close the ring with first vertex  */
		/*ring->list[vi].x = obj->padfX[vs]; */
		/*ring->list[vi].y = obj->padfY[vs]; */
		/*ring->list[vi].z = obj->padfZ[vs]; */
		/*ring->list[vi].m = obj->padfM[vs]; */

		/* Clockwise (or single-part). It's an Outer Ring ! */
		if(area < 0.0 || obj->nParts ==1) {
			Outer[out_index] = ring;
			out_index++;
		}

		/* Counterclockwise. It's an Inner Ring ! */
		else {
			Inner[in_index] = ring;
			in_index++;
		}
	}

	LWDEBUGF(4, "FindPolygons[%d]: found %d Outer, %d Inners\n", call, out_index, in_index);

	/* Put the inner rings into the list of the outer rings */
	/* of which they are within */
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
			/*fprintf(stderr, "!PIP %s\nOUTE %s\n", dump_ring(inner), dump_ring(Outer[i])); */
		}

		if ( outer )
		{
			outer->linked++;
			while(outer->next) outer = outer->next;
			outer->next = inner;
		}
		else
		{
			/* The ring wasn't within any outer rings, */
			/* assume it is a new outer ring. */
			LWDEBUGF(4, "FindPolygons[%d]: hole %d is orphan\n", call, pi);

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
	/* Release all memory */
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
	free(polys);
}

/** @brief This function basically deals with the polygon case. */
/*		it sorts the polys in order of outer,inner,inner, so that inners */
/*		always come after outers they are within  */
void
InsertPolygon(void)
{
	Ring **Outer;
	int polygon_total, ring_total;
	int pi, vi; // part index and vertex index
	int u;

	LWCOLLECTION *lwcollection = NULL;

	LWGEOM **lwpolygons;
	uchar *serialized_lwgeom;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;

	LWPOLY *lwpoly;
	DYNPTARRAY *dpas;
	POINTARRAY ***pas;
	POINT4D point4d;

	int dims = 0, hasz = 0, hasm = 0;
	int result;

	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (wkbtype & WKBZOFFSET) hasz = 1;
	if (!hwgeom)
		if (wkbtype & WKBMOFFSET) hasm = 1;
	TYPE_SETZM(dims, hasz, hasm);

	polygon_total = FindPolygons(obj, &Outer);

	if (simple_geometries == 1 && polygon_total != 1) /* We write Non-MULTI geometries, but have several parts: */
	{
		fprintf(stderr, "We have a Multipolygon with %d parts, can't use -S switch!\n", polygon_total);
		exit(1);
	}

	/* Allocate memory for our array of LWPOLYs */
	lwpolygons = malloc(sizeof(LWPOLY *) * polygon_total);

	/* Allocate memory for our POINTARRAY pointers for each polygon */
	pas = malloc(sizeof(POINTARRAY **) * polygon_total);

	/* Cycle through each individual polygon */
	for(pi = 0; pi < polygon_total; pi++)
	{
		Ring *polyring;
		int ring_index = 0;

		/* Firstly count through the total number of rings in this polygon */
		ring_total = 0;
		polyring = Outer[pi];
		while (polyring)
		{
			ring_total++;
			polyring = polyring->next;
		}

		/* Reserve memory for the POINTARRAYs representing each ring */
		pas[pi] = malloc(sizeof(POINTARRAY *) * ring_total);

		/* Cycle through each ring within the polygon, starting with the outer */
		polyring = Outer[pi];

		while (polyring)
		{
			/* Create a DYNPTARRAY containing the points making up the ring */
			dpas = dynptarray_create(polyring->n, dims);

			for(vi = 0; vi < polyring->n; vi++)
			{
				/* Build up a point array of all the points in this ring */
				point4d.x = polyring->list[vi].x;
				point4d.y = polyring->list[vi].y;

				if (wkbtype & WKBZOFFSET)
					point4d.z = polyring->list[vi].z;
				if (wkbtype & WKBMOFFSET)
					point4d.m = polyring->list[vi].m;

				dynptarray_addPoint4d(dpas, &point4d, 0);
			}

			/* Copy the POINTARRAY pointer from the DYNPTARRAY structure so we can
			 use the LWPOLY constructor */
			pas[pi][ring_index] = dpas->pa;

			/* Free the DYNPTARRAY structure (we don't need this part anymore as we
			have the reference to the internal POINTARRAY) */
			lwfree(dpas);

			polyring = polyring->next;
			ring_index++;
		}

		/* Generate the LWGEOM */
		lwpoly = lwpoly_construct(sr_id, NULL, ring_total, pas[pi]);
		lwpolygons[pi] = lwpoly_as_lwgeom(lwpoly);
	}

	ReleasePolygons(Outer, polygon_total);

	/* If using MULTIPOLYGONS then generate the serialized collection, otherwise just a single POLYGON */
	if (simple_geometries == 0)
	{
		lwcollection = lwcollection_construct(MULTIPOLYGONTYPE, sr_id, NULL, polygon_total, lwpolygons);
		serialized_lwgeom = lwgeom_serialize(lwcollection_as_lwgeom(lwcollection));
	}
	else
	{
		serialized_lwgeom = lwgeom_serialize(lwpolygons[0]);
	}

	if (!hwgeom)
		result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);
	else
		result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE);

	if (result)
	{
		fprintf(stderr, "ERROR: %s\n", lwg_unparser_result.message);
		exit(1);
	}

	OutputGeometry(lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
		lwfree(lwg_unparser_result.wkoutput);
		lwfree(serialized_lwgeom);

	/* Cycle through each polygon, freeing everything we need... */
	for (u = 0; u < polygon_total; u++)
		lwpoly_free(lwgeom_as_lwpoly(lwpolygons[u]));

	/* Free the pointer arrays */
	lwfree(pas);
	lwfree(lwpolygons);
	if (simple_geometries == 0)
		lwfree(lwcollection);
}

/**
 * @brief Insert either a POINT or MULTIPOINT into the output stream
 */
void
InsertPoint(void)
{
	LWCOLLECTION *lwcollection;

	LWGEOM **lwmultipoints;
	uchar *serialized_lwgeom;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;

	DYNPTARRAY **dpas;
	POINT4D point4d;

	int dims = 0, hasz = 0, hasm = 0;
	int result;
	int u;

	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (wkbtype & WKBZOFFSET) hasz = 1;
	if (!hwgeom)
		if (wkbtype & WKBMOFFSET) hasm = 1;
	TYPE_SETZM(dims, hasz, hasm);

	/* Allocate memory for our array of LWPOINTs and our dynptarrays */
	lwmultipoints = malloc(sizeof(LWPOINT *) * obj->nVertices);
	dpas = malloc(sizeof(DYNPTARRAY *) * obj->nVertices);

	/* We need an array of pointers to each of our sub-geometries */
	for (u = 0; u < obj->nVertices; u++)
	{
		/* Generate the point */
		point4d.x = obj->padfX[u];
		point4d.y = obj->padfY[u];

		if (wkbtype & WKBZOFFSET)
			point4d.z = obj->padfZ[u];
		if (wkbtype & WKBMOFFSET)
			point4d.m = obj->padfM[u];

		/* Create a dynptarray containing a single point */
		dpas[u] = dynptarray_create(1, dims);
		dynptarray_addPoint4d(dpas[u], &point4d, 0);

		/* Generate the LWPOINT */
		lwmultipoints[u] = lwpoint_as_lwgeom(lwpoint_construct(sr_id, NULL, dpas[u]->pa));
	}

	/* If we have more than 1 vertex then we are working on a MULTIPOINT and so generate a MULTIPOINT
	rather than a POINT */
	if (obj->nVertices > 1)
	{
		lwcollection = lwcollection_construct(MULTIPOINTTYPE, sr_id, NULL, obj->nVertices, lwmultipoints);
		serialized_lwgeom = lwgeom_serialize(lwcollection_as_lwgeom(lwcollection));
	}
	else
	{
		serialized_lwgeom = lwgeom_serialize(lwmultipoints[0]);
	}

	if (!hwgeom)
		result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);
	else
		result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE);

	if (result)
	{
		fprintf(stderr, "ERROR: %s\n", lwg_unparser_result.message);
		exit(1);
	}

	OutputGeometry(lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
		lwfree(lwg_unparser_result.wkoutput);
		lwfree(serialized_lwgeom);

	for (u = 0; u < obj->nVertices; u++)
	{
			lwpoint_free(lwgeom_as_lwpoint(lwmultipoints[u]));
			lwfree(dpas[u]);
	}

	lwfree(dpas);
	lwfree(lwmultipoints);
}

void
OutputGeometry(char *geometry)
{
	/* This function outputs the specified geometry string (WKB or WKT) formatted
	 * according to whether we have specified dump format or hwgeom format */

	if (hwgeom)
	{
		if (!dump_format)
			printf("GeomFromText('");
		else
		{
			/* Output SRID if relevant */
			if (sr_id != 0)
				printf("SRID=%d;", sr_id);
		}

		printf("%s", geometry);

		if (!dump_format)
		{
			printf("'");

			/* Output SRID if relevant */
			if (sr_id != 0)
				printf(", %d)", sr_id);

			printf(");\n");
		}
		else
			printf("\n");
	}
	else
	{
		if (!dump_format)
			printf("'");

		printf("%s", geometry);

		if (!dump_format)
			printf("');\n");
		else
			printf("\n");
	}
}


int
ParseCmdline(int ARGC, char **ARGV)
{
	int c;
	int curindex=0;
	char  *ptr;
	extern char *optarg;
	extern int optind;

	if ( ARGC == 1 ) {
		usage(ARGV[0], 0, stdout);
	}

	while ((c = pgis_getopt(ARGC, ARGV, "kcdapDs:Sg:iW:wIN:n")) != EOF){
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
				(void)sscanf(optarg, "%d", &sr_id);
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
				usage(ARGV[0], 0, stdout);
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


void
SetPgType(void)
{
	switch(shpfiletype)
	{
		case SHPT_POINT: /* Point */
			pgtype = "POINT";
			wkbtype = POINTTYPE;
			pgdims = 2;
			break;
		case SHPT_ARC: /* PolyLine */
			pgtype = "MULTILINESTRING";
			wkbtype = MULTILINETYPE ;
			pgdims = 2;
			break;
		case SHPT_POLYGON: /* Polygon */
			pgtype = "MULTIPOLYGON";
			wkbtype = MULTIPOLYGONTYPE;
			pgdims = 2;
			break;
		case SHPT_MULTIPOINT: /* MultiPoint */
			pgtype = "MULTIPOINT";
			wkbtype = MULTIPOINTTYPE;
			pgdims = 2;
			break;
		case SHPT_POINTM: /* PointM */
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
		case SHPT_ARCM: /* PolyLineM */
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
		case SHPT_POLYGONM: /* PolygonM */
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
		case SHPT_MULTIPOINTM: /* MultiPointM */
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
		case SHPT_POINTZ: /* PointZ */
			wkbtype = POINTTYPE | WKBMOFFSET | WKBZOFFSET;
			pgtype = "POINT";
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_ARCZ: /* PolyLineZ */
			pgtype = "MULTILINESTRING";
			wkbtype = MULTILINETYPE | WKBZOFFSET | WKBMOFFSET;
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_POLYGONZ: /* MultiPolygonZ */
			pgtype = "MULTIPOLYGON";
			wkbtype = MULTIPOLYGONTYPE | WKBZOFFSET | WKBMOFFSET;
			if ( ! hwgeom ) pgdims = 4;
			else pgdims = 3;
			break;
		case SHPT_MULTIPOINTZ: /* MultiPointZ */
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

		if (simple_geometries)
		{
				// adjust geometry name for CREATE TABLE by skipping MULTI
				if ((wkbtype & 0x7) == MULTIPOLYGONTYPE) pgtype += 5;
				if ((wkbtype & 0x7) == MULTILINETYPE) pgtype += 5;
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

void
DropTable(char *schema, char *table, char *geom)
{
		/**---------------Drop the table--------------------------
		 * TODO: \todo if the table has more then one geometry column
		 * 	the DROP TABLE call will leave spurious records in
		 * 	geometry_columns.
		 *
		 * If the geometry column in the table being dropped
		 * does not match 'the_geom' or the name specified with
		 * -g an error is returned by DropGeometryColumn.
		 *
		 * The table to be dropped might not exist.
		 *
		 */
		if ( schema )
		{
			if (readshape == 1){
				printf("SELECT DropGeometryColumn('%s','%s','%s');\n",
					schema, table, geom);
			}
			printf("DROP TABLE \"%s\".\"%s\";\n", schema, table);
		}
		else
		{
			if (readshape == 1){
				printf("SELECT DropGeometryColumn('','%s','%s');\n",
					table, geom);
			}
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
#ifdef HAVE_ICONV
	char *utf8str;
#endif

	num_fields = DBFGetFieldCount( hDBFHandle );
	num_records = DBFGetRecordCount(hDBFHandle);
	field_names = malloc(num_fields*sizeof(char*));
	types = (DBFFieldType *)malloc(num_fields*sizeof(int));
	widths = malloc(num_fields*sizeof(int));
	precisions = malloc(num_fields*sizeof(int));
	if (readshape == 1)
	{
		col_names = malloc((num_fields+2) * sizeof(char) * MAXFIELDNAMELEN);
	}
	{	//for dbf only, we do not need to allocate slot for the_geom
		col_names = malloc((num_fields+1) * sizeof(char) * MAXFIELDNAMELEN);
	}
	strcpy(col_names, "(" );

	/*fprintf(stderr, "Number of fields from DBF: %d\n", num_fields); */
	for(j=0;j<num_fields;j++)
	{
		type = DBFGetFieldInfo(hDBFHandle, j, name, &field_width, &field_precision);

/*fprintf(stderr, "Field %d (%s) width/decimals: %d/%d\n", j, name, field_width, field_precision); */
		types[j] = type;
		widths[j] = field_width;
		precisions[j] = field_precision;

#ifdef HAVE_ICONV
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
		if (readshape == 1 || j < (num_fields - 1)){
			//don't include last comma if its the last field and no geometry field will follow
			strcat(col_names, "\",");
		}
		else {
			strcat(col_names, "\"");
		}
	}
	/*sprintf(col_names, "%s\"%s\")", col_names, geom);*/
	if (readshape == 1){
		strcat(col_names, geom);
	}
	strcat(col_names, ")");
}

#ifdef HAVE_ICONV

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

	outbytesleft = inbytesleft*3+1; /* UTF8 string can be 3 times larger */
					/* then local string */
	outputbuf = (char *) malloc (outbytesleft);
	if (!outputbuf)
	{
		fprintf(stderr, "utf8: malloc: %s\n", strerror (errno));
		return NULL;
	}
	memset (outputbuf, 0, outbytesleft);
	outputptr = outputbuf;

	if (-1==iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft))
	{
		fprintf(stderr, "utf8: iconv: %s\n", strerror (errno));
		return NULL;
	}

	iconv_close (cd);

	return outputbuf;
}

#endif /* defined HAVE_ICONV */

/**********************************************************************
 * $Log$
 * Revision 1.109  2008/04/09 14:12:17  robe
 *         - Added support to load dbf-only files
 *
 * Revision 1.108  2006/06/16 14:12:17  strk
 *         - BUGFIX in pgsql2shp successful return code.
 *         - BUGFIX in shp2pgsql handling of MultiLine WKT.
 *
 * Revision 1.107  2006/04/18 09:16:26  strk
 * Substituted bzero() use with memset()
 *
 * Revision 1.106  2006/01/16 10:42:58  strk
 * Added support for Bool and Date DBF<=>PGIS mapping
 *
 * Revision 1.105  2006/01/09 16:40:16  strk
 * ISO C90 comments, signedness mismatch fixes
 *
 * Revision 1.104  2005/11/01 09:25:47  strk
 * Reworked NULL geometries handling code letting user specify policy (insert,skip,abort). Insert is the default.
 *
 * Revision 1.103  2005/10/24 15:54:22  strk
 * fixed wrong assumption about maximum size of integer attributes (width is maximum size of text representation)
 *
 * Revision 1.102  2005/10/24 11:30:59  strk
 *
 * Fixed a bug in string attributes handling truncating values of maximum
 * allowed length, curtesy of Lars Roessiger.
 * Reworked integer attributes handling to be stricter in dbf->sql mapping
 * and to allow for big int8 values in sql->dbf conversion
 *
 * Revision 1.101  2005/10/21 11:33:55  strk
 * Applied patch by Lars Roessiger handling numerical values with a trailing decima
 * l dot
 *
 * Revision 1.100  2005/10/13 13:40:20  strk
 * Fixed return code from shp2pgsql
 *
 * Revision 1.99  2005/10/03 18:08:55  strk
 * Stricter string attributes lenght handling. DBF header will be used
 * to set varchar maxlenght, (var)char typmod will be used to set DBF header
 * len.
 *
 * Revision 1.98  2005/10/03 07:45:58  strk
 * Issued a warning when -W is specified and no UTF8 support has been compiled in.
 *
 * Revision 1.97  2005/09/30 08:59:29  strk
 * Fixed release of stack memory occurring when shp2pgsql is compiled with USE_ICONV defined, an attribute value needs to be escaped and no -W is used
 *
 * Revision 1.96  2005/08/29 22:36:25  strk
 * Removed premature object destruction in InsertLineString{WKT,} causing segfault
 *
 * Revision 1.95  2005/08/29 11:48:33  strk
 * Fixed sprintf() calls to avoid overlapping memory,
 * reworked not-null objects existance check to reduce startup costs.
 *
 * Revision 1.94  2005/07/27 02:47:14  strk
 * Support for multibyte field names in loader
 *
 * Revision 1.93  2005/07/27 02:35:50  strk
 * Minor cleanups in loader
 *
 * Revision 1.92  2005/07/27 02:07:01  strk
 * Fixed handling of POINT types as WKT (-w) in loader
 *
 * Revision 1.91  2005/07/04 09:47:03  strk
 * Added conservative iconv detection code
 *
 * Revision 1.90  2005/06/16 17:55:58  strk
 * Added -I switch for GiST index creation in loader
 *
 * Revision 1.89  2005/04/21 09:08:34  strk
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
