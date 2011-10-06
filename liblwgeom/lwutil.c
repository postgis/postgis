#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


/* Global variables */
#include "../postgis_config.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

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

static char *lwgeomTypeName[] =
{
	"Unknown",
	"Point",
	"LineString",
	"Polygon",
	"MultiPoint",
	"MultiLineString",
	"MultiPolygon",
	"GeometryCollection",
	"CircularString",
	"CompoundCurve",
	"CurvePolygon",
	"MultiCurve",
	"MultiSurface",
	"PolyhedralSurface",
	"Triangle",
	"Tin"
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


const char* lwtype_name(uint8_t type)
{
	if ( type > 15 )
	{
		/* assert(0); */
		return "Invalid type";
	}
	return lwgeomTypeName[(int ) type];
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


/*
 * Returns a new string which contains a maximum of maxlength characters starting
 * from startpos and finishing at endpos (0-based indexing). If the string is
 * truncated then the first or last characters are replaced by "..." as
 * appropriate.
 *
 * The caller should specify start or end truncation by setting the truncdirection
 * parameter as follows:
 *    0 - start truncation (i.e. characters are removed from the beginning)
 *    1 - end trunctation (i.e. characters are removed from the end)
 */

char *lwmessage_truncate(char *str, int startpos, int endpos, int maxlength, int truncdirection)
{
	char *output;
	char *outstart;

	/* Allocate space for new string */
	output = lwalloc(maxlength + 4);
	output[0] = '\0';

	/* Start truncation */
	if (truncdirection == 0)
	{
		/* Calculate the start position */
		if (endpos - startpos < maxlength)
		{
			outstart = str + startpos;
			strncat(output, outstart, endpos - startpos + 1);
		}
		else
		{
			if (maxlength >= 3)
			{
				/* Add "..." prefix */
				outstart = str + endpos + 1 - maxlength + 3;
				strncat(output, "...", 3);
				strncat(output, outstart, maxlength - 3);
			}
			else
			{
				/* maxlength is too small; just output "..." */
				strncat(output, "...", 3);
			}
		}
	}

	/* End truncation */
	if (truncdirection == 1)
	{
		/* Calculate the end position */
		if (endpos - startpos < maxlength)
		{
			outstart = str + startpos;
			strncat(output, outstart, endpos - startpos + 1);
		}
		else
		{
			if (maxlength >= 3)
			{
				/* Add "..." suffix */
				outstart = str + startpos;
				strncat(output, outstart, maxlength - 3);
				strncat(output, "...", 3);
			}
			else
			{
				/* maxlength is too small; just output "..." */
				strncat(output, "...", 3);
			}
		}
	}

	return output;
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
error_if_srid_mismatch(int srid1, int srid2)
{
	if ( srid1 != srid2 )
	{
		lwerror("Operation on mixed SRID geometries");
	}
}

int
clamp_srid(int srid)
{
	if ( srid <= 0 && srid != SRID_UNKNOWN ) {
		lwnotice("SRID value %d converted to the officially unknown SRID value %d", srid, SRID_UNKNOWN);
		srid = SRID_UNKNOWN;
	}
	return srid;
}
