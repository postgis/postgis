/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * http://postgis.refractions.net
 *
 * Copyright (C) 2011      Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2009-2010 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2008      Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2004-2007 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

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
 * For win32, this can not be in a separate libary
 * See ticket #1158
 */
/* PG_MODULE_MAGIC; */

/**
* Utility to convert cstrings to textp pointers 
*/
text* 
cstring2text(const char *cstring)
{
	text *output;
	size_t sz;
	
	/* Guard against null input */
	if( !cstring )
		return NULL;
		
	sz = strlen(cstring);
	output = palloc(sz + VARHDRSZ);
	if ( ! output ) 
		return NULL;
	SET_VARSIZE(output, sz + VARHDRSZ);
	if ( sz )
		memcpy(VARDATA(output),cstring,sz);
	return output;
}

char*
text2cstring(const text *textptr)
{
	size_t size = VARSIZE(textptr) - VARHDRSZ;
	char *str = lwalloc(size+1);
	memcpy(str, VARDATA(textptr), size);
	str[size]='\0';
	return str;
}


/*
 * Error message parsing functions
 *
 * Produces nicely formatted messages for parser/unparser errors with optional HINT
 */

void
pg_parser_errhint(LWGEOM_PARSER_RESULT *lwg_parser_result)
{
	char *hintbuffer;

	/* Only display the parser position if the location is > 0; this provides a nicer output when the first token
	   within the input stream cannot be matched */
	if (lwg_parser_result->errlocation > 0)
	{
		/* Return a copy of the input string start truncated
		 * at the error location */
		hintbuffer = lwmessage_truncate(
			(char *)lwg_parser_result->wkinput, 0,
			lwg_parser_result->errlocation - 1, 40, 0);

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
#ifdef GSERIALIZED_ON

	size_t gser_size;
	GSERIALIZED *gser;
	gser = gserialized_from_lwgeom(in, 0, &gser_size);
	SET_VARSIZE(gser, gser_size);
	return gser;

#else
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
#endif
}

LWGEOM *
pglwgeom_deserialize(PG_LWGEOM *in)
{
#ifdef GSERIALIZED_ON
	return lwgeom_from_gserialized(in);
#else
	return lwgeom_deserialize(SERIALIZED_FORM(in));
#endif
}


PG_LWGEOM *
PG_LWGEOM_construct(uint8_t *ser, int srid, int wantbbox)
{
#ifdef GSERIALIZED_ON
	lwerror("PG_LWGEOM_construct called!");
	return NULL;
#else
	int size;
	uint8_t *iptr, *optr, *eptr;
	int wantsrid = 0;
	BOX2DFLOAT4 box;
	PG_LWGEOM *result;

	/* COMPUTE_BBOX FOR_COMPLEX_GEOMS */
	if ( is_worth_caching_serialized_bbox(ser) )
	{
		/* if ( ! wantbbox ) elog(NOTICE, "PG_LWGEOM_construct forced wantbbox=1 due to rule FOR_COMPLEX_GEOMS"); */
		wantbbox=1;
	}

	size = serialized_lwgeom_size(ser);
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

	if ( srid != SRID_UNKNOWN )
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
		memcpy(optr, &srid, 4);
		optr += 4;
	}
	memcpy(optr, iptr, eptr-iptr);

	return result;
#endif
}


/*
 * Set the SRID of a PG_LWGEOM
 * Returns a newly allocated PG_LWGEOM object.
 * Allocation will be done using the lwalloc.
 */
