#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "fmgr.h"
#include "utils/elog.h"


#include "lwgeom.h"



#define DEBUG





extern char *wkb_to_lwgeom(char *wkb, int SRID,int *size);
extern char *lwgeom_to_wkb(char *serialized_form,int *wkblength,char desiredWKBEndian);
extern void swap_char(char	*a,char *b);
extern void	flip_endian_double(char 	*d);
extern void		flip_endian_int32(char		*i);
extern char wkb_dims(uint32 type);
extern char wkb_simpletype (uint32 type);
extern uint32 constructWKBType(int simple_type, char dims);
extern bool requiresflip(char WKBendianflag);
extern void flipPoints(char *pts, int npoints, char dims);
extern uint32 constructWKBType(int simple_type, char dims);

extern LWPOINT *wkb_point_to_lwpoint(char *wkb,int SRID);
extern LWLINE *wkb_line_to_lwline(char *wkb,int SRID);
extern LWPOLY *wkb_poly_to_lwpoly(char *wkb,int SRID);

extern char *lwline_to_wkb(LWLINE *line, char desiredWKBEndian, int *wkbsize);
extern char *lwpoint_to_wkb(LWPOINT *point, char desiredWKBEndian, int *wkbsize);
extern char *lwpoly_to_wkb(LWPOLY *poly, char desiredWKBEndian, int *wkbsize);


extern unsigned char	parse_hex(char *str);
extern void deparse_hex(unsigned char str, unsigned char *result);


// 3d or 4d.  There is NOT a (x,y,m) point type!!!
#define WKB3DOFFSET 0x80000000
#define WKB4DOFFSET 0x40000000

Datum LWGEOMFromWKB(PG_FUNCTION_ARGS);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS);


Datum LWGEOM_in(PG_FUNCTION_ARGS);
Datum LWGEOM_out(PG_FUNCTION_ARGS);


// WKB structure  -- exactly the same as TEXT
typedef struct Well_known_bin {
    int32 size;    // total size of this structure
    unsigned char  data[1]; //THIS HOLD VARIABLE LENGTH DATA
} WellKnownBinary;



PG_FUNCTION_INFO_V1(LWGEOM_in);
Datum LWGEOM_in(PG_FUNCTION_ARGS)
{
	    char                    *str = PG_GETARG_CSTRING(0);
	    WellKnownBinary         *wkb;
	    char 					*result,*lwgeom;
        int                     size;
        int                     t;
        int                     input_str_len;

        int SRID = -1; //default (change)

	input_str_len = strlen(str);

	if  ( ( ( (int)(input_str_len/2.0) ) *2.0) != input_str_len)
	{
			elog(ERROR,"LWGEOM_in parser - should be even number of characters!");
			PG_RETURN_NULL();
	}

	if (strspn(str,"0123456789ABCDEF") != strlen(str) )
	{
			elog(ERROR,"WKB_in parser - input contains bad characters.  Should only have '0123456789ABCDEF'!");
			PG_RETURN_NULL();
	}
	size = (input_str_len/2) + 4;
	wkb = (WellKnownBinary *) palloc(size);
	wkb->size = size;

	for (t=0;t<input_str_len/2;t++)
	{
			((unsigned char *)wkb)[t+4] = parse_hex( &str[t*2]) ;
	}

	//have WKB string (and we can safely modify it)
			lwgeom = wkb_to_lwgeom(((char *) wkb)+4, SRID,&size);

			pfree(wkb); // no longer referenced!

			result = palloc(size+4);

			memcpy(result+4, lwgeom, size);
			size+=4;
			memcpy(result, &size, 4);

			pfree(lwgeom);

		PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_out);
