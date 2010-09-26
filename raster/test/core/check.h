/* 
 * $Id$
 *
 * WKTRaster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
 * Copyright (C) 2009  Sandro Santilli <strk@keybit.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#define FLT_DIG_PRINTF FLT_DIG + 6
#define DBL_DIG_PRINTF DBL_DIG + 6

#ifndef DOUBLE_EQ
#define DOUBLE_EQ(obt, exp, epsilon) \
    ((((obt) - (exp)) <= (epsilon)) && (((obt) - (exp)) >= -(epsilon)))
#endif

#ifndef FLOAT_EQ
#define FLOAT_EQ(obt, exp, epsilon) \
    ((((obt) - (exp)) <= (epsilon)) && (((obt) - (exp)) >= -(epsilon)))
#endif

#define CHECK(exp) { if ( !exp ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    exit(EXIT_FAILURE); } }

/* FIXME: Macro uses C99 integral type, might be not portable */
#define CHECK_EQUALS(obt, exp) { if ( (long long)obt != (long long)exp ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %lld, expected = %lld\n", (long long)obt, (long long)exp ); \
    exit(EXIT_FAILURE); } }

#define CHECK_EQUALS_FLOAT_EX(obt, exp, eps) { if ( !FLOAT_EQ(obt, exp, eps) ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %.*g, expected = %.*g\n", FLT_DIG_PRINTF, obt, FLT_DIG_PRINTF, exp ); \
    exit(EXIT_FAILURE); } }

#define CHECK_EQUALS_FLOAT(obt, exp) { if ( !FLOAT_EQ(obt, exp, FLT_EPSILON) ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %.*g, expected = %.*g\n", FLT_DIG_PRINTF, obt, FLT_DIG_PRINTF, exp ); \
    exit(EXIT_FAILURE); } }


#define CHECK_EQUALS_DOUBLE_EX(obt, exp, eps) { if ( !DOUBLE_EQ(obt, exp, eps) ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %.*g, expected = %.*g\n", DBL_DIG_PRINTF, obt, DBL_DIG_PRINTF, exp ); \
    exit(EXIT_FAILURE); } }

#define CHECK_EQUALS_DOUBLE(obt, exp) { if ( !DOUBLE_EQ(obt, exp, DBL_EPSILON) ) { \
    fprintf(stderr, "Check failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %.*g, expected = %.*g\n", DBL_DIG_PRINTF, obt, DBL_DIG_PRINTF, exp ); \
    exit(EXIT_FAILURE); } }
