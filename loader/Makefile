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

OBJS = shpopen.o dbfopen.o getopt.o

override CPPFLAGS := -g -I$(srcdir) -I$(top_builddir)/src/interfaces/libpq $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"'

all: shp2pgsql pgsql2shp

pgsql2shp: $(OBJS) pgsql2shp.o
	$(CC) $(CFLAGS) $(OBJS) pgsql2shp.o $(libpq) $(LDFLAGS) $(LIBS) -o $@ 

shp2pgsql: $(OBJS) shp2pgsql.o
	$(CC) $(CFLAGS) $(OBJS) shp2pgsql.o $(libpq) $(LDFLAGS) $(LIBS) -o $@ 

install: all
	$(INSTALL_PROGRAM) pgsql2shp $(bindir)
	$(INSTALL_PROGRAM) shp2pgsql $(bindir)

clean:
	@rm -f $(OBJS) shp2pgsql.o pgsql2shp.o shp2pgsql pgsql2shp

