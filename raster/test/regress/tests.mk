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
  --before-upgrade-script $(topsrcdir)/raster/test/regress/hooks/hook-before-upgrade-raster.sql \
  $(RUNTESTFLAGS_INTERNAL) \
  --after-upgrade-script $(topsrcdir)/raster/test/regress/hooks/hook-after-upgrade-raster.sql

RASTER_TEST_FIRST = \
	$(topsrcdir)/raster/test/regress/check_gdal \
	$(topsrcdir)/raster/test/regress/load_outdb

RASTER_TEST_LAST = \
	$(topsrcdir)/raster/test/regress/clean

RASTER_TEST_METADATA = \
	$(topsrcdir)/raster/test/regress/check_raster_columns \
	$(topsrcdir)/raster/test/regress/check_raster_overviews

RASTER_TEST_IO = \
	$(topsrcdir)/raster/test/regress/rt_io

RASTER_TEST_BASIC_FUNC = \
	$(topsrcdir)/raster/test/regress/rt_bytea \
	$(topsrcdir)/raster/test/regress/rt_wkb \
	$(topsrcdir)/raster/test/regress/box3d \
	$(topsrcdir)/raster/test/regress/rt_addband \
	$(topsrcdir)/raster/test/regress/rt_band \
	$(topsrcdir)/raster/test/regress/rt_tile

RASTER_TEST_PROPS = \
	$(topsrcdir)/raster/test/regress/rt_dimensions \
	$(topsrcdir)/raster/test/regress/rt_scale \
	$(topsrcdir)/raster/test/regress/rt_pixelsize \
	$(topsrcdir)/raster/test/regress/rt_upperleft \
	$(topsrcdir)/raster/test/regress/rt_rotation \
	$(topsrcdir)/raster/test/regress/rt_georeference \
	$(topsrcdir)/raster/test/regress/rt_set_properties \
	$(topsrcdir)/raster/test/regress/rt_isempty \
	$(topsrcdir)/raster/test/regress/rt_hasnoband \
	$(topsrcdir)/raster/test/regress/rt_metadata \
	$(topsrcdir)/raster/test/regress/rt_rastertoworldcoord \
	$(topsrcdir)/raster/test/regress/rt_worldtorastercoord \
	$(topsrcdir)/raster/test/regress/rt_convexhull \
	$(topsrcdir)/raster/test/regress/rt_envelope

RASTER_TEST_BANDPROPS = \
	$(topsrcdir)/raster/test/regress/rt_band_properties \
	$(topsrcdir)/raster/test/regress/rt_set_band_properties \
	$(topsrcdir)/raster/test/regress/rt_pixelaspolygons \
	$(topsrcdir)/raster/test/regress/rt_pixelaspoints \
	$(topsrcdir)/raster/test/regress/rt_pixelascentroids \
	$(topsrcdir)/raster/test/regress/rt_setvalues_array \
	$(topsrcdir)/raster/test/regress/rt_summarystats \
	$(topsrcdir)/raster/test/regress/rt_count \
	$(topsrcdir)/raster/test/regress/rt_histogram \
	$(topsrcdir)/raster/test/regress/rt_quantile \
	$(topsrcdir)/raster/test/regress/rt_valuecount \
	$(topsrcdir)/raster/test/regress/rt_valuepercent \
	$(topsrcdir)/raster/test/regress/rt_bandmetadata \
	$(topsrcdir)/raster/test/regress/rt_pixelvalue \
	$(topsrcdir)/raster/test/regress/rt_neighborhood \
	$(topsrcdir)/raster/test/regress/rt_nearestvalue \
	$(topsrcdir)/raster/test/regress/rt_pixelofvalue \
	$(topsrcdir)/raster/test/regress/rt_polygon \
	$(topsrcdir)/raster/test/regress/rt_setbandpath

