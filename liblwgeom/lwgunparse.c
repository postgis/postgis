/*
 * Written by Ralph Mason ralph.mason<at>telogis.com
 *
 * Copyright Telogis 2004
 * www.telogis.com
 *
 * $Id$
 */


#include <string.h>
#include <math.h>
#include <stdio.h>
/* Solaris9 does not provide stdint.h */
/* #include <stdint.h> */
#include <inttypes.h>

#include "liblwgeom_internal.h"
#include "wktparse.h"


/*-- Typedefs ---------------------------------------------- */

typedef uint32_t int4;
typedef uchar* (*outfunc)(uchar*,int);
typedef uchar* (*outwkbfunc)(uchar*);

/*-- Prototypes -------------------------------------------- */

void ensure(int chars);
void to_end(void);
void write_str(const char* str);
void write_double(double val);
void write_int(int i);
int4 read_int(uchar** geom);
double read_double(uchar** geom);
uchar* output_point(uchar* geom,int supress);
uchar* output_single(uchar* geom,int supress);
uchar* output_collection(uchar* geom,outfunc func,int supress);
uchar* output_line_collection(uchar* geom,outfunc func,int supress);
uchar* output_polygon_collection(uchar* geom,int suppress);
uchar* output_polygon_ring_collection(uchar* geom,outfunc func,int supress);
uchar* output_triangle_collection(uchar* geom,outfunc func,int supress);
uchar* output_circstring_collection(uchar* geom,outfunc func,int supress);
uchar* output_curvepoly(uchar* geom, int supress);
uchar* output_multipoint(uchar* geom,int suppress);
uchar* output_compound(uchar* geom, int suppress);
uchar* output_multisurface(uchar* geom, int suppress);

void write_wkb_hex_bytes(uchar* ptr, uint32 cnt, size_t size);
void write_wkb_bin_bytes(uchar* ptr, uint32 cnt, size_t size);
void write_wkb_bin_flip_bytes(uchar* ptr, uint32 cnt, size_t size);
void write_wkb_hex_flip_bytes(uchar* ptr, uint32 cnt, size_t size);

void write_wkb_int(int i);
uchar* output_wkb_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_polygon_collection(uchar* geom);
uchar* output_wkb_polygon_ring_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_triangle_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_line_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_circstring_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_point(uchar* geom);
uchar* output_wkb(uchar* geom);

/*-- Globals ----------------------------------------------- */

static int unparser_ferror_occured;
static int dims;
static allocator local_malloc;
static freeor local_free;
static char*  out_start;
static char*  out_pos;
static int len;
static uchar endianbyte;
void (*write_wkb_bytes)(uchar* ptr,uint32 cnt,size_t size);

/*
 * Unparser current instance check flags - a bitmap of flags that determine which checks are enabled during the current unparse
 * (see liblwgeom.h for the related PARSER_CHECK constants)
 */
int current_unparser_check_flags;

/*
 * Unparser current instance result structure - the result structure being used for the current unparse
 */
LWGEOM_UNPARSER_RESULT *current_lwg_unparser_result;

/*
 * Unparser error messages
 *
 * IMPORTANT: Make sure the order of these messages matches the UNPARSER_ERROR constants in liblwgeom.h!
 * The 0th element should always be empty since it is unused (error constants start from -1)
 */

const char *unparser_error_messages[] =
{
	"",
	"geometry requires more points",
	"geometry must have an odd number of points",
	"geometry contains non-closed rings"
};

/* Macro to return the error message and the current position within WKT */
#define LWGEOM_WKT_UNPARSER_ERROR(errcode) \
        do { \
		if (!unparser_ferror_occured) { \
                	unparser_ferror_occured = -1 * errcode; \
                	current_lwg_unparser_result->message = unparser_error_messages[errcode]; \
                	current_lwg_unparser_result->errlocation = (out_pos - out_start); \
		} \
        } while (0);

/* Macro to return the error message and the current position within WKB */
#define LWGEOM_WKB_UNPARSER_ERROR(errcode) \
        do { \
		if (!unparser_ferror_occured) { \
                	unparser_ferror_occured = -1 * errcode; \
                	current_lwg_unparser_result->message = unparser_error_messages[errcode]; \
                	current_lwg_unparser_result->errlocation = (out_pos - out_start); \
		} \
        } while (0);

/*---------------------------------------------------------- */






