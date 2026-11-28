/*
 * PostGIS Raster - Raster Types for PostGIS
 *
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 * Copyright (C) 2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "CUnit/Basic.h"
#include "cu_tester.h"

static void
test_rgb_to_hsv(void)
{
	double rgb[3] = {0, 0, 0};
	double hsv[3] = {0, 0, 0};

	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0, DBL_EPSILON);

	rgb[0] = 0;
	rgb[1] = 0;
	rgb[2] = 1;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 2 / 3., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 1, DBL_EPSILON);

	rgb[0] = 0;
	rgb[1] = 0.25;
	rgb[2] = 0.5;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 7 / 12., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0.5, DBL_EPSILON);

	rgb[0] = 0.5;
	rgb[1] = 1;
	rgb[2] = 0.5;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 1 / 3., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 1, DBL_EPSILON);

	rgb[0] = 0.2;
	rgb[1] = 0.4;
	rgb[2] = 0.4;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0.4, DBL_EPSILON);
}

static void
test_hsv_to_rgb(void)
{
	double hsv[3] = {0, 0, 0};
	double rgb[3] = {0, 0, 0};

	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0, DBL_EPSILON);

	hsv[0] = 2 / 3.;
	hsv[1] = 1;
	hsv[2] = 1;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 1, DBL_EPSILON);

	hsv[0] = 7 / 12.;
	hsv[1] = 1;
	hsv[2] = 0.5;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0.25, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.5, DBL_EPSILON);

	hsv[0] = 1 / 3.;
	hsv[1] = 0.5;
	hsv[2] = 1;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.5, DBL_EPSILON);

	hsv[0] = 0.5;
	hsv[1] = 0.5;
	hsv[2] = 0.4;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0.2, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0.4, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.4, DBL_EPSILON);
}

static void
test_util_gdal_open(void)
{
	extern char *gdal_enabled_drivers;
	char *saved_drivers = gdal_enabled_drivers;
	/* The VSICURL gate also covers local VSI paths. */
	const char *path = "/vsisubfile/0," POSTGIS_TOP_SRC_DIR "/raster/test/regress/loader/testraster.tif";
	GDALDatasetH ds;

	rt_util_gdal_register_all(1);

	/* all drivers disabled */
	gdal_enabled_drivers = GDAL_DISABLE_ALL;
	ds = rt_util_gdal_open("/tmp/foo", GA_ReadOnly, 0);
	CU_ASSERT(ds == NULL);

	gdal_enabled_drivers = "GTiff";
	cu_error_msg_reset();
	ds = rt_util_gdal_open(path, GA_ReadOnly, 0);
	CU_ASSERT_PTR_NULL(ds);
	CU_ASSERT_STRING_EQUAL(cu_error_msg, "rt_util_gdal_open: Cannot open VSICURL file. VSICURL disabled");
	if (ds)
		GDALClose(ds);

	gdal_enabled_drivers = "GTiff " GDAL_VSICURL;
	cu_error_msg_reset();
	ds = rt_util_gdal_open(path, GA_ReadOnly, 0);
	CU_ASSERT_PTR_NOT_NULL(ds);
	if (ds)
		GDALClose(ds);

	cu_error_msg_reset();
	gdal_enabled_drivers = saved_drivers;
}

/* register tests */
void misc_suite_setup(void);
void
misc_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("misc", NULL, NULL);
	PG_ADD_TEST(suite, test_rgb_to_hsv);
	PG_ADD_TEST(suite, test_hsv_to_rgb);
	PG_ADD_TEST(suite, test_util_gdal_open);
}
