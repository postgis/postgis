#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "fmgr.h"
#include "utils/elog.h"
#if USE_VERSION > 73
# include "lib/stringinfo.h" // for binary input
#endif


#include "liblwgeom.h"
#include "stringBuffer.h"


//#define DEBUG 1

#include "lwgeom_pg.h"
#include "wktparse.h"

void elog_ERROR(const char* string);


Datum LWGEOM_in(PG_FUNCTION_ARGS);
Datum LWGEOM_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS);
#if USE_VERSION > 73
Datum LWGEOM_recv(PG_FUNCTION_ARGS);
Datum LWGEOM_send(PG_FUNCTION_ARGS);
#endif


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
//  returns a PG_LWGEOM object
PG_FUNCTION_INFO_V1(LWGEOM_in);
Datum LWGEOM_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
 	char *semicolonLoc,start;
	PG_LWGEOM *ret;

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

	// will handle both EWKB and EWKT
	ret = (PG_LWGEOM *)parse_lwgeom_wkt(str);

	if ( is_worth_caching_pglwgeom_bbox(ret) )
	{
		ret = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(ret)));
	}

	PG_RETURN_POINTER(ret);
}


// LWGEOM_out(lwgeom) --> cstring
// output is 'SRID=#;<wkb in hex form>'
// ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
// WKB is machine endian
// if SRID=-1, the 'SRID=-1;' will probably not be present.
PG_FUNCTION_INFO_V1(LWGEOM_out);
Datum LWGEOM_out(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *result;

	init_pg_func();

	lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	result = unparse_WKB(SERIALIZED_FORM(lwgeom),lwalloc,lwfree,-1);

	PG_RETURN_CSTRING(result);
}

// LWGEOM_to_text(lwgeom) --> text
// output is 'SRID=#;<wkb in hex form>'
// ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
// WKB is machine endian
// if SRID=-1, the 'SRID=-1;' will probably not be present.
PG_FUNCTION_INFO_V1(LWGEOM_to_text);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *result;
	text *text_result;

	init_pg_func();

	lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	result = unparse_WKB(SERIALIZED_FORM(lwgeom),lwalloc,lwfree,-1);

	text_result = palloc(strlen(result)+VARHDRSZ);
	memcpy(VARDATA(text_result),result,strlen(result));
	VARATT_SIZEP(text_result) = strlen(result)+VARHDRSZ;
	pfree(result);

	PG_RETURN_POINTER(text_result);
}

// LWGEOMFromWKB(wkb,  [SRID] )
// NOTE: wkb is in *binary* not hex form.
//
// NOTE: this function is unoptimized, as it convert binary
// form to hex form and then calls the unparser ...
//
PG_FUNCTION_INFO_V1(LWGEOMFromWKB);
Datum LWGEOMFromWKB(PG_FUNCTION_ARGS)
{
	WellKnownBinary *wkb_input;
	char   *wkb_srid_hexized;
	int    size_result,size_header;
	int    SRID = -1;
	char	sridText[100];
	char	*loc;
	PG_LWGEOM *lwgeom;
	int t;

	wkb_input = (WellKnownBinary *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (  ( PG_NARGS()>1) && ( ! PG_ARGISNULL(1) ))
		SRID = PG_GETARG_INT32(1);
	else
		SRID = -1;

#ifdef DEBUG
	elog(NOTICE,"LWGEOMFromWKB: entry with SRID=%i",SRID);
#endif

	// convert WKB to hexized WKB string

	size_header = sprintf(sridText,"SRID=%i;",SRID);
	//SRID text size + wkb size (+1 = NULL term)
	size_result = size_header +  2*(wkb_input->size-4) + 1; 

	wkb_srid_hexized = palloc(size_result);
	wkb_srid_hexized[0] = 0; // empty
	strcpy(wkb_srid_hexized, sridText);
	loc = wkb_srid_hexized + size_header; // points to null in "SRID=#;"

	for (t=0; t< (wkb_input->size -4); t++)
	{
		deparse_hex( ((unsigned char *) wkb_input)[4 + t], &loc[t*2]);
	}

	wkb_srid_hexized[size_result-1] = 0; // null term

#ifdef DEBUG
	elog(NOTICE,"size_header = %i",size_header);
	elog(NOTICE,"size_result = %i", size_result);
	elog(NOTICE,"LWGEOMFromWKB :: '%s'", wkb_srid_hexized);
#endif

	lwgeom = (PG_LWGEOM *)parse_lwgeom_wkt(wkb_srid_hexized);

	pfree(wkb_srid_hexized);

	if ( is_worth_caching_pglwgeom_bbox(lwgeom) )
	{
		lwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(lwgeom)));
	}