/*
 * Ensure there is enough space for chars bytes.
 * Reallocate memory is this is not the case.
 */
void
ensure(int chars)
{

	int pos = out_pos - out_start;

	if (  (pos + chars) >= len )
	{
		char* newp =(char*)local_malloc(len*2);
		memcpy(newp,out_start,len);
		local_free(out_start);
		out_start = newp;
		out_pos = newp + pos;
		len *=2;
	}
}

void
to_end(void)
{
	while (*out_pos)
	{
		out_pos++;
	}
}

void
write_str(const char* str)
{
	ensure(32);
	strcpy(out_pos,str);
	to_end();
}

void
write_double(double val)
{
	ensure(32);
	sprintf(out_pos,"%.15g",val);
	to_end();
}

void
write_int(int i)
{
	ensure(32);
	sprintf(out_pos,"%i",i);
	to_end();
}

int4
read_int(uchar** geom)
{
	int4 ret;
#ifdef SHRINK_INTS
	if ( getMachineEndian() == NDR )
	{
		if ( (**geom)& 0x01)
		{
			ret = **geom >>1;
			(*geom)++;
			return ret;
		}
	}
	else
	{
		if ( (**geom)& 0x80)
		{
			ret = **geom & ~0x80;
			(*geom)++;
			return ret;
		}
	}
#endif
	memcpy(&ret,*geom,4);

#ifdef SHRINK_INTS
	if ( getMachineEndian() == NDR )
	{
		ret >>= 1;
	}
#endif

	(*geom)+=4;
	return ret;
}

double round(double);

double read_double(uchar** geom)
{
	double ret;
	memcpy(&ret, *geom, 8);
	(*geom)+=8;
	return ret;
}

uchar *
output_point(uchar* geom,int supress)
{
	int i;

	for ( i = 0 ; i < dims ; i++ )
	{
		write_double(read_double(&geom));
		if (i +1 < dims )
			write_str(" ");
	}
	return geom;
}

uchar *
output_single(uchar* geom,int supress)
{
	write_str("(");
	geom=output_point(geom,supress);
	write_str(")");
	return geom;
}

/* Output a standard collection */
uchar *
output_collection(uchar* geom,outfunc func,int supress)
{
	int cnt = read_int(&geom);
	if ( cnt == 0 )
	{
		write_str(" EMPTY");
	}
	else
	{
		write_str("(");
		while (cnt--)
		{
			geom=func(geom,supress);
			if ( cnt )
			{
				write_str(",");
			}
		}
		write_str(")");
	}
	return geom;
}

/* Output a set of LINESTRING points */
uchar *
output_line_collection(uchar* geom,outfunc func,int supress)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	if ( cnt == 0 )
	{
		write_str(" EMPTY");
	}
	else
	{
		write_str("(");
		while (cnt--)
		{
			geom=func(geom,supress);
			if ( cnt )
			{
				write_str(",");
			}
		}
		write_str(")");
	}

	/* Ensure that LINESTRING has a minimum of 2 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 2)
		LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);

	return geom;
}

uchar *
output_triangle_collection(uchar* geom,outfunc func,int supress)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	if ( cnt == 0 )
	{
		write_str(" EMPTY");
	}
	else
	{
		write_str("((");
		while (cnt--)
		{
			geom=func(geom,supress);
			if ( cnt )
			{
				write_str(",");
			}
		}
		write_str("))");
	}

	/* Ensure that TRIANGLE has 4 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt != 4)
		LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);

	return geom;
}

/* Output an individual ring from a POLYGON  */
uchar *
output_polygon_ring_collection(uchar* geom,outfunc func,int supress)
{
	uchar *temp;
	int dimcount;
	double *first_point;
	double *last_point;
	int cnt;
	int orig_cnt;

	first_point = lwalloc(dims * sizeof(double));
	last_point = lwalloc(dims * sizeof(double));

	cnt = read_int(&geom);
	orig_cnt = cnt;
	if ( cnt == 0 )
	{
		write_str(" EMPTY");
	}
	else
	{
		write_str("(");

		/* Store the first point of the ring (use a temp var since read_double alters
		  	the pointer after use) */
		temp = geom;
		dimcount = 0;
		while (dimcount < dims)
		{
			first_point[dimcount] = read_double(&temp);
			dimcount++;
		}

		while (cnt--)
		{
			geom=func(geom,supress);
			if ( cnt )
			{
				write_str(",");
			}
		}
		write_str(")");

		/* Store the last point of the ring (note: we will have moved past it, so we
		  	need to count backwards) */
		temp = geom - sizeof(double) * dims;
		dimcount = 0;
		while (dimcount < dims)
		{
			last_point[dimcount] = read_double(&temp);
			dimcount++;
		}

		/* Check if they are the same...

		WARNING: due to various GEOS bugs related to producing rings with incorrect
		          3rd dimensions, the closure check here for outgoing geometries only checks on 2
		dimensions. This is currently different to the parser! */

		if (
		    (first_point[0] != last_point[0] || first_point[1] != last_point[1] ) &&
		    (current_unparser_check_flags & PARSER_CHECK_CLOSURE))
		{
			LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_UNCLOSED);
		}


		/* Ensure that POLYGON has a minimum of 4 points */
		if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 4)
		{
			LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
		}
	}

	lwfree(first_point);
	lwfree(last_point);

	return geom;
}

