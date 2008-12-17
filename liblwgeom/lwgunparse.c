/*
 * Written by Ralph Mason ralph.mason<at>telogis.com
 *
 * Copyright Telogis 2004
 * www.telogis.com
 *
 * $Id: wktunparse.c 2781 2008-05-22 20:43:00Z mcayland $
 */


#include <string.h>
#include <math.h>
#include <stdio.h>
/* TO get byte order */
#include <sys/types.h>
#include <sys/param.h>
/* Solaris9 does not provide stdint.h */
/* #include <stdint.h> */
#include <inttypes.h>

#include "liblwgeom.h"
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
uchar* output_curve_collection(uchar* geom,outfunc func,int supress);
uchar* output_multipoint(uchar* geom,int suppress);
uchar* output_compound(uchar* geom, int suppress);
uchar* output_multisurface(uchar* geom, int suppress);

void write_wkb_hex_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_bin_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_bin_flip_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_hex_flip_bytes(uchar* ptr, unsigned int cnt, size_t size);

void write_wkb_int(int i);
uchar* output_wkb_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_polygon_collection(uchar* geom);
uchar* output_wkb_polygon_ring_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_line_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_curve_collection(uchar* geom,outwkbfunc func);
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
static int lwgi;
static uchar endianbyte;
void (*write_wkb_bytes)(uchar* ptr,unsigned int cnt,size_t size);

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

