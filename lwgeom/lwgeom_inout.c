#include "../postgis_config.h"

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

#include "fmgr.h"
#include "utils/elog.h"
# include "lib/stringinfo.h" /* for binary input */


#include "liblwgeom.h"
#include "stringBuffer.h"



#include "lwgeom_pg.h"
#include "profile.h"

void elog_ERROR(const char* string);


Datum LWGEOM_in(PG_FUNCTION_ARGS);
Datum LWGEOM_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS);
Datum LWGEOM_asHEXEWKB(PG_FUNCTION_ARGS);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS);
Datum LWGEOM_recv(PG_FUNCTION_ARGS);
Datum LWGEOM_send(PG_FUNCTION_ARGS);
Datum BOOL_to_text(PG_FUNCTION_ARGS);


/*
 *  included here so we can be independent from postgis
 * WKB structure  -- exactly the same as TEXT
 */
typedef struct Well_known_bin {
    int32 size;		/* total size of this structure */
    uchar  data[1];	/* THIS HOLD VARIABLE LENGTH DATA */
} WellKnownBinary;


/*
 * LWGEOM_in(cstring)
 * format is '[SRID=#;]wkt|wkb'
 *  LWGEOM_in( 'SRID=99;POINT(0 0)')
 *  LWGEOM_in( 'POINT(0 0)')            --> assumes SRID=-1
 *  LWGEOM_in( 'SRID=99;0101000000000000000000F03F000000000000004')
 *  returns a PG_LWGEOM object
 */
PG_FUNCTION_INFO_V1(LWGEOM_in);
Datum LWGEOM_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
    SERIALIZED_LWGEOM *serialized_lwgeom;
    LWGEOM *lwgeom;	
    PG_LWGEOM *ret;

	/* will handle both HEXEWKB and EWKT */
    serialized_lwgeom = ewkt_to_lwgeom(str);
	lwgeom = lwgeom_deserialize(serialized_lwgeom->lwgeom);
    
    ret = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	if ( is_worth_caching_pglwgeom_bbox(ret) )
	{
		ret = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(ret)));
	}

	PG_RETURN_POINTER(ret);
}

/*
 * LWGEOM_out(lwgeom) --> cstring
 * output is 'SRID=#;<wkb in hex form>'
 * ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
 * WKB is machine endian
 * if SRID=-1, the 'SRID=-1;' will probably not be present.
 */
PG_FUNCTION_INFO_V1(LWGEOM_out);
Datum LWGEOM_out(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *result;

	lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	result = serialized_lwgeom_to_hexwkb(SERIALIZED_FORM(lwgeom), -1, NULL);

	PG_RETURN_CSTRING(result);
}

/*
 * AsHEXEWKB(geom, string)
 */
PG_FUNCTION_INFO_V1(LWGEOM_asHEXEWKB);
Datum LWGEOM_asHEXEWKB(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *result;
	text *text_result;
	size_t size;
	text *type;
	unsigned int byteorder=-1;

	lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		type = PG_GETARG_TEXT_P(1);
		if (VARSIZE(type) < 7)  
		{
			elog(ERROR,"AsHEXEWKB(geometry, <type>) - type should be 'XDR' or 'NDR'.  type length is %i",VARSIZE(type) -VARHDRSZ);
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

	result = serialized_lwgeom_to_hexwkb(SERIALIZED_FORM(lwgeom), byteorder, &size);

	text_result = palloc(size+VARHDRSZ);
	memcpy(VARDATA(text_result),result,size);
	SET_VARSIZE(text_result, size+VARHDRSZ);
	pfree(result);

	PG_RETURN_POINTER(text_result);

}

/*
 * LWGEOM_to_text(lwgeom) --> text
 * output is 'SRID=#;<wkb in hex form>'
 * ie. 'SRID=-99;0101000000000000000000F03F0000000000000040'
 * WKB is machine endian
 * if SRID=-1, the 'SRID=-1;' will probably not be present.
 */
PG_FUNCTION_INFO_V1(LWGEOM_to_text);
Datum LWGEOM_to_text(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom;
	char *result;
	text *text_result;
	size_t size;

	lwgeom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	result = serialized_lwgeom_to_hexwkb(SERIALIZED_FORM(lwgeom), -1, &size);

	text_result = palloc(size+VARHDRSZ);
	memcpy(VARDATA(text_result),result,size);
	SET_VARSIZE(text_result, size+VARHDRSZ);
	pfree(result);

	PG_RETURN_POINTER(text_result);
}

/*
 * LWGEOMFromWKB(wkb,  [SRID] )
 * NOTE: wkb is in *binary* not hex form.
 *
 * NOTE: this function parses EWKB (extended form)
 *       which also contains SRID info. When the SRID
 *	 argument is provided it will override any SRID
 *	 info found in EWKB format.
 *
 * NOTE: this function is *unoptimized* as will copy
 *       the object when adding SRID and when adding BBOX.
 *	 additionally, it suffers from the non-optimal
 *	 pglwgeom_from_ewkb function.
 *
 */
PG_FUNCTION_INFO_V1(LWGEOMFromWKB);
Datum LWGEOMFromWKB(PG_FUNCTION_ARGS)
{
	WellKnownBinary *wkb_input;
	PG_LWGEOM *lwgeom, *lwgeom2;

	wkb_input = (WellKnownBinary *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	lwgeom2 = pglwgeom_from_ewkb((uchar *)VARDATA(wkb_input),
		VARSIZE(wkb_input)-VARHDRSZ);

	if (  ( PG_NARGS()>1) && ( ! PG_ARGISNULL(1) ))
	{
		lwgeom = pglwgeom_setSRID(lwgeom2, PG_GETARG_INT32(1));
		lwfree(lwgeom2);
	}
	else lwgeom = lwgeom2;

	if ( is_worth_caching_pglwgeom_bbox(lwgeom) )
	{
		lwgeom = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(lwgeom)));
	}

	PG_RETURN_POINTER(lwgeom);
}

