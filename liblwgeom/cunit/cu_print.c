/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"

static void
test_lwpoint_to_latlon_assert_format(char *point_wkt, const char *format, const char *expected)
{
	LWPOINT * test_point = (LWPOINT*)lwgeom_from_wkt(point_wkt, LW_PARSER_CHECK_NONE);
	int num_old_failures, num_new_failures;
	char * actual;
	cu_error_msg_reset();
	actual = lwpoint_to_latlon(test_point, format);
	if (0 != strlen(cu_error_msg))
	{
		printf("\nAssert failed:\n\tFormat [%s] generated an error: %s\n", format, cu_error_msg);
		CU_FAIL();
	}
	num_old_failures = CU_get_number_of_failures();
	CU_ASSERT_STRING_EQUAL(actual, expected);
	num_new_failures = CU_get_number_of_failures();
	if (num_new_failures > num_old_failures)
	{
		printf("\nAssert failed:\n\t%s\t(actual)\n\t%s\t(expected)\n", actual, expected);
	}
	lwfree(actual);
	lwpoint_free(test_point);
}
static void
test_lwpoint_to_latlon_assert_error(char *point_wkt, const char *format)
{
	LWPOINT * test_point = (LWPOINT*)lwgeom_from_wkt(point_wkt, LW_PARSER_CHECK_NONE);
	cu_error_msg_reset();
	char* tmp = lwpoint_to_latlon(test_point, format);
	lwfree(tmp);
	if (0 == strlen(cu_error_msg))
	{
		printf("\nAssert failed:\n\tFormat [%s] did not generate an error.\n", format);
		CU_FAIL();
	}
	else
	{
		cu_error_msg_reset();
	}
	lwpoint_free(test_point);
}

/*
** Test points around the globe using the default format.  Null and empty string both mean use the default.
*/
static void
test_lwpoint_to_latlon_default_format(void)
{
	test_lwpoint_to_latlon_assert_format("POINT(0 0)",
					     NULL,
					     "0\xC2\xB0"
					     "0'0.000\"N 0\xC2\xB0"
					     "0'0.000\"E");
	test_lwpoint_to_latlon_assert_format("POINT(45.4545 12.34567)",
					     "",
					     "12\xC2\xB0"
					     "20'44.412\"N 45\xC2\xB0"
					     "27'16.200\"E");
	test_lwpoint_to_latlon_assert_format("POINT(180 90)",
					     NULL,
					     "90\xC2\xB0"
					     "0'0.000\"N 180\xC2\xB0"
					     "0'0.000\"E");
	test_lwpoint_to_latlon_assert_format("POINT(181 91)",
					     "",
					     "89\xC2\xB0"
					     "0'0.000\"N 1\xC2\xB0"
					     "0'0.000\"E");
	test_lwpoint_to_latlon_assert_format("POINT(180.0001 90.0001)",
					     NULL,
					     "89\xC2\xB0"
					     "59'59.640\"N 0\xC2\xB0"
					     "0'0.360\"E");
	test_lwpoint_to_latlon_assert_format("POINT(45.4545 -12.34567)",
					     "",
					     "12\xC2\xB0"
					     "20'44.412\"S 45\xC2\xB0"
					     "27'16.200\"E");
	test_lwpoint_to_latlon_assert_format("POINT(180 -90)",
					     NULL,
					     "90\xC2\xB0"
					     "0'0.000\"S 180\xC2\xB0"
					     "0'0.000\"E");
	test_lwpoint_to_latlon_assert_format("POINT(181 -91)",
					     "",
					     "89\xC2\xB0"
					     "0'0.000\"S 1\xC2\xB0"
					     "0'0.000\"E");
	test_lwpoint_to_latlon_assert_format("POINT(180.0001 -90.0001)",
					     NULL,
					     "89\xC2\xB0"
					     "59'59.640\"S 0\xC2\xB0"
					     "0'0.360\"E");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 12.34567)",
					     "",
					     "12\xC2\xB0"
					     "20'44.412\"N 45\xC2\xB0"
					     "27'16.200\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-180 90)",
					     NULL,
					     "90\xC2\xB0"
					     "0'0.000\"N 180\xC2\xB0"
					     "0'0.000\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-181 91)",
					     "",
					     "89\xC2\xB0"
					     "0'0.000\"N 1\xC2\xB0"
					     "0'0.000\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-180.0001 90.0001)",
					     NULL,
					     "89\xC2\xB0"
					     "59'59.640\"N 0\xC2\xB0"
					     "0'0.360\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)",
					     "",
					     "12\xC2\xB0"
					     "20'44.412\"S 45\xC2\xB0"
					     "27'16.200\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-180 -90)",
					     NULL,
					     "90\xC2\xB0"
					     "0'0.000\"S 180\xC2\xB0"
					     "0'0.000\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-181 -91)",
					     "",
					     "89\xC2\xB0"
					     "0'0.000\"S 1\xC2\xB0"
					     "0'0.000\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-180.0001 -90.0001)",
					     NULL,
					     "89\xC2\xB0"
					     "59'59.640\"S 0\xC2\xB0"
					     "0'0.360\"W");
	test_lwpoint_to_latlon_assert_format("POINT(-2348982391.123456 -238749827.34879)",
					     "",
					     "12\xC2\xB0"
					     "39'4.356\"N 31\xC2\xB0"
					     "7'24.442\"W");
	/* See https://trac.osgeo.org/postgis/ticket/3688 */
	test_lwpoint_to_latlon_assert_format("POINT (76.6 -76.6)",
					     NULL,
					     "76\xC2\xB0"
					     "36'0.000\"S 76\xC2\xB0"
					     "36'0.000\"E");
}

