#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CONTEXT_PG 0
#define CONTEXT_SA 1


#ifdef STANDALONE
#define DEFAULT_CONTEXT CONTEXT_SA
#else
#define DEFAULT_CONTEXT CONTEXT_PG
#endif

/* Global variables */
#if DEFAULT_CONTEXT == CONTEXT_SA
#include "liblwgeom.h"
lwallocator lwalloc_var = default_allocator;
lwreallocator lwrealloc_var = default_reallocator;
lwfreeor lwfree_var = default_freeor;
lwreporter lwerror = default_errorreporter;
lwreporter lwnotice = default_noticereporter;
#else
#include "lwgeom_pg.h"
#include "liblwgeom.h"

lwallocator lwalloc_var = pg_alloc;
lwreallocator lwrealloc_var = pg_realloc;
lwfreeor lwfree_var = pg_free;
lwreporter lwerror = pg_error;
lwreporter lwnotice = pg_notice;
#endif

static char *lwgeomTypeName[] = {
	"Unknown",
	"Point",
	"Line",
	"Polygon",
	"MultiPoint",
	"MultiLine",
	"MultiPolygon",
	"GeometryCollection"
};

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
	printf("%s\n", msg);
	va_end(ap);
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
	fprintf(stderr, "%s\n", msg);
	va_end(ap);
	free(msg);
	exit(1);
}

const char *
lwgeom_typename(int type)
{
	return lwgeomTypeName[type];
}

void *
lwalloc(size_t size)
{
#ifdef DEBUG_ALLOCS
	void *mem = lwalloc_var(size);
	lwnotice("lwalloc: %d@%p", size, mem);
	return mem;
#else // ! DEBUG_ALLOCS
	return lwalloc_var(size);
#endif
}

void *
lwrealloc(void *mem, size_t size)
{
	return lwrealloc_var(mem, size);
}

void
lwfree(void *mem)
{
	return lwfree_var(mem);
}

/*
 * Removes trailing zeros and dot for a %f formatted number.
 * Modifies input.
 */
void
trim_trailing_zeros(char *str)
{
	char *ptr, *totrim=NULL;
	int len;
	int i;

	//lwnotice("input: %s", str);
	
	ptr = strchr(str, '.');
	if ( ! ptr ) return; // no dot, no decimal digits

	//lwnotice("ptr: %s", ptr);

	len = strlen(ptr);
	for (i=len-1; i; i--)
	{
		if ( ptr[i] != '0' ) break;
		totrim=&ptr[i];
	}
	if ( totrim )
	{
		if ( ptr == totrim-1 ) *ptr = '\0';
		else *totrim = '\0';
	}
	
	//lwnotice("output: %s", str);
}

char getMachineEndian()
{
	static int endian_check_int = 1; // dont modify this!!!

	return *((char *) &endian_check_int); // 0 = big endian | xdr,
					      // 1 = little endian | ndr
}


