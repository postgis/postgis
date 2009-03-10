#include <postgres.h>
#include <fmgr.h>
#include <executor/spi.h>

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PARANOIA_LEVEL 1

/*
 * This is required for builds against pgsql 8.2
 */
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


/*
 * Error message parsing functions
 *
 * Produces nicely formatted messages for parser/unparser errors with optional HINT
 */

void
pg_parser_errhint(LWGEOM_PARSER_RESULT *lwg_parser_result)
{
	char *hintbuffer;

	/* Return a copy of the input string start truncated at the error location */
	hintbuffer = lwmessage_truncate((char *)lwg_parser_result->wkinput, 0, lwg_parser_result->errlocation - 1, 40, 0);
	
	/* Only display the parser position if the location is > 0; this provides a nicer output when the first token
	   within the input stream cannot be matched */	
	if (lwg_parser_result->errlocation > 0)
	{
		ereport(ERROR,
			(errmsg("%s", lwg_parser_result->message),
			errhint("\"%s\" <-- parse error at position %d within geometry", hintbuffer, lwg_parser_result->errlocation))
		);	
	}
	else
	{
		ereport(ERROR,
			(errmsg("%s", lwg_parser_result->message),
			errhint("You must specify a valid OGC WKT geometry type such as POINT, LINESTRING or POLYGON"))
		);	
	}	
}

void
pg_unparser_errhint(LWGEOM_UNPARSER_RESULT *lwg_unparser_result)
{
	/* For the unparser simply output the error message without any associated HINT */
	elog(ERROR, "%s", lwg_unparser_result->message);
}


void *
pg_alloc(size_t size)
{
	void * result;

	POSTGIS_DEBUGF(5, "  pg_alloc(%d) called", (int)size);

	result = palloc(size);

	POSTGIS_DEBUGF(5, "  pg_alloc(%d) returning %p", (int)size, result);

	if ( ! result )
	{
		ereport(ERROR, (errmsg_internal("Out of virtual memory")));
		return NULL;
	}
	return result;
}

void *
pg_realloc(void *mem, size_t size)
{
	void * result;

	POSTGIS_DEBUGF(5, "  pg_realloc(%p, %d) called", mem, (int)size);

	result = repalloc(mem, size);

	POSTGIS_DEBUGF(5, "  pg_realloc(%p, %d) returning %p", mem, (int)size, result);

	return result;
}

void
pg_free(void *ptr)
{
	pfree(ptr);
}

void
pg_error(const char *fmt, va_list ap)
{
#define ERRMSG_MAXLEN 256

	char errmsg[ERRMSG_MAXLEN+1];

	vsnprintf (errmsg, ERRMSG_MAXLEN, fmt, ap);

	errmsg[ERRMSG_MAXLEN]='\0';
	ereport(ERROR, (errmsg_internal("%s", errmsg)));
}

void
pg_notice(const char *fmt, va_list ap)
{
	char *msg;

	/*
	 * This is a GNU extension.
	 * Dunno how to handle errors here.
	 */
	if (!lw_vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}
	ereport(NOTICE, (errmsg_internal("%s", msg)));
	free(msg);
}

void
lwgeom_init_allocators(void)
{
	/* liblwgeom callback - install PostgreSQL handlers */
	lwalloc_var = pg_alloc;
	lwrealloc_var = pg_realloc;
	lwfree_var = pg_free;
	lwerror_var = pg_error;
	lwnotice_var = pg_notice;
}

PG_LWGEOM *
pglwgeom_serialize(LWGEOM *in)
{
	size_t size;
	PG_LWGEOM *result;

#if POSTGIS_AUTOCACHE_BBOX
	if ( ! in->bbox && is_worth_caching_lwgeom_bbox(in) )
	{
		lwgeom_add_bbox(in);
	}
#endif

	size = lwgeom_serialize_size(in) + VARHDRSZ;

	POSTGIS_DEBUGF(3, "lwgeom_serialize_size returned %d", (int)size-VARHDRSZ);

	result = palloc(size);
	SET_VARSIZE(result, size);
	lwgeom_serialize_buf(in, SERIALIZED_FORM(result), &size);

        POSTGIS_DEBUGF(3, "pglwgeom_serialize: serialized size: %d, computed size: %d", (int)size, VARSIZE(result)-VARHDRSZ);

#if PARANOIA_LEVEL > 0
	if ( size != VARSIZE(result)-VARHDRSZ )
	{
		lwerror("pglwgeom_serialize: serialized size:%d, computed size:%d", (int)size, VARSIZE(result)-VARHDRSZ);
		return NULL;
	}
#endif

	return result;
}

