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
	size_t gser_size;
	GSERIALIZED *gser;
	gser = gserialized_from_lwgeom(in, 0, &gser_size);
	SET_VARSIZE(gser, gser_size);
	return gser;
}

LWGEOM *
pglwgeom_deserialize(PG_LWGEOM *in)
{
	return lwgeom_from_gserialized(in);
}


PG_LWGEOM *
PG_LWGEOM_construct(uint8_t *ser, int srid, int wantbbox)
{
	lwerror("PG_LWGEOM_construct called!");
	return NULL;
}


/*
 * Set the SRID of a PG_LWGEOM
 * Returns a newly allocated PG_LWGEOM object.
 * Allocation will be done using the lwalloc.
 */
PG_LWGEOM *
pglwgeom_set_srid(PG_LWGEOM *lwgeom, int32 new_srid)
{
	gserialized_set_srid(lwgeom, new_srid);
	return lwgeom;
}

/*
 * get the SRID from the LWGEOM
 * none present => -1
 */
int
pglwgeom_get_srid(PG_LWGEOM *lwgeom)
{
	return gserialized_get_srid(lwgeom);
}

int
pglwgeom_get_type(const PG_LWGEOM *lwgeom)
{
	return gserialized_get_type(lwgeom);
}

int
pglwgeom_get_zm(const PG_LWGEOM *lwgeom)
{
	return 2 * FLAGS_GET_Z(lwgeom->flags) + FLAGS_GET_M(lwgeom->flags);
}

bool
pglwgeom_has_bbox(const PG_LWGEOM *lwgeom)
{
	return FLAGS_GET_BBOX(lwgeom->flags);
}

bool
pglwgeom_has_z(const PG_LWGEOM *lwgeom)
{
	return FLAGS_GET_Z(lwgeom->flags);
}

bool
pglwgeom_has_m(const PG_LWGEOM *lwgeom)
{
	return FLAGS_GET_M(lwgeom->flags);
}

PG_LWGEOM* pglwgeom_drop_bbox(PG_LWGEOM *geom)
{
	return gserialized_drop_gidx(geom);
}

size_t pglwgeom_size(const PG_LWGEOM *geom)
{
	return VARSIZE(geom) - VARHDRSZ;
};

int pglwgeom_ndims(const PG_LWGEOM *geom)
{
	return FLAGS_NDIMS(geom->flags);
}

int pglwgeom_getbox2d_p(const PG_LWGEOM *geom, BOX2DFLOAT4 *box)
{
	LWGEOM *lwgeom;
	int ret = gserialized_get_gbox_p(geom, box);
	if ( LW_FAILURE == ret ) {
		/* See http://trac.osgeo.org/postgis/ticket/1023 */
		lwgeom = lwgeom_from_gserialized(geom);
		ret = lwgeom_calculate_gbox(lwgeom, box);
                lwgeom_free(lwgeom);
	}
	return ret;
}

int
pglwgeom_is_empty(const PG_LWGEOM *geom)
{
	return gserialized_is_empty(geom);
}

char
is_worth_caching_pglwgeom_bbox(const PG_LWGEOM *in)
{
	lwerror("is_worth_caching_pglwgeom_bbox called!");
	return false;
}