/*
 * WKBFromLWGEOM(lwgeom) --> wkb
 * this will have no 'SRID=#;'
 */
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
/* #define BINARY_FROM_HEX 1 */

	PG_LWGEOM *lwgeom_input; /* SRID=#;<hexized wkb> */
	char *hexized_wkb; /* hexized_wkb_srid w/o srid */
	char *result; /* wkb */
	int size_result;
#ifdef BINARY_FROM_HEX
	char *hexized_wkb_srid;
	char *semicolonLoc;
	int t;
#endif /* BINARY_FROM_HEX */
	text *type;
	unsigned int byteorder=-1;
	size_t size;

#ifdef PROFILE
	profstart(PROF_QRUN);
#endif

	if ( (PG_NARGS()>1) && (!PG_ARGISNULL(1)) )
	{
		type = PG_GETARG_TEXT_P(1);
		if (VARSIZE(type) < 7)  
		{
			elog(ERROR,"asbinary(geometry, <type>) - type should be 'XDR' or 'NDR'.  type length is %i",VARSIZE(type) -VARHDRSZ);
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

#ifdef BINARY_FROM_HEX
	hexized_wkb_srid = serialized_lwgeom_to_hexwkb(SERIALIZED_FORM(lwgeom_input), byteorder, &size);

	LWDEBUGF(3, "in WKBFromLWGEOM with WKB = '%s'", hexized_wkb_srid);

	hexized_wkb = hexized_wkb_srid;

	semicolonLoc = strchr(hexized_wkb_srid,';');
	if (semicolonLoc != NULL)
	{
		hexized_wkb = (semicolonLoc+1);
	}

	LWDEBUGF(3, "in WKBFromLWGEOM with WKB (with no 'SRID=#;' = '%s'",
		hexized_wkb);

	size_result = size/2 + VARHDRSZ;
	result = palloc(size_result);
	SET_VARSIZE(result, size_result);

	/* have a hexized string, want to make it binary */
	for (t=0; t< (size/2); t++)
	{
		((uchar *) VARDATA(result))[t] = parse_hex(  hexized_wkb + (t*2) );
	}

	pfree(hexized_wkb_srid);

#else /* ndef BINARY_FROM_HEX */

	hexized_wkb = serialized_lwgeom_to_ewkb(SERIALIZED_FORM(lwgeom_input), byteorder, &size);

	size_result = size+VARHDRSZ;
	result = palloc(size_result);
	SET_VARSIZE(result, size_result);
	memcpy(VARDATA(result), hexized_wkb, size);
	pfree(hexized_wkb);

#endif

#ifdef PROFILE
	profstop(PROF_QRUN);
	lwnotice("serialized_lwgeom_to_ewkb: prof: %lu", proftime[PROF_QRUN]);
#endif

	LWDEBUGF(3, "Output size is %lu (comp: %lu)",
		VARSIZE(result), (unsigned long)size);

	PG_RETURN_POINTER(result);
}

/* puts a bbox inside the geometry */
PG_FUNCTION_INFO_V1(LWGEOM_addBBOX);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	BOX2DFLOAT4	box;
	uchar	old_type;
	int		size;

	POSTGIS_DEBUG(2, "in LWGEOM_addBBOX");

	if (lwgeom_hasBBOX( lwgeom->type ) )
	{
		POSTGIS_DEBUG(3, "LWGEOM_addBBOX  -- already has bbox");

		/* easy - already has one.  Just copy! */
		result = palloc (VARSIZE(lwgeom));
		SET_VARSIZE(result, VARSIZE(lwgeom));
		memcpy(VARDATA(result), VARDATA(lwgeom), VARSIZE(lwgeom)-VARHDRSZ);
		PG_RETURN_POINTER(result);
	}

	POSTGIS_DEBUG(3, "LWGEOM_addBBOX  -- giving it a bbox");

	/* construct new one */
	if ( ! getbox2d_p(SERIALIZED_FORM(lwgeom), &box) )
	{
		/* Empty geom, no bbox to add */
		result = palloc (VARSIZE(lwgeom));
		SET_VARSIZE(result, VARSIZE(lwgeom));
		memcpy(VARDATA(result), VARDATA(lwgeom), VARSIZE(lwgeom)-VARHDRSZ);
		PG_RETURN_POINTER(result);
	}
	old_type = lwgeom->type;

	size = VARSIZE(lwgeom)+sizeof(BOX2DFLOAT4);

	result = palloc(size); /* 16 for bbox2d */
	SET_VARSIZE(result, size);

	result->type = lwgeom_makeType_full(
		TYPE_HASZ(old_type),
		TYPE_HASM(old_type),
		lwgeom_hasSRID(old_type), lwgeom_getType(old_type), 1);

	/* copy in bbox */
	memcpy(result->data, &box, sizeof(BOX2DFLOAT4));

	POSTGIS_DEBUGF(3, "result->type hasbbox: %d", TYPE_HASBBOX(result->type));
	POSTGIS_DEBUG(3, "LWGEOM_addBBOX  -- about to copy serialized form");

	/* everything but the type and length */
	memcpy((char *)VARDATA(result)+sizeof(BOX2DFLOAT4)+1, (char *)VARDATA(lwgeom)+1, VARSIZE(lwgeom)-VARHDRSZ-1);

	PG_RETURN_POINTER(result);
}

