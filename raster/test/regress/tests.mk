# **********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * Copyright (C) 2020 Sandro Santilli <strk@kbt.io>
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

override RUNTESTFLAGS := $(RUNTESTFLAGS) --raster

override RUNTESTFLAGS_INTERNAL := \
  --before-upgrade-script $(top_srcdir)/raster/test/regress/hooks/hook-before-upgrade-raster.sql \
  $(RUNTESTFLAGS_INTERNAL) \
  --after-upgrade-script $(top_srcdir)/raster/test/regress/hooks/hook-after-upgrade-raster.sql

RASTER_TEST_FIRST = \
	$(top_srcdir)/raster/test/regress/check_gdal \
	$(top_srcdir)/raster/test/regress/loader/load_outdb

RASTER_TEST_LAST = \
	$(top_srcdir)/raster/test/regress/clean

RASTER_TEST_METADATA = \
	$(top_srcdir)/raster/test/regress/check_raster_columns \
	$(top_srcdir)/raster/test/regress/check_raster_overviews

RASTER_TEST_IO = \
	$(top_srcdir)/raster/test/regress/rt_io

RASTER_TEST_BASIC_FUNC = \
	$(top_srcdir)/raster/test/regress/rt_bytea \
	$(top_srcdir)/raster/test/regress/rt_wkb \
	$(top_srcdir)/raster/test/regress/box3d \
	$(top_srcdir)/raster/test/regress/rt_addband \
	$(top_srcdir)/raster/test/regress/rt_band \
	$(top_srcdir)/raster/test/regress/rt_tile

RASTER_TEST_PROPS = \
	$(top_srcdir)/raster/test/regress/rt_dimensions \
	$(top_srcdir)/raster/test/regress/rt_scale \
	$(top_srcdir)/raster/test/regress/rt_pixelsize \
	$(top_srcdir)/raster/test/regress/rt_upperleft \
	$(top_srcdir)/raster/test/regress/rt_rotation \
	$(top_srcdir)/raster/test/regress/rt_georeference \
	$(top_srcdir)/raster/test/regress/rt_set_properties \
	$(top_srcdir)/raster/test/regress/rt_isempty \
	$(top_srcdir)/raster/test/regress/rt_hasnoband \
	$(top_srcdir)/raster/test/regress/rt_metadata \
	$(top_srcdir)/raster/test/regress/rt_rastertoworldcoord \
	$(top_srcdir)/raster/test/regress/rt_worldtorastercoord \
	$(top_srcdir)/raster/test/regress/rt_convexhull \
	$(top_srcdir)/raster/test/regress/rt_envelope

RASTER_TEST_BANDPROPS = \
	$(top_srcdir)/raster/test/regress/rt_band_properties \
	$(top_srcdir)/raster/test/regress/rt_set_band_properties \
	$(top_srcdir)/raster/test/regress/rt_pixelaspolygons \
	$(top_srcdir)/raster/test/regress/rt_pixelaspoints \
	$(top_srcdir)/raster/test/regress/rt_pixelascentroids \
	$(top_srcdir)/raster/test/regress/rt_setvalues_array \
	$(top_srcdir)/raster/test/regress/rt_summarystats \
	$(top_srcdir)/raster/test/regress/rt_count \
	$(top_srcdir)/raster/test/regress/rt_histogram \
	$(top_srcdir)/raster/test/regress/rt_quantile \
	$(top_srcdir)/raster/test/regress/rt_valuecount \
	$(top_srcdir)/raster/test/regress/rt_valuepercent \
	$(top_srcdir)/raster/test/regress/rt_bandmetadata \
	$(top_srcdir)/raster/test/regress/rt_pixelvalue \
	$(top_srcdir)/raster/test/regress/rt_neighborhood \
	$(top_srcdir)/raster/test/regress/rt_nearestvalue \
	$(top_srcdir)/raster/test/regress/rt_pixelofvalue \
	$(top_srcdir)/raster/test/regress/rt_polygon \
	$(top_srcdir)/raster/test/regress/rt_setbandpath

