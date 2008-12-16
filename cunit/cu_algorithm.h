#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "lwalgorithm.h"

/***********************************************************************
** for Computational Geometry Suite
*/

/* Set-up / clean-up functions */
CU_pSuite register_cg_suite(void);
int init_cg_suite(void);
int clean_cg_suite(void);

/* Test functions */
void testSegmentSide(void);
void testSegmentIntersects(void);
void testLineCrossingShortLines(void);
void testLineCrossingLongLines(void);


