/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "liblwgeom_internal.h"
#include "cu_tester.h"

/* ADD YOUR SUITE HERE (1 of 2) */
extern CU_SuiteInfo print_suite;
extern CU_SuiteInfo algorithms_suite;
extern CU_SuiteInfo misc_suite;
extern CU_SuiteInfo ptarray_suite;
extern CU_SuiteInfo measures_suite;
extern CU_SuiteInfo wkt_out_suite;
extern CU_SuiteInfo wkt_in_suite;
extern CU_SuiteInfo wkb_out_suite;
extern CU_SuiteInfo wkb_in_suite;
extern CU_SuiteInfo libgeom_suite;
extern CU_SuiteInfo surface_suite;
extern CU_SuiteInfo geodetic_suite;
extern CU_SuiteInfo geos_suite;
extern CU_SuiteInfo homogenize_suite;
extern CU_SuiteInfo out_gml_suite;
extern CU_SuiteInfo out_kml_suite;
extern CU_SuiteInfo out_geojson_suite;
extern CU_SuiteInfo out_svg_suite;
extern CU_SuiteInfo out_x3d_suite;

/*
** The main() function for setting up and running the tests.
** Returns a CUE_SUCCESS on successful running, another
** CUnit error code on failure.
*/
int main(int argc, char *argv[])
{
	/* ADD YOUR SUITE HERE (2 of 2) */
	CU_SuiteInfo suites[] =
	{
		print_suite,
		misc_suite,
		ptarray_suite,
		algorithms_suite,
		measures_suite,
		wkt_out_suite,
		wkt_in_suite,
		wkb_out_suite,
		wkb_in_suite,
		libgeom_suite,
		surface_suite,
		geodetic_suite,
		geos_suite,
		homogenize_suite,
		out_gml_suite,
		out_kml_suite,
		out_geojson_suite,
		out_svg_suite,
		out_x3d_suite,
		CU_SUITE_INFO_NULL
	};

	int index;
	char *suite_name;
	CU_pSuite suite_to_run;
	char *test_name;
	CU_pTest test_to_run;
	CU_ErrorCode errCode = 0;
	CU_pTestRegistry registry;
	int num_run;
	int num_failed;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
	{
		errCode = CU_get_error();
		printf("    Error attempting to initialize registry: %d.  See CUError.h for error code list.\n", errCode);
		return errCode;
	}

	/* Register all the test suites. */
	if (CUE_SUCCESS != CU_register_suites(suites))
	{
		errCode = CU_get_error();
		printf("    Error attempting to register test suites: %d.  See CUError.h for error code list.\n", errCode);
		return errCode;
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	if (argc <= 1)
	{
		errCode = CU_basic_run_tests();
	}
	else
	{
		/* NOTE: The cunit functions used here (CU_get_registry, CU_get_suite_by_name, and CU_get_test_by_name) are
		 *       listed with the following warning: "Internal CUnit system functions.  Should not be routinely called by users."
		 *       However, there didn't seem to be any other way to get tests by name, so we're calling them. */
		registry = CU_get_registry();
		for (index = 1; index < argc; index++)
		{
			suite_name = argv[index];
			test_name = NULL;
			suite_to_run = CU_get_suite_by_name(suite_name, registry);
			if (NULL == suite_to_run)
			{
				/* See if it's a test name instead of a suite name. */
				suite_to_run = registry->pSuite;
				while (suite_to_run != NULL)
				{
					test_to_run = CU_get_test_by_name(suite_name, suite_to_run);
					if (test_to_run != NULL)
					{
						/* It was a test name. */
						test_name = suite_name;
						suite_name = suite_to_run->pName;
						break;
					}
					suite_to_run = suite_to_run->pNext;
				}
			}
			if (suite_to_run == NULL)
			{
				printf("\n'%s' does not appear to be either a suite name or a test name.\n\n", suite_name);
			}
			else
			{
				if (test_name != NULL)
				{
					/* Run only this test. */
					printf("\nRunning test '%s' in suite '%s'.\n", test_name, suite_name);
					/* This should be CU_basic_run_test, but that method is broken, see:
					 *     https://sourceforge.net/tracker/?func=detail&aid=2851925&group_id=32992&atid=407088
					 * This one doesn't output anything for success, so we have to do it manually. */
					errCode = CU_run_test(suite_to_run, test_to_run);
					if (errCode != CUE_SUCCESS)
					{
						printf("    Error attempting to run tests: %d.  See CUError.h for error code list.\n", errCode);
					}
					else
					{
						num_run = CU_get_number_of_asserts();
						num_failed = CU_get_number_of_failures();
						printf("\n    %s - asserts - %3d passed, %3d failed, %3d total.\n\n",
						       (0 == num_failed ? "PASSED" : "FAILED"), (num_run - num_failed), num_failed, num_run);
					}
				}
				else
				{
					/* Run all the tests in the suite. */
					printf("\nRunning all tests in suite '%s'.\n", suite_name);
					/* This should be CU_basic_run_suite, but that method is broken, see:
					 *     https://sourceforge.net/tracker/?func=detail&aid=2851925&group_id=32992&atid=407088
					 * This one doesn't output anything for success, so we have to do it manually. */
					errCode = CU_run_suite(suite_to_run);
					if (errCode != CUE_SUCCESS)
					{
						printf("    Error attempting to run tests: %d.  See CUError.h for error code list.\n", errCode);
					}
					else
					{
						num_run = CU_get_number_of_tests_run();
						num_failed = CU_get_number_of_tests_failed();
						printf("\n    %s -   tests - %3d passed, %3d failed, %3d total.\n",
						       (0 == num_failed ? "PASSED" : "FAILED"), (num_run - num_failed), num_failed, num_run);
						num_run = CU_get_number_of_asserts();
						num_failed = CU_get_number_of_failures();
						printf("           - asserts - %3d passed, %3d failed, %3d total.\n\n",
						       (num_run - num_failed), num_failed, num_run);
					}
				}
			}
		}
		/* Presumably if the CU_basic_run_[test|suite] functions worked, we wouldn't have to do this. */
		CU_basic_show_failures(CU_get_failure_list());
		printf("\n\n"); /* basic_show_failures leaves off line breaks. */
	}
	num_failed = CU_get_number_of_failures();
	CU_cleanup_registry();
	return num_failed;
}
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
	free(msg);
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

