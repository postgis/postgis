#
# PostGIS Loader Makefile
#
subdir = contrib/postgis/loader

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

OBJS = shpopen.o dbfopen.o

override CPPFLAGS := -I$(srcdir) -L$(top_builddir)/src/interfaces/libpq -lpq -I$(top_builddir)/src/interfaces/libpq $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"'

all: shp2pgsql pgsql2shp

pgsql2shp: $(OBJS)

shp2pgsql: $(OBJS)

clean:
	@rm -f $(OBJS) shp2pgsql pgsql2shp