/* Ouput the points from a CIRCULARSTRING */
uchar *
output_circstring_collection(uchar* geom,outfunc func,int supress)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	if ( cnt == 0 )
	{
		write_str(" EMPTY");
	}
	else
	{
		write_str("(");
		while (cnt--)
		{
			geom=func(geom,supress);
			if ( cnt )
			{
				write_str(",");
			}
		}
		write_str(")");
	}

	/* Ensure that a CIRCULARSTRING has a minimum of 3 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 3)
	{
		LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	/* Ensure that a CIRCULARSTRING has an odd number of points */
	if ((current_unparser_check_flags & PARSER_CHECK_ODD) && orig_cnt % 2 != 1)
	{
		LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_ODDPOINTS);
	}

	return geom;
}

/* Output a POLYGON consisting of a number of rings */
uchar *
output_polygon_collection(uchar* geom,int suppress)
{
	return output_polygon_ring_collection(geom,output_point,suppress);
}

uchar *output_wkt(uchar* geom, int supress);

/* special case for multipoint to supress extra brackets */
uchar *output_multipoint(uchar* geom,int suppress)
{
	uchar type = *geom;

	if ( TYPE_GETTYPE(type)  == POINTTYPE )
		return output_point(++geom,suppress);

	return output_wkt(geom,suppress);
}

/* Special case for compound curves: suppress the LINESTRING prefix from a curve if it appears as
   a component of a COMPOUNDCURVE, but not CIRCULARSTRING */
uchar *output_compound(uchar* geom, int suppress)
{
	uchar type;

	LWDEBUG(2, "output_compound called.");

	type=*geom;
	switch (TYPE_GETTYPE(type))
	{
	case LINETYPE:
		geom = output_wkt(geom,2);
		break;
	case CIRCSTRINGTYPE:
		geom = output_wkt(geom,1);
		break;
	}
	return geom;
}

/*
 * Supress linestring but not circularstring and correctly handle compoundcurve
 */
uchar *output_curvepoly(uchar* geom, int supress)
{
	unsigned type;
	type = *geom++;

	LWDEBUG(2, "output_curvepoly.");

	switch (TYPE_GETTYPE(type))
	{
	case LINETYPE:
		geom = output_collection(geom,output_point,0);
		break;
	case CIRCSTRINGTYPE:
		write_str("CIRCULARSTRING");
		geom = output_circstring_collection(geom,output_point,1);
		break;
	case COMPOUNDTYPE:
		write_str("COMPOUNDCURVE");
		geom = output_collection(geom,output_compound,1);
		break;
	}
	return geom;
}

/* Special case for multisurfaces: suppress the POLYGON prefix from a surface if it appears as
   a component of a MULTISURFACE, but not CURVEPOLYGON */
uchar *output_multisurface(uchar* geom, int suppress)
{
	uchar type=*geom;

	LWDEBUG(2, "output_multisurface called.");

	switch (TYPE_GETTYPE(type))
	{
	case POLYGONTYPE:
		geom = output_wkt(geom,2);
		break;
	case CURVEPOLYTYPE:
		geom = output_wkt(geom,1);
		break;
	}
	return geom;
}


/*
 * Suppress=0 -- write TYPE, M, coords
 * Suppress=1 -- write TYPE, coords
 * Suppress=2 -- write only coords
 */
