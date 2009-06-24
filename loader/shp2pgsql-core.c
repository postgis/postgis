/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2008 OpenGeo.org
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * Using shapelib 1.2.8, this program reads in shape files and
 * processes it's contents into a Insert statements which can be
 * easily piped into a database frontend.
 * Specifically designed to insert type 'geometry' (a custom
 * written PostgreSQL type) for the shape files and PostgreSQL
 * standard types for all attributes of the entity.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 * Original Author: Jeff Lounsbury <jeffloun@refractions.net>
 *
 **********************************************************************/

#include "../postgis_config.h"
#include "shp2pgsql-core.h"

#define	POINTTYPE	1
#define	LINETYPE	2
#define	POLYGONTYPE	3
#define	MULTIPOINTTYPE	4
#define	MULTILINETYPE	5
#define	MULTIPOLYGONTYPE	6
#define	COLLECTIONTYPE	7

#define WKBZOFFSET 0x80000000
#define WKBMOFFSET 0x40000000


typedef struct
{
	double x, y, z, m;
}
Point;

typedef struct Ring
{
	Point *list;		/* list of points */
	struct Ring  *next;
	int n;			/* number of points in list */
	unsigned int linked; 	/* number of "next" rings */
}
Ring;


/* Public globals */
int dump_format = 0; /* 0 = SQL inserts, 1 = dump */
int simple_geometries = 0; /* 0 = MULTIPOLYGON/MULTILINESTRING, 1 = force to POLYGON/LINESTRING */
int quoteidentifiers = 0; /* 0 = columnname, 1 = "columnName" */
int forceint4 = 0; /* 0 = allow int8 fields, 1 = no int8 fields */
int createindex = 0; /* 0 = no index, 1 = create index after load */
int readshape = 1; /* 0 = load DBF file only, 1 = load everything */
char opt = ' '; /* load mode: c = create, d = delete, a = append, p = prepare */
char *table = NULL; /* table to load into */
char *schema = NULL; /* schema to load into */
char *geom = NULL; /* geometry column name to use */
#ifdef HAVE_ICONV
char *encoding = NULL; /* iconv encoding name */
#endif
int null_policy = insert_null; /* how to handle nulls */
int sr_id = 0; /* SRID specified */
char *shp_file = NULL; /* the shape file (without the .shp extension) */
int gui_mode = 0; /* 1 = GUI, 0 = commandline */
int translation_stage = TRANSLATION_IDLE;

/* Private globals */
stringbuffer_t *sb_row; /* stringbuffer to append results to */
char    *col_names = NULL;
char	*pgtype;
int	istypeM = 0;
int	pgdims;
unsigned int wkbtype;
DBFFieldType *types;	/* Fields type, width and precision */
SHPHandle  hSHPHandle;
DBFHandle  hDBFHandle;
int shpfiletype;
SHPObject  *obj=NULL;
int 	*widths;
int 	*precisions;
int     num_fields,num_records,num_entities;
int cur_entity = -1;
char    **field_names;


/* Prototypes */
int Insert_attributes(DBFHandle hDBFHandle, int row);
char *escape_copy_string(char *str);
char *escape_insert_string(char *str);
int PIP( Point P, Point* V, int n );
int CreateTable(void);
int CreateIndex(void);
void usage(char *me, int exitcode, FILE* out);
int InsertPoint(void);
int InsertPolygon(void);
int InsertLineString(void);
int OutputGeometry(char *geometry);
void SetPgType(void);
#ifdef HAVE_ICONV
char *utf8(const char *fromcode, char *inputbuf);
#endif
int FindPolygons(SHPObject *obj, Ring ***Out);
void ReleasePolygons(Ring **polys, int npolys);
int DropTable(char *schema, char *table, char *geom);
void GetFieldsSpec(void);
int LoadData(void);
int OpenShape(void);
void LowerCase(char *s);
void Cleanup(void);

static int
pgis_exec(const char *sql)
{
#ifdef PGUI
	return pgui_exec(sql);
#else
	printf("%s;\n", sql);
	return 1;
#endif
}

static int
pgis_copy_write(const char *line)
{
#ifdef PGUI
	return pgui_copy_write(line);
#else
	printf("%s", line);
	return 1;
#endif
}