LWGEOM *
pglwgeom_deserialize(PG_LWGEOM *in)
{
	return lwgeom_deserialize(SERIALIZED_FORM(in));
}

Oid
getGeometryOID(void)
{
	static Oid OID = InvalidOid;
	int SPIcode;
	bool isnull;
	char *query = "select OID from pg_type where typname = 'geometry'";	

	if ( OID != InvalidOid ) return OID;
	
	SPIcode = SPI_connect();
	if (SPIcode  != SPI_OK_CONNECT) {
		lwerror("getGeometryOID(): couldn't connection to SPI");
	}

	SPIcode = SPI_exec(query, 0); 
	if (SPIcode != SPI_OK_SELECT ) {
		lwerror("getGeometryOID(): error querying geometry oid");
	}
	if (SPI_processed != 1) {
		lwerror("getGeometryOID(): error querying geometry oid");
	}

	OID = (Oid)SPI_getbinval(SPI_tuptable->vals[0],
		SPI_tuptable->tupdesc, 1, &isnull);

	if (isnull) 
		lwerror("getGeometryOID(): couldn't find geometry oid");

	return OID;
}

PG_LWGEOM *
PG_LWGEOM_construct(uchar *ser, int SRID, int wantbbox)
{
	int size;
	uchar *iptr, *optr, *eptr;
	int wantsrid = 0;
	BOX2DFLOAT4 box;
	PG_LWGEOM *result;

	/* COMPUTE_BBOX FOR_COMPLEX_GEOMS */
	if ( is_worth_caching_serialized_bbox(ser) ) 
	{
		/* if ( ! wantbbox ) elog(NOTICE, "PG_LWGEOM_construct forced wantbbox=1 due to rule FOR_COMPLEX_GEOMS"); */
		wantbbox=1;
	}

	size = lwgeom_size(ser);
	eptr = ser+size; /* end of subgeom */

	iptr = ser+1; /* skip type */
	if ( lwgeom_hasSRID(ser[0]) )
	{
		iptr += 4; /* skip SRID */
		size -= 4;
	}
	if ( lwgeom_hasBBOX(ser[0]) )
	{
		iptr += sizeof(BOX2DFLOAT4); /* skip BBOX */
		size -= sizeof(BOX2DFLOAT4);
	}

	if ( SRID != -1 )
	{
		wantsrid = 1;
		size += 4;
	}
	if ( wantbbox )
	{
		size += sizeof(BOX2DFLOAT4);
		getbox2d_p(ser, &box);
	}

	size+=4; /* size header */

	result = lwalloc(size);
	SET_VARSIZE(result, size);

	result->type = lwgeom_makeType_full(
		TYPE_HASZ(ser[0]), TYPE_HASM(ser[0]),
		wantsrid, lwgeom_getType(ser[0]), wantbbox);
	optr = result->data;
	if ( wantbbox )
	{
		memcpy(optr, &box, sizeof(BOX2DFLOAT4));
		optr += sizeof(BOX2DFLOAT4);
	}
	if ( wantsrid )
	{
		memcpy(optr, &SRID, 4);
		optr += 4;
	}
	memcpy(optr, iptr, eptr-iptr);

	return result;
}

/*
 * Make a PG_LWGEOM object from a WKB binary representation.
 */
PG_LWGEOM *
pglwgeom_from_ewkb(uchar *ewkb, int flags, size_t ewkblen)
{
	PG_LWGEOM *ret;
	LWGEOM_PARSER_RESULT lwg_parser_result;
	char *hexewkb;
	size_t hexewkblen = ewkblen*2;
	int i, result;

	hexewkb = lwalloc(hexewkblen+1);
	for (i=0; i<ewkblen; i++)
	{
		deparse_hex(ewkb[i], &hexewkb[i*2]);
	}
	hexewkb[hexewkblen] = '\0';

	result = serialized_lwgeom_from_ewkt(&lwg_parser_result, hexewkb, flags);
	if (result)
		PG_PARSER_ERROR(lwg_parser_result);

	ret = (PG_LWGEOM *)palloc(lwg_parser_result.size + VARHDRSZ);
	SET_VARSIZE(ret, lwg_parser_result.size + VARHDRSZ);
	memcpy(VARDATA(ret), lwg_parser_result.serialized_lwgeom, lwg_parser_result.size);

	lwfree(hexewkb);

	return ret;
}

