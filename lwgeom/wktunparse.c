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
/* TO get byte order */
#include <sys/types.h>
#include <sys/param.h>

#include "liblwgeom.h"
#include "wktparse.h"


/*-- Typedefs ---------------------------------------------- */

typedef unsigned long int4;
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
uchar* output_collection_2(uchar* geom,int suppress);
uchar* output_multipoint(uchar* geom,int suppress);

void write_wkb_hex_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_bin_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_bin_flip_bytes(uchar* ptr, unsigned int cnt, size_t size);
void write_wkb_hex_flip_bytes(uchar* ptr, unsigned int cnt, size_t size);

void write_wkb_int(int i);
uchar* output_wkb_collection(uchar* geom,outwkbfunc func);
uchar* output_wkb_collection_2(uchar* geom);
uchar* output_wkb_point(uchar* geom);
uchar* output_wkb(uchar* geom);

/*-- Globals ----------------------------------------------- */

static int dims;
static allocator local_malloc;
static freeor local_free;
static char*  out_start;
static char*  out_pos;
static int len;
static int lwgi;
static uchar endianbyte;
void (*write_wkb_bytes)(uchar* ptr,unsigned int cnt,size_t size);

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

uchar *
output_collection_2(uchar* geom,int suppress)
{
	return output_collection(geom,output_point,suppress);
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
			geom = output_collection(geom,output_point,0);
			break;
		case POLYGONTYPE:
			if ( supress < 2 )
			{
				if (writeM) write_str("POLYGONM");
				else write_str("POLYGON");
			}
			geom = output_collection(geom,output_collection_2,0);
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
		case MULTIPOLYGONTYPE:
			if ( supress < 2 )
			{
				if (writeM) write_str("MULTIPOLYGONM");
				else write_str("MULTIPOLYGON");
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
			geom =output_collection(geom,output_collection_2,0);
			lwgi--;
			break;
	}
	return geom;
}

char *
unparse_WKT(uchar* serialized, allocator alloc, freeor free)
{

	if (serialized==NULL)
		return NULL;

	local_malloc=alloc;
	local_free=free;
	len = 128;
	out_start = out_pos = alloc(len);
	lwgi=0;

	output_wkt(serialized, 0);

	return out_start;
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

uchar *
output_wkb_collection(uchar* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
#ifdef PGIS_DEBUG
	lwnotice("output_wkb_collection: %d iterations loop", cnt);
#endif
	write_wkb_int(cnt);
	while(cnt--) geom=func(geom);
	return geom;
}

uchar *
output_wkb_collection_2(uchar* geom){
	return output_wkb_collection(geom,output_wkb_point);
}


uchar *
output_wkb(uchar* geom)
{
	unsigned char type=*geom++;
	int4 wkbtype;

	dims = TYPE_NDIMS(type); 
#ifdef PGIS_DEBUG
	lwnotice("output_wkb: dims set to %d", dims);
#endif

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
			geom=output_wkb_collection(geom,output_wkb_point);
			break;
		case POLYGONTYPE:
			geom=output_wkb_collection(geom,output_wkb_collection_2);
			break;
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
			geom = output_wkb_collection(geom,output_wkb_collection_2);
			lwgi--;
			break;
	}
	return geom;
}

char *
unparse_WKB(uchar* serialized, allocator alloc, freeor free, char endian, size_t *outsize, uchar hex)
{
#ifdef PGIS_DEBUG
	lwnotice("unparse_WKB(%p,...) called", serialized);
#endif

	if (serialized==NULL)
		return NULL;

	local_malloc=alloc;
	local_free=free;
	len = 128;
	out_start = out_pos = alloc(len);
	lwgi=0;

	if ( endian == -1 )
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

	if ( outsize ) *outsize = (out_pos-out_start);

	return out_start;
}


/******************************************************************
 * $Log$
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
