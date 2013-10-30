/*
 * PostGIS Raster - Raster Types for PostGIS
 * http://www.postgis.org/support/wiki/index.php?WKTRasterHomePage
 *
 * Copyright (C) 2012 Regents of the University of California
 *   <bkpark@ucdavis.edu>
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

#include "CUnit/Basic.h"
#include "cu_tester.h"

static void test_band_get_nearest_pixel() {
	rt_raster rast;
	rt_band band;
	uint32_t x, y;
	int rtn;
	const int maxX = 10;
	const int maxY = 10;
	rt_pixel npixels = NULL;

	double **value;
	int **nodata;
	int dimx;
	int dimy;

	rast = rt_raster_new(maxX, maxY);
	CU_ASSERT(rast != NULL);

	band = cu_add_band(rast, PT_32BUI, 1, 0);
	CU_ASSERT(band != NULL);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_set_pixel(band, x, y, 1, NULL);
		}
	}

	rt_band_set_pixel(band, 0, 0, 0, NULL);
	rt_band_set_pixel(band, 3, 0, 0, NULL);
	rt_band_set_pixel(band, 6, 0, 0, NULL);
	rt_band_set_pixel(band, 9, 0, 0, NULL);
	rt_band_set_pixel(band, 1, 2, 0, NULL);
	rt_band_set_pixel(band, 4, 2, 0, NULL);
	rt_band_set_pixel(band, 7, 2, 0, NULL);
	rt_band_set_pixel(band, 2, 4, 0, NULL);
	rt_band_set_pixel(band, 5, 4, 0, NULL);
	rt_band_set_pixel(band, 8, 4, 0, NULL);
	rt_band_set_pixel(band, 0, 6, 0, NULL);
	rt_band_set_pixel(band, 3, 6, 0, NULL);
	rt_band_set_pixel(band, 6, 6, 0, NULL);
	rt_band_set_pixel(band, 9, 6, 0, NULL);
	rt_band_set_pixel(band, 1, 8, 0, NULL);
	rt_band_set_pixel(band, 4, 8, 0, NULL);
	rt_band_set_pixel(band, 7, 8, 0, NULL);

	/* 0,0 */
	rtn = rt_band_get_nearest_pixel(
		band,
		0, 0,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 3);
	if (rtn)
		rtdealloc(npixels);

	/* 1,1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		1, 1,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 6);
	if (rtn)
		rtdealloc(npixels);

	/* 4,4 */
	rtn = rt_band_get_nearest_pixel(
		band,
		4, 4,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 7);
	if (rtn)
		rtdealloc(npixels);

	/* 4,4 distance 2 */
	rtn = rt_band_get_nearest_pixel(
		band,
		4, 4,
		2, 2,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 19);
	if (rtn)
		rtdealloc(npixels);

	/* 10,10 */
	rtn = rt_band_get_nearest_pixel(
		band,
		10, 10,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 1);
	if (rtn)
		rtdealloc(npixels);

	/* 11,11 distance 1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		11, 11,
		1, 1,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 0);
	if (rtn)
		rtdealloc(npixels);

	/* -1,-1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		-1, -1,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 3);
	if (rtn)
		rtdealloc(npixels);

	/* -1,-1 distance 1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		-1, -1,
		1, 1,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 0);
	if (rtn)
		rtdealloc(npixels);

	/* -1,1 distance 1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		-1, 1,
		1, 1,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 2);

	rtn = rt_pixel_set_to_array(
		npixels, rtn,
		-1, 1, 
		1, 1,
		&value,
		&nodata,
		&dimx, &dimy
	);
	rtdealloc(npixels);
	CU_ASSERT_EQUAL(rtn, ES_NONE);
	CU_ASSERT_EQUAL(dimx, 3);
	CU_ASSERT_EQUAL(dimy, 3);

	for (x = 0; x < dimx; x++) {
		rtdealloc(nodata[x]);
		rtdealloc(value[x]);
	}

	rtdealloc(nodata);
	rtdealloc(value);


	/* -2,2 distance 1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		-2, 2,
		1, 1,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 0);
	if (rtn)
		rtdealloc(npixels);

	/* -10,2 distance 3 */
	rtn = rt_band_get_nearest_pixel(
		band,
		-10, 2,
		3, 3,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 0);
	if (rtn)
		rtdealloc(npixels);

	/* -10,2 distance 3 include NODATA */
	rtn = rt_band_get_nearest_pixel(
		band,
		-10, 2,
		3, 3,
		0,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 48);
	if (rtn)
		rtdealloc(npixels);

	/* 4,4 distance 3,2 */
	rtn = rt_band_get_nearest_pixel(
		band,
		4, 4,
		3, 2,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 27);
	if (rtn)
		rtdealloc(npixels);

	/* 2,7 distance 3,1 */
	rtn = rt_band_get_nearest_pixel(
		band,
		2, 7,
		3, 1,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 13);
	if (rtn)
		rtdealloc(npixels);

	/* 10,10 distance 1,3 */
	rtn = rt_band_get_nearest_pixel(
		band,
		10,10,
		1, 3,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 3);
	if (rtn)
		rtdealloc(npixels);

	/* band with no NODATA */
	band = cu_add_band(rast, PT_32BUI, 0, 0);
	CU_ASSERT(band != NULL);

	/* 0,0 */
	rtn = rt_band_get_nearest_pixel(
		band,
		0, 0,
		0, 0,
		1,
		&npixels
	);
	CU_ASSERT_EQUAL(rtn, 8);
	if (rtn)
		rtdealloc(npixels);

	cu_free_raster(rast);
}

static void test_band_get_pixel_of_value() {
	rt_raster rast;
	rt_band band;
	uint32_t x, y;
	int rtn;
	const int maxX = 10;
	const int maxY = 10;
	rt_pixel pixels = NULL;

	double search0[1] = {0};
	double search1[1] = {1};
	double search2[2] = {3, 5};

	rast = rt_raster_new(maxX, maxY);
	CU_ASSERT(rast != NULL);

	band = cu_add_band(rast, PT_32BUI, 1, 0);
	CU_ASSERT(band != NULL);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_set_pixel(band, x, y, 1, NULL);
		}
	}

	rt_band_set_pixel(band, 0, 0, 0, NULL);
	rt_band_set_pixel(band, 3, 0, 0, NULL);
	rt_band_set_pixel(band, 6, 0, 0, NULL);
	rt_band_set_pixel(band, 9, 0, 0, NULL);
	rt_band_set_pixel(band, 1, 2, 0, NULL);
	rt_band_set_pixel(band, 4, 2, 0, NULL);
	rt_band_set_pixel(band, 7, 2, 0, NULL);
	rt_band_set_pixel(band, 2, 4, 0, NULL);
	rt_band_set_pixel(band, 5, 4, 0, NULL);
	rt_band_set_pixel(band, 8, 4, 0, NULL);
	rt_band_set_pixel(band, 0, 6, 0, NULL);
	rt_band_set_pixel(band, 3, 6, 0, NULL);
	rt_band_set_pixel(band, 6, 6, 0, NULL);
	rt_band_set_pixel(band, 9, 6, 0, NULL);
	rt_band_set_pixel(band, 1, 8, 0, NULL);
	rt_band_set_pixel(band, 4, 8, 0, NULL);
	rt_band_set_pixel(band, 7, 8, 0, NULL);

	pixels = NULL;
	rtn = rt_band_get_pixel_of_value(
		band, TRUE,
		search1, 1,
		&pixels
	);
	CU_ASSERT_EQUAL(rtn, 83);
	if (rtn)
		rtdealloc(pixels);

	pixels = NULL;
	rtn = rt_band_get_pixel_of_value(
		band, FALSE,
		search0, 1,
		&pixels
	);
	CU_ASSERT_EQUAL(rtn, 17);
	if (rtn)
		rtdealloc(pixels);

	rt_band_set_pixel(band, 4, 2, 3, NULL);
	rt_band_set_pixel(band, 7, 2, 5, NULL);
	rt_band_set_pixel(band, 1, 8, 3, NULL);

	pixels = NULL;
	rtn = rt_band_get_pixel_of_value(
		band, TRUE,
		search2, 2,
		&pixels
	);
	CU_ASSERT_EQUAL(rtn, 3);
	if (rtn)
		rtdealloc(pixels);

	cu_free_raster(rast);
}

/* register tests */
CU_TestInfo band_misc_tests[] = {
	PG_TEST(test_band_get_nearest_pixel),
	PG_TEST(test_band_get_pixel_of_value),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo band_misc_suite = {"band_misc",  NULL,  NULL, band_misc_tests};