uchar *
output_wkt(uchar* geom, int supress)
{

	uchar type=*geom++;
	char writeM=0;
	dims = TYPE_NDIMS(type); /* ((type & 0x30) >> 4)+2; */

	LWDEBUG(2, "output_wkt called.");

	if ( ! supress && !TYPE_HASZ(type) && TYPE_HASM(type) ) writeM=1;


	/* Skip the bounding box if there is one */
	if ( TYPE_HASBBOX(type) )
	{
		geom+=16;
	}

	if ( TYPE_HASSRID(type) )
	{
		write_str("SRID=");
		write_int(read_int(&geom));
		write_str(";");
	}

	switch (TYPE_GETTYPE(type))
	{
	case POINTTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("POINTM");
			else write_str("POINT");
		}
		geom=output_single(geom,0);
		break;
	case LINETYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("LINESTRINGM");
			else write_str("LINESTRING");
		}
		geom = output_line_collection(geom,output_point,0);
		break;
	case CIRCSTRINGTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("CIRCULARSTRINGM");
			else write_str("CIRCULARSTRING");
		}
		geom = output_circstring_collection(geom,output_point,0);
		break;
	case POLYGONTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("POLYGONM");
			else write_str("POLYGON");
		}
		geom = output_collection(geom,output_polygon_collection,0);
		break;
	case TRIANGLETYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("TRIANGLEM");
			else write_str("TRIANGLE");
		}
		geom = output_triangle_collection(geom,output_point,0);
		break;
	case COMPOUNDTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("COMPOUNDCURVEM");
			else write_str("COMPOUNDCURVE");
		}
		geom = output_collection(geom, output_compound,1);
		break;
	case CURVEPOLYTYPE:
		if (supress < 2)
		{
			if (writeM) write_str("CURVEPOLYGONM");
			else write_str("CURVEPOLYGON");
		}
		geom = output_collection(geom, output_curvepoly,0);
		break;
	case MULTIPOINTTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("MULTIPOINTM");
			else write_str("MULTIPOINT");
		}
		geom = output_collection(geom,output_multipoint,2);
		break;
	case MULTILINETYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("MULTILINESTRINGM");
			else write_str("MULTILINESTRING");
		}
		geom = output_collection(geom,output_wkt,2);
		break;
	case MULTICURVETYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("MULTICURVEM");
			else write_str("MULTICURVE");
		}
		geom = output_collection(geom,output_compound,2);
		break;
	case MULTIPOLYGONTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("MULTIPOLYGONM");
			else write_str("MULTIPOLYGON");
		}
		geom = output_collection(geom,output_wkt,2);
		break;
	case MULTISURFACETYPE:
		if ( supress < 2)
		{
			if (writeM) write_str("MULTISURFACEM");
			else write_str("MULTISURFACE");
		}
		geom = output_collection(geom,output_multisurface,2);
		break;
	case POLYHEDRALSURFACETYPE:
		if ( supress < 2)
		{
			if (writeM) write_str("POLYHEDRALSURFACEM");
			else write_str("POLYHEDRALSURFACE");
		}
		geom = output_collection(geom,output_wkt,2);
		break;
	case TINTYPE:
		if ( supress < 2)
		{
			if (writeM) write_str("TINM");
			else write_str("TIN");
		}
		geom = output_collection(geom,output_wkt,2);
		break;
	case COLLECTIONTYPE:
		if ( supress < 2 )
		{
			if (writeM) write_str("GEOMETRYCOLLECTIONM");
			else write_str("GEOMETRYCOLLECTION");
		}
		geom = output_collection(geom,output_wkt,1);
		break;

	}
	return geom;
}

int
unparse_WKT(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar* serialized, allocator alloc, freeor free, int flags)
{

	LWDEBUGF(2, "unparse_WKT called with parser flags %d.", flags);

	if (serialized==NULL)
		return 0;

	/* Setup the inital parser flags and empty the return struct */
	current_lwg_unparser_result = lwg_unparser_result;
	current_unparser_check_flags = flags;
	lwg_unparser_result->wkoutput = NULL;
	lwg_unparser_result->size = 0;
	lwg_unparser_result->serialized_lwgeom = serialized;

	unparser_ferror_occured = 0;
	local_malloc=alloc;
	local_free=free;
	len = 128;
	out_start = out_pos = alloc(len);

	output_wkt(serialized, 0);

	/* Store the result in the struct */
	lwg_unparser_result->wkoutput = out_start;
	lwg_unparser_result->size = strlen(out_start);

	return unparser_ferror_occured;
}