#ifdef DEBUG
	elog(NOTICE, "LWGEOMFromWKB returning %s", unparse_WKB(SERIALIZED_FORM(lwgeom), pg_alloc, pg_free, -1));
#endif

	PG_RETURN_POINTER(lwgeom);
}

// WKBFromLWGEOM(lwgeom) --> wkb
// this will have no 'SRID=#;'
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom_input; // SRID=#;<hexized wkb>
	char *hexized_wkb_srid;
	char *hexized_wkb; // hexized_wkb_srid w/o srid
	char *result; //wkb
	int len_hexized_wkb;
	int size_result;
	char *semicolonLoc;
	int t;
	text *type;
	unsigned int byteorder=-1;

	init_pg_func();

	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		type = PG_GETARG_TEXT_P(1);
		if (VARSIZE(type) < 7)  
		{
			elog(ERROR,"asbinary(geometry, <type>) - type should be 'XDR' or 'NDR'.  type length is %i",VARSIZE(type) -4);
			PG_RETURN_NULL();
		}

		if  ( ! strncmp(VARDATA(type), "xdr", 3) ||
			! strncmp(VARDATA(type), "XDR", 3) )
		{
			byteorder = XDR;
		}
		else
		{
			byteorder = NDR;
		}
	}

	lwgeom_input = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	hexized_wkb_srid = unparse_WKB(SERIALIZED_FORM(lwgeom_input),
		lwalloc, lwfree, byteorder);

//elog(NOTICE, "in WKBFromLWGEOM with WKB = '%s'", hexized_wkb_srid);

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
		((unsigned char *) result +4)[t] = parse_hex(  hexized_wkb + (t*2) );
	}

	pfree(hexized_wkb_srid);

	PG_RETURN_POINTER(result);
}

// puts a bbox inside the geometry
PG_FUNCTION_INFO_V1(LWGEOM_addBBOX);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
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
	if ( ! getbox2d_p(SERIALIZED_FORM(lwgeom), &box) )
	{
		// Empty geom, no bbox to add
		result = palloc (lwgeom->size);
		memcpy(result, lwgeom, lwgeom->size);
		PG_RETURN_POINTER(result);
	}
	old_type = lwgeom->type;

	size = lwgeom->size+sizeof(BOX2DFLOAT4);

	result = palloc(size);// 16 for bbox2d

	memcpy(result,&size,4); // size
	result->type = lwgeom_makeType_full(
		TYPE_HASZ(old_type),
		TYPE_HASM(old_type),
		lwgeom_hasSRID(old_type), lwgeom_getType(old_type), 1);
	// copy in bbox
	memcpy(result->data, &box, sizeof(BOX2DFLOAT4));

	//lwnotice("result->type hasbbox: %d", TYPE_HASBBOX(result->type));

//elog(NOTICE,"LWGEOM_addBBOX  -- about to copy serialized form");
	// everything but the type and length
	memcpy(result->data+sizeof(BOX2DFLOAT4), lwgeom->data, lwgeom->size-5);

	PG_RETURN_POINTER(result);
}

char
is_worth_caching_pglwgeom_bbox(PG_LWGEOM *in)
{
#if ! AUTOCACHE_BBOX
	return false;
#endif
	if ( TYPE_GETTYPE(in->type) == POINTTYPE ) return false;
	return true;
}

char
is_worth_caching_lwgeom_bbox(LWGEOM *in)
{
#if ! AUTOCACHE_BBOX
	return false;
#endif
	if ( TYPE_GETTYPE(in->type) == POINTTYPE ) return false;
	return true;
}

// removes a bbox from a geometry
PG_FUNCTION_INFO_V1(LWGEOM_dropBBOX);
Datum LWGEOM_dropBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	unsigned char	old_type;
	int		size;