/*
 * Test all possible combinations of the orders of the parameters.
 */
static void
test_lwpoint_to_latlon_format_orders(void)
{
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C DD MM SS", "S 12 20 44 W 45 27 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C DD SS MM", "S 12 44 20 W 45 16 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C MM DD SS", "S 20 12 44 W 27 45 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C MM SS DD", "S 20 44 12 W 27 16 45");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C SS DD MM", "S 44 12 20 W 16 45 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "C SS MM DD", "S 44 20 12 W 16 27 45");

	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD C MM SS", "12 S 20 44 45 W 27 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD C SS MM", "12 S 44 20 45 W 16 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM C DD SS", "20 S 12 44 27 W 45 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM C SS DD", "20 S 44 12 27 W 16 45");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS C DD MM", "44 S 12 20 16 W 45 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS C MM DD", "44 S 20 12 16 W 27 45");

	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD MM C SS", "12 20 S 44 45 27 W 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD SS C MM", "12 44 S 20 45 16 W 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM DD C SS", "20 12 S 44 27 45 W 16");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM SS C DD", "20 44 S 12 27 16 W 45");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS DD C MM", "44 12 S 20 16 45 W 27");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS MM C DD", "44 20 S 12 16 27 W 45");

	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD MM SS C", "12 20 44 S 45 27 16 W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD SS MM C", "12 44 20 S 45 16 27 W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM DD SS C", "20 12 44 S 27 45 16 W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "MM SS DD C", "20 44 12 S 27 16 45 W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS DD MM C", "44 12 20 S 16 45 27 W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "SS MM DD C", "44 20 12 S 16 27 45 W");
}

/*
 * Test with and without the optional parameters.
 */
static void
test_lwpoint_to_latlon_optional_format(void)
{
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD.DDD", "-12.346 -45.455");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD.DDD C", "12.346 S 45.455 W");
	test_lwpoint_to_latlon_assert_format(
	    "POINT(-45.4545 -12.34567)", "DD.DDD MM.MMM", "-12.000 20.740 -45.000 27.270");
	test_lwpoint_to_latlon_assert_format(
	    "POINT(-45.4545 -12.34567)", "DD.DDD MM.MMM C", "12.000 20.740 S 45.000 27.270 W");
	test_lwpoint_to_latlon_assert_format(
	    "POINT(-45.4545 -12.34567)", "DD.DDD MM.MMM SS.SSS", "-12.000 20.000 44.412 -45.000 27.000 16.200");
	test_lwpoint_to_latlon_assert_format(
	    "POINT(-45.4545 -12.34567)", "DD.DDD MM.MMM SS.SSS C", "12.000 20.000 44.412 S 45.000 27.000 16.200 W");
}

