/**********************************************************************
 * $Id: cu_print.h 4786 2009-11-11 19:02:19Z pramsey $
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
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom.h"
#include "cu_tester.h"

/***********************************************************************
** for Print Suite
*/

/* Test functions */
void test_lwprint_default_format(void);
void test_lwprint_format_order(void);
void test_lwprint_format_optional(void);
void test_lwprint_bad_formats(void);
void test_lwprint_oddball_format(void);
