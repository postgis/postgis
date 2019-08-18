/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2004-2015 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2006 Mark Leslie <mark.leslie@lisasoft.com>
 * Copyright (C) 2008-2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2009-2015 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2010 Olivier Courtin <olivier.courtin@camptocamp.com>
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h> /* for tolower */

/* Global variables */
#include "../postgis_config.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

/* Default allocators */
static void * default_allocator(size_t size);
static void default_freeor(void *mem);
static void * default_reallocator(void *mem, size_t size);
lwallocator lwalloc_var = default_allocator;
lwreallocator lwrealloc_var = default_reallocator;
lwfreeor lwfree_var = default_freeor;

/* Default reporters */
static void default_noticereporter(const char *fmt, va_list ap);
static void default_errorreporter(const char *fmt, va_list ap);
lwreporter lwnotice_var = default_noticereporter;
lwreporter lwerror_var = default_errorreporter;

/* Default logger */
static void default_debuglogger(int level, const char *fmt, va_list ap);
lwdebuglogger lwdebug_var = default_debuglogger;

#define LW_MSG_MAXLEN 256

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
 * Default allocators
 *
 * We include some default allocators that use malloc/free/realloc
 * along with stdout/stderr since this is the most common use case
 *
 */

static void *
default_allocator(size_t size)
{
	void *mem = malloc(size);
	return mem;
}

static void
default_freeor(void *mem)
{
	free(mem);
}

static void *
default_reallocator(void *mem, size_t size)
{
	void *ret = realloc(mem, size);
	return ret;
}

/*
 * Default lwnotice/lwerror handlers
 *
 * Since variadic functions cannot pass their parameters directly, we need
 * wrappers for these functions to convert the arguments into a va_list
 * structure.
 */

static void
default_noticereporter(const char *fmt, va_list ap)
{
	char msg[LW_MSG_MAXLEN+1];
	vsnprintf (msg, LW_MSG_MAXLEN, fmt, ap);
	msg[LW_MSG_MAXLEN]='\0';
	fprintf(stderr, "%s\n", msg);
}

static void
default_debuglogger(int level, const char *fmt, va_list ap)
{
	char msg[LW_MSG_MAXLEN+1];
	if ( POSTGIS_DEBUG_LEVEL >= level )
	{
		/* Space pad the debug output */
		int i;
		for ( i = 0; i < level; i++ )
			msg[i] = ' ';
		vsnprintf(msg+i, LW_MSG_MAXLEN-i, fmt, ap);
		msg[LW_MSG_MAXLEN]='\0';
		fprintf(stderr, "%s\n", msg);
	}
}

static void
default_errorreporter(const char *fmt, va_list ap)
{
	char msg[LW_MSG_MAXLEN+1];
	vsnprintf (msg, LW_MSG_MAXLEN, fmt, ap);
	msg[LW_MSG_MAXLEN]='\0';
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

/**
 * This function is called by programs which want to set up custom handling
 * for memory management and error reporting
 *
 * Only non-NULL values change their respective handler
 */
void
lwgeom_set_handlers(lwallocator allocator, lwreallocator reallocator,
	        lwfreeor freeor, lwreporter errorreporter,
	        lwreporter noticereporter) {

	if ( allocator ) lwalloc_var = allocator;
	if ( reallocator ) lwrealloc_var = reallocator;
	if ( freeor ) lwfree_var = freeor;

	if ( errorreporter ) lwerror_var = errorreporter;
	if ( noticereporter ) lwnotice_var = noticereporter;
}

void
lwgeom_set_debuglogger(lwdebuglogger debuglogger) {

	if ( debuglogger ) lwdebug_var = debuglogger;
}

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

void
lwdebug(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	(*lwdebug_var)(level, fmt, ap);

	va_end(ap);
}

const char*
lwtype_name(uint8_t type)
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

char *
lwstrdup(const char* a)
{
	size_t l = strlen(a)+1;
	char *b = lwalloc(l);
	strncpy(b, a, l);
	return b;
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
 *    1 - end truncation (i.e. characters are removed from the end)
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
				strncat(output, "...", 4);
				strncat(output, outstart, maxlength - 3);
			}
			else
			{
				/* maxlength is too small; just output "..." */
				strncat(output, "...", 4);
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
				strncat(output, "...", 4);
			}
			else
			{
				/* maxlength is too small; just output "..." */
				strncat(output, "...", 4);
			}
		}
	}

	return output;
}

