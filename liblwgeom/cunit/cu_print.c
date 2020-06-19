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

#define test_lwprint_assert(d, precision, expected) \
	lwprint_double((d), (precision), result); \
	ASSERT_STRING_EQUAL(result, (expected));

static void
test_lwprint(void)
{
	static const int precision_start = -1;               /* Check for negatives */
	static const int precision_end = OUT_MAX_DIGITS + 2; /* Ask for more digits than available in all cases */

	/* Numbers close to zero should always be printed as zero */
	for (int i = precision_start; i < precision_end; i++)
		test_lwprint_assert(nextafter(0, 1), i, "0");

	/*  2 = 0x4000000000000000
	 * -2 = 0xC000000000000000
	 * Both with exact representation, so we should never see extra digits
	 */
	for (int i = precision_start; i < precision_end; i++)
	{
		test_lwprint_assert(2.0, i, "2");
		test_lwprint_assert(-2.0, i, "-2");
	}

	/* 0.3 doesn't have an exact representation but it's the shortest representation for 0x3FD3333333333333
	 * 2.99999999999999988897769753748E-1 = 0x3FD3333333333333
	 */
	test_lwprint_assert(0.3, 0, "0");
	for (int i = 1; i < precision_end; i++)
	{
		test_lwprint_assert(0.3, i, "0.3");
		test_lwprint_assert(2.99999999999999988897769753748E-1, i, "0.3");
	}

	/* 2.5 has an exact representation (0x4004000000000000) */
	test_lwprint_assert(2.5, 0, "2");
	for (int i = 1; i < precision_end; i++)
	{
		test_lwprint_assert(2.5, i, "2.5");
	}

	/* Test trailing zeros and rounding
	 * 0.0000000298023223876953125 == 0x3E60000000000000 == 2.98023223876953125E-8
	 */
	test_lwprint_assert(0.0000000298023223876953125, -1, "0");
	test_lwprint_assert(0.0000000298023223876953125, 0, "0");
	test_lwprint_assert(0.0000000298023223876953125, 1, "0");
	test_lwprint_assert(0.0000000298023223876953125, 2, "0");
	test_lwprint_assert(0.0000000298023223876953125, 3, "0");
	test_lwprint_assert(0.0000000298023223876953125, 4, "0");
	test_lwprint_assert(0.0000000298023223876953125, 5, "0");
	test_lwprint_assert(0.0000000298023223876953125, 6, "0");
	test_lwprint_assert(0.0000000298023223876953125, 7, "0");
	test_lwprint_assert(0.0000000298023223876953125, 8, "0.00000003");
	test_lwprint_assert(0.0000000298023223876953125, 9, "0.00000003");
	test_lwprint_assert(0.0000000298023223876953125, 10, "0.0000000298");
	test_lwprint_assert(0.0000000298023223876953125, 11, "0.0000000298");
	test_lwprint_assert(0.0000000298023223876953125, 12, "0.000000029802");
	test_lwprint_assert(0.0000000298023223876953125, 13, "0.0000000298023");
	test_lwprint_assert(0.0000000298023223876953125, 14, "0.00000002980232");
	test_lwprint_assert(0.0000000298023223876953125, 15, "0.000000029802322");
	test_lwprint_assert(0.0000000298023223876953125, 16, "0.0000000298023224");
	test_lwprint_assert(0.0000000298023223876953125, 17, "0.00000002980232239");
	test_lwprint_assert(0.0000000298023223876953125, 18, "0.000000029802322388");
	test_lwprint_assert(0.0000000298023223876953125, 19, "0.0000000298023223877");
	test_lwprint_assert(0.0000000298023223876953125, 20, "0.0000000298023223877");
	test_lwprint_assert(0.0000000298023223876953125, 21, "0.000000029802322387695");
	test_lwprint_assert(0.0000000298023223876953125, 22, "0.0000000298023223876953");
	test_lwprint_assert(0.0000000298023223876953125, 23, "0.00000002980232238769531");
	test_lwprint_assert(0.0000000298023223876953125, 24, "0.000000029802322387695312");
	test_lwprint_assert(0.0000000298023223876953125, 40, "0.000000029802322387695312");

	/* Negative 0 after rounding should be printed as 0 */
	test_lwprint_assert(-0.0005, 0, "0");
	test_lwprint_assert(-0.0005, 1, "0");
	test_lwprint_assert(-0.0005, 2, "0");
	test_lwprint_assert(-0.0005, 3, "0");
	test_lwprint_assert(-0.0005, 4, "-0.0005");

	/* Rounding on the first decimal digit */
	test_lwprint_assert(-2.5, 0, "-2");
	test_lwprint_assert(-1.5, 0, "-2");
	test_lwprint_assert(-0.99, 0, "-1");
	test_lwprint_assert(-0.5, 0, "0");
	test_lwprint_assert(-0.01, 0, "0");
	test_lwprint_assert(0.5, 0, "0");
	test_lwprint_assert(0.99, 0, "1");
	test_lwprint_assert(1.5, 0, "2");
	test_lwprint_assert(2.5, 0, "2");
	for (int i = 0; i < 15; i++)
		test_lwprint_assert(-0.99999999999999988898, i, "-1");
	for (int i = 15; i < 20; i++)
		test_lwprint_assert(-0.99999999999999988898, i, "-0.9999999999999999");

	for (int i = 0; i < 16; i++)
		test_lwprint_assert(0.99999999999999977796, i, "1");
	for (int i = 16; i < 20; i++)
		test_lwprint_assert(0.99999999999999977796, i, "0.9999999999999998");


	/* Test regression changes (output that changed in the tests vs 3.0) */
		/* There is at most 17 significative digits */
	test_lwprint_assert(-123456789012345.12345678, 20, "-123456789012345.12");
	test_lwprint_assert(123456789012345.12345678, 20, "123456789012345.12");

		/* Big numbers that use scientific notation print all significant digits */
	test_lwprint_assert(9e+300, 0, "9e+300");
	test_lwprint_assert(9e+300, 15, "9e+300");
	test_lwprint_assert(-9e+300, 0, "-9e+300");
	test_lwprint_assert(-9e+300, 15, "-9e+300");
	test_lwprint_assert(9.000000000000001e+300, 0, "9.000000000000001e+300");
	test_lwprint_assert(9.000000000000001e+300, 15, "9.000000000000001e+300");
	test_lwprint_assert(-9.000000000000001e+300, 0, "-9.000000000000001e+300");
	test_lwprint_assert(-9.000000000000001e+300, 15, "-9.000000000000001e+300");

		/* Precision is respected (as number of decimal digits) */
	test_lwprint_assert(92115.51207431706, 12, "92115.51207431706")
	test_lwprint_assert(463412.82600000006, 12, "463412.82600000006");
	test_lwprint_assert(463462.2069374289, 12, "463462.2069374289");
	test_lwprint_assert(-115.17281600000001, OUT_DEFAULT_DECIMAL_DIGITS, "-115.17281600000001");
	test_lwprint_assert(-115.17281600000001, 12, "-115.172816");
	test_lwprint_assert(36.11464599999999, OUT_DEFAULT_DECIMAL_DIGITS, "36.11464599999999");
	test_lwprint_assert(36.11464599999999, 12, "36.114646");
	test_lwprint_assert(400000, 0, "400000");
	test_lwprint_assert(400000, 12, "400000");
	test_lwprint_assert(400000, 20, "400000");
	test_lwprint_assert(5.0833333333333330372738600999582558870316, 15, "5.083333333333333");
	test_lwprint_assert(1.4142135623730951, 15, "1.414213562373095");
	test_lwprint_assert(143.62025166838282, 15, "143.62025166838282");
	test_lwprint_assert(-30.037497356076827, 15, "-30.037497356076827");
	test_lwprint_assert(142.92857147299705, 15, "142.92857147299705");
	test_lwprint_assert(-32.75101196874403, 15, "-32.75101196874403");

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
	test_lwprint_assert(149.57565307617187, 15, "149.57565307617188");

	/* Shortest representation is used */
	test_lwprint_assert(7000109.9999999990686774253845214843750000000000, 8, "7000110");
	test_lwprint_assert(7000109.9999999990686774253845214843750000000000, 12, "7000109.999999999");

	/* SnapToGrid by itself is not enough to limit output decimals */
	const double d = 526355.92112222222;
	const double gridsize = 0.00001;
	const double gridded = rint(d / gridsize) * gridsize; /* Formula from ptarray_grid_in_place */
	test_lwprint_assert(gridded, 15, "526355.9211200001");
	test_lwprint_assert(gridded, 5, "526355.92112");

	/* TODO: Test high numbers (close to the limit to scientific notation) */

	/* TODO: Test scientific notation with precision 0 or document its behaviour
	 * as anything bigger than OUT_MAX_DOUBLE will be printed in exponential notation with
	 * ALL (up to 17) significant digits
	 */

	/* Test special values (+-inf, NaNs) */
	for (int i = precision_start; i < precision_end; i++)
	{
		test_lwprint_assert(NAN, i, "NaN");
		test_lwprint_assert(INFINITY, i, "Infinity");
		test_lwprint_assert(-INFINITY, i, "-Infinity");
	}
}

static void
test_lwprint_roundtrip(void)
{
	/* Test roundtrip with the first value outside the range that's always printed as zero */
	double original_value = nextafter(FP_TOLERANCE, 1);
	char s[OUT_DOUBLE_BUFFER_SIZE] = {0};
	lwprint_double(original_value, 50, s);
	ASSERT_DOUBLE_EQUAL(atof(s), original_value);

	original_value = nextafter(-FP_TOLERANCE, -1);
	lwprint_double(original_value, 50, s);
	ASSERT_DOUBLE_EQUAL(atof(s), original_value);

	/* TODO: Add some more roundtrip tests */
	/* TODO: Change 50 for the appropiate macro (max precision required to guarantee roundtrip, taking into account
	 * the extra zeros) i.e: FP_TOLERANCE precision + 17
	 */

	/* TODO: Simplify with macros ? */
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

