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
  $(topsrcdir)/topology/test/regress/addedge.sql \
	$(topsrcdir)/topology/test/regress/addface2.5d.sql \
  $(topsrcdir)/topology/test/regress/addface.sql \
	$(topsrcdir)/topology/test/regress/addnode.sql \
	$(topsrcdir)/topology/test/regress/addtopogeometrycolumn.sql \
	$(topsrcdir)/topology/test/regress/copytopology.sql \
	$(topsrcdir)/topology/test/regress/createtopogeom.sql \
	$(topsrcdir)/topology/test/regress/createtopology.sql \
	$(topsrcdir)/topology/test/regress/droptopogeometrycolumn.sql \
	$(topsrcdir)/topology/test/regress/droptopology.sql \
	$(topsrcdir)/topology/test/regress/geometry_cast.sql \
	$(topsrcdir)/topology/test/regress/getedgebypoint.sql \
	$(topsrcdir)/topology/test/regress/getfacebypoint.sql \
	$(topsrcdir)/topology/test/regress/getnodebypoint.sql \
	$(topsrcdir)/topology/test/regress/getringedges.sql \
	$(topsrcdir)/topology/test/regress/gettopogeomelements.sql \
	$(topsrcdir)/topology/test/regress/gml.sql \
	$(topsrcdir)/topology/test/regress/layertrigger.sql \
	$(topsrcdir)/topology/test/regress/legacy_invalid.sql \
	$(topsrcdir)/topology/test/regress/legacy_predicate.sql \
	$(topsrcdir)/topology/test/regress/legacy_query.sql \
	$(topsrcdir)/topology/test/regress/legacy_validate.sql \
	$(topsrcdir)/topology/test/regress/polygonize.sql \
	$(topsrcdir)/topology/test/regress/sqlmm.sql \
	$(topsrcdir)/topology/test/regress/st_addedgemodface.sql \
	$(topsrcdir)/topology/test/regress/st_addedgenewfaces.sql \
	$(topsrcdir)/topology/test/regress/st_addisoedge.sql \
	$(topsrcdir)/topology/test/regress/st_addisonode.sql \
	$(topsrcdir)/topology/test/regress/st_changeedgegeom.sql \
	$(topsrcdir)/topology/test/regress/st_createtopogeo.sql \
	$(topsrcdir)/topology/test/regress/st_getfaceedges.sql \
	$(topsrcdir)/topology/test/regress/st_getfacegeometry.sql \
	$(topsrcdir)/topology/test/regress/st_modedgeheal.sql \
	$(topsrcdir)/topology/test/regress/st_modedgesplit.sql \
	$(topsrcdir)/topology/test/regress/st_newedgeheal.sql \
	$(topsrcdir)/topology/test/regress/st_newedgessplit.sql \
	$(topsrcdir)/topology/test/regress/st_remedgemodface.sql \
	$(topsrcdir)/topology/test/regress/st_remedgenewface.sql \
	$(topsrcdir)/topology/test/regress/st_simplify.sql \
	$(topsrcdir)/topology/test/regress/topo2.5d.sql \
	$(topsrcdir)/topology/test/regress/topoelementarray_agg.sql \
	$(topsrcdir)/topology/test/regress/topoelement.sql \
	$(topsrcdir)/topology/test/regress/topogeo_addlinestring.sql \
	$(topsrcdir)/topology/test/regress/topogeo_addpoint.sql \
	$(topsrcdir)/topology/test/regress/topogeo_addpolygon.sql \
  $(topsrcdir)/topology/test/regress/topogeom_edit.sql \
	$(topsrcdir)/topology/test/regress/topogeometry_type.sql \
	$(topsrcdir)/topology/test/regress/topojson.sql \
  $(topsrcdir)/topology/test/regress/topologysummary.sql \
	$(topsrcdir)/topology/test/regress/totopogeom.sql \
	$(topsrcdir)/topology/test/regress/validatetopology.sql