static int
pgis_copy_start(const char *sql)
{
#ifdef PGUI
	return pgui_copy_start(sql);
#else
	printf("%s;\n", sql);
	return 1;
#endif
}

static int
pgis_copy_end(const int rollback)
{
#ifdef PGUI
	return pgui_copy_end(rollback);
#else
	printf("\\.\n");
	return 1;
#endif
}

static void
pgis_logf(const char *fmt, ... )
{
#ifndef PGUI
	char *msg;
#endif
	va_list ap;

	va_start(ap, fmt);

#ifdef PGUI
	pgui_log_va(fmt, ap);
#else
	if (!vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}
	fprintf(stderr, "%s\n", msg);
	free(msg);
#endif
	va_end(ap);
}

/* liblwgeom allocator callback - install the defaults (malloc/free/stdout/stderr) */
/* TODO hook lwnotice/lwerr up to the GUI */
void lwgeom_init_allocators()
{
	lwgeom_install_default_allocators();
}


/*
 * Escape input string suitable for COPY
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

	while (*ptr)
	{
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
	while (*ptr)
	{
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


/*
 * Escape input string suitable for INSERT
 */

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

	while (*ptr)
	{
		if ( *ptr == '\'' ) toescape++;
		ptr++;
	}

	if (toescape == 0) return str;

	size = ptr-str+toescape+1;

	result = calloc(1, size);

	optr=result;
	ptr=str;
	while (*ptr)
	{
		if ( *ptr == '\'') *optr++='\'';
		*optr++=*ptr++;
	}
	*optr='\0';

#ifdef HAVE_ICONV
	if ( encoding ) free(str);
#endif

	return result;
}



/*
 * PIP(): crossing number test for a point in a polygon
 *      input:   P = a point,
 *               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
 *      returns: 0 = outside, 1 = inside
 */
int
PIP( Point P, Point* V, int n )
{
	int cn = 0;    /* the crossing number counter */
	int i;

	/* loop through all edges of the polygon */
	for (i=0; i<n-1; i++)
	{    /* edge from V[i] to V[i+1] */
		if (((V[i].y <= P.y) && (V[i+1].y > P.y))    /* an upward crossing */
		        || ((V[i].y > P.y) && (V[i+1].y <= P.y)))
		{ /* a downward crossing */
			double vt = (float)(P.y - V[i].y) / (V[i+1].y - V[i].y);
			if (P.x < V[i].x + vt * (V[i+1].x - V[i].x)) /* P.x < intersect */
				++cn;   /* a valid crossing of y=P.y right of P.x */
		}
	}
	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */

}





int
Insert_attributes(DBFHandle hDBFHandle, int row)
{
	int i,num_fields;
	char val[1024];
	char *escval;

	num_fields = DBFGetFieldCount( hDBFHandle );
	for ( i = 0; i < num_fields; i++ )
	{
		if (DBFIsAttributeNULL( hDBFHandle, row, i))
		{
			if (dump_format)
			{
				stringbuffer_append(sb_row, "\\N");
			}
			else
			{
				stringbuffer_append(sb_row, "NULL");
			}
		}
		else /* Attribute NOT NULL */
		{
			switch (types[i])
			{
			case FTInteger:
			case FTDouble:
				if ( -1 == snprintf(val, 1024, "%s", DBFReadStringAttribute(hDBFHandle, row, i)) )
				{
					pgis_logf("Warning: field %d name truncated", i);
					val[1023] = '\0';
				}
				/* pg_atoi() does not do this */
				if ( val[0] == '\0' )
				{
					val[0] = '0';
					val[1] = '\0';
				}
				if ( val[strlen(val)-1] == '.' ) val[strlen(val)-1] = '\0';
				break;

			case FTString:
			case FTLogical:
			case FTDate:
				if ( -1 == snprintf(val, 1024, "%s", DBFReadStringAttribute(hDBFHandle, row, i)) )
				{
					pgis_logf("Warning: field %d name truncated", i);
					val[1023] = '\0';
				}
				break;

			default:
				pgis_logf(
				    "Error: field %d has invalid or unknown field type (%d)",
				    i, types[i]);
				return 0;
			}
			
			if (dump_format)
			{
				escval = escape_copy_string(val);
				stringbuffer_aprintf(sb_row, "%s", escval);
				//printf("\t");
			}
			else
			{
				escval = escape_insert_string(val);
				stringbuffer_aprintf(sb_row, "'%s'", escval);
				//printf(",");
			}
			if ( val != escval ) free(escval);
		}
		//only put in delimeter if not last field or a shape will follow
		if (readshape == 1 || i < (num_fields - 1))
		{
			if (dump_format)
			{
				stringbuffer_append_c(sb_row, '\t');
			}
			else
			{
				stringbuffer_append_c(sb_row, ',');
			}
		}
	}
	return 1;
}




