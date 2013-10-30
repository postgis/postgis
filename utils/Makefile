# **********************************************************************
# * $Id$
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.refractions.net
# *
# * Copyright 2008 Mark Cave-Ayland
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

# Separate PGXS-enabled Makefile for documentation installation (it is
# not possible to merge into the main Makefile as has been done for
# the shapefile loader)

MODULE_doc=postgis-2.1
MODULEDIR=contrib/$(MODULE_doc)

# Files to be copied to the contrib/ directory
DATA_built=postgis_restore.pl

# PGXS information
PG_CONFIG = /usr/bin/pg_config 
PGXS := /usr/lib/postgresql/9.1/lib/pgxs/src/makefiles/pgxs.mk
include $(PGXS)

SHELL = /bin/bash
INSTALL = $(SHELL) ../install-sh

SCRIPTS = \
	postgis_restore.pl \
	create_undef.pl \
	svn_repo_revision.pl \
	postgis_proc_upgrade.pl \
	profile_intersects.pl \
	test_estimation.pl \
	test_joinestimation.pl

SRID_MAXIMUM = 999999
SRID_USER_MAXIMUM = 998999

all: postgis_restore.pl
	chmod +x $(SCRIPTS)

postgis_restore.pl: postgis_restore.pl.in
	sed 's,@SRID_MAXIMUM@,$(SRID_MAXIMUM),g;s,@SRID_USER_MAXIMUM@,$(SRID_USER_MAXIMUM),' $< >$@

installdir:
	@mkdir -p "$(DESTDIR)$(datadir)/$(MODULEDIR)"

uninstall: 
	@rm -f "$(DESTDIR)$(datadir)/$(MODULEDIR)/postgis_restore.pl"

install: installdir 
	$(LIBTOOL) --mode=install $(INSTALL) postgis_restore.pl "$(DESTDIR)$(datadir)/$(MODULEDIR)/postgis_restore.pl"

clean:
	rm -f postgis_restore.pl

distclean: clean
	rm -f Makefile

# And there's nothing to check
check:
