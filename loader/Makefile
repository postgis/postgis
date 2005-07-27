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
		ifneq ($(findstring 7.3,$(VERSION)),)
			USE_VERSION=73
		else
			ifneq ($(findstring 7.4,$(VERSION)),)
				USE_VERSION=74
			else
				ifneq ($(findstring 8.0,$(VERSION)),)
					USE_VERSION=80
				else
					USE_VERSION=81
				endif
			endif
		endif
	endif
endif

#---------------------------------------------------------------

override CFLAGS := -g -I.. -I$(srcdir) -I$(top_builddir)/src/interfaces/libpq $(CFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' -DUSE_VERSION=$(USE_VERSION)

all: shp2pgsql$(EXE) pgsql2shp$(EXE)

pgsql2shp$(EXE): $(OBJS) pgsql2shp.o
	$(CC) $(CFLAGS) $(OBJS) pgsql2shp.o $(libpq) $(LDFLAGS) -o $@ 

shp2pgsql$(EXE): $(OBJS) shp2pgsql.o
	$(CC) $(CFLAGS) $(OBJS) shp2pgsql.o $(LDFLAGS) -o $@ 

install: all
	$(INSTALL_PROGRAM) pgsql2shp$(EXE) $(DESTDIR)$(bindir)/pgsql2shp$(EXE)
	$(INSTALL_PROGRAM) shp2pgsql$(EXE) $(DESTDIR)$(bindir)/shp2pgsql$(EXE)

clean:
	@rm -f $(OBJS) shp2pgsql.o pgsql2shp.o shp2pgsql$(EXE) pgsql2shp$(EXE)

