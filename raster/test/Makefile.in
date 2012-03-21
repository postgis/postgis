#############################################################################
# $Id$
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#
#############################################################################

RT_CORE=../rt_core

all: check

core-check:
	$(MAKE) -C core check

check: core-check
	$(MAKE) -C regress check

clean:
	$(MAKE) -C core $@
	$(MAKE) -C regress $@

distclean: clean
	$(MAKE) -C core $@
	$(MAKE) -C regress $@
	rm -f Makefile
