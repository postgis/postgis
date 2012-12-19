#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "rt_api.h"
#include "check.h"

static char *
lwgeom_to_text(const LWGEOM *lwgeom) {
	char *wkt;
	size_t wkt_size;

	wkt = lwgeom_to_wkt(lwgeom, WKT_ISO, DBL_DIG, &wkt_size);

	return wkt;
}

static rt_band
addBand(rt_raster raster, rt_pixtype pixtype, int hasnodata, double nodataval)
{
    void* mem;
    int32_t bandNum;
    size_t datasize;
    uint16_t width = rt_raster_get_width(raster);
    uint16_t height = rt_raster_get_height(raster);

    datasize = rt_pixtype_size(pixtype)*width*height;
    mem = rtalloc(datasize);

    rt_band band = rt_band_new_inline(width, height,
                                      pixtype, hasnodata, nodataval, mem);
    assert(band);
    bandNum = rt_raster_add_band(raster, band, 100);
    assert(bandNum>=0);
    return band;
}

static void
deepRelease(rt_raster raster) {
	uint16_t i;
	uint16_t nbands = rt_raster_get_num_bands(raster);

	for (i = 0; i < nbands; ++i) {
		rt_band band = rt_raster_get_band(raster, i);
		if (!rt_band_is_offline(band) && !rt_band_get_ownsdata_flag(band)) {
			void* mem = rt_band_get_data(band);
			if (mem) rtdealloc(mem);
		}
		rt_band_destroy(band);
	}

	rt_raster_destroy(raster);
	raster = NULL;
}

static rt_raster
fillRasterToPolygonize(int hasnodata, double nodatavalue)
{
    /* Create raster */

    uint16_t width = 9;
    uint16_t height = 9;

    rt_raster raster = rt_raster_new(width, height);
		rt_raster_set_scale(raster, 1, 1);

    rt_band band = addBand(raster, PT_32BF, hasnodata, nodatavalue);

    {
        int x, y;
        for (x = 0; x < rt_band_get_width(band); ++x)
            for (y = 0; y < rt_band_get_height(band); ++y)
                rt_band_set_pixel(band, x, y, 0.0, NULL);
    }
    
    rt_band_set_pixel(band, 3, 1, 1.8, NULL);
    rt_band_set_pixel(band, 4, 1, 1.8, NULL);
    rt_band_set_pixel(band, 5, 1, 2.8, NULL);
    rt_band_set_pixel(band, 2, 2, 1.8, NULL);
    rt_band_set_pixel(band, 3, 2, 1.8, NULL);
    rt_band_set_pixel(band, 4, 2, 1.8, NULL);
    rt_band_set_pixel(band, 5, 2, 2.8, NULL);
    rt_band_set_pixel(band, 6, 2, 2.8, NULL);
    rt_band_set_pixel(band, 1, 3, 1.8, NULL);
    rt_band_set_pixel(band, 2, 3, 1.8, NULL);
    rt_band_set_pixel(band, 6, 3, 2.8, NULL);
    rt_band_set_pixel(band, 7, 3, 2.8, NULL);
    rt_band_set_pixel(band, 1, 4, 1.8, NULL);
    rt_band_set_pixel(band, 2, 4, 1.8, NULL);
    rt_band_set_pixel(band, 6, 4, 2.8, NULL);
    rt_band_set_pixel(band, 7, 4, 2.8, NULL);
    rt_band_set_pixel(band, 1, 5, 1.8, NULL);
    rt_band_set_pixel(band, 2, 5, 1.8, NULL);
    rt_band_set_pixel(band, 6, 5, 2.8, NULL);
    rt_band_set_pixel(band, 7, 5, 2.8, NULL);
    rt_band_set_pixel(band, 2, 6, 1.8, NULL);
    rt_band_set_pixel(band, 3, 6, 1.8, NULL);
    rt_band_set_pixel(band, 4, 6, 1.8, NULL);
    rt_band_set_pixel(band, 5, 6, 2.8, NULL);
    rt_band_set_pixel(band, 6, 6, 2.8, NULL);
    rt_band_set_pixel(band, 3, 7, 1.8, NULL);
    rt_band_set_pixel(band, 4, 7, 1.8, NULL);
    rt_band_set_pixel(band, 5, 7, 2.8, NULL);

    return raster;
}

static void testGDALConfigured() {
	int rtn;
	
	rtn = rt_util_gdal_configured();
	CHECK((rtn != 0));
}

static void testGDALPolygonize() {
	int i;
	rt_raster rt;
	int nPols = 0;
	rt_geomval gv = NULL;
	char *wkt = NULL;

	rt = fillRasterToPolygonize(1, -1.0);
	CHECK(rt_raster_has_band(rt, 0));

	nPols = 0;
	gv = rt_raster_gdal_polygonize(rt, 0, TRUE, &nPols);

	/*
	for (i = 0; i < nPols; i++) {
		wkt = lwgeom_to_text((const LWGEOM *) gv[i].geom);
		printf("(i, val, geom) = (%d, %f, '%s')\n", i, gv[i].val, wkt);
		rtdealloc(wkt);
	}
	*/

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[0].val, 1.8));
#else
	CHECK(FLT_EQ(gv[0].val, 2.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[0].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 3,3 6,6 6,6 3,3 3))"));
	rtdealloc(wkt);

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[2].val, 2.8));
#else
	CHECK(FLT_EQ(gv[2].val, 3.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[2].geom);
	CHECK(!strcmp(wkt, "POLYGON((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[3].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);

	for (i = 0; i < nPols; i++) lwgeom_free((LWGEOM *) gv[i].geom);
	rtdealloc(gv);
	deepRelease(rt);

	/* Second test: NODATA value = 1.8 */
#ifdef GDALFPOLYGONIZE
	rt = fillRasterToPolygonize(1, 1.8);
#else
	rt = fillRasterToPolygonize(1, 2.0);
#endif

	/* We can check rt_raster_has_band here too */
	CHECK(rt_raster_has_band(rt, 0));

	nPols = 0;
	gv = rt_raster_gdal_polygonize(rt, 0, TRUE, &nPols);

	/*
	for (i = 0; i < nPols; i++) {
		wkt = lwgeom_to_text((const LWGEOM *) gv[i].geom);
		printf("(i, val, geom) = (%d, %f, %s)\n", i, gv[i].val, wkt);
		rtdealloc(wkt);
	}
	*/

#ifdef GDALFPOLYGONIZE
	CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 3,3 6,6 6,6 3,3 3))"));
	rtdealloc(wkt);

	CHECK(FLT_EQ(gv[2].val, 2.8));
	wkt = lwgeom_to_text((const LWGEOM *) gv[2].geom);
	CHECK(!strcmp(wkt, "POLYGON((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[3].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);
#else
	CHECK_EQUALS_DOUBLE(gv[0].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[0].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 3,3 6,6 6,6 3,3 3))"));
	rtdealloc(wkt);

	CHECK(FLT_EQ(gv[1].val, 3.0));
	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[2].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[2].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);
#endif

	for (i = 0; i < nPols; i++) lwgeom_free((LWGEOM *) gv[i].geom);
	rtdealloc(gv);
	deepRelease(rt);

	/* Third test: NODATA value = 2.8 */
#ifdef GDALFPOLYGONIZE
	rt = fillRasterToPolygonize(1, 2.8);
#else	
	rt = fillRasterToPolygonize(1, 3.0);
#endif

	/* We can check rt_raster_has_band here too */
	CHECK(rt_raster_has_band(rt, 0));

	nPols = 0;
	gv = rt_raster_gdal_polygonize(rt, 0, TRUE, &nPols);

	/*
	for (i = 0; i < nPols; i++) {
		wkt = lwgeom_to_text((const LWGEOM *) gv[i].geom);
		printf("(i, val, geom) = (%d, %f, %s)\n", i, gv[i].val, wkt);
		rtdealloc(wkt);
	}
	*/

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[0].val, 1.8));

	CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[3].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);
#else
	CHECK(FLT_EQ(gv[0].val, 2.0));

	CHECK_EQUALS_DOUBLE(gv[2].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[2].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[0].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 3,3 6,6 6,6 3,3 3))"));
	rtdealloc(wkt);

	for (i = 0; i < nPols; i++) lwgeom_free((LWGEOM *) gv[i].geom);
	rtdealloc(gv);
	deepRelease(rt);

	/* Fourth test: NODATA value = 0 */
	rt = fillRasterToPolygonize(1, 0.0);
	/* We can check rt_raster_has_band here too */
	CHECK(rt_raster_has_band(rt, 0));

	nPols = 0;
	gv = rt_raster_gdal_polygonize(rt, 0, TRUE, &nPols);
	
	/*
	for (i = 0; i < nPols; i++) {
		wkt = lwgeom_to_text((const LWGEOM *) gv[i].geom);
		printf("(i, val, geom) = (%d, %f, %s)\n", i, gv[i].val, wkt);
		rtdealloc(wkt);
	}
	*/

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[0].val, 1.8));
#else
	CHECK(FLT_EQ(gv[0].val, 2.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[0].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));
	rtdealloc(wkt);

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[1].val, 2.8));
#else
	CHECK(FLT_EQ(gv[1].val, 3.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));
	rtdealloc(wkt);

	for (i = 0; i < nPols; i++) lwgeom_free((LWGEOM *) gv[i].geom);
	rtdealloc(gv);
	deepRelease(rt);

	/* Last test: There is no NODATA value (all values are valid) */
	rt = fillRasterToPolygonize(0, 0.0);
	/* We can check rt_raster_has_band here too */
	CHECK(rt_raster_has_band(rt, 0));

	nPols = 0;
	gv = rt_raster_gdal_polygonize(rt, 0, TRUE, &nPols);

	/*
	for (i = 0; i < nPols; i++) {
		wkt = lwgeom_to_text((const LWGEOM *) gv[i].geom);
		printf("(i, val, geom) = (%d, %f, %s)\n", i, gv[i].val, wkt);
		rtdealloc(wkt);
	}
	*/

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[0].val, 1.8));
#else
	CHECK(FLT_EQ(gv[0].val, 2.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[0].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[1].geom);
	CHECK(!strcmp(wkt, "POLYGON((3 3,3 6,6 6,6 3,3 3))"));
	rtdealloc(wkt);

#ifdef GDALFPOLYGONIZE
	CHECK(FLT_EQ(gv[2].val, 2.8));
#else
	CHECK(FLT_EQ(gv[2].val, 3.0));
#endif

	wkt = lwgeom_to_text((const LWGEOM *) gv[2].geom);
	CHECK(!strcmp(wkt, "POLYGON((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));
	rtdealloc(wkt);

	CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
	wkt = lwgeom_to_text((const LWGEOM *) gv[3].geom);
	CHECK(!strcmp(wkt, "POLYGON((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
	rtdealloc(wkt);

	for (i = 0; i < nPols; i++) lwgeom_free((LWGEOM *) gv[i].geom);
	rtdealloc(gv);
	deepRelease(rt);
}

static void testBand1BB(rt_band band)
{
    int failure;
		int clamped;
    double val = 0;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
    CHECK(!clamped);
		rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
    CHECK(!clamped);
		rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_nodata(band, 3, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 3, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
                CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
                CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 0, NULL);
                CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
                CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0);

            }
        }
    }

}