//elog(NOTICE,"in LWGEOM_dropBBOX");

	if (!lwgeom_hasBBOX( lwgeom->type ) )
	{
//elog(NOTICE,"LWGEOM_dropBBOX  -- doesnt have a bbox already");
		result = palloc (lwgeom->size);
		memcpy(result, lwgeom, lwgeom->size);
		PG_RETURN_POINTER(result);
	}

//elog(NOTICE,"LWGEOM_dropBBOX  -- dropping the bbox");

	//construct new one
	old_type = lwgeom->type;

	size = lwgeom->size-sizeof(BOX2DFLOAT4);

	result = palloc(size);// 16 for bbox2d

	memcpy(result,&size,4); // size
	result->type = lwgeom_makeType_full(
		TYPE_HASZ(old_type),
		TYPE_HASM(old_type),
		lwgeom_hasSRID(old_type), lwgeom_getType(old_type), 0);

	// everything but the type and length
	memcpy(result->data, lwgeom->data+sizeof(BOX2DFLOAT4), lwgeom->size-5-sizeof(BOX2DFLOAT4));

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
	// text
	text *wkt_input = PG_GETARG_TEXT_P(0);
	PG_LWGEOM *ret;  //with length
	char *wkt;
	int wkt_size ;

	init_pg_func();

	wkt_size = VARSIZE(wkt_input)-VARHDRSZ; // actual letters
	//(*(int*) wkt_input) -4; 

	wkt = palloc( wkt_size+1); //+1 for null
	memcpy(wkt, VARDATA(wkt_input), wkt_size );
	wkt[wkt_size] = 0; // null term


//elog(NOTICE,"in parse_WKT_lwgeom");
//elog(NOTICE,"in parse_WKT_lwgeom with input: '%s'",wkt);

	ret = (PG_LWGEOM *)parse_lwg((const char *)wkt, (allocator)lwalloc, (report_error)elog_ERROR);
//elog(NOTICE,"parse_WKT_lwgeom:: finished parse");
	pfree (wkt);

	if (ret == NULL) elog(ERROR,"parse_WKT:: couldnt parse!");

	if ( is_worth_caching_pglwgeom_bbox(ret) )
	{
		ret = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(ret)));
	}

	PG_RETURN_POINTER(ret);
}


#if USE_VERSION > 73
/*
 * This function must advance the StringInfo.cursor pointer
 * and leave it at the end of StringInfo.buf. If it fails
 * to do so the backend will raise an exception with message:
 * ERROR:  incorrect binary data format in bind parameter #
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_recv);
Datum LWGEOM_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
        bytea *wkb;
	PG_LWGEOM *result;

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_recv start");
#endif

	/* Add VARLENA size info to make it a valid varlena object */
	wkb = (bytea *)palloc(buf->len+VARHDRSZ);
	VARATT_SIZEP(wkb) = buf->len+VARHDRSZ;
	memcpy(VARATT_DATA(wkb), buf->data, buf->len);

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_recv calling LWGEOMFromWKB");
#endif

	/* Call LWGEOM_from_bytea function... */
	result = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOMFromWKB, PointerGetDatum(wkb)));

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_recv advancing StringInfo buffer");
#endif

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_from_bytea returned %s", unparse_WKB(SERIALIZED_FORM(result),pg_alloc,pg_free,-1));
#endif


	/* Set cursor to the end of buffer (so the backend is happy) */
	buf->cursor = buf->len;

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_recv returning");
#endif

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_send);
Datum LWGEOM_send(PG_FUNCTION_ARGS)
{
	bytea *result;

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_send called");
#endif

	result = (bytea *)DatumGetPointer(DirectFunctionCall1(
		WKBFromLWGEOM, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_to_bytea);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS)
{
	bytea *result;

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_to_bytea called");
#endif

	result = (bytea *)DatumGetPointer(DirectFunctionCall1(
		WKBFromLWGEOM, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_from_bytea);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *result;

#ifdef DEBUG
	elog(NOTICE, "LWGEOM_from_bytea start");
#endif

	result = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOMFromWKB, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

#endif // USE_VERSION > 73
