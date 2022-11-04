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


TESTS += \
	$(top_srcdir)/regress/loader/Point \
	$(top_srcdir)/regress/loader/PointM \
	$(top_srcdir)/regress/loader/PointZ \
	$(top_srcdir)/regress/loader/MultiPoint \
	$(top_srcdir)/regress/loader/MultiPointM \
	$(top_srcdir)/regress/loader/MultiPointZ \
	$(top_srcdir)/regress/loader/Arc \
	$(top_srcdir)/regress/loader/ArcM \
	$(top_srcdir)/regress/loader/ArcZ \
	$(top_srcdir)/regress/loader/Polygon \
	$(top_srcdir)/regress/loader/PolygonM \
	$(top_srcdir)/regress/loader/PolygonZ \
	$(top_srcdir)/regress/loader/TSTPolygon \
	$(top_srcdir)/regress/loader/TSIPolygon \
	$(top_srcdir)/regress/loader/TSTIPolygon \
	$(top_srcdir)/regress/loader/PointWithSchema \
	$(top_srcdir)/regress/loader/NoTransPoint \
	$(top_srcdir)/regress/loader/NotReallyMultiPoint \
	$(top_srcdir)/regress/loader/MultiToSinglePoint \
	$(top_srcdir)/regress/loader/ReprojectPts \
	$(top_srcdir)/regress/loader/ReprojectPtsD \
	$(top_srcdir)/regress/loader/ReprojectPtsGeog \
	$(top_srcdir)/regress/loader/ReprojectPtsGeogD \
	$(top_srcdir)/regress/loader/Latin1 \
	$(top_srcdir)/regress/loader/Latin1-implicit \
	$(top_srcdir)/regress/loader/mfile \
	$(top_srcdir)/regress/loader/TestSkipANALYZE \
	$(top_srcdir)/regress/loader/TestANALYZE \
	$(top_srcdir)/regress/loader/CharNoWidth