int32_t
clamp_srid(int32_t srid)
{
	int newsrid = srid;

	if ( newsrid <= 0 ) {
		if ( newsrid != SRID_UNKNOWN ) {
			newsrid = SRID_UNKNOWN;
			lwnotice("SRID value %d converted to the officially unknown SRID value %d", srid, newsrid);
		}
	} else if ( srid > SRID_MAXIMUM ) {
    newsrid = SRID_USER_MAXIMUM + 1 +
      /* -1 is to reduce likelyhood of clashes */
      /* NOTE: must match implementation in postgis_restore.pl */
      ( srid % ( SRID_MAXIMUM - SRID_USER_MAXIMUM - 1 ) );
		lwnotice("SRID value %d > SRID_MAXIMUM converted to %d", srid, newsrid);
	}

	return newsrid;
}




/* Structure for the type array */
struct geomtype_struct
{
	char *typename;
	int type;
	int z;
	int m;
};

/* Type array. Note that the order of this array is important in
   that any typename in the list must *NOT* occur within an entry
   before it. Otherwise if we search for "POINT" at the top of the
   list we would also match MULTIPOINT, for example. */

struct geomtype_struct geomtype_struct_array[] =
{
	{ "GEOMETRYCOLLECTIONZM", COLLECTIONTYPE, 1, 1 },
	{ "GEOMETRYCOLLECTIONZ", COLLECTIONTYPE, 1, 0 },
	{ "GEOMETRYCOLLECTIONM", COLLECTIONTYPE, 0, 1 },
	{ "GEOMETRYCOLLECTION", COLLECTIONTYPE, 0, 0 },

	{ "GEOMETRYZM", 0, 1, 1 },
	{ "GEOMETRYZ", 0, 1, 0 },
	{ "GEOMETRYM", 0, 0, 1 },
	{ "GEOMETRY", 0, 0, 0 },

	{ "POLYHEDRALSURFACEZM", POLYHEDRALSURFACETYPE, 1, 1 },
	{ "POLYHEDRALSURFACEZ", POLYHEDRALSURFACETYPE, 1, 0 },
	{ "POLYHEDRALSURFACEM", POLYHEDRALSURFACETYPE, 0, 1 },
	{ "POLYHEDRALSURFACE", POLYHEDRALSURFACETYPE, 0, 0 },

	{ "TINZM", TINTYPE, 1, 1 },
	{ "TINZ", TINTYPE, 1, 0 },
	{ "TINM", TINTYPE, 0, 1 },
	{ "TIN", TINTYPE, 0, 0 },

	{ "CIRCULARSTRINGZM", CIRCSTRINGTYPE, 1, 1 },
	{ "CIRCULARSTRINGZ", CIRCSTRINGTYPE, 1, 0 },
	{ "CIRCULARSTRINGM", CIRCSTRINGTYPE, 0, 1 },
	{ "CIRCULARSTRING", CIRCSTRINGTYPE, 0, 0 },

	{ "COMPOUNDCURVEZM", COMPOUNDTYPE, 1, 1 },
	{ "COMPOUNDCURVEZ", COMPOUNDTYPE, 1, 0 },
	{ "COMPOUNDCURVEM", COMPOUNDTYPE, 0, 1 },
	{ "COMPOUNDCURVE", COMPOUNDTYPE, 0, 0 },

	{ "CURVEPOLYGONZM", CURVEPOLYTYPE, 1, 1 },
	{ "CURVEPOLYGONZ", CURVEPOLYTYPE, 1, 0 },
	{ "CURVEPOLYGONM", CURVEPOLYTYPE, 0, 1 },
	{ "CURVEPOLYGON", CURVEPOLYTYPE, 0, 0 },

	{ "MULTICURVEZM", MULTICURVETYPE, 1, 1 },
	{ "MULTICURVEZ", MULTICURVETYPE, 1, 0 },
	{ "MULTICURVEM", MULTICURVETYPE, 0, 1 },
	{ "MULTICURVE", MULTICURVETYPE, 0, 0 },

	{ "MULTISURFACEZM", MULTISURFACETYPE, 1, 1 },
	{ "MULTISURFACEZ", MULTISURFACETYPE, 1, 0 },
	{ "MULTISURFACEM", MULTISURFACETYPE, 0, 1 },
	{ "MULTISURFACE", MULTISURFACETYPE, 0, 0 },

	{ "MULTILINESTRINGZM", MULTILINETYPE, 1, 1 },
	{ "MULTILINESTRINGZ", MULTILINETYPE, 1, 0 },
	{ "MULTILINESTRINGM", MULTILINETYPE, 0, 1 },
	{ "MULTILINESTRING", MULTILINETYPE, 0, 0 },

	{ "MULTIPOLYGONZM", MULTIPOLYGONTYPE, 1, 1 },
	{ "MULTIPOLYGONZ", MULTIPOLYGONTYPE, 1, 0 },
	{ "MULTIPOLYGONM", MULTIPOLYGONTYPE, 0, 1 },
	{ "MULTIPOLYGON", MULTIPOLYGONTYPE, 0, 0 },

