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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h> /* for fabs */
#include <float.h> /* for FLT_EPSILON and DBL_EPSILON */

#include "rt_api.h"
#include "check.h"


int
main()
{
    /* will use default allocators and message handlers */
    rt_raster raster = NULL;
    const char *hexwkb = NULL;
    const char *out = NULL;
    uint32_t len = 0;
    int i = 0;

    /* ------------------------------------------------------ */
    /* No bands, 7x8 - little endian                          */
    /* ------------------------------------------------------ */

    hexwkb =
"01"               /* little endian (uint8 ndr) */
"0000"             /* version (uint16 0) */
"0000"             /* nBands (uint16 0) */
"000000000000F03F" /* scaleX (float64 1) */
"0000000000000040" /* scaleY (float64 2) */
"0000000000000840" /* ipX (float64 3) */
"0000000000001040" /* ipY (float64 4) */
"0000000000001440" /* skewX (float64 5) */
"0000000000001840" /* skewY (float64 6) */
"0A000000"         /* SRID (int32 10) */
"0700"             /* width (uint16 7) */
"0800"             /* height (uint16 8) */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 0);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 7);
    CHECK_EQUALS(rt_raster_get_height(raster), 8);

    out  = rt_raster_to_hexwkb(raster, &len);
/*
    printf(" in hexwkb len: %d\n", strlen(hexwkb));
    printf("out hexwkb len: %d\n", len);
    printf(" in hexwkb: %s\n", hexwkb);
    printf("out hexwkb: %s\n", out);
*/
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian...
    CHECK( ! strcmp(hexwkb, out) );
*/
    free((/*no const*/ void*)out);

    {
        void *serialized;
        rt_raster rast2;

        serialized = rt_raster_serialize(raster);
        rast2 = rt_raster_deserialize(serialized);

        rt_raster_destroy(rast2);
        free(serialized);
    }

    rt_raster_destroy(raster);

    /* ------------------------------------------------------ */
    /* No bands, 7x8 - big endian                             */
    /* ------------------------------------------------------ */

    hexwkb =
"00"               /* big endian (uint8 xdr) */
"0000"             /* version (uint16 0) */
"0000"             /* nBands (uint16 0) */
"3FF0000000000000" /* scaleX (float64 1) */
"4000000000000000" /* scaleY (float64 2) */
"4008000000000000" /* ipX (float64 3) */
"4010000000000000" /* ipY (float64 4) */
"4014000000000000" /* skewX (float64 5) */
"4018000000000000" /* skewY (float64 6) */
"0000000A"         /* SRID (int32 10) */
"0007"             /* width (uint16 7) */
"0008"             /* height (uint16 8) */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 0);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 7);
    CHECK_EQUALS(rt_raster_get_height(raster), 8);

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    printf(" in hexwkb: %s\n", hexwkb);
    printf("out hexwkb: %s\n", out);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian...
    CHECK( ! strcmp(hexwkb, out) );
*/

    rt_raster_destroy(raster);
    free((/*no const*/ void*)out);

    /* ------------------------------------------------------ */
    /* 1x1, little endian, band0(1bb)                         */
    /* ------------------------------------------------------ */

    hexwkb =
"01"               /* little endian (uint8 ndr) */
"0000"             /* version (uint16 0) */
"0100"             /* nBands (uint16 1) */
"000000000000F03F" /* scaleX (float64 1) */
"0000000000000040" /* scaleY (float64 2) */
"0000000000000840" /* ipX (float64 3) */
"0000000000001040" /* ipY (float64 4) */
"0000000000001440" /* skewX (float64 5) */
"0000000000001840" /* skewY (float64 6) */
"0A000000"         /* SRID (int32 10) */
"0100"             /* width (uint16 1) */
"0100"             /* height (uint16 1) */
"40"               /* First band type (1BB, in memory, hasnodata) */
"00"               /* nodata value (0) */
"01"               /* pix(0,0) == 1 */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 1);
    CHECK_EQUALS(rt_raster_get_height(raster), 1);
    {
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_1BB);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), 0);
        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 1);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian...
    CHECK( ! strcmp(hexwkb, out) );