static void testBand2BUI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(band, 3, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 3);

    failure = rt_band_set_nodata(band, 4, &clamped); /* invalid: out of range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_nodata(band, 5, &clamped); /* invalid: out of range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 4, &clamped); /* out of range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 5, &clamped); /* out of range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);


    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 2, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 2);

                failure = rt_band_set_pixel(band, x, y, 3, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 3);
            }
        }
    }

}

static void testBand4BUI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(band, 4, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(band, 8, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(band, 15, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(band, 16, &clamped);  /* out of value range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_nodata(band, 17, &clamped);  /* out of value range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 35, &clamped); /* out of value range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);


    {
        int x, y;

        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 3, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 3);

                failure = rt_band_set_pixel(band, x, y, 7, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 7);

                failure = rt_band_set_pixel(band, x, y, 15, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 15);
            }
        }
    }

}

static void testBand8BUI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(band, 4, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(band, 8, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(band, 15, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(band, 31, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(band, 255, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(band, 256, &clamped); /* out of value range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    failure = rt_band_set_pixel(band, 0, 0, 256, &clamped); /* out of value range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 31, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 31);

                failure = rt_band_set_pixel(band, x, y, 255, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(band, x, y, 1, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);
            }
        }
    }
}

static void testBand8BSI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 2, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(band, 4, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(band, 8, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(band, 15, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(band, 31, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(band, -127, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, -127);

    failure = rt_band_set_nodata(band, 127, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 127);

    /* out of range (-127..127) */
    failure = rt_band_set_nodata(band, -129, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-127..127) */
    failure = rt_band_set_nodata(band, 129, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-127..127) */
    failure = rt_band_set_pixel(band, 0, 0, -129, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-127..127) */
    failure = rt_band_set_pixel(band, 0, 0, 129, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);


    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 31, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 31);

                failure = rt_band_set_pixel(band, x, y, 1, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, -127, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, -127);

                failure = rt_band_set_pixel(band, x, y, 127, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 127);

            }
        }
    }
}

static void testBand16BUI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 31, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(band, 255, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(band, 65535, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(band, 65536, &clamped); /* out of range */
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of value range */
    failure = rt_band_set_pixel(band, 0, 0, 65536, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of dimensions range */
    failure = rt_band_set_pixel(band, rt_band_get_width(band), 0, 0, &clamped);
    CHECK((failure != ES_NONE));

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 255, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(band, x, y, 65535, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 65535);
            }
        }
    }
}

static void testBand16BSI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 31, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(band, 255, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(band, -32767, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, -32767);

    failure = rt_band_set_nodata(band, 32767, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, 32767);

    /* out of range (-32767..32767) */
    failure = rt_band_set_nodata(band, -32769, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-32767..32767) */
    failure = rt_band_set_nodata(band, 32769, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-32767..32767) */
    failure = rt_band_set_pixel(band, 0, 0, -32769, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of range (-32767..32767) */
    failure = rt_band_set_pixel(band, 0, 0, 32769, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of dimensions range */
    failure = rt_band_set_pixel(band, rt_band_get_width(band), 0, 0, NULL);
    CHECK((failure != ES_NONE));

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 255, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(band, x, y, -32767, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, -32767);

                failure = rt_band_set_pixel(band, x, y, 32767, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 32767);
            }
        }
    }
}

static void testBand32BUI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 65535, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(band, 4294967295UL, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 4294967295UL);

    /* out of range */
    failure = rt_band_set_nodata(band, 4294967296ULL, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of value range */
    failure = rt_band_set_pixel(band, 0, 0, 4294967296ULL, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of dimensions range */
    failure = rt_band_set_pixel(band, rt_band_get_width(band), 0, 4294967296ULL, NULL);
    CHECK((failure != ES_NONE));

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 0, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(band, x, y, 65535, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 65535);

                failure = rt_band_set_pixel(band, x, y, 4294967295UL, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 4294967295UL);
            }
        }
    }
}

static void testBand32BSI(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 65535, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(band, 2147483647, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    /*printf("32BSI pix is %ld\n", (long int)val);*/
    CHECK_EQUALS(val, 2147483647);

    /* out of range */
    failure = rt_band_set_nodata(band, 2147483648UL, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of value range */
    failure = rt_band_set_pixel(band, 0, 0, 2147483648UL, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(clamped);

    /* out of dimensions range */
    failure = rt_band_set_pixel(band, rt_band_get_width(band), 0, 0, NULL);
    CHECK((failure != ES_NONE));


    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 0, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(band, x, y, 65535, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 65535);

                failure = rt_band_set_pixel(band, x, y, 2147483647, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 2147483647);
            }
        }
    }
}

static void testBand32BF(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 65535.5, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    //printf("set 65535.56 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS_DOUBLE(val, 65535.5);

    failure = rt_band_set_nodata(band, 0.006, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS_DOUBLE(val, 0.0060000000521540); /* XXX: Alternatively, use CHECK_EQUALS_DOUBLE_EX */

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 0, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(band, x, y, 65535.5, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS_DOUBLE(val, 65535.5);

                failure = rt_band_set_pixel(band, x, y, 0.006, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS_DOUBLE(val, 0.0060000000521540);

            }
        }
    }
}

