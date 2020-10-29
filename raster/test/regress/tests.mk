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

RASTER_TEST_FIRST = \
	check_gdal \
	load_outdb

RASTER_TEST_LAST = \
	clean

RASTER_TEST_METADATA = \
	check_raster_columns \
	check_raster_overviews

RASTER_TEST_IO = \
	rt_io

RASTER_TEST_BASIC_FUNC = \
	rt_bytea \
	rt_wkb \
	box3d \
	rt_addband \
	rt_band \
	rt_tile

RASTER_TEST_PROPS = \
	rt_dimensions \
	rt_scale \
	rt_pixelsize \
	rt_upperleft \
	rt_rotation \
	rt_georeference \
	rt_set_properties \
	rt_isempty \
	rt_hasnoband \
	rt_metadata \
	rt_rastertoworldcoord \
	rt_worldtorastercoord \
	rt_convexhull \
	rt_envelope

RASTER_TEST_BANDPROPS = \
	rt_band_properties \
	rt_set_band_properties \
	rt_pixelaspolygons \
	rt_pixelaspoints \
	rt_pixelascentroids \
	rt_setvalues_array \
	rt_summarystats \
	rt_count \
	rt_histogram \
	rt_quantile \
	rt_valuecount \
	rt_valuepercent \
	rt_bandmetadata \
	rt_pixelvalue \
	rt_neighborhood \
	rt_nearestvalue \
	rt_pixelofvalue \
	rt_polygon \
	rt_setbandpath

RASTER_TEST_UTILITY = \
	rt_utility \
	rt_fromgdalraster \
	rt_asgdalraster \
	rt_astiff \
	rt_asjpeg \
	rt_aspng \
	rt_reclass \
	rt_gdalwarp \
	rt_asraster \
	rt_dumpvalues \
	rt_makeemptycoverage \
	rt_createoverview

RASTER_TEST_MAPALGEBRA = \
	rt_mapalgebraexpr \
	rt_mapalgebrafct \
	rt_mapalgebraexpr_2raster \
	rt_mapalgebrafct_2raster \
	rt_mapalgebrafctngb \
	rt_mapalgebrafctngb_userfunc \
	rt_intersection \
	rt_clip \
	rt_mapalgebra \
	rt_mapalgebra_expr \
	rt_mapalgebra_mask \
	rt_union \
	rt_invdistweight4ma \
	rt_4ma \
	rt_setvalues_geomval \
	rt_elevation_functions \
	rt_colormap \
	rt_grayscale

RASTER_TEST_SREL = \
	rt_gist_relationships \
	rt_intersects \
	rt_samealignment \
	rt_geos_relationships \
	rt_iscoveragetile

RASTER_TEST_BUGS = \
	bug_test_car5 \
	permitted_gdal_drivers \
	tickets

RASTER_TEST_LOADER = \
	loader/Basic \
	loader/Projected \
	loader/BasicCopy \
	loader/BasicFilename \
	loader/BasicOutDB \
	loader/Tiled10x10 \
	loader/Tiled10x10Copy \
	loader/Tiled8x8 \
	loader/TiledAuto \
	loader/TiledAutoSkipNoData

RASTER_TESTS := $(RASTER_TEST_FIRST) \
	$(RASTER_TEST_METADATA) $(RASTER_TEST_IO) $(RASTER_TEST_BASIC_FUNC) \
	$(RASTER_TEST_PROPS) $(RASTER_TEST_BANDPROPS) \
	$(RASTER_TEST_UTILITY) $(RASTER_TEST_MAPALGEBRA) $(RASTER_TEST_SREL) \
	$(RASTER_TEST_BUGS) \
	$(RASTER_TEST_LOADER) \
	$(RASTER_TEST_LAST)

TESTS += $(patsubst %, $(topsrcdir)/raster/test/regress/%, $(RASTER_TESTS))