static void
test_lwpoint_to_latlon_oddball_formats(void)
{
	test_lwpoint_to_latlon_assert_format(
	    "POINT(-45.4545 -12.34567)", "DD.DDDMM.MMMSS.SSSC", "12.00020.00044.412S 45.00027.00016.200W");
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DDMM.MMM", "-1220.740 -4527.270");
	/* "##." will be printed as "##" */
	test_lwpoint_to_latlon_assert_format("POINT(-45.4545 -12.34567)", "DD.MM.MMM", "-1220.740 -4527.270");
}

/*
 * Test using formats that should produce errors.
 */
static void
test_lwpoint_to_latlon_bad_formats(void)
{
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "DD.DDD SS.SSS");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "MM.MMM SS.SSS");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "DD.DDD SS.SSS DD");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "DD MM SS MM");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "DD MM SS SS");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "C DD.DDD C");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)",
					    "C \xC2"
					    "DD.DDD");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)", "C DD.DDD \xC2");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)",
					    "C DD\x80"
					    "MM ");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)",
					    "C DD \xFF"
					    "MM");
	test_lwpoint_to_latlon_assert_error("POINT(1.23456 7.89012)",
					    "C DD \xB0"
					    "MM");
	test_lwpoint_to_latlon_assert_error(
	    "POINT(1.23456 7.89012)",
	    "DD.DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
	test_lwpoint_to_latlon_assert_error(
	    "POINT(1.23456 7.89012)",
	    "DD.DDD jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj");
}

char result[OUT_DOUBLE_BUFFER_SIZE] = {0};

#define assert_lwprint_equal(d, precision, expected) \
	lwprint_double((d), (precision), result); \
	ASSERT_STRING_EQUAL(result, (expected));