/*
 * formerly main()
 */
int
translation_start ()
{

	sb_row = stringbuffer_create();

	/*
	 * Open shapefile and initialize globals
	 */
	if ( ! OpenShape() )
		return 0;

	if (readshape == 1)
	{
		/*
		 * Compute output geometry type
		 */

		SetPgType();

		pgis_logf("Shapefile type: %s", SHPTypeName(shpfiletype));
		pgis_logf("PostGIS type: %s [%dD]", pgtype, pgdims);
	}

#ifdef HAVE_ICONV
	if ( encoding )
	{
		if ( ! pgis_exec("SET CLIENT_ENCODING TO UTF8") ) return 0;
	}
#endif /* defined HAVE_ICONV */

	/*
	 * Use SQL-standard string escaping rather than PostgreSQL standard
	 */
	if ( ! pgis_exec("SET STANDARD_CONFORMING_STRINGS TO ON") ) return 0;

	/*
	 * Drop table if requested
	 */
	if (opt == 'd')
		if ( ! DropTable(schema, table, geom) )
			return 0;

	/*
	 * Get col names and types for table creation
	 * and data load.
	 */
	GetFieldsSpec();

	if ( ! pgis_exec("BEGIN") ) return 0;

	/*
	 * If not in 'append' mode create the spatial table
	 */
	if (opt != 'a')
		if ( ! CreateTable() )
			return 0;

	translation_stage = TRANSLATION_LOAD; /* done start */
	return 1;
}

int
translation_middle()
{
	/*
	 * Generate INSERT or COPY lines
	 */
	if (opt != 'p')
	{
		if ( ! LoadData() )
			return 0;
	}
	else
	{
		translation_stage = TRANSLATION_CLEANUP; /* done middle */
	}
	return 1;
}

int
translation_end()
{
	/*
	 * Create GiST index if requested
	 */
	if (createindex)
		if ( ! CreateIndex() )
			return 0;

	if ( ! pgis_exec("END") ) return 0; /* End the last transaction */

	translation_stage = TRANSLATION_DONE;

	return 1;
}

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

int
OpenShape(void)
{
	int j;
	SHPObject *obj=NULL;

	if (readshape == 1)
	{
		hSHPHandle = SHPOpen( shp_file, "rb" );
		if (hSHPHandle == NULL)
		{
			pgis_logf("%s: shape (.shp) or index files (.shx) can not be opened, will just import attribute data.", shp_file);
			readshape = 0;
		}
	}

	hDBFHandle = DBFOpen( shp_file, "rb" );
	if ((hSHPHandle == NULL && readshape == 1) || hDBFHandle == NULL)
	{
		pgis_logf("%s: dbf file (.dbf) can not be opened.",shp_file);
		return 0;
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
					pgis_logf("Error reading shape object %d", j);
					return 0;
				}
				if ( obj->nVertices == 0 )
				{
					pgis_logf("Empty geometries found, aborted.");
					return 0;
				}
				SHPDestroyObject(obj);
			}
		}
	}
	else
	{
		num_entities = DBFGetRecordCount(hDBFHandle);
	}

	return 1;
}

