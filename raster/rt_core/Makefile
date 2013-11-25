#############################################################################
# $Id: Makefile.in 9523 2012-03-21 21:36:59Z pramsey $
#
# Copyright (c) 2009 Sandro Santilli <strk@keybit.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#
#############################################################################

AR=ar rs

CC=gcc
LIBLWGEOM_LDFLAGS=../../liblwgeom/.libs/liblwgeom.a
LIBLWGEOM_CFLAGS=-I../../liblwgeom
LIBGDAL_CFLAGS=-I/usr/include/gdal
LIBGDAL_LDFLAGS=-L/usr/lib -lgdal
GEOS_LDFLAGS=-L/usr/local/lib -lgeos_c
PROJ_LDFLAGS= -lproj
LDFLAGS=$(LIBLWGEOM_LDFLAGS) $(LIBGDAL_LDFLAGS) $(PROJ_LDFLAGS) $(GEOS_LDFLAGS)
CFLAGS=-g -O2  -fPIC -DPIC  -Wall -Wmissing-prototypes $(LIBLWGEOM_CFLAGS) $(LIBGDAL_CFLAGS)  -I/usr/local/include

# Standalone RTCORE objects
RT_OBJS=rt_api.o
RT_LIB=librtcore.a
RT_HEADERS=rt_api.h

all: $(RT_LIB)

clean:
	rm -f $(RT_OBJS)
	rm -f $(RT_LIB)

distclean: clean
	rm -f Makefile

librtcore.a: $(RT_OBJS) $(RT_HEADERS)
	$(AR) $(RT_LIB) $(RT_OBJS)
