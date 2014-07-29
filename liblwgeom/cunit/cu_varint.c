/**********************************************************************
 * $Id: cu_varint.c 6160 2013-09-21 01:28:12Z nicklas $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"

#include "liblwgeom_internal.h"
#include "liblwgeom.h"

#include "varint.h"
#include "cu_tester.h"

static void do_test_s32_varint(int32_t nr,int expected_size, char* expected_res)
{
	uint8_t buf[8];	
	int size;
	char *hex;
	
	size = varint_s32_encoded_size(nr);
	CU_ASSERT_EQUAL(size,expected_size);
	varint_s32_encode_buf(nr, buf);
	hex = hexbytes_from_bytes(buf,size);
  if ( strcmp(hex,expected_res) ) {
    printf("Expected: %s\nObtained: %s\n", expected_res, hex);
  }
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
  lwfree(hex);
}

static void do_test_u64_varint(uint64_t nr,int expected_size, char* expected_res)
{
	uint8_t buf[8];	
	int size;
	char *hex;
	
	size = varint_u64_encoded_size(nr);
	CU_ASSERT_EQUAL(size,expected_size);
	varint_u64_encode_buf(nr, buf);
	hex = hexbytes_from_bytes(buf,size);
  if ( strcmp(hex,expected_res) ) {
    printf("Expected: %s\nObtained: %s\n", expected_res, hex);
  }
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
  lwfree(hex);
}

static void do_test_s64_varint(int64_t nr,int expected_size, char* expected_res)
{
	uint8_t buf[8];	
	int size;
	char *hex;
	
	size = varint_s64_encoded_size(nr);
	CU_ASSERT_EQUAL(size,expected_size);
	varint_s64_encode_buf(nr, buf);
	hex = hexbytes_from_bytes(buf,size);
  if ( strcmp(hex,expected_res) ) {
    printf("Expected: %s\nObtained: %s\n", expected_res, hex);
  }
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
  lwfree(hex);
}

static void test_varint(void)
{

	do_test_u64_varint(1, 1, "01");
	do_test_u64_varint(300, 2, "AC02");
	do_test_u64_varint(150, 2, "9601");
	
	do_test_s64_varint(1, 1, "02");
	do_test_s64_varint(-1, 1, "01");
	do_test_s64_varint(-2, 1, "03");
#if 0 /* FIXME! */
	do_test_s64_varint(2147483647, 4, "FFFFFFFE");
	do_test_s64_varint(-2147483648, 4, "FFFFFFFF");
#endif

	/* TODO: test signed/unsigned 32bit varints */
}


/*
** Used by the test harness to register the tests in this file.
*/
CU_TestInfo varint_tests[] =
{
	PG_TEST(test_varint),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo varint_suite = {"varint", NULL, NULL, varint_tests };