Datum LWGEOM_out(PG_FUNCTION_ARGS)
{
		char		        *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char				*result;
		int					size_result;
		int					t;
		int					size;

		char *wkb = lwgeom_to_wkb(lwgeom+4,&size,1); // 0=XDR, 1=NDR

		size_result = size *2 +1; //+1 for null char
		result = palloc (size_result);
		result[size_result-1] = 0; //null terminate

		for (t=0; t< size; t++)
		{
			deparse_hex( ((unsigned char *) wkb)[t], &result[t*2]);
		}

		pfree(wkb);

		PG_RETURN_CSTRING(result);
}



// LWGEOMFromWKB(wkb,  [SRID] )
PG_FUNCTION_INFO_V1(LWGEOMFromWKB);
Datum LWGEOMFromWKB(PG_FUNCTION_ARGS)
{
		char   *wkb_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int    SRID = -1;
		int    size;
		char *lwgeom;
		char * result;
		char	*wkb_copy;


				if (  ( PG_NARGS()>1) && ( ! PG_ARGISNULL(1) ))
					SRID = PG_GETARG_INT32(1);
				else
					SRID = -1;

#ifdef DEBUG
        elog(NOTICE,"LWGEOMFromWKB: entry with SRID=%i\n",SRID);
#endif

					// need to do this because there might be a bunch of endian flips!
		wkb_copy = palloc( *((int32 *) wkb_input));
		memcpy(wkb_copy, wkb_input+4, *((int32 *) wkb_input) -4);

		lwgeom = wkb_to_lwgeom(wkb_copy, SRID,&size);

		pfree(wkb_copy); // no longer referenced!

		result = palloc(size+4);

		memcpy(result+4, lwgeom, size);
		size+=4;
		memcpy(result, &size, 4);

		pfree(lwgeom);

		PG_RETURN_POINTER(result);
}

// WKBFromLWGEOM(wkb)
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
		char   *lwgeom_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int    size;
		char *wkb = lwgeom_to_wkb(lwgeom_input+4,&size,1); // 0=XDR, 1=NDR

		char *result = palloc(size+4);

		memcpy(result+4, wkb, size);
		size+=4;
		memcpy(result, &size, 4);
		pfree(wkb);

		PG_RETURN_POINTER(result);
}


//given a wkb (with 3d and 4d extension)
// create the serialized form of an equivelent LWGEOM
// size of the lwgeom form is also returned
//
// note - to actually serialize, you should put the first 4
// bytes as size (size +4), then the results of this function.
//
// because this function modifies the wkb (to make it the
// server's endian), you should give it a copy of the wkb.
// The underlying LWGEOM will point into this WKB, so
// be extreamly careful when you free it!
//
// also, wkb should point to the 1st wkb character; NOT
// the postgresql length int32.
char *wkb_to_lwgeom(char *wkb, int SRID,int *size)
{
	uint32 wkbtype;
	char   dims;
	bool need_flip =  requiresflip( wkb[0] );

	LWPOINT *pt;
	LWLINE  *line;
	LWPOLY  *poly;
	char    *multigeom = NULL;
	char	*result = NULL;
	int simpletype;

	if (need_flip)
		flip_endian_int32(wkb+1);

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

#ifdef DEBUG
        elog(NOTICE,"wkb_to_lwgeom: entry with SRID=%i, dims=%i, simpletype=%i\n",SRID,(int) dims, (int) simpletype);
#endif


	switch (simpletype)
	{
		case POINTTYPE:
						pt = wkb_point_to_lwpoint(wkb, SRID);
						result  = lwpoint_serialize(pt);
						*size = lwpoint_findlength(result);
						pfree_point(pt);
						break;
		case LINETYPE:
						line = wkb_line_to_lwline(wkb, SRID);
	printLWLINE(line);
	elog(NOTICE,"have line with %i points",line->points->npoints);
						result  = lwline_serialize(line);
	elog(NOTICE,"serialized it");
						*size = lwline_findlength(result);
	elog(NOTICE,"serialized length=%i",*size);
						pfree_line(line);
						break;
		case POLYGONTYPE:
						poly = wkb_poly_to_lwpoly(wkb, SRID);
	printLWPOLY(poly);
						result  = lwpoly_serialize(poly);
	elog(NOTICE,"serialized polygon");
						*size = lwpoly_findlength(result);
	elog(NOTICE,"serialized polygon has length = %i",*size);
						pfree_polygon(poly);
						break;
		case MULTIPOINTTYPE:
						result = multigeom;
						break;
		case MULTILINETYPE:
						break;
		case MULTIPOLYGONTYPE:
						break;
		case COLLECTIONTYPE:
						break;
	}
#ifdef DEBUG
        elog(NOTICE,"wkb_to_lwgeom:returning");
#endif

	return result;
}

