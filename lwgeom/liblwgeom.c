#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lwgeom_pg.h"
#include "liblwgeom.h"

#define CONTEXT_PG 0
#define CONTEXT_SA 1

#define DEFAULT_CONTEXT CONTEXT_PG

/* Global variables */
#if DEFAULT_CONTEXT == SA
lwallocator lwalloc = default_allocator;
lwreallocator lwrealloc = default_reallocator;
lwfreeor lwfree = default_freeor;
lwreporter lwerror = default_errorreporter;
lwreporter lwnotice = default_noticereporter;
#else
lwallocator lwalloc = pg_alloc;
lwreallocator lwrealloc = pg_realloc;
lwfreeor lwfree = pg_free;
lwreporter lwerror = pg_error;
lwreporter lwnotice = pg_notice;
#endif


void *
default_allocator(size_t size)
{
	void *mem = malloc(size);
	return mem;
}

void
default_freeor(void *mem)
{
	free(mem);
}

void *
default_reallocator(void *mem, size_t size)
{
	void *ret = realloc(mem, size);
	return ret;
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
	printf("%s", msg);
	free(msg);
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