*/

    rt_raster_destroy(raster);
    free((/*no const*/ void*)out);

    /* ------------------------------------------------------ */
    /* 3x2, big endian, band0(8BSI)                           */
    /* ------------------------------------------------------ */

    hexwkb =
"01"               /* little endian (uint8 ndr) */
"0000"             /* version (uint16 0) */
"0100"             /* nBands (uint16 1) */
"000000000000F03F" /* scaleX (float64 1) */
"0000000000000040" /* scaleY (float64 2) */
"0000000000000840" /* ipX (float64 3) */
"0000000000001040" /* ipY (float64 4) */
"0000000000001440" /* skewX (float64 5) */
"0000000000001840" /* skewY (float64 6) */
"0A000000"         /* SRID (int32 10) */
"0300"             /* width (uint16 3) */
"0200"             /* height (uint16 2) */
"43"               /* First band type (8BSI, in memory, hasnodata) */
"FF"               /* nodata value (-1) */
"FF"               /* pix(0,0) == -1 */
"00"               /* pix(1,0) ==  0 */
"01"               /* pix(2,0) == 1 */
"7F"               /* pix(0,1) == 127 */
"0A"               /* pix(1,1) == 10 */
"02"               /* pix(2,1) == 2 */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 3);
    CHECK_EQUALS(rt_raster_get_height(raster), 2);
    {
        double val;
        int failure;

        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_8BSI);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), -1);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, -1);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 0);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 1);

        failure = rt_band_get_pixel(band, 0, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 127);

        failure = rt_band_get_pixel(band, 1, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 10);

        failure = rt_band_get_pixel(band, 2, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 2);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian...
    CHECK( ! strcmp(hexwkb, out) );
*/

    free((/*no const*/ void*)out);

    {
        void *serialized;
        rt_raster rast2;

        serialized = rt_raster_serialize(raster);
        rast2 = rt_raster_deserialize(serialized);

        rt_raster_destroy(rast2);
        free(serialized);

    }

    rt_raster_destroy(raster);

    /* ------------------------------------------------------ */
    /* 3x2, little endian, band0(16BSI)                       */
    /* ------------------------------------------------------ */

    hexwkb =
"01"               /* little endian (uint8 ndr) */
"0000"             /* version (uint16 0) */
"0100"             /* nBands (uint16 1) */
"000000000000F03F" /* scaleX (float64 1) */
"0000000000000040" /* scaleY (float64 2) */
"0000000000000840" /* ipX (float64 3) */
"0000000000001040" /* ipY (float64 4) */
"0000000000001440" /* skewX (float64 5) */
"0000000000001840" /* skewY (float64 6) */
"0A000000"         /* SRID (int32 10) */
"0300"             /* width (uint16 3) */
"0200"             /* height (uint16 2) */
"05"               /* First band type (16BSI, in memory) */
"FFFF"               /* nodata value (-1) */
"FFFF"               /* pix(0,0) == -1 */
"0000"               /* pix(1,0) ==  0 */
"F0FF"               /* pix(2,0) == -16 */
"7F00"               /* pix(0,1) == 127 */
"0A00"               /* pix(1,1) == 10 */
"0200"               /* pix(2,1) == 2 */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 3);
    CHECK_EQUALS(rt_raster_get_height(raster), 2);
    {
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_16BSI);
        CHECK(!rt_band_is_offline(band));
        CHECK(!rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), -1);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, -1);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 0);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, -16);

        failure = rt_band_get_pixel(band, 0, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 127);

        failure = rt_band_get_pixel(band, 1, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 10);

        failure = rt_band_get_pixel(band, 2, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 2);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian
    CHECK( ! strcmp(hexwkb, out) );