//NOTE: THIS CHANGES THE ENDIAN OF THE WKB!!
// we make sure the point is correctly endianed
// and make a LWPOINT that points into it.
// wkb --> point to the endian definition of the wkb point
LWPOINT *wkb_point_to_lwpoint(char *wkb,int SRID)
{
	uint32 wkbtype;
	char   dims;
	char   simpletype;
	POINTARRAY *pa;

	bool need_flip =  requiresflip( wkb[0] );
	if (need_flip)
		flip_endian_int32(wkb+1);

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

#ifdef DEBUG
        elog(NOTICE,"wkb_point_to_lwpoint: entry with SRID=%i, dims=%i, simpletype=%i\n",SRID,(int) dims, (int) simpletype);
#endif




	if (simpletype != POINTTYPE)
		elog(ERROR,"WKB::point parser - go wrong type");

	pa = pointArray_construct(wkb+5, dims, 1);



	return lwpoint_construct(dims, SRID, pa);
}

LWLINE *wkb_line_to_lwline(char *wkb,int SRID)
{
		uint32 wkbtype;
		char   dims;
		char   simpletype;
		POINTARRAY *pa;
		int npoints;

	bool need_flip =  requiresflip( wkb[0] );
	if (need_flip)
		flip_endian_int32(wkb+1); // type

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

	if (simpletype != LINETYPE)
		elog(ERROR,"WKB::line parser - go wrong type");

	if (need_flip)
		flip_endian_int32(wkb+5); // npoints

	npoints = get_uint32(wkb+5);

	if (need_flip)
		flipPoints(wkb+9,npoints,dims);

	pa = pointArray_construct(wkb+9, dims, npoints);
printPA(pa);
	return lwline_construct(dims, SRID, pa);
}

LWPOLY *wkb_poly_to_lwpoly(char *wkb,int SRID)
{
		uint32 wkbtype;
		char   dims;
		char   simpletype;
		POINTARRAY *pa;
		int npoints;
		POINTARRAY **rings;
		int nrings;
		int t;
		char *loc;
		int ptsize =16;

		bool need_flip =  requiresflip( wkb[0] );
		if (need_flip)
			flip_endian_int32(wkb+1); // type

		wkbtype = get_uint32(wkb+1);
		dims = wkb_dims(wkbtype);
		simpletype = wkb_simpletype(wkbtype);

		if (simpletype != POLYGONTYPE)
			elog(ERROR,"WKB::polygon parser - go wrong type");

		if (need_flip)
			flip_endian_int32(wkb+5); // nrings

		nrings = get_uint32(wkb+5);

		loc = wkb+9;

			//point size

		ptsize =16;
		if (dims == FOURDIMS)
		{
			ptsize = 32;
		}
		if (dims == THREEDIMS)
		{
			ptsize = 24;
        }


		rings = (POINTARRAY **) palloc(sizeof(POINTARRAY*) * nrings);

		for (t=0;t<nrings;t++)
		{

			if (need_flip)
				flip_endian_int32(loc); // npoints

			npoints = get_uint32(loc);

					// read a ring
			if (need_flip)
				flipPoints(loc+4,npoints,dims);
elog(NOTICE,"wkb_poly_to_lwpoly:: doing ring %i with %i points", t, npoints);
			pa = pointArray_construct(loc+4, dims, npoints);

			loc += 4;
			loc += npoints * ptsize;
			rings[t] = pa;
		}
		return lwpoly_construct(dims, SRID, nrings,rings);
}


