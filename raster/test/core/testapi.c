#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h> /* for fabs */
#include <float.h> /* for FLT_EPSILON and DBL_EPSILON */

#include "rt_api.h"
#include "check.h"

static rt_band addBand(rt_context ctx, rt_raster raster, rt_pixtype pixtype, int hasnodata, double nodataval);
static void deepRelease(rt_context ctx, rt_raster raster);
static void testBand1BB(rt_context ctx, rt_band band);
static rt_raster fillRasterToPolygonize(rt_context ctx, int hasnodata, double nodatavalue);

static rt_band
addBand(rt_context ctx, rt_raster raster, rt_pixtype pixtype, int hasnodata, double nodataval)
{
    void* mem;
    int32_t bandNum;
    size_t datasize;
    uint16_t width = rt_raster_get_width(ctx, raster);
    uint16_t height = rt_raster_get_height(ctx, raster);

    datasize = rt_pixtype_size(ctx, pixtype)*width*height;
    mem = malloc(datasize);

    rt_band band = rt_band_new_inline(ctx, width, height,
                                      pixtype, hasnodata, nodataval, mem);
    assert(band);
    bandNum = rt_raster_add_band(ctx, raster, band, 100);
    assert(bandNum>=0);
    return band;
}

static void
deepRelease(rt_context ctx, rt_raster raster)
{
    uint16_t i, nbands=rt_raster_get_num_bands(ctx, raster);
    for (i=0; i<nbands; ++i)
    {
        rt_band band = rt_raster_get_band(ctx, raster, i);
        void* mem = rt_band_get_data(ctx, band);
        if ( mem ) free(mem);
        rt_band_destroy(ctx, band);
    }
    rt_raster_destroy(ctx, raster);
}


