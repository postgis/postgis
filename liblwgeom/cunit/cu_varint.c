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
#include "varint.h"
#include "cu_tester.h"

static void do_test_u32_varint(uint32_t nr,int expected_size, char* expected_res)
{
	uint8_t *buf, *original;	
	int size;
	char *hex;
	
	buf = original=lwalloc(8);
	size = varint_u32_encoded_size(nr);
	if ( size != expected_size ) {
	  printf("Expected: %d\nObtained: %d\n", expected_size, size);
	}
	CU_ASSERT_EQUAL(size,expected_size);
	varint_u32_encode_buf(nr, &buf);
	hex = hexbytes_from_bytes(original,size);
	if ( strcmp(hex,expected_res) ) {
	  printf("Expected: %s\nObtained: %s\n", expected_res, hex);
	}
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
lwfree(original);	
  lwfree(hex);
}

static void do_test_s32_varint(int32_t nr,int expected_size, char* expected_res)
{
	uint8_t *buf, *original;
	int size;
	char *hex;
	
	buf = original=lwalloc(8);
	size = varint_s32_encoded_size(nr);
	if ( size != expected_size ) {
	  printf("Expected: %d\nObtained: %d\n", expected_size, size);
	}
	CU_ASSERT_EQUAL(size,expected_size);
	varint_s32_encode_buf(nr, &buf);
	hex = hexbytes_from_bytes(original,size);
	if ( strcmp(hex,expected_res) ) {
	  printf("Expected: %s\nObtained: %s\n", expected_res, hex);
	}
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
lwfree(original);	
  lwfree(hex);
}

static void do_test_u64_varint(uint64_t nr,int expected_size, char* expected_res)
{
	uint8_t *buf, *original;
	int size;
	char *hex;
	
	buf = original=lwalloc(8);
	size = varint_u64_encoded_size(nr);
	if ( size != expected_size ) {
	  printf("Expected: %d\nObtained: %d\n", expected_size, size);
	}
	CU_ASSERT_EQUAL(size,expected_size);
	varint_u64_encode_buf(nr, &buf);
	hex = hexbytes_from_bytes(original,size);
	if ( strcmp(hex,expected_res) ) {
	  printf("Expected: %s\nObtained: %s\n", expected_res, hex);
	}
	CU_ASSERT_STRING_EQUAL(hex, expected_res);
lwfree(original);	
  lwfree(hex);
}

static void do_test_s64_varint(int64_t nr,int expected_size, char* expected_res)
{
	uint8_t *buf, *original;
	int size;
	char *hex;
	
	buf = original=lwalloc(8);
	size = varint_s64_encoded_size(nr);
	if ( size != expected_size ) {
	  printf("Expected: %d\nObtained: %d\n", expected_size, size);
	}
	CU_ASSERT_EQUAL(size,expected_size);
	varint_s64_encode_buf(nr, &buf);
	hex = hexbytes_from_bytes(original,size);
	if ( strcmp(hex,expected_res) ) {
	  printf("Expected: %s\nObtained: %s\n", expected_res, hex);
	}
	CU_ASSERT_STRING_EQUAL(hex, expected_res);	
lwfree(original);	
  lwfree(hex);
}

static void test_varint(void)
{

	do_test_u64_varint(1, 1, "01");
	do_test_u64_varint(300, 2, "AC02");
	do_test_u64_varint(150, 2, "9601");
	do_test_u64_varint(240, 2, "F001"); 
	do_test_u64_varint(0x4000, 3, "808001");
  /*
                0100:0000 0000:0000 - input (0x4000)
      1000:0000 1000:0000 0000:0001 - output (0x808001)
       000:0000  000:0000  000:0001 - chop
       000:0001  000:0000  000:0000 - swap
         0:0000 0100:0000 0000:0000 - concat = input
   */
	do_test_u64_varint(2147483647, 5, "FFFFFFFF07");
  /*
              0111:1111 1111:1111 1111:1111 1111:1111 - input (0x7FFFFFFF)
    1111:1111 1111:1111 1111:1111 1111:1111 0000:0111 - output(0xFFFFFFFF07)
     111:1111  111:1111  111:1111  111:1111  000:0111 - chop
     000:0111  111:1111  111:1111  111:1111  111:1111 - swap
              0111:1111 1111:1111 1111:1111 1111:1111 - concat = input
                      |         |         |         |
                   2^32      2^16       2^8       2^0
   */
	do_test_s64_varint(1, 1, "02");
	do_test_s64_varint(-1, 1, "01");
	do_test_s64_varint(-2, 1, "03");

	do_test_u32_varint(2147483647, 5, "FFFFFFFF07");
  /*
              0111:1111 1111:1111 1111:1111 1111:1111 - input (7fffffff)
    1111:1111 1111:1111 1111:1111 1111:1111 0000:0111 - output (ffffff07)
     111:1111  111:1111  111:1111  111:1111  000:0111 - chop
     000:0111  111:1111  111:1111  111:1111  111:1111 - swap
              0111:1111 1111:1111 1111:1111 1111:1111 - concat = input
                      |         |         |         |
                   2^32      2^16       2^8       2^0
   */
	do_test_s32_varint(2147483647, 5, "FEFFFFFF0F");
  /*
              0111:1111 1111:1111 1111:1111 1111:1111 - input (7fffffff)
    1111:1110 1111:1111 1111:1111 1111:1111 0000:1111 - output(feffffff0f)
    1111:1111 1111:1111 1111:1111 1111:1111 0000:0111 - zigzag (ffffff07)
     111:1111  111:1111  111:1111  111:1111  000:0111 - chop
     000:0111  111:1111  111:1111  111:1111  111:1111 - swap
              0111:1111 1111:1111 1111:1111 1111:1111 - concat = input
                      |         |         |         |
                   2^32      2^16       2^8       2^0
   */
	do_test_s32_varint(-2147483648, 5, "FFFFFFFF0F");

	do_test_s32_varint(1, 1, "02");
  /*
    0000:0001 - input (01)
    0000:0010 - A: input << 1
    0000:0000 - B: input >> 31
    0000:0010 - zigzag (A xor B) == output
   */

	do_test_s32_varint(-1, 1, "01");
  /*
    1111:1111 ... 1111:1111 - input (FFFFFFFF)
    1111:1111 ... 1111:1110 - A: input << 1
    1111:1111 ... 1111:1111 - B: input >> 31
    0000:0000 ... 0000:0001 - zigzag (A xor B) == output
   */


}


/*
** Used by the test harness to register the tests in this file.
*/
void varint_suite_setup(void);
void varint_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("varint", NULL, NULL);
	PG_ADD_TEST(suite, test_varint);
}