	{ "MULTIPOINTZM", MULTIPOINTTYPE, 1, 1 },
	{ "MULTIPOINTZ", MULTIPOINTTYPE, 1, 0 },
	{ "MULTIPOINTM", MULTIPOINTTYPE, 0, 1 },
	{ "MULTIPOINT", MULTIPOINTTYPE, 0, 0 },

	{ "LINESTRINGZM", LINETYPE, 1, 1 },
	{ "LINESTRINGZ", LINETYPE, 1, 0 },
	{ "LINESTRINGM", LINETYPE, 0, 1 },
	{ "LINESTRING", LINETYPE, 0, 0 },

	{ "TRIANGLEZM", TRIANGLETYPE, 1, 1 },
	{ "TRIANGLEZ", TRIANGLETYPE, 1, 0 },
	{ "TRIANGLEM", TRIANGLETYPE, 0, 1 },
	{ "TRIANGLE", TRIANGLETYPE, 0, 0 },

	{ "POLYGONZM", POLYGONTYPE, 1, 1 },
	{ "POLYGONZ", POLYGONTYPE, 1, 0 },
	{ "POLYGONM", POLYGONTYPE, 0, 1 },
	{ "POLYGON", POLYGONTYPE, 0, 0 },

	{ "POINTZM", POINTTYPE, 1, 1 },
	{ "POINTZ", POINTTYPE, 1, 0 },
	{ "POINTM", POINTTYPE, 0, 1 },
	{ "POINT", POINTTYPE, 0, 0 }

};
#define GEOMTYPE_STRUCT_ARRAY_LEN (sizeof geomtype_struct_array/sizeof(struct geomtype_struct))

/*
* We use a very simple upper case mapper here, because the system toupper() function
* is locale dependent and may have trouble mapping lower case strings to the upper
* case ones we expect (see, the "Turkisk I", http://www.i18nguy.com/unicode/turkish-i18n.html)
* We could also count on PgSQL sending us *lower* case inputs, as it seems to do that
* regardless of the case the user provides for the type arguments.
*/
const char dumb_upper_map[128] = "................................................0123456789.......ABCDEFGHIJKLMNOPQRSTUVWXYZ......ABCDEFGHIJKLMNOPQRSTUVWXYZ.....";

static char dumb_toupper(int in)
{
	if ( in < 0 || in > 127 )
		return '.';
	return dumb_upper_map[in];
}

lwflags_t lwflags(int hasz, int hasm, int geodetic)
{
	lwflags_t flags = 0;
	if (hasz)
		FLAGS_SET_Z(flags, 1);
	if (hasm)
		FLAGS_SET_M(flags, 1);
	if (geodetic)
		FLAGS_SET_GEODETIC(flags, 1);
	return flags;
}

/**
* Calculate type integer and dimensional flags from string input.
* Case insensitive, and insensitive to spaces at front and back.
* Type == 0 in the case of the string "GEOMETRY" or "GEOGRAPHY".
* Return LW_SUCCESS for success.
*/
int geometry_type_from_string(const char *str, uint8_t *type, int *z, int *m)
{
	char *tmpstr;
	size_t tmpstartpos, tmpendpos;
	size_t i;

	assert(str);
	assert(type);
	assert(z);
	assert(m);

	/* Initialize. */
	*type = 0;
	*z = 0;
	*m = 0;

	/* Locate any leading/trailing spaces */
	tmpstartpos = 0;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] != ' ')
		{
			tmpstartpos = i;
			break;
		}
	}

	tmpendpos = strlen(str) - 1;
	for (i = strlen(str) - 1; i != 0; i--)
	{
		if (str[i] != ' ')
		{
			tmpendpos = i;
			break;
		}
	}

	/* Copy and convert to upper case for comparison */
	tmpstr = lwalloc(tmpendpos - tmpstartpos + 2);
	for (i = tmpstartpos; i <= tmpendpos; i++)
		tmpstr[i - tmpstartpos] = dumb_toupper(str[i]);

	/* Add NULL to terminate */
	tmpstr[i - tmpstartpos] = '\0';

	/* Now check for the type */
	for (i = 0; i < GEOMTYPE_STRUCT_ARRAY_LEN; i++)
	{
		if (!strcmp(tmpstr, geomtype_struct_array[i].typename))
		{
			*type = geomtype_struct_array[i].type;
			*z = geomtype_struct_array[i].z;
			*m = geomtype_struct_array[i].m;

			lwfree(tmpstr);

			return LW_SUCCESS;
		}

	}

	lwfree(tmpstr);

	return LW_FAILURE;
}






