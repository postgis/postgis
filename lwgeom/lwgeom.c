#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lwgeom_pg.h"
#include "liblwgeom.h"

#define CONTEXT_PG 0
#define CONTEXT_STANDALONE 1

/* Define this to the default context liblwgeom runs with */
#define DEFAULT_CONTEXT CONTEXT_PG


/* globals */
#if DEFAULT_CONTEXT == CONTEXT_PG
lwallocator lwalloc = pg_alloc;
lwreallocator lwrealloc = pg_realloc;
lwfreeor lwfree = pg_free;
lwreporter lwerror = pg_error;
lwreporter lwnotice = pg_notice;
#else
lwallocator lwalloc = default_allocator;
lwreallocator lwrealloc = default_reallocator;
lwfreeor lwfree = default_freeor;
lwreporter lwerror = default_errorreporter;
lwreporter lwnotice = default_noticereporter;
#endif

LWGEOM *
lwgeom_deserialize(char *srl)
{
	LWGEOM *result;
	int type = lwgeom_getType(srl[0]);

	result = lwalloc(sizeof(LWGEOM));
	result->type = type;
	switch (type)
	{
		case POINTTYPE:
			result->point = lwpoint_deserialize(srl);
			break;
		case LINETYPE:
			result->line = lwline_deserialize(srl);
			break;
		case POLYGONTYPE:
			result->poly = lwpoly_deserialize(srl);
			break;
		case MULTIPOINTTYPE:
			result->mpoint = lwmpoint_deserialize(srl);
			break;
		case MULTILINETYPE:
			result->mline = lwmline_deserialize(srl);
			break;
		case MULTIPOLYGONTYPE:
			result->mpoly = lwmpoly_deserialize(srl);
			break;
		case COLLECTIONTYPE:
			result->collection = lwcollection_deserialize(srl);
			break;
		default:
			lwerror("Unknown geometry type: %d", type);
			return NULL;
	}

	return result;
}

void *
default_allocator(size_t size)
{
	void * result;
	result = malloc(size);
	return result;
}

void *
default_reallocator(void *mem, size_t size)
{
	void * result;
	result = realloc(mem, size);
	return result;
}

void
default_errorreporter(const char *fmt, ...)
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
	va_end(ap);
	fprintf(stderr, "%s", msg);
	free(msg);
}

void
default_noticereporter(const char *fmt, ...)
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
	va_end(ap);
	fprintf(stderr, "%s", msg);
	free(msg);
}

void
default_freeor(void *ptr)
{
	free(ptr);
}