int
CreateTable(void)
{
	int j;
	int field_precision, field_width;
	DBFFieldType type = -1;

	/*
	 * Create a table for inserting the shapes into with appropriate
	 * columns and types
	 */

	stringbuffer_clear(sb_row);

	if ( schema )
	{
		stringbuffer_aprintf(sb_row, "CREATE TABLE \"%s\".\"%s\" (gid serial PRIMARY KEY", schema, table);
	}
	else
	{
		stringbuffer_aprintf(sb_row, "CREATE TABLE \"%s\" (gid serial PRIMARY KEY", table);
	}

	for (j=0;j<num_fields;j++)
	{
		type = types[j];
		field_width = widths[j];
		field_precision = precisions[j];

		stringbuffer_aprintf(sb_row, ",\n\"%s\" ", field_names[j]);

		if (type == FTString)
		{
			/* use DBF attribute size as maximum width */
			stringbuffer_aprintf (sb_row, "varchar(%d)", field_width);
		}
		else if (type == FTDate)
		{
			stringbuffer_append (sb_row, "date");
		}
		else if (type == FTInteger)
		{
			if ( forceint4 )
			{
				stringbuffer_append (sb_row, "int4");
			}
			else if  ( field_width < 5 )
			{
				stringbuffer_append (sb_row, "int2");
			}
			else if  ( field_width < 10 )
			{
				stringbuffer_append (sb_row, "int4");
			}
			else if  ( field_width < 19 )
			{
				stringbuffer_append (sb_row, "int8");
			}
			else
			{
				stringbuffer_aprintf (sb_row, "numeric(%d,0)", field_width);
			}
		}
		else if (type == FTDouble)
		{
			if ( field_width > 18 )
			{
				stringbuffer_append (sb_row, "numeric");
			}
			else
			{
				stringbuffer_append (sb_row, "float8");
			}
		}
		else if (type == FTLogical)
		{
			stringbuffer_append (sb_row, "boolean");
		}
		else
		{
			pgis_logf ("Invalid type in DBF file");
		}
	}
	/* Run the CREATE TABLE statement in the buffer and clear. */
	stringbuffer_append_c(sb_row, ')');
	if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
	stringbuffer_clear(sb_row);

	/* Create the geometry column with an addgeometry call */
	if ( schema && readshape == 1 )
	{
		stringbuffer_aprintf(sb_row, "SELECT AddGeometryColumn('%s','%s','%s','%d',",
		                     schema, table, geom, sr_id);
	}
	else if (readshape == 1)
	{
		stringbuffer_aprintf(sb_row, "SELECT AddGeometryColumn('','%s','%s','%d',",
		                     table, geom, sr_id);
	}
	if (pgtype)
	{ //pgtype will only be set if we are loading geometries
		stringbuffer_aprintf(sb_row, "'%s',%d)", pgtype, pgdims);
	}

	/* Run the AddGeometryColumn() statement in the buffer and clear. */
	if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
	stringbuffer_clear(sb_row);

	return 1;
}

int
CreateIndex(void)
{

	stringbuffer_clear(sb_row);

	if ( schema )
	{
		stringbuffer_aprintf(sb_row, "CREATE INDEX \"%s_%s_gist\" ON \"%s\".\"%s\" using gist (\"%s\" gist_geometry_ops)", table, geom, schema, table, geom);
	}
	else
	{
		stringbuffer_aprintf(sb_row, "CREATE INDEX \"%s_%s_gist\" ON \"%s\" using gist (\"%s\" gist_geometry_ops)", table, geom, table, geom);
	}

	/* Run the CREATE INDEX statement in the buffer and clear. */
	if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
	stringbuffer_clear(sb_row);

	return 1;
}

