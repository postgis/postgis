/*
 * PostGIS Raster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright (C) 2012 Regents of the University of California
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
test_raster_new(void)
{
	rt_raster raster = NULL;

	raster = rt_raster_new(0, 0);
	CU_ASSERT(raster != NULL);
	cu_free_raster(raster);

	raster = rt_raster_new(1, 1);
	CU_ASSERT(raster != NULL);
	cu_free_raster(raster);

	raster = rt_raster_new(10, 10);
	CU_ASSERT(raster != NULL);
	cu_free_raster(raster);
}

static void
test_raster_empty(void)
{
	rt_raster raster = NULL;

	/* check that raster is empty */
	raster = rt_raster_new(0, 0);
	CU_ASSERT(raster != NULL);
	CU_ASSERT(rt_raster_is_empty(raster));
	cu_free_raster(raster);

	/* create raster */
	raster = rt_raster_new(1, 1);
	CU_ASSERT(raster != NULL);

	/* check that raster is not empty */
	CU_ASSERT(!rt_raster_is_empty(raster));

	cu_free_raster(raster);
}

static void
test_raster_metadata(void)
{
	rt_raster raster = NULL;

	/* create raster */
	raster = rt_raster_new(5, 5);
	CU_ASSERT(raster != NULL);

	/* # of bands */
	CU_ASSERT_EQUAL(rt_raster_get_num_bands(raster), 0);

	/* has bands */
	CU_ASSERT(!rt_raster_has_band(raster, 1));

	/* upper-left corner */
	rt_raster_set_offsets(raster, 30, -70);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_x_offset(raster), 30, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_y_offset(raster), -70, DBL_EPSILON);

	/* scale */
	rt_raster_set_scale(raster, 10, -10);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_x_scale(raster), 10, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_y_scale(raster), -10, DBL_EPSILON);

	/* skew */
	rt_raster_set_skews(raster, 0.0001, -0.05);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_x_skew(raster), 0.0001, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rt_raster_get_y_skew(raster), -0.05, DBL_EPSILON);

	/* srid */
	rt_raster_set_srid(raster, 4326);
	CU_ASSERT_EQUAL(rt_raster_get_srid(raster), 4326);
	rt_raster_set_srid(raster, 4269);
	CU_ASSERT_EQUAL(rt_raster_get_srid(raster), 4269);

	cu_free_raster(raster);
}

