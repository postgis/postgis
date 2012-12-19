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

static void test_raster_convex_hull() {
	rt_raster raster = NULL;
	LWGEOM *hull = NULL;
	LWPOLY *poly = NULL;
	POINTARRAY *ring = NULL;
	POINT4D pt;

	/* NULL raster */
	CU_ASSERT_EQUAL(rt_raster_get_convex_hull(NULL, &hull), ES_NONE);
	CU_ASSERT(hull == NULL);

	/* width = 0, height = 0 */
	raster = rt_raster_new(0, 0);
	CU_ASSERT(raster != NULL);

	CU_ASSERT_EQUAL(rt_raster_get_convex_hull(raster, &hull), ES_NONE);
	CU_ASSERT_EQUAL(hull->type, POINTTYPE);
	lwgeom_free(hull);
	cu_free_raster(raster);

	/* width = 0 */
	raster = rt_raster_new(0, 256);
	CU_ASSERT(raster != NULL);

	CU_ASSERT_EQUAL(rt_raster_get_convex_hull(raster, &hull), ES_NONE);
	CU_ASSERT_EQUAL(hull->type, LINETYPE);
	lwgeom_free(hull);
	cu_free_raster(raster);

	/* height = 0 */
	raster = rt_raster_new(256, 0);
	CU_ASSERT(raster != NULL);

	CU_ASSERT_EQUAL(rt_raster_get_convex_hull(raster, &hull), ES_NONE);
	CU_ASSERT_EQUAL(hull->type, LINETYPE);
	lwgeom_free(hull);
	cu_free_raster(raster);

	/* normal raster */
	raster = rt_raster_new(256, 256);
	CU_ASSERT(raster != NULL);

	rt_raster_set_offsets(raster, 0.5, 0.5);
	rt_raster_set_scale(raster, 1, 1);
	rt_raster_set_skews(raster, 4, 5);

	CU_ASSERT_EQUAL(rt_raster_get_convex_hull(raster, &hull), ES_NONE);
	poly = lwgeom_as_lwpoly(hull);
	CU_ASSERT_EQUAL(poly->srid, rt_raster_get_srid(raster));
	CU_ASSERT_EQUAL(poly->nrings, 1);

	ring = poly->rings[0];
	CU_ASSERT(ring != NULL);
	CU_ASSERT_EQUAL(ring->npoints, 5);

	getPoint4d_p(ring, 0, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.5, DBL_EPSILON);

	getPoint4d_p(ring, 1, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 256.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 1280.5, DBL_EPSILON);

	getPoint4d_p(ring, 2, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 1280.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 1536.5, DBL_EPSILON);

	getPoint4d_p(ring, 3, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 1024.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 256.5, DBL_EPSILON);

	getPoint4d_p(ring, 4, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.5, DBL_EPSILON);

	lwgeom_free(hull);
	cu_free_raster(raster);
}

static char *
lwgeom_to_text(const LWGEOM *lwgeom) {
	char *wkt;
	size_t wkt_size;

	wkt = lwgeom_to_wkt(lwgeom, WKT_ISO, DBL_DIG, &wkt_size);

	return wkt;
}

static void test_raster_surface() {
	rt_raster rast;
	rt_band band;
	const int maxX = 5;
	const int maxY = 5;
	int x, y;
	char *wkt = NULL;
	LWMPOLY *mpoly = NULL;
	int err;

	rast = rt_raster_new(maxX, maxY);
	CU_ASSERT(rast != NULL);

	rt_raster_set_offsets(rast, 0, 0);
	rt_raster_set_scale(rast, 1, -1);

	band = cu_add_band(rast, PT_32BUI, 1, 0);
	CU_ASSERT(band != NULL);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++) {
			rt_band_set_pixel(band, x, y, 1, NULL);
		}
	}

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((0 0,0 -5,5 -5,5 0,0 0)))");
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* 0,0 NODATA */
	rt_band_set_pixel(band, 0, 0, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0)))");
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 1,1 NODATA */
	rt_band_set_pixel(band, 1, 1, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -1,1 -1)))");
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 2,2 NODATA */
	rt_band_set_pixel(band, 2, 2, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
#if POSTGIS_GEOS_VERSION >= 33
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 -1,1 0,5 0,5 -5,4 -5,0 -5,0 -1,1 -1),(1 -1,1 -2,2 -2,2 -1,1 -1),(2 -2,2 -3,3 -3,3 -2,2 -2)))");
#else
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))");
#endif
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 3,3 NODATA */
	rt_band_set_pixel(band, 3, 3, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
#if POSTGIS_GEOS_VERSION >= 33
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 -1,1 0,5 0,5 -5,4 -5,0 -5,0 -1,1 -1),(1 -1,1 -2,2 -2,2 -1,1 -1),(2 -2,2 -3,3 -3,3 -2,2 -2),(3 -3,3 -4,4 -4,4 -3,3 -3)))");
#else
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -3,3 -3,3 -4,4 -4,4 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))");
#endif
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 4,4 NODATA */
	rt_band_set_pixel(band, 4, 4, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((4 -4,4 -5,0 -5,0 -1,1 -1,1 -2,2 -2,2 -3,3 -3,3 -4,4 -4)),((1 -1,1 0,5 0,5 -4,4 -4,4 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))");
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* a whole lot more NODATA */
	rt_band_set_pixel(band, 4, 0, 0, NULL);
	rt_band_set_pixel(band, 3, 1, 0, NULL);
	rt_band_set_pixel(band, 1, 3, 0, NULL);
	rt_band_set_pixel(band, 0, 4, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CU_ASSERT_EQUAL(err, ES_NONE);
	CU_ASSERT(mpoly != NULL);
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CU_ASSERT_STRING_EQUAL(wkt, "MULTIPOLYGON(((1 -4,2 -4,2 -3,3 -3,3 -4,4 -4,4 -5,3 -5,1 -5,1 -4)),((1 -4,0 -4,0 -1,1 -1,1 -2,2 -2,2 -3,1 -3,1 -4)),((3 -2,4 -2,4 -1,5 -1,5 -4,4 -4,4 -3,3 -3,3 -2)),((3 -2,2 -2,2 -1,1 -1,1 0,4 0,4 -1,3 -1,3 -2)))");
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	cu_free_raster(rast);
}

static void test_raster_pixel_as_polygon() {
	rt_raster rast;
	rt_band band;
	uint32_t x, y;
	int rtn;
	const int maxX = 10;
	const int maxY = 10;
	LWPOLY *poly = NULL;

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

	poly = rt_raster_pixel_as_polygon(rast, 1, 1);
	CU_ASSERT(poly != NULL);
	lwpoly_free(poly);

	cu_free_raster(rast);
}

/* register tests */
CU_TestInfo raster_geometry_tests[] = {
	/* TODO: rt_raster_envelope()
	PG_TEST(test_raster_envelope),
	*/
	PG_TEST(test_raster_convex_hull),
	PG_TEST(test_raster_surface),
	PG_TEST(test_raster_pixel_as_polygon),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo raster_geometry_suite = {"raster_geometry",  NULL,  NULL, raster_geometry_tests};
