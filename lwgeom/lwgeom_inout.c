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
#include "stringBuffer.h"


//#define DEBUG

#include "lwgeom_pg.h"
#include "wktparse.h"

void elog_ERROR(const char* string);


extern unsigned char	parse_hex(char *str);
extern void deparse_hex(unsigned char str, unsigned char *result);
extern char *parse_lwgeom_wkt(char *wkt_input);


// 3d or 4d.  There is NOT a (x,y,m) point type!!!
#define WKB3DOFFSET 0x80000000
#define WKB4DOFFSET 0x40000000

Datum LWGEOMFromWKB(PG_FUNCTION_ARGS);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS);

Datum LWGEOM_in(PG_FUNCTION_ARGS);
Datum LWGEOM_out(PG_FUNCTION_ARGS);

// needed for OGC conformance

Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_getBBOX(PG_FUNCTION_ARGS);


Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS);


// included here so we can be independent from postgis
// WKB structure  -- exactly the same as TEXT
typedef struct Well_known_bin {
    int32 size;             // total size of this structure
    unsigned char  data[1]; //THIS HOLD VARIABLE LENGTH DATA
} WellKnownBinary;





// LWGEOM_in(cstring)
// format is '[SRID=#;]wkt|wkb'
//  LWGEOM_in( 'SRID=99;POINT(0 0)')
//  LWGEOM_in( 'POINT(0 0)')            --> assumes SRID=-1
//  LWGEOM_in( 'SRID=99;0101000000000000000000F03F000000000000004')
//  returns a LWGEOM object
PG_FUNCTION_INFO_V1(LWGEOM_in);
Datum LWGEOM_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
 	char *semicolonLoc,start;

	//determine if its WKB or WKT

	semicolonLoc = strchr(str,';');
	if (semicolonLoc == NULL)
	{
		start=str[0];
	}
	else
	{
		start=semicolonLoc[1]; // one in
	}

	// this is included just for redundancy (new parser can handle wkt and wkb)

	if ( ( (start >= '0') &&  (start <= '9') ) ||
		( (start >= 'A') &&  (start <= 'F') ))
	{
		//its WKB
		//PG_RETURN_POINTER(parse_lwgeom_serialized_form(str));
		PG_RETURN_POINTER( parse_lwgeom_wkt(str) );   // this function handles wkt and wkb (in hex-form)
	}
	else if ( (start == 'P') || (start == 'L') || (start == 'M') ||
		(start == 'G') || (start == 'p') || (start == 'l') ||
		(start == 'm') || (start == 'g'))
	{
		// its WKT
		// this function handles wkt and wkb (in hex-form)
		PG_RETURN_POINTER( parse_lwgeom_wkt(str) );
	}
	else
	{
		elog(ERROR,"couldnt determine if input lwgeom is WKB or WKT");
		PG_RETURN_NULL();
	}
}


// LWGEOM_out(lwgeom) --> cstring
// output is 'SRID=#;<wkb in hex form>'
// ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
// WKB is machine endian
// if SRID=-1, the 'SRID=-1;' will probably not be present.
PG_FUNCTION_INFO_V1(LWGEOM_out);
Datum LWGEOM_out(PG_FUNCTION_ARGS)
{
	char *lwgeom = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result = unparse_WKB(lwgeom,palloc_fn,free_fn);

	PG_RETURN_CSTRING(result);
}