static void
test_lwprint(void)
{
	static const int precision_start = -1;               /* Check for negatives */
	static const int precision_end = OUT_MAX_DIGITS + 2; /* Ask for more digits than available in all cases */

	/* Negative zero should be printed as 0 */
	for (int i = precision_start; i < precision_end; i++)
		assert_lwprint_equal(-0, i, "0");

	/*  2 = 0x4000000000000000
	 * -2 = 0xC000000000000000
	 * Both with exact representation, so we should never see extra digits
	 */
	for (int i = precision_start; i < precision_end; i++)
	{
		assert_lwprint_equal(2.0, i, "2");
		assert_lwprint_equal(-2.0, i, "-2");
	}

	/* 0.3 doesn't have an exact representation but it's the shortest representation for 0x3FD3333333333333
	 * 2.99999999999999988897769753748E-1 = 0x3FD3333333333333
	 */
	assert_lwprint_equal(0.3, 0, "0");
	for (int i = 1; i < precision_end; i++)
	{
		assert_lwprint_equal(0.3, i, "0.3");
		assert_lwprint_equal(2.99999999999999988897769753748E-1, i, "0.3");
	}

	/* 2.5 has an exact representation (0x4004000000000000) */
	assert_lwprint_equal(2.5, 0, "2");
	for (int i = 1; i < precision_end; i++)
	{
		assert_lwprint_equal(2.5, i, "2.5");
	}

	/* Test trailing zeros and rounding
	 * 0.0000000298023223876953125 == 0x3E60000000000000 == 2.98023223876953125E-8
	 */
	assert_lwprint_equal(0.0000000298023223876953125, -1, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 0, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 1, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 2, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 3, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 4, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 5, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 6, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 7, "0");
	assert_lwprint_equal(0.0000000298023223876953125, 8, "0.00000003");
	assert_lwprint_equal(0.0000000298023223876953125, 9, "0.00000003");
	assert_lwprint_equal(0.0000000298023223876953125, 10, "0.0000000298");
	assert_lwprint_equal(0.0000000298023223876953125, 11, "0.0000000298");
	assert_lwprint_equal(0.0000000298023223876953125, 12, "0.000000029802");
	assert_lwprint_equal(0.0000000298023223876953125, 13, "0.0000000298023");
	assert_lwprint_equal(0.0000000298023223876953125, 14, "0.00000002980232");
	assert_lwprint_equal(0.0000000298023223876953125, 15, "0.000000029802322");
	assert_lwprint_equal(0.0000000298023223876953125, 16, "0.0000000298023224");
	assert_lwprint_equal(0.0000000298023223876953125, 17, "0.00000002980232239");
	assert_lwprint_equal(0.0000000298023223876953125, 18, "0.000000029802322388");
	assert_lwprint_equal(0.0000000298023223876953125, 19, "0.0000000298023223877");
	assert_lwprint_equal(0.0000000298023223876953125, 20, "0.0000000298023223877");
	assert_lwprint_equal(0.0000000298023223876953125, 21, "0.000000029802322387695");
	assert_lwprint_equal(0.0000000298023223876953125, 22, "0.0000000298023223876953");
	assert_lwprint_equal(0.0000000298023223876953125, 23, "0.00000002980232238769531");
	assert_lwprint_equal(0.0000000298023223876953125, 24, "0.000000029802322387695312");
	assert_lwprint_equal(0.0000000298023223876953125, 40, "0.000000029802322387695312");

	/* Negative 0 after rounding should be printed as 0 */
	assert_lwprint_equal(-0.0005, 0, "0");
	assert_lwprint_equal(-0.0005, 1, "0");
	assert_lwprint_equal(-0.0005, 2, "0");
	assert_lwprint_equal(-0.0005, 3, "0");
	assert_lwprint_equal(-0.0005, 4, "-0.0005");

	/* Rounding on the first decimal digit */
	assert_lwprint_equal(-2.5, 0, "-2");
	assert_lwprint_equal(-1.5, 0, "-2");
	assert_lwprint_equal(-0.99, 0, "-1");
	assert_lwprint_equal(-0.5, 0, "0");
	assert_lwprint_equal(-0.01, 0, "0");
	assert_lwprint_equal(0.5, 0, "0");
	assert_lwprint_equal(0.99, 0, "1");
	assert_lwprint_equal(1.5, 0, "2");
	assert_lwprint_equal(2.5, 0, "2");

	/* Check rounding */
	assert_lwprint_equal(0.035, 2, "0.04");
	assert_lwprint_equal(0.045, 2, "0.04");
	assert_lwprint_equal(0.04500000000000001, 2, "0.05");
	assert_lwprint_equal(0.077, 2, "0.08");
	assert_lwprint_equal(0.087, 2, "0.09");
	assert_lwprint_equal(1.05, 1, "1");
	assert_lwprint_equal(1.005, 2, "1");
	assert_lwprint_equal(2.05, 1, "2");
	assert_lwprint_equal(2.005, 2, "2");

	for (int i = 0; i < 15; i++)
		assert_lwprint_equal(-0.99999999999999988898, i, "-1");
	for (int i = 15; i < 20; i++)
		assert_lwprint_equal(-0.99999999999999988898, i, "-0.9999999999999999");

	for (int i = 0; i < 16; i++)
		assert_lwprint_equal(0.99999999999999977796, i, "1");
	for (int i = 16; i < 20; i++)
		assert_lwprint_equal(0.99999999999999977796, i, "0.9999999999999998");

	assert_lwprint_equal(0.0999999999999999916733273153113, 0, "0");
	assert_lwprint_equal(-0.0999999999999999916733273153113, 0, "0");
	for (int i = 1; i < 15; i++)
	{
		assert_lwprint_equal(0.0999999999999999916733273153113, i, "0.1");
		assert_lwprint_equal(-0.0999999999999999916733273153113, i, "-0.1");
	}

	assert_lwprint_equal(0.00999999999999999847, 0, "0");
	assert_lwprint_equal(-0.00999999999999999847, 0, "0");
	assert_lwprint_equal(0.00999999999999999847, 1, "0");
	assert_lwprint_equal(-0.00999999999999999847, 1, "0");
	for (int i = 2; i < 15; i++)
	{
		assert_lwprint_equal(0.00999999999999999847, i, "0.01");
		assert_lwprint_equal(-0.00999999999999999847, i, "-0.01");
	}

	/* Test regression changes (output that changed in the tests vs 3.0) */
		/* There is at most 17 significative digits */
	assert_lwprint_equal(-123456789012345.12345678, 20, "-123456789012345.12");
	assert_lwprint_equal(123456789012345.12345678, 20, "123456789012345.12");

	/* Precision is respected (as number of decimal digits) */
	assert_lwprint_equal(92115.51207431706, 12, "92115.51207431706");
	assert_lwprint_equal(463412.82600000006, 12, "463412.82600000006");
	assert_lwprint_equal(463462.2069374289, 12, "463462.2069374289");
	assert_lwprint_equal(-115.17281600000001, OUT_DEFAULT_DECIMAL_DIGITS, "-115.17281600000001");
	assert_lwprint_equal(-115.17281600000001, 12, "-115.172816");
	assert_lwprint_equal(36.11464599999999, OUT_DEFAULT_DECIMAL_DIGITS, "36.11464599999999");
	assert_lwprint_equal(36.11464599999999, 12, "36.114646");
	assert_lwprint_equal(400000, 0, "400000");
	assert_lwprint_equal(400000, 12, "400000");
	assert_lwprint_equal(400000, 20, "400000");
	assert_lwprint_equal(5.0833333333333330372738600999582558870316, 15, "5.083333333333333");
	assert_lwprint_equal(1.4142135623730951, 15, "1.414213562373095");
	assert_lwprint_equal(143.62025166838282, 15, "143.62025166838282");
	assert_lwprint_equal(-30.037497356076827, 15, "-30.037497356076827");
	assert_lwprint_equal(142.92857147299705, 15, "142.92857147299705");
	assert_lwprint_equal(-32.75101196874403, 15, "-32.75101196874403");

	/* Note about this:
	 * 149.57565307617187 == 0x4062B26BC0000000
	 * 149.57565307617188 == 0x4062B26BC0000000
	 *
	 * 0x4062B26BC0000000 == 149.575653076171875
	 * Both if we consider "round to nearest, ties to even" (which is what we use)
	 * or "round to nearest, ties away from zero":
	 * 75 => 80 => 8 for the last digit
	 *
	 * It acts the same way in PostgreSQL:
		# Select '149.57565307617187'::float8, '149.575653076171875'::float8, '149.57565307617188'::float8;
			   float8       |       float8       |       float8
		--------------------+--------------------+--------------------
		 149.57565307617188 | 149.57565307617188 | 149.57565307617188
		(1 row)
	 */
	assert_lwprint_equal(149.57565307617187, 15, "149.57565307617188");
	assert_lwprint_equal(-149.57565307617187, 15, "-149.57565307617188");

	/* Shortest representation is used */
	assert_lwprint_equal(7000109.9999999990686774253845214843750000000000, 8, "7000110");
	assert_lwprint_equal(7000109.9999999990686774253845214843750000000000, 12, "7000109.999999999");

	/* SnapToGrid by itself is not enough to limit output decimals */
	const double d = 526355.92112222222;
	const double gridsize = 0.00001;
	const double gridded = rint(d / gridsize) * gridsize; /* Formula from ptarray_grid_in_place */
	assert_lwprint_equal(gridded, 15, "526355.9211200001");
	assert_lwprint_equal(gridded, 5, "526355.92112");

	/* Test the change towards scientific notation */
	assert_lwprint_equal(nextafter(OUT_MAX_DOUBLE, 0), OUT_MAX_DIGITS, "999999999999999.9");
	assert_lwprint_equal(nextafter(-OUT_MAX_DOUBLE, 0), OUT_MAX_DIGITS, "-999999999999999.9");
	assert_lwprint_equal(OUT_MAX_DOUBLE, OUT_MAX_DIGITS, "1e+15");
	assert_lwprint_equal(-OUT_MAX_DOUBLE, OUT_MAX_DIGITS, "-1e+15");
	assert_lwprint_equal(nextafter(OUT_MAX_DOUBLE, INFINITY), OUT_MAX_DIGITS, "1.0000000000000001e+15");
	assert_lwprint_equal(nextafter(-OUT_MAX_DOUBLE, -INFINITY), OUT_MAX_DIGITS, "-1.0000000000000001e+15");

	assert_lwprint_equal(nextafter(OUT_MIN_DOUBLE, 0), OUT_MAX_DIGITS, "9.999999999999999e-9");
	assert_lwprint_equal(nextafter(-OUT_MIN_DOUBLE, 0), OUT_MAX_DIGITS, "-9.999999999999999e-9");
	assert_lwprint_equal(OUT_MIN_DOUBLE, OUT_MAX_DIGITS, "1e-8");
	assert_lwprint_equal(-OUT_MIN_DOUBLE, OUT_MAX_DIGITS, "-1e-8");
	assert_lwprint_equal(nextafter(OUT_MIN_DOUBLE, INFINITY), OUT_MAX_DIGITS, "0.000000010000000000000002");
	assert_lwprint_equal(nextafter(-OUT_MIN_DOUBLE, -INFINITY), OUT_MAX_DIGITS, "-0.000000010000000000000002");


	/* Big numbers that use scientific notation respect the precision parameter */
	assert_lwprint_equal(9e+300, 0, "9e+300");
	assert_lwprint_equal(9e+300, 15, "9e+300");
	assert_lwprint_equal(-9e+300, 0, "-9e+300");
	assert_lwprint_equal(-9e+300, 15, "-9e+300");
	assert_lwprint_equal(9.000000000000001e+300, 0, "9e+300");
	assert_lwprint_equal(9.000000000000001e+300, 15, "9.000000000000001e+300");
	assert_lwprint_equal(-9.000000000000001e+300, 0, "-9e+300");
	assert_lwprint_equal(-9.000000000000001e+300, 15, "-9.000000000000001e+300");

	assert_lwprint_equal(6917529027641081856.0, 17, "6.917529027641082e+18");
	assert_lwprint_equal(6917529027641081856.0, 16, "6.917529027641082e+18");
	assert_lwprint_equal(6917529027641081856.0, 15, "6.917529027641082e+18");
	assert_lwprint_equal(6917529027641081856.0, 14, "6.91752902764108e+18");
	assert_lwprint_equal(6917529027641081856.0, 13, "6.9175290276411e+18");
	assert_lwprint_equal(6917529027641081856.0, 12, "6.917529027641e+18");
	assert_lwprint_equal(6917529027641081856.0, 11, "6.91752902764e+18");
	assert_lwprint_equal(6917529027641081856.0, 10, "6.9175290276e+18");
	assert_lwprint_equal(6917529027641081856.0, 9, "6.917529028e+18");
	assert_lwprint_equal(6917529027641081856.0, 8, "6.91752903e+18");
	assert_lwprint_equal(6917529027641081856.0, 7, "6.917529e+18");
	assert_lwprint_equal(6917529027641081856.0, 6, "6.917529e+18");
	assert_lwprint_equal(6917529027641081856.0, 5, "6.91753e+18");
	assert_lwprint_equal(6917529027641081856.0, 4, "6.9175e+18");
	assert_lwprint_equal(6917529027641081856.0, 3, "6.918e+18");
	assert_lwprint_equal(6917529027641081856.0, 2, "6.92e+18");
	assert_lwprint_equal(6917529027641081856.0, 1, "6.9e+18");
	assert_lwprint_equal(6917529027641081856.0, 0, "7e+18");

	/* Test special values (+-inf, NaNs) */
	for (int i = precision_start; i < precision_end; i++)
	{
		assert_lwprint_equal(NAN, i, "NaN");
		assert_lwprint_equal(INFINITY, i, "Infinity");
		assert_lwprint_equal(-INFINITY, i, "-Infinity");
	}

	/* Extremes */
	assert_lwprint_equal(2.2250738585072014e-308, OUT_MAX_DIGITS, "2.2250738585072014e-308");
	assert_lwprint_equal(1.7976931348623157e+308, OUT_MAX_DIGITS, "1.7976931348623157e+308");  /* Max */
	assert_lwprint_equal(2.9802322387695312E-8, OUT_MAX_DIGITS, "0.000000029802322387695312"); /* Trailing zeros */
}