static char outchr[]=
{
	"0123456789ABCDEF"
};

/* Write HEX bytes flipping */
void
write_wkb_hex_flip_bytes(uchar* ptr, uint32 cnt, size_t size)
{
	uint32 bc; /* byte count */

	ensure(cnt*2*size);

	while (cnt--)
	{
		for (bc=size; bc; bc--)
		{
			*out_pos++ = outchr[ptr[bc-1]>>4];
			*out_pos++ = outchr[ptr[bc-1]&0x0F];
		}
		ptr+=size;
	}
}

/* Write HEX bytes w/out flipping */
void
write_wkb_hex_bytes(uchar* ptr, uint32 cnt, size_t size)
{
	uint32 bc; /* byte count */

	ensure(cnt*2*size);

	while (cnt--)
	{
		for (bc=0; bc<size; bc++)
		{
			*out_pos++ = outchr[ptr[bc]>>4];
			*out_pos++ = outchr[ptr[bc]&0x0F];
		}
		ptr+=size;
	}
}

/* Write BIN bytes flipping */
void
write_wkb_bin_flip_bytes(uchar* ptr, uint32 cnt, size_t size)
{
	uint32 bc; /* byte count */

	ensure(cnt*size);

	while (cnt--)
	{
		for (bc=size; bc; bc--)
			*out_pos++ = ptr[bc-1];
		ptr+=size;
	}
}


/* Write BIN bytes w/out flipping */
void
write_wkb_bin_bytes(uchar* ptr, uint32 cnt, size_t size)
{
	uint32 bc; /* byte count */

	ensure(cnt*size);

	/* Could just use a memcpy here ... */
	while (cnt--)
	{
		for (bc=0; bc<size; bc++)
			*out_pos++ = ptr[bc];
		ptr+=size;
	}
}

uchar *
output_wkb_point(uchar* geom)
{
	write_wkb_bytes(geom,dims,8);
	return geom + (8*dims);
}

void
write_wkb_int(int i)
{
	write_wkb_bytes((uchar*)&i,1,4);
}

/* Output a standard collection */
uchar *
output_wkb_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);

	LWDEBUGF(2, "output_wkb_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while (cnt--) geom=func(geom);
	return geom;
}

/* Output a set of LINESTRING points */
uchar *
output_wkb_line_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_line_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while (cnt--) geom=func(geom);

	/* Ensure that LINESTRING has a minimum of 2 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 2)
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	return geom;
}

/* Output an individual ring from a POLYGON  */
uchar *
output_wkb_polygon_ring_collection(uchar* geom,outwkbfunc func)
{
	uchar *temp;
	int dimcount;
	double *first_point;
	double *last_point;
	int cnt;
	int orig_cnt;

	cnt = read_int(&geom);
	orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_polygon_ring_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);

	if ( ! cnt ) return geom;

	first_point = lwalloc(dims * sizeof(double));
	last_point = lwalloc(dims * sizeof(double));

	/* Store the first point of the ring (use a temp var since read_double alters
	   the pointer after use) */
	temp = geom;
	dimcount = 0;
	while (dimcount < dims)
	{
		first_point[dimcount] = read_double(&temp);
		dimcount++;
	}

	while (cnt--) geom=func(geom);

	/* Store the last point of the ring (note: we will have moved past it, so we
	   need to count backwards) */
	temp = geom - sizeof(double) * dims;
	dimcount = 0;
	while (dimcount < dims)
	{
		last_point[dimcount] = read_double(&temp);
		dimcount++;
	}

	/* Check if they are the same... */
	if (((first_point[0] != last_point[0]) ||
	        (first_point[1] != last_point[1])) &&
	        (current_unparser_check_flags & PARSER_CHECK_CLOSURE))
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_UNCLOSED);
	}

	/* Ensure that POLYGON has a minimum of 4 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 4)
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	lwfree(first_point);
	lwfree(last_point);

	return geom;
}

/* Output a POLYGON consisting of a number of rings */
uchar *
output_wkb_polygon_collection(uchar* geom)
{
	LWDEBUG(2, "output_wkb_polygon_collection");

	return output_wkb_polygon_ring_collection(geom,output_wkb_point);
}