*/

    rt_raster_destroy(raster);
    free((/*no const*/ void*)out);

    /* ------------------------------------------------------ */
    /* 3x2, big endian, band0(16BSI)                          */
    /* ------------------------------------------------------ */

    hexwkb =
"00"               /* big endian (uint8 xdr) */
"0000"             /* version (uint16 0) */
"0001"             /* nBands (uint16 1) */
"3FF0000000000000" /* scaleX (float64 1) */
"4000000000000000" /* scaleY (float64 2) */
"4008000000000000" /* ipX (float64 3) */
"4010000000000000" /* ipY (float64 4) */
"4014000000000000" /* skewX (float64 5) */
"4018000000000000" /* skewY (float64 6) */
"0000000A"         /* SRID (int32 10) */
"0003"             /* width (uint16 3) */
"0002"             /* height (uint16 2) */
"05"               /* First band type (16BSI, in memory) */
"FFFF"               /* nodata value (-1) */
"FFFF"               /* pix(0,0) == -1 */
"0000"               /* pix(1,0) ==  0 */
"FFF0"               /* pix(2,0) == -16 */
"007F"               /* pix(0,1) == 127 */
"000A"               /* pix(1,1) == 10 */
"0002"               /* pix(2,1) == 2 */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 3);
    CHECK_EQUALS(rt_raster_get_height(raster), 2);
    {
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_16BSI);
        CHECK(!rt_band_is_offline(band));
        CHECK(!rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), -1);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, -1);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 0);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, -16);

        failure = rt_band_get_pixel(band, 0, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 127);

        failure = rt_band_get_pixel(band, 1, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 10);

        failure = rt_band_get_pixel(band, 2, 1, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 2);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian
    CHECK( ! strcmp(hexwkb, out) );
*/

    rt_raster_destroy(raster);
    free((/*no const*/ void*)out);

    /* ------------------------------------------------------ */
    /* 3x2, bit endian, band0(16BSI ext: 3;/tmp/t.tif)        */
    /* ------------------------------------------------------ */

    hexwkb =
"00"               /* big endian (uint8 xdr) */
"0000"             /* version (uint16 0) */
"0001"             /* nBands (uint16 1) */
"3FF0000000000000" /* scaleX (float64 1) */
"4000000000000000" /* scaleY (float64 2) */
"4008000000000000" /* ipX (float64 3) */
"4010000000000000" /* ipY (float64 4) */
"4014000000000000" /* skewX (float64 5) */
"4018000000000000" /* skewY (float64 6) */
"0000000A"         /* SRID (int32 10) */
"0003"             /* width (uint16 3) */
"0002"             /* height (uint16 2) */
"C5"               /* First band type (16BSI, on disk, hasnodata) */
"FFFF"             /* nodata value (-1) */
"03"               /* ext band num == 3 */
/* ext band path == /tmp/t.tif */
"2F746D702F742E74696600"
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 1);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), 2);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 3);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 4);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 5);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 6);
    CHECK_EQUALS(rt_raster_get_srid(raster), 10);
    CHECK_EQUALS(rt_raster_get_width(raster), 3);
    CHECK_EQUALS(rt_raster_get_height(raster), 2);
    {
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_16BSI);
        CHECK(rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), -1);
        printf("ext band path: %s\n", rt_band_get_ext_path(band));
        printf("ext band  num: %u\n", rt_band_get_ext_band_num(band));
        CHECK( ! strcmp(rt_band_get_ext_path(band), "/tmp/t.tif"));
        CHECK_EQUALS(rt_band_get_ext_band_num(band), 3);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian
    CHECK( ! strcmp(hexwkb, out) );
*/

    rt_raster_destroy(raster);
    free((/*no const*/ void*)out);

    /* ------------------------------------------------------ */
    /* 1x3, little endian, band0 16BSI, nodata 1, srid -1     */
    /* ------------------------------------------------------ */

    hexwkb =