static rt_raster
fillRasterToPolygonize(rt_context ctx, int hasnodata, double nodatavalue)
{
    /* Create raster */

    /* First test raster */
    /*
    uint16_t width = 2;
    uint16_t height = 2;
    */
    
    /* Second test raster */
    uint16_t width = 9;
    uint16_t height = 9;
    
    /* Third test raster */
    /*
    uint16_t width = 5;
    uint16_t height = 5;
    */

    rt_raster raster = rt_raster_new(ctx, width, height);
    
    /* Fill raster. Option 1: simple raster */
    /*
    rt_band band = addBand(ctx, raster, PT_32BSI, 0, 0);

    rt_band_set_pixel(ctx, band, 0, 0, 1);
    rt_band_set_pixel(ctx, band, 0, 1, 1);
    rt_band_set_pixel(ctx, band, 1, 0, 1);
    rt_band_set_pixel(ctx, band, 1, 1, 1);
    */
    

    /* Fill raster. Option 2: 9x9, 1 band */    
    rt_band band = addBand(ctx, raster, PT_32BUI, hasnodata, nodatavalue);
    
    {
        int x, y;
        for (x = 0; x < rt_band_get_width(ctx, band); ++x)
             for (y = 0; y < rt_band_get_height(ctx, band); ++y)
    			rt_band_set_pixel(ctx, band, x, y, 0);
	}
    /**/
    rt_band_set_pixel(ctx, band, 3, 1, 1);
    rt_band_set_pixel(ctx, band, 4, 1, 1);
    rt_band_set_pixel(ctx, band, 5, 1, 2);
    rt_band_set_pixel(ctx, band, 2, 2, 1);
    rt_band_set_pixel(ctx, band, 3, 2, 1);
    rt_band_set_pixel(ctx, band, 4, 2, 1);
    rt_band_set_pixel(ctx, band, 5, 2, 2);
    rt_band_set_pixel(ctx, band, 6, 2, 2);
    rt_band_set_pixel(ctx, band, 1, 3, 1);
    rt_band_set_pixel(ctx, band, 2, 3, 1);
    rt_band_set_pixel(ctx, band, 6, 3, 2);
    rt_band_set_pixel(ctx, band, 7, 3, 2);
    rt_band_set_pixel(ctx, band, 1, 4, 1);
    rt_band_set_pixel(ctx, band, 2, 4, 1);
    rt_band_set_pixel(ctx, band, 6, 4, 2);
    rt_band_set_pixel(ctx, band, 7, 4, 2);
    rt_band_set_pixel(ctx, band, 1, 5, 1);
    rt_band_set_pixel(ctx, band, 2, 5, 1);
    rt_band_set_pixel(ctx, band, 6, 5, 2);
    rt_band_set_pixel(ctx, band, 7, 5, 2);
    rt_band_set_pixel(ctx, band, 2, 6, 1);
    rt_band_set_pixel(ctx, band, 3, 6, 1);
    rt_band_set_pixel(ctx, band, 4, 6, 1);
    rt_band_set_pixel(ctx, band, 5, 6, 2);
    rt_band_set_pixel(ctx, band, 6, 6, 2);
    rt_band_set_pixel(ctx, band, 3, 7, 1);
    rt_band_set_pixel(ctx, band, 4, 7, 1);
    rt_band_set_pixel(ctx, band, 5, 7, 2);
    


    /* Fill raster. Option 3: 5x5, 1 band */
    /*
    rt_band band = addBand(ctx, raster, PT_8BUI, 1, 255);
    
    rt_band_set_pixel(ctx, band, 0, 0, 253);
    rt_band_set_pixel(ctx, band, 1, 0, 254);
    rt_band_set_pixel(ctx, band, 2, 0, 253);
    rt_band_set_pixel(ctx, band, 3, 0, 254);
    rt_band_set_pixel(ctx, band, 4, 0, 254);
    rt_band_set_pixel(ctx, band, 0, 1, 253);
    rt_band_set_pixel(ctx, band, 1, 1, 254);
    rt_band_set_pixel(ctx, band, 2, 1, 254);
    rt_band_set_pixel(ctx, band, 3, 1, 253);
    rt_band_set_pixel(ctx, band, 4, 1, 249);
    rt_band_set_pixel(ctx, band, 0, 2, 250);
    rt_band_set_pixel(ctx, band, 1, 2, 254);
    rt_band_set_pixel(ctx, band, 2, 2, 254);
    rt_band_set_pixel(ctx, band, 3, 2, 252);
    rt_band_set_pixel(ctx, band, 4, 2, 249);
    rt_band_set_pixel(ctx, band, 0, 3, 251);
    rt_band_set_pixel(ctx, band, 1, 3, 253);
    rt_band_set_pixel(ctx, band, 2, 3, 254);
    rt_band_set_pixel(ctx, band, 3, 3, 254);
    rt_band_set_pixel(ctx, band, 4, 3, 253);
    rt_band_set_pixel(ctx, band, 0, 4, 252);
    rt_band_set_pixel(ctx, band, 1, 4, 250);
    rt_band_set_pixel(ctx, band, 2, 4, 254);
    rt_band_set_pixel(ctx, band, 3, 4, 254);
    rt_band_set_pixel(ctx, band, 4, 4, 254);
    */
     
    rt_raster_add_band(ctx, raster, band, 100);
    
    return raster;
}

static void testBand1BB(rt_context ctx, rt_band band)
{
    int failure;
    double val = 0;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    //printf("1BB nodata is %g\n", rt_band_get_nodata(ctx, band));
    CHECK_EQUALS(rt_band_get_nodata(ctx, band), 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    //printf("1BB nodata is %g\n", rt_band_get_nodata(ctx, band));
    CHECK_EQUALS(rt_band_get_nodata(ctx, band), 0);

    failure = rt_band_set_nodata(ctx, band, 2);
    CHECK(failure); /* out of range value */

    failure = rt_band_set_nodata(ctx, band, 3);
    CHECK(failure); /* out of range value */

    failure = rt_band_set_pixel(ctx, band, 0, 0, 2);
    CHECK(failure); /* out of range value */

    failure = rt_band_set_pixel(ctx, band, 0, 0, 3);
    CHECK(failure); /* out of range value */

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 0);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0);

            }
        }
    }

}