RASTER_TEST_UTILITY = \
	$(top_srcdir)/raster/test/regress/rt_utility \
	$(top_srcdir)/raster/test/regress/rt_fromgdalraster \
	$(top_srcdir)/raster/test/regress/rt_asgdalraster \
	$(top_srcdir)/raster/test/regress/rt_astiff \
	$(top_srcdir)/raster/test/regress/rt_asjpeg \
	$(top_srcdir)/raster/test/regress/rt_aspng \
	$(top_srcdir)/raster/test/regress/rt_reclass \
	$(top_srcdir)/raster/test/regress/rt_gdalwarp \
	$(top_srcdir)/raster/test/regress/rt_gdalcontour \
	$(top_srcdir)/raster/test/regress/rt_asraster \
	$(top_srcdir)/raster/test/regress/rt_dumpvalues \
	$(top_srcdir)/raster/test/regress/rt_makeemptycoverage \
	$(top_srcdir)/raster/test/regress/rt_createoverview

RASTER_TEST_MAPALGEBRA = \
	$(top_srcdir)/raster/test/regress/rt_mapalgebraexpr \
	$(top_srcdir)/raster/test/regress/rt_mapalgebrafct \
	$(top_srcdir)/raster/test/regress/rt_mapalgebraexpr_2raster \
	$(top_srcdir)/raster/test/regress/rt_mapalgebrafct_2raster \
	$(top_srcdir)/raster/test/regress/rt_mapalgebrafctngb \
	$(top_srcdir)/raster/test/regress/rt_mapalgebrafctngb_userfunc \
	$(top_srcdir)/raster/test/regress/rt_intersection \
	$(top_srcdir)/raster/test/regress/rt_clip \
	$(top_srcdir)/raster/test/regress/rt_mapalgebra \
	$(top_srcdir)/raster/test/regress/rt_mapalgebra_expr \
	$(top_srcdir)/raster/test/regress/rt_mapalgebra_mask \
	$(top_srcdir)/raster/test/regress/rt_union \
	$(top_srcdir)/raster/test/regress/rt_invdistweight4ma \
	$(top_srcdir)/raster/test/regress/rt_4ma \
	$(top_srcdir)/raster/test/regress/rt_setvalues_geomval \
	$(top_srcdir)/raster/test/regress/rt_elevation_functions \
	$(top_srcdir)/raster/test/regress/rt_colormap \
	$(top_srcdir)/raster/test/regress/rt_grayscale

RASTER_TEST_SREL = \
	$(top_srcdir)/raster/test/regress/rt_gist_relationships \
	$(top_srcdir)/raster/test/regress/rt_intersects \
	$(top_srcdir)/raster/test/regress/rt_samealignment \
	$(top_srcdir)/raster/test/regress/rt_geos_relationships \
	$(top_srcdir)/raster/test/regress/rt_iscoveragetile

RASTER_TEST_BUGS = \
	$(top_srcdir)/raster/test/regress/bug_test_car5 \
	$(top_srcdir)/raster/test/regress/permitted_gdal_drivers \
	$(top_srcdir)/raster/test/regress/tickets

RASTER_TEST_LOADER = \
	$(top_srcdir)/raster/test/regress/loader/Basic \
	$(top_srcdir)/raster/test/regress/loader/Projected \
	$(top_srcdir)/raster/test/regress/loader/BasicCopy \
	$(top_srcdir)/raster/test/regress/loader/BasicFilename \
	$(top_srcdir)/raster/test/regress/loader/BasicOutDB \
	$(top_srcdir)/raster/test/regress/loader/Tiled10x10 \
	$(top_srcdir)/raster/test/regress/loader/Tiled10x10Copy \
	$(top_srcdir)/raster/test/regress/loader/Tiled8x8 \
	$(top_srcdir)/raster/test/regress/loader/TiledAuto \
	$(top_srcdir)/raster/test/regress/loader/TiledAutoSkipNoData \
	$(top_srcdir)/raster/test/regress/loader/TiledAutoCopyn

RASTER_TESTS := $(RASTER_TEST_FIRST) \
	$(RASTER_TEST_METADATA) $(RASTER_TEST_IO) $(RASTER_TEST_BASIC_FUNC) \
	$(RASTER_TEST_PROPS) $(RASTER_TEST_BANDPROPS) \
	$(RASTER_TEST_UTILITY) $(RASTER_TEST_MAPALGEBRA) $(RASTER_TEST_SREL) \
	$(RASTER_TEST_BUGS) \
	$(RASTER_TEST_LOADER) \
	$(RASTER_TEST_LAST)

TESTS += $(RASTER_TESTS)