char
is_worth_caching_pglwgeom_bbox(const PG_LWGEOM *in)
{
#if ! POSTGIS_AUTOCACHE_BBOX
	return false;
#endif
	if ( TYPE_GETTYPE(in->type) == POINTTYPE ) return false;
	return true;
}

char
is_worth_caching_serialized_bbox(const uchar *in)
{
#if ! POSTGIS_AUTOCACHE_BBOX
	return false;
#endif
	if ( TYPE_GETTYPE((uchar)in[0]) == POINTTYPE ) return false;
	return true;
}

char
is_worth_caching_lwgeom_bbox(const LWGEOM *in)
{
#if ! POSTGIS_AUTOCACHE_BBOX
	return false;
#endif
	if ( TYPE_GETTYPE(in->type) == POINTTYPE ) return false;
	return true;
}

/* removes a bbox from a geometry */
PG_FUNCTION_INFO_V1(LWGEOM_dropBBOX);
Datum LWGEOM_dropBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	uchar old_type;
	int size;

	POSTGIS_DEBUG(2, "in LWGEOM_dropBBOX");

	if (!lwgeom_hasBBOX( lwgeom->type ) )
	{
		POSTGIS_DEBUG(3, "LWGEOM_dropBBOX  -- doesnt have a bbox already");

		result = palloc (VARSIZE(lwgeom));
		SET_VARSIZE(result, VARSIZE(lwgeom));
		memcpy(VARDATA(result), VARDATA(lwgeom), VARSIZE(lwgeom)-VARHDRSZ);
		PG_RETURN_POINTER(result);
	}

	POSTGIS_DEBUG(3, "LWGEOM_dropBBOX  -- dropping the bbox");

	/* construct new one */
	old_type = lwgeom->type;

	size = VARSIZE(lwgeom)-sizeof(BOX2DFLOAT4);

	result = palloc(size); /* 16 for bbox2d */
	SET_VARSIZE(result, size);

	result->type = lwgeom_makeType_full(
		TYPE_HASZ(old_type),
		TYPE_HASM(old_type),
		lwgeom_hasSRID(old_type), lwgeom_getType(old_type), 0);

	/* everything but the type and length */
	memcpy((char *)VARDATA(result)+1, ((char *)(lwgeom->data))+sizeof(BOX2DFLOAT4), size-VARHDRSZ-1);

	PG_RETURN_POINTER(result);
}