"01"               /* little endian (uint8 ndr) */
"0000"             /* version (uint16 0) */
"0100"             /* nBands (uint16 1) */
"0000000000805640" /* scaleX (float64 90.0) */
"00000000008056C0" /* scaleY (float64 -90.0) */
"000000001C992D41" /* ipX (float64 969870.0) */
"00000000E49E2341" /* ipY (float64 642930.0) */
"0000000000000000" /* skewX (float64 0) */
"0000000000000000" /* skewY (float64 0) */
"FFFFFFFF"         /* SRID (int32 -1) */
"0300"             /* width (uint16 3) */
"0100"             /* height (uint16 1) */
"45"               /* First band type (16BSI, in memory, hasnodata) */
"0100"             /* nodata value (1) */
"0100"             /* pix(0,0) == 1 */
"B401"             /* pix(1,0) == 436 */
"AF01"             /* pix(2,0) == 431 */
    ;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 1);
    CHECK_EQUALS(rt_raster_get_x_scale(raster), 90);
    CHECK_EQUALS(rt_raster_get_y_scale(raster), -90);
    CHECK_EQUALS(rt_raster_get_x_offset(raster), 969870.0);
    CHECK_EQUALS(rt_raster_get_y_offset(raster), 642930.0);
    CHECK_EQUALS(rt_raster_get_x_skew(raster), 0);
    CHECK_EQUALS(rt_raster_get_y_skew(raster), 0);
    CHECK_EQUALS(rt_raster_get_srid(raster), -1);
    CHECK_EQUALS(rt_raster_get_width(raster), 3);
    CHECK_EQUALS(rt_raster_get_height(raster), 1);
    {
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_16BSI);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), 1);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 1);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 436);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 431);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
/*
    printf(" in hexwkb len: %d\n", strlen(hexwkb));
    printf("out hexwkb len: %d\n", len);
    printf(" in hexwkb: %s\n", hexwkb);
    printf("out hexwkb: %s\n", out);
*/
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian
    CHECK( ! strcmp(hexwkb, out) );