static void testBand64BF(rt_band band)
{
    double val;
    int failure;
		int clamped;

    failure = rt_band_set_nodata(band, 1, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(band, 0, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(band, 65535.56, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 65535.56);

    failure = rt_band_set_nodata(band, 0.006, &clamped);
    CHECK_EQUALS(failure, ES_NONE);
		CHECK(!clamped);
    rt_band_get_nodata(band, &val);
    CHECK_EQUALS(val, 0.006);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(band); ++x)
        {
            for (y=0; y<rt_band_get_height(band); ++y)
            {
                failure = rt_band_set_pixel(band, x, y, 1, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(band, x, y, 0, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(band, x, y, 65535.56, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 65535.56);

                failure = rt_band_set_pixel(band, x, y, 0.006, NULL);
    						CHECK_EQUALS(failure, ES_NONE);
                failure = rt_band_get_pixel(band, x, y, &val, NULL);
							 	CHECK_EQUALS(failure, ES_NONE);
                CHECK_EQUALS(val, 0.006);

            }
        }
    }
}

static void testBandHasNoData(rt_band band)
{
    int flag;

    flag = rt_band_get_hasnodata_flag(band);
    CHECK_EQUALS(flag, 1);

    rt_band_set_hasnodata_flag(band, 0);
    flag = rt_band_get_hasnodata_flag(band);
    CHECK_EQUALS(flag, 0);

    rt_band_set_hasnodata_flag(band, 10);
    flag = rt_band_get_hasnodata_flag(band);
    CHECK_EQUALS(flag, 1);

    rt_band_set_hasnodata_flag(band, -10);
    flag = rt_band_get_hasnodata_flag(band);
    CHECK_EQUALS(flag, 1);
}

static void testRasterFromBand() {
	uint32_t bandNums[] = {1,3};
	int lenBandNums = 2;
	rt_raster raster;
	rt_raster rast;
	rt_band band;
	uint32_t xmax = 100;
	uint32_t ymax = 100;
	uint32_t x;

	raster = rt_raster_new(xmax, ymax);
	assert(raster);

	for (x = 0; x < 5; x++) {
		band = addBand(raster, PT_32BUI, 0, 0);
		CHECK(band);
		rt_band_set_nodata(band, 0, NULL);
	}

	rast = rt_raster_from_band(raster, bandNums, lenBandNums);
	assert(rast);

	CHECK(rast);
	CHECK(!rt_raster_is_empty(rast));
	CHECK(rt_raster_has_band(rast, 1));

	deepRelease(rast);
	deepRelease(raster);
}

static void testBandStats() {
	rt_bandstats stats = NULL;
	rt_histogram histogram = NULL;
	double bin_width[] = {100};
	double quantiles[] = {0.1, 0.3, 0.5, 0.7, 0.9};
	double quantiles2[] = {0.66666667};
	rt_quantile quantile = NULL;
	uint32_t count = 0;

	rt_raster raster;
	rt_band band;
	uint32_t x;
	uint32_t xmax = 100;
	uint32_t y;
	uint32_t ymax = 100;
	uint32_t max_run;
	double nodata;
	int rtn;

	uint32_t values[] = {0, 91, 55, 86, 76, 41, 36, 97, 25, 63, 68, 2, 78, 15, 82, 47};
	struct quantile_llist *qlls = NULL;
	uint32_t qlls_count;

	raster = rt_raster_new(xmax, ymax);
	assert(raster);
	band = addBand(raster, PT_32BUI, 1, 0);
	CHECK(band);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, x + y, NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	rt_band_get_nodata(band, &nodata);
	CHECK_EQUALS(nodata, 0);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 1, 0, 1, NULL, NULL, NULL);
	CHECK(stats);
	CHECK_EQUALS(stats->min, 1);
	CHECK_EQUALS(stats->max, 198);

	quantile = (rt_quantile) rt_band_get_quantiles(stats, NULL, 0, &count);
	CHECK(quantile);
	rtdealloc(quantile);

	histogram = (rt_histogram) rt_band_get_histogram(stats, 0, NULL, 0, 0, 0, 0, &count);
	CHECK(histogram);
	rtdealloc(histogram);

	histogram = (rt_histogram) rt_band_get_histogram(stats, 0, NULL, 0, 1, 0, 0, &count);
	CHECK(histogram);
	rtdealloc(histogram);

	histogram = (rt_histogram) rt_band_get_histogram(stats, 0, bin_width, 1, 0, 0, 0, &count);
	CHECK(histogram);
	rtdealloc(histogram);

	rtdealloc(stats->values);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 1, 0.1, 1, NULL, NULL, NULL);
	CHECK(stats);

	quantile = (rt_quantile) rt_band_get_quantiles(stats, NULL, 0, &count);
	CHECK(quantile);
	rtdealloc(quantile);

	quantile = (rt_quantile) rt_band_get_quantiles(stats, quantiles, 5, &count);
	CHECK(quantile);
	CHECK((count == 5));
	rtdealloc(quantile);

	histogram = (rt_histogram) rt_band_get_histogram(stats, 0, NULL, 0, 0, 0, 0, &count);
	CHECK(histogram);
	rtdealloc(histogram);

	rtdealloc(stats->values);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 1, 0.15, 0, NULL, NULL, NULL);
	CHECK(stats);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 1, 0.2, 0, NULL, NULL, NULL);
	CHECK(stats);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 1, 0.25, 0, NULL, NULL, NULL);
	CHECK(stats);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 0, 0, 1, NULL, NULL, NULL);
	CHECK(stats);
	CHECK_EQUALS(stats->min, 0);
	CHECK_EQUALS(stats->max, 198);

	quantile = (rt_quantile) rt_band_get_quantiles(stats, NULL, 0, &count);
	CHECK(quantile);
	rtdealloc(quantile);

	rtdealloc(stats->values);
	rtdealloc(stats);

	stats = (rt_bandstats) rt_band_get_summary_stats(band, 0, 0.1, 1, NULL, NULL, NULL);
	CHECK(stats);

	quantile = (rt_quantile) rt_band_get_quantiles(stats, NULL, 0, &count);
	CHECK(quantile);
	rtdealloc(quantile);

	rtdealloc(stats->values);
	rtdealloc(stats);

	deepRelease(raster);

	xmax = 4;
	ymax = 4;
	raster = rt_raster_new(4, 4);
	assert(raster);
	band = addBand(raster, PT_8BUI, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, values[(x * ymax) + y], NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	rt_band_get_nodata(band, &nodata);
	CHECK_EQUALS(nodata, 0);

	quantile = (rt_quantile) rt_band_get_quantiles_stream(
		band, 1, 1, 15,
		&qlls, &qlls_count,
		quantiles2, 1,
		&count);
	CHECK(quantile);
	CHECK(count);
	CHECK((qlls_count > 0));
	CHECK(FLT_EQ(quantile[0].value, 78));
	rtdealloc(quantile);
	quantile_llist_destroy(&qlls, qlls_count);
	qlls = NULL;
	qlls_count = 0;

	deepRelease(raster);

	xmax = 100;
	ymax = 100;
	raster = rt_raster_new(xmax, ymax);
	assert(raster);
	band = addBand(raster, PT_64BF, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, (((double) x * y) + (x + y) + (x + y * x)) / (x + y + 1), NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	rt_band_get_nodata(band, &nodata);
	CHECK_EQUALS(nodata, 0);

	max_run = 5;
	for (x = 0; x < max_run; x++) {
		quantile = (rt_quantile) rt_band_get_quantiles_stream(
			band, 1, 1, xmax * ymax * max_run,
			&qlls, &qlls_count,
			quantiles2, 1,
			&count);
		CHECK(quantile);
		CHECK(count);
		CHECK((qlls_count > 0));
		rtdealloc(quantile);
	}

	quantile_llist_destroy(&qlls, qlls_count);
	qlls = NULL;
	qlls_count = 0;

	deepRelease(raster);
}

static void testRasterReplaceBand() {
	rt_raster raster;
	rt_band band;
	rt_band rband;
	void* mem;
	size_t datasize;
	uint16_t width;
	uint16_t height;
	double nodata;

	raster = rt_raster_new(10, 10);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_8BUI, 0, 0);
	CHECK(band);
	band = addBand(raster, PT_8BUI, 1, 255);
	CHECK(band);

	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);

	datasize = rt_pixtype_size(PT_8BUI) * width * height;
	mem = rtalloc(datasize);
	band = rt_band_new_inline(width, height, PT_8BUI, 1, 1, mem);
	assert(band);

	rband = rt_raster_replace_band(raster, band, 0);
	CHECK(rband);
	rt_band_get_nodata(rt_raster_get_band(raster, 0), &nodata);
	CHECK((nodata == 1));

	deepRelease(raster);

	mem = rt_band_get_data(rband);
	rt_band_destroy(rband);
	if (mem) rtdealloc(mem);
}