/* Output a set of TRIANGLE points */
uchar *
output_wkb_triangle_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_triangle_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while (cnt--) geom=func(geom);

	/* Ensure that TRIANGLE has exactly 4 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt !=4)
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	return geom;
}

/* Output an individual ring from a POLYGON  */
/* Ouput the points from a CIRCULARSTRING */
uchar *
output_wkb_circstring_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_circstring_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while (cnt--) geom=func(geom);

	/* Ensure that a CIRCULARSTRING has a minimum of 3 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 3)
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	/* Ensure that a CIRCULARSTRING has an odd number of points */
	if ((current_unparser_check_flags & PARSER_CHECK_ODD) && orig_cnt % 2 != 1)
	{
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_ODDPOINTS);
	}

	return geom;
}

uchar *
output_wkb(uchar* geom)
{
	uchar type=*geom++;
	int4 wkbtype;

	dims = TYPE_NDIMS(type);

	LWDEBUGF(2, "output_wkb: type %d, dims set to %d",
		TYPE_GETTYPE(type), dims);

	/* Skip the bounding box */
	if ( TYPE_HASBBOX(type) )
	{
		geom+=16;
	}

	wkbtype = TYPE_GETTYPE(type);

	if ( TYPE_HASZ(type) )
		wkbtype |= WKBZOFFSET;
	if ( TYPE_HASM(type) )
		wkbtype |= WKBMOFFSET;
	if ( TYPE_HASSRID(type) )
	{
		wkbtype |= WKBSRIDFLAG;
	}


	/* Write byteorder (as from WKB specs...) */
	write_wkb_bytes(&endianbyte,1,1);

	write_wkb_int(wkbtype);

	if ( TYPE_HASSRID(type) )
	{
		write_wkb_int(read_int(&geom));
	}

	switch (TYPE_GETTYPE(type))
	{
	case POINTTYPE:
		geom=output_wkb_point(geom);
		break;
	case LINETYPE:
		geom=output_wkb_line_collection(geom,output_wkb_point);
		break;
	case CIRCSTRINGTYPE:
		geom=output_wkb_circstring_collection(geom,output_wkb_point);
		break;
	case POLYGONTYPE:
		geom=output_wkb_collection(geom,output_wkb_polygon_collection);
		break;
	case TRIANGLETYPE:
		geom=output_wkb_triangle_collection(geom,output_wkb_point);
		break;
	case COMPOUNDTYPE:
		geom=output_wkb_collection(geom,output_wkb);
		break;
	case CURVEPOLYTYPE:
		geom=output_wkb_collection(geom,output_wkb);
		break;
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		geom = output_wkb_collection(geom,output_wkb);
		break;
	}
	return geom;
}

int
unparse_WKB(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar* serialized, allocator alloc, freeor free, int flags, char endian, uchar hex)
{
	LWDEBUGF(2, "unparse_WKB(%p,...) called with parser flags %d", serialized, flags);

	if (serialized==0)
		return 0;

	/* Setup the inital parser flags and empty the return struct */
	current_lwg_unparser_result = lwg_unparser_result;
	current_unparser_check_flags = flags;
	lwg_unparser_result->wkoutput = NULL;
	lwg_unparser_result->size = 0;
	lwg_unparser_result->serialized_lwgeom = serialized;

	unparser_ferror_occured = 0;
	local_malloc=alloc;
	local_free=free;
	len = 128;
	out_start = out_pos = alloc(len);

	if ( endian == (char)-1 )
	{
		endianbyte = getMachineEndian();
		if ( hex ) write_wkb_bytes = write_wkb_hex_bytes;
		else write_wkb_bytes = write_wkb_bin_bytes;
	}
	else
	{
		endianbyte = endian;
		if ( endianbyte != getMachineEndian() )
		{
			if ( hex ) write_wkb_bytes = write_wkb_hex_flip_bytes;
			else write_wkb_bytes = write_wkb_bin_flip_bytes;
		}
		else
		{
			if ( hex ) write_wkb_bytes = write_wkb_hex_bytes;
			else write_wkb_bytes = write_wkb_bin_bytes;
		}
	}

	output_wkb(serialized);

	if ( hex )
	{
		ensure(1);
		*out_pos=0;
	}

	/* Store the result in the struct */
	lwg_unparser_result->wkoutput = out_start;
	lwg_unparser_result->size = (out_pos-out_start);

	return unparser_ferror_occured;
}
