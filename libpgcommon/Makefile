# **********************************************************************
# * $Id: Makefile.in 
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.refractions.net
# * Copyright 2008 Mark Cave-Ayland
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

CC=gcc
CFLAGS=-g -O2 -I../liblwgeom -I/usr/include/postgresql/9.1/server   -fPIC -DPIC  -Wall -Wmissing-prototypes 
LDFLAGS= 

NUMERICFLAGS= -ffloat-store

YACC=yacc
LEX=:

# Standalone COMMON objects
SA_OBJS = \
	gserialized_gist.o \
	lwgeom_transform.o \
	lwgeom_cache.o \
	lwgeom_pg.o


SA_HEADERS = \
	lwgeom_pg.h \
	lwgeom_transform.h \
	lwgeom_cache.h \
	gserialized_gist.h \
	pgsql_compat.h

all: libpgcommon.a

# nothing to install or uninstall
install uninstall:

libpgcommon.a: $(SA_OBJS) $(SA_HEADERS)
	ar rs libpgcommon.a $(SA_OBJS) 

maintainer-clean: clean

clean: 
	$(MAKE) -C cunit clean
	rm -f $(SA_OBJS) 
	rm -f $(NM_OBJS) 
	rm -f libpgcommon.a 

distclean: clean
	$(MAKE) -C cunit distclean
	rm -f Makefile

check: libpgcommon.a
	$(MAKE) -C cunit check

# Command to build each of the .o files
$(SA_OBJS): %.o: %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

$(SA_OBJS): ../postgis_config.h