static void testBandReclass() {
	rt_reclassexpr *exprset;

	rt_raster raster;
	rt_band band;
	uint16_t x;
	uint16_t y;
	double nodata;
	int cnt = 2;
	int i = 0;
	int rtn;
	rt_band newband;
	double val;

	raster = rt_raster_new(100, 10);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_16BUI, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	for (x = 0; x < 100; x++) {
		for (y = 0; y < 10; y++) {
			rtn = rt_band_set_pixel(band, x, y, x * y + (x + y), NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	rt_band_get_nodata(band, &nodata);
	CHECK_EQUALS(nodata, 0);

	exprset = rtalloc(cnt * sizeof(rt_reclassexpr));
	assert(exprset);

	for (i = 0; i < cnt; i++) {
		exprset[i] = rtalloc(sizeof(struct rt_reclassexpr_t));
		assert(exprset[i]);

		if (i == 0) {
			/* nodata */
			exprset[i]->src.min = 0;
			exprset[i]->src.inc_min = 0;
			exprset[i]->src.exc_min = 0;

			exprset[i]->src.max = 0;
			exprset[i]->src.inc_max = 0;
			exprset[i]->src.exc_max = 0;

			exprset[i]->dst.min = 0;
			exprset[i]->dst.max = 0;
		}
		else {
			/* range */
			exprset[i]->src.min = 0;
			exprset[i]->src.inc_min = 0;
			exprset[i]->src.exc_min = 0;

			exprset[i]->src.max = 1000;
			exprset[i]->src.inc_max = 1;
			exprset[i]->src.exc_max = 0;

			exprset[i]->dst.min = 1;
			exprset[i]->dst.max = 255;
		}
	}

	newband = rt_band_reclass(band, PT_8BUI, 0, 0, exprset, cnt);
	CHECK(newband);

	rtn = rt_band_get_pixel(newband, 0, 0, &val, NULL);
 	CHECK_EQUALS(rtn, ES_NONE);
	CHECK_EQUALS(val, 0);

	rtn = rt_band_get_pixel(newband, 49, 5, &val, NULL);
 	CHECK_EQUALS(rtn, ES_NONE);
	CHECK_EQUALS(val, 77);

	rtn = rt_band_get_pixel(newband, 99, 9, &val, NULL);
 	CHECK_EQUALS(rtn, ES_NONE);
	CHECK_EQUALS(val, 255);

	for (i = cnt - 1; i >= 0; i--) rtdealloc(exprset[i]);
	rtdealloc(exprset);
	deepRelease(raster);

	rt_band_destroy(newband);
}

static void testGDALDrivers() {
	int i;
	uint32_t size;
	rt_gdaldriver drv;

	drv = (rt_gdaldriver) rt_raster_gdal_drivers(&size, 1);
	/*printf("size: %d\n", size);*/
	CHECK(drv);

	for (i = 0; i < size; i++) {
		/*printf("gdal_driver: %s\n", drv[i].short_name);*/
		CHECK(drv[i].short_name);
		rtdealloc(drv[i].short_name);
		rtdealloc(drv[i].long_name);
		rtdealloc(drv[i].create_options);
	}

	rtdealloc(drv);
}

static void testRasterToGDAL() {
	rt_raster raster;
	rt_band band;
	uint32_t x;
	uint32_t xmax = 100;
	uint32_t y;
	uint32_t ymax = 100;
	int rtn = 0;
	char srs[] = "PROJCS[\"unnamed\",GEOGCS[\"unnamed ellipse\",DATUM[\"unknown\",SPHEROID[\"unnamed\",6370997,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Lambert_Azimuthal_Equal_Area\"],PARAMETER[\"latitude_of_center\",45],PARAMETER[\"longitude_of_center\",-100],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1],AUTHORITY[\"EPSG\",\"2163\"]]";

	uint64_t gdalSize;
	uint8_t *gdal = NULL;

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_64BF, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	rt_raster_set_offsets(raster, -500000, 600000);
	rt_raster_set_scale(raster, 1000, -1000);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, (((double) x * y) + (x + y) + (x + y * x)) / (x + y + 1), NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	gdal = rt_raster_to_gdal(raster, srs, "GTiff", NULL, &gdalSize);
	/*printf("gdalSize: %d\n", (int) gdalSize);*/
	CHECK(gdalSize);

	/*
	FILE *fh = NULL;
	fh = fopen("/tmp/out.tif", "w");
	fwrite(gdal, sizeof(uint8_t), gdalSize, fh);
	fclose(fh);
	*/

	if (gdal) CPLFree(gdal);

	deepRelease(raster);

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_8BSI, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	rt_raster_set_offsets(raster, -500000, 600000);
	rt_raster_set_scale(raster, 1000, -1000);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, x, NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	/* add check that band isn't NODATA */
	CHECK((rt_band_check_is_nodata(band) == FALSE));

	gdal = rt_raster_to_gdal(raster, srs, "PNG", NULL, &gdalSize);
	/*printf("gdalSize: %d\n", (int) gdalSize);*/
	CHECK(gdalSize);

	if (gdal) CPLFree(gdal);

	deepRelease(raster);
}

static void testValueCount() {
	rt_valuecount vcnts = NULL;

	rt_raster raster;
	rt_band band;
	uint32_t x;
	uint32_t xmax = 100;
	uint32_t y;
	uint32_t ymax = 100;
	uint32_t rtn = 0;

	double count[] = {3, 4, 5};

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_64BF, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, (((double) x * y) + (x + y) + (x + y * x)) / (x + y + 1), NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}
	vcnts = rt_band_get_value_count(band, 1, NULL, 0, 0, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	vcnts = rt_band_get_value_count(band, 1, NULL, 0, 0.01, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	vcnts = rt_band_get_value_count(band, 1, NULL, 0, 0.1, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	vcnts = rt_band_get_value_count(band, 1, NULL, 0, 1, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	vcnts = rt_band_get_value_count(band, 1, NULL, 0, 10, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	vcnts = rt_band_get_value_count(band, 1, count, 3, 1, NULL, &rtn);
	CHECK(vcnts);
	CHECK((rtn > 0));
	rtdealloc(vcnts);

	deepRelease(raster);
}

static void testGDALToRaster() {
	rt_raster raster;
	rt_raster rast;
	rt_band band;
	const uint32_t xmax = 100;
	const uint32_t ymax = 100;
	uint32_t x;
	uint32_t y;
	int v;
	double values[xmax][ymax];
	int rtn = 0;
	double value;

	GDALDriverH gddrv = NULL;
	GDALDatasetH gdds = NULL;

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_64BF, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			values[x][y] = (((double) x * y) + (x + y) + (x + y * x)) / (x + y + 1);
			rtn = rt_band_set_pixel(band, x, y, values[x][y], NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	gdds = rt_raster_to_gdal_mem(raster, NULL, NULL, NULL, 0, &gddrv);
	CHECK(gddrv);
	CHECK(gdds);
	CHECK((GDALGetRasterXSize(gdds) == xmax));
	CHECK((GDALGetRasterYSize(gdds) == ymax));

	rast = rt_raster_from_gdal_dataset(gdds);
	CHECK(rast);
	CHECK((rt_raster_get_num_bands(rast) == 1));

	band = rt_raster_get_band(rast, 0);
	CHECK(band);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_get_pixel(band, x, y, &value, NULL);
 			CHECK_EQUALS(rtn, ES_NONE);
			CHECK(FLT_EQ(value, values[x][y]));
		}
	}

	GDALClose(gdds);

	deepRelease(rast);
	deepRelease(raster);

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_8BSI, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	v = -127;
	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			values[x][y] = v++;
			rtn = rt_band_set_pixel(band, x, y, values[x][y], NULL);
			CHECK_EQUALS(rtn, ES_NONE);
			if (v == 128)
				v = -127;
		}
	}

	gdds = rt_raster_to_gdal_mem(raster, NULL, NULL, NULL, 0, &gddrv);
	CHECK(gddrv);
	CHECK(gdds);
	CHECK((GDALGetRasterXSize(gdds) == xmax));
	CHECK((GDALGetRasterYSize(gdds) == ymax));

	rast = rt_raster_from_gdal_dataset(gdds);
	CHECK(rast);
	CHECK((rt_raster_get_num_bands(rast) == 1));

	band = rt_raster_get_band(rast, 0);
	CHECK(band);
	CHECK((rt_band_get_pixtype(band) == PT_16BSI));

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_get_pixel(band, x, y, &value, NULL);
 			CHECK_EQUALS(rtn, ES_NONE);
			CHECK(FLT_EQ(value, values[x][y]));
		}
	}

	GDALClose(gdds);

	deepRelease(rast);
	deepRelease(raster);
}

static void testGDALWarp() {
	rt_raster raster;
	rt_raster rast;
	rt_band band;
	uint32_t x;
	uint32_t xmax = 100;
	uint32_t y;
	uint32_t ymax = 100;
	int rtn = 0;
	double value = 0;

	char src_srs[] = "PROJCS[\"unnamed\",GEOGCS[\"unnamed ellipse\",DATUM[\"unknown\",SPHEROID[\"unnamed\",6370997,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Lambert_Azimuthal_Equal_Area\"],PARAMETER[\"latitude_of_center\",45],PARAMETER[\"longitude_of_center\",-100],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1],AUTHORITY[\"EPSG\",\"2163\"]]";

	char dst_srs[] = "PROJCS[\"NAD83 / California Albers\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4269\"]],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],PROJECTION[\"Albers_Conic_Equal_Area\"],PARAMETER[\"standard_parallel_1\",34],PARAMETER[\"standard_parallel_2\",40.5],PARAMETER[\"latitude_of_center\",0],PARAMETER[\"longitude_of_center\",-120],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",-4000000],AUTHORITY[\"EPSG\",\"3310\"],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH]]";

	raster = rt_raster_new(xmax, ymax);
	assert(raster); /* or we're out of virtual memory */
	band = addBand(raster, PT_64BF, 0, 0);
	CHECK(band);
	rt_band_set_nodata(band, 0, NULL);

	rt_raster_set_offsets(raster, -500000, 600000);
	rt_raster_set_scale(raster, 1000, -1000);

	for (x = 0; x < xmax; x++) {
		for (y = 0; y < ymax; y++) {
			rtn = rt_band_set_pixel(band, x, y, (((double) x * y) + (x + y) + (x + y * x)) / (x + y + 1), NULL);
			CHECK_EQUALS(rtn, ES_NONE);
		}
	}

	rast = rt_raster_gdal_warp(
		raster,
		src_srs, dst_srs,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		GRA_NearestNeighbour, -1
	);
	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 122));
	CHECK((rt_raster_get_height(rast) == 116));
	CHECK((rt_raster_get_num_bands(rast) != 0));

	band = rt_raster_get_band(rast, 0);
	CHECK(band);

	CHECK(rt_band_get_hasnodata_flag(band));
	rt_band_get_nodata(band, &value);
	CHECK(FLT_EQ(value, 0.));

	CHECK_EQUALS(rt_band_get_pixel(band, 0, 0, &value, NULL), ES_NONE);
	CHECK(FLT_EQ(value, 0.));

	deepRelease(rast);
	deepRelease(raster);
}

static void testComputeSkewedExtent() {
	rt_envelope extent;
	rt_raster rast;
	double skew[2] = {0.25, 0.25};
	double scale[2] = {1, -1};

	extent.MinX = 0;
	extent.MaxY = 0;
	extent.MaxX = 2;
	extent.MinY = -2;
	extent.UpperLeftX = extent.MinX;
	extent.UpperLeftY = extent.MaxY;

	rast = rt_raster_compute_skewed_raster(
		extent,
		skew,
		scale,
		0
	);

	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 2));
	CHECK((rt_raster_get_height(rast) == 3));
	CHECK(FLT_EQ(rt_raster_get_x_offset(rast), -0.5));
	CHECK(FLT_EQ(rt_raster_get_y_offset(rast), 0));

	deepRelease(rast);
}

static void testGDALRasterize() {
	rt_raster raster;
	char srs[] = "PROJCS[\"unnamed\",GEOGCS[\"unnamed ellipse\",DATUM[\"unknown\",SPHEROID[\"unnamed\",6370997,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Lambert_Azimuthal_Equal_Area\"],PARAMETER[\"latitude_of_center\",45],PARAMETER[\"longitude_of_center\",-100],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"Meter\",1],AUTHORITY[\"EPSG\",\"2163\"]]";
	const char wkb_hex[] = "010300000001000000050000000000000080841ec100000000600122410000000080841ec100000000804f22410000000040e81dc100000000804f22410000000040e81dc100000000600122410000000080841ec10000000060012241";
	const char *pos = wkb_hex;
	unsigned char *wkb = NULL;
	int wkb_len = 0;
	int i;
	double scale_x = 100;
	double scale_y = -100;

	rt_pixtype pixtype[] = {PT_8BUI};
	double init[] = {0};
	double value[] = {1};
	double nodata[] = {0};
	uint8_t nodata_mask[] = {1};

	/* hex to byte */
	wkb_len = (int) ceil(((double) strlen(wkb_hex)) / 2);
	wkb = (unsigned char *) rtalloc(sizeof(unsigned char) * wkb_len);
	for (i = 0; i < wkb_len; i++) {
		sscanf(pos, "%2hhx", &wkb[i]);
		pos += 2;
	}

	raster = rt_raster_gdal_rasterize(
		wkb,
		wkb_len, srs,
		1, pixtype,
		init, value,
		nodata, nodata_mask,
		NULL, NULL,
		&scale_x, &scale_y,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL
	);

	CHECK(raster);
	CHECK((rt_raster_get_width(raster) == 100));
	CHECK((rt_raster_get_height(raster) == 100));
	CHECK((rt_raster_get_num_bands(raster) != 0));
	CHECK((rt_raster_get_x_offset(raster) == -500000));
	CHECK((rt_raster_get_y_offset(raster) == 600000));
	deepRelease(raster);

	rtdealloc(wkb);
}

