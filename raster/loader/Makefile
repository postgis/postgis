#############################################################################
# $Id$
#
# Copyright (c) 2011 Regents of the University of California
#   <bkpark@ucdavis.edu>
# Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#
#############################################################################

# Set CFLAGS after PGXS, otherwise it will get overwritten with the PGXS
# version which is not what we want. 
CC=gcc
top_builddir = ../..
SHELL = /bin/bash
INSTALL = $(SHELL) ../../install-sh
LIBTOOL = $(SHELL) $(top_builddir)/libtool

# Filenames with extension as determined by the OS
RASTER2PGSQL=raster2pgsql

# PostgreSQL executable directory
PGSQL_BINDIR=/usr/lib/postgresql/9.1/bin

RT_CORE=../rt_core

LIBLWGEOM_LDFLAGS=../../liblwgeom/liblwgeom.la
LIBLWGEOM_CFLAGS=-I../../liblwgeom
LIBGDAL_CFLAGS=-I/usr/include/gdal
LIBGDAL_LDFLAGS=-L/usr/lib -lgdal
LIBGDAL_DEPLIBS_LDFLAGS=
PROJ_CFLAGS=
GEOS_CFLAGS=-I/usr/local/include
GEOS_LDFLAGS=-L/usr/local/lib -lgeos_c

RTCORE_CFLAGS=-I$(RT_CORE)
RTCORE_LDFLAGS=$(RT_CORE)/librtcore.a

# GetText includes and libraries
GETTEXT_CFLAGS = 
GETTEXT_LDFLAGS =  

# iconv flags
ICONV_LDFLAGS= -lc
ICONV_CFLAGS=

CFLAGS = \
	-g -O2  -fPIC -DPIC  -Wall -Wmissing-prototypes \
	$(RTCORE_CFLAGS) \
	$(LIBLWGEOM_CFLAGS) \
	$(PROJ_CFLAGS) \
	$(LIBGDAL_CFLAGS) \
	$(GEOS_CFLAGS) \
	$(GETTEXT_CFLAGS) \
	$(ICONV_CFLAGS)

LDFLAGS = \
	 \
	$(LIBLWGEOM_LDFLAGS) \
	$(LIBGDAL_LDFLAGS) \
	$(LIBGDAL_DEPLIBS_LDFLAGS) \
	$(GEOS_LDFLAGS) \
	$(GETTEXT_LDFLAGS) \
	$(ICONV_LDFLAGS) \
	-lm

all: $(RASTER2PGSQL)

raster2pgsql.o: raster2pgsql.c
	$(CC) $(CFLAGS) -c $<

$(RASTER2PGSQL): $(RT_CORE)/librtcore.a raster2pgsql.o
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

installdir:
	@mkdir -p $(DESTDIR)$(PGSQL_BINDIR)

install: installdir
	$(LIBTOOL) --mode=install $(INSTALL) $(RASTER2PGSQL) "$(DESTDIR)$(PGSQL_BINDIR)/$(RASTER2PGSQL)"

uninstall:
	$(LIBTOOL) --mode=uninstall $(RM) "$(DESTDIR)$(PGSQL_BINDIR)/$(RASTER2PGSQL)"

$(RT_CORE)/librtcore.a:
	$(MAKE) -C ../rt_core

clean:
	rm -rf .libs
	rm -f *.o $(RASTER2PGSQL)

distclean: clean
	rm -f Makefile