*/
    free((/*no const*/ void*)out);

    {
        void *serialized;
        rt_raster rast2;

        serialized = rt_raster_serialize(raster);
        rast2 = rt_raster_deserialize(serialized);

        rt_raster_destroy(rast2);
        free(serialized);
    }

    rt_raster_destroy(raster);

    /* ------------------------------------------------------ */
    /* 5x5, little endian, 3 x band 8BUI (RGB),               */
    /* nodata 0, srid -1                                      */
    /* Test case completes regress/bug_test_car5.sql          */
    /* Test case repeated 4 times to mimic 4 tiles insertion  */
    /* ------------------------------------------------------ */
    for (i = 0; i < 5; ++i)
    {

    hexwkb =
"01"                /* little endian (uint8 ndr) */
"0000"              /* version (uint16 0) */
"0300"              /* nBands (uint16 3) */
"9A9999999999A93F"  /* scaleX (float64 0.050000) */
"9A9999999999A9BF"  /* scaleY (float64 -0.050000) */
"000000E02B274A41"  /* ipX (float64 3427927.750000) */
"0000000077195641"  /* ipY (float64 5793244.000000) */
"0000000000000000"  /* skewX (float64 0.000000) */
"0000000000000000"  /* skewY (float64 0.000000) */
"FFFFFFFF"          /* srid (int32 -1) */
"0500"              /* width (uint16 5) */
"0500"              /* height (uint16 5) */
"44"                /* 1st band pixel type (8BUI, in memory, hasnodata) */
"00"                /* 1st band nodata 0 */
"FDFEFDFEFEFDFEFEFDF9FAFEFEFCF9FBFDFEFEFDFCFAFEFEFE" /* 1st band pixels */
"44"                /* 2nd band pixel type (8BUI, in memory, hasnodata) */
"00"                /* 2nd band nodata 0 */
"4E627AADD16076B4F9FE6370A9F5FE59637AB0E54F58617087" /* 2nd band pixels */
"44"                /* 3rd band pixel type (8BUI, in memory, hasnodata) */
"00"                /* 3rd band nodata 0 */
"46566487A1506CA2E3FA5A6CAFFBFE4D566DA4CB3E454C5665" /* 3rd band pixels */
;

    raster = rt_raster_from_hexwkb(hexwkb, strlen(hexwkb));
    CHECK(raster);
    CHECK_EQUALS(rt_raster_get_num_bands(raster), 3);
    CHECK_EQUALS_DOUBLE(rt_raster_get_x_scale(raster), 0.05);
    CHECK_EQUALS_DOUBLE(rt_raster_get_y_scale(raster), -0.05);
    CHECK_EQUALS_DOUBLE(rt_raster_get_x_offset(raster), 3427927.75);
    CHECK_EQUALS_DOUBLE(rt_raster_get_y_offset(raster), 5793244.00);
    CHECK_EQUALS_DOUBLE(rt_raster_get_x_skew(raster), 0.0);
    CHECK_EQUALS_DOUBLE(rt_raster_get_y_skew(raster), 0.0);
    CHECK_EQUALS(rt_raster_get_srid(raster), -1);
    CHECK_EQUALS(rt_raster_get_width(raster), 5);
    CHECK_EQUALS(rt_raster_get_height(raster), 5);
    {
        /* Test 1st band */
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 0);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_8BUI);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), 0);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 253);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 254);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 253);

        failure = rt_band_get_pixel(band, 3, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 254);

        failure = rt_band_get_pixel(band, 4, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 254);
    }

    {
        /* Test 2nd band */
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 1);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_8BUI);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), 0);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 78);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 98);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 122);

        failure = rt_band_get_pixel(band, 3, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 173);

        failure = rt_band_get_pixel(band, 4, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 209);
    }

    {
        /* Test 3rd band */
        double val;
        int failure;
        rt_band band = rt_raster_get_band(raster, 2);
        CHECK(band);
        CHECK_EQUALS(rt_band_get_pixtype(band), PT_8BUI);
        CHECK(!rt_band_is_offline(band));
        CHECK(rt_band_get_hasnodata_flag(band));
        CHECK_EQUALS(rt_band_get_nodata(band), 0);

        failure = rt_band_get_pixel(band, 0, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 70);

        failure = rt_band_get_pixel(band, 1, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 86);

        failure = rt_band_get_pixel(band, 2, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 100);

        failure = rt_band_get_pixel(band, 3, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 135);

        failure = rt_band_get_pixel(band, 4, 0, &val);
        CHECK(!failure);
        CHECK_EQUALS(val, 161);
    }

    out  = rt_raster_to_hexwkb(raster, &len);
    printf(" in hexwkb len: %u\n", strlen(hexwkb));
    printf("out hexwkb len: %u\n", len);
    CHECK_EQUALS(len, strlen(hexwkb));
/* would depend on machine endian
    CHECK( ! strcmp(hexwkb, out) );
*/

    free((/*no const*/ void*)out);
    {
        void *serialized;
        rt_raster rast2;

        serialized = rt_raster_serialize(raster);
        rast2 = rt_raster_deserialize(serialized);

        rt_raster_destroy(rast2);
        free(serialized);
    }
    rt_raster_destroy(raster);

    } /* for-loop running car5 tests */

    /* ------------------------------------------------------ */
    /* TODO: New test cases                                   */
    /* ------------------------------------------------------ */

    /* new test case */

    /* ------------------------------------------------------ */
    /*  Success summary                                       */
    /* ------------------------------------------------------ */

    printf("All tests successful !\n");

    return EXIT_SUCCESS;
}

/* This is needed by liblwgeom */
void
lwgeom_init_allocators(void)
{
    lwgeom_install_default_allocators();
}

void rt_init_allocators(void)
{
    rt_install_default_allocators();
}