PG_LWGEOM *
pglwgeom_set_srid(PG_LWGEOM *lwgeom, int32 new_srid)
{
#ifdef GSERIALIZED_ON
	gserialized_set_srid(lwgeom, new_srid);
	return lwgeom;
#else
	uint8_t type = lwgeom->type;
	int bbox_offset=0; /* 0=no bbox, otherwise sizeof(BOX2DFLOAT4) */
	int len,len_new,len_left;
	PG_LWGEOM *result;
	uint8_t *loc_new, *loc_old;

	if (lwgeom_hasBBOX(type))
		bbox_offset = sizeof(BOX2DFLOAT4);

	len = lwgeom->size;

	if (lwgeom_hasSRID(type))
	{
		if ( new_srid != SRID_UNKNOWN )
		{
			/* we create a new one and copy the SRID in */
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
			memcpy(result->data+bbox_offset, &new_srid,4);
		}
		else
		{
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
		if ( new_srid == SRID_UNKNOWN )
		{
			result = lwalloc(len);
			memcpy(result, lwgeom, len);
		}
		/* need to add one */
		else
		{
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

			memcpy(loc_new, &new_srid,4);
			loc_new +=4;
			memcpy(loc_new, loc_old, len_left);
		}
	}
	return result;
#endif
}

/*
 * get the SRID from the LWGEOM
 * none present => -1
 */
int
pglwgeom_get_srid(PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return gserialized_get_srid(lwgeom);
#else
	uint8_t type = lwgeom->type;
	uint8_t *loc = lwgeom->data;

	if ( ! lwgeom_hasSRID(type)) return -1;

	if (lwgeom_hasBBOX(type))
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	return lw_get_int32(loc);
#endif
}

int
pglwgeom_get_type(const PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return gserialized_get_type(lwgeom);
#else
	return TYPE_GETTYPE(lwgeom->type);
#endif
}

int
pglwgeom_get_zm(const PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return 2 * FLAGS_GET_Z(lwgeom->flags) + FLAGS_GET_M(lwgeom->flags);
#else
	return TYPE_GETZM(lwgeom->type);
#endif
}

bool
pglwgeom_has_bbox(const PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return FLAGS_GET_BBOX(lwgeom->flags);
#else
	return TYPE_HASBBOX(lwgeom->type);
#endif
}

bool
pglwgeom_has_z(const PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return FLAGS_GET_Z(lwgeom->flags);
#else
	return TYPE_HASZ(lwgeom->type);
#endif
}

bool
pglwgeom_has_m(const PG_LWGEOM *lwgeom)
{
#ifdef GSERIALIZED_ON
	return FLAGS_GET_M(lwgeom->flags);
#else
	return TYPE_HASM(lwgeom->type);
#endif
}

PG_LWGEOM* pglwgeom_drop_bbox(PG_LWGEOM *geom)
{
#ifdef GSERIALIZED_ON
	return gserialized_drop_gidx(geom);
#else
	size_t size = VARSIZE(geom);
	size_t newsize = size;
	bool hasbox = pglwgeom_has_bbox(geom);
	PG_LWGEOM *geomout;
	uint8_t type = geom->type;
	
	if ( hasbox )
		newsize = size - sizeof(BOX2DFLOAT4);
		
	geomout = palloc(newsize);
	SET_VARSIZE(geomout, newsize);
	TYPE_SETHASBBOX(type, 0);
	geomout->type = type;
	
	if ( ! hasbox ) 
		memcpy(VARDATA(geomout),VARDATA(geom),newsize - VARHDRSZ);
	else
		memcpy(VARDATA(geomout)+1,VARDATA(geom)+1+sizeof(BOX2DFLOAT4),newsize - VARHDRSZ - 1);
	
	return geomout;
#endif
}

size_t pglwgeom_size(const PG_LWGEOM *geom)
{
#ifdef GSERIALIZED_ON
	return VARSIZE(geom) - VARHDRSZ;
#else
	return serialized_lwgeom_size(SERIALIZED_FORM(geom));	
#endif
};

int pglwgeom_ndims(const PG_LWGEOM *geom)
{
#ifdef GSERIALIZED_ON
	return FLAGS_NDIMS(geom->flags);
#else
	return TYPE_NDIMS(geom->type);
#endif
}

int pglwgeom_getbox2d_p(const PG_LWGEOM *geom, BOX2DFLOAT4 *box)
{
#ifdef GSERIALIZED_ON
	LWGEOM *lwgeom;
	int ret = gserialized_get_gbox_p(geom, box);
	if ( LW_FAILURE == ret ) {
		/* See http://trac.osgeo.org/postgis/ticket/1023 */
		lwgeom = lwgeom_from_gserialized(geom);
		ret = lwgeom_calculate_gbox(lwgeom, box);
                lwgeom_free(lwgeom);
	}
	return ret;
#else
	return getbox2d_p(SERIALIZED_FORM(geom), box);
#endif
}

BOX3D *pglwgeom_compute_serialized_box3d(const PG_LWGEOM *geom)
{
#ifdef GSERIALIZED_ON
	lwerror("pglwgeom_compute_serialized_box3d called!");
	return NULL;
#else
	return compute_serialized_box3d(SERIALIZED_FORM(geom));
#endif
}

int pglwgeom_compute_serialized_box3d_p(const PG_LWGEOM *geom, BOX3D *box3d)
{
#ifdef GSERIALIZED_ON
	lwerror("pglwgeom_compute_serialized_box3d_p called!");
	return 0;
#else
	return compute_serialized_box3d_p(SERIALIZED_FORM(geom), box3d);	
#endif
}


int
pglwgeom_is_empty(const PG_LWGEOM *geom)
{
#ifdef GSERIALIZED_ON
	return gserialized_is_empty(geom);
#else
	uint8_t *serialized_form = SERIALIZED_FORM(geom);
	int type = pglwgeom_get_type(geom);
	uint8_t utype = serialized_form[0];
	uint8_t *loc = serialized_form + 1;

	if ( type == POINTTYPE ) return LW_FALSE;

	if ( TYPE_HASBBOX(utype) )
	{
		loc += sizeof(BOX2DFLOAT4);
	}

	if ( TYPE_HASSRID(utype) )
	{
		loc += 4;
	}

	/* For lines this is npoints, for polys it is nrings, for collections it is ngeoms */
	if ( lw_get_uint32(loc) > 0 )
		return LW_FALSE;
	else
		return LW_TRUE;	
#endif	
}

char
is_worth_caching_pglwgeom_bbox(const PG_LWGEOM *in)
{
#ifdef GSERIALIZED_ON
	lwerror("is_worth_caching_pglwgeom_bbox called!");
	return false;
#else
	if ( pglwgeom_get_type(in) == POINTTYPE ) return false;
	return true;
#endif
}

