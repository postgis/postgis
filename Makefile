# Configuration Directives

#---------------------------------------------------------------
# Set USE_PROJ to 1 for Proj4 reprojection support
#
USE_PROJ=1
PROJ_DIR=/usr/local

#---------------------------------------------------------------
# Set USE_GEOS to 1 for GEOS spatial predicate and operator
# support
#
USE_GEOS=1
GEOS_DIR=/usr/local

#---------------------------------------------------------------
# Set USE_STATS to 1 for new GiST statistics collection support
# Note that this support requires additional columns in 
# GEOMETRY_COLUMNS, so see the list archives for info or
# install a fresh database using postgis.sql
#
USE_STATS=0

#---------------------------------------------------------------
subdir=contrib/postgis

#---------------------------------------------------------------
# Default the root of the PostgreSQL source tree 
# To use a non-standard location set the PGSQL_SRC environment
# variable to the appropriate location.
#
ifeq (${PGSQL_SRC},) 
	top_builddir = ../..
	include $(top_builddir)/src/Makefile.global
	LPATH := $$libdir
else
	top_builddir = ${PGSQL_SRC}
	include $(top_builddir)/src/Makefile.global
	LPATH := ${PWD}
endif

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
# Shared library parameters.
#
NAME=postgis
SO_MAJOR_VERSION=0
SO_MINOR_VERSION=8

#---------------------------------------------------------------

override CFLAGS += -g
override CFLAGS += -I$(srcdir) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
override CFLAGS += -DUSE_VERSION=$(USE_VERSION)

ifeq ($(USE_GEOS),1)
	override CFLAGS += -I$(GEOS_DIR)/include/geos -DUSE_GEOS
endif
ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
endif

override DLLLIBS += $(BE_DLLLIBS) 

override CXXFLAGS := $(CFLAGS)

#---------------------------------------------------------------
# Add index selectivity to C flags
#
ifeq ($(USE_STATS),1)
	override CFLAGS += -DUSE_STATS
endif
 
#---------------------------------------------------------------
# Select proper GiST support C file
#
ifeq ($(USE_VERSION),71) 
	GIST_SUPPORT=71
	GIST_ESTIMATE=
else
	GIST_SUPPORT=72
	GIST_ESTIMATE=postgis_estimate.o
endif

ifeq ($(USE_GEOS),1)
	GEOS_WRAPPER=postgis_geos_wrapper.o
endif

OBJS=postgis_debug.o postgis_ops.o postgis_fn.o postgis_inout.o postgis_proj.o postgis_chip.o postgis_transform.o postgis_gist_$(GIST_SUPPORT).o $(GIST_ESTIMATE) postgis_geos.o $(GEOS_WRAPPER)

#---------------------------------------------------------------
# Add libraries that libpq depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)

SHLIB_LINK = $(filter -L%, $(LDFLAGS)) 
ifeq ($(USE_GEOS),1)
	SHLIB_LINK += -lstdc++ -L$(GEOS_DIR)/lib -lgeos
endif
ifeq ($(USE_PROJ),1)
	SHLIB_LINK += -L$(PROJ_DIR)/lib -lproj
endif
SHLIB_LINK += $(BE_DLLLIBS) 

#---------------------------------------------------------------
# Makefile targets

include $(top_srcdir)/src/Makefile.shlib

postgis_geos_wrapper.o: postgis_geos_wrapper.cpp

all: all-lib $(NAME).sql $(NAME).sql $(NAME)_undef.sql loaderdumper

loaderdumper:
	$(MAKE) -C loader

# Shared library stuff

$(NAME).sql: $(NAME)_sql_common.sql.in $(NAME)_sql_$(USE_VERSION)_end.sql.in $(NAME)_sql_$(USE_VERSION)_start.sql.in 
	cat $(NAME)_sql_$(USE_VERSION)_start.sql.in $(NAME)_sql_common.sql.in $(NAME)_sql_$(USE_VERSION)_end.sql.in | sed -e 's:@MODULE_FILENAME@:$(LPATH)/$(shlib):g;s:@POSTGIS_VERSION@:$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION):g'  > $@ 

$(NAME)_undef.sql: $(NAME).sql create_undef.pl
	perl create_undef.pl $< $(USE_VERSION) > $@ 

install: all installdirs install-lib
	$(INSTALL_DATA) $(srcdir)/README.$(NAME)  $(docdir)/contrib
	$(INSTALL_DATA) $(NAME).sql $(datadir)/contrib
	$(INSTALL_DATA) $(NAME)_undef.sql $(datadir)/contrib
	$(INSTALL_DATA) spatial_ref_sys.sql $(datadir)/contrib
	$(INSTALL_DATA) README.postgis $(datadir)/contrib
	$(MAKE) -C loader install

installdirs:
	$(mkinstalldirs) $(docdir)/contrib $(datadir)/contrib $(libdir)

uninstall: uninstall-lib
	@rm -f $(docdir)/contrib/README.$(NAME) $(datadir)/contrib/$(NAME).sql

clean distclean maintainer-clean: clean-lib
	@rm -f $(OBJS) $(NAME).sql $(NAME)_undef.sql
	$(MAKE) -C loader clean

test: all
	csh regress/regress.csh $(TEST_DB)
