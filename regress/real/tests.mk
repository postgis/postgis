# **********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * Copyright (C) 2021 Regina Obe <lr@pcorp.us>
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

override RUNTESTFLAGS := $(RUNTESTFLAGS) --tiger --extension

RUNTESTFLAGS_INTERNAL += \
  --after-create-script $(topsrcdir)/regress/real/load_data.sql \
	--before-uninstall-script $(topsrcdir)/regress/real/drop_data.sql

TESTS += \
	$(topsrcdir)/regress/real/index_tiger_data \
	$(topsrcdir)/regress/real/gist_presort