static void testIntersects() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	rtn = rt_raster_intersects(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_intersects(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testOverlaps() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	rtn = rt_raster_overlaps(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_overlaps(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testTouches() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_touches(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_touches(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 3)
	*/
	rt_raster_set_offsets(rast2, 0, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-1, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 3)
	*/
	rt_raster_set_offsets(rast2, -1, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_touches(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testContains() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_contains(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 3)
	*/
	rt_raster_set_offsets(rast2, 0, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-1, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 3)
	*/
	rt_raster_set_offsets(rast2, -1, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_contains(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testContainsProperly() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_contains_properly(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 3)
	*/
	rt_raster_set_offsets(rast2, 0, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-1, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 3)
	*/
	rt_raster_set_offsets(rast2, -1, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_contains_properly(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testCovers() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_covers(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_covers(
		rast1, -1,
		rast2, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 3)
	*/
	rt_raster_set_offsets(rast2, 0, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-1, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 3)
	*/
	rt_raster_set_offsets(rast2, -1, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_covers(
		rast1, 0,
		rast2, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testCoveredBy() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_coveredby(
		rast1, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_coveredby(
		rast2, -1,
		rast1, -1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 3)
	*/
	rt_raster_set_offsets(rast2, 0, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-1, 1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 3)
	*/
	rt_raster_set_offsets(rast2, -1, 1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_coveredby(
		rast2, 0,
		rast1, 0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testDWithin() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast1, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast1, 0,
		-1.,
		&result
	);
	CHECK((rtn != ES_NONE));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		1.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	rtn = rt_raster_within_distance(
		rast1, -1,
		rast2, -1,
		2.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		1.1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-10, -1)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(-7, 2)
	*/
	rt_raster_set_offsets(rast2, -10, -1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		5,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		6,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testDFullyWithin() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band1;
	rt_band band2;
	double nodata;
	int rtn;
	int result;

	/*
		rast1

		(-1, -1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(1, 1)
	*/
	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -1, -1);

	band1 = addBand(rast1, PT_8BUI, 1, 0);
	CHECK(band1);
	rt_band_set_nodata(band1, 0, NULL);
	rtn = rt_band_set_pixel(band1, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band1, 1, 1, 1, NULL);

	rt_band_get_nodata(band1, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast1, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast1, 0,
		-1.,
		&result
	);
	CHECK((rtn != ES_NONE));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		0.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		1.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_fully_within_distance(
		rast1, -1,
		rast2, -1,
		5.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|1|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		2.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|1|0|
						+-+-+
						|1|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		5.,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|1|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		5,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0, 0)
						+-+-+
						|0|0|
						+-+-+
						|0|0|
						+-+-+
								(2, 2)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		10,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(2, 0)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(4, 2)
	*/
	rt_raster_set_offsets(rast2, 2, 0);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		0,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		5.9,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rt_raster_set_scale(rast2, 0.4, 0.4);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		3,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-0.1, 0.1)
						+-+-+
						|1|1|
						+-+-+
						|1|1|
						+-+-+
								(0.9, 0.9)
	*/
	rt_raster_set_offsets(rast2, -0.1, 0.1);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		2,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	deepRelease(rast2);

	/*
		rast2

		(0, 0)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(3, 3)
	*/
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rt_band_get_nodata(band2, &nodata);
	CHECK_EQUALS(nodata, 0);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		6,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(1, 1)
	*/
	rt_raster_set_offsets(rast2, -2, -2);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		4.25,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|1|
						+-+-+-+
						|1|1|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		3.5,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|1|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		3.65,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/*
		rast2

		(-2, -2)
						+-+-+-+
						|0|1|0|
						+-+-+-+
						|1|0|0|
						+-+-+-+
						|0|0|0|
						+-+-+-+
									(1, 1)
	*/
	rtn = rt_band_set_pixel(band2, 0, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 0, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 0, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		3.6,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	/*
		rast2

		(-10, -1)
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
						|1|1|1|
						+-+-+-+
									(-7, 2)
	*/
	rt_raster_set_offsets(rast2, -10, -1);

	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 1, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		5,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result != 1));

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		11.5,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);

	/* skew tests */
	/* rast2 (skewed by -0.5, 0.5) */
	rast2 = rt_raster_new(3, 3);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);
	rt_raster_set_skews(rast2, -0.5, 0.5);

	band2 = addBand(rast2, PT_8BUI, 1, 0);
	CHECK(band2);
	rt_band_set_nodata(band2, 0, NULL);
	rtn = rt_band_set_pixel(band2, 0, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 0, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 0, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 1, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 1, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 1, 2, 3, NULL);
	rtn = rt_band_set_pixel(band2, 2, 0, 1, NULL);
	rtn = rt_band_set_pixel(band2, 2, 1, 2, NULL);
	rtn = rt_band_set_pixel(band2, 2, 2, 3, NULL);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		6.1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by -1, 1) */
	rt_raster_set_skews(rast2, -1, 1);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		7.1,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	/* rast2 (skewed by 1, -1) */
	rt_raster_set_skews(rast2, 1, -1);

	rtn = rt_raster_fully_within_distance(
		rast1, 0,
		rast2, 0,
		8,
		&result
	);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((result == 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testAlignment() {
	rt_raster rast1;
	rt_raster rast2;
	int rtn;
	int aligned;
	char *reason;

	rast1 = rt_raster_new(2, 2);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);

	rast2 = rt_raster_new(10, 10);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, NULL);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned != 0));

	rt_raster_set_scale(rast2, 0.1, 0.1);
	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, &reason);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned == 0));
	CHECK(!strcmp(reason, "The rasters have different scales on the X axis"));
	rt_raster_set_scale(rast2, 1, 1);

	rt_raster_set_skews(rast2, -0.5, 0.5);
	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, &reason);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned == 0));
	CHECK(!strcmp(reason, "The rasters have different skews on the X axis"));
	rt_raster_set_skews(rast2, 0, 0);

	rt_raster_set_offsets(rast2, 1, 1);
	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, NULL);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned != 0));

	rt_raster_set_offsets(rast2, 2, 3);
	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, NULL);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned != 0));

	rt_raster_set_offsets(rast2, 0.1, 0.1);
	rtn = rt_raster_same_alignment(rast1, rast2, &aligned, &reason);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK((aligned == 0));
	CHECK(!strcmp(reason, "The rasters (pixel corner coordinates) are not aligned"));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testFromTwoRasters() {
	rt_raster rast1;
	rt_raster rast2;
	rt_raster rast = NULL;
	int err;
	double offset[4] = {0.};

	rast1 = rt_raster_new(4, 4);
	assert(rast1);
	rt_raster_set_scale(rast1, 1, 1);
	rt_raster_set_offsets(rast1, -2, -2);

	rast2 = rt_raster_new(2, 2);
	assert(rast2);
	rt_raster_set_scale(rast2, 1, 1);

	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_FIRST,
		&rast,
		offset
	);
	CHECK(err == ES_NONE);
	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 4));
	CHECK((rt_raster_get_height(rast) == 4));
	CHECK(FLT_EQ(offset[0], 0));
	CHECK(FLT_EQ(offset[1], 0));
	CHECK(FLT_EQ(offset[2], 2));
	CHECK(FLT_EQ(offset[3], 2));
	deepRelease(rast);

	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_SECOND,
		&rast,
		offset
	);
	CHECK(err == ES_NONE);
	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 2));
	CHECK((rt_raster_get_height(rast) == 2));
	CHECK(FLT_EQ(offset[0], -2));
	CHECK(FLT_EQ(offset[1], -2));
	CHECK(FLT_EQ(offset[2], 0));
	CHECK(FLT_EQ(offset[3], 0));
	deepRelease(rast);

	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_INTERSECTION,
		&rast,
		offset
	);
	CHECK(err == ES_NONE);
	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 2));
	CHECK((rt_raster_get_height(rast) == 2));
	CHECK(FLT_EQ(offset[0], -2));
	CHECK(FLT_EQ(offset[1], -2));
	CHECK(FLT_EQ(offset[2], 0));
	CHECK(FLT_EQ(offset[3], 0));
	deepRelease(rast);

	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_UNION,
		&rast,
		offset
	);
	CHECK(err == ES_NONE);
	CHECK(rast);
	CHECK((rt_raster_get_width(rast) == 4));
	CHECK((rt_raster_get_height(rast) == 4));
	CHECK(FLT_EQ(offset[0], 0));
	CHECK(FLT_EQ(offset[1], 0));
	CHECK(FLT_EQ(offset[2], 2));
	CHECK(FLT_EQ(offset[3], 2));
	deepRelease(rast);

	rt_raster_set_scale(rast2, 1, 0.1);
	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_UNION,
		&rast,
		offset
	);
	CHECK(err != ES_NONE);
	rt_raster_set_scale(rast2, 1, 1);

	rt_raster_set_srid(rast2, 9999);
	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_UNION,
		&rast,
		offset
	);
	CHECK(err != ES_NONE);
	rt_raster_set_srid(rast2, 0);

	rt_raster_set_skews(rast2, -1, 1);
	err = rt_raster_from_two_rasters(
		rast1, rast2,
		ET_UNION,
		&rast,
		offset
	);
	CHECK(err != ES_NONE);

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testLoadOfflineBand() {
	rt_raster rast;
	rt_band band;
	const int maxX = 10;
	const int maxY = 10;
	const char *path = "../regress/loader/testraster.tif";
	int rtn;
	int x;
	int y;
	double val;

	rast = rt_raster_new(maxX, maxY);
	assert(rast);
	rt_raster_set_offsets(rast, 80, 80);

	band = rt_band_new_offline(maxX, maxY, PT_8BUI, 0, 0, 2, path);
	assert(band);
	rtn = rt_raster_add_band(rast, band, 0);
	CHECK((rtn >= 0));

	rtn = rt_band_load_offline_data(band);
	CHECK((rtn == 0));
	CHECK(band->data.offline.mem);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_get_pixel(band, x, y, &val, NULL);
 			CHECK_EQUALS(rtn, ES_NONE);
			CHECK(FLT_EQ(val, 255));
		}
	}

	/* test rt_band_check_is_nodata */
	rtdealloc(band->data.offline.mem);
	band->data.offline.mem = NULL;
	CHECK((rt_band_check_is_nodata(band) == FALSE));

	deepRelease(rast);
}

static void testCellGeoPoint() {
	rt_raster raster;
	int rtn;
	double xr, yr;
	double xw, yw;
	double gt[6] = {-128.604911499087763, 0.002424431085498, 0, 53.626968388905752, 0, -0.002424431085498};

	raster = rt_raster_new(1, 1);
	assert(raster); /* or we're out of virtual memory */
	rt_raster_set_srid(raster, 4326);
	rt_raster_set_geotransform_matrix(raster, gt);

	xr = 0;
	yr = 0;
	rtn = rt_raster_cell_to_geopoint(raster, xr, yr, &xw, &yw, NULL);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK(FLT_EQ(xw, gt[0]));
	CHECK(FLT_EQ(yw, gt[3]));

	rtn = rt_raster_geopoint_to_cell(raster, xw, yw, &xr, &yr, NULL);
	CHECK_EQUALS(rtn, ES_NONE);
	CHECK(FLT_EQ(xr, 0));
	CHECK(FLT_EQ(yr, 0));

	deepRelease(raster);
}