// LWGEOMFromWKB(wkb,  [SRID] )
// NOTE: wkb is in *binary* not hex form.
PG_FUNCTION_INFO_V1(LWGEOMFromWKB);
Datum LWGEOMFromWKB(PG_FUNCTION_ARGS)
{
		WellKnownBinary   *wkb_input = (WellKnownBinary *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char   *wkb_srid_hexized;
		int    size_result,size_header;
		int    SRID = -1;
		char	sridText[100];
		char	*loc;
		char	*lwgeom;
		int t;


				if (  ( PG_NARGS()>1) && ( ! PG_ARGISNULL(1) ))
					SRID = PG_GETARG_INT32(1);
				else
					SRID = -1;

 //       elog(NOTICE,"LWGEOMFromWKB: entry with SRID=%i",SRID);
	elog(NOTICE,"wkb => lwgeom CONVERSION");


			// convert WKB to hexized WKB string




	    size_header = sprintf(sridText,"SRID=%i;",SRID);
	    size_result = size_header +  2*(wkb_input->size -4) + 1;  //SRID text size + wkb size (+1 = NULL term)

	    wkb_srid_hexized = palloc(size_result);
	    wkb_srid_hexized[0] = 0; // empty
	    strcpy(wkb_srid_hexized, sridText);
	    loc = wkb_srid_hexized + size_header; // points to null in "SRID=#;"

	    for (t=0; t< (wkb_input->size -4); t++)
		{
				deparse_hex( ((unsigned char *) wkb_input)[4 + t], &loc[t*2]);
		}

	    wkb_srid_hexized[size_result-1] = 0; // null term

//elog(NOTICE,"size_header = %i",size_header);
//elog(NOTICE,"size_result = %i", size_result);
//elog(NOTICE,"LWGEOMFromWKB :: '%s'", wkb_srid_hexized);

		lwgeom =  parse_lwgeom_wkt(wkb_srid_hexized);

		pfree(wkb_srid_hexized);

		PG_RETURN_POINTER(lwgeom);
}

// WKBFromLWGEOM(lwgeom) --> wkb
// this will have no 'SRID=#;'
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
	char *lwgeom_input = (char *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	// SRID=#;<hexized wkb>
	char *hexized_wkb_srid = unparse_WKB(lwgeom_input,palloc_fn,free_fn);

	char *hexized_wkb; // hexized_wkb_srid w/o srid
	char *result; //wkb

	int len_hexized_wkb;
	int			size_result;
	char		*semicolonLoc;
		int t;

//elog(NOTICE, "in WKBFromLWGEOM with WKB = '%s'", hexized_wkb_srid);
	elog(NOTICE,"lwgeom => wkb CONVERSION");

		hexized_wkb = hexized_wkb_srid;
		semicolonLoc = strchr(hexized_wkb_srid,';');


				if (semicolonLoc != NULL)
				{
					hexized_wkb = (semicolonLoc+1);
				}

//elog(NOTICE, "in WKBFromLWGEOM with WKB (with no 'SRID=#;' = '%s'", hexized_wkb);


		len_hexized_wkb = strlen(hexized_wkb);
		size_result = len_hexized_wkb/2 + 4;
		result = palloc(size_result);

		memcpy(result, &size_result,4); // size header



			// have a hexized string, want to make it binary

		for (t=0; t< (len_hexized_wkb/2); t++)
		{
			((unsigned char *) result +4)[t] = 	parse_hex(  hexized_wkb + (t*2) );
		}

		pfree(hexized_wkb_srid);

		PG_RETURN_POINTER(result);
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

// puts a bbox inside the geometry
PG_FUNCTION_INFO_V1(LWGEOM_addBBOX);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = (LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *result;
	BOX2DFLOAT4	box;
	unsigned char	old_type;
	int		size;


//elog(NOTICE,"in LWGEOM_addBBOX");

	if (lwgeom_hasBBOX( lwgeom->type ) )
	{
//elog(NOTICE,"LWGEOM_addBBOX  -- already has bbox");
		// easy - already has one.  Just copy!
		result = palloc (lwgeom->size);
		memcpy(result, lwgeom, lwgeom->size);
		PG_RETURN_POINTER(result);
	}

//elog(NOTICE,"LWGEOM_addBBOX  -- giving it a bbox");

	//construct new one
	getbox2d_p(SERIALIZED_FORM(lwgeom), &box);
	old_type = lwgeom->type;

	size = lwgeom->size+sizeof(BOX2DFLOAT4);

	result = palloc(size);// 16 for bbox2d

	memcpy(result,&size,4); // size
	result->type = lwgeom_makeType_full(lwgeom_ndims(old_type),
		lwgeom_hasSRID(old_type), lwgeom_getType(old_type), 1);
	// copy in bbox
	memcpy(result->data, &box, sizeof(BOX2DFLOAT4));

//elog(NOTICE,"LWGEOM_addBBOX  -- about to copy serialized form");
	// everything but the type and length
	memcpy(result->data+sizeof(BOX2DFLOAT4), lwgeom->data, lwgeom->size-5);

	PG_RETURN_POINTER(result);
}


//for the wkt parser

void elog_ERROR(const char* string)
{
	elog(ERROR,string);
}

// parse WKT input
// parse_WKT_lwgeom(TEXT) -> LWGEOM
PG_FUNCTION_INFO_V1(parse_WKT_lwgeom);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS)
{
		const char   *wkt_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));  // text
		char   		 *serialized_form;  //with length
		char		 *wkt;
		int			  wkt_size ;



		wkt_size = (*(int*) wkt_input) -4;  // actual letters

		wkt = palloc( wkt_size+1); //+1 for null
		memcpy(wkt, wkt_input+4, wkt_size );
		wkt[wkt_size] = 0; // null term


//elog(NOTICE,"in parse_WKT_lwgeom");
//elog(NOTICE,"in parse_WKT_lwgeom with input: '%s'",wkt);

		//serialized_form = parse_WKT((const char *)wkt,(allocator) malloc, (report_error)elog_ERROR);
		serialized_form = parse_lwg((const char *)wkt,(allocator) palloc_fn, (report_error)elog_ERROR);
//elog(NOTICE,"parse_WKT_lwgeom:: finished parse");
		pfree (wkt);

		if (serialized_form == NULL)
			elog(ERROR,"parse_WKT:: couldnt parse!");



	//	printBYTES(serialized_form+4,*((int*) serialized_form)-4);

	//	result = palloc( (*((int*) serialized_form)));
	//	memcpy(result, serialized_form, *((int*) serialized_form));
	//	free(serialized_form);

	//	PG_RETURN_POINTER(result);
		//PG_RETURN_NULL();

		PG_RETURN_POINTER(serialized_form);
}


char *parse_lwgeom_wkt(char *wkt_input)
{
	char *serialized_form = parse_lwg((const char *)wkt_input,(allocator) palloc_fn, (report_error)elog_ERROR);


	//elog(NOTICE,"parse_lwgeom_wkt with %s",wkt_input);

	if (serialized_form == NULL)
		elog(ERROR,"parse_WKT:: couldnt parse!");

	return serialized_form;

	//result = palloc( (*((int*) serialized_form)));
	//memcpy(result, serialized_form, *((int*) serialized_form));
	//free(serialized_form);
	//return result;
}







