/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2017 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> /* for M_PI */
#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"

/* #define SKIP_TEST_RETAIN_ANGLE 1 */


static LWGEOM* lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if( LW_FAILURE == lwgeom_parse_wkt(&r, (char*)str, LW_PARSER_CHECK_NONE) )
		return NULL;
	return r.geom;
}

static char* lwgeom_to_text(const LWGEOM *geom, int prec)
{
	gridspec grid;
	LWGEOM *gridded;
	char *wkt;

	memset(&grid, 0, sizeof(gridspec));
	grid.xsize = prec;
	grid.ysize = prec;
	gridded = lwgeom_grid(geom, &grid);

	wkt = lwgeom_to_wkt(gridded, WKT_ISO, 15, NULL);
	lwgeom_free(gridded);
	return wkt;
}

static void test_lwcurve_linearize(void)
{
	LWGEOM *in;
	LWGEOM *out, *out2;
	char *str;
	int toltype;

	/***********************************************************
	 *
	 *  Segments per quadrant tolerance type
	 *
	 ***********************************************************/

	toltype = LW_LINEARIZE_TOLERANCE_TYPE_SEGS_PER_QUAD;

	/* 2 quadrants arc (180 degrees, PI radians) */
	in = lwgeom_from_text("CIRCULARSTRING(0 0,100 100,200 0)");
	/* 2 segment per quadrant */
	out = lwcurve_linearize(in, 2, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,30 70,100 100,170 70,200 0)");
	lwfree(str);
	lwgeom_free(out);

	/* 3 segment per quadrant */
	out = lwcurve_linearize(in, 3, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,14 50,50 86,100 100,150 86,186 50,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* 3.5 segment per quadrant (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, 3.5, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: segments per quadrant must be an integer value, got 3.5");
	lwgeom_free(out);
	/* -2 segment per quadrant (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, -2, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: segments per quadrant must be at least 1, got -2");
	lwgeom_free(out);
	/* 0 segment per quadrant (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, 0, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: segments per quadrant must be at least 1, got 0");
	lwgeom_free(out);
	lwgeom_free(in);

	/* 1.5 quadrants arc (145 degrees - PI*3/4 radians ) */
	in = lwgeom_from_text("CIRCULARSTRING(29.2893218813453 70.7106781186548,100 100,200 0)");
	/* 2 segment per quadrant */
	out = lwcurve_linearize(in, 2, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(30 70,100 100,170 70,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* 3 segment per quadrant - non-symmetric */
	out = lwcurve_linearize(in, 3, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(30 70,74 96,126 96,170 70,196 26,200 0)");
	lwfree(str);
	lwgeom_free(out);

	/* 3 segment per quadrant - symmetric */
	out = lwcurve_linearize(in, 3, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(30 70,70 96,116 98,158 80,190 46,200 0)");
	lwfree(str);
	lwgeom_free(out);

#ifndef SKIP_TEST_RETAIN_ANGLE
	/* 3 segment per quadrant - symmetric/retain_angle */
	out = lwcurve_linearize(in, 3, toltype,
		LW_LINEARIZE_FLAG_SYMMETRIC |
		LW_LINEARIZE_FLAG_RETAIN_ANGLE
	);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(30 70,40 80,86 100,138 92,180 60,200 14,200 0)");
	lwfree(str);
	lwgeom_free(out);
#endif /* SKIP_TEST_RETAIN_ANGLE */

	lwgeom_free(in);

	/* 10 segments per quadrant (circular) */
	in = lwgeom_from_text("CIRCULARSTRING(0 0,1 0,0 0)");
	// out = lwcurve_linearize(in, 10, toltype, 0);
	out = lwcurve_linearize(in, 10, toltype, 0);
	// printf("OUT: %s\n", lwgeom_to_wkt(out, WKT_EXTENDED, 5, NULL));
	ASSERT_INT_EQUAL(10*4, lwgeom_count_vertices(out));
	lwgeom_free(out);
	lwgeom_free(in);

	/***********************************************************
	 *
	 *  Maximum deviation tolerance type
	 *
	 ***********************************************************/

	toltype = LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION;

	in = lwgeom_from_text("CIRCULARSTRING(0 0,100 100,200 0)");

	/* Maximum of 10 units of difference, asymmetric */
	out = lwcurve_linearize(in, 10, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,38 78,124 98,190 42,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 0 units of difference (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, 0, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: max deviation must be bigger than 0, got 0");
	/* Maximum of -2 units of difference (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, -2, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: max deviation must be bigger than 0, got -2");
	/* Maximum of 10 units of difference, symmetric */
	out = lwcurve_linearize(in, 10, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,30 70,100 100,170 70,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 20 units of difference, asymmetric */
	out = lwcurve_linearize(in, 20, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,72 96,184 54,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 20 units of difference, symmetric */
	out = lwcurve_linearize(in, 20, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,50 86,150 86,200 0)");
	lwfree(str);
	lwgeom_free(out);

#ifndef SKIP_TEST_RETAIN_ANGLE
	/* Maximum of 20 units of difference, symmetric/retain angle */
	out = lwcurve_linearize(in, 20, toltype,
		LW_LINEARIZE_FLAG_SYMMETRIC |
		LW_LINEARIZE_FLAG_RETAIN_ANGLE
	);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,4 28,100 100,196 28,200 0)");
	lwfree(str);
	lwgeom_free(out);
#endif /* SKIP_TEST_RETAIN_ANGLE */

	lwgeom_free(in);

	/*
	 * Clockwise ~90 degrees south-west to north-west quadrants
	 * starting at ~22 degrees west of vertical line
	 *
	 * See https://trac.osgeo.org/postgis/ticket/3772
	 */
	toltype = LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION;
	in = lwgeom_from_text("CIRCULARSTRING(71.96 -65.64,22.2 -18.52,20 50)");

	/* 4 units of max deviation - symmetric */
	out = lwcurve_linearize(in, 4, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(72 -66,34 -38,16 4,20 50)");
	lwfree(str);
	lwgeom_free(out);
	lwgeom_free(in);

	/*
	 * Clockwise ~90 degrees north-west to south-west quadrants
	 * starting at ~22 degrees west of vertical line
	 *
	 * See https://trac.osgeo.org/postgis/ticket/3772
	 */
	toltype = LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION;
	in = lwgeom_from_text("CIRCULARSTRING(20 50,22.2 -18.52,71.96 -65.64)");

	/* 4 units of max deviation - symmetric */
	out = lwcurve_linearize(in, 4, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(20 50,16 4,34 -38,72 -66)");
	lwfree(str);
	lwgeom_free(out);

	/* max deviation bigger than twice the radius
	 * we really only want to make sure NOT to enter
	 * an infinite loop here.
	 * See https://trac.osgeo.org/postgis/ticket/4031
	 */
	out = lwcurve_linearize(in, 500, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(20 50,22 -18,72 -66)");
	lwfree(str);
	lwgeom_free(out);

	lwgeom_free(in);

	/*
	 * ROBUSTNESS: big radius, small tolerance
	 * See https://trac.osgeo.org/postgis/ticket/4058
	 * NOTE: we are really only interested in not entering
	 *       an infinite loop here
	 */
	toltype = LW_LINEARIZE_TOLERANCE_TYPE_MAX_DEVIATION;
	in = lwgeom_from_text("CIRCULARSTRING("
			"2696000.553 1125699.831999999936670, "
			"2695950.552000000141561 1125749.833000000100583, "
			"2695865.195999999996275 1125835.189000)");
	out = lwcurve_linearize(in, 0.0001, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(2696000 1125700,2695932 1125768,2695866 1125836)");
	lwfree(str);
	lwgeom_free(out);
	out = lwcurve_linearize(in, 0.0001, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(2696000 1125700,2695932 1125768,2695866 1125836)");
	lwfree(str);
	lwgeom_free(out);
#ifndef SKIP_TEST_RETAIN_ANGLE
	out = lwcurve_linearize(in, 0.0001, toltype,
		LW_LINEARIZE_FLAG_SYMMETRIC |
		LW_LINEARIZE_FLAG_RETAIN_ANGLE
	);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(2696000 1125700,2695932 1125768,2695866 1125836)");
	lwfree(str);
	lwgeom_free(out);
#endif /* SKIP_TEST_RETAIN_ANGLE */
	lwgeom_free(in);

	/***********************************************************
	 *
	 *  Maximum angle tolerance type
	 *
	 ***********************************************************/

	toltype = LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE;

	in = lwgeom_from_text("CIRCULARSTRING(0 0,100 100,200 0)");

	/* Maximum of 45 degrees per segment, asymmetric */
	out = lwcurve_linearize(in, M_PI / 4.0, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,30 70,100 100,170 70,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 0 degrees per segment (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, 0, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: max angle must be bigger than 0, got 0");
	/* Maximum of -2 degrees per segment (invalid) */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, -2, toltype, 0);
	CU_ASSERT( out == NULL );
	ASSERT_STRING_EQUAL(cu_error_msg, "lwarc_linearize: max angle must be bigger than 0, got -2");
	/* Maximum of 360 degrees per segment, just return minimum of two segments... */
	cu_error_msg_reset();
	out = lwcurve_linearize(in, M_PI*4, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,100 100,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 70 degrees per segment, asymmetric */
	out = lwcurve_linearize(in, 70 * M_PI / 180, toltype, 0);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,66 94,176 64,200 0)");
	lwfree(str);
	lwgeom_free(out);
	/* Maximum of 70 degrees per segment, symmetric */
	out = lwcurve_linearize(in, 70 * M_PI / 180, toltype, LW_LINEARIZE_FLAG_SYMMETRIC);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,50 86,150 86,200 0)");
	lwfree(str);
	lwgeom_free(out);

#ifndef SKIP_TEST_RETAIN_ANGLE
	/* Maximum of 70 degrees, symmetric/retain angle */
	out = lwcurve_linearize(in, 70 * M_PI / 180, toltype,
		LW_LINEARIZE_FLAG_SYMMETRIC |
		LW_LINEARIZE_FLAG_RETAIN_ANGLE
	);
	str = lwgeom_to_text(out, 2);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,6 34,100 100,194 34,200 0)");
	lwfree(str);
	lwgeom_free(out);
#endif /* SKIP_TEST_RETAIN_ANGLE */

	lwgeom_free(in);

	/***********************************************************
	 *
	 *  Direction neutrality
	 *
	 ***********************************************************/

	in = lwgeom_from_text("CIRCULARSTRING(71.96 -65.64,22.2 -18.52,20 50)");
	out = lwcurve_linearize(in, M_PI/4.0,
			 LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE,
			 LW_LINEARIZE_FLAG_SYMMETRIC);
	lwgeom_reverse_in_place(in);
	out2 = lwcurve_linearize(in, M_PI/4.0,
				 LW_LINEARIZE_TOLERANCE_TYPE_MAX_ANGLE,
				 LW_LINEARIZE_FLAG_SYMMETRIC);
	lwgeom_reverse_in_place(out2);
	if ( ! lwgeom_same(out, out2) )
	{
		fprintf(stderr, "linearization is not direction neutral:\n");
		str = lwgeom_to_wkt(out, WKT_ISO, 18, NULL);
		fprintf(stderr, "OUT1: %s\n", str);
		lwfree(str);
		str = lwgeom_to_wkt(out2, WKT_ISO, 18, NULL);
		fprintf(stderr, "OUT2: %s\n", str);
		lwfree(str);
	}
	CU_ASSERT( lwgeom_same(out, out2) );
	lwgeom_free(out2);
	lwgeom_free(out);
	lwgeom_free(in);
}

static void test_unstroke()
{
	LWGEOM *in, *out;
	char *str;

	/* It would be nice if this example returned two arcs (it's the intersection of two circles)
	   but it looks like the intersection itself is too sloppy in generating the derived point
	   to accurately reconstruct the circles.
	in = lwgeom_from_text("POLYGON((0.5 0,0.471177920604846 -0.292635483024192,0.38581929876693 -0.574025148547634,0.247204418453818 -0.833355349529403,0.0606601717798223 -1.06066017177982,-5.44089437167602e-17 -1.11044268820754,-0.0606601717798188 -1.06066017177982,-0.247204418453816 -0.833355349529406,-0.385819298766929 -0.574025148547639,-0.471177920604845 -0.292635483024197,-0.5 -4.84663372329776e-15,-0.471177920604847 0.292635483024187,-0.385819298766932 0.57402514854763,-0.247204418453821 0.833355349529398,-0.0606601717798256 1.06066017177982,3.45538806345173e-16 1.11044268820754,0.0606601717798183 1.06066017177982,0.247204418453816 0.833355349529407,0.385819298766929 0.574025148547638,0.471177920604845 0.292635483024196,0.5 0))");
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	printf("%s\n", str);
	ASSERT_STRING_EQUAL(str, "CIRCULARSTRING(-1 0,0 1,0 -1)");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);
	*/

	in = lwgeom_from_text("CIRCULARSTRING(-1 0,0 1,0 -1)");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(str, "CIRCULARSTRING(-1 0,0.70710678 0.70710678,0 -1)");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("COMPOUNDCURVE(CIRCULARSTRING(-1 0,0 1,0 -1),(0 -1,-1 -1))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(str, "COMPOUNDCURVE(CIRCULARSTRING(-1 0,0.70710678 0.70710678,0 -1),(0 -1,-1 -1))");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("COMPOUNDCURVE((-3 -3,-1 0),CIRCULARSTRING(-1 0,0 1,0 -1),(0 -1,0 -1.5,0 -2),CIRCULARSTRING(0 -2,-1 -3,1 -3),(1 -3,5 5))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(
	    str,
	    "COMPOUNDCURVE((-3 -3,-1 0),CIRCULARSTRING(-1 0,0.70710678 "
	    "0.70710678,0 -1),(0 -1,0 -1.5,0 -2),CIRCULARSTRING(0 "
	    "-2,-0.70710678 -3.70710678,1 -3),(1 -3,5 5))");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("COMPOUNDCURVE(CIRCULARSTRING(-1 0,0 1,0 -1),CIRCULARSTRING(0 -1,-1 -2,1 -2))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(
	    str,
	    "COMPOUNDCURVE(CIRCULARSTRING(-1 0,0.70710678 0.70710678,0 "
	    "-1),CIRCULARSTRING(0 -1,-0.70710678 -2.70710678,1 -2))");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("COMPOUNDCURVE((0 0, 1 1), CIRCULARSTRING(1 1, 2 2, 3 1), (3 1, 4 4))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	ASSERT_STRING_EQUAL(str, "COMPOUNDCURVE((0 0,1 1),CIRCULARSTRING(1 1,2 2,3 1),(3 1,4 4))");
	lwgeom_free(in);
	lwgeom_free(out);
	// printf("%s\n", str);
	lwfree(str);

	// See http://trac.osgeo.org/postgis/ticket/2425
	// and http://trac.osgeo.org/postgis/ticket/2420
	in = lwgeom_from_text("LINESTRING(0 0,10 0,10 10,0 10,0 0)");
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,10 0,10 10,0 10,0 0)");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("LINESTRING(10 10,0 10,0 0,10 0)");
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	ASSERT_STRING_EQUAL(str, "LINESTRING(10 10,0 10,0 0,10 0)");
	// printf("%s\n", str);
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("LINESTRING(0 0,10 0,10 10,0 10)");
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,10 0,10 10,0 10)");
	// printf("%s\n", str);
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	// See http://trac.osgeo.org/postgis/ticket/2412
	in = lwgeom_from_text("LINESTRING(0 0, 1 1)");
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(str, "LINESTRING(0 0,1 1)");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("GEOMETRYCOLLECTION(LINESTRING(10 10,10 11),LINESTRING(10 11,11 11),LINESTRING(11 11,10 10))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	ASSERT_STRING_EQUAL(str, "GEOMETRYCOLLECTION(LINESTRING(10 10,10 11),LINESTRING(10 11,11 11),LINESTRING(11 11,10 10))");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);

	in = lwgeom_from_text("GEOMETRYCOLLECTION(LINESTRING(4 4,4 8),CIRCULARSTRING(4 8,6 10,8 8),LINESTRING(8 8,8 4))");
	out = lwgeom_stroke(in,8);
	lwgeom_free(in);
	in = out;
	out = lwgeom_unstroke(in);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	// printf("%s\n", str);
	ASSERT_STRING_EQUAL(str, "GEOMETRYCOLLECTION(LINESTRING(4 4,4 8),CIRCULARSTRING(4 8,6 10,8 8),LINESTRING(8 8,8 4))");
	lwgeom_free(in);
	lwgeom_free(out);
	lwfree(str);
}

/*
** Used by the test harness to register the tests in this file.
*/
void lwstroke_suite_setup(void);
void lwstroke_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("lwstroke", NULL, NULL);
	PG_ADD_TEST(suite, test_lwcurve_linearize);
	PG_ADD_TEST(suite, test_unstroke);
}