static void testNearestPixel() {
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
	assert(rast);

	band = addBand(rast, PT_32BUI, 1, 0);
	CHECK(band);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_set_pixel(band, x, y, 1, NULL);
			CHECK_EQUALS(rtn, ES_NONE);
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
	CHECK((rtn == 3));
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
	CHECK((rtn == 6));
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
	CHECK((rtn == 7));
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
	CHECK((rtn == 19));
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
	CHECK((rtn == 1));
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
	CHECK((rtn == 0));
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
	CHECK((rtn == 3));
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
	CHECK((rtn == 0));
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
	CHECK((rtn == 2));

	rtn = rt_pixel_set_to_array(
		npixels, rtn,
		-1, 1, 
		1, 1,
		&value,
		&nodata,
		&dimx, &dimy
	);
	rtdealloc(npixels);
	CHECK((rtn == ES_NONE));
	CHECK((dimx == 3));
	CHECK((dimy == 3));

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
	CHECK((rtn == 0));
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
	CHECK((rtn == 0));
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
	CHECK((rtn == 48));
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
	CHECK((rtn == 27));
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
	CHECK((rtn == 13));
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
	CHECK((rtn == 3));
	if (rtn)
		rtdealloc(npixels);

	/* band with no NODATA */
	band = addBand(rast, PT_32BUI, 0, 0);
	CHECK(band);

	/* 0,0 */
	rtn = rt_band_get_nearest_pixel(
		band,
		0, 0,
		0, 0,
		1,
		&npixels
	);
	CHECK((rtn == 8));
	if (rtn)
		rtdealloc(npixels);

	deepRelease(rast);
}

static void testPixelOfValue() {
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
	assert(rast);

	band = addBand(rast, PT_32BUI, 1, 0);
	CHECK(band);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_set_pixel(band, x, y, 1, NULL);
			CHECK_EQUALS(rtn, ES_NONE);
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
	CHECK((rtn == 83));
	if (rtn)
		rtdealloc(pixels);

	pixels = NULL;
	rtn = rt_band_get_pixel_of_value(
		band, FALSE,
		search0, 1,
		&pixels
	);
	CHECK((rtn == 17));
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
	CHECK((rtn == 3));
	if (rtn)
		rtdealloc(pixels);

	deepRelease(rast);
}

static void testPixelAsPolygon() {
	rt_raster rast;
	rt_band band;
	uint32_t x, y;
	int rtn;
	const int maxX = 10;
	const int maxY = 10;
	LWPOLY *poly = NULL;

	rast = rt_raster_new(maxX, maxY);
	assert(rast);

	band = addBand(rast, PT_32BUI, 1, 0);
	CHECK(band);

	for (x = 0; x < maxX; x++) {
		for (y = 0; y < maxY; y++) {
			rtn = rt_band_set_pixel(band, x, y, 1, NULL);
			CHECK_EQUALS(rtn, ES_NONE);
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
	CHECK((poly != NULL));
	lwpoly_free(poly);

	deepRelease(rast);
}

static void testRasterSurface() {
	rt_raster rast;
	rt_band band;
	const int maxX = 5;
	const int maxY = 5;
	int x, y;
	char *wkt = NULL;
	LWMPOLY *mpoly = NULL;
	int err;

	rast = rt_raster_new(maxX, maxY);
	assert(rast);

	rt_raster_set_offsets(rast, 0, 0);
	rt_raster_set_scale(rast, 1, -1);

	band = addBand(rast, PT_32BUI, 1, 0);
	CHECK(band);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++) {
			rt_band_set_pixel(band, x, y, 1, NULL);
		}
	}

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((0 0,0 -5,5 -5,5 0,0 0)))"));
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* 0,0 NODATA */
	rt_band_set_pixel(band, 0, 0, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0)))"));
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 1,1 NODATA */
	rt_band_set_pixel(band, 1, 1, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -1,1 -1)))"));
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 2,2 NODATA */
	rt_band_set_pixel(band, 2, 2, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
#if POSTGIS_GEOS_VERSION >= 33
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 -1,1 0,5 0,5 -5,4 -5,0 -5,0 -1,1 -1),(1 -1,1 -2,2 -2,2 -1,1 -1),(2 -2,2 -3,3 -3,3 -2,2 -2)))"));
#else
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))"));
#endif
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 3,3 NODATA */
	rt_band_set_pixel(band, 3, 3, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
#if POSTGIS_GEOS_VERSION >= 33
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 -1,1 0,5 0,5 -5,4 -5,0 -5,0 -1,1 -1),(1 -1,1 -2,2 -2,2 -1,1 -1),(2 -2,2 -3,3 -3,3 -2,2 -2),(3 -3,3 -4,4 -4,4 -3,3 -3)))"));
#else
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 0,1 -1,0 -1,0 -5,4 -5,5 -5,5 0,1 0),(1 -1,1 -2,2 -2,2 -3,3 -3,3 -4,4 -4,4 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))"));
#endif
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* plus 4,4 NODATA */
	rt_band_set_pixel(band, 4, 4, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((4 -4,4 -5,0 -5,0 -1,1 -1,1 -2,2 -2,2 -3,3 -3,3 -4,4 -4)),((1 -1,1 0,5 0,5 -4,4 -4,4 -3,3 -3,3 -2,2 -2,2 -1,1 -1)))"));
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	/* a whole lot more NODATA */
	rt_band_set_pixel(band, 4, 0, 0, NULL);
	rt_band_set_pixel(band, 3, 1, 0, NULL);
	rt_band_set_pixel(band, 1, 3, 0, NULL);
	rt_band_set_pixel(band, 0, 4, 0, NULL);

	err = rt_raster_surface(rast, 0, &mpoly);
	CHECK(err == ES_NONE);
	CHECK((mpoly != NULL));
	wkt = lwgeom_to_text(lwmpoly_as_lwgeom(mpoly));
	CHECK(!strcmp(wkt, "MULTIPOLYGON(((1 -4,2 -4,2 -3,3 -3,3 -4,4 -4,4 -5,3 -5,1 -5,1 -4)),((1 -4,0 -4,0 -1,1 -1,1 -2,2 -2,2 -3,1 -3,1 -4)),((3 -2,4 -2,4 -1,5 -1,5 -4,4 -4,4 -3,3 -3,3 -2)),((3 -2,2 -2,2 -1,1 -1,1 0,4 0,4 -1,3 -1,3 -2)))"));
	rtdealloc(wkt);
	lwmpoly_free(mpoly);
	mpoly = NULL;

	deepRelease(rast);
}

typedef struct _userargs_t* _userargs;
struct _userargs_t {
	uint16_t rasters;
	uint32_t rows;
	uint32_t columns;
};

/* callback for 1 raster, 0 distance, FIRST or SECOND or LAST or UNION or INTERSECTION */
static int testRasterIterator1_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 0));
		CHECK((arg->nodata[0][0][0] == 0));
	}
	/* 4,4 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 4
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 24));
		CHECK((arg->nodata[0][0][0] == 0));
	}
	/* 1,1 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK((arg->nodata[0][0][0] == 1));
	}
	/* 2,2 */
	else if (
		arg->dst_pixel[0] == 2 &&
		arg->dst_pixel[1] == 2
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 12));
		CHECK((arg->nodata[0][0][0] == 0));
	}
	/* 3,1 */
	else if (
		arg->dst_pixel[0] == 3 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 8));
		CHECK((arg->nodata[0][0][0] == 0));
	}
	/* 1,0 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 1));
		CHECK((arg->nodata[0][0][0] == 0));
	}

	return 1;
}

/* callback for 2 raster, 0 distance, UNION */
static int testRasterIterator2_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 0));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 4,4 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 4
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 24));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 118));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 1,1 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK(FLT_EQ(arg->values[1][0][0], 100));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 2,2 */
	else if (
		arg->dst_pixel[0] == 2 &&
		arg->dst_pixel[1] == 2
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 12));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 106));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 3,1 */
	else if (
		arg->dst_pixel[0] == 3 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 8));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 102));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 1,0 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 1));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 1,3 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 3
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 16));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 5,0 */
	else if (
		arg->dst_pixel[0] == 5 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK((arg->nodata[1][0][0] == 1));
	}

	return 1;
}

/* callback for 2 raster, 0 distance, INTERSECTION */
static int testRasterIterator3_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK(FLT_EQ(arg->values[1][0][0], 100));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 0,3 */
	else if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 3
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 21));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 115));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 3,0 */
	else if (
		arg->dst_pixel[0] == 3 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 9));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 103));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 3,3 */
	else if (
		arg->dst_pixel[0] == 3 &&
		arg->dst_pixel[1] == 3
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 24));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 118));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 0,2 */
	else if (
		arg->dst_pixel[0] == 3 &&
		arg->dst_pixel[1] == 3
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 16));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}

	return 1;
}

/* callback for 2 raster, 0 distance, FIRST */
static int testRasterIterator4_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 0));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 4,4 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 4
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 24));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 118));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 4,1 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 9));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 103));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 4,0 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 4));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}

	return 1;
}

/* callback for 2 raster, 0 distance, SECOND or LAST */
static int testRasterIterator5_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK(FLT_EQ(arg->values[1][0][0], 100));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 4,4 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 4
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK(FLT_EQ(arg->values[1][0][0], 124));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 4,1 */
	else if (
		arg->dst_pixel[0] == 4 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK((arg->nodata[0][0][0] == 1));

		CHECK(FLT_EQ(arg->values[1][0][0], 109));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 0,2 */
	else if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 2
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 16));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}

	return 1;
}