const char *unparser_error_messages[] = {
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
ensure(int chars){

	int pos = out_pos - out_start;

	if (  (pos + chars) >= len ){
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
	while(*out_pos){
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
write_double(double val){
	ensure(32);
	if (lwgi)
		sprintf(out_pos,"%.8g",val);
	else
		sprintf(out_pos,"%.15g",val);
	to_end();
}

void
write_int(int i){
	ensure(32);
	sprintf(out_pos,"%i",i);
	to_end();
}

int4
read_int(uchar** geom)
{
	int4 ret;
#ifdef SHRINK_INTS
	if ( getMachineEndian() == NDR ){
		if( (**geom)& 0x01){
			ret = **geom >>1;
			(*geom)++;
			return ret;
		}
	}
	else{
		if( (**geom)& 0x80){
			ret = **geom & ~0x80;
			(*geom)++;
			return ret;
		}
	}
#endif
	memcpy(&ret,*geom,4);

#ifdef SHRINK_INTS
	if ( getMachineEndian() == NDR ){
		ret >>= 1;
	}
#endif

	(*geom)+=4;
	return ret;
}

double round(double);

double read_double(uchar** geom){
	if (lwgi){
		double ret = *((int4*)*geom);
		ret /= 0xb60b60;
		(*geom)+=4;
		return ret-180.0;
	}
	else{
		double ret;
 		memcpy(&ret, *geom, 8);
		(*geom)+=8;
		return ret;
	}
}
           
uchar *
output_point(uchar* geom,int supress)
{
	int i;

	for( i = 0 ; i < dims ; i++ ){
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
	if ( cnt == 0 ){
		write_str(" EMPTY");
	}
	else{
		write_str("(");
		while(cnt--){
			geom=func(geom,supress);
			if ( cnt ){
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

	if ( cnt == 0 ){
		write_str(" EMPTY");
	}
	else{
		write_str("(");
		while(cnt--){
			geom=func(geom,supress);
			if ( cnt ){
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

/* Output an individual ring from a POLYGON  */
uchar *
output_polygon_ring_collection(uchar* geom,outfunc func,int supress)
{
        uchar *temp;
        int dimcount;
        double first_point[dims];
        double last_point[dims];

	int cnt = read_int(&geom);
	int orig_cnt = cnt;
	if ( cnt == 0 ){
		write_str(" EMPTY");
	}
	else{
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
	
		while(cnt--){
			geom=func(geom,supress);
			if ( cnt ){
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

        	/* Check if they are the same... */
        	if (memcmp(&first_point, &last_point, sizeof(double) * dims) &&
			(current_unparser_check_flags & PARSER_CHECK_CLOSURE))
                	LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_UNCLOSED);	

		/* Ensure that POLYGON has a minimum of 4 points */
        	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 4)
                	LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}
	return geom;
}

/* Ouput the points from a CIRCULARSTRING */
uchar *
output_curve_collection(uchar* geom,outfunc func,int supress)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	if ( cnt == 0 ){
		write_str(" EMPTY");
	}
	else{
		write_str("(");
		while(cnt--){
			geom=func(geom,supress);
			if ( cnt ){
				write_str(",");
			}
		}
		write_str(")");
	}

	/* Ensure that a CIRCULARSTRING has a minimum of 3 points */
        if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 3) {
                LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	/* Ensure that a CIRCULARSTRING has an odd number of points */
        if ((current_unparser_check_flags & PARSER_CHECK_ODD) && orig_cnt % 2 != 1) {
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
uchar *output_multipoint(uchar* geom,int suppress){
	unsigned type = *geom & 0x0f;
	
	if ( type  == POINTTYPE )
		return output_point(++geom,suppress);
	else if ( type == POINTTYPEI ){
		lwgi++;
		geom=output_point(++geom,0);
		lwgi--;
		return geom;
	}
	
	return output_wkt(geom,suppress);
}

/* Special case for compound curves: suppress the LINESTRING prefix from a curve if it appears as
   a component of a COMPOUNDCURVE, but not CIRCULARSTRING */
uchar *output_compound(uchar* geom, int suppress) {
        unsigned type;

        LWDEBUG(2, "output_compound called.");

        type=*geom;
        switch(TYPE_GETTYPE(type)) 
        {
                case LINETYPE:
                        geom = output_wkt(geom,2);
                        break;
                case CURVETYPE:
                        geom = output_wkt(geom,1);
                        break;
        }
	return geom;
}

/* Special case for multisurfaces: suppress the POLYGON prefix from a surface if it appears as
   a component of a MULTISURFACE, but not CURVEPOLYGON */
uchar *output_multisurface(uchar* geom, int suppress) {
        unsigned type;

        LWDEBUG(2, "output_multisurface called.");

        type=*geom;
        switch(TYPE_GETTYPE(type))
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

	unsigned type=*geom++;
	char writeM=0;
	dims = TYPE_NDIMS(type); /* ((type & 0x30) >> 4)+2; */

        LWDEBUG(2, "output_wkt called.");

	if ( ! supress && !TYPE_HASZ(type) && TYPE_HASM(type) ) writeM=1;


	/* Skip the bounding box if there is one */
	if ( TYPE_HASBBOX(type) )
	{
		geom+=16;
	}

	if ( TYPE_HASSRID(type) ) {
		write_str("SRID=");write_int(read_int(&geom));write_str(";");
	}

	switch(TYPE_GETTYPE(type)) {
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
                case CURVETYPE:
                        if ( supress < 2 )
                        {
                                if(writeM) write_str("CIRCULARSTRINGM");
                                else write_str("CIRCULARSTRING");
                        }
                        geom = output_curve_collection(geom,output_point,0);
                        break;
		case POLYGONTYPE:
			if ( supress < 2 )
			{
				if (writeM) write_str("POLYGONM");
				else write_str("POLYGON");
			}
			geom = output_collection(geom,output_polygon_collection,0);
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
                                if(writeM) write_str("CURVEPOLYGONM");
                                else write_str("CURVEPOLYGON");
                        }
                        geom = output_collection(geom, output_compound,0);
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
		case COLLECTIONTYPE:
			if ( supress < 2 )
			{
				if (writeM) write_str("GEOMETRYCOLLECTIONM");
				else write_str("GEOMETRYCOLLECTION");
			}
			geom = output_collection(geom,output_wkt,1);
			break;

		case POINTTYPEI:
			if ( supress < 2 )
			{
				if (writeM) write_str("POINTM");
				else write_str("POINT");
			}
			lwgi++;
			geom=output_single(geom,0);
			lwgi--;
			break;
		case LINETYPEI:
			if ( supress < 2 )
			{
				if (writeM) write_str("LINESTRINGM");
				else write_str("LINESTRING");
			}
			lwgi++;
			geom = output_collection(geom,output_point,0);
			lwgi--;
			break;
		case POLYGONTYPEI:
			if ( supress < 2 )
			{
				if (writeM) write_str("POLYGONM");
				else write_str("POLYGON");
			}
			lwgi++;
			geom = output_collection(geom,output_polygon_collection,0);
			lwgi--;
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
	lwgi=0;

	output_wkt(serialized, 0);

	/* Store the result in the struct */
	lwg_unparser_result->wkoutput = out_start;
	lwg_unparser_result->size = strlen(out_start);

	return unparser_ferror_occured;
}

static char outchr[]={"0123456789ABCDEF" };

/* Write HEX bytes flipping */
void
write_wkb_hex_flip_bytes(uchar* ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; /* byte count */

	ensure(cnt*2*size);

	while(cnt--){
		for(bc=size; bc; bc--)
		{
			*out_pos++ = outchr[ptr[bc-1]>>4];
			*out_pos++ = outchr[ptr[bc-1]&0x0F];
		}
		ptr+=size;
	}
}

/* Write HEX bytes w/out flipping */
void
write_wkb_hex_bytes(uchar* ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; /* byte count */

	ensure(cnt*2*size);

	while(cnt--){
		for(bc=0; bc<size; bc++)
		{
			*out_pos++ = outchr[ptr[bc]>>4];
			*out_pos++ = outchr[ptr[bc]&0x0F];
		}
		ptr+=size;
	}
}

/* Write BIN bytes flipping */
void
write_wkb_bin_flip_bytes(uchar* ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; /* byte count */

	ensure(cnt*size);

	while(cnt--)
	{
		for(bc=size; bc; bc--)
			*out_pos++ = ptr[bc-1];
		ptr+=size;
	}
}


/* Write BIN bytes w/out flipping */
void
write_wkb_bin_bytes(uchar* ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; /* byte count */

	ensure(cnt*size);

	/* Could just use a memcpy here ... */
	while(cnt--)
	{
		for(bc=0; bc<size; bc++)
			*out_pos++ = ptr[bc];
		ptr+=size;
	}
}

uchar *
output_wkb_point(uchar* geom)
{
	if ( lwgi ){
		write_wkb_bytes(geom,dims,4);
		return geom + (4*dims);
	}
	else{
		write_wkb_bytes(geom,dims,8);
		return geom + (8*dims);
	}
}

void
write_wkb_int(int i){
	write_wkb_bytes((uchar*)&i,1,4);
}

/* Output a standard collection */
uchar *
output_wkb_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);

	LWDEBUGF(2, "output_wkb_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while(cnt--) geom=func(geom);
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
	while(cnt--) geom=func(geom);

	/* Ensure that LINESTRING has a minimum of 2 points */
        if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 2) {
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
	double first_point[dims];
	double last_point[dims];

	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_polygon_ring_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);

	/* Store the first point of the ring (use a temp var since read_double alters 
	   the pointer after use) */
	temp = geom;
	dimcount = 0;
	while (dimcount < dims)
	{
		first_point[dimcount] = read_double(&temp);
		dimcount++;
	}

	while(cnt--) geom=func(geom); 

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
	if (memcmp(&first_point, &last_point, sizeof(double) * dims) &&
		(current_unparser_check_flags & PARSER_CHECK_CLOSURE)) {
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_UNCLOSED);
	}

	/* Ensure that POLYGON has a minimum of 4 points */
	if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 4)
		LWGEOM_WKT_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);

	return geom;
}

/* Output a POLYGON consisting of a number of rings */
uchar *
output_wkb_polygon_collection(uchar* geom)
{
	LWDEBUG(2, "output_wkb_polygon_collection");

	return output_wkb_polygon_ring_collection(geom,output_wkb_point); 
}

/* Ouput the points from a CIRCULARSTRING */
uchar *
output_wkb_curve_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
	int orig_cnt = cnt;

	LWDEBUGF(2, "output_wkb_curve_collection: %d iterations loop", cnt);

	write_wkb_int(cnt);
	while(cnt--) geom=func(geom);

	/* Ensure that a CIRCULARSTRING has a minimum of 3 points */
        if ((current_unparser_check_flags & PARSER_CHECK_MINPOINTS) && orig_cnt < 3) {
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_MOREPOINTS);
	}

	/* Ensure that a CIRCULARSTRING has an odd number of points */
        if ((current_unparser_check_flags & PARSER_CHECK_ODD) && orig_cnt % 2 != 1) {
		LWGEOM_WKB_UNPARSER_ERROR(UNPARSER_ERROR_ODDPOINTS);
	}

	return geom;
}

uchar *
output_wkb(uchar* geom)
{
	unsigned char type=*geom++;
	int4 wkbtype;

	dims = TYPE_NDIMS(type); 

	LWDEBUGF(2, "output_wkb: dims set to %d", dims);

	/* Skip the bounding box */
	if ( TYPE_HASBBOX(type) ) { 
		geom+=16;
	}

	wkbtype = TYPE_GETTYPE(type); 

	if ( TYPE_HASZ(type) )
		 wkbtype |= WKBZOFFSET;
	if ( TYPE_HASM(type) )
		 wkbtype |= WKBMOFFSET;
	if ( TYPE_HASSRID(type) ) {
		wkbtype |= WKBSRIDFLAG;
	}


	/* Write byteorder (as from WKB specs...) */
	write_wkb_bytes(&endianbyte,1,1);

	write_wkb_int(wkbtype);

	if ( TYPE_HASSRID(type) ) {
		write_wkb_int(read_int(&geom));
	}

	switch(TYPE_GETTYPE(type)){
		case POINTTYPE:
			geom=output_wkb_point(geom);
			break;
		case LINETYPE:
			geom=output_wkb_line_collection(geom,output_wkb_point);
			break;
                case CURVETYPE:
                        geom=output_wkb_curve_collection(geom,output_wkb_point);
                        break;
		case POLYGONTYPE:
			geom=output_wkb_collection(geom,output_wkb_polygon_collection);
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
		case COLLECTIONTYPE:
			geom = output_wkb_collection(geom,output_wkb);
			break;

		/*
			These don't output standard wkb at the moment
			the output and integer version.

			however you could run it through the wkt parser
			for a lwg and then output that.  There should
			also be a force_to_real_(lwgi)
		*/
		case POINTTYPEI:
			lwgi++;
			geom=output_wkb_point(geom);
			lwgi--;
			break;
		case LINETYPEI:
			lwgi++;
			geom = output_wkb_collection(geom,output_wkb_point);
			lwgi--;
			break;
		case POLYGONTYPEI:
			lwgi++;
			geom = output_wkb_collection(geom,output_wkb_polygon_collection);
			lwgi--;
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
	lwgi=0;

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

	if ( hex ) {
		ensure(1);
		*out_pos=0;
	}

	/* Store the result in the struct */	
	lwg_unparser_result->wkoutput = out_start;
	lwg_unparser_result->size = (out_pos-out_start);

	return unparser_ferror_occured;	
}


/******************************************************************
 * $Log$
 * Revision 1.23  2006/02/06 11:12:22  strk
 * uint32_t typedef moved back from wktparse.h to lwgparse.c and wktunparse.c
 *
 * Revision 1.22  2006/02/03 20:53:37  strk
 * Swapped stdint.h (unavailable on Solaris9) with inttypes.h
 *
 * Revision 1.21  2006/02/03 09:52:14  strk
 * Changed int4 typedefs to use POSIX uint32_t
 *
 * Revision 1.20  2006/01/09 15:12:02  strk
 * ISO C90 comments
 *
 * Revision 1.19  2005/03/10 18:19:16  strk
 * Made void args explicit to make newer compilers happy
 *
 * Revision 1.18  2005/02/21 16:16:14  strk
 * Changed byte to uchar to avoid clashes with win32 headers.
 *
 * Revision 1.17  2005/02/07 13:21:10  strk
 * Replaced DEBUG* macros with PGIS_DEBUG*, to avoid clashes with postgresql DEBUG
 *
 * Revision 1.16  2005/01/18 09:32:03  strk
 * Changed unparse_WKB interface to take an output size pointer and an HEXFORM
 * specifier. Reworked code in wktunparse to use function pointers.
 *
 * Revision 1.15  2004/12/21 15:19:01  strk
 * Canonical binary reverted back to EWKB, now supporting SRID inclusion.
 *
 * Revision 1.14  2004/12/17 11:08:53  strk
 * Moved getMachineEndian from parser to liblwgeom.{h,c}.
 * Added XDR and NDR defines.
 * Fixed all usage of them.
 *
 * Revision 1.13  2004/10/25 12:27:33  strk
 * Removed useless network type includes,
 * Added param.h include for BYTE_ORDER defines under win32.
 *
 * Revision 1.12  2004/10/21 19:48:34  strk
 * Stricter syntax fixes. Reported by Sébastien NICAISE <snicaise@iciatechnologies.com>
 *
 * Revision 1.11  2004/10/15 07:35:41  strk
 * Fixed a bug introduced by me (byteorder skipped for inner geoms in WKB)
 *
 * Revision 1.10  2004/10/11 14:03:33  strk
 * Added endiannes specification to unparse_WKB, AsBinary, lwgeom_to_wkb.
 *
 ******************************************************************/