// takes a lwgeom and converts it to an appropriate wkb
//
// length of the wkb is returned in wkblength (this doesnt
//  include the 4 bytes needed for the postgresql int32 length)
//
char *lwgeom_to_wkb(char *serialized_form,int *wkblength,char desiredWKBEndian)
{
	char simple_type = lwgeom_getType(serialized_form[0]);
	char *result = NULL;
	LWPOINT *pt;
	LWLINE *line;
	LWPOLY *poly;
	char   *multigeom = NULL;

#ifdef DEBUG
        elog(NOTICE,"lwgeom_to_wkb: entry with  simpletype=%i, dims=%i\n",(int) simple_type,(int)  lwgeom_is3d( serialized_form[0]) );
#endif


	switch (simple_type)
	{
		case POINTTYPE:
						pt = lwpoint_deserialize(serialized_form);
						result = lwpoint_to_wkb(pt, desiredWKBEndian, wkblength);
						pfree_point(pt);
						break;
		case LINETYPE:
						line = lwline_deserialize(serialized_form);
						result = lwline_to_wkb(line, desiredWKBEndian, wkblength);
						pfree_line(line);
						break;
		case POLYGONTYPE:
	elog(NOTICE,"lwgeom_to_wkb :: deserializing polygon");
						poly = lwpoly_deserialize(serialized_form);
	printLWPOLY(poly);
						result = lwpoly_to_wkb(poly, desiredWKBEndian, wkblength);
						pfree_polygon(poly );
						break;
		case MULTIPOINTTYPE:
						result  = multigeom;
						break;
		case MULTILINETYPE:
						break;
		case MULTIPOLYGONTYPE:
						break;
		case COLLECTIONTYPE:
						break;
	}
	return result;
}

char *lwpoint_to_wkb(LWPOINT *pt, char desiredWKBEndian, int *wkbsize)
{
	int ptsize = pointArray_ptsize(pt->point);
	char * result;
	uint32 wkbtype ;
	bool need_flip =  requiresflip( desiredWKBEndian );

#ifdef DEBUG
        elog(NOTICE,"lwpoint_to_wkb:  pa dims = %i\n", (int)pt->point->is3d );
#endif


	*wkbsize = 1+ 4+ ptsize; //endian, type, point

	result = palloc(*wkbsize);

	result[0] = desiredWKBEndian; //endian flag

	wkbtype = constructWKBType(POINTTYPE, pt->point->is3d);

#ifdef DEBUG
        elog(NOTICE,"lwpoint_to_wkb: entry with wkbtype=%i, pa dims = %i\n",wkbtype, (int)pt->point->is3d );
#endif



	memcpy(result+1, &wkbtype, 4);
	if (need_flip)
		flip_endian_int32(result+1);

	memcpy(result+5, pt->point->serialized_pointlist, pointArray_ptsize(pt->point) );
	if (need_flip)
		flipPoints(result+5, 1, pt->point->is3d);

	return result;
}

char *lwline_to_wkb(LWLINE *line, char desiredWKBEndian, int *wkbsize)
{
		int ptsize = pointArray_ptsize(line->points);
		char * result;
		uint32 wkbtype ;
		bool need_flip =  requiresflip( desiredWKBEndian );


printLWLINE(line);

		*wkbsize = 1+ 4+4+ line->points->npoints * ptsize; //endian, type, npoints, points

		result = palloc(*wkbsize);

		result[0] = desiredWKBEndian; //endian flag

		wkbtype = constructWKBType(LINETYPE, line->points->is3d);
		memcpy(result+1, &wkbtype, 4);
		if (need_flip)
			flip_endian_int32(result+1);

		memcpy(result+5, &line->points->npoints, 4);
		if (need_flip)
			flip_endian_int32(result+5);

		memcpy(result+9, line->points->serialized_pointlist, ptsize * line->points->npoints);
		if (need_flip)
			flipPoints(result+9, line->points->npoints, line->points->is3d);
		return result;
}