int
LoadData(void)
{
	int trans=0;

	if (cur_entity == -1 && dump_format)
	{
		char *copysql;
		if ( schema )
		{
			asprintf(&copysql, "COPY \"%s\".\"%s\" %s FROM stdin",
			         schema, table, col_names);
		}
		else
		{
			asprintf(&copysql, "COPY \"%s\" %s FROM stdin",
			         table, col_names);
		}
		pgis_copy_start(copysql);
		free(copysql);
	}

	/**************************************************************
	 * 
	 *   MAIN SHAPE OBJECTS SCAN
	 * 
	 **************************************************************/
	while (cur_entity <  num_entities - 1)
	{

		cur_entity++;

		/*wrap a transaction block around each 250 inserts... */
		if ( ! dump_format )
		{
			if (trans == 250)
			{
				trans=0;
				if ( ! pgis_exec("END") ) return 0;
				if ( ! pgis_exec("BEGIN") ) return 0;
			}
		}
		trans++;
		/* transaction stuff done */

		if ( gui_mode && ( cur_entity % 200 == 0 ) )
		{
			pgis_logf("Feature #%d", cur_entity);
		}

		/* skip the record if it has been deleted */
		if (readshape != 1 && DBFReadDeleted(hDBFHandle, cur_entity))
		{
			continue;
		}

		/* open the next object */
		if (readshape == 1)
		{
			obj = SHPReadObject(hSHPHandle,cur_entity);
			if ( ! obj )
			{
				pgis_logf("Error reading shape object %d", cur_entity);
				return 0;
			}

			if ( null_policy == skip_null && obj->nVertices == 0 )
			{
				SHPDestroyObject(obj);
				continue;
			}
		}

		/* New row, clear the stringbuffer. */
		stringbuffer_clear(sb_row);

		if (!dump_format)
		{
			if ( schema )
			{
				stringbuffer_aprintf(sb_row, "INSERT INTO \"%s\".\"%s\" %s VALUES (",
				                     schema, table, col_names);
			}
			else
			{
				stringbuffer_aprintf(sb_row, "INSERT INTO \"%s\" %s VALUES (",
				                     table, col_names);
			}
		}
		if ( ! Insert_attributes(hDBFHandle,cur_entity) ) return 0;

		if (readshape == 1)
		{
			/* ---------- NULL SHAPE ----------------- */
			if (obj->nVertices == 0)
			{
				if (dump_format)
				{
					stringbuffer_append(sb_row, "\\N\n");
					pgis_copy_write(stringbuffer_getstring(sb_row));
				}
				else
				{
					stringbuffer_append(sb_row, "NULL)");
					if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
				}
				SHPDestroyObject(obj);
				continue;
			}

			switch (obj->nSHPType)
			{
			case SHPT_POLYGON:
			case SHPT_POLYGONM:
			case SHPT_POLYGONZ:
				if ( ! InsertPolygon() ) return 0;
				break;

			case SHPT_POINT:
			case SHPT_POINTM:
			case SHPT_POINTZ:
			case SHPT_MULTIPOINT:
			case SHPT_MULTIPOINTM:
			case SHPT_MULTIPOINTZ:
				if ( ! InsertPoint() ) return 0;
				break;

			case SHPT_ARC:
			case SHPT_ARCM:
			case SHPT_ARCZ:
				if ( ! InsertLineString() ) return 0;
				break;

			default:
				pgis_logf ("**** Type is NOT SUPPORTED, type id = %d ****",
				           obj->nSHPType);
				break;

			}

			SHPDestroyObject(obj);
		}
		else
		{
			if ( dump_format )
			{   /* close for dbf only dump format */
				stringbuffer_append_c(sb_row, '\n');
				pgis_copy_write(stringbuffer_getstring(sb_row));
			}
			else
			{   /* close for dbf only sql insert format */
				if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
			}
		}

		/* Just do 100 entries at a time, then return to the idle loop. */
		if ( cur_entity % 100 == 0 ) return 1;

	} /* END of MAIN SHAPE OBJECT LOOP */

	if ( dump_format )
	{
		pgis_copy_end(0);
	}

	translation_stage = TRANSLATION_CLEANUP; /* done middle */

	return 1;
}


