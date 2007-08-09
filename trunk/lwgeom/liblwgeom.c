#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CONTEXT_PG 0
#define CONTEXT_SA 1

/* #define PGIS_DEBUG_ALLOCS 1 */


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
	"GeometryCollection",
        "Curve",
        "CompoundString",
        "Invalid Type",  /* POINTTYPEI */
        "Invalid Type",  /* LINETYPEI */
        "Invalid Type",  /* POLYTYPEI */
        "CurvePolygon",
        "MultiCurve",
        "MultiSurface"
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
	// something went wrong somewhere
	if ( type < 0 || type > 12 ) {
		// assert(0);
		return "Invalid type";
	}
	return lwgeomTypeName[type];
}

void *
lwalloc(size_t size)
{
#ifdef PGIS_DEBUG_ALLOCS
	void *mem = lwalloc_var(size);
	lwnotice("lwalloc: %d@%p", size, mem);
	return mem;
#else /* ! PGIS_DEBUG_ALLOCS */
	return lwalloc_var(size);
#endif
}

void *
lwrealloc(void *mem, size_t size)
{
#ifdef PGIS_DEBUG_ALLOCS
	lwnotice("lwrealloc: %d@%p", size, mem);
#endif
	return lwrealloc_var(mem, size);
}

void
lwfree(void *mem)
{
	lwfree_var(mem);
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

#ifdef PGIS_DEBUG
	lwnotice("input: %s", str);
#endif
	
	ptr = strchr(str, '.');
	if ( ! ptr ) return; /* no dot, no decimal digits */

#ifdef PGIS_DEBUG
	lwnotice("ptr: %s", ptr);
#endif

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
	
#ifdef PGIS_DEBUG
	lwnotice("output: %s", str);
#endif
}

char
getMachineEndian(void)
{
	static int endian_check_int = 1; /* dont modify this!!! */

	return *((char *) &endian_check_int); /* 0 = big endian | xdr,
					       * 1 = little endian | ndr
	                                       */
}


void
errorIfSRIDMismatch(int srid1, int srid2)
{
	if ( srid1 != srid2 )
	{
		lwerror("Operation on mixed SRID geometries");
	}
}
