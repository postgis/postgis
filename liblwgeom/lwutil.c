#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


/* Global variables */
#include "../postgis_config.h"
#include "liblwgeom.h"

void *init_allocator(size_t size);
void init_freeor(void *mem);
void *init_reallocator(void *mem, size_t size);
void init_noticereporter(const char *fmt, va_list ap);
void init_errorreporter(const char *fmt, va_list ap);

lwallocator lwalloc_var = init_allocator;
lwreallocator lwrealloc_var = init_reallocator;
lwfreeor lwfree_var = init_freeor;
lwreporter lwnotice_var = init_noticereporter;
lwreporter lwerror_var = init_errorreporter;

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


/*
 * lwnotice/lwerror handlers
 *
 * Since variadic functions cannot pass their parameters directly, we need
 * wrappers for these functions to convert the arguments into a va_list
 * structure.
 */

void
lwnotice(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	(*lwnotice_var)(fmt, ap);

	va_end(ap);
}

void
lwerror(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	(*lwerror_var)(fmt, ap);

	va_end(ap);
}

/*
 * Initialisation allocators
 *
 * These are used the first time any of the allocators are called
 * to enable executables/libraries that link into liblwgeom to
 * be able to set up their own allocators. This is mainly useful
 * for older PostgreSQL versions that don't have functions that
 * are called upon startup.
 */

void *
init_allocator(size_t size)
{
	lwgeom_init_allocators();

	return lwalloc_var(size);
}

void 
init_freeor(void *mem)
{
	lwgeom_init_allocators();

	lwfree_var(mem);
}

void *
init_reallocator(void *mem, size_t size)
{
	lwgeom_init_allocators();

	return lwrealloc_var(mem, size);
}

void
init_noticereporter(const char *fmt, va_list ap)
{
	lwgeom_init_allocators();

	(*lwnotice_var)(fmt, ap);
}
	
void
init_errorreporter(const char *fmt, va_list ap)
{
	lwgeom_init_allocators();

	(*lwerror_var)(fmt, ap);
}


/*
 * Default allocators
 *
 * We include some default allocators that use malloc/free/realloc
 * along with stdout/stderr since this is the most common use case
 *
 */

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
default_noticereporter(const char *fmt, va_list ap)
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
	printf("%s\n", msg);
	free(msg);
}

void
default_errorreporter(const char *fmt, va_list ap)
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
	fprintf(stderr, "%s\n", msg);
	free(msg);
	exit(1);
}


/*
 * This function should be called from lwgeom_init_allocators() by programs
 * which wish to use the default allocators above
 */

void lwgeom_install_default_allocators(void)
{
	lwalloc_var = default_allocator;
	lwrealloc_var = default_reallocator;
	lwfree_var = default_freeor;
	lwerror_var = default_errorreporter;	
	lwnotice_var = default_noticereporter;
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
	void *mem = lwalloc_var(size);
	LWDEBUGF(5, "lwalloc: %d@%p", size, mem);
	return mem;
}

void *
lwrealloc(void *mem, size_t size)
{
	LWDEBUGF(5, "lwrealloc: %d@%p", size, mem);
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

	LWDEBUGF(3, "input: %s", str);
	
	ptr = strchr(str, '.');
	if ( ! ptr ) return; /* no dot, no decimal digits */

	LWDEBUGF(3, "ptr: %s", ptr);

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
	
	LWDEBUGF(3, "output: %s", str);
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
