#
# PostGIS Loader Makefile
#
SHELL = /bin/sh

subdir = contrib/postgis/loader
EXE = 

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
	EXE = .exe
endif

# Root of the pgsql source tree 
ifeq (${PGSQL_SRC},) 
	top_builddir = ../../..
	include $(top_builddir)/src/Makefile.global
	libdir := $(libdir)/contrib
else
	top_builddir = ${PGSQL_SRC}
	include $(top_builddir)/src/Makefile.global
	libdir := ${PWD}
endif

OBJS = shpopen.o dbfopen.o getopt.o

#---------------------------------------------------------------
# Test the version string and set the USE_VERSION macro
# appropriately.
#
ifneq ($(findstring 7.1,$(VERSION)),)
	USE_VERSION=71
else
	ifneq ($(findstring 7.2,$(VERSION)),)
                USE_VERSION=72
	else
                USE_VERSION=73
	endif
endif

#---------------------------------------------------------------

override CPPFLAGS := -I$(srcdir) -I$(top_builddir)/src/interfaces/libpq $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' -DUSE_VERSION=$(USE_VERSION)

all: shp2pgsql$(EXE) pgsql2shp$(EXE)

pgsql2shp$(EXE): $(OBJS) pgsql2shp.o
	$(CC) $(CFLAGS) $(OBJS) pgsql2shp.o $(libpq) $(LDFLAGS) $(LIBS) -o $@ 

shp2pgsql$(EXE): $(OBJS) shp2pgsql.o
	$(CC) $(CFLAGS) $(OBJS) shp2pgsql.o $(libpq) $(LDFLAGS) $(LIBS) -o $@ 

install: all
	$(INSTALL_PROGRAM) pgsql2shp$(EXE) $(bindir)/pgsql2shp$(EXE)
	$(INSTALL_PROGRAM) shp2pgsql$(EXE) $(bindir)/shp2pgsql$(EXE)

clean:
	@rm -f $(OBJS) shp2pgsql.o pgsql2shp.o shp2pgsql$(EXE) pgsql2shp$(EXE)