char *lwpoly_to_wkb(LWPOLY *poly, char desiredWKBEndian, int *wkbsize)
{
		int ptsize = pointArray_ptsize(poly->rings[0]);
		char * result;
		uint32 wkbtype ;
		bool need_flip =  requiresflip( desiredWKBEndian );
		int total_points =0;
		char *loc;
		int t;


		for (t=0;t<poly->nrings;t++)
		{
			total_points += poly->rings[t]->npoints;
		}

		*wkbsize = 1+ 4+ total_points * ptsize + 4* poly->nrings; //endian, type, all points, ring lengths

		result = palloc(*wkbsize);

		result[0] = desiredWKBEndian; //endian flag

		wkbtype = constructWKBType(POLYGONTYPE, poly->is3d);
		memcpy(result+1, &wkbtype, 4);  // type
		if (need_flip)
			flip_endian_int32(result+1);

		memcpy(result+5, &poly->nrings, 4);     // nrings
		if (need_flip)
			flip_endian_int32(result+5);

		loc = result+9;

		for (t=0;t<poly->nrings;t++)
		{
			int npoints =poly->rings[t]->npoints;
			memcpy( loc, &npoints, 4); // npoints
			if (need_flip)
				flip_endian_int32(loc);
			memcpy(loc+4, poly->rings[t]->serialized_pointlist, ptsize * npoints);
			if (need_flip)
				flipPoints(loc+4, npoints, poly->is3d);
			loc += 4+ ptsize * npoints;
		}
		return result;
}


bool requiresflip(char WKBendianflag)
{
	if (WKBendianflag == 1) // NDR
		return ( BYTE_ORDER != LITTLE_ENDIAN );
	if (WKBendianflag == 0) // xDR
		return ( BYTE_ORDER != BIG_ENDIAN );
	elog(ERROR,"WKB: endian flag isnt a 0 or 1.  WKB screwed?");
	return 0; //shouldnt get here
}

void swap_char(char	*a,char *b)
{
	char c;

	c = *a;
	*a=*b;
	*b=c;
}


void	flip_endian_double(char 	*d)
{
	swap_char(d+7, d);
	swap_char(d+6, d+1);
	swap_char(d+5, d+2);
	swap_char(d+4, d+3);
}

void		flip_endian_int32(char		*i)
{
	swap_char (i+3,i);
	swap_char (i+2,i+1);
}

// given a wkb type
// return twoDims, threeDims, or fourDims
char wkb_dims(uint32 type)
{
	if (type & 0x80000000)
		return THREEDIMS;
	if (type & 0x40000000)
		return FOURDIMS;
	return TWODIMS;
}


char wkb_simpletype (uint32 type)
{
	return type & 0x0F;
}

void flipPoints(char *pts, int npoints, char dims)
{
	int t;
	char *loc = pts;
	int size =16;

	if (dims == FOURDIMS)
	{
		size = 32;
	}
	if (dims == THREEDIMS)
	{
		size = 24;
	}

	for (t=0;t<npoints;t++)
	{
		flip_endian_double(loc);
		flip_endian_double(loc+8);
		if ( (dims == THREEDIMS)  || (dims == FOURDIMS) )
		{
			flip_endian_double(loc+16);
		}
		if (dims == FOURDIMS)
		{
			flip_endian_double(loc+24);
		}
		loc += size;
	}
}

uint32 constructWKBType(int simple_type, char dims)
{
	if (dims == TWODIMS)
		return simple_type;
	if (dims == THREEDIMS)
		return simple_type | 0x80000000;

	return simple_type | 0x40000000;
}


