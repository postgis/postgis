/*
 * Written by Ralph Mason ralph.mason<at>telogis.com
 *
 * Copyright Telogis 2004
 * www.telogis.com
 *
 * $Id$
 */


#include "wktparse.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
//TO get byte order
#include <sys/types.h>
#include <sys/param.h>
//#include <netinet/in.h>
//#include <netinet/in_systm.h>
//#include <netinet/ip.h>

#include "liblwgeom.h"

static int endian_check_int = 1; // dont modify this!!!

#define LITTLE_ENDIAN_CHECK 1
static unsigned int getMachineEndian()
{
	// 0 = big endian, 1 = little endian
	if ( *((char *) &endian_check_int) )
	{
		return LITTLE_ENDIAN;
	}
	else
	{
		return BIG_ENDIAN;
	}
}

//-- Typedefs ----------------------------------------------

typedef unsigned long int4;
typedef byte* (*outfunc)(byte*,int);
typedef byte* (*outwkbfunc)(byte*);

//-- Prototypes --------------------------------------------

void ensure(int chars);
void to_end(void);
void write_str(const char* str);
void write_double(double val);
void write_int(int i);
int4 read_int(byte** geom);
double read_double(byte** geom);
byte* output_point(byte* geom,int supress);
byte* output_single(byte* geom,int supress);
byte* output_collection(byte* geom,outfunc func,int supress);
byte* output_collection_2(byte* geom,int suppress);
byte* output_multipoint(byte* geom,int suppress);
void write_wkb_bytes(byte* ptr,unsigned int cnt,size_t size);
void write_wkb_int(int i);
byte* output_wkb_collection(byte* geom,outwkbfunc func);
byte* output_wkb_collection_2(byte* geom);
byte* output_wkb_point(byte* geom);
byte* output_wkb(byte* geom);

//-- Globals -----------------------------------------------

static int dims;
static allocator local_malloc;
static freeor local_free;
static char*  out_start;
static char*  out_pos;
static int len;
static int lwgi;
static int flipbytes;
byte endianbyte;

//----------------------------------------------------------



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

void to_end(){
	while(*out_pos){
		out_pos++;
	}
}

void write_str(const char* str){
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
read_int(byte** geom)
{
	int4 ret;
#ifdef SHRINK_INTS
	if ( getMachineEndian() == LITTLE_ENDIAN_CHECK ){
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
	if ( getMachineEndian() == LITTLE_ENDIAN_CHECK ){
		ret >>= 1;
	}
#endif

	(*geom)+=4;
	return ret;
}

double round(double);

double read_double(byte** geom){
	if (lwgi){
		double ret = *((int4*)*geom);
		ret /= 0xb60b60;
		(*geom)+=4;
		return ret-180.0;
	}
	else{
		//double ret = *((double*)*geom);
		double ret;
 		memcpy(&ret, *geom, 8);
		(*geom)+=8;
		return ret;
	}
}
           
byte *
output_point(byte* geom,int supress)
{
	int i;

	for( i = 0 ; i < dims ; i++ ){
		write_double(read_double(&geom));
		if (i +1 < dims )
			write_str(" ");
	}
	return geom;
}

byte *
output_single(byte* geom,int supress)
{
	write_str("(");
	geom=output_point(geom,supress);
	write_str(")");
	return geom;
}

byte *
output_collection(byte* geom,outfunc func,int supress)
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

byte *
output_collection_2(byte* geom,int suppress)
{
	return output_collection(geom,output_point,suppress);
}

byte *output_wkt(byte* geom, int supress);

/* special case for multipoint to supress extra brackets */
byte *output_multipoint(byte* geom,int suppress){
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

// Suppress=0 // write TYPE, M, coords
// Suppress=1 // write TYPE, coords 
// Suppress=2 // write only coords
byte *
output_wkt(byte* geom, int supress)
{

	unsigned type=*geom++;
	char writeM=0;
	dims = TYPE_NDIMS(type); //((type & 0x30) >> 4)+2;

	if ( ! supress && !TYPE_HASZ(type) && TYPE_HASM(type) ) writeM=1;


	//Skip the bounding box if there is one
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
unparse_WKT(byte* serialized, allocator alloc, freeor free)
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

void
write_wkb_bytes(byte* ptr, unsigned int cnt, size_t size)
{
	unsigned int bc; // byte count

	ensure(cnt*2*size);

	while(cnt--){
		if (flipbytes)
		{
			for(bc=size; bc; bc--)
			{
				*out_pos++ = outchr[ptr[bc-1]>>4];
				*out_pos++ = outchr[ptr[bc-1]&0x0F];
			}
		}
		else
		{
			for(bc=0; bc<size; bc++)
			{
				*out_pos++ = outchr[ptr[bc]>>4];
				*out_pos++ = outchr[ptr[bc]&0x0F];
			}
		}
		ptr+=size;
	}
}

byte *
output_wkb_point(byte* geom)
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
	write_wkb_bytes((byte*)&i,1,4);
}

byte *
output_wkb_collection(byte* geom,outwkbfunc func)
{
	int cnt = read_int(&geom);
#ifdef DEBUG
	lwnotice("output_wkb_collection: %d iterations loop", cnt);
#endif
	write_wkb_int(cnt);
	while(cnt--) geom=func(geom);
	return geom;
}

byte *
output_wkb_collection_2(byte* geom){
	return output_wkb_collection(geom,output_wkb_point);
}


byte *
output_wkb(byte* geom)
{
	unsigned char type=*geom++;
	int4 wkbtype;

	dims = TYPE_NDIMS(type); 
#ifdef DEBUG
	lwnotice("output_wkb: dims set to %d", dims);
#endif

	//Skip the bounding box
	if ( TYPE_HASBBOX(type) ) { 
		geom+=16;
	}

	if ( TYPE_HASSRID(type) ) {
		write_str("SRID=");
		write_int(read_int(&geom));
		write_str(";");
	}

	//type&=0x0f;
	wkbtype = TYPE_GETTYPE(type); 

	if ( TYPE_HASZ(type) )
		 wkbtype |= WKBZOFFSET;
	if ( TYPE_HASM(type) )
		 wkbtype |= WKBMOFFSET;

	// Write byteorder (as from WKB specs...)
	write_wkb_bytes(&endianbyte,1,1);

	write_wkb_int(wkbtype);

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
unparse_WKB(byte* serialized, allocator alloc, freeor free, unsigned int endian)
{

#ifdef DEBUG
	lwnotice("unparse_WKB(%p,...) called", serialized);
#endif

	if (serialized==NULL)
		return NULL;

	local_malloc=alloc;
	local_free=free;
	len = 128;
	out_start = out_pos = alloc(len);
	lwgi=0;

	if ( endian == -1 ) endian = getMachineEndian();

	if ( endian == LITTLE_ENDIAN) endianbyte=1;
	else endianbyte=0;

	if ( endian != getMachineEndian() ) flipbytes = 1;
	else flipbytes = 0;

	output_wkb(serialized);
	ensure(1);
	*out_pos=0;

	return out_start;
}


/******************************************************************
 * $Log$
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