static void testBand2BUI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);
    CHECK(!failure);

    failure = rt_band_set_nodata(ctx, band, 0);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);
    CHECK(!failure);

    failure = rt_band_set_nodata(ctx, band, 2);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 2);
    CHECK(!failure);

    failure = rt_band_set_nodata(ctx, band, 3);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 3);
    CHECK(!failure);

    failure = rt_band_set_nodata(ctx, band, 4); /* invalid: out of range */
    CHECK(failure);

    failure = rt_band_set_nodata(ctx, band, 5); /* invalid: out of range */
    CHECK(failure);

    failure = rt_band_set_pixel(ctx, band, 0, 0, 4); /* out of range */
    CHECK(failure);

    failure = rt_band_set_pixel(ctx, band, 0, 0, 5); /* out of range */
    CHECK(failure);


    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 2);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 2);

                failure = rt_band_set_pixel(ctx, band, x, y, 3);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 3);
            }
        }
    }

}

static void testBand4BUI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    val = rt_band_get_nodata(ctx, band);
    CHECK(!failure);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 2);
    val = rt_band_get_nodata(ctx, band);
    CHECK(!failure);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(ctx, band, 4); 
    val = rt_band_get_nodata(ctx, band);
    CHECK(!failure);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(ctx, band, 8); 
    val = rt_band_get_nodata(ctx, band);
    CHECK(!failure);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(ctx, band, 15); 
    val = rt_band_get_nodata(ctx, band);
    CHECK(!failure);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(ctx, band, 16);  /* out of value range */
    CHECK(failure);

    failure = rt_band_set_nodata(ctx, band, 17);  /* out of value range */
    CHECK(failure);

    failure = rt_band_set_pixel(ctx, band, 0, 0, 35); /* out of value range */
    CHECK(failure);


    {
        int x, y;
        
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 3);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 3);

                failure = rt_band_set_pixel(ctx, band, x, y, 7);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 7);

                failure = rt_band_set_pixel(ctx, band, x, y, 15);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 15);
            }
        }
    }

}

static void testBand8BUI(rt_context ctx, rt_band band)
{
    double val;
    int failure; 

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 2);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(ctx, band, 4); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(ctx, band, 8); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(ctx, band, 15); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(ctx, band, 31);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(ctx, band, 255);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(ctx, band, 256); /* out of value range */
    CHECK(failure);

    failure = rt_band_set_pixel(ctx, band, 0, 0, 256); /* out of value range */
    CHECK(failure);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 31);  
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 31);

                failure = rt_band_set_pixel(ctx, band, x, y, 255);  
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);
            }
        }
    }
}

static void testBand8BSI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 2);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 2);

    failure = rt_band_set_nodata(ctx, band, 4); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 4);

    failure = rt_band_set_nodata(ctx, band, 8); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 8);

    failure = rt_band_set_nodata(ctx, band, 15); 
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 15);

    failure = rt_band_set_nodata(ctx, band, 31);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(ctx, band, -127);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, -127);

    failure = rt_band_set_nodata(ctx, band, 127);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 127);

    /* out of range (-127..127) */
    failure = rt_band_set_nodata(ctx, band, -129);  
    CHECK(failure);

    /* out of range (-127..127) */
    failure = rt_band_set_nodata(ctx, band, 129);  
    CHECK(failure);

    /* out of range (-127..127) */
    failure = rt_band_set_pixel(ctx, band, 0, 0, -129);  
    CHECK(failure);

    /* out of range (-127..127) */
    failure = rt_band_set_pixel(ctx, band, 0, 0, 129);  
    CHECK(failure);


    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 31);  
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 31);

                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, -127);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, -127);

                failure = rt_band_set_pixel(ctx, band, x, y, 127);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 127);

            }
        }
    }
}

static void testBand16BUI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 31);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(ctx, band, 255);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(ctx, band, 65535);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(ctx, band, 65536); /* out of range */
    CHECK(failure);

    /* out of value range */
    failure = rt_band_set_pixel(ctx, band, 0, 0, 65536);
    CHECK(failure);

    /* out of dimensions range */
    failure = rt_band_set_pixel(ctx, band, rt_band_get_width(ctx, band), 0, 0);
    CHECK(failure);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 255);  
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(ctx, band, x, y, 65535);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 65535);
            }
        }
    }
}

