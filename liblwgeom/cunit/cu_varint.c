/**********************************************************************
 * $Id: cu_varint.c 6160 2013-09-21 01:28:12Z nicklas $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
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

#include "lwout_twkb.c"
#include "cu_tester.h"


static void do_test_unsigned_varint(uint64_t nr,int expected_size, char* expected_res)
{
	uint8_t buf[8];	
	int size;
	
	size = u_getvarint_size(nr);
	CU_ASSERT_EQUAL(size,expected_size);
	u_varint_to_twkb_buf(nr, buf);
	CU_ASSERT_STRING_EQUAL( hexbytes_from_bytes(buf,size),expected_res);	
}

static void do_test_signed_varint(uint64_t nr,int expected_size, char* expected_res)
{
	uint8_t buf[8];	
	int size;
	
	size = u_getvarint_size(nr);
	CU_ASSERT_EQUAL(size,expected_size);
	s_varint_to_twkb_buf(nr, buf);
	CU_ASSERT_STRING_EQUAL( hexbytes_from_bytes(buf,size),expected_res);	
}


static void test_varint(void)
{

	do_test_unsigned_varint(1,1, "01");
	do_test_unsigned_varint(300,2, "AC02");
	do_test_unsigned_varint(150,2, "9601");
	
	do_test_signed_varint(1,1, "02");
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
