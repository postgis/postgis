#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "postgres.h"
#include "executor/spi.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

//#undef PGIS_DEBUG

void *
pg_alloc(size_t size)
{
	void * result;
#ifdef PGIS_DEBUG
	lwnotice("  pg_alloc(%d) called", size);
#endif
	result = palloc(size);
#ifdef PGIS_DEBUG
	lwnotice("  pg_alloc(%d) returning %p", size, result);
#endif
	if ( ! result )
	{
		elog(ERROR, "Out of virtual memory");
		return NULL;
	}
	return result;
}

void *
pg_realloc(void *mem, size_t size)
{
	void * result;
#ifdef PGIS_DEBUG
	lwnotice("  pg_realloc(%p, %d) called", mem, size);
#endif
	result = repalloc(mem, size);
#ifdef PGIS_DEBUG
	lwnotice("  pg_realloc(%p, %d) returning %p", mem, size, result);
#endif
	return result;
}

void
pg_free(void *ptr)
{
	pfree(ptr);
}

void
pg_error(const char *fmt, ...)
{
	char *msg;
	va_list ap;

	va_start (ap, fmt);

	/*
	 * This is a GNU extension.
	 * Dunno how to handle errors here.
	 */
	if (!vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}
	elog(ERROR, "%s", msg);
	va_end(ap);
	free(msg);
}

void
pg_notice(const char *fmt, ...)
{
	char *msg;
	va_list ap;

	va_start (ap, fmt);

	/*
	 * This is a GNU extension.
	 * Dunno how to handle errors here.
	 */
	if (!vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}
	elog(NOTICE, "%s", msg);
	va_end(ap);
	free(msg);
}

void
init_pg_func(void)
{
	lwalloc_var = pg_alloc;
	lwrealloc_var = pg_realloc;
	lwfree_var = pg_free;
	lwerror = pg_error;
	lwnotice = pg_notice;
}

PG_LWGEOM *
pglwgeom_serialize(LWGEOM *in)
{
	size_t size;
	PG_LWGEOM *result;

#if AUTOCACHE_BBOX
	if ( ! in->bbox && is_worth_caching_lwgeom_bbox(in) )
	{
		lwgeom_addBBOX(in);
	}
#endif

	size = lwgeom_serialize_size(in) + VARHDRSZ;
	//lwnotice("lwgeom_serialize_size returned %d", size-VARHDRSZ);
	result = palloc(size);
	result->size = (size);
	lwgeom_serialize_buf(in, SERIALIZED_FORM(result), &size);
	if ( size != result->size-VARHDRSZ )
	{
		lwerror("pglwgeom_serialize: serialized size:%d, computed size:%d", size, result->size-VARHDRSZ);
		return NULL;
	}

	return result;
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
		// if ( ! wantbbox ) elog(NOTICE, "PG_LWGEOM_construct forced wantbbox=1 due to rule FOR_COMPLEX_GEOMS");
		wantbbox=1;
	}

	size = lwgeom_size(ser);
	eptr = ser+size; // end of subgeom

	iptr = ser+1; // skip type
	if ( lwgeom_hasSRID(ser[0]) )
	{
		iptr += 4; // skip SRID
		size -= 4;
	}
	if ( lwgeom_hasBBOX(ser[0]) )
	{
		iptr += sizeof(BOX2DFLOAT4); // skip BBOX
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

	size+=4; // size header

	result = lwalloc(size);
	result->size = size;

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