static void testBand16BSI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 31);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 31);

    failure = rt_band_set_nodata(ctx, band, 255);  
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 255);

    failure = rt_band_set_nodata(ctx, band, -32767);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, -32767);

    failure = rt_band_set_nodata(ctx, band, 32767);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    //printf("set 65535 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS(val, 32767);

    /* out of range (-32767..32767) */
    failure = rt_band_set_nodata(ctx, band, -32769);
    CHECK(failure);

    /* out of range (-32767..32767) */
    failure = rt_band_set_nodata(ctx, band, 32769);
    CHECK(failure);

    /* out of range (-32767..32767) */
    failure = rt_band_set_pixel(ctx, band, 0, 0, -32769); 
    CHECK(failure);

    /* out of range (-32767..32767) */
    failure = rt_band_set_pixel(ctx, band, 0, 0, 32769); 
    CHECK(failure);

    /* out of dimensions range */
    failure = rt_band_set_pixel(ctx, band, rt_band_get_width(ctx, band), 0, 0); 
    CHECK(failure);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 255);  
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 255);

                failure = rt_band_set_pixel(ctx, band, x, y, -32767);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, -32767);

                failure = rt_band_set_pixel(ctx, band, x, y, 32767);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 32767);
            }
        }
    }
}

static void testBand32BUI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 65535);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(ctx, band, 4294967295UL);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 4294967295UL);

    /* out of range */
    failure = rt_band_set_nodata(ctx, band, 4294967296ULL);
    CHECK(failure);

    /* out of value range */
    failure = rt_band_set_pixel(ctx, band, 0, 0, 4294967296ULL);
    CHECK(failure);

    /* out of dimensions range */
    failure = rt_band_set_pixel(ctx, band, rt_band_get_width(ctx, band),
                                0, 4294967296ULL);
    CHECK(failure);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 0);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(ctx, band, x, y, 65535);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 65535);

                failure = rt_band_set_pixel(ctx, band, x, y, 4294967295UL); 
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 4294967295UL);
            }
        }
    }
}

static void testBand32BSI(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 65535);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 65535);

    failure = rt_band_set_nodata(ctx, band, 2147483647);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    /*printf("32BSI pix is %ld\n", (long int)val);*/
    CHECK_EQUALS(val, 2147483647);

    failure = rt_band_set_nodata(ctx, band, 2147483648UL);
    /* out of range */
    CHECK(failure);

    /* out of value range */
    failure = rt_band_set_pixel(ctx, band, 0, 0, 2147483648UL);  
    CHECK(failure);

    /* out of dimensions range */
    failure = rt_band_set_pixel(ctx, band, rt_band_get_width(ctx, band), 0, 0);
    CHECK(failure);


    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 0);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(ctx, band, x, y, 65535);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 65535);

                failure = rt_band_set_pixel(ctx, band, x, y, 2147483647); 
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 2147483647);
            }
        }
    }
}

static void testBand32BF(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 65535.5);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    //printf("set 65535.56 on %s band gets %g back\n", pixtypeName, val);
    CHECK_EQUALS_DOUBLE(val, 65535.5);

    failure = rt_band_set_nodata(ctx, band, 0.006); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS_DOUBLE(val, 0.0060000000521540); /* XXX: Alternatively, use CHECK_EQUALS_DOUBLE_EX */

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 0);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(ctx, band, x, y, 65535.5);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS_DOUBLE(val, 65535.5);

                failure = rt_band_set_pixel(ctx, band, x, y, 0.006); 
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS_DOUBLE(val, 0.0060000000521540);

            }
        }
    }
}

static void testBand64BF(rt_context ctx, rt_band band)
{
    double val;
    int failure;

    failure = rt_band_set_nodata(ctx, band, 1);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 1);

    failure = rt_band_set_nodata(ctx, band, 0);
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0);

    failure = rt_band_set_nodata(ctx, band, 65535.56);   
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 65535.56);

    failure = rt_band_set_nodata(ctx, band, 0.006); 
    CHECK(!failure);
    val = rt_band_get_nodata(ctx, band);
    CHECK_EQUALS(val, 0.006);

    {
        int x, y;
        for (x=0; x<rt_band_get_width(ctx, band); ++x)
        {
            for (y=0; y<rt_band_get_height(ctx, band); ++y)
            {
                failure = rt_band_set_pixel(ctx, band, x, y, 1);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 1);

                failure = rt_band_set_pixel(ctx, band, x, y, 0);
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0);

                failure = rt_band_set_pixel(ctx, band, x, y, 65535.56);   
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 65535.56);

                failure = rt_band_set_pixel(ctx, band, x, y, 0.006); 
                CHECK(!failure);
                failure = rt_band_get_pixel(ctx, band, x, y, &val);
                CHECK(!failure);
                CHECK_EQUALS(val, 0.006);

            }
        }
    }
}

