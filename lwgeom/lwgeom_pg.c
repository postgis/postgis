#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "postgres.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

void *
pg_alloc(size_t size)
{
	void * result;
	result = palloc(size);
	//elog(NOTICE,"  palloc(%d) = %p", size, result);
	return result;
}

void *
pg_realloc(void *mem, size_t size)
{
	void * result;
	result = repalloc(mem, size);
	return result;
}

void
pg_free(void *ptr)
{
	pfree(ptr);
}

void
pg_error(const char *fmt, ...)
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
	elog(ERROR, "%s", msg);
	free(msg);
}

void
pg_notice(const char *fmt, ...)
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
	elog(NOTICE, "%s", msg);
	free(msg);
}

void
init_pg_func()
{
	lwalloc = pg_alloc;
	lwrealloc = pg_realloc;
	lwfree = pg_free;
	lwerror = pg_error;
	lwnotice = pg_notice;
}