RASTER_TEST_UTILITY = \
	$(topsrcdir)/raster/test/regress/rt_utility \
	$(topsrcdir)/raster/test/regress/rt_fromgdalraster \
	$(topsrcdir)/raster/test/regress/rt_asgdalraster \
	$(topsrcdir)/raster/test/regress/rt_astiff \
	$(topsrcdir)/raster/test/regress/rt_asjpeg \
	$(topsrcdir)/raster/test/regress/rt_aspng \
	$(topsrcdir)/raster/test/regress/rt_reclass \
	$(topsrcdir)/raster/test/regress/rt_gdalwarp \
	$(topsrcdir)/raster/test/regress/rt_gdalcontour \
	$(topsrcdir)/raster/test/regress/rt_asraster \
	$(topsrcdir)/raster/test/regress/rt_dumpvalues \
	$(topsrcdir)/raster/test/regress/rt_makeemptycoverage \
	$(topsrcdir)/raster/test/regress/rt_createoverview

RASTER_TEST_MAPALGEBRA = \
	$(topsrcdir)/raster/test/regress/rt_mapalgebraexpr \
	$(topsrcdir)/raster/test/regress/rt_mapalgebrafct \
	$(topsrcdir)/raster/test/regress/rt_mapalgebraexpr_2raster \
	$(topsrcdir)/raster/test/regress/rt_mapalgebrafct_2raster \
	$(topsrcdir)/raster/test/regress/rt_mapalgebrafctngb \
	$(topsrcdir)/raster/test/regress/rt_mapalgebrafctngb_userfunc \
	$(topsrcdir)/raster/test/regress/rt_intersection \
	$(topsrcdir)/raster/test/regress/rt_clip \
	$(topsrcdir)/raster/test/regress/rt_mapalgebra \
	$(topsrcdir)/raster/test/regress/rt_mapalgebra_expr \
	$(topsrcdir)/raster/test/regress/rt_mapalgebra_mask \
	$(topsrcdir)/raster/test/regress/rt_union \
	$(topsrcdir)/raster/test/regress/rt_invdistweight4ma \
	$(topsrcdir)/raster/test/regress/rt_4ma \
	$(topsrcdir)/raster/test/regress/rt_setvalues_geomval \
	$(topsrcdir)/raster/test/regress/rt_elevation_functions \
	$(topsrcdir)/raster/test/regress/rt_colormap \
	$(topsrcdir)/raster/test/regress/rt_grayscale

RASTER_TEST_SREL = \
	$(topsrcdir)/raster/test/regress/rt_gist_relationships \
	$(topsrcdir)/raster/test/regress/rt_intersects \
	$(topsrcdir)/raster/test/regress/rt_samealignment \
	$(topsrcdir)/raster/test/regress/rt_geos_relationships \
	$(topsrcdir)/raster/test/regress/rt_iscoveragetile

RASTER_TEST_BUGS = \
	$(topsrcdir)/raster/test/regress/bug_test_car5 \
	$(topsrcdir)/raster/test/regress/permitted_gdal_drivers \
	$(topsrcdir)/raster/test/regress/tickets

RASTER_TEST_LOADER = \
	$(topsrcdir)/raster/test/regress/loader/Basic \
	$(topsrcdir)/raster/test/regress/loader/Projected \
	$(topsrcdir)/raster/test/regress/loader/BasicCopy \
	$(topsrcdir)/raster/test/regress/loader/BasicFilename \
	$(topsrcdir)/raster/test/regress/loader/BasicOutDB \
	$(topsrcdir)/raster/test/regress/loader/Tiled10x10 \
	$(topsrcdir)/raster/test/regress/loader/Tiled10x10Copy \
	$(topsrcdir)/raster/test/regress/loader/Tiled8x8 \
	$(topsrcdir)/raster/test/regress/loader/TiledAuto \
	$(topsrcdir)/raster/test/regress/loader/TiledAutoSkipNoData

RASTER_TESTS := $(RASTER_TEST_FIRST) \
	$(RASTER_TEST_METADATA) $(RASTER_TEST_IO) $(RASTER_TEST_BASIC_FUNC) \
	$(RASTER_TEST_PROPS) $(RASTER_TEST_BANDPROPS) \
	$(RASTER_TEST_UTILITY) $(RASTER_TEST_MAPALGEBRA) $(RASTER_TEST_SREL) \
	$(RASTER_TEST_BUGS) \
	$(RASTER_TEST_LOADER) \
	$(RASTER_TEST_LAST)

TESTS += $(RASTER_TESTS)