/* callback for 2 raster, 0 distance, CUSTOM */
static int testRasterIterator6_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 16));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 1,0 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 17));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 111));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 0,1 */
	else if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 21));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 115));
		CHECK((arg->nodata[1][0][0] == 0));
	}
	/* 1,1 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][0][0], 22));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK(FLT_EQ(arg->values[1][0][0], 116));
		CHECK((arg->nodata[1][0][0] == 0));
	}

	return 1;
}

/* callback for 2 raster, 1 distance, CUSTOM */
static int testRasterIterator7_callback(rt_iterator_arg arg, void *userarg, double *value, int *nodata) {
	_userargs _userarg = (_userargs) userarg;

	/* check that we're getting what we expect from userarg */
	CHECK((arg->rasters == _userarg->rasters));
	CHECK((arg->rows == _userarg->rows));
	CHECK((arg->columns == _userarg->columns));

	/* 0,0 */
	if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][1][1], 16));
		CHECK((arg->nodata[0][1][1] == 0));

		CHECK((arg->nodata[1][1][1] == 1));

		CHECK(FLT_EQ(arg->values[0][0][0], 10));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}
	/* 1,0 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 0
	) {
		CHECK(FLT_EQ(arg->values[0][1][1], 17));
		CHECK((arg->nodata[0][1][1] == 0));

		CHECK(FLT_EQ(arg->values[1][1][1], 111));
		CHECK((arg->nodata[1][1][1] == 0));

		CHECK(FLT_EQ(arg->values[0][2][2], 23));
		CHECK((arg->nodata[0][2][2] == 0));

		CHECK(FLT_EQ(arg->values[1][2][2], 117));
		CHECK((arg->nodata[1][2][2] == 0));
	}
	/* 0,1 */
	else if (
		arg->dst_pixel[0] == 0 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][1][1], 21));
		CHECK((arg->nodata[0][1][1] == 0));

		CHECK(FLT_EQ(arg->values[1][1][1], 115));
		CHECK((arg->nodata[1][1][1] == 0));

		CHECK((arg->nodata[0][2][0] == 1));

		CHECK((arg->nodata[1][2][0] == 1));
	}
	/* 1,1 */
	else if (
		arg->dst_pixel[0] == 1 &&
		arg->dst_pixel[1] == 1
	) {
		CHECK(FLT_EQ(arg->values[0][1][1], 22));
		CHECK((arg->nodata[0][1][1] == 0));

		CHECK(FLT_EQ(arg->values[1][1][1], 116));
		CHECK((arg->nodata[1][1][1] == 0));

		CHECK(FLT_EQ(arg->values[0][0][0], 16));
		CHECK((arg->nodata[0][0][0] == 0));

		CHECK((arg->nodata[1][0][0] == 1));
	}

	return 1;
}

static void testRasterIterator() {
	rt_raster rast1;
	rt_raster rast2;
	rt_raster rast3;

	int num = 2;

	rt_raster rtn = NULL;
	rt_band band;
	int maxX = 5;
	int maxY = 5;
	rt_iterator itrset;
	_userargs userargs;
	int noerr = 0;
	int x = 0;
	int y = 0;

	rast1 = rt_raster_new(maxX, maxY);
	assert(rast1);

	rt_raster_set_offsets(rast1, 0, 0);
	rt_raster_set_scale(rast1, 1, -1);

	band = addBand(rast1, PT_32BUI, 1, 6);
	CHECK(band);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++) {
			rt_band_set_pixel(band, x, y, x + (y * maxX), NULL);
		}
	}

	rast2 = rt_raster_new(maxX, maxY);
	assert(rast2);

	rt_raster_set_offsets(rast2, 1, -1);
	rt_raster_set_scale(rast2, 1, -1);

	band = addBand(rast2, PT_32BUI, 1, 110);
	CHECK(band);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++) {
			rt_band_set_pixel(band, x, y, (x + (y * maxX)) + 100, NULL);
		}
	}

	rast3 = rt_raster_new(2, 2);
	assert(rast3);

	rt_raster_set_offsets(rast3, 1, -3);
	rt_raster_set_scale(rast3, 1, -1);

	/* allocate user args */
	userargs = rtalloc(sizeof(struct _userargs_t));

	/* allocate itrset */
	itrset = rtalloc(sizeof(struct rt_iterator_t) * num);
	itrset[0].raster = rast1;
	itrset[0].nband = 0;
	itrset[0].nbnodata = 1;
	itrset[1].raster = rast2;
	itrset[1].nband = 0;
	itrset[1].nbnodata = 1;

	/* 1 raster, 0 distance, FIRST or SECOND or LAST or UNION or INTERSECTION */
	userargs->rasters = 1;
	userargs->rows = 1;
	userargs->columns = 1;

	noerr = rt_raster_iterator(
		itrset, 1,
		ET_INTERSECTION, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator1_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 5));
	CHECK((rt_raster_get_height(rtn) == 5));
	CHECK((rt_raster_get_x_offset(rtn) == 0));
	CHECK((rt_raster_get_y_offset(rtn) == 0));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 1 raster, 0 distance, FIRST or SECOND or LAST or UNION or INTERSECTION */
	userargs->rasters = 1;
	userargs->rows = 1;
	userargs->columns = 1;

	noerr = rt_raster_iterator(
		itrset, 1,
		ET_UNION, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator1_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 5));
	CHECK((rt_raster_get_height(rtn) == 5));
	CHECK((rt_raster_get_x_offset(rtn) == 0));
	CHECK((rt_raster_get_y_offset(rtn) == 0));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 0 distance, UNION */
	userargs->rasters = 2;
	userargs->rows = 1;
	userargs->columns = 1;

	noerr = rt_raster_iterator(
		itrset, 2,
		ET_UNION, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator2_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 6));
	CHECK((rt_raster_get_height(rtn) == 6));
	CHECK((rt_raster_get_x_offset(rtn) == 0));
	CHECK((rt_raster_get_y_offset(rtn) == 0));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 0 distance, INTERSECTION */
	noerr = rt_raster_iterator(
		itrset, 2,
		ET_INTERSECTION, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator3_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 4));
	CHECK((rt_raster_get_height(rtn) == 4));
	CHECK((rt_raster_get_x_offset(rtn) == 1));
	CHECK((rt_raster_get_y_offset(rtn) == -1));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 0 distance, FIRST */
	noerr = rt_raster_iterator(
		itrset, 2,
		ET_FIRST, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator4_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 5));
	CHECK((rt_raster_get_height(rtn) == 5));
	CHECK((rt_raster_get_x_offset(rtn) == 0));
	CHECK((rt_raster_get_y_offset(rtn) == 0));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 0 distance, LAST or SECOND */
	noerr = rt_raster_iterator(
		itrset, 2,
		ET_LAST, NULL,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator5_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 5));
	CHECK((rt_raster_get_height(rtn) == 5));
	CHECK((rt_raster_get_x_offset(rtn) == 1));
	CHECK((rt_raster_get_y_offset(rtn) == -1));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 0 distance, CUSTOM */
	noerr = rt_raster_iterator(
		itrset, 2,
		ET_CUSTOM, rast3,
		PT_32BUI,
		1, 0,
		0, 0,
		userargs,
		testRasterIterator6_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 2));
	CHECK((rt_raster_get_height(rtn) == 2));
	CHECK((rt_raster_get_x_offset(rtn) == 1));
	CHECK((rt_raster_get_y_offset(rtn) == -3));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	/* 2 raster, 1 distance, CUSTOM */
	userargs->rasters = 2;
	userargs->rows = 3;
	userargs->columns = 3;

	noerr = rt_raster_iterator(
		itrset, 2,
		ET_CUSTOM, rast3,
		PT_32BUI,
		1, 0,
		1, 1,
		userargs,
		testRasterIterator7_callback,
		&rtn
	);
	CHECK(noerr == ES_NONE);
	CHECK((rt_raster_get_width(rtn) == 2));
	CHECK((rt_raster_get_height(rtn) == 2));
	CHECK((rt_raster_get_x_offset(rtn) == 1));
	CHECK((rt_raster_get_y_offset(rtn) == -3));
	CHECK((rt_raster_get_x_scale(rtn) == 1));
	CHECK((rt_raster_get_y_scale(rtn) == -1));
	CHECK((rt_raster_get_x_skew(rtn) == 0));
	CHECK((rt_raster_get_y_skew(rtn) == 0));
	CHECK((rt_raster_get_srid(rtn) == 0));

	if (rtn != NULL) deepRelease(rtn);
	rtn = NULL;

	rtdealloc(userargs);
	rtdealloc(itrset);

	deepRelease(rast1);
	deepRelease(rast2);
	deepRelease(rast3);

	if (rtn != NULL) deepRelease(rtn);
}

static void testRasterClone() {
	rt_raster rast1;
	rt_raster rast2;
	rt_band band;

	int maxX = 5;
	int maxY = 5;
	double gt[6];

	rast1 = rt_raster_new(maxX, maxY);
	assert(rast1);

	rt_raster_set_offsets(rast1, 0, 0);
	rt_raster_set_scale(rast1, 1, -1);
	rt_raster_set_srid(rast1, 4326);

	band = addBand(rast1, PT_32BUI, 1, 6);
	CHECK(band);

	/* clone without bands */
	rast2 = rt_raster_clone(rast1, 0);
	CHECK(rast2);
	CHECK((rt_raster_get_num_bands(rast2) == 0));

	rt_raster_get_geotransform_matrix(rast2, gt);
	CHECK((rt_raster_get_srid(rast2) == 4326));
	CHECK((FLT_EQ(gt[0], 0)));
	CHECK((FLT_EQ(gt[1], 1)));
	CHECK((FLT_EQ(gt[2], 0)));
	CHECK((FLT_EQ(gt[3], 0)));
	CHECK((FLT_EQ(gt[4], 0)));
	CHECK((FLT_EQ(gt[5], -1)));

	deepRelease(rast2);

	/* clone with bands */
	rast2 = rt_raster_clone(rast1, 1);
	CHECK(rast2);
	CHECK((rt_raster_get_num_bands(rast2) == 1));

	deepRelease(rast2);
	deepRelease(rast1);
}

static void testGetPixelLine() {
	rt_raster rast;
	rt_band band;
	int maxX = 5;
	int maxY = 5;
	int x = 0;
	int y = 0;
	void *vals = NULL;
	uint16_t nvals = 0;
	int err = 0;

	rast = rt_raster_new(maxX, maxY);
	assert(rast);

	rt_raster_set_scale(rast, 1, -1);

	band = addBand(rast, PT_8BSI, 0, 0);
	CHECK(band);

	for (y = 0; y < maxY; y++) {
		for (x = 0; x < maxX; x++)
			rt_band_set_pixel(band, x, y, x + (y * maxX), NULL);
	}

	err = rt_band_get_pixel_line(band, 0, 0, maxX, &vals, &nvals);
	CHECK_EQUALS(err, ES_NONE);
	CHECK((nvals == maxX));
	CHECK((((int8_t *) vals)[3] == 3));
	rtdealloc(vals);
	
	err = rt_band_get_pixel_line(band, 4, 4, maxX, &vals, &nvals);
	CHECK_EQUALS(err, ES_NONE);
	CHECK((nvals == 1));
	CHECK((((int8_t *) vals)[0] == 24));
	rtdealloc(vals);

	err = rt_band_get_pixel_line(band, maxX, maxY, maxX, &vals, &nvals);
	CHECK((err != ES_NONE));

	deepRelease(rast);
}

