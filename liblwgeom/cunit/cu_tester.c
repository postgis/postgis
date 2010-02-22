/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "liblwgeom.h"
#include "cu_tester.h"


/**
 * CUnit error handler
 * Log message in a global var instead of printing in stderr
 *
 * CAUTION: Not stop execution on lwerror case !!!
 */
static void
cu_errorreporter(const char *fmt, va_list ap)
{
	char *msg;

	/** This is a GNU extension.
	* Dunno how to handle errors here.
	 */
	if (!lw_vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}

	strncpy(cu_error_msg, msg, MAX_CUNIT_ERROR_LENGTH);
}

void
cu_error_msg_reset()
{
	memset(cu_error_msg, '\0', MAX_CUNIT_ERROR_LENGTH);
}

/*
** Set up liblwgeom to run in stand-alone mode using the
** usual system memory handling functions.
*/
void lwgeom_init_allocators(void)
{
	lwalloc_var = default_allocator;
	lwrealloc_var = default_reallocator;
	lwfree_var = default_freeor;
	lwnotice_var = default_noticereporter;
	lwerror_var = cu_errorreporter;
}

/*
** The main() function for setting up and running the tests.
** Returns a CUE_SUCCESS on successful running, another
** CUnit error code on failure.
*/
int main()
{

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* Add the algorithms suite to the registry */
	if (NULL == register_cg_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the measures suite to the registry */
	if (NULL == register_measures_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the wkt suite to the registry */
	if (NULL == register_wkt_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the libgeom suite to the registry */
	if (NULL == register_libgeom_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the geodetic suite to the registry */
	if (NULL == register_geodetic_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the homogenize suite to the registry */
	if (NULL == register_homogenize_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the gml suite to the registry */
	if (NULL == register_out_gml_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the kml suite to the registry */
	if (NULL == register_out_kml_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the geojson suite to the registry */
	if (NULL == register_out_geojson_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Add the svg suite to the registry */
	if (NULL == register_out_svg_suite())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
