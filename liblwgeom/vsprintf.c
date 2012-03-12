/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2012 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"

int
lw_vasprintf (char **result, const char *format, va_list args)
{
#if HAVE_VASPRINTF
	return vasprintf(result, format, args);
#else
	return rpl_vasprintf(result, format, args);
#endif
}

int
lw_vsnprintf(char *s, size_t n, const char *format, va_list ap)
{
#if HAVE_VSNPRINTF
	return vsnprintf(s, n, format, ap);
#else
	return rpl_vsnprintf(s, n, format, ap);
#endif
}


int
lw_snprintf
#if __STDC__
(char *s, size_t n, const char *format, ...)
#else
(s, n, va_alist)
char *s;
size_t n;
va_dcl
#endif
{
	va_list args;
	int done;

#if __STDC__
	va_start (args, format);
#else
	char *format;
	va_start (args);
	format = va_arg (args, char *);
#endif
	done = lw_vsnprintf (s, n, format, args);
	va_end (args);

	return done;
}

int
lw_asprintf
#if __STDC__
(char **s, const char *format, ...)
#else
(s, va_alist)
char **s;
va_dcl
#endif
{
	va_list args;
	int done;

#if __STDC__
	va_start (args, format);
#else
	char *format;
	va_start (args);
	format = va_arg (args, char *);
#endif
	done = lw_vasprintf (s, format, args);
	va_end (args);

	return done;
}