/* Macro to test rountrip of lwprint_double when using enough precision digits (OUT_MAX_DIGITS) */
#define assert_lwprint_roundtrip(d) \
	{ \
		char s[OUT_DOUBLE_BUFFER_SIZE] = {0}; \
		lwprint_double(d, OUT_MAX_DIGITS, s); \
		ASSERT_DOUBLE_EQUAL(atof(s), d); \
	}

static void
test_lwprint_roundtrip(void)
{
	/* Test roundtrip with the first value outside the range that's always printed as zero */
	assert_lwprint_roundtrip(nextafter(FP_TOLERANCE, 1));
	assert_lwprint_roundtrip(nextafter(-FP_TOLERANCE, -1));

	/* Test roundtrip in around the switch to scientific notation */
	assert_lwprint_roundtrip(nextafter(OUT_MAX_DOUBLE, 0));
	assert_lwprint_roundtrip(nextafter(-OUT_MAX_DOUBLE, 0));
	assert_lwprint_roundtrip(OUT_MAX_DOUBLE);
	assert_lwprint_roundtrip(OUT_MAX_DOUBLE);
	assert_lwprint_roundtrip(nextafter(OUT_MAX_DOUBLE, INFINITY));
	assert_lwprint_roundtrip(nextafter(-OUT_MAX_DOUBLE, -INFINITY));

	/* Test some numbers */
	assert_lwprint_roundtrip(0);
	assert_lwprint_roundtrip(0.0000000298023223876953125);
	assert_lwprint_roundtrip(0.3);
	assert_lwprint_roundtrip(0.5);
	assert_lwprint_roundtrip(7000109.9999999990686774253845214843750000000000);
	assert_lwprint_roundtrip(6917529027641081856.0);
	assert_lwprint_roundtrip(9e+300);
	assert_lwprint_roundtrip(-9e+300);
	assert_lwprint_roundtrip(9.000000000000001e+300);
	assert_lwprint_roundtrip(-9.000000000000001e+300);

	/* Even if we write the **same** number differently as the (compiler) input the roundtrip is guaranteed */
	assert_lwprint_roundtrip(149.57565307617187);
	assert_lwprint_roundtrip(149.57565307617188);
	assert_lwprint_roundtrip(-149.57565307617187);
	assert_lwprint_roundtrip(-149.57565307617188);

	/* Extremes */
	assert_lwprint_roundtrip(2.2250738585072014e-308); /* We normalize small numbers to 0 */
	assert_lwprint_roundtrip(1.7976931348623157e+308);
	assert_lwprint_roundtrip(2.9802322387695312E-8);

	/* Special cases */
	assert_lwprint_roundtrip(-0); /* -0 is considered equal to 0 */

	/* Disabled because Windows / MinGW doesn't like them (#4735)
	 * 	assert_lwprint_roundtrip(INFINITY);
	 *  assert_lwprint_roundtrip(-INFINITY);
	 */

	/* nan is never equal to nan
	 * assert_lwprint_roundtrip(NAN);
	 */
}

/*
** Callback used by the test harness to register the tests in this file.
*/
void print_suite_setup(void);
void print_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("printing", NULL, NULL);
	PG_ADD_TEST(suite, test_lwpoint_to_latlon_default_format);
	PG_ADD_TEST(suite, test_lwpoint_to_latlon_format_orders);
	PG_ADD_TEST(suite, test_lwpoint_to_latlon_optional_format);
	PG_ADD_TEST(suite, test_lwpoint_to_latlon_oddball_formats);
	PG_ADD_TEST(suite, test_lwpoint_to_latlon_bad_formats);
	PG_ADD_TEST(suite, test_lwprint);
	PG_ADD_TEST(suite, test_lwprint_roundtrip);
}