/* for the wkt parser */
void elog_ERROR(const char* string)
{
	elog(ERROR,string);
}

/*
 * parse WKT input
 * parse_WKT_lwgeom(TEXT) -> LWGEOM
 */
PG_FUNCTION_INFO_V1(parse_WKT_lwgeom);
Datum parse_WKT_lwgeom(PG_FUNCTION_ARGS)
{
	/* text */
	text *wkt_input = PG_GETARG_TEXT_P(0);
	PG_LWGEOM *ret;  /*with length */
	SERIALIZED_LWGEOM *serialized_lwgeom;
	LWGEOM *lwgeom;	
	char *wkt;
	int wkt_size ;

	wkt_size = VARSIZE(wkt_input)-VARHDRSZ; /* actual letters */

	wkt = palloc( wkt_size+1); /* +1 for null */
	memcpy(wkt, VARDATA(wkt_input), wkt_size );
	wkt[wkt_size] = 0; /* null term */


	POSTGIS_DEBUGF(3, "in parse_WKT_lwgeom with input: '%s'",wkt);

	serialized_lwgeom = ewkt_to_lwgeom(wkt);
	lwgeom = lwgeom_deserialize(serialized_lwgeom->lwgeom);
    
	ret = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	POSTGIS_DEBUG(3, "parse_WKT_lwgeom:: finished parse");

	pfree (wkt);

	if (ret == NULL) elog(ERROR,"parse_WKT:: couldnt parse!");

	if ( is_worth_caching_pglwgeom_bbox(ret) )
	{
		ret = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
			LWGEOM_addBBOX, PointerGetDatum(ret)));
	}

	PG_RETURN_POINTER(ret);
}


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

	POSTGIS_DEBUG(2, "LWGEOM_recv start");

	/* Add VARLENA size info to make it a valid varlena object */
	wkb = (bytea *)palloc(buf->len+VARHDRSZ);
	SET_VARSIZE(wkb, buf->len+VARHDRSZ);
	memcpy(VARDATA(wkb), buf->data, buf->len);

	POSTGIS_DEBUG(3, "LWGEOM_recv calling LWGEOMFromWKB");

	/* Call LWGEOM_from_bytea function... */
	result = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOMFromWKB, PointerGetDatum(wkb)));

	POSTGIS_DEBUG(3, "LWGEOM_recv advancing StringInfo buffer");

	POSTGIS_DEBUGF(3, "LWGEOM_from_bytea returned %s", serialized_lwgeom_to_hexwkb(SERIALIZED_FORM(result), -1, NULL));

	/* Set cursor to the end of buffer (so the backend is happy) */
	buf->cursor = buf->len;

	POSTGIS_DEBUG(3, "LWGEOM_recv returning");

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_send);
Datum LWGEOM_send(PG_FUNCTION_ARGS)
{
	bytea *result;

	POSTGIS_DEBUG(2, "LWGEOM_send called");

	result = (bytea *)DatumGetPointer(DirectFunctionCall1(
		WKBFromLWGEOM, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_to_bytea);
Datum LWGEOM_to_bytea(PG_FUNCTION_ARGS)
{
	bytea *result;

	POSTGIS_DEBUG(2, "LWGEOM_to_bytea called");

	result = (bytea *)DatumGetPointer(DirectFunctionCall1(
		WKBFromLWGEOM, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_from_bytea);
Datum LWGEOM_from_bytea(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_from_bytea start");

	result = (PG_LWGEOM *)DatumGetPointer(DirectFunctionCall1(
		LWGEOMFromWKB, PG_GETARG_DATUM(0)));

        PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOOL_to_text);
Datum BOOL_to_text(PG_FUNCTION_ARGS)
{
	bool b = PG_GETARG_BOOL(0);
	char c;
	text *result;

	c = b ? 't' : 'f';

	result = palloc(VARHDRSZ+1*sizeof(char));
	SET_VARSIZE(result, VARHDRSZ+1*sizeof(char));
	memcpy(VARDATA(result), &c, 1*sizeof(char));

	PG_RETURN_POINTER(result);

}
