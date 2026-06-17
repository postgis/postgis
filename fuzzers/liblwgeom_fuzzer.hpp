/******************************************************************************
 *
 * Project:  PostGIS
 * Purpose:  Shared liblwgeom fuzzer support
 *
 ******************************************************************************
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/

#ifndef POSTGIS_LIBLWGEOM_FUZZER_HPP
#define POSTGIS_LIBLWGEOM_FUZZER_HPP

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

extern "C" {
#include "liblwgeom.h"
}

static jmp_buf postgis_lwgeom_fuzzer_jmp_buf;
static void **postgis_lwgeom_fuzzer_ptrs = NULL;
static size_t postgis_lwgeom_fuzzer_count = 0;
static size_t postgis_lwgeom_fuzzer_capacity = 0;

#define POSTGIS_LWGEOM_FUZZER_SETJMP() setjmp(postgis_lwgeom_fuzzer_jmp_buf)

static int
postgis_lwgeom_fuzzer_track_add(void *ptr)
{
	void **new_ptrs;
	size_t new_capacity;

	if (postgis_lwgeom_fuzzer_count == postgis_lwgeom_fuzzer_capacity)
	{
		new_capacity = postgis_lwgeom_fuzzer_capacity ? postgis_lwgeom_fuzzer_capacity * 2 : 64;
		new_ptrs = (void **)realloc(postgis_lwgeom_fuzzer_ptrs, new_capacity * sizeof(void *));
		if (new_ptrs == NULL)
			return 0;
		postgis_lwgeom_fuzzer_ptrs = new_ptrs;
		postgis_lwgeom_fuzzer_capacity = new_capacity;
	}

	postgis_lwgeom_fuzzer_ptrs[postgis_lwgeom_fuzzer_count++] = ptr;
	return 1;
}

static void
postgis_lwgeom_fuzzer_track_remove(void *ptr)
{
	for (size_t i = 0; i < postgis_lwgeom_fuzzer_count; i++)
	{
		if (postgis_lwgeom_fuzzer_ptrs[i] == ptr)
		{
			postgis_lwgeom_fuzzer_ptrs[i] = postgis_lwgeom_fuzzer_ptrs[--postgis_lwgeom_fuzzer_count];
			return;
		}
	}
}

static void
postgis_lwgeom_fuzzer_cleanup_allocations(void)
{
	for (size_t i = 0; i < postgis_lwgeom_fuzzer_count; i++)
		free(postgis_lwgeom_fuzzer_ptrs[i]);
	postgis_lwgeom_fuzzer_count = 0;
}

static void *
postgis_lwgeom_fuzzer_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr != NULL && !postgis_lwgeom_fuzzer_track_add(ptr))
	{
		free(ptr);
		return NULL;
	}
	return ptr;
}

static void *
postgis_lwgeom_fuzzer_realloc(void *ptr, size_t size)
{
	void *new_ptr;

	if (ptr == NULL)
		return postgis_lwgeom_fuzzer_malloc(size);
	if (size == 0)
	{
		postgis_lwgeom_fuzzer_track_remove(ptr);
		free(ptr);
		return NULL;
	}

	new_ptr = realloc(ptr, size);
	if (new_ptr == NULL)
		return NULL;

	for (size_t i = 0; i < postgis_lwgeom_fuzzer_count; i++)
	{
		if (postgis_lwgeom_fuzzer_ptrs[i] == ptr)
		{
			postgis_lwgeom_fuzzer_ptrs[i] = new_ptr;
			return new_ptr;
		}
	}

	if (!postgis_lwgeom_fuzzer_track_add(new_ptr))
	{
		free(new_ptr);
		return NULL;
	}
	return new_ptr;
}

static void
postgis_lwgeom_fuzzer_free(void *ptr)
{
	if (ptr == NULL)
		return;
	postgis_lwgeom_fuzzer_track_remove(ptr);
	free(ptr);
}

extern "C" {
static void
postgis_lwgeom_fuzzer_noticereporter(const char *, va_list)
{}

static void
postgis_lwgeom_fuzzer_errorreporter(const char *, va_list)
{
	postgis_lwgeom_fuzzer_cleanup_allocations();
	longjmp(postgis_lwgeom_fuzzer_jmp_buf, 1);
}

static void
postgis_lwgeom_fuzzer_debuglogger(int, const char *, va_list)
{}
}

static void
postgis_lwgeom_fuzzer_initialize(void)
{
	/* liblwgeom reports parser errors through the configured error reporter.
	 * Track liblwgeom allocations so longjmp error paths release allocations
	 * made before the jump.
	 */
	lwgeom_set_handlers(postgis_lwgeom_fuzzer_malloc,
			    postgis_lwgeom_fuzzer_realloc,
			    postgis_lwgeom_fuzzer_free,
			    postgis_lwgeom_fuzzer_errorreporter,
			    postgis_lwgeom_fuzzer_noticereporter);
	lwgeom_set_debuglogger(postgis_lwgeom_fuzzer_debuglogger);
}

#endif
