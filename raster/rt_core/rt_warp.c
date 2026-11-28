/*
 *
 * WKTRaster - Raster Types for PostGIS
 *
 * Copyright (C) 2013 Bborie Park <dustymugs@gmail.com>
 * Copyright (C) 2011-2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 * Copyright (C) 2010-2011 Jorge Arevalo <jorge.arevalo@deimos-space.com>
 * Copyright (C) 2010-2011 David Zwarg <dzwarg@azavea.com>
 * Copyright (C) 2009-2011 Pierre Racine <pierre.racine@sbf.ulaval.ca>
 * Copyright (C) 2009-2011 Mateusz Loskot <mateusz@loskot.net>
 * Copyright (C) 2008-2009 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
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

#include "../../postgis_config.h"

#include <math.h>

#include "librtcore.h"
#include "librtcore_internal.h"

/******************************************************************************
 * rt_raster_gdal_warp()
 ******************************************************************************/

typedef struct _rti_warp_arg_t *_rti_warp_arg;
struct _rti_warp_arg_t {

	struct {
		GDALDriverH drv;
		GDALDatasetH ds;
		char *srs;
		int destroy_drv;
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
_rti_warp_arg_init()
{
	_rti_warp_arg arg = NULL;

	arg = rtalloc(sizeof(struct _rti_warp_arg_t));
	if (!arg)
	{
		rterror("_rti_warp_arg_init: Could not allocate memory for _rti_warp_arg");
		return NULL;
	}

	arg->src.drv = NULL;
	arg->src.destroy_drv = 0;
	arg->src.ds = NULL;
	arg->src.srs = NULL;

	arg->dst.drv = NULL;
	arg->dst.destroy_drv = 0;
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

/*
 * Use ceiling semantics so fractional output dimensions cover the source
 * extent, but keep nearly integral dimensions rounded for compatibility.
 */
static int
_rti_warp_ceil_dimension(double extent_size, double scale)
{
	double pixels;
	double rounded;

	if (scale == 0.0 || isnan(scale))
	{
		rterror("rt_raster_gdal_warp: Scale cannot be zero");
		return 0;
	}

	pixels = fabs(extent_size) / scale;
	rounded = floor(pixels + 0.5);

	if (FLT_EQ(pixels, rounded))
		return (int)fmax(rounded, 1);

	return (int)fmax(ceil(pixels), 1);
}

static bool
_rti_warp_nonzero(const double *value)
{ return value && !isnan(*value) && *value != 0.0; }

/*
 * Bound the source envelope in raster coordinates. For an aligned extent,
 * reduce the anchor to its fractional cell coordinates to avoid cancellation
 * when an equivalent grid origin is far from the source. The inverse
 * helpers in rt_raster.c use double precision and geopoint_to_cell also
 * rounds each point before we can distinguish lower and upper bounds.
 */
static const char *
_rti_warp_fit_extent(rt_envelope *extent, double gt[6], bool aligned, bool fit_scale, int dim[2])
{
	const long double kx = gt[2], ky = gt[4];
	if (fit_scale)
	{
		const long double w = extent->MaxX - extent->MinX, h = extent->MaxY - extent->MinY;
		const long double area = fabsl((long double)gt[1] * gt[5] - kx * ky);
		if (w * fabsl(gt[5]) + h * fabsl(kx) > dim[0] * area ||
		    h * fabsl(gt[1]) + w * fabsl(ky) > dim[1] * area)
		{
			/* Allow room for the shear across the full width and height. */
			gt[1] = (w + dim[1] * fabsl(kx)) / dim[0];
			gt[5] = -(h + dim[0] * fabsl(ky)) / dim[1];
		}
	}

	const long double sx = gt[1], sy = gt[5], det = sx * sy - kx * ky;
	if (!isfinite(det) || fabsl(det) <= 4 * DBL_EPSILON * (fabsl(sx * sy) + fabsl(kx * ky)))
		return "Cannot align a singular raster transform";

	/* Take the remainder before division can round away the grid phase. */
	const long double grid_col = fmodl(gt[0] * sy - gt[3] * kx, det) / det;
	const long double grid_row = fmodl(gt[3] * sx - gt[0] * ky, det) / det;
	const long double origin_x = grid_col * sx + grid_row * kx;
	const long double origin_y = grid_col * ky + grid_row * sy;
	const long double x[2] = {extent->MinX - origin_x, extent->MaxX - origin_x};
	const long double y[2] = {extent->MinY - origin_y, extent->MaxY - origin_y};
	const long double inv_x[2] = {sy, -ky}, inv_y[2] = {-kx, sx};
	long double lower[2];

	for (uint32_t axis = 0; axis < 2; axis++)
	{
		long double min = INFINITY, max = -INFINITY;
		for (uint32_t i = 0; i < 4; i++)
		{
			const long double a = x[i % 2] * inv_x[axis], b = y[i / 2] * inv_y[axis];
			long double cell = (a + b) / det;
			const long double nearest = roundl(cell);
			const long double error =
			    DBL_EPSILON * fabsl(cell) + 4 * LDBL_EPSILON * (fabsl(a) + fabsl(b)) / fabsl(det);
			if (fabsl(cell - nearest) <= error)
				cell = nearest;
			min = fminl(min, cell);
			max = fmaxl(max, cell);
		}

		lower[axis] = aligned ? floorl(min) : min;
		if (!dim[axis])
		{
			const long double span = max - min;
			/* Keep the historical unaligned padding where it fits. */
			const long double size =
			    aligned ? ceill(max) - lower[axis] : (span == UINT16_MAX ? span : floorl(span) + 1);
			if (!(size >= 1 && size <= UINT16_MAX))
				return "Aligned raster dimensions exceed the supported range";
			dim[axis] = size;
		}
	}

	extent->UpperLeftX = origin_x + lower[0] * sx + lower[1] * kx;
	extent->UpperLeftY = origin_y + lower[0] * ky + lower[1] * sy;
	return NULL;
}

static void
_rti_warp_arg_destroy(_rti_warp_arg arg)
{
	int i = 0;

	if (arg->dst.ds)
		GDALClose(arg->dst.ds);
	if (arg->dst.srs)
		CPLFree(arg->dst.srs);

	if (arg->dst.drv && arg->dst.destroy_drv)
	{
		GDALDeregisterDriver(arg->dst.drv);
		GDALDestroyDriver(arg->dst.drv);
	}

	if (arg->src.ds)
		GDALClose(arg->src.ds);
	if (arg->src.srs)
		CPLFree(arg->src.srs);

	if (arg->src.drv && arg->src.destroy_drv)
	{
		GDALDeregisterDriver(arg->src.drv);
		GDALDestroyDriver(arg->src.drv);
	}

	if (arg->transform.func == GDALApproxTransform)
	{
		if (arg->transform.arg.imgproj)
			GDALDestroyGenImgProjTransformer(arg->transform.arg.imgproj);
	}

	if (arg->wopts)
		GDALDestroyWarpOptions(arg->wopts);

	if (arg->transform.option.len > 0 && arg->transform.option.item)
	{
		for (i = 0; i < arg->transform.option.len; i++)
		{
			if (arg->transform.option.item[i])
				rtdealloc(arg->transform.option.item[i]);
		}
		rtdealloc(arg->transform.option.item);
	}

	rtdealloc(arg);
	arg = NULL;
}

/**
 * Return a warped raster using GDAL Warp API.
 * When non-zero skew values are requested, the output extent is computed
 * via rt_raster_compute_skewed_raster().
 */
rt_raster
rt_raster_gdal_warp(rt_raster raster,
		    const char *src_srs,
		    const char *dst_srs,
		    double *scale_x,
		    double *scale_y,
		    int *width,
		    int *height,
		    double *ul_xw,
		    double *ul_yw,
		    double *grid_xw,
		    double *grid_yw,
		    double *skew_x,
		    double *skew_y,
		    GDALResampleAlg resample_alg,
		    double max_err)
{
	CPLErr cplerr;
	char *dst_options[] = {"SUBCLASS=VRTWarpedDataset", NULL};
	const char *init_dest_nodata_option = "INIT_DEST=NO_DATA";
#if POSTGIS_GDAL_VERSION >= 30302
	const char *unified_src_nodata_option = "UNIFIED_SRC_NODATA=PARTIAL";
#else
	const char *unified_src_nodata_option = "UNIFIED_SRC_NODATA=NO";
#endif
	_rti_warp_arg arg = NULL;

	int nodata_count = 0;
	int has_nodata_count = 0;

	GDALRasterBandH band;
	rt_band rtband = NULL;
	rt_pixtype pt = PT_END;
	GDALDataType gdal_pt = GDT_Unknown;
	double nodata = 0.0;

	double _gt[6] = {0};
	double src_gt[6] = {0};
	double dst_extent[4];
	rt_envelope extent;

	int _dim[2] = {0};
	int suggested_dim[2] = {0};
	double _skew[2] = {0};
	double _scale[2] = {0};
	int ul_user = 0;
	int aligned_extent = 0;

	rt_raster rast = NULL;
	rt_raster nodata_src = NULL;
	rt_raster data_src = NULL;
	rt_raster nodata_warp = NULL;
	rt_raster data_warp = NULL;
	rt_raster src = NULL;
	uint32_t *nodata_bands = NULL;
	uint32_t *data_bands = NULL;
	int *group_index = NULL;
	int *group_has_nodata = NULL;
	int i = 0;
	int numBands = 0;
	int nodata_pos = 0;
	int data_pos = 0;
	int use_init_dest_nodata = 0;

	int default_geotransform = 0;

	RASTER_DEBUG(3, "rt_raster_gdal_warp: starting");

	assert(raster);

	numBands = rt_raster_get_num_bands(raster);
	for (i = 0; i < numBands; i++)
	{
		rtband = rt_raster_get_band(raster, i);
		if (!rtband)
		{
			rterror("rt_raster_gdal_warp: Could not get band %d for checking nodata state", i);
			return NULL;
		}
		if (rt_band_get_hasnodata_flag(rtband) != FALSE)
			has_nodata_count++;
	}

	if (has_nodata_count > 0 && has_nodata_count < numBands)
	{
		nodata_bands = rtalloc(sizeof(uint32_t) * has_nodata_count);
		data_bands = rtalloc(sizeof(uint32_t) * (numBands - has_nodata_count));
		group_index = rtalloc(sizeof(int) * numBands);
		group_has_nodata = rtalloc(sizeof(int) * numBands);
		if (!nodata_bands || !data_bands || !group_index || !group_has_nodata)
		{
			rterror("rt_raster_gdal_warp: Could not allocate mixed nodata band mapping");
			rtdealloc(nodata_bands);
			rtdealloc(data_bands);
			rtdealloc(group_index);
			rtdealloc(group_has_nodata);
			return NULL;
		}

		for (i = 0; i < numBands; i++)
		{
			rtband = rt_raster_get_band(raster, i);
			if (rt_band_get_hasnodata_flag(rtband) != FALSE)
			{
				nodata_bands[nodata_pos] = i;
				group_index[i] = nodata_pos++;
				group_has_nodata[i] = 1;
			}
			else
			{
				data_bands[data_pos] = i;
				group_index[i] = data_pos++;
				group_has_nodata[i] = 0;
			}
		}

		nodata_src = rt_raster_from_band(raster, nodata_bands, has_nodata_count);
		data_src = rt_raster_from_band(raster, data_bands, numBands - has_nodata_count);
		if (!nodata_src || !data_src)
		{
			rterror("rt_raster_gdal_warp: Could not split mixed nodata raster");
			if (nodata_src)
				rt_raster_destroy(nodata_src);
			if (data_src)
				rt_raster_destroy(data_src);
			rtdealloc(nodata_bands);
			rtdealloc(data_bands);
			rtdealloc(group_index);
			rtdealloc(group_has_nodata);
			return NULL;
		}

		nodata_warp = rt_raster_gdal_warp(nodata_src,
						  src_srs,
						  dst_srs,
						  scale_x,
						  scale_y,
						  width,
						  height,
						  ul_xw,
						  ul_yw,
						  grid_xw,
						  grid_yw,
						  skew_x,
						  skew_y,
						  resample_alg,
						  max_err);
		data_warp = rt_raster_gdal_warp(data_src,
						src_srs,
						dst_srs,
						scale_x,
						scale_y,
						width,
						height,
						ul_xw,
						ul_yw,
						grid_xw,
						grid_yw,
						skew_x,
						skew_y,
						resample_alg,
						max_err);
		rt_raster_destroy(nodata_src);
		rt_raster_destroy(data_src);
		if (!nodata_warp || !data_warp)
		{
			rterror("rt_raster_gdal_warp: Could not warp mixed nodata raster groups");
			if (nodata_warp)
				rt_raster_destroy(nodata_warp);
			if (data_warp)
				rt_raster_destroy(data_warp);
			rtdealloc(nodata_bands);
			rtdealloc(data_bands);
			rtdealloc(group_index);
			rtdealloc(group_has_nodata);
			return NULL;
		}

		rast = rt_raster_clone(nodata_warp, 0);
		if (!rast)
		{
			rterror("rt_raster_gdal_warp: Could not create mixed nodata output raster");
			rt_raster_destroy(nodata_warp);
			rt_raster_destroy(data_warp);
			rtdealloc(nodata_bands);
			rtdealloc(data_bands);
			rtdealloc(group_index);
			rtdealloc(group_has_nodata);
			return NULL;
		}

		for (i = 0; i < numBands; i++)
		{
			src = group_has_nodata[i] ? nodata_warp : data_warp;
			if (rt_raster_copy_band(rast, src, group_index[i], i) < 0)
			{
				rterror("rt_raster_gdal_warp: Could not merge warped mixed nodata band");
				rt_raster_destroy(rast);
				rt_raster_destroy(nodata_warp);
				rt_raster_destroy(data_warp);
				rtdealloc(nodata_bands);
				rtdealloc(data_bands);
				rtdealloc(group_index);
				rtdealloc(group_has_nodata);
				return NULL;
			}
		}

		rt_raster_destroy(nodata_warp);
		rt_raster_destroy(data_warp);
		rtdealloc(nodata_bands);
		rtdealloc(data_bands);
		rtdealloc(group_index);
		rtdealloc(group_has_nodata);

		RASTER_DEBUG(3, "rt_raster_gdal_warp: done");
		return rast;
	}

	arg = _rti_warp_arg_init();
	if (!arg)
	{
		rterror("rt_raster_gdal_warp: Could not initialize internal variables");
		return NULL;
	}

	if (max_err < 0.)
		max_err = 0.125;

	/* handle srs */
	if (src_srs)
	{
		if (dst_srs && strcmp(src_srs, dst_srs) != 0)
		{
			RASTER_DEBUG(4, "rt_raster_gdal_warp: Warp includes reprojection");
			arg->src.srs = rt_util_gdal_convert_sr(src_srs, 0);
			arg->dst.srs = rt_util_gdal_convert_sr(dst_srs, 0);
			if (!arg->src.srs || !arg->dst.srs)
			{
				rterror("rt_raster_gdal_warp: Could not convert srs values to GDAL accepted format");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}
		}
	}
	else if (dst_srs)
	{
		rterror("rt_raster_gdal_warp: SRS required for input raster if SRS provided for warped raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* load raster into GDAL MEM dataset */
	arg->src.ds =
	    rt_raster_to_gdal_mem(raster, arg->src.srs, NULL, NULL, 0, &(arg->src.drv), &(arg->src.destroy_drv));
	if (!arg->src.ds)
	{
		rterror("rt_raster_gdal_warp: Could not convert raster to GDAL MEM format");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/*
	 * Give GDAL the affine transform explicitly for default rasters, which
	 * dataset-based discovery may treat as ungeoreferenced.
	 */
	if (!src_srs && !dst_srs && rt_raster_get_srid(raster) == SRID_UNKNOWN)
	{
		rt_raster_get_geotransform_matrix(raster, src_gt);
		default_geotransform = FLT_EQ(src_gt[0], 0.0) && FLT_EQ(src_gt[3], 0.0) && FLT_EQ(src_gt[1], 1.0) &&
				       FLT_EQ(src_gt[5], -1.0) && FLT_EQ(src_gt[2], 0.0) && FLT_EQ(src_gt[4], 0.0);
		if (default_geotransform)
			rtwarn("Raster has default geotransform. Adjusting metadata for use of GDAL Warp API");
	}

	/* set transform options */
	if (arg->src.srs || arg->dst.srs)
	{
		arg->transform.option.len = 2;
		arg->transform.option.item = rtalloc(sizeof(char *) * (arg->transform.option.len + 1));
		if (!arg->transform.option.item)
		{
			rterror("rt_raster_gdal_warp: Could not allocate memory for transform options");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		memset(arg->transform.option.item, 0, sizeof(char *) * (arg->transform.option.len + 1));

		for (i = 0; i < arg->transform.option.len; i++)
		{
			const char *srs = i ? arg->dst.srs : arg->src.srs;
			const char *lbl = i ? "DST_SRS=" : "SRC_SRS=";
			size_t sz = sizeof(char) * (strlen(lbl) + 1);
			if (srs)
				sz += strlen(srs);
			arg->transform.option.item[i] = (char *)rtalloc(sz);
			if (!arg->transform.option.item[i])
			{
				rterror("rt_raster_gdal_warp: Could not allocate memory for transform options");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}
			sprintf(arg->transform.option.item[i], "%s%s", lbl, srs ? srs : "");
		}
	}
	else
		arg->transform.option.len = 0;

	/* transformation object for building dst dataset */
	arg->transform.arg.transform =
	    default_geotransform ? GDALCreateGenImgProjTransformer3(NULL, src_gt, NULL, NULL)
				 : GDALCreateGenImgProjTransformer2(arg->src.ds, NULL, arg->transform.option.item);
	if (!arg->transform.arg.transform)
	{
		rterror("rt_raster_gdal_warp: Could not create GDAL transformation object");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* get approximate output georeferenced bounds and resolution */
	cplerr = GDALSuggestedWarpOutput2(arg->src.ds,
					  GDALGenImgProjTransform,
					  arg->transform.arg.transform,
					  _gt,
					  &(_dim[0]),
					  &(_dim[1]),
					  dst_extent,
					  0);
	if (cplerr != CE_None)
	{
		rterror("rt_raster_gdal_warp: Could not get GDAL suggested warp output");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	suggested_dim[0] = _dim[0];
	suggested_dim[1] = _dim[1];
	GDALDestroyGenImgProjTransformer(arg->transform.arg.transform);
	arg->transform.arg.transform = NULL;

	_dim[0] = 0;
	_dim[1] = 0;

	extent.MinX = dst_extent[0];
	extent.MinY = dst_extent[1];
	extent.MaxX = dst_extent[2];
	extent.MaxY = dst_extent[3];
	extent.UpperLeftX = dst_extent[0];
	extent.UpperLeftY = dst_extent[3];
	/* scale and width/height are mutually exclusive */
	if ((scale_x || scale_y) && (width || height))
	{
		rterror("rt_raster_gdal_warp: Scale X/Y and width/height are mutually exclusive");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	if (width)
	{
		_dim[0] = abs(*width);
		_scale[0] = fabs((extent.MaxX - extent.MinX) / _dim[0]);
	}
	if (height)
	{
		_dim[1] = abs(*height);
		_scale[1] = fabs((extent.MaxY - extent.MinY) / _dim[1]);
	}

	if (_rti_warp_nonzero(scale_x) && _rti_warp_nonzero(scale_y))
	{
		_scale[0] = fabs(*scale_x);
		_scale[1] = fabs(*scale_y);
		if (!src_srs || !dst_srs || strcmp(src_srs, dst_srs) == 0)
		{
			if (rt_raster_get_envelope(raster, &extent) != ES_NONE)
			{
				rterror("rt_raster_gdal_warp: Could not get source raster envelope");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}
		}
	}
	else if (!scale_x != !scale_y)
	{
		rterror("rt_raster_gdal_warp: Both X and Y scale values must be provided");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* scale not defined: use suggested */
	if ((_scale[0] == 0.0 || isnan(_scale[0])) && (_scale[1] == 0.0 || isnan(_scale[1])))
	{
		_scale[0] = fabs(_gt[1]);
		_scale[1] = fabs(_gt[5]);
		if (src_srs && dst_srs && strcmp(src_srs, dst_srs) != 0 && !skew_x && !skew_y)
		{
			_dim[0] = suggested_dim[0];
			_dim[1] = suggested_dim[1];
		}
	}

	RASTER_DEBUGF(4, "rt_raster_gdal_warp: Using scale: %f x %f", _scale[0], -1 * _scale[1]);

	/* Preserve the axis convention used by each warp path. */
	const bool flip[2] = {scale_x && *scale_x < 0.0, scale_y && *scale_y > 0.0};
	const double *requested_skew[2] = {skew_x, skew_y};
	for (uint32_t axis = 0; axis < 2; axis++)
	{
		if (requested_skew[axis])
			_skew[axis] = flip[default_geotransform ? 1 - axis : axis] ? -*requested_skew[axis]
										   : *requested_skew[axis];
	}

	RASTER_DEBUGF(4, "rt_raster_gdal_warp: Using skew: %f x %f", _skew[0], _skew[1]);

	aligned_extent = default_geotransform && grid_xw && grid_yw && !ul_xw && !ul_yw && !width && !height;
	const bool skewed = _rti_warp_nonzero(&_skew[0]) || _rti_warp_nonzero(&_skew[1]);
	if (default_geotransform && (aligned_extent || skewed))
	{
		double grid_gt[6] = {aligned_extent ? *grid_xw : 0,
				     _scale[0],
				     _skew[0],
				     aligned_extent ? *grid_yw : 0,
				     _skew[1],
				     -_scale[1]};
		if (rt_raster_get_envelope(raster, &extent) != ES_NONE)
		{
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		const bool fit_scale = _dim[0] && _dim[1] && !grid_xw && !grid_yw && !ul_xw && !ul_yw;
		const char *error = _rti_warp_fit_extent(&extent, grid_gt, aligned_extent, fit_scale, _dim);
		if (error)
		{
			_rti_warp_arg_destroy(arg);
			rterror("rt_raster_gdal_warp: %s", error);
			return NULL;
		}
		_scale[0] = grid_gt[1];
		_scale[1] = -grid_gt[5];
	}
	else if (skewed)
	{
		rt_raster skewedrast = rt_raster_compute_skewed_raster(extent, _skew, _scale);
		if (!skewedrast)
		{
			rterror("rt_raster_gdal_warp: Could not compute skewed extent");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		if (!_dim[0])
			_dim[0] = rt_raster_get_width(skewedrast);
		if (!_dim[1])
			_dim[1] = rt_raster_get_height(skewedrast);

		extent.UpperLeftX = rt_raster_get_x_offset(skewedrast);
		extent.UpperLeftY = rt_raster_get_y_offset(skewedrast);
		rt_raster_destroy(skewedrast);
	}

	/* dimensions not defined, compute from extent */
	if (!_dim[0])
		_dim[0] = _rti_warp_ceil_dimension(extent.MaxX - extent.MinX, _scale[0]);
	if (!_dim[1])
		_dim[1] = _rti_warp_ceil_dimension(extent.MaxY - extent.MinY, _scale[1]);

	/* temporary raster */
	rast = rt_raster_new(_dim[0], _dim[1]);
	if (!rast)
	{
		rterror("rt_raster_gdal_warp: Out of memory allocating temporary raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	rt_raster_set_offsets(rast, extent.UpperLeftX, extent.UpperLeftY);
	rt_raster_set_scale(rast, _scale[0], -_scale[1]);
	rt_raster_set_skews(rast, _skew[0], _skew[1]);

	rt_raster_get_geotransform_matrix(rast, _gt);
	RASTER_DEBUGF(3,
		      "rt_raster_gdal_warp: Temp raster geotransform: %f,%f,%f,%f,%f,%f",
		      _gt[0],
		      _gt[1],
		      _gt[2],
		      _gt[3],
		      _gt[4],
		      _gt[5]);

	/* user-defined upper-left corner */
	if (ul_xw && ul_yw)
	{
		ul_user = 1;
		rt_raster_set_offsets(rast, *ul_xw, *ul_yw);
		extent.UpperLeftX = *ul_xw;
		extent.UpperLeftY = *ul_yw;
	}
	else if (!ul_xw != !ul_yw)
	{
		rterror("rt_raster_gdal_warp: Both X and Y upper-left corner values must be provided");
		rt_raster_destroy(rast);
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* alignment only considered if upper-left corner not provided */
	if (!aligned_extent && !ul_user && (grid_xw || grid_yw))
	{
		if (!grid_xw != !grid_yw)
		{
			rterror("rt_raster_gdal_warp: Both X and Y alignment values must be provided");
			rt_raster_destroy(rast);
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		RASTER_DEBUGF(4, "Aligning extent to user-specified grid: %f, %f", *grid_xw, *grid_yw);

		do
		{
			double _r[2] = {0};
			double _w[2] = {0};
			if (FLT_EQ(*grid_xw, extent.UpperLeftX) && FLT_EQ(*grid_yw, extent.UpperLeftY))
			{
				RASTER_DEBUG(3, "Skipping raster alignment as it is already aligned to grid");
				break;
			}

			extent.UpperLeftX = rast->ipX;
			extent.UpperLeftY = rast->ipY;
			rt_raster_set_offsets(rast, *grid_xw, *grid_yw);

			/* process upper-left corner */
			if (rt_raster_geopoint_to_cell(
				rast, extent.UpperLeftX, extent.UpperLeftY, &(_r[0]), &(_r[1]), NULL) != ES_NONE)
			{
				rterror("rt_raster_gdal_warp: Could not compute raster pixel for spatial coordinates");
				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			if (rt_raster_cell_to_geopoint(rast, _r[0], _r[1], &(_w[0]), &(_w[1]), NULL) != ES_NONE)
			{
				rterror("rt_raster_gdal_warp: Could not compute spatial coordinates for raster pixel");
				rt_raster_destroy(rast);
				_rti_warp_arg_destroy(arg);
				return NULL;
			}

			const int *requested_dim[2] = {width, height};
			uint16_t *dim[2] = {&rast->width, &rast->height};
			double *scale[2] = {&rast->scaleX, &rast->scaleY};
			const double origin[2] = {extent.UpperLeftX, extent.UpperLeftY};
			for (uint32_t axis = 0; axis < 2; axis++)
			{
				if (!FLT_NEQ(_w[axis], origin[axis]))
					continue;
				if (!requested_dim[axis])
				{
					if (*dim[axis] == UINT16_MAX)
					{
						rt_raster_destroy(rast);
						_rti_warp_arg_destroy(arg);
						rterror(
						    "rt_raster_gdal_warp: Aligned raster dimensions exceed the supported range");
						return NULL;
					}
					(*dim[axis])++;
				}
				else
				{
					/* A fixed dimension implies that the scale is adjustable. */
					double corner[2];
					GDALApplyGeoTransform(_gt,
							      axis == 0 ? *dim[axis] : 0,
							      axis == 1 ? *dim[axis] : 0,
							      &corner[0],
							      &corner[1]);
					const double adjusted = fabs((corner[axis] - _w[axis]) / *dim[axis]);
					*scale[axis] = axis == 0 ? adjusted : -adjusted;
				}
			}

			rt_raster_set_offsets(rast, _w[0], _w[1]);
			RASTER_DEBUGF(4, "aligned offsets: %f x %f", _w[0], _w[1]);
		} while (0);
	}

	/* get key attributes */
	_dim[0] = rast->width;
	_dim[1] = rast->height;
	rt_raster_get_geotransform_matrix(rast, _gt);

	const double *requested_scale[2] = {scale_x, scale_y};
	for (uint32_t axis = 0; axis < 2; axis++)
	{
		if (!flip[axis])
			continue;

		if (default_geotransform)
		{
			/* Reverse the full pixel or line vector. */
			for (uint32_t offset = 0; offset < 6; offset += 3)
			{
				double *step = &_gt[offset + axis + 1];
				if (!ul_user)
					_gt[offset] += _dim[axis] * *step;
				*step = -*step;
			}
		}
		else
		{
			/* Preserve the historical signed-scale placement for georeferenced inputs. */
			double corner[2];
			GDALApplyGeoTransform(
			    _gt, axis == 0 ? _dim[axis] : 0, axis == 1 ? _dim[axis] : 0, &corner[0], &corner[1]);
			const uint32_t offset = 3 * axis;
			_gt[offset] = corner[axis];
			_gt[offset + axis + 1] = *requested_scale[axis];
			if (_rti_warp_nonzero(requested_skew[axis]))
				_gt[offset + 2 - axis] = *requested_skew[axis];
		}
	}
	rt_raster_destroy(rast);
	rast = NULL;

	RASTER_DEBUGF(3,
		      "rt_raster_gdal_warp: Final geotransform: %f,%f,%f,%f,%f,%f",
		      _gt[0],
		      _gt[1],
		      _gt[2],
		      _gt[3],
		      _gt[4],
		      _gt[5]);
	RASTER_DEBUGF(3, "rt_raster_gdal_warp: Raster dimensions: %d x %d", _dim[0], _dim[1]);

	if (_dim[0] == 0 || _dim[1] == 0)
	{
		rterror("rt_raster_gdal_warp: The width (%d) or height (%d) of the warped raster is zero",
			_dim[0],
			_dim[1]);
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* load VRT driver */
	if (!rt_util_gdal_driver_registered("VRT"))
	{
		GDALRegister_VRT();
		arg->dst.destroy_drv = 1;
	}
	arg->dst.drv = GDALGetDriverByName("VRT");
	if (!arg->dst.drv)
	{
		rterror("rt_raster_gdal_warp: Could not load the output GDAL VRT driver");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* create dst dataset */
	arg->dst.ds = GDALCreate(arg->dst.drv, "", _dim[0], _dim[1], 0, GDT_Byte, dst_options);
	if (!arg->dst.ds)
	{
		rterror("rt_raster_gdal_warp: Could not create GDAL VRT dataset");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* set dst srs */
	if (arg->dst.srs)
	{
		cplerr = GDALSetProjection(arg->dst.ds, arg->dst.srs);
		if (cplerr != CE_None)
		{
			rterror("rt_raster_gdal_warp: Could not set projection");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
	}

	/* set dst geotransform */
	cplerr = GDALSetGeoTransform(arg->dst.ds, _gt);
	if (cplerr != CE_None)
	{
		rterror("rt_raster_gdal_warp: Could not set geotransform");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	/* add bands */
	for (i = 0; i < numBands; i++)
	{
		rtband = rt_raster_get_band(raster, i);
		if (!rtband)
		{
			rterror("rt_raster_gdal_warp: Could not get band %d for adding to VRT dataset", i);
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		pt = rt_band_get_pixtype(rtband);
		gdal_pt = rt_util_pixtype_to_gdal_datatype(pt);
		if (gdal_pt == GDT_Unknown)
			rtwarn("rt_raster_gdal_warp: Unknown pixel type for band %d", i);

		cplerr = GDALAddBand(arg->dst.ds, gdal_pt, NULL);
		if (cplerr != CE_None)
		{
			rterror("rt_raster_gdal_warp: Could not add band to VRT dataset");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		band = GDALGetRasterBand(arg->dst.ds, i + 1);
		if (!band)
		{
			rterror("rt_raster_gdal_warp: Could not get GDAL band for additional processing");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}

		if (rt_band_get_hasnodata_flag(rtband) != FALSE)
		{
			nodata_count++;
			rt_band_get_nodata(rtband, &nodata);
			if (GDALSetRasterNoDataValue(band, nodata) != CE_None)
				rtwarn("rt_raster_gdal_warp: Could not set nodata value for band %d", i);
		}
	}

	/* create transformation object */
	arg->transform.arg.transform = arg->transform.arg.imgproj =
	    default_geotransform
		? GDALCreateGenImgProjTransformer3(NULL, src_gt, NULL, _gt)
		: GDALCreateGenImgProjTransformer2(arg->src.ds, arg->dst.ds, arg->transform.option.item);
	if (!arg->transform.arg.transform)
	{
		rterror("rt_raster_gdal_warp: Could not create GDAL transformation object");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	arg->transform.func = GDALGenImgProjTransform;

	if (max_err > 0.0)
	{
		arg->transform.arg.transform = arg->transform.arg.approx =
		    GDALCreateApproxTransformer(GDALGenImgProjTransform, arg->transform.arg.imgproj, max_err);
		if (!arg->transform.arg.transform)
		{
			rterror("rt_raster_gdal_warp: Could not create GDAL approximate transformation object");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		arg->transform.func = GDALApproxTransform;
	}

	/* warp options */
	arg->wopts = GDALCreateWarpOptions();
	if (!arg->wopts)
	{
		rterror("rt_raster_gdal_warp: Could not create GDAL warp options object");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}

	arg->wopts->eResampleAlg = resample_alg;
	arg->wopts->hSrcDS = arg->src.ds;
	arg->wopts->hDstDS = arg->dst.ds;
	arg->wopts->pfnTransformer = arg->transform.func;
	arg->wopts->pTransformerArg = arg->transform.arg.transform;
	/* Route long GDAL warp operations through the registered interrupt hook. */
	arg->wopts->pfnProgress = rt_util_gdal_progress_func;
	arg->wopts->pProgressArg = (void *)"GDALWarp";
	/*
	 * GDAL 3.11 warns, and later treats as an error, when INIT_DEST=NO_DATA
	 * is requested without destination nodata values for every band.
	 */
	use_init_dest_nodata = numBands > 0 && nodata_count == numBands;
	arg->wopts->papszWarpOptions = (char **)CPLMalloc(sizeof(char *) * (use_init_dest_nodata ? 3 : 2));
	i = 0;
	if (use_init_dest_nodata)
	{
		arg->wopts->papszWarpOptions[i] =
		    (char *)CPLMalloc(sizeof(char) * (strlen(init_dest_nodata_option) + 1));
		strcpy(arg->wopts->papszWarpOptions[i++], init_dest_nodata_option);
	}
	arg->wopts->papszWarpOptions[i] = (char *)CPLMalloc(sizeof(char) * (strlen(unified_src_nodata_option) + 1));
	strcpy(arg->wopts->papszWarpOptions[i++], unified_src_nodata_option);
	arg->wopts->papszWarpOptions[i] = NULL;

	/* band mapping */
	arg->wopts->nBandCount = numBands;
	arg->wopts->panSrcBands = (int *)CPLMalloc(sizeof(int) * arg->wopts->nBandCount);
	arg->wopts->panDstBands = (int *)CPLMalloc(sizeof(int) * arg->wopts->nBandCount);
	for (i = 0; i < arg->wopts->nBandCount; i++)
		arg->wopts->panDstBands[i] = arg->wopts->panSrcBands[i] = i + 1;

	/*
	 * When every band declares nodata, pass the explicit vector to GDAL.
	 * Mixed-band rasters keep their intrinsic per-band nodata metadata; a
	 * synthetic value for bands without nodata can collide with valid pixels.
	 */
	if (numBands > 0 && nodata_count == numBands)
	{
		arg->wopts->padfSrcNoDataReal = (double *)CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfDstNoDataReal = (double *)CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfSrcNoDataImag = (double *)CPLMalloc(numBands * sizeof(double));
		arg->wopts->padfDstNoDataImag = (double *)CPLMalloc(numBands * sizeof(double));
		if (!arg->wopts->padfSrcNoDataReal || !arg->wopts->padfDstNoDataReal ||
		    !arg->wopts->padfSrcNoDataImag || !arg->wopts->padfDstNoDataImag)
		{
			rterror("rt_raster_gdal_warp: Out of memory allocating nodata mapping");
			_rti_warp_arg_destroy(arg);
			return NULL;
		}
		for (i = 0; i < numBands; i++)
		{
			band = rt_raster_get_band(raster, i);
			if (!band)
			{
				rterror("rt_raster_gdal_warp: Could not process bands for nodata values");
				_rti_warp_arg_destroy(arg);
				return NULL;
			}
			rt_band_get_nodata(band, &(arg->wopts->padfSrcNoDataReal[i]));
			arg->wopts->padfDstNoDataReal[i] = arg->wopts->padfSrcNoDataReal[i];
			arg->wopts->padfDstNoDataImag[i] = arg->wopts->padfSrcNoDataImag[i] = 0.0;
		}
	}

	/* warp */
	cplerr = GDALInitializeWarpedVRT(arg->dst.ds, arg->wopts);
	if (cplerr != CE_None)
	{
		rterror("rt_raster_gdal_warp: Could not warp raster");
		_rti_warp_arg_destroy(arg);
		return NULL;
	}
	GDALFlushCache(arg->dst.ds);

	/* convert to raster */
	rast = rt_raster_from_gdal_dataset(arg->dst.ds);
	_rti_warp_arg_destroy(arg);

	if (!rast)
	{
		rterror("rt_raster_gdal_warp: Could not convert warped dataset to raster");
		return NULL;
	}

	RASTER_DEBUG(3, "rt_raster_gdal_warp: done");
	return rast;
}