/*
 * Return the EWKB (binary) representation for a PG_LWGEOM.
 */
char *
pglwgeom_to_ewkb(PG_LWGEOM *geom, int flags, char byteorder, size_t *outsize)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	int result;
	char *wkoutput;
	uchar *srl = &(geom->type);

	result = serialized_lwgeom_to_ewkb(&lwg_unparser_result, srl, flags, byteorder);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

	*outsize = lwg_unparser_result.size;

	/* Make a copy of the wkoutput so it can be used outside of the function */
	wkoutput = palloc(lwg_unparser_result.size);
	memcpy(wkoutput, lwg_unparser_result.wkoutput, lwg_unparser_result.size);

	return wkoutput;
}

/*
 * Set the SRID of a PG_LWGEOM
 * Returns a newly allocated PG_LWGEOM object.
 * Allocation will be done using the lwalloc.
 */
PG_LWGEOM *
pglwgeom_setSRID(PG_LWGEOM *lwgeom, int32 newSRID)
{
	uchar type = lwgeom->type;
	int bbox_offset=0; /* 0=no bbox, otherwise sizeof(BOX2DFLOAT4) */
	int len,len_new,len_left;
	PG_LWGEOM *result;
	uchar *loc_new, *loc_old;

	if (lwgeom_hasBBOX(type))
		bbox_offset = sizeof(BOX2DFLOAT4);

	len = lwgeom->size;

	if (lwgeom_hasSRID(type))
	{
		if ( newSRID != -1 ) {
			/* we create a new one and copy the SRID in */
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
			memcpy(result->data+bbox_offset, &newSRID,4);
		} else {
			/* we create a new one dropping the SRID */
			result = lwalloc(len-4);
			result->size = len-4;
			result->type = lwgeom_makeType_full(
				TYPE_HASZ(type), TYPE_HASM(type),
				0, lwgeom_getType(type),
				lwgeom_hasBBOX(type));
			loc_new = result->data;
			loc_old = lwgeom->data;
			len_left = len-4-1;

			/* handle bbox (if there) */
			if (lwgeom_hasBBOX(type))
			{
				memcpy(loc_new, loc_old, sizeof(BOX2DFLOAT4));
				loc_new += sizeof(BOX2DFLOAT4);
				loc_old += sizeof(BOX2DFLOAT4);
				len_left -= sizeof(BOX2DFLOAT4);
			}

			/* skip SRID, copy the remaining */
			loc_old += 4;
			len_left -= 4;
			memcpy(loc_new, loc_old, len_left);

		}

	}
	else 
	{
		/* just copy input, already w/out a SRID */
		if ( newSRID == -1 ) {
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
		}
		/* need to add one */
		else {
			len_new = len + 4; /* +4 for SRID */
			result = lwalloc(len_new);
			memcpy(result, &len_new, 4); /* size copy in */
			result->type = lwgeom_makeType_full(
				TYPE_HASZ(type), TYPE_HASM(type),
				1, lwgeom_getType(type),lwgeom_hasBBOX(type));

			loc_new = result->data;
			loc_old = lwgeom->data;

			len_left = len -4-1; /* old length - size - type */

			/* handle bbox (if there) */

			if (lwgeom_hasBBOX(type))
			{
				memcpy(loc_new, loc_old, sizeof(BOX2DFLOAT4)) ; /* copy in bbox */
				loc_new += sizeof(BOX2DFLOAT4);
				loc_old += sizeof(BOX2DFLOAT4);
				len_left -= sizeof(BOX2DFLOAT4);
			}

			/* put in SRID */

			memcpy(loc_new, &newSRID,4);
			loc_new +=4;
			memcpy(loc_new, loc_old, len_left);
		}
	}
	return result;
}

/*
 * get the SRID from the LWGEOM
 * none present => -1
 */
int
pglwgeom_getSRID(PG_LWGEOM *lwgeom)
{
	uchar type = lwgeom->type;
	uchar *loc = lwgeom->data;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return lw_get_int32(loc);
}

