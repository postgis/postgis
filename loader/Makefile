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

OBJS = shp2pgsql.o shpopen.o dbfopen.o
PROG = shp2pgsql

all: $(PROG)

$(PROG): $(OBJS)

clean:
	@rm -f $(OBJS) $(PROG)