static void
test_raster_clone(void)
{
	rt_raster rast1;
	rt_raster rast2;
	rt_band band;

	int maxX = 5;
	int maxY = 5;
	double gt[6];

	rast1 = rt_raster_new(maxX, maxY);
	CU_ASSERT(rast1 != NULL);

	rt_raster_set_offsets(rast1, 0, 0);
	rt_raster_set_scale(rast1, 1, -1);
	rt_raster_set_srid(rast1, 4326);

	band = cu_add_band(rast1, PT_32BUI, 1, 6);
	CU_ASSERT(band != NULL);

	/* clone without bands */
	rast2 = rt_raster_clone(rast1, 0);
	CU_ASSERT(rast2 != NULL);
	CU_ASSERT_EQUAL(rt_raster_get_num_bands(rast2), 0);

	rt_raster_get_geotransform_matrix(rast2, gt);
	CU_ASSERT_EQUAL(rt_raster_get_srid(rast2), 4326);
	CU_ASSERT_DOUBLE_EQUAL(gt[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(gt[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(gt[2], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(gt[3], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(gt[4], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(gt[5], -1, DBL_EPSILON);

	cu_free_raster(rast2);

	/* clone with bands */
	rast2 = rt_raster_clone(rast1, 1);
	CU_ASSERT(rast2 != NULL);
	CU_ASSERT_EQUAL(rt_raster_get_num_bands(rast2), 1);

	cu_free_raster(rast2);
	cu_free_raster(rast1);
}

static void
test_raster_from_band(void)
{
	uint32_t bandNums[] = {1, 3};
	int lenBandNums = 2;
	rt_raster raster;
	rt_raster rast;
	rt_band band;
	uint32_t xmax = 100;
	uint32_t ymax = 100;
	uint32_t x;

	raster = rt_raster_new(xmax, ymax);
	CU_ASSERT(raster != NULL);

	for (x = 0; x < 5; x++)
	{
		band = cu_add_band(raster, PT_32BUI, 0, 0);
		CU_ASSERT(band != NULL);
		rt_band_set_nodata(band, 0, NULL);
	}

	rast = rt_raster_from_band(raster, bandNums, lenBandNums);
	CU_ASSERT(rast != NULL);

	CU_ASSERT(!rt_raster_is_empty(rast));
	CU_ASSERT(rt_raster_has_band(rast, 1));

	cu_free_raster(rast);
	cu_free_raster(raster);
}

static void
test_raster_replace_band(void)
{
	rt_raster raster;
	rt_band band;
	rt_band rband;
	void *mem;
	size_t datasize;
	uint16_t width;
	uint16_t height;
	double nodata;

	raster = rt_raster_new(10, 10);
	CU_ASSERT(raster != NULL); /* or we're out of virtual memory */
	band = cu_add_band(raster, PT_8BUI, 0, 0);
	CU_ASSERT(band != NULL);
	band = cu_add_band(raster, PT_8BUI, 1, 255);
	CU_ASSERT(band != NULL);

	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);

	datasize = rt_pixtype_size(PT_8BUI) * width * height;
	mem = rtalloc(datasize);
	band = rt_band_new_inline(width, height, PT_8BUI, 1, 1, mem);
	CU_ASSERT(band != NULL);
	rt_band_set_ownsdata_flag(band, 1);

	rband = rt_raster_replace_band(raster, band, 0);
	CU_ASSERT(rband != NULL);
	rt_band_get_nodata(rt_raster_get_band(raster, 0), &nodata);
	CU_ASSERT_DOUBLE_EQUAL(nodata, 1, DBL_EPSILON);

	rt_band_destroy(rband);
	cu_free_raster(raster);
}

static void
test_raster_generate_new_band_size_overflow(void)
{
	rt_raster raster;
	int bandnum;

	raster = rt_raster_new(UINT16_MAX, UINT16_MAX);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);

	cu_error_msg_reset();
	bandnum = rt_raster_generate_new_band(raster, PT_64BF, 0, 0, 0, -1);
	CU_ASSERT_EQUAL(bandnum, -1);
	CU_ASSERT_STRING_EQUAL(cu_error_msg,
			       "rt_raster_generate_new_band: Raster dimensions are too large for band allocation");

	rt_raster_destroy(raster);
}

static size_t raster_overflow_alloc_size;

static void *poisoned_allocations[32];
static size_t poisoned_allocation_count;
static size_t poisoned_largest_allocation;
static size_t poisoned_foreign_deallocations;

static void *
raster_overflow_allocator(size_t size)
{
	raster_overflow_alloc_size = size;
	return NULL;
}

static void *
poisoned_allocator(size_t size)
{
	void *mem = default_rt_allocator(size);

	if (mem != NULL)
	{
		if (size > poisoned_largest_allocation)
			poisoned_largest_allocation = size;
		memset(mem, 0xa5, size);
		CU_ASSERT_FATAL(poisoned_allocation_count < 32);
		poisoned_allocations[poisoned_allocation_count++] = mem;
	}
	return mem;
}

static void
poisoned_deallocator(void *mem)
{
	size_t i;

	for (i = 0; i < poisoned_allocation_count; i++)
	{
		if (poisoned_allocations[i] == mem)
		{
			poisoned_allocations[i] = NULL;
			default_rt_deallocator(mem);
			return;
		}
	}
	poisoned_foreign_deallocations++;
}

static void
poisoned_allocator_reset(void)
{
	memset(poisoned_allocations, 0, sizeof(poisoned_allocations));
	poisoned_allocation_count = 0;
	poisoned_largest_allocation = 0;
	poisoned_foreign_deallocations = 0;
}

static size_t fail_nth_allocation;
static size_t fail_nth_allocation_count;
static size_t fail_nth_deallocation_count;

static void *
fail_nth_allocator(size_t size)
{
	fail_nth_allocation_count++;
	if (fail_nth_allocation_count == fail_nth_allocation)
		return NULL;
	return default_rt_allocator(size);
}

static void
fail_nth_deallocator(void *mem)
{
	fail_nth_deallocation_count++;
	default_rt_deallocator(mem);
}

static void
test_raster_serializers_reject_oversized_band(void)
{
	rt_raster raster;
	rt_band band;
	uint8_t *serialized;
	uint8_t *wkb;
	uint32_t wkbsize = UINT32_MAX;
	uint32_t hexwkbsize = UINT32_MAX;
	char *hexwkb;
	void *data;

	/* 32768 x 32768 x 32BUI is the supplied boundary without allocating it. */
	raster = rt_raster_new(32768, 32768);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);
	data = rtalloc(1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(data);
	band = rt_band_new_inline(32768, 32768, PT_32BUI, 0, 0, data);
	CU_ASSERT_PTR_NOT_NULL_FATAL(band);
	rt_band_set_ownsdata_flag(band, 1);
	CU_ASSERT_EQUAL(rt_raster_add_band(raster, band, 0), 0);

	/* A rejected size must not reach an allocator as a zero-byte request. */
	raster_overflow_alloc_size = SIZE_MAX;
	rt_set_handlers(raster_overflow_allocator,
			default_rt_reallocator,
			default_rt_deallocator,
			default_rt_error_handler,
			default_rt_info_handler,
			default_rt_warning_handler);
	serialized = rt_raster_serialize(raster);
	CU_ASSERT_PTR_NULL(serialized);
	CU_ASSERT_EQUAL(raster_overflow_alloc_size, SIZE_MAX);

	raster_overflow_alloc_size = SIZE_MAX;
	wkb = rt_raster_to_wkb(raster, FALSE, &wkbsize);
	CU_ASSERT_PTR_NULL(wkb);
	CU_ASSERT_EQUAL(wkbsize, 0);
	CU_ASSERT_EQUAL(raster_overflow_alloc_size, SIZE_MAX);

	raster_overflow_alloc_size = SIZE_MAX;
	hexwkb = rt_raster_to_hexwkb(raster, FALSE, &hexwkbsize);
	CU_ASSERT_PTR_NULL(hexwkb);
	CU_ASSERT_EQUAL(hexwkbsize, 0);
	CU_ASSERT_EQUAL(raster_overflow_alloc_size, SIZE_MAX);

	rt_install_default_allocators();
	cu_free_raster(raster);
}

static void
test_raster_deserialize_rejects_truncated_band(void)
{
	uint8_t serialized[sizeof(struct rt_raster_serialized_t) + 8] = {0};
	struct rt_raster_serialized_t *header = (struct rt_raster_serialized_t *)serialized;
	rt_raster raster;

	header->size = sizeof(serialized);
	header->numBands = 1;
	header->width = 32768;
	header->height = 32768;
	serialized[sizeof(*header)] = PT_32BUI;

	/* The band header is present, but its 4 GiB inline payload is absent. */
	raster = rt_raster_deserialize_with_size(serialized, sizeof(serialized), FALSE);
	CU_ASSERT_PTR_NULL(raster);
}

static void
test_raster_deserialize_uses_correct_size_contract(void)
{
	uint8_t serialized[sizeof(struct rt_raster_serialized_t)] = {0};
	struct rt_raster_serialized_t *header = (struct rt_raster_serialized_t *)serialized;
	rt_raster raster;

	/* Raw serialized input cannot claim bytes that were not supplied. */
	header->size = sizeof(serialized) + 1;
	header->numBands = 0;
	raster = rt_raster_deserialize_with_size(serialized, sizeof(serialized), FALSE);
	CU_ASSERT_PTR_NULL(raster);

	/* PostgreSQL overlays this word with a varlena length encoding. */
	raster = rt_raster_deserialize_pgraster(serialized, sizeof(serialized), FALSE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);
	CU_ASSERT_EQUAL(raster->size, sizeof(serialized));
	cu_free_raster(raster);

	/* Header-only calls use the same authoritative datum boundary. */
	header->numBands = 1;
	raster = rt_raster_deserialize_pgraster(serialized, sizeof(serialized), TRUE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);
	CU_ASSERT_PTR_NULL(raster->bands);
	rtdealloc(raster);
}

static void
test_raster_readers_destroy_only_initialized_bands(void)
{
	uint8_t serialized[sizeof(struct rt_raster_serialized_t) + 4] = {0};
	uint8_t wkb[sizeof(struct rt_raster_serialized_t) + 1 + 4] = {0};
	struct rt_raster_serialized_t *header = (struct rt_raster_serialized_t *)serialized;
	size_t wkb_header_size = sizeof(struct rt_raster_serialized_t) - 3;
	rt_raster raster;

	header->numBands = 1;
	wkb[0] = 1;
	wkb[3] = 1;

	poisoned_allocator_reset();
	rt_set_handlers(poisoned_allocator,
			default_rt_reallocator,
			poisoned_deallocator,
			default_rt_error_handler,
			default_rt_info_handler,
			default_rt_warning_handler);

	/* No band byte, invalid offline pixtype, and missing offline path. */
	header->size = sizeof(*header);
	raster = rt_raster_deserialize_with_size(serialized, header->size, FALSE);
	CU_ASSERT_PTR_NULL(raster);
	serialized[sizeof(*header)] = 0x80 | PT_END;
	header->size = sizeof(*header) + 1;
	raster = rt_raster_deserialize_with_size(serialized, header->size, FALSE);
	CU_ASSERT_PTR_NULL(raster);
	serialized[sizeof(*header)] = 0x80 | PT_8BUI;
	serialized[sizeof(*header) + 1] = 0;
	serialized[sizeof(*header) + 2] = 0;
	serialized[sizeof(*header) + 3] = 'x';
	header->size = sizeof(serialized);
	raster = rt_raster_deserialize_with_size(serialized, header->size, FALSE);
	CU_ASSERT_PTR_NULL(raster);

	/* The same malformed cases through the WKB reader. */
	raster = rt_raster_from_wkb(wkb, wkb_header_size);
	CU_ASSERT_PTR_NULL(raster);
	wkb[wkb_header_size] = 0x80 | PT_END;
	raster = rt_raster_from_wkb(wkb, wkb_header_size + 1);
	CU_ASSERT_PTR_NULL(raster);
	wkb[wkb_header_size] = 0x80 | PT_8BUI;
	wkb[wkb_header_size + 1] = 0;
	wkb[wkb_header_size + 2] = 0;
	wkb[wkb_header_size + 3] = 'x';
	raster = rt_raster_from_wkb(wkb, wkb_header_size + 4);
	CU_ASSERT_PTR_NULL(raster);

	CU_ASSERT_EQUAL(poisoned_foreign_deallocations, 0);
	CU_ASSERT_TRUE(poisoned_largest_allocation < 1024);
	for (size_t i = 0; i < poisoned_allocation_count; i++)
		CU_ASSERT_PTR_NULL(poisoned_allocations[i]);
	rt_install_default_allocators();
}

static void
test_raster_band_init_value(void)
{
	uint8_t data = 0;
	rt_band band;

	band = rt_band_new_inline(1, 1, PT_8BUI, 0, 0, &data);
	CU_ASSERT_PTR_NOT_NULL_FATAL(band);
	data = 0xa5;
	rt_band_init_value(band, 0);
	CU_ASSERT_EQUAL(data, 0);
	rt_band_destroy(band);

#if SIZE_MAX == UINT32_MAX
	/* This boundary wraps to zero before the fixed product guard on 32-bit. */
	band = rt_band_new_inline(32768, 32768, PT_32BUI, 0, 0, &data);
	CU_ASSERT_PTR_NOT_NULL_FATAL(band);
	cu_error_msg_reset();
	rt_set_handlers(default_rt_allocator,
			default_rt_reallocator,
			default_rt_deallocator,
			default_rt_error_handler,
			default_rt_info_handler,
			default_rt_warning_handler);
	rt_band_init_value(band, 0);
	CU_ASSERT_STRING_EQUAL(cu_error_msg,
			       "rt_band_init_value: Raster dimensions are too large for band initialization");
	rt_install_default_allocators();
	rt_band_destroy(band);
#else
	CU_ASSERT_TRUE(SIZE_MAX > UINT32_MAX);
#endif
}

static void
test_raster_band_allocation_failure_cleanup(void)
{
	uint8_t data = 0;
	rt_band source;
	rt_band duplicate;
	rt_band reclassed;
	struct rt_classpair_t pair = {0, 0};
	struct rt_reclassmap_t map = {1, PT_8BUI, PT_8BUI, &pair};

	source = rt_band_new_inline(1, 1, PT_8BUI, 1, 0, &data);
	CU_ASSERT_PTR_NOT_NULL_FATAL(source);
	fail_nth_allocation = 2;
	fail_nth_allocation_count = 0;
	fail_nth_deallocation_count = 0;
	rt_set_handlers(fail_nth_allocator,
			default_rt_reallocator,
			fail_nth_deallocator,
			default_rt_error_handler,
			default_rt_info_handler,
			default_rt_warning_handler);
	duplicate = rt_band_duplicate(source);
	CU_ASSERT_PTR_NULL(duplicate);
	CU_ASSERT_EQUAL(fail_nth_allocation_count, 2);
	CU_ASSERT_EQUAL(fail_nth_deallocation_count, 1);
	rt_install_default_allocators();
	rt_band_destroy(source);

	source = rt_band_new_inline(1, 1, PT_8BUI, 1, 0, &data);
	CU_ASSERT_PTR_NOT_NULL_FATAL(source);
	fail_nth_allocation = 1;
	fail_nth_allocation_count = 0;
	fail_nth_deallocation_count = 0;
	rt_set_handlers(fail_nth_allocator,
			default_rt_reallocator,
			fail_nth_deallocator,
			default_rt_error_handler,
			default_rt_info_handler,
			default_rt_warning_handler);
	reclassed = rt_band_reclass_exact(source, &map, 1, 0);
	CU_ASSERT_PTR_NULL(reclassed);
	CU_ASSERT_EQUAL(fail_nth_allocation_count, 1);
	CU_ASSERT_EQUAL(fail_nth_deallocation_count, 0);
	rt_install_default_allocators();
	rt_band_destroy(source);
}

static void
test_raster_size_aware_roundtrips(void)
{
	rt_raster raster;
	rt_raster copy;
	rt_band band;
	uint8_t *serialized;
	size_t serialized_size;

	raster = rt_raster_new(0, 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);
	serialized = rt_raster_serialize(raster);
	CU_ASSERT_PTR_NOT_NULL_FATAL(serialized);
	serialized_size = ((struct rt_raster_serialized_t *)serialized)->size;
	copy = rt_raster_deserialize_with_size(serialized, serialized_size, FALSE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(copy);
	CU_ASSERT_EQUAL(copy->width, 0);
	CU_ASSERT_EQUAL(copy->height, 0);
	CU_ASSERT_EQUAL(copy->numBands, 0);
	cu_free_raster(copy);
	free(serialized);
	cu_free_raster(raster);

	raster = rt_raster_new(1, 1);
	CU_ASSERT_PTR_NOT_NULL_FATAL(raster);
	band = cu_add_band(raster, PT_8BUI, 0, 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(band);
	serialized = rt_raster_serialize(raster);
	CU_ASSERT_PTR_NOT_NULL_FATAL(serialized);
	serialized_size = ((struct rt_raster_serialized_t *)serialized)->size;
	copy = rt_raster_deserialize_with_size(serialized, serialized_size, FALSE);
	CU_ASSERT_PTR_NOT_NULL_FATAL(copy);
	CU_ASSERT_EQUAL(copy->width, 1);
	CU_ASSERT_EQUAL(copy->height, 1);
	CU_ASSERT_EQUAL(copy->numBands, 1);
	cu_free_raster(copy);
	free(serialized);
	cu_free_raster(raster);
}

/* register tests */
void raster_basics_suite_setup(void);
void
raster_basics_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("raster_basics", NULL, NULL);
	PG_ADD_TEST(suite, test_raster_new);
	PG_ADD_TEST(suite, test_raster_empty);
	PG_ADD_TEST(suite, test_raster_metadata);
	PG_ADD_TEST(suite, test_raster_clone);
	PG_ADD_TEST(suite, test_raster_from_band);
	PG_ADD_TEST(suite, test_raster_replace_band);
	PG_ADD_TEST(suite, test_raster_generate_new_band_size_overflow);
	PG_ADD_TEST(suite, test_raster_serializers_reject_oversized_band);
	PG_ADD_TEST(suite, test_raster_deserialize_rejects_truncated_band);
	PG_ADD_TEST(suite, test_raster_deserialize_uses_correct_size_contract);
	PG_ADD_TEST(suite, test_raster_readers_destroy_only_initialized_bands);
	PG_ADD_TEST(suite, test_raster_band_init_value);
	PG_ADD_TEST(suite, test_raster_band_allocation_failure_cleanup);
	PG_ADD_TEST(suite, test_raster_size_aware_roundtrips);
}
