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

override RUNTESTFLAGS := $(RUNTESTFLAGS) --topology

override RUNTESTFLAGS_INTERNAL := \
  --before-upgrade-script $(top_srcdir)/topology/test/regress/hooks/hook-before-upgrade-topology.sql \
  $(RUNTESTFLAGS_INTERNAL) \
  --after-upgrade-script $(top_srcdir)/topology/test/regress/hooks/hook-after-upgrade-topology.sql

TESTS += \
	$(top_srcdir)/topology/test/regress/addedge.sql \
	$(top_srcdir)/topology/test/regress/addface2.5d.sql \
	$(top_srcdir)/topology/test/regress/addface.sql \
	$(top_srcdir)/topology/test/regress/addnode.sql \
	$(top_srcdir)/topology/test/regress/addtopogeometrycolumn.sql \
	$(top_srcdir)/topology/test/regress/cleartopogeom.sql \
	$(top_srcdir)/topology/test/regress/copytopology.sql \
	$(top_srcdir)/topology/test/regress/createtopogeom.sql \
	$(top_srcdir)/topology/test/regress/createtopology.sql \
	$(top_srcdir)/topology/test/regress/droptopogeometrycolumn.sql \
	$(top_srcdir)/topology/test/regress/droptopology.sql \
	$(top_srcdir)/topology/test/regress/findlayer.sql \
	$(top_srcdir)/topology/test/regress/findtopology.sql \
	$(top_srcdir)/topology/test/regress/geometry_cast.sql \
	$(top_srcdir)/topology/test/regress/getedgebypoint.sql \
	$(top_srcdir)/topology/test/regress/getfacebypoint.sql \
	$(top_srcdir)/topology/test/regress/getfacecontainingpoint.sql \
	$(top_srcdir)/topology/test/regress/getnodebypoint.sql \
	$(top_srcdir)/topology/test/regress/getnodeedges.sql \
	$(top_srcdir)/topology/test/regress/getringedges.sql \
	$(top_srcdir)/topology/test/regress/gettopogeomelements.sql \
	$(top_srcdir)/topology/test/regress/gml.sql \
	$(top_srcdir)/topology/test/regress/layertrigger.sql \
	$(top_srcdir)/topology/test/regress/legacy_invalid.sql \
	$(top_srcdir)/topology/test/regress/legacy_predicate.sql \
	$(top_srcdir)/topology/test/regress/legacy_query.sql \
	$(top_srcdir)/topology/test/regress/legacy_validate.sql \
	$(top_srcdir)/topology/test/regress/polygonize.sql \
	$(top_srcdir)/topology/test/regress/removeunusedprimitives.sql \
	$(top_srcdir)/topology/test/regress/sqlmm.sql \
	$(top_srcdir)/topology/test/regress/st_addedgemodface.sql \
	$(top_srcdir)/topology/test/regress/st_addedgenewfaces.sql \
	$(top_srcdir)/topology/test/regress/st_addisoedge.sql \
	$(top_srcdir)/topology/test/regress/st_addisonode.sql \
	$(top_srcdir)/topology/test/regress/st_changeedgegeom.sql \
	$(top_srcdir)/topology/test/regress/st_createtopogeo.sql \
	$(top_srcdir)/topology/test/regress/st_getfaceedges.sql \
	$(top_srcdir)/topology/test/regress/st_getfacegeometry.sql \
	$(top_srcdir)/topology/test/regress/st_modedgeheal.sql \
	$(top_srcdir)/topology/test/regress/st_modedgesplit.sql \
	$(top_srcdir)/topology/test/regress/st_moveisonode.sql \
	$(top_srcdir)/topology/test/regress/st_newedgeheal.sql \
	$(top_srcdir)/topology/test/regress/st_newedgessplit.sql \
	$(top_srcdir)/topology/test/regress/st_remedgemodface.sql \
	$(top_srcdir)/topology/test/regress/st_remedgenewface.sql \
	$(top_srcdir)/topology/test/regress/st_removeisoedge.sql \
	$(top_srcdir)/topology/test/regress/st_removeisonode.sql \
	$(top_srcdir)/topology/test/regress/st_simplify.sql \
	$(top_srcdir)/topology/test/regress/topo2.5d.sql \
	$(top_srcdir)/topology/test/regress/topoelementarray_agg.sql \
	$(top_srcdir)/topology/test/regress/topoelement.sql \
	$(top_srcdir)/topology/test/regress/topogeo_addlinestring.sql \
	$(top_srcdir)/topology/test/regress/topogeo_addpoint.sql \
	$(top_srcdir)/topology/test/regress/topogeo_addpolygon.sql \
	$(top_srcdir)/topology/test/regress/topogeom_addtopogeom.sql \
	$(top_srcdir)/topology/test/regress/topogeom_edit.sql \
	$(top_srcdir)/topology/test/regress/topogeometry_srid.sql \
	$(top_srcdir)/topology/test/regress/topogeometry_type.sql \
	$(top_srcdir)/topology/test/regress/topojson.sql \
	$(top_srcdir)/topology/test/regress/topologysummary.sql \
	$(top_srcdir)/topology/test/regress/totopogeom.sql \
	$(top_srcdir)/topology/test/regress/validatetopology.sql
