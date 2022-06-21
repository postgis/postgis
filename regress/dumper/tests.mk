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
	$(top_srcdir)/regress/dumper/mfiledmp \
	$(top_srcdir)/regress/dumper/literalsrid \
	$(top_srcdir)/regress/dumper/realtable \
	$(top_srcdir)/regress/dumper/nullsintable \
	$(top_srcdir)/regress/dumper/null3d \
	$(top_srcdir)/regress/dumper/withclause
