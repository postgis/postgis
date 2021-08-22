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
	$(topsrcdir)/regress/dumper/mfiledmp \
	$(topsrcdir)/regress/dumper/literalsrid \
	$(topsrcdir)/regress/dumper/realtable \
	$(topsrcdir)/regress/dumper/nullsintable \
	$(topsrcdir)/regress/dumper/null3d
