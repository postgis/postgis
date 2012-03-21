#############################################################################
# $Id$
#
# Copyright (c) 2011 Regents of the University of California
#   <bkpark@ucdavis.edu>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#
#############################################################################

RT_CORE=../rt_core
RT_PG=../rt_pg

all: plpgsql python

plpgsql:
#	$(MAKE) -C plpgsql

python:
	$(MAKE) -C python

install:
#	$(MAKE) -C plpgsql
	$(MAKE) -C python

uninstall:

clean:
#	$(MAKE) -C plpgsql $@
	$(MAKE) -C python $@

distclean: clean
#	$(MAKE) -C plpgsql $@
	$(MAKE) -C python $@
	rm -f Makefile