int
main()
{
    rt_raster raster;
    rt_band band_1BB, band_2BUI, band_4BUI,
            band_8BSI, band_8BUI, band_16BSI, band_16BUI,
            band_32BSI, band_32BUI, band_32BF,
            band_64BF;

    raster = rt_raster_new(256, 256);
    assert(raster); /* or we're out of virtual memory */
		rt_raster_set_scale(raster, 1, 1);

	printf("Checking empty and hasnoband functions...\n");
	{ /* Check isEmpty and hasnoband */
		CHECK(!rt_raster_is_empty(raster));

		/* Create a dummy empty raster to test the opposite
		 * to the previous sentence
		 */
		rt_raster emptyraster = rt_raster_new(0, 0);
		CHECK(rt_raster_is_empty(emptyraster));
		rt_raster_destroy(emptyraster);

		/* Once we add a band to this raster, we'll check the opposite */
		CHECK(!rt_raster_has_band(raster, 1));
	}


	printf("Checking raster properties...\n");
    { /* Check scale */
        float scale;
        scale = rt_raster_get_x_scale(raster);
        CHECK_EQUALS(scale, 1);
        scale = rt_raster_get_y_scale(raster);
        CHECK_EQUALS(scale, 1);
    }

    { /* Check offsets */
        float off;

        off = rt_raster_get_x_offset(raster);
        CHECK_EQUALS(off, 0.5);

        off = rt_raster_get_y_offset(raster);
        CHECK_EQUALS(off, 0.5);

        rt_raster_set_offsets(raster, 30, 70);

        off = rt_raster_get_x_offset(raster);
        CHECK_EQUALS(off, 30);

        off = rt_raster_get_y_offset(raster);
        CHECK_EQUALS(off, 70);

        rt_raster_set_offsets(raster, 0.5, 0.5);
    }

    { /* Check skew */
        float off;

        off = rt_raster_get_x_skew(raster);
        CHECK_EQUALS(off, 0);

        off = rt_raster_get_y_skew(raster);
        CHECK_EQUALS(off, 0);

        rt_raster_set_skews(raster, 4, 5);

        off = rt_raster_get_x_skew(raster);
        CHECK_EQUALS(off, 4);

        off = rt_raster_get_y_skew(raster);
        CHECK_EQUALS(off, 5);

        rt_raster_set_skews(raster, 0, 0);
    }

    { /* Check SRID */
        int32_t srid;
        srid = rt_raster_get_srid(raster);
        CHECK_EQUALS(srid, 0);

        rt_raster_set_srid(raster, 65546);
        srid = rt_raster_get_srid(raster);
        CHECK_EQUALS(srid, 65546);
    }

    printf("Raster starts with %d bands\n",
        rt_raster_get_num_bands(raster));

    { /* Check convex hull, based on offset, scale and rotation */
        LWPOLY *convexhull;
        POINTARRAY *ring;
        POINT4D pt;

        /* will rotate the raster to see difference with the envelope */
        rt_raster_set_skews(raster, 4, 5);

        convexhull = rt_raster_get_convex_hull(raster);
        CHECK_EQUALS(convexhull->srid, rt_raster_get_srid(raster));
        CHECK_EQUALS(convexhull->nrings, 1);
        ring = convexhull->rings[0];
        CHECK(ring);
        CHECK_EQUALS(ring->npoints, 5);

        getPoint4d_p(ring, 0, &pt);
        printf("First point on convexhull ring is %g,%g\n", pt.x, pt.y);
        CHECK_EQUALS_DOUBLE(pt.x, 0.5);
        CHECK_EQUALS_DOUBLE(pt.y, 0.5);

        getPoint4d_p(ring, 1, &pt);
        printf("Second point on convexhull ring is %g,%g\n", pt.x, pt.y);
        CHECK_EQUALS_DOUBLE(pt.x, 256.5);
        CHECK_EQUALS_DOUBLE(pt.y, 1280.5);

        getPoint4d_p(ring, 2, &pt);
        printf("Third point on convexhull ring is %g,%g\n", pt.x, pt.y);
        CHECK_EQUALS_DOUBLE(pt.x, 1280.5);
        CHECK_EQUALS_DOUBLE(pt.y, 1536.5);

        getPoint4d_p(ring, 3, &pt);
        printf("Fourth point on convexhull ring is %g,%g\n", pt.x, pt.y);
        CHECK_EQUALS_DOUBLE(pt.x, 1024.5);
        CHECK_EQUALS_DOUBLE(pt.y, 256.5);

        getPoint4d_p(ring, 4, &pt);
        printf("Fifth point on convexhull ring is %g,%g\n", pt.x, pt.y);
        CHECK_EQUALS_DOUBLE(pt.x, 0.5);
        CHECK_EQUALS_DOUBLE(pt.y, 0.5);

        lwpoly_free(convexhull);

        rt_raster_set_skews(raster, 0, 0);
    }
		/* done all above */

		/* done */
		printf("Testing rt_util_gdal_configured... ");
		testGDALConfigured();
		printf("OK\n");

		/* done */
    printf("Testing rt_raster_gdal_polygonize... ");
		testGDALPolygonize();
		printf("OK\n");

		/* done */
    printf("Testing 1BB band... ");
    band_1BB = addBand(raster, PT_1BB, 0, 0);
    testBand1BB(band_1BB);
		printf("OK\n");

		/* done */
    printf("Testing 2BB band... ");
    band_2BUI = addBand(raster, PT_2BUI, 0, 0);
    testBand2BUI(band_2BUI);
		printf("OK\n");

		/* done */
    printf("Testing 4BUI band... ");
    band_4BUI = addBand(raster, PT_4BUI, 0, 0);
    testBand4BUI(band_4BUI);
		printf("OK\n");

		/* done */
    printf("Testing 8BUI band... ");
    band_8BUI = addBand(raster, PT_8BUI, 0, 0);
    testBand8BUI(band_8BUI);
		printf("OK\n");

		/* done */
    printf("Testing 8BSI band... ");
    band_8BSI = addBand(raster, PT_8BSI, 0, 0);
    testBand8BSI(band_8BSI);
		printf("OK\n");

		/* done */
    printf("Testing 16BSI band... ");
    band_16BSI = addBand(raster, PT_16BSI, 0, 0);
    testBand16BSI(band_16BSI);
		printf("OK\n");

		/* done */
    printf("Testing 16BUI band... ");
    band_16BUI = addBand(raster, PT_16BUI, 0, 0);
    testBand16BUI(band_16BUI);
		printf("OK\n");

		/* done */
    printf("Testing 32BUI band... ");
    band_32BUI = addBand(raster, PT_32BUI, 0, 0);
    testBand32BUI(band_32BUI);
		printf("OK\n");

		/* done */
    printf("Testing 32BSI band... ");
    band_32BSI = addBand(raster, PT_32BSI, 0, 0);
    testBand32BSI(band_32BSI);
		printf("OK\n");

		/* done */
    printf("Testing 32BF band... ");
    band_32BF = addBand(raster, PT_32BF, 0, 0);
    testBand32BF(band_32BF);
		printf("OK\n");

		/* done */
    printf("Testing 64BF band... ");
    band_64BF = addBand(raster, PT_64BF, 0, 0);
    testBand64BF(band_64BF);
		printf("OK\n");

		/* done */
    printf("Testing band hasnodata flag... ");
    testBandHasNoData(band_64BF);
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_from_band... ");
		testRasterFromBand();
		printf("OK\n");

		/* done */
		printf("Testing band stats... ");
		testBandStats();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_replace_band... ");
		testRasterReplaceBand();
		printf("OK\n");

		/* done */
		printf("Testing rt_band_reclass... ");
		testBandReclass();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_to_gdal... ");
		testRasterToGDAL();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_gdal_drivers... ");
		testGDALDrivers();
		printf("OK\n");

		/* done */
		printf("Testing rt_band_get_value_count... ");
		testValueCount();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_from_gdal_dataset... ");
		testGDALToRaster();
		printf("OK\n");

		/* done */
		printf("Testing rt_util_compute_skewed_extent... ");
		testComputeSkewedExtent();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_gdal_warp... ");
		testGDALWarp();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_gdal_rasterize... ");
		testGDALRasterize();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_intersects... ");
		testIntersects();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_surface... ");
		testRasterSurface();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_overlaps... ");
		testOverlaps();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_touches... ");
		testTouches();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_contains... ");
		testContains();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_contains_properly... ");
		testContainsProperly();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_covers... ");
		testCovers();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_covered_by... ");
		testCoveredBy();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_within_distance... ");
		testDWithin();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_fully_within_distance... ");
		testDFullyWithin();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_same_alignment... ");
		testAlignment();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_from_two_rasters... ");
		testFromTwoRasters();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_load_offline_band... ");
		testLoadOfflineBand();
		printf("OK\n");

		/* done */
		printf("Testing cell <-> geopoint... ");
		testCellGeoPoint();
		printf("OK\n");

		/* done */
		printf("Testing rt_band_get_nearest_pixel... ");
		testNearestPixel();
		printf("OK\n");

		/* done */
		printf("Testing rt_band_get_pixel_of_value... ");
		testPixelOfValue();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_pixel_as_polygon... ");
		testPixelAsPolygon();
		printf("OK\n");

		/* done */
		printf("Testing rt_raster_iterator... ");
		testRasterIterator();
		printf("OK\n");

		/* done */
		printf("Test rt_raster_clone... ");
		testRasterClone();
		printf("OK\n");

		/* done */
		printf("Test rt_band_get_pixel_line... ");
		testGetPixelLine();
		printf("OK\n");

    deepRelease(raster);

    return EXIT_SUCCESS;
}


void rt_init_allocators(void)
{
    rt_install_default_allocators();
}
