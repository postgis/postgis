/*
 * $Id: rt_api.c 11541 2013-06-11 22:52:01Z dustymugs $
 *
 * WKTRaster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright (C) 2011-2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@azavea.com>
 * Copyright (C) 2009-2011 Pierre Racine <pierre.racine@sbf.ulaval.ca>
 * Copyright (C) 2009-2011 Mateusz Loskot <mateusz@loskot.net>
 * Copyright (C) 2008-2009 Sandro Santilli <strk@keybit.net>
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

#include <math.h>
#include <stdio.h>  /* for printf (default message handler) */
#include <stdarg.h> /* for va_list, va_start etc */
#include <string.h> /* for memcpy and strlen */
#include <assert.h>
#include <time.h> /* for time */
#include "rt_api.h"
#include "gdal_vrt.h"

/******************************************************************************
* Some rules for *.(c|h) files in rt_core
*
* All functions in rt_core that receive a band index parameter
*   must be 0-based
*
* Variables and functions internal for a public function should be prefixed
*   with _rti_, e.g. _rti_iterator_arg.
******************************************************************************/

/*--- Utilities -------------------------------------------------*/

static void
swap_char(uint8_t *a, uint8_t *b) {
    uint8_t c = 0;

    assert(NULL != a && NULL != b);

    c = *a;
    *a = *b;
    *b = c;
}

static void
flip_endian_16(uint8_t *d) {
    assert(NULL != d);

    swap_char(d, d + 1);
}

static void
flip_endian_32(uint8_t *d) {
    assert(NULL != d);

    swap_char(d, d + 3);
    swap_char(d + 1, d + 2);
}

static void
flip_endian_64(uint8_t *d) {
    assert(NULL != d);

    swap_char(d + 7, d);
    swap_char(d + 6, d + 1);
    swap_char(d + 5, d + 2);
    swap_char(d + 4, d + 3);
}

uint8_t
rt_util_clamp_to_1BB(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_1BBMAX);
}

uint8_t
rt_util_clamp_to_2BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_2BUIMAX);
}

uint8_t
rt_util_clamp_to_4BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), POSTGIS_RT_4BUIMAX);
}

int8_t
rt_util_clamp_to_8BSI(double value) {
    return (int8_t)fmin(fmax((value), SCHAR_MIN), SCHAR_MAX);
}

uint8_t
rt_util_clamp_to_8BUI(double value) {
    return (uint8_t)fmin(fmax((value), 0), UCHAR_MAX);
}

int16_t
rt_util_clamp_to_16BSI(double value) {
    return (int16_t)fmin(fmax((value), SHRT_MIN), SHRT_MAX);
}

uint16_t
rt_util_clamp_to_16BUI(double value) {
    return (uint16_t)fmin(fmax((value), 0), USHRT_MAX);
}

int32_t
rt_util_clamp_to_32BSI(double value) {
    return (int32_t)fmin(fmax((value), INT_MIN), INT_MAX);
}

uint32_t
rt_util_clamp_to_32BUI(double value) {
    return (uint32_t)fmin(fmax((value), 0), UINT_MAX);
}

float
rt_util_clamp_to_32F(double value) {
    return (float)fmin(fmax((value), -FLT_MAX), FLT_MAX);
}

/* quicksort */
#define SWAP(x, y) { double t; t = x; x = y; y = t; }
#define ORDER(x, y) if (x > y) SWAP(x, y)

static double pivot(double *left, double *right) {
	double l, m, r, *p;

	l = *left;
	m = *(left + (right - left) / 2);
	r = *right;

	/* order */
	ORDER(l, m);
	ORDER(l, r);
	ORDER(m, r);

	/* pivot is higher of two values */
	if (l < m) return m;
	if (m < r) return r;

	/* find pivot that isn't left */
	for (p = left + 1; p <= right; ++p) {
		if (*p != *left)
			return (*p < *left) ? *left : *p;
	}

	/* all values are same */
	return -1;
}

static double *partition(double *left, double *right, double pivot) {
	while (left <= right) {
		while (*left < pivot) ++left;
		while (*right >= pivot) --right;

		if (left < right) {
			SWAP(*left, *right);
			++left;
			--right;
		}
	}

	return left;
}

static void quicksort(double *left, double *right) {
	double p = pivot(left, right);
	double *pos;

	if (p != -1) {
		pos = partition(left, right, p);
		quicksort(left, pos - 1);
		quicksort(pos, right);
	}
}

/**
 * Convert cstring name to GDAL Resample Algorithm
 *
 * @param algname : cstring name to convert
 *
 * @return valid GDAL resampling algorithm
 */
GDALResampleAlg
rt_util_gdal_resample_alg(const char *algname) {
	assert(algname != NULL && strlen(algname) > 0);

	if (strcmp(algname, "NEARESTNEIGHBOUR") == 0)
		return GRA_NearestNeighbour;
	else if (strcmp(algname, "NEARESTNEIGHBOR") == 0)
		return GRA_NearestNeighbour;
	else if (strcmp(algname, "BILINEAR") == 0)
		return GRA_Bilinear;
	else if (strcmp(algname, "CUBICSPLINE") == 0)
		return GRA_CubicSpline;
	else if (strcmp(algname, "CUBIC") == 0)
		return GRA_Cubic;
	else if (strcmp(algname, "LANCZOS") == 0)
		return GRA_Lanczos;

	return GRA_NearestNeighbour;
}

/**
 * Convert rt_pixtype to GDALDataType
 *
 * @param pt : pixeltype to convert
 *
 * @return valid GDALDataType
 */
GDALDataType
rt_util_pixtype_to_gdal_datatype(rt_pixtype pt) {
	switch (pt) {
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BUI:
			return GDT_Byte;
		case PT_8BSI:
		case PT_16BSI:
			return GDT_Int16;
		case PT_16BUI:
			return GDT_UInt16;
		case PT_32BSI:
			return GDT_Int32;
		case PT_32BUI:
			return GDT_UInt32;
		case PT_32BF:
			return GDT_Float32;
		case PT_64BF:
			return GDT_Float64;
		default:
			return GDT_Unknown;
	}

	return GDT_Unknown;
}

/**
 * Convert GDALDataType to rt_pixtype
 *
 * @param gdt : GDAL datatype to convert
 *
 * @return valid rt_pixtype
 */
rt_pixtype
rt_util_gdal_datatype_to_pixtype(GDALDataType gdt) {
	switch (gdt) {
		case GDT_Byte:
			return PT_8BUI;
		case GDT_UInt16:
			return PT_16BUI;
		case GDT_Int16:
			return PT_16BSI;
		case GDT_UInt32:
			return PT_32BUI;
		case GDT_Int32:
			return PT_32BSI;
		case GDT_Float32:
			return PT_32BF;
		case GDT_Float64:
			return PT_64BF;
		default:
			return PT_END;
	}

	return PT_END;
}

/*
	get GDAL runtime version information
*/
const char*
rt_util_gdal_version(const char *request) {
	if (NULL == request || !strlen(request))
		return GDALVersionInfo("RELEASE_NAME");
	else
		return GDALVersionInfo(request);
}

/*
	computed extent type
*/
rt_extenttype
rt_util_extent_type(const char *name) {
	assert(name != NULL && strlen(name) > 0);

	if (strcmp(name, "UNION") == 0)
		return ET_UNION;
	else if (strcmp(name, "FIRST") == 0)
		return ET_FIRST;
	else if (strcmp(name, "SECOND") == 0)
		return ET_SECOND;
	else if (strcmp(name, "LAST") == 0)
		return ET_LAST;
	else if (strcmp(name, "CUSTOM") == 0)
		return ET_CUSTOM;
	else
		return ET_INTERSECTION;
}

/*
	convert the spatial reference string from a GDAL recognized format to either WKT or Proj4
*/
char*
rt_util_gdal_convert_sr(const char *srs, int proj4) {
	OGRSpatialReferenceH hsrs;
	char *rtn = NULL;

	assert(srs != NULL);

	hsrs = OSRNewSpatialReference(NULL);
	if (OSRSetFromUserInput(hsrs, srs) == OGRERR_NONE) {
		if (proj4)
			OSRExportToProj4(hsrs, &rtn);
		else
			OSRExportToWkt(hsrs, &rtn);
	}
	else {
		rterror("rt_util_gdal_convert_sr: Could not process the provided srs: %s", srs);
		return NULL;
	}

	OSRDestroySpatialReference(hsrs);
	if (rtn == NULL) {
		rterror("rt_util_gdal_convert_sr: Could not process the provided srs: %s", srs);
		return NULL;
	}

	return rtn;
}

/*
	is the spatial reference string supported by GDAL
*/
int
rt_util_gdal_supported_sr(const char *srs) {
	OGRSpatialReferenceH hsrs;
	OGRErr rtn = OGRERR_NONE;

	assert(srs != NULL);

	hsrs = OSRNewSpatialReference(NULL);
	rtn = OSRSetFromUserInput(hsrs, srs);
	OSRDestroySpatialReference(hsrs);

	if (rtn == OGRERR_NONE)
		return 1;
	else
		return 0;
}

/**
 * Get auth name and code
 *
 * @param authname: authority organization of code. calling function
 * is expected to free the memory allocated for value
 * @param authcode: code assigned by authority organization. calling function
 * is expected to free the memory allocated for value
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate
rt_util_gdal_sr_auth_info(GDALDatasetH hds, char **authname, char **authcode) {
	const char *srs = NULL;

	assert(authname != NULL);
	assert(authcode != NULL);

	*authname = NULL;
	*authcode = NULL;

	srs = GDALGetProjectionRef(hds);
	if (srs != NULL && srs[0] != '\0') {
		OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);

		if (OSRSetFromUserInput(hSRS, srs) == OGRERR_NONE) {
			const char* pszAuthorityName = OSRGetAuthorityName(hSRS, NULL);
			const char* pszAuthorityCode = OSRGetAuthorityCode(hSRS, NULL);

			if (pszAuthorityName != NULL && pszAuthorityCode != NULL) {
				*authname = rtalloc(sizeof(char) * (strlen(pszAuthorityName) + 1));
				*authcode = rtalloc(sizeof(char) * (strlen(pszAuthorityCode) + 1));

				if (*authname == NULL || *authcode == NULL) {
					rterror("rt_util_gdal_sr_auth_info: Could not allocate memory for auth name and code");
					if (*authname != NULL) rtdealloc(*authname);
					if (*authcode != NULL) rtdealloc(*authcode);
					OSRDestroySpatialReference(hSRS);
					return ES_ERROR;
				}

				strncpy(*authname, pszAuthorityName, strlen(pszAuthorityName) + 1);
				strncpy(*authcode, pszAuthorityCode, strlen(pszAuthorityCode) + 1);
			}
		}

		OSRDestroySpatialReference(hSRS);
	}

	return ES_NONE;
}

/*
	is GDAL configured correctly?
*/
int rt_util_gdal_configured(void) {

	/* set of EPSG codes */
	if (!rt_util_gdal_supported_sr("EPSG:4326"))
		return 0;
	if (!rt_util_gdal_supported_sr("EPSG:4269"))
		return 0;
	if (!rt_util_gdal_supported_sr("EPSG:4267"))
		return 0;
	if (!rt_util_gdal_supported_sr("EPSG:3310"))
		return 0;
	if (!rt_util_gdal_supported_sr("EPSG:2163"))
		return 0;

	return 1;
}

/*
	register all GDAL drivers
*/
void
rt_util_gdal_register_all(void) {
	static int registered = 0;

	if (registered)
		return;

	GDALAllRegister();
	registered = 1;
}

/*
	is the driver registered?
*/
int
rt_util_gdal_driver_registered(const char *drv) {
	int count = GDALGetDriverCount();
	int i = 0;
	GDALDriverH hdrv = NULL;

	if (drv == NULL || !strlen(drv) || count < 1)
		return 0;

	for (i = 0; i < count; i++) {
		hdrv = GDALGetDriver(i);
		if (hdrv == NULL) continue;

		if (strcmp(drv, GDALGetDriverShortName(hdrv)) == 0)
			return 1;
	}

	return 0;
}

void
rt_util_from_ogr_envelope(
	OGREnvelope	env,
	rt_envelope *ext
) {
	assert(ext != NULL);

	ext->MinX = env.MinX;
	ext->MaxX = env.MaxX;
	ext->MinY = env.MinY;
	ext->MaxY = env.MaxY;

	ext->UpperLeftX = env.MinX;
	ext->UpperLeftY = env.MaxY;
}

void
rt_util_to_ogr_envelope(
	rt_envelope ext,
	OGREnvelope	*env
) {
	assert(env != NULL);

	env->MinX = ext.MinX;
	env->MaxX = ext.MaxX;
	env->MinY = ext.MinY;
	env->MaxY = ext.MaxY;
}

LWPOLY *
rt_util_envelope_to_lwpoly(
	rt_envelope env
) {
	LWPOLY *npoly = NULL;
	POINTARRAY **rings = NULL;
	POINTARRAY *pts = NULL;
	POINT4D p4d;

	rings = (POINTARRAY **) rtalloc(sizeof (POINTARRAY*));
	if (!rings) {
		rterror("rt_util_envelope_to_lwpoly: Out of memory building envelope's geometry");
		return NULL;
	}
	rings[0] = ptarray_construct(0, 0, 5);
	if (!rings[0]) {
		rterror("rt_util_envelope_to_lwpoly: Out of memory building envelope's geometry ring");
		return NULL;
	}

	pts = rings[0];
	
	/* Upper-left corner (first and last points) */
	p4d.x = env.MinX;
	p4d.y = env.MaxY;
	ptarray_set_point4d(pts, 0, &p4d);
	ptarray_set_point4d(pts, 4, &p4d);

	/* Upper-right corner (we go clockwise) */
	p4d.x = env.MaxX;
	p4d.y = env.MaxY;
	ptarray_set_point4d(pts, 1, &p4d);

	/* Lower-right corner */
	p4d.x = env.MaxX;
	p4d.y = env.MinY;
	ptarray_set_point4d(pts, 2, &p4d);

	/* Lower-left corner */
	p4d.x = env.MinX;
	p4d.y = env.MinY;
	ptarray_set_point4d(pts, 3, &p4d);

	npoly = lwpoly_construct(SRID_UNKNOWN, 0, 1, rings);
	if (npoly == NULL) {
		rterror("rt_util_envelope_to_lwpoly: Could not build envelope's geometry");
		return NULL;
	}

	return npoly;
}

int
rt_util_same_geotransform_matrix(double *gt1, double *gt2) {
	int k = 0;

	if (gt1 == NULL || gt2 == NULL)
		return FALSE;

	for (k = 0; k < 6; k++) {
		if (FLT_NEQ(gt1[k], gt2[k]))
			return FALSE;
	}

	return TRUE;
}

/* coordinates in RGB and HSV are floating point values between 0 and 1 */
rt_errorstate
rt_util_rgb_to_hsv(double rgb[3], double hsv[3]) {
	int i;

	double minc;
	double maxc;

	double h = 0.;
	double s = 0.;
	double v = 0.;

	minc = rgb[0];
	maxc = rgb[0];

	/* get min and max values from RGB */
	for (i = 1; i < 3; i++) {
		if (rgb[i] > maxc)
			maxc = rgb[i];
		if (rgb[i] < minc)
			minc = rgb[i];
	}
	v = maxc;

	if (maxc != minc) {
		double diff = 0.;
		double rc = 0.;
		double gc = 0.;
		double bc = 0.;
		double junk = 0.;

		diff = maxc - minc;
		s = diff / maxc;
		rc = (maxc - rgb[0]) / diff;
		gc = (maxc - rgb[1]) / diff;
		bc = (maxc - rgb[2]) / diff;

		if (DBL_EQ(rgb[0], maxc))
			h = bc - gc;
		else if (DBL_EQ(rgb[1], maxc))
			h = 2.0 + rc - bc;
		else
			h = 4.0 + gc - rc;

		h = modf((h / 6.0), &junk);
	}

	hsv[0] = h;
	hsv[1] = s;
	hsv[2] = v;

	return ES_NONE;
}

/* coordinates in RGB and HSV are floating point values between 0 and 1 */
rt_errorstate
rt_util_hsv_to_rgb(double hsv[3], double rgb[3]) {
	double r = 0;
	double g = 0;
	double b = 0;
	double v = hsv[2];

	if (DBL_EQ(hsv[1], 0.))
		r = g = b = v;
	else {
		double i;
		double f;
		double p;
		double q;
		double t;

		int a;

		i = floor(hsv[0] * 6.);
		f = (hsv[0] * 6.0) - i;
		p = v * (1. - hsv[1]);
		q = v * (1. - hsv[1] * f);
		t = v * (1. - hsv[1] * (1. - f));

		a = (int) i;
		switch (a) {
			case 1:
				r = q;
				g = v;
				b = p;
				break;
			case 2:
				r = p;
				g = v;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = v;
				break;
			case 4:
				r = t;
				g = p;
				b = v;
				break;
			case 5:
				r = v;
				g = p;
				b = q;
				break;
			case 0:
			case 6:
			default:
				r = v;
				g = t;
				b = p;
				break;
		}
	}

	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;

	return ES_NONE;
}

/*- rt_context -------------------------------------------------------*/

/* Functions definitions */
void * init_rt_allocator(size_t size);
void * init_rt_reallocator(void * mem, size_t size);
void init_rt_deallocator(void * mem);
void init_rt_errorreporter(const char * fmt, va_list ap);
void init_rt_warnreporter(const char * fmt, va_list ap);
void init_rt_inforeporter(const char * fmt, va_list ap);

/*
 * Default allocators
 *
 * We include some default allocators that use malloc/free/realloc
 * along with stdout/stderr since this is the most common use case
 *
 */
void *
default_rt_allocator(size_t size)
{
    void *mem = malloc(size);
    return mem;
}

void *
default_rt_reallocator(void *mem, size_t size)
{
    void *ret = realloc(mem, size);
    return ret;
}

void
default_rt_deallocator(void *mem)
{
    free(mem);
}

void
default_rt_error_handler(const char *fmt, va_list ap) {

    static const char *label = "ERROR: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}

void
default_rt_warning_handler(const char *fmt, va_list ap) {

    static const char *label = "WARNING: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}

void
default_rt_info_handler(const char *fmt, va_list ap) {

    static const char *label = "INFO: ";
    char newfmt[1024] = {0};
    snprintf(newfmt, 1024, "%s%s\n", label, fmt);
    newfmt[1023] = '\0';

    vprintf(newfmt, ap);

    va_end(ap);
}

/**
 * Struct definition here
 */
struct rt_context_t {
    rt_allocator alloc;
    rt_reallocator realloc;
    rt_deallocator dealloc;
    rt_message_handler err;
    rt_message_handler warn;
    rt_message_handler info;
};

/* Static variable, to be used for all rt_core functions */
static struct rt_context_t ctx_t = {
    .alloc = init_rt_allocator,
    .realloc = init_rt_reallocator,
    .dealloc = init_rt_deallocator,
    .err = init_rt_errorreporter,
    .warn = init_rt_warnreporter,
    .info = init_rt_inforeporter
};


/**
 * This function is normally called by rt_init_allocators when no special memory
 * management is needed. Useful in raster core testing and in the (future)
 * loader, when we need to use raster core functions but we don't have
 * PostgreSQL backend behind. We must take care of memory by ourselves in those
 * situations
 */
void
rt_install_default_allocators(void)
{
    ctx_t.alloc = default_rt_allocator;
    ctx_t.realloc = default_rt_reallocator;
    ctx_t.dealloc = default_rt_deallocator;
    ctx_t.err = default_rt_error_handler;
    ctx_t.info = default_rt_info_handler;
    ctx_t.warn = default_rt_warning_handler;
}


/**
 * This function is called by rt_init_allocators when the PostgreSQL backend is
 * taking care of the memory and we want to use palloc family
 */
void
rt_set_handlers(rt_allocator allocator, rt_reallocator reallocator,
        rt_deallocator deallocator, rt_message_handler error_handler,
        rt_message_handler info_handler, rt_message_handler warning_handler) {

    ctx_t.alloc = allocator;
    ctx_t.realloc = reallocator;
    ctx_t.dealloc = deallocator;

    ctx_t.err = error_handler;
    ctx_t.info = info_handler;
    ctx_t.warn = warning_handler;
}



/**
 * Initialisation allocators
 *
 * These are used the first time any of the allocators are called to enable
 * executables/libraries that link into raster to be able to set up their own
 * allocators. This is mainly useful for older PostgreSQL versions that don't
 * have functions that are called upon startup.
 **/
void *
init_rt_allocator(size_t size)
{
    rt_init_allocators();

    return ctx_t.alloc(size);
}

void
init_rt_deallocator(void *mem)
{
    rt_init_allocators();

    ctx_t.dealloc(mem);
}


void *
init_rt_reallocator(void *mem, size_t size)
{
    rt_init_allocators();

    return ctx_t.realloc(mem, size);
}

void
init_rt_inforeporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.info)(fmt, ap);
}

void
init_rt_warnreporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.warn)(fmt, ap);
}


void
init_rt_errorreporter(const char *fmt, va_list ap)
{
    rt_init_allocators();

    (*ctx_t.err)(fmt, ap);

}



/**
 * Raster core memory management functions.
 *
 * They use the functions defined by the caller.
 */
void *
rtalloc(size_t size) {
    void * mem = ctx_t.alloc(size);
    RASTER_DEBUGF(5, "rtalloc called: %d@%p", size, mem);
    return mem;
}


void *
rtrealloc(void * mem, size_t size) {
    void * result = ctx_t.realloc(mem, size);
    RASTER_DEBUGF(5, "rtrealloc called: %d@%p", size, result);
    return result;
}

void
rtdealloc(void * mem) {
    ctx_t.dealloc(mem);
    RASTER_DEBUG(5, "rtdealloc called");
}

/**
 * Raster core error and info handlers
 *
 * Since variadic functions cannot pass their parameters directly, we need
 * wrappers for these functions to convert the arguments into a va_list
 * structure.
 */
void
rterror(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.err)(fmt, ap);

    va_end(ap);
}

void
rtinfo(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.info)(fmt, ap);

    va_end(ap);
}


void
rtwarn(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);

    /* Call the supplied function */
    (*ctx_t.warn)(fmt, ap);

    va_end(ap);
}



int
rt_util_dbl_trunc_warning(
	double initialvalue,
	int32_t checkvalint, uint32_t checkvaluint,
	float checkvalfloat, double checkvaldouble,
	rt_pixtype pixtype
) {
	int result = 0;

	switch (pixtype) {
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BSI:
		case PT_8BUI:
		case PT_16BSI:
		case PT_16BUI:
		case PT_32BSI: {
			if (fabs(checkvalint - initialvalue) >= 1) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got clamped from %f to %d",
					rt_pixtype_name(pixtype),
					initialvalue, checkvalint
				);
#endif
				result = 1;
			}
			else if (FLT_NEQ(checkvalint, initialvalue)) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got truncated from %f to %d",
					rt_pixtype_name(pixtype),
					initialvalue, checkvalint
				);
#endif
				result = 1;
			}
			break;
		}
		case PT_32BUI: {
			if (fabs(checkvaluint - initialvalue) >= 1) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got clamped from %f to %u",
					rt_pixtype_name(pixtype),
					initialvalue, checkvaluint
				);
#endif
				result = 1;
			}
			else if (FLT_NEQ(checkvaluint, initialvalue)) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got truncated from %f to %u",
					rt_pixtype_name(pixtype),
					initialvalue, checkvaluint
				);
#endif
				result = 1;
			}
			break;
		}
		case PT_32BF: {
			/*
				For float, because the initial value is a double,
				there is very often a difference between the desired value and the obtained one
			*/
			if (FLT_NEQ(checkvalfloat, initialvalue)) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got converted from %f to %f",
					rt_pixtype_name(pixtype),
					initialvalue, checkvalfloat
				);
#endif
				result = 1;
			}
			break;
		}
		case PT_64BF: {
			if (FLT_NEQ(checkvaldouble, initialvalue)) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
				rtwarn("Value set for %s band got converted from %f to %f",
					rt_pixtype_name(pixtype),
					initialvalue, checkvaldouble
				);
#endif
				result = 1;
			}
			break;
		}
		case PT_END:
			break;
	}

	return result;
}

/*--- Debug and Testing Utilities --------------------------------------------*/

#if POSTGIS_DEBUG_LEVEL > 2

static char*
d_binary_to_hex(const uint8_t * const raw, uint32_t size, uint32_t *hexsize) {
    char* hex = NULL;
    uint32_t i = 0;


    assert(NULL != raw);
    assert(NULL != hexsize);


    *hexsize = size * 2; /* hex is 2 times bytes */
    hex = (char*) rtalloc((*hexsize) + 1);
    if (!hex) {
        rterror("d_binary_to_hex: Out of memory hexifying raw binary");
        return NULL;
    }
    hex[*hexsize] = '\0'; /* Null-terminate */

    for (i = 0; i < size; ++i) {
        deparse_hex(raw[i], &(hex[2 * i]));
    }

    assert(NULL != hex);
    assert(0 == strlen(hex) % 2);
    return hex;
}

static void
d_print_binary_hex(const char* msg, const uint8_t * const raw, uint32_t size) {
    char* hex = NULL;
    uint32_t hexsize = 0;


    assert(NULL != msg);
    assert(NULL != raw);


    hex = d_binary_to_hex(raw, size, &hexsize);
    if (NULL != hex) {
        rtinfo("%s\t%s", msg, hex);
        rtdealloc(hex);
    }
}

static size_t
d_binptr_to_pos(const uint8_t * const ptr, const uint8_t * const end, size_t size) {
    assert(NULL != ptr && NULL != end);

    return (size - (end - ptr));
}

#define CHECK_BINPTR_POSITION(ptr, end, size, pos) \
    { if (pos != d_binptr_to_pos(ptr, end, size)) { \
    fprintf(stderr, "Check of binary stream pointer position failed on line %d\n", __LINE__); \
    fprintf(stderr, "\tactual = %lu, expected = %lu\n", (long unsigned)d_binptr_to_pos(ptr, end, size), (long unsigned)pos); \
    } }

#else

#define CHECK_BINPTR_POSITION(ptr, end, size, pos) ((void)0);

#endif /* ifndef POSTGIS_DEBUG_LEVEL > 3 */

/*- rt_pixeltype -----------------------------------------------------*/

int
rt_pixtype_size(rt_pixtype pixtype) {
	int pixbytes = -1;

	switch (pixtype) {
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BSI:
		case PT_8BUI:
			pixbytes = 1;
			break;
		case PT_16BSI:
		case PT_16BUI:
			pixbytes = 2;
			break;
		case PT_32BSI:
		case PT_32BUI:
		case PT_32BF:
			pixbytes = 4;
			break;
		case PT_64BF:
			pixbytes = 8;
			break;
		default:
			rterror("rt_pixtype_size: Unknown pixeltype %d", pixtype);
			pixbytes = -1;
			break;
	}

	RASTER_DEBUGF(3, "Pixel type = %s and size = %d bytes",
		rt_pixtype_name(pixtype), pixbytes);

	return pixbytes;
}

int
rt_pixtype_alignment(rt_pixtype pixtype) {
	return rt_pixtype_size(pixtype);
}

rt_pixtype
rt_pixtype_index_from_name(const char* pixname) {
	assert(pixname && strlen(pixname) > 0);

	if (strcmp(pixname, "1BB") == 0)
		return PT_1BB;
	else if (strcmp(pixname, "2BUI") == 0)
		return PT_2BUI;
	else if (strcmp(pixname, "4BUI") == 0)
		return PT_4BUI;
	else if (strcmp(pixname, "8BSI") == 0)
		return PT_8BSI;
	else if (strcmp(pixname, "8BUI") == 0)
		return PT_8BUI;
	else if (strcmp(pixname, "16BSI") == 0)
		return PT_16BSI;
	else if (strcmp(pixname, "16BUI") == 0)
		return PT_16BUI;
	else if (strcmp(pixname, "32BSI") == 0)
		return PT_32BSI;
	else if (strcmp(pixname, "32BUI") == 0)
		return PT_32BUI;
	else if (strcmp(pixname, "32BF") == 0)
		return PT_32BF;
	else if (strcmp(pixname, "64BF") == 0)
		return PT_64BF;

	return PT_END;
}

const char*
rt_pixtype_name(rt_pixtype pixtype) {
	switch (pixtype) {
		case PT_1BB:
			return "1BB";
		case PT_2BUI:
			return "2BUI";
		case PT_4BUI:
			return "4BUI";
		case PT_8BSI:
			return "8BSI";
		case PT_8BUI:
			return "8BUI";
		case PT_16BSI:
			return "16BSI";
		case PT_16BUI:
			return "16BUI";
		case PT_32BSI:
			return "32BSI";
		case PT_32BUI:
			return "32BUI";
		case PT_32BF:
			return "32BF";
		case PT_64BF:
			return "64BF";
		default:
			rterror("rt_pixtype_name: Unknown pixeltype %d", pixtype);
			return "Unknown";
	}
}

/**
 * Return minimum value possible for pixel type
 *
 * @param pixtype : the pixel type to get minimum possible value for
 *
 * @return the minimum possible value for the pixel type.
 */
double
rt_pixtype_get_min_value(rt_pixtype pixtype) {
	switch (pixtype) {
		case PT_1BB: {
			return (double) rt_util_clamp_to_1BB((double) CHAR_MIN);
		}
		case PT_2BUI: {
			return (double) rt_util_clamp_to_2BUI((double) CHAR_MIN);
		}
		case PT_4BUI: {
			return (double) rt_util_clamp_to_4BUI((double) CHAR_MIN);
		}
		case PT_8BUI: {
			return (double) rt_util_clamp_to_8BUI((double) CHAR_MIN);
		}
		case PT_8BSI: {
			return (double) rt_util_clamp_to_8BSI((double) SCHAR_MIN);
		}
		case PT_16BSI: {
			return (double) rt_util_clamp_to_16BSI((double) SHRT_MIN);
		}
		case PT_16BUI: {
			return (double) rt_util_clamp_to_16BUI((double) SHRT_MIN);
		}
		case PT_32BSI: {
			return (double) rt_util_clamp_to_32BSI((double) INT_MIN);
		}
		case PT_32BUI: {
			return (double) rt_util_clamp_to_32BUI((double) INT_MIN);
		}
		case PT_32BF: {
			return (double) -FLT_MAX;
		}
		case PT_64BF: {
			return (double) -DBL_MAX;
		}
		default: {
			rterror("rt_pixtype_get_min_value: Unknown pixeltype %d", pixtype);
			return (double) rt_util_clamp_to_8BUI((double) CHAR_MIN);
		}
	}
}

/**
 * Returns 1 if clamped values are equal, 0 if not equal, -1 if error
 *
 * @param pixtype : the pixel type to clamp the provided values
 * @param val : value to compare to reference value
 * @param refval : reference value to be compared with
 * @param isequal : non-zero if clamped values are equal, 0 otherwise
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate rt_pixtype_compare_clamped_values(
	rt_pixtype pixtype,
	double val, double refval,
	int *isequal
) {
	assert(isequal != NULL);
	*isequal = 0;

	switch (pixtype) {
		case PT_1BB:
			if (rt_util_clamp_to_1BB(val) == rt_util_clamp_to_1BB(refval))
				*isequal = 1;
			break;
		case PT_2BUI:
			if (rt_util_clamp_to_2BUI(val) == rt_util_clamp_to_2BUI(refval))
				*isequal = 1;
			break;
		case PT_4BUI:
			if (rt_util_clamp_to_4BUI(val) == rt_util_clamp_to_4BUI(refval))
				*isequal = 1;
			break;
		case PT_8BSI:
			if (rt_util_clamp_to_8BSI(val) == rt_util_clamp_to_8BSI(refval))
				*isequal = 1;
			break;
		case PT_8BUI:
			if (rt_util_clamp_to_8BUI(val) == rt_util_clamp_to_8BUI(refval))
				*isequal = 1;
			break;
		case PT_16BSI:
			if (rt_util_clamp_to_16BSI(val) == rt_util_clamp_to_16BSI(refval))
				*isequal = 1;
			break;
		case PT_16BUI:
			if (rt_util_clamp_to_16BUI(val) == rt_util_clamp_to_16BUI(refval))
				*isequal = 1;
			break;
		case PT_32BSI:
			if (rt_util_clamp_to_32BSI(val) == rt_util_clamp_to_32BSI(refval))
				*isequal = 1;
			break;
		case PT_32BUI:
			if (rt_util_clamp_to_32BUI(val) == rt_util_clamp_to_32BUI(refval))
				*isequal = 1;
			break;
		case PT_32BF:
			if (FLT_EQ(rt_util_clamp_to_32F(val), rt_util_clamp_to_32F(refval)))
				*isequal = 1;
			break;
		case PT_64BF:
			if (FLT_EQ(val, refval))
				*isequal = 1;
			break;
		default:
			rterror("rt_pixtype_compare_clamped_values: Unknown pixeltype %d", pixtype);
			return ES_ERROR;
	}

	return ES_NONE;
}

/*- rt_pixel ----------------------------------------------------------*/

/*
 * Convert an array of rt_pixel objects to two 2D arrays of value and NODATA.
 * The dimensions of the returned 2D array are [Y][X], going by row Y and
 * then column X.
 *
 * @param npixel : array of rt_pixel objects
 * @param count : number of elements in npixel
 * @param x : the column of the center pixel (0-based)
 * @param y : the line of the center pixel (0-based)
 * @param distancex : the number of pixels around the specified pixel
 * along the X axis
 * @param distancey : the number of pixels around the specified pixel
 * along the Y axis
 * @param value : pointer to pointer for 2D value array
 * @param nodata : pointer to pointer for 2D NODATA array
 * @param dimx : size of value and nodata along the X axis
 * @param dimy : size of value and nodata along the Y axis
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate rt_pixel_set_to_array(
	rt_pixel npixel, int count,
	int x, int y,
	uint16_t distancex, uint16_t distancey,
	double ***value,
	int ***nodata,
	int *dimx, int *dimy
) {
	uint32_t i;
	uint32_t j;
	uint32_t dim[2] = {0};
	double **values = NULL;
	int **nodatas = NULL;
	int zero[2] = {0};
	int _x;
	int _y;

	assert(npixel != NULL && count > 0);
	assert(value != NULL);
	assert(nodata != NULL);

	/* dimensions */
	dim[0] = distancex * 2 + 1;
	dim[1] = distancey * 2 + 1;
	RASTER_DEBUGF(4, "dimensions = %d x %d", dim[0], dim[1]);

	/* establish 2D arrays (Y axis) */
	values = rtalloc(sizeof(double *) * dim[1]);
	nodatas = rtalloc(sizeof(int *) * dim[1]);

	if (values == NULL || nodatas == NULL) {
		rterror("rt_pixel_set_to_array: Could not allocate memory for 2D array");
		return ES_ERROR;
	}

	/* initialize X axis */
	for (i = 0; i < dim[1]; i++) {
		values[i] = rtalloc(sizeof(double) * dim[0]);
		nodatas[i] = rtalloc(sizeof(int) * dim[0]);

		if (values[i] == NULL || nodatas[i] == NULL) {
			rterror("rt_pixel_set_to_array: Could not allocate memory for dimension of 2D array");

			if (values[i] == NULL) {
				for (j = 0; j < i; j++) {
					rtdealloc(values[j]);
					rtdealloc(nodatas[j]);
				}
			}
			else {
				for (j = 0; j <= i; j++) {
					rtdealloc(values[j]);
					if (j < i)
						rtdealloc(nodatas[j]);
				}
			}

			rtdealloc(values);
			rtdealloc(nodatas);
			
			return ES_ERROR;
		}

		/* set values to 0 */
		memset(values[i], 0, sizeof(double) * dim[0]);

		/* set nodatas to 1 */
		for (j = 0; j < dim[0]; j++)
			nodatas[i][j] = 1;
	}

	/* get 0,0 of grid */
	zero[0] = x - distancex;
	zero[1] = y - distancey;

	/* populate 2D arrays */
	for (i = 0; i < count; i++) {
		if (npixel[i].nodata)
			continue;

		_x = npixel[i].x - zero[0];
		_y = npixel[i].y - zero[1];

		RASTER_DEBUGF(4, "absolute x,y: %d x %d", npixel[i].x, npixel[i].y);
		RASTER_DEBUGF(4, "relative x,y: %d x %d", _x, _y);

		values[_y][_x] = npixel[i].value;
		nodatas[_y][_x] = 0;

		RASTER_DEBUGF(4, "(x, y, nodata, value) = (%d, %d, %d, %f)", _x, _y, nodatas[_y][_x], values[_y][_x]);
	}

	*value = &(*values);
	*nodata = &(*nodatas);
	if (dimx != NULL)
		*dimx = dim[0];
	if (dimy != NULL)
		*dimy = dim[1];

	return ES_NONE;
}

/*- rt_band ----------------------------------------------------------*/

/**
 * Create an in-db rt_band with no data
 *
 * @param width     : number of pixel columns
 * @param height    : number of pixel rows
 * @param pixtype   : pixel type for the band
 * @param hasnodata : indicates if the band has nodata value
 * @param nodataval : the nodata value, will be appropriately
 *                    truncated to fit the pixtype size.
 * @param data      : pointer to actual band data, required to
 *                    be aligned accordingly to
 *                    rt_pixtype_aligment(pixtype) and big enough
 *                    to hold raster width*height values.
 *                    Data will NOT be copied, ownership is left
 *                    to caller which is responsible to keep it
 *                    allocated for the whole lifetime of the returned
 *                    rt_band.
 *
 * @return an rt_band, or 0 on failure
 */
rt_band
rt_band_new_inline(
	uint16_t width, uint16_t height,
	rt_pixtype pixtype,
	uint32_t hasnodata, double nodataval,
	uint8_t* data
) {
	rt_band band = NULL;

	assert(NULL != data);

	band = rtalloc(sizeof(struct rt_band_t));
	if (band == NULL) {
		rterror("rt_band_new_inline: Out of memory allocating rt_band");
		return NULL;
	}

	RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s", band, rt_pixtype_name(pixtype));

	band->pixtype = pixtype;
	band->offline = 0;
	band->width = width;
	band->height = height;
	band->hasnodata = hasnodata ? 1 : 0;
	band->isnodata = FALSE; /* we don't know what is in data, so must be FALSE */
	band->nodataval = 0;
	band->data.mem = data;
	band->ownsdata = 0; /* we do NOT own this data!!! */
	band->raster = NULL;

	RASTER_DEBUGF(3, "Created rt_band with dimensions %d x %d", band->width, band->height);

	/* properly set nodataval as it may need to be constrained to the data type */
	if (hasnodata && rt_band_set_nodata(band, nodataval, NULL) != ES_NONE) {
		rterror("rt_band_new_inline: Could not set NODATA value");
		rt_band_destroy(band);
		return NULL;
	}

	return band;
}

/**
 * Create an out-db rt_band
 *
 * @param width     : number of pixel columns
 * @param height    : number of pixel rows
 * @param pixtype   : pixel type for the band
 * @param hasnodata : indicates if the band has nodata value
 * @param nodataval : the nodata value, will be appropriately
 *                    truncated to fit the pixtype size.
 * @param bandNum   : 0-based band number in the external file
 *                    to associate this band with.
 * @param path      : NULL-terminated path string pointing to the file
 *                    containing band data. The string will NOT be
 *                    copied, ownership is left to caller which is
 *                    responsible to keep it allocated for the whole
 *                    lifetime of the returned rt_band.
 *
 * @return an rt_band, or 0 on failure
 */
rt_band
rt_band_new_offline(
	uint16_t width, uint16_t height,
	rt_pixtype pixtype,
	uint32_t hasnodata, double nodataval,
	uint8_t bandNum, const char* path
) {
	rt_band band = NULL;
	int pathlen = 0;

	assert(NULL != path);

	band = rtalloc(sizeof(struct rt_band_t));
	if (band == NULL) {
		rterror("rt_band_new_offline: Out of memory allocating rt_band");
		return NULL;
	}

	RASTER_DEBUGF(3, "Created rt_band @ %p with pixtype %s",
		band, rt_pixtype_name(pixtype)
	); 

	band->pixtype = pixtype;
	band->offline = 1;
	band->width = width;
	band->height = height;
	band->hasnodata = hasnodata ? 1 : 0;
	band->nodataval = 0;
	band->isnodata = FALSE; /* we don't know if the offline band is NODATA */
	band->ownsdata = 0; /* offline, flag is useless as all offline data cache is owned internally */
	band->raster = NULL;

	/* properly set nodataval as it may need to be constrained to the data type */
	if (hasnodata && rt_band_set_nodata(band, nodataval, NULL) != ES_NONE) {
		rterror("rt_band_new_offline: Could not set NODATA value");
		rt_band_destroy(band);
		return NULL;
	}

	band->data.offline.bandNum = bandNum;

	/* memory for data.offline.path is managed internally */
	pathlen = strlen(path);
	band->data.offline.path = rtalloc(sizeof(char) * (pathlen + 1));
	if (band->data.offline.path == NULL) {
		rterror("rt_band_new_offline: Out of memory allocating offline path");
		rt_band_destroy(band);
		return NULL;
	}
	memcpy(band->data.offline.path, path, pathlen);
	band->data.offline.path[pathlen] = '\0';

	band->data.offline.mem = NULL;

	return band;
}

/**
 * Create a new band duplicated from source band.  Memory is allocated
 * for band path (if band is offline) or band data (if band is online).
 * The caller is responsible for freeing the memory when the returned
 * rt_band is destroyed.
 *
 * @param : the band to copy
 *
 * @return an rt_band or NULL on failure
 */
rt_band
rt_band_duplicate(rt_band band) {
	rt_band rtn = NULL;

	assert(band != NULL);

	/* offline */
	if (band->offline) {
		rtn = rt_band_new_offline(
			band->width, band->height,
			band->pixtype,
			band->hasnodata, band->nodataval,
			band->data.offline.bandNum, (const char *) band->data.offline.path 
		);
	}
	/* online */
	else {
		uint8_t *data = NULL;
		data = rtalloc(rt_pixtype_size(band->pixtype) * band->width * band->height);
		if (data == NULL) {
			rterror("rt_band_duplicate: Out of memory allocating online band data");
			return NULL;
		}
		memcpy(data, band->data.mem, rt_pixtype_size(band->pixtype) * band->width * band->height);

		rtn = rt_band_new_inline(
			band->width, band->height,
			band->pixtype,
			band->hasnodata, band->nodataval,
			data
		);
		rt_band_set_ownsdata_flag(rtn, 1); /* we DO own this data!!! */
	}

	if (rtn == NULL) {
		rterror("rt_band_duplicate: Could not copy band");
		return NULL;
	}

	return rtn;
}

int
rt_band_is_offline(rt_band band) {

    assert(NULL != band);


    return band->offline ? 1 : 0;
}

/**
 * Destroy a raster band
 *
 * @param band : the band to destroy
 */
void
rt_band_destroy(rt_band band) { 
	if (band == NULL)
		return;

	RASTER_DEBUGF(3, "Destroying rt_band @ %p", band);

	/* offline band */
	if (band->offline) {
		/* memory cache */
		if (band->data.offline.mem != NULL)
			rtdealloc(band->data.offline.mem);
		/* offline file path */
		if (band->data.offline.path != NULL)
			rtdealloc(band->data.offline.path);
	}
	/* inline band and band owns the data */
	else if (band->data.mem != NULL && band->ownsdata)
		rtdealloc(band->data.mem);

	rtdealloc(band);
}

const char*
rt_band_get_ext_path(rt_band band) {

    assert(NULL != band);


    if (!band->offline) {
        RASTER_DEBUG(3, "rt_band_get_ext_path: Band is not offline");
        return NULL;
    }
    return band->data.offline.path;
}

rt_errorstate
rt_band_get_ext_band_num(rt_band band, uint8_t *bandnum) {
	assert(NULL != band);
	assert(NULL != bandnum);

	*bandnum = 0;

	if (!band->offline) {
		RASTER_DEBUG(3, "rt_band_get_ext_band_num: Band is not offline");
		return ES_ERROR;
	}

	*bandnum = band->data.offline.bandNum;

	return ES_NONE;
}

/**
	* Get pointer to raster band data
	*
	* @param band : the band who's data to get
	*
	* @return pointer to band data or NULL if error
	*/
void *
rt_band_get_data(rt_band band) {
	assert(NULL != band);

	if (band->offline) {
		if (band->data.offline.mem != NULL)
			return band->data.offline.mem;

		if (rt_band_load_offline_data(band) != ES_NONE)
			return NULL;
		else
			return band->data.offline.mem;
	}
	else
		return band->data.mem;
}

/**
	* Load offline band's data.  Loaded data is internally owned
	* and should not be released by the caller.  Data will be
	* released when band is destroyed with rt_band_destroy().
	*
	* @param band : the band who's data to get
	*
	* @return ES_NONE if success, ES_ERROR if failure
	*/
rt_errorstate
rt_band_load_offline_data(rt_band band) {
	GDALDatasetH hdsSrc = NULL;
	int nband = 0;
	VRTDatasetH hdsDst = NULL;
	VRTSourcedRasterBandH hbandDst = NULL;
	double gt[6] = {0.};
	double ogt[6] = {0};
	double offset[2] = {0};

	rt_raster _rast = NULL;
	rt_band _band = NULL;
	int aligned = 0;
	int err = ES_NONE;

	assert(band != NULL);
	assert(band->raster != NULL);

	if (!band->offline) {
		rterror("rt_band_load_offline_data: Band is not offline");
		return ES_ERROR;
	}
	else if (!strlen(band->data.offline.path)) {
		rterror("rt_band_load_offline_data: Offline band does not a have a specified file");
		return ES_ERROR;
	}

	rt_util_gdal_register_all();
	hdsSrc = GDALOpenShared(band->data.offline.path, GA_ReadOnly);
	if (hdsSrc == NULL) {
		rterror("rt_band_load_offline_data: Cannot open offline raster: %s", band->data.offline.path);
		return ES_ERROR;
	}

	/* # of bands */
	nband = GDALGetRasterCount(hdsSrc);
	if (!nband) {
		rterror("rt_band_load_offline_data: No bands found in offline raster: %s", band->data.offline.path);
		GDALClose(hdsSrc);
		return ES_ERROR;
	}
	/* bandNum is 0-based */
	else if (band->data.offline.bandNum + 1 > nband) {
		rterror("rt_band_load_offline_data: Specified band %d not found in offline raster: %s", band->data.offline.bandNum, band->data.offline.path);
		GDALClose(hdsSrc);
		return ES_ERROR;
	}

	/* get raster's geotransform */
	rt_raster_get_geotransform_matrix(band->raster, gt);
	RASTER_DEBUGF(3, "Raster geotransform (%f, %f, %f, %f, %f, %f)",
		gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);

	/* get offline raster's geotransform */
	if (GDALGetGeoTransform(hdsSrc, ogt) != CE_None) {
		RASTER_DEBUG(4, "Using default geotransform matrix (0, 1, 0, 0, 0, -1)");
		ogt[0] = 0;
		ogt[1] = 1;
		ogt[2] = 0;
		ogt[3] = 0;
		ogt[4] = 0;
		ogt[5] = -1;
	}
	RASTER_DEBUGF(3, "Offline geotransform (%f, %f, %f, %f, %f, %f)",
		ogt[0], ogt[1], ogt[2], ogt[3], ogt[4], ogt[5]);

	/* are rasters aligned? */
	_rast = rt_raster_new(1, 1);
	rt_raster_set_geotransform_matrix(_rast, ogt);
	rt_raster_set_srid(_rast, band->raster->srid);
	err = rt_raster_same_alignment(band->raster, _rast, &aligned, NULL);
	rt_raster_destroy(_rast);

	if (err != ES_NONE) {
		rterror("rt_band_load_offline_data: Could not test alignment of in-db representation of out-db raster");
		GDALClose(hdsSrc);
		return ES_ERROR;
	}
	else if (!aligned) {
		rtwarn("The in-db representation of the out-db raster is not aligned. Band data may be incorrect");
	}

	/* get offsets */
	rt_raster_geopoint_to_cell(
		band->raster,
		ogt[0], ogt[3],
		&(offset[0]), &(offset[1]),
		NULL
	);

	RASTER_DEBUGF(4, "offsets: (%f, %f)", offset[0], offset[1]);

	/* create VRT dataset */
	hdsDst = VRTCreate(band->width, band->height);
	GDALSetGeoTransform(hdsDst, gt);
	/*
	GDALSetDescription(hdsDst, "/tmp/offline.vrt");
	*/

	/* add band as simple sources */
	GDALAddBand(hdsDst, rt_util_pixtype_to_gdal_datatype(band->pixtype), NULL);
	hbandDst = (VRTSourcedRasterBandH) GDALGetRasterBand(hdsDst, 1);

	if (band->hasnodata)
		GDALSetRasterNoDataValue(hbandDst, band->nodataval);

	VRTAddSimpleSource(
		hbandDst, GDALGetRasterBand(hdsSrc, band->data.offline.bandNum + 1),
		abs(offset[0]), abs(offset[1]),
		band->width, band->height,
		0, 0,
		band->width, band->height,
		"near", VRT_NODATA_UNSET
	);

	/* make sure VRT reflects all changes */
	VRTFlushCache(hdsDst);

	/* convert VRT dataset to rt_raster */
	_rast = rt_raster_from_gdal_dataset(hdsDst);

	GDALClose(hdsDst);
	/* XXX: need to find a way to clean up the GDALOpenShared datasets at end of transaction */
	/* GDALClose(hdsSrc); */

	if (_rast == NULL) {
		rterror("rt_band_load_offline_data: Cannot load data from offline raster: %s", band->data.offline.path);
		return ES_ERROR;
	}

	_band = rt_raster_get_band(_rast, 0);
	if (_band == NULL) {
		rterror("rt_band_load_offline_data: Cannot load data from offline raster: %s", band->data.offline.path);
		rt_raster_destroy(_rast);
		return ES_ERROR;
	}

	/* band->data.offline.mem not NULL, free first */
	if (band->data.offline.mem != NULL) {
		rtdealloc(band->data.offline.mem);
		band->data.offline.mem = NULL;
	}

	band->data.offline.mem = _band->data.mem;

	rtdealloc(_band); /* cannot use rt_band_destroy */
	rt_raster_destroy(_rast);

	return ES_NONE;
}

rt_pixtype
rt_band_get_pixtype(rt_band band) {

    assert(NULL != band);


    return band->pixtype;
}

uint16_t
rt_band_get_width(rt_band band) {

    assert(NULL != band);


    return band->width;
}

uint16_t
rt_band_get_height(rt_band band) {

    assert(NULL != band);


    return band->height;
}

/* Get ownsdata flag */
int
rt_band_get_ownsdata_flag(rt_band band) {
	assert(NULL != band);

	return band->ownsdata ? 1 : 0;
}

/* set ownsdata flag */
void
rt_band_set_ownsdata_flag(rt_band band, int flag) {
	assert(NULL != band);

	band->ownsdata = flag ? 1 : 0;
}

#ifdef OPTIMIZE_SPACE

/*
 * Set given number of bits of the given byte,
 * starting from given bitOffset (from the first)
 * to the given value.
 *
 * Examples:
 *   char ch;
 *   ch=0; setBits(&ch, 1, 1, 0) -> ch==8
 *   ch=0; setBits(&ch, 3, 2, 1) -> ch==96 (0x60)
 *
 * Note that number of bits set must be <= 8-bitOffset
 *
 */
static void
setBits(char* ch, double val, int bits, int bitOffset) {
    char mask = 0xFF >> (8 - bits);
    char ival = val;


		assert(ch != NULL);
    assert(8 - bitOffset >= bits);

    RASTER_DEBUGF(4, "ival:%d bits:%d mask:%hhx bitoffset:%d\n",
            ival, bits, mask, bitOffset);

    /* clear all but significant bits from ival */
    ival &= mask;
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
    if (ival != val) {
        rtwarn("Pixel value for %d-bits band got truncated"
                " from %g to %hhu", bits, val, ival);
    }
#endif /* POSTGIS_RASTER_WARN_ON_TRUNCATION */

    RASTER_DEBUGF(4, " cleared ival:%hhx\n", ival);


    /* Shift ival so the significant bits start at
     * the first bit */
    ival <<= (8 - bitOffset - bits);

    RASTER_DEBUGF(4, " ival shifted:%hhx\n", ival);
    RASTER_DEBUGF(4, "  ch:%hhx\n", *ch);

    /* clear first bits of target */
    *ch &= ~(mask << (8 - bits - bitOffset));

    RASTER_DEBUGF(4, "  ch cleared:%hhx\n", *ch);

    /* Set the first bit of target */
    *ch |= ival;

    RASTER_DEBUGF(4, "  ch ored:%hhx\n", *ch);

}
#endif /* OPTIMIZE_SPACE */

int
rt_band_get_hasnodata_flag(rt_band band) {
	assert(NULL != band);

	return band->hasnodata ? 1 : 0;
}

void
rt_band_set_hasnodata_flag(rt_band band, int flag) {

    assert(NULL != band);

    band->hasnodata = (flag) ? 1 : 0;

		/* isnodata depends on hasnodata */
		if (!band->hasnodata && band->isnodata) {
			RASTER_DEBUG(3, "Setting isnodata to FALSE as band no longer has NODATA");
			band->isnodata = 0;
		}
}

rt_errorstate
rt_band_set_isnodata_flag(rt_band band, int flag) {
	assert(NULL != band);

	if (!band->hasnodata) {
		/* silently permit setting isnodata flag to FALSE */
		if (!flag)
			band->isnodata = 0;
		else {
			rterror("rt_band_set_isnodata_flag: Cannot set isnodata flag as band has no NODATA");
			return ES_ERROR;
		}
	}
	else 
		band->isnodata = (flag) ? 1 : 0;

	return ES_NONE;
}

int
rt_band_get_isnodata_flag(rt_band band) {
	assert(NULL != band);

	if (band->hasnodata)
		return band->isnodata ? 1 : 0;
	else
		return 0;
}

/**
 * Set nodata value
 *
 * @param band : the band to set nodata value to
 * @param val : the nodata value
 * @param converted : if non-zero, value was truncated/clamped/coverted
 *
 * @return ES_NONE or ES_ERROR
 */
rt_errorstate
rt_band_set_nodata(rt_band band, double val, int *converted) {
	rt_pixtype pixtype = PT_END;
	int32_t checkvalint = 0;
	uint32_t checkvaluint = 0;
	float checkvalfloat = 0;
	double checkvaldouble = 0;

	assert(NULL != band);

	if (converted != NULL)
		*converted = 0;

	pixtype = band->pixtype;

	RASTER_DEBUGF(3, "rt_band_set_nodata: setting nodata value %g with band type %s", val, rt_pixtype_name(pixtype));

	/* return -1 on out of range */
	switch (pixtype) {
		case PT_1BB: {
			band->nodataval = rt_util_clamp_to_1BB(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_2BUI: {
			band->nodataval = rt_util_clamp_to_2BUI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_4BUI: {
			band->nodataval = rt_util_clamp_to_4BUI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_8BSI: {
			band->nodataval = rt_util_clamp_to_8BSI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_8BUI: {
			band->nodataval = rt_util_clamp_to_8BUI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_16BSI: {
			band->nodataval = rt_util_clamp_to_16BSI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_16BUI: {
			band->nodataval = rt_util_clamp_to_16BUI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_32BSI: {
			band->nodataval = rt_util_clamp_to_32BSI(val);
			checkvalint = band->nodataval;
			break;
		}
		case PT_32BUI: {
			band->nodataval = rt_util_clamp_to_32BUI(val);
			checkvaluint = band->nodataval;
			break;
		}
		case PT_32BF: {
			band->nodataval = rt_util_clamp_to_32F(val);
			checkvalfloat = band->nodataval;
			break;
		}
		case PT_64BF: {
			band->nodataval = val;
			checkvaldouble = band->nodataval;
			break;
		}
		default: {
			rterror("rt_band_set_nodata: Unknown pixeltype %d", pixtype);
			band->hasnodata = 0;
			return ES_ERROR;
		}
	}

	RASTER_DEBUGF(3, "rt_band_set_nodata: band->hasnodata = %d", band->hasnodata);
	RASTER_DEBUGF(3, "rt_band_set_nodata: band->nodataval = %f", band->nodataval); 
	/* the nodata value was just set, so this band has NODATA */
	band->hasnodata = 1;

	/* also set isnodata flag to false */
	band->isnodata = 0; 

	if (rt_util_dbl_trunc_warning(
		val,
		checkvalint, checkvaluint,
		checkvalfloat, checkvaldouble,
		pixtype
	) && converted != NULL) {
		*converted = 1;
	}

	return ES_NONE;
}

/**
 * Set values of multiple pixels.  Unlike rt_band_set_pixel,
 * values in vals are expected to be of the band's pixel type
 * as this function uses memcpy.
 *
 * It is important to be careful when using this function as
 * the number of values being set may exceed a pixel "row".
 * Remember that the band values are stored in a stream (1-D array)
 * regardless of what the raster's width and height might be.
 * So, setting a number of values may cross multiple pixel "rows".
 *
 * @param band : the band to set value to
 * @param x : X coordinate (0-based)
 * @param y : Y coordinate (0-based)
 * @param vals : the pixel values to apply
 * @param len : # of elements in vals
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate
rt_band_set_pixel_line(
	rt_band band,
	int x, int y,
	void *vals, uint32_t len
) {
	rt_pixtype pixtype = PT_END;
	int size = 0;
	uint8_t *data = NULL;
	uint32_t offset = 0;

	assert(NULL != band);
	assert(vals != NULL && len > 0);

	RASTER_DEBUGF(3, "length of values = %d", len);

	if (band->offline) {
		rterror("rt_band_set_pixel_line not implemented yet for OFFDB bands");
		return ES_ERROR;
	}

	pixtype = band->pixtype;
	size = rt_pixtype_size(pixtype);

	if (
		x < 0 || x >= band->width ||
		y < 0 || y >= band->height
	) {
		rterror("rt_band_set_pixel_line: Coordinates out of range (%d, %d) vs (%d, %d)", x, y, band->width, band->height);
		return ES_ERROR;
	}

	data = rt_band_get_data(band);
	offset = x + (y * band->width);
	RASTER_DEBUGF(4, "offset = %d", offset);

	/* make sure len of values to copy don't exceed end of data */
	if (len > (band->width * band->height) - offset) {
		rterror("rt_band_set_pixel_line: Could not apply pixels as values length exceeds end of data");
		return ES_ERROR;
	}

	switch (pixtype) {
		case PT_1BB:
		case PT_2BUI:
		case PT_4BUI:
		case PT_8BUI:
		case PT_8BSI: {
			uint8_t *ptr = data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_16BUI: {
			uint16_t *ptr = (uint16_t *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_16BSI: {
			int16_t *ptr = (int16_t *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_32BUI: {
			uint32_t *ptr = (uint32_t *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_32BSI: {
			int32_t *ptr = (int32_t *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_32BF: {
			float *ptr = (float *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		case PT_64BF: {
			double *ptr = (double *) data;
			ptr += offset;
			memcpy(ptr, vals, size * len);
			break;
		}
		default: {
			rterror("rt_band_set_pixel_line: Unknown pixeltype %d", pixtype);
			return ES_ERROR;
		}
	}

#if POSTGIS_DEBUG_LEVEL > 0
	{
		double value;
		rt_band_get_pixel(band, x, y, &value, NULL);
		RASTER_DEBUGF(4, "pixel at (%d, %d) = %f", x, y, value);
	}
#endif

	/* set band's isnodata flag to FALSE */
	if (rt_band_get_hasnodata_flag(band))
		rt_band_set_isnodata_flag(band, 0);

	return ES_NONE;
}

/**
 * Set single pixel's value
 *
 * @param band : the band to set value to
 * @param x : x ordinate (0-based)
 * @param y : y ordinate (0-based)
 * @param val : the pixel value
 * @param converted : (optional) non-zero if value truncated/clamped/converted
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate
rt_band_set_pixel(
	rt_band band,
	int x, int y,
	double val,
	int *converted
) {
	rt_pixtype pixtype = PT_END;
	unsigned char* data = NULL;
	uint32_t offset = 0;

	int32_t checkvalint = 0;
	uint32_t checkvaluint = 0;
	float checkvalfloat = 0;
	double checkvaldouble = 0;

	assert(NULL != band);

	if (converted != NULL)
		*converted = 0;

	if (band->offline) {
		rterror("rt_band_set_pixel not implemented yet for OFFDB bands");
		return ES_ERROR;
	}

	pixtype = band->pixtype;

	if (
		x < 0 || x >= band->width ||
		y < 0 || y >= band->height
	) {
		rterror("rt_band_set_pixel: Coordinates out of range");
		return ES_ERROR;
	}

	/* check that clamped value isn't clamped NODATA */
	if (band->hasnodata && pixtype != PT_64BF) {
		double newval;
		int corrected;

		rt_band_corrected_clamped_value(band, val, &newval, &corrected);

		if (corrected) {
#if POSTGIS_RASTER_WARN_ON_TRUNCATION > 0
			rtwarn("Value for pixel %d x %d has been corrected as clamped value becomes NODATA", x, y);
#endif
			val = newval;

			if (converted != NULL)
				*converted = 1;
		}
	}

	data = rt_band_get_data(band);
	offset = x + (y * band->width);

	switch (pixtype) {
		case PT_1BB: {
			data[offset] = rt_util_clamp_to_1BB(val);
			checkvalint = data[offset];
			break;
		}
		case PT_2BUI: {
			data[offset] = rt_util_clamp_to_2BUI(val);
			checkvalint = data[offset];
			break;
		}
		case PT_4BUI: {
			data[offset] = rt_util_clamp_to_4BUI(val);
			checkvalint = data[offset];
			break;
		}
		case PT_8BSI: {
			data[offset] = rt_util_clamp_to_8BSI(val);
			checkvalint = (int8_t) data[offset];
			break;
		}
		case PT_8BUI: {
			data[offset] = rt_util_clamp_to_8BUI(val);
			checkvalint = data[offset];
			break;
		}
		case PT_16BSI: {
			int16_t *ptr = (int16_t*) data; /* we assume correct alignment */
			ptr[offset] = rt_util_clamp_to_16BSI(val);
			checkvalint = (int16_t) ptr[offset];
			break;
		}
		case PT_16BUI: {
			uint16_t *ptr = (uint16_t*) data; /* we assume correct alignment */
			ptr[offset] = rt_util_clamp_to_16BUI(val);
			checkvalint = ptr[offset];
			break;
		}
		case PT_32BSI: {
			int32_t *ptr = (int32_t*) data; /* we assume correct alignment */
			ptr[offset] = rt_util_clamp_to_32BSI(val);
			checkvalint = (int32_t) ptr[offset];
			break;
		}
		case PT_32BUI: {
			uint32_t *ptr = (uint32_t*) data; /* we assume correct alignment */
			ptr[offset] = rt_util_clamp_to_32BUI(val);
			checkvaluint = ptr[offset];
			break;
		}
		case PT_32BF: {
			float *ptr = (float*) data; /* we assume correct alignment */
			ptr[offset] = rt_util_clamp_to_32F(val);
			checkvalfloat = ptr[offset];
			break;
		}
		case PT_64BF: {
			double *ptr = (double*) data; /* we assume correct alignment */
			ptr[offset] = val;
			checkvaldouble = ptr[offset];
			break;
		}
		default: {
			rterror("rt_band_set_pixel: Unknown pixeltype %d", pixtype);
			return ES_ERROR;
		}
	}

	/* If the stored value is not NODATA, reset the isnodata flag */
	if (!rt_band_clamped_value_is_nodata(band, val)) {
		RASTER_DEBUG(3, "Band has a value that is not NODATA. Setting isnodata to FALSE");
		band->isnodata = FALSE;
	}

	/* Overflow checking */
	if (rt_util_dbl_trunc_warning(
		val,
		checkvalint, checkvaluint,
		checkvalfloat, checkvaldouble,
		pixtype
	) && converted != NULL) {
		*converted = 1;
	}

	return ES_NONE;
}

/**
 * Get values of multiple pixels.  Unlike rt_band_get_pixel,
 * values in vals are of the band's pixel type so cannot be
 * assumed to be double.  Function uses memcpy.
 *
 * It is important to be careful when using this function as
 * the number of values being fetched may exceed a pixel "row".
 * Remember that the band values are stored in a stream (1-D array)
 * regardless of what the raster's width and height might be.
 * So, getting a number of values may cross multiple pixel "rows".
 *
 * @param band : the band to get pixel value from
 * @param x : pixel column (0-based)
 * @param y : pixel row (0-based)
 * @param len : the number of pixels to get
 * @param **vals : the pixel values
 * @param *nvals : the number of pixel values being returned
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate rt_band_get_pixel_line(
	rt_band band,
	int x, int y,
	uint16_t len,
	void **vals, uint16_t *nvals
) {
	uint8_t *_vals = NULL;
	int pixsize = 0;
	uint8_t *data = NULL;
	uint32_t offset = 0; 
	uint16_t _nvals = 0;
	int maxlen = 0;
	uint8_t *ptr = NULL;

	assert(NULL != band);
	assert(vals != NULL && nvals != NULL);

	/* initialize to no values */
	*nvals = 0;

	if (
		x < 0 || x >= band->width ||
		y < 0 || y >= band->height
	) {
		rtwarn("Attempting to get pixel values with out of range raster coordinates: (%d, %d)", x, y);
		return ES_ERROR;
	}

	if (len < 1)
		return ES_NONE;

	data = rt_band_get_data(band);
	if (data == NULL) {
		rterror("rt_band_get_pixel_line: Cannot get band data");
		return ES_ERROR;
	}

	/* +1 for the nodata value */
	offset = x + (y * band->width);
	RASTER_DEBUGF(4, "offset = %d", offset);

	pixsize = rt_pixtype_size(band->pixtype);
	RASTER_DEBUGF(4, "pixsize = %d", pixsize);

	/* cap _nvals so that it doesn't overflow */
	_nvals = len;
	maxlen = band->width * band->height;

	if (((int) (offset + _nvals)) > maxlen) {
		_nvals = maxlen - offset;
		rtwarn("Limiting returning number values to %d", _nvals);
	}
	RASTER_DEBUGF(4, "_nvals = %d", _nvals);

	ptr = data + (offset * pixsize);

	_vals = rtalloc(_nvals * pixsize);
	if (_vals == NULL) {
		rterror("rt_band_get_pixel_line: Could not allocate memory for pixel values");
		return ES_ERROR;
	}

	/* copy pixels */
	memcpy(_vals, ptr, _nvals * pixsize);

	*vals = _vals;
	*nvals = _nvals;

	return ES_NONE;
}

/**
 * Get pixel value. If band's isnodata flag is TRUE, value returned 
 * will be the band's NODATA value
 *
 * @param band : the band to set nodata value to
 * @param x : x ordinate (0-based)
 * @param y : x ordinate (0-based)
 * @param *value : pixel value
 * @param *nodata : 0 if pixel is not NODATA
 *
 * @return 0 on success, -1 on error (value out of valid range).
 */
rt_errorstate
rt_band_get_pixel(
	rt_band band,
	int x, int y,
	double *value,
	int *nodata
) {
	rt_pixtype pixtype = PT_END;
	uint8_t* data = NULL;
	uint32_t offset = 0; 

	assert(NULL != band);
	assert(NULL != value);

	/* set nodata to 0 */
	if (nodata != NULL)
		*nodata = 0;

	if (
		x < 0 || x >= band->width ||
		y < 0 || y >= band->height
	) {
		rtwarn("Attempting to get pixel value with out of range raster coordinates: (%d, %d)", x, y);
		return ES_ERROR;
	}

	/* band is NODATA */
	if (band->isnodata) {
		RASTER_DEBUG(3, "Band's isnodata flag is TRUE. Returning NODATA value");
		*value = band->nodataval;
		if (nodata != NULL) *nodata = 1;
		return ES_NONE;
	}

	data = rt_band_get_data(band);
	if (data == NULL) {
		rterror("rt_band_get_pixel: Cannot get band data");
		return ES_ERROR;
	}

	/* +1 for the nodata value */
	offset = x + (y * band->width);

	pixtype = band->pixtype;

	switch (pixtype) {
		case PT_1BB:
#ifdef OPTIMIZE_SPACE
			{
				int byteOffset = offset / 8;
				int bitOffset = offset % 8;
				data += byteOffset;

				/* Bit to set is bitOffset into data */
				*value = getBits(data, val, 1, bitOffset);
				break;
			}
#endif
		case PT_2BUI:
#ifdef OPTIMIZE_SPACE
			{
				int byteOffset = offset / 4;
				int bitOffset = offset % 4;
				data += byteOffset;

				/* Bits to set start at bitOffset into data */
				*value = getBits(data, val, 2, bitOffset);
				break;
			}
#endif
		case PT_4BUI:
#ifdef OPTIMIZE_SPACE
			{
				int byteOffset = offset / 2;
				int bitOffset = offset % 2;
				data += byteOffset;

				/* Bits to set start at bitOffset into data */
				*value = getBits(data, val, 2, bitOffset);
				break;
			}
#endif
		case PT_8BSI: {
			int8_t val = data[offset];
			*value = val;
			break;
		}
		case PT_8BUI: {
			uint8_t val = data[offset];
			*value = val;
			break;
		}
		case PT_16BSI: {
			int16_t *ptr = (int16_t*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		case PT_16BUI: {
			uint16_t *ptr = (uint16_t*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		case PT_32BSI: {
			int32_t *ptr = (int32_t*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		case PT_32BUI: {
			uint32_t *ptr = (uint32_t*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		case PT_32BF: {
			float *ptr = (float*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		case PT_64BF: {
			double *ptr = (double*) data; /* we assume correct alignment */
			*value = ptr[offset];
			break;
		}
		default: {
			rterror("rt_band_get_pixel: Unknown pixeltype %d", pixtype);
			return ES_ERROR;
		}
	}

	/* set NODATA flag */
	if (band->hasnodata && nodata != NULL) {
		if (rt_band_clamped_value_is_nodata(band, *value))
			*nodata = 1;
	}

	return ES_NONE;
}

/**
 * Get nearest pixel(s) with value (not NODATA) to specified pixel
 *
 * @param band : the band to get nearest pixel(s) from
 * @param x : the column of the pixel (0-based)
 * @param y : the line of the pixel (0-based)
 * @param distancex : the number of pixels around the specified pixel
 * along the X axis
 * @param distancey : the number of pixels around the specified pixel
 * along the Y axis
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * to check for pixels with value
 * @param npixels : return set of rt_pixel object or NULL
 *
 * @return -1 on error, otherwise the number of rt_pixel objects
 * in npixels
 */
int rt_band_get_nearest_pixel(
	rt_band band,
	int x, int y,
	uint16_t distancex, uint16_t distancey,
	int exclude_nodata_value,
	rt_pixel *npixels
) {
	rt_pixel npixel = NULL;
	int extent[4] = {0};
	int max_extent[4] = {0};
	int d0 = 0;
	int distance[2] = {0};
	uint32_t _d[2] = {0};
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	int _max = 0;
	int _x = 0;
	int _y = 0;
	int *_min = NULL;
	double pixval = 0;
	double minval = 0;
	uint32_t count = 0;
	int isnodata = 0;

	int inextent = 0;

	assert(NULL != band);
	assert(NULL != npixels);

	RASTER_DEBUG(3, "Starting");

	/* process distance */
	distance[0] = distancex;
	distance[1] = distancey;

	/* no distance, means get nearest pixels and return */
	if (!distance[0] && !distance[1])
		d0 = 1;

	RASTER_DEBUGF(4, "Selected pixel: %d x %d", x, y);
	RASTER_DEBUGF(4, "Distances: %d x %d", distance[0], distance[1]);

	/* shortcuts if outside band extent */
	if (
		exclude_nodata_value && (
			(x < 0 || x > band->width) ||
			(y < 0 || y > band->height)
		)
	) {
		/* no distances specified, jump to pixel close to extent */
		if (d0) {
			if (x < 0)
				x = -1;
			else if (x > band->width)
				x = band->width;

			if (y < 0)
				y = -1;
			else if (y > band->height)
				y = band->height;

			RASTER_DEBUGF(4, "Moved selected pixel: %d x %d", x, y);
		}
		/*
			distances specified
			if distances won't capture extent of band, return 0
		*/
		else if (
			((x < 0 && abs(x) > distance[0]) || (x - band->width >= distance[0])) ||
			((y < 0 && abs(y) > distance[1]) || (y - band->height >= distance[1]))
		) {
			RASTER_DEBUG(4, "No nearest pixels possible for provided pixel and distances");
			return 0;
		}
	}

	/* no NODATA, exclude is FALSE */
	if (!band->hasnodata)
		exclude_nodata_value = FALSE;
	/* band is NODATA and excluding NODATA */
	else if (exclude_nodata_value && band->isnodata) {
		RASTER_DEBUG(4, "No nearest pixels possible as band is NODATA and excluding NODATA values");
		return 0;
	}

	/* determine the maximum distance to prevent an infinite loop */
	if (d0) {
		int a, b;

		/* X axis */
		a = abs(x);
		b = abs(x - band->width);

		if (a > b)
			distance[0] = a;
		else
			distance[0] = b;

		/* Y axis */
		a = abs(y);
		b = abs(y - band->height);
		if (a > b)
			distance[1] = a;
		else
			distance[1] = b;

		RASTER_DEBUGF(4, "Maximum distances: %d x %d", distance[0], distance[1]);
	}

	/* minimum possible value for pixel type */
	minval = rt_pixtype_get_min_value(band->pixtype);
	RASTER_DEBUGF(4, "pixtype: %s", rt_pixtype_name(band->pixtype));
	RASTER_DEBUGF(4, "minval: %f", minval);

	/* set variables */
	count = 0;
	*npixels = NULL;

	/* maximum extent */
	max_extent[0] = x - distance[0]; /* min X */
	max_extent[1] = y - distance[1]; /* min Y */
	max_extent[2] = x + distance[0]; /* max X */
	max_extent[3] = y + distance[1]; /* max Y */
	RASTER_DEBUGF(4, "Maximum Extent: (%d, %d, %d, %d)",
		max_extent[0], max_extent[1], max_extent[2], max_extent[3]);

	_d[0] = 0;
	_d[1] = 0;
	do {
		_d[0]++;
		_d[1]++;

		extent[0] = x - _d[0]; /* min x */
		extent[1] = y - _d[1]; /* min y */
		extent[2] = x + _d[0]; /* max x */
		extent[3] = y + _d[1]; /* max y */

		RASTER_DEBUGF(4, "Processing distances: %d x %d", _d[0], _d[1]);
		RASTER_DEBUGF(4, "Extent: (%d, %d, %d, %d)",
			extent[0], extent[1], extent[2], extent[3]);

		for (i = 0; i < 2; i++) {

			/* by row */
			if (i < 1)
				_max = extent[2] - extent[0] + 1;
			/* by column */
			else
				_max = extent[3] - extent[1] + 1;
			_max = abs(_max);

			for (j = 0; j < 2; j++) {
				/* by row */
				if (i < 1) {
					_x = extent[0];
					_min = &_x;

					/* top row */
					if (j < 1)
						_y = extent[1];
					/* bottom row */
					else
						_y = extent[3];
				}
				/* by column */
				else {
					_y = extent[1] + 1;
					_min = &_y;

					/* left column */
					if (j < 1) {
						_x = extent[0];
						_max -= 2;
					}
					/* right column */
					else
						_x = extent[2];
				}

				RASTER_DEBUGF(4, "_min, _max: %d, %d", *_min, _max);
				for (k = 0; k < _max; k++) {
					/* check that _x and _y are not outside max extent */
					if (
						_x < max_extent[0] || _x > max_extent[2] ||
						_y < max_extent[1] || _y > max_extent[3]
					) {
						(*_min)++;
						continue;
					}

					/* outside band extent, set to NODATA */
					if (
						(_x < 0 || _x >= band->width) ||
						(_y < 0 || _y >= band->height)
					) {
						/* no NODATA, set to minimum possible value */
						if (!band->hasnodata)
							pixval = minval;
						/* has NODATA, use NODATA */
						else
							pixval = band->nodataval;
						RASTER_DEBUGF(4, "NODATA pixel outside band extent: (x, y, val) = (%d, %d, %f)", _x, _y, pixval);
						inextent = 0;
						isnodata = 1;
					}
					else {
						if (rt_band_get_pixel(
							band,
							_x, _y,
							&pixval,
							&isnodata
						) != ES_NONE) {
							rterror("rt_band_get_nearest_pixel: Could not get pixel value");
							if (count) rtdealloc(*npixels);
							return -1;
						}
						RASTER_DEBUGF(4, "Pixel: (x, y, val) = (%d, %d, %f)", _x, _y, pixval);
						inextent = 1;
					}

					/* use pixval? */
					if (!exclude_nodata_value || (exclude_nodata_value && !isnodata)) {
						/* add pixel to result set */
						RASTER_DEBUGF(4, "Adding pixel to set of nearest pixels: (x, y, val) = (%d, %d, %f)", _x, _y, pixval);
						count++;

						if (*npixels == NULL)
							*npixels = (rt_pixel) rtalloc(sizeof(struct rt_pixel_t) * count);
						else
							*npixels = (rt_pixel) rtrealloc(*npixels, sizeof(struct rt_pixel_t) * count);
						if (*npixels == NULL) {
							rterror("rt_band_get_nearest_pixel: Could not allocate memory for nearest pixel(s)");
							return -1;
						}

						npixel = &((*npixels)[count - 1]);
						npixel->x = _x;
						npixel->y = _y;
						npixel->value = pixval;

						/* special case for when outside band extent */
						if (!inextent && !band->hasnodata)
							npixel->nodata = 1;
						else
							npixel->nodata = 0;
					}

					(*_min)++;
				}
			}
		}

		/* distance threshholds met */
		if (_d[0] >= distance[0] && _d[1] >= distance[1])
			break;
		else if (d0 && count)
			break;
	}
	while (1);

	RASTER_DEBUGF(3, "Nearest pixels in return: %d", count);

	return count;
}

/**
 * Search band for pixel(s) with search values
 *
 * @param band : the band to query for minimum and maximum pixel values
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * @param searchset : array of values to count
 * @param searchcount : the number of search values
 * @param pixels : pixels with the search value
 *
 * @return -1 on error, otherwise number of pixels
 */
int
rt_band_get_pixel_of_value(
	rt_band band, int exclude_nodata_value,
	double *searchset, int searchcount,
	rt_pixel *pixels
) {
	int x;
	int y;
	int i;
	double pixval;
	int err;
	int count = 0;
	int isnodata = 0;
	int isequal = 0;

	rt_pixel pixel = NULL;

	assert(NULL != band);
	assert(NULL != pixels);
	assert(NULL != searchset && searchcount > 0);

	if (!band->hasnodata)
		exclude_nodata_value = FALSE;
	/* band is NODATA and exclude_nodata_value = TRUE, nothing to search */
	else if (exclude_nodata_value && band->isnodata) {
		RASTER_DEBUG(4, "Pixels cannot be searched as band is NODATA and excluding NODATA values");
		return 0;
	}

	for (x = 0; x < band->width; x++) {
		for (y = 0; y < band->height; y++) {
			err = rt_band_get_pixel(band, x, y, &pixval, &isnodata);
			if (err != ES_NONE) {
				rterror("rt_band_get_pixel_of_value: Cannot get band pixel");
				return -1;
			}
			else if (exclude_nodata_value && isnodata)
				continue;

			for (i = 0; i < searchcount; i++) {
				if (rt_pixtype_compare_clamped_values(band->pixtype, searchset[i], pixval, &isequal) != ES_NONE) {
					continue;
				}

				if (FLT_NEQ(pixval, searchset[i]) || !isequal)
					continue;

				/* match found */
				count++;
				if (*pixels == NULL)
					*pixels = (rt_pixel) rtalloc(sizeof(struct rt_pixel_t) * count);
				else
					*pixels = (rt_pixel) rtrealloc(*pixels, sizeof(struct rt_pixel_t) * count);
				if (*pixels == NULL) {
					rterror("rt_band_get_pixel_of_value: Could not allocate memory for pixel(s)");
					return -1;
				}

				pixel = &((*pixels)[count - 1]);
				pixel->x = x;
				pixel->y = y;
				pixel->nodata = 0;
				pixel->value = pixval;
			}
		}
	}

	return count;
}

/**
 * Get NODATA value
 *
 * @param band : the band whose NODATA value will be returned
 * @param nodata : the band's NODATA value
 *
 * @return ES_NONE or ES_ERROR
 */
rt_errorstate
rt_band_get_nodata(rt_band band, double *nodata) { 
	assert(NULL != band);
	assert(NULL != nodata);

	*nodata = band->nodataval;

	if (!band->hasnodata) {
		rterror("rt_band_get_nodata: Band has no NODATA value");
		return ES_ERROR;
	}

	return ES_NONE;
}

double
rt_band_get_min_value(rt_band band) {
	assert(NULL != band);

	return rt_pixtype_get_min_value(band->pixtype);
}

int
rt_band_check_is_nodata(rt_band band) {
	int i, j, err;
	double pxValue;
	int isnodata = 0;

	assert(NULL != band);

	/* Check if band has nodata value */
	if (!band->hasnodata) {
		RASTER_DEBUG(3, "Band has no NODATA value");
		band->isnodata = FALSE;
		return FALSE;
	}

	pxValue = band->nodataval;

	/* Check all pixels */
	for (i = 0; i < band->width; i++) {
		for (j = 0; j < band->height; j++) {
			err = rt_band_get_pixel(band, i, j, &pxValue, &isnodata);
			if (err != ES_NONE) {
				rterror("rt_band_check_is_nodata: Cannot get band pixel");
				return FALSE;
			}
			else if (!isnodata) {
				band->isnodata = FALSE;
				return FALSE;
			}
		}
	}

	band->isnodata = TRUE;
	return TRUE;
}

/**
 * Compare clamped value to band's clamped NODATA value.
 *
 * @param band : the band whose NODATA value will be used for comparison
 * @param val : the value to compare to the NODATA value
 *
 * @return 2 if unclamped value is unclamped NODATA
 *         1 if clamped value is clamped NODATA
 *         0 if clamped value is NOT clamped NODATA
 */
int
rt_band_clamped_value_is_nodata(rt_band band, double val) {
	int isequal = 0;

	assert(NULL != band);

	/* no NODATA, so never equal */
	if (!band->hasnodata)
		return 0;

	/* value is exactly NODATA */
	if (FLT_EQ(val, band->nodataval))
		return 2;

	/* ignore error from rt_pixtype_compare_clamped_values */
	rt_pixtype_compare_clamped_values(
		band->pixtype,
		val, band->nodataval,
		&isequal
	);

	return isequal ? 1 : 0;
}

/**
 * Correct value when clamped value is equal to clamped NODATA value.
 * Correction does NOT occur if unclamped value is exactly unclamped
 * NODATA value.
 * 
 * @param band : the band whose NODATA value will be used for comparison
 * @param val : the value to compare to the NODATA value and correct
 * @param *newval : pointer to corrected value
 * @param *corrected : (optional) non-zero if val was corrected
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_band_corrected_clamped_value(
	rt_band band,
	double val,
	double *newval, int *corrected
) {
	double minval = 0.;

	assert(NULL != band);
	assert(NULL != newval);

	if (corrected != NULL)
		*corrected = 0;

	/* no need to correct if clamped values IS NOT clamped NODATA */
	if (rt_band_clamped_value_is_nodata(band, val) != 1) {
		*newval = val;
		return ES_NONE;
	}

	minval = rt_pixtype_get_min_value(band->pixtype);
	*newval = val;

	switch (band->pixtype) {
		case PT_1BB:
			*newval = !band->nodataval;
			break;
		case PT_2BUI:
			if (rt_util_clamp_to_2BUI(val) == rt_util_clamp_to_2BUI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_4BUI:
			if (rt_util_clamp_to_4BUI(val) == rt_util_clamp_to_4BUI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_8BSI:
			if (rt_util_clamp_to_8BSI(val) == rt_util_clamp_to_8BSI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_8BUI:
			if (rt_util_clamp_to_8BUI(val) == rt_util_clamp_to_8BUI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_16BSI:
			if (rt_util_clamp_to_16BSI(val) == rt_util_clamp_to_16BSI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_16BUI:
			if (rt_util_clamp_to_16BUI(val) == rt_util_clamp_to_16BUI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_32BSI:
			if (rt_util_clamp_to_32BSI(val) == rt_util_clamp_to_32BSI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_32BUI:
			if (rt_util_clamp_to_32BUI(val) == rt_util_clamp_to_32BUI(minval))
				(*newval)++;
			else
				(*newval)--;
			break;
		case PT_32BF:
			if (FLT_EQ(rt_util_clamp_to_32F(val), rt_util_clamp_to_32F(minval)))
				*newval += FLT_EPSILON;
			else
				*newval -= FLT_EPSILON;
			break;
		case PT_64BF:
			break;
		default:
			rterror("rt_band_corrected_clamped_value: Unknown pixeltype %d", band->pixtype);
			return ES_ERROR;
	}

	if (corrected != NULL)
		*corrected = 1;

	return ES_NONE;
}

/**
 * Compute summary statistics for a band
 *
 * @param band : the band to query for summary stats
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * @param sample : percentage of pixels to sample
 * @param inc_vals : flag to include values in return struct
 * @param cK : number of pixels counted thus far in coverage
 * @param cM : M component of 1-pass stddev for coverage
 * @param cQ : Q component of 1-pass stddev for coverage
 *
 * @return the summary statistics for a band or NULL
 */
rt_bandstats
rt_band_get_summary_stats(
	rt_band band,
	int exclude_nodata_value, double sample, int inc_vals,
	uint64_t *cK, double *cM, double *cQ
) {
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t z = 0;
	uint32_t offset = 0;
	uint32_t diff = 0;
	int rtn;
	int hasnodata = FALSE;
	double nodata = 0;
	double *values = NULL;
	double value;
	int isnodata = 0;
	rt_bandstats stats = NULL;

	uint32_t do_sample = 0;
	uint32_t sample_size = 0;
	uint32_t sample_per = 0;
	uint32_t sample_int = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	double sum = 0;
	uint32_t k = 0;
	double M = 0;
	double Q = 0;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != band);

	/* band is empty (width < 1 || height < 1) */
	if (band->width < 1 || band->height < 1) {
		stats = (rt_bandstats) rtalloc(sizeof(struct rt_bandstats_t));
		if (NULL == stats) {
			rterror("rt_band_get_summary_stats: Could not allocate memory for stats");
			return NULL;
		}

		rtwarn("Band is empty as width and/or height is 0");

		stats->sample = 1;
		stats->sorted = 0;
		stats->values = NULL;
		stats->count = 0;
		stats->min = stats->max = 0;
		stats->sum = 0;
		stats->mean = 0;
		stats->stddev = -1;

		return stats;
	}

	hasnodata = rt_band_get_hasnodata_flag(band);
	if (hasnodata != FALSE)
		rt_band_get_nodata(band, &nodata);
	else
		exclude_nodata_value = 0;

	RASTER_DEBUGF(3, "nodata = %f", nodata);
	RASTER_DEBUGF(3, "hasnodata = %d", hasnodata);
	RASTER_DEBUGF(3, "exclude_nodata_value = %d", exclude_nodata_value);

	/* entire band is nodata */
	if (rt_band_get_isnodata_flag(band) != FALSE) {
		stats = (rt_bandstats) rtalloc(sizeof(struct rt_bandstats_t));
		if (NULL == stats) {
			rterror("rt_band_get_summary_stats: Could not allocate memory for stats");
			return NULL;
		}

		stats->sample = 1;
		stats->sorted = 0;
		stats->values = NULL;

		if (exclude_nodata_value) {
			rtwarn("All pixels of band have the NODATA value");

			stats->count = 0;
			stats->min = stats->max = 0;
			stats->sum = 0;
			stats->mean = 0;
			stats->stddev = -1;
		}
		else {
			stats->count = band->width * band->height;
			stats->min = stats->max = nodata;
			stats->sum = stats->count * nodata;
			stats->mean = nodata;
			stats->stddev = 0;
		}

		return stats;
	}

	/* clamp percentage */
	if (
		(sample < 0 || FLT_EQ(sample, 0.0)) ||
		(sample > 1 || FLT_EQ(sample, 1.0))
	) {
		do_sample = 0;
		sample = 1;
	}
	else
		do_sample = 1;
	RASTER_DEBUGF(3, "do_sample = %d", do_sample);

	/* sample all pixels */
	if (!do_sample) {
		sample_size = band->width * band->height;
		sample_per = band->height;
	}
	/*
	 randomly sample a percentage of available pixels
	 sampling method is known as
	 	"systematic random sample without replacement"
	*/
	else {
		sample_size = round((band->width * band->height) * sample);
		sample_per = round(sample_size / band->width);
		if (sample_per < 1)
			sample_per = 1;
		sample_int = round(band->height / sample_per);
		srand(time(NULL));
	}

	RASTER_DEBUGF(3, "sampling %d of %d available pixels w/ %d per set"
		, sample_size, (band->width * band->height), sample_per);

	if (inc_vals) {
		values = rtalloc(sizeof(double) * sample_size);
		if (NULL == values) {
			rtwarn("Could not allocate memory for values");
			inc_vals = 0;
		}
	}

	/* initialize stats */
	stats = (rt_bandstats) rtalloc(sizeof(struct rt_bandstats_t));
	if (NULL == stats) {
		rterror("rt_band_get_summary_stats: Could not allocate memory for stats");
		return NULL;
	}
	stats->sample = sample;
	stats->count = 0;
	stats->sum = 0;
	stats->mean = 0;
	stats->stddev = -1;
	stats->min = stats->max = 0;
	stats->values = NULL;
	stats->sorted = 0;

	for (x = 0, j = 0, k = 0; x < band->width; x++) {
		y = -1;
		diff = 0;

		for (i = 0, z = 0; i < sample_per; i++) {
			if (!do_sample)
				y = i;
			else {
				offset = (rand() % sample_int) + 1;
				y += diff + offset;
				diff = sample_int - offset;
			}
			RASTER_DEBUGF(5, "(x, y, z) = (%d, %d, %d)", x, y, z);
			if (y >= band->height || z > sample_per) break;

			rtn = rt_band_get_pixel(band, x, y, &value, &isnodata);

			j++;
			if (rtn == ES_NONE && (!exclude_nodata_value || (exclude_nodata_value && !isnodata))) {

				/* inc_vals set, collect pixel values */
				if (inc_vals) values[k] = value;

				/* average */
				k++;
				sum += value;

				/*
					one-pass standard deviation
					http://www.eecs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
				*/
				if (k == 1) {
					Q = 0;
					M = value;
				}
				else {
					Q += (((k  - 1) * pow(value - M, 2)) / k);
					M += ((value - M ) / k);
				}

				/* coverage one-pass standard deviation */
				if (NULL != cK) {
					(*cK)++;
					if (*cK == 1) {
						*cQ = 0;
						*cM = value;
					}
					else {
						*cQ += (((*cK  - 1) * pow(value - *cM, 2)) / *cK);
						*cM += ((value - *cM ) / *cK);
					}
				}

				/* min/max */
				if (stats->count < 1) {
					stats->count = 1;
					stats->min = stats->max = value;
				}
				else {
					if (value < stats->min)
						stats->min = value;
					if (value > stats->max)
						stats->max = value;
				}

			}

			z++;
		}
	}

	RASTER_DEBUG(3, "sampling complete");

	stats->count = k;
	if (k > 0) {
		if (inc_vals) {
			/* free unused memory */
			if (sample_size != k) {
				values = rtrealloc(values, k * sizeof(double));
			}

			stats->values = values;
		}

		stats->sum = sum;
		stats->mean = sum / k;

		/* standard deviation */
		if (!do_sample)
			stats->stddev = sqrt(Q / k);
		/* sample deviation */
		else {
			if (k < 2)
				stats->stddev = -1;
			else
				stats->stddev = sqrt(Q / (k - 1));
		}
	}
	/* inc_vals thus values allocated but not used */
	else if (inc_vals)
		rtdealloc(values);

	/* if do_sample is one */
	if (do_sample && k < 1)
		rtwarn("All sampled pixels of band have the NODATA value");

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "(time, count, mean, stddev, min, max) = (%0.4f, %d, %f, %f, %f, %f)",
		elapsed, stats->count, stats->mean, stats->stddev, stats->min, stats->max);
#endif

	RASTER_DEBUG(3, "done");
	return stats;
}

/**
 * Count the distribution of data
 *
 * @param stats : a populated stats struct for processing
 * @param bin_count : the number of bins to group the data by
 * @param bin_width : the width of each bin as an array
 * @param bin_width_count : number of values in bin_width
 * @param right : evaluate bins by (a,b] rather than default [a,b)
 * @param min : user-defined minimum value of the histogram
 *   a value less than the minimum value is not counted in any bins
 *   if min = max, min and max are not used
 * @param max : user-defined maximum value of the histogram
 *   a value greater than the max value is not counted in any bins
 *   if min = max, min and max are not used
 * @param rtn_count : set to the number of bins being returned
 *
 * @return the histogram of the data or NULL
 */
rt_histogram
rt_band_get_histogram(
	rt_bandstats stats,
	int bin_count, double *bin_width, int bin_width_count,
	int right, double min, double max,
	uint32_t *rtn_count
) {
	rt_histogram bins = NULL;
	int init_width = 0;
	int i;
	int j;
	double tmp;
	double value;
	int sum = 0;
	double qmin;
	double qmax;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != stats);
	assert(NULL != rtn_count);

	if (stats->count < 1 || NULL == stats->values) {
		rterror("rt_util_get_histogram: rt_bandstats object has no value");
		return NULL;
	}

	/* bin width must be positive numbers and not zero */
	if (NULL != bin_width && bin_width_count > 0) {
		for (i = 0; i < bin_width_count; i++) {
			if (bin_width[i] < 0 || FLT_EQ(bin_width[i], 0.0)) {
				rterror("rt_util_get_histogram: bin_width element is less than or equal to zero");
				return NULL;
			}
		}
	}

	/* ignore min and max parameters */
	if (FLT_EQ(max, min)) {
		qmin = stats->min;
		qmax = stats->max;
	}
	else {
		qmin = min;
		qmax = max;
		if (qmin > qmax) {
			qmin = max;
			qmax = min;
		}
	}

	/* # of bins not provided */
	if (bin_count <= 0) {
		/*
			determine # of bins
			http://en.wikipedia.org/wiki/Histogram

			all computed bins are assumed to have equal width
		*/
		/* Square-root choice for stats->count < 30 */
		if (stats->count < 30)
			bin_count = ceil(sqrt(stats->count));
		/* Sturges' formula for stats->count >= 30 */
		else
			bin_count = ceil(log2((double) stats->count) + 1.);

		/* bin_width_count provided and bin_width has value */
		if (bin_width_count > 0 && NULL != bin_width) {
			/* user has defined something specific */
			if (bin_width_count > bin_count)
				bin_count = bin_width_count;
			else if (bin_width_count > 1) {
				tmp = 0;
				for (i = 0; i < bin_width_count; i++) tmp += bin_width[i];
				bin_count = ceil((qmax - qmin) / tmp) * bin_width_count;
			}
			else
				bin_count = ceil((qmax - qmin) / bin_width[0]);
		}
		/* set bin width count to zero so that one can be calculated */
		else {
			bin_width_count = 0;
		}
	}

	/* min and max the same */
	if (FLT_EQ(qmax, qmin)) bin_count = 1;

	RASTER_DEBUGF(3, "bin_count = %d", bin_count);

	/* bin count = 1, all values are in one bin */
	if (bin_count < 2) {
		bins = rtalloc(sizeof(struct rt_histogram_t));
		if (NULL == bins) {
			rterror("rt_util_get_histogram: Could not allocate memory for histogram");
			return NULL;
		}

		bins->count = stats->count;
		bins->percent = -1;
		bins->min = qmin;
		bins->max = qmax;
		bins->inc_min = bins->inc_max = 1;

		*rtn_count = bin_count;
		return bins;
	}

	/* establish bin width */
	if (bin_width_count == 0) {
		bin_width_count = 1;

		/* bin_width unallocated */
		if (NULL == bin_width) {
			bin_width = rtalloc(sizeof(double));
			if (NULL == bin_width) {
				rterror("rt_util_get_histogram: Could not allocate memory for bin widths");
				return NULL;
			}
			init_width = 1;
		}

		bin_width[0] = (qmax - qmin) / bin_count;
	}

	/* initialize bins */
	bins = rtalloc(bin_count * sizeof(struct rt_histogram_t));
	if (NULL == bins) {
		rterror("rt_util_get_histogram: Could not allocate memory for histogram");
		if (init_width) rtdealloc(bin_width);
		return NULL;
	}
	if (!right)
		tmp = qmin;
	else
		tmp = qmax;
	for (i = 0; i < bin_count;) {
		for (j = 0; j < bin_width_count; j++) {
			bins[i].count = 0;
			bins->percent = -1;

			if (!right) {
				bins[i].min = tmp;
				tmp += bin_width[j];
				bins[i].max = tmp;

				bins[i].inc_min = 1;
				bins[i].inc_max = 0;
			}
			else {
				bins[i].max = tmp;
				tmp -= bin_width[j];
				bins[i].min = tmp;

				bins[i].inc_min = 0;
				bins[i].inc_max = 1;
			}

			i++;
		}
	}
	if (!right) {
		bins[bin_count - 1].inc_max = 1;

		/* align last bin to the max value */
		if (bins[bin_count - 1].max < qmax)
			bins[bin_count - 1].max = qmax;
	}
	else {
		bins[bin_count - 1].inc_min = 1;

		/* align first bin to the min value */
		if (bins[bin_count - 1].min > qmin)
			bins[bin_count - 1].min = qmin;
	}

	/* process the values */
	for (i = 0; i < stats->count; i++) {
		value = stats->values[i];

		/* default, [a, b) */
		if (!right) {
			for (j = 0; j < bin_count; j++) {
				if (
					(!bins[j].inc_max && value < bins[j].max) || (
						bins[j].inc_max && (
							(value < bins[j].max) ||
							FLT_EQ(value, bins[j].max)
						)
					)
				) {
					bins[j].count++;
					sum++;
					break;
				}
			}
		}
		/* (a, b] */
		else {
			for (j = 0; j < bin_count; j++) {
				if (
					(!bins[j].inc_min && value > bins[j].min) || (
						bins[j].inc_min && (
							(value > bins[j].min) ||
							FLT_EQ(value, bins[j].min)
						)
					)
				) {
					bins[j].count++;
					sum++;
					break;
				}
			}
		}
	}

	for (i = 0; i < bin_count; i++) {
		bins[i].percent = ((double) bins[i].count) / sum;
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);

	for (j = 0; j < bin_count; j++) {
		RASTER_DEBUGF(5, "(min, max, inc_min, inc_max, count, sum, percent) = (%f, %f, %d, %d, %d, %d, %f)",
			bins[j].min, bins[j].max, bins[j].inc_min, bins[j].inc_max, bins[j].count, sum, bins[j].percent);
	}
#endif

	if (init_width) rtdealloc(bin_width);
	*rtn_count = bin_count;
	RASTER_DEBUG(3, "done");
	return bins;
}

/**
 * Compute the default set of or requested quantiles for a set of data
 * the quantile formula used is same as Excel and R default method
 *
 * @param stats : a populated stats struct for processing
 * @param quantiles : the quantiles to be computed
 * @param quantiles_count : the number of quantiles to be computed
 * @param rtn_count : set to the number of quantiles being returned
 *
 * @return the default set of or requested quantiles for a band or NULL
 */
rt_quantile
rt_band_get_quantiles(
	rt_bandstats stats,
	double *quantiles, int quantiles_count,
	uint32_t *rtn_count
) {
	rt_quantile rtn;
	int init_quantiles = 0;
	int i = 0;
	double h;
	int hl;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != stats);
	assert(NULL != rtn_count);

	if (stats->count < 1 || NULL == stats->values) {
		rterror("rt_band_get_quantiles: rt_bandstats object has no value");
		return NULL;
	}

	/* quantiles not provided */
	if (NULL == quantiles) {
		/* quantile count not specified, default to quartiles */
		if (quantiles_count < 2)
			quantiles_count = 5;

		quantiles = rtalloc(sizeof(double) * quantiles_count);
		init_quantiles = 1;
		if (NULL == quantiles) {
			rterror("rt_band_get_quantiles: Could not allocate memory for quantile input");
			return NULL;
		}

		quantiles_count--;
		for (i = 0; i <= quantiles_count; i++)
			quantiles[i] = ((double) i) / quantiles_count;
		quantiles_count++;
	}

	/* check quantiles */
	for (i = 0; i < quantiles_count; i++) {
		if (quantiles[i] < 0. || quantiles[i] > 1.) {
			rterror("rt_band_get_quantiles: Quantile value not between 0 and 1");
			if (init_quantiles) rtdealloc(quantiles);
			return NULL;
		}
	}
	quicksort(quantiles, quantiles + quantiles_count - 1);

	/* initialize rt_quantile */
	rtn = rtalloc(sizeof(struct rt_quantile_t) * quantiles_count);
	if (NULL == rtn) {
		rterror("rt_band_get_quantiles: Could not allocate memory for quantile output");
		if (init_quantiles) rtdealloc(quantiles);
		return NULL;
	}

	/* sort values */
	if (!stats->sorted) {
		quicksort(stats->values, stats->values + stats->count - 1);
		stats->sorted = 1;
	}

	/*
		make quantiles

		formula is that used in R (method 7) and Excel from
			http://en.wikipedia.org/wiki/Quantile
	*/
	for (i = 0; i < quantiles_count; i++) {
		rtn[i].quantile = quantiles[i];

		h = ((stats->count - 1.) * quantiles[i]) + 1.;
		hl = floor(h);

		/* h greater than hl, do full equation */
		if (h > hl)
			rtn[i].value = stats->values[hl - 1] + ((h - hl) * (stats->values[hl] - stats->values[hl - 1]));
		/* shortcut as second part of equation is zero */
		else
			rtn[i].value = stats->values[hl - 1];
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);
#endif

	*rtn_count = quantiles_count;
	if (init_quantiles) rtdealloc(quantiles);
	RASTER_DEBUG(3, "done");
	return rtn;
}

static struct quantile_llist_element *quantile_llist_search(
	struct quantile_llist_element *element,
	double needle
) {
	if (NULL == element)
		return NULL;
	else if (FLT_NEQ(needle, element->value)) {
		if (NULL != element->next)
			return quantile_llist_search(element->next, needle);
		else
			return NULL;
	}
	else
		return element;
}

static struct quantile_llist_element *quantile_llist_insert(
	struct quantile_llist_element *element,
	double value,
	uint32_t *idx
) {
	struct quantile_llist_element *qle = NULL;

	if (NULL == element) {
		qle = rtalloc(sizeof(struct quantile_llist_element));
		RASTER_DEBUGF(4, "qle @ %p is only element in list", qle);
		if (NULL == qle) return NULL;

		qle->value = value;
		qle->count = 1;

		qle->prev = NULL;
		qle->next = NULL;

		if (NULL != idx) *idx = 0;
		return qle;
	}
	else if (value > element->value) {
		if (NULL != idx) *idx += 1;
		if (NULL != element->next)
			return quantile_llist_insert(element->next, value, idx);
		/* insert as last element in list */
		else {
			qle = rtalloc(sizeof(struct quantile_llist_element));
			RASTER_DEBUGF(4, "insert qle @ %p as last element", qle);
			if (NULL == qle) return NULL;

			qle->value = value;
			qle->count = 1;

			qle->prev = element;
			qle->next = NULL;
			element->next = qle;

			return qle;
		}
	}
	/* insert before current element */
	else {
		qle = rtalloc(sizeof(struct quantile_llist_element));
		RASTER_DEBUGF(4, "insert qle @ %p before current element", qle);
		if (NULL == qle) return NULL;

		qle->value = value;
		qle->count = 1;

		if (NULL != element->prev) element->prev->next = qle;
		qle->next = element;
		qle->prev = element->prev;
		element->prev = qle;

		return qle;
	}
}

static int quantile_llist_delete(struct quantile_llist_element *element) {
	if (NULL == element) return 0;

	/* beginning of list */
	if (NULL == element->prev && NULL != element->next) {
		element->next->prev = NULL;
	}
	/* end of list */
	else if (NULL != element->prev && NULL == element->next) {
		element->prev->next = NULL;
	}
	/* within list */
	else if (NULL != element->prev && NULL != element->next) {
		element->prev->next = element->next;
		element->next->prev = element->prev;
	}

	RASTER_DEBUGF(4, "qle @ %p destroyed", element);
	rtdealloc(element);

	return 1;
}

int quantile_llist_destroy(struct quantile_llist **list, uint32_t list_count) {
	struct quantile_llist_element *element = NULL;
	uint32_t i;

	if (NULL == *list) return 0;

	for (i = 0; i < list_count; i++) {
		element = (*list)[i].head;
		while (NULL != element->next) {
			quantile_llist_delete(element->next);
		}
		quantile_llist_delete(element);

		rtdealloc((*list)[i].index);
	}

	rtdealloc(*list);
	return 1;
}

static void quantile_llist_index_update(struct quantile_llist *qll, struct quantile_llist_element *qle, uint32_t idx) {
	uint32_t anchor = (uint32_t) floor(idx / 100);

	if (qll->tail == qle) return;

	if (
		(anchor != 0) && (
			NULL == qll->index[anchor].element ||
			idx <= qll->index[anchor].index
		)
	) {
		qll->index[anchor].index = idx;
		qll->index[anchor].element = qle;
	}

	if (anchor != 0 && NULL == qll->index[0].element) {
		qll->index[0].index = 0;
		qll->index[0].element = qll->head;
	}
}

static void quantile_llist_index_delete(struct quantile_llist *qll, struct quantile_llist_element *qle) {
	uint32_t i = 0;

	for (i = 0; i < qll->index_max; i++) {
		if (
			NULL == qll->index[i].element ||
			(qll->index[i].element) != qle
		) {
			continue;
		}

		RASTER_DEBUGF(5, "deleting index: %d => %f", i, qle->value);
		qll->index[i].index = -1;
		qll->index[i].element = NULL;
	}
}

static struct quantile_llist_element *quantile_llist_index_search(
	struct quantile_llist *qll,
	double value,
	uint32_t *index
) {
	uint32_t i = 0, j = 0;
	RASTER_DEBUGF(5, "searching index for value %f", value);

	for (i = 0; i < qll->index_max; i++) {
		if (NULL == qll->index[i].element) {
			if (i < 1) break;
			continue;
		}
		if (value > (qll->index[i]).element->value) continue;

		if (FLT_EQ(value, qll->index[i].element->value)) {
			RASTER_DEBUGF(5, "using index value at %d = %f", i, qll->index[i].element->value);
			*index = i * 100;
			return qll->index[i].element;
		}
		else if (i > 0) {
			for (j = 1; j < i; j++) {
				if (NULL != qll->index[i - j].element) {
					RASTER_DEBUGF(5, "using index value at %d = %f", i - j, qll->index[i - j].element->value);
					*index = (i - j) * 100;
					return qll->index[i - j].element;
				}
			}
		}
	}

	*index = 0;
	return qll->head;
}

static void quantile_llist_index_reset(struct quantile_llist *qll) {
	uint32_t i = 0;

	RASTER_DEBUG(5, "resetting index");
	for (i = 0; i < qll->index_max; i++) {
		qll->index[i].index = -1;
		qll->index[i].element = NULL;
	}
}

/**
 * Compute the default set of or requested quantiles for a coverage
 *
 * This function is based upon the algorithm described in:
 *
 * A One-Pass Space-Efficient Algorithm for Finding Quantiles (1995)
 *   by Rakesh Agrawal, Arun Swami
 *   in Proc. 7th Intl. Conf. Management of Data (COMAD-95)
 *
 * http://www.almaden.ibm.com/cs/projects/iis/hdb/Publications/papers/comad95.pdf
 *
 * In the future, it may be worth exploring algorithms that don't
 *   require the size of the coverage
 *
 * @param band : the band to include in the quantile search
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * @param sample : percentage of pixels to sample
 * @param cov_count : number of values in coverage
 * @param qlls : set of quantile_llist structures
 * @param qlls_count : the number of quantile_llist structures
 * @param quantiles : the quantiles to be computed
 *   if bot qlls and quantiles provided, qlls is used
 * @param quantiles_count : the number of quantiles to be computed
 * @param rtn_count : the number of quantiles being returned
 *
 * @return the default set of or requested quantiles for a band or NULL
 */
rt_quantile
rt_band_get_quantiles_stream(
	rt_band band,
	int exclude_nodata_value, double sample,
	uint64_t cov_count,
	struct quantile_llist **qlls, uint32_t *qlls_count,
	double *quantiles, int quantiles_count,
	uint32_t *rtn_count
) {
	rt_quantile rtn = NULL;
	int init_quantiles = 0;

	struct quantile_llist *qll = NULL;
	struct quantile_llist_element *qle = NULL;
	struct quantile_llist_element *qls = NULL;
	const uint32_t MAX_VALUES = 750;

	uint8_t *data = NULL;
	double value;
	int isnodata = 0;

	uint32_t a = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t z = 0;
	uint32_t idx = 0;
	uint32_t offset = 0;
	uint32_t diff = 0;
	uint8_t exists = 0;

	uint32_t do_sample = 0;
	uint32_t sample_size = 0;
	uint32_t sample_per = 0;
	uint32_t sample_int = 0;
	int status;

	RASTER_DEBUG(3, "starting");

	assert(NULL != band);
	assert(cov_count > 1);
	assert(NULL != rtn_count);
	RASTER_DEBUGF(3, "cov_count = %d", cov_count);

	data = rt_band_get_data(band);
	if (data == NULL) {
		rterror("rt_band_get_summary_stats: Cannot get band data");
		return NULL;
	}

	if (!rt_band_get_hasnodata_flag(band))
		exclude_nodata_value = 0;
	RASTER_DEBUGF(3, "exclude_nodata_value = %d", exclude_nodata_value);

	/* quantile_llist not provided */
	if (NULL == *qlls) {
		/* quantiles not provided */
		if (NULL == quantiles) {
			/* quantile count not specified, default to quartiles */
			if (quantiles_count < 2)
				quantiles_count = 5;

			quantiles = rtalloc(sizeof(double) * quantiles_count);
			init_quantiles = 1;
			if (NULL == quantiles) {
				rterror("rt_band_get_quantiles_stream: Could not allocate memory for quantile input");
				return NULL;
			}

			quantiles_count--;
			for (i = 0; i <= quantiles_count; i++)
				quantiles[i] = ((double) i) / quantiles_count;
			quantiles_count++;
		}

		/* check quantiles */
		for (i = 0; i < quantiles_count; i++) {
			if (quantiles[i] < 0. || quantiles[i] > 1.) {
				rterror("rt_band_get_quantiles_stream: Quantile value not between 0 and 1");
				if (init_quantiles) rtdealloc(quantiles);
				return NULL;
			}
		}
		quicksort(quantiles, quantiles + quantiles_count - 1);

		/* initialize linked-list set */
		*qlls_count = quantiles_count * 2;
		RASTER_DEBUGF(4, "qlls_count = %d", *qlls_count);
		*qlls = rtalloc(sizeof(struct quantile_llist) * *qlls_count);
		if (NULL == *qlls) {
			rterror("rt_band_get_quantiles_stream: Could not allocate memory for quantile output");
			if (init_quantiles) rtdealloc(quantiles);
			return NULL;
		}

		j = (uint32_t) floor(MAX_VALUES / 100.) + 1;
		for (i = 0; i < *qlls_count; i++) {
			qll = &((*qlls)[i]);
			qll->quantile = quantiles[(i * quantiles_count) / *qlls_count];
			qll->count = 0;
			qll->sum1 = 0;
			qll->sum2 = 0;
			qll->head = NULL;
			qll->tail = NULL;

			/* initialize index */
			qll->index = rtalloc(sizeof(struct quantile_llist_index) * j);
			if (NULL == qll->index) {
				rterror("rt_band_get_quantiles_stream: Could not allocate memory for quantile output");
				if (init_quantiles) rtdealloc(quantiles);
				return NULL;
			}
			qll->index_max = j;
			quantile_llist_index_reset(qll);

			/* AL-GEQ */
			if (!(i % 2)) {
				qll->algeq = 1;
				qll->tau = (uint64_t) ROUND(cov_count - (cov_count * qll->quantile), 0);
				if (qll->tau < 1) qll->tau = 1;
			}
			/* AL-GT */
			else {
				qll->algeq = 0;
				qll->tau = cov_count - (*qlls)[i - 1].tau + 1;
			}

			RASTER_DEBUGF(4, "qll init: (algeq, quantile, count, tau, sum1, sum2) = (%d, %f, %d, %d, %d, %d)",
				qll->algeq, qll->quantile, qll->count, qll->tau, qll->sum1, qll->sum2);
			RASTER_DEBUGF(4, "qll init: (head, tail) = (%p, %p)", qll->head, qll->tail);
		}

		if (init_quantiles) rtdealloc(quantiles);
	}

	/* clamp percentage */
	if (
		(sample < 0 || FLT_EQ(sample, 0.0)) ||
		(sample > 1 || FLT_EQ(sample, 1.0))
	) {
		do_sample = 0;
		sample = 1;
	}
	else
		do_sample = 1;
	RASTER_DEBUGF(3, "do_sample = %d", do_sample);

	/* sample all pixels */
	if (!do_sample) {
		sample_size = band->width * band->height;
		sample_per = band->height;
	}
	/*
	 randomly sample a percentage of available pixels
	 sampling method is known as
	 	"systematic random sample without replacement"
	*/
	else {
		sample_size = round((band->width * band->height) * sample);
		sample_per = round(sample_size / band->width);
		sample_int = round(band->height / sample_per);
		srand(time(NULL));
	}
	RASTER_DEBUGF(3, "sampling %d of %d available pixels w/ %d per set"
		, sample_size, (band->width * band->height), sample_per);

	for (x = 0, j = 0, k = 0; x < band->width; x++) {
		y = -1;
		diff = 0;

		/* exclude_nodata_value = TRUE and band is NODATA */
		if (exclude_nodata_value && rt_band_get_isnodata_flag(band)) {
			RASTER_DEBUG(3, "Skipping quantile calcuation as band is NODATA");
			break;
		}

		for (i = 0, z = 0; i < sample_per; i++) {
			if (do_sample != 1)
				y = i;
			else {
				offset = (rand() % sample_int) + 1;
				y += diff + offset;
				diff = sample_int - offset;
			}
			RASTER_DEBUGF(5, "(x, y, z) = (%d, %d, %d)", x, y, z);
			if (y >= band->height || z > sample_per) break;

			status = rt_band_get_pixel(band, x, y, &value, &isnodata);

			j++;
			if (status == ES_NONE && (!exclude_nodata_value || (exclude_nodata_value && !isnodata))) {

				/* process each quantile */
				for (a = 0; a < *qlls_count; a++) {
					qll = &((*qlls)[a]);
					qls = NULL;
					RASTER_DEBUGF(4, "%d of %d (%f)", a + 1, *qlls_count, qll->quantile);
					RASTER_DEBUGF(5, "qll before: (algeq, quantile, count, tau, sum1, sum2) = (%d, %f, %d, %d, %d, %d)",
						qll->algeq, qll->quantile, qll->count, qll->tau, qll->sum1, qll->sum2);
					RASTER_DEBUGF(5, "qll before: (head, tail) = (%p, %p)", qll->head, qll->tail);

					/* OPTIMIZATION: shortcuts for quantiles of zero or one */
					if (FLT_EQ(qll->quantile, 0.)) {
						if (NULL != qll->head) {
							if (value < qll->head->value)
								qll->head->value = value;
						}
						else {
							qle = quantile_llist_insert(qll->head, value, NULL);
							qll->head = qle;
							qll->tail = qle;
							qll->count = 1;
						}

						RASTER_DEBUGF(4, "quantile shortcut for %f\n\n", qll->quantile);
						continue;
					}
					else if (FLT_EQ(qll->quantile, 1.)) {
						if (NULL != qll->head) {
							if (value > qll->head->value)
								qll->head->value = value;
						}
						else {
							qle = quantile_llist_insert(qll->head, value, NULL);
							qll->head = qle;
							qll->tail = qle;
							qll->count = 1;
						}

						RASTER_DEBUGF(4, "quantile shortcut for %f\n\n", qll->quantile);
						continue;
					}

					/* value exists in list */
					/* OPTIMIZATION: check to see if value exceeds last value */
					if (NULL != qll->tail && value > qll->tail->value)
						qle = NULL;
					/* OPTIMIZATION: check to see if value equals last value */
					else if (NULL != qll->tail && FLT_EQ(value, qll->tail->value))
						qle = qll->tail;
					/* OPTIMIZATION: use index if possible */
					else {
						qls = quantile_llist_index_search(qll, value, &idx);
						qle = quantile_llist_search(qls, value);
					}

					/* value found */
					if (NULL != qle) {
						RASTER_DEBUGF(4, "%f found in list", value);
						RASTER_DEBUGF(5, "qle before: (value, count) = (%f, %d)", qle->value, qle->count);

						qle->count++;
						qll->sum1++;

						if (qll->algeq)
							qll->sum2 = qll->sum1 - qll->head->count;
						else
							qll->sum2 = qll->sum1 - qll->tail->count;

						RASTER_DEBUGF(4, "qle after: (value, count) = (%f, %d)", qle->value, qle->count);
					}
					/* can still add element */
					else if (qll->count < MAX_VALUES) {
						RASTER_DEBUGF(4, "Adding %f to list", value);

						/* insert */
						/* OPTIMIZATION: check to see if value exceeds last value */
						if (NULL != qll->tail && (value > qll->tail->value || FLT_EQ(value, qll->tail->value))) {
							idx = qll->count;
							qle = quantile_llist_insert(qll->tail, value, &idx);
						}
						/* OPTIMIZATION: use index if possible */
						else
							qle = quantile_llist_insert(qls, value, &idx);
						if (NULL == qle) return NULL;
						RASTER_DEBUGF(5, "value added at index: %d => %f", idx, value);
						qll->count++;
						qll->sum1++;

						/* first element */
						if (NULL == qle->prev)
							qll->head = qle;
						/* last element */
						if (NULL == qle->next)
							qll->tail = qle;

						if (qll->algeq)
							qll->sum2 = qll->sum1 - qll->head->count;
						else
							qll->sum2 = qll->sum1 - qll->tail->count;

						/* index is only needed if there are at least 100 values */
						quantile_llist_index_update(qll, qle, idx);

						RASTER_DEBUGF(5, "qle, prev, next, head, tail = %p, %p, %p, %p, %p", qle, qle->prev, qle->next, qll->head, qll->tail);
					}
					/* AL-GEQ */
					else if (qll->algeq) {
						RASTER_DEBUGF(4, "value, head->value = %f, %f", value, qll->head->value);

						if (value < qll->head->value) {
							/* ignore value if test isn't true */
							if (qll->sum1 >= qll->tau) {
								RASTER_DEBUGF(4, "skipping %f", value);
							}
							else {

								/* delete last element */
								RASTER_DEBUGF(4, "deleting %f from list", qll->tail->value);
								qle = qll->tail->prev;
								RASTER_DEBUGF(5, "to-be tail is %f with count %d", qle->value, qle->count);
								qle->count += qll->tail->count;
								quantile_llist_index_delete(qll, qll->tail);
								quantile_llist_delete(qll->tail);
								qll->tail = qle;
								qll->count--;
								RASTER_DEBUGF(5, "tail is %f with count %d", qll->tail->value, qll->tail->count);

								/* insert value */
								RASTER_DEBUGF(4, "adding %f to list", value);
								/* OPTIMIZATION: check to see if value exceeds last value */
								if (NULL != qll->tail && (value > qll->tail->value || FLT_EQ(value, qll->tail->value))) {
									idx = qll->count;
									qle = quantile_llist_insert(qll->tail, value, &idx);
								}
								/* OPTIMIZATION: use index if possible */
								else {
									qls = quantile_llist_index_search(qll, value, &idx);
									qle = quantile_llist_insert(qls, value, &idx);
								}
								if (NULL == qle) return NULL;
								RASTER_DEBUGF(5, "value added at index: %d => %f", idx, value);
								qll->count++;
								qll->sum1++;

								/* first element */
								if (NULL == qle->prev)
									qll->head = qle;
								/* last element */
								if (NULL == qle->next)
									qll->tail = qle;

								qll->sum2 = qll->sum1 - qll->head->count;

								quantile_llist_index_update(qll, qle, idx);

								RASTER_DEBUGF(5, "qle, head, tail = %p, %p, %p", qle, qll->head, qll->tail);

							}
						}
						else {
							qle = qll->tail;
							while (NULL != qle) {
								if (qle->value < value) {
									qle->count++;
									qll->sum1++;
									qll->sum2 = qll->sum1 - qll->head->count;
									RASTER_DEBUGF(4, "incremented count of %f by 1 to %d", qle->value, qle->count);
									break;
								}

								qle = qle->prev;
							}
						}
					}
					/* AL-GT */
					else {
						RASTER_DEBUGF(4, "value, tail->value = %f, %f", value, qll->tail->value);

						if (value > qll->tail->value) {
							/* ignore value if test isn't true */
							if (qll->sum1 >= qll->tau) {
								RASTER_DEBUGF(4, "skipping %f", value);
							}
							else {

								/* delete last element */
								RASTER_DEBUGF(4, "deleting %f from list", qll->head->value);
								qle = qll->head->next;
								RASTER_DEBUGF(5, "to-be tail is %f with count %d", qle->value, qle->count);
								qle->count += qll->head->count;
								quantile_llist_index_delete(qll, qll->head);
								quantile_llist_delete(qll->head);
								qll->head = qle;
								qll->count--;
								quantile_llist_index_update(qll, NULL, 0);
								RASTER_DEBUGF(5, "tail is %f with count %d", qll->head->value, qll->head->count);

								/* insert value */
								RASTER_DEBUGF(4, "adding %f to list", value);
								/* OPTIMIZATION: check to see if value exceeds last value */
								if (NULL != qll->tail && (value > qll->tail->value || FLT_EQ(value, qll->tail->value))) {
									idx = qll->count;
									qle = quantile_llist_insert(qll->tail, value, &idx);
								}
								/* OPTIMIZATION: use index if possible */
								else {
									qls = quantile_llist_index_search(qll, value, &idx);
									qle = quantile_llist_insert(qls, value, &idx);
								}
								if (NULL == qle) return NULL;
								RASTER_DEBUGF(5, "value added at index: %d => %f", idx, value);
								qll->count++;
								qll->sum1++;

								/* first element */
								if (NULL == qle->prev)
									qll->head = qle;
								/* last element */
								if (NULL == qle->next)
									qll->tail = qle;

								qll->sum2 = qll->sum1 - qll->tail->count;

								quantile_llist_index_update(qll, qle, idx);

								RASTER_DEBUGF(5, "qle, head, tail = %p, %p, %p", qle, qll->head, qll->tail);

							}
						}
						else {
							qle = qll->head;
							while (NULL != qle) {
								if (qle->value > value) {
									qle->count++;
									qll->sum1++;
									qll->sum2 = qll->sum1 - qll->tail->count;
									RASTER_DEBUGF(4, "incremented count of %f by 1 to %d", qle->value, qle->count);
									break;
								}

								qle = qle->next;
							}
						}
					}

					RASTER_DEBUGF(5, "sum2, tau = %d, %d", qll->sum2, qll->tau);
					if (qll->sum2 >= qll->tau) {
						/* AL-GEQ */
						if (qll->algeq) {
							RASTER_DEBUGF(4, "deleting first element %f from list", qll->head->value);

							if (NULL != qll->head->next) {
								qle = qll->head->next;
								qll->sum1 -= qll->head->count;
								qll->sum2 = qll->sum1 - qle->count;
								quantile_llist_index_delete(qll, qll->head);
								quantile_llist_delete(qll->head);
								qll->head = qle;
								qll->count--;

								quantile_llist_index_update(qll, NULL, 0);
							}
							else {
								quantile_llist_delete(qll->head);
								qll->head = NULL;
								qll->tail = NULL;
								qll->sum1 = 0;
								qll->sum2 = 0;
								qll->count = 0;

								quantile_llist_index_reset(qll);
							}
						}
						/* AL-GT */
						else {
							RASTER_DEBUGF(4, "deleting first element %f from list", qll->tail->value);

							if (NULL != qll->tail->prev) {
								qle = qll->tail->prev;
								qll->sum1 -= qll->tail->count;
								qll->sum2 = qll->sum1 - qle->count;
								quantile_llist_index_delete(qll, qll->tail);
								quantile_llist_delete(qll->tail);
								qll->tail = qle;
								qll->count--;
							}
							else {
								quantile_llist_delete(qll->tail);
								qll->head = NULL;
								qll->tail = NULL;
								qll->sum1 = 0;
								qll->sum2 = 0;
								qll->count = 0;

								quantile_llist_index_reset(qll);
							}
						}
					}

					RASTER_DEBUGF(5, "qll after: (algeq, quantile, count, tau, sum1, sum2) = (%d, %f, %d, %d, %d, %d)",
						qll->algeq, qll->quantile, qll->count, qll->tau, qll->sum1, qll->sum2);
					RASTER_DEBUGF(5, "qll after: (head, tail) = (%p, %p)\n\n", qll->head, qll->tail);
				}

			}
			else {
				RASTER_DEBUGF(5, "skipping value at (x, y) = (%d, %d)", x, y);
			}

			z++;
		}
	}

	/* process quantiles */
	*rtn_count = *qlls_count / 2;
	rtn = rtalloc(sizeof(struct rt_quantile_t) * *rtn_count);
	if (NULL == rtn) return NULL;

	RASTER_DEBUGF(3, "returning %d quantiles", *rtn_count);
	for (i = 0, k = 0; i < *qlls_count; i++) {
		qll = &((*qlls)[i]);

		exists = 0;
		for (x = 0; x < k; x++) {
			if (FLT_EQ(qll->quantile, rtn[x].quantile)) {
				exists = 1;
				break;
			}
		}
		if (exists) continue;

		RASTER_DEBUGF(5, "qll: (algeq, quantile, count, tau, sum1, sum2) = (%d, %f, %d, %d, %d, %d)",
			qll->algeq, qll->quantile, qll->count, qll->tau, qll->sum1, qll->sum2);
		RASTER_DEBUGF(5, "qll: (head, tail) = (%p, %p)", qll->head, qll->tail);

		rtn[k].quantile = qll->quantile;
		rtn[k].has_value = 0;

		/* check that qll->head and qll->tail have value */
		if (qll->head == NULL || qll->tail == NULL)
			continue;

		/* AL-GEQ */
		if (qll->algeq)
			qle = qll->head;
		/* AM-GT */
		else
			qle = qll->tail;

		exists = 0;
		for (j = i + 1; j < *qlls_count; j++) {
			if (FLT_EQ((*qlls)[j].quantile, qll->quantile)) {

				RASTER_DEBUGF(5, "qlls[%d]: (algeq, quantile, count, tau, sum1, sum2) = (%d, %f, %d, %d, %d, %d)",
					j, (*qlls)[j].algeq, (*qlls)[j].quantile, (*qlls)[j].count, (*qlls)[j].tau, (*qlls)[j].sum1, (*qlls)[j].sum2);
				RASTER_DEBUGF(5, "qlls[%d]: (head, tail) = (%p, %p)", j, (*qlls)[j].head, (*qlls)[j].tail);

				exists = 1;
				break;
			}
		}

		/* weighted average for quantile */
		if (exists) {
			if ((*qlls)[j].algeq) {
				rtn[k].value = ((qle->value * qle->count) + ((*qlls)[j].head->value * (*qlls)[j].head->count)) / (qle->count + (*qlls)[j].head->count);
				RASTER_DEBUGF(5, "qlls[%d].head: (value, count) = (%f, %d)", j, (*qlls)[j].head->value, (*qlls)[j].head->count);
			}
			else {
				rtn[k].value = ((qle->value * qle->count) + ((*qlls)[j].tail->value * (*qlls)[j].tail->count)) / (qle->count + (*qlls)[j].tail->count);
				RASTER_DEBUGF(5, "qlls[%d].tail: (value, count) = (%f, %d)", j, (*qlls)[j].tail->value, (*qlls)[j].tail->count);
			}
		}
		/* straight value for quantile */
		else {
			rtn[k].value = qle->value;
		}
		rtn[k].has_value = 1;
		RASTER_DEBUGF(3, "(quantile, value) = (%f, %f)\n\n", rtn[k].quantile, rtn[k].value);

		k++;
	}

	RASTER_DEBUG(3, "done");
	return rtn;
}

/**
 * Count the number of times provided value(s) occur in
 * the band
 *
 * @param band : the band to query for minimum and maximum pixel values
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * @param search_values : array of values to count
 * @param search_values_count : the number of search values
 * @param roundto : the decimal place to round the values to
 * @param rtn_total : the number of pixels examined in the band
 * @param rtn_count : the number of value counts being returned
 *
 * @return the number of times the provide value(s) occur or NULL
 */
rt_valuecount
rt_band_get_value_count(
	rt_band band, int exclude_nodata_value,
	double *search_values, uint32_t search_values_count, double roundto,
	uint32_t *rtn_total, uint32_t *rtn_count
) {
	rt_valuecount vcnts = NULL;
	rt_pixtype pixtype = PT_END;
	uint8_t *data = NULL;
	double nodata = 0;

	int scale = 0;
	int doround = 0;
	double tmpd = 0;
	int i = 0;

	uint32_t x = 0;
	uint32_t y = 0;
	int rtn;
	double pxlval;
	int isnodata = 0;
	double rpxlval;
	uint32_t total = 0;
	int vcnts_count = 0;
	int new_valuecount = 0;

#if POSTGIS_DEBUG_LEVEL > 0
	clock_t start, stop;
	double elapsed = 0;
#endif

	RASTER_DEBUG(3, "starting");
#if POSTGIS_DEBUG_LEVEL > 0
	start = clock();
#endif

	assert(NULL != band);
	assert(NULL != rtn_count);

	data = rt_band_get_data(band);
	if (data == NULL) {
		rterror("rt_band_get_summary_stats: Cannot get band data");
		return NULL;
	}

	pixtype = band->pixtype;

	if (rt_band_get_hasnodata_flag(band)) {
		rt_band_get_nodata(band, &nodata);
		RASTER_DEBUGF(3, "hasnodata, nodataval = 1, %f", nodata);
	}
	else {
		exclude_nodata_value = 0;
		RASTER_DEBUG(3, "hasnodata, nodataval = 0, 0");
	}

	RASTER_DEBUGF(3, "exclude_nodata_value = %d", exclude_nodata_value);

	/* process roundto */
	if (roundto < 0 || FLT_EQ(roundto, 0.0)) {
		roundto = 0;
		scale = 0;
	}
	/* tenths, hundredths, thousandths, etc */
	else if (roundto < 1) {
    switch (pixtype) {
			/* integer band types don't have digits after the decimal place */
			case PT_1BB:
			case PT_2BUI:
			case PT_4BUI:
			case PT_8BSI:
			case PT_8BUI:
			case PT_16BSI:
			case PT_16BUI:
			case PT_32BSI:
			case PT_32BUI:
				roundto = 0;
				break;
			/* floating points, check the rounding */
			case PT_32BF:
			case PT_64BF:
				for (scale = 0; scale <= 20; scale++) {
					tmpd = roundto * pow(10, scale);
					if (FLT_EQ((tmpd - ((int) tmpd)), 0.0)) break;
				}
				break;
			case PT_END:
				break;
		}
	}
	/* ones, tens, hundreds, etc */
	else {
		for (scale = 0; scale >= -20; scale--) {
			tmpd = roundto * pow(10, scale);
			if (tmpd < 1 || FLT_EQ(tmpd, 1.0)) {
				if (scale == 0) doround = 1;
				break;
			}
		}
	}

	if (scale != 0 || doround)
		doround = 1;
	else
		doround = 0;
	RASTER_DEBUGF(3, "scale = %d", scale);
	RASTER_DEBUGF(3, "doround = %d", doround);

	/* process search_values */
	if (search_values_count > 0 && NULL != search_values) {
		vcnts = (rt_valuecount) rtalloc(sizeof(struct rt_valuecount_t) * search_values_count);
		if (NULL == vcnts) {
			rterror("rt_band_get_count_of_values: Could not allocate memory for value counts");
			*rtn_count = 0;
			return NULL;
		}

		for (i = 0; i < search_values_count; i++) {
			vcnts[i].count = 0;
			vcnts[i].percent = 0;
			if (!doround)
				vcnts[i].value = search_values[i];
			else
				vcnts[i].value = ROUND(search_values[i], scale);
		}
		vcnts_count = i;
	}
	else
		search_values_count = 0;
	RASTER_DEBUGF(3, "search_values_count = %d", search_values_count);

	/* entire band is nodata */
	if (rt_band_get_isnodata_flag(band) != FALSE) {
		if (exclude_nodata_value) {
			rtwarn("All pixels of band have the NODATA value");
			return NULL;
		}
		else {
			if (search_values_count > 0) {
				/* check for nodata match */
				for (i = 0; i < search_values_count; i++) {
					if (!doround)
						tmpd = nodata;
					else
						tmpd = ROUND(nodata, scale);

					if (FLT_NEQ(tmpd, vcnts[i].value))
						continue;

					vcnts[i].count = band->width * band->height;
					if (NULL != rtn_total) *rtn_total = vcnts[i].count;
					vcnts->percent = 1.0;
				}

				*rtn_count = vcnts_count;
			}
			/* no defined search values */
			else {
				vcnts = (rt_valuecount) rtalloc(sizeof(struct rt_valuecount_t));
				if (NULL == vcnts) {
					rterror("rt_band_get_count_of_values: Could not allocate memory for value counts");
					*rtn_count = 0;
					return NULL;
				}

				vcnts->value = nodata;
				vcnts->count = band->width * band->height;
				if (NULL != rtn_total) *rtn_total = vcnts[i].count;
				vcnts->percent = 1.0;

				*rtn_count = 1;
			}

			return vcnts;
		}
	}

	for (x = 0; x < band->width; x++) {
		for (y = 0; y < band->height; y++) {
			rtn = rt_band_get_pixel(band, x, y, &pxlval, &isnodata);

			/* error getting value, continue */
			if (rtn != ES_NONE)
				continue;

			if (!exclude_nodata_value || (exclude_nodata_value && !isnodata)) {
				total++;
				if (doround) {
					rpxlval = ROUND(pxlval, scale);
				}
				else
					rpxlval = pxlval;
				RASTER_DEBUGF(5, "(pxlval, rpxlval) => (%0.6f, %0.6f)", pxlval, rpxlval);

				new_valuecount = 1;
				/* search for match in existing valuecounts */
				for (i = 0; i < vcnts_count; i++) {
					/* match found */
					if (FLT_EQ(vcnts[i].value, rpxlval)) {
						vcnts[i].count++;
						new_valuecount = 0;
						RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[i].value, vcnts[i].count);
						break;
					}
				}

				/*
					don't add new valuecount either because
						- no need for new one
						- user-defined search values
				*/
				if (!new_valuecount || search_values_count > 0) continue;

				/* add new valuecount */
				vcnts = rtrealloc(vcnts, sizeof(struct rt_valuecount_t) * (vcnts_count + 1));
				if (NULL == vcnts) {
					rterror("rt_band_get_count_of_values: Could not allocate memory for value counts");
					*rtn_count = 0;
					return NULL;
				}

				vcnts[vcnts_count].value = rpxlval;
				vcnts[vcnts_count].count = 1;
				vcnts[vcnts_count].percent = 0;
				RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[vcnts_count].value, vcnts[vcnts_count].count);
				vcnts_count++;
			}
		}
	}

#if POSTGIS_DEBUG_LEVEL > 0
	stop = clock();
	elapsed = ((double) (stop - start)) / CLOCKS_PER_SEC;
	RASTER_DEBUGF(3, "elapsed time = %0.4f", elapsed);
#endif

	for (i = 0; i < vcnts_count; i++) {
		vcnts[i].percent = (double) vcnts[i].count / total;
		RASTER_DEBUGF(5, "(value, count) => (%0.6f, %d)", vcnts[i].value, vcnts[i].count);
	}

	RASTER_DEBUG(3, "done");
	if (NULL != rtn_total) *rtn_total = total;
	*rtn_count = vcnts_count;
	return vcnts;
}

/**
 * Returns new band with values reclassified
 *
 * @param srcband : the band who's values will be reclassified
 * @param pixtype : pixel type of the new band
 * @param hasnodata : indicates if the band has a nodata value
 * @param nodataval : nodata value for the new band
 * @param exprset : array of rt_reclassexpr structs
 * @param exprcount : number of elements in expr
 *
 * @return a new rt_band or NULL on error
 */
rt_band
rt_band_reclass(
	rt_band srcband, rt_pixtype pixtype,
	uint32_t hasnodata, double nodataval,
	rt_reclassexpr *exprset, int exprcount
) {
	rt_band band = NULL;
	uint32_t width = 0;
	uint32_t height = 0;
	int numval = 0;
	int memsize = 0;
	void *mem = NULL;
	uint32_t src_hasnodata = 0;
	double src_nodataval = 0.0;
	int isnodata = 0;

	int rtn;
	uint32_t x;
	uint32_t y;
	int i;
	double or = 0;
	double ov = 0;
	double nr = 0;
	double nv = 0;
	int do_nv = 0;
	rt_reclassexpr expr = NULL;

	assert(NULL != srcband);
	assert(NULL != exprset && exprcount > 0);
	RASTER_DEBUGF(4, "exprcount = %d", exprcount);
	RASTER_DEBUGF(4, "exprset @ %p", exprset);

	/* source nodata */
	src_hasnodata = rt_band_get_hasnodata_flag(srcband);
	if (src_hasnodata)
		rt_band_get_nodata(srcband, &src_nodataval);

	/* size of memory block to allocate */
	width = rt_band_get_width(srcband);
	height = rt_band_get_height(srcband);
	numval = width * height;
	memsize = rt_pixtype_size(pixtype) * numval;
	mem = (int *) rtalloc(memsize);
	if (!mem) {
		rterror("rt_band_reclass: Could not allocate memory for band");
		return 0;
	}

	/* initialize to zero */
	if (!hasnodata) {
		memset(mem, 0, memsize);
	}
	/* initialize to nodataval */
	else {
		int32_t checkvalint = 0;
		uint32_t checkvaluint = 0;
		double checkvaldouble = 0;
		float checkvalfloat = 0;

		switch (pixtype) {
			case PT_1BB:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_1BB(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_2BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_2BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_4BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_4BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_8BSI:
			{
				int8_t *ptr = mem;
				int8_t clamped_initval = rt_util_clamp_to_8BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_8BUI:
			{
				uint8_t *ptr = mem;
				uint8_t clamped_initval = rt_util_clamp_to_8BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_16BSI:
			{
				int16_t *ptr = mem;
				int16_t clamped_initval = rt_util_clamp_to_16BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_16BUI:
			{
				uint16_t *ptr = mem;
				uint16_t clamped_initval = rt_util_clamp_to_16BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_32BSI:
			{
				int32_t *ptr = mem;
				int32_t clamped_initval = rt_util_clamp_to_32BSI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalint = ptr[0];
				break;
			}
			case PT_32BUI:
			{
				uint32_t *ptr = mem;
				uint32_t clamped_initval = rt_util_clamp_to_32BUI(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvaluint = ptr[0];
				break;
			}
			case PT_32BF:
			{
				float *ptr = mem;
				float clamped_initval = rt_util_clamp_to_32F(nodataval);
				for (i = 0; i < numval; i++)
					ptr[i] = clamped_initval;
				checkvalfloat = ptr[0];
				break;
			}
			case PT_64BF:
			{
				double *ptr = mem;
				for (i = 0; i < numval; i++)
					ptr[i] = nodataval;
				checkvaldouble = ptr[0];
				break;
			}
			default:
			{
				rterror("rt_band_reclass: Unknown pixeltype %d", pixtype);
				rtdealloc(mem);
				return 0;
			}
		}

		/* Overflow checking */
		rt_util_dbl_trunc_warning(
			nodataval,
			checkvalint, checkvaluint,
			checkvalfloat, checkvaldouble,
			pixtype
		);
	}
	RASTER_DEBUGF(3, "rt_band_reclass: width = %d height = %d", width, height);

	band = rt_band_new_inline(width, height, pixtype, hasnodata, nodataval, mem);
	if (!band) {
		rterror("rt_band_reclass: Could not create new band");
		rtdealloc(mem);
		return 0;
	}
	rt_band_set_ownsdata_flag(band, 1); /* we DO own this data!!! */
	RASTER_DEBUGF(3, "rt_band_reclass: new band @ %p", band);

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			rtn = rt_band_get_pixel(srcband, x, y, &ov, &isnodata);

			/* error getting value, skip */
			if (rtn != ES_NONE) {
				RASTER_DEBUGF(3, "Cannot get value at %d, %d", x, y);
				continue;
			}

			do {
				do_nv = 0;

				/* no data*/
				if (hasnodata && isnodata) {
					do_nv = 1;
					break;
				}

				for (i = 0; i < exprcount; i++) {
					expr = exprset[i];

					/* ov matches min and max*/
					if (
						FLT_EQ(expr->src.min, ov) &&
						FLT_EQ(expr->src.max, ov)
					) {
						do_nv = 1;
						break;
					}

					/* process min */
					if ((
						expr->src.exc_min && (
							expr->src.min > ov ||
							FLT_EQ(expr->src.min, ov)
						)) || (
						expr->src.inc_min && (
							expr->src.min < ov ||
							FLT_EQ(expr->src.min, ov)
						)) || (
						expr->src.min < ov
					)) {
						/* process max */
						if ((
							expr->src.exc_max && (
								ov > expr->src.max ||
								FLT_EQ(expr->src.max, ov)
							)) || (
								expr->src.inc_max && (
								ov < expr->src.max ||
								FLT_EQ(expr->src.max, ov)
							)) || (
							ov < expr->src.max
						)) {
							do_nv = 1;
							break;
						}
					}
				}
			}
			while (0);

			/* no expression matched, do not continue */
			if (!do_nv) continue;
			RASTER_DEBUGF(3, "Using exprset[%d] unless NODATA", i);

			/* converting a value from one range to another range
			OldRange = (OldMax - OldMin)
			NewRange = (NewMax - NewMin)
			NewValue = (((OldValue - OldMin) * NewRange) / OldRange) + NewMin
			*/

			/* NODATA */
			if (hasnodata && isnodata) {
				nv = nodataval;
			}
			/*
				"src" min and max is the same, prevent division by zero
				set nv to "dst" min, which should be the same as "dst" max
			*/
			else if (FLT_EQ(expr->src.max, expr->src.min)) {
				nv = expr->dst.min;
			}
			else {
				or = expr->src.max - expr->src.min;
				nr = expr->dst.max - expr->dst.min;
				nv = (((ov - expr->src.min) * nr) / or) + expr->dst.min;

				/* if dst range is from high to low */
				if (expr->dst.min > expr->dst.max) {
					if (nv > expr->dst.min)
						nv = expr->dst.min;
					else if (nv < expr->dst.max)
						nv = expr->dst.max;
				}
				/* if dst range is from low to high */
				else {
					if (nv < expr->dst.min)
						nv = expr->dst.min;
					else if (nv > expr->dst.max)
						nv = expr->dst.max;
				}
			}

			/* round the value for integers */
			switch (pixtype) {
				case PT_1BB:
				case PT_2BUI:
				case PT_4BUI:
				case PT_8BSI:
				case PT_8BUI:
				case PT_16BSI:
				case PT_16BUI:
				case PT_32BSI:
				case PT_32BUI:
					nv = round(nv);
					break;
				default:
					break;
			}

			RASTER_DEBUGF(4, "(%d, %d) ov: %f or: %f - %f nr: %f - %f nv: %f"
				, x
				, y
				, ov
				, (NULL != expr) ? expr->src.min : 0
				, (NULL != expr) ? expr->src.max : 0
				, (NULL != expr) ? expr->dst.min : 0
				, (NULL != expr) ? expr->dst.max : 0
				, nv
			);
			if (rt_band_set_pixel(band, x, y, nv, NULL) != ES_NONE) {
				rterror("rt_band_reclass: Could not assign value to new band");
				rt_band_destroy(band);
				rtdealloc(mem);
				return 0;
			}

			expr = NULL;
		}
	}

	return band;
}

/*- rt_raster --------------------------------------------------------*/

/**
 * Construct a raster with given dimensions.
 *
 * Transform will be set to identity.
 * Will contain no bands.
 *
 * @param width : number of pixel columns
 * @param height : number of pixel rows
 *
 * @return an rt_raster or NULL if out of memory
 */
rt_raster
rt_raster_new(uint32_t width, uint32_t height) {
	rt_raster ret = NULL;

	ret = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
	if (!ret) {
		rterror("rt_raster_new: Out of virtual memory creating an rt_raster");
		return NULL;
	}

	RASTER_DEBUGF(3, "Created rt_raster @ %p", ret);

	if (width > 65535 || height > 65535) {
		rterror("rt_raster_new: Dimensions requested exceed the maximum (65535 x 65535) permitted for a raster");
		rt_raster_destroy(ret);
		return NULL;
	}

	ret->width = width;
	ret->height = height;
	ret->scaleX = 1;
	ret->scaleY = -1;
	ret->ipX = 0.0;
	ret->ipY = 0.0;
	ret->skewX = 0.0;
	ret->skewY = 0.0;
	ret->srid = SRID_UNKNOWN;

	ret->numBands = 0;
	ret->bands = NULL; 

	return ret;
}

void
rt_raster_destroy(rt_raster raster) {
	if (raster == NULL)
		return;

	RASTER_DEBUGF(3, "Destroying rt_raster @ %p", raster);

	if (raster->bands)
		rtdealloc(raster->bands);

	rtdealloc(raster);
}

static void
_rt_raster_geotransform_warn_offline_band(rt_raster raster) {
	int numband = 0;
	int i = 0;
	rt_band band = NULL;

	if (raster == NULL)
		return;

	numband = rt_raster_get_num_bands(raster);
	if (numband < 1)
		return;

	for (i = 0; i < numband; i++) {
		band = rt_raster_get_band(raster, i);
		if (NULL == band)
			continue;

		if (!rt_band_is_offline(band))
			continue;

		rtwarn("Changes made to raster geotransform matrix may affect out-db band data. Returned band data may be incorrect");
		break;
	}
}

uint16_t
rt_raster_get_width(rt_raster raster) {

    assert(NULL != raster);

    return raster->width;
}

uint16_t
rt_raster_get_height(rt_raster raster) {

    assert(NULL != raster);

    return raster->height;
}

void
rt_raster_set_scale(
	rt_raster raster,
	double scaleX, double scaleY
) {
	assert(NULL != raster);

	raster->scaleX = scaleX;
	raster->scaleY = scaleY;

	_rt_raster_geotransform_warn_offline_band(raster);
}

double
rt_raster_get_x_scale(rt_raster raster) {


    assert(NULL != raster);

    return raster->scaleX;
}

double
rt_raster_get_y_scale(rt_raster raster) {


    assert(NULL != raster);

    return raster->scaleY;
}

void
rt_raster_set_skews(
	rt_raster raster,
	double skewX, double skewY
) {
	assert(NULL != raster);

	raster->skewX = skewX;
	raster->skewY = skewY;

	_rt_raster_geotransform_warn_offline_band(raster);
}

double
rt_raster_get_x_skew(rt_raster raster) {


    assert(NULL != raster);

    return raster->skewX;
}

double
rt_raster_get_y_skew(rt_raster raster) {


    assert(NULL != raster);

    return raster->skewY;
}

void
rt_raster_set_offsets(
	rt_raster raster,
	double x, double y
) {

	assert(NULL != raster);

	raster->ipX = x;
	raster->ipY = y;

	_rt_raster_geotransform_warn_offline_band(raster);
}

double
rt_raster_get_x_offset(rt_raster raster) {


    assert(NULL != raster);

    return raster->ipX;
}

double
rt_raster_get_y_offset(rt_raster raster) {


    assert(NULL != raster);

    return raster->ipY;
}

void
rt_raster_get_phys_params(rt_raster rast,
        double *i_mag, double *j_mag, double *theta_i, double *theta_ij)
{
    double o11, o12, o21, o22 ; /* geotransform coefficients */

    if (rast == NULL) return ;
    if ( (i_mag==NULL) || (j_mag==NULL) || (theta_i==NULL) || (theta_ij==NULL))
        return ;

    /* retrieve coefficients from raster */
    o11 = rt_raster_get_x_scale(rast) ;
    o12 = rt_raster_get_x_skew(rast) ;
    o21 = rt_raster_get_y_skew(rast) ;
    o22 = rt_raster_get_y_scale(rast) ;

    rt_raster_calc_phys_params(o11, o12, o21, o22, i_mag, j_mag, theta_i, theta_ij);
}

void
rt_raster_calc_phys_params(double xscale, double xskew, double yskew, double yscale,
                           double *i_mag, double *j_mag, double *theta_i, double *theta_ij)

{
    double theta_test ;

    if ( (i_mag==NULL) || (j_mag==NULL) || (theta_i==NULL) || (theta_ij==NULL))
        return ;

    /* pixel size in the i direction */
    *i_mag = sqrt(xscale*xscale + yskew*yskew) ;

    /* pixel size in the j direction */
    *j_mag = sqrt(xskew*xskew + yscale*yscale) ;

    /* Rotation
     * ========
     * Two steps:
     * 1] calculate the magnitude of the angle between the x axis and
     *     the i basis vector.
     * 2] Calculate the sign of theta_i based on the angle between the y axis
     *     and the i basis vector.
     */
    *theta_i = acos(xscale/(*i_mag)) ;  /* magnitude */
    theta_test = acos(yskew/(*i_mag)) ; /* sign */
    if (theta_test < M_PI_2){
        *theta_i = -(*theta_i) ;
    }


    /* Angular separation of basis vectors
     * ===================================
     * Two steps:
     * 1] calculate the magnitude of the angle between the j basis vector and
     *     the i basis vector.
     * 2] Calculate the sign of theta_ij based on the angle between the
     *    perpendicular of the i basis vector and the j basis vector.
     */
    *theta_ij = acos(((xscale*xskew) + (yskew*yscale))/((*i_mag)*(*j_mag))) ;
    theta_test = acos( ((-yskew*xskew)+(xscale*yscale)) /
            ((*i_mag)*(*j_mag)));
    if (theta_test > M_PI_2) {
        *theta_ij = -(*theta_ij) ;
    }
}

void
rt_raster_set_phys_params(rt_raster rast,double i_mag, double j_mag, double theta_i, double theta_ij)
{
    double o11, o12, o21, o22 ; /* calculated geotransform coefficients */
    int success ;

    if (rast == NULL) return ;

    success = rt_raster_calc_gt_coeff(i_mag, j_mag, theta_i, theta_ij,
                            &o11, &o12, &o21, &o22) ;

    if (success) {
        rt_raster_set_scale(rast, o11, o22) ;
        rt_raster_set_skews(rast, o12, o21) ;
    }
}

int
rt_raster_calc_gt_coeff(double i_mag, double j_mag, double theta_i, double theta_ij,
                        double *xscale, double *xskew, double *yskew, double *yscale)
{
    double f ;        /* reflection flag 1.0 or -1.0 */
    double k_i ;      /* shearing coefficient */
    double s_i, s_j ; /* scaling coefficients */
    double cos_theta_i, sin_theta_i ;

    if ( (xscale==NULL) || (xskew==NULL) || (yskew==NULL) || (yscale==NULL)) {
        return 0;
    }

    if ( (theta_ij == 0.0) || (theta_ij == M_PI)) {
        return 0;
    }

    /* Reflection across the i axis */
    f=1.0 ;
    if (theta_ij < 0) {
        f = -1.0;
    }

    /* scaling along i axis */
    s_i = i_mag ;

    /* shearing parallel to i axis */
    k_i = tan(f*M_PI_2 - theta_ij) ;

    /* scaling along j axis */
    s_j = j_mag / (sqrt(k_i*k_i + 1)) ;

    /* putting it altogether */
    cos_theta_i = cos(theta_i) ;
    sin_theta_i = sin(theta_i) ;
    *xscale = s_i * cos_theta_i ;
    *xskew  = k_i * s_j * f * cos_theta_i + s_j * f * sin_theta_i ;
    *yskew  = -s_i * sin_theta_i ;
    *yscale = -k_i * s_j * f * sin_theta_i + s_j * f * cos_theta_i ;
    return 1;
}

int32_t
rt_raster_get_srid(rt_raster raster) {
	assert(NULL != raster);

	return clamp_srid(raster->srid);
}

void
rt_raster_set_srid(rt_raster raster, int32_t srid) {
	assert(NULL != raster);

	raster->srid = clamp_srid(srid);

	_rt_raster_geotransform_warn_offline_band(raster);
}

int
rt_raster_get_num_bands(rt_raster raster) {


    assert(NULL != raster);

    return raster->numBands;
}

rt_band
rt_raster_get_band(rt_raster raster, int n) {
	assert(NULL != raster);

	if (n >= raster->numBands || n < 0)
		return NULL;

	return raster->bands[n];
}

/**
 * Add band data to a raster.
 *
 * @param raster : the raster to add a band to
 * @param band : the band to add, ownership left to caller.
 *               Band dimensions are required to match with raster ones.
 * @param index : the position where to insert the new band (0 based)
 *
 * @return identifier (position) for the just-added raster, or -1 on error
 */
int
rt_raster_add_band(rt_raster raster, rt_band band, int index) {
    rt_band *oldbands = NULL;
    rt_band oldband = NULL;
    rt_band tmpband = NULL;
    uint16_t i = 0;



    assert(NULL != raster);
		assert(NULL != band);

    RASTER_DEBUGF(3, "Adding band %p to raster %p", band, raster);

    if (band->width != raster->width || band->height != raster->height) {
        rterror("rt_raster_add_band: Can't add a %dx%d band to a %dx%d raster",
                band->width, band->height, raster->width, raster->height);
        return -1;
    }

    if (index > raster->numBands)
        index = raster->numBands;

    if (index < 0)
        index = 0;

    oldbands = raster->bands;

    RASTER_DEBUGF(3, "Oldbands at %p", oldbands);

    raster->bands = (rt_band*) rtrealloc(raster->bands,
            sizeof (rt_band)*(raster->numBands + 1)
            );

    RASTER_DEBUG(3, "Checking bands");

    if (NULL == raster->bands) {
        rterror("rt_raster_add_band: Out of virtual memory "
                "reallocating band pointers");
        raster->bands = oldbands;
        return -1;
    }

    RASTER_DEBUGF(4, "realloc returned %p", raster->bands);

    for (i = 0; i <= raster->numBands; ++i) {
        if (i == index) {
            oldband = raster->bands[i];
            raster->bands[i] = band;
        } else if (i > index) {
            tmpband = raster->bands[i];
            raster->bands[i] = oldband;
            oldband = tmpband;
        }
    }

		band->raster = raster;

    raster->numBands++;

    RASTER_DEBUGF(4, "Raster now has %d bands", raster->numBands);

    return index;
}

/**
 * Generate a new inline band and add it to a raster.
 * Memory is allocated in this function for band data.
 *
 * @param raster : the raster to add a band to
 * @param pixtype : the pixel type for the new band
 * @param initialvalue : initial value for pixels
 * @param hasnodata : indicates if the band has a nodata value
 * @param nodatavalue : nodata value for the new band
 * @param index : position to add the new band in the raster
 *
 * @return identifier (position) for the just-added raster, or -1 on error
 */
int
rt_raster_generate_new_band(
	rt_raster raster, rt_pixtype pixtype,
	double initialvalue, uint32_t hasnodata, double nodatavalue,
	int index
) {
    rt_band band = NULL;
    int width = 0;
    int height = 0;
    int numval = 0;
    int datasize = 0;
    int oldnumbands = 0;
    int numbands = 0;
    void * mem = NULL;
    int32_t checkvalint = 0;
    uint32_t checkvaluint = 0;
    double checkvaldouble = 0;
    float checkvalfloat = 0;
    int i;


    assert(NULL != raster);

    /* Make sure index is in a valid range */
    oldnumbands = rt_raster_get_num_bands(raster);
    if (index < 0)
        index = 0;
    else if (index > oldnumbands + 1)
        index = oldnumbands + 1;

    /* Determine size of memory block to allocate and allocate it */
    width = rt_raster_get_width(raster);
    height = rt_raster_get_height(raster);
    numval = width * height;
    datasize = rt_pixtype_size(pixtype) * numval;

    mem = (int *)rtalloc(datasize);
    if (!mem) {
        rterror("rt_raster_generate_new_band: Could not allocate memory for band");
        return -1;
    }

    if (FLT_EQ(initialvalue, 0.0))
        memset(mem, 0, datasize);
    else {
        switch (pixtype)
        {
            case PT_1BB:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_1BB(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_2BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_2BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_4BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_4BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BSI:
            {
                int8_t *ptr = mem;
                int8_t clamped_initval = rt_util_clamp_to_8BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_8BUI:
            {
                uint8_t *ptr = mem;
                uint8_t clamped_initval = rt_util_clamp_to_8BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BSI:
            {
                int16_t *ptr = mem;
                int16_t clamped_initval = rt_util_clamp_to_16BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_16BUI:
            {
                uint16_t *ptr = mem;
                uint16_t clamped_initval = rt_util_clamp_to_16BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BSI:
            {
                int32_t *ptr = mem;
                int32_t clamped_initval = rt_util_clamp_to_32BSI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalint = ptr[0];
                break;
            }
            case PT_32BUI:
            {
                uint32_t *ptr = mem;
                uint32_t clamped_initval = rt_util_clamp_to_32BUI(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvaluint = ptr[0];
                break;
            }
            case PT_32BF:
            {
                float *ptr = mem;
                float clamped_initval = rt_util_clamp_to_32F(initialvalue);
                for (i = 0; i < numval; i++)
                    ptr[i] = clamped_initval;
                checkvalfloat = ptr[0];
                break;
            }
            case PT_64BF:
            {
                double *ptr = mem;
                for (i = 0; i < numval; i++)
                    ptr[i] = initialvalue;
                checkvaldouble = ptr[0];
                break;
            }
            default:
            {
                rterror("rt_raster_generate_new_band: Unknown pixeltype %d", pixtype);
                rtdealloc(mem);
                return -1;
            }
        }
    }

    /* Overflow checking */
    rt_util_dbl_trunc_warning(
			initialvalue,
			checkvalint, checkvaluint,
			checkvalfloat, checkvaldouble,
			pixtype
		);

    band = rt_band_new_inline(width, height, pixtype, hasnodata, nodatavalue, mem);
    if (! band) {
        rterror("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        rtdealloc(mem);
        return -1;
    }
		rt_band_set_ownsdata_flag(band, 1); /* we DO own this data!!! */
    index = rt_raster_add_band(raster, band, index);
    numbands = rt_raster_get_num_bands(raster);
    if (numbands == oldnumbands || index == -1) {
        rterror("rt_raster_generate_new_band: Could not add band to raster. Aborting");
        rt_band_destroy(band);
    }

		/* set isnodata if hasnodata = TRUE and initial value = nodatavalue */
		if (hasnodata && FLT_EQ(initialvalue, nodatavalue))
			rt_band_set_isnodata_flag(band, 1);

    return index;
}

/**
 * Get 6-element array of raster inverse geotransform matrix
 *
 * @param raster : the raster to get matrix of
 * @param gt : optional input parameter, 6-element geotransform matrix
 * @param igt : output parameter, 6-element inverse geotransform matrix
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_get_inverse_geotransform_matrix(
	rt_raster raster,
	double *gt, double *igt
) {
	double _gt[6] = {0};

	assert((raster != NULL || gt != NULL));
	assert(igt != NULL);

	if (gt == NULL)
		rt_raster_get_geotransform_matrix(raster, _gt);
	else
		memcpy(_gt, gt, sizeof(double) * 6);
	
	if (!GDALInvGeoTransform(_gt, igt)) {
		rterror("rt_raster_get_inverse_geotransform_matrix: Could not compute inverse geotransform matrix");
		return ES_ERROR;
	}

	return ES_NONE;
}

/**
 * Get 6-element array of raster geotransform matrix
 *
 * @param raster : the raster to get matrix of
 * @param gt : output parameter, 6-element geotransform matrix
 *
 */
void
rt_raster_get_geotransform_matrix(rt_raster raster,
	double *gt) {
	assert(NULL != raster);
	assert(NULL != gt);

	gt[0] = raster->ipX;
	gt[1] = raster->scaleX;
	gt[2] = raster->skewX;
	gt[3] = raster->ipY;
	gt[4] = raster->skewY;
	gt[5] = raster->scaleY;
}

/**
 * Set raster's geotransform using 6-element array
 *
 * @param raster : the raster to set matrix of
 * @param gt : intput parameter, 6-element geotransform matrix
 *
 */
void
rt_raster_set_geotransform_matrix(rt_raster raster,
	double *gt) {
	assert(NULL != raster);
	assert(NULL != gt);

	raster->ipX = gt[0];
	raster->scaleX = gt[1];
	raster->skewX = gt[2];
	raster->ipY = gt[3];
	raster->skewY = gt[4];
	raster->scaleY = gt[5];

	_rt_raster_geotransform_warn_offline_band(raster);
}

/**
 * Convert an xr, yr raster point to an xw, yw point on map
 *
 * @param raster : the raster to get info from
 * @param xr : the pixel's column
 * @param yr : the pixel's row
 * @param xw : output parameter, X ordinate of the geographical point
 * @param yw : output parameter, Y ordinate of the geographical point
 * @param gt : input/output parameter, 3x2 geotransform matrix
 *
 * @return ES_NONE if success, ES_ERROR if error 
 */
rt_errorstate
rt_raster_cell_to_geopoint(
	rt_raster raster,
	double xr, double yr,
	double *xw, double *yw,
	double *gt
) {
	double _gt[6] = {0};

	assert(NULL != raster);
	assert(NULL != xw && NULL != yw);

	if (NULL != gt)
		memcpy(_gt, gt, sizeof(double) * 6);

	/* scale of matrix is not set */
	if (
		FLT_EQ(_gt[1], 0) ||
		FLT_EQ(_gt[5], 0)
	) {
		rt_raster_get_geotransform_matrix(raster, _gt);
	}

	RASTER_DEBUGF(4, "gt = (%f, %f, %f, %f, %f, %f)",
		_gt[0],
		_gt[1],
		_gt[2],
		_gt[3],
		_gt[4],
		_gt[5]
	);

	GDALApplyGeoTransform(_gt, xr, yr, xw, yw);
	RASTER_DEBUGF(4, "GDALApplyGeoTransform (c -> g) for (%f, %f) = (%f, %f)",
		xr, yr, *xw, *yw);

	return ES_NONE;
}

/**
 * Convert an xw,yw map point to a xr,yr raster point
 *
 * @param raster : the raster to get info from
 * @param xw : X ordinate of the geographical point
 * @param yw : Y ordinate of the geographical point
 * @param xr : output parameter, the pixel's column
 * @param yr : output parameter, the pixel's row
 * @param igt : input/output parameter, inverse geotransform matrix
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_raster_geopoint_to_cell(
	rt_raster raster,
	double xw, double yw,
	double *xr, double *yr,
	double *igt
) {
	double _igt[6] = {0};
	double rnd = 0;

	assert(NULL != raster);
	assert(NULL != xr && NULL != yr);

	if (igt != NULL)
		memcpy(_igt, igt, sizeof(double) * 6);

	/* matrix is not set */
	if (
		FLT_EQ(_igt[0], 0.) &&
		FLT_EQ(_igt[1], 0.) &&
		FLT_EQ(_igt[2], 0.) &&
		FLT_EQ(_igt[3], 0.) &&
		FLT_EQ(_igt[4], 0.) &&
		FLT_EQ(_igt[5], 0.)
	) {
		if (rt_raster_get_inverse_geotransform_matrix(raster, NULL, _igt) != ES_NONE) {
			rterror("rt_raster_geopoint_to_cell: Could not get inverse geotransform matrix");
			return ES_ERROR;
		}
	}

	GDALApplyGeoTransform(_igt, xw, yw, xr, yr);
	RASTER_DEBUGF(4, "GDALApplyGeoTransform (g -> c) for (%f, %f) = (%f, %f)",
		xw, yw, *xr, *yr);

	rnd = ROUND(*xr, 0);
	if (FLT_EQ(rnd, *xr))
		*xr = rnd;
	else
		*xr = floor(*xr);

	rnd = ROUND(*yr, 0);
	if (FLT_EQ(rnd, *yr))
		*yr = rnd;
	else
		*yr = floor(*yr);

	RASTER_DEBUGF(4, "Corrected GDALApplyGeoTransform (g -> c) for (%f, %f) = (%f, %f)",
		xw, yw, *xr, *yr);

	return ES_NONE;
}

/**
 * Returns a set of "geomval" value, one for each group of pixel
 * sharing the same value for the provided band.
 *
 * A "geomval" value is a complex type composed of a geometry 
 * in LWPOLY representation (one for each group of pixel sharing
 * the same value) and the value associated with this geometry.
 *
 * @param raster : the raster to get info from.
 * @param nband : the band to polygonize. 0-based
 * @param exclude_nodata_value : if non-zero, ignore nodata values
 * to check for pixels with value
 *
 * @return A set of "geomval" values, one for each group of pixels
 * sharing the same value for the provided band. The returned values are
 * LWPOLY geometries.
 */
rt_geomval
rt_raster_gdal_polygonize(
	rt_raster raster, int nband,
	int exclude_nodata_value,
	int *pnElements
) {
	CPLErr cplerr = CE_None;
	char *pszQuery;
	long j;
	OGRSFDriverH ogr_drv = NULL;
	GDALDriverH gdal_drv = NULL;
	GDALDatasetH memdataset = NULL;
	GDALRasterBandH gdal_band = NULL;
	OGRDataSourceH memdatasource = NULL;
	rt_geomval pols = NULL;
	OGRLayerH hLayer = NULL;
	OGRFeatureH hFeature = NULL;
	OGRGeometryH hGeom = NULL;
	OGRFieldDefnH hFldDfn = NULL;
	unsigned char *wkb = NULL;
	int wkbsize = 0;
	LWGEOM *lwgeom = NULL;
	int nFeatureCount = 0;
	rt_band band = NULL;
	int iPixVal = -1;
	double dValue = 0.0;
	int iBandHasNodataValue = FALSE;
	double dBandNoData = 0.0;

	/* for checking that a geometry is valid */
	GEOSGeometry *ggeom = NULL;
	int isValid;
	LWGEOM *lwgeomValid = NULL;

	uint32_t bandNums[1] = {nband};
	int excludeNodataValues[1] = {exclude_nodata_value};

	/* checks */
	assert(NULL != raster);
	assert(NULL != pnElements);

	RASTER_DEBUG(2, "In rt_raster_gdal_polygonize");

	*pnElements = 0;

	/*******************************
	 * Get band
	 *******************************/
	band = rt_raster_get_band(raster, nband);
	if (NULL == band) {
		rterror("rt_raster_gdal_polygonize: Error getting band %d from raster", nband);
		return NULL;
	}

	if (exclude_nodata_value) {

		/* band is NODATA */
		if (rt_band_get_isnodata_flag(band)) {
			RASTER_DEBUG(3, "Band is NODATA.  Returning null");
			*pnElements = 0;
			return NULL;
		}

		iBandHasNodataValue = rt_band_get_hasnodata_flag(band);
		if (iBandHasNodataValue)
			rt_band_get_nodata(band, &dBandNoData);
		else
			exclude_nodata_value = FALSE;
	}

	/*****************************************************
	 * Convert raster to GDAL MEM dataset
	 *****************************************************/
	memdataset = rt_raster_to_gdal_mem(raster, NULL, bandNums, excludeNodataValues, 1, &gdal_drv);
	if (NULL == memdataset) {
		rterror("rt_raster_gdal_polygonize: Couldn't convert raster to GDAL MEM dataset");
		return NULL;
	}

	/*****************************
	 * Register ogr mem driver
	 *****************************/
	OGRRegisterAll();

	RASTER_DEBUG(3, "creating OGR MEM vector");

	/*****************************************************
	 * Create an OGR in-memory vector for layers
	 *****************************************************/
	ogr_drv = OGRGetDriverByName("Memory");
	memdatasource = OGR_Dr_CreateDataSource(ogr_drv, "", NULL);
	if (NULL == memdatasource) {
		rterror("rt_raster_gdal_polygonize: Couldn't create a OGR Datasource to store pols");
		GDALClose(memdataset);
		return NULL;
	}

	/* Can MEM driver create new layers? */
	if (!OGR_DS_TestCapability(memdatasource, ODsCCreateLayer)) {
		rterror("rt_raster_gdal_polygonize: MEM driver can't create new layers, aborting");

		/* xxx jorgearevalo: what should we do now? */
		GDALClose(memdataset);
		OGRReleaseDataSource(memdatasource);

		return NULL;
	}

	RASTER_DEBUG(3, "polygonizying GDAL MEM raster band");

	/*****************************
	 * Polygonize the raster band
	 *****************************/

	/**
	 * From GDALPolygonize function header: "Polygon features will be
	 * created on the output layer, with polygon geometries representing
	 * the polygons". So,the WKB geometry type should be "wkbPolygon"
	 **/
	hLayer = OGR_DS_CreateLayer(memdatasource, "PolygonizedLayer", NULL, wkbPolygon, NULL);

	if (NULL == hLayer) {
		rterror("rt_raster_gdal_polygonize: Couldn't create layer to store polygons");

		GDALClose(memdataset);
		OGRReleaseDataSource(memdatasource);

		return NULL;
	}

	/**
	 * Create a new field in the layer, to store the px value
	 */

	/* First, create a field definition to create the field */
	hFldDfn = OGR_Fld_Create("PixelValue", OFTReal);

	/* Second, create the field */
	if (OGR_L_CreateField(hLayer, hFldDfn, TRUE) != OGRERR_NONE) {
		rtwarn("Couldn't create a field in OGR Layer. The polygons generated won't be able to store the pixel value");
		iPixVal = -1;
	}
	else {
		/* Index to the new field created in the layer */
		iPixVal = 0;
	}

	/* Get GDAL raster band */
	gdal_band = GDALGetRasterBand(memdataset, 1);
	if (NULL == gdal_band) {
		rterror("rt_raster_gdal_polygonize: Couldn't get GDAL band to polygonize");

		GDALClose(memdataset);
		OGR_Fld_Destroy(hFldDfn);
		OGR_DS_DeleteLayer(memdatasource, 0);
		OGRReleaseDataSource(memdatasource);
		
		return NULL;
	}

	/**
	 * We don't need a raster mask band. Each band has a nodata value.
	 **/
#ifdef GDALFPOLYGONIZE
	cplerr = GDALFPolygonize(gdal_band, NULL, hLayer, iPixVal, NULL, NULL, NULL);
#else
	cplerr = GDALPolygonize(gdal_band, NULL, hLayer, iPixVal, NULL, NULL, NULL);
#endif

	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_polygonize: Could not polygonize GDAL band");

		GDALClose(memdataset);
		OGR_Fld_Destroy(hFldDfn);
		OGR_DS_DeleteLayer(memdatasource, 0);
		OGRReleaseDataSource(memdatasource);

		return NULL;
	}

	/**
	 * Optimization: Apply a OGR SQL filter to the layer to select the
	 * features different from NODATA value.
	 *
	 * Thanks to David Zwarg.
	 **/
	if (iBandHasNodataValue) {
		pszQuery = (char *) rtalloc(50 * sizeof (char));
		sprintf(pszQuery, "PixelValue != %f", dBandNoData );
		OGRErr e = OGR_L_SetAttributeFilter(hLayer, pszQuery);
		if (e != OGRERR_NONE) {
			rtwarn("Error filtering NODATA values for band. All values will be treated as data values");
		}
	}
	else {
		pszQuery = NULL;
	}

	/*********************************************************************
	 * Transform OGR layers to WKB polygons
	 * XXX jorgearevalo: GDALPolygonize does not set the coordinate system
	 * on the output layer. Application code should do this when the layer
	 * is created, presumably matching the raster coordinate system.
	 *********************************************************************/
	nFeatureCount = OGR_L_GetFeatureCount(hLayer, TRUE);

	/* Allocate memory for pols */
	pols = (rt_geomval) rtalloc(nFeatureCount * sizeof(struct rt_geomval_t));

	if (NULL == pols) {
		rterror("rt_raster_gdal_polygonize: Could not allocate memory for geomval set");

		GDALClose(memdataset);
		OGR_Fld_Destroy(hFldDfn);
		OGR_DS_DeleteLayer(memdatasource, 0);
		if (NULL != pszQuery)
			rtdealloc(pszQuery);
		OGRReleaseDataSource(memdatasource);

		return NULL;
	}

	/* initialize GEOS */
	initGEOS(lwnotice, lwgeom_geos_error);

	RASTER_DEBUGF(3, "storing polygons (%d)", nFeatureCount);

	/* Reset feature reading to start in the first feature */
	OGR_L_ResetReading(hLayer);

	for (j = 0; j < nFeatureCount; j++) {
		hFeature = OGR_L_GetNextFeature(hLayer);
		dValue = OGR_F_GetFieldAsDouble(hFeature, iPixVal);

		hGeom = OGR_F_GetGeometryRef(hFeature);
		wkbsize = OGR_G_WkbSize(hGeom);

		/* allocate wkb buffer */
		wkb = rtalloc(sizeof(unsigned char) * wkbsize);
		if (wkb == NULL) {
			rterror("rt_raster_gdal_polygonize: Could not allocate memory for WKB buffer");

			OGR_F_Destroy(hFeature);
			GDALClose(memdataset);
			OGR_Fld_Destroy(hFldDfn);
			OGR_DS_DeleteLayer(memdatasource, 0);
			if (NULL != pszQuery)
				rtdealloc(pszQuery);
			OGRReleaseDataSource(memdatasource);

			return NULL;
		}

		/* export WKB with LSB byte order */
		OGR_G_ExportToWkb(hGeom, wkbNDR, wkb);

		/* convert WKB to LWGEOM */
		lwgeom = lwgeom_from_wkb(wkb, wkbsize, LW_PARSER_CHECK_NONE);

#if POSTGIS_DEBUG_LEVEL > 3
		{
			char *wkt = NULL;
			OGR_G_ExportToWkt(hGeom, &wkt);
			RASTER_DEBUGF(4, "GDAL wkt = %s", wkt);
			CPLFree(wkt);

			d_print_binary_hex("GDAL wkb", wkb, wkbsize);
		}
#endif

		/* cleanup unnecessary stuff */
		rtdealloc(wkb);
		wkb = NULL;
		wkbsize = 0;

		OGR_F_Destroy(hFeature);

		/* specify SRID */
		lwgeom_set_srid(lwgeom, rt_raster_get_srid(raster));

		/*
			is geometry valid?
			if not, try to make valid
		*/
		do {
#if POSTGIS_GEOS_VERSION < 33
			/* nothing can be done if the geometry was invalid if GEOS < 3.3 */
			break;
#endif

			ggeom = (GEOSGeometry *) LWGEOM2GEOS(lwgeom);
			if (ggeom == NULL) {
				rtwarn("Cannot test geometry for validity");
				break;
			}

			isValid = GEOSisValid(ggeom);

			GEOSGeom_destroy(ggeom);
			ggeom = NULL;

			/* geometry is valid */
			if (isValid)
				break;

			RASTER_DEBUG(3, "fixing invalid geometry");

			/* make geometry valid */
			lwgeomValid = lwgeom_make_valid(lwgeom);
			if (lwgeomValid == NULL) {
				rtwarn("Cannot fix invalid geometry");
				break;
			}

			lwgeom_free(lwgeom);
			lwgeom = lwgeomValid;
		}
		while (0);

		/* save lwgeom */
		pols[j].geom = lwgeom_as_lwpoly(lwgeom);

#if POSTGIS_DEBUG_LEVEL > 3
		{
			char *wkt = lwgeom_to_wkt(lwgeom, WKT_ISO, DBL_DIG, NULL);
			RASTER_DEBUGF(4, "LWGEOM wkt = %s", wkt);
			rtdealloc(wkt);

			size_t lwwkbsize = 0;
			uint8_t *lwwkb = lwgeom_to_wkb(lwgeom, WKB_ISO | WKB_NDR, &lwwkbsize);
			if (lwwkbsize) {
				d_print_binary_hex("LWGEOM wkb", lwwkb, lwwkbsize);
				rtdealloc(lwwkb);
			}
		}
#endif

		/* set pixel value */
		pols[j].val = dValue;
	}

	*pnElements = nFeatureCount;

	RASTER_DEBUG(3, "destroying GDAL MEM raster");
	GDALClose(memdataset);

	RASTER_DEBUG(3, "destroying OGR MEM vector");
	OGR_Fld_Destroy(hFldDfn);
	OGR_DS_DeleteLayer(memdatasource, 0);
	if (NULL != pszQuery) rtdealloc(pszQuery);
	OGRReleaseDataSource(memdatasource);

	return pols;
}

/**
 * Get raster's convex hull.
 *
 * The convex hull is typically a 4 vertices (5 to be closed)
 * single ring polygon bearing the raster's rotation and using
 * projection coordinates.
 *
 * @param raster : the raster to get info from
 * @param **hull : pointer to convex hull
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_raster_get_convex_hull(rt_raster raster, LWGEOM **hull) {
	double gt[6] = {0.0};
	int srid = SRID_UNKNOWN;

	POINTARRAY *pts = NULL;
	POINT4D p4d;

	assert(hull != NULL);
	*hull = NULL;

	/* raster is NULL, convex hull is NULL */
	if (raster == NULL)
		return ES_NONE;

	/* raster metadata */
	srid = rt_raster_get_srid(raster);
	rt_raster_get_geotransform_matrix(raster, gt);

	RASTER_DEBUGF(3, "rt_raster_get_convex_hull: raster is %dx%d", raster->width, raster->height); 

	/* return point or line since at least one of the two dimensions is 0 */
	if ((!raster->width) || (!raster->height)) {
		p4d.x = gt[0];
		p4d.y = gt[3];

		/* return point */
		if (!raster->width && !raster->height) {
			LWPOINT *point = lwpoint_make2d(srid, p4d.x, p4d.y);
			*hull = lwpoint_as_lwgeom(point);
		}
		/* return linestring */
		else {
			LWLINE *line = NULL;
			pts = ptarray_construct_empty(0, 0, 2);

			/* first point of line */
			ptarray_append_point(pts, &p4d, LW_TRUE);

			/* second point of line */
			if (rt_raster_cell_to_geopoint(
				raster,
				rt_raster_get_width(raster), rt_raster_get_height(raster),
				&p4d.x, &p4d.y,
				gt
			) != ES_NONE) {
				rterror("rt_raster_get_convex_hull: Could not get second point for linestring");
				return ES_ERROR;
			}
			ptarray_append_point(pts, &p4d, LW_TRUE);
			line = lwline_construct(srid, NULL, pts);

			*hull = lwline_as_lwgeom(line);
		}

		return ES_NONE;
	}
	else {
		POINTARRAY **rings = NULL;
		LWPOLY* poly = NULL;

		/* only one ring */
		rings = (POINTARRAY **) rtalloc(sizeof (POINTARRAY*));
		if (!rings) {
			rterror("rt_raster_get_convex_hull: Could not allocate memory for polygon ring");
			return ES_ERROR;
		}
		rings[0] = ptarray_construct(0, 0, 5);
		/* TODO: handle error on ptarray construction */
		/* XXX jorgearevalo: the error conditions aren't managed in ptarray_construct */
		if (!rings[0]) {
			rterror("rt_raster_get_convex_hull: Could not construct point array");
			return ES_ERROR;
		}
		pts = rings[0];

		/* Upper-left corner (first and last points) */
		p4d.x = gt[0];
		p4d.y = gt[3];
		ptarray_set_point4d(pts, 0, &p4d);
		ptarray_set_point4d(pts, 4, &p4d);

		/* Upper-right corner (we go clockwise) */
		rt_raster_cell_to_geopoint(
			raster,
			raster->width, 0,
			&p4d.x, &p4d.y,
			gt
		);
		ptarray_set_point4d(pts, 1, &p4d);

		/* Lower-right corner */
		rt_raster_cell_to_geopoint(
			raster,
			raster->width, raster->height,
			&p4d.x, &p4d.y,
			gt
		);
		ptarray_set_point4d(pts, 2, &p4d);

		/* Lower-left corner */
		rt_raster_cell_to_geopoint(
			raster,
			0, raster->height,
			&p4d.x, &p4d.y,
			gt
		);
		ptarray_set_point4d(pts, 3, &p4d);

		poly = lwpoly_construct(srid, 0, 1, rings);
		*hull = lwpoly_as_lwgeom(poly);
	}

	return ES_NONE;
}

/**
 * Get raster's envelope.
 *
 * The envelope is the minimum bounding rectangle of the raster
 *
 * @param raster : the raster to get envelope of
 * @param env : pointer to rt_envelope
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_get_envelope(
	rt_raster raster,
	rt_envelope *env
) {
	int i;
	int rtn;
	int set = 0;
	double _r[2] = {0.};
	double _w[2] = {0.};
	double _gt[6] = {0.};

	assert(raster != NULL);
	assert(env != NULL);

	rt_raster_get_geotransform_matrix(raster, _gt);

	for (i = 0; i < 4; i++) {
		switch (i) {
			case 0:
				_r[0] = 0;
				_r[1] = 0;
				break;
			case 1:
				_r[0] = 0;
				_r[1] = raster->height;
				break;
			case 2:
				_r[0] = raster->width;
				_r[1] = raster->height;
				break;
			case 3:
				_r[0] = raster->width;
				_r[1] = 0;
				break;
		}

		rtn = rt_raster_cell_to_geopoint(
			raster,
			_r[0], _r[1],
			&(_w[0]), &(_w[1]),
			_gt
		);
		if (rtn != ES_NONE) {
			rterror("rt_raster_get_envelope: Could not compute spatial coordinates for raster pixel");
			return ES_ERROR;
		}

		if (!set) {
			set = 1;
			env->MinX = _w[0];
			env->MaxX = _w[0];
			env->MinY = _w[1];
			env->MaxY = _w[1];
		}
		else {
			if (_w[0] < env->MinX)
				env->MinX = _w[0];
			else if (_w[0] > env->MaxX)
				env->MaxX = _w[0];

			if (_w[1] < env->MinY)
				env->MinY = _w[1];
			else if (_w[1] > env->MaxY)
				env->MaxY = _w[1];
		}
	}

	return ES_NONE;
}

/*
 * Compute skewed extent that covers unskewed extent.
 *
 * @param envelope : unskewed extent of type rt_envelope
 * @param skew : pointer to 2-element array (x, y) of skew
 * @param scale : pointer to 2-element array (x, y) of scale
 * @param tolerance : value between 0 and 1 where the smaller the tolerance
 * results in an extent approaching the "minimum" skewed extent.
 * If value <= 0, tolerance = 0.1. If value > 1, tolerance = 1.
 *
 * @return skewed raster who's extent covers unskewed extent, NULL on error
 */
rt_raster
rt_raster_compute_skewed_raster(
	rt_envelope extent,
	double *skew,
	double *scale,
	double tolerance
) {
	uint32_t run = 0;
	uint32_t max_run = 1;
	double dbl_run = 0;

	int rtn;
	int covers = 0;
	rt_raster raster;
	double _gt[6] = {0};
	double _igt[6] = {0};
	int _d[2] = {1, -1};
	int _dlast = 0;
	int _dlastpos = 0;
	double _w[2] = {0};
	double _r[2] = {0};
	double _xy[2] = {0};
	int i;
	int j;
	int x;
	int y;

	LWGEOM *geom = NULL;
	GEOSGeometry *sgeom = NULL;
	GEOSGeometry *ngeom = NULL;

	if (
		(tolerance < 0.) ||
		FLT_EQ(tolerance, 0.)
	) {
		tolerance = 0.1;
	}
	else if (tolerance > 1.)
		tolerance = 1;

	dbl_run = tolerance;
	while (dbl_run < 10) {
		dbl_run *= 10.;
		max_run *= 10;
	}

	/* scale must be provided */
	if (scale == NULL)
		return NULL;
	for (i = 0; i < 2; i++) {
		if (FLT_EQ(scale[i], 0)) {
			rterror("rt_raster_compute_skewed_raster: Scale cannot be zero");
			return 0;
		}

		if (i < 1)
			_gt[1] = fabs(scale[i] * tolerance);
		else
			_gt[5] = fabs(scale[i] * tolerance);
	}
	/* conform scale-y to be negative */
	_gt[5] *= -1;

	/* skew not provided or skew is zero, return raster of correct dim and spatial attributes */
	if (
		(skew == NULL) || (
			FLT_EQ(skew[0], 0) &&
			FLT_EQ(skew[1], 0)
		)
	) {
		int _dim[2] = {
			(int) fmax((fabs(extent.MaxX - extent.MinX) + (fabs(scale[0]) / 2.)) / fabs(scale[0]), 1),
			(int) fmax((fabs(extent.MaxY - extent.MinY) + (fabs(scale[1]) / 2.)) / fabs(scale[1]), 1)
		};

		raster = rt_raster_new(_dim[0], _dim[1]);
		if (raster == NULL) {
			rterror("rt_raster_compute_skewed_raster: Could not create output raster");
			return NULL;
		}

		rt_raster_set_offsets(raster, extent.MinX, extent.MaxY);
		rt_raster_set_scale(raster, fabs(scale[0]), -1 * fabs(scale[1]));
		rt_raster_set_skews(raster, skew[0], skew[1]);

		return raster;
	}

	/* direction to shift upper-left corner */
	if (skew[0] > 0.)
		_d[0] = -1;
	if (skew[1] < 0.)
		_d[1] = 1;

	/* geotransform */
	_gt[0] = extent.UpperLeftX;
	_gt[2] = skew[0] * tolerance;
	_gt[3] = extent.UpperLeftY;
	_gt[4] = skew[1] * tolerance;

	RASTER_DEBUGF(4, "Initial geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]
	);
	RASTER_DEBUGF(4, "Delta: %d, %d", _d[0], _d[1]);

	/* simple raster */
	if ((raster = rt_raster_new(1, 1)) == NULL) {
		rterror("rt_raster_compute_skewed_raster: Out of memory allocating extent raster");
		return NULL;
	}
	rt_raster_set_geotransform_matrix(raster, _gt);

	/* get inverse geotransform matrix */
	if (!GDALInvGeoTransform(_gt, _igt)) {
		rterror("rt_raster_compute_skewed_raster: Could not compute inverse geotransform matrix");
		rt_raster_destroy(raster);
		return NULL;
	}
	RASTER_DEBUGF(4, "Inverse geotransform: %f, %f, %f, %f, %f, %f",
		_igt[0], _igt[1], _igt[2], _igt[3], _igt[4], _igt[5]
	);

	/* shift along axis */
	for (i = 0; i < 2; i++) {
		covers = 0;
		run = 0;

		RASTER_DEBUGF(3, "Shifting along %s axis", i < 1 ? "X" : "Y");

		do {

			/* prevent possible infinite loop */
			if (run > max_run) {
				rterror("rt_raster_compute_skewed_raster: Could not compute skewed extent due to check preventing infinite loop");
				rt_raster_destroy(raster);
				return NULL;
			}

			/*
				check the four corners that they are covered along the specific axis
				pixel column should be >= 0
			*/
			for (j = 0; j < 4; j++) {
				switch (j) {
					/* upper-left */
					case 0:
						_xy[0] = extent.MinX;
						_xy[1] = extent.MaxY;
						break;
					/* lower-left */
					case 1:
						_xy[0] = extent.MinX;
						_xy[1] = extent.MinY;
						break;
					/* lower-right */
					case 2:
						_xy[0] = extent.MaxX;
						_xy[1] = extent.MinY;
						break;
					/* upper-right */
					case 3:
						_xy[0] = extent.MaxX;
						_xy[1] = extent.MaxY;
						break;
				}

				rtn = rt_raster_geopoint_to_cell(
					raster,
					_xy[0], _xy[1],
					&(_r[0]), &(_r[1]),
					_igt
				);
				if (rtn != ES_NONE) {
					rterror("rt_raster_compute_skewed_raster: Could not compute raster pixel for spatial coordinates");
					rt_raster_destroy(raster);
					return NULL;
				}

				RASTER_DEBUGF(4, "Point %d at cell %d x %d", j, (int) _r[0], (int) _r[1]);

				/* raster doesn't cover point */
				if ((int) _r[i] < 0) {
					RASTER_DEBUGF(4, "Point outside of skewed extent: %d", j);
					covers = 0;

					if (_dlastpos != j) {
						_dlast = (int) _r[i];
						_dlastpos = j;
					}
					else if ((int) _r[i] < _dlast) {
						RASTER_DEBUG(4, "Point going in wrong direction.  Reversing direction");
						_d[i] *= -1;
						_dlastpos = -1;
						run = 0;
					}

					break;
				}

				covers++;
			}

			if (!covers) {
				x = 0;
				y = 0;
				if (i < 1)
					x = _d[i] * fabs(_r[i]);
				else
					y = _d[i] * fabs(_r[i]);

				rtn = rt_raster_cell_to_geopoint(
					raster,
					x, y,
					&(_w[0]), &(_w[1]),
					_gt
				);
				if (rtn != ES_NONE) {
					rterror("rt_raster_compute_skewed_raster: Could not compute spatial coordinates for raster pixel");
					rt_raster_destroy(raster);
					return NULL;
				}

				/* adjust ul */
				if (i < 1)
					_gt[0] = _w[i];
				else
					_gt[3] = _w[i];
				rt_raster_set_geotransform_matrix(raster, _gt);
				RASTER_DEBUGF(4, "Shifted geotransform: %f, %f, %f, %f, %f, %f",
					_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]
				);

				/* get inverse geotransform matrix */
				if (!GDALInvGeoTransform(_gt, _igt)) {
					rterror("rt_raster_compute_skewed_raster: Could not compute inverse geotransform matrix");
					rt_raster_destroy(raster);
					return NULL;
				}
				RASTER_DEBUGF(4, "Inverse geotransform: %f, %f, %f, %f, %f, %f",
					_igt[0], _igt[1], _igt[2], _igt[3], _igt[4], _igt[5]
				);
			}

			run++;
		}
		while (!covers);
	}

	/* covers test */
	rtn = rt_raster_geopoint_to_cell(
		raster,
		extent.MaxX, extent.MinY,
		&(_r[0]), &(_r[1]),
		_igt
	);
	if (rtn != ES_NONE) {
		rterror("rt_raster_compute_skewed_raster: Could not compute raster pixel for spatial coordinates");
		rt_raster_destroy(raster);
		return NULL;
	}

	RASTER_DEBUGF(4, "geopoint %f x %f at cell %d x %d", extent.MaxX, extent.MinY, (int) _r[0], (int) _r[1]);

	raster->width = _r[0];
	raster->height = _r[1];

	/* initialize GEOS */
	initGEOS(lwnotice, lwgeom_geos_error);

	/* create reference LWPOLY */
	{
		LWPOLY *npoly = rt_util_envelope_to_lwpoly(extent);
		if (npoly == NULL) {
			rterror("rt_raster_compute_skewed_raster: Could not build extent's geometry for covers test");
			rt_raster_destroy(raster);
			return NULL;
		}

		ngeom = (GEOSGeometry *) LWGEOM2GEOS(lwpoly_as_lwgeom(npoly));
		lwpoly_free(npoly);
	}

	do {
		covers = 0;

		/* construct sgeom from raster */
		if ((rt_raster_get_convex_hull(raster, &geom) != ES_NONE) || geom == NULL) {
			rterror("rt_raster_compute_skewed_raster: Could not build skewed extent's geometry for covers test");
			GEOSGeom_destroy(ngeom);
			rt_raster_destroy(raster);
			return NULL;
		}

		sgeom = (GEOSGeometry *) LWGEOM2GEOS(geom);
		lwgeom_free(geom);

		covers = GEOSRelatePattern(sgeom, ngeom, "******FF*");
		GEOSGeom_destroy(sgeom);

		if (covers == 2) {
			rterror("rt_raster_compute_skewed_raster: Could not run covers test");
			GEOSGeom_destroy(ngeom);
			rt_raster_destroy(raster);
			return NULL;
		}

		if (covers)
			break;

		raster->width++;
		raster->height++;
	}
	while (!covers);

	RASTER_DEBUGF(4, "Skewed extent does cover normal extent with dimensions %d x %d", raster->width, raster->height);

	raster->width = (int) ((((double) raster->width) * fabs(_gt[1]) + fabs(scale[0] / 2.)) / fabs(scale[0]));
	raster->height = (int) ((((double) raster->height) * fabs(_gt[5]) + fabs(scale[1] / 2.)) / fabs(scale[1]));
	_gt[1] = fabs(scale[0]);
	_gt[5] = -1 * fabs(scale[1]);
	_gt[2] = skew[0];
	_gt[4] = skew[1];
	rt_raster_set_geotransform_matrix(raster, _gt);

	/* minimize width/height */
	for (i = 0; i < 2; i++) {
		covers = 1;
		do {
			if (i < 1)
				raster->width--;
			else
				raster->height--;
			
			/* construct sgeom from raster */
			if ((rt_raster_get_convex_hull(raster, &geom) != ES_NONE) || geom == NULL) {
				rterror("rt_raster_compute_skewed_raster: Could not build skewed extent's geometry for minimizing dimensions");
				GEOSGeom_destroy(ngeom);
				rt_raster_destroy(raster);
				return NULL;
			}

			sgeom = (GEOSGeometry *) LWGEOM2GEOS(geom);
			lwgeom_free(geom);

			covers = GEOSRelatePattern(sgeom, ngeom, "******FF*");
			GEOSGeom_destroy(sgeom);

			if (covers == 2) {
				rterror("rt_raster_compute_skewed_raster: Could not run covers test for minimizing dimensions");
				GEOSGeom_destroy(ngeom);
				rt_raster_destroy(raster);
				return NULL;
			}

			if (!covers) {
				if (i < 1)
					raster->width++;
				else
					raster->height++;

				break;
			}
		}
		while (covers);
	}

	GEOSGeom_destroy(ngeom);

	return raster;
}

/*--------- WKB I/O ---------------------------------------------------*/

static uint8_t
isMachineLittleEndian(void) {
    static int endian_check_int = 1; /* dont modify this!!! */
    /* 0=big endian|xdr --  1=little endian|ndr */
    return *((uint8_t *) & endian_check_int);
}

static uint8_t
read_uint8(const uint8_t** from) {
    assert(NULL != from);

    return *(*from)++;
}

/* unused up to now
static void
write_uint8(uint8_t** from, uint8_t v)
{
    assert(NULL != from);

 *(*from)++ = v;
}
 */

static int8_t
read_int8(const uint8_t** from) {
    assert(NULL != from);

    return (int8_t) read_uint8(from);
}

/* unused up to now
static void
write_int8(uint8_t** from, int8_t v)
{
    assert(NULL != from);

 *(*from)++ = v;
}
 */

static uint16_t
read_uint16(const uint8_t** from, uint8_t littleEndian) {
    uint16_t ret = 0;

    assert(NULL != from);

    if (littleEndian) {
        ret = (*from)[0] |
                (*from)[1] << 8;
    } else {
        /* big endian */
        ret = (*from)[0] << 8 |
                (*from)[1];
    }
    *from += 2;
    return ret;
}

static void
write_uint16(uint8_t** to, uint8_t littleEndian, uint16_t v) {
    assert(NULL != to);

    if (littleEndian) {
        (*to)[0] = v & 0x00FF;
        (*to)[1] = v >> 8;
    } else {
        (*to)[1] = v & 0x00FF;
        (*to)[0] = v >> 8;
    }
    *to += 2;
}

static int16_t
read_int16(const uint8_t** from, uint8_t littleEndian) {
    assert(NULL != from);

    return read_uint16(from, littleEndian);
}

/* unused up to now
static void
write_int16(uint8_t** to, uint8_t littleEndian, int16_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x00FF;
        (*to)[1] = v >> 8;
    }
    else
    {
        (*to)[1] = v & 0x00FF;
        (*to)[0] = v >> 8;
    }
 *to += 2;
}
 */

static uint32_t
read_uint32(const uint8_t** from, uint8_t littleEndian) {
    uint32_t ret = 0;

    assert(NULL != from);

    if (littleEndian) {
        ret = (uint32_t) ((*from)[0] & 0xff) |
                (uint32_t) ((*from)[1] & 0xff) << 8 |
                (uint32_t) ((*from)[2] & 0xff) << 16 |
                (uint32_t) ((*from)[3] & 0xff) << 24;
    } else {
        /* big endian */
        ret = (uint32_t) ((*from)[3] & 0xff) |
                (uint32_t) ((*from)[2] & 0xff) << 8 |
                (uint32_t) ((*from)[1] & 0xff) << 16 |
                (uint32_t) ((*from)[0] & 0xff) << 24;
    }

    *from += 4;
    return ret;
}

/* unused up to now
static void
write_uint32(uint8_t** to, uint8_t littleEndian, uint32_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x000000FF;
        (*to)[1] = ( v & 0x0000FF00 ) >> 8;
        (*to)[2] = ( v & 0x00FF0000 ) >> 16;
        (*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
        (*to)[3] = v & 0x000000FF;
        (*to)[2] = ( v & 0x0000FF00 ) >> 8;
        (*to)[1] = ( v & 0x00FF0000 ) >> 16;
        (*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
 */

static int32_t
read_int32(const uint8_t** from, uint8_t littleEndian) {
    assert(NULL != from);

    return read_uint32(from, littleEndian);
}

/* unused up to now
static void
write_int32(uint8_t** to, uint8_t littleEndian, int32_t v)
{
    assert(NULL != to);

    if ( littleEndian )
    {
        (*to)[0] = v & 0x000000FF;
        (*to)[1] = ( v & 0x0000FF00 ) >> 8;
        (*to)[2] = ( v & 0x00FF0000 ) >> 16;
        (*to)[3] = ( v & 0xFF000000 ) >> 24;
    }
    else
    {
        (*to)[3] = v & 0x000000FF;
        (*to)[2] = ( v & 0x0000FF00 ) >> 8;
        (*to)[1] = ( v & 0x00FF0000 ) >> 16;
        (*to)[0] = ( v & 0xFF000000 ) >> 24;
    }
 *to += 4;
}
 */

static float
read_float32(const uint8_t** from, uint8_t littleEndian) {

    union {
        float f;
        uint32_t i;
    } ret;

    ret.i = read_uint32(from, littleEndian);

    return ret.f;
}

/* unused up to now
static void
write_float32(uint8_t** from, uint8_t littleEndian, float f)
{
    union {
        float f;
        uint32_t i;
    } u;

    u.f = f;
    write_uint32(from, littleEndian, u.i);
}
 */

static double
read_float64(const uint8_t** from, uint8_t littleEndian) {

    union {
        double d;
        uint64_t i;
    } ret;

    assert(NULL != from);

    if (littleEndian) {
        ret.i = (uint64_t) ((*from)[0] & 0xff) |
                (uint64_t) ((*from)[1] & 0xff) << 8 |
                (uint64_t) ((*from)[2] & 0xff) << 16 |
                (uint64_t) ((*from)[3] & 0xff) << 24 |
                (uint64_t) ((*from)[4] & 0xff) << 32 |
                (uint64_t) ((*from)[5] & 0xff) << 40 |
                (uint64_t) ((*from)[6] & 0xff) << 48 |
                (uint64_t) ((*from)[7] & 0xff) << 56;
    } else {
        /* big endian */
        ret.i = (uint64_t) ((*from)[7] & 0xff) |
                (uint64_t) ((*from)[6] & 0xff) << 8 |
                (uint64_t) ((*from)[5] & 0xff) << 16 |
                (uint64_t) ((*from)[4] & 0xff) << 24 |
                (uint64_t) ((*from)[3] & 0xff) << 32 |
                (uint64_t) ((*from)[2] & 0xff) << 40 |
                (uint64_t) ((*from)[1] & 0xff) << 48 |
                (uint64_t) ((*from)[0] & 0xff) << 56;
    }

    *from += 8;
    return ret.d;
}

/* unused up to now
static void
write_float64(uint8_t** to, uint8_t littleEndian, double v)
{
    union {
        double d;
        uint64_t i;
    } u;

    assert(NULL != to);

    u.d = v;

    if ( littleEndian )
    {
        (*to)[0] =   u.i & 0x00000000000000FFULL;
        (*to)[1] = ( u.i & 0x000000000000FF00ULL ) >> 8;
        (*to)[2] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
        (*to)[3] = ( u.i & 0x00000000FF000000ULL ) >> 24;
        (*to)[4] = ( u.i & 0x000000FF00000000ULL ) >> 32;
        (*to)[5] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
        (*to)[6] = ( u.i & 0x00FF000000000000ULL ) >> 48;
        (*to)[7] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
    else
    {
        (*to)[7] =   u.i & 0x00000000000000FFULL;
        (*to)[6] = ( u.i & 0x000000000000FF00ULL ) >> 8;
        (*to)[5] = ( u.i & 0x0000000000FF0000ULL ) >> 16;
        (*to)[4] = ( u.i & 0x00000000FF000000ULL ) >> 24;
        (*to)[3] = ( u.i & 0x000000FF00000000ULL ) >> 32;
        (*to)[2] = ( u.i & 0x0000FF0000000000ULL ) >> 40;
        (*to)[1] = ( u.i & 0x00FF000000000000ULL ) >> 48;
        (*to)[0] = ( u.i & 0xFF00000000000000ULL ) >> 56;
    }
 *to += 8;
}
 */

#define BANDTYPE_FLAGS_MASK 0xF0
#define BANDTYPE_PIXTYPE_MASK 0x0F
#define BANDTYPE_FLAG_OFFDB     (1<<7)
#define BANDTYPE_FLAG_HASNODATA (1<<6)
#define BANDTYPE_FLAG_ISNODATA  (1<<5)
#define BANDTYPE_FLAG_RESERVED3 (1<<4)

#define BANDTYPE_PIXTYPE(x) ((x)&BANDTYPE_PIXTYPE_MASK)
#define BANDTYPE_IS_OFFDB(x) ((x)&BANDTYPE_FLAG_OFFDB)
#define BANDTYPE_HAS_NODATA(x) ((x)&BANDTYPE_FLAG_HASNODATA)
#define BANDTYPE_IS_NODATA(x) ((x)&BANDTYPE_FLAG_ISNODATA)

/* Read band from WKB as at start of band */
static rt_band
rt_band_from_wkb(
	uint16_t width, uint16_t height,
	const uint8_t** ptr, const uint8_t* end,
	uint8_t littleEndian
) {
	rt_band band = NULL;
	int pixbytes = 0;
	uint8_t type = 0;
	unsigned long sz = 0;
	uint32_t v = 0;

	assert(NULL != ptr);
	assert(NULL != end);

	band = rtalloc(sizeof (struct rt_band_t));
	if (!band) {
		rterror("rt_band_from_wkb: Out of memory allocating rt_band during WKB parsing");
		return NULL;
	}
	band->ownsdata = 0; /* assume we don't own data */

	if (end - *ptr < 1) {
		rterror("rt_band_from_wkb: Premature end of WKB on band reading (%s:%d)",
			__FILE__, __LINE__);
		rt_band_destroy(band);
		return NULL;
	}
	type = read_uint8(ptr);

	if ((type & BANDTYPE_PIXTYPE_MASK) >= PT_END) {
		rterror("rt_band_from_wkb: Invalid pixtype %d", type & BANDTYPE_PIXTYPE_MASK);
		rt_band_destroy(band);
		return NULL;
	}

	band->pixtype = type & BANDTYPE_PIXTYPE_MASK;
	band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
	band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
	band->isnodata = band->hasnodata ? (BANDTYPE_IS_NODATA(type) ? 1 : 0) : 0;
	band->width = width;
	band->height = height;

	RASTER_DEBUGF(3, " Band pixtype:%s, offline:%d, hasnodata:%d",
		rt_pixtype_name(band->pixtype),
		band->offline,
		band->hasnodata
	);

	/* Check there's enough bytes to read nodata value */
	pixbytes = rt_pixtype_size(band->pixtype);
	if (((*ptr) + pixbytes) >= end) {
		rterror("rt_band_from_wkb: Premature end of WKB on band novalue reading");
		rt_band_destroy(band);
		return NULL;
	}

	/* Read nodata value */
	switch (band->pixtype) {
		case PT_1BB: {
			band->nodataval = ((int) read_uint8(ptr)) & 0x01;
			break;
		}
		case PT_2BUI: {
			band->nodataval = ((int) read_uint8(ptr)) & 0x03;
			break;
		}
		case PT_4BUI: {
			band->nodataval = ((int) read_uint8(ptr)) & 0x0F;
			break;
		}
		case PT_8BSI: {
			band->nodataval = read_int8(ptr);
			break;
		}
		case PT_8BUI: {
			band->nodataval = read_uint8(ptr);
			break;
		}
		case PT_16BSI: {
			band->nodataval = read_int16(ptr, littleEndian);
			break;
		}
		case PT_16BUI: {
			band->nodataval = read_uint16(ptr, littleEndian);
			break;
		}
		case PT_32BSI: {
			band->nodataval = read_int32(ptr, littleEndian);
			break;
		}
		case PT_32BUI: {
			band->nodataval = read_uint32(ptr, littleEndian);
			break;
		}
		case PT_32BF: {
			band->nodataval = read_float32(ptr, littleEndian);
			break;
		}
		case PT_64BF: {
			band->nodataval = read_float64(ptr, littleEndian);
			break;
		}
		default: {
			rterror("rt_band_from_wkb: Unknown pixeltype %d", band->pixtype);
			rt_band_destroy(band);
			return NULL;
		}
	}

	RASTER_DEBUGF(3, " Nodata value: %g, pixbytes: %d, ptr @ %p, end @ %p",
		band->nodataval, pixbytes, *ptr, end);

	if (band->offline) {
		if (((*ptr) + 1) >= end) {
			rterror("rt_band_from_wkb: Premature end of WKB on offline "
				"band data bandNum reading (%s:%d)",
				__FILE__, __LINE__
			);
			rt_band_destroy(band);
			return NULL;
		}

		band->data.offline.bandNum = read_int8(ptr);
		band->data.offline.mem = NULL;

		{
			/* check we have a NULL-termination */
			sz = 0;
			while ((*ptr)[sz] && &((*ptr)[sz]) < end) ++sz;
			if (&((*ptr)[sz]) >= end) {
				rterror("rt_band_from_wkb: Premature end of WKB on band offline path reading");
				rt_band_destroy(band);
				return NULL;
			}

			/* we never own offline band data */
			band->ownsdata = 0;

			band->data.offline.path = rtalloc(sz + 1);
			if (band->data.offline.path == NULL) {
				rterror("rt_band_from_wkb: Out of memory allocating for offline path of band");
				rt_band_destroy(band);
				return NULL;
			}

			memcpy(band->data.offline.path, *ptr, sz);
			band->data.offline.path[sz] = '\0';

			RASTER_DEBUGF(3, "OFFDB band path is %s (size is %d)",
				band->data.offline.path, sz);

			*ptr += sz + 1;

			/* TODO: How could we know if the offline band is a nodata band? */
			/* trust in the force */
			/*band->isnodata = FALSE;*/
		}

		return band;
	}

	/* This is an on-disk band */
	sz = width * height * pixbytes;
	if (((*ptr) + sz) > end) {
		rterror("rt_band_from_wkb: Premature end of WKB on band data reading (%s:%d)",
			__FILE__, __LINE__);
		rt_band_destroy(band);
		return NULL;
	}

	band->data.mem = rtalloc(sz);
	if (!band->data.mem) {
		rterror("rt_band_from_wkb: Out of memory during band creation in WKB parser");
		rt_band_destroy(band);
		return NULL;
	}

	band->ownsdata = 1; /* we DO own this data!!! */
	memcpy(band->data.mem, *ptr, sz);
	*ptr += sz;

	/* Should now flip values if > 8bit and
	 * littleEndian != isMachineLittleEndian */
	if (pixbytes > 1) {
		if (isMachineLittleEndian() != littleEndian) {
			void (*flipper)(uint8_t*) = 0;
			uint8_t *flipme = NULL;

			if (pixbytes == 2)
				flipper = flip_endian_16;
			else if (pixbytes == 4)
				flipper = flip_endian_32;
			else if (pixbytes == 8)
				flipper = flip_endian_64;
			else {
				rterror("rt_band_from_wkb: Unexpected pix bytes %d", pixbytes);
				rt_band_destroy(band);
				return NULL;
			}

			flipme = band->data.mem;
			sz = width * height;
			for (v = 0; v < sz; ++v) {
				flipper(flipme);
				flipme += pixbytes;
			}
		}
	}
	/* And should check for invalid values for < 8bit types */
	else if (
		band->pixtype == PT_1BB ||
		band->pixtype == PT_2BUI ||
		band->pixtype == PT_4BUI
	) {
		uint8_t maxVal = band->pixtype == PT_1BB ? 1 : (band->pixtype == PT_2BUI ? 3 : 15);
		uint8_t val;

		sz = width*height;
		for (v = 0; v < sz; ++v) {
			val = ((uint8_t*) band->data.mem)[v];
			if (val > maxVal) {
				rterror("rt_band_from_wkb: Invalid value %d for pixel of type %s",
					val, rt_pixtype_name(band->pixtype));
				rt_band_destroy(band);
				return NULL;
			}
		}
	}

	/* And we should check if the band is a nodata band */
	/* TODO: No!! This is too slow */
	/*rt_band_check_is_nodata(band);*/

	return band;
}

/* -4 for size, +1 for endian */
#define RT_WKB_HDR_SZ (sizeof(struct rt_raster_serialized_t)-4+1)

rt_raster
rt_raster_from_wkb(const uint8_t* wkb, uint32_t wkbsize) {
	const uint8_t *ptr = wkb;
	const uint8_t *wkbend = NULL;
	rt_raster rast = NULL;
	uint8_t endian = 0;
	uint16_t version = 0;
	uint16_t i = 0;
	uint16_t j = 0;

	assert(NULL != ptr);

	/* Check that wkbsize is >= sizeof(rt_raster_serialized) */
	if (wkbsize < RT_WKB_HDR_SZ) {
		rterror("rt_raster_from_wkb: wkb size (%d)  < min size (%d)",
			wkbsize, RT_WKB_HDR_SZ);
		return NULL;
	}
	wkbend = wkb + wkbsize;

	RASTER_DEBUGF(3, "Parsing header from wkb position %d (expected 0)",
		d_binptr_to_pos(ptr, wkbend, wkbsize));

	CHECK_BINPTR_POSITION(ptr, wkbend, wkbsize, 0);

	/* Read endianness */
	endian = *ptr;
	ptr += 1;

	/* Read version of protocol */
	version = read_uint16(&ptr, endian);
	if (version != 0) {
		rterror("rt_raster_from_wkb: WKB version %d unsupported", version);
		return NULL;
	}

	/* Read other components of raster header */
	rast = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
	if (!rast) {
		rterror("rt_raster_from_wkb: Out of memory allocating raster for wkb input");
		return NULL;
	}

	rast->numBands = read_uint16(&ptr, endian);
	rast->scaleX = read_float64(&ptr, endian);
	rast->scaleY = read_float64(&ptr, endian);
	rast->ipX = read_float64(&ptr, endian);
	rast->ipY = read_float64(&ptr, endian);
	rast->skewX = read_float64(&ptr, endian);
	rast->skewY = read_float64(&ptr, endian);
	rast->srid = clamp_srid(read_int32(&ptr, endian));
	rast->width = read_uint16(&ptr, endian);
	rast->height = read_uint16(&ptr, endian);

	/* Consistency checking, should have been checked before */
	assert(ptr <= wkbend);

	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster numBands: %d",
		rast->numBands);
	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster scale: %gx%g",
		rast->scaleX, rast->scaleY);
	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster ip: %gx%g",
		rast->ipX, rast->ipY);
	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster skew: %gx%g",
		rast->skewX, rast->skewY);
	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster srid: %d",
		rast->srid);
	RASTER_DEBUGF(3, "rt_raster_from_wkb: Raster dims: %dx%d",
		rast->width, rast->height);
	RASTER_DEBUGF(3, "Parsing raster header finished at wkb position %d (expected 61)",
		d_binptr_to_pos(ptr, wkbend, wkbsize));

	CHECK_BINPTR_POSITION(ptr, wkbend, wkbsize, 61);

	/* Read all bands of raster */
	if (!rast->numBands) {
		/* Here ptr should have been left to right after last used byte */
		if (ptr < wkbend) {
			rtwarn("%d bytes of WKB remained unparsed", wkbend - ptr);
		}
		else if (ptr > wkbend) {
			/* Easier to get a segfault before I guess */
			rtwarn("We parsed %d bytes more then available!", ptr - wkbend);
		}

		rast->bands = NULL;
		return rast;
	}

	/* Now read the bands */
	rast->bands = (rt_band*) rtalloc(sizeof(rt_band) * rast->numBands);
	if (!rast->bands) {
		rterror("rt_raster_from_wkb: Out of memory allocating bands for WKB raster decoding");
		rt_raster_destroy(rast);
		return NULL;
	}

	/* ptr should now point to start of first band */
	/* we should have checked this before */
	assert(ptr <= wkbend);

	for (i = 0; i < rast->numBands; ++i) {
		RASTER_DEBUGF(3, "Parsing band %d from wkb position %d", i,
			d_binptr_to_pos(ptr, wkbend, wkbsize));

		rt_band band = rt_band_from_wkb(rast->width, rast->height,
			&ptr, wkbend, endian);
		if (!band) {
			rterror("rt_raster_from_wkb: Error reading WKB form of band %d", i);
			for (j = 0; j < i; j++) rt_band_destroy(rast->bands[j]);
			rt_raster_destroy(rast);
			return NULL;
		}

		band->raster = rast;
		rast->bands[i] = band;
	}

	/* Here ptr should have been left to right after last used byte */
	if (ptr < wkbend) {
		rtwarn("%d bytes of WKB remained unparsed", wkbend - ptr);
	}
	else if (ptr > wkbend) {
		/* Easier to get a segfault before I guess */
		rtwarn("We parsed %d bytes more then available!", ptr - wkbend);
	}

	return rast;
}

rt_raster
rt_raster_from_hexwkb(const char* hexwkb, uint32_t hexwkbsize) {
	rt_raster ret = NULL;
	uint8_t* wkb = NULL;
	uint32_t wkbsize = 0;
	uint32_t i = 0;

	assert(NULL != hexwkb);

	RASTER_DEBUGF(3, "input wkb: %s", hexwkb);
	RASTER_DEBUGF(3, "input wkbsize: %d", hexwkbsize);

	if (hexwkbsize % 2) {
		rterror("rt_raster_from_hexwkb: Raster HEXWKB input must have an even number of characters");
		return NULL;
	}
	wkbsize = hexwkbsize / 2;

	wkb = rtalloc(wkbsize);
	if (!wkb) {
		rterror("rt_raster_from_hexwkb: Out of memory allocating memory for decoding HEXWKB");
		return NULL;
	}

	/* parse full hex */
	for (i = 0; i < wkbsize; ++i) {
		wkb[i] = parse_hex((char*) & (hexwkb[i * 2]));
	}

	ret = rt_raster_from_wkb(wkb, wkbsize);
	rtdealloc(wkb); /* as long as rt_raster_from_wkb copies memory */

	return ret;
}

static uint32_t
rt_raster_wkb_size(rt_raster raster, int outasin) {
	uint32_t size = RT_WKB_HDR_SZ;
	uint16_t i = 0;

	assert(NULL != raster);

	RASTER_DEBUGF(3, "rt_raster_wkb_size: computing size for %d bands",
		raster->numBands);

	for (i = 0; i < raster->numBands; ++i) {
		rt_band band = raster->bands[i];
		rt_pixtype pixtype = band->pixtype;
		int pixbytes = rt_pixtype_size(pixtype);

		RASTER_DEBUGF(3, "rt_raster_wkb_size: adding size of band %d", i);

		if (pixbytes < 1) {
			rterror("rt_raster_wkb_size: Corrupted band: unknown pixtype");
			return 0;
		}

		/* Add space for band type */
		size += 1;

		/* Add space for nodata value */
		size += pixbytes;

		if (!outasin && band->offline) {
			/* Add space for band number */
			size += 1;

			/* Add space for null-terminated path */
			size += strlen(band->data.offline.path) + 1;
		}
		else {
			/* Add space for actual data */
			size += pixbytes * raster->width * raster->height;
		}
	}

	return size;
}

/**
 * Return this raster in WKB form
 *
 * @param raster : the raster
 * @param outasin : if TRUE, out-db bands are treated as in-db
 * @param wkbsize : will be set to the size of returned wkb form
 *
 * @return WKB of raster or NULL on error
 */
uint8_t *
rt_raster_to_wkb(rt_raster raster, int outasin, uint32_t *wkbsize) {

#if POSTGIS_DEBUG_LEVEL > 0
	const uint8_t *wkbend = NULL;
#endif

	uint8_t *wkb = NULL;
	uint8_t *ptr = NULL;
	uint16_t i = 0;
	uint8_t littleEndian = isMachineLittleEndian();

	assert(NULL != raster);
	assert(NULL != wkbsize);

	RASTER_DEBUG(2, "rt_raster_to_wkb: about to call rt_raster_wkb_size");

	*wkbsize = rt_raster_wkb_size(raster, outasin);
	RASTER_DEBUGF(3, "rt_raster_to_wkb: found size: %d", *wkbsize);

	wkb = (uint8_t*) rtalloc(*wkbsize);
	if (!wkb) {
		rterror("rt_raster_to_wkb: Out of memory allocating WKB for raster");
		return NULL;
	}

	ptr = wkb;

#if POSTGIS_DEBUG_LEVEL > 2
	wkbend = ptr + (*wkbsize);
#endif
	RASTER_DEBUGF(3, "Writing raster header to wkb on position %d (expected 0)",
		d_binptr_to_pos(ptr, wkbend, *wkbsize));

	/* Write endianness */
	*ptr = littleEndian;
	ptr += 1;

	/* Write version(size - (end - ptr)) */
	write_uint16(&ptr, littleEndian, 0);

	/* Copy header (from numBands up) */
	memcpy(ptr, &(raster->numBands), sizeof (struct rt_raster_serialized_t) - 6);
	ptr += sizeof (struct rt_raster_serialized_t) - 6;

	RASTER_DEBUGF(3, "Writing bands header to wkb position %d (expected 61)",
		d_binptr_to_pos(ptr, wkbend, *wkbsize));

	/* Serialize bands now */
	for (i = 0; i < raster->numBands; ++i) {
		rt_band band = raster->bands[i];
		rt_pixtype pixtype = band->pixtype;
		int pixbytes = rt_pixtype_size(pixtype);

		RASTER_DEBUGF(3, "Writing WKB for band %d", i);
		RASTER_DEBUGF(3, "Writing band pixel type to wkb position %d",
			d_binptr_to_pos(ptr, wkbend, *wkbsize));

		if (pixbytes < 1) {
			rterror("rt_raster_to_wkb: Corrupted band: unknown pixtype");
			rtdealloc(wkb);
			return NULL;
		}

		/* Add band type */
		*ptr = band->pixtype;
		if (!outasin && band->offline) *ptr |= BANDTYPE_FLAG_OFFDB;
		if (band->hasnodata) *ptr |= BANDTYPE_FLAG_HASNODATA;
		if (band->isnodata) *ptr |= BANDTYPE_FLAG_ISNODATA;
		ptr += 1;

#if 0
		/* no padding required for WKB */
		/* Add padding (if needed) */
		if (pixbytes > 1) {
			memset(ptr, '\0', pixbytes - 1);
			ptr += pixbytes - 1;
		}
		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!(((uint64_t) ptr) % pixbytes));
#endif

		RASTER_DEBUGF(3, "Writing band nodata to wkb position %d",
			d_binptr_to_pos(ptr, wkbend, *wkbsize));

		/* Add nodata value */
		switch (pixtype) {
			case PT_1BB:
			case PT_2BUI:
			case PT_4BUI:
			case PT_8BUI: {
				uint8_t v = band->nodataval;
				*ptr = v;
				ptr += 1;
				break;
			}
			case PT_8BSI: {
				int8_t v = band->nodataval;
				*ptr = v;
				ptr += 1;
				break;
			}
			case PT_16BSI:
			case PT_16BUI: {
				uint16_t v = band->nodataval;
				memcpy(ptr, &v, 2);
				ptr += 2;
				break;
			}
			case PT_32BSI:
			case PT_32BUI: {
				uint32_t v = band->nodataval;
				memcpy(ptr, &v, 4);
				ptr += 4;
				break;
			}
			case PT_32BF: {
				float v = band->nodataval;
				memcpy(ptr, &v, 4);
				ptr += 4;
				break;
			}
			case PT_64BF: {
				memcpy(ptr, &band->nodataval, 8);
				ptr += 8;
				break;
			}
			default:
				rterror("rt_raster_to_wkb: Fatal error caused by unknown pixel type. Aborting.");
				rtdealloc(wkb);
				abort(); /* shoudn't happen */
				return 0;
		}

#if 0
		/* no padding for WKB */
		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((uint64_t) ptr % pixbytes));
#endif

		if (!outasin && band->offline) {
			/* Write band number */
			*ptr = band->data.offline.bandNum;
			ptr += 1;

			/* Write path */
			strcpy((char*) ptr, band->data.offline.path);
			ptr += strlen(band->data.offline.path) + 1;
		}
		else {
			/* Write data */
			uint32_t datasize = raster->width * raster->height * pixbytes;
			RASTER_DEBUGF(4, "rt_raster_to_wkb: Copying %d bytes", datasize);

			memcpy(ptr, rt_band_get_data(band), datasize);

			ptr += datasize;
		}

#if 0
		/* no padding for WKB */
		/* Pad up to 8-bytes boundary */
		while ((uint64_t) ptr % 8) {
			*ptr = 0;
			++ptr;
		}

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((uint64_t) ptr % pixbytes));
#endif
	}

	return wkb;
}

char *
rt_raster_to_hexwkb(rt_raster raster, int outasin, uint32_t *hexwkbsize) {
	uint8_t *wkb = NULL;
	char* hexwkb = NULL;
	uint32_t i = 0;
	uint32_t wkbsize = 0;

	assert(NULL != raster);
	assert(NULL != hexwkbsize);

	RASTER_DEBUG(2, "rt_raster_to_hexwkb: calling rt_raster_to_wkb");

	wkb = rt_raster_to_wkb(raster, outasin, &wkbsize);

	RASTER_DEBUG(3, "rt_raster_to_hexwkb: rt_raster_to_wkb returned");

	*hexwkbsize = wkbsize * 2; /* hex is 2 times bytes */
	hexwkb = (char*) rtalloc((*hexwkbsize) + 1);
	if (!hexwkb) {
		rterror("rt_raster_to_hexwkb: Out of memory hexifying raster WKB");
		rtdealloc(wkb);
		return NULL;
	}
	hexwkb[*hexwkbsize] = '\0'; /* Null-terminate */

	for (i = 0; i < wkbsize; ++i) {
		deparse_hex(wkb[i], &(hexwkb[2 * i]));
	}

	rtdealloc(wkb); /* we don't need this anymore */

	RASTER_DEBUGF(3, "rt_raster_to_hexwkb: output wkb: %s", hexwkb);
	return hexwkb;
}

/*--------- Serializer/Deserializer --------------------------------------*/

static uint32_t
rt_raster_serialized_size(rt_raster raster) {
	uint32_t size = sizeof (struct rt_raster_serialized_t);
	uint16_t i = 0;

	assert(NULL != raster);

	RASTER_DEBUGF(3, "Serialized size with just header:%d - now adding size of %d bands",
		size, raster->numBands);

	for (i = 0; i < raster->numBands; ++i) {
		rt_band band = raster->bands[i];
		rt_pixtype pixtype = band->pixtype;
		int pixbytes = rt_pixtype_size(pixtype);

		if (pixbytes < 1) {
			rterror("rt_raster_serialized_size: Corrupted band: unknown pixtype");
			return 0;
		}

		/* Add space for band type, hasnodata flag and data padding */
		size += pixbytes;

		/* Add space for nodata value */
		size += pixbytes;

		if (band->offline) {
			/* Add space for band number */
			size += 1;

			/* Add space for null-terminated path */
			size += strlen(band->data.offline.path) + 1;
		}
		else {
			/* Add space for raster band data */
			size += pixbytes * raster->width * raster->height;
		}

		RASTER_DEBUGF(3, "Size before alignment is %d", size);

		/* Align size to 8-bytes boundary (trailing padding) */
		/* XXX jorgearevalo: bug here. If the size is actually 8-bytes aligned,
		   this line will add 8 bytes trailing padding, and it's not necessary */
		/*size += 8 - (size % 8);*/
		if (size % 8)
			size += 8 - (size % 8);

		RASTER_DEBUGF(3, "Size after alignment is %d", size);
	}

	return size;
}

/**
 * Return this raster in serialized form.
 * Memory (band data included) is copied from rt_raster.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 */
void*
rt_raster_serialize(rt_raster raster) {
	uint32_t size = 0;
	uint8_t* ret = NULL;
	uint8_t* ptr = NULL;
	uint16_t i = 0;

	assert(NULL != raster);

	size = rt_raster_serialized_size(raster);
	ret = (uint8_t*) rtalloc(size);
	if (!ret) {
		rterror("rt_raster_serialize: Out of memory allocating %d bytes for serializing a raster", size);
		return NULL;
	}
	memset(ret, '-', size);
	ptr = ret;

	RASTER_DEBUGF(3, "sizeof(struct rt_raster_serialized_t):%u",
		sizeof (struct rt_raster_serialized_t));
	RASTER_DEBUGF(3, "sizeof(struct rt_raster_t):%u",
		sizeof (struct rt_raster_t));
	RASTER_DEBUGF(3, "serialized size:%lu", (long unsigned) size);

	/* Set size */
	/* NOTE: Value of rt_raster.size may be updated in
	 * returned object, for instance, by rt_pg layer to
	 * store value calculated by SET_VARSIZE.
	 */
	raster->size = size;

	/* Set version */
	raster->version = 0;

	/* Copy header */
	memcpy(ptr, raster, sizeof (struct rt_raster_serialized_t));

	RASTER_DEBUG(3, "Start hex dump of raster being serialized using 0x2D to mark non-written bytes");

#if POSTGIS_DEBUG_LEVEL > 2
	uint8_t* dbg_ptr = ptr;
	d_print_binary_hex("HEADER", dbg_ptr, size);
#endif

	ptr += sizeof (struct rt_raster_serialized_t);

	/* Serialize bands now */
	for (i = 0; i < raster->numBands; ++i) {
		rt_band band = raster->bands[i];
		assert(NULL != band);

		rt_pixtype pixtype = band->pixtype;
		int pixbytes = rt_pixtype_size(pixtype);
		if (pixbytes < 1) {
			rterror("rt_raster_serialize: Corrupted band: unknown pixtype");
			rtdealloc(ret);
			return NULL;
		}

		/* Add band type */
		*ptr = band->pixtype;
		if (band->offline) {
			*ptr |= BANDTYPE_FLAG_OFFDB;
		}
		if (band->hasnodata) {
			*ptr |= BANDTYPE_FLAG_HASNODATA;
		}

		if (band->isnodata) {
			*ptr |= BANDTYPE_FLAG_ISNODATA;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("PIXTYPE", dbg_ptr, size);
#endif

		ptr += 1;

		/* Add padding (if needed) */
		if (pixbytes > 1) {
			memset(ptr, '\0', pixbytes - 1);
			ptr += pixbytes - 1;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("PADDING", dbg_ptr, size);
#endif

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));

		/* Add nodata value */
		switch (pixtype) {
			case PT_1BB:
			case PT_2BUI:
			case PT_4BUI:
			case PT_8BUI: {
				uint8_t v = band->nodataval;
				*ptr = v;
				ptr += 1;
				break;
			}
			case PT_8BSI: {
				int8_t v = band->nodataval;
				*ptr = v;
				ptr += 1;
				break;
			}
			case PT_16BSI:
			case PT_16BUI: {
				uint16_t v = band->nodataval;
				memcpy(ptr, &v, 2);
				ptr += 2;
				break;
			}
			case PT_32BSI:
			case PT_32BUI: {
				uint32_t v = band->nodataval;
				memcpy(ptr, &v, 4);
				ptr += 4;
				break;
			}
			case PT_32BF: {
				float v = band->nodataval;
				memcpy(ptr, &v, 4);
				ptr += 4;
				break;
			}
			case PT_64BF: {
				memcpy(ptr, &band->nodataval, 8);
				ptr += 8;
				break;
			}
			default:
				rterror("rt_raster_serialize: Fatal error caused by unknown pixel type. Aborting.");
				rtdealloc(ret);
				return NULL;
		}

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("nodata", dbg_ptr, size);
#endif

		if (band->offline) {
			/* Write band number */
			*ptr = band->data.offline.bandNum;
			ptr += 1;

			/* Write path */
			strcpy((char*) ptr, band->data.offline.path);
			ptr += strlen(band->data.offline.path) + 1;
		}
		else {
			/* Write data */
			uint32_t datasize = raster->width * raster->height * pixbytes;
			memcpy(ptr, band->data.mem, datasize);
			ptr += datasize;
		}

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("BAND", dbg_ptr, size);
#endif

		/* Pad up to 8-bytes boundary */
		while ((uintptr_t) ptr % 8) {
			*ptr = 0;
			++ptr;

			RASTER_DEBUGF(3, "PAD at %d", (uintptr_t) ptr % 8);
		}

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - ret) % pixbytes));
	} /* for-loop over bands */

#if POSTGIS_DEBUG_LEVEL > 2
		d_print_binary_hex("SERIALIZED RASTER", dbg_ptr, size);
#endif
	return ret;
}

/**
 * Return a raster from a serialized form.
 *
 * Serialized form is documented in doc/RFC1-SerializedFormat.
 *
 * NOTE: the raster will contain pointer to the serialized
 * form (including band data), which must be kept alive.
 */
rt_raster
rt_raster_deserialize(void* serialized, int header_only) {
	rt_raster rast = NULL;
	const uint8_t *ptr = NULL;
	const uint8_t *beg = NULL;
	uint16_t i = 0;
	uint16_t j = 0;
	uint8_t littleEndian = isMachineLittleEndian();

	assert(NULL != serialized);

	RASTER_DEBUG(2, "rt_raster_deserialize: Entering...");

	/* NOTE: Value of rt_raster.size may be different
	 * than actual size of raster data being read.
	 * See note on SET_VARSIZE in rt_raster_serialize function above.
	 */

	/* Allocate memory for deserialized raster header */ 
	RASTER_DEBUG(3, "rt_raster_deserialize: Allocating memory for deserialized raster header");
	rast = (rt_raster) rtalloc(sizeof (struct rt_raster_t));
	if (!rast) {
		rterror("rt_raster_deserialize: Out of memory allocating raster for deserialization");
		return NULL;
	}

	/* Deserialize raster header */
	RASTER_DEBUG(3, "rt_raster_deserialize: Deserialize raster header");
	memcpy(rast, serialized, sizeof (struct rt_raster_serialized_t));

	if (0 == rast->numBands || header_only) {
		rast->bands = 0;
		return rast;
	}

	beg = (const uint8_t*) serialized;

	/* Allocate registry of raster bands */
	RASTER_DEBUG(3, "rt_raster_deserialize: Allocating memory for bands");
	rast->bands = rtalloc(rast->numBands * sizeof (rt_band));
	if (rast->bands == NULL) {
		rterror("rt_raster_deserialize: Out of memory allocating bands");
		rtdealloc(rast);
		return NULL;
	}

	RASTER_DEBUGF(3, "rt_raster_deserialize: %d bands", rast->numBands);

	/* Move to the beginning of first band */
	ptr = beg;
	ptr += sizeof (struct rt_raster_serialized_t);

	/* Deserialize bands now */
	for (i = 0; i < rast->numBands; ++i) {
		rt_band band = NULL;
		uint8_t type = 0;
		int pixbytes = 0;

		band = rtalloc(sizeof(struct rt_band_t));
		if (!band) {
			rterror("rt_raster_deserialize: Out of memory allocating rt_band during deserialization");
			for (j = 0; j < i; j++) rt_band_destroy(rast->bands[j]);
			rt_raster_destroy(rast);
			return NULL;
		}

		rast->bands[i] = band;

		type = *ptr;
		ptr++;
		band->pixtype = type & BANDTYPE_PIXTYPE_MASK;

		RASTER_DEBUGF(3, "rt_raster_deserialize: band %d with pixel type %s", i, rt_pixtype_name(band->pixtype));

		band->offline = BANDTYPE_IS_OFFDB(type) ? 1 : 0;
		band->hasnodata = BANDTYPE_HAS_NODATA(type) ? 1 : 0;
		band->isnodata = band->hasnodata ? (BANDTYPE_IS_NODATA(type) ? 1 : 0) : 0;
		band->width = rast->width;
		band->height = rast->height;
		band->ownsdata = 0; /* we do NOT own this data!!! */
		band->raster = rast;

		/* Advance by data padding */
		pixbytes = rt_pixtype_size(band->pixtype);
		ptr += pixbytes - 1;

		/* Read nodata value */
		switch (band->pixtype) {
			case PT_1BB: {
				band->nodataval = ((int) read_uint8(&ptr)) & 0x01;
				break;
			}
			case PT_2BUI: {
				band->nodataval = ((int) read_uint8(&ptr)) & 0x03;
				break;
			}
			case PT_4BUI: {
				band->nodataval = ((int) read_uint8(&ptr)) & 0x0F;
				break;
			}
			case PT_8BSI: {
				band->nodataval = read_int8(&ptr);
				break;
			}
			case PT_8BUI: {
				band->nodataval = read_uint8(&ptr);
				break;
			}
			case PT_16BSI: {
				band->nodataval = read_int16(&ptr, littleEndian);
				break;
			}
			case PT_16BUI: {
				band->nodataval = read_uint16(&ptr, littleEndian);
				break;
			}
			case PT_32BSI: {
				band->nodataval = read_int32(&ptr, littleEndian);
				break;
			}
			case PT_32BUI: {
				band->nodataval = read_uint32(&ptr, littleEndian);
				break;
			}
			case PT_32BF: {
				band->nodataval = read_float32(&ptr, littleEndian);
				break;
			}
			case PT_64BF: {
				band->nodataval = read_float64(&ptr, littleEndian);
				break;
			}
			default: {
				rterror("rt_raster_deserialize: Unknown pixeltype %d", band->pixtype);
				for (j = 0; j <= i; j++) rt_band_destroy(rast->bands[j]);
				rt_raster_destroy(rast);
				return NULL;
			}
		}

		RASTER_DEBUGF(3, "rt_raster_deserialize: has nodata flag %d", band->hasnodata);
		RASTER_DEBUGF(3, "rt_raster_deserialize: nodata value %g", band->nodataval);

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - beg) % pixbytes));

		if (band->offline) {
			int pathlen = 0;

			/* Read band number */
			band->data.offline.bandNum = *ptr;
			ptr += 1;

			/* Register path */
			pathlen = strlen((char*) ptr);
			band->data.offline.path = rtalloc(sizeof(char) * (pathlen + 1));
			if (band->data.offline.path == NULL) {
				rterror("rt_raster_deserialize: Could not allocate momory for offline band path");
				for (j = 0; j <= i; j++) rt_band_destroy(rast->bands[j]);
				rt_raster_destroy(rast);
				return NULL;
			}

			memcpy(band->data.offline.path, ptr, pathlen);
			band->data.offline.path[pathlen] = '\0';
			ptr += pathlen + 1;

			band->data.offline.mem = NULL;
		}
		else {
			/* Register data */
			const uint32_t datasize = rast->width * rast->height * pixbytes;
			band->data.mem = (uint8_t*) ptr;
			ptr += datasize;
		}

		/* Skip bytes of padding up to 8-bytes boundary */
#if POSTGIS_DEBUG_LEVEL > 0
		const uint8_t *padbeg = ptr;
#endif
		while (0 != ((ptr - beg) % 8)) {
			++ptr;
		}

		RASTER_DEBUGF(3, "rt_raster_deserialize: skip %d bytes of 8-bytes boundary padding", ptr - padbeg);

		/* Consistency checking (ptr is pixbytes-aligned) */
		assert(!((ptr - beg) % pixbytes));
	}

	return rast;
}

/**
 * Return TRUE if the raster is empty. i.e. is NULL, width = 0 or height = 0
 *
 * @param raster : the raster to get info from
 *
 * @return TRUE if the raster is empty, FALSE otherwise
 */
int
rt_raster_is_empty(rt_raster raster) {
	return (NULL == raster || raster->height <= 0 || raster->width <= 0);
}

/**
 * Return TRUE if the raster has a band of this number.
 *
 * @param raster : the raster to get info from
 * @param nband : the band number. 0-based
 *
 * @return TRUE if the raster has a band of this number, FALSE otherwise
 */
int
rt_raster_has_band(rt_raster raster, int nband) {
	return !(NULL == raster || nband >= raster->numBands || nband < 0);
}

/**
 * Copy one band from one raster to another.  Bands are duplicated from
 * fromrast to torast using rt_band_duplicate.  The caller will need
 * to ensure that the copied band's data or path remains allocated
 * for the lifetime of the copied bands.
 *
 * @param torast : raster to copy band to
 * @param fromrast : raster to copy band from
 * @param fromindex : index of band in source raster, 0-based
 * @param toindex : index of new band in destination raster, 0-based
 *
 * @return The band index of the second raster where the new band is copied.
 *   -1 if error
 */
int
rt_raster_copy_band(
	rt_raster torast, rt_raster fromrast,
	int fromindex, int toindex
) {
	rt_band srcband = NULL;
	rt_band dstband = NULL;

	assert(NULL != torast);
	assert(NULL != fromrast);

	/* Check raster dimensions */
	if (torast->height != fromrast->height || torast->width != fromrast->width) {
		rtwarn("rt_raster_copy_band: Attempting to add a band with different width or height");
		return -1;
	}

	/* Check bands limits */
	if (fromrast->numBands < 1) {
		rtwarn("rt_raster_copy_band: Second raster has no band");
		return -1;
	}
	else if (fromindex < 0) {
		rtwarn("rt_raster_copy_band: Band index for second raster < 0. Defaulted to 0");
		fromindex = 0;
	}
	else if (fromindex >= fromrast->numBands) {
		rtwarn("rt_raster_copy_band: Band index for second raster > number of bands, truncated from %u to %u", fromindex, fromrast->numBands - 1);
		fromindex = fromrast->numBands - 1;
	}

	if (toindex < 0) {
		rtwarn("rt_raster_copy_band: Band index for first raster < 0. Defaulted to 0");
		toindex = 0;
	}
	else if (toindex > torast->numBands) {
		rtwarn("rt_raster_copy_band: Band index for first raster > number of bands, truncated from %u to %u", toindex, torast->numBands);
		toindex = torast->numBands;
	}

	/* Get band from source raster */
	srcband = rt_raster_get_band(fromrast, fromindex);

	/* duplicate band */
	dstband = rt_band_duplicate(srcband);

	/* Add band to the second raster */
	return rt_raster_add_band(torast, dstband, toindex);
}

/**
 * Construct a new rt_raster from an existing rt_raster and an array
 * of band numbers
 *
 * @param raster : the source raster
 * @param bandNums : array of band numbers to extract from source raster
 *                   and add to the new raster (0 based)
 * @param count : number of elements in bandNums
 *
 * @return a new rt_raster or NULL on error
 */
rt_raster
rt_raster_from_band(rt_raster raster, uint32_t *bandNums, int count) {
	rt_raster rast = NULL;
	int i = 0;
	int j = 0;
	int idx;
	int32_t flag;
	double gt[6] = {0.};

	assert(NULL != raster);
	assert(NULL != bandNums);

	RASTER_DEBUGF(3, "rt_raster_from_band: source raster has %d bands",
		rt_raster_get_num_bands(raster));

	/* create new raster */
	rast = rt_raster_new(raster->width, raster->height);
	if (NULL == rast) {
		rterror("rt_raster_from_band: Out of memory allocating new raster");
		return NULL;
	}

	/* copy raster attributes */
	rt_raster_get_geotransform_matrix(raster, gt);
	rt_raster_set_geotransform_matrix(rast, gt);

	/* srid */
	rt_raster_set_srid(rast, raster->srid);

	/* copy bands */
	for (i = 0; i < count; i++) {
		idx = bandNums[i];
		flag = rt_raster_copy_band(rast, raster, idx, i);

		if (flag < 0) {
			rterror("rt_raster_from_band: Could not copy band");
			for (j = 0; j < i; j++) rt_band_destroy(rast->bands[j]);
			rt_raster_destroy(rast);
			return NULL;
		}

		RASTER_DEBUGF(3, "rt_raster_from_band: band created at index %d",
			flag);
	}

	RASTER_DEBUGF(3, "rt_raster_from_band: new raster has %d bands",
		rt_raster_get_num_bands(rast));
	return rast;
}

/**
 * Replace band at provided index with new band
 *
 * @param raster: raster of band to be replaced
 * @param band : new band to add to raster
 * @param index : index of band to replace (0-based)
 *
 * @return NULL on error or replaced band
 */
rt_band
rt_raster_replace_band(rt_raster raster, rt_band band, int index) {
	rt_band oldband = NULL;
	assert(NULL != raster);
	assert(NULL != band);

	if (band->width != raster->width || band->height != raster->height) {
		rterror("rt_raster_replace_band: Band does not match raster's dimensions: %dx%d band to %dx%d raster",
			band->width, band->height, raster->width, raster->height);
		return 0;
	}

	if (index >= raster->numBands || index < 0) {
		rterror("rt_raster_replace_band: Band index is not valid");
		return 0;
	}

	oldband = rt_raster_get_band(raster, index);
	RASTER_DEBUGF(3, "rt_raster_replace_band: old band at %p", oldband);
	RASTER_DEBUGF(3, "rt_raster_replace_band: new band at %p", band);

	raster->bands[index] = band;
	RASTER_DEBUGF(3, "rt_raster_replace_band: new band at %p", raster->bands[index]);

	band->raster = raster;
	oldband->raster = NULL;

	return oldband;
}

/**
 * Clone an existing raster
 *
 * @param raster : raster to clone
 * @param deep : flag indicating if bands should be cloned
 *
 * @return a new rt_raster or NULL on error
 */
rt_raster
rt_raster_clone(rt_raster raster, uint8_t deep) {
	rt_raster rtn = NULL;
	double gt[6] = {0};

	assert(NULL != raster);

	if (deep) {
		int numband = rt_raster_get_num_bands(raster);
		uint32_t *nband = NULL;
		int i = 0;

		nband = rtalloc(sizeof(uint32_t) * numband);
		if (nband == NULL) {
			rterror("rt_raster_clone: Could not allocate memory for deep clone");
			return NULL;
		}
		for (i = 0; i < numband; i++)
			nband[i] = i;

		rtn = rt_raster_from_band(raster, nband, numband);
		rtdealloc(nband);

		return rtn;
	}

	rtn = rt_raster_new(
		rt_raster_get_width(raster),
		rt_raster_get_height(raster)
	);
	if (rtn == NULL) {
		rterror("rt_raster_clone: Could not create cloned raster");
		return NULL;
	}

	rt_raster_get_geotransform_matrix(raster, gt);
	rt_raster_set_geotransform_matrix(rtn, gt);
	rt_raster_set_srid(rtn, rt_raster_get_srid(raster));

	return rtn;
}

/**
 * Return formatted GDAL raster from raster
 *
 * @param raster : the raster to convert
 * @param srs : the raster's coordinate system in OGC WKT
 * @param format : format to convert to. GDAL driver short name
 * @param options : list of format creation options. array of strings
 * @param gdalsize : will be set to the size of returned bytea
 *
 * @return formatted GDAL raster.  the calling function is responsible
 *   for freeing the returned data using CPLFree()
 */
uint8_t*
rt_raster_to_gdal(rt_raster raster, const char *srs,
	char *format, char **options, uint64_t *gdalsize) {
	GDALDriverH src_drv = NULL;
	GDALDatasetH src_ds = NULL;

	vsi_l_offset rtn_lenvsi;
	uint64_t rtn_len = 0;

	GDALDriverH rtn_drv = NULL;
	GDALDatasetH rtn_ds = NULL;
	uint8_t *rtn = NULL;

	assert(NULL != raster);
	assert(NULL != gdalsize);

	/* any supported format is possible */
	rt_util_gdal_register_all();
	RASTER_DEBUG(3, "loaded all supported GDAL formats");

	/* output format not specified */
	if (format == NULL || !strlen(format))
		format = "GTiff";
	RASTER_DEBUGF(3, "output format is %s", format);

	/* load raster into a GDAL MEM raster */
	src_ds = rt_raster_to_gdal_mem(raster, srs, NULL, NULL, 0, &src_drv);
	if (NULL == src_ds) {
		rterror("rt_raster_to_gdal: Could not convert raster to GDAL MEM format");
		return 0;
	}

	/* load driver */
	rtn_drv = GDALGetDriverByName(format);
	if (NULL == rtn_drv) {
		rterror("rt_raster_to_gdal: Could not load the output GDAL driver");
		GDALClose(src_ds);
		return 0;
	}
	RASTER_DEBUG(3, "Output driver loaded");

	/* convert GDAL MEM raster to output format */
	RASTER_DEBUG(3, "Copying GDAL MEM raster to memory file in output format");
	rtn_ds = GDALCreateCopy(
		rtn_drv,
		"/vsimem/out.dat", /* should be fine assuming this is in a process */
		src_ds,
		FALSE, /* should copy be strictly equivelent? */
		options, /* format options */
		NULL, /* progress function */
		NULL /* progress data */
	);
	if (NULL == rtn_ds) {
		rterror("rt_raster_to_gdal: Could not create the output GDAL dataset");
		GDALClose(src_ds);
		return 0;
	}

	/* close source dataset */
	GDALClose(src_ds);
	RASTER_DEBUG(3, "Closed GDAL MEM raster");

	RASTER_DEBUGF(4, "dataset SRS: %s", GDALGetProjectionRef(rtn_ds));

	/* close dataset, this also flushes any pending writes */
	GDALClose(rtn_ds);
	RASTER_DEBUG(3, "Closed GDAL output raster");

	RASTER_DEBUG(3, "Done copying GDAL MEM raster to memory file in output format");

	/* from memory file to buffer */
	RASTER_DEBUG(3, "Copying GDAL memory file to buffer");
	rtn = VSIGetMemFileBuffer("/vsimem/out.dat", &rtn_lenvsi, TRUE);
	RASTER_DEBUG(3, "Done copying GDAL memory file to buffer");
	if (NULL == rtn) {
		rterror("rt_raster_to_gdal: Could not create the output GDAL raster");
		return 0;
	}

	rtn_len = (uint64_t) rtn_lenvsi;
	*gdalsize = rtn_len;

	return rtn;
}

/**
 * Returns a set of available GDAL drivers
 *
 * @param drv_count : number of GDAL drivers available
 * @param cancc : if non-zero, filter drivers to only those
 *   with support for CreateCopy and VirtualIO
 *
 * @return set of "gdaldriver" values of available GDAL drivers
 */
rt_gdaldriver
rt_raster_gdal_drivers(uint32_t *drv_count, uint8_t cancc) {
	const char *state;
	const char *txt;
	int txt_len;
	GDALDriverH *drv = NULL;
	rt_gdaldriver rtn = NULL;
	int count;
	int i;
	uint32_t j;

	assert(drv_count != NULL);

	rt_util_gdal_register_all();
	count = GDALGetDriverCount();
	rtn = (rt_gdaldriver) rtalloc(count * sizeof(struct rt_gdaldriver_t));
	if (NULL == rtn) {
		rterror("rt_raster_gdal_drivers: Could not allocate memory for gdaldriver structure");
		return 0;
	}

	for (i = 0, j = 0; i < count; i++) {
		drv = GDALGetDriver(i);

		if (cancc) {
			/* CreateCopy support */
			state = GDALGetMetadataItem(drv, GDAL_DCAP_CREATECOPY, NULL);
			if (state == NULL) continue;

			/* VirtualIO support */
			state = GDALGetMetadataItem(drv, GDAL_DCAP_VIRTUALIO, NULL);
			if (state == NULL) continue;
		}

		/* index of driver */
		rtn[j].idx = i;

		/* short name */
		txt = GDALGetDriverShortName(drv);
		txt_len = strlen(txt);

		if (cancc) {
			RASTER_DEBUGF(3, "rt_raster_gdal_driver: driver %s (%d) supports CreateCopy() and VirtualIO()", txt, i);
		}

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].short_name = (char *) rtalloc(txt_len);
		memcpy(rtn[j].short_name, txt, txt_len);

		/* long name */
		txt = GDALGetDriverLongName(drv);
		txt_len = strlen(txt);

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].long_name = (char *) rtalloc(txt_len);
		memcpy(rtn[j].long_name, txt, txt_len);

		/* creation options */
		txt = GDALGetDriverCreationOptionList(drv);
		txt_len = strlen(txt);

		txt_len = (txt_len + 1) * sizeof(char);
		rtn[j].create_options = (char *) rtalloc(txt_len);
		memcpy(rtn[j].create_options, txt, txt_len);

		j++;
	}

	/* free unused memory */
	rtn = rtrealloc(rtn, j * sizeof(struct rt_gdaldriver_t));
	*drv_count = j;

	return rtn;
}

/**
 * Return GDAL dataset using GDAL MEM driver from raster
 *
 * @param raster : raster to convert to GDAL MEM
 * @param srs : the raster's coordinate system in OGC WKT
 * @param bandNums : array of band numbers to extract from raster
 *   and include in the GDAL dataset (0 based)
 * @param excludeNodataValues : array of zero, nonzero where if non-zero,
 *   ignore nodata values for the band
 * @param count : number of elements in bandNums
 * @param rtn_drv : is set to the GDAL driver object
 *
 * @return GDAL dataset using GDAL MEM driver
 */
GDALDatasetH
rt_raster_to_gdal_mem(
	rt_raster raster,
	const char *srs,
	uint32_t *bandNums,
	int *excludeNodataValues,
	int count,
	GDALDriverH *rtn_drv
) {
	GDALDriverH drv = NULL;
	GDALDatasetH ds = NULL;
	double gt[6] = {0.0};
	CPLErr cplerr;
	GDALDataType gdal_pt = GDT_Unknown;
	GDALRasterBandH band;
	void *pVoid;
	char *pszDataPointer;
	char szGDALOption[50];
	char *apszOptions[4];
	double nodata = 0.0;
	int allocBandNums = 0;
	int allocNodataValues = 0;

	int i;
	int numBands;
	uint32_t width = 0;
	uint32_t height = 0;
	rt_band rtband = NULL;
	rt_pixtype pt = PT_END;

	assert(NULL != raster);
	assert(NULL != rtn_drv);

	/* store raster in GDAL MEM raster */
	if (!rt_util_gdal_driver_registered("MEM"))
		GDALRegister_MEM();
	drv = GDALGetDriverByName("MEM");
	if (NULL == drv) {
		rterror("rt_raster_to_gdal_mem: Could not load the MEM GDAL driver");
		return 0;
	}
	*rtn_drv = drv;

	width = rt_raster_get_width(raster);
	height = rt_raster_get_height(raster);
	ds = GDALCreate(
		drv, "",
		width, height,
		0, GDT_Byte, NULL
	);
	if (NULL == ds) {
		rterror("rt_raster_to_gdal_mem: Could not create a GDALDataset to convert into");
		return 0;
	}

	/* add geotransform */
	rt_raster_get_geotransform_matrix(raster, gt);
	cplerr = GDALSetGeoTransform(ds, gt);
	if (cplerr != CE_None) {
		rterror("rt_raster_to_gdal_mem: Could not set geotransformation");
		GDALClose(ds);
		return 0;
	}

	/* set spatial reference */
	if (NULL != srs && strlen(srs)) {
		char *_srs = rt_util_gdal_convert_sr(srs, 0);
		if (_srs == NULL) {
			rterror("rt_raster_to_gdal_mem: Could not convert srs to GDAL accepted format");
			GDALClose(ds);
			return 0;
		}

		cplerr = GDALSetProjection(ds, _srs);
		CPLFree(_srs);
		if (cplerr != CE_None) {
			rterror("rt_raster_to_gdal_mem: Could not set projection");
			GDALClose(ds);
			return 0;
		}
		RASTER_DEBUGF(3, "Projection set to: %s", GDALGetProjectionRef(ds));
	}

	/* process bandNums */
	numBands = rt_raster_get_num_bands(raster);
	if (NULL != bandNums && count > 0) {
		for (i = 0; i < count; i++) {
			if (bandNums[i] >= numBands) {
				rterror("rt_raster_to_gdal_mem: The band index %d is invalid", bandNums[i]);
				GDALClose(ds);
				return 0;
			}
		}
	}
	else {
		count = numBands;
		bandNums = (uint32_t *) rtalloc(sizeof(uint32_t) * count);
		if (NULL == bandNums) {
			rterror("rt_raster_to_gdal_mem: Could not allocate memory for band indices");
			GDALClose(ds);
			return 0;
		}
		allocBandNums = 1;
		for (i = 0; i < count; i++) bandNums[i] = i;
	}

	/* process exclude_nodata_values */
	if (NULL == excludeNodataValues) {
		excludeNodataValues = (int *) rtalloc(sizeof(int) * count);
		if (NULL == excludeNodataValues) {
			rterror("rt_raster_to_gdal_mem: Could not allocate memory for NODATA flags");
			GDALClose(ds);
			return 0;
		}
		allocNodataValues = 1;
		for (i = 0; i < count; i++) excludeNodataValues[i] = 1;
	}

	/* add band(s) */
	for (i = 0; i < count; i++) {
		rtband = rt_raster_get_band(raster, bandNums[i]);
		if (NULL == rtband) {
			rterror("rt_raster_to_gdal_mem: Could not get requested band index %d", bandNums[i]);
			if (allocBandNums) rtdealloc(bandNums);
			if (allocNodataValues) rtdealloc(excludeNodataValues);
			GDALClose(ds);
			return 0;
		}

		pt = rt_band_get_pixtype(rtband);
		gdal_pt = rt_util_pixtype_to_gdal_datatype(pt);
		if (gdal_pt == GDT_Unknown)
			rtwarn("rt_raster_to_gdal_mem: Unknown pixel type for band");

		/*
			For all pixel types other than PT_8BSI, set pointer to start of data
		*/
		if (pt != PT_8BSI) {
			pVoid = rt_band_get_data(rtband);
			RASTER_DEBUGF(4, "Band data is at pos %p", pVoid);

			pszDataPointer = (char *) rtalloc(20 * sizeof (char));
			sprintf(pszDataPointer, "%p", pVoid);
			RASTER_DEBUGF(4, "rt_raster_to_gdal_mem: szDatapointer is %p",
				pszDataPointer);

			if (strnicmp(pszDataPointer, "0x", 2) == 0)
				sprintf(szGDALOption, "DATAPOINTER=%s", pszDataPointer);
			else
				sprintf(szGDALOption, "DATAPOINTER=0x%s", pszDataPointer);

			RASTER_DEBUG(3, "Storing info for GDAL MEM raster band");

			apszOptions[0] = szGDALOption;
			apszOptions[1] = NULL; /* pixel offset, not needed */
			apszOptions[2] = NULL; /* line offset, not needed */
			apszOptions[3] = NULL;

			/* free */
			rtdealloc(pszDataPointer);

			/* add band */
			if (GDALAddBand(ds, gdal_pt, apszOptions) == CE_Failure) {
				rterror("rt_raster_to_gdal_mem: Could not add GDAL raster band");
				if (allocBandNums) rtdealloc(bandNums);
				GDALClose(ds);
				return 0;
			}
		}
		/*
			PT_8BSI is special as GDAL has no equivalent pixel type.
			Must convert 8BSI to 16BSI so create basic band
		*/
		else {
			/* add band */
			if (GDALAddBand(ds, gdal_pt, NULL) == CE_Failure) {
				rterror("rt_raster_to_gdal_mem: Could not add GDAL raster band");
				if (allocBandNums) rtdealloc(bandNums);
				if (allocNodataValues) rtdealloc(excludeNodataValues);
				GDALClose(ds);
				return 0;
			}
		}

		/* check band count */
		if (GDALGetRasterCount(ds) != i + 1) {
			rterror("rt_raster_to_gdal_mem: Error creating GDAL MEM raster band");
			if (allocBandNums) rtdealloc(bandNums);
			if (allocNodataValues) rtdealloc(excludeNodataValues);
			GDALClose(ds);
			return 0;
		}

		/* get new band */
		band = NULL;
		band = GDALGetRasterBand(ds, i + 1);
		if (NULL == band) {
			rterror("rt_raster_to_gdal_mem: Could not get GDAL band for additional processing");
			if (allocBandNums) rtdealloc(bandNums);
			if (allocNodataValues) rtdealloc(excludeNodataValues);
			GDALClose(ds);
			return 0;
		}

		/* PT_8BSI requires manual setting of pixels */
		if (pt == PT_8BSI) {
			int nXBlocks, nYBlocks;
			int nXBlockSize, nYBlockSize;
			int iXBlock, iYBlock;
			int nXValid, nYValid;
			int iX, iY;
			int iXMax, iYMax;

			int x, y, z;
			uint32_t valueslen = 0;
			int16_t *values = NULL;
			double value = 0.;

			/* this makes use of GDAL's "natural" blocks */
			GDALGetBlockSize(band, &nXBlockSize, &nYBlockSize);
			nXBlocks = (width + nXBlockSize - 1) / nXBlockSize;
			nYBlocks = (height + nYBlockSize - 1) / nYBlockSize;
			RASTER_DEBUGF(4, "(nXBlockSize, nYBlockSize) = (%d, %d)", nXBlockSize, nYBlockSize);
			RASTER_DEBUGF(4, "(nXBlocks, nYBlocks) = (%d, %d)", nXBlocks, nYBlocks);

			/* length is for the desired pixel type */
			valueslen = rt_pixtype_size(PT_16BSI) * nXBlockSize * nYBlockSize;
			values = rtalloc(valueslen);
			if (NULL == values) {
				rterror("rt_raster_to_gdal_mem: Could not allocate memory for GDAL band pixel values");
				if (allocBandNums) rtdealloc(bandNums);
				if (allocNodataValues) rtdealloc(excludeNodataValues);
				GDALClose(ds);
				return 0;
			}

			for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
				for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
					memset(values, 0, valueslen);

					x = iXBlock * nXBlockSize;
					y = iYBlock * nYBlockSize;
					RASTER_DEBUGF(4, "(iXBlock, iYBlock) = (%d, %d)", iXBlock, iYBlock);
					RASTER_DEBUGF(4, "(x, y) = (%d, %d)", x, y);

					/* valid block width */
					if ((iXBlock + 1) * nXBlockSize > width)
						nXValid = width - (iXBlock * nXBlockSize);
					else
						nXValid = nXBlockSize;

					/* valid block height */
					if ((iYBlock + 1) * nYBlockSize > height)
						nYValid = height - (iYBlock * nYBlockSize);
					else
						nYValid = nYBlockSize;

					RASTER_DEBUGF(4, "(nXValid, nYValid) = (%d, %d)", nXValid, nYValid);

					/* convert 8BSI values to 16BSI */
					z = 0;
					iYMax = y + nYValid;
					iXMax = x + nXValid;
					for (iY = y; iY < iYMax; iY++)  {
						for (iX = x; iX < iXMax; iX++)  {
							if (rt_band_get_pixel(rtband, iX, iY, &value, NULL) != ES_NONE) {
								rterror("rt_raster_to_gdal_mem: Could not get pixel value to convert from 8BSI to 16BSI");
								rtdealloc(values);
								if (allocBandNums) rtdealloc(bandNums);
								if (allocNodataValues) rtdealloc(excludeNodataValues);
								GDALClose(ds);
								return 0;
							}

							values[z++] = rt_util_clamp_to_16BSI(value);
						}
					}

					/* burn values */
					if (GDALRasterIO(
						band, GF_Write,
						x, y,
						nXValid, nYValid,
						values, nXValid, nYValid,
						gdal_pt,
						0, 0
					) != CE_None) {
						rterror("rt_raster_to_gdal_mem: Could not write converted 8BSI to 16BSI values to GDAL band");
						rtdealloc(values);
						if (allocBandNums) rtdealloc(bandNums);
						if (allocNodataValues) rtdealloc(excludeNodataValues);
						GDALClose(ds);
						return 0;
					}
				}
			}

			rtdealloc(values);
		}

		/* Add nodata value for band */
		if (rt_band_get_hasnodata_flag(rtband) != FALSE && excludeNodataValues[i]) {
			rt_band_get_nodata(rtband, &nodata);
			if (GDALSetRasterNoDataValue(band, nodata) != CE_None)
				rtwarn("rt_raster_to_gdal_mem: Could not set nodata value for band");
			RASTER_DEBUGF(3, "nodata value set to %f", GDALGetRasterNoDataValue(band, NULL));
		}

#if POSTGIS_DEBUG_LEVEL > 3
		{
			GDALRasterBandH _grb = NULL;
			double _min;
			double _max;
			double _mean;
			double _stddev;

			_grb = GDALGetRasterBand(ds, i + 1);
			GDALComputeRasterStatistics(_grb, FALSE, &_min, &_max, &_mean, &_stddev, NULL, NULL);
			RASTER_DEBUGF(4, "GDAL Band %d stats: %f, %f, %f, %f", i + 1, _min, _max, _mean, _stddev);
		}
#endif

	}

	/* necessary??? */
	GDALFlushCache(ds);

	if (allocBandNums) rtdealloc(bandNums);
	if (allocNodataValues) rtdealloc(excludeNodataValues);

	return ds;
}

/**
 * Return a raster from a GDAL dataset
 *
 * @param ds : the GDAL dataset to convert to a raster
 *
 * @return raster or NULL
 */
rt_raster
rt_raster_from_gdal_dataset(GDALDatasetH ds) {
	rt_raster rast = NULL;
	double gt[6] = {0};
	CPLErr cplerr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t numBands = 0;
	int i = 0;
	char *authname = NULL;
	char *authcode = NULL;

	GDALRasterBandH gdband = NULL;
	GDALDataType gdpixtype = GDT_Unknown;
	rt_band band;
	int32_t idx;
	rt_pixtype pt = PT_END;
	uint32_t ptlen = 0;
	int hasnodata = 0;
	double nodataval;

	int x;
	int y;

	int nXBlocks, nYBlocks;
	int nXBlockSize, nYBlockSize;
	int iXBlock, iYBlock;
	int nXValid, nYValid;
	int iY;

	void *values = NULL;
	uint32_t valueslen = 0;
	void *ptr = NULL;

	assert(NULL != ds);

	/* raster size */
	width = GDALGetRasterXSize(ds);
	height = GDALGetRasterYSize(ds);
	RASTER_DEBUGF(3, "Raster dimensions (width x height): %d x %d", width, height);

	/* create new raster */
	RASTER_DEBUG(3, "Creating new raster");
	rast = rt_raster_new(width, height);
	if (NULL == rast) {
		rterror("rt_raster_from_gdal_dataset: Out of memory allocating new raster");
		return NULL;
	}
	RASTER_DEBUGF(3, "Created raster dimensions (width x height): %d x %d", rast->width, rast->height);

	/* get raster attributes */
	cplerr = GDALGetGeoTransform(ds, gt);
	if (GDALGetGeoTransform(ds, gt) != CE_None) {
		RASTER_DEBUG(4, "Using default geotransform matrix (0, 1, 0, 0, 0, -1)");
		gt[0] = 0;
		gt[1] = 1;
		gt[2] = 0;
		gt[3] = 0;
		gt[4] = 0;
		gt[5] = -1;
	}

	/* apply raster attributes */
	rt_raster_set_geotransform_matrix(rast, gt);

	RASTER_DEBUGF(3, "Raster geotransform (%f, %f, %f, %f, %f, %f)",
		gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);

	/* srid */
	if (rt_util_gdal_sr_auth_info(ds, &authname, &authcode) == ES_NONE) {
		if (
			authname != NULL &&
			strcmp(authname, "EPSG") == 0 &&
			authcode != NULL
		) {
			rt_raster_set_srid(rast, atoi(authcode));
			RASTER_DEBUGF(3, "New raster's SRID = %d", rast->srid);
		}

		if (authname != NULL)
			rtdealloc(authname);
		if (authcode != NULL)
			rtdealloc(authcode);
	}

	numBands = GDALGetRasterCount(ds);

#if POSTGIS_DEBUG_LEVEL > 3
	for (i = 1; i <= numBands; i++) {
		GDALRasterBandH _grb = NULL;
		double _min;
		double _max;
		double _mean;
		double _stddev;

		_grb = GDALGetRasterBand(ds, i);
		GDALComputeRasterStatistics(_grb, FALSE, &_min, &_max, &_mean, &_stddev, NULL, NULL);
		RASTER_DEBUGF(4, "GDAL Band %d stats: %f, %f, %f, %f", i, _min, _max, _mean, _stddev);
	}
#endif

	/* copy bands */
	for (i = 1; i <= numBands; i++) {
		RASTER_DEBUGF(3, "Processing band %d of %d", i, numBands);
		gdband = NULL;
		gdband = GDALGetRasterBand(ds, i);
		if (NULL == gdband) {
			rterror("rt_raster_from_gdal_dataset: Could not get GDAL band");
			rt_raster_destroy(rast);
			return NULL;
		}
		RASTER_DEBUGF(4, "gdband @ %p", gdband);

		/* pixtype */
		gdpixtype = GDALGetRasterDataType(gdband);
		RASTER_DEBUGF(4, "gdpixtype, size = %s, %d", GDALGetDataTypeName(gdpixtype), GDALGetDataTypeSize(gdpixtype) / 8);
		pt = rt_util_gdal_datatype_to_pixtype(gdpixtype);
		if (pt == PT_END) {
			rterror("rt_raster_from_gdal_dataset: Unknown pixel type for GDAL band");
			rt_raster_destroy(rast);
			return NULL;
		}
		ptlen = rt_pixtype_size(pt);

		/* size: width and height */
		width = GDALGetRasterBandXSize(gdband);
		height = GDALGetRasterBandYSize(gdband);
		RASTER_DEBUGF(3, "GDAL band dimensions (width x height): %d x %d", width, height);

		/* nodata */
		nodataval = GDALGetRasterNoDataValue(gdband, &hasnodata);
		RASTER_DEBUGF(3, "(hasnodata, nodataval) = (%d, %f)", hasnodata, nodataval);

		/* create band object */
		idx = rt_raster_generate_new_band(
			rast, pt,
			(hasnodata ? nodataval : 0),
			hasnodata, nodataval, rt_raster_get_num_bands(rast)
		);
		if (idx < 0) {
			rterror("rt_raster_from_gdal_dataset: Could not allocate memory for raster band");
			rt_raster_destroy(rast);
			return NULL;
		}
		band = rt_raster_get_band(rast, idx);
		RASTER_DEBUGF(3, "Created band of dimension (width x height): %d x %d", band->width, band->height);

		/* this makes use of GDAL's "natural" blocks */
		GDALGetBlockSize(gdband, &nXBlockSize, &nYBlockSize);
		nXBlocks = (width + nXBlockSize - 1) / nXBlockSize;
		nYBlocks = (height + nYBlockSize - 1) / nYBlockSize;
		RASTER_DEBUGF(4, "(nXBlockSize, nYBlockSize) = (%d, %d)", nXBlockSize, nYBlockSize);
		RASTER_DEBUGF(4, "(nXBlocks, nYBlocks) = (%d, %d)", nXBlocks, nYBlocks);

		/* allocate memory for values */
		valueslen = ptlen * nXBlockSize * nYBlockSize;
		switch (gdpixtype) {
			case GDT_Byte:
				values = (uint8_t *) rtalloc(valueslen);
				break;
			case GDT_UInt16:
				values = (uint16_t *) rtalloc(valueslen);
				break;
			case GDT_Int16:
				values = (int16_t *) rtalloc(valueslen);
				break;
			case GDT_UInt32:
				values = (uint32_t *) rtalloc(valueslen);
				break;
			case GDT_Int32:
				values = (int32_t *) rtalloc(valueslen);
				break;
			case GDT_Float32:
				values = (float *) rtalloc(valueslen);
				break;
			case GDT_Float64:
				values = (double *) rtalloc(valueslen);
				break;
			default:
				/* should NEVER get here */
				rterror("rt_raster_from_gdal_dataset: Could not allocate memory for unknown pixel type");
				rt_raster_destroy(rast);
				return NULL;
		}
		if (values == NULL) {
			rterror("rt_raster_from_gdal_dataset: Could not allocate memory for GDAL band pixel values");
			rt_raster_destroy(rast);
			return NULL;
		}
		RASTER_DEBUGF(3, "values @ %p of length = %d", values, valueslen);

		for (iYBlock = 0; iYBlock < nYBlocks; iYBlock++) {
			for (iXBlock = 0; iXBlock < nXBlocks; iXBlock++) {
				x = iXBlock * nXBlockSize;
				y = iYBlock * nYBlockSize;
				RASTER_DEBUGF(4, "(iXBlock, iYBlock) = (%d, %d)", iXBlock, iYBlock);
				RASTER_DEBUGF(4, "(x, y) = (%d, %d)", x, y);

				memset(values, 0, valueslen);

				/* valid block width */
				if ((iXBlock + 1) * nXBlockSize > width)
					nXValid = width - (iXBlock * nXBlockSize);
				else
					nXValid = nXBlockSize;

				/* valid block height */
				if ((iYBlock + 1) * nYBlockSize > height)
					nYValid = height - (iYBlock * nYBlockSize);
				else
					nYValid = nYBlockSize;

				RASTER_DEBUGF(4, "(nXValid, nYValid) = (%d, %d)", nXValid, nYValid);

				cplerr = GDALRasterIO(
					gdband, GF_Read,
					x, y,
					nXValid, nYValid,
					values, nXValid, nYValid,
					gdpixtype,
					0, 0
				);
				if (cplerr != CE_None) {
					rterror("rt_raster_from_gdal_dataset: Could not get data from GDAL raster");
					rtdealloc(values);
					rt_raster_destroy(rast);
					return NULL;
				}

				/* if block width is same as raster width, shortcut */
				if (nXBlocks == 1 && nYBlockSize > 1 && nXValid == width) {
					x = 0;
					y = nYBlockSize * iYBlock;

					RASTER_DEBUGF(4, "Setting set of pixel lines at (%d, %d) for %d pixels", x, y, nXValid * nYValid);
					rt_band_set_pixel_line(band, x, y, values, nXValid * nYValid);
				}
				else {
					ptr = values;
					x = nXBlockSize * iXBlock;
					for (iY = 0; iY < nYValid; iY++) {
						y = iY + (nYBlockSize * iYBlock);

						RASTER_DEBUGF(4, "Setting pixel line at (%d, %d) for %d pixels", x, y, nXValid);
						rt_band_set_pixel_line(band, x, y, ptr, nXValid);
						ptr += (nXValid * ptlen);
					}
				}
			}
		}

		/* free memory */
		rtdealloc(values);
	}

	return rast;
}

/******************************************************************************
* rt_raster_gdal_warp()
******************************************************************************/

typedef struct _rti_warp_arg_t* _rti_warp_arg;
struct _rti_warp_arg_t {

	struct {
		GDALDriverH drv;
		GDALDatasetH ds;
		char *srs;
	} src, dst;

	GDALWarpOptions *wopts;

	struct {
		struct {
			char **item;
			int len;
		} option;

		struct {
			void *transform;
			void *imgproj;
			void *approx;
		} arg;

		GDALTransformerFunc func;
	} transform;

};

static _rti_warp_arg
_rti_warp_arg_init() {
	_rti_warp_arg arg = NULL;

	arg = rtalloc(sizeof(struct _rti_warp_arg_t));
	if (arg == NULL) {
		rterror("_rti_warp_arg_init: Could not allocate memory for _rti_warp_arg");
		return NULL;
	}

	arg->src.drv = NULL;
	arg->src.ds = NULL;
	arg->src.srs = NULL;

	arg->dst.drv = NULL;
	arg->dst.ds = NULL;
	arg->dst.srs = NULL;

	arg->wopts = NULL;

	arg->transform.option.item = NULL;
	arg->transform.option.len = 0;

	arg->transform.arg.transform = NULL;
	arg->transform.arg.imgproj = NULL;
	arg->transform.arg.approx = NULL;

	arg->transform.func = NULL;

	return arg;
}

static void
_rti_warp_arg_destroy(_rti_warp_arg arg) {
	int i = 0;

	if (arg->dst.ds != NULL)
		GDALClose(arg->dst.ds);
	if (arg->dst.srs != NULL)
		CPLFree(arg->dst.srs);

	if (arg->src.ds != NULL)
		GDALClose(arg->src.ds);
	if (arg->src.srs != NULL)
		CPLFree(arg->src.srs);

	if (arg->transform.func == GDALApproxTransform) {
		if (arg->transform.arg.imgproj != NULL)
			GDALDestroyGenImgProjTransformer(arg->transform.arg.imgproj);
	}

	if (arg->wopts != NULL)
		GDALDestroyWarpOptions(arg->wopts);

	if (arg->transform.option.len > 0 && arg->transform.option.item != NULL) {
		for (i = 0; i < arg->transform.option.len; i++) {
			if (arg->transform.option.item[i] != NULL)
				rtdealloc(arg->transform.option.item[i]);
		}
		rtdealloc(arg->transform.option.item);
	}

	rtdealloc(arg);
	arg = NULL;
}

/**
 * Return a warped raster using GDAL Warp API
 *
 * @param raster : raster to transform
 * @param src_srs : the raster's coordinate system in OGC WKT
 * @param dst_srs : the warped raster's coordinate system in OGC WKT
 * @param scale_x : the x size of pixels of the warped raster's pixels in
 *   units of dst_srs
 * @param scale_y : the y size of pixels of the warped raster's pixels in
 *   units of dst_srs
 * @param width : the number of columns of the warped raster.  note that
 *   width/height CANNOT be used with scale_x/scale_y
 * @param height : the number of rows of the warped raster.  note that
 *   width/height CANNOT be used with scale_x/scale_y
 * @param ul_xw : the X value of upper-left corner of the warped raster in
 *   units of dst_srs
 * @param ul_yw : the Y value of upper-left corner of the warped raster in
 *   units of dst_srs
 * @param grid_xw : the X value of point on a grid to align warped raster
 *   to in units of dst_srs
 * @param grid_yw : the Y value of point on a grid to align warped raster
 *   to in units of dst_srs
 * @param skew_x : the X skew of the warped raster in units of dst_srs
 * @param skew_y : the Y skew of the warped raster in units of dst_srs
 * @param resample_alg : the resampling algorithm
 * @param max_err : maximum error measured in input pixels permitted
 *   (0.0 for exact calculations)
 *
 * @return the warped raster or NULL
 */
rt_raster rt_raster_gdal_warp(
	rt_raster raster,
	const char *src_srs, const char *dst_srs,
	double *scale_x, double *scale_y,
	int *width, int *height,
	double *ul_xw, double *ul_yw,
	double *grid_xw, double *grid_yw,
	double *skew_x, double *skew_y,
	GDALResampleAlg resample_alg, double max_err
) {
	CPLErr cplerr;
	char *dst_options[] = {"SUBCLASS=VRTWarpedDataset", NULL};
	_rti_warp_arg arg = NULL;

	int hasnodata = 0;

	GDALRasterBandH band;
	rt_band rtband = NULL;
	rt_pixtype pt = PT_END;
	GDALDataType gdal_pt = GDT_Unknown;
	double nodata = 0.0;

	double _gt[6] = {0};
	double dst_extent[4];
	rt_envelope extent;

	int _dim[2] = {0};
	double _skew[2] = {0};
	double _scale[2] = {0};
	int ul_user = 0;

	rt_raster rast = NULL;
	int i = 0;
	int numBands = 0;

	int subgt = 0;

	RASTER_DEBUG(3, "starting");

	assert(NULL != raster);

	/* internal variables */
	arg = _rti_warp_arg_init();
	if (arg == NULL) {
		rterror("rt_raster_gdal_warp: Could not initialize internal variables");
		return NULL;
	}

	/*
		max_err must be gte zero

		the value 0.125 is the default used in gdalwarp.cpp on line 283
	*/
	if (max_err < 0.) max_err = 0.125;
	RASTER_DEBUGF(4, "max_err = %f", max_err);

	/* handle srs */
	if (src_srs != NULL) {
		/* reprojection taking place */
		if (dst_srs != NULL && strcmp(src_srs, dst_srs) != 0) {
			RASTER_DEBUG(4, "Warp operation does include a reprojection");
			arg->src.srs = rt_util_gdal_convert_sr(src_srs, 0);
			arg->dst.srs = rt_util_gdal_convert_sr(dst_srs, 0);

			if (arg->src.srs == NULL || arg->dst.srs == NULL) {
				rterror("rt_raster_gdal_warp: Could not convert srs values to GDAL accepted format");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}
		}
		/* no reprojection, a stub just for clarity */
		else {
			RASTER_DEBUG(4, "Warp operation does NOT include reprojection");
		}
	}
	else if (dst_srs != NULL) {
		/* dst_srs provided but not src_srs */
		rterror("rt_raster_gdal_warp: SRS required for input raster if SRS provided for warped raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* load raster into a GDAL MEM dataset */
	arg->src.ds = rt_raster_to_gdal_mem(raster, arg->src.srs, NULL, NULL, 0, &(arg->src.drv));
	if (NULL == arg->src.ds) {
		rterror("rt_raster_gdal_warp: Could not convert raster to GDAL MEM format");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	RASTER_DEBUG(3, "raster loaded into GDAL MEM dataset");

	/* special case when src_srs and dst_srs is NULL and raster's geotransform matrix is default */
	if (src_srs == NULL && dst_srs == NULL && rt_raster_get_srid(raster) == SRID_UNKNOWN) {
		double gt[6];

#if POSTGIS_DEBUG_LEVEL > 3
		GDALGetGeoTransform(arg->src.ds, gt);
		RASTER_DEBUGF(3, "GDAL MEM geotransform: %f, %f, %f, %f, %f, %f",
			gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);
#endif

		/* default geotransform */
		rt_raster_get_geotransform_matrix(raster, gt);
		RASTER_DEBUGF(3, "raster geotransform: %f, %f, %f, %f, %f, %f",
			gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);

		if (
			FLT_EQ(gt[0], 0) &&
			FLT_EQ(gt[1], 1) &&
			FLT_EQ(gt[2], 0) &&
			FLT_EQ(gt[3], 0) &&
			FLT_EQ(gt[4], 0) &&
			FLT_EQ(gt[5], -1)
		) {
			double ngt[6] = {0, 10, 0, 0, 0, -10};

			rtinfo("Raster has default geotransform. Adjusting metadata for use of GDAL Warp API");

			GDALSetGeoTransform(arg->src.ds, ngt);
			GDALFlushCache(arg->src.ds);

			subgt = 1;

#if POSTGIS_DEBUG_LEVEL > 3
			GDALGetGeoTransform(arg->src.ds, gt);
			RASTER_DEBUGF(3, "GDAL MEM geotransform: %f, %f, %f, %f, %f, %f",
				gt[0], gt[1], gt[2], gt[3], gt[4], gt[5]);
#endif
		}
	}

	/* set transform options */
	if (arg->src.srs != NULL || arg->dst.srs != NULL) {
		arg->transform.option.len = 2;
		arg->transform.option.item = rtalloc(sizeof(char *) * (arg->transform.option.len + 1));
		if (NULL == arg->transform.option.item) {
			rterror("rt_raster_gdal_warp: Could not allocation memory for transform options");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		memset(arg->transform.option.item, 0, sizeof(char *) * (arg->transform.option.len + 1));

		for (i = 0; i < arg->transform.option.len; i++) {
			switch (i) {
				case 1:
					if (arg->dst.srs != NULL)
						arg->transform.option.item[i] = (char *) rtalloc(sizeof(char) * (strlen("DST_SRS=") + strlen(arg->dst.srs) + 1));
					else
						arg->transform.option.item[i] = (char *) rtalloc(sizeof(char) * (strlen("DST_SRS=") + 1));
					break;
				case 0:
					if (arg->src.srs != NULL)
						arg->transform.option.item[i] = (char *) rtalloc(sizeof(char) * (strlen("SRC_SRS=") + strlen(arg->src.srs) + 1));
					else
						arg->transform.option.item[i] = (char *) rtalloc(sizeof(char) * (strlen("SRC_SRS=") + 1));
					break;
			}
			if (NULL == arg->transform.option.item[i]) {
				rterror("rt_raster_gdal_warp: Could not allocation memory for transform options");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			switch (i) {
				case 1:
					if (arg->dst.srs != NULL) {
						snprintf(
							arg->transform.option.item[i],
							sizeof(char) * (strlen("DST_SRS=") + strlen(arg->dst.srs) + 1),
							"DST_SRS=%s",
							arg->dst.srs
						);
					}
					else
						sprintf(arg->transform.option.item[i], "%s", "DST_SRS=");
					break;
				case 0:
					if (arg->src.srs != NULL) {
						snprintf(
							arg->transform.option.item[i],
							sizeof(char) * (strlen("SRC_SRS=") + strlen(arg->src.srs) + 1),
							"SRC_SRS=%s",
							arg->src.srs
						);
					}
					else
						sprintf(arg->transform.option.item[i], "%s", "SRC_SRS=");
					break;
			}
			RASTER_DEBUGF(4, "arg->transform.option.item[%d] = %s", i, arg->transform.option.item[i]);
		}
	}
	else
		arg->transform.option.len = 0;

	/* transformation object for building dst dataset */
	arg->transform.arg.transform = GDALCreateGenImgProjTransformer2(arg->src.ds, NULL, arg->transform.option.item);
	if (NULL == arg->transform.arg.transform) {
		rterror("rt_raster_gdal_warp: Could not create GDAL transformation object for output dataset creation");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* get approximate output georeferenced bounds and resolution */
	cplerr = GDALSuggestedWarpOutput2(
		arg->src.ds, GDALGenImgProjTransform,
		arg->transform.arg.transform, _gt, &(_dim[0]), &(_dim[1]), dst_extent, 0);
	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_warp: Could not get GDAL suggested warp output for output dataset creation");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	GDALDestroyGenImgProjTransformer(arg->transform.arg.transform);
	arg->transform.arg.transform = NULL;

	/*
		don't use suggested dimensions as use of suggested scales
		on suggested extent will result in suggested dimensions
	*/
	_dim[0] = 0;
	_dim[1] = 0;

	RASTER_DEBUGF(3, "Suggested geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]);

	/* store extent in easier-to-use object */
	extent.MinX = dst_extent[0];
	extent.MinY = dst_extent[1];
	extent.MaxX = dst_extent[2];
	extent.MaxY = dst_extent[3];

	extent.UpperLeftX = dst_extent[0];
	extent.UpperLeftY = dst_extent[3];

	RASTER_DEBUGF(3, "Suggested extent: %f, %f, %f, %f",
		extent.MinX, extent.MinY, extent.MaxX, extent.MaxY);

	/* scale and width/height are mutually exclusive */
	if (
		((NULL != scale_x) || (NULL != scale_y)) &&
		((NULL != width) || (NULL != height))
	) {
		rterror("rt_raster_gdal_warp: Scale X/Y and width/height are mutually exclusive.  Only provide one");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* user-defined width */
	if (NULL != width) {
		_dim[0] = abs(*width);
		_scale[0] = fabs((extent.MaxX - extent.MinX) / ((double) _dim[0]));
	}
	/* user-defined height */
	if (NULL != height) {
		_dim[1] = abs(*height);
		_scale[1] = fabs((extent.MaxY - extent.MinY) / ((double) _dim[1]));
	}

	/* user-defined scale */
	if (
		((NULL != scale_x) && (FLT_NEQ(*scale_x, 0.0))) &&
		((NULL != scale_y) && (FLT_NEQ(*scale_y, 0.0)))
	) {
		_scale[0] = fabs(*scale_x);
		_scale[1] = fabs(*scale_y);

		/* special override */
		if (subgt) {
			_scale[0] *= 10;
			_scale[1] *= 10;
		}
	}
	else if (
		((NULL != scale_x) && (NULL == scale_y)) ||
		((NULL == scale_x) && (NULL != scale_y))
	) {
		rterror("rt_raster_gdal_warp: Both X and Y scale values must be provided for scale");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* scale not defined, use suggested */
	if (FLT_EQ(_scale[0], 0) && FLT_EQ(_scale[1], 0)) {
		_scale[0] = fabs(_gt[1]);
		_scale[1] = fabs(_gt[5]);
	}

	RASTER_DEBUGF(4, "Using scale: %f x %f", _scale[0], -1 * _scale[1]);

	/* user-defined skew */
	if (NULL != skew_x) {
		_skew[0] = *skew_x;

		/*
			negative scale-x affects skew
			for now, force skew to be in left-right, top-down orientation
		*/
		if (
			NULL != scale_x &&
			*scale_x < 0.
		) {
			_skew[0] *= -1;
		}
	}
	if (NULL != skew_y) {
		_skew[1] = *skew_y;

		/*
			positive scale-y affects skew
			for now, force skew to be in left-right, top-down orientation
		*/
		if (
			NULL != scale_y &&
			*scale_y > 0.
		) {
			_skew[1] *= -1;
		}
	}

	RASTER_DEBUGF(4, "Using skew: %f x %f", _skew[0], _skew[1]);

	/* reprocess extent if skewed */
	if (
		FLT_NEQ(_skew[0], 0) ||
		FLT_NEQ(_skew[1], 0)
	) {
		rt_raster skewedrast;

		RASTER_DEBUG(3, "Computing skewed extent's envelope");

		skewedrast = rt_raster_compute_skewed_raster(
			extent,
			_skew,
			_scale,
			0.01
		);
		if (skewedrast == NULL) {
			rterror("rt_raster_gdal_warp: Could not compute skewed raster");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		if (_dim[0] == 0)
			_dim[0] = skewedrast->width;
		if (_dim[1] == 0)
			_dim[1] = skewedrast->height;

		extent.UpperLeftX = skewedrast->ipX;
		extent.UpperLeftY = skewedrast->ipY;

		rt_raster_destroy(skewedrast);
	}

	/* dimensions not defined, compute */
	if (!_dim[0])
		_dim[0] = (int) fmax((fabs(extent.MaxX - extent.MinX) + (_scale[0] / 2.)) / _scale[0], 1);
	if (!_dim[1])
		_dim[1] = (int) fmax((fabs(extent.MaxY - extent.MinY) + (_scale[1] / 2.)) / _scale[1], 1);

	/* temporary raster */
	rast = rt_raster_new(_dim[0], _dim[1]);
	if (rast == NULL) {
		rterror("rt_raster_gdal_warp: Out of memory allocating temporary raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* set raster's spatial attributes */
	rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);
	rt_raster_set_scale(rast, _scale[0], -1 * _scale[1]);
	rt_raster_set_skews(rast, _skew[0], _skew[1]);

	rt_raster_get_geotransform_matrix(rast, _gt);
	RASTER_DEBUGF(3, "Temp raster's geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]);
	RASTER_DEBUGF(3, "Temp raster's dimensions (width x height): %d x %d",
		_dim[0], _dim[1]);

	/* user-defined upper-left corner */
	if (
		NULL != ul_xw &&
		NULL != ul_yw
	) {
		ul_user = 1;

		RASTER_DEBUGF(4, "Using user-specified upper-left corner: %f, %f", *ul_xw, *ul_yw);

		/* set upper-left corner */
		rt_raster_set_offsets(rast, *ul_xw, *ul_yw);
		extent.UpperLeftX = *ul_xw;
		extent.UpperLeftY = *ul_yw;
	}
	else if (
		((NULL != ul_xw) && (NULL == ul_yw)) ||
		((NULL == ul_xw) && (NULL != ul_yw))
	) {
		rterror("rt_raster_gdal_warp: Both X and Y upper-left corner values must be provided");
		rt_raster_destroy(rast);
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* alignment only considered if upper-left corner not provided */
	if (
		!ul_user && (
			(NULL != grid_xw) || (NULL != grid_yw)
		)
	) {

		if (
			((NULL != grid_xw) && (NULL == grid_yw)) ||
			((NULL == grid_xw) && (NULL != grid_yw))
		) {
			rterror("rt_raster_gdal_warp: Both X and Y alignment values must be provided");
			rt_raster_destroy(rast);
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		RASTER_DEBUGF(4, "Aligning extent to user-specified grid: %f, %f", *grid_xw, *grid_yw);

		do {
			double _r[2] = {0};
			double _w[2] = {0};

			/* raster is already aligned */
			if (FLT_EQ(*grid_xw, extent.UpperLeftX) && FLT_EQ(*grid_yw, extent.UpperLeftY)) {
				RASTER_DEBUG(3, "Skipping raster alignment as it is already aligned to grid");
				break;
			}

			extent.UpperLeftX = rast->ipX;
			extent.UpperLeftY = rast->ipY;
			rt_raster_set_offsets(rast, *grid_xw, *grid_yw);

			/* process upper-left corner */
			if (rt_raster_geopoint_to_cell(
				rast,
				extent.UpperLeftX, extent.UpperLeftY,
				&(_r[0]), &(_r[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_warp: Could not compute raster pixel for spatial coordinates");
				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			if (rt_raster_cell_to_geopoint(
				rast,
				_r[0], _r[1],
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");

				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			/* shift occurred */
			if (FLT_NEQ(_w[0], extent.UpperLeftX)) {
				if (NULL == width)
					rast->width++;
				else if (NULL == scale_x) {
					double _c[2] = {0};

					rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);

					/* get upper-right corner */
					if (rt_raster_cell_to_geopoint(
						rast,
						rast->width, 0,
						&(_c[0]), &(_c[1]),
						NULL
					) != ES_NONE) {
						rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");
						rt_raster_destroy(rast);
						_rti_warp_arg_destroy(arg);
						return NULL;
					}

					rast->scaleX = fabs((_c[0] - _w[0]) / ((double) rast->width));
				}
			}
			if (FLT_NEQ(_w[1], extent.UpperLeftY)) {
				if (NULL == height)
					rast->height++;
				else if (NULL == scale_y) {
					double _c[2] = {0};

					rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);

					/* get upper-right corner */
					if (rt_raster_cell_to_geopoint(
						rast,
						0, rast->height,
						&(_c[0]), &(_c[1]),
						NULL
					) != ES_NONE) {
						rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");

						rt_raster_destroy(rast);
						_rti_warp_arg_destroy(arg);
						return NULL;
					}

					rast->scaleY = -1 * fabs((_c[1] - _w[1]) / ((double) rast->height));
				}
			}

			rt_raster_set_offsets(rast, _w[0], _w[1]);
			RASTER_DEBUGF(4, "aligned offsets: %f x %f", _w[0], _w[1]);
		}
		while (0);
	}

	/*
		after this point, rt_envelope extent is no longer used
	*/

	/* get key attributes from rast */
	_dim[0] = rast->width;
	_dim[1] = rast->height;
	rt_raster_get_geotransform_matrix(rast, _gt);

	/* scale-x is negative or scale-y is positive */
	if ((
		(NULL != scale_x) && (*scale_x < 0.)
	) || (
		(NULL != scale_y) && (*scale_y > 0)
	)) {
		double _w[2] = {0};

		/* negative scale-x */
		if (
			(NULL != scale_x) &&
			(*scale_x < 0.)
		) {
			if (rt_raster_cell_to_geopoint(
				rast,
				rast->width, 0,
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");
				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			_gt[0] = _w[0];
			_gt[1] = *scale_x;

			/* check for skew */
			if (NULL != skew_x && FLT_NEQ(*skew_x, 0))
				_gt[2] = *skew_x;
		}
		/* positive scale-y */
		if (
			(NULL != scale_y) &&
			(*scale_y > 0)
		) {
			if (rt_raster_cell_to_geopoint(
				rast,
				0, rast->height,
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");
				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			_gt[3] = _w[1];
			_gt[5] = *scale_y;

			/* check for skew */
			if (NULL != skew_y && FLT_NEQ(*skew_y, 0))
				_gt[4] = *skew_y;
		}
	}

	rt_raster_destroy(rast);
	rast = NULL;

	RASTER_DEBUGF(3, "Applied geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]);
	RASTER_DEBUGF(3, "Raster dimensions (width x height): %d x %d",
		_dim[0], _dim[1]);

	if (FLT_EQ(_dim[0], 0) || FLT_EQ(_dim[1], 0)) {
		rterror("rt_raster_gdal_warp: The width (%f) or height (%f) of the warped raster is zero", _dim[0], _dim[1]);
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* load VRT driver */
	if (!rt_util_gdal_driver_registered("VRT"))
		GDALRegister_VRT();
	arg->dst.drv = GDALGetDriverByName("VRT");
	if (NULL == arg->dst.drv) {
		rterror("rt_raster_gdal_warp: Could not load the output GDAL VRT driver");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* create dst dataset */
	arg->dst.ds = GDALCreate(arg->dst.drv, "", _dim[0], _dim[1], 0, GDT_Byte, dst_options);
	if (NULL == arg->dst.ds) {
		rterror("rt_raster_gdal_warp: Could not create GDAL VRT dataset");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* set dst srs */
	if (arg->dst.srs != NULL) {
		cplerr = GDALSetProjection(arg->dst.ds, arg->dst.srs);
		if (cplerr != CE_None) {
			rterror("rt_raster_gdal_warp: Could not set projection");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		RASTER_DEBUGF(3, "Applied SRS: %s", GDALGetProjectionRef(arg->dst.ds));
	}

	/* set dst geotransform */
	cplerr = GDALSetGeoTransform(arg->dst.ds, _gt);
	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_warp: Could not set geotransform");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* add bands to dst dataset */
	numBands = rt_raster_get_num_bands(raster);
	for (i = 0; i < numBands; i++) {
		rtband = rt_raster_get_band(raster, i);
		if (NULL == rtband) {
			rterror("rt_raster_gdal_warp: Could not get band %d for adding to VRT dataset", i);
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		pt = rt_band_get_pixtype(rtband);
		gdal_pt = rt_util_pixtype_to_gdal_datatype(pt);
		if (gdal_pt == GDT_Unknown)
			rtwarn("rt_raster_gdal_warp: Unknown pixel type for band %d", i);

		cplerr = GDALAddBand(arg->dst.ds, gdal_pt, NULL);
		if (cplerr != CE_None) {
			rterror("rt_raster_gdal_warp: Could not add band to VRT dataset");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		/* get band to write data to */
		band = NULL;
		band = GDALGetRasterBand(arg->dst.ds, i + 1);
		if (NULL == band) {
			rterror("rt_raster_gdal_warp: Could not get GDAL band for additional processing");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		/* set nodata */
		if (rt_band_get_hasnodata_flag(rtband) != FALSE) {
			hasnodata = 1;
			rt_band_get_nodata(rtband, &nodata);
			if (GDALSetRasterNoDataValue(band, nodata) != CE_None)
				rtwarn("rt_raster_gdal_warp: Could not set nodata value for band %d", i);
			RASTER_DEBUGF(3, "nodata value set to %f", GDALGetRasterNoDataValue(band, NULL));
		}
	}

	/* create transformation object */
	arg->transform.arg.transform = arg->transform.arg.imgproj = GDALCreateGenImgProjTransformer2(
		arg->src.ds, arg->dst.ds,
		arg->transform.option.item
	);
	if (NULL == arg->transform.arg.transform) {
		rterror("rt_raster_gdal_warp: Could not create GDAL transformation object");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	arg->transform.func = GDALGenImgProjTransform;

	/* use approximate transformation object */
	if (max_err > 0.0) {
		arg->transform.arg.transform = arg->transform.arg.approx = GDALCreateApproxTransformer(
			GDALGenImgProjTransform,
			arg->transform.arg.imgproj, max_err
		);
		if (NULL == arg->transform.arg.transform) {
			rterror("rt_raster_gdal_warp: Could not create GDAL approximate transformation object");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		arg->transform.func = GDALApproxTransform;
	}

	/* warp options */
	arg->wopts = GDALCreateWarpOptions();
	if (NULL == arg->wopts) {
		rterror("rt_raster_gdal_warp: Could not create GDAL warp options object");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* set options */
	arg->wopts->eResampleAlg = resample_alg;
	arg->wopts->hSrcDS = arg->src.ds;
	arg->wopts->hDstDS = arg->dst.ds;
	arg->wopts->pfnTransformer = arg->transform.func;
	arg->wopts->pTransformerArg = arg->transform.arg.transform;
	arg->wopts->papszWarpOptions = (char **) CPLMalloc(sizeof(char *) * 2);
	arg->wopts->papszWarpOptions[0] = (char *) CPLMalloc(sizeof(char) * (strlen("INIT_DEST=NO_DATA") + 1));
	strcpy(arg->wopts->papszWarpOptions[0], "INIT_DEST=NO_DATA");
	arg->wopts->papszWarpOptions[1] = NULL;

	/* set band mapping */
	arg->wopts->nBandCount = numBands;
	arg->wopts->panSrcBands = (int *) CPLMalloc(sizeof(int) * arg->wopts->nBandCount);
	arg->wopts->panDstBands = (int *) CPLMalloc(sizeof(int) * arg->wopts->nBandCount);
	for (i = 0; i < arg->wopts->nBandCount; i++)
		arg->wopts->panDstBands[i] = arg->wopts->panSrcBands[i] = i + 1;

	/* set nodata mapping */
	if (hasnodata) {
		RASTER_DEBUG(3, "Setting nodata mapping");
		arg->wopts->padfSrcNoDataReal = (double *) CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfDstNoDataReal = (double *) CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfSrcNoDataImag = (double *) CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfDstNoDataImag = (double *) CPLMalloc(numBands * sizeof(double));
		if (
			NULL == arg->wopts->padfSrcNoDataReal ||
			NULL == arg->wopts->padfDstNoDataReal ||
			NULL == arg->wopts->padfSrcNoDataImag ||
			NULL == arg->wopts->padfDstNoDataImag
		) {
			rterror("rt_raster_gdal_warp: Out of memory allocating nodata mapping");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		for (i = 0; i < numBands; i++) {
			band = rt_raster_get_band(raster, i);
			if (!band) {
				rterror("rt_raster_gdal_warp: Could not process bands for nodata values");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			if (!rt_band_get_hasnodata_flag(band)) {
				/*
					based on line 1004 of gdalwarp.cpp
					the problem is that there is a chance that this number is a legitimate value
				*/
				arg->wopts->padfSrcNoDataReal[i] = -123456.789;
			}
			else {
				rt_band_get_nodata(band, &(arg->wopts->padfSrcNoDataReal[i]));
			}

			arg->wopts->padfDstNoDataReal[i] = arg->wopts->padfSrcNoDataReal[i];
			arg->wopts->padfDstNoDataImag[i] = arg->wopts->padfSrcNoDataImag[i] = 0.0;
			RASTER_DEBUGF(4, "Mapped nodata value for band %d: %f (%f) => %f (%f)",
				i,
				arg->wopts->padfSrcNoDataReal[i], arg->wopts->padfSrcNoDataImag[i],
				arg->wopts->padfDstNoDataReal[i], arg->wopts->padfDstNoDataImag[i]
			);
		}
	}

	/* warp raster */
	RASTER_DEBUG(3, "Warping raster");
	cplerr = GDALInitializeWarpedVRT(arg->dst.ds, arg->wopts);
	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_warp: Could not warp raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	/*
	GDALSetDescription(arg->dst.ds, "/tmp/warped.vrt");
	*/
	GDALFlushCache(arg->dst.ds);
	RASTER_DEBUG(3, "Raster warped");

	/* convert gdal dataset to raster */
	RASTER_DEBUG(3, "Converting GDAL dataset to raster");
	rast = rt_raster_from_gdal_dataset(arg->dst.ds);

	_rti_warp_arg_destroy(arg);

	if (NULL == rast) {
		rterror("rt_raster_gdal_warp: Could not warp raster");
		return NULL;
	}

	/* substitute geotransform matrix, reset back to default */
	if (subgt) {
		double gt[6] = {0, 1, 0, 0, 0, -1};

		rt_raster_set_geotransform_matrix(rast, gt);
	}

	RASTER_DEBUG(3, "done");

	return rast;
}

/******************************************************************************
* rt_raster_gdal_rasterize()
******************************************************************************/

typedef struct _rti_rasterize_arg_t* _rti_rasterize_arg;
struct _rti_rasterize_arg_t {
	uint8_t noband;

	uint32_t numbands; 

	rt_pixtype *pixtype;
	double *init;
	double *nodata;
	uint8_t *hasnodata;
	double *value;
	int *bandlist;
};

static _rti_rasterize_arg
_rti_rasterize_arg_init() {
	_rti_rasterize_arg arg = NULL;

	arg = rtalloc(sizeof(struct _rti_rasterize_arg_t));
	if (arg == NULL) {
		rterror("_rti_rasterize_arg_init: Could not allocate memory for _rti_rasterize_arg");
		return NULL;
	}

	arg->noband = 0;

	arg->numbands = 0;
	arg->pixtype = NULL;
	arg->init = NULL;
	arg->nodata = NULL;
	arg->hasnodata = NULL;
	arg->value = NULL;
	arg->bandlist = NULL;

	return arg;
}

static void
_rti_rasterize_arg_destroy(_rti_rasterize_arg arg) {
	if (arg->noband) {
		if (arg->pixtype != NULL)
			rtdealloc(arg->pixtype);
		if (arg->init != NULL)
			rtdealloc(arg->init);
		if (arg->nodata != NULL)
			rtdealloc(arg->nodata);
		if (arg->hasnodata != NULL)
			rtdealloc(arg->hasnodata);
		if (arg->value != NULL)
			rtdealloc(arg->value);
	}

	if (arg->bandlist != NULL)
		rtdealloc(arg->bandlist);

	rtdealloc(arg);
}

/**
 * Return a raster of the provided geometry
 *
 * @param wkb : WKB representation of the geometry to convert
 * @param wkb_len : length of the WKB representation of the geometry
 * @param srs : the geometry's coordinate system in OGC WKT
 * @param num_bands : number of bands in the output raster
 * @param pixtype : data type of each band
 * @param init : array of values to initialize each band with
 * @param value : array of values for pixels of geometry
 * @param nodata : array of nodata values for each band
 * @param hasnodata : array flagging the presence of nodata for each band
 * @param width : the number of columns of the raster
 * @param height : the number of rows of the raster
 * @param scale_x : the pixel width of the raster
 * @param scale_y : the pixel height of the raster
 * @param ul_xw : the X value of upper-left corner of the raster
 * @param ul_yw : the Y value of upper-left corner of the raster
 * @param grid_xw : the X value of point on grid to align raster to
 * @param grid_yw : the Y value of point on grid to align raster to
 * @param skew_x : the X skew of the raster
 * @param skew_y : the Y skew of the raster
 * @param options : array of options.  only option is "ALL_TOUCHED"
 *
 * @return the raster of the provided geometry or NULL
 */
rt_raster
rt_raster_gdal_rasterize(
	const unsigned char *wkb, uint32_t wkb_len,
	const char *srs,
	uint32_t num_bands, rt_pixtype *pixtype,
	double *init, double *value,
	double *nodata, uint8_t *hasnodata,
	int *width, int *height,
	double *scale_x, double *scale_y,
	double *ul_xw, double *ul_yw,
	double *grid_xw, double *grid_yw,
	double *skew_x, double *skew_y,
	char **options
) {
	rt_raster rast = NULL;
	int i = 0;
	int err = 0;

	_rti_rasterize_arg arg = NULL;

	int _dim[2] = {0};
	double _scale[2] = {0};
	double _skew[2] = {0};

	OGRErr ogrerr;
	OGRSpatialReferenceH src_sr = NULL;
	OGRGeometryH src_geom;
	OGREnvelope src_env;
	rt_envelope extent;
	OGRwkbGeometryType wkbtype = wkbUnknown;

	int ul_user = 0;

	CPLErr cplerr;
	double _gt[6] = {0};
	GDALDriverH _drv = NULL;
	GDALDatasetH _ds = NULL;
	GDALRasterBandH _band = NULL;

	uint16_t _width = 0;
	uint16_t _height = 0;

	RASTER_DEBUG(3, "starting");

	assert(NULL != wkb);
	assert(0 != wkb_len);

	/* internal variables */
	arg = _rti_rasterize_arg_init();
	if (arg == NULL) {
		rterror("rt_raster_gdal_rasterize: Could not initialize internal variables");
		return NULL;
	}

	/* no bands, raster is a mask */
	if (num_bands < 1) {
		arg->noband = 1;
		arg->numbands = 1;

		arg->pixtype = (rt_pixtype *) rtalloc(sizeof(rt_pixtype));
		arg->pixtype[0] = PT_8BUI;

		arg->init = (double *) rtalloc(sizeof(double));
		arg->init[0] = 0;

		arg->nodata = (double *) rtalloc(sizeof(double));
		arg->nodata[0] = 0;

		arg->hasnodata = (uint8_t *) rtalloc(sizeof(uint8_t));
		arg->hasnodata[0] = 1;

		arg->value = (double *) rtalloc(sizeof(double));
		arg->value[0] = 1;
	}
	else {
		arg->noband = 0;
		arg->numbands = num_bands;

		arg->pixtype = pixtype;
		arg->init = init;
		arg->nodata = nodata;
		arg->hasnodata = hasnodata;
		arg->value = value;
	}

	/* OGR spatial reference */
	if (NULL != srs && strlen(srs)) {
		src_sr = OSRNewSpatialReference(NULL);
		if (OSRSetFromUserInput(src_sr, srs) != OGRERR_NONE) {
			rterror("rt_raster_gdal_rasterize: Could not create OSR spatial reference using the provided srs: %s", srs);
			_rti_rasterize_arg_destroy(arg);
			return NULL;
		}
	}

	/* convert WKB to OGR Geometry */
	ogrerr = OGR_G_CreateFromWkb((unsigned char *) wkb, src_sr, &src_geom, wkb_len);
	if (ogrerr != OGRERR_NONE) {
		rterror("rt_raster_gdal_rasterize: Could not create OGR Geometry from WKB");

		_rti_rasterize_arg_destroy(arg);

		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}

	/* OGR Geometry is empty */
	if (OGR_G_IsEmpty(src_geom)) {
		rtinfo("Geometry provided is empty. Returning empty raster");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return rt_raster_new(0, 0);
	}

	/* get envelope */
	OGR_G_GetEnvelope(src_geom, &src_env);
	rt_util_from_ogr_envelope(src_env, &extent);

	RASTER_DEBUGF(3, "Suggested raster envelope: %f, %f, %f, %f",
		extent.MinX, extent.MinY, extent.MaxX, extent.MaxY);

	/* user-defined scale */
	if (
		(NULL != scale_x) &&
		(NULL != scale_y) &&
		(FLT_NEQ(*scale_x, 0.0)) &&
		(FLT_NEQ(*scale_y, 0.0))
	) {
		/* for now, force scale to be in left-right, top-down orientation */
		_scale[0] = fabs(*scale_x);
		_scale[1] = fabs(*scale_y);
	}
	/* user-defined width/height */
	else if (
		(NULL != width) &&
		(NULL != height) &&
		(FLT_NEQ(*width, 0.0)) &&
		(FLT_NEQ(*height, 0.0))
	) {
		_dim[0] = fabs(*width);
		_dim[1] = fabs(*height);

		if (FLT_NEQ(extent.MaxX, extent.MinX))
			_scale[0] = fabs((extent.MaxX - extent.MinX) / _dim[0]);
		else
			_scale[0] = 1.;

		if (FLT_NEQ(extent.MaxY, extent.MinY))
			_scale[1] = fabs((extent.MaxY - extent.MinY) / _dim[1]);
		else
			_scale[1] = 1.;
	}
	else {
		rterror("rt_raster_gdal_rasterize: Values must be provided for width and height or X and Y of scale");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}
	RASTER_DEBUGF(3, "scale (x, y) = %f, %f", _scale[0], -1 * _scale[1]);
	RASTER_DEBUGF(3, "dim (x, y) = %d, %d", _dim[0], _dim[1]);

	/* user-defined skew */
	if (NULL != skew_x) {
		_skew[0] = *skew_x;

		/*
			negative scale-x affects skew
			for now, force skew to be in left-right, top-down orientation
		*/
		if (
			NULL != scale_x &&
			*scale_x < 0.
		) {
			_skew[0] *= -1;
		}
	}
	if (NULL != skew_y) {
		_skew[1] = *skew_y;

		/*
			positive scale-y affects skew
			for now, force skew to be in left-right, top-down orientation
		*/
		if (
			NULL != scale_y &&
			*scale_y > 0.
		) {
			_skew[1] *= -1;
		}
	}

	/*
	 	if geometry is a point, a linestring or set of either and bounds not set,
		increase extent by a pixel to avoid missing points on border

		a whole pixel is used instead of half-pixel due to backward
		compatibility with GDAL 1.6, 1.7 and 1.8.  1.9+ works fine with half-pixel.
	*/
	wkbtype = wkbFlatten(OGR_G_GetGeometryType(src_geom));
	if ((
			(wkbtype == wkbPoint) ||
			(wkbtype == wkbMultiPoint) ||
			(wkbtype == wkbLineString) ||
			(wkbtype == wkbMultiLineString)
		) &&
		FLT_EQ(_dim[0], 0) &&
		FLT_EQ(_dim[1], 0)
	) {
		int result;
		LWPOLY *epoly = NULL;
		LWGEOM *lwgeom = NULL;
		GEOSGeometry *egeom = NULL;
		GEOSGeometry *geom = NULL;

		RASTER_DEBUG(3, "Testing geometry is properly contained by extent");

		/*
			see if geometry is properly contained by extent
			all parts of geometry lies within extent
		*/

		/* initialize GEOS */
		initGEOS(lwnotice, lwgeom_geos_error);

		/* convert envelope to geometry */
		RASTER_DEBUG(4, "Converting envelope to geometry");
		epoly = rt_util_envelope_to_lwpoly(extent);
		if (epoly == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not create envelope's geometry to test if geometry is properly contained by extent");

			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			return NULL;
		}

		egeom = (GEOSGeometry *) LWGEOM2GEOS(lwpoly_as_lwgeom(epoly));
		lwpoly_free(epoly);

		/* convert WKB to geometry */
		RASTER_DEBUG(4, "Converting WKB to geometry");
		lwgeom = lwgeom_from_wkb(wkb, wkb_len, LW_PARSER_CHECK_NONE);
		geom = (GEOSGeometry *) LWGEOM2GEOS(lwgeom);
		lwgeom_free(lwgeom);

		result = GEOSRelatePattern(egeom, geom, "T**FF*FF*");
		GEOSGeom_destroy(geom);
		GEOSGeom_destroy(egeom);

		if (result == 2) {
			rterror("rt_raster_gdal_rasterize: Could not test if geometry is properly contained by extent for geometry within extent");

			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			return NULL;
		}

		/* geometry NOT properly contained by extent */
		if (!result) {

#if POSTGIS_GDAL_VERSION > 18

			/* check alignment flag: grid_xw */
			if (
				(NULL == ul_xw && NULL == ul_yw) &&
				(NULL != grid_xw && NULL != grid_xw) &&
				FLT_NEQ(*grid_xw, extent.MinX)
			) {
				// do nothing
				RASTER_DEBUG(3, "Skipping extent adjustment on X-axis due to upcoming alignment");
			}
			else {
				RASTER_DEBUG(3, "Adjusting extent for GDAL > 1.8 by half the scale on X-axis");
				extent.MinX -= (_scale[0] / 2.);
				extent.MaxX += (_scale[0] / 2.);
			}

			/* check alignment flag: grid_yw */
			if (
				(NULL == ul_xw && NULL == ul_yw) &&
				(NULL != grid_xw && NULL != grid_xw) &&
				FLT_NEQ(*grid_yw, extent.MaxY)
			) {
				// do nothing
				RASTER_DEBUG(3, "Skipping extent adjustment on Y-axis due to upcoming alignment");
			}
			else {
				RASTER_DEBUG(3, "Adjusting extent for GDAL > 1.8 by half the scale on Y-axis");
				extent.MinY -= (_scale[1] / 2.);
				extent.MaxY += (_scale[1] / 2.);
			}

#else

			/* check alignment flag: grid_xw */
			if (
				(NULL == ul_xw && NULL == ul_yw) &&
				(NULL != grid_xw && NULL != grid_xw) &&
				FLT_NEQ(*grid_xw, extent.MinX)
			) {
				// do nothing
				RASTER_DEBUG(3, "Skipping extent adjustment on X-axis due to upcoming alignment");
			}
			else {
				RASTER_DEBUG(3, "Adjusting extent for GDAL <= 1.8 by the scale on X-axis");
				extent.MinX -= _scale[0];
				extent.MaxX += _scale[0];
			}


			/* check alignment flag: grid_yw */
			if (
				(NULL == ul_xw && NULL == ul_yw) &&
				(NULL != grid_xw && NULL != grid_xw) &&
				FLT_NEQ(*grid_yw, extent.MaxY)
			) {
				// do nothing
				RASTER_DEBUG(3, "Skipping extent adjustment on Y-axis due to upcoming alignment");
			}
			else {
				RASTER_DEBUG(3, "Adjusting extent for GDAL <= 1.8 by the scale on Y-axis");
				extent.MinY -= _scale[1];
				extent.MaxY += _scale[1];
			}

#endif

		}

		RASTER_DEBUGF(3, "Adjusted extent: %f, %f, %f, %f",
			extent.MinX, extent.MinY, extent.MaxX, extent.MaxY);

		extent.UpperLeftX = extent.MinX;
		extent.UpperLeftY = extent.MaxY;
	}

	/* reprocess extent if skewed */
	if (
		FLT_NEQ(_skew[0], 0) ||
		FLT_NEQ(_skew[1], 0)
	) {
		rt_raster skewedrast;

		RASTER_DEBUG(3, "Computing skewed extent's envelope");

		skewedrast = rt_raster_compute_skewed_raster(
			extent,
			_skew,
			_scale,
			0.01
		);
		if (skewedrast == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not compute skewed raster");

			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			return NULL;
		}

		_dim[0] = skewedrast->width;
		_dim[1] = skewedrast->height;

		extent.UpperLeftX = skewedrast->ipX;
		extent.UpperLeftY = skewedrast->ipY;

		rt_raster_destroy(skewedrast);
	}

	/* raster dimensions */
	if (!_dim[0])
		_dim[0] = (int) fmax((fabs(extent.MaxX - extent.MinX) + (_scale[0] / 2.)) / _scale[0], 1);
	if (!_dim[1])
		_dim[1] = (int) fmax((fabs(extent.MaxY - extent.MinY) + (_scale[1] / 2.)) / _scale[1], 1);

	/* temporary raster */
	rast = rt_raster_new(_dim[0], _dim[1]);
	if (rast == NULL) {
		rterror("rt_raster_gdal_rasterize: Out of memory allocating temporary raster");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}

	/* set raster's spatial attributes */
	rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);
	rt_raster_set_scale(rast, _scale[0], -1 * _scale[1]);
	rt_raster_set_skews(rast, _skew[0], _skew[1]);

	rt_raster_get_geotransform_matrix(rast, _gt);
	RASTER_DEBUGF(3, "Temp raster's geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]);
	RASTER_DEBUGF(3, "Temp raster's dimensions (width x height): %d x %d",
		_dim[0], _dim[1]);

	/* user-specified upper-left corner */
	if (
		NULL != ul_xw &&
		NULL != ul_yw
	) {
		ul_user = 1;

		RASTER_DEBUGF(4, "Using user-specified upper-left corner: %f, %f", *ul_xw, *ul_yw);

		/* set upper-left corner */
		rt_raster_set_offsets(rast, *ul_xw, *ul_yw);
		extent.UpperLeftX = *ul_xw;
		extent.UpperLeftY = *ul_yw;
	}
	else if (
		((NULL != ul_xw) && (NULL == ul_yw)) ||
		((NULL == ul_xw) && (NULL != ul_yw))
	) {
		rterror("rt_raster_gdal_rasterize: Both X and Y upper-left corner values must be provided");

		rt_raster_destroy(rast);
		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}

	/* alignment only considered if upper-left corner not provided */
	if (
		!ul_user && (
			(NULL != grid_xw) || (NULL != grid_yw)
		)
	) {

		if (
			((NULL != grid_xw) && (NULL == grid_yw)) ||
			((NULL == grid_xw) && (NULL != grid_yw))
		) {
			rterror("rt_raster_gdal_rasterize: Both X and Y alignment values must be provided");

			rt_raster_destroy(rast);
			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			return NULL;
		}

		RASTER_DEBUGF(4, "Aligning extent to user-specified grid: %f, %f", *grid_xw, *grid_yw);

		do {
			double _r[2] = {0};
			double _w[2] = {0};

			/* raster is already aligned */
			if (FLT_EQ(*grid_xw, extent.UpperLeftX) && FLT_EQ(*grid_yw, extent.UpperLeftY)) {
				RASTER_DEBUG(3, "Skipping raster alignment as it is already aligned to grid");
				break;
			}

			extent.UpperLeftX = rast->ipX;
			extent.UpperLeftY = rast->ipY;
			rt_raster_set_offsets(rast, *grid_xw, *grid_yw);

			/* process upper-left corner */
			if (rt_raster_geopoint_to_cell(
				rast,
				extent.UpperLeftX, extent.UpperLeftY,
				&(_r[0]), &(_r[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_rasterize: Could not compute raster pixel for spatial coordinates");

				rt_raster_destroy(rast);
				_rti_rasterize_arg_destroy(arg);

				OGR_G_DestroyGeometry(src_geom);
				if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
				/* OGRCleanupAll(); */

				return NULL;
			}

			if (rt_raster_cell_to_geopoint(
				rast,
				_r[0], _r[1],
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_rasterize: Could not compute spatial coordinates for raster pixel");

				rt_raster_destroy(rast);
				_rti_rasterize_arg_destroy(arg);

				OGR_G_DestroyGeometry(src_geom);
				if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
				/* OGRCleanupAll(); */

				return NULL;
			}

			/* shift occurred */
			if (FLT_NEQ(_w[0], extent.UpperLeftX)) {
				if (NULL == width)
					rast->width++;
				else if (NULL == scale_x) {
					double _c[2] = {0};

					rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);

					/* get upper-right corner */
					if (rt_raster_cell_to_geopoint(
						rast,
						rast->width, 0,
						&(_c[0]), &(_c[1]),
						NULL
					) != ES_NONE) {
						rterror("rt_raster_gdal_rasterize: Could not compute spatial coordinates for raster pixel");

						rt_raster_destroy(rast);
						_rti_rasterize_arg_destroy(arg);

						OGR_G_DestroyGeometry(src_geom);
						if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
						/* OGRCleanupAll(); */

						return NULL;
					}

					rast->scaleX = fabs((_c[0] - _w[0]) / ((double) rast->width));
				}
			}
			if (FLT_NEQ(_w[1], extent.UpperLeftY)) {
				if (NULL == height)
					rast->height++;
				else if (NULL == scale_y) {
					double _c[2] = {0};

					rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);

					/* get upper-right corner */
					if (rt_raster_cell_to_geopoint(
						rast,
						0, rast->height,
						&(_c[0]), &(_c[1]),
						NULL
					) != ES_NONE) {
						rterror("rt_raster_gdal_rasterize: Could not compute spatial coordinates for raster pixel");

						rt_raster_destroy(rast);
						_rti_rasterize_arg_destroy(arg);

						OGR_G_DestroyGeometry(src_geom);
						if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
						/* OGRCleanupAll(); */

						return NULL;
					}

					rast->scaleY = -1 * fabs((_c[1] - _w[1]) / ((double) rast->height));
				}
			}

			rt_raster_set_offsets(rast, _w[0], _w[1]);
		}
		while (0);
	}

	/*
		after this point, rt_envelope extent is no longer used
	*/

	/* get key attributes from rast */
	_dim[0] = rast->width;
	_dim[1] = rast->height;
	rt_raster_get_geotransform_matrix(rast, _gt);

	/* scale-x is negative or scale-y is positive */
	if ((
		(NULL != scale_x) && (*scale_x < 0.)
	) || (
		(NULL != scale_y) && (*scale_y > 0)
	)) {
		double _w[2] = {0};

		/* negative scale-x */
		if (
			(NULL != scale_x) &&
			(*scale_x < 0.)
		) {
			RASTER_DEBUG(3, "Processing negative scale-x");

			if (rt_raster_cell_to_geopoint(
				rast,
				_dim[0], 0,
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_rasterize: Could not compute spatial coordinates for raster pixel");

				rt_raster_destroy(rast);
				_rti_rasterize_arg_destroy(arg);

				OGR_G_DestroyGeometry(src_geom);
				if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
				/* OGRCleanupAll(); */

				return NULL;
			}

			_gt[0] = _w[0];
			_gt[1] = *scale_x;

			/* check for skew */
			if (NULL != skew_x && FLT_NEQ(*skew_x, 0))
				_gt[2] = *skew_x;
		}
		/* positive scale-y */
		if (
			(NULL != scale_y) &&
			(*scale_y > 0)
		) {
			RASTER_DEBUG(3, "Processing positive scale-y");

			if (rt_raster_cell_to_geopoint(
				rast,
				0, _dim[1],
				&(_w[0]), &(_w[1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_gdal_rasterize: Could not compute spatial coordinates for raster pixel");

				rt_raster_destroy(rast);
				_rti_rasterize_arg_destroy(arg);

				OGR_G_DestroyGeometry(src_geom);
				if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
				/* OGRCleanupAll(); */

				return NULL;
			}

			_gt[3] = _w[1];
			_gt[5] = *scale_y;

			/* check for skew */
			if (NULL != skew_y && FLT_NEQ(*skew_y, 0))
				_gt[4] = *skew_y;
		}
	}

	rt_raster_destroy(rast);
	rast = NULL;

	RASTER_DEBUGF(3, "Applied geotransform: %f, %f, %f, %f, %f, %f",
		_gt[0], _gt[1], _gt[2], _gt[3], _gt[4], _gt[5]);
	RASTER_DEBUGF(3, "Raster dimensions (width x height): %d x %d",
		_dim[0], _dim[1]);

	/* load GDAL mem */
	if (!rt_util_gdal_driver_registered("MEM"))
		GDALRegister_MEM();
	_drv = GDALGetDriverByName("MEM");
	if (NULL == _drv) {
		rterror("rt_raster_gdal_rasterize: Could not load the MEM GDAL driver");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}

	_ds = GDALCreate(_drv, "", _dim[0], _dim[1], 0, GDT_Byte, NULL);
	if (NULL == _ds) {
		rterror("rt_raster_gdal_rasterize: Could not create a GDALDataset to rasterize the geometry into");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		return NULL;
	}

	/* set geotransform */
	cplerr = GDALSetGeoTransform(_ds, _gt);
	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_rasterize: Could not set geotransform on GDALDataset");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		GDALClose(_ds);

		return NULL;
	}

	/* set SRS */
	if (NULL != src_sr) {
		char *_srs = NULL;
		OSRExportToWkt(src_sr, &_srs);

		cplerr = GDALSetProjection(_ds, _srs);
		CPLFree(_srs);
		if (cplerr != CE_None) {
			rterror("rt_raster_gdal_rasterize: Could not set projection on GDALDataset");

			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			GDALClose(_ds);

			return NULL;
		}
	}

	/* set bands */
	for (i = 0; i < arg->numbands; i++) {
		err = 0;

		do {
			/* add band */
			cplerr = GDALAddBand(_ds, rt_util_pixtype_to_gdal_datatype(arg->pixtype[i]), NULL);
			if (cplerr != CE_None) {
				rterror("rt_raster_gdal_rasterize: Could not add band to GDALDataset");
				err = 1;
				break;
			}

			_band = GDALGetRasterBand(_ds, i + 1);
			if (NULL == _band) {
				rterror("rt_raster_gdal_rasterize: Could not get band %d from GDALDataset", i + 1);
				err = 1;
				break;
			}

			/* nodata value */
			if (arg->hasnodata[i]) {
				RASTER_DEBUGF(4, "Setting NODATA value of band %d to %f", i, arg->nodata[i]);
				cplerr = GDALSetRasterNoDataValue(_band, arg->nodata[i]);
				if (cplerr != CE_None) {
					rterror("rt_raster_gdal_rasterize: Could not set nodata value");
					err = 1;
					break;
				}
				RASTER_DEBUGF(4, "NODATA value set to %f", GDALGetRasterNoDataValue(_band, NULL));
			}

			/* initial value */
			cplerr = GDALFillRaster(_band, arg->init[i], 0);
			if (cplerr != CE_None) {
				rterror("rt_raster_gdal_rasterize: Could not set initial value");
				err = 1;
				break;
			}
		}
		while (0);

		if (err) {
			_rti_rasterize_arg_destroy(arg);

			OGR_G_DestroyGeometry(src_geom);
			if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
			/* OGRCleanupAll(); */

			GDALClose(_ds);

			return NULL;
		}
	}

	arg->bandlist = (int *) rtalloc(sizeof(int) * arg->numbands);
	for (i = 0; i < arg->numbands; i++) arg->bandlist[i] = i + 1;


	/* burn geometry */
	cplerr = GDALRasterizeGeometries(
		_ds,
		arg->numbands, arg->bandlist,
		1, &src_geom,
		NULL, NULL,
		arg->value,
		options,
		NULL, NULL
	);
	if (cplerr != CE_None) {
		rterror("rt_raster_gdal_rasterize: Could not rasterize geometry");

		_rti_rasterize_arg_destroy(arg);

		OGR_G_DestroyGeometry(src_geom);
		if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
		/* OGRCleanupAll(); */

		GDALClose(_ds);

		return NULL;
	}

	/* convert gdal dataset to raster */
	GDALFlushCache(_ds);
	RASTER_DEBUG(3, "Converting GDAL dataset to raster");
	rast = rt_raster_from_gdal_dataset(_ds);

	OGR_G_DestroyGeometry(src_geom);
	if (src_sr != NULL) OSRDestroySpatialReference(src_sr);
	/* OGRCleanupAll(); */

	GDALClose(_ds);

	if (NULL == rast) {
		rterror("rt_raster_gdal_rasterize: Could not rasterize geometry");
		return NULL;
	}

	/* width, height */
	_width = rt_raster_get_width(rast);
	_height = rt_raster_get_height(rast);

	/* check each band for pixtype */
	for (i = 0; i < arg->numbands; i++) {
		uint8_t *data = NULL;
		rt_band band = NULL;
		rt_band oldband = NULL;

		double val = 0;
		int nodata = 0;
		int hasnodata = 0;
		double nodataval = 0;
		int x = 0;
		int y = 0;

		oldband = rt_raster_get_band(rast, i);
		if (oldband == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not get band %d of output raster", i);
			_rti_rasterize_arg_destroy(arg);
			rt_raster_destroy(rast);
			return NULL;
		}

		/* band is of user-specified type */
		if (rt_band_get_pixtype(oldband) == arg->pixtype[i])
			continue;

		/* hasnodata, nodataval */
		hasnodata = rt_band_get_hasnodata_flag(oldband);
		if (hasnodata)
			rt_band_get_nodata(oldband, &nodataval);

		/* allocate data */
		data = rtalloc(rt_pixtype_size(arg->pixtype[i]) * _width * _height);
		if (data == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not allocate memory for band data");
			_rti_rasterize_arg_destroy(arg);
			rt_raster_destroy(rast);
			return NULL;
		}
		memset(data, 0, rt_pixtype_size(arg->pixtype[i]) * _width * _height);

		/* create new band of correct type */
		band = rt_band_new_inline(
			_width, _height,
			arg->pixtype[i],
			hasnodata, nodataval,
			data
		);
		if (band == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not create band");
			rtdealloc(data);
			_rti_rasterize_arg_destroy(arg);
			rt_raster_destroy(rast);
			return NULL;
		}

		/* give ownership of data to band */
		rt_band_set_ownsdata_flag(band, 1);

		/* copy pixel by pixel */
		for (x = 0; x < _width; x++) {
			for (y = 0; y < _height; y++) {
				err = rt_band_get_pixel(oldband, x, y, &val, &nodata);
				if (err != ES_NONE) {
					rterror("rt_raster_gdal_rasterize: Could not get pixel value");
					_rti_rasterize_arg_destroy(arg);
					rt_raster_destroy(rast);
					rt_band_destroy(band);
					return NULL;
				}

				if (nodata)
					val = nodataval;

				err = rt_band_set_pixel(band, x, y, val, NULL);
				if (err != ES_NONE) {
					rterror("rt_raster_gdal_rasterize: Could not set pixel value");
					_rti_rasterize_arg_destroy(arg);
					rt_raster_destroy(rast);
					rt_band_destroy(band);
					return NULL;
				}
			}
		}

		/* replace band */
		oldband = rt_raster_replace_band(rast, band, i);
		if (oldband == NULL) {
			rterror("rt_raster_gdal_rasterize: Could not replace band %d of output raster", i);
			_rti_rasterize_arg_destroy(arg);
			rt_raster_destroy(rast);
			rt_band_destroy(band);
			return NULL;
		}

		/* free oldband */
		rt_band_destroy(oldband);
	}

	_rti_rasterize_arg_destroy(arg);

	RASTER_DEBUG(3, "done");

	return rast;
}

/******************************************************************************
* rt_raster_intersects()
******************************************************************************/

static
int rt_raster_intersects_algorithm(
	rt_raster rast1, rt_raster rast2,
	rt_band band1, rt_band band2,
	int hasnodata1, int hasnodata2,
	double nodata1, double nodata2
) {
	int i;
	int byHeight = 1;
	uint32_t dimValue;

	uint32_t row;
	uint32_t rowoffset;
	uint32_t col;
	uint32_t coloffset;

	enum line_points {X1, Y1, X2, Y2};
	enum point {pX, pY};
	double line1[4] = {0.};
	double line2[4] = {0.};
	double P[2] = {0.};
	double Qw[2] = {0.};
	double Qr[2] = {0.};
	double gt1[6] = {0.};
	double gt2[6] = {0.};
	double igt1[6] = {0};
	double igt2[6] = {0};
	double d;
	double val1;
	int noval1;
	int isnodata1;
	double val2;
	int noval2;
	int isnodata2;
	uint32_t adjacent[8] = {0};

	double xscale;
	double yscale;

	uint16_t width1;
	uint16_t height1;
	uint16_t width2;
	uint16_t height2;

	width1 = rt_raster_get_width(rast1);
	height1 = rt_raster_get_height(rast1);
	width2 = rt_raster_get_width(rast2);
	height2 = rt_raster_get_height(rast2);

	/* sampling scale */
	xscale = fmin(rt_raster_get_x_scale(rast1), rt_raster_get_x_scale(rast2)) / 10.;
	yscale = fmin(rt_raster_get_y_scale(rast1), rt_raster_get_y_scale(rast2)) / 10.;

	/* see if skew made rast2's rows are parallel to rast1's cols */
	rt_raster_cell_to_geopoint(
		rast1,
		0, 0,
		&(line1[X1]), &(line1[Y1]),
		gt1
	);

	rt_raster_cell_to_geopoint(
		rast1,
		0, height1,
		&(line1[X2]), &(line1[Y2]),
		gt1
	);

	rt_raster_cell_to_geopoint(
		rast2,
		0, 0,
		&(line2[X1]), &(line2[Y1]),
		gt2
	);

	rt_raster_cell_to_geopoint(
		rast2,
		width2, 0,
		&(line2[X2]), &(line2[Y2]),
		gt2
	);

	/* parallel vertically */
	if (FLT_EQ(line1[X2] - line1[X1], 0.) && FLT_EQ(line2[X2] - line2[X1], 0.))
		byHeight = 0;
	/* parallel */
	else if (FLT_EQ(((line1[Y2] - line1[Y1]) / (line1[X2] - line1[X1])), ((line2[Y2] - line2[Y1]) / (line2[X2] - line2[X1]))))
		byHeight = 0;

	if (byHeight)
		dimValue = height2;
	else
		dimValue = width2;
	RASTER_DEBUGF(4, "byHeight: %d, dimValue: %d", byHeight, dimValue);

	/* 3 x 3 search */
	for (coloffset = 0; coloffset < 3; coloffset++) {
		for (rowoffset = 0; rowoffset < 3; rowoffset++) {
			/* smaller raster */
			for (col = coloffset; col <= width1; col += 3) {

				rt_raster_cell_to_geopoint(
					rast1,
					col, 0,
					&(line1[X1]), &(line1[Y1]),
					gt1
				);

				rt_raster_cell_to_geopoint(
					rast1,
					col, height1,
					&(line1[X2]), &(line1[Y2]),
					gt1
				);

				/* larger raster */
				for (row = rowoffset; row <= dimValue; row += 3) {

					if (byHeight) {
						rt_raster_cell_to_geopoint(
							rast2,
							0, row,
							&(line2[X1]), &(line2[Y1]),
							gt2
						);

						rt_raster_cell_to_geopoint(
							rast2,
							width2, row,
							&(line2[X2]), &(line2[Y2]),
							gt2
						);
					}
					else {
						rt_raster_cell_to_geopoint(
							rast2,
							row, 0,
							&(line2[X1]), &(line2[Y1]),
							gt2
						);

						rt_raster_cell_to_geopoint(
							rast2,
							row, height2,
							&(line2[X2]), &(line2[Y2]),
							gt2
						);
					}

					RASTER_DEBUGF(4, "(col, row) = (%d, %d)", col, row);
					RASTER_DEBUGF(4, "line1(x1, y1, x2, y2) = (%f, %f, %f, %f)",
						line1[X1], line1[Y1], line1[X2], line1[Y2]);
					RASTER_DEBUGF(4, "line2(x1, y1, x2, y2) = (%f, %f, %f, %f)",
						line2[X1], line2[Y1], line2[X2], line2[Y2]);

					/* intersection */
					/* http://en.wikipedia.org/wiki/Line-line_intersection */
					d = ((line1[X1] - line1[X2]) * (line2[Y1] - line2[Y2])) - ((line1[Y1] - line1[Y2]) * (line2[X1] - line2[X2]));
					/* no intersection */
					if (FLT_EQ(d, 0.)) {
						continue;
					}

					P[pX] = (((line1[X1] * line1[Y2]) - (line1[Y1] * line1[X2])) * (line2[X1] - line2[X2])) -
						((line1[X1] - line1[X2]) * ((line2[X1] * line2[Y2]) - (line2[Y1] * line2[X2])));
					P[pX] = P[pX] / d;

					P[pY] = (((line1[X1] * line1[Y2]) - (line1[Y1] * line1[X2])) * (line2[Y1] - line2[Y2])) -
						((line1[Y1] - line1[Y2]) * ((line2[X1] * line2[Y2]) - (line2[Y1] * line2[X2])));
					P[pY] = P[pY] / d;

					RASTER_DEBUGF(4, "P(x, y) = (%f, %f)", P[pX], P[pY]);

					/* intersection within bounds */
					if ((
							(FLT_EQ(P[pX], line1[X1]) || FLT_EQ(P[pX], line1[X2])) ||
								(P[pX] > fmin(line1[X1], line1[X2]) && (P[pX] < fmax(line1[X1], line1[X2])))
						) && (
							(FLT_EQ(P[pY], line1[Y1]) || FLT_EQ(P[pY], line1[Y2])) ||
								(P[pY] > fmin(line1[Y1], line1[Y2]) && (P[pY] < fmax(line1[Y1], line1[Y2])))
						) && (
							(FLT_EQ(P[pX], line2[X1]) || FLT_EQ(P[pX], line2[X2])) ||
								(P[pX] > fmin(line2[X1], line2[X2]) && (P[pX] < fmax(line2[X1], line2[X2])))
						) && (
							(FLT_EQ(P[pY], line2[Y1]) || FLT_EQ(P[pY], line2[Y2])) ||
								(P[pY] > fmin(line2[Y1], line2[Y2]) && (P[pY] < fmax(line2[Y1], line2[Y2])))
					)) {
						RASTER_DEBUG(4, "within bounds");

						for (i = 0; i < 8; i++) adjacent[i] = 0;

						/* test points around intersection */
						for (i = 0; i < 8; i++) {
							switch (i) {
								case 7:
									Qw[pX] = P[pX] - xscale;
									Qw[pY] = P[pY] + yscale;
									break;
								/* 270 degrees = 09:00 */
								case 6:
									Qw[pX] = P[pX] - xscale;
									Qw[pY] = P[pY];
									break;
								case 5:
									Qw[pX] = P[pX] - xscale;
									Qw[pY] = P[pY] - yscale;
									break;
								/* 180 degrees = 06:00 */
								case 4:
									Qw[pX] = P[pX];
									Qw[pY] = P[pY] - yscale;
									break;
								case 3:
									Qw[pX] = P[pX] + xscale;
									Qw[pY] = P[pY] - yscale;
									break;
								/* 90 degrees = 03:00 */
								case 2:
									Qw[pX] = P[pX] + xscale;
									Qw[pY] = P[pY];
									break;
								/* 45 degrees */
								case 1:
									Qw[pX] = P[pX] + xscale;
									Qw[pY] = P[pY] + yscale;
									break;
								/* 0 degrees = 00:00 */
								case 0:
									Qw[pX] = P[pX];
									Qw[pY] = P[pY] + yscale;
									break;
							}

							/* unable to convert point to cell */
							noval1 = 0;
							if (rt_raster_geopoint_to_cell(
								rast1,
								Qw[pX], Qw[pY],
								&(Qr[pX]), &(Qr[pY]),
								igt1
							) != ES_NONE) {
								noval1 = 1;
							}
							/* cell is outside bounds of grid */
							else if (
								(Qr[pX] < 0 || Qr[pX] > width1 || FLT_EQ(Qr[pX], width1)) ||
								(Qr[pY] < 0 || Qr[pY] > height1 || FLT_EQ(Qr[pY], height1))
							) {
								noval1 = 1;
							}
							else if (hasnodata1 == FALSE)
								val1 = 1;
							/* unable to get value at cell */
							else if (rt_band_get_pixel(band1, Qr[pX], Qr[pY], &val1, &isnodata1) != ES_NONE)
								noval1 = 1;

							/* unable to convert point to cell */
							noval2 = 0;
							if (rt_raster_geopoint_to_cell(
								rast2,
								Qw[pX], Qw[pY],
								&(Qr[pX]), &(Qr[pY]),
								igt2
							) != ES_NONE) {
								noval2 = 1;
							}
							/* cell is outside bounds of grid */
							else if (
								(Qr[pX] < 0 || Qr[pX] > width2 || FLT_EQ(Qr[pX], width2)) ||
								(Qr[pY] < 0 || Qr[pY] > height2 || FLT_EQ(Qr[pY], height2))
							) {
								noval2 = 1;
							}
							else if (hasnodata2 == FALSE)
								val2 = 1;
							/* unable to get value at cell */
							else if (rt_band_get_pixel(band2, Qr[pX], Qr[pY], &val2, &isnodata2) != ES_NONE)
								noval2 = 1;

							if (!noval1) {
								RASTER_DEBUGF(4, "val1 = %f", val1);
							}
							if (!noval2) {
								RASTER_DEBUGF(4, "val2 = %f", val2);
							}

							/* pixels touch */
							if (!noval1 && (
								(hasnodata1 == FALSE) || !isnodata1
							)) {
								adjacent[i]++;
							}
							if (!noval2 && (
								(hasnodata2 == FALSE) || !isnodata2
							)) {
								adjacent[i] += 3;
							}

							/* two pixel values not present */
							if (noval1 || noval2) {
								RASTER_DEBUGF(4, "noval1 = %d, noval2 = %d", noval1, noval2);
								continue;
							}

							/* pixels valid, so intersect */
							if (
								((hasnodata1 == FALSE) || !isnodata1) &&
								((hasnodata2 == FALSE) || !isnodata2)
							) {
								RASTER_DEBUG(3, "The two rasters do intersect");

								return 1;
							}
						}

						/* pixels touch */
						for (i = 0; i < 4; i++) {
							RASTER_DEBUGF(4, "adjacent[%d] = %d, adjacent[%d] = %d"
								, i, adjacent[i], i + 4, adjacent[i + 4]);
							if (adjacent[i] == 0) continue;

							if (adjacent[i] + adjacent[i + 4] == 4) {
								RASTER_DEBUG(3, "The two rasters touch");

								return 1;
							}
						}
					}
					else {
						RASTER_DEBUG(4, "outside of bounds");
					}
				}
			}
		}
	}

	return 0;
}

/**
 * Return zero if error occurred in function.
 * Parameter intersects returns non-zero if two rasters intersect
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param intersects : non-zero value if the two rasters' bands intersects
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_raster_intersects(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *intersects
) {
	int i;
	int j;
	int within = 0;

	LWGEOM *hull[2] = {NULL};
	GEOSGeometry *ghull[2] = {NULL};

	uint16_t width1;
	uint16_t height1;
	uint16_t width2;
	uint16_t height2;
	double area1;
	double area2;
	double pixarea1;
	double pixarea2;
	rt_raster rastS = NULL;
	rt_raster rastL = NULL;
	uint16_t *widthS = NULL;
	uint16_t *heightS = NULL;
	uint16_t *widthL = NULL;
	uint16_t *heightL = NULL;
	int nbandS;
	int nbandL;
	rt_band bandS = NULL;
	rt_band bandL = NULL;
	int hasnodataS = FALSE;
	int hasnodataL = FALSE;
	double nodataS = 0;
	double nodataL = 0;
	int isnodataS = 0;
	int isnodataL = 0;
	double gtS[6] = {0};
	double igtL[6] = {0};

	uint32_t row;
	uint32_t rowoffset;
	uint32_t col;
	uint32_t coloffset;

	enum line_points {X1, Y1, X2, Y2};
	enum point {pX, pY};
	double lineS[4];
	double Qr[2];
	double valS;
	double valL;

	RASTER_DEBUG(3, "Starting");

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != intersects);

	if (nband1 < 0 && nband2 < 0) {
		nband1 = -1;
		nband2 = -1;
	}
	else {
		assert(nband1 >= 0 && nband1 < rt_raster_get_num_bands(rast1));
		assert(nband2 >= 0 && nband2 < rt_raster_get_num_bands(rast2));
	}

	/* same srid */
	if (rt_raster_get_srid(rast1) != rt_raster_get_srid(rast2)) {
		rterror("rt_raster_intersects: The two rasters provided have different SRIDs");
		*intersects = 0;
		return ES_ERROR;
	}

	/* raster extents need to intersect */
	do {
		int rtn;

		initGEOS(lwnotice, lwgeom_geos_error);

		rtn = 1;
		for (i = 0; i < 2; i++) {
			if ((rt_raster_get_convex_hull(i < 1 ? rast1 : rast2, &(hull[i])) != ES_NONE) || NULL == hull[i]) {
				for (j = 0; j < i; j++) {
					GEOSGeom_destroy(ghull[j]);
					lwgeom_free(hull[j]);
				}
				rtn = 0;
				break;
			}
			ghull[i] = (GEOSGeometry *) LWGEOM2GEOS(hull[i]);
			if (NULL == ghull[i]) {
				for (j = 0; j < i; j++) {
					GEOSGeom_destroy(ghull[j]);
					lwgeom_free(hull[j]);
				}
				lwgeom_free(hull[i]);
				rtn = 0;
				break;
			}
		}
		if (!rtn) break;

		/* test to see if raster within the other */
		within = 0;
		if (GEOSWithin(ghull[0], ghull[1]) == 1)
			within = -1;
		else if (GEOSWithin(ghull[1], ghull[0]) == 1)
			within = 1;

		if (within != 0)
			rtn = 1;
		else
			rtn = GEOSIntersects(ghull[0], ghull[1]);

		for (i = 0; i < 2; i++) {
			GEOSGeom_destroy(ghull[i]);
			lwgeom_free(hull[i]);
		}

		if (rtn != 2) {
			RASTER_DEBUGF(4, "convex hulls of rasters do %sintersect", rtn != 1 ? "NOT " : "");
			if (rtn != 1) {
				*intersects = 0;
				return ES_NONE;
			}
			/* band isn't specified */
			else if (nband1 < 0) {
				*intersects = 1;
				return ES_NONE;
			}
		}
		else {
			RASTER_DEBUG(4, "GEOSIntersects() returned a 2!!!!");
		}
	}
	while (0);

	/* smaller raster by area or width */
	width1 = rt_raster_get_width(rast1);
	height1 = rt_raster_get_height(rast1);
	width2 = rt_raster_get_width(rast2);
	height2 = rt_raster_get_height(rast2);
	pixarea1 = fabs(rt_raster_get_x_scale(rast1) * rt_raster_get_y_scale(rast1));
	pixarea2 = fabs(rt_raster_get_x_scale(rast2) * rt_raster_get_y_scale(rast2));
	area1 = fabs(width1 * height1 * pixarea1);
	area2 = fabs(width2 * height2 * pixarea2);
	RASTER_DEBUGF(4, "pixarea1, pixarea2, area1, area2 = %f, %f, %f, %f",
		pixarea1, pixarea2, area1, area2);
	if (
		(within <= 0) ||
		(area1 < area2) ||
		FLT_EQ(area1, area2) ||
		(area1 < pixarea2) || /* area of rast1 smaller than pixel area of rast2 */
		FLT_EQ(area1, pixarea2)
	) {
		rastS = rast1;
		nbandS = nband1;
		widthS = &width1;
		heightS = &height1;

		rastL = rast2;
		nbandL = nband2;
		widthL = &width2;
		heightL = &height2;
	}
	else {
		rastS = rast2;
		nbandS = nband2;
		widthS = &width2;
		heightS = &height2;

		rastL = rast1;
		nbandL = nband1;
		widthL = &width1;
		heightL = &height1;
	}

	/* no band to use, set band to zero */
	if (nband1 < 0) {
		nbandS = 0;
		nbandL = 0;
	}

	RASTER_DEBUGF(4, "rast1 @ %p", rast1);
	RASTER_DEBUGF(4, "rast2 @ %p", rast2);
	RASTER_DEBUGF(4, "rastS @ %p", rastS);
	RASTER_DEBUGF(4, "rastL @ %p", rastL);

	/* load band of smaller raster */
	bandS = rt_raster_get_band(rastS, nbandS);
	if (NULL == bandS) {
		rterror("rt_raster_intersects: Could not get band %d of the first raster", nbandS);
		*intersects = 0;
		return ES_ERROR;
	}

	hasnodataS = rt_band_get_hasnodata_flag(bandS);
	if (hasnodataS != FALSE)
		rt_band_get_nodata(bandS, &nodataS);

	/* load band of larger raster */
	bandL = rt_raster_get_band(rastL, nbandL);
	if (NULL == bandL) {
		rterror("rt_raster_intersects: Could not get band %d of the first raster", nbandL);
		*intersects = 0;
		return ES_ERROR;
	}

	hasnodataL = rt_band_get_hasnodata_flag(bandL);
	if (hasnodataL != FALSE)
		rt_band_get_nodata(bandL, &nodataL);

	/* no band to use, ignore nodata */
	if (nband1 < 0) {
		hasnodataS = FALSE;
		hasnodataL = FALSE;
	}

	/* hasnodata(S|L) = TRUE and one of the two bands is isnodata */
	if (
		(hasnodataS && rt_band_get_isnodata_flag(bandS)) ||
		(hasnodataL && rt_band_get_isnodata_flag(bandL))
	) {
		RASTER_DEBUG(3, "One of the two raster bands is NODATA. The two rasters do not intersect");
		*intersects = 0;
		return ES_NONE;
	}

	/* special case where a raster can fit inside another raster's pixel */
	if (within != 0 && ((pixarea1 > area2) || (pixarea2 > area1))) {
		RASTER_DEBUG(4, "Using special case of raster fitting into another raster's pixel");
		/* 3 x 3 search */
		for (coloffset = 0; coloffset < 3; coloffset++) {
			for (rowoffset = 0; rowoffset < 3; rowoffset++) {
				for (col = coloffset; col < *widthS; col += 3) {
					for (row = rowoffset; row < *heightS; row += 3) {
						if (hasnodataS == FALSE)
							valS = 1;
						else if (rt_band_get_pixel(bandS, col, row, &valS, &isnodataS) != ES_NONE)
							continue;

						if ((hasnodataS == FALSE) || !isnodataS) {
							rt_raster_cell_to_geopoint(
								rastS,
								col, row,
								&(lineS[X1]), &(lineS[Y1]),
								gtS
							);

							if (rt_raster_geopoint_to_cell(
								rastL,
								lineS[X1], lineS[Y1],
								&(Qr[pX]), &(Qr[pY]),
								igtL
							) != ES_NONE) {
								continue;
							}

							if (
								(Qr[pX] < 0 || Qr[pX] > *widthL || FLT_EQ(Qr[pX], *widthL)) ||
								(Qr[pY] < 0 || Qr[pY] > *heightL || FLT_EQ(Qr[pY], *heightL))
							) {
								continue;
							}

							if (hasnodataS == FALSE)
								valL = 1;
							else if (rt_band_get_pixel(bandL, Qr[pX], Qr[pY], &valL, &isnodataL) != ES_NONE)
								continue;

							if ((hasnodataL == FALSE) || !isnodataL) {
								RASTER_DEBUG(3, "The two rasters do intersect");
								*intersects = 1;
								return ES_NONE;
							}
						}
					}
				}
			}
		}
		RASTER_DEBUG(4, "Smaller raster not in the other raster's pixel. Continuing");
	}

	RASTER_DEBUG(4, "Testing smaller raster vs larger raster");
	*intersects = rt_raster_intersects_algorithm(
		rastS, rastL,
		bandS, bandL,
		hasnodataS, hasnodataL,
		nodataS, nodataL
	);

	if (*intersects) return ES_NONE;

	RASTER_DEBUG(4, "Testing larger raster vs smaller raster");
	*intersects = rt_raster_intersects_algorithm(
		rastL, rastS,
		bandL, bandS,
		hasnodataL, hasnodataS,
		nodataL, nodataS
	);

	if (*intersects) return ES_NONE;

	RASTER_DEBUG(3, "The two rasters do not intersect");

	*intersects = 0;
	return ES_NONE;
}

/******************************************************************************
* GEOS-based spatial relationship tests
******************************************************************************/

static
rt_errorstate rt_raster_geos_spatial_relationship(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	rt_geos_spatial_test testtype,
	int *testresult
) {
	LWMPOLY *surface1 = NULL;
	LWMPOLY *surface2 = NULL;
	GEOSGeometry *geom1 = NULL;
	GEOSGeometry *geom2 = NULL;
	int rtn = 0;
	int flag = 0;

	RASTER_DEBUG(3, "Starting");

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != testresult);

	if (nband1 < 0 && nband2 < 0) {
		nband1 = -1;
		nband2 = -1;
	}
	else {
		assert(nband1 >= 0 && nband1 < rt_raster_get_num_bands(rast1));
		assert(nband2 >= 0 && nband2 < rt_raster_get_num_bands(rast2));
	}

	/* initialize to zero, false result of spatial relationship test */
	*testresult = 0;

	/* same srid */
	if (rt_raster_get_srid(rast1) != rt_raster_get_srid(rast2)) {
		rterror("rt_raster_geos_spatial_relationship: The two rasters provided have different SRIDs");
		return ES_ERROR;
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	/* get LWMPOLY of each band */
	if (rt_raster_surface(rast1, nband1, &surface1) != ES_NONE) {
		rterror("rt_raster_geos_spatial_relationship: Could not get surface of the specified band from the first raster");
		return ES_ERROR;
	}
	if (rt_raster_surface(rast2, nband2, &surface2) != ES_NONE) {
		rterror("rt_raster_geos_spatial_relationship: Could not get surface of the specified band from the second raster");
		lwmpoly_free(surface1);
		return ES_ERROR;
	}

	/* either surface is NULL, spatial relationship test is false */
	if (surface1 == NULL || surface2 == NULL) {
		if (surface1 != NULL) lwmpoly_free(surface1);
		if (surface2 != NULL) lwmpoly_free(surface2);
		return ES_NONE;
	}

	/* convert LWMPOLY to GEOSGeometry */
	geom1 = LWGEOM2GEOS(lwmpoly_as_lwgeom(surface1));
	lwmpoly_free(surface1);
	if (geom1 == NULL) {
		rterror("rt_raster_geos_spatial_relationship: Could not convert surface of the specified band from the first raster to a GEOSGeometry");
		lwmpoly_free(surface2);
		return ES_ERROR;
	}

	geom2 = LWGEOM2GEOS(lwmpoly_as_lwgeom(surface2));
	lwmpoly_free(surface2);
	if (geom2 == NULL) {
		rterror("rt_raster_geos_spatial_relationship: Could not convert surface of the specified band from the second raster to a GEOSGeometry");
		return ES_ERROR;
	}

	flag = 0;
	switch (testtype) {
		case GSR_OVERLAPS:
			rtn = GEOSOverlaps(geom1, geom2);
			break;
		case GSR_TOUCHES:
			rtn = GEOSTouches(geom1, geom2);
			break;
		case GSR_CONTAINS:
			rtn = GEOSContains(geom1, geom2);
			break;
		case GSR_CONTAINSPROPERLY:
			rtn = GEOSRelatePattern(geom1, geom2, "T**FF*FF*");
			break;
		case GSR_COVERS:
			rtn = GEOSRelatePattern(geom1, geom2, "******FF*");
			break;
		case GSR_COVEREDBY:
			rtn = GEOSRelatePattern(geom1, geom2, "**F**F***");
			break;
		default:
			rterror("rt_raster_geos_spatial_relationship: Unknown or unsupported GEOS spatial relationship test");
			flag = -1;
			break;
	}
	GEOSGeom_destroy(geom1);
	GEOSGeom_destroy(geom2);

	/* something happened in the spatial relationship test */
	if (rtn == 2) {
		rterror("rt_raster_geos_spatial_relationship: Could not run the appropriate GEOS spatial relationship test");
		flag = ES_ERROR;
	}
	/* spatial relationship test ran fine */
	else if (flag >= 0) {
		if (rtn != 0)
			*testresult = 1;
		flag = ES_NONE;
	}
	/* flag < 0 for when testtype is unknown */
	else
		flag = ES_ERROR;

	return flag;
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter overlaps returns non-zero if two rasters overlap
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param overlaps : non-zero value if the two rasters' bands overlaps
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_overlaps(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *overlaps
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_OVERLAPS,
		overlaps
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter touches returns non-zero if two rasters touch
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param touches : non-zero value if the two rasters' bands touch
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_touches(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *touches
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_TOUCHES,
		touches
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter contains returns non-zero if rast1 contains rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param contains : non-zero value if rast1 contains rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_contains(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *contains
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_CONTAINS,
		contains
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter contains returns non-zero if rast1 contains properly rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param contains : non-zero value if rast1 contains properly rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_contains_properly(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *contains
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_CONTAINSPROPERLY,
		contains
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter covers returns non-zero if rast1 covers rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param covers : non-zero value if rast1 covers rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_covers(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *covers
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_COVERS,
		covers
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter coveredby returns non-zero if rast1 is covered by rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param coveredby : non-zero value if rast1 is covered by rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_coveredby(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	int *coveredby
) {
	RASTER_DEBUG(3, "Starting");

	return rt_raster_geos_spatial_relationship(
		rast1, nband1,
		rast2, nband2,
		GSR_COVEREDBY,
		coveredby
	);
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter dwithin returns non-zero if rast1 is within the specified
 *   distance of rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param dwithin : non-zero value if rast1 is within the specified distance
 *   of rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_within_distance(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	double distance,
	int *dwithin
) {
	LWMPOLY *surface = NULL;
	LWGEOM *surface1 = NULL;
	LWGEOM *surface2 = NULL;
	double mindist = 0;

	RASTER_DEBUG(3, "Starting");

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != dwithin);

	if (nband1 < 0 && nband2 < 0) {
		nband1 = -1;
		nband2 = -1;
	}
	else {
		assert(nband1 >= 0 && nband1 < rt_raster_get_num_bands(rast1));
		assert(nband2 >= 0 && nband2 < rt_raster_get_num_bands(rast2));
	}

	/* initialize to zero, false result */
	*dwithin = 0;

	/* same srid */
	if (rt_raster_get_srid(rast1) != rt_raster_get_srid(rast2)) {
		rterror("rt_raster_distance_within: The two rasters provided have different SRIDs");
		return ES_ERROR;
	}

	/* distance cannot be less than zero */
	if (distance < 0) {
		rterror("rt_raster_distance_within: Distance cannot be less than zero");
		return ES_ERROR;
	}

	/* get LWMPOLY of each band */
	if (rt_raster_surface(rast1, nband1, &surface) != ES_NONE) {
		rterror("rt_raster_distance_within: Could not get surface of the specified band from the first raster");
		return ES_ERROR;
	}
	surface1 = lwmpoly_as_lwgeom(surface);

	if (rt_raster_surface(rast2, nband2, &surface) != ES_NONE) {
		rterror("rt_raster_distance_within: Could not get surface of the specified band from the second raster");
		lwgeom_free(surface1);
		return ES_ERROR;
	}
	surface2 = lwmpoly_as_lwgeom(surface);

	/* either surface is NULL, test is false */
	if (surface1 == NULL || surface2 == NULL) {
		if (surface1 != NULL) lwgeom_free(surface1);
		if (surface2 != NULL) lwgeom_free(surface2);
		return ES_NONE;
	}

	/* get the min distance between the two surfaces */
	mindist = lwgeom_mindistance2d_tolerance(surface1, surface2, distance);

	lwgeom_free(surface1);
	lwgeom_free(surface2);

	/* if distance >= mindist, true */
	if (FLT_EQ(mindist, distance) || distance > mindist)
		*dwithin = 1;

	RASTER_DEBUGF(3, "(mindist, distance) = (%f, %f, %d)", mindist, distance, *dwithin);

	return ES_NONE;
}

/**
 * Return ES_ERROR if error occurred in function.
 * Parameter dfwithin returns non-zero if rast1 is fully within the specified
 *   distance of rast2
 *
 * @param rast1 : the first raster whose band will be tested
 * @param nband1 : the 0-based band of raster rast1 to use
 *   if value is less than zero, bands are ignored.
 *   if nband1 gte zero, nband2 must be gte zero
 * @param rast2 : the second raster whose band will be tested
 * @param nband2 : the 0-based band of raster rast2 to use
 *   if value is less than zero, bands are ignored
 *   if nband2 gte zero, nband1 must be gte zero
 * @param dfwithin : non-zero value if rast1 is fully within the specified
 *   distance of rast2
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_fully_within_distance(
	rt_raster rast1, int nband1,
	rt_raster rast2, int nband2,
	double distance,
	int *dfwithin
) {
	LWMPOLY *surface = NULL;
	LWGEOM *surface1 = NULL;
	LWGEOM *surface2 = NULL;
	double maxdist = 0;

	RASTER_DEBUG(3, "Starting");

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != dfwithin);

	if (nband1 < 0 && nband2 < 0) {
		nband1 = -1;
		nband2 = -1;
	}
	else {
		assert(nband1 >= 0 && nband1 < rt_raster_get_num_bands(rast1));
		assert(nband2 >= 0 && nband2 < rt_raster_get_num_bands(rast2));
	}

	/* initialize to zero, false result */
	*dfwithin = 0;

	/* same srid */
	if (rt_raster_get_srid(rast1) != rt_raster_get_srid(rast2)) {
		rterror("rt_raster_fully_within_distance: The two rasters provided have different SRIDs");
		return ES_ERROR;
	}

	/* distance cannot be less than zero */
	if (distance < 0) {
		rterror("rt_raster_fully_within_distance: Distance cannot be less than zero");
		return ES_ERROR;
	}

	/* get LWMPOLY of each band */
	if (rt_raster_surface(rast1, nband1, &surface) != ES_NONE) {
		rterror("rt_raster_fully_within_distance: Could not get surface of the specified band from the first raster");
		return ES_ERROR;
	}
	surface1 = lwmpoly_as_lwgeom(surface);

	if (rt_raster_surface(rast2, nband2, &surface) != ES_NONE) {
		rterror("rt_raster_fully_within_distance: Could not get surface of the specified band from the second raster");
		lwgeom_free(surface1);
		return ES_ERROR;
	}
	surface2 = lwmpoly_as_lwgeom(surface);

	/* either surface is NULL, test is false */
	if (surface1 == NULL || surface2 == NULL) {
		if (surface1 != NULL) lwgeom_free(surface1);
		if (surface2 != NULL) lwgeom_free(surface2);
		return ES_NONE;
	}

	/* get the maximum distance between the two surfaces */
	maxdist = lwgeom_maxdistance2d_tolerance(surface1, surface2, distance);

	lwgeom_free(surface1);
	lwgeom_free(surface2);

	/* if distance >= maxdist, true */
	if (FLT_EQ(maxdist, distance) || distance > maxdist)
		*dfwithin = 1;

	RASTER_DEBUGF(3, "(maxdist, distance, dfwithin) = (%f, %f, %d)", maxdist, distance, *dfwithin);

	return ES_NONE;
}

/*
 * Return ES_ERROR if error occurred in function.
 * Paramter aligned returns non-zero if two rasters are aligned
 *
 * @param rast1 : the first raster for alignment test
 * @param rast2 : the second raster for alignment test
 * @param *aligned : non-zero value if the two rasters are aligned
 * @param *reason : reason why rasters are not aligned
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_raster_same_alignment(
	rt_raster rast1,
	rt_raster rast2,
	int *aligned, char **reason
) {
	double xr;
	double yr;
	double xw;
	double yw;
	int err = 0;

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != aligned);

	err = 0;
	/* same srid */
	if (rt_raster_get_srid(rast1) != rt_raster_get_srid(rast2)) {
		if (reason != NULL) *reason = "The rasters have different SRIDs";
		err = 1;
	}
	/* scales must match */
	else if (FLT_NEQ(fabs(rast1->scaleX), fabs(rast2->scaleX))) {
		if (reason != NULL) *reason = "The rasters have different scales on the X axis";
		err = 1;
	}
	else if (FLT_NEQ(fabs(rast1->scaleY), fabs(rast2->scaleY))) {
		if (reason != NULL) *reason = "The rasters have different scales on the Y axis";
		err = 1;
	}
	/* skews must match */
	else if (FLT_NEQ(rast1->skewX, rast2->skewX)) {
		if (reason != NULL) *reason = "The rasters have different skews on the X axis";
		err = 1;
	}
	else if (FLT_NEQ(rast1->skewY, rast2->skewY)) {
		if (reason != NULL) *reason = "The rasters have different skews on the Y axis";
		err = 1;
	}

	if (err) {
		*aligned = 0;
		return ES_NONE;
	}

	/* raster coordinates in context of second raster of first raster's upper-left corner */
	if (rt_raster_geopoint_to_cell(
			rast2,
			rast1->ipX, rast1->ipY,
			&xr, &yr,
			NULL
	) != ES_NONE) {
		rterror("rt_raster_same_alignment: Could not get raster coordinates of second raster from first raster's spatial coordinates");
		*aligned = 0;
		return ES_ERROR;
	}

	/* spatial coordinates of raster coordinates from above */
	if (rt_raster_cell_to_geopoint(
		rast2,
		xr, yr,
		&xw, &yw,
		NULL
	) != ES_NONE) {
		rterror("rt_raster_same_alignment: Could not get spatial coordinates of second raster from raster coordinates");
		*aligned = 0;
		return ES_ERROR;
	}

	RASTER_DEBUGF(4, "rast1(ipX, ipxY) = (%f, %f)", rast1->ipX, rast1->ipY);
	RASTER_DEBUGF(4, "rast2(xr, yr) = (%f, %f)", xr, yr);
	RASTER_DEBUGF(4, "rast2(xw, yw) = (%f, %f)", xw, yw);

	/* spatial coordinates are identical to that of first raster's upper-left corner */
	if (FLT_EQ(xw, rast1->ipX) && FLT_EQ(yw, rast1->ipY)) {
		if (reason != NULL) *reason = "The rasters are aligned";
		*aligned = 1;
		return ES_NONE;
	}

	/* no alignment */
	if (reason != NULL) *reason = "The rasters (pixel corner coordinates) are not aligned";

	*aligned = 0;
	return ES_NONE;
}

/*
 * Return raster of computed extent specified extenttype applied
 * on two input rasters.  The raster returned should be freed by
 * the caller
 *
 * @param rast1 : the first raster
 * @param rast2 : the second raster
 * @param extenttype : type of extent for the output raster
 * @param *rtnraster : raster of computed extent
 * @param *offset : 4-element array indicating the X,Y offsets
 * for each raster. 0,1 for rast1 X,Y. 2,3 for rast2 X,Y.
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate
rt_raster_from_two_rasters(
	rt_raster rast1, rt_raster rast2,
	rt_extenttype extenttype,
	rt_raster *rtnraster, double *offset
) {
	int i;

	rt_raster _rast[2] = {rast1, rast2};
	double _offset[2][4] = {{0.}};
	uint16_t _dim[2][2] = {{0}};

	rt_raster raster = NULL;
	int aligned = 0;
	int dim[2] = {0};
	double gt[6] = {0.};

	assert(NULL != rast1);
	assert(NULL != rast2);
	assert(NULL != rtnraster);

	/* set rtnraster to NULL */
	*rtnraster = NULL;

	/* rasters must be aligned */
	if (rt_raster_same_alignment(rast1, rast2, &aligned, NULL) != ES_NONE) {
		rterror("rt_raster_from_two_rasters: Could not test for alignment on the two rasters");
		return ES_ERROR;
	}
	if (!aligned) {
		rterror("rt_raster_from_two_rasters: The two rasters provided do not have the same alignment");
		return ES_ERROR;
	}

	/* dimensions */
	_dim[0][0] = rast1->width;
	_dim[0][1] = rast1->height;
	_dim[1][0] = rast2->width;
	_dim[1][1] = rast2->height;

	/* get raster offsets */
	if (rt_raster_geopoint_to_cell(
		_rast[1],
		_rast[0]->ipX, _rast[0]->ipY,
		&(_offset[1][0]), &(_offset[1][1]),
		NULL
	) != ES_NONE) {
		rterror("rt_raster_from_two_rasters: Could not compute offsets of the second raster relative to the first raster");
		return ES_ERROR;
	}
	_offset[1][0] = -1 * _offset[1][0];
	_offset[1][1] = -1 * _offset[1][1];
	_offset[1][2] = _offset[1][0] + _dim[1][0] - 1;
	_offset[1][3] = _offset[1][1] + _dim[1][1] - 1;

	i = -1;
	switch (extenttype) {
		case ET_FIRST:
			i = 0;
			_offset[0][0] = 0.;
			_offset[0][1] = 0.;
		case ET_LAST:
		case ET_SECOND:
			if (i < 0) {
				i = 1;
				_offset[0][0] = -1 * _offset[1][0];
				_offset[0][1] = -1 * _offset[1][1];
				_offset[1][0] = 0.;
				_offset[1][1] = 0.;
			}

			dim[0] = _dim[i][0];
			dim[1] = _dim[i][1];
			raster = rt_raster_new(
				dim[0],
				dim[1]
			);
			if (raster == NULL) {
				rterror("rt_raster_from_two_rasters: Could not create output raster");
				return ES_ERROR;
			}
			rt_raster_set_srid(raster, _rast[i]->srid);
			rt_raster_get_geotransform_matrix(_rast[i], gt);
			rt_raster_set_geotransform_matrix(raster, gt);
			break;
		case ET_UNION: {
			double off[4] = {0};

			rt_raster_get_geotransform_matrix(_rast[0], gt);
			RASTER_DEBUGF(4, "gt = (%f, %f, %f, %f, %f, %f)",
				gt[0],
				gt[1],
				gt[2],
				gt[3],
				gt[4],
				gt[5]
			);

			/* new raster upper left offset */
			off[0] = 0;
			if (_offset[1][0] < 0)
				off[0] = _offset[1][0];
			off[1] = 0;
			if (_offset[1][1] < 0)
				off[1] = _offset[1][1];

			/* new raster lower right offset */
			off[2] = _dim[0][0] - 1;
			if ((int) _offset[1][2] >= _dim[0][0])
				off[2] = _offset[1][2];
			off[3] = _dim[0][1] - 1;
			if ((int) _offset[1][3] >= _dim[0][1])
				off[3] = _offset[1][3];

			/* upper left corner */
			if (rt_raster_cell_to_geopoint(
				_rast[0],
				off[0], off[1],
				&(gt[0]), &(gt[3]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get spatial coordinates of upper-left pixel of output raster");
				return ES_ERROR;
			}

			dim[0] = off[2] - off[0] + 1;
			dim[1] = off[3] - off[1] + 1;
			RASTER_DEBUGF(4, "off = (%f, %f, %f, %f)",
				off[0],
				off[1],
				off[2],
				off[3]
			);
			RASTER_DEBUGF(4, "dim = (%d, %d)", dim[0], dim[1]);

			raster = rt_raster_new(
				dim[0],
				dim[1]
			);
			if (raster == NULL) {
				rterror("rt_raster_from_two_rasters: Could not create output raster");
				return ES_ERROR;
			}
			rt_raster_set_srid(raster, _rast[0]->srid);
			rt_raster_set_geotransform_matrix(raster, gt);
			RASTER_DEBUGF(4, "gt = (%f, %f, %f, %f, %f, %f)",
				gt[0],
				gt[1],
				gt[2],
				gt[3],
				gt[4],
				gt[5]
			);

			/* get offsets */
			if (rt_raster_geopoint_to_cell(
				_rast[0],
				gt[0], gt[3],
				&(_offset[0][0]), &(_offset[0][1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get offsets of the FIRST raster relative to the output raster");
				rt_raster_destroy(raster);
				return ES_ERROR;
			}
			_offset[0][0] *= -1;
			_offset[0][1] *= -1;

			if (rt_raster_geopoint_to_cell(
				_rast[1],
				gt[0], gt[3],
				&(_offset[1][0]), &(_offset[1][1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get offsets of the SECOND raster relative to the output raster");
				rt_raster_destroy(raster);
				return ES_ERROR;
			}
			_offset[1][0] *= -1;
			_offset[1][1] *= -1;
			break;
		}
		case ET_INTERSECTION: {
			double off[4] = {0};

			/* no intersection */
			if (
				(_offset[1][2] < 0 || _offset[1][0] > (_dim[0][0] - 1)) ||
				(_offset[1][3] < 0 || _offset[1][1] > (_dim[0][1] - 1))
			) {
				RASTER_DEBUG(3, "The two rasters provided have no intersection.  Returning no band raster");

				raster = rt_raster_new(0, 0);
				if (raster == NULL) {
					rterror("rt_raster_from_two_rasters: Could not create output raster");
					return ES_ERROR;
				}
				rt_raster_set_srid(raster, _rast[0]->srid);
				rt_raster_set_scale(raster, 0, 0);

				/* set offsets if provided */
				if (NULL != offset) {
					for (i = 0; i < 4; i++)
						offset[i] = _offset[i / 2][i % 2];
				}

				*rtnraster = raster;
				return ES_NONE;
			}

			if (_offset[1][0] > 0)
				off[0] = _offset[1][0];
			if (_offset[1][1] > 0)
				off[1] = _offset[1][1];

			off[2] = _dim[0][0] - 1;
			if (_offset[1][2] < _dim[0][0])
				off[2] = _offset[1][2];
			off[3] = _dim[0][1] - 1;
			if (_offset[1][3] < _dim[0][1])
				off[3] = _offset[1][3];

			dim[0] = off[2] - off[0] + 1;
			dim[1] = off[3] - off[1] + 1;
			raster = rt_raster_new(
				dim[0],
				dim[1]
			);
			if (raster == NULL) {
				rterror("rt_raster_from_two_rasters: Could not create output raster");
				return ES_ERROR;
			}
			rt_raster_set_srid(raster, _rast[0]->srid);

			/* get upper-left corner */
			rt_raster_get_geotransform_matrix(_rast[0], gt);
			if (rt_raster_cell_to_geopoint(
				_rast[0],
				off[0], off[1],
				&(gt[0]), &(gt[3]),
				gt
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get spatial coordinates of upper-left pixel of output raster");
				rt_raster_destroy(raster);
				return ES_ERROR;
			}

			rt_raster_set_geotransform_matrix(raster, gt);

			/* get offsets */
			if (rt_raster_geopoint_to_cell(
				_rast[0],
				gt[0], gt[3],
				&(_offset[0][0]), &(_offset[0][1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get pixel coordinates to compute the offsets of the FIRST raster relative to the output raster");
				rt_raster_destroy(raster);
				return ES_ERROR;
			}
			_offset[0][0] *= -1;
			_offset[0][1] *= -1;

			if (rt_raster_geopoint_to_cell(
				_rast[1],
				gt[0], gt[3],
				&(_offset[1][0]), &(_offset[1][1]),
				NULL
			) != ES_NONE) {
				rterror("rt_raster_from_two_rasters: Could not get pixel coordinates to compute the offsets of the SECOND raster relative to the output raster");
				rt_raster_destroy(raster);
				return ES_ERROR;
			}
			_offset[1][0] *= -1;
			_offset[1][1] *= -1;
			break;
		}
		case ET_CUSTOM:
			rterror("rt_raster_from_two_rasters: Extent type ET_CUSTOM is not supported by this function");
			break;
	}

	/* set offsets if provided */
	if (NULL != offset) {
		for (i = 0; i < 4; i++)
			offset[i] = _offset[i / 2][i % 2];
	}

	*rtnraster = raster;
	return ES_NONE;
}

/**
 * Get a raster pixel as a polygon.
 *
 * The pixel shape is a 4 vertices (5 to be closed) single
 * ring polygon bearing the raster's rotation
 * and using projection coordinates
 *
 * @param raster : the raster to get pixel from
 * @param x : the column number
 * @param y : the row number
 *
 * @return the pixel polygon, or NULL on error.
 *
 */
LWPOLY*
rt_raster_pixel_as_polygon(rt_raster rast, int x, int y)
{
    double scale_x, scale_y;
    double skew_x, skew_y;
    double ul_x, ul_y;
    int srid;
    POINTARRAY **points;
    POINT4D p, p0;
    LWPOLY *poly;

		assert(rast != NULL);

    scale_x = rt_raster_get_x_scale(rast);
    scale_y = rt_raster_get_y_scale(rast);
    skew_x = rt_raster_get_x_skew(rast);
    skew_y = rt_raster_get_y_skew(rast);
    ul_x = rt_raster_get_x_offset(rast);
    ul_y = rt_raster_get_y_offset(rast);
    srid = rt_raster_get_srid(rast);

    points = rtalloc(sizeof(POINTARRAY *)*1);
    points[0] = ptarray_construct(0, 0, 5);

    p0.x = scale_x * x + skew_x * y + ul_x;
    p0.y = scale_y * y + skew_y * x + ul_y;
    ptarray_set_point4d(points[0], 0, &p0);

    p.x = p0.x + scale_x;
    p.y = p0.y + skew_y;
    ptarray_set_point4d(points[0], 1, &p);

    p.x = p0.x + scale_x + skew_x;
    p.y = p0.y + scale_y + skew_y;
    ptarray_set_point4d(points[0], 2, &p);

    p.x = p0.x + skew_x;
    p.y = p0.y + scale_y;
    ptarray_set_point4d(points[0], 3, &p);

    /* close it */
    ptarray_set_point4d(points[0], 4, &p0);

    poly = lwpoly_construct(srid, NULL, 1, points);

    return poly;
}

/**
 * Get a raster as a surface (multipolygon).  If a band is specified,
 * those pixels with value (not NODATA) contribute to the area
 * of the output multipolygon.
 *
 * @param raster : the raster to convert to a multipolygon
 * @param nband : the 0-based band of raster rast to use
 *   if value is less than zero, bands are ignored.
 * @param *surface : raster as a surface (multipolygon).
 *   if all pixels are NODATA, NULL is set
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate rt_raster_surface(rt_raster raster, int nband, LWMPOLY **surface) {
	rt_band band = NULL;
	LWGEOM *mpoly = NULL;
	LWGEOM *tmp = NULL;
	LWGEOM *clone = NULL;
	rt_geomval gv = NULL;
	int gvcount = 0;
	GEOSGeometry *gc = NULL;
	GEOSGeometry *gunion = NULL;
	GEOSGeometry **geoms = NULL;
	int geomscount = 0;
	int i = 0;

	assert(surface != NULL);

	/* init *surface to NULL */
	*surface = NULL;

	/* raster is empty, surface = NULL */
	if (rt_raster_is_empty(raster))
		return ES_NONE;

	/* if nband < 0, return the convex hull as a multipolygon */
	if (nband < 0) {
		/*
			lwgeom_as_multi() only does a shallow clone internally
			so input and output geometries may share memory
			hence the deep clone of the output geometry for returning
			is the only way to guarentee the memory isn't shared
		*/
		if (rt_raster_get_convex_hull(raster, &tmp) != ES_NONE) {
			rterror("rt_raster_surface: Could not get convex hull of raster");
			return ES_ERROR;
		}
		mpoly = lwgeom_as_multi(tmp);
		clone = lwgeom_clone_deep(mpoly);
		lwgeom_free(tmp);
		lwgeom_free(mpoly);

		*surface = lwgeom_as_lwmpoly(clone);
		return ES_NONE;
	}
	/* check that nband is valid */
	else if (nband >= rt_raster_get_num_bands(raster)) {
		rterror("rt_raster_surface: The band index %d is invalid", nband);
		return ES_ERROR;
	}

	/* get band */
	band = rt_raster_get_band(raster, nband);
	if (band == NULL) {
		rterror("rt_raster_surface: Error getting band %d from raster", nband);
		return ES_ERROR;
	}

	/* band does not have a NODATA flag, return convex hull */
	if (!rt_band_get_hasnodata_flag(band)) {
		/*
			lwgeom_as_multi() only does a shallow clone internally
			so input and output geometries may share memory
			hence the deep clone of the output geometry for returning
			is the only way to guarentee the memory isn't shared
		*/
		if (rt_raster_get_convex_hull(raster, &tmp) != ES_NONE) {
			rterror("rt_raster_surface: Could not get convex hull of raster");
			return ES_ERROR;
		}
		mpoly = lwgeom_as_multi(tmp);
		clone = lwgeom_clone_deep(mpoly);
		lwgeom_free(tmp);
		lwgeom_free(mpoly);

		*surface = lwgeom_as_lwmpoly(clone);
		return ES_NONE;
	}
	/* band is NODATA, return NULL */
	else if (rt_band_get_isnodata_flag(band)) {
		RASTER_DEBUG(3, "Band is NODATA.  Returning NULL");
		return ES_NONE;
	}

	/* initialize GEOS */
	initGEOS(lwnotice, lwgeom_geos_error);

	/* use gdal polygonize */
	gv = rt_raster_gdal_polygonize(raster, nband, 1, &gvcount);
	/* no polygons returned */
	if (gvcount < 1) {
		RASTER_DEBUG(3, "All pixels of band are NODATA.  Returning NULL");
		if (gv != NULL) rtdealloc(gv);
		return ES_NONE;
	}
	/* more than 1 polygon */
	else if (gvcount > 1) {
		/* convert LWPOLY to GEOSGeometry */
		geomscount = gvcount;
		geoms = rtalloc(sizeof(GEOSGeometry *) * geomscount);
		if (geoms == NULL) {
			rterror("rt_raster_surface: Could not allocate memory for pixel polygons as GEOSGeometry");
			for (i = 0; i < gvcount; i++) lwpoly_free(gv[i].geom);
			rtdealloc(gv);
			return ES_ERROR;
		}
		for (i = 0; i < gvcount; i++) {
#if POSTGIS_DEBUG_LEVEL > 3
			{
				char *wkt = lwgeom_to_wkt(lwpoly_as_lwgeom(gv[i].geom), WKT_ISO, DBL_DIG, NULL);
				RASTER_DEBUGF(4, "geom %d = %s", i, wkt);
				rtdealloc(wkt);
			}
#endif

			geoms[i] = LWGEOM2GEOS(lwpoly_as_lwgeom(gv[i].geom));
			lwpoly_free(gv[i].geom);
		}
		rtdealloc(gv);

		/* create geometry collection */
#if POSTGIS_GEOS_VERSION >= 33
		gc = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, geoms, geomscount);
#else
		gc = GEOSGeom_createCollection(GEOS_MULTIPOLYGON, geoms, geomscount);
#endif

		if (gc == NULL) {
#if POSTGIS_GEOS_VERSION >= 33
			rterror("rt_raster_surface: Could not create GEOS GEOMETRYCOLLECTION from set of pixel polygons");
#else
			rterror("rt_raster_surface: Could not create GEOS MULTIPOLYGON from set of pixel polygons");
#endif

			for (i = 0; i < geomscount; i++)
				GEOSGeom_destroy(geoms[i]);
			rtdealloc(geoms);
			return ES_ERROR;
		}

		/* run the union */
#if POSTGIS_GEOS_VERSION >= 33
		gunion = GEOSUnaryUnion(gc);
#else
		gunion = GEOSUnionCascaded(gc);
#endif
		GEOSGeom_destroy(gc);
		rtdealloc(geoms);

		if (gunion == NULL) {
#if POSTGIS_GEOS_VERSION >= 33
			rterror("rt_raster_surface: Could not union the pixel polygons using GEOSUnaryUnion()");
#else
			rterror("rt_raster_surface: Could not union the pixel polygons using GEOSUnionCascaded()");
#endif
			return ES_ERROR;
		}

		/* convert union result from GEOSGeometry to LWGEOM */
		mpoly = GEOS2LWGEOM(gunion, 0);

		/*
			is geometry valid?
			if not, try to make valid
		*/
		do {
			LWGEOM *mpolyValid = NULL;

#if POSTGIS_GEOS_VERSION < 33
			break;
#endif

			if (GEOSisValid(gunion))
				break;

			/* make geometry valid */
			mpolyValid = lwgeom_make_valid(mpoly);
			if (mpolyValid == NULL) {
				rtwarn("Cannot fix invalid geometry");
				break;
			}

			lwgeom_free(mpoly);
			mpoly = mpolyValid;
		}
		while (0);

		GEOSGeom_destroy(gunion);
	}
	else {
		mpoly = lwpoly_as_lwgeom(gv[0].geom);
		rtdealloc(gv);

#if POSTGIS_DEBUG_LEVEL > 3
			{
				char *wkt = lwgeom_to_wkt(mpoly, WKT_ISO, DBL_DIG, NULL);
				RASTER_DEBUGF(4, "geom 0 = %s", wkt);
				rtdealloc(wkt);
			}
#endif
	}

	/* specify SRID */
	lwgeom_set_srid(mpoly, rt_raster_get_srid(raster));

	if (mpoly != NULL) {
		/* convert to multi */
		if (!lwgeom_is_collection(mpoly)) {
			tmp = mpoly;

#if POSTGIS_DEBUG_LEVEL > 3
			{
				char *wkt = lwgeom_to_wkt(mpoly, WKT_ISO, DBL_DIG, NULL);
				RASTER_DEBUGF(4, "before multi = %s", wkt);
				rtdealloc(wkt);
			}
#endif

			RASTER_DEBUGF(4, "mpoly @ %p", mpoly);

			/*
				lwgeom_as_multi() only does a shallow clone internally
				so input and output geometries may share memory
				hence the deep clone of the output geometry for returning
				is the only way to guarentee the memory isn't shared
			*/
			mpoly = lwgeom_as_multi(tmp);
			clone = lwgeom_clone_deep(mpoly);
			lwgeom_free(tmp);
			lwgeom_free(mpoly);
			mpoly = clone;

			RASTER_DEBUGF(4, "mpoly @ %p", mpoly);

#if POSTGIS_DEBUG_LEVEL > 3
			{
				char *wkt = lwgeom_to_wkt(mpoly, WKT_ISO, DBL_DIG, NULL);
				RASTER_DEBUGF(4, "after multi = %s", wkt);
				rtdealloc(wkt);
			}
#endif
		}

#if POSTGIS_DEBUG_LEVEL > 3
		{
			char *wkt = lwgeom_to_wkt(mpoly, WKT_ISO, DBL_DIG, NULL);
			RASTER_DEBUGF(4, "returning geometry = %s", wkt);
			rtdealloc(wkt);
		}
#endif

		*surface = lwgeom_as_lwmpoly(mpoly);
		return ES_NONE;
	}

	return ES_NONE;
}

/******************************************************************************
* rt_raster_iterator()
******************************************************************************/

typedef struct _rti_iterator_arg_t* _rti_iterator_arg;
struct _rti_iterator_arg_t {
	int count;

	rt_raster *raster;
	int *isempty;
	double **offset;
	int *width;
	int *height;

	struct {
		rt_band *rtband;
		int *hasnodata;
		int *isnodata;
		double *nodataval;
		double *minval;
	} band;

	struct {
		uint16_t x;
		uint16_t y;
	} distance;

	struct {
		uint32_t rows;
		uint32_t columns;
	} dimension;

	struct {
		double **values;
		int **nodata;
	} empty;

	rt_iterator_arg arg;
};

static _rti_iterator_arg
_rti_iterator_arg_init() {
	_rti_iterator_arg _param;

	_param = rtalloc(sizeof(struct _rti_iterator_arg_t));
	if (_param == NULL) {
		rterror("_rti_iterator_arg_init: Could not allocate memory for _rti_iterator_arg");
		return NULL;
	}

	_param->count = 0;

	_param->raster = NULL;
	_param->isempty = NULL;
	_param->offset = NULL;
	_param->width = NULL;
	_param->height = NULL;

	_param->band.rtband = NULL;
	_param->band.hasnodata = NULL;
	_param->band.isnodata = NULL;
	_param->band.nodataval = NULL;
	_param->band.minval = NULL;

	_param->distance.x = 0;
	_param->distance.y = 0;

	_param->dimension.rows = 0;
	_param->dimension.columns = 0;

	_param->empty.values = NULL;
	_param->empty.nodata = NULL;

	_param->arg = NULL;

	return _param;
}

static void
_rti_iterator_arg_destroy(_rti_iterator_arg _param) {
	int i = 0;

	if (_param->raster != NULL)
		rtdealloc(_param->raster);
	if (_param->isempty != NULL)
		rtdealloc(_param->isempty);
	if (_param->width != NULL)
		rtdealloc(_param->width);
	if (_param->height != NULL)
		rtdealloc(_param->height);

	if (_param->band.rtband != NULL)
		rtdealloc(_param->band.rtband);
	if (_param->band.hasnodata != NULL)
		rtdealloc(_param->band.hasnodata);
	if (_param->band.isnodata != NULL)
		rtdealloc(_param->band.isnodata);
	if (_param->band.nodataval != NULL)
		rtdealloc(_param->band.nodataval);
	if (_param->band.minval != NULL)
		rtdealloc(_param->band.minval);

	if (_param->offset != NULL) {
		for (i = 0; i < _param->count; i++) {
			if (_param->offset[i] == NULL)
				continue;
			rtdealloc(_param->offset[i]);
		}
		rtdealloc(_param->offset);
	}

	if (_param->empty.values != NULL) {
		for (i = 0; i < _param->dimension.rows; i++) {
			if (_param->empty.values[i] == NULL)
				continue;
			rtdealloc(_param->empty.values[i]);
		}
		rtdealloc(_param->empty.values);
	}
	if (_param->empty.nodata != NULL) {
		for (i = 0; i < _param->dimension.rows; i++) {
			if (_param->empty.nodata[i] == NULL)
				continue;
			rtdealloc(_param->empty.nodata[i]);
		}
		rtdealloc(_param->empty.nodata);
	}

	if (_param->arg != NULL) {
		if (_param->arg->values != NULL)
			rtdealloc(_param->arg->values);
		if (_param->arg->nodata != NULL)
			rtdealloc(_param->arg->nodata);
		if (_param->arg->src_pixel != NULL) {
			for (i = 0; i < _param->count; i++) {
				if (_param->arg->src_pixel[i] == NULL)
					continue;
				rtdealloc(_param->arg->src_pixel[i]);
			}

			rtdealloc(_param->arg->src_pixel);
		}

		rtdealloc(_param->arg);
	}

	rtdealloc(_param);
}

static int
_rti_iterator_arg_populate(
	_rti_iterator_arg _param,
	rt_iterator itrset, uint16_t itrcount,
	uint16_t distancex, uint16_t distancey,
	int *allnull, int *allempty
) {
	int i = 0;
	int hasband = 0;

	_param->count = itrcount;
	_param->distance.x = distancex;
	_param->distance.y = distancey;
	_param->dimension.columns = distancex * 2 + 1;
	_param->dimension.rows = distancey * 2 + 1;

	/* allocate memory for children */
	_param->raster = rtalloc(sizeof(rt_raster) * itrcount);
	_param->isempty = rtalloc(sizeof(int) * itrcount);
	_param->width = rtalloc(sizeof(int) * itrcount);
	_param->height = rtalloc(sizeof(int) * itrcount);

	_param->offset = rtalloc(sizeof(double *) * itrcount);

	_param->band.rtband = rtalloc(sizeof(rt_band) * itrcount);
	_param->band.hasnodata = rtalloc(sizeof(int) * itrcount);
	_param->band.isnodata = rtalloc(sizeof(int) * itrcount);
	_param->band.nodataval = rtalloc(sizeof(double) * itrcount);
	_param->band.minval = rtalloc(sizeof(double) * itrcount);

	if (
		_param->raster == NULL ||
		_param->isempty == NULL ||
		_param->width == NULL ||
		_param->height == NULL ||
		_param->offset == NULL ||
		_param->band.rtband == NULL ||
		_param->band.hasnodata == NULL ||
		_param->band.isnodata == NULL ||
		_param->band.nodataval == NULL ||
		_param->band.minval == NULL
	) {
		rterror("_rti_iterator_arg_populate: Could not allocate memory for children of _rti_iterator_arg");
		return 0;
	}

	*allnull = 0;
	*allempty = 0;

	/*
		check input rasters
			not empty, band # is valid
			copy raster pointers and set flags
	*/
	for (i = 0; i < itrcount; i++) {
		/* initialize elements */
		_param->raster[i] = NULL;
		_param->isempty[i] = 0;
		_param->width[i] = 0;
		_param->height[i] = 0;

		_param->offset[i] = NULL;

		_param->band.rtband[i] = NULL;
		_param->band.hasnodata[i] = 0;
		_param->band.isnodata[i] = 0;
		_param->band.nodataval[i] = 0;
		_param->band.minval[i] = 0;

		/* set isempty */
		if (itrset[i].raster == NULL) {
			_param->isempty[i] = 1;

			(*allnull)++;
			(*allempty)++;

			continue;
		}
		else if (rt_raster_is_empty(itrset[i].raster)) {
			_param->isempty[i] = 1;

			(*allempty)++;

			continue;
		}

		/* check band number */
		hasband = rt_raster_has_band(itrset[i].raster, itrset[i].nband);
		if (!hasband) {
			if (!itrset[i].nbnodata) {
				rterror("_rti_iterator_arg_populate: Band %d not found for raster %d", itrset[i].nband, i);
				return 0;
			}
			else {
				RASTER_DEBUGF(4, "Band %d not found for raster %d. Using NODATA", itrset[i].nband, i);
			}
		}

		_param->raster[i] = itrset[i].raster;
		if (hasband) {
			_param->band.rtband[i] = rt_raster_get_band(itrset[i].raster, itrset[i].nband);
			if (_param->band.rtband[i] == NULL) {
				rterror("_rti_iterator_arg_populate: Could not get band %d for raster %d", itrset[i].nband, i);
				return 0;
			}

			/* hasnodata */
			_param->band.hasnodata[i] = rt_band_get_hasnodata_flag(_param->band.rtband[i]);

			/* hasnodata = TRUE */
			if (_param->band.hasnodata[i]) {
				/* nodataval */
				rt_band_get_nodata(_param->band.rtband[i], &(_param->band.nodataval[i]));

				/* isnodata */
				_param->band.isnodata[i] = rt_band_get_isnodata_flag(_param->band.rtband[i]);
			}
			/* hasnodata = FALSE */
			else {
				/* minval */
				_param->band.minval[i] = rt_band_get_min_value(_param->band.rtband[i]);
			}
		}

		/* width, height */
		_param->width[i] = rt_raster_get_width(_param->raster[i]);
		_param->height[i] = rt_raster_get_height(_param->raster[i]);

		/* init offset */
		_param->offset[i] = rtalloc(sizeof(double) * 2);
		if (_param->offset[i] == NULL) {
			rterror("_rti_iterator_arg_populate: Could not allocate memory for offsets");
			return 0;
		}
	}

	return 1;
}

static int
_rti_iterator_arg_empty_init(_rti_iterator_arg _param) {
	int x = 0;
	int y = 0;

	_param->empty.values = rtalloc(sizeof(double *) * _param->dimension.rows);
	_param->empty.nodata = rtalloc(sizeof(int *) * _param->dimension.rows);
	if (_param->empty.values == NULL || _param->empty.nodata == NULL) {
		rterror("_rti_iterator_arg_empty_init: Could not allocate memory for empty values and NODATA");
		return 0;
	}

	for (y = 0; y < _param->dimension.rows; y++) {
		_param->empty.values[y] = rtalloc(sizeof(double) * _param->dimension.columns);
		_param->empty.nodata[y] = rtalloc(sizeof(int) * _param->dimension.columns);

		if (_param->empty.values[y] == NULL || _param->empty.nodata[y] == NULL) {
			rterror("_rti_iterator_arg_empty_init: Could not allocate memory for elements of empty values and NODATA");
			return 0;
		}

		for (x = 0; x < _param->dimension.columns; x++) {
			_param->empty.values[y][x] = 0;
			_param->empty.nodata[y][x] = 1;
		}
	}

	return 1;
}

static int
_rti_iterator_arg_callback_init(_rti_iterator_arg _param) {
	int i = 0;

	_param->arg = rtalloc(sizeof(struct rt_iterator_arg_t));
	if (_param->arg == NULL) {
		rterror("_rti_iterator_arg_callback_init: Could not allocate memory for rt_iterator_arg");
		return 0;
	}

	_param->arg->values = NULL;
	_param->arg->nodata = NULL;
	_param->arg->src_pixel = NULL;

	/* initialize argument components */
	_param->arg->values = rtalloc(sizeof(double **) * _param->count);
	_param->arg->nodata = rtalloc(sizeof(int **) * _param->count);
	_param->arg->src_pixel = rtalloc(sizeof(int *) * _param->count);
	if (_param->arg->values == NULL || _param->arg->nodata == NULL || _param->arg->src_pixel == NULL) {
		rterror("_rti_iterator_arg_callback_init: Could not allocate memory for element of rt_iterator_arg");
		return 0;
	}
	memset(_param->arg->values, 0, sizeof(double **) * _param->count);
	memset(_param->arg->nodata, 0, sizeof(int **) * _param->count);

	/* initialize pos */
	for (i = 0; i < _param->count; i++) {

		_param->arg->src_pixel[i] = rtalloc(sizeof(int) * 2);
		if (_param->arg->src_pixel[i] == NULL) {
			rterror("_rti_iterator_arg_callback_init: Could not allocate memory for position elements of rt_iterator_arg");
			return 0;
		}
		memset(_param->arg->src_pixel[i], 0, sizeof(int) * 2);
	}

	_param->arg->rasters = _param->count;
	_param->arg->rows = _param->dimension.rows;
	_param->arg->columns = _param->dimension.columns;

	_param->arg->dst_pixel[0] = 0;
	_param->arg->dst_pixel[1] = 0;

	return 1;
}

static void
_rti_iterator_arg_callback_clean(_rti_iterator_arg _param) {
	int i = 0;
	int y = 0;

	for (i = 0; i < _param->count; i++) {
		RASTER_DEBUGF(5, "empty at @ %p", _param->empty.values);
		RASTER_DEBUGF(5, "values at @ %p", _param->arg->values[i]);

		if (_param->arg->values[i] == _param->empty.values) {
			_param->arg->values[i] = NULL;
			_param->arg->nodata[i] = NULL;

			continue;
		}

		for (y = 0; y < _param->dimension.rows; y++) {
			rtdealloc(_param->arg->values[i][y]);
			rtdealloc(_param->arg->nodata[i][y]);
		}

		rtdealloc(_param->arg->values[i]);
		rtdealloc(_param->arg->nodata[i]);

		_param->arg->values[i] = NULL;
		_param->arg->nodata[i] = NULL;
	}
}

/**
 * n-raster iterator.
 * The raster returned should be freed by the caller
 *
 * @param itrset : set of rt_iterator objects.
 * @param itrcount : number of objects in itrset.
 * @param extenttype : type of extent for the output raster.
 * @param customextent : raster specifying custom extent.
 * is only used if extenttype is ET_CUSTOM.
 * @param pixtype : the desired pixel type of the output raster's band.
 * @param hasnodata : indicates if the band has nodata value
 * @param nodataval : the nodata value, will be appropriately
 * truncated to fit the pixtype size.
 * @param distancex : the number of pixels around the specified pixel
 * along the X axis
 * @param distancey : the number of pixels around the specified pixel
 * along the Y axis
 * @param userarg : pointer to any argument that is passed as-is to callback.
 * @param callback : callback function for actual processing of pixel values.
 * @param *rtnraster : return one band raster from iterator process
 *
 * The callback function _must_ have the following signature.
 *
 *    int FNAME(rt_iterator_arg arg, void *userarg, double *value, int *nodata)
 *
 * The callback function _must_ return zero (error) or non-zero (success)
 * indicating whether the function ran successfully.
 * The parameters passed to the callback function are as follows.
 *
 * - rt_iterator_arg arg: struct containing pixel values, NODATA flags and metadata
 * - void *userarg: NULL or calling function provides to rt_raster_iterator() for use by callback function
 * - double *value: value of pixel to be burned by rt_raster_iterator()
 * - int *nodata: flag (0 or 1) indicating that pixel to be burned is NODATA
 *
 * @return ES_NONE on success, ES_ERROR on error
 */
rt_errorstate
rt_raster_iterator(
	rt_iterator itrset, uint16_t itrcount,
	rt_extenttype extenttype, rt_raster customextent,
	rt_pixtype pixtype,
	uint8_t hasnodata, double nodataval,
	uint16_t distancex, uint16_t distancey,
	void *userarg,
	int (*callback)(
		rt_iterator_arg arg,
		void *userarg,
		double *value,
		int *nodata
	),
	rt_raster *rtnraster
) {
	/* output raster */
	rt_raster rtnrast = NULL;
	/* output raster's band */
	rt_band rtnband = NULL;

	/* working raster */
	rt_raster rast = NULL;

	_rti_iterator_arg _param = NULL;
	int allnull = 0;
	int allempty = 0;
	int aligned = 0;
	double offset[4] = {0.};
	rt_pixel npixels;

	int i = 0;
	int status = 0;
	int inextent = 0;
	int x = 0;
	int y = 0;
	int _x = 0;
	int _y = 0;

	int _width = 0;
	int _height = 0;

	double minval;
	double value;
	int isnodata;
	int nodata;

	RASTER_DEBUG(3, "Starting...");

	assert(itrset != NULL && itrcount > 0);
	assert(rtnraster != NULL);

	/* init rtnraster to NULL */
	*rtnraster = NULL;

	/* check that callback function is not NULL */
	if (callback == NULL) {
		rterror("rt_raster_iterator: Callback function not provided");
		return ES_ERROR;
	}

	/* check that custom extent is provided if extenttype = ET_CUSTOM */
	if (extenttype == ET_CUSTOM && rt_raster_is_empty(customextent)) {
		rterror("rt_raster_iterator: Custom extent cannot be empty if extent type is ET_CUSTOM");
		return ES_ERROR;
	}

	/* check that pixtype != PT_END */
	if (pixtype == PT_END) {
		rterror("rt_raster_iterator: Pixel type cannot be PT_END");
		return ES_ERROR;
	}

	/* initialize _param */
	if ((_param = _rti_iterator_arg_init()) == NULL) {
		rterror("rt_raster_iterator: Could not initialize internal variables");
		return ES_ERROR;
	}

	/* fill _param */
	if (!_rti_iterator_arg_populate(_param, itrset, itrcount, distancex, distancey, &allnull, &allempty)) {
		rterror("rt_raster_iterator: Could not populate for internal variables");
		_rti_iterator_arg_destroy(_param);
		return ES_ERROR;
	}

	/* shortcut if all null, return NULL */
	if (allnull == itrcount) {
		RASTER_DEBUG(3, "all rasters are NULL, returning NULL");

		_rti_iterator_arg_destroy(_param);

		return ES_NONE;
	}
	/* shortcut if all empty, return empty raster */
	else if (allempty == itrcount) {
		RASTER_DEBUG(3, "all rasters are empty, returning empty raster");

		_rti_iterator_arg_destroy(_param);

		rtnrast = rt_raster_new(0, 0);
		if (rtnrast == NULL) {
			rterror("rt_raster_iterator: Could not create empty raster");
			return ES_ERROR;
		}
		rt_raster_set_scale(rtnrast, 0, 0);

		*rtnraster = rtnrast;
		return ES_NONE;
	}

	/* check that all rasters are aligned */
	RASTER_DEBUG(3, "checking alignment of all rasters");
	rast = NULL;

	/* find raster to use as reference */
	/* use custom if provided */
	if (extenttype == ET_CUSTOM) {
		RASTER_DEBUG(4, "using custom extent as reference raster");
		rast = customextent;
	}
	/* use first valid one in _param->raster */
	else {
		for (i = 0; i < itrcount; i++) {
			if (!_param->isempty[i]) {
				RASTER_DEBUGF(4, "using raster at index %d as reference raster", i);
				rast = _param->raster[i];
				break;
			}
		}
	}

	/* no rasters found, SHOULD NEVER BE HERE! */
	if (rast == NULL) {
		rterror("rt_raster_iterator: Could not find reference raster to use for alignment tests");

		_rti_iterator_arg_destroy(_param);

		return ES_ERROR;
	}

	do {
		aligned = 1;

		/* check custom first if set. also skip if rasters are the same */
		if (extenttype == ET_CUSTOM && rast != customextent) {
			if (rt_raster_same_alignment(rast, customextent, &aligned, NULL) != ES_NONE) {
				rterror("rt_raster_iterator: Could not test for alignment between reference raster and custom extent");

				_rti_iterator_arg_destroy(_param);

				return ES_ERROR;
			}

			RASTER_DEBUGF(5, "custom extent alignment: %d", aligned);
			if (!aligned)
				break;
		}

		for (i = 0; i < itrcount; i++) {
			/* skip NULL rasters and if rasters are the same */
			if (_param->isempty[i] || rast == _param->raster[i])
				continue;

			if (rt_raster_same_alignment(rast, _param->raster[i], &aligned, NULL) != ES_NONE) {
				rterror("rt_raster_iterator: Could not test for alignment between reference raster and raster %d", i);

				_rti_iterator_arg_destroy(_param);

				return ES_ERROR;
			}
			RASTER_DEBUGF(5, "raster at index %d alignment: %d", i, aligned);

			/* abort checking since a raster isn't aligned */
			if (!aligned)
				break;
		}
	}
	while (0);

	/* not aligned, error */
	if (!aligned) {
		rterror("rt_raster_iterator: The set of rasters provided (custom extent included, if appropriate) do not have the same alignment");

		_rti_iterator_arg_destroy(_param);

		return ES_ERROR;
	}

	/* use extenttype to build output raster (no bands though) */
	i = -1;
	switch (extenttype) {
		case ET_INTERSECTION:
		case ET_UNION:
			/* make copy of first "real" raster */
			rtnrast = rtalloc(sizeof(struct rt_raster_t));
			if (rtnrast == NULL) {
				rterror("rt_raster_iterator: Could not allocate memory for output raster");

				_rti_iterator_arg_destroy(_param);

				return ES_ERROR;
			}

			for (i = 0; i < itrcount; i++) {
				if (!_param->isempty[i]) {
					memcpy(rtnrast, _param->raster[i], sizeof(struct rt_raster_serialized_t));
					break;
				}
			}
			rtnrast->numBands = 0;
			rtnrast->bands = NULL;

			/* get extent of output raster */
			rast = NULL;
			for (i = i + 1; i < itrcount; i++) {
				if (_param->isempty[i])
					continue;

				status = rt_raster_from_two_rasters(rtnrast, _param->raster[i], extenttype, &rast, NULL);
				rtdealloc(rtnrast);

				if (rast == NULL || status != ES_NONE) {
					rterror("rt_raster_iterator: Could not compute %s extent of rasters",
						extenttype == ET_UNION ? "union" : "intersection"
					);

					_rti_iterator_arg_destroy(_param);

					return ES_ERROR;
				}
				else if (rt_raster_is_empty(rast)) {
					rtinfo("rt_raster_iterator: Computed raster for %s extent is empty",
						extenttype == ET_UNION ? "union" : "intersection"
					);

					_rti_iterator_arg_destroy(_param);

					*rtnraster = rast;
					return ES_NONE;
				}

				rtnrast = rast;
				rast = NULL;
			}

			break;
		/*
			first, second and last have similar checks
			and continue into custom
		*/
		case ET_FIRST:
			i = 0;
		case ET_SECOND:
			if (i < 0) {
				if (itrcount < 2)
					i = 0;
				else
					i = 1;
			}
		case ET_LAST:
			if (i < 0) i = itrcount - 1;
			
			/* input raster is null, return NULL */
			if (_param->raster[i] == NULL) {
				RASTER_DEBUGF(3, "returning NULL as %s raster is NULL and extent type is ET_%s",
					(i == 0 ? "first" : (i == 1 ? "second" : "last")),
					(i == 0 ? "FIRST" : (i == 1 ? "SECOND" : "LAST"))
				);

				_rti_iterator_arg_destroy(_param);

				return ES_NONE;
			}
			/* input raster is empty, return empty raster */
			else if (_param->isempty[i]) {
				RASTER_DEBUGF(3, "returning empty raster as %s raster is empty and extent type is ET_%s",
					(i == 0 ? "first" : (i == 1 ? "second" : "last")),
					(i == 0 ? "FIRST" : (i == 1 ? "SECOND" : "LAST"))
				);

				_rti_iterator_arg_destroy(_param);

				rtnrast = rt_raster_new(0, 0);
				if (rtnrast == NULL) {
					rterror("rt_raster_iterator: Could not create empty raster");
					return ES_ERROR;
				}
				rt_raster_set_scale(rtnrast, 0, 0);

				*rtnraster = rtnrast;
				return ES_NONE;
			}
		/* copy the custom extent raster */
		case ET_CUSTOM:
			rtnrast = rtalloc(sizeof(struct rt_raster_t));
			if (rtnrast == NULL) {
				rterror("rt_raster_iterator: Could not allocate memory for output raster");

				_rti_iterator_arg_destroy(_param);

				return ES_ERROR;
			}

			switch (extenttype) {
				case ET_CUSTOM:
					memcpy(rtnrast, customextent, sizeof(struct rt_raster_serialized_t));
					break;
				/* first, second, last */
				default:
					memcpy(rtnrast, _param->raster[i], sizeof(struct rt_raster_serialized_t));
					break;
			}
			rtnrast->numBands = 0;
			rtnrast->bands = NULL;
			break;
	}

	_width = rt_raster_get_width(rtnrast);
	_height = rt_raster_get_height(rtnrast);

	RASTER_DEBUGF(4, "rtnrast (width, height, ulx, uly, scalex, scaley, skewx, skewy, srid) = (%d, %d, %f, %f, %f, %f, %f, %f, %d)",
		_width,
		_height,
		rt_raster_get_x_offset(rtnrast),
		rt_raster_get_y_offset(rtnrast),
		rt_raster_get_x_scale(rtnrast),
		rt_raster_get_y_scale(rtnrast),
		rt_raster_get_x_skew(rtnrast),
		rt_raster_get_y_skew(rtnrast),
		rt_raster_get_srid(rtnrast)
	);

	/* init values and NODATA for use with empty rasters */
	if (!_rti_iterator_arg_empty_init(_param)) {
		rterror("rt_raster_iterator: Could not initialize empty values and NODATA");

		_rti_iterator_arg_destroy(_param);
		rt_raster_destroy(rtnrast);
		
		return ES_ERROR;
	}

	/* create output band */
	if (rt_raster_generate_new_band(
		rtnrast,
		pixtype,
		nodataval,
		hasnodata, nodataval,
		0
	) < 0) {
		rterror("rt_raster_iterator: Could not add new band to output raster");

		_rti_iterator_arg_destroy(_param);
		rt_raster_destroy(rtnrast);

		return ES_ERROR;
	}

	/* get output band */
	rtnband = rt_raster_get_band(rtnrast, 0);
	if (rtnband == NULL) {
		rterror("rt_raster_iterator: Could not get new band from output raster");

		_rti_iterator_arg_destroy(_param);
		rt_raster_destroy(rtnrast);

		return ES_ERROR;
	}

	/* output band's minimum value */
	minval = rt_band_get_min_value(rtnband);

	/* initialize argument for callback function */
	if (!_rti_iterator_arg_callback_init(_param)) {
		rterror("rt_raster_iterator: Could not initialize callback function argument");

		_rti_iterator_arg_destroy(_param);
		rt_band_destroy(rtnband);
		rt_raster_destroy(rtnrast);

		return ES_ERROR;
	}

	/* fill _param->offset */
	for (i = 0; i < itrcount; i++) {
		if (_param->isempty[i])
			continue;

		status = rt_raster_from_two_rasters(rtnrast, _param->raster[i], ET_FIRST, &rast, offset);
		rtdealloc(rast);
		if (status != ES_NONE) {
			rterror("rt_raster_iterator: Could not compute raster offsets");

			_rti_iterator_arg_destroy(_param);
			rt_band_destroy(rtnband);
			rt_raster_destroy(rtnrast);

			return ES_ERROR;
		}

		_param->offset[i][0] = offset[2];
		_param->offset[i][1] = offset[3];
		RASTER_DEBUGF(4, "rast %d offset: %f %f", i, offset[2], offset[3]);
	}

	/* loop over each pixel (POI) of output raster */
	/* _x,_y are for output raster */
	/* x,y are for input raster */
	for (_y = 0; _y < _height; _y++) {
		for (_x = 0; _x < _width; _x++) {
			RASTER_DEBUGF(4, "iterating output pixel (x, y) = (%d, %d)", _x, _y);
			_param->arg->dst_pixel[0] = _x;
			_param->arg->dst_pixel[1] = _y;

			/* loop through each input raster */
			for (i = 0; i < itrcount; i++) {
				RASTER_DEBUGF(4, "raster %d", i);

				/*
					empty raster
					OR band does not exist and flag set to use NODATA
					OR band is NODATA
				*/
				if (
					_param->isempty[i] ||
					(_param->band.rtband[i] == NULL && itrset[i].nbnodata) ||
					_param->band.isnodata[i]
				) {
					RASTER_DEBUG(4, "empty raster, band does not exist or band is NODATA. using empty values and NODATA");
					
					x = _x;
					y = _y;

					_param->arg->values[i] = _param->empty.values;
					_param->arg->nodata[i] = _param->empty.nodata;

					continue;
				}

				/* input raster's X,Y */
				x = _x - (int) _param->offset[i][0];
				y = _y - (int) _param->offset[i][1];
				RASTER_DEBUGF(4, "source pixel (x, y) = (%d, %d)", x, y);

				_param->arg->src_pixel[i][0] = x;
				_param->arg->src_pixel[i][1] = y;

				/* neighborhood */
				npixels = NULL;
				status = 0;
				if (distancex > 0 && distancey > 0) {
					RASTER_DEBUG(4, "getting neighborhood");

					status = rt_band_get_nearest_pixel(
						_param->band.rtband[i],
						x, y,
						distancex, distancey,
						1,
						&npixels
					);
					if (status < 0) {
						rterror("rt_raster_iterator: Could not get pixel neighborhood");

						_rti_iterator_arg_destroy(_param);
						rt_band_destroy(rtnband);
						rt_raster_destroy(rtnrast);

						return ES_ERROR;
					}
				}

				/* get value of POI */
				/* get pixel's value */
				if (
					(x >= 0 && x < _param->width[i]) &&
					(y >= 0 && y < _param->height[i])
				) {
					RASTER_DEBUG(4, "getting value of POI");
					if (rt_band_get_pixel(
						_param->band.rtband[i],
						x, y,
						&value,
						&isnodata
					) != ES_NONE) {
						rterror("rt_raster_iterator: Could not get the pixel value of band");

						_rti_iterator_arg_destroy(_param);
						rt_band_destroy(rtnband);
						rt_raster_destroy(rtnrast);

						return ES_ERROR;
					}
					inextent = 1;
				}
				/* outside band extent, set to NODATA */
				else {
					RASTER_DEBUG(4, "Outside band extent, setting value to NODATA");
					/* has NODATA, use NODATA */
					if (_param->band.hasnodata[i])
						value = _param->band.nodataval[i];
					/* no NODATA, use min possible value */
					else
						value = _param->band.minval[i];

					inextent = 0;
					isnodata = 1;
				}

				/* add pixel to neighborhood */
				status++;
				if (status > 1)
					npixels = (rt_pixel) rtrealloc(npixels, sizeof(struct rt_pixel_t) * status);
				else
					npixels = (rt_pixel) rtalloc(sizeof(struct rt_pixel_t));

				if (npixels == NULL) {
					rterror("rt_raster_iterator: Could not reallocate memory for neighborhood");

					_rti_iterator_arg_destroy(_param);
					rt_band_destroy(rtnband);
					rt_raster_destroy(rtnrast);

					return ES_ERROR;
				}

				npixels[status - 1].x = x;
				npixels[status - 1].y = y;
				npixels[status - 1].nodata = 1;
				npixels[status - 1].value = value;

				/* set nodata flag */
				if ((!_param->band.hasnodata[i] && inextent) || !isnodata) {
					npixels[status - 1].nodata = 0;
				}
				RASTER_DEBUGF(4, "value, nodata: %f, %d", value, npixels[status - 1].nodata);

				/* convert set of rt_pixel to 2D array */
				status = rt_pixel_set_to_array(
					npixels, status,
					x, y,
					distancex, distancey,
					&(_param->arg->values[i]),
					&(_param->arg->nodata[i]),
					NULL, NULL
				);
				rtdealloc(npixels);
				if (status != ES_NONE) {
					rterror("rt_raster_iterator: Could not create 2D array of neighborhood");

					_rti_iterator_arg_destroy(_param);
					rt_band_destroy(rtnband);
					rt_raster_destroy(rtnrast);

					return ES_ERROR;
				}
			}
	
			/* callback */
			RASTER_DEBUG(4, "calling callback function");
			value = 0;
			nodata = 0;
			status = callback(_param->arg, userarg, &value, &nodata);

			/* free memory from callback */
			_rti_iterator_arg_callback_clean(_param);

			/* handle callback status */
			if (status == 0) {
				rterror("rt_raster_iterator: Callback function returned an error");

				_rti_iterator_arg_destroy(_param);
				rt_band_destroy(rtnband);
				rt_raster_destroy(rtnrast);

				return ES_ERROR;
			}

			/* burn value to pixel */
			status = 0;
			if (!nodata) {
				status = rt_band_set_pixel(rtnband, _x, _y, value, NULL);
				RASTER_DEBUGF(4, "burning pixel (%d, %d) with value: %f", _x, _y, value);
			}
			else if (!hasnodata) {
				status = rt_band_set_pixel(rtnband, _x, _y, minval, NULL);
				RASTER_DEBUGF(4, "burning pixel (%d, %d) with minval: %f", _x, _y, minval);
			}
			else {
				RASTER_DEBUGF(4, "NOT burning pixel (%d, %d)", _x, _y);
			}
			if (status != ES_NONE) {
				rterror("rt_raster_iterator: Could not set pixel value");

				_rti_iterator_arg_destroy(_param);
				rt_band_destroy(rtnband);
				rt_raster_destroy(rtnrast);

				return ES_ERROR;
			}
		}
	}

	/* lots of cleanup */
	_rti_iterator_arg_destroy(_param);

	*rtnraster = rtnrast;
	return ES_NONE;
}

/******************************************************************************
* rt_raster_perimeter()
******************************************************************************/
static rt_errorstate
_rti_raster_get_band_perimeter(rt_band band, uint16_t *trim) {
	uint16_t width = 0;
	uint16_t height = 0;
	int x = 0;
	int y = 0;
	int offset = 0;
	int done[4] = {0};
	double value = 0;
	int nodata = 0;

	assert(band != NULL);
	assert(band->raster != NULL);
	assert(trim != NULL);

	memset(trim, 0, sizeof(uint16_t) * 4);

	width = rt_band_get_width(band);
	height = rt_band_get_height(band);
		
	/* top */
	for (y = 0; y < height; y++) {
		for (offset = 0; offset < 3; offset++) {
			/* every third pixel */
			for (x = offset; x < width; x += 3) {
				if (rt_band_get_pixel(band, x, y, &value, &nodata) != ES_NONE) {
					rterror("_rti_raster_get_band_perimeter: Could not get band pixel");
					return ES_ERROR;
				}

				RASTER_DEBUGF(4, "top (x, y, value, nodata) = (%d, %d, %f, %d)", x, y, value, nodata);
				if (!nodata) {
					trim[0] = y;
					done[0] = 1;
					break;
				}
			}

			if (done[0])
				break;
		}

		if (done[0])
			break;
	}

	/* right */
	for (x = width - 1; x >= 0; x--) {
		for (offset = 0; offset < 3; offset++) {
			/* every third pixel */
			for (y = offset; y < height; y += 3) {
				if (rt_band_get_pixel(band, x, y, &value, &nodata) != ES_NONE) {
					rterror("_rti_raster_get_band_perimeter: Could not get band pixel");
					return ES_ERROR;
				}

				RASTER_DEBUGF(4, "right (x, y, value, nodata) = (%d, %d, %f, %d)", x, y, value, nodata);
				if (!nodata) {
					trim[1] = width - (x + 1);
					done[1] = 1;
					break;
				}
			}

			if (done[1])
				break;
		}

		if (done[1])
			break;
	}

	/* bottom */
	for (y = height - 1; y >= 0; y--) {
		for (offset = 0; offset < 3; offset++) {
			/* every third pixel */
			for (x = offset; x < width; x += 3) {
				if (rt_band_get_pixel(band, x, y, &value, &nodata) != ES_NONE) {
					rterror("_rti_raster_get_band_perimeter: Could not get band pixel");
					return ES_ERROR;
				}

				RASTER_DEBUGF(4, "bottom (x, y, value, nodata) = (%d, %d, %f, %d)", x, y, value, nodata);
				if (!nodata) {
					trim[2] = height - (y + 1);
					done[2] = 1;
					break;
				}
			}

			if (done[2])
				break;
		}

		if (done[2])
			break;
	}

	/* left */
	for (x = 0; x < width; x++) {
		for (offset = 0; offset < 3; offset++) {
			/* every third pixel */
			for (y = offset; y < height; y += 3) {
				if (rt_band_get_pixel(band, x, y, &value, &nodata) != ES_NONE) {
					rterror("_rti_raster_get_band_perimeter: Could not get band pixel");
					return ES_ERROR;
				}

				RASTER_DEBUGF(4, "left (x, , value, nodata) = (%d, %d, %f, %d)", x, y, value, nodata);
				if (!nodata) {
					trim[3] = x;
					done[3] = 1;
					break;
				}
			}

			if (done[3])
				break;
		}

		if (done[3])
			break;
	}

	RASTER_DEBUGF(4, "trim = (%d, %d, %d, %d)",
		trim[0], trim[1], trim[2], trim[3]);

	return ES_NONE;
}

/**
 * Get raster perimeter
 *
 * The perimeter is a 4 vertices (5 to be closed)
 * single ring polygon bearing the raster's rotation and using
 * projection coordinates.
 *
 * @param raster : the raster to get info from
 * @param nband : the band for the perimeter. 0-based
 * value less than zero means all bands
 * @param **perimeter : pointer to perimeter
 *
 * @return ES_NONE if success, ES_ERROR if error
 */
rt_errorstate rt_raster_get_perimeter(
	rt_raster raster, int nband,
	LWGEOM **perimeter
) {
	rt_band band = NULL;
	int numband = 0;
	uint16_t *_nband = NULL;
	int i = 0;
	int j = 0;
	uint16_t _trim[4] = {0};
	uint16_t trim[4] = {0}; /* top, right, bottom, left */
	int isset[4] = {0};
	double gt[6] = {0.0};
	int srid = SRID_UNKNOWN;

	POINTARRAY *pts = NULL;
	POINT4D p4d;
	POINTARRAY **rings = NULL;
	LWPOLY* poly = NULL;

	assert(perimeter != NULL);

	*perimeter = NULL;

	/* empty raster, no perimeter */
	if (rt_raster_is_empty(raster))
		return ES_NONE;

	/* raster metadata */
	srid = rt_raster_get_srid(raster);
	rt_raster_get_geotransform_matrix(raster, gt);
	numband = rt_raster_get_num_bands(raster);

	RASTER_DEBUGF(3, "rt_raster_get_perimeter: raster is %dx%d", raster->width, raster->height); 

	/* nband < 0 means all bands */
	if (nband >= 0) {
		if (nband >= numband) {
			rterror("rt_raster_get_boundary: Band %d not found for raster", nband);
			return ES_ERROR;
		}

		numband = 1;
	}
	else
		nband = -1;
	
	RASTER_DEBUGF(3, "rt_raster_get_perimeter: nband, numband = %d, %d", nband, numband); 

	_nband = rtalloc(sizeof(uint16_t) * numband);
	if (_nband == NULL) {
		rterror("rt_raster_get_boundary: Could not allocate memory for band indices");
		return ES_ERROR;
	}

	if (nband < 0) {
		for (i = 0; i < numband; i++)
			_nband[i] = i;
	}
	else
		_nband[0] = nband;

	for (i = 0; i < numband; i++) {
		band = rt_raster_get_band(raster, _nband[i]);
		if (band == NULL) {
			rterror("rt_raster_get_boundary: Could not get band at index %d", _nband[i]);
			rtdealloc(_nband);
			return ES_ERROR;
		}

		/* band is nodata */
		if (rt_band_get_isnodata_flag(band) != 0)
			continue;

		if (_rti_raster_get_band_perimeter(band, trim) != ES_NONE) {
			rterror("rt_raster_get_boundary: Could not get band perimeter");
			rtdealloc(_nband);
			return ES_ERROR;
		}

		for (j = 0; j < 4; j++) {
			if (!isset[j] || trim[j] < _trim[j]) {
				_trim[j] = trim[j];
				isset[j] = 1;
			}
		}
	}

	/* no longer needed */
	rtdealloc(_nband);

	/* check isset, just need to check one element */
	if (!isset[0]) {
		/* return NULL as bands are empty */
		return ES_NONE;
	}

	RASTER_DEBUGF(4, "trim = (%d, %d, %d, %d)",
		trim[0], trim[1], trim[2], trim[3]);

	/* only one ring */
	rings = (POINTARRAY **) rtalloc(sizeof (POINTARRAY*));
	if (!rings) {
		rterror("rt_raster_get_perimeter: Could not allocate memory for polygon ring");
		return ES_ERROR;
	}
	rings[0] = ptarray_construct(0, 0, 5);
	if (!rings[0]) {
		rterror("rt_raster_get_perimeter: Could not construct point array");
		return ES_ERROR;
	}
	pts = rings[0];

	/* Upper-left corner (first and last points) */
	rt_raster_cell_to_geopoint(
		raster,
		_trim[3], _trim[0],
		&p4d.x, &p4d.y,
		gt
	);
	ptarray_set_point4d(pts, 0, &p4d);
	ptarray_set_point4d(pts, 4, &p4d);

	/* Upper-right corner (we go clockwise) */
	rt_raster_cell_to_geopoint(
		raster,
		raster->width - _trim[1], _trim[0],
		&p4d.x, &p4d.y,
		gt
	);
	ptarray_set_point4d(pts, 1, &p4d);

	/* Lower-right corner */
	rt_raster_cell_to_geopoint(
		raster,
		raster->width - _trim[1], raster->height - _trim[2],
		&p4d.x, &p4d.y,
		gt
	);
	ptarray_set_point4d(pts, 2, &p4d);

	/* Lower-left corner */
	rt_raster_cell_to_geopoint(
		raster,
		_trim[3], raster->height - _trim[2],
		&p4d.x, &p4d.y,
		gt
	);
	ptarray_set_point4d(pts, 3, &p4d);

	poly = lwpoly_construct(srid, 0, 1, rings);
	*perimeter = lwpoly_as_lwgeom(poly);

	return ES_NONE;
}

/******************************************************************************
* rt_raster_colormap()
******************************************************************************/

typedef struct _rti_colormap_arg_t* _rti_colormap_arg;
struct _rti_colormap_arg_t {
	rt_raster raster;
	rt_band band;

	rt_colormap_entry nodataentry;
	int hasnodata;
	double nodataval;

	int nexpr;
	rt_reclassexpr *expr;

	int npos;
	uint16_t *pos;

};

static _rti_colormap_arg
_rti_colormap_arg_init(rt_raster raster) {
	_rti_colormap_arg arg = NULL;

	arg = rtalloc(sizeof(struct _rti_colormap_arg_t));
	if (arg == NULL) {
		rterror("_rti_colormap_arg_init: Could not allocate memory for _rti_color_arg");
		return NULL;
	}

	arg->band = NULL;
	arg->nodataentry = NULL;
	arg->hasnodata = 0;
	arg->nodataval = 0;

	if (raster == NULL)
		arg->raster = NULL;
	/* raster provided */
	else {
		arg->raster = rt_raster_clone(raster, 0);
		if (arg->raster == NULL) {
			rterror("_rti_colormap_arg_init: Could not create output raster");
			return NULL;
		}
	}

	arg->nexpr = 0;
	arg->expr = NULL;

	arg->npos = 0;
	arg->pos = NULL;

	return arg;
}

static void
_rti_colormap_arg_destroy(_rti_colormap_arg arg) {
	int i = 0;

	if (arg->raster != NULL) {
		rt_band band = NULL;

		for (i = rt_raster_get_num_bands(arg->raster) - 1; i >= 0; i--) {
			band = rt_raster_get_band(arg->raster, i);
			if (band != NULL)
				rt_band_destroy(band);
		}

		rt_raster_destroy(arg->raster);
	}

	if (arg->nexpr) {
		for (i = 0; i < arg->nexpr; i++) {
			if (arg->expr[i] != NULL)
				rtdealloc(arg->expr[i]);
		}
		rtdealloc(arg->expr);
	}

	if (arg->npos)
		rtdealloc(arg->pos);

	rtdealloc(arg);
	arg = NULL;
}

/**
 * Returns a new raster with up to four 8BUI bands (RGBA) from
 * applying a colormap to the user-specified band of the
 * input raster.
 *
 * @param raster: input raster
 * @param nband: 0-based index of the band to process with colormap
 * @param colormap: rt_colormap object of colormap to apply to band
 *
 * @return new raster or NULL on error
 */
rt_raster rt_raster_colormap(
	rt_raster raster, int nband,
	rt_colormap colormap
) {
	_rti_colormap_arg arg = NULL;
	rt_raster rtnraster = NULL;
	rt_band band = NULL;
	int i = 0;
	int j = 0;
	int k = 0;

	assert(colormap != NULL);

	/* empty raster */
	if (rt_raster_is_empty(raster))
		return NULL;

	/* no colormap entries */
	if (colormap->nentry < 1) {
		rterror("rt_raster_colormap: colormap must have at least one entry");
		return NULL;
	}

	/* nband is valid */
	if (!rt_raster_has_band(raster, nband)) {
		rterror("rt_raster_colormap: raster has no band at index %d", nband);
		return NULL;
	}

	band = rt_raster_get_band(raster, nband);
	if (band == NULL) {
		rterror("rt_raster_colormap: Could not get band at index %d", nband);
		return NULL;
	}

	/* init internal variables */
	arg = _rti_colormap_arg_init(raster);
	if (arg == NULL) {
		rterror("rt_raster_colormap: Could not initialize internal variables");
		return NULL;
	}

	/* handle NODATA */
	if (rt_band_get_hasnodata_flag(band)) {
		arg->hasnodata = 1;
		rt_band_get_nodata(band, &(arg->nodataval));
	}

	/* # of colors */
	if (colormap->ncolor < 1) {
		rterror("rt_raster_colormap: At least one color must be provided");
		_rti_colormap_arg_destroy(arg);
		return NULL;
	}
	else if (colormap->ncolor > 4) {
		rtinfo("More than four colors indicated. Using only the first four colors");
		colormap->ncolor = 4;
	}

	/* find non-NODATA entries */
	arg->npos = 0;
	arg->pos = rtalloc(sizeof(uint16_t) * colormap->nentry);
	if (arg->pos == NULL) {
		rterror("rt_raster_colormap: Could not allocate memory for valid entries");
		_rti_colormap_arg_destroy(arg);
		return NULL;
	}
	for (i = 0, j = 0; i < colormap->nentry; i++) {
		/* special handling for NODATA entries */
		if (colormap->entry[i].isnodata) {
			/* first NODATA entry found, use it */
			if (arg->nodataentry == NULL)
				arg->nodataentry = &(colormap->entry[i]);
			else
				rtwarn("More than one colormap entry found for NODATA value. Only using first NOTDATA entry");

			continue;
		}

		(arg->npos)++;
		arg->pos[j++] = i;
	}

	/* INTERPOLATE and only one non-NODATA entry */
	if (colormap->method == CM_INTERPOLATE && arg->npos < 2) {
		rtinfo("Method INTERPOLATE requires at least two non-NODATA colormap entries. Using NEAREST instead");
		colormap->method = CM_NEAREST;
	}

	/* NODATA entry but band has no NODATA value */
	if (!arg->hasnodata && arg->nodataentry != NULL) {
		rtinfo("Band at index %d has no NODATA value. Ignoring NODATA entry", nband);
		arg->nodataentry = NULL;
	}

	/* allocate expr */
	arg->nexpr = arg->npos;

	/* INTERPOLATE needs one less than the number of entries */
	if (colormap->method == CM_INTERPOLATE)
		arg->nexpr -= 1;
	/* EXACT requires a no matching expression */
	else if (colormap->method == CM_EXACT)
		arg->nexpr += 1;

	/* NODATA entry exists, add expression */
	if (arg->nodataentry != NULL)
		arg->nexpr += 1;
	arg->expr = rtalloc(sizeof(rt_reclassexpr) * arg->nexpr);
	if (arg->expr == NULL) {
		rterror("rt_raster_colormap: Could not allocate memory for reclass expressions");
		_rti_colormap_arg_destroy(arg);
		return NULL;
	}
	RASTER_DEBUGF(4, "nexpr = %d", arg->nexpr);
	RASTER_DEBUGF(4, "expr @ %p", arg->expr);

	for (i = 0; i < arg->nexpr; i++) {
		arg->expr[i] = rtalloc(sizeof(struct rt_reclassexpr_t));
		if (arg->expr[i] == NULL) {
			rterror("rt_raster_colormap: Could not allocate memory for reclass expression");
			_rti_colormap_arg_destroy(arg);
			return NULL;
		}
	}

	/* reclassify bands */
	/* by # of colors */
	for (i = 0; i < colormap->ncolor; i++) {
		k = 0;

		/* handle NODATA entry first */
		if (arg->nodataentry != NULL) {
			arg->expr[k]->src.min = arg->nodataentry->value;
			arg->expr[k]->src.max = arg->nodataentry->value;
			arg->expr[k]->src.inc_min = 1;
			arg->expr[k]->src.inc_max = 1;
			arg->expr[k]->src.exc_min = 0;
			arg->expr[k]->src.exc_max = 0;

			arg->expr[k]->dst.min = arg->nodataentry->color[i];
			arg->expr[k]->dst.max = arg->nodataentry->color[i];

			arg->expr[k]->dst.inc_min = 1;
			arg->expr[k]->dst.inc_max = 1;
			arg->expr[k]->dst.exc_min = 0;
			arg->expr[k]->dst.exc_max = 0;

			RASTER_DEBUGF(4, "NODATA expr[%d]->src (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->src.min,
				arg->expr[k]->src.max,
				arg->expr[k]->src.inc_min,
				arg->expr[k]->src.inc_max,
				arg->expr[k]->src.exc_min,
				arg->expr[k]->src.exc_max
			);
			RASTER_DEBUGF(4, "NODATA expr[%d]->dst (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->dst.min,
				arg->expr[k]->dst.max,
				arg->expr[k]->dst.inc_min,
				arg->expr[k]->dst.inc_max,
				arg->expr[k]->dst.exc_min,
				arg->expr[k]->dst.exc_max
			);

			k++;
		}

		/* by non-NODATA entry */
		for (j = 0; j < arg->npos; j++) {
			if (colormap->method == CM_INTERPOLATE) {
				if (j == arg->npos - 1)
					continue;

				arg->expr[k]->src.min = colormap->entry[arg->pos[j + 1]].value;
				arg->expr[k]->src.inc_min = 1;
				arg->expr[k]->src.exc_min = 0;

				arg->expr[k]->src.max = colormap->entry[arg->pos[j]].value;
				arg->expr[k]->src.inc_max = 1;
				arg->expr[k]->src.exc_max = 0;

				arg->expr[k]->dst.min = colormap->entry[arg->pos[j + 1]].color[i];
				arg->expr[k]->dst.max = colormap->entry[arg->pos[j]].color[i];

				arg->expr[k]->dst.inc_min = 1;
				arg->expr[k]->dst.exc_min = 0;

				arg->expr[k]->dst.inc_max = 1;
				arg->expr[k]->dst.exc_max = 0;
			}
			else if (colormap->method == CM_NEAREST) {

				/* NOT last entry */
				if (j != arg->npos - 1) {
					arg->expr[k]->src.min = ((colormap->entry[arg->pos[j]].value - colormap->entry[arg->pos[j + 1]].value) / 2.) + colormap->entry[arg->pos[j + 1]].value;
					arg->expr[k]->src.inc_min = 0;
					arg->expr[k]->src.exc_min = 0;
				}
				/* last entry */
				else {
					arg->expr[k]->src.min = colormap->entry[arg->pos[j]].value;
					arg->expr[k]->src.inc_min = 1;
					arg->expr[k]->src.exc_min = 1;
				}

				/* NOT first entry */
				if (j > 0) {
					arg->expr[k]->src.max = arg->expr[k - 1]->src.min;
					arg->expr[k]->src.inc_max = 1;
					arg->expr[k]->src.exc_max = 0;
				}
				/* first entry */
				else {
					arg->expr[k]->src.max = colormap->entry[arg->pos[j]].value;
					arg->expr[k]->src.inc_max = 1;
					arg->expr[k]->src.exc_max = 1;
				}

				arg->expr[k]->dst.min = colormap->entry[arg->pos[j]].color[i];
				arg->expr[k]->dst.inc_min = 1;
				arg->expr[k]->dst.exc_min = 0;

				arg->expr[k]->dst.max = colormap->entry[arg->pos[j]].color[i];
				arg->expr[k]->dst.inc_max = 1;
				arg->expr[k]->dst.exc_max = 0;
			}
			else if (colormap->method == CM_EXACT) {
				arg->expr[k]->src.min = colormap->entry[arg->pos[j]].value;
				arg->expr[k]->src.inc_min = 1;
				arg->expr[k]->src.exc_min = 0;

				arg->expr[k]->src.max = colormap->entry[arg->pos[j]].value;
				arg->expr[k]->src.inc_max = 1;
				arg->expr[k]->src.exc_max = 0;

				arg->expr[k]->dst.min = colormap->entry[arg->pos[j]].color[i];
				arg->expr[k]->dst.inc_min = 1;
				arg->expr[k]->dst.exc_min = 0;

				arg->expr[k]->dst.max = colormap->entry[arg->pos[j]].color[i];
				arg->expr[k]->dst.inc_max = 1;
				arg->expr[k]->dst.exc_max = 0;
			}

			RASTER_DEBUGF(4, "expr[%d]->src (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->src.min,
				arg->expr[k]->src.max,
				arg->expr[k]->src.inc_min,
				arg->expr[k]->src.inc_max,
				arg->expr[k]->src.exc_min,
				arg->expr[k]->src.exc_max
			);

			RASTER_DEBUGF(4, "expr[%d]->dst (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->dst.min,
				arg->expr[k]->dst.max,
				arg->expr[k]->dst.inc_min,
				arg->expr[k]->dst.inc_max,
				arg->expr[k]->dst.exc_min,
				arg->expr[k]->dst.exc_max
			);

			k++;
		}

		/* EXACT has one last expression for catching all uncaught values */
		if (colormap->method == CM_EXACT) {
			arg->expr[k]->src.min = 0;
			arg->expr[k]->src.inc_min = 1;
			arg->expr[k]->src.exc_min = 1;

			arg->expr[k]->src.max = 0;
			arg->expr[k]->src.inc_max = 1;
			arg->expr[k]->src.exc_max = 1;

			arg->expr[k]->dst.min = 0;
			arg->expr[k]->dst.inc_min = 1;
			arg->expr[k]->dst.exc_min = 0;

			arg->expr[k]->dst.max = 0;
			arg->expr[k]->dst.inc_max = 1;
			arg->expr[k]->dst.exc_max = 0;

			RASTER_DEBUGF(4, "expr[%d]->src (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->src.min,
				arg->expr[k]->src.max,
				arg->expr[k]->src.inc_min,
				arg->expr[k]->src.inc_max,
				arg->expr[k]->src.exc_min,
				arg->expr[k]->src.exc_max
			);

			RASTER_DEBUGF(4, "expr[%d]->dst (min, max, in, ix, en, ex) = (%f, %f, %d, %d, %d, %d)",
				k,
				arg->expr[k]->dst.min,
				arg->expr[k]->dst.max,
				arg->expr[k]->dst.inc_min,
				arg->expr[k]->dst.inc_max,
				arg->expr[k]->dst.exc_min,
				arg->expr[k]->dst.exc_max
			);

			k++;
		}

		/* call rt_band_reclass */
		arg->band = rt_band_reclass(band, PT_8BUI, 0, 0, arg->expr, arg->nexpr);
		if (arg->band == NULL) {
			rterror("rt_raster_colormap: Could not reclassify band");
			_rti_colormap_arg_destroy(arg);
			return NULL;
		}

		/* add reclassified band to raster */
		if (rt_raster_add_band(arg->raster, arg->band, rt_raster_get_num_bands(arg->raster)) < 0) {
			rterror("rt_raster_colormap: Could not add reclassified band to output raster");
			_rti_colormap_arg_destroy(arg);
			return NULL;
		}
	}

	rtnraster = arg->raster;
	arg->raster = NULL;
	_rti_colormap_arg_destroy(arg);

	return rtnraster;
}