static void testBandHasNoData(rt_context ctx, rt_band band)
{
    int flag;

    flag = rt_band_get_hasnodata_flag(ctx, band);
    CHECK_EQUALS(flag, 1);

    rt_band_set_hasnodata_flag(ctx, band, 0);
    flag = rt_band_get_hasnodata_flag(ctx, band);
    CHECK_EQUALS(flag, 0);

    rt_band_set_hasnodata_flag(ctx, band, 10);
    flag = rt_band_get_hasnodata_flag(ctx, band);
    CHECK_EQUALS(flag, 1);

    rt_band_set_hasnodata_flag(ctx, band, -10);
    flag = rt_band_get_hasnodata_flag(ctx, band);
    CHECK_EQUALS(flag, 1);
}

int
main()
{
    /* will use default allocators and message handlers */
    rt_context ctx;
    
    rt_raster raster;
    rt_band band_1BB, band_2BUI, band_4BUI,
            band_8BSI, band_8BUI, band_16BSI, band_16BUI,
            band_32BSI, band_32BUI, band_32BF,
            band_64BF;

    ctx = rt_context_new(0, 0, 0);
    raster = rt_raster_new(ctx, 256, 256);
    assert(raster); /* or we're out of virtual memory */
	
	printf("Checking empty and hasnoband functions...\n");
	{ /* Check isEmpty and hasnoband */
		CHECK(!rt_raster_is_empty(ctx, raster));
		
		/* Create a dummy empty raster to test the opposite
		 * to the previous sentence
		 */
		rt_raster emptyraster = rt_raster_new(ctx, 0, 0);
		CHECK(rt_raster_is_empty(ctx, emptyraster));
		rt_raster_destroy(ctx, emptyraster);
		
		/* Once we add a band to this raster, we'll check the opposite */
		CHECK(rt_raster_has_no_band(ctx, raster, 1));
	}

	
	printf("Checking raster properties...\n");
    { /* Check scale */
        float scale;
        scale = rt_raster_get_x_scale(ctx, raster);
        CHECK_EQUALS(scale, 1);
        scale = rt_raster_get_y_scale(ctx, raster);
        CHECK_EQUALS(scale, 1);
    }

    { /* Check offsets */
        float off;

        off = rt_raster_get_x_offset(ctx, raster);
        CHECK_EQUALS(off, 0.5);

        off = rt_raster_get_y_offset(ctx, raster);
        CHECK_EQUALS(off, 0.5);

        rt_raster_set_offsets(ctx, raster, 30, 70);

        off = rt_raster_get_x_offset(ctx, raster);
        CHECK_EQUALS(off, 30);

        off = rt_raster_get_y_offset(ctx, raster);
        CHECK_EQUALS(off, 70);

        rt_raster_set_offsets(ctx, raster, 0.5, 0.5);
    }

    { /* Check skew */
        float off;

        off = rt_raster_get_x_skew(ctx, raster);
        CHECK_EQUALS(off, 0);

        off = rt_raster_get_y_skew(ctx, raster);
        CHECK_EQUALS(off, 0);

        rt_raster_set_skews(ctx, raster, 4, 5);

        off = rt_raster_get_x_skew(ctx, raster);
        CHECK_EQUALS(off, 4);

        off = rt_raster_get_y_skew(ctx, raster);
        CHECK_EQUALS(off, 5);

        rt_raster_set_skews(ctx, raster, 0, 0);
    }

    { /* Check SRID */
        int32_t srid;
        srid = rt_raster_get_srid(ctx, raster);
        CHECK_EQUALS(srid, -1);

        rt_raster_set_srid(ctx, raster, 65546);
        srid = rt_raster_get_srid(ctx, raster);
        CHECK_EQUALS(srid, 65546);
    }

    printf("Raster starts with %d bands\n",
        rt_raster_get_num_bands(ctx, raster));

    { /* Check convex hull, based on offset, scale and rotation */
        LWPOLY *convexhull;
        POINTARRAY *ring;
        POINT4D pt;

        /* will rotate the raster to see difference with the envelope */
        rt_raster_set_skews(ctx, raster, 4, 5);

        convexhull = rt_raster_get_convex_hull(ctx, raster);
        CHECK_EQUALS(convexhull->srid, rt_raster_get_srid(ctx, raster));
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

        rt_raster_set_skews(ctx, raster, 0, 0);
    }

    {   /* Check ST_AsPolygon */
        printf("Testing polygonize function\n");

		/* First test: NODATA value = -1 */
        rt_raster rt = fillRasterToPolygonize(ctx, 1, -1.0);
				
		printf("*** POLYGONIZE WITH NODATA = -1.0\n");

		/* We can check rt_raster_has_no_band here too */
		CHECK(!rt_raster_has_no_band(ctx, rt, 1));

        /**
         * Need to define again, to access the struct fields
         **/
        struct rt_geomval_t {
            int srid;
            double val;
            char * geom;
        };

        typedef struct rt_geomval_t* rt_geomval;
        int nPols = 0;
        
        rt_geomval gv = (rt_geomval) rt_raster_dump_as_wktpolygons(ctx, rt, 1, &nPols);

        CHECK_EQUALS_DOUBLE(gv[0].val, 1.0);
        CHECK(!strcmp(gv[0].geom, "POLYGON ((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));

		CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
		CHECK(!strcmp(gv[1].geom, "POLYGON ((3 3,3 6,6 6,6 3,3 3))"));

        CHECK_EQUALS_DOUBLE(gv[2].val, 2.0);
        CHECK(!strcmp(gv[2].geom, "POLYGON ((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));   

		CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
		CHECK(!strcmp(gv[3].geom, "POLYGON ((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
		
        
        rt_raster_destroy(ctx, rt);

	
		/* Second test: NODATA value = 1 */
        rt = fillRasterToPolygonize(ctx, 1, 1.0);
				
		/* We can check rt_raster_has_no_band here too */
		CHECK(!rt_raster_has_no_band(ctx, rt, 1));

        nPols = 0;
        
        gv = (rt_geomval) rt_raster_dump_as_wktpolygons(ctx, rt, 1, &nPols);


		CHECK_EQUALS_DOUBLE(gv[0].val, 0.0);
		CHECK(!strcmp(gv[0].geom, "POLYGON ((3 3,3 6,6 6,6 3,3 3))"));

        CHECK_EQUALS_DOUBLE(gv[1].val, 2.0);
        CHECK(!strcmp(gv[1].geom, "POLYGON ((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));   

		CHECK_EQUALS_DOUBLE(gv[2].val, 0.0);
		CHECK(!strcmp(gv[2].geom, "POLYGON ((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
        rt_raster_destroy(ctx, rt);
 
		/* Third test: NODATA value = 2 */
        rt = fillRasterToPolygonize(ctx, 1, 2.0);
				
		/* We can check rt_raster_has_no_band here too */
		CHECK(!rt_raster_has_no_band(ctx, rt, 1));

        nPols = 0;
        
        gv = (rt_geomval) rt_raster_dump_as_wktpolygons(ctx, rt, 1, &nPols);

        CHECK_EQUALS_DOUBLE(gv[0].val, 1.0);
        CHECK(!strcmp(gv[0].geom, "POLYGON ((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));

		CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
		CHECK(!strcmp(gv[1].geom, "POLYGON ((3 3,3 6,6 6,6 3,3 3))"));

		CHECK_EQUALS_DOUBLE(gv[2].val, 0.0);
		CHECK(!strcmp(gv[2].geom, "POLYGON ((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
        rt_raster_destroy(ctx, rt);
 

		/* Fourth test: NODATA value = 0 */
        rt = fillRasterToPolygonize(ctx, 1, 0.0);
				
		/* We can check rt_raster_has_no_band here too */
		CHECK(!rt_raster_has_no_band(ctx, rt, 1));

        nPols = 0;
        
        gv = (rt_geomval) rt_raster_dump_as_wktpolygons(ctx, rt, 1, &nPols);
		
        CHECK_EQUALS_DOUBLE(gv[0].val, 1.0);
        CHECK(!strcmp(gv[0].geom, "POLYGON ((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));
        
		CHECK_EQUALS_DOUBLE(gv[1].val, 2.0);
        CHECK(!strcmp(gv[1].geom, "POLYGON ((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));   

		rt_raster_destroy(ctx, rt);
 
		/* Last test: There is no NODATA value (all values are valid) */
        rt = fillRasterToPolygonize(ctx, 0, 1.0);
		
		/* We can check rt_raster_has_no_band here too */
		CHECK(!rt_raster_has_no_band(ctx, rt, 1));

        nPols = 0;
        
        gv = (rt_geomval) rt_raster_dump_as_wktpolygons(ctx, rt, 1, &nPols);

        CHECK_EQUALS_DOUBLE(gv[0].val, 1.0);
        CHECK(!strcmp(gv[0].geom, "POLYGON ((3 1,3 2,2 2,2 3,1 3,1 6,2 6,2 7,3 7,3 8,5 8,5 6,3 6,3 3,4 3,5 3,5 1,3 1))"));

		CHECK_EQUALS_DOUBLE(gv[1].val, 0.0);
		CHECK(!strcmp(gv[1].geom, "POLYGON ((3 3,3 6,6 6,6 3,3 3))"));

        CHECK_EQUALS_DOUBLE(gv[2].val, 2.0);
        CHECK(!strcmp(gv[2].geom, "POLYGON ((5 1,5 3,6 3,6 6,5 6,5 8,6 8,6 7,7 7,7 6,8 6,8 3,7 3,7 2,6 2,6 1,5 1))"));   

		CHECK_EQUALS_DOUBLE(gv[3].val, 0.0);
		CHECK(!strcmp(gv[3].geom, "POLYGON ((0 0,0 9,9 9,9 0,0 0),(6 7,6 8,3 8,3 7,2 7,2 6,1 6,1 3,2 3,2 2,3 2,3 1,6 1,6 2,7 2,7 3,8 3,8 6,7 6,7 7,6 7))"));
		rt_raster_destroy(ctx, rt);

    }

    printf("Testing 1BB band\n");
    band_1BB = addBand(ctx, raster, PT_1BB, 0, 0);
    testBand1BB(ctx, band_1BB);

    printf("Testing 2BB band\n");
    band_2BUI = addBand(ctx, raster, PT_2BUI, 0, 0);
    testBand2BUI(ctx, band_2BUI);

    printf("Testing 4BUI band\n");
    band_4BUI = addBand(ctx, raster, PT_4BUI, 0, 0);
    testBand4BUI(ctx, band_4BUI);

    printf("Testing 8BUI band\n");
    band_8BUI = addBand(ctx, raster, PT_8BUI, 0, 0);
    testBand8BUI(ctx, band_8BUI);

    printf("Testing 8BSI band\n");
    band_8BSI = addBand(ctx, raster, PT_8BSI, 0, 0);
    testBand8BSI(ctx, band_8BSI);

    printf("Testing 16BSI band\n");
    band_16BSI = addBand(ctx, raster, PT_16BSI, 0, 0);
    testBand16BSI(ctx, band_16BSI);

    printf("Testing 16BUI band\n");
    band_16BUI = addBand(ctx, raster, PT_16BUI, 0, 0);
    testBand16BUI(ctx, band_16BUI);

    printf("Testing 32BUI band\n");
    band_32BUI = addBand(ctx, raster, PT_32BUI, 0, 0);
    testBand32BUI(ctx, band_32BUI);

    printf("Testing 32BSI band\n");
    band_32BSI = addBand(ctx, raster, PT_32BSI, 0, 0);
    testBand32BSI(ctx, band_32BSI);

    printf("Testing 32BF band\n");
    band_32BF = addBand(ctx, raster, PT_32BF, 0, 0);
    testBand32BF(ctx, band_32BF);

    printf("Testing 64BF band\n");
    band_64BF = addBand(ctx, raster, PT_64BF, 0, 0);
    testBand64BF(ctx, band_64BF);

    printf("Testing band hasnodata flag\n");
    testBandHasNoData(ctx, band_64BF);

    deepRelease(ctx, raster);
    rt_context_destroy(ctx);

    return EXIT_SUCCESS;
}

/* This is needed by liblwgeom */
void
lwgeom_init_allocators(void)
{
    lwgeom_install_default_allocators();
}
