# **********************************************************************
# * $Id: Makefile.in
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.refractions.net
# * Copyright 2008 Paul Ramsey, Mark Cave-Ayland
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

CC=gcc
CFLAGS=-g -O2  -Wall -Wmissing-prototypes -I/usr/include  
LDFLAGS = -L/usr/lib -lgeos_c
top_builddir = ../..
SHELL = /bin/bash
LIBTOOL = $(SHELL) $(top_builddir)/libtool

CUNIT_LDFLAGS= -lcunit
CUNIT_CPPFLAGS= -I..

# ADD YOUR NEW TEST FILE HERE (1/1)
OBJS=	\
	cu_algorithm.o \
	cu_buildarea.o \
	cu_clean.o \
	cu_print.o \
	cu_misc.o \
	cu_ptarray.o \
	cu_geodetic.o \
	cu_geos.o \
	cu_tree.o \
	cu_measures.o \
	cu_node.o \
	cu_libgeom.o \
	cu_split.o \
	cu_stringbuffer.o \
	cu_triangulate.o \
	cu_homogenize.o \
	cu_force_sfs.o \
	cu_out_wkt.o \
	cu_out_wkb.o \
	cu_out_gml.o \
	cu_out_kml.o \
	cu_out_geojson.o \
	cu_out_svg.o \
	cu_surface.o \
	cu_out_x3d.o \
	cu_in_geojson.o \
	cu_in_wkb.o \
	cu_in_wkt.o \
	cu_tester.o 

ifeq (sfcgal,sfcgal)
CXXFLAGS += -I/usr/local/include -DHAVE_SFCGAL
LDFLAGS += -L/usr/local/lib -lSFCGAL
OBJS += cu_sfcgal.o
endif

# If we couldn't find the cunit library then display a helpful message
ifeq ($(CUNIT_LDFLAGS),)
all: requirements_not_met_cunit
check: requirements_not_met_cunit
else

# Build the unit tester
all: cu_tester

# Build and run the unit tester
check: cu_tester
	@./cu_tester

endif

# Build the main unit test executable
cu_tester: ../liblwgeom.la $(OBJS)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -o $@ $(OBJS) ../liblwgeom.la $(CUNIT_LDFLAGS)
	#$(CC) -o $@ $(OBJS) ../.libs/liblwgeom.a -lm $(CUNIT_LDFLAGS) $(LDFLAGS)

# Command to build each of the .o files
$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(CUNIT_CPPFLAGS) -c -o $@ $<

# Clean target
clean:
	rm -f $(OBJS)
	rm -f cu_tester

distclean: clean
	rm -f Makefile

# Requirements message
requirements_not_met_cunit:
	@echo
	@echo "WARNING:"
	@echo
	@echo "configure was unable to find CUnit which is required for unit testing."
	@echo "In order to enable unit testing, you must install CUnit and then re-run configure."
	@echo
