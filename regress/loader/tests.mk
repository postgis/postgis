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
	$(topsrcdir)/regress/loader/Point \
	$(topsrcdir)/regress/loader/PointM \
	$(topsrcdir)/regress/loader/PointZ \
	$(topsrcdir)/regress/loader/MultiPoint \
	$(topsrcdir)/regress/loader/MultiPointM \
	$(topsrcdir)/regress/loader/MultiPointZ \
	$(topsrcdir)/regress/loader/Arc \
	$(topsrcdir)/regress/loader/ArcM \
	$(topsrcdir)/regress/loader/ArcZ \
	$(topsrcdir)/regress/loader/Polygon \
	$(topsrcdir)/regress/loader/PolygonM \
	$(topsrcdir)/regress/loader/PolygonZ \
	$(topsrcdir)/regress/loader/TSTPolygon \
	$(topsrcdir)/regress/loader/TSIPolygon \
	$(topsrcdir)/regress/loader/TSTIPolygon \
	$(topsrcdir)/regress/loader/PointWithSchema \
	$(topsrcdir)/regress/loader/NoTransPoint \
	$(topsrcdir)/regress/loader/NotReallyMultiPoint \
	$(topsrcdir)/regress/loader/MultiToSinglePoint \
	$(topsrcdir)/regress/loader/ReprojectPts \
	$(topsrcdir)/regress/loader/ReprojectPtsD \
	$(topsrcdir)/regress/loader/ReprojectPtsGeog \
	$(topsrcdir)/regress/loader/ReprojectPtsGeogD \
	$(topsrcdir)/regress/loader/Latin1 \
	$(topsrcdir)/regress/loader/Latin1-implicit \
	$(topsrcdir)/regress/loader/mfile