//given one byte, populate result with two byte representing
// the hex number
// ie deparse_hex( 255, mystr)
//		-> mystr[0] = 'F' and mystr[1] = 'F'
// no error checking done
void deparse_hex(unsigned char str, unsigned char *result)
{
	int	input_high;
	int  input_low;

	input_high = (str>>4);
	input_low = (str & 0x0F);

	switch (input_high)
	{
		case 0:
			result[0] = '0';
			break;
		case 1:
			result[0] = '1';
			break;
		case 2:
			result[0] = '2';
			break;
		case 3:
			result[0] = '3';
			break;
		case 4:
			result[0] = '4';
			break;
		case 5:
			result[0] = '5';
			break;
		case 6:
			result[0] = '6';
			break;
		case 7:
			result[0] = '7';
			break;
		case 8:
			result[0] = '8';
			break;
		case 9:
			result[0] = '9';
			break;
		case 10:
			result[0] = 'A';
			break;
		case 11:
			result[0] = 'B';
			break;
		case 12:
			result[0] = 'C';
			break;
		case 13:
			result[0] = 'D';
			break;
		case 14:
			result[0] = 'E';
			break;
		case 15:
			result[0] = 'F';
			break;
	}

	switch (input_low)
	{
		case 0:
			result[1] = '0';
			break;
		case 1:
			result[1] = '1';
			break;
		case 2:
			result[1] = '2';
			break;
		case 3:
			result[1] = '3';
			break;
		case 4:
			result[1] = '4';
			break;
		case 5:
			result[1] = '5';
			break;
		case 6:
			result[1] = '6';
			break;
		case 7:
			result[1] = '7';
			break;
		case 8:
			result[1] = '8';
			break;
		case 9:
			result[1] = '9';
			break;
		case 10:
			result[1] = 'A';
			break;
		case 11:
			result[1] = 'B';
			break;
		case 12:
			result[1] = 'C';
			break;
		case 13:
			result[1] = 'D';
			break;
		case 14:
			result[1] = 'E';
			break;
		case 15:
			result[1] = 'F';
			break;
	}
}


//given a string with at least 2 chars in it, convert them to
// a byte value.  No error checking done!
unsigned char	parse_hex(char *str)
{
	//do this a little brute force to make it faster

	unsigned char		result_high = 0;
	unsigned char		result_low = 0;

	switch (str[0])
	{
		case '0' :
			result_high = 0;
			break;
		case '1' :
			result_high = 1;
			break;
		case '2' :
			result_high = 2;
			break;
		case '3' :
			result_high = 3;
			break;
		case '4' :
			result_high = 4;
			break;
		case '5' :
			result_high = 5;
			break;
		case '6' :
			result_high = 6;
			break;
		case '7' :
			result_high = 7;
			break;
		case '8' :
			result_high = 8;
			break;
		case '9' :
			result_high = 9;
			break;
		case 'A' :
			result_high = 10;
			break;
		case 'B' :
			result_high = 11;
			break;
		case 'C' :
			result_high = 12;
			break;
		case 'D' :
			result_high = 13;
			break;
		case 'E' :
			result_high = 14;
			break;
		case 'F' :
			result_high = 15;
			break;
	}
	switch (str[1])
	{
		case '0' :
			result_low = 0;
			break;
		case '1' :
			result_low = 1;
			break;
		case '2' :
			result_low = 2;
			break;
		case '3' :
			result_low = 3;
			break;
		case '4' :
			result_low = 4;
			break;
		case '5' :
			result_low = 5;
			break;
		case '6' :
			result_low = 6;
			break;
		case '7' :
			result_low = 7;
			break;
		case '8' :
			result_low = 8;
			break;
		case '9' :
			result_low = 9;
			break;
		case 'A' :
			result_low = 10;
			break;
		case 'B' :
			result_low = 11;
			break;
		case 'C' :
			result_low = 12;
			break;
		case 'D' :
			result_low = 13;
			break;
		case 'E' :
			result_low = 14;
			break;
		case 'F' :
			result_low = 15;
			break;
	}
	return (unsigned char) ((result_high<<4) + result_low);
}

