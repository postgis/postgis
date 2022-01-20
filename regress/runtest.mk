# **********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * Copyright (C) 2020-2022 Sandro Santilli <strk@kbt.io>
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

abstopsrcdir := $(realpath $(top_srcdir))
abssrcdir := $(realpath .)
abs_top_builddir := $(realpath $(top_builddir))

TESTS := $(patsubst $(topsrcdir)/%,$(abstopsrcdir)/%,$(TESTS))
TESTS := $(patsubst $(abssrcdir)/%,./%,$(TESTS))

.PHONY: check-regress
check-regress:
	@echo "RUNTESTFLAGS: $(RUNTESTFLAGS)"
	@echo "RUNTESTFLAGS_INTERNAL: $(RUNTESTFLAGS_INTERNAL)"

	POSTGIS_TOP_BUILD_DIR=$(abs_top_builddir) $(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(RUNTESTFLAGS_INTERNAL) $(TESTS)

	@if echo "$(RUNTESTFLAGS)" | grep -vq -- --upgrade; then \
		echo "Running upgrade test as RUNTESTFLAGS did not contain that"; \
		POSTGIS_TOP_BUILD_DIR=$(abs_top_builddir) $(PERL) $(topsrcdir)/regress/run_test.pl \
      --upgrade \
      $(RUNTESTFLAGS) \
      $(RUNTESTFLAGS_INTERNAL) \
      $(TESTS); \
	else \
		echo "Skipping upgrade test as RUNTESTFLAGS already requested upgrades"; \
	fi

check-long:
	$(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(TESTS) $(TESTS_SLOW)