int
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

	if (wkbtype & WKBZOFFSET) hasz = 1;
	if (wkbtype & WKBMOFFSET) hasm = 1;
	TYPE_SETZM(dims, hasz, hasm);

	if (simple_geometries == 1 && obj->nParts > 1)
	{
		pgis_logf("We have a Multilinestring with %d parts, can't use -S switch!", obj->nParts);
		return 0;
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

	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);

	if (result)
	{
		pgis_logf("ERROR: %s", lwg_unparser_result.message);
		return 0;
	}

	if ( ! OutputGeometry(lwg_unparser_result.wkoutput) ) return 0;

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

	return 1;
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
		if (area < 0.0 || obj->nParts ==1)
		{
			Outer[out_index] = ring;
			out_index++;
		}

		/* Counterclockwise. It's an Inner Ring ! */
		else
		{
			Inner[in_index] = ring;
			in_index++;
		}
	}

	LWDEBUGF(4, "FindPolygons[%d]: found %d Outer, %d Inners\n", call, out_index, in_index);

	/* Put the inner rings into the list of the outer rings */
	/* of which they are within */
	for (pi=0; pi<in_index; pi++)
	{
		Point pt,pt2;
		int i;
		Ring *inner=Inner[pi], *outer=NULL;

		pt.x = inner->list[0].x;
		pt.y = inner->list[0].y;

		pt2.x = inner->list[1].x;
		pt2.y = inner->list[1].y;

		for (i=0; i<out_index; i++)
		{
			int in;

			in = PIP(pt, Outer[i]->list, Outer[i]->n);
			if ( in || PIP(pt2, Outer[i]->list, Outer[i]->n) )
			{
				outer = Outer[i];
				break;
			}
		}

		if ( outer )
		{
			outer->linked++;
			while (outer->next) outer = outer->next;
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
	for (pi=0; pi<npolys; pi++)
	{
		Ring *Poly, *temp;
		Poly = polys[pi];
		while (Poly != NULL)
		{
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
	}
	free(polys);
}

/*This function basically deals with the polygon case. */
/*it sorts the polys in order of outer,inner,inner, so that inners */
/*always come after outers they are within  */
int
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

	/* Determine the correct dimensions: */
	if (wkbtype & WKBZOFFSET) hasz = 1;
	if (wkbtype & WKBMOFFSET) hasm = 1;
	TYPE_SETZM(dims, hasz, hasm);

	polygon_total = FindPolygons(obj, &Outer);

	if (simple_geometries == 1 && polygon_total != 1) /* We write Non-MULTI geometries, but have several parts: */
	{
		pgis_logf("We have a Multipolygon with %d parts, can't use -S switch!", polygon_total);
		return 0;
	}

	/* Allocate memory for our array of LWPOLYs */
	lwpolygons = malloc(sizeof(LWPOLY *) * polygon_total);

	/* Allocate memory for our POINTARRAY pointers for each polygon */
	pas = malloc(sizeof(POINTARRAY **) * polygon_total);

	/* Cycle through each individual polygon */
	for (pi = 0; pi < polygon_total; pi++)
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

			for (vi = 0; vi < polyring->n; vi++)
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

	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);

	if (result)
	{
		pgis_logf( "ERROR: %s", lwg_unparser_result.message);
		return 0;
	}

	if ( ! OutputGeometry(lwg_unparser_result.wkoutput) ) return 0;

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

	return 1;
}

/*
 * Insert either a POINT or MULTIPOINT into the output stream
 */
int
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

	/* Determine the correct dimensions:  */
	if (wkbtype & WKBZOFFSET) hasz = 1;
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

	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, -1);

	if (result)
	{
		pgis_logf("ERROR: %s", lwg_unparser_result.message);
		return 0;
	}

	if ( ! OutputGeometry(lwg_unparser_result.wkoutput) ) return 0;

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

	return 1;
}

int
OutputGeometry(char *geometry)
{
	if (!dump_format)
		stringbuffer_append(sb_row, "'");

	stringbuffer_aprintf(sb_row, "%s", geometry);

	if (!dump_format)
	{
		stringbuffer_append(sb_row, "')");
		if ( ! pgis_exec(stringbuffer_getstring(sb_row)) ) return 0;
	}
	else
	{
		stringbuffer_append_c(sb_row, '\n');
		pgis_copy_write(stringbuffer_getstring(sb_row));
	}
	stringbuffer_clear(sb_row);
	return 1;

}



void
SetPgType(void)
{
	switch (shpfiletype)
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
		pgtype = "POINTM";
		pgdims = 3;
		istypeM = 1;
		break;
	case SHPT_ARCM: /* PolyLineM */
		wkbtype = MULTILINETYPE | WKBMOFFSET;
		pgtype = "MULTILINESTRINGM";
		pgdims = 3;
		istypeM = 1;
		break;
	case SHPT_POLYGONM: /* PolygonM */
		wkbtype = MULTIPOLYGONTYPE | WKBMOFFSET;
		pgtype = "MULTIPOLYGONM";
		pgdims = 3;
		istypeM = 1;
		break;
	case SHPT_MULTIPOINTM: /* MultiPointM */
		wkbtype = MULTIPOINTTYPE | WKBMOFFSET;
		pgtype = "MULTIPOINTM";
		pgdims = 3;
		istypeM = 1;
		break;
	case SHPT_POINTZ: /* PointZ */
		wkbtype = POINTTYPE | WKBMOFFSET | WKBZOFFSET;
		pgtype = "POINT";
		pgdims = 4;
		break;
	case SHPT_ARCZ: /* PolyLineZ */
		pgtype = "MULTILINESTRING";
		wkbtype = MULTILINETYPE | WKBZOFFSET | WKBMOFFSET;
		pgdims = 4;
		break;
	case SHPT_POLYGONZ: /* MultiPolygonZ */
		pgtype = "MULTIPOLYGON";
		wkbtype = MULTIPOLYGONTYPE | WKBZOFFSET | WKBMOFFSET;
		pgdims = 4;
		break;
	case SHPT_MULTIPOINTZ: /* MultiPointZ */
		pgtype = "MULTIPOINT";
		wkbtype = MULTIPOINTTYPE | WKBZOFFSET | WKBMOFFSET;
		pgdims = 4;
		break;
	default:
		pgtype = "GEOMETRY";
		wkbtype = COLLECTIONTYPE | WKBZOFFSET | WKBMOFFSET;
		pgdims = 4;
		pgis_logf("Unknown geometry type: %d", shpfiletype);
		break;
	}

	if (simple_geometries)
	{
		// adjust geometry name for CREATE TABLE by skipping MULTI
		if ((wkbtype & 0x7) == MULTIPOLYGONTYPE) pgtype += 5;
		if ((wkbtype & 0x7) == MULTILINETYPE) pgtype += 5;
	}
}

int
DropTable(char *schema, char *table, char *geom)
{
	/*---------------Drop the table--------------------------
	 * TODO: if the table has more then one geometry column
	 * the DROP TABLE call will leave spurious records in
	 * geometry_columns. 
	 *
	 * If the geometry column in the table being dropped
	 * does not match 'the_geom' or the name specified with
	 * -g an error is returned by DropGeometryColumn. 
	 *
	 * The table to be dropped might not exist.
	 *
	 */
	char *sql;

	if ( schema )
	{
		if (readshape == 1)
		{
			asprintf(&sql, "SELECT DropGeometryColumn('%s','%s','%s')",	schema, table, geom);
			if ( ! pgis_exec(sql) )
			{
				free(sql);
				return 0;
			}
			free(sql);
		}
		asprintf(&sql, "DROP TABLE \"%s\".\"%s\"", schema, table);
		if ( ! pgis_exec(sql) )
		{
			free(sql);
			return 0;
		}
		free(sql);
	}
	else
	{
		if (readshape == 1)
		{
			asprintf(&sql, "SELECT DropGeometryColumn('','%s','%s')", table, geom);
			if  ( ! pgis_exec(sql) )
			{
				free(sql);
				return 0;
			}
			free(sql);
		}
		asprintf(&sql, "DROP TABLE \"%s\"", table);
		if ( ! pgis_exec(sql) )
		{
			free(sql);
			return 0;
		}
		free(sql);
	}
	return 1;
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
	for (j=0;j<num_fields;j++)
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
		if ( name[0]=='_' ||
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
		for (z=0; z < j ; z++)
		{
			if (strcmp(field_names[z],name)==0)
			{
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
		if (readshape == 1 || j < (num_fields - 1))
		{
			//don't include last comma if its the last field and no geometry field will follow
			strcat(col_names, "\",");
		}
		else
		{
			strcat(col_names, "\"");
		}
	}
	/*sprintf(col_names, "%s\"%s\")", col_names, geom);*/
	if (readshape == 1)
	{
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
		pgis_logf("utf8: iconv_open: %s", strerror (errno));
		return NULL;
	}

	outbytesleft = inbytesleft*3+1; /* UTF8 string can be 3 times larger */
	/* then local string */
	outputbuf = (char *) malloc (outbytesleft);
	if (!outputbuf)
	{
		pgis_logf("utf8: malloc: %s", strerror (errno));
		return NULL;
	}
	memset (outputbuf, 0, outbytesleft);
	outputptr = outputbuf;

	if (-1==iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft))
	{
		pgis_logf("utf8: %s", strerror (errno));
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
