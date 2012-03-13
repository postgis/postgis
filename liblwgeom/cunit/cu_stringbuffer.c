/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2012 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "stringbuffer.h"
#include "cu_tester.h"


static void test_stringbuffer_append(void)
{
	stringbuffer_t *sb;
	const char *str;

	sb = stringbuffer_create_with_size(2);
	stringbuffer_append(sb, "hello world");
	str = stringbuffer_getstring(sb);

	CU_ASSERT_STRING_EQUAL("hello world", str);
}

static void test_stringbuffer_aprintf(void)
{
	stringbuffer_t *sb;
	const char *str;

	sb = stringbuffer_create_with_size(2);
	stringbuffer_aprintf(sb, "hello %dth world", 14);
	str = stringbuffer_getstring(sb);

	CU_ASSERT_STRING_EQUAL("hello 14th world", str);
}


/* TODO: add more... */

/*
** Used by the test harness to register the tests in this file.
*/
CU_TestInfo stringbuffer_tests[] =
{
	PG_TEST(test_stringbuffer_append),
	PG_TEST(test_stringbuffer_aprintf),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo stringbuffer_suite = {"stringbuffer", NULL, NULL, stringbuffer_tests };